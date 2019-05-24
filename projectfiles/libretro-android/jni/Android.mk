LOCAL_PATH := $(call my-dir)

MAIN_FBA_DIR         := $(LOCAL_PATH)/../../../src
FBA_BURN_DIR         := $(MAIN_FBA_DIR)/burn
FBA_BURN_DRIVERS_DIR := $(MAIN_FBA_DIR)/burn/drv
FBA_BURNER_DIR       := $(MAIN_FBA_DIR)/burner
LIBRETRO_DIR         := $(FBA_BURNER_DIR)/libretro
FBA_CPU_DIR          := $(MAIN_FBA_DIR)/cpu
FBA_LIB_DIR          := $(MAIN_FBA_DIR)/dep/libs
FBA_INTERFACE_DIR    := $(MAIN_FBA_DIR)/intf
FBA_GENERATED_DIR    := $(MAIN_FBA_DIR)/dep/generated
FBA_SCRIPTS_DIR      := $(MAIN_FBA_DIR)/dep/scripts
M68K_DIR             := $(FBA_CPU_DIR)/m68k
NEOGEO_DIR           := $(FBA_BURN_DRIVERS_DIR)/neogeo

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
	$(FBA_BURNER_DIR)/gamc.cpp \
	$(FBA_BURNER_DIR)/un7z.cpp

FBA_BURN_DIRS := $(FBA_BURN_DIR) \
	$(FBA_BURN_DIR)/devices \
	$(FBA_BURN_DIR)/snd \
	$(FBA_BURN_DRIVERS_DIR) \
	$(NEOGEO_DIR)

FBA_CPU_DIRS := $(FBA_CPU_DIR) \
	$(M68K_DIR) \
	$(FBA_CPU_DIR)/z80

FBA_SRC_DIRS := $(FBA_BURNER_DIR) $(FBA_BURN_DIRS) $(FBA_CPU_DIRS) $(FBA_BURNER_DIRS)

FBA_INCLUDES := $(FBA_BURNER_DIR)/win32 \
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

COREFLAGS := -fno-stack-protector -DUSE_SPEEDHACKS -D__LIBRETRO_OPTIMIZATIONS__ -D__LIBRETRO__ -Wno-write-strings -DUSE_FILE32API -DANDROID -DFRONTEND_SUPPORTS_RGB565 -DWANT_NEOGEOCD
COREFLAGS += -Wno-c++11-narrowing

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE       := retro
LOCAL_SRC_FILES    := $(filter-out $(BURN_BLACKLIST),$(foreach dir,$(FBA_SRC_DIRS),$(wildcard $(dir)/*.cpp))) $(filter-out $(BURN_BLACKLIST),$(foreach dir,$(FBA_SRC_DIRS),$(wildcard $(dir)/*.c))) $(LIBRETRO_DIR)/libretro.cpp $(LIBRETRO_DIR)/neocdlist.cpp 
LOCAL_CXXFLAGS     := $(COREFLAGS)
LOCAL_CFLAGS       := $(COREFLAGS)
LOCAL_C_INCLUDES   := $(FBA_INCLUDES)
LOCAL_LDFLAGS      := -Wl,-version-script=$(LIBRETRO_DIR)/link.T
LOCAL_LDLIBS       := -lz
LOCAL_CPP_FEATURES := exceptions rtti
include $(BUILD_SHARED_LIBRARY)
