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
#include "ofi_events.h"

int MPIDI_OFI_progress(int vci, int blocking)
{
    int mpi_errno, ofi_ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_PROGRESS);

    mpi_errno = MPIDI_OFI_progress_test(vci, &ofi_ret);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_PROGRESS);
    return mpi_errno;
}

int MPIDI_OFI_progress_test(int vci, int *ofi_ret_ptr)
{
    int mpi_errno;
    struct fi_cq_tagged_entry wc[MPIDI_OFI_NUM_CQ_ENTRIES];
    ssize_t ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_PROGRESS_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_PROGRESS_TEST);

    if (unlikely(MPIDI_OFI_get_buffered(wc, 1))) {
        mpi_errno = MPIDI_OFI_handle_cq_entries(wc, 1);
        if (mpi_errno != MPI_SUCCESS)
            ret = 0;
        else
            ret = 1;
    } else if (likely(1)) {
        ret = fi_cq_read(MPIDI_OFI_global.ctx[vci].cq, (void *) wc, MPIDI_OFI_NUM_CQ_ENTRIES);

        if (likely(ret > 0))
            mpi_errno = MPIDI_OFI_handle_cq_entries(wc, ret);
        else if (ret == -FI_EAGAIN)
            mpi_errno = MPI_SUCCESS;
        else
            mpi_errno = MPIDI_OFI_handle_cq_error(vci, ret);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_PROGRESS_TEST);

    *(ofi_ret_ptr) = (int) ret;
    return mpi_errno;
}
