/*
 * Copyright (c) 2020-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <plat_topology.h>
#include <tftf_lib.h>

static const struct {
	unsigned int cluster_id;
	unsigned int cpu_id;
} tc_cores[] = {
	/* Cluster0: 8 cores*/
	{ 0, 0 },
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 0, 4 },
	{ 0, 5 },
	{ 0, 6 },
	{ 0, 7 }
};

/*
 * The power domain tree descriptor. The cluster power domains are
 * arranged so that when the PSCI generic code creates the power domain tree,
 * the indices of the CPU power domain nodes it allocates match the linear
 * indices returned by plat_core_pos_by_mpidr().
 */
const unsigned char tc_pd_tree_desc[] = {
	/* Number of root nodes */
	TC_CLUSTER_COUNT,
	/* Number of children for the 1st node */
	TC_MAX_CPUS_PER_CLUSTER,
	/* Number of children for the 2nd node */
	TC_MAX_CPUS_PER_CLUSTER
};

const unsigned char *tftf_plat_get_pwr_domain_tree_desc(void)
{
	return tc_pd_tree_desc;
}

uint64_t tftf_plat_get_mpidr(unsigned int core_pos)
{
	uint64_t mpid;

	assert(core_pos < PLATFORM_CORE_COUNT);

	mpid = (uint64_t)make_mpid(tc_cores[core_pos].cluster_id,
				   tc_cores[core_pos].cpu_id);

	return mpid;
}
