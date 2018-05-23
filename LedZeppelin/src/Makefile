all: serveur1-LedZeppelin serveur2-LedZeppelin serveur3-LedZeppelin

serveur1-LedZeppelin: server1.c tcp.c tcp.h thread.c
	gcc -pthread -Wall tcp.c server1.c thread.c -o serveur1-LedZeppelin -lm

serveur2-LedZeppelin: server2.c tcp.c tcp.h thread.c
	gcc -pthread -Wall tcp.c server2.c thread.c -o serveur2-LedZeppelin -lm

serveur3-LedZeppelin: server3.c tcp.c tcp.h thread.c
	gcc -pthread -Wall tcp.c server3.c thread.c -o serveur3-LedZeppelin -lm

clean:
	rm -rf serveur1-LedZeppelin serveur2-LedZeppelin serveur3-LedZeppelin
