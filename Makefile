server_old: server_old.o
	gcc -g server_old.o -o server_old.debug
server_old.o: server.c
	gcc -g -Wall -c server.c

client: client.o
	gcc -g client.o -o client.debug
client.o: client.c
	gcc -g -Wall -c client.c

new_client: new_client.o
	gcc -g new_client.o -o new_client.debug
new_client.o: new_client.c
	gcc -g -Wall -c new_client.c


clean:
	rm *.o *.debug
