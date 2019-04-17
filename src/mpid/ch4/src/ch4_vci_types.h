/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * (C) 2019 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 *
 */

#ifndef CH4_VCI_TYPES_H_INCLUDED
#define CH4_VCI_TYPES_H_INCLUDED

#include "mpir_request.h"
#include "mpid_thread.h"

#define MPIDI_VCI_ROOT 0
#define MPIDI_VCI_INVALID -1

#define MPIDI_VCI_POOL(field) MPIDI_global.vci_pool.field
#define MPIDI_VCI(i) MPIDI_VCI_POOL(vci)[i]



/* VCI */
typedef struct MPIDI_vci {
    char padding[64];
    MPID_Thread_mutex_t lock;   /* lock to protect the objects in this VCI */
    struct MPIR_Request request_direct[MPIR_REQUEST_PREALLOC]; /* a request pool for this VCI */
    MPIR_Object_alloc_t request_pool; /* a request pool for this VCI */
    MPIR_Request *lw_req;       /* pre-allocated completed request for this VCI */
    int ref_count;              /* number of objects referring to this VCI */
    int is_free;                /* flag to check if this VCI is free or not */
    int vni;                    /* index to the VNI in the netmod's pool */
    int vsi;                    /* index to the VSI in the shmmod's pool */
} MPIDI_vci_t;

/* VCI pool */
typedef struct MPIDI_vci_pool {
    int next_free_vci;
    int max_vcis;
    MPID_Thread_mutex_t lock;   /* lock to protect the VCI pool */
    MPIDI_vci_t *vci;           /* array of VCIs */
} MPIDI_vci_pool_t;

/* VCI hash */
typedef struct MPIDI_vci_hash {
    union {
        struct {
            int vci;
        } single;
        /* TODO: struct multi */
    } u;
} MPIDI_vci_hash_t;

#endif /* CH4_VCI_TYPES_H_INCLUDED */
