cc = gcc
flags = -Wall
clientsrc = client.c
serversrc = multiserver.c

.PHONY: all

all: client server

client: $(clientsrc)
	$(cc) $(flags) -o client $(clientsrc)

server: $(serversrc)
	$(cc) $(flags) -o server $(serversrc)