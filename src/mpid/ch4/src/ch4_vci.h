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

#endif /* CH4_VCI_H_INCLUDED */
