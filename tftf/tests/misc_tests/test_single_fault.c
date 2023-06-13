/*
 * Copyright (c) 2018-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <arch_helpers.h>
#include <sdei.h>
#include <tftf_lib.h>

#ifdef __aarch64__

uint64_t sdei_event_received;
extern void inject_unrecoverable_ras_error(void);
extern int serror_sdei_event_handler(int ev, uint64_t arg);

int sdei_handler(int ev, uint64_t arg)
{
	sdei_event_received = 1;
	tftf_testcase_printf("SError SDEI event received.\n");

	return 0;
}

test_result_t test_single_fault(void)
{
	int64_t ret;
	const int event_id = 5000;

	/* Register SDEI handler */
	ret = sdei_event_register(event_id, serror_sdei_event_handler, 0,
			SDEI_REGF_RM_PE, read_mpidr_el1());
	if (ret < 0) {
		tftf_testcase_printf("SDEI event register failed: 0x%llx\n",
			ret);
		return TEST_RESULT_FAIL;
	}

	ret = sdei_event_enable(event_id);
	if (ret < 0) {
		tftf_testcase_printf("SDEI event enable failed: 0x%llx\n", ret);
		return TEST_RESULT_FAIL;
	}

	ret = sdei_pe_unmask();
	if (ret < 0) {
		tftf_testcase_printf("SDEI pe unmask failed: 0x%llx\n", ret);
		return TEST_RESULT_FAIL;
	}

	inject_unrecoverable_ras_error();

	return TEST_RESULT_SUCCESS;
}

#else

test_result_t test_single_fault(void)
{
	tftf_testcase_printf("Not supported on AArch32.\n");
	return TEST_RESULT_SKIPPED;
}

#endif
