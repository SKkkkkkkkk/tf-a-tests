PLAT_INCLUDES := \
-Iplat/seehi/rhea/include

PLAT_SOURCES := \
plat/seehi/rhea/plat_setup.c \
plat/seehi/rhea/plat_helpers.S \
drivers/ti/uart/aarch64/16550_console.S \
drivers/arm/gic/arm_gic_v2v3.c \
drivers/arm/gic/gic_common.c \
drivers/arm/gic/gic_v3.c \
drivers/arm/gic/gic_v2.c \
plat/seehi/rhea/plat_pwr_state.c \


# plat/arm/common/arm_timers.c	\
# drivers/arm/timer/system_timer.c \


ifeq ($(USE_NVM),1)
$(error "DUBHE port of TFTF doesn't currently support USE_NVM=1")
endif

FIRMWARE_UPDATE := 1