/*
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <platform_def.h>

	.globl	platform_get_core_pos
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_crash_console_flush

	/*
	 * Return 0 to 7 as logical CPU IDs
	 */
func platform_get_core_pos
	lsr	x1, x0, #MPIDR_AFF1_SHIFT
	and	x1, x1, #MPIDR_AFFLVL_MASK /* core id */
	lsr	x2, x0, #MPIDR_AFF2_SHIFT
	and	x2, x2, #MPIDR_AFFLVL_MASK /* cluster id */

	/* core_id > PLATFORM_CORE_COUNT_PER_CLUSTER */
	mov	x0, #-1
	cmp	x1, #(PLATFORM_CORE_COUNT_PER_CLUSTER - 1)
	b.hi	1f

	/* cluster_id > PLATFORM_CLUSTER_COUNT */
	cmp	x2, #(PLATFORM_CLUSTER_COUNT - 1)
	b.hi	1f

	/* CorePos = CoreId + (ClusterId * cpus per cluster) */
	mov	x3, #PLATFORM_CORE_COUNT_PER_CLUSTER
	mul	x3, x2, x3
	add	x0, x1, x3

1:
	ret
endfunc platform_get_core_pos

	/* ---------------------------------------------
	 * int plat_crash_console_init(void)
	 * Function to initialize the crash console
	 * without a C Runtime to print crash report.
	 * Clobber list : x0 - x4
	 * ---------------------------------------------
	 */
func plat_crash_console_init
	mov_imm	x0, CRASH_CONSOLE_BASE
	mov_imm	x1, CRASH_CONSOLE_CLK_IN_HZ
	mov_imm	x2, CRASH_CONSOLE_BAUDRATE
	b	console_init
endfunc plat_crash_console_init

	/* ---------------------------------------------
	 * int plat_crash_console_putc(int c)
	 * Function to print a character on the crash
	 * console without a C Runtime.
	 * Clobber list : x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_putc
	mov_imm	x1, CRASH_CONSOLE_BASE
	b	console_putc
endfunc plat_crash_console_putc

	/* ---------------------------------------------
	 * int plat_crash_console_flush()
	 * Function to force a write of all buffered
	 * data that hasn't been output.
	 * Out : return -1 on error else return 0.
	 * Clobber list : x0 - x1
	 * ---------------------------------------------
	 */
func plat_crash_console_flush
	mov_imm	x1, CRASH_CONSOLE_BASE
	b	console_flush
endfunc plat_crash_console_flush
