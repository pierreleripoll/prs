#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tcp.h"

int udp_descripteur, pere;

struct sockaddr_in addr_client;
int taille_addr_client = sizeof(addr_client);

double get_time_ms() {
	struct timeval t;
	gettimeofday(&t, NULL);
	return (t.tv_sec + (t.tv_usec / 1000000.0)) * 1000.0;
}

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

		struct stat sb;
		stat(message_recu,&sb);
		int sizeFile = sb.st_size;

		/******DEBUT DE CONNECTION************/
		int valid = 0, pointeur=1;

		char * buffer = initBuff(sizeFile);

		/***************Pour faire en sorte que la socket ne soit pas bloquée**********************/
		int reuse = 1;
		setsockopt(descripteur, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

		clock_t startMSS, endMSS;
		double cpu_time_used;

		int nPacketsSend = 0;
		int SIZE = 10;

		int pid = fork();
		if(pid == 0) {
			pidTime = fork();
			if(pidTime == 0) { //TIMER
				double timeActuel = get_time_ms();
				if(timeActuel - get_time_ms() >= 10) {
					for(int i=0; i<SNWD; i++) {
						Buff_t[i].timeWait--;
					}
				}
			} else {

			}
		} else {

		}



		int n_seg = loadFile(buffer,message_recu);
		int ack = 0;
		int swnd = 16;
		int premierPassage = 0;
		double MSStotal = 0;
		int nombre=1;
		double coef = 2;

		while(valid < n_seg){
			printf("SWND=%d VALID=%d ACK=%d\n",swnd,valid,ack);
			for(pointeur=valid+1;pointeur<valid+1+swnd;pointeur++){
				if(pointeur<=n_seg){
					envoyerSegment(udp_descripteur,(struct sockaddr *) &addr_client,pointeur,buffer,sizeFile);
					nPacketsSend++;
				}
			}
			startMSS = clock();

			while(recvfrom(udp_descripteur, message_recu, sizeof(message_recu),0,(struct sockaddr *)&addr_client, (socklen_t*)&taille_addr_client) >0){
				//if(strcmp(message_recu,"ACK") > 0){
				//printf("Recu : %s\n",message_recu);
				if(premierPassage==0) {
					endMSS = clock();
					premierPassage=1;
					double MSS = ((double) (endMSS - startMSS)) / CLOCKS_PER_SEC * 1000000;
					MSStotal += MSS;
					tv.tv_usec = MSStotal / nombre;
					nombre++;
					printf("MSS = %lf et moyenne = %ld\n",MSS,tv.tv_usec);
					setsockopt(udp_descripteur, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));
				}
				if(atoi(&message_recu[3])>ack) ack = atoi(&message_recu[3]);
				printf("ACK %d\n",ack);
				if(ack==n_seg) break;
				//}
			}
			premierPassage=0;
			if(ack==n_seg) break;
			if(ack == valid+swnd)		swnd = swnd*4;
			else swnd = swnd/2;
			valid = ack;
			if(swnd<1) swnd =1;
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
