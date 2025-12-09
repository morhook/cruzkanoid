# Makefile for Cruzkanoid
# Designed for Turbo C 3.0 inside a DOS environment (like DOSBox-X)
# This Makefile can be used for command-line compilation within DOSBox-X,
# though the primary recommended method involves using the Turbo C IDE (TC.EXE).

# Compiler settings for Turbo C 3.0
TC = tcc
# -ml: large memory model
# -O: optimize
CFLAGS = -ml -O
LIBS = 

# Default target: compile cruzkan.exe
all: cruzkan.exe

cruzkan.exe: CRUZKAN.C
	$(TC) $(CFLAGS) CRUZKAN.C

# To clean up compiled files
clean:
	-del cruzkan.exe
	-del *.obj

# For cross-compilation from Linux using DJGPP (alternative method)
# Note: Code may need changes to work with DJGPP
CC = i586-pc-msdosdjgpp-gcc
DJGPP_CFLAGS = -Wall -O2
DJGPP_LIBS = -lpc

djgpp: CRUZKAN.C
	$(CC) $(DJGPP_CFLAGS) -o cruzkan.exe CRUZKAN.C $(DJGPP_LIBS)
