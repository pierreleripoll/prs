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


void *functionThreadSend(void* arg) {
  ArgThreadEnvoi_t * argT = arg;
  BufferCircular_t *bufferC = argT->bufferC;
  int sock = argT->sock;
  int taille_buffer_circular = argT->taille_buffer_circular;
  int rtt = argT->rtt;
  int snwd = argT->snwd;
  struct sockaddr *addr = argT->addr;
  int *nPacketsSend = argT->nPacketsSend;
  int i, start, stop;
  while(1) {
     pthread_mutex_lock(&bufferC->mutexStart);
     start=bufferC->start;
     pthread_mutex_unlock(&bufferC->mutexStart);

     pthread_mutex_lock(&bufferC->mutexStop);
     stop=bufferC->stop;
     pthread_mutex_unlock(&bufferC->mutexStop);

    if(stop==-1){
      if(PRINT) printf("Thread envoi fini\n");
      return NULL;
    }
    int j=start;

    for(i=0;i<snwd;i++){
      //if(PRINT) printf("TEnvoi : %d to %d\n",start,stop);
    //  if(PRINT) printf("N buff case %d : %d\n",i,bufferC->buffer[i].numPck);
      j++;
      if(j==taille_buffer_circular) j = 0;
      if(j<stop || (start>stop && j<taille_buffer_circular) ){
        pthread_mutex_lock(&bufferC->buffer[i].mutexBuff);
        if(bufferC->buffer[i].timeWait <= 0 || bufferC->buffer[i].ackWarning > WARNING) {

          envoyerSegment(sock,addr,&bufferC->buffer[i]);
          *nPacketsSend = *nPacketsSend+1;
          bufferC->buffer[i].timeWait = rtt;
          bufferC->buffer[i].ackWarning=0;
        //  if(PRINT) printf("*******TEMPS RESET %d ****************\n",bufferC->buffer[i].timeWait);
        }
        pthread_mutex_unlock(&bufferC->buffer[i].mutexBuff);

      }
    }
  }
  return NULL;
}

int chargeBuff(char * bufferFile, int numSeg, int size, Buff_t * buff ){
  pthread_mutex_lock(&buff->mutexBuff);
	buff->buffer =bufferFile;
	buff->sizeBuff = size;
	buff->timeWait=0;
	buff->numPck = numSeg;
	buff->ackWarning=0;
	if(PRINT) printf("Buff %d, size %d, timeWait %d\n",buff->numPck,buff->sizeBuff,buff->timeWait);
  pthread_mutex_unlock(&buff->mutexBuff);
	return 1;
}

void *functionThreadReceive(void* arg) {

  ArgThreadReceive_t * argT = arg;
  BufferCircular_t *bufferC = argT->bufferC;
  int sock = argT->sock;
  int taille_buffer_circular = argT->taille_buffer_circular;
  int *rtt = argT->rtt;
  int *snwd = argT->snwd;
  struct sockaddr *addr = argT->addr;
  int *ackReceived = argT->ackReceived;
  char message_recu[2000];
  int newAck;
  int taille_addr_client = argT->taille_addr_client;
  int n_seg_total= argT->n_seg_total;
  while(1) {
    while(recvfrom(sock, message_recu, sizeof(message_recu),0,addr, (socklen_t*)&taille_addr_client) >0){
      if(strcmp(message_recu,"ACK") > 0){
        if(PRINT) printf("Recu : %s\n",message_recu);
        newAck = atoi(&message_recu[3]);
        if(PRINT) printf("ACK %d\n",newAck);
        int i;
        for(i=0;i<TAILLE_BUFFER_CIRCULAR;i++){
          pthread_mutex_lock(&bufferC->buffer[i].mutexBuff);
          if(bufferC->buffer[i].numPck == newAck+1) bufferC->buffer[i].ackWarning = bufferC->buffer[i].ackWarning +1;
          pthread_mutex_unlock(&bufferC->buffer[i].mutexBuff);
        }
        pthread_mutex_lock(argT->mutexAck);
        *ackReceived = newAck;
        pthread_mutex_unlock(argT->mutexAck);

        }
    }
  }
}

void *functionThreadTime(void* arg) {
  int *t = arg;
  if(PRINT) printf("functionThreadTime created arg==%d\n",*t );
  while(1) {
    usleep(100);
    if(*t > 0) {
      *t = *t-1;
      //if(PRINT) printf("%d\n",*t);
    }
  }
  return NULL;
}

BufferCircular_t * initBufferCircular(int taille_buffer_circular){
 Buff_t * pointeurFirstBuff = malloc(sizeof(Buff_t)*taille_buffer_circular);
 memset(pointeurFirstBuff,(int) '\0',taille_buffer_circular*sizeof(Buff_t));
 BufferCircular_t * bufferCircular = malloc(sizeof(BufferCircular_t));
 bufferCircular->buffer = pointeurFirstBuff;
 bufferCircular->start = 0;
 bufferCircular->stop = 0;
 pthread_mutex_init(&(bufferCircular->mutexStop), NULL);
 pthread_mutex_init(&(bufferCircular->mutexStart), NULL);
 return bufferCircular;
}

int startThreadTime(Buff_t * buff){
 if(pthread_create(&(buff->threadTime), NULL, functionThreadTime, &(buff->timeWait)) == -1) {
   perror("pthread_create");
   return EXIT_FAILURE;
 }
}
