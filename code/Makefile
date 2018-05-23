all: server

server: server.c tcp.c tcp.h thread.c
	gcc -pthread -Wall tcp.c server.c thread.c -o server -lm

clean:
	rm -rf server
