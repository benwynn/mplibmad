
# Architecture to build for (x86, x64, armv7m, xtensa, xtensawin, rv32imc)
ARCH ?= armv7emsp

# libmad flags
MAD_SRC := $(filter-out libmad/minimad.c, $(wildcard libmad/*.c))

# FPM: Fixed Point Math, optimizations for different situations (architectures and compiler capability)
# FPM_64BIT: compiler knows how to geneerate instructions that can handle 64-bit values with 32-bit registers
FPM := -DFPM_64BIT

# put all the MAD-specific stuff together to add to CFLAGS later
MAD_CFLAGS := ${FPM} -DHAVE_CONFIG_H

# micropython natmod settings
ifdef PICO_SDK_PATH
# assume micropython and pico-sdk are at the same level
MPY_DIR ?= ${PICO_SDK_PATH}/../micropython
else
# otherwise, assume we're inside the micropython directory a few levels
MPY_DIR ?= ../../..
endif

MOD    := mplibmad_$(ARCH)
SRC    := module.c natglue.c ${MAD_SRC}
CFLAGS += -Wno-unused-variable ${MAD_CFLAGS}

include ${MPY_DIR}/py/dynruntime.mk

