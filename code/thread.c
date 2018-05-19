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
  struct sockaddr *addr = argT->addr;

  int i, start, stop;
  while(1) {
     start=bufferC->start;
     stop=bufferC->stop;
    if(stop==-1){
      printf("Thread envoi fini\n");
      return NULL;
    }
    int j=bufferC->start;

    for(i=0;i<TAILLE_BUFFER_CIRCULAR;i++){
      //printf("TEnvoi : %d to %d\n",start,stop);
    //  printf("N buff case %d : %d\n",i,bufferC->buffer[i].numPck);
      j++;
      if(j==TAILLE_BUFFER_CIRCULAR) j = 0;
      if(j<stop || (start>stop && j<TAILLE_BUFFER_CIRCULAR) ){
        if(bufferC->buffer[i].timeWait <= 0) {
          envoyerSegment(sock,addr,&bufferC->buffer[i]);
          bufferC->buffer[i].timeWait = RTT;
        //  printf("*******TEMPS RESET %d ****************\n",bufferC->buffer[i].timeWait);
        }
      }
    }
  }
  return NULL;
}

void *functionThreadReceive(void* arg) {
  Buff_t *buffer = arg;
  while(1) {
    sleep(1);
    buffer->numPck++;
    printf("NumPack buffer : %d\n",buffer->numPck);
  }
  return NULL;
}

void *functionThreadTime(void* arg) {
  int *t = arg;
  printf("functionThreadTime created arg==%d\n",*t );
  while(1) {
    usleep(1000);
    if(*t > 0) {
      *t-=1;
      //printf("%d\n",*t);
    }
  }
  return NULL;
}

BufferCircular_t * initBufferCircular(){
 Buff_t * pointeurFirstBuff = malloc(sizeof(Buff_t)*TAILLE_BUFFER_CIRCULAR);
 memset(pointeurFirstBuff,(int) '\0',TAILLE_BUFFER_CIRCULAR*sizeof(Buff_t));
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
