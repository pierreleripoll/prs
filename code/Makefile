all: server

server: server.c tcp.c tcp.h
	gcc -pthread -Wall tcp.c server.c -o server -lm

clean:
	rm -rf server
