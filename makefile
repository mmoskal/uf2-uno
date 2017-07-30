#
#             LUFA Library
#     Copyright (C) Dean Camera, 2017.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

# Run "make help" for target help.

MCU          = atmega16u2
ARCH         = AVR8
BOARD        = USER
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = uf2uno
SRC          = $(TARGET).c hid.c Descriptors.c Lib/DataflashManager.c Lib/SCSI.c $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
LUFA_PATH    = ../../../../LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -IConfig/ -Wall -Werror -W -Wno-unused-parameter
LD_FLAGS     =

CC_FLAGS += -DAVR_RESET_LINE_PORT="PORTD"
CC_FLAGS += -DAVR_RESET_LINE_DDR="DDRD"
CC_FLAGS += -DAVR_RESET_LINE_MASK="(1 << 7)"
CC_FLAGS += -DTX_RX_LED_PULSE_MS=3
CC_FLAGS += -DPING_PONG_LED_PULSE_MS=100

# Default target
all:

DPROG = dfu-programmer $(MCU)

recover:
	$(DPROG) erase || :
	$(DPROG) flash tmp/Genuino-usbserial-atmega16u2-Uno-R3.hex

burn: all
	$(DPROG) erase || :
	$(DPROG) flash $(TARGET).hex

b: burn

# Include LUFA-specific DMBS extension modules
DMBS_LUFA_PATH ?= $(LUFA_PATH)/Build/LUFA
include $(DMBS_LUFA_PATH)/lufa-sources.mk
include $(DMBS_LUFA_PATH)/lufa-gcc.mk

# Include common DMBS build system modules
DMBS_PATH      ?= $(LUFA_PATH)/Build/DMBS/DMBS
include $(DMBS_PATH)/core.mk
include $(DMBS_PATH)/cppcheck.mk
include $(DMBS_PATH)/doxygen.mk
include $(DMBS_PATH)/dfu.mk
include $(DMBS_PATH)/gcc.mk
include $(DMBS_PATH)/hid.mk
include $(DMBS_PATH)/avrdude.mk
include $(DMBS_PATH)/atprogram.mk
