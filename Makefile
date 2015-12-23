all: server client
server: server.c
	gcc -Wall -o server server.c -lpthread -lrt -I.
	
client: client.c
	gcc -Wall -o client client.c -lrt -I.

