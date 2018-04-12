#ifndef _TCP_H
#define _TCP_H

#define adresse_serveur "10.43.7.43"
#define adresse_loopback "127.0.0.1"

#define maxConnection 5
#define maxPort 6

#define TAILLE_MAX_SEGMENT 512
#define TAILLE_ENTETE 6

int port[maxPort]; //varie entre 6000 et 6005

int connectServer(int port, int pid[maxConnection],int i);

int initialization_socket(int port);

int portDispo(char port[4]);


int envoyerBinary(int sock, char nom_fichier[64]);
int receive(int sock, char nom_fichier[64]);



#endif
