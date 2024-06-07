/*
 * Copyright (c) 2018-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <assert.h>
#include <drivers/console.h>
#include <drivers/arm/arm_gic.h>
#include <platform.h>
#include <platform_def.h>
#include <mmio.h>

#define MAP_DEVICE	MAP_REGION_FLAT( \
DEVICE_START_BASE, \
(DEVICE_END_BASE - DEVICE_START_BASE), \
MT_DEVICE|MT_RW|MT_SECURE)

static const mmap_region_t mmap[] = {
	MAP_REGION_FLAT(SRAM_BASE, SRAM_SIZE, MT_MEMORY | MT_EXECUTE | MT_RW | MT_NS),
	MAP_REGION_FLAT(DRAM_BASE, DRAM_SIZE, MT_MEMORY | MT_RW | MT_NS),	
	MAP_DEVICE,
	{0}
};


/* Power Domain Tree Descriptor array */
const unsigned char plat_power_domain_tree_desc[] = {
	/* Number of clusters */
	PLATFORM_CLUSTER_COUNT,
	/* Number of children for the first cluster node */
	PLATFORM_CORE_COUNT
};


const unsigned char *tftf_plat_get_pwr_domain_tree_desc(void)
{
	return plat_power_domain_tree_desc;
}

/*
 * Generate the MPID from the core position.
 */
uint64_t tftf_plat_get_mpidr(unsigned int core_pos)
{
	uint64_t mpid;
	unsigned int coreid, clusterid;

	assert(core_pos < PLATFORM_CORE_COUNT);

	coreid = core_pos % PLATFORM_CORE_COUNT_PER_CLUSTER;
	clusterid = core_pos / PLATFORM_CORE_COUNT_PER_CLUSTER;

	if (clusterid >= PLATFORM_CLUSTER_COUNT)
		return INVALID_MPID;

	mpid = MPIDR_MT_MASK | (coreid << MPIDR_AFF1_SHIFT) | (clusterid << MPIDR_AFF2_SHIFT);

	return mpid;
}

void tftf_plat_arch_setup(void)
{
	tftf_plat_configure_mmu();
}

void tftf_early_platform_setup(void)
{
	if(console_init(CRASH_CONSOLE_BASE, CRASH_CONSOLE_CLK_IN_HZ,
		CRASH_CONSOLE_BAUDRATE) != 1)
		while(1);
	#define FRACTIONAL_VALUE_DELTA 625U
	uint8_t dlf_value;
	uint64_t fractional_value = ((CRASH_CONSOLE_CLK_IN_HZ*10000)/(16*CRASH_CONSOLE_BAUDRATE))%10000;
	dlf_value = fractional_value/FRACTIONAL_VALUE_DELTA;
	if((fractional_value%FRACTIONAL_VALUE_DELTA) >= (FRACTIONAL_VALUE_DELTA/2))
		++dlf_value;
	mmio_write_32(CRASH_CONSOLE_BASE + 0xc0, dlf_value);
}

void tftf_platform_setup(void)
{
	arm_gic_init(0, GICD_BASE, GICR_BASE);
	arm_gic_setup_global();
	arm_gic_setup_local();
}

const mmap_region_t *tftf_platform_get_mmap(void)
{
	return mmap;
}