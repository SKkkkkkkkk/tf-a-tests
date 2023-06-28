/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/arm/arm_gic.h>
#include <xlat_tables_v2.h>

static const mmap_region_t mmap[] = {
	MAP_REGION_FLAT(TC_DEVICE0_BASE, TC_DEVICE0_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(TC_DEVICE1_BASE, TC_DEVICE1_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(DRAM_BASE, TFTF_BASE - DRAM_BASE,
			MT_MEMORY | MT_RW | MT_NS),
	{0}
};

const mmap_region_t *tftf_platform_get_mmap(void)
{
	return mmap;
}

void plat_arm_gic_init(void)
{
	arm_gic_init(TC_GICC_BASE, TC_GICD_BASE, TC_GICR_BASE);
}
