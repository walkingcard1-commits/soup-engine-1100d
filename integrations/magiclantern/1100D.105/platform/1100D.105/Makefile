MODEL = 1100D
FW_VERSION = 105

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

ML_SRC_SMALL_FONTS = y
CONFIG_LVAPP_HACK_RELOC = y

ML_MODULES_SYM_NAME = t3_$(FW_VERSION).sym

MAIN_FIRMWARE_ADDR = 0xFF010000
PLATFORM_ARCH = armv5te

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
RESTARTSTART = 0xC80100
ML_BOOT_OBJ = boot-d45-am.o

FIRMWARE_ID_1100D = 0x80000288

# Soup Engine Film page. The 1100D gives Magic Lantern about 512 KB in
# total, so features.h trims what the Film page needs room for.

SOUP_ENGINE := $(TOP_DIR)/../../engine
ML_SRC_EXTRA_OBJS = \
    $(BUILD_DIR)/film_menu.o $(BUILD_DIR)/film_develop.o \
    $(BUILD_DIR)/film_picker.o $(BUILD_DIR)/soup_ui.o $(BUILD_DIR)/soup_ui_font.o \
    $(BUILD_DIR)/soup_se.o $(BUILD_DIR)/soup_cr2.o \
    $(BUILD_DIR)/soup_jpeg_dec.o $(BUILD_DIR)/soup_jpeg_enc.o \
    $(BUILD_DIR)/soup_look.o $(BUILD_DIR)/soup_stock.o \
    $(BUILD_DIR)/soup_wb.o $(BUILD_DIR)/soup_color.o \
    $(BUILD_DIR)/soup_grade.o $(BUILD_DIR)/soup_lut.o \
    $(BUILD_DIR)/soup_resize.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete

$(BUILD_DIR)/soup_%.o: $(SOUP_ENGINE)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -DSE_UI_STILLS -I$(SOUP_ENGINE) -c -o $@ $<

$(BUILD_DIR)/film_picker.o: $(SRC_DIR)/film_picker.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -I$(SOUP_ENGINE) -c -o $@ $< $(INCLUDE_ARGS) $(LFLAGS)

$(BUILD_DIR)/film_develop.o: $(SRC_DIR)/film_develop.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -I$(SOUP_ENGINE) -c -o $@ $< $(INCLUDE_ARGS) $(LFLAGS)
