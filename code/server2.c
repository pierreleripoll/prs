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

#define TAILLE_BUFFER_CIRCULAR 200
#define SNWD 200
#define RTT 250 //en dixième de milliseconde

int udp_descripteur, pere;

struct sockaddr_in addr_client;
int taille_addr_client = sizeof(addr_client);


void handle_signal() {}

int main(int argc, char **argv)
{
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

     taille_buffer_circular = atoi(argv[1]);
     snwd = atoi(argv[2]);
     rtt = atoi(argv[3]);
     //printf("%d , %d , %d\n",taille_buffer_circular,snwd,rtt);
   }

	/*******************Arguments lors du lancement du programme********************/
	if(argc == 2) {
		portUsr = atoi(argv[1]);
		if(portUsr <= 1023 && portUsr >= 65535) {
			printf("Erreur de port renseigné (en dehors de la range 1023-65534)\n");
			return 0;
		}
		//if(PRINT) printf("port renseigne : %d\n", portUsr);
	} else {
		portUsr = 3500;
		//if(PRINT) printf("default port %d\n", portUsr);
	}

	char message_recu[200];
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
		//if(PRINT) printf("***CONNECTED TRANSFERT***\nConnected=%d\n",connected);
		/********CONNECTION REUSSI *******************/
		recvfrom(udp_descripteur, message_recu, sizeof(message_recu),0,(struct sockaddr *)&addr_client, (socklen_t*)&taille_addr_client);
		//if(PRINT) printf("on a recu : %s\n", message_recu);


		struct stat sb;
		stat(message_recu,&sb);
		int sizeFile = sb.st_size;


		int n_seg_total = (sizeFile/TAILLE_UTILE)+1;
		FILE *fichier=openFichier(message_recu);
		//FILE *fp ;
		//fp = fopen("out.txt","wb");


  //  if(PRINT) printf("Debut du threading\n");
		pthread_t threadEnvoi;
		pthread_t threadReceive;



		BufferCircular_t * bufferCircular = initBufferCircular(taille_buffer_circular);
		//if(PRINT) printf("Circular buffer initialized\n");
		int i;
    int n_seg= 1, pointeurFile= 0 ,ack =0, newAck=0;
		int *ackReceived = malloc(sizeof(int));
		*ackReceived=0;

		for(i=0;i<taille_buffer_circular;i++){
      chargeBuff(fichier,n_seg,TAILLE_UTILE,&(bufferCircular->buffer[i]));
      if(USERTT) startThreadTime(&(bufferCircular->buffer[i]));
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
		}
		//else{printf("Thread Envoi created\n");}

		if(pthread_create(&threadReceive, NULL, functionThreadReceive, argThreadReceive) == -1) {
			perror("pthread_create");
			return EXIT_FAILURE;
		}
		//else{printf("Thread Receive created\n");}
    //fprintf(stderr, "BUFFER FILE : \n%s\n\n\n",&bufferFile[(n_seg_total-1)*TAILLE_UTILE] );

    while(ack != n_seg_total){

			pthread_mutex_lock(argThreadReceive->mutexAck);
			newAck = *ackReceived;
			pthread_mutex_unlock(argThreadReceive->mutexAck);

      if(newAck>ack){
        //if(PRINT) printf("ACK :%d  START : %d STOP : %d\n",ack,bufferCircular->start,bufferCircular->stop);


          int k,j,size= 0;
          for(k=0;k<taille_buffer_circular;k++){ // on cherche le début des paquets validés

						if(k==taille_buffer_circular) k =0;
						if(bufferCircular->buffer[k].numPck == ack+1){ // on a trouvé le paquet validé
              for(j=0;j<(newAck-ack);j++){
                if(n_seg<=n_seg_total){
                  if(k==taille_buffer_circular) k =0; //si jamais on est a la limite du buffer C
                  if(n_seg == n_seg_total) { //c'est le dernier paquet a envoyer
                    size = sizeFile - ((n_seg-1)*TAILLE_UTILE);
                  }
                  else size = TAILLE_UTILE;
                  chargeBuff(fichier,n_seg,size,&(bufferCircular->buffer[k]));
                  n_seg++;
                  pointeurFile+=TAILLE_UTILE;
                  k=k+1;
                }
              }
              break;
            }

          }//endFOR

		  pthread_mutex_lock(&bufferCircular->mutexStop);
          bufferCircular->stop = bufferCircular->stop+ (newAck-ack);
          if(bufferCircular->stop>=taille_buffer_circular) bufferCircular->stop = bufferCircular->stop -taille_buffer_circular;
					ack = newAck;
					if (ack>=n_seg_total) bufferCircular->stop = -1;
				pthread_mutex_unlock(&bufferCircular->mutexStop);


      }//ENF IF ACKRECEVEID > ACK

    } //END BOUCLE MAIN RECEIVER



		//if(PRINT) printf("Fin déclaration du threading\n");

//------------------------------------------------------------------------------------------------
//************************************************************************************************
//END TRANSMISSION

		pthread_join(threadEnvoi,NULL);
		pthread_join(threadReceive,NULL);
		int n =0;

		for(n=0;n<500;n++){
			sendto(udp_descripteur, "FIN", 30+1,0,(struct sockaddr *) &addr_client, sizeof(addr_client));
			usleep(500);
		 }



		close(udp_descripteur);
		fclose(fichier);
		exit(0);
	}


	//------------------------------------------------------------------------------------------------
	//************************************************************************************************
	//KILL PROCESS FILS

	int x;
	for( x=0; x<maxConnection; x++) {
		if(pid[x] != 0) {
			i = x;
		}
	}
	return 0;
}
