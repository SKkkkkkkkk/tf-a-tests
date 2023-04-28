/*
 * Copyright (c) 2020-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ARCH_FEATURES_H
#define ARCH_FEATURES_H

#include <stdbool.h>
#include <arch_helpers.h>

static inline bool is_armv7_gentimer_present(void)
{
	/* The Generic Timer is always present in an ARMv8-A implementation */
	return true;
}

static inline bool is_armv8_1_pan_present(void)
{
	return ((read_id_aa64mmfr1_el1() >> ID_AA64MMFR1_EL1_PAN_SHIFT) &
		ID_AA64MMFR1_EL1_PAN_MASK) != 0U;
}

static inline bool is_armv8_2_sve_present(void)
{
	return ((read_id_aa64pfr0_el1() >> ID_AA64PFR0_SVE_SHIFT) &
		ID_AA64PFR0_SVE_MASK) == 1U;
}

static inline bool is_armv8_2_ttcnp_present(void)
{
	return ((read_id_aa64mmfr2_el1() >> ID_AA64MMFR2_EL1_CNP_SHIFT) &
		ID_AA64MMFR2_EL1_CNP_MASK) != 0U;
}

static inline bool is_feat_pacqarma3_present(void)
{
	uint64_t mask_id_aa64isar2 =
		(ID_AA64ISAR2_GPA3_MASK << ID_AA64ISAR2_GPA3_SHIFT) |
		(ID_AA64ISAR2_APA3_MASK << ID_AA64ISAR2_APA3_SHIFT);

	/* If any of the fields is not zero, QARMA3 algorithm is present */
	return (read_id_aa64isar2_el1() & mask_id_aa64isar2) != 0U;
}

static inline bool is_armv8_3_pauth_present(void)
{
	uint64_t mask_id_aa64isar1 =
		(ID_AA64ISAR1_GPI_MASK << ID_AA64ISAR1_GPI_SHIFT) |
		(ID_AA64ISAR1_GPA_MASK << ID_AA64ISAR1_GPA_SHIFT) |
		(ID_AA64ISAR1_API_MASK << ID_AA64ISAR1_API_SHIFT) |
		(ID_AA64ISAR1_APA_MASK << ID_AA64ISAR1_APA_SHIFT);

	/*
	 * If any of the fields is not zero or QARMA3 is present,
	 * PAuth is present.
	 */
	return ((read_id_aa64isar1_el1() & mask_id_aa64isar1) != 0U ||
		is_feat_pacqarma3_present());
}

static inline bool is_armv8_3_pauth_apa_api_apa3_present(void)
{
	uint64_t mask_id_aa64isar1 =
		(ID_AA64ISAR1_API_MASK << ID_AA64ISAR1_API_SHIFT) |
		(ID_AA64ISAR1_APA_MASK << ID_AA64ISAR1_APA_SHIFT);

	uint64_t mask_id_aa64isar2 =
		(ID_AA64ISAR2_APA3_MASK << ID_AA64ISAR2_APA3_SHIFT);

	return ((read_id_aa64isar1_el1() & mask_id_aa64isar1) |
		(read_id_aa64isar2_el1() & mask_id_aa64isar2)) != 0U;
}

static inline bool is_armv8_3_pauth_gpa_gpi_gpa3_present(void)
{
	uint64_t mask_id_aa64isar1 =
		(ID_AA64ISAR1_GPI_MASK << ID_AA64ISAR1_GPI_SHIFT) |
		(ID_AA64ISAR1_GPA_MASK << ID_AA64ISAR1_GPA_SHIFT);

	uint64_t mask_id_aa64isar2 =
		(ID_AA64ISAR2_GPA3_MASK << ID_AA64ISAR2_GPA3_SHIFT);

	return ((read_id_aa64isar1_el1() & mask_id_aa64isar1) |
		(read_id_aa64isar2_el1() & mask_id_aa64isar2)) != 0U;
}

static inline bool is_armv8_4_dit_present(void)
{
	return ((read_id_aa64pfr0_el1() >> ID_AA64PFR0_DIT_SHIFT) &
		ID_AA64PFR0_DIT_MASK) == 1U;
}

static inline bool is_armv8_4_ttst_present(void)
{
	return ((read_id_aa64mmfr2_el1() >> ID_AA64MMFR2_EL1_ST_SHIFT) &
		ID_AA64MMFR2_EL1_ST_MASK) == 1U;
}

static inline bool is_armv8_5_bti_present(void)
{
	return ((read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_BT_SHIFT) &
		ID_AA64PFR1_EL1_BT_MASK) == BTI_IMPLEMENTED;
}

static inline unsigned int get_armv8_5_mte_support(void)
{
	return ((read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_MTE_SHIFT) &
		ID_AA64PFR1_EL1_MTE_MASK);
}

static inline bool is_armv8_6_fgt_present(void)
{
	return ((read_id_aa64mmfr0_el1() >> ID_AA64MMFR0_EL1_FGT_SHIFT) &
		ID_AA64MMFR0_EL1_FGT_MASK) == ID_AA64MMFR0_EL1_FGT_SUPPORTED;
}

static inline unsigned long int get_armv8_6_ecv_support(void)
{
	return ((read_id_aa64mmfr0_el1() >> ID_AA64MMFR0_EL1_ECV_SHIFT) &
		ID_AA64MMFR0_EL1_ECV_MASK);
}

static inline unsigned long int get_pa_range(void)
{
	return ((read_id_aa64mmfr0_el1() >> ID_AA64MMFR0_EL1_PARANGE_SHIFT) &
		ID_AA64MMFR0_EL1_PARANGE_MASK);
}

static inline uint32_t arch_get_debug_version(void)
{
	return ((read_id_aa64dfr0_el1() & ID_AA64DFR0_DEBUG_BITS) >>
		ID_AA64DFR0_DEBUG_SHIFT);
}

static inline bool get_armv9_0_trbe_support(void)
{
	return ((read_id_aa64dfr0_el1() >> ID_AA64DFR0_TRACEBUFFER_SHIFT) &
		ID_AA64DFR0_TRACEBUFFER_MASK) ==
		ID_AA64DFR0_TRACEBUFFER_SUPPORTED;
}

static inline bool get_armv8_4_trf_support(void)
{
	return ((read_id_aa64dfr0_el1() >> ID_AA64DFR0_TRACEFILT_SHIFT) &
		ID_AA64DFR0_TRACEFILT_MASK) ==
		ID_AA64DFR0_TRACEFILT_SUPPORTED;
}

static inline bool get_armv8_0_sys_reg_trace_support(void)
{
	return ((read_id_aa64dfr0_el1() >> ID_AA64DFR0_TRACEVER_SHIFT) &
		ID_AA64DFR0_TRACEVER_MASK) ==
		ID_AA64DFR0_TRACEVER_SUPPORTED;
}

static inline unsigned int get_armv9_2_feat_rme_support(void)
{
	/*
	 * Return the RME version, zero if not supported.  This function can be
	 * used as both an integer value for the RME version or compared to zero
	 * to detect RME presence.
	 */
	return (unsigned int)(read_id_aa64pfr0_el1() >>
		ID_AA64PFR0_FEAT_RME_SHIFT) & ID_AA64PFR0_FEAT_RME_MASK;
}

static inline bool get_feat_hcx_support(void)
{
	return (((read_id_aa64mmfr1_el1() >> ID_AA64MMFR1_EL1_HCX_SHIFT) &
		ID_AA64MMFR1_EL1_HCX_MASK) == ID_AA64MMFR1_EL1_HCX_SUPPORTED);
}

static inline bool get_feat_afp_present(void)
{
	return (((read_id_aa64mmfr1_el1() >> ID_AA64MMFR1_EL1_AFP_SHIFT) &
		  ID_AA64MMFR1_EL1_AFP_MASK) == ID_AA64MMFR1_EL1_AFP_SUPPORTED);
}

static inline bool get_feat_brbe_support(void)
{
	return ((read_id_aa64dfr0_el1() >> ID_AA64DFR0_BRBE_SHIFT) &
		ID_AA64DFR0_BRBE_MASK) ==
		ID_AA64DFR0_BRBE_SUPPORTED;
}

static inline bool get_feat_wfxt_present(void)
{
	return (((read_id_aa64isar2_el1() >> ID_AA64ISAR2_WFXT_SHIFT) &
		ID_AA64ISAR2_WFXT_MASK) == ID_AA64ISAR2_WFXT_SUPPORTED);
}

static inline bool is_feat_rng_trap_present(void)
{
	return (((read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_RNDR_TRAP_SHIFT) &
			ID_AA64PFR1_EL1_RNDR_TRAP_MASK)
			== ID_AA64PFR1_EL1_RNG_TRAP_SUPPORTED);
}

static inline unsigned int spe_get_version(void)
{
	return (unsigned int)((read_id_aa64dfr0_el1() >> ID_AA64DFR0_PMS_SHIFT) &
		ID_AA64DFR0_PMS_MASK);
}

static inline bool get_feat_pmuv3_supported(void)
{
	return (((read_id_aa64dfr0_el1() >> ID_AA64DFR0_PMUVER_SHIFT) &
		ID_AA64DFR0_PMUVER_MASK) != ID_AA64DFR0_PMUVER_NOT_SUPPORTED);
}

static inline bool get_feat_hpmn0_supported(void)
{
	return (((read_id_aa64dfr0_el1() >> ID_AA64DFR0_HPMN0_SHIFT) &
		ID_AA64DFR0_HPMN0_MASK) == ID_AA64DFR0_HPMN0_SUPPORTED);
}

static inline bool is_feat_sme_supported(void)
{
	uint64_t features;

	features = read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_SME_SHIFT;
	return (features & ID_AA64PFR1_EL1_SME_MASK) >= ID_AA64PFR1_EL1_SME_SUPPORTED;
}

static inline bool is_feat_sme_fa64_supported(void)
{
	uint64_t features;

	features = read_id_aa64smfr0_el1();
	return (features & ID_AA64SMFR0_EL1_FA64_BIT) != 0U;
}

static inline bool is_feat_sme2_supported(void)
{
	uint64_t features;

	features = read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_SME_SHIFT;
	return (features & ID_AA64PFR1_EL1_SME_MASK) >= ID_AA64PFR1_EL1_SME2_SUPPORTED;
}

#endif /* ARCH_FEATURES_H */
