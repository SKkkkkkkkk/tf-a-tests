/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <assert.h>
#include <console.h>
#include <gic_v2.h>
#include <platform.h>
#include <platform_def.h>


static const mmap_region_t mmap[] = {
	{0}
};

/* Power Domain Tree Descriptor array */
const unsigned char hikey960_power_domain_tree_desc[] = {
	/* Number of root nodes */
	1,
	/* Number of clusters */
	PLATFORM_CLUSTER_COUNT,
	/* Number of children for the first cluster node */
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	/* Number of children for the second cluster node */
	PLATFORM_CORE_COUNT_PER_CLUSTER,
};


const unsigned char *tftf_plat_get_pwr_domain_tree_desc(void)
{
	return hikey960_power_domain_tree_desc;
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

	mpid = (coreid << MPIDR_AFF0_SHIFT) | (clusterid << MPIDR_AFF1_SHIFT);

	return mpid;
}

void tftf_plat_arch_setup(void)
{
	tftf_plat_configure_mmu();
}

void tftf_early_platform_setup(void)
{
	console_init(CRASH_CONSOLE_BASE, PL011_UART_CLK_IN_HZ,
		PL011_BAUDRATE);
}

void tftf_platform_setup(void)
{
	gicv2_init(GICC_REG_BASE, GICD_REG_BASE);
	gicv2_probe_gic_cpu_id();
	gicv2_setup_cpuif();
}

const mmap_region_t *tftf_platform_get_mmap(void)
{
	return mmap;
}
