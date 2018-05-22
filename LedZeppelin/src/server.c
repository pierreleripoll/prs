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


void handle_signal() {}

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

  int taille_buffer_circular = TAILLE_BUFFER_CIRCULAR;
  int snwd = SNWD;
  int rtt = RTT;

  if(argc==4){
     if(PRINT) printf("ARGUMENTS TOKEN\n");
     taille_buffer_circular = atoi(argv[1]);
     snwd = atoi(argv[2]);
     rtt = atoi(argv[3]);
     if(PRINT) printf("%d , %d , %d\n",taille_buffer_circular,snwd,rtt);
   }

	/*******************Arguments lors du lancement du programme********************/
	if(argc == 2) {
		portUsr = atoi(argv[1]);
		if(portUsr <= 1023 && portUsr >= 65535) {
			if(PRINT) printf("Erreur de port renseigné (en dehors de la range 1023-65534)\n");
			return 0;
		}
		if(PRINT) printf("port renseigne : %d\n", portUsr);
	} else {
		portUsr = 3500;
		if(PRINT) printf("default port %d\n", portUsr);
	}


	char message_recu[2000];
	strcpy(message_recu, "");

	while(pere==0) {
		connected = connectServer(portUsr, &udp_descripteur, pid,i);

		if(pid[i] == 0) { //si je suis le fils
			pere = 1;
		}
	}

	/********************* LANCEMENT BOUCLE INFINIE - PROGRAMME **************/
	if(connected != 0) {
		if(PRINT) printf("***CONNECTED TRANSFERT***\nConnected=%d\n",connected);
		/********CONNECTION REUSSI *******************/
		clock_gettime(CLOCK_REALTIME, &requestStart);

		recvfrom(udp_descripteur, message_recu, sizeof(message_recu),0,(struct sockaddr *)&addr_client, (socklen_t*)&taille_addr_client);

		if(PRINT) printf("on a recu : %s\n", message_recu);

		struct stat sb;
		stat(message_recu,&sb);
		int sizeFile = sb.st_size;

		char * bufferFile = initBuff(sizeFile);
		int n_seg_total = loadFile(bufferFile,message_recu)+1;
		int nPacketsSend = 0;


    if(PRINT) printf("Debut du threading\n");
		pthread_t threadEnvoi;
		pthread_t threadReceive;

		BufferCircular_t * bufferCircular = initBufferCircular(taille_buffer_circular);
		if(PRINT) printf("Circular buffer initialized\n");
		int i;
    int n_seg= 1, pointeurFile= 0 ,ack =0, newAck=0;
		int *ackReceived = malloc(sizeof(int));
		*ackReceived=0;
		for(i=0;i<taille_buffer_circular;i++){
      chargeBuff(&bufferFile[pointeurFile],n_seg,TAILLE_UTILE,&(bufferCircular->buffer[i]));
      startThreadTime(&(bufferCircular->buffer[i]));
			pthread_mutex_init(&bufferCircular->buffer[i].mutexBuff,NULL);
      pointeurFile+=TAILLE_UTILE;
      n_seg++;
    }

		pthread_mutex_init(&bufferCircular->mutexStart,NULL);
		pthread_mutex_init(&bufferCircular->mutexStop,NULL);
		pthread_mutex_lock(&bufferCircular->mutexStart);
		bufferCircular->start=0;
		pthread_mutex_unlock(&bufferCircular->mutexStart);
		pthread_mutex_lock(&bufferCircular->mutexStop);
		bufferCircular->stop=snwd-1;
		pthread_mutex_unlock(&bufferCircular->mutexStop);
		pthread_mutex_t mutexAck;

		ArgThreadEnvoi_t * argThreadEnvoi = malloc(sizeof(ArgThreadEnvoi_t));
    argThreadEnvoi->bufferC=bufferCircular;
    argThreadEnvoi->sock=udp_descripteur;
    argThreadEnvoi->addr=(struct sockaddr *) &addr_client;
    argThreadEnvoi->nPacketsSend = &nPacketsSend;
    argThreadEnvoi->rtt = rtt;
		argThreadEnvoi->snwd= snwd;
    argThreadEnvoi->taille_buffer_circular = taille_buffer_circular;

		ArgThreadReceive_t * argThreadReceive = malloc(sizeof(ArgThreadReceive_t));
		argThreadReceive->bufferC=bufferCircular;
		argThreadReceive->sock=udp_descripteur;
		argThreadReceive->addr=(struct sockaddr *) &addr_client;
		argThreadReceive->rtt = &rtt;
		argThreadReceive->snwd= &snwd;
		argThreadReceive->taille_buffer_circular = taille_buffer_circular;
		argThreadReceive->ackReceived = ackReceived;
		argThreadReceive->taille_addr_client = taille_addr_client;
		argThreadReceive->mutexAck=&mutexAck;
		argThreadReceive->n_seg_total = n_seg_total;
		pthread_mutex_init(&mutexAck,NULL);


		if(pthread_create(&threadEnvoi, NULL, functionThreadSend, argThreadEnvoi) == -1) {
			perror("pthread_create");
			return EXIT_FAILURE;
		}else{
			printf("Thread Envoi created\n");
		}

		if(pthread_create(&threadReceive, NULL, functionThreadReceive, argThreadReceive) == -1) {
			perror("pthread_create");
			return EXIT_FAILURE;
		}
		else{
			printf("Thread Receive created\n");
		}
    //fprintf(stderr, "BUFFER FILE : \n%s\n\n\n",&bufferFile[(n_seg_total-1)*TAILLE_UTILE] );

    while(ack != n_seg_total){

    	// while(recvfrom(udp_descripteur, message_recu, sizeof(message_recu),0,(struct sockaddr *)&addr_client, (socklen_t*)&taille_addr_client) >0){
			// 	if(strcmp(message_recu,"ACK") > 0){
			// 		if(PRINT) printf("Recu : %s\n",message_recu);
			// 		*ackReceived = atoi(&message_recu[3]);
			// 		if(PRINT) printf("ACK %d\n",*ackReceived);
			// 		int i;
			// 		for(i=0;i<TAILLE_BUFFER_CIRCULAR;i++){
			// 			pthread_mutex_lock(&bufferCircular->buffer[i].mutexBuff);
			// 			if(bufferCircular->buffer[i].numPck == *ackReceived+1) bufferCircular->buffer[i].ackWarning = bufferCircular->buffer[i].ackWarning +1;
			// 			pthread_mutex_unlock(&bufferCircular->buffer[i].mutexBuff);
			// 		}
			// 		if(*ackReceived>ack){
      //       if(PRINT) printf("BREAK\n");
      //       break;
      //
      //     }
			// 	  }
			// }
			pthread_mutex_lock(argThreadReceive->mutexAck);
			newAck = *ackReceived;
			pthread_mutex_unlock(argThreadReceive->mutexAck);

      if(newAck>ack){
        if(PRINT) printf("ACK :%d  START : %d STOP : %d\n",ack,bufferCircular->start,bufferCircular->stop);

				pthread_mutex_lock(&bufferCircular->mutexStart);
        bufferCircular->start = bufferCircular->start +(newAck-ack);
        if(bufferCircular->start>=taille_buffer_circular) bufferCircular->start = bufferCircular->start -taille_buffer_circular;
				pthread_mutex_unlock(&bufferCircular->mutexStart);

          int k,j,size= 0;
          for(k=0;k<taille_buffer_circular;k++){ // on cherche le début des paquets validés
            if(bufferCircular->buffer[k].numPck == ack+1){ // on a trouvé le paquet validé
              for(j=0;j<(newAck-ack);j++){
                if(n_seg<=n_seg_total){
                  if(k==taille_buffer_circular) k =0; //si jamais on est a la limite du buffer C
                  if(n_seg == n_seg_total) { //c'est le dernier paquet a envoyer
                    size = sizeFile - ((n_seg-1)*TAILLE_UTILE);
                    //fprintf(stderr,"LASTPAQUET SIZE : %d\n",size);
                  }
                  else size = TAILLE_UTILE;
                  chargeBuff(&bufferFile[pointeurFile],n_seg,size,&(bufferCircular->buffer[k]));
                  n_seg++;
                  pointeurFile+=TAILLE_UTILE;
                  k++;
                }
              }
              break;
            }
          }//endFOR

          if(PRINT) printf("*ackReceived %d\n",newAck);
					pthread_mutex_lock(&bufferCircular->mutexStop);
          bufferCircular->stop = bufferCircular->stop+ (newAck-ack);
          if(bufferCircular->stop>=taille_buffer_circular) bufferCircular->stop = bufferCircular->stop -taille_buffer_circular;
					ack = newAck;
					if (ack>=n_seg_total) bufferCircular->stop = -1;
					pthread_mutex_unlock(&bufferCircular->mutexStop);


          if(PRINT) printf("ACK :%d  START : %d STOP : %d\n",ack,bufferCircular->start,bufferCircular->stop);
          // if(PRINT) printf("Etat buffer Circular :\n");
          // int i;
          // for(i=0;i<taille_buffer_circular;i++){
          //   if(PRINT) printf("%d : %d | ",i,bufferCircular->buffer[i].numPck);
          // }

      }//ENF IF ACKRECEVEID > ACK

    } //END BOUCLE MAIN RECEIVER






		if(PRINT) printf("Fin déclaration du threading\n");

//------------------------------------------------------------------------------------------------
//************************************************************************************************
//BEGIN TRANSMISSION



		// while(valid < n_seg){
		// 	if(PRINT) printf("SWND=%d VALID=%d ACK=%d\n",swnd,valid,ack);
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
		// 			//if(PRINT) printf("Recu : %s\n",message_recu);
		// 			if(atoi(&message_recu[3])>ack) ack = atoi(&message_recu[3]);
		// 			if(PRINT) printf("ACK %d\n",ack);
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
		if(PRINT || PRINT_RESULT) printf("%d packets send\n",nPacketsSend);
		pthread_cancel(threadEnvoi);
		pthread_cancel(threadReceive);

		//fclose(fp);
		clock_gettime(CLOCK_REALTIME, &requestEnd);

		//if(PRINT) printf("CLOCK =%ld\n",clock());
		//if(PRINT) printf("CLOCKS_PER_SEC =%ld\n",CLOCKS_PER_SEC);
		double realTime = ( requestEnd.tv_sec - requestStart.tv_sec )
		  + ( requestEnd.tv_nsec - requestStart.tv_nsec ) / 1E9;


		if(PRINT || PRINT_RESULT) printf("TIME = %.6f\nDEBIT = %.2fko/s\n",realTime,(double) sizeFile/(realTime*1000));
		free(bufferFile);
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
