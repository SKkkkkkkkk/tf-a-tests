/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SME_H
#define SME_H

#define MAX_VL			(512)
#define MAX_VL_B		(MAX_VL / 8)
#define SME_SMCR_LEN_MAX	U(0x1FF)

typedef enum {
	SMSTART,	/* enters streaming sve mode and enables SME ZA array */
	SMSTART_SM,	/* enters streaming sve mode only */
	SMSTART_ZA,	/* enables SME ZA array storage only */
} smestart_instruction_type_t;

typedef enum {
	SMSTOP,		/* exits streaming sve mode, & disables SME ZA array */
	SMSTOP_SM,	/* exits streaming sve mode only */
	SMSTOP_ZA,	/* disables SME ZA array storage only */
} smestop_instruction_type_t;

/* SME feature related prototypes. */
void sme_enable(void);
void sme_smstart(smestart_instruction_type_t smstart_type);
void sme_smstop(smestop_instruction_type_t smstop_type);

/* Assembly function prototypes. */
uint64_t sme_rdvl_1(void);
void sme_try_illegal_instruction(void);
void sme_vector_to_ZA(const uint64_t *input_vector);
void sme_ZA_to_vector(const uint64_t *output_vector);

#endif /* SME_H */
