server: server.o
	gcc -g server.o -o server.debug
server.o: server.c
	gcc -g -Wall -c server.c

client: client.o
	gcc -g client.o -o client.debug
client.o: client.c
	gcc -g -Wall -c client.c

clean:
	rm *.o *.debug
