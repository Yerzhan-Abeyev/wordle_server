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
default: http-server

http-server: http-server.o game.o http-request.o hashmap.o html.o

http-server.o: http-server.c http-request.h game.h

http-request.o: http-request.c http-request.h

testing.o: testing.c game.h

hashmap.o: hashmap.c hashmap.h

game.o: game.c hashmap.h html.h game.h

html.o: html.c html.h game.h

.PHONY: clean
clean:
	rm -f *.o *.a a.out core http-server

.PHONY: all
all: clean http-server


