/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <runtime_services/realm_payload/realm_payload_test.h>

u_register_t realm_version(void)
{
	smc_args args = { RMI_RMM_REQ_VERSION };
	smc_ret_values ret;

	ret = tftf_smc(&args);
	return ret.ret0;
}

u_register_t realm_granule_delegate(u_register_t add)
{
	smc_args args = { 0 };
	smc_ret_values ret;

	args.fid = SMC_RMM_GRANULE_DELEGATE;
	args.arg1 = add;

	ret = tftf_smc(&args);
	return ret.ret0;
}

u_register_t realm_granule_undelegate(u_register_t add)
{
	smc_args args = { 0 };
	smc_ret_values ret;

	args.fid = SMC_RMM_GRANULE_UNDELEGATE;
	args.arg1 = add;

	ret = tftf_smc(&args);
	return ret.ret0;
}

u_register_t realm_create(u_register_t rd_addr, u_register_t realm_params_addr)
{
	smc_args args = { 0 };
	smc_ret_values ret;

	args.fid = SMC_RMM_REALM_CREATE;
	args.arg1 = rd_addr;
	args.arg2 = realm_params_addr;

	ret = tftf_smc(&args);
	return ret.ret0;
}

u_register_t realm_destroy(u_register_t rd_addr)
{
	smc_args args = { 0 };
	smc_ret_values ret;

	args.fid = SMC_RMM_REALM_DESTROY;
	args.arg1 = rd_addr;

	ret = tftf_smc(&args);
	return ret.ret0;
}