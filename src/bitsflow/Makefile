# Makefile to build libmicropython.a

MICROPY_ROM_TEXT_COMPRESSION ?= 1
FROZEN_MANIFEST ?= manifest.py

CROSS_COMPILE = arm-none-eabi-
CFLAGS_EXTRA = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4

include ../../lib/micropython/py/mkenv.mk
-include mpconfigport.mk

# QSTR definitions (must come before including py.mk).
QSTR_DEFS = qstrdefsport.h

# Include py core make definitions.
include $(TOP)/py/py.mk

MP_VER_FILE = $(HEADER_BUILD)/mpversion.h
MBIT_VER_FILE = $(HEADER_BUILD)/bitsflowversion.h

LOCAL_LIB_DIR = ../../lib
CMSIS_DIR = $(LOCAL_LIB_DIR)/codal/libraries/codal-nrf52/inc/cmsis
NRFX_DIR = $(LOCAL_LIB_DIR)/codal/libraries/codal-nrf52/nrfx

INC += -I.
INC += -I../codal_app
INC += -I$(LOCAL_LIB_DIR)
INC += -I$(CMSIS_DIR)
INC += -I$(NRFX_DIR)
INC += -I$(NRFX_DIR)/drivers/include
INC += -I$(NRFX_DIR)/hal
INC += -I$(NRFX_DIR)/mdk
INC += -I$(TOP)
INC += -I$(TOP)/ports/nrf
INC += -I$(BUILD)

# Compiler settings.
CWARN = -Wall -Werror
CWARN += -Wpointer-arith -Wuninitialized
CFLAGS_ARCH += -DNRF52833_XXAA
CFLAGS = $(INC) $(CWARN) -std=c99 $(CFLAGS_MOD) $(CFLAGS_ARCH) $(COPT) $(CFLAGS_EXTRA)

# Debugging/Optimization
ifdef DEBUG
COPT = -O0
else
COPT = -Os -DNDEBUG
endif

CFLAGS += -g
LDFLAGS_ARCH = -Wl,-map,$@.map
LDFLAGS = $(LDFLAGS_MOD) $(LDFLAGS_ARCH) -lm $(LDFLAGS_EXTRA)

SRC_C += \
	drv_display.c \
	drv_image.c \
	drv_radio.c \
	drv_softtimer.c \
	drv_system.c \
	help.c \
	iters.c \
	main.c \
	bitsflow_accelerometer.c \
	bitsflow_button.c \
	bitsflow_compass.c \
	bitsflow_display.c \
	bitsflow_i2c.c \
	bitsflow_image.c \
	bitsflow_constimage.c \
	bitsflow_microphone.c \
	bitsflow_pin.c \
	bitsflow_pinaudio.c \
	bitsflow_pinmode.c \
	bitsflow_sound.c \
	bitsflow_soundeffect.c \
	bitsflow_soundevent.c \
	bitsflow_speaker.c \
	bitsflow_spi.c \
	bitsflow_uart.c \
	bitsflowfs.c \
	modantigravity.c \
	modaudio.c \
	modlog.c \
	modlove.c \
	modmachine.c \
	modbitsflow.c \
	modmusic.c \
	modmusictunes.c \
	modos.c \
	modpower.c \
	modradio.c \
	modspeech.c \
	modthis.c \
	modutime.c \
	mphalport.c \

SRC_C += \
	shared/readline/readline.c \
	shared/runtime/gchelper_native.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	$(abspath $(NRFX_DIR)/drivers/src/nrfx_nvmc.c) \
	$(abspath $(LOCAL_LIB_DIR)/sam/main.c) \
	$(abspath $(LOCAL_LIB_DIR)/sam/reciter.c) \
	$(abspath $(LOCAL_LIB_DIR)/sam/render.c) \
	$(abspath $(LOCAL_LIB_DIR)/sam/sam.c) \
	$(abspath $(LOCAL_LIB_DIR)/sam/debug.c) \

SRC_O += \
	shared/runtime/gchelper_m3.o \

OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_O))
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o))

# List of sources for qstr extraction.
SRC_QSTR += $(SRC_C) $(LIB_SRC_C)
# Append any auto-generated sources that are needed by sources listed in.
# SRC_QSTR
SRC_QSTR_AUTO_DEPS +=
QSTR_GLOBAL_REQUIREMENTS += $(MBIT_VER_FILE)

# Top-level rule.
all: lib $(MBIT_VER_FILE)

# Rule to build header with bitsflow specific version information.
# Also rebuild MicroPython version header in correct directory to pick up git hash.
$(MBIT_VER_FILE): FORCE
	(cd $(TOP) && $(PYTHON) py/makeversionhdr.py $(abspath $(MP_VER_FILE)))
	$(PYTHON) $(TOP)/py/makeversionhdr.py $(MBIT_VER_FILE).pre
	$(CAT) $(MBIT_VER_FILE).pre | $(SED) s/MICROPY_/BITSFLOW_/ > $(MBIT_VER_FILE)

# Suppress warnings from SAM library.
$(BUILD)/$(abspath $(LOCAL_LIB_DIR))/sam/sam.o: CWARN += -Wno-array-bounds

include $(TOP)/py/mkrules.mk

CLEAN_EXTRA += $(LIBMICROPYTHON)
