/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "ch4_vci.h"

int MPIDI_VCI_pool_get_next_free_vci(int * free_vci)
{
    int mpi_errno;
    int my_next_free_vci, new_next_free_vci;

    mpi_errno = MPI_SUCCESS;
    
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_POOL(lock));
    
    my_next_free_vci = MPIDI_VCI_POOL(next_free_vci);
    new_next_free_vci = my_next_free_vci + 1;

    if (new_next_free_vci == MPIDI_VCI_POOL(max_vcis)) {
        /* Search for a free VCI */
        int index;
        for (index = 0; index < MPIDI_VCI_POOL(max_vcis); index++) {
            if (MPIDI_VCI_POOL(vcis)[index].free == MPIDI_VCI_FREE__YES) {
                new_next_free_vci = index;
                goto found_next_free_vci;
            }
        }
        /* program should never reach here since the VNI or VSI
         * allocation should've failed. If VNI or VSI haven't
         * failed, there must be a free VCI. */
    }

  found_next_free_vci:
    MPIDI_VCI_POOL(next_free_vci) = new_next_free_vci;

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_POOL(lock));
    return mpi_errno;
}

int MPIDI_VCI_init(int vci, int vni, int vsi)
{
    int mpi_errno;
    mpi_errno = MPI_SUCCESS;

    MPIDI_VCI_POOL(vcis)[vci].vni = vni;
    MPIDI_VCI_POOL(vcis)[vci].vsi = vsi;
    MPID_Thread_mutex_create(&MPIDI_VCI_POOL(vcis)[vci].lock, &mpi_errno);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POPFATAL(mpi_errno);
    }
    MPIDI_VCI_POOL(vcis)[vci].free = MPIDI_VCI_FREE__NO;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_VCI_alloc(MPIDI_VCI_resource resources, MPIDI_VCI_type type, MPIDI_VCI_property properties, int * vci)
{
    int mpi_errno = MPI_SUCCESS;
    int my_vci, my_vni, my_vsi;
    MPIDI_VCI_t *my_vci;

    my_vni = MPIDI_VNI_NONE;
    my_vsi = MPIDI_VSI_NONE;

    switch (type) {
        case MPIDI_VCI_TYPE__EXCLUSIVE:
            if (resources & MPIDI_VCI_RESOURCE__NM) {
                /* Allocate a VNI */
                MPIDI_NM_VCI_alloc(MPIDI_VNI_RESOURCE__GENERIC, MPIDI_VNI_PROPERTY__GENERIC, &my_vni);
                if (my_vni == MPIDI_VNI_ALLOC_FAILED) {
                    my_vci = MPIDI_VCI_ALLOC_FAILED;
                    goto fn_exit;
                }
            }
            if (resources & MPIDI_VCI_RESOURCE__SHM) {
                /* Allocate a VSI */
                MPIDI_SHM_VSI_alloc(MPIDI_VSI_RESOURCE__GENERIC, MPIDI_VSI_PROPERTY__GENERIC, &my_vsi);
                if (my_vsi == MPIDI_VSI_ALLOC_FAILED) {
                    my_vci = MPIDI_VCI_ALLOC_FAILED;
                    goto fn_exit;
                }
            }
            /* Get next free VCI */
            mpi_errno = MPIDI_VCI_pool_get_next_free_vci(&my_vci);
            if (mpi_errno != MPI_SUCCESS) {
                MPI_ERR_POP(mpi_errno);
            }
            /* Initialize this VCI */
            mpi_errno = MPIDI_VCI_init(my_vci, my_vni, my_vsi);
            if (mpi_errno != MPI_SUCCESS) {
                MPI_ERR_POP(mpi_errno);
            }
            break;
        case MPIDI_VCI_TYPE__SHARED:
            if (!MPIDI_VCI_POOL(vcis)[MPIDI_ROOT_VCI]) {
                /* allocate the root VCI */
                MPIDI_NM_VCI_alloc(MPIDI_VNI_RESOURCE__GENERIC, MPIDI_VNI_PROPERTY__GENERIC, &my_vni);
                if (my_vni == MPIDI_VNI_ALLOC_FAILED) {
                    my_vci = MPIDI_VCI_ALLOC_FAILED;
                    goto fn_exit;
                }
                MPIDI_SHM_VSI_alloc(MPIDI_VSI_RESOURCE__GENERIC, MPIDI_VSI_PROPERRTY__GENERIC, &my_vsi);
                if (my_vsi = MPIDI_VSI_ALLOC_FAILED) {
                    my_vci = MPIDI_VCI_ALLOC_FAILED;
                    goto fn_exit;
                }
            } else {
                /* return root VCI */
                my_vci = MPIDI_ROOT_VCI;
                goto fn_exit;
            }
            break;
    }

  fn_exit:
    *vci = my_vci;
    return mpi_errno;
  fn_fail:
    my_vci = MPIDI_VCI_ALLOC_FAILED;
    goto fn_exit;
}

int MPIDI_VCI_free(int vci)
{
    int mpi_errno;
    mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_POOL(lock));
    
    MPID_Thread_mutex_destroy(&MPIDI_VCI_POOL(vcis)[vci].lock, &mpi_errno);
    MPIR_Assert(mpi_errno == 0);

    MPIDI_NM_VNI_free(MPIDI_VCI_POOL(vcis)[vci].vni);
    MPIDI_VCI_POOL(vcis)[vci].vni = MPIDI_VNI_NONE;
    
    MPIDI_SHM_VSI_free(MPIDI_VCI_POOL(vcis)[vci].vsi);
    MPIDI_VCI_POOL(vcis)[vci].vsi = MPIDI_VSI_NONE;

    MPIDI_VCI_POOL(vcis)[vci].free = MPIDI_VCI_FREE__YES;
    
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_POOL(lock));
    return mpi_errno;
}
