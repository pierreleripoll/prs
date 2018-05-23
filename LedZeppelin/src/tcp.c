#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "tcp.h"





int connectServer(int port, int *udp_descripteur, int pid[maxConnection], int i) {
	struct sockaddr_in addr_client;
	int taille_addr_client = sizeof(addr_client);

	int portSocket = 0;

	char message_recu[10];
	strcpy(message_recu, "");

	/*******************Creation des sockets --- UDP -- CONNECTION ***************************/
	int udp_connection = initialization_socket(port);

	/********************CONNECTION**********************/

	recvfrom(udp_connection, message_recu, sizeof(message_recu),0,(struct sockaddr *)&addr_client, (socklen_t*)&taille_addr_client);
	if(strcmp(message_recu,"SYN") == 0) {
		//if(PRINT) printf("tentative de connection\n");

		char porti[4], msg[11];
		portDispo(porti);
		portSocket = atoi(porti);

		pid[i]=fork();

		if(pid[i]==0){

			/*******************Creation des sockets --- UDP ON EST LE FILS ***************************/
			*udp_descripteur = initialization_socket(portSocket);
			//if(PRINT){	printf("Fin boucle init fils\n");}
			kill(getppid(),SIGUSR1);
			return portSocket;
		}

		// CONTINUER SEULEMENT LORSQUE LE FILS EST PRET//
		if(pause()<0){
			perror("pause");
		}
		//if(PRINT)	printf("Daddy woke up\n");

		strcpy(msg, "SYN-ACK");
		strcat(msg, porti);
		//if(PRINT)	printf("msg : %s\n",msg);

		sendto(udp_connection, msg, 12, 0,(struct sockaddr *)&addr_client, (socklen_t)taille_addr_client);

		recvfrom(udp_connection, message_recu, sizeof(message_recu),0,(struct sockaddr *)&addr_client, (socklen_t*)&taille_addr_client);
		if(strcmp(message_recu,"ACK") == 0) {
			//if(PRINT){printf("CONNECTED SERVEUR\n");}
			return portSocket;
		}
	}
	return 0;
}

int portDispo(char porti[4]) {
	int i;
	for(i=0; i<maxPort; i++) {
		if(port[i] == 0) {
			port[i] = 1;
			sprintf(porti, "%d", 6000+i);
			return 0;
		}
	}
	return 1;
}

int initialization_socket(int port) {
	/*******************Creation des sockets --- UDP -- CONNECTION ***************************/
	int udp_connection = socket(AF_INET, SOCK_DGRAM, 0); //on crée la socket UDP
	if(udp_connection < 0) {
		//if(PRINT)			printf("Erreur, socket UDP connection non crée\n");

		return -1;
	} else {
		//if(PRINT){		printf("Descripteur UDP-connection = %d\n", udp_connection);}
		/*Pour faire en sorte que la socket ne soit pas bloquée*/
		int reuse = 1;
		setsockopt(udp_connection, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

		/*pour la remise à zero de la variable*/
		struct sockaddr_in my_addr;
		memset((char*)&my_addr, 0, sizeof(my_addr));


		/*Initialisation socket addresse*/
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons(port);
		my_addr.sin_addr.s_addr = htons(INADDR_ANY);	//pour le serveur

		bind(udp_connection, (struct sockaddr*)&my_addr, sizeof(my_addr));
		return udp_connection;
	}
	return -1;
}

int envoyerBinary(int sock,struct sockaddr *addr, char nom_fichier[64]) {
	int i;
	int num_segment = 0;

	FILE *fichier;
	fichier = fopen(nom_fichier,"rb");
	char buffer[1024][TAILLE_UTILE];
	//unsigned int entete[32];
	//if(PRINT){printf("Commencement à recevoir, ouverture du fichier %s\n", nom_fichier);}
		//	printf("***ENVOIE***\n");
	num_segment = fread(buffer,1024,TAILLE_UTILE,fichier);
	char message[TAILLE_MAX_SEGMENT];

	for (i=0;i<num_segment;i++){
		//printf("Buff %d :\n%s\n",i,buffer[i]);
		snprintf(message,7,"%06d",i);
		//if(PRINT){printf("Entete message : %s\n",message);}
		strcat(message,buffer[i]);
		//	printf("Message final :\n%s\n--------------------\n",message);
		if(sendto(sock, message, 1024,0,addr,sizeof(*addr))==-1){
			//if(PRINT){printf("Error to send i %d",i);	}
			perror("send");
			return -1;
		}
	}

	return 1;
}

int loadFile(char * buff, char nom_fichier[64]){
	size_t num_segment = 0;

	FILE *fichier;
	fichier = fopen(nom_fichier,"rb");
	//if(PRINT){printf("Commencement à recevoir, ouverture du fichier %s\n", nom_fichier);}
	struct stat sb;
	stat(nom_fichier,&sb);
	//if(PRINT) printf("File size : %ld\n",sb.st_size);
	//	if(PRINT) printf("***ENVOIE***\n");
	num_segment = fread(buff,TAILLE_UTILE,(sb.st_size/TAILLE_UTILE)+1,fichier);
	if ( ferror( fichier ) != 0 ) {
			fputs("Error reading file", stderr);
			perror("fread");
	}
	fclose(fichier);
	//if(PRINT) printf("Buffer size : %zu TAILLE_UTILE = %d\n",num_segment,TAILLE_UTILE);
	//num_segment=num_segment/TAILLE_UTILE;

	//if(PRINT) printf("%zu segments\n",num_segment+1);
	//if(PRINT) printf("Buffer : %s\n",buff);


	return num_segment;
}


FILE * openFichier(char nom_fichier[64]){
	FILE * fichier;
	fichier = fopen(nom_fichier,"rb");
	return fichier;

}

char * initBuff(int sizeFile){
	int nElem = sizeFile / (TAILLE_UTILE+1);
	////fprintf(stderr, "NElem = %d\n",nElem );
	char * buffer = malloc(sizeof(char)* (nElem+1) * (TAILLE_UTILE+1));
	memset(buffer,(int)EOF,(nElem+1)*(TAILLE_UTILE+1));
	return buffer;
}


int envoyerSegment(int sock, struct sockaddr *addr, Buff_t * buff){

	if(sendto(sock, buff->buffer, buff->sizeBuff+TAILLE_ENTETE,0,addr,sizeof(*addr))==-1){
		perror("sendto");
		return -1;
	}
	return 1;
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
//ORDRE DE RECEPTION
//ACK -> SLOW-START
//slow start : envoie du premier paquet : détermination du RTT (temps le plus court)
//ensuite on envoie deux paquets, puis 4, 8, 16 etc... si on ne reçoit pas l'ack dans le temps de RTT, on divise par deux
//à l'approche de la fenêtre (comment trouver la taille de la fenêtre?!?) on augmente 1 par 1
