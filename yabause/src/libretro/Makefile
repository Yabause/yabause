DEBUG = 0
DEBUG_ASAN = 0
DEBUG_UBSAN = 0
DEBUG_TSAN = 0
HAVE_SSE = 1
__USE_OPENGL_DEBUG__ = 0
HAVE_MUSASHI = 1
FORCE_GLES = 0
FASTMATH = 1
UNROLL_LOOPS = 1
ARCH_IS_LINUX = 0
ARCH_IS_MACOSX = 0
ARCH_IS_WINDOWS = 0
FORCE_GLEW = 0
HAVE_SYS_PARAM_H = 1
HAVE_FSEEKO = 0
USE_RTHREADS = 0

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

# Unix
ifneq (,$(findstring unix,$(platform)))
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -lpthread -lGL
	ARCH_IS_LINUX = 1
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
		FORCE_GLES = 1
		HAVE_SSE = 0
		FLAGS += -DARM
	endif

# Jetson Nano
else ifeq ($(platform), jetson-nano)
	override platform += unix
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -lpthread -lGL
	ARCH_IS_LINUX = 1
	HAVE_SSE = 0
	FLAGS += -march=armv8-a+crc -mtune=cortex-a57

# Switch Lakka (arm64)
else ifeq ($(platform), arm64)
	override platform += unix
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -lpthread -lGL
	ARCH_IS_LINUX = 1
	HAVE_SSE = 0

# RockPro64
else ifneq (,$(findstring rockpro64,$(platform)))
	override platform += unix
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -lpthread -lGL
	ARCH_IS_LINUX = 1
	HAVE_SSE = 0
	FORCE_GLES = 1
	FLAGS += -march=armv8-a+crc -mcpu=cortex-a72 -mtune=cortex-a72.cortex-a53 -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad

# ODROIDs
else ifeq ($(platform), odroid-n2)
	override platform += unix
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -lpthread -lGL
	ARCH_IS_LINUX = 1
	HAVE_SSE = 0
	FORCE_GLES = 1
	# ODROID-N2
	ifneq (, $(findstring clang,$(CC)))
		FLAGS += -march=armv8-a+crc
	else
		FLAGS += -march=armv8-a+crc -mtune=cortex-a73.cortex-a53
	endif

else ifneq (,$(findstring odroid,$(platform)))
	override platform += unix
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -lpthread -lGL
	ARCH_IS_LINUX = 1
	BOARD := $(shell cat /proc/cpuinfo | grep -i odroid | awk '{print $$3}')

	ifneq (,$(findstring ODROID-XU3,$(BOARD)))
		HAVE_SSE = 0
		FORCE_GLES = 1
		# ODROID-XU3 & -XU3 Lite
		ifeq "$(shell expr `gcc -dumpversion` \>= 4.9)" "1"
			FLAGS += -marm -mfloat-abi=hard -mcpu=cortex-a15 -mtune=cortex-a15.cortex-a7 -mfpu=neon-vfpv4 -mvectorize-with-neon-quad
		else
			FLAGS += -marm -mfloat-abi=hard -mcpu=cortex-a9 -mfpu=neon
		endif
	else ifneq (,$(findstring ODROID-XU4,$(BOARD)))
		HAVE_SSE = 0
		FORCE_GLES = 1
		# ODROID-XU4 on newer kernels now identify as ODROID-XU4
		ifeq "$(shell expr `gcc -dumpversion` \>= 4.9)" "1"
			FLAGS += -marm -mfloat-abi=hard -mcpu=cortex-a15 -mtune=cortex-a15.cortex-a7 -mfpu=neon-vfpv4 -mvectorize-with-neon-quad
		else
			FLAGS += -marm -mfloat-abi=hard -mcpu=cortex-a9 -mfpu=neon
		endif
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
	ARCH_IS_MACOSX = 1
	SHARED := -dynamiclib
	ifeq ($(arch),ppc)
		ENDIANNESS_DEFINES := -DWORDS_BIGENDIAN -DMSB_FIRST -D__ppc__
	endif
	LDFLAGS += -lpthread -framework OpenGL

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
	CC ?= gcc
	CXX ?= g++
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -static-libgcc -static-libstdc++ -lwinmm -lopengl32
	FLAGS += -D_WINDOWS
	HAVE_FSEEKO = 1
	ARCH_IS_WINDOWS = 1

endif

include Makefile.common

ifeq ($(__USE_OPENGL_DEBUG__),1)
	FLAGS += -D__USE_OPENGL_DEBUG__
endif

ifeq ($(HAVE_SSE),1)
	FLAGS += -mfpmath=sse
endif

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

ifeq ($(DEBUG_TSAN), 1)
	DEBUG = 1
	FLAGS += -ltsan -fsanitize=thread
	LDFLAGS += -ltsan -fsanitize=thread
endif

ifeq ($(DEBUG), 1)
	FLAGS += -O0 -g
else
	FLAGS += -O3 -DNDEBUG
endif

OBJECTS += $(OBJECTS_C) $(OBJECTS_CXX) $(OBJECTS_S)

LDFLAGS += $(fpic) $(SHARED)
FLAGS += $(fpic)

INCFLAGS := $(foreach dir,$(INCLUDE_DIRS),-I$(dir))

WARNINGS :=
FLAGS += $(INCFLAGS) $(ENDIANNESS_DEFINES) $(WARNINGS)

CXXFLAGS += $(FLAGS) -std=gnu++11
CFLAGS += $(FLAGS) -std=gnu11

all: $(TARGET)

generate-files:
ifeq ($(HAVE_MUSASHI), 1)
	$(CC) -o $(SOURCE_DIR)/musashi/$(M68KMAKE_EXE) $(SOURCE_DIR)/musashi/m68kmake.c;\
	$(SOURCE_DIR)/musashi/$(M68KMAKE_EXE) $(SOURCE_DIR)/musashi/ $(SOURCE_DIR)/musashi/m68k_in.c
else
	$(CC) -DC68K_GEN -o $(SOURCE_DIR)/c68k/$(GEN68K_EXE) $(SOURCE_DIR)/c68k/c68kexec.c $(SOURCE_DIR)/c68k/c68k.c $(SOURCE_DIR)/c68k/gen68k.c;\
	cd $(SOURCE_DIR)/c68k/; ./$(GEN68K_EXE)
endif

generate-files-clean:
	rm -f $(M68KMAKE_INC_SOURCES) $(SOURCE_DIR)/musashi/$(M68KMAKE_EXE) $(GEN68K_INC_SOURCES) $(SOURCE_DIR)/c68k/$(GEN68K_EXE)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CXX) -o $@ $^ $(LDFLAGS)
endif

%.S.o: %.S
	$(CC_AS) $(CFLAGS) -c $^ -o $@

%.s.o: %.s
	$(CC_AS) $(CFLAGS) -c $^ -o $@

%.cpp.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

%.c.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(C68KEXEC_OBJECT): $(C68KEXEC_SOURCE)
	$(CC) -c -o $@ $< $(CFLAGS) -O0

clean:
	rm -f $(TARGET) $(OBJECTS) $(SOURCE_DIR)/musashi/$(M68KMAKE_EXE) $(SOURCE_DIR)/c68k/$(GEN68K_EXE)

.PHONY: clean
