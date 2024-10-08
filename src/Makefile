# Makefile to build WASM and simulator JS.

# Build upon the codal_port code.
# CODAL_PORT = $(abspath ../lib/micropython-microbit-v2/src/codal_port)
# CODAL_APP = $(abspath ../lib/micropython-microbit-v2/src/codal_app)
CODAL_PORT = $(abspath ./bitsflow)

MICROPY_ROM_TEXT_COMPRESSION ?= 1
FROZEN_MANIFEST ?= $(CODAL_PORT)/manifest.py

include ../lib/micropython-microbit-v2/lib/micropython/py/mkenv.mk
-include mpconfigport.mk

# QSTR definitions (must come before including py.mk).
QSTR_DEFS = $(CODAL_PORT)/qstrdefsport.h

# Include py core make definitions.
include $(TOP)/py/py.mk

CC = emcc
LD = emcc

MP_VER_FILE = $(HEADER_BUILD)/mpversion.h
MBIT_VER_FILE = $(HEADER_BUILD)/bitsflowversion.h

LOCAL_LIB_DIR = ../lib/micropython-microbit-v2/lib

INC += -I.
INC += -I$(CODAL_PORT)
# INC += -I$(CODAL_APP)
INC += -I$(LOCAL_LIB_DIR)
INC += -I$(TOP)
INC += -I$(BUILD)

# Compiler settings.
CWARN += -Wall -Wpointer-arith -Wuninitialized -Wno-array-bounds
CFLAGS += $(INC) $(CWARN) -std=c99 -funsigned-char $(CFLAGS_MOD) $(CFLAGS_ARCH) $(COPT) $(CFLAGS_EXTRA)

# Debugging/Optimization
ifdef DEBUG
COPT += -O3
CFLAGS += -g
else
COPT += -O3 -DNDEBUG
endif

JSFLAGS += -s ASYNCIFY
# We can hit lower values due to user stack use. See stack_size.py example.
JSFLAGS += -s ASYNCIFY_STACK_SIZE=262144
JSFLAGS += -s EXIT_RUNTIME
JSFLAGS += -s MODULARIZE=1
JSFLAGS += -s EXPORT_NAME=createModule
JSFLAGS += -s EXPORTED_FUNCTIONS="['_mp_js_main','_bitsflow_hal_audio_ready_callback','_bitsflow_hal_audio_speech_ready_callback','_bitsflow_hal_gesture_callback','_bitsflow_hal_level_detector_callback','_bitsflow_radio_rx_buffer','_mp_js_force_stop','_mp_js_request_stop']"
JSFLAGS += -s EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap']" --js-library jshal.js

ifdef DEBUG
JSFLAGS += -g
endif

SRC_C += \
	drv_radio.c \
	bitsflowfs.c \
	bitsflowhal_js.c \
	main.c \
	mphalport.c \
	modmachine.c \

SRC_C += $(addprefix $(CODAL_PORT)/, \
	drv_display.c \
	drv_image.c \
	drv_softtimer.c \
	drv_system.c \
	help.c \
	iters.c \
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
	)

SRC_C += \
	shared/readline/readline.c \
	shared/runtime/interrupt_char.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	$(CODAL_PORT)/sam/main.c \
	$(CODAL_PORT)/sam/reciter.c \
	$(CODAL_PORT)/sam/render.c \
	$(CODAL_PORT)/sam/sam.c \
	$(CODAL_PORT)/sam/debug.c \

# $(abspath $(LOCAL_LIB_DIR)/sam/main.c) \
# $(abspath $(LOCAL_LIB_DIR)/sam/reciter.c) \
# $(abspath $(LOCAL_LIB_DIR)/sam/render.c) \
# $(abspath $(LOCAL_LIB_DIR)/sam/sam.c) \
# $(abspath $(LOCAL_LIB_DIR)/sam/debug.c) \

SRC_O += \
	lib/utils/gchelper_m3.o \

OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o))

# List of sources for qstr extraction.
SRC_QSTR += $(SRC_C) $(LIB_SRC_C)
# Append any auto-generated sources that are needed by sources listed in.
# SRC_QSTR
SRC_QSTR_AUTO_DEPS +=
QSTR_GLOBAL_REQUIREMENTS += $(MBIT_VER_FILE)

# Top-level rule.
all: $(MBIT_VER_FILE) $(BUILD)/micropython.js

# Rule to build header with micro:bit specific version information.
# Also rebuild MicroPython version header in correct directory to pick up git hash.
$(MBIT_VER_FILE): FORCE
	$(Q)mkdir -p $(HEADER_BUILD)
	(cd $(TOP) && $(PYTHON) py/makeversionhdr.py $(abspath $(MP_VER_FILE)))
	$(PYTHON) $(TOP)/py/makeversionhdr.py $(MBIT_VER_FILE).pre
	$(CAT) $(MBIT_VER_FILE).pre | $(SED) s/MICROPY_/BITSFLOW_/ > $(MBIT_VER_FILE)

$(BUILD)/micropython.js: $(OBJ) jshal.js simulator-js
	$(ECHO) "LINK $(BUILD)/firmware.js"
	$(Q)emcc $(LDFLAGS) -o $(BUILD)/firmware.js $(OBJ) $(JSFLAGS)

simulator-js:
	npx esbuild ./simulator.ts --bundle --outfile=$(BUILD)/simulator.js --loader:.svg=text

include $(TOP)/py/mkrules.mk

.PHONY: simulator-js