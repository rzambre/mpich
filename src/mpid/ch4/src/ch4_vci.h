/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef CH4_VCI_H_INCLUDED
#define CH4_VCI_H_INCLUDED

#include "ch4_vci_types.h"

void MPIDI_VCI_alloc(MPIDI_VCI_resource resources, MPIDI_VCI_type type, MPIDI_VCI_property properties,  int *ch4_vci);

void MPIDI_VCI_free(int ch4_vci);

MPL_STATIC_INLINE_PREFIX int MPIDI_VCI_get(MPIR_Comm * comm_ptr, int rank, int tag)
{
    int vci;

    return vci;
}

#endif /* CH4I_VCI_H_INCLUDED */
