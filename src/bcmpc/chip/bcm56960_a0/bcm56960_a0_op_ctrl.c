/*! \file bcm56960_a0_op_ctrl.c
 *
 * This is a temporary sample implementation for demonstration purposes.
 * The operations should be handled by each component driver eventually.
 */
/*
 * Copyright: (c) 2018 Broadcom. All Rights Reserved. "Broadcom" refers to 
 * Broadcom Limited and/or its subsidiaries.
 * 
 * Broadcom Switch Software License
 * 
 * This license governs the use of the accompanying Broadcom software. Your 
 * use of the software indicates your acceptance of the terms and conditions 
 * of this license. If you do not agree to the terms and conditions of this 
 * license, do not use the software.
 * 1. Definitions
 *    "Licensor" means any person or entity that distributes its Work.
 *    "Software" means the original work of authorship made available under 
 *    this license.
 *    "Work" means the Software and any additions to or derivative works of 
 *    the Software that are made available under this license.
 *    The terms "reproduce," "reproduction," "derivative works," and 
 *    "distribution" have the meaning as provided under U.S. copyright law.
 *    Works, including the Software, are "made available" under this license 
 *    by including in or with the Work either (a) a copyright notice 
 *    referencing the applicability of this license to the Work, or (b) a copy 
 *    of this license.
 * 2. Grant of Copyright License
 *    Subject to the terms and conditions of this license, each Licensor 
 *    grants to you a perpetual, worldwide, non-exclusive, and royalty-free 
 *    copyright license to reproduce, prepare derivative works of, publicly 
 *    display, publicly perform, sublicense and distribute its Work and any 
 *    resulting derivative works in any form.
 * 3. Grant of Patent License
 *    Subject to the terms and conditions of this license, each Licensor 
 *    grants to you a perpetual, worldwide, non-exclusive, and royalty-free 
 *    patent license to make, have made, use, offer to sell, sell, import, and 
 *    otherwise transfer its Work, in whole or in part. This patent license 
 *    applies only to the patent claims licensable by Licensor that would be 
 *    infringed by Licensor's Work (or portion thereof) individually and 
 *    excluding any combinations with any other materials or technology.
 *    If you institute patent litigation against any Licensor (including a 
 *    cross-claim or counterclaim in a lawsuit) to enforce any patents that 
 *    you allege are infringed by any Work, then your patent license from such 
 *    Licensor to the Work shall terminate as of the date such litigation is 
 *    filed.
 * 4. Redistribution
 *    You may reproduce or distribute the Work only if (a) you do so under 
 *    this License, (b) you include a complete copy of this License with your 
 *    distribution, and (c) you retain without modification any copyright, 
 *    patent, trademark, or attribution notices that are present in the Work.
 * 5. Derivative Works
 *    You may specify that additional or different terms apply to the use, 
 *    reproduction, and distribution of your derivative works of the Work 
 *    ("Your Terms") only if (a) Your Terms provide that the limitations of 
 *    Section 7 apply to your derivative works, and (b) you identify the 
 *    specific derivative works that are subject to Your Terms. 
 *    Notwithstanding Your Terms, this license (including the redistribution 
 *    requirements in Section 4) will continue to apply to the Work itself.
 * 6. Trademarks
 *    This license does not grant any rights to use any Licensor's or its 
 *    affiliates' names, logos, or trademarks, except as necessary to 
 *    reproduce the notices described in this license.
 * 7. Limitations
 *    Platform. The Work and any derivative works thereof may only be used, or 
 *    intended for use, with a Broadcom switch integrated circuit.
 *    No Reverse Engineering. You will not use the Work to disassemble, 
 *    reverse engineer, decompile, or attempt to ascertain the underlying 
 *    technology of a Broadcom switch integrated circuit.
 * 8. Termination
 *    If you violate any term of this license, then your rights under this 
 *    license (including the license grants of Sections 2 and 3) will 
 *    terminate immediately.
 * 9. Disclaimer of Warranty
 *    THE WORK IS PROVIDED "AS IS" WITHOUT WARRANTIES OR CONDITIONS OF ANY 
 *    KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WARRANTIES OR CONDITIONS OF 
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE OR 
 *    NON-INFRINGEMENT. YOU BEAR THE RISK OF UNDERTAKING ANY ACTIVITIES UNDER 
 *    THIS LICENSE. SOME STATES' CONSUMER LAWS DO NOT ALLOW EXCLUSION OF AN 
 *    IMPLIED WARRANTY, SO THIS DISCLAIMER MAY NOT APPLY TO YOU.
 * 10. Limitation of Liability
 *    EXCEPT AS PROHIBITED BY APPLICABLE LAW, IN NO EVENT AND UNDER NO LEGAL 
 *    THEORY, WHETHER IN TORT (INCLUDING NEGLIGENCE), CONTRACT, OR OTHERWISE 
 *    SHALL ANY LICENSOR BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY DIRECT, 
 *    INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF 
 *    OR RELATED TO THIS LICENSE, THE USE OR INABILITY TO USE THE WORK 
 *    (INCLUDING BUT NOT LIMITED TO LOSS OF GOODWILL, BUSINESS INTERRUPTION, 
 *    LOST PROFITS OR DATA, COMPUTER FAILURE OR MALFUNCTION, OR ANY OTHER 
 *    COMMERCIAL DAMAGES OR LOSSES), EVEN IF THE LICENSOR HAS BEEN ADVISED OF 
 *    THE POSSIBILITY OF SUCH DAMAGES.
 */

#include <bsl/bsl.h>
#include <shr/shr_debug.h>

#include <sal/sal_sleep.h>

#include <bcmdrd/bcmdrd_dev.h>

#include <bcmpc/bcmpc_types.h>
#include <bcmpc/bcmpc_drv.h>
#include <bcmpc/bcmpc_lport.h>

#include <bcmbd/chip/bcm56960_a0_acc.h>

#include "bcm56960_a0_pc_internal.h"

/*******************************************************************************
 * Local definitions
 */

/* Debug log target definition */
#define BSL_LOG_MODULE BSL_LS_BCMPC_DEV

/*! Common operation handler. */
typedef int
(*bcm56960_a0_op_f)(int unit, int port, int param);

/*! This structure is used to define the operation for the each driver. */
typedef struct bcm56960_a0_op_hdl_s {

    /*! Operation name. */
    char *name;

    /*! Operation handler. */
    bcm56960_a0_op_f func;

} bcm56960_a0_op_hdl_t;


/* For manipulating port bitmap memory fields */
#define NUM_PHYS_PORTS 136
#define NUM_LOGIC_PORTS 136
#define PBM_PORT_WORDS ((NUM_PHYS_PORTS / 32) + 1)
#define PBM_LPORT_WORDS ((NUM_LOGIC_PORTS / 32) + 1)
#define PBM_MEMBER(_pbm, _port) \
            ((_pbm)[(_port) >> 5] & LSHIFT32(1, (_port) & 0x1f))
#define PBM_PORT_ADD(_pbm, _port) \
            ((_pbm)[(_port) >> 5] |= LSHIFT32(1, (_port) & 0x1f))
#define PBM_PORT_REMOVE(_pbm, _port) \
            ((_pbm)[(_port) >> 5] &= ~(LSHIFT32(1, (_port) & 0x1f)))


/*******************************************************************************
 * Private utilities
 */

static int
bcm56960_a0_drv_op_exec(int unit, bcm56960_a0_op_hdl_t *drv_hdls, int hdls_cnt,
                        int port, bcmpc_operation_t *op)
{
    int i;
    bcm56960_a0_op_hdl_t *op_hdl;

    SHR_FUNC_ENTER(unit);

    for (i = 0; i < hdls_cnt; i++) {
        op_hdl = &drv_hdls[i];

        if (sal_strcmp(op->name, op_hdl->name) != 0) {
            continue;
        }

        SHR_RETURN_VAL_EXIT
            (op_hdl->func(unit, port, op->param));
    }

    SHR_RETURN_VAL_EXIT(SHR_E_UNAVAIL);

exit:
    SHR_FUNC_EXIT();
}


/*******************************************************************************
 * Ingress/Egress PIPE operations
 */

static int
bcm56960_a0_pipe_fwd_enable(int unit, int lport, int enable)
{
    int ioerr = 0;
    EPC_LINK_BMAPm_t epc_link;
    uint32_t pbm[PBM_LPORT_WORDS];

    ioerr += READ_EPC_LINK_BMAPm(unit, 0, &epc_link);
    EPC_LINK_BMAPm_PORT_BITMAPf_GET(epc_link, pbm);

    if (enable) {
        PBM_PORT_ADD(pbm, lport);
    } else {
        PBM_PORT_REMOVE(pbm, lport);
    }

    EPC_LINK_BMAPm_PORT_BITMAPf_SET(epc_link, pbm);
    ioerr += WRITE_EPC_LINK_BMAPm(unit, 0, epc_link);

    return ioerr ? SHR_E_ACCESS : SHR_E_NONE;
}

static int
bcm56960_a0_pipe_ep_credit_reset(int unit, int lport, int reset)
{
    int ioerr = 0;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;

    ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, lport, &egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, lport, egr_port_credit_reset);
    sal_usleep(1000);
    ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, lport, &egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
    ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, lport, egr_port_credit_reset);

    return ioerr ? SHR_E_ACCESS : SHR_E_NONE;
}

static bcm56960_a0_op_hdl_t bcm56960_a0_pipe_ops[] = {
    { "fwd_enable", bcm56960_a0_pipe_fwd_enable },
    { "ep_credit", bcm56960_a0_pipe_ep_credit_reset },
};

static int
bcm56960_a0_debug_op_exec(int unit, bcmpc_pport_t pport,
                          bcmpc_operation_t *op)
{
    int lport;

    lport = bcmpc_pport_to_lport(unit, pport);
    if (lport == BCMPC_INVALID_LPORT) {
        return SHR_E_PORT;
    }

    return bcm56960_a0_drv_op_exec(unit, bcm56960_a0_pipe_ops,
                                   COUNTOF(bcm56960_a0_pipe_ops), lport, op);
}


/*******************************************************************************
 * Public function
 */

int
bcm56960_a0_pc_op_exec(int unit, bcmpc_pport_t pport, bcmpc_operation_t *op)
{
    SHR_FUNC_ENTER(unit);

    if (sal_strcmp(op->drv, "debug") == 0) {
        SHR_IF_ERR_EXIT
            (bcm56960_a0_debug_op_exec(unit, pport, op));
    } else {
        SHR_RETURN_VAL_EXIT(SHR_E_UNAVAIL);
    }

exit:
    if (SHR_FUNC_VAL_IS(SHR_E_UNAVAIL)) {
        LOG_ERROR(BSL_LOG_MODULE,
            (BSL_META_U(unit, "Operation not supported: drv[%s] op[%s]\n"),
             op->drv, op->name));
    }

    SHR_FUNC_EXIT();
}
