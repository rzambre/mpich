/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "ch4_vci.h"

#ifndef MPIDI_CH4_DIRECT_NETMOD
static void vci_arm(MPIDI_vci_t * vci, int vni, int vsi);
#else
static void vci_arm(MPIDI_vci_t * vci, int vni);
#endif
static void vci_unarm(MPIDI_vci_t * vci);
static void vci_get_free(int *free_vci);
static void vci_update_next_free();

#undef FUNCNAME
#define FUNCNAME MPIDI_vci_pool_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
#ifndef MPIDI_CH4_DIRECT_NETMOD
int MPIDI_vci_pool_alloc(int num_vsis, int num_vnis)
#else
int MPIDI_vci_pool_alloc(int num_vnis)
#endif
{
    int mpi_errno = MPI_SUCCESS;
    int i, req_i;
    MPIR_Request *temp_req[MPIDI_MAX_REQUEST_CACHE_COUNT];

    /* Statically allocate the maximum possible size of the CH4 VCI pool.
     *      * The root VCI takes 1 NM and 1 SHM VCI. The rest could be VSI-only
     *           * or VNI-only VCIs. It is guaranteed that we have at least 1 VNI and
     *                * at least 1 VSI. */
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_VCI_POOL(max_vcis) = 1 + (num_vnis - 1) + (num_vsis - 1);
#else
    MPIDI_VCI_POOL(max_vcis) = num_vnis;
#endif

    MPIDI_VCI_POOL(vci) = MPL_malloc(MPIDI_VCI_POOL(max_vcis) * sizeof(MPIDI_vci_t), MPL_MEM_OTHER);

    /* Initialize all VCIs in the pool */
    for (i = 0; i < MPIDI_VCI_POOL(max_vcis); i++) {
        MPID_Thread_mutex_create(&MPIDI_VCI(i).lock, &mpi_errno);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        /* A SEND request covers all possible use-cases */
        MPIDI_VCI(i).lw_req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIDI_REQUEST(MPIDI_VCI(i).lw_req, vci) = i;
        MPIR_ERR_CHKANDSTMT(MPIDI_VCI(i).lw_req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        MPIR_cc_set(&MPIDI_VCI(i).lw_req->cc, 0);

        /* Fill up the request cache for this VCI */
        for (req_i = 0; req_i < MPIDI_MAX_REQUEST_CACHE_COUNT; req_i++) {
            MPIDI_VCI(i).request_cache[req_i] = MPIR_Handle_obj_alloc(&MPIR_Request_mem);
        }
        MPIDI_VCI(i).request_cache_count = MPIDI_MAX_REQUEST_CACHE_COUNT;

        MPIDI_VCI(i).ref_count = 0;

        /* For now, one-to-one mapping between VCI and VNI */
        MPIDI_VCI(i).vni = i;
#ifndef MPIDI_CH4_DIRECT_NETMOD
        MPIDI_VCI(i).vsi = i;
#endif
    }

    MPID_Thread_mutex_create(&MPIDI_VCI_POOL(lock), &mpi_errno);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

 /* This is called only during finalize time. So, no thread
  *  * safety required here. */
#undef FUNCNAME
#define FUNCNAME MPIDI_vci_pool_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_vci_pool_free(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i, req_i;

    /* Free the VCIs that have not been freed yet */
    for (i = 0; i < MPIDI_VCI_POOL(max_vcis); i++) {
        for (req_i = 0; req_i < MPIDI_VCI(i).request_cache_count; req_i++) {
            MPIR_Handle_obj_free(&MPIR_Request_mem, MPIDI_VCI(i).request_cache[req_i]);
        }
    }

    MPID_Thread_mutex_destroy(&MPIDI_VCI_POOL(lock), &mpi_errno);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    MPL_free(MPIDI_VCI_POOL(vci));

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
