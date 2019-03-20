/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef CH4_VCI_TYPES_H_INCLUDED
#define CH4_VCI_TYPES_H_INCLUDED

#define MPIDI_ROOT_VCI 0
#define MPIDI_VCI_ALLOC_FAILED -1

#define MPIDI_VCI_FREE__NO 0
#define MPIDI_VCI_FREE__YES 1

#define MPIDI_VCI_POOL(field) MPIDI_CH4_Global.vci_pool.field

/* VCI resources */
typedef enum {
    MPIDI_VCI_RESOURCE__TX = 0x1,       /* Can send */
    MPIDI_VCI_RESOURCE__RX = 0x2,       /* Can receive */
} MPIDI_VCI_resource;

/* VCI types */
typedef enum {
    MPIDI_VCI_TYPE__SHARED = 0x1,       /* VCI can be used by multiple objects */
    MPIDI_VCI_TYPE__EXCLUSIVE = 0x2,    /* VCI can be used by only one object */
} MPIDI_VCI_type;

/* VCI properties */
typedef enum {
    MPIDI_VCI_PROPERTY__TAGGED_ORDERED = 0x1,
    MPIDI_VCI_PROPERTY__TAGGED_UNORDERED = 0x2,
    MPIDI_VCI_PROPERTY__RAR = 0x4,
    MPIDI_VCI_PROPERTY__WAR = 0x8,
    MPIDI_VCI_PROPERTY__RAW = 0x10,
    MPIDI_VCI_PROPERTY__WAW = 0x20,
} MPIDI_VCI_property;

#define MPIDI_VCI_RESOURCE__GENERIC = MPIDI_VCI_RESOURCE__TX | \
                                      MPIDI_VCI_RESOURCE__RX | \
                                      MPIDI_VCI_RESOURCE__NM | \
                                      MPIDI_VCI_RESOURCE__SHM
#define MPIDI_VCI_PROPERTY__GENERIC = MPIDI_VCI_PROPERTY__TAGGED_ORDERED | \
                                      MPIDI_VCI_PROPERTY__TAGGED_UNORDERED | \
                                      MPIDI_VCI_PROPERTY__RAR | \
                                      MPIDI_VCI_PROPERTY__WAR | \
                                      MPIDI_VCI_PROPERTY__RAW | \
                                      MPIDI_VCI_PROPERTY__WAW

typedef struct MPIDI_VCI {
    MPID_Thread_mutex_t lock; /* lock that protects the object in this VCI */
    int free; /* is this VCI free or not */
    int vni; /* which VNI in the netmod pool */
    int vsi; /* which VSI in the shmmod pool */
} MPIDI_VCI_t;

typedef struct MPIDI_VCI_pool {
    MPIDI_VCI_t *vcis;
    int next_free_vci;
    int max_vcis;
    MPID_Thread_mutex_t lock;
} MPIDI_VCI_pool_t;

typedef struct MPIDI_VCI_hash {
    union {
        struct {
            int vci;
        } single;
    } u;
} MPIDI_VCI_hash_t;

#endif /* CH4_VCI_TYPES_H_INCLUDED */
