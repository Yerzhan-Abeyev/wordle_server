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
default: server

server: server.o http-request.o network.o dataStructures.o hashmap.o  game.o  utility.o html.o

server.o: server.c dataStructures.h http-request.h network.h utility.h

http-request.o: http-request.c http-request.h network.h dataStructures.h game.h utility.h

network.o: network.c network.h dataStructures.h http-request.h utility.h

dataStructures.o: dataStructures.c dataStructures.h utility.h

hashmap.o: hashmap.c hashmap.h

game.o: game.c hashmap.h html.h game.h

utility.o: utility.c utility.h

html.o: html.c html.h game.h

.PHONY: clean
clean:
	rm -f *.o *.a a.out core http-server

.PHONY: all
all: clean http-server
