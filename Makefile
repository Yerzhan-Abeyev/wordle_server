# Set CC to gcc to use gcc as our C compiler
CC = gcc

# Compilation options:
# -g: include debugging info symbols
# -Wall: enable all warnings
CFLAGS = -g -Wall 

# Linking options:
LDFLAGS = 

# List the libraries you need to link with in LDLIBS.
# For example, use -lm for the math library.
LDLIBS = 

.PHONY: default
default: testing

testing: testing.o hashmap.o html.o game.o

testing.o: testing.c game.h

hashmap.o: hashmap.c hashmap.h

game.o: game.c hashmap.h html.h game.h

html.o: html.c html.h game.h

.PHONY: clean
clean:
	rm -f *.o *.a a.out core testing

.PHONY: all
all: clean testing


