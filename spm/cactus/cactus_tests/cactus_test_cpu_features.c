/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cactus_message_loop.h"
#include "cactus_test_cmds.h"
#include <fpu.h>
#include "spm_common.h"

/*
 * Note Test must exercise FILL and COMPARE command in
 * sequence and on same CPU.
 */
static fpu_reg_state_t g_fpu_temp;
static unsigned int core_pos;
/*
 * Fill SIMD vectors from secure world side with a unique value.
 */
CACTUS_CMD_HANDLER(req_simd_fill, CACTUS_REQ_SIMD_FILL_CMD)
{
	core_pos = platform_get_core_pos(read_mpidr_el1());
	fpu_state_fill_regs_and_template(&g_fpu_temp);
	return cactus_response(ffa_dir_msg_dest(*args),
			       ffa_dir_msg_source(*args),
			       CACTUS_SUCCESS);
}

/*
 * compare FPU state(SIMD vectors, FPCR, FPSR) from secure world side with the previous
 * SIMD_SECURE_VALUE unique value.
 */
CACTUS_CMD_HANDLER(req_simd_compare, CACTUS_CMP_SIMD_VALUE_CMD)
{
	bool test_succeed = false;

	unsigned int core_pos1 = platform_get_core_pos(read_mpidr_el1());
	if (core_pos1 == core_pos) {
		test_succeed = fpu_state_compare_template(&g_fpu_temp);
	}
	return cactus_response(ffa_dir_msg_dest(*args),
			ffa_dir_msg_source(*args),
			test_succeed ? CACTUS_SUCCESS : CACTUS_ERROR);
}
