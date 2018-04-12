#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tcp.h"


int udp_descripteur, pere;

struct sockaddr_in addr_client;
int taille_addr_client = sizeof(addr_client);

void handle_signal() {

	printf("SIGNAL HANDLE\n");

	if(pere != 0) {
		sendto(udp_descripteur, "FIN", 4, 0,(struct sockaddr *)&addr_client, (socklen_t)taille_addr_client);
		close(udp_descripteur);
	} else {
		sleep(1);
	}
		printf("SIGNAL HANDLE END\n");
}


int max(int x, int y) {
	if(x < y) {
		return y;
	} else {
		return x;
	}
}

void remiseAZero(int pid[maxConnection]) {
	int i;
	for(i=0; i<maxConnection; i++) {
		pid[i] = 0;
	}
}

int main(int argc, char **argv)
{
	int portUsr, reuse, i, connected, pid[maxConnection];
	i=0;
	pere = 0;
	remiseAZero(pid);
	signal(SIGINT, handle_signal);
	connected = 0;

	/*******************Arguments lors du lancement du programme********************/
	if(argc == 2) {
		portUsr = atoi(argv[1]);
		if(portUsr <= 1023 && portUsr >= 65535) {
			printf("Erreur de port renseigné (en dehors de la range 1023-65534)\n");
			return 0;
		}
		printf("port renseigne : %d\n", portUsr);
	} else {
		portUsr = 3500;
		printf("default port %d\n", portUsr);
	}
	/*****************************variable*************************************/
	char message_recu[2000];
	strcpy(message_recu, "");

	while(pere==0) {
		/************* CONNECTION *****************/
		connected = connectServer(portUsr, pid,i);

		if(pid[i] == 0) { //si je suis le fils
			pere = 1;
			/*******************Creation des sockets --- UDP ***************************/
			udp_descripteur = socket(AF_INET, SOCK_DGRAM, 0); //on crée la socket UDP
			if(udp_descripteur < 0) {
				printf("Erreur, socket UDP non crée\n");
			} else {
				printf("Descripteur prive %d\n", udp_descripteur);

				/*Pour faire en sorte que la socket ne soit pas bloquée*/
				reuse = 1;
				setsockopt(udp_descripteur, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

				/*pour la remise à zero de la variable*/
				struct sockaddr_in my_addrUDP;
				memset((char*)&my_addrUDP, 0, sizeof(my_addrUDP));


				/*Initialisation socket addresse*/
				my_addrUDP.sin_family = AF_INET;
				my_addrUDP.sin_port = htons(connected);
				my_addrUDP.sin_addr.s_addr = htons(INADDR_ANY);	//pour le serveur

				bind(udp_descripteur, (struct sockaddr*)&my_addrUDP, sizeof(my_addrUDP));
				printf("Fin boucle init fils\n");
				kill(getppid(),SIGUSR1);
				}
		}
	}

	/********************* LANCEMENT BOUCLE INFINIE - PROGRAMME **************/
	while(connected != 0) {
		printf("***CONNECTED***\nPere=%d\nConnected=%d\n",pere,connected);
		/********CONNECTION REUSSI *******************/
		recvfrom(udp_descripteur, message_recu, sizeof(message_recu),0,(struct sockaddr *)&addr_client, (socklen_t*)&taille_addr_client);
		printf("on a recu : %s\n", message_recu);

		envoyerBinary(udp_descripteur, message_recu);
		/*
		sendto(udp_descripteur, message_recu, strlen(message_recu)+1, 0,(struct sockaddr *)&addr_client, (socklen_t)taille_addr_client);
		if(strcmp(message_recu,"exit") == 0) {
		printf("EXIT\n");
		close(udp_descripteur);
		pid[i] = 0;
		port[connected - 6000] = 0;
		exit(0);
	} else if(strcmp(message_recu,"exitall") == 0) {
	printf("EXIT- ALL\n");
	close(udp_descripteur);
	pid[i] = 0;
	port[connected - 6000] = 0;
	kill(getppid(), SIGINT);
	exit(0);
} */
}


for(int x=0; x<maxConnection; x++) {
	if(pid[x] != 0) {
		i = x;
	}
}

return 0;
}
