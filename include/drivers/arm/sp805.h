/*
 * Copyright (c) 2018-2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __SP805_H__
#define __SP805_H__

/* SP805 register offset */
#define SP805_WDOG_LOAD_OFF		0x000
#define SP805_WDOG_VALUE_0FF		0x004
#define SP805_WDOG_CTRL_OFF		0x008
#define SP805_WDOG_INT_CLR_OFF		0x00c
#define SP805_WDOG_RIS_OFF		0x010
#define SP805_WDOG_MIS_OFF		0x014
#define SP805_WDOG_LOCK_OFF		0xc00
#define SP805_WDOG_ITCR_OFF		0xf00
#define SP805_WDOG_ITOP_OFF		0xf04
#define SP805_WDOG_PERIPH_ID_OFF	0xfe0
#define SP805_WDOG_PCELL_ID_OFF		0xff0

/*
 * Magic word to unlock access to all other watchdog registers, Writing any other
 * value locks them.
 */
#define SP805_WDOG_UNLOCK_ACCESS	0x1ACCE551

/* Register field definitions */
#define SP805_WDOG_CTRL_MASK		0x03
#define SP805_WDOG_CTRL_RESEN		(1 << 1)
#define SP805_WDOG_CTRL_INTEN		(1 << 0)
#define SP805_WDOG_RIS_WDOGRIS		(1 << 0)
#define SP805_WDOG_RIS_MASK		0x1
#define SP805_WDOG_MIS_WDOGMIS		(1 << 0)
#define SP805_WDOG_MIS_MASK		0x1
#define SP805_WDOG_ITCR_MASK		0x1
#define SP805_WDOG_ITOP_MASK		0x3
#define SP805_WDOG_PART_NUM_SHIFT	0
#define SP805_WDOG_PART_NUM_MASK	0xfff
#define SP805_WDOG_DESIGNER_ID_SHIFT	12
#define SP805_WDOG_DESIGNER_ID_MASK	0xff
#define SP805_WDOG_REV_SHIFT		20
#define SP805_WDOG_REV_MASK		0xf
#define SP805_WDOG_CFG_SHIFT		24
#define SP805_WDOG_CFG_MASK		0xff
#define SP805_WDOG_PCELL_ID_SHIFT	0
#define SP805_WDOG_PCELL_ID_MASK	0xff

#define ARM_SP805_TWDG_CLK_HZ	32768

/* Public APIs for non-trusted watchdog module. */
void sp805_wdog_start(unsigned int wdog_cycles);
void sp805_wdog_stop(void);
void sp805_wdog_refresh(void);

/* Public APIs for trusted watchdog module. */
void sp805_twdog_start(unsigned int wdog_cycles);
void sp805_twdog_stop(void);
void sp805_twdog_refresh(void);

#endif /* __SP805_H__ */

