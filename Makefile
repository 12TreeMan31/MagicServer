SRC_DIR := src/
OBJ_DIR := obj
BIN_DIR := .

server_old: server_old.o
	gcc -g server_old.o -o server_old.debug
server_old.o: $(SRC_DIR)
	gcc -g -Wall -c $(SRC_DIR)server.c -o server_old.o

client: client.o
	gcc -g client.o -o client.debug
client.o: $(SRC_DIR)
	gcc -g -Wall -c $(SRC_DIR)client.c

new_client: new_client.o
	gcc -g new_client.o -o new_client.debug
new_client.o: $(SRC_DIR)
	gcc -g -Wall -c $(SRC_DIR)new_client.c


clean:
	rm *.o *.debug
