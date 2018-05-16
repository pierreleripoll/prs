#ifndef _TCP_H
#define _TCP_H

#define adresse_serveur "10.43.7.43"
#define adresse_loopback "127.0.0.1"


#define maxConnection 5
#define maxPort 6

#define TAILLE_MAX_SEGMENT 1500
#define TAILLE_ENTETE 6
#define TAILLE_UTILE (TAILLE_MAX_SEGMENT-TAILLE_ENTETE)

#define SNWD 10

int port[maxPort]; //varie entre 6000 et 6005

int connectServer(int port, int* udp_descripteur, int pid[maxConnection],int i);

int initialization_socket(int port);

int portDispo(char port[4]);

int envoyerBinary(int sock,struct sockaddr *addr, char nom_fichier[64]);
int envoyerSegment(int sock, struct sockaddr *addr, int numSegment, char * buff,int sizeFile);
int loadFile(char * buff, char nom_fichier[64]);

char *initBuff(int sizeFile);
int receive(int sock, char nom_fichier[64]);

int max(int x, int y);
void remiseAZero(int pid[maxConnection]);


typedef struct Buff {
  int numPck;
  char buffer[TAILLE_UTILE];
  int timeWait;
} Buff_t;

Buff_t * initBufferCircular(int size);


#endif
