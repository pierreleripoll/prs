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
		exit(0);
	} else {
		printf("FATHER CAUGHT SIGNAL\n");
	}
	printf("SIGNAL HANDLE END\n");
}

int main(int argc, char **argv)
{
	int portUsr, i, connected, pid[maxConnection];
	i=0;
	pere = 0;
	remiseAZero(pid);
	signal(SIGUSR1, handle_signal);
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

	char message_recu[2000];
	strcpy(message_recu, "");

	while(pere==0) {
		/************* CONNECTION *****************/
		connected = connectServer(portUsr, &udp_descripteur, pid,i);

		if(pid[i] == 0) { //si je suis le fils
			pere = 1;
		}
	}

	/********************* LANCEMENT BOUCLE INFINIE - PROGRAMME **************/
	if(connected != 0) {
		printf("***CONNECTED TRANSFERT***\nConnected=%d\n",connected);
		/********CONNECTION, NOM DU FICHIER A ENVOER *******************/
		recvfrom(udp_descripteur, message_recu, sizeof(message_recu),0,(struct sockaddr *)&addr_client, (socklen_t*)&taille_addr_client);
		printf("on a recu : %s\n", message_recu);

		/******DEBUT DE CONNECTION************/
		int valid = 0;
		char * buffer = initBuff();
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 10;
		if (setsockopt(udp_descripteur, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
			perror("Error");
		}
		int n_seg = loadFile(buffer,message_recu);
		int ack = 0;
		sendto(udp_descripteur, "FIN", 1024,0,(struct sockaddr *) &addr_client, sizeof(addr_client));
		while(valid != n_seg){
			envoyerSegment(udp_descripteur,(struct sockaddr *) &addr_client,valid,buffer);
			if(recvfrom(udp_descripteur, message_recu, sizeof(message_recu),0,(struct sockaddr *)&addr_client, (socklen_t*)&taille_addr_client) >0){
				if(strcmp(message_recu,"ACK") > 0){
					printf("Recu : %s\n",message_recu);
					ack = atoi(&message_recu[3]) +1;
					if(ack<valid) valid = ack;
				}
			} else {
				valid ++;
			}
		}

		sendto(udp_descripteur, "FIN", 1024,0,(struct sockaddr *) &addr_client, sizeof(addr_client));
		exit(0);
	}


	for(int x=0; x<maxConnection; x++) {
		if(pid[x] != 0) {
			i = x;
		}
	}
	return 0;
}
