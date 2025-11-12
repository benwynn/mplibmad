
# Architecture to build for (x86, x64, armv7m, xtensa, xtensawin, rv32imc)
ARCH ?= x64

# libmad flags
# FPM_64BIT: compiler knows how to geneerate instructions that can handle 64-bit values with 32-bit registers
FPM := -DFPM_64BIT
ASO := -DASO_INTERLEAVE1 -DASO_IMDCT
MAD_CFLAGS := ${FPM} ${ASO} -DHAVE_CONFIG_H

# micropython natmod settings
ifdef PICO_SDK_PATH
MPY_DIR ?= ${PICO_SDK_PATH}/../micropython
else
MPY_DIR ?= ../../..
endif

MOD     := mp-libmad_$(ARCH)
SRC     := module.c $(wildcard libmad/*.c)
CFLAGS += -Wno-unused-variable ${MAD_CFLAGS}

include ${MPY_DIR}/py/dynruntime.mk
