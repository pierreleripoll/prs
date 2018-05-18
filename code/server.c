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
#include <pthread.h>

#include "tcp.h"


int udp_descripteur, pere;

struct sockaddr_in addr_client;
int taille_addr_client = sizeof(addr_client);

void *threadEnvoi(void *arg)
{
    printf("Nous sommes dans le thread.\n");

    /* Pour enlever le warning */
    (void) arg;
    pthread_exit(NULL);
}

void *threadAck(void *arg)
{
    printf("Nous sommes dans le thread.\n");

    /* Pour enlever le warning */
    (void) arg;
    pthread_exit(NULL);
}

void *threadTime(void *arg)
{
    printf("Nous sommes dans le thread.\n");

    /* Pour enlever le warning */
    (void) arg;
    pthread_exit(NULL);
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
	struct timespec requestStart, requestEnd;
	int portUsr, i, connected, pid[maxConnection];
	i=0;
	pere = 0;
	remiseAZero(pid);
	signal(SIGUSR1, handle_signal);
	//signal(SIGINT,handle_signal);
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
		/********CONNECTION REUSSI *******************/
		clock_gettime(CLOCK_REALTIME, &requestStart);

		recvfrom(udp_descripteur, message_recu, sizeof(message_recu),0,(struct sockaddr *)&addr_client, (socklen_t*)&taille_addr_client);
		printf("on a recu : %s\n", message_recu);

		int valid = 0, pointeur = 1;

		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 5;

		struct stat sb;
		stat(message_recu,&sb);
		int sizeFile = sb.st_size;

		char * buffer = initBuff(sizeFile);

		Buff_t * bufferCircular = initBufferCircular(SNWD);

		int nPacketsSend = 0;

		if (setsockopt(udp_descripteur, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
			perror("Error");
		}
		int n_seg = loadFile(buffer,message_recu)+1;
		//FILE *fp ;
		//fp = fopen("out.txt","wb");


		int ack = 0;
		int swnd = 16;

		int start = 0;
		int stop = 0;
//------------------------------------------------------------------------------------------------
//************************************************************************************************
//BEGIN TRANSMISSION


		pthread_t threadEnvoi;
		pthread_t threadAck;
		pthread_t threadTime;

		printf("Avant création des threads\n");
		if(pthread_create(&threadEnvoi, NULL, thread, NULL) == -1) {
		perror("pthread_create");
		return EXIT_FAILURE;
		}
		if(pthread_create(&threadAck, NULL, thread, NULL) == -1) {
		perror("pthread_create");
		return EXIT_FAILURE;
		}
		if(pthread_create(&threadTime, NULL, thread, NULL) == -1) {
		perror("pthread_create");
		return EXIT_FAILURE;
		}
		printf("Après création des threads\n");


		// while(valid < n_seg){
		// 	printf("SWND=%d VALID=%d ACK=%d\n",swnd,valid,ack);
		// 	for(pointeur=valid+1;pointeur<valid+1+swnd;pointeur++){
		// 		if(pointeur<=n_seg){
		// 			envoyerSegment(udp_descripteur,(struct sockaddr *) &addr_client,pointeur,buffer,sizeFile);
		// 			nPacketsSend++;
		// 		}
		// 		else break;
		// 	 }
    //
		// 	while(recvfrom(udp_descripteur, message_recu, sizeof(message_recu),0,(struct sockaddr *)&addr_client, (socklen_t*)&taille_addr_client) >0){
		// 		//if(strcmp(message_recu,"ACK") > 0){
		// 			//printf("Recu : %s\n",message_recu);
		// 			if(atoi(&message_recu[3])>ack) ack = atoi(&message_recu[3]);
		// 			printf("ACK %d\n",ack);
		// 			if(ack==n_seg) break;
		// 		//}
		// 	}
		// 	if(ack==n_seg) break;
		// 	if(ack == valid+swnd)		swnd = swnd*4;
		// 	else swnd = swnd/2;
		// 	valid = ack;
		// 	if(swnd<1) swnd =1;
		// }




//------------------------------------------------------------------------------------------------
//************************************************************************************************
//END TRANSMISSION


		sendto(udp_descripteur, "FIN", TAILLE_MAX_SEGMENT,0,(struct sockaddr *) &addr_client, sizeof(addr_client));
		printf("%d packets send\n",nPacketsSend);
		//fclose(fp);
		clock_gettime(CLOCK_REALTIME, &requestEnd);

		//printf("CLOCK =%ld\n",clock());
		//printf("CLOCKS_PER_SEC =%ld\n",CLOCKS_PER_SEC);
		double realTime = ( requestEnd.tv_sec - requestStart.tv_sec )
		  + ( requestEnd.tv_nsec - requestStart.tv_nsec ) / 1E9;

		printf("TIME = %.6f\nDEBIT = %.2fko/s\n",realTime,(double) sizeFile/(realTime*1000));
		free(buffer);
		exit(0);
	}


	//------------------------------------------------------------------------------------------------
	//************************************************************************************************
	//KILL PROCESS FILS


	for(int x=0; x<maxConnection; x++) {
		if(pid[x] != 0) {
			i = x;
		}
	}
	return 0;
}
