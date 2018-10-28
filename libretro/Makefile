DEBUG = 0
DEBUG_ASAN = 0
DEBUG_UBSAN = 0
HAVE_SSE = 0
HAVE_GL_DEBUG = 0

ifeq ($(platform),)
	platform = unix
	ifeq ($(shell uname -a),)
		platform = win
	else ifneq ($(findstring Darwin,$(shell uname -a)),)
		platform = osx
	else ifneq ($(findstring MINGW,$(shell uname -a)),)
		platform = win
	endif
endif

# system platform
ifeq ($(platform),)
   platform = unix
   ifeq ($(shell uname -a),)
      platform = win
      EXE_EXT=.exe
   else ifneq ($(findstring Darwin,$(shell uname -a)),)
      platform = osx
      arch = intel
      ifeq ($(shell uname -p),powerpc)
         arch = ppc
      endif
   else ifneq ($(findstring MINGW,$(shell uname -a)),)
      platform = win
      EXE_EXT=.exe
   endif
else ifneq (,$(findstring armv,$(platform)))
   override platform += unix
endif

CORE_DIR := .

TARGET_NAME = kronos
CC_AS ?= $(CC)
GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

# Unix
ifneq (,$(findstring unix,$(platform)))
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -lpthread -lGL -lGLESv2
	FLAGS += -DARCH_IS_LINUX=1
	# ARM
	ifneq (,$(findstring armv,$(platform)))
		ifneq (,$(findstring cortexa8,$(platform)))
			FLAGS += -marm -mcpu=cortex-a8
		else ifneq (,$(findstring cortexa9,$(platform)))
			FLAGS += -marm -mcpu=cortex-a9
		endif
		FLAGS += -marm
		ifneq (,$(findstring neon,$(platform)))
			FLAGS += -mfpu=neon
			HAVE_NEON = 1
		endif
		ifneq (,$(findstring softfloat,$(platform)))
			FLAGS += -mfloat-abi=softfp
		else ifneq (,$(findstring hardfloat,$(platform)))
			FLAGS += -mfloat-abi=hard
		endif
		FLAGS += -DARM
	endif

# OS X
else ifneq (,$(findstring osx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro.dylib
	fpic := -fPIC
OSXVER = $(shell sw_vers -productVersion | cut -d. -f 2)
	OSX_GT_MOJAVE = $(shell (( $(OSXVER) >= 14)) && echo "YES")
ifneq ($(OSX_GT_MOJAVE),YES)
	#this breaks compiling on Mac OS Mojave
	fpic += -mmacosx-version-min=10.1
endif
	FLAGS += -DARCH_IS_MACOSX=1
	SHARED := -dynamiclib
	FLAGS += -DARCH_IS_MACOSX=1
ifeq ($(arch),ppc)
	ENDIANNESS_DEFINES := -DWORDS_BIGENDIAN -DMSB_FIRST -D__ppc__
endif
	LDFLAGS += -lpthread -lGL -lGLESv2

else ifneq (,$(findstring ios,$(platform)))
	# iOS
	TARGET := $(TARGET_NAME)_libretro_ios.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
	CC = clang -arch armv7 -isysroot $(IOSSDK)
	CC_AS =  perl ./libretro/gas-preprocessor.pl $(CC)
	CXX = clang++ -arch armv7 -isysroot $(IOSSDK)

# sncps3
else ifneq (,$(findstring sncps3,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_ps3.a
	CC = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
	CXX = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
	AR = $(CELL_SDK)/host-win32/sn/bin/ps3snarl.exe
	ENDIANNESS_DEFINES = -DWORDS_BIGENDIAN -DMSB_FIRST -D__ppc__
	STATIC_LINKING = 1

# Lightweight PS3 Homebrew SDK
else ifneq (,$(findstring psl1ght,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
	CC = $(PS3DEV)/ppu/bin/ppu-g++$(EXE_EXT)
	AR = $(PS3DEV)/ppu/bin/ppu-ar$(EXE_EXT)
	ENDIANNESS_DEFINES = -DWORDS_BIGENDIAN -DMSB_FIRST -D__ppc__
	STATIC_LINKING = 1

# Vita
else ifneq (,$(findstring vita,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = arm-vita-eabi-gcc$(EXE_EXT)
	CCX = arm-vita-eabi-g++$(EXE_EXT)
	CC_AS = arm-vita-eabi-gcc$(EXE_EXT)
	AR = arm-vita-eabi-ar$(EXE_EXT)
	ENDIANNESS_DEFINES += -DVITA
	STATIC_LINKING = 1

else ifeq ($(platform), switch)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	include $(LIBTRANSISTOR_HOME)/libtransistor.mk
	STATIC_LINKING=1

# Emscripten
else ifeq ($(platform), emscripten)
	TARGET := $(TARGET_NAME)_libretro_$(platform).bc
	STATIC_LINKING = 1

# GCW0
else ifeq ($(platform), gcw0)
	TARGET := $(TARGET_NAME)_libretro.so
	CC = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
	CXX = /opt/gcw0-toolchain/usr/bin/mipsel-linux-g++
	AR = /opt/gcw0-toolchain/usr/bin/mipsel-linux-ar
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += $(PTHREAD_FLAGS) -lrt
	FLAGS += $(PTHREAD_FLAGS) -DHAVE_MKDIR
	FLAGS += -ffast-math -march=mips32 -mtune=mips32r2 -mhard-float
	fpic := -fPIC
	CFLAGS += -DFAMEC_NO_GOTOS

# Windows
else
	TARGET := $(TARGET_NAME)_libretro.dll
	CC = gcc
	CXX = g++
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -static-libgcc -static-libstdc++ -lwinmm -lopengl32
	FLAGS += -DHAVE_WFOPEN -D_WINDOWS

endif

include Makefile.common

ifeq ($(HAVE_GL_DEBUG),1)
FLAGS += -D__USE_OPENGL_DEBUG__
endif

ifeq ($(HAVE_SSE),1)
FLAGS += -mfpmath=sse
endif

OBJECTS += $(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o) $(C68KEXEC_OBJECT)

ifeq ($(DEBUG_ASAN), 1)
   DEBUG = 1
   DEBUG_UBSAN = 0
   FLAGS += -lasan -fsanitize=address
   LDFLAGS += -lasan -fsanitize=address
endif

ifeq ($(DEBUG_UBSAN), 1)
   DEBUG = 1
   FLAGS += -lubsan -fsanitize=undefined
   LDFLAGS += -lubsan -fsanitize=undefined
endif

ifeq ($(DEBUG), 1)
FLAGS += -O0 -g
else
FLAGS += -O3 -DNDEBUG
endif

LDFLAGS += $(fpic) $(SHARED)
FLAGS += $(fpic)

WARNINGS :=
FLAGS += $(INCFLAGS) -D__LIBRETRO__ $(ENDIANNESS_DEFINES) -DSIZEOF_DOUBLE=8 $(WARNINGS) -DUSE_16BPP=1 -DUSE_RGB_565=1 \
	-DNO_CLI -DDYNAREC_KRONOS=1 -DHAVE_BUILTIN_BSWAP16=1 -DHAVE_BUILTIN_BSWAP32=1 -DHAVE_C99_VARIADIC_MACROS=1 \
	-DHAVE_FLOORF=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_MUSASHI=1 -DHAVE_STDINT_H=1 -DHAVE_SYS_TIME_H=1 -DIMPROVED_SAVESTATES \
	-DPACKAGE=\"Kronos\" -DSPRITE_CACHE=1 -DHAVE_LIBGL -D_OGLES3_ -DCELL_ASYNC=1 -DRGB_ASYNC=1 -DVDP1_TEXTURE_ASYNC=1 -DHAVE_THREADS=1 \
	-DVERSION=\"1.4.5\"

CXXFLAGS += $(FLAGS)

CFLAGS += $(FLAGS)
CFLAGS += -std=gnu99

all: $(TARGET)

generate-files:
	cp $(SOURCE_DIR)/zlib/zconf.h.in $(SOURCE_DIR)/zlib/zconf.h;\
	$(CC) -o $(SOURCE_DIR)/musashi/$(M68KMAKE_EXE) $(SOURCE_DIR)/musashi/m68kmake.c;\
	$(SOURCE_DIR)/musashi/$(M68KMAKE_EXE) $(SOURCE_DIR)/musashi/ $(SOURCE_DIR)/musashi/m68k_in.c;\
	$(CC) -DC68K_GEN -o $(SOURCE_DIR)/c68k/$(GEN68K_EXE) $(SOURCE_DIR)/c68k/c68kexec.c $(SOURCE_DIR)/c68k/c68k.c $(SOURCE_DIR)/c68k/gen68k.c;\
	cd $(SOURCE_DIR)/c68k/; ./$(GEN68K_EXE)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CXX) -o $@ $^ $(LDFLAGS)
endif

%.o: %.S
	$(CC_AS) $(CFLAGS) -c $^ -o $@

%.o: %.s
	$(CC_AS) $(CFLAGS) -c $^ -o $@

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(C68KEXEC_OBJECT): $(C68KEXEC_SOURCE)
	$(CC) -c -o $@ $< $(CFLAGS) -O0

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: clean
