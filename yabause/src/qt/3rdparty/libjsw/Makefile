# ########################################################################
#
#                        Makefile for libjsw
#
#   Rules:
#
#	all	-- builds library.
#	install -- installs library.
#	clean	-- remove object and other work files.
#


# ########################################################################
# Install Options:
#
PREFIX = /usr


# ########################################################################
# Library Name and Version:
#
LIBPFX = libjsw
LIBVER = 1.5.7


# ########################################################################
# Compiler Flags:
#   
#   These are definations to enable or disable certain compile time
#   options. Omitting a defination turns that option off.
#
#   Each argument is of the format -D<option> where <option> is
#   one of the following:
#
#	* None available *
#
#   Other arguments include:
#
#       -O#                     Specifies the optimization level the
#                               compiler is to compile at. This (attempts)
#                               to improve the efficiency of the generated
#                               program when it runs. Available values for
#                               # are from 0 to 2 (some compilers allow
#                               higher values). When in doubt, set # to 2.
#
#       -g                      Compile with debugging information,
#                               this is useful for determining why this
#                               program (if it did) crash. However this
#                               may hinder performance, so don't use
#                               this option unless you are attempting
#                               to debug the program.
#

CFLAGS = -Wall -O2 -g
CFLAGS += -ffast-math

CPPFLAGS = -D__cplusplus


# ########################################################################
# Dependant Libraries:
#
INC_DIRS =
LIBS     = -shared
LIB_DIRS =


# ########################################################################
# Program Source and Header Files:
#
#   Do not modify any values in this section unless you know what you
#   are doing or have been instructed to do so.
#
include Makefile.srclist

LS      = ls
LSFLAGS = -s -h -c --color=auto

CC  = cc
CPP = c++
LIB = $(LIBPFX).so.$(LIBVER)
OBJ_C   = $(SRC_C:.c=.o)
OBJ_CPP = $(SRC_CPP:.cpp=.o)
.c.o:
	@echo "Compiling module $*.o"
	@+$(CC) -c $*.c $(INC_DIRS) $(CFLAGS)
.cpp.o:
	@echo "Compiling module $*.o"
	@+$(CPP) -c $*.cpp $(INC_DIRS) $(CFLAGS) $(CPPFLAGS)


# ########################################################################
# Build Rules:
#
$(LIB): prebuild modules postbuild

modules: $(OBJ_C) $(OBJ_CPP)
	@echo  -n "Linking modules..."
	@$(CC) $(OBJ_C) $(OBJ_CPP) -Wl,-soname=$(LIB) -shared -o $(LIB) $(LIBS) $(LIB_DIRS)
	@echo -n "   "
	@$(RM) $(RMFLAGS) $(LIBPFX).so
	@$(LINK) -s $(LIB) $(LIBPFX).so
	@$(LINK) -s $(LIB) $(LIBPFX).so.1
	@-$(LS) $(LSFLAGS) $(LIB)

prebuild:
	@echo "Building library \"$(LIB)\"..."

postbuild:
	@echo "Build done."
	@echo "To install, type \"su\" (to gain root privileges) and then type \"make install\"."

all: $(LIB)


# ########################################################################
# Install Rules:
#

INSTALL      = install
INSTBINFLAGS = -m 0755 -s
INSTUIDFLAGS = -m 4755
INSTLIBFLAGS = -m 0755 -s
INSTINCFLAGS = -m 0644
INSTMANFLAGS = -m 0644
INSTCFGFLAGS = -m 0644
INSTDATFLAGS = -m 0444

COPY      = cp
COPYFLAGS = -i -v

MKDIR      = mkdir
MKDIRFLAGS = -p

LINK      = ln
LINKFLAGS = -s -f

RM      = rm
RMFLAGS = -f

LDCONFIG      = ldconfig
LDCONFIGFLAGS =

JSW_INC_DIR = $(PREFIX)/include
JSW_LIB_DIR = $(PREFIX)/lib
JSW_MAN_DIR = $(PREFIX)/man/man3

HR = "-------------------------------------------------------------------------"

install: preinstall install_lib install_devel install_data postinstall

install_lib:
	@$(MKDIR) $(MKDIRFLAGS) $(JSW_LIB_DIR)
	@echo "Installing $(LIBPFX).so.$(LIBVER) -> $(JSW_LIB_DIR)"
	@$(INSTALL) $(INSTLIBFLAGS) $(LIBPFX).so.$(LIBVER) $(JSW_LIB_DIR)
	@$(RM) $(RMFLAGS) $(JSW_LIB_DIR)/$(LIBPFX).so
	@$(LINK) $(LINKFLAGS) $(LIBPFX).so.$(LIBVER) $(JSW_LIB_DIR)/$(LIBPFX).so
	@$(LINK) $(LINKFLAGS) $(LIBPFX).so.$(LIBVER) $(JSW_LIB_DIR)/$(LIBPFX).so.1

install_devel:
	@$(MKDIR) $(MKDIRFLAGS) $(JSW_INC_DIR)
	@echo "Installing jsw.h -> $(JSW_INC_DIR)"
	@$(INSTALL) $(INSTINCFLAGS) ../include/jsw.h $(JSW_INC_DIR)

install_data:
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSClose.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSDriverQueryVersion.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSDriverVersion.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSFreeAttributesList.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSGetAttributesList.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSGetAxisCoeff.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSGetAxisCoeffNZ.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSGetButtonState.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSInit.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSIntro.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSIsAxisAllocated.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSIsButtonAllocated.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSLoadCalibrationUNIX.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSLoadDeviceNamesUNIX.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSResetAllAxisTolorance.3
	@$(RM) $(RMFLAGS) $(JSW_MAN_DIR)/JSUpdate.3
	@$(MKDIR) $(MKDIRFLAGS) $(JSW_MAN_DIR)
	@echo "Installing manual pages -> $(JSW_MAN_DIR)"
	@$(INSTALL) $(INSTMANFLAGS) man/* $(JSW_MAN_DIR)

preinstall:
	@echo "Installing \"$(LIBPFX)\"..."

postinstall:
	@-$(LDCONFIG) $(LDCONFIGFLAGS)
	@echo "Install done."
	@echo $(HR)
	@echo "Library $(LIBPFX) installed in:"
	@echo " "
	@echo "        $(JSW_LIB_DIR)"
	@echo " "
	@echo "Header files are installed in:"
	@echo " "
	@echo "        $(JSW_INC_DIR)"
	@echo " "
	@echo "Manual pages installed in:"
	@echo " "
	@echo "        $(JSW_MAN_DIR)"
	@echo " "
	@echo "To link your programs to this library add -ljsw to the compiler"
	@echo "command."
	@echo " "
	@echo "To learn about the libjsw api, type \"man JSIntro\""
	@echo " "
	@echo "IMPORTANT: Read README if upgrading from version 1.4.x to 1.5.x!!"
	@echo " "


# ########################################################################
# Maintainance and Misc Rules:
#
clean:
	@echo "Cleaning library \"$(LIB)\"..."
	@echo "Deleting all intermediate files..."
	@$(RM) $(RMFLAGS) a.out core *.o $(LIBPFX).so $(LIBPFX).so.1 $(LIBPFX).so.$(LIBVER)
	@echo "Clean done."

# ########################################################################
