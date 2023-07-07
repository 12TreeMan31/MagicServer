SRC_DIR := src/
OBJ_DIR := obj
BIN_DIR := .
#-lev
magic: magic.o log.o
	gcc -g magic.o log.o -o magic.debug

magic.o: src/magic_server.c
	gcc -g -Wall -c src/magic_server.c -o magic.o
log.o: includes/log.c
	gcc -g -Wall -c includes/log.c -o log.o


clean:
	rm *.o *.debug
