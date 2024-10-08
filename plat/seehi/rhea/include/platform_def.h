#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <arch.h>
#include <lib/utils_def.h>
#include <plat/common/common_def.h>
#include "memmap.h"

/*******************************************************************************
 * Platform binary types for linking
 ******************************************************************************/
#ifdef __aarch64__
#define PLATFORM_LINKER_FORMAT		"elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH		aarch64
#else
#define PLATFORM_LINKER_FORMAT		"elf32-littlearm"
#define PLATFORM_LINKER_ARCH		arm
#endif

/*******************************************************************************
 * Platform specific page table and MMU setup constants
 ******************************************************************************/
#define PLAT_PHY_ADDR_SPACE_SIZE	(ULL(1) << 32)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(ULL(1) << 32)

/* stack memory available to each CPU */
#define PLATFORM_STACK_SIZE			0x1400
#define PCPU_DV_MEM_STACK_SIZE		0x100

/*******************************************************************************
 * Used to align variables on the biggest cache line size in the platform.
 * This is known only to the platform as it might have a combination of
 * integrated and external caches.
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT   6
#define CACHE_WRITEBACK_GRANULE (1 << CACHE_WRITEBACK_SHIFT)

#define PLATFORM_CLUSTER_COUNT			1
#define PLATFORM_CORE_COUNT_PER_CLUSTER 4
#define PLATFORM_CORE_COUNT				(PLATFORM_CORE_COUNT_PER_CLUSTER * PLATFORM_CLUSTER_COUNT)
#define PLATFORM_NUM_AFFS				(PLATFORM_CLUSTER_COUNT + PLATFORM_CORE_COUNT)
#define PLATFORM_MAX_AFFLVL				1
#define PLAT_MAX_PWR_LEVEL				PLATFORM_MAX_AFFLVL

/* Local state bit width for each level in the state-ID field of power state */
#define PLAT_LOCAL_PSTATE_WIDTH		4
#define PLAT_MAX_PWR_STATES_PER_LVL	2

/*******************************************************************************
 * Non-Secure Software Generated Interupts IDs
 ******************************************************************************/
#define IRQ_NS_SGI_0			0
#define IRQ_NS_SGI_1			1
#define IRQ_NS_SGI_2			2
#define IRQ_NS_SGI_3			3
#define IRQ_NS_SGI_4			4
#define IRQ_NS_SGI_5			5
#define IRQ_NS_SGI_6			6
#define IRQ_NS_SGI_7			7

#define PLAT_MAX_SPI_OFFSET_ID 		128

#if IMAGE_TFTF
/* For testing xlat tables lib v2 */
#define MAX_XLAT_TABLES			20
#define MAX_MMAP_REGIONS		50
#else
#define MAX_XLAT_TABLES			10
#define MAX_MMAP_REGIONS		16
#endif

#define ROM_BASE A55_BOOTROM_BASE
#define ROM_SIZE UL(0x00040000) //256KB

#define SRAM_BASE APRAM_BASE
#define SRAM_SIZE UL(0x00080000) //512KB

#define DRAM_BASE AP_DRAM_BASE
#define DRAM_SIZE UL(0x18000000) //384MiB

#define DEVICE_START_BASE				UART0_BASE
#define DEVICE_END_BASE					round_up(IOMUX_CFG_BASE + 4096, PAGE_SIZE)

#define TFTF_BASE 			UL(0x42000000)
#define TFTF_NVM_OFFSET		UL(/*DRAM_BASE+*/0x500000)
#define TFTF_NVM_SIZE		UL(0x04000000)

#define NS_BL1U_BASE		(ROM_BASE + (76*1024))
#define NS_BL1U_RO_LIMIT	(ROM_BASE + ROM_SIZE)
#define NS_BL1U_RW_BASE		(SRAM_BASE + SRAM_SIZE - (128*1024))
#define NS_BL1U_RW_LIMIT	(SRAM_BASE + SRAM_SIZE)

#define CRASH_CONSOLE_BASE			UART0_BASE
#define CRASH_CONSOLE_CLK_IN_HZ		UL(25000000)
#define CRASH_CONSOLE_BAUDRATE		115200

// #define SYS_CNT_BASE1 		GENERIC_TIMER_BASE
// #define IRQ_CNTPSIRQ1		92

#define ITScount 1
#define RDcount  4
#define GICD_BASE	GIC600_BASE
#define GICR_BASE	(GIC600_BASE + ((4 + (2 * ITScount)) << 16))

#endif /* PLATFORM_DEF_H */
