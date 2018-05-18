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
  BufferCircular_t *bufferC = arg;
  int i;
  while(1) {
    for(i=0;i<SNWD;i++){
      if(bufferC->buffer[i].timeWait <= 0) {
        bufferC->buffer[i].timeWait = RTT;
        printf("*******TEMPS RESET %d ****************\n",bufferC->buffer[i].timeWait);
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

BufferCircular_t * initBufferCircular(int size){
 Buff_t * pointeurFirstBuff = malloc(sizeof(Buff_t)*size);
 memset(pointeurFirstBuff,(int) '\0',size*sizeof(Buff_t));
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
