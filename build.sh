#!/usr/bin/env bash
CROSS_COMPILE=aarch64-none-elf- \
make \
PLAT=rhea \
TESTS=smc \
tftf \
FWU_BL_TEST=0 \
ns_bl1u \
V=0