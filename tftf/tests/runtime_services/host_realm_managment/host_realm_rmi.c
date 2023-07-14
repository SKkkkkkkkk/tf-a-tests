/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <string.h>

#include <debug.h>
#include <heap/page_alloc.h>
#include <test_helpers.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_rmi.h>
#include <host_shared_data.h>
#include <plat/common/platform.h>
#include <realm_def.h>
#include <tftf_lib.h>

#define SET_ARG(_n) {			\
	case _n:			\
	regs[_n] = rand64();		\
	CONC(args->arg, _n) = regs[_n];	\
	__attribute__((fallthrough));	\
}

#define	CHECK_RET(_n) {					\
	if (CONC(ret_val.ret, _n) != regs[_n]) {	\
		cmp_flag |= (1U << _n);			\
	}						\
}

static bool rmi_cmp_result;
static unsigned short vmid;

static smc_ret_values host_rmi_handler(smc_args *args, unsigned int in_reg)
{
	u_register_t regs[8];
	smc_ret_values ret_val;
	unsigned int cmp_flag = 0U;

	assert(args != NULL);
	assert((in_reg >= 1U) && (in_reg <= 7U));

	/* Function identifier */
	regs[0] = (u_register_t)args->fid;

	/* X4 and X5 can be passed as parameters */
	regs[4] = args->arg4;
	regs[5] = args->arg5;

	/* SMC calls arguments in X1-X7 */
	switch (in_reg) {
		SET_ARG(1);
		SET_ARG(2);
		SET_ARG(3);
		SET_ARG(4);
		SET_ARG(5);
		SET_ARG(6);
	default:
		regs[7] = rand();
		args->arg7 = regs[7];
	}

	ret_val = tftf_smc(args);

	/*
	 * According to SMCCC v1.2 X4-X7 registers' values
	 * must be preserved unless they contain result,
	 * as specified in the function definition.
	 */
	if (regs[0] != RMI_RTT_READ_ENTRY) {
		CHECK_RET(4);
	}

	CHECK_RET(5);
	CHECK_RET(6);
	CHECK_RET(7);

	if (cmp_flag != 0U) {
		rmi_cmp_result = false;

		ERROR("RMI SMC 0x%lx corrupted registers: %s %s %s %s\n",
			regs[0],
			(((cmp_flag & (1U << 4)) != 0U) ? "X4" : ""),
			(((cmp_flag & (1U << 5)) != 0U) ? "X5" : ""),
			(((cmp_flag & (1U << 6)) != 0U) ? "X6" : ""),
			(((cmp_flag & (1U << 7)) != 0U) ? "X7" : ""));
	}

	return ret_val;
}

void host_rmi_init_cmp_result(void)
{
	rmi_cmp_result = true;
}

bool host_rmi_get_cmp_result(void)
{
	return rmi_cmp_result;
}

static inline u_register_t host_rmi_data_create(bool unknown,
						u_register_t rd,
						u_register_t data,
						u_register_t map_addr,
						u_register_t src)
{
	if (unknown) {
		return host_rmi_handler(&(smc_args) {RMI_DATA_CREATE_UNKNOWN,
					rd, data, map_addr}, 4U).ret0;
	} else {
		return host_rmi_handler(&(smc_args) {RMI_DATA_CREATE,
					/* X5 = flags */
					rd, data, map_addr, src, 0UL}, 6U).ret0;
	}
}

static inline u_register_t host_rmi_realm_activate(u_register_t rd)
{
	return host_rmi_handler(&(smc_args) {RMI_REALM_ACTIVATE, rd}, 2U).ret0;
}

u_register_t host_rmi_realm_create(u_register_t rd, u_register_t params_ptr)
{
	return host_rmi_handler(&(smc_args) {RMI_REALM_CREATE, rd, params_ptr},
						3U).ret0;
}

u_register_t host_rmi_realm_destroy(u_register_t rd)
{
	return host_rmi_handler(&(smc_args) {RMI_REALM_DESTROY, rd}, 2U).ret0;
}

static inline u_register_t host_rmi_data_destroy(u_register_t rd,
						 u_register_t map_addr,
						 u_register_t *data,
						 u_register_t *top)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {RMI_DATA_DESTROY, rd, map_addr,
						(u_register_t)&rets}, 4U);

	*data = rets.ret1;
	*top = rets.ret2;
	return rets.ret0;
}

static inline u_register_t host_rmi_rec_create(u_register_t rd,
						u_register_t rec,
						u_register_t params_ptr)
{
	return host_rmi_handler(&(smc_args) {RMI_REC_CREATE,
				rd, rec, params_ptr}, 4U).ret0;
}

static inline u_register_t host_rmi_rec_destroy(u_register_t rec)
{
	return host_rmi_handler(&(smc_args) {RMI_REC_DESTROY, rec}, 2U).ret0;
}

static inline u_register_t host_rmi_rtt_create(u_register_t rd,
						u_register_t rtt,
						u_register_t map_addr,
						u_register_t level)
{
	return host_rmi_handler(&(smc_args) {RMI_RTT_CREATE,
				rd, rtt, map_addr, level}, 5U).ret0;
}

static inline u_register_t host_rmi_rtt_destroy(u_register_t rd,
						u_register_t map_addr,
						u_register_t level,
						u_register_t *rtt,
						u_register_t *top)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {RMI_RTT_DESTROY,
				rd, map_addr, level, (u_register_t)&rets}, 5U);
	*rtt = rets.ret1;
	*top = rets.ret2;
	return rets.ret0;
}

u_register_t host_rmi_features(u_register_t index, u_register_t *features)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {RMI_FEATURES, index}, 2U);
	*features = rets.ret1;
	return rets.ret0;
}

static inline u_register_t host_rmi_rtt_init_ripas(u_register_t rd,
						   u_register_t start,
						   u_register_t end,
						   u_register_t *top)

{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {RMI_RTT_INIT_RIPAS,
				rd, start, end}, 4U);
	*top = rets.ret1;
	return rets.ret0;
}

static inline u_register_t host_rmi_rtt_fold(u_register_t rd,
					     u_register_t map_addr,
					     u_register_t level,
					     u_register_t *pa)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {RMI_RTT_FOLD,
				rd, map_addr, level, (u_register_t)&rets}, 5U);
	*pa = rets.ret1;
	return rets.ret0;
}

static inline u_register_t host_rmi_rec_aux_count(u_register_t rd,
						  u_register_t *aux_count)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {RMI_REC_AUX_COUNT, rd}, 2U);
	*aux_count = rets.ret1;
	return rets.ret0;
}

static inline u_register_t host_rmi_rtt_set_ripas(u_register_t rd,
						  u_register_t rec,
						  u_register_t start,
						  u_register_t end,
						  u_register_t *top)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {RMI_RTT_SET_RIPAS,
				rd, rec, start, end}, 5U);
	*top = rets.ret1;
	return rets.ret0;
}

static inline u_register_t host_rmi_rtt_mapunprotected(u_register_t rd,
							u_register_t map_addr,
							u_register_t level,
							u_register_t ns_pa)
{
	return host_rmi_handler(&(smc_args) {RMI_RTT_MAP_UNPROTECTED,
				rd, map_addr, level, ns_pa}, 5U).ret0;
}

static u_register_t host_rmi_rtt_readentry(u_register_t rd,
					   u_register_t map_addr,
					   u_register_t level,
					   struct rtt_entry *rtt)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {RMI_RTT_READ_ENTRY,
					rd, map_addr, level}, 4U);
	rtt->walk_level = rets.ret1;
	rtt->state = rets.ret2;
	rtt->out_addr = rets.ret3;
	rtt->ripas = rets.ret4;
	return rets.ret0;
}

static inline u_register_t host_rmi_rtt_unmap_unprotected(u_register_t rd,
							  u_register_t map_addr,
							  u_register_t level,
							  u_register_t *top)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {RMI_RTT_UNMAP_UNPROTECTED,
					rd, map_addr, level}, 4U);
	*top = rets.ret1;
	return rets.ret0;
}

static inline u_register_t host_rtt_level_mapsize(u_register_t level)
{
	if (level > RTT_MAX_LEVEL) {
		return PAGE_SIZE;
	}

	return (1UL << RTT_LEVEL_SHIFT(level));
}

static inline u_register_t host_realm_rtt_create(struct realm *realm,
						 u_register_t addr,
						 u_register_t level,
						 u_register_t phys)
{
	addr = ALIGN_DOWN(addr, host_rtt_level_mapsize(level - 1U));
	return host_rmi_rtt_create(realm->rd, phys, addr, level);
}

static u_register_t host_rmi_create_rtt_levels(struct realm *realm,
						u_register_t map_addr,
						u_register_t level,
						u_register_t max_level)
{
	u_register_t rtt, ret;

	while (level++ < max_level) {
		rtt = (u_register_t)page_alloc(PAGE_SIZE);
		if (rtt == HEAP_NULL_PTR) {
			ERROR("Failed to allocate memory for rtt\n");
			return REALM_ERROR;
		} else {
			ret = host_rmi_granule_delegate(rtt);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, rtt=0x%lx ret=0x%lx\n",
					"host_rmi_granule_delegate", rtt, ret);
				return REALM_ERROR;
			}
		}
		ret = host_realm_rtt_create(realm, map_addr, level, rtt);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, rtt=0x%lx ret=0x%lx\n",
				"host_realm_rtt_create", rtt, ret);
			host_rmi_granule_undelegate(rtt);
			page_free(rtt);
			return REALM_ERROR;
		}
	}

	return REALM_SUCCESS;
}

static u_register_t host_realm_fold_rtt(u_register_t rd, u_register_t addr,
					u_register_t level)
{
	struct rtt_entry rtt;
	u_register_t pa, ret;

	ret = host_rmi_rtt_readentry(rd, addr, level, &rtt);

	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, level=0x%lx addr=0x%lx ret=0x%lx\n",
			"host_rmi_rtt_readentry", level, addr, ret);
		return REALM_ERROR;
	}

	if (rtt.state != RMI_TABLE) {
		ERROR("%s() failed, rtt.state=%lu\n", "rmi_rtt_readentry",
			rtt.state);
		return REALM_ERROR;
	}

	ret = host_rmi_rtt_fold(rd, addr, level + 1U, &pa);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
			"host_rmi_rtt_fold", addr, ret);
		return REALM_ERROR;
	}

	page_free(rtt.out_addr);

	return REALM_SUCCESS;

}

static u_register_t host_realm_map_protected_data(bool unknown,
						  struct realm *realm,
						  u_register_t target_pa,
						  u_register_t map_size,
						  u_register_t src_pa)
{
	u_register_t rd = realm->rd;
	u_register_t map_level, level;
	u_register_t ret = 0UL;
	u_register_t size;
	u_register_t phys = target_pa;
	u_register_t map_addr = target_pa;
	u_register_t end_addr = map_addr + map_size;
	u_register_t top;

	if (!IS_ALIGNED(map_addr, map_size)) {
		return REALM_ERROR;
	}

	switch (map_size) {
	case PAGE_SIZE:
		map_level = 3UL;
		break;
	case RTT_L2_BLOCK_SIZE:
		map_level = 2UL;
		break;
	default:
		ERROR("Unknown map_size=0x%lx\n", map_size);
		return REALM_ERROR;
	}

	ret = host_rmi_rtt_init_ripas(rd, map_addr, end_addr, &top);
	if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT) {
		ret = host_rmi_create_rtt_levels(realm, map_addr,
						 RMI_RETURN_INDEX(ret),
						 map_level);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, ret=0x%lx line=%u\n",
				"host_rmi_create_rtt_levels", ret, __LINE__);
			goto err;
		}
		ret = host_rmi_rtt_init_ripas(rd, map_addr, end_addr, &top);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, ret=0x%lx line=%u\n",
				"host_rmi_rtt_init_ripas", ret, __LINE__);
			goto err;
		}
	}
	for (size = 0UL; size < map_size; size += PAGE_SIZE) {
		ret = host_rmi_granule_delegate(phys);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, PA=0x%lx ret=0x%lx\n",
				"host_rmi_granule_delegate", phys, ret);
			return REALM_ERROR;
		}

		ret = host_rmi_data_create(unknown, rd, phys, map_addr, src_pa);

		if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT) {
			/* Create missing RTTs and retry */
			level = RMI_RETURN_INDEX(ret);
			ret = host_rmi_create_rtt_levels(realm, map_addr, level,
							 map_level);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, ret=0x%lx line=%u\n",
					"host_rmi_create_rtt_levels",
					ret, __LINE__);
				goto err;
			}

			ret = host_rmi_data_create(unknown, rd, phys, map_addr,
						   src_pa);
		}

		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, ret=0x%lx\n",
				"host_rmi_data_create", ret);
			goto err;
		}

		phys += PAGE_SIZE;
		src_pa += PAGE_SIZE;
		map_addr += PAGE_SIZE;
	}

	if (map_size == RTT_L2_BLOCK_SIZE) {
		ret = host_realm_fold_rtt(rd, target_pa, map_level);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, ret=0x%lx\n",
				"host_realm_fold_rtt", ret);
			goto err;
		}
	}

	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx\n", __func__, ret);
		goto err;
	}

	return REALM_SUCCESS;

err:
	while (size >= PAGE_SIZE) {
		u_register_t data, top;

		ret = host_rmi_data_destroy(rd, map_addr, &data, &top);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
				"host_rmi_data_destroy", map_addr, ret);
		}

		ret = host_rmi_granule_undelegate(phys);
		if (ret != RMI_SUCCESS) {
			/* Page can't be returned to NS world so is lost */
			ERROR("%s() failed, ret=0x%lx\n",
				"host_rmi_granule_undelegate", ret);
		}
		phys -= PAGE_SIZE;
		size -= PAGE_SIZE;
		map_addr -= PAGE_SIZE;
	}

	return REALM_ERROR;
}

u_register_t host_realm_map_unprotected(struct realm *realm,
					u_register_t ns_pa,
					u_register_t map_size)
{
	u_register_t rd = realm->rd;
	u_register_t map_level, level;
	u_register_t ret = 0UL;
	u_register_t phys = ns_pa;
	u_register_t map_addr = ns_pa |
			(1UL << (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
			realm->rmm_feat_reg0) - 1UL)) ;

	if (!IS_ALIGNED(map_addr, map_size)) {
		return REALM_ERROR;
	}

	switch (map_size) {
	case PAGE_SIZE:
		map_level = 3UL;
		break;
	case RTT_L2_BLOCK_SIZE:
		map_level = 2UL;
		break;
	default:
		ERROR("Unknown map_size=0x%lx\n", map_size);
		return REALM_ERROR;
	}

	u_register_t desc = phys | S2TTE_ATTR_FWB_WB_RW;

	ret = host_rmi_rtt_mapunprotected(rd, map_addr, map_level, desc);

	if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT) {
		/* Create missing RTTs and retry */
		level = RMI_RETURN_INDEX(ret);
		ret = host_rmi_create_rtt_levels(realm, map_addr, level,
						 map_level);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, ret=0x%lx line=%u\n",
				"host_rmi_create_rtt_levels", ret, __LINE__);
			return REALM_ERROR;
		}

		ret = host_rmi_rtt_mapunprotected(rd, map_addr, map_level,
						  desc);
	}
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx\n", "host_rmi_rtt_mapunprotected",
			ret);
		return REALM_ERROR;
	}

	return REALM_SUCCESS;
}

static u_register_t host_realm_rtt_destroy(struct realm *realm,
					   u_register_t addr,
					   u_register_t level,
					   u_register_t *rtt,
					   u_register_t *top)
{
	addr = ALIGN_DOWN(addr, host_rtt_level_mapsize(level - 1U));
	return host_rmi_rtt_destroy(realm->rd, addr, level, rtt, top);
}

static u_register_t host_realm_destroy_free_rtt(struct realm *realm,
						u_register_t addr,
						u_register_t level,
						u_register_t rtt_granule)
{
	u_register_t rtt, top, ret;

	ret = host_realm_rtt_destroy(realm, addr, level, &rtt, &top);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx\n",
			"host_realm_rtt_destroy", ret);
		return REALM_ERROR;
	}

	ret = host_rmi_granule_undelegate(rtt_granule);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rtt=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", rtt_granule, ret);
		return REALM_ERROR;
	}

	page_free(rtt_granule);
	return REALM_SUCCESS;
}

static u_register_t host_realm_destroy_undelegate_range(struct realm *realm,
							u_register_t ipa,
							u_register_t addr,
							u_register_t size)
{
	u_register_t rd = realm->rd;
	u_register_t ret;
	u_register_t data, top;

	while (size >= PAGE_SIZE) {
		ret = host_rmi_data_destroy(rd, ipa, &data, &top);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
				"host_rmi_data_destroy", ipa, ret);
			return REALM_ERROR;
		}

		ret = host_rmi_granule_undelegate(addr);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
				"host_rmi_granule_undelegate", ipa, ret);
			return REALM_ERROR;
		}

		page_free(addr);

		addr += PAGE_SIZE;
		ipa += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	return REALM_SUCCESS;
}

static u_register_t host_realm_tear_down_rtt_range(struct realm *realm,
						   u_register_t level,
						   u_register_t start,
						   u_register_t end)
{
	u_register_t rd = realm->rd;
	u_register_t map_size = host_rtt_level_mapsize(level);
	u_register_t map_addr, next_addr, rtt_out_addr, end_addr, top;
	struct rtt_entry rtt;
	u_register_t ret;

	for (map_addr = start; map_addr < end; map_addr = next_addr) {
		next_addr = ALIGN(map_addr + 1U, map_size);
		end_addr = MIN(next_addr, end);

		ret = host_rmi_rtt_readentry(rd, ALIGN_DOWN(map_addr, map_size),
						level, &rtt);
		if (ret != RMI_SUCCESS) {
			continue;
		}

		rtt_out_addr = rtt.out_addr;

		switch (rtt.state) {
		case RMI_ASSIGNED:
			if (rtt.ripas == RMI_EMPTY) {
				ret = host_rmi_rtt_unmap_unprotected(
								rd,
								map_addr,
								level,
								&top);
				if (ret != RMI_SUCCESS) {
					ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
						"host_rmi_rtt_unmap_unprotected",
						map_addr, ret);
					return REALM_ERROR;
				}
			} else {
				ret = host_realm_destroy_undelegate_range(
									realm,
									map_addr,
									rtt_out_addr,
									map_size);
				if (ret != RMI_SUCCESS) {
					ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
						"host_realm_destroy_undelegate_range",
						map_addr, ret);
					return REALM_ERROR;
				}
			}
			break;
		case RMI_UNASSIGNED:
			break;
		case RMI_TABLE:
			ret = host_realm_tear_down_rtt_range(realm, level + 1U,
							     map_addr,
							     end_addr);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, map_addr=0x%lx ret=0x%lx\n",
					"host_realm_tear_down_rtt_range",
					map_addr, ret);
				return REALM_ERROR;
			}

			ret = host_realm_destroy_free_rtt(realm, map_addr,
							  level + 1U,
							  rtt_out_addr);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, map_addr=0x%lx ret=0x%lx\n",
					"host_realm_destroy_free_rtt",
					map_addr, ret);
				return REALM_ERROR;
			}
			break;
		default:
			return REALM_ERROR;
		}
	}

	return REALM_SUCCESS;
}

u_register_t host_rmi_granule_delegate(u_register_t addr)
{
	return host_rmi_handler(&(smc_args) {RMI_GRANULE_DELEGATE, addr}, 2U).ret0;
}

u_register_t host_rmi_granule_undelegate(u_register_t addr)
{
	return host_rmi_handler(&(smc_args) {RMI_GRANULE_UNDELEGATE, addr}, 2U).ret0;
}

u_register_t host_rmi_version(void)
{
	return host_rmi_handler(&(smc_args) {RMI_VERSION}, 1U).ret0;
}

u_register_t host_realm_create(struct realm *realm)
{
	struct rmi_realm_params *params;
	u_register_t ret;

	realm->par_size = REALM_MAX_LOAD_IMG_SIZE;

	realm->state = REALM_STATE_NULL;
	/*
	 * Allocate memory for PAR - Realm image. Granule delegation
	 * of PAR will be performed during rtt creation.
	 */
	realm->par_base = (u_register_t)page_alloc(realm->par_size);
	if (realm->par_base == HEAP_NULL_PTR) {
		ERROR("page_alloc failed, base=0x%lx, size=0x%lx\n",
			  realm->par_base, realm->par_size);
		return REALM_ERROR;
	}

	/* Allocate and delegate RD */
	realm->rd = (u_register_t)page_alloc(PAGE_SIZE);
	if (realm->rd == HEAP_NULL_PTR) {
		ERROR("Failed to allocate memory for rd\n");
		goto err_free_par;
	} else {
		ret = host_rmi_granule_delegate(realm->rd);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, rd=0x%lx ret=0x%lx\n",
				"host_rmi_granule_delegate", realm->rd, ret);
			goto err_free_rd;
		}
	}

	/* Allocate and delegate RTT */
	realm->rtt_addr = (u_register_t)page_alloc(PAGE_SIZE);
	if (realm->rtt_addr == HEAP_NULL_PTR) {
		ERROR("Failed to allocate memory for rtt_addr\n");
		goto err_undelegate_rd;
	} else {
		ret = host_rmi_granule_delegate(realm->rtt_addr);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, rtt_addr=0x%lx ret=0x%lx\n",
				"host_rmi_granule_delegate", realm->rtt_addr, ret);
			goto err_free_rtt;
		}
	}

	/* Allocate memory for params */
	params = (struct rmi_realm_params *)page_alloc(PAGE_SIZE);
	if (params == NULL) {
		ERROR("Failed to allocate memory for params\n");
		goto err_undelegate_rtt;
	}

	/* Populate params */
	params->s2sz = EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
				realm->rmm_feat_reg0);
	params->num_bps = realm->num_bps;
	params->num_wps = realm->num_wps;

	/* SVE enable and vector length */
	if ((realm->rmm_feat_reg0 & RMI_FEATURE_REGISTER_0_SVE_EN) != 0UL) {
		params->flags = RMI_REALM_FLAGS_SVE;
		params->sve_vl = realm->sve_vl;
	} else {
		params->flags = 0UL;
		params->sve_vl = 0U;
	}

	/* PMU enable and number of event counters */
	if ((realm->rmm_feat_reg0 & RMI_FEATURE_REGISTER_0_PMU_EN) != 0UL) {
		params->flags |= RMI_REALM_FLAGS_PMU;
		params->pmu_num_ctrs = realm->pmu_num_ctrs;
	} else {
		params->pmu_num_ctrs = 0U;
	}

	params->hash_algo = RMI_HASH_SHA_256;
	params->vmid = vmid++;
	params->rtt_base = realm->rtt_addr;
	params->rtt_level_start = 0L;
	params->rtt_num_start = 1U;

	/* Create Realm */
	ret = host_rmi_realm_create(realm->rd, (u_register_t)params);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rd=0x%lx ret=0x%lx\n",
			"host_rmi_realm_create", realm->rd, ret);
		goto err_free_params;
	}

	ret = host_rmi_rec_aux_count(realm->rd, &realm->num_aux);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rd=0x%lx ret=0x%lx\n",
			"host_rmi_rec_aux_count", realm->rd, ret);
		host_rmi_realm_destroy(realm->rd);
		goto err_free_params;
	}

	realm->state = REALM_STATE_NEW;

	/* Free params */
	page_free((u_register_t)params);
	return REALM_SUCCESS;

err_free_params:
	page_free((u_register_t)params);

err_undelegate_rtt:
	ret = host_rmi_granule_undelegate(realm->rtt_addr);
	if (ret != RMI_SUCCESS) {
		WARN("%s() failed, rtt_addr=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", realm->rtt_addr, ret);
	}

err_free_rtt:
	page_free(realm->rtt_addr);

err_undelegate_rd:
	ret = host_rmi_granule_undelegate(realm->rd);
	if (ret != RMI_SUCCESS) {
		WARN("%s() failed, rd=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", realm->rd, ret);
	}
err_free_rd:
	page_free(realm->rd);

err_free_par:
	page_free(realm->par_base);

	return REALM_ERROR;
}

u_register_t host_realm_map_payload_image(struct realm *realm,
					  u_register_t realm_payload_adr)
{
	u_register_t src_pa = realm_payload_adr;
	u_register_t i = 0UL;
	u_register_t ret;

	/* MAP image regions */
	while (i < (realm->par_size / PAGE_SIZE)) {
		ret = host_realm_map_protected_data(false, realm,
						realm->par_base + i * PAGE_SIZE,
						PAGE_SIZE,
						src_pa + i * PAGE_SIZE);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, par_base=0x%lx ret=0x%lx\n",
				"host_realm_map_protected_data",
				realm->par_base, ret);
			return REALM_ERROR;
		}
		i++;
	}

	return REALM_SUCCESS;
}

u_register_t host_realm_init_ipa_state(struct realm *realm, u_register_t level,
					u_register_t start, uint64_t end)
{
	u_register_t rd = realm->rd, ret;
	u_register_t map_size = host_rtt_level_mapsize(level);
	u_register_t top;

	while (start < end) {
		ret = host_rmi_rtt_init_ripas(rd, start, end, &top);
		if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT) {
			int cur_level = RMI_RETURN_INDEX(ret);

			if (cur_level < level) {
				ret = host_rmi_create_rtt_levels(realm,
								 start,
								 cur_level,
								 level);
				if (ret != RMI_SUCCESS) {
					ERROR("%s() failed, ret=0x%lx line=%u\n",
						"host_rmi_create_rtt_levels",
						ret, __LINE__);
					return ret;
				}
				/* Retry with the RTT levels in place */
				continue;
			}

			if (level >= RTT_MAX_LEVEL) {
				return REALM_ERROR;
			}

			/* There's an entry at a lower level, recurse */
			host_realm_init_ipa_state(realm, start,
						  start + map_size, level + 1);
		} else if (ret != RMI_SUCCESS) {
			return REALM_ERROR;
		}

		start += map_size;
	}

	return RMI_SUCCESS;
}

u_register_t host_realm_map_ns_shared(struct realm *realm,
					u_register_t ns_shared_mem_adr,
					u_register_t ns_shared_mem_size)
{
	u_register_t i = 0UL;
	u_register_t ret;

	realm->ipa_ns_buffer = ns_shared_mem_adr |
			(1UL << (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
			realm->rmm_feat_reg0) - 1));
	realm->ns_buffer_size = ns_shared_mem_size;
	/* MAP SHARED_NS region */
	while (i < ns_shared_mem_size / PAGE_SIZE) {
		ret = host_realm_map_unprotected(realm, ns_shared_mem_adr +
						 (i * PAGE_SIZE), PAGE_SIZE);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, par_base=0x%lx ret=0x%lx\n",
				"host_realm_map_unprotected",
				(ns_shared_mem_adr + i * PAGE_SIZE), ret);
			return REALM_ERROR;
		}
		i++;
	}
	return REALM_SUCCESS;
}

static void host_realm_free_rec_aux(u_register_t *aux_pages,
					unsigned int num_aux)
{
	u_register_t ret;

	for (unsigned int i = 0U; i < num_aux; i++) {
		ret = host_rmi_granule_undelegate(aux_pages[i]);
		if (ret != RMI_SUCCESS) {
			WARN("%s() failed, index=%u ret=0x%lx\n",
				"host_rmi_granule_undelegate", i, ret);
		}
		page_free(aux_pages[i]);
	}
}

static u_register_t host_realm_alloc_rec_aux(struct realm *realm,
						struct rmi_rec_params *params)
{
	u_register_t ret;
	unsigned int i;

	for (i = 0U; i < realm->num_aux; i++) {
		params->aux[i] = (u_register_t)page_alloc(PAGE_SIZE);
		if (params->aux[i] == HEAP_NULL_PTR) {
			ERROR("Failed to allocate memory for aux rec\n");
			goto err_free_mem;
		}
		ret = host_rmi_granule_delegate(params->aux[i]);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, index=%u ret=0x%lx\n",
				"host_rmi_granule_delegate", i, ret);
			goto err_free_mem;
		}

		/* We need a copy in Realm object for final destruction */
		realm->aux_pages[i] = params->aux[i];
	}
	return RMI_SUCCESS;
err_free_mem:
	host_realm_free_rec_aux(params->aux, i);
	return ret;
}

u_register_t host_realm_rec_create(struct realm *realm)
{
	struct rmi_rec_params *rec_params = HEAP_NULL_PTR;
	u_register_t ret;

	/* Allocate memory for run object */
	realm->run = (u_register_t)page_alloc(PAGE_SIZE);
	if (realm->run == HEAP_NULL_PTR) {
		ERROR("Failed to allocate memory for run\n");
		return REALM_ERROR;
	}
	(void)memset((void *)realm->run, 0x0, PAGE_SIZE);

	/* Allocate and delegate REC */
	realm->rec = (u_register_t)page_alloc(PAGE_SIZE);
	if (realm->rec == HEAP_NULL_PTR) {
		ERROR("Failed to allocate memory for REC\n");
		goto err_free_mem;
	} else {
		ret = host_rmi_granule_delegate(realm->rec);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, rec=0x%lx ret=0x%lx\n",
				"host_rmi_granule_delegate", realm->rd, ret);
			goto err_free_mem;
		}
	}

	/* Allocate memory for rec_params */
	rec_params = (struct rmi_rec_params *)page_alloc(PAGE_SIZE);
	if (rec_params == NULL) {
		ERROR("Failed to allocate memory for rec_params\n");
		goto err_undelegate_rec;
	}
	(void)memset(rec_params, 0x0, PAGE_SIZE);

	/* Populate rec_params */
	for (unsigned int i = 0U; i < (sizeof(rec_params->gprs) /
			sizeof(rec_params->gprs[0]));
			i++) {
		rec_params->gprs[i] = 0x0UL;
	}

	/* Delegate the required number of auxiliary Granules  */
	ret = host_realm_alloc_rec_aux(realm, rec_params);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx\n", "host_realm_alloc_rec_aux",
			ret);
		goto err_free_mem;
	}

	rec_params->pc = realm->par_base;
	rec_params->flags = RMI_RUNNABLE;
	rec_params->mpidr = 0x0UL;
	rec_params->num_aux = realm->num_aux;

	/* Create REC  */
	ret = host_rmi_rec_create(realm->rd, realm->rec, (u_register_t)rec_params);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx\n", "host_rmi_rec_create", ret);
		goto err_free_rec_aux;
	}

	/* Free rec_params */
	page_free((u_register_t)rec_params);
	return REALM_SUCCESS;

err_free_rec_aux:
	host_realm_free_rec_aux(rec_params->aux, realm->num_aux);

err_undelegate_rec:
	ret = host_rmi_granule_undelegate(realm->rec);
	if (ret != RMI_SUCCESS) {
		WARN("%s() failed, rec=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", realm->rec, ret);
	}

err_free_mem:
	page_free(realm->run);
	page_free(realm->rec);
	page_free((u_register_t)rec_params);

	return REALM_ERROR;
}

u_register_t host_realm_activate(struct realm *realm)
{
	u_register_t ret;

	/* Activate Realm  */
	ret = host_rmi_realm_activate(realm->rd);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx\n", "host_rmi_realm_activate",
			ret);
		return REALM_ERROR;
	}

	realm->state = REALM_STATE_ACTIVE;

	return REALM_SUCCESS;
}

u_register_t host_realm_destroy(struct realm *realm)
{
	u_register_t ret;

	if (realm->state == REALM_STATE_NULL) {
		return REALM_SUCCESS;
	}

	if (realm->state == REALM_STATE_NEW) {
		goto undo_from_new_state;
	}

	if (realm->state != REALM_STATE_ACTIVE) {
		ERROR("Invalid realm state found 0x%x\n", realm->state);
		return REALM_ERROR;
	}

	/* For each REC - Destroy, undelegate and free */
	ret = host_rmi_rec_destroy(realm->rec);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rec=0x%lx ret=0x%lx\n",
			"host_rmi_rec_destroy", realm->rec, ret);
		return REALM_ERROR;
	}

	ret = host_rmi_granule_undelegate(realm->rec);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rec=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", realm->rec, ret);
		return REALM_ERROR;
	}

	host_realm_free_rec_aux(realm->aux_pages, realm->num_aux);
	page_free(realm->rec);

	/* Free run object */
	page_free(realm->run);

	/*
	 * For each data granule - Destroy, undelegate and free
	 * RTTs (level 1U and below) must be destroyed leaf-upwards,
	 * using RMI_DATA_DESTROY, RMI_RTT_DESTROY and RMI_GRANULE_UNDELEGATE
	 * commands.
	 */
	if (host_realm_tear_down_rtt_range(realm, 0UL, 0UL,
				(1UL << (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
				realm->rmm_feat_reg0) - 1))) != RMI_SUCCESS) {
		ERROR("host_realm_tear_down_rtt_range() line=%u\n", __LINE__);
		return REALM_ERROR;
	}
	if (host_realm_tear_down_rtt_range(realm, 0UL, realm->ipa_ns_buffer,
			(realm->ipa_ns_buffer + realm->ns_buffer_size)) !=
			RMI_SUCCESS) {
		ERROR("host_realm_tear_down_rtt_range() line=%u\n", __LINE__);
		return REALM_ERROR;
	}
undo_from_new_state:

	/*
	 * RD Destroy, undelegate and free
	 * RTT(L0) undelegate and free
	 * PAR free
	 */
	ret = host_rmi_realm_destroy(realm->rd);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rd=0x%lx ret=0x%lx\n",
			"host_rmi_realm_destroy", realm->rd, ret);
		return REALM_ERROR;
	}

	ret = host_rmi_granule_undelegate(realm->rd);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rd=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", realm->rd, ret);
		return REALM_ERROR;
	}

	ret = host_rmi_granule_undelegate(realm->rtt_addr);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rtt_addr=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", realm->rtt_addr, ret);
		return REALM_ERROR;
	}

	page_free(realm->rd);
	page_free(realm->rtt_addr);
	page_free(realm->par_base);

	return REALM_SUCCESS;
}

u_register_t host_realm_rec_enter(struct realm *realm,
				  u_register_t *exit_reason,
				  unsigned int *host_call_result)
{
	struct rmi_rec_run *run = (struct rmi_rec_run *)realm->run;
	u_register_t ret;
	bool re_enter_rec;

	do {
		re_enter_rec = false;
		ret = host_rmi_handler(&(smc_args) {RMI_REC_ENTER,
					realm->rec, realm->run}, 3U).ret0;
		VERBOSE("%s() run->exit.exit_reason=%lu "
			"run->exit.esr=0x%lx EC_BITS=%u ISS_DFSC_MASK=0x%lx\n",
			__func__, run->exit.exit_reason, run->exit.esr,
			((EC_BITS(run->exit.esr) == EC_DABORT_CUR_EL)),
			(ISS_BITS(run->exit.esr) & ISS_DFSC_MASK));

		/* If a data abort because of a GPF */
		if (EC_BITS(run->exit.esr) == EC_DABORT_CUR_EL) {
			ERROR("EC_BITS(run->exit.esr) == EC_DABORT_CUR_EL\n");
			if ((ISS_BITS(run->exit.esr) & ISS_DFSC_MASK) ==
				DFSC_GPF_DABORT) {
				ERROR("DFSC_GPF_DABORT\n");
			}
		}

		if (ret != RMI_SUCCESS) {
			return ret;
		}

		if (run->exit.exit_reason == RMI_EXIT_HOST_CALL) {
			switch (run->exit.imm) {
			case HOST_CALL_GET_SHARED_BUFF_CMD:
				run->entry.gprs[0] = realm->ipa_ns_buffer;
				re_enter_rec = true;
				break;
			case HOST_CALL_EXIT_SUCCESS_CMD:
				*host_call_result = TEST_RESULT_SUCCESS;
				break;
			case HOST_CALL_EXIT_FAILED_CMD:
				*host_call_result = TEST_RESULT_FAIL;
			default:
				break;
			}
		}
	} while (re_enter_rec);

	*exit_reason = run->exit.exit_reason;
	return ret;
}
