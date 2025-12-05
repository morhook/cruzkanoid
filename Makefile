# Makefile for DOS Cruzkanoid clone
# Requires Turbo C or similar DOS C compiler

# For Turbo C
TC = tcc
CFLAGS = -ml -O
LIBS =

# For DJGPP (cross-compile from Linux)
CC = i586-pc-msdosdjgpp-gcc
DJGPP_CFLAGS = -Wall -O2
DJGPP_LIBS = -lpc

# Default target
cruzkan.exe: cruzkan.c
	$(TC) $(CFLAGS) cruzkan.c

# For DJGPP compilation
djgpp: cruzkan.c
	$(CC) $(DJGPP_CFLAGS) -o cruzkan.exe cruzkan.c $(DJGPP_LIBS)

clean:
	del cruzkan.exe
	del *.obj
