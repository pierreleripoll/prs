#ifndef _TCP_H
#define _TCP_H

#define adresse_serveur "10.43.7.43"
#define adresse_loopback "127.0.0.1"


#define maxConnection 5
#define maxPort 6

#define TAILLE_MAX_SEGMENT 1500
#define TAILLE_ENTETE 6
#define TAILLE_UTILE (TAILLE_MAX_SEGMENT-TAILLE_ENTETE)

#define TAILLE_BUFFER_CIRCULAR 30
#define SNWD 30
#define RTT 10 //en dixième de milliseconde

typedef struct Buff {
  int numPck;
  char * buffer;
  int timeWait;
  pthread_t threadTime;
  int sizeBuff;
} Buff_t;

typedef struct bufferCircular{
  Buff_t * buffer;
  int start;
  int stop;
  pthread_mutex_t mutexStart;
  pthread_mutex_t mutexStop;
} BufferCircular_t;

typedef struct ArgThreadEnvoi{
  int rtt;
  int taille_buffer_circular;
  int * nPacketsSend;
  BufferCircular_t * bufferC;
  int sock;
  struct sockaddr *addr;
}ArgThreadEnvoi_t;


int port[maxPort]; //varie entre 6000 et 6005

int connectServer(int port, int* udp_descripteur, int pid[maxConnection],int i);

int initialization_socket(int port);

int portDispo(char port[4]);

int envoyerBinary(int sock,struct sockaddr *addr, char nom_fichier[64]);
//int envoyerSegment(int sock, struct sockaddr *addr, int numSegment, char * buff,int sizeFile);
int envoyerSegment(int sock, struct sockaddr *addr, Buff_t * buff);
int loadFile(char * buff, char nom_fichier[64]);
int chargeBuff(char * bufferFile, int numSeg, int size, Buff_t * buff );


char *initBuff(int sizeFile);
int receive(int sock, char nom_fichier[64]);

int max(int x, int y);
void remiseAZero(int pid[maxConnection]);

void *functionThreadSend(void* arg);
void *functionThreadReceive(void* arg);
void *functionThreadTime(void* arg);

int startThreadTime(Buff_t * buff);




BufferCircular_t * initBufferCircular(int taille_buffer_circular);


#endif
