/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __SPCI_HELPERS_H__
#define __SPCI_HELPERS_H__


#include <spci_svc.h>
#include "tftf_lib.h"

#define SPM_VM_ID_FIRST                 (1)

#define SPM_VM_GET_COUNT                (0xFF01)
#define SPM_VCPU_GET_COUNT              (0xFF02)
#define SPM_DEBUG_LOG                   (0xBD000000)

/* Hypervisor ID at physical SPCI instance */
#define HYP_ID          (0)

/* By convention, SP IDs (as opposed to VM IDs) have bit 15 set */
#define SP_ID(x)        ((x) | (1 << 15))

typedef unsigned short spci_vm_id_t;
typedef unsigned short spci_vm_count_t;
typedef unsigned short spci_vcpu_count_t;

/* Functions */

static inline spci_vcpu_count_t spm_vcpu_get_count(spci_vm_id_t vm_id)
{
	hvc_args args = {
		.fid = SPM_VCPU_GET_COUNT,
		.arg1 = vm_id
	};

	hvc_ret_values ret = tftf_hvc(&args);

	return ret.ret0;
}

static inline spci_vm_count_t spm_vm_get_count(void)
{
	hvc_args args = {
		.fid = SPM_VM_GET_COUNT
	};

	hvc_ret_values ret = tftf_hvc(&args);

	return ret.ret0;
}

static inline void spm_debug_log(char c)
{
	hvc_args args = {
		.fid = SPM_DEBUG_LOG,
		.arg1 = c
	};

	(void)tftf_hvc(&args);
}

static inline smc_ret_values spci_id_get(void)
{
	smc_args args = {
		.fid = SPCI_ID_GET
	};

	return tftf_smc(&args);
}

static inline smc_ret_values spci_msg_wait(void)
{
	smc_args args = {
		.fid = SPCI_MSG_WAIT
	};

	return tftf_smc(&args);
}

/* Send response through registers using direct messaging */
static inline smc_ret_values spci_msg_send_direct_resp(spci_vm_id_t sender_vm_id,
						spci_vm_id_t target_vm_id,
						uint32_t message)
{
	smc_args args = {
		.fid = SPCI_MSG_SEND_DIRECT_RESP_SMC32,
		.arg1 = ((uint32_t)sender_vm_id << 16) | target_vm_id,
		.arg3 = message
	};

	return tftf_smc(&args);
}

static inline smc_ret_values spci_error(int32_t error_code)
{
	smc_args args = {
		.fid = SPCI_ERROR,
		.arg1 = 0,
		.arg2 = error_code
	};

	return tftf_smc(&args);
}

#endif /* __SPCI_HELPERS_H__ */
