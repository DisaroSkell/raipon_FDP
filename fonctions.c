#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions.h"

void * reception(void * argpointer){
    struct argsrec * args = argpointer;
    while(!(args->fin)) {
        ssize_t len = 0;
        ssize_t rcv_len = recv(args->socket, &len, sizeof(len), 0);
        if (rcv_len == -1) perror("Erreur réception taille message");
        if (rcv_len == 0) {
            printf("Non connecté au serveur, fin du thread\n");
            exit(0);
        }

        char * msg = (char *) malloc((len)*sizeof(char));
        ssize_t rcv = recv(args->socket, msg, len, 0);
        if (rcv == -1) perror("Erreur réception message");
        if (rcv == 0) {
            printf("Non connecté au serveur, fin du thread\n");
            exit(0);
        }

        printf("Message reçu : %s\n", msg);
    }
}

int envoi(int dS) {
    printf("Entrez un message\n");
    char * m = (char *) malloc(50*sizeof(char));
    fgets( m, 30*sizeof(char), stdin );

    size_t len= strlen(m)+1;
    int snd2 = send(dS, &len, sizeof(len), 0);
    if (snd2 == -1) perror("Erreur envoi taille message");

    int snd = send(dS, m, len , 0);
    if (snd == -1) perror("Erreur envoi message");
    printf("Message Envoyé \n");

    return strcmp(m, "fin\n") != 0;
}

void* traitement_serveur(void * paramspointer){

    struct traitement_params * params = paramspointer;

    int numclient = params->numclient;

    size_t len = 0;
    printf("%d: Client socket = %d\n", numclient, params->clienttab[numclient]);
    while (1) {
        ssize_t rcv_len = recv(params->clienttab[numclient], &len, sizeof(len), 0);
        if (rcv_len == -1) perror("Erreur réception taille message");
        if (rcv_len == 0) {
            printf("Non connecté au client, fin du thread\n");
            pthread_exit(0);
        }
        printf("%d: Longueur du message reçu: %d\n", numclient, (int)len);

        char * msg = (char *) malloc((len)*sizeof(char));
        ssize_t rcv = recv(params->clienttab[numclient], msg, len, 0);
        if (rcv == -1) perror("Erreur réception message");
        if (rcv == 0) {
            printf("Non connecté au client, fin du thread\n");
            pthread_exit(0);
        }
        else {
            printf("%d: Message reçu : %s\n", numclient, msg);
        }

        int end = strcmp(msg, "fin\n");

        if (end == 0) {
            char * messagedeco = "Déconnexion en cours...\n";
            int tailledeco = strlen(messagedeco);
            int senddecotaille = send(params->clienttab[numclient], &tailledeco, sizeof(tailledeco), 0);
            int senddeco = send(params->clienttab[numclient], messagedeco, tailledeco, 0);
            printf("%d: Fin du thread\n", numclient);
            pthread_exit(0);
        }

        struct commande cmd = gestion_commande(msg);

        if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "mp") == 0) {
            for(int i = 0; i<=100; i++){
                if (params->clienttab[i] == cmd.user) {
                    printf("%d: client = %d\n", numclient, params->clienttab[i]);
                    int taille = strlen(cmd.message);
                    int snd2 = send(params->clienttab[i], &taille, sizeof(size_t), 0);
                    if (snd2 == -1) perror("Erreur envoi taille message");
                    int snd = send(params->clienttab[i], cmd.message, strlen(cmd.message), 0);
                    if (snd == -1) perror("Erreur envoi message");
                    printf("%d: Message Envoyé au client %d: %s\n", numclient, params->clienttab[i], cmd.message);
                }
            }
        } else {
            for(int i = 0; i<=100; i++){
                if (params->clienttab[i] != params->clienttab[numclient] && params->clienttab[i] != 0) {
                    printf("%d: client = %d\n", numclient, params->clienttab[i]);
                    int snd2 = send(params->clienttab[i], &len, sizeof(len), 0);
                    if (snd2 == -1) perror("Erreur envoi taille message");
                    int snd = send(params->clienttab[i], msg, len, 0);
                    if (snd == -1) perror("Erreur envoi message");
                    printf("%d: Message Envoyé au client %d: %s\n", numclient, params->clienttab[i], msg);
                }
            }
        }
        free(msg);
    }
}

struct commande gestion_commande(char * slashmsg) {
    struct commande result;
    result.id_op = 0;
    result.nom_cmd = "";
    result.message = "";
    result.user = 0;

    if (memcmp(slashmsg, "/", strlen("/")) == 0) {
        result.id_op = 1;

        char * msg = (char *) malloc((strlen(slashmsg)-1)*sizeof(char));
        strcpy(msg, strtok(slashmsg, "/")); // On enlève le slash au début du message

        char * msgmodif = (char *) malloc((strlen(msg))*sizeof(char));
        strcpy(msgmodif, msg);

        char * token = strtok(msgmodif, " "); // On segmente la commande avec les espaces

        if (token == NULL) {
            perror("Vous devez mettre quelque chose après ce / !");
            result.id_op = -1;
            return result;
        }

        char * cmd = (char *) malloc(strlen(token)*sizeof(char));

        strcpy(cmd, token); // Le premier morceau est le nom de la commande

        if (strcmp(cmd,"mp") == 0) {
            result.nom_cmd = (char *) malloc(strlen("mp")*sizeof(char));
            strcpy(result.nom_cmd, "mp");

            token = strtok(NULL, " "); // On regarde la suite, ici: l'utilisateur

            if (token == NULL) {
                perror("Vous devez mettre l'utilisateur après /mp !");
                result.id_op = -1;
                return result;
            }

            char * userstr = malloc(strlen(token)*sizeof(char));
            strcpy(userstr,token);

            result.user = atoi(token);

            token = strtok(NULL, " "); // On regarde la suite, ici: le message

            if (token == NULL) {
                perror("Vous devez mettre l'un message après l'utilisateur !");
                result.id_op = -1;
                return result;
            }

            // La longueur du message à envoyer, c'est la longueur de la commande sans le slash (msg) sans la commande (result.nom_cmd) et sans l'utilisateur (result.user) (-2 avec les 2 espaces)
            char * mp = (char *) malloc((strlen(msg)-strlen(result.nom_cmd)-strlen(userstr)-2)*sizeof(char));
            strcpy(mp,"");

            while (token != NULL) {
                mp = strcat(mp, token);
                mp = strcat(mp, " ");

                token = strtok(NULL, " ");
            }

            result.message = (char *) malloc(strlen(mp)*sizeof(char));
            result.message = mp;
        } else {
            perror("Commande non reconnue");
            return result;
        }
    }
    return result;
}