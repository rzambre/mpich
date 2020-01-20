/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "mpidu_bc.h"
#include "ofi_noinline.h"

int MPIDI_OFI_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_COMM_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_COMM_CREATE_HOOK);

    MPIDIU_map_create(&MPIDI_OFI_COMM(comm).huge_send_counters, MPL_MEM_COMM);
    MPIDIU_map_create(&MPIDI_OFI_COMM(comm).huge_recv_counters, MPL_MEM_COMM);

    mpi_errno = MPIDIG_init_comm(comm);

    /* no connection for non-dynamic or non-root-rank of intercomm */
    MPIDI_OFI_COMM(comm).conn_id = -1;

    /* eagain defaults to off */
    if (comm->hints[MPIR_COMM_HINT_EAGAIN] == 0) {
        comm->hints[MPIR_COMM_HINT_EAGAIN] = FALSE;
    }

    /* if this is MPI_COMM_WORLD, finish bc exchange */
    if (comm == MPIR_Process.comm_world) {
        if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
            void *table;
            fi_addr_t *mapped_table;
            size_t rem_bcs, num_nodes = MPIDI_global.max_node_id + 1;
            int i, curr, node;

            MPIR_Assert(MPII_Comm_is_node_consecutive(comm));
            rem_bcs = MPIR_Comm_size(comm) - num_nodes;
            MPIDU_bc_allgather(comm, MPIDI_global.node_map[0], &MPIDI_OFI_global.addrname,
                               MPIDI_OFI_global.addrnamelen, TRUE, &table, NULL);
            MPIR_CHKLMEM_MALLOC(mapped_table, fi_addr_t *, rem_bcs * sizeof(fi_addr_t),
                                mpi_errno, "mapped_table", MPL_MEM_ADDRESS);
            MPIDI_OFI_CALL(fi_av_insert
                           (MPIDI_OFI_global.av, table, rem_bcs, mapped_table, 0ULL, NULL), avmap);

            /* insert new addresses, skipping over node roots */
            for (i = 1, curr = 0, node = 0; i < MPIR_Comm_size(comm); i++) {
                if (comm->internode_table[i] == node + 1) {
                    node++;
                    continue;
                }
                MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest[0][0 /*WRONG*/] = mapped_table[curr];
#if MPIDI_OFI_ENABLE_RUNTIME_CHECKS
                MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).ep_idx = 0;
#else
#if MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS
                MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).ep_idx = 0;
#endif
#endif
                curr++;
            }
            MPIDU_bc_table_destroy(table);
        } else {
            /* Exchange the addresses of the rest of the VNIs using the ROOT VNI */
            int my_rank = MPIR_Comm_rank(comm);
            int world_size = MPIR_Comm_size(comm);
            int num_vnis = MPIDI_OFI_VNI_POOL(max_vnis);

            /* get addr name length */
            size_t name_len = 0;
            int ret = fi_getname((fid_t) MPIDI_OFI_VNI(0).ep, NULL, &name_len);
            MPIR_Assert(ret == -FI_ETOOSMALL);
            MPIR_Assert(name_len > 0);

            int my_len = num_vnis * name_len;
            char *all_names = MPL_malloc(world_size * my_len, MPL_MEM_ADDRESS);
            MPIR_Assert(all_names);

            char *my_names = all_names + my_rank*my_len;

            /* put in my addrnames */
            for (int i = 0; i < num_vnis; i++) {
                size_t actual_name_len = name_len;
                char *vni_addrname = my_names + i * name_len;
                MPIDI_OFI_CALL(fi_getname((fid_t) MPIDI_OFI_VNI(i).ep, vni_addrname,
                            &actual_name_len), getname);
                MPIR_Assert(actual_name_len == name_len);
            }
            /* Allgather */
            MPIR_Comm *comm = MPIR_Process.comm_world;
            MPIR_Errflag_t errflag = MPIR_ERR_NONE;
            mpi_errno = MPIR_Allgather_intra_auto(MPI_IN_PLACE, 0, MPI_BYTE,
                    all_names, my_len, MPI_BYTE, comm, &errflag);

            /* insert the addresses */
            fi_addr_t *mapped_table;
            mapped_table = (fi_addr_t *) MPL_malloc(world_size * num_vnis * sizeof(fi_addr_t), MPL_MEM_ADDRESS);
            for (int vni_local = 0; vni_local < num_vnis; vni_local++) {
                MPIDI_OFI_CALL(fi_av_insert(MPIDI_OFI_VNI(vni_local).av, all_names, world_size * num_vnis,
                            mapped_table, 0ULL, NULL), avmap);
                for (int r = 0; r < world_size; r++) {
                    MPIDI_OFI_addr_t *av = &MPIDI_OFI_AV(&MPIDIU_get_av(0, r));
                    for (int vni_remote = 0; vni_remote < num_vnis; vni_remote++) {
                        if (vni_local == 0 && vni_remote == 0) {
                            continue;
                        }
                        int idx = r * num_vnis + vni_remote;
                        MPIR_Assert(mapped_table[idx] != FI_ADDR_NOTAVAIL);
                        av->dest[vni_local][vni_remote] = mapped_table[idx];
                    }
                }
            }

            MPL_free(all_names);
            MPL_free(mapped_table);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_COMM_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_COMM_FREE_HOOK);

    mpi_errno = MPIDIG_destroy_comm(comm);
    MPIDIU_map_destroy(MPIDI_OFI_COMM(comm).huge_send_counters);
    MPIDIU_map_destroy(MPIDI_OFI_COMM(comm).huge_recv_counters);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_COMM_FREE_HOOK);
    return mpi_errno;
}
