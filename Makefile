DEBUG = 0
LIBRETRO_OPTIMIZATIONS = 1
FRONTEND_SUPPORTS_RGB565 = 1

ifeq ($(platform),)
   platform = unix
   ifeq ($(shell uname -a),)
      platform = win
      EXE_EXT=.exe
   else ifneq ($(findstring Darwin,$(shell uname -a)),)
      platform = osx
   else ifneq ($(findstring MINGW,$(shell uname -a)),)
      platform = win
   endif
else ifneq (,$(findstring armv,$(platform)))
   override platform += unix
else ifneq (,$(findstring rpi,$(platform)))
   override platform += unix
endif

MAIN_FBA_DIR := src
FBA_BURN_DIR := $(MAIN_FBA_DIR)/burn
FBA_BURN_DRIVERS_DIR := $(MAIN_FBA_DIR)/burn/drv
FBA_BURNER_DIR := $(MAIN_FBA_DIR)/burner
LIBRETRO_DIR := $(FBA_BURNER_DIR)/libretro
FBA_CPU_DIR := $(MAIN_FBA_DIR)/cpu
FBA_LIB_DIR := $(MAIN_FBA_DIR)/dep/libs
FBA_INTERFACE_DIR := $(MAIN_FBA_DIR)/intf
FBA_GENERATED_DIR = $(MAIN_FBA_DIR)/dep/generated

EXTERNAL_ZLIB = 0
7Z_SUPPORT = 0

TARGET_NAME := fbalpha2012_neogeo
BURN_BLACKLIST :=
FBA_LIBRETRO_DIRS := $(LIBRETRO_DIR)

ifneq (,$(findstring unix,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,-no-undefined -Wl,--version-script=$(LIBRETRO_DIR)/link.T
   
   # Raspberry Pi
   ifneq (,$(findstring rpi,$(platform)))
      PLATFORM_DEFINES += -fomit-frame-pointer -ffast-math
      PLATFORM_DEFINES += -DARM
      CXXFLAGS += -fno-rtti -fno-exceptions
      ifneq (,$(findstring rpi2,$(platform)))
         PLATFORM_DEFINES += -marm -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
      else ifneq (,$(findstring rpi3,$(platform)))
         PLATFORM_DEFINES += -marm -mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard
      endif
   endif
   
else ifeq ($(platform), osx)
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib

# iOS
else ifneq (,$(findstring ios,$(platform)))

   TARGET := $(TARGET_NAME)_libretro_ios.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   
   CC = cc -arch armv7 -isysroot $(IOSSDK)
   CXX = c++ -arch armv7 -isysroot $(IOSSDK)
   CFLAGS += -DIOS
ifeq ($(platform),ios9)
	CC += -miphoneos-version-min=8.0
	CXX +=  -miphoneos-version-min=8.0
	CFLAGS += -miphoneos-version-min=8.0
else
	CC += -miphoneos-version-min=5.0
	CXX +=  -miphoneos-version-min=5.0
	CFLAGS += -miphoneos-version-min=5.0
endif

# QNX
else ifeq ($(platform), qnx)
   TARGET := $(TARGET_NAME)_libretro_$(platform).so
   fpic := -fPIC
   SHARED := -lcpp -lm -shared -Wl,-no-undefined -Wl,--version-script=$(LIBRETRO_DIR)/link.T

	CC = qcc -Vgcc_ntoarmv7le
	CXX = QCC -Vgcc_ntoarmv7le_cpp
	AR = qcc -Vgcc_ntoarmv7le
	PLATFORM_DEFINES := -D__BLACKBERRY_QNX__ -marm -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=softfp

# PS3
else ifeq ($(platform), ps3)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
   CXX = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-g++.exe
   AR = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-ar.exe
   ENDIANNESS_DEFINES =  -DMSB_FIRST
   PLATFORM_DEFINES += -D__CELLOS_LV2__
   EXTERNAL_ZLIB = 1
	STATIC_LINKING = 1
	
# sncps3
else ifeq ($(platform), sncps3)
   TARGET := $(TARGET_NAME)_libretro_ps3.a
   CXX	= $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
   CC = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
   AR = $(CELL_SDK)/host-win32/sn/bin/ps3snarl.exe
   ENDIANNESS_DEFINES =  -DMSB_FIRST
   PLATFORM_DEFINES += -D__CELLOS_LV2__ -DSN_TARGET_PS3
   EXTERNAL_ZLIB = 1
	STATIC_LINKING = 1

# PS Vita
else ifeq ($(platform), vita)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = arm-vita-eabi-gcc$(EXE_EXT)
	CC_AS = arm-vita-eabi-gcc$(EXE_EXT)
	CXX = arm-vita-eabi-g++$(EXE_EXT)
	AR = arm-vita-eabi-ar$(EXE_EXT)
   PLATFORM_DEFINES += -DVITA
   EXTERNAL_ZLIB = 1
	STATIC_LINKING = 1
   CFLAGS += -O3 -mfloat-abi=hard -ffast-math -fsingle-precision-constant
   CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
   CPU_ARCH := arm

# psl1ght
else ifeq ($(platform), psl1ght)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
   CXX = $(PS3DEV)/ppu/bin/ppu-g++$(EXE_EXT)
   AR = $(PS3DEV)/ppu/bin/ppu-ar$(EXE_EXT)
   ENDIANNESS_DEFINES =  -DMSB_FIRST
   PLATFORM_DEFINES += -D__CELLOS_LV2__
   EXTERNAL_ZLIB = 1
	STATIC_LINKING = 1
	
# Xbox 360
else ifeq ($(platform), xenon)
   TARGET := $(TARGET_NAME)_libretro_xenon360.a
   CC = xenon-gcc$(EXE_EXT)
   CXX = xenon-g++$(EXE_EXT)
   AR = xenon-ar$(EXE_EXT)
   ENDIANNESS_DEFINES =  -DMSB_FIRST
   PLATFORM_DEFINES := -D__LIBXENON__ -m32 -D__ppc__
	STATIC_LINKING = 1

# NGC
else ifeq ($(platform), ngc)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   ENDIANNESS_DEFINES =  -DMSB_FIRST
   PLATFORM_DEFINES := -DGEKKO -DHW_DOL -mrvl -mcpu=750 -meabi -mhard-float
   PLATFORM_DEFINES += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
   EXTERNAL_ZLIB = 1
	STATIC_LINKING = 1
	
# Wii
else ifeq ($(platform), wii)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   ENDIANNESS_DEFINES =  -DMSB_FIRST
   PLATFORM_DEFINES := -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float
   PLATFORM_DEFINES += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
   EXTERNAL_ZLIB = 1
   STATIC_LINKING = 1

# Wii U
else ifeq ($(platform), wiiu)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   ENDIANNESS_DEFINES =  -DMSB_FIRST
   PLATFORM_DEFINES := -DGEKKO -DWIIU -DHW_RVL -mwup -mcpu=750 -meabi -mhard-float
   PLATFORM_DEFINES += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
   EXTERNAL_ZLIB = 1
   STATIC_LINKING = 1

# 3DS
else ifeq ($(platform), ctr)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   EXTERNAL_ZLIB = 1
   CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITARM)/bin/arm-none-eabi-g++$(EXE_EXT)
   AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
   PLATFORM_DEFINES += -DARM11 -D_3DS
   PLATFORM_DEFINES += -march=armv6k -mtune=mpcore -mfloat-abi=hard
   PLATFORM_DEFINES += -Wall -mword-relocations
   PLATFORM_DEFINES += -fomit-frame-pointer -ffast-math
   CXXFLAGS += -fno-rtti -fno-exceptions -std=gnu++11
   STATIC_LINKING = 1
   BURN_BLACKLIST += $(FBA_BURN_DIR)/burn_memory.c
   FBA_LIBRETRO_DIRS += $(LIBRETRO_DIR)/3ds

# Emscripten
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_$(platform).bc
   PLATFORM_DEFINES := -DUSE_FILE32API
   ENDIANNESS_DEFINES := -DNO_UNALIGNED_MEM
   EXTERNAL_ZLIB = 1
	STATIC_LINKING = 1

# GCW0
else ifeq ($(platform), gcw0)
   TARGET := $(TARGET_NAME)_libretro.so
   CC = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
   CXX = /opt/gcw0-toolchain/usr/bin/mipsel-linux-g++
   AR = /opt/gcw0-toolchain/usr/bin/mipsel-linux-ar
   fpic := -fPIC
   SHARED := -shared -Wl,-no-undefined -Wl,--version-script=$(LIBRETRO_DIR)/link.T
   LDFLAGS += $(PTHREAD_FLAGS)
   CFLAGS += $(PTHREAD_FLAGS) -DHAVE_MKDIR
   CFLAGS += -ffast-math -march=mips32 -mtune=mips32r2 -mhard-float
   CXXFLAGS += -ffast-math -march=mips32 -mtune=mips32r2 -mhard-float

# MIYOOMINI
else ifeq ($(platform),miyoomini)
	TARGET := $(TARGET_NAME)_libretro.so
	CC = $(CROSS_COMPILE)gcc
	CPP = $(CROSS_COMPILE)gcc -E
	CXX = $(CROSS_COMPILE)g++
	AR = $(CROSS_COMPILE)ar
	fpic := -fPIC
	SHARED := -shared -Wl,--version-script=$(LIBRETRO_DIR)/link.T -Wl,--no-undefined
	CFLAGS += -flto=4 -fipa-pta -fipa-ra -fwhole-program -fuse-linker-plugin
	CFLAGS += -falign-functions=1 -falign-jumps=1 -falign-loops=1
	CFLAGS += -fno-stack-protector -fno-ident -fomit-frame-pointer
	CFLAGS += -fno-unwind-tables -fno-asynchronous-unwind-tables
	CFLAGS += -fmerge-all-constants -fno-math-errno -ffast-math 
	CFLAGS += -ftree-vectorize -funswitch-loops -funroll-loops -fno-common
	CFLAGS += -fdata-sections -ffunction-sections -Wl,-s -Wl,--gc-sections
	CFLAGS += -marm -march=armv7ve+simd -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
	CFLAGS += -D_GNU_SOURCE
	CXXFLAGS += $(CFLAGS) -fno-exceptions -fno-rtti -std=c++98
	LDFLAGS += -flto=4 -fipa-pta -fipa-ra -fuse-linker-plugin
	LDFLAGS += -lz -Wl,-s -Wl,--gc-sections 
	ARCH = arm
	EXTERNAL_ZLIB = 1

# Windows
else
   TARGET := $(TARGET_NAME)_libretro.dll
   CC = gcc
   CXX = g++
   SHARED := -shared -Wl,-no-undefined -Wl,--version-script=$(LIBRETRO_DIR)/link.T
   LDFLAGS += -static-libgcc -static-libstdc++
endif

.PHONY: clean clean-objs

all: $(TARGET)

NEOGEO_DIR := $(FBA_BURN_DRIVERS_DIR)/neogeo

FBA_BURN_DIRS := $(FBA_BURN_DIR) \
	$(FBA_BURN_DIR)/devices \
	$(FBA_BURN_DIR)/snd \
	$(NEOGEO_DIR) \
	$(FBA_BURN_DRIVERS_DIR)

FBA_CPU_DIRS := $(FBA_CPU_DIR) \
	$(FBA_CPU_DIR)/m68k \
	$(FBA_CPU_DIR)/z80

FBA_DEFINES := -DUSE_SPEEDHACKS -D__LIBRETRO__ \
	-D__LIBRETRO_OPTIMIZATIONS__ \
	-DWANT_NEOGEOCD \
	$(ENDIANNESS_DEFINES) \
	$(PLATFORM_DEFINES)

ifneq ($(EXTERNAL_ZLIB), 1)
FBA_LIB_DIRS += $(FBA_LIB_DIR)/zlib
endif

ifeq ($(7Z_SUPPORT), 1)
FBA_DEFINES += -DINCLUDE_7Z_SUPPORT
else
BURN_BLACKLIST += $(FBA_BURNER_DIR)/un7z.cpp
endif

FBA_SRC_DIRS := $(FBA_BURNER_DIR) $(FBA_BURN_DIRS) $(FBA_CPU_DIRS) $(FBA_BURNER_DIRS) $(FBA_LIBRETRO_DIRS) $(FBA_LIB_DIRS)

FBA_CXXSRCS := $(filter-out $(BURN_BLACKLIST),$(foreach dir,$(FBA_SRC_DIRS),$(wildcard $(dir)/*.cpp)))
FBA_CXXOBJ := $(FBA_CXXSRCS:.cpp=.o)
FBA_CSRCS := $(filter-out $(BURN_BLACKLIST),$(foreach dir,$(FBA_SRC_DIRS),$(wildcard $(dir)/*.c)))
FBA_COBJ := $(FBA_CSRCS:.c=.o)

OBJS := $(FBA_COBJ) $(FBA_CXXOBJ)

ifneq ($(platform),qnx)
   FBA_DEFINES += -DINLINE="static inline"
endif

INCDIRS := -I$(LIBRETRO_DIR) \
	-I$(FBA_BURN_DIR) \
	-I$(MAIN_FBA_DIR)/cpu \
	-I$(FBA_BURN_DIR)/snd \
	-I$(FBA_BURN_DIR)/devices \
	-I$(FBA_INTERFACE_DIR) \
	-I$(FBA_INTERFACE_DIR)/input \
	-I$(FBA_INTERFACE_DIR)/cd \
	-I$(FBA_BURNER_DIR) \
	-I$(FBA_CPU_DIR) \
	-I$(FBA_LIB_DIR)/zlib \
	-I$(FBA_GENERATED_DIR) \
	-I$(FBA_LIB_DIR) \
	-I$(NEOGEO_DIR)

ifeq ($(LIBRETRO_OPTIMIZATIONS), 1)
FBA_DEFINES += -D__LIBRETRO_OPTIMIZATIONS__ 
endif

ifneq ($(platform), sncps3)
CFLAGS += -std=gnu99
endif

ifeq ($(DEBUG), 1)
CFLAGS += -O0 -g
CXXFLAGS += -O0 -g
else ifeq ($(platform), emscripten)
CFLAGS += -O2 -DNDEBUG
CXXFLAGS += -O2 -DNDEBUG
else
CFLAGS += -O3 -DNDEBUG
CXXFLAGS += -O3 -DNDEBUG
endif

ifeq ($(platform), sncps3)
WARNINGS_DEFINES =
else
WARNINGS_DEFINES = -Wno-write-strings
endif

$(info    FBA_DEFINES is $(FBA_DEFINES))

CFLAGS += $(fpic) $(WARNINGS_DEFINES) $(FBA_DEFINES)
CXXFLAGS += $(fpic) $(WARNINGS_DEFINES) $(FBA_DEFINES)
LDFLAGS += $(fpic)

ifeq ($(FRONTEND_SUPPORTS_RGB565), 1)
CFLAGS += -DFRONTEND_SUPPORTS_RGB565
CXXFLAGS += -DFRONTEND_SUPPORTS_RGB565
endif

$(TARGET): $(OBJS)
	@echo "LD $@"
ifeq ($(STATIC_LINKING), 1)
	@$(AR) rcs $@ $(OBJS)
else
	@$(CXX) -o $@ $(SHARED) $(OBJS) $(LDFLAGS)
endif

%.o: %.cpp
	@echo "CXX $<"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(INCDIRS)

%.o: %.c
	@echo "CC $<"
	@$(CC) -c -o $@ $< $(CFLAGS) $(INCDIRS)

clean-objs:
	rm -f $(OBJS)

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)
