DEBUG = 0
DEBUG_ASAN = 0
DEBUG_UBSAN = 0
DEBUG_TSAN = 0
HAVE_SSE = 1
FASTMATH = 1
DYNAREC = 0
HAVE_GRIFFIN = 0
HAVE_THREADS = 1
HAVE_MUSASHI = 1
HAVE_UWP = 0
HAVE_SYS_PARAM_H = 1
HAVE_FSEEKO = 0
ARCH_IS_LINUX = 0
ARCH_IS_MACOSX = 0
ARCH_IS_WINDOWS = 0
# The following is broken upstream ?
USE_PLAY_JIT = 0
USE_SCSP2 = 0
# debug tools, unneeded ?
ENABLE_TSUNAMI = 0

SPACE :=
SPACE := $(SPACE) $(SPACE)
BACKSLASH :=
BACKSLASH := \$(BACKSLASH)
filter_out1 = $(filter-out $(firstword $1),$1)
filter_out2 = $(call filter_out1,$(call filter_out1,$1))

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
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
	ifeq ($(shell uname -p),powerpc)
		arch = ppc
	endif
	ifeq ($(shell uname -p),arm)
		arch = arm64
	endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

ifneq (,$(findstring armv,$(platform)))
	override platform += unix
endif

CORE_DIR := .

TARGET_NAME = yabause
CC_AS ?= $(CC)
GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	FLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

# Unix
ifneq (,$(findstring unix,$(platform)))
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	ifneq ($(findstring SunOS,$(shell uname -a)),)
		SHARED := -shared -z defs
	else
		SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	endif
	#DYNAREC = 2
	ifneq (,$(findstring Haiku,$(shell uname -s)))
		LDFLAGS += -lroot
	else
		FLAGS += -pthread
		LDFLAGS += -pthread
		ARCH_IS_LINUX = 1
	endif

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
		HAVE_SSE = 0
		FLAGS += -DARM
	endif

else ifneq (,$(findstring linux-portable,$(platform)))
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC -nostdlib
	SHARED := -shared -Wl,--version-script=link.T
	#DYNAREC = 2
	FLAGS += -pthread
	LDFLAGS += -pthread
	ARCH_IS_LINUX = 1

# OS X
else ifneq (,$(findstring osx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro.dylib
	fpic := -fPIC
	MACSOSVER = $(shell sw_vers -productVersion | cut -d. -f 1)
	OSXVER = $(shell sw_vers -productVersion | cut -d. -f 2)
	OSX_LT_MOJAVE = $(shell (( $(MACSOSVER) < 11)) && (( $(OSXVER) < 14)) && echo "YES")
ifeq ($(OSX_LT_MOJAVE),YES)
	#this breaks compiling on Mac OS Mojave
	fpic += -mmacosx-version-min=10.1
endif
	SHARED := -dynamiclib
ifeq ($(arch),ppc)
	ENDIANNESS_DEFINES := -DWORDS_BIGENDIAN -DMSB_FIRST -D__ppc__
	HAVE_SSE = 0
endif
ifeq ($(shell uname -p),arm)
	HAVE_SSE = 0
endif
	#DYNAREC = 2
	FLAGS += -pthread
	LDFLAGS += -pthread
	ARCH_IS_MACOSX = 1

   ifeq ($(CROSS_COMPILE),1)
		TARGET_RULE   = -target $(LIBRETRO_APPLE_PLATFORM) -isysroot $(LIBRETRO_APPLE_ISYSROOT)
		CFLAGS   += $(TARGET_RULE)
		CPPFLAGS += $(TARGET_RULE)
		CXXFLAGS += $(TARGET_RULE)
		LDFLAGS  += $(TARGET_RULE)
   endif

	CFLAGS  += $(ARCHFLAGS)
	CXXFLAGS  += $(ARCHFLAGS)
	LDFLAGS += $(ARCHFLAGS)

else ifneq (,$(findstring ios,$(platform)))
	# iOS
	TARGET := $(TARGET_NAME)_libretro_ios.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
        MINVERSION :=
ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif
	CC_AS =  perl ./libretro/gas-preprocessor.pl $(CC)
	ifeq ($(platform), ios-arm64)
	  CC = cc -arch arm64 -isysroot $(IOSSDK)
	  CXX = c++ -arch arm64 -isysroot $(IOSSDK)
        else
	  CC = cc -arch armv7 -isysroot $(IOSSDK)
	  CXX = c++ -arch armv7 -isysroot $(IOSSDK)
	endif
ifeq ($(platform),$(filter $(platform),ios9 ios-arm64))
        MINVERSION = -miphoneos-version-min=8.0
else
        MINVERSION = -miphoneos-version-min=5.0
endif
        CFLAGS += $(MINVERSION)
        CXXFLAGS += $(MINVERSION)

	HAVE_SSE = 0

else ifeq ($(platform), tvos-arm64)
	# tvOS
	TARGET := $(TARGET_NAME)_libretro_tvos.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk appletvos Path)
endif
	CC_AS =  perl ./libretro/gas-preprocessor.pl $(CC)
	HAVE_SSE = 0

        CC = cc -arch arm64 -isysroot $(IOSSDK)
        CXX = c++ -arch arm64 -isysroot $(IOSSDK)

# Classic Platforms ####################
# Platform affix = classic_<ISA>_<µARCH>
# Help at https://modmyclassic.com/comp

# (armv7 a7, hard point, neon based) ### 
# NESC, SNESC, C64 mini 
else ifeq ($(platform), classic_armv7_a7)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	CFLAGS += -pthread
	CFLAGS += -DARM -Ofast \
	-flto=4 -fwhole-program -fuse-linker-plugin \
	-fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-stack-protector -fno-ident -fomit-frame-pointer \
	-falign-functions=1 -falign-jumps=1 -falign-loops=1 \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-unroll-loops \
	-fmerge-all-constants -fno-math-errno \
	-marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
	CXXFLAGS += $(CFLAGS)
	HAVE_PTHREADS = 1
	HAVE_NEON = 1
	ARCH = arm
	LDFLAGS += -pthread
	ifeq ($(shell echo `$(CC) -dumpversion` "< 4.9" | bc -l), 1)
	  CFLAGS += -march=armv7-a
	else
	  CFLAGS += -march=armv7ve
	  # If gcc is 5.0 or later
	  ifeq ($(shell echo `$(CC) -dumpversion` ">= 5" | bc -l), 1)
	    LDFLAGS += -static-libgcc -static-libstdc++
	  endif
	endif
	
# (armv8 a35, hard point, neon based) ###
# Playstation Classic
else ifeq ($(platform), classic_armv8_a35)
        TARGET := $(TARGET_NAME)_libretro.so
        fpic := -fPIC
        SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
        CFLAGS += -pthread
        CFLAGS += -DARM -Ofast \
        -flto -fwhole-program -fuse-linker-plugin \
        -fdata-sections -ffunction-sections -Wl,--gc-sections \
        -fno-stack-protector -fno-ident -fomit-frame-pointer \
        -falign-functions=1 -falign-jumps=1 -falign-loops=1 \
        -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-unroll-loops \
        -fmerge-all-constants -fno-math-errno \
        -marm -mtune=cortex-a35 -mfpu=neon-fp-armv8 -mfloat-abi=hard
        CXXFLAGS += $(CFLAGS)
        HAVE_PTHREADS = 1
        HAVE_NEON = 1
        FLAGS += -DARM
        ARCH = arm
        ARCH_IS_LINUX = 1
        LDFLAGS += -pthread
        CFLAGS += -march=armv8-a
        LDFLAGS += -static-libgcc -static-libstdc++
#######################################

# QNX
else ifneq (,$(findstring qnx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_$(platform).so
	fpic := -fPIC
	SHARED := -shared -Wl,--version-script=link.T
	CC = qcc -Vgcc_ntoarmv7le
	CC_AS = $(CC)
	CXX = QCC -Vgcc_ntoarmv7le
	AR = qcc -Vgcc_ntoarmv7le
	ENDIANNESS_DEFINES += -D__BLACKBERRY_QNX__ -marm -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=softfp -fms-extensions
	HAVE_SSE = 0

# PS3
else ifneq (,$(findstring ps3,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
	CXX = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-g++.exe
	AR = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-ar.exe
	ENDIANNESS_DEFINES = -DWORDS_BIGENDIAN -DMSB_FIRST -D__ppc__
	STATIC_LINKING = 1
	HAVE_SSE = 0

# sncps3
else ifneq (,$(findstring sncps3,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_ps3.a
	CC = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
	CXX = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe $(SOURCE_DIR)/musashi/$(M68KMAKE_EXE) $(SOURCE_DIR)/c68k/$(GEN68K_EXE)
	AR = $(CELL_SDK)/host-win32/sn/bin/ps3snarl.exe
	ENDIANNESS_DEFINES = -DWORDS_BIGENDIAN -DMSB_FIRST -D__ppc__
	STATIC_LINKING = 1
	HAVE_SSE = 0

# Lightweight PS3 Homebrew SDK
else ifneq (,$(findstring psl1ght,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
	CC = $(PS3DEV)/ppu/bin/ppu-g++$(EXE_EXT)
	AR = $(PS3DEV)/ppu/bin/ppu-ar$(EXE_EXT)
	ENDIANNESS_DEFINES = -DWORDS_BIGENDIAN -DMSB_FIRST -D__ppc__
	STATIC_LINKING = 1
	HAVE_SSE = 0

# PSP
else ifneq (,$(findstring psp1,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = psp-gcc$(EXE_EXT)
	CC_AS = $(CC)
	CXX = psp-g++$(EXE_EXT)
	AR = psp-ar$(EXE_EXT)
	ENDIANNESS_DEFINES += -DPSP -G0 
	STATIC_LINKING = 1
	HAVE_SSE = 0

# Vita
else ifneq (,$(findstring vita,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = arm-vita-eabi-gcc$(EXE_EXT)
	CCX = arm-vita-eabi-g++$(EXE_EXT)
	CC_AS = arm-vita-eabi-gcc$(EXE_EXT)
	AR = arm-vita-eabi-ar$(EXE_EXT)
	ENDIANNESS_DEFINES += -DVITA
	#DYNAREC = 2
	STATIC_LINKING = 1
	HAVE_SSE = 0

# Nintendo Game Cube
else ifneq (,$(findstring ngc,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	ENDIANNESS_DEFINES = -DWORDS_BIGENDIAN -DMSB_FIRST -DGEKKO -DHW_DOL -mrvl -mcpu=750 -meabi -mhard-float -D__ppc__
	STATIC_LINKING = 1
	HAVE_SSE = 0

# Nintendo Wii
else ifneq (,$(findstring wii,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	ENDIANNESS_DEFINES = -DWORDS_BIGENDIAN -DMSB_FIRST -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float -D__ppc__
	STATIC_LINKING = 1
	HAVE_SSE = 0

else ifeq ($(platform), switch)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	include $(LIBTRANSISTOR_HOME)/libtransistor.mk
	STATIC_LINKING=1
	HAVE_SSE = 0

# Nintendo Switch (libnx)
else ifeq ($(platform), libnx)
	include $(DEVKITPRO)/libnx/switch_rules
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	DEFINES := -DSWITCH=1 -U__linux__ -U__linux -DRARCH_INTERNAL
	CFLAGS	:=	 $(DEFINES) -g \
				-O2 \
				-fPIE -I$(LIBNX)/include/ -ffunction-sections -fdata-sections -ftls-model=local-exec -Wl,--allow-multiple-definition -specs=$(LIBNX)/switch.specs
	CFLAGS += $(INCDIRS)
	CFLAGS	+=	$(INCLUDE)  -D__SWITCH__ -DHAVE_LIBNX
	CXXFLAGS := $(ASFLAGS) $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
	CFLAGS += -std=gnu11
	STATIC_LINKING = 1
	HAVE_SSE = 0

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
	HAVE_SSE = 0

# Windows MSVC 2017 all architectures
else ifneq (,$(findstring windows_msvc2017,$(platform)))

    NO_GCC := 1

	PlatformSuffix = $(subst windows_msvc2017_,,$(platform))
	ifneq (,$(findstring desktop,$(PlatformSuffix)))
		WinPartition = desktop
		MSVC2017CompileFlags = -DWINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP -FS
		LDFLAGS += -MANIFEST -LTCG:incremental -NXCOMPAT -DYNAMICBASE -DEBUG -OPT:REF -INCREMENTAL:NO -SUBSYSTEM:WINDOWS -MANIFESTUAC:"level='asInvoker' uiAccess='false'" -OPT:ICF -ERRORREPORT:PROMPT -NOLOGO -TLBID:1
		LIBS += kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib
	else ifneq (,$(findstring uwp,$(PlatformSuffix)))
		WinPartition = uwp
		MSVC2017CompileFlags = -DWINAPI_FAMILY=WINAPI_FAMILY_APP -D_WINDLL -D_UNICODE -DUNICODE -D__WRL_NO_DEFAULT_LIB__ -EHsc -FS
		LDFLAGS += -APPCONTAINER -NXCOMPAT -DYNAMICBASE -MANIFEST:NO -LTCG -OPT:REF -SUBSYSTEM:CONSOLE -MANIFESTUAC:NO -OPT:ICF -ERRORREPORT:PROMPT -NOLOGO -TLBID:1 -DEBUG:FULL -WINMD:NO
		LIBS += WindowsApp.lib
		HAVE_UWP = 1
	endif

	MSVC2017CompileFlags += -DC68K_NO_JUMP_TABLE -DHAVE_C99_VARIADIC_MACROS -DWIN32
	FLAGS += $(MSVC2017CompileFlags)

	TargetArchMoniker = $(subst $(WinPartition)_,,$(PlatformSuffix))

	CC  = cl.exe
	CXX = cl.exe
	LD = link.exe

	reg_query = $(call filter_out2,$(subst $2,,$(shell reg query "$2" -v "$1" 2>nul)))
	fix_path = $(subst $(SPACE),\ ,$(subst \,/,$1))

	ProgramFiles86w := $(shell cmd //c "echo %PROGRAMFILES(x86)%")
	ProgramFiles86 := $(shell cygpath "$(ProgramFiles86w)")

	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_CURRENT_USER\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_CURRENT_USER\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir := $(WindowsSdkDir)

	WindowsSDKVersion ?= $(firstword $(foreach folder,$(subst $(subst \,/,$(WindowsSdkDir)Include/),,$(wildcard $(call fix_path,$(WindowsSdkDir)Include\*))),$(if $(wildcard $(call fix_path,$(WindowsSdkDir)Include/$(folder)/um/Windows.h)),$(folder),)))$(BACKSLASH)
	WindowsSDKVersion := $(WindowsSDKVersion)

	VsInstallBuildTools = $(ProgramFiles86)/Microsoft Visual Studio/2017/BuildTools
	VsInstallEnterprise = $(ProgramFiles86)/Microsoft Visual Studio/2017/Enterprise
	VsInstallProfessional = $(ProgramFiles86)/Microsoft Visual Studio/2017/Professional
	VsInstallCommunity = $(ProgramFiles86)/Microsoft Visual Studio/2017/Community

	VsInstallRoot ?= $(shell if [ -d "$(VsInstallBuildTools)" ]; then echo "$(VsInstallBuildTools)"; fi)
	ifeq ($(VsInstallRoot), )
		VsInstallRoot = $(shell if [ -d "$(VsInstallEnterprise)" ]; then echo "$(VsInstallEnterprise)"; fi)
	endif
	ifeq ($(VsInstallRoot), )
		VsInstallRoot = $(shell if [ -d "$(VsInstallProfessional)" ]; then echo "$(VsInstallProfessional)"; fi)
	endif
	ifeq ($(VsInstallRoot), )
		VsInstallRoot = $(shell if [ -d "$(VsInstallCommunity)" ]; then echo "$(VsInstallCommunity)"; fi)
	endif
	VsInstallRoot := $(VsInstallRoot)

	VcCompilerToolsVer := $(shell cat "$(VsInstallRoot)/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt" | grep -o '[0-9\.]*')
	VcCompilerToolsDir := $(VsInstallRoot)/VC/Tools/MSVC/$(VcCompilerToolsVer)

	WindowsSDKSharedIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\$(WindowsSDKVersion)\shared")
	WindowsSDKUCRTIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\$(WindowsSDKVersion)\ucrt")
	WindowsSDKUMIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\$(WindowsSDKVersion)\um")
	WindowsSDKUCRTLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib\$(WindowsSDKVersion)\ucrt\$(TargetArchMoniker)")
	WindowsSDKUMLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib\$(WindowsSDKVersion)\um\$(TargetArchMoniker)")

	# For some reason the HostX86 compiler doesn't like compiling for x64
	# ("no such file" opening a shared library), and vice-versa.
	# Work around it for now by using the strictly x86 compiler for x86, and x64 for x64.
	# NOTE: What about ARM?
	ifneq (,$(findstring x64,$(TargetArchMoniker)))
		VCCompilerToolsBinDir := $(VcCompilerToolsDir)\bin\HostX64
	else
		VCCompilerToolsBinDir := $(VcCompilerToolsDir)\bin\HostX86
	endif

	PATH := $(shell IFS=$$'\n'; cygpath "$(VCCompilerToolsBinDir)/$(TargetArchMoniker)"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VsInstallRoot)/Common7/IDE")
	INCLUDE := $(shell IFS=$$'\n'; cygpath -w "$(VcCompilerToolsDir)/include")
	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VcCompilerToolsDir)/lib/$(TargetArchMoniker)")
	ifneq (,$(findstring uwp,$(PlatformSuffix)))
		LIB := $(shell IFS=$$'\n'; cygpath -w "$(LIB)/store")
	endif
    
	export INCLUDE := $(INCLUDE);$(WindowsSDKSharedIncludeDir);$(WindowsSDKUCRTIncludeDir);$(WindowsSDKUMIncludeDir)
	export LIB := $(LIB);$(WindowsSDKUCRTLibDir);$(WindowsSDKUMLibDir)
	TARGET := $(TARGET_NAME)_libretro.dll
	PSS_STYLE :=2
	LDFLAGS += -DLL
	HAVE_SYS_PARAM_H = 0
	ARCH_IS_WINDOWS = 1
	HAVE_SSE = 0

# Windows
else
	TARGET := $(TARGET_NAME)_libretro.dll
	CC ?= gcc
	CXX ?= g++
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -static-libgcc -static-libstdc++ -lwinmm
	HAVE_FSEEKO = 1
	ARCH_IS_WINDOWS = 1
	#DYNAREC = 2

endif

include Makefile.common

ifeq ($(HAVE_SYS_PARAM_H), 1)
	FLAGS += -DHAVE_SYS_PARAM_H
endif

ifeq ($(HAVE_FSEEKO), 1)
	FLAGS += -DHAVE_FSEEKO
endif

OBJECTS += $(OBJECTS_C) $(OBJECTS_CXX)

LDFLAGS += $(fpic) $(SHARED)
FLAGS += $(fpic)

INCFLAGS := $(foreach dir,$(INCLUDE_DIRS),-I$(dir))

WARNINGS :=
FLAGS += $(INCFLAGS) -D__LIBRETRO__ $(ENDIANNESS_DEFINES) -DSIZEOF_DOUBLE=8 $(WARNINGS) -DUSE_16BPP=1 -DUSE_RGB_565=1 \
	-DNO_CLI -DHAVE_SYS_TIME_H -DHAVE_GETTIMEOFDAY -DHAVE_STDINT_H=1 -DVERSION=\"0.9.15\" \
	-DHAVE_LROUND -D_7ZIP_ST -DFLAC__HAS_OGG=0 -DFLAC__NO_DLL

CXXFLAGS += $(FLAGS)
CFLAGS += $(FLAGS)
C68KFLAGS += $(FLAGS)

ifeq (,$(findstring msvc,$(platform)))
	ifeq ($(platform),qnx)
		CFLAGS += -Wc,-std=c99
	else
		CFLAGS += -std=gnu99
	endif
endif

ifeq ($(USE_PLAY_JIT),1)
	CXXFLAGS += -std=c++11 -fexceptions -frtti
else
	CXXFLAGS += -std=gnu++99
endif

OBJOUT   = -o
LINKOUT  = -o 

ifneq (,$(findstring msvc,$(platform)))
	OBJOUT = -Fo
	LINKOUT = -out:
	LD = link.exe
else
	LD = $(CXX)
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

ifeq ($(FASTMATH), 1)
	ifeq (,$(findstring msvc,$(platform)))
		FLAGS += -ffast-math
	endif
endif

ifeq ($(HAVE_SSE),1)
	FLAGS += -mfpmath=sse
endif

ifeq ($(DEBUG), 1)
	ifneq (,$(findstring msvc,$(platform)))
		ifeq ($(STATIC_LINKING),1)
			CFLAGS += -MTd
			CXXFLAGS += -MTd
			C68KFLAGS += -MTd
		else
			CFLAGS += -MDd
			CXXFLAGS += -MDd
			C68KFLAGS += -MDd
		endif
		CFLAGS += -Od -Zi -DDEBUG -D_DEBUG
		CXXFLAGS += -Od -Zi -DDEBUG -D_DEBUG
		C68KFLAGS += -Od -Zi -DDEBUG -D_DEBUG
	else
		CFLAGS += -O0 -g -DDEBUG
		CXXFLAGS += -O0 -g -DDEBUG
		C68KFLAGS += -O0 -g -DDEBUG
	endif
else
	ifneq (,$(findstring msvc,$(platform)))
		ifeq ($(STATIC_LINKING),1)
			CFLAGS += -MT
			CXXFLAGS += -MT
			C68KFLAGS += -MT
		else
			CFLAGS += -MD
			CXXFLAGS += -MD
			C68KFLAGS += -MD
		endif
		CFLAGS += -O2 -DNDEBUG
		CXXFLAGS += -O2 -DNDEBUG
		C68KFLAGS += -O0 -DNDEBUG
	else
		CFLAGS += -O3 -DNDEBUG
		CXXFLAGS += -O3 -DNDEBUG
		C68KFLAGS += -O0 -DNDEBUG
	endif
endif

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
	$(LD) $(LDFLAGS) $(LINKOUT)$@ $(OBJECTS) $(LIBS)
endif

%.S.o: %.S
	$(CC_AS) $(CFLAGS) -c $^ -o $@

%.s.o: %.s
	$(CC_AS) $(CFLAGS) -c $^ -o $@

%.cpp.o: %.cpp
	$(CXX) -c $(OBJOUT)$@ $< $(CXXFLAGS)

%.c.o: %.c
	$(CC) -c $(OBJOUT)$@ $< $(CFLAGS)

$(C68KEXEC_OBJECT): $(C68KEXEC_SOURCE)
	$(CC) -c $(OBJOUT)$@ $< $(C68KFLAGS)

clean:
	rm -f $(TARGET) $(OBJECTS) $(SOURCE_DIR)/musashi/$(M68KMAKE_EXE) $(SOURCE_DIR)/c68k/$(GEN68K_EXE)

.PHONY: clean
