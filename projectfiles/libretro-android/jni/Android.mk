CYCLONE_ENABLED := 0
HAVE_GRIFFIN    := 0

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)


MAIN_FBA_DIR := ../../../src
FBA_BURN_DIR := $(MAIN_FBA_DIR)/burn
FBA_BURN_DRIVERS_DIR := $(MAIN_FBA_DIR)/burn/drv
FBA_BURNER_DIR := $(MAIN_FBA_DIR)/burner
LIBRETRO_DIR := $(FBA_BURNER_DIR)/libretro
FBA_CPU_DIR := $(MAIN_FBA_DIR)/cpu
FBA_LIB_DIR := $(MAIN_FBA_DIR)/dep/libs
FBA_INTERFACE_DIR := $(MAIN_FBA_DIR)/intf
FBA_GENERATED_DIR = $(MAIN_FBA_DIR)/dep/generated
FBA_SCRIPTS_DIR = $(MAIN_FBA_DIR)/dep/scripts
GRIFFIN_DIR := ../../../griffin-libretro

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	LOCAL_CXXFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

ifeq ($(TARGET_ARCH),arm)
LOCAL_CXXFLAGS += -DANDROID_ARM
LOCAL_ARM_MODE := arm
ifeq ($(CYCLONE_ENABLED), 1)
CYCLONE_SRC := $(FBA_CPU_DIR)/cyclone/cyclone.s
CYCLONE_DEFINES := -DBUILD_C68K
endif
endif

ifeq ($(TARGET_ARCH),x86)
LOCAL_CXXFLAGS +=  -DANDROID_X86
endif

ifeq ($(TARGET_ARCH),mips)
LOCAL_CXXFLAGS += -DANDROID_MIPS -D__mips__ -D__MIPSEL__
endif

BURN_BLACKLIST := $(FBA_CPU_DIR)/arm7/arm7exec.c \
	$(FBA_CPU_DIR)/m68k/m68k_in.c \
	$(FBA_CPU_DIR)/m68k/m68kmake.c \
	$(FBA_CPU_DIR)/m68k/m68kdasm.c \
	$(FBA_BURNER_DIR)/sshot.cpp \
	$(FBA_BURNER_DIR)/conc.cpp \
	$(FBA_BURNER_DIR)/dat.cpp \
	$(FBA_BURNER_DIR)/cong.cpp \
	$(FBA_BURNER_DIR)/image.cpp \
	$(FBA_BURNER_DIR)/misc.cpp \
	$(FBA_BURNER_DIR)/gami.cpp \
	$(FBA_BURNER_DIR)/gamc.cpp

ifeq ($(HAVE_GRIFFIN), 1)
GRIFFIN_CXX_SRC_FILES := $(GRIFFIN_DIR)/cps12.cpp $(GRIFFIN_DIR)/cps3.cpp $(GRIFFIN_DIR)/neogeo.cpp $(GRIFFIN_DIR)/pgm.cpp $(GRIFFIN_DIR)/snes.cpp $(GRIFFIN_DIR)/galaxian.cpp
GRIFFIN_CXX_SRC_FILES += $(GRIFFIN_DIR)/cpu-m68k.cpp
BURN_BLACKLIST += $(FBA_CPU_DIR)/m68000_intf.cpp
else
NEOGEO_DIR := $(FBA_BURN_DRIVERS_DIR)/neogeo
endif

FBA_BURN_DIRS := $(FBA_BURN_DIR) \
	$(FBA_BURN_DIR)/devices \
	$(FBA_BURN_DIR)/snd \
	$(FBA_BURN_DRIVERS_DIR) \
	$(NEOGEO_DIR)

FBA_CPU_DIRS := $(FBA_CPU_DIR) \
	$(M68K_DIR) \
	$(FBA_CPU_DIR)/z80

FBA_SRC_DIRS := $(FBA_BURNER_DIR) $(FBA_BURN_DIRS) $(FBA_CPU_DIRS) $(FBA_BURNER_DIRS)

LOCAL_MODULE    := libretro

LOCAL_SRC_FILES := $(GRIFFIN_CXX_SRC_FILES) $(CYCLONE_SRC)  $(filter-out $(BURN_BLACKLIST),$(foreach dir,$(FBA_SRC_DIRS),$(wildcard $(dir)/*.cpp))) $(filter-out $(BURN_BLACKLIST),$(foreach dir,$(FBA_SRC_DIRS),$(wildcard $(dir)/*.c))) $(LIBRETRO_DIR)/libretro.cpp $(LIBRETRO_DIR)/neocdlist.cpp 

GLOBAL_DEFINES := -DWANT_NEOGEOCD

LOCAL_CXXFLAGS += -O2 -fno-stack-protector -DUSE_SPEEDHACKS -D__LIBRETRO_OPTIMIZATIONS__ -D__LIBRETRO__ -Wno-write-strings -DUSE_FILE32API -DANDROID -DFRONTEND_SUPPORTS_RGB565 $(CYCLONE_DEFINES) $(GLOBAL_DEFINES)
LOCAL_CFLAGS = -O2 -fno-stack-protector -DUSE_SPEEDHACKS -D__LIBRETRO_OPTIMIZATIONS__ -D__LIBRETRO__ -Wno-write-strings -DUSE_FILE32API -DANDROID -DFRONTEND_SUPPORTS_RGB565 $(CYCLONE_DEFINES) $(GLOBAL_DEFINES)

LOCAL_C_INCLUDES = $(FBA_BURNER_DIR)/win32 \
	$(LIBRETRO_DIR) \
	$(LIBRETRO_DIR)/libretro-common/include \
	$(LIBRETRO_DIR)/tchar \
	$(FBA_BURN_DIR) \
	$(MAIN_FBA_DIR)/cpu \
	$(FBA_BURN_DIR)/snd \
	$(FBA_BURN_DIR)/devices \
	$(FBA_INTERFACE_DIR) \
	$(FBA_INTERFACE_DIR)/input \
	$(FBA_INTERFACE_DIR)/cd \
	$(FBA_BURNER_DIR) \
	$(FBA_CPU_DIR) \
	$(FBA_LIB_DIR)/lib7z \
	$(FBA_LIB_DIR)/zlib \
	$(FBA_BURN_DIR)/drv/neogeo \
	$(FBA_GENERATED_DIR) \
	$(FBA_LIB_DIR)

LOCAL_LDLIBS += -lz

include $(BUILD_SHARED_LIBRARY)
