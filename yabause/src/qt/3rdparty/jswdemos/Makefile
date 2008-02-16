# Compiler flags
CFLAGS = -g -O2 -Wall

# Libraries to link to
LIB = -ljsw

# Library paths
LIB_DIRS =

# Include paths
INC_DIRS =

# Compiler
CC = gcc
CPP = g++

# Utilities
RM      = rm
RMFLAGS = -f

BIN_ALL = axis basic button listing waitbtn

# Rules
all: $(BIN_ALL)

axis:
	@echo "Compiling program \"axis\""
	@$(CC) axis.c -o axis $(CFLAGS) $(LIB) $(LIB_DIRS)

basic:
	@echo "Compiling program \"basic\""
	@$(CC) basic.c -o basic $(CFLAGS) $(LIB) $(LIB_DIRS)

button:
	@echo "Compiling program \"button\""
	@$(CC) button.c -o button $(CFLAGS) $(LIB) $(LIB_DIRS)

listing:
	@echo "Compiling program \"listing\""
	@$(CC) listing.c -o listing $(CFLAGS) $(LIB) $(LIB_DIRS)
waitbtn:
	@echo "Compiling program \"waitbtn\""
	@$(CC) waitbtn.c -o waitbtn $(CFLAGS) $(LIB) $(LIB_DIRS)

# Clean up intermediate files
clean:
	@echo "Deleting all intermediate files..."
	@$(RM) $(RMFLAGS) a.out core *.o $(BIN_ALL)
	@echo "Clean done."
