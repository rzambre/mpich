/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CH4_VCI_H_INCLUDED
#define CH4_VCI_H_INCLUDED

#include "ch4_impl.h"

 /** Slow-path function declarations **/
#ifndef MPIDI_CH4_DIRECT_NETMOD
int MPIDI_vci_pool_alloc(int num_vsis, int num_vnis);
#else
int MPIDI_vci_pool_alloc(int num_vnis);
#endif
int MPIDI_vci_pool_free(void);
int MPIDI_vci_alloc(MPIDI_vci_type_t type, MPIDI_vci_resource_t resources,
                    MPIDI_vci_property_t properties, int *vci);
int MPIDI_vci_free(int vci);

/** Fast-path functions **/
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get(MPIR_Comm * comm_ptr, int rank, int tag)
{
    /* Only single  VCI per comm */
    return MPIDI_COMM_VCI(comm_ptr);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_host_sends(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return (tag == MPI_ANY_TAG) ? 0 : (tag >> (MPIDI_COMM_VCI_HASH(comm_ptr).num_tag_bits_for_app
                    + MPIDI_COMM_VCI_HASH(comm_ptr).num_tag_bits_for_vci));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_remote_sends(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return (tag == MPI_ANY_TAG) ? 0 : ((tag >> MPIDI_COMM_VCI_HASH(comm_ptr).num_tag_bits_for_app)
            & ((1 << MPIDI_COMM_VCI_HASH(comm_ptr).num_tag_bits_for_vci) - 1));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_host_recvs(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return (tag == MPI_ANY_TAG) ? 0 : ((tag >> MPIDI_COMM_VCI_HASH(comm_ptr).num_tag_bits_for_app)
            & ((1 << MPIDI_COMM_VCI_HASH(comm_ptr).num_tag_bits_for_vci) - 1));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_remote_recvs(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return (tag == MPI_ANY_TAG) ? 0 : (tag >> (MPIDI_COMM_VCI_HASH(comm_ptr).num_tag_bits_for_app
                    + MPIDI_COMM_VCI_HASH(comm_ptr).num_tag_bits_for_vci));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_with_tid(MPIR_Comm * comm)
{
    int vci, hash;

    MPL_thread_id_t tid;
    MPL_thread_self(&tid);
     
    hash = abs((tid * 2654435761) >> 9) % MPIDI_COMM_VCI_COUNT(comm);
    vci = MPIDI_COMM_MULTI_VCI(comm)[hash];
    
    //printf("Thread %d got VCI %d\n", tid, vci);
    return vci;
}
#endif /* CH4_VCI_H_INCLUDED */
