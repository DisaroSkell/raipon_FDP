#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions.h"
#include <semaphore.h>
#include <signal.h>

client clients[nb_client_max];
sem_t semaphore;
sem_t semaphoreCli;
int socketServeur;

void * reception(void * argpointer){
    argsrec * args = argpointer;
    int len = 0;
    while(!(args->fin)) {
        ssize_t rcv_len = recv(args->socket, &len, sizeof(len), 0);
        if (rcv_len == -1) {
            perror("Erreur réception taille message");
            exit(0);
        }
        if (rcv_len == 0) {
            printf("Non connecté au serveur, fin du thread\n");
            exit(0);
        }

        char * msg = (char *) malloc((len)*sizeof(char));
        ssize_t rcv = recv(args->socket, msg, len, 0);
        if (rcv == -1) {
            perror("Erreur réception message");
            exit(0);
        }
        if (rcv == 0) {
            printf("Non connecté au serveur, fin du thread\n");
            exit(0);
        }
        else {
            printf("Message reçu :\n");
            printf("%s\n", msg);
        }
    }
}

int lecture_message(int dS) {
    printf("Entrez un message\n");
    char * m = (char *) malloc(50*sizeof(char));
    fgets(m, 30*sizeof(char), stdin);

    envoi_message(dS, m);

    return strcmp(m, "/fin\n") != 0;
}

// Envoie par le socket
int envoi_message(int socket, char * msg) {
    int resultat = 0;
    
    int len= strlen(msg)+1;

    char * message = (char *) malloc(len*sizeof(char));
    strcpy(message, msg);

    int sndlen = send(socket, &len, sizeof(len), 0);
    if (sndlen == -1) {
        perror("Erreur envoi taille message");
        resultat = -1;
    }
    else {
        int sndmsg = send(socket, message, len, 0);
        if (sndmsg == -1) {
            perror("Erreur envoi message");
            resultat = -1;
        }
        else printf("Message envoyé:\n%s\n", message);
    }

    return resultat;
}

void* traitement_serveur(void * paramspointer){

    traitement_params * params = paramspointer;

    int numclient = params->numclient;

    client init;
    init.socket = 0;
    init.pseudo = "";

    int len = 0;
    printf("%d: Client socket = %d\n", numclient, clients[numclient].socket);
    while (1) {
        ssize_t rcv_len = recv(clients[numclient].socket, &len, sizeof(len), 0);
        if (rcv_len == -1) perror("Erreur réception taille message");
        if (rcv_len == 0) {
            printf("Non connecté au client, fin du thread\n");
            
            if (sem_wait(&semaphoreCli) == -1) perror("Erreur blocage sémaphore");
            clients[numclient] = init;
            sem_post(&semaphoreCli);
            sem_post(&semaphore);
            
            pthread_exit(0);
        }
        printf("%d: Longueur du message reçu: %d\n", numclient, (int)len);

        char * msg = (char *) malloc((len)*sizeof(char));
        ssize_t rcv = recv(clients[numclient].socket, msg, len, 0);
        if (rcv == -1) perror("Erreur réception message");
        if (rcv == 0) {
            printf("Non connecté au client, fin du thread\n");
            
            if (sem_wait(&semaphoreCli) == -1) perror("Erreur blocage sémaphore");
            clients[numclient] = init;
            sem_post(&semaphoreCli);
            sem_post(&semaphore);
            
            pthread_exit(0);
        }
        else {
            printf("%d: Message reçu : %s\n", numclient, msg);
        }

        commande cmd = gestion_commande(msg);

        if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "mp") == 0) {
            int destinataire = chercher_client(cmd.user);
            if (destinataire == -1) {
                printf("Destinataire non trouvé !\n");
            } else {
                envoi_direct(destinataire, cmd.message, clients[numclient].pseudo);
            }
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "fin") == 0) {
            envoi_direct(numclient, "Déconnexion en cours...\n", "Serveur");
            
            printf("%d: Fin du thread\n", numclient);
            
            if (sem_wait(&semaphoreCli) == -1) perror("Erreur blocage sémaphore");
            clients[numclient] = init;
            sem_post(&semaphoreCli);
            sem_post(&semaphore);
            
            pthread_exit(0);
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "manuel") == 0) {
            envoi_direct(numclient, lire_manuel(), "Serveur");
        }
        else {
            for(int i = 0; i < nb_client_max; i++){
                if (clients[i].socket != clients[numclient].socket && clients[i].socket != 0) {
                    envoi_direct(i, msg, clients[numclient].pseudo);
                }
            }
        }
        free(msg);
    }
}

// Envoie par l'index dans le tableau clients
// On va envoyer le message comme ceci : "<pseudo> : message"
int envoi_direct(int numreceveur, char * msg, char * expediteur) {
    int resultat = 0;

    // "<" + "pseudo" + "> : " + msg + "\0"
    int len = 1 + strlen(expediteur) + 4 + strlen(msg) + 1;

    char * message = (char *) malloc(len*sizeof(char));
    strcpy(message, "<");
    message = strcat(message, expediteur);
    message = strcat(message, "> : ");
    message = strcat(message, msg);

    int sndlen = send(clients[numreceveur].socket, &len, sizeof(len), 0);
    if (sndlen == -1) {
        perror("Erreur envoi taille message");
        resultat = -1;
    }
    else {
        int sndmsg = send(clients[numreceveur].socket, message, len, 0);
        if (sndmsg == -1) {
            perror("Erreur envoi message");
            resultat = -1;
        }
        else printf("Message Envoyé au client %d (%s):\n%s\n", clients[numreceveur].socket, clients[numreceveur].pseudo, message);
    }

    return resultat;
}

// Cette fonction cherche un client en fonction de son pseudo.
// Renvoie -1 si non trouvé. Renvoie l'index dans le tableau clients sinon.
int chercher_client(char * pseudo) {
    int resultat = -1;

    for(int i = 0; i < nb_client_max; i++){
        if (strcmp(clients[i].pseudo, pseudo) == 0) {
            if (resultat != -1) {
                printf("Plusieurs personnes ont le même pseudo !\n");
                return resultat;
            }
            resultat = i;
        }
    }

    return resultat;
}

// Cette fonction cherche une place dans le tableau clients.
// Renvoie l'index de la place dans le tableau clients.
int chercher_place() {
    int resultat = -1;
    int i = 0;

    while (i < nb_client_max && resultat == -1) {
        if (clients[i].socket == 0) {
            resultat = i;
        }

        i++;
    }

    return resultat;
}

// Une partie du code de cette fonction a été trouvé sur ce site : https://www.codegrepper.com/code-examples/c/c+read+file+into+string
char * lire_manuel() {
    char * buffer = 0;
    long length;
    FILE * f = fopen ("manuel", "rb");

    if (f) {
        fseek (f, 0, SEEK_END);
        length = ftell (f);
        fseek (f, 0, SEEK_SET);
        buffer = malloc (length);
        if (buffer) {
            fread (buffer, 1, length, f);
        }
        fclose (f);
    }

    if (buffer) {
        return buffer;
    }
    else {
        perror("Problème dans la lecture du fichier");
    }
}

commande gestion_commande(char * slashmsg) {
    commande result;
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

            result.user = malloc(strlen(token)*sizeof(char));
            strcpy(result.user,token);

            token = strtok(NULL, " "); // On regarde la suite, ici: le message

            if (token == NULL) {
                perror("Vous devez mettre l'un message après l'utilisateur !");
                result.id_op = -1;
                return result;
            }

            // La longueur du message à envoyer, c'est la longueur de la commande sans le slash (msg) sans la commande (result.nom_cmd) et sans l'utilisateur (result.user) (-2 avec les 2 espaces)
            char * mp = (char *) malloc((strlen(msg)-strlen(result.nom_cmd)-strlen(result.user)-2)*sizeof(char));
            strcpy(mp,"");

            while (token != NULL) {
                mp = strcat(mp, token);
                mp = strcat(mp, " ");

                token = strtok(NULL, " ");
            }

            result.message = (char *) malloc(strlen(mp)*sizeof(char));
            result.message = mp;
        }
        else if (strcmp(cmd,"fin\n") == 0) {
            result.nom_cmd = (char *) malloc(strlen("fin")*sizeof(char));
            strcpy(result.nom_cmd, "fin");
        }
        else if (strcmp(cmd,"manuel\n") == 0) {
            result.nom_cmd = (char *) malloc(strlen("manuel")*sizeof(char));
            strcpy(result.nom_cmd, "manuel");
        }
        else {
            perror("Commande non reconnue");
            return result;
        }
    }
    return result;
}

void signal_handle(int sig){
    for(int i = 0; i < nb_client_max; i++){
        if (clients[i].socket != 0) {
            envoi_direct(i, "Fermeture du serveur\n", "Serveur");
        }
    }

    printf("Fin du programme\n");
    exit(0);
}

void signal_handleCli(int sig){
    envoi_message(socketServeur, "Cette personne se déconnecte.\n");

    printf("Fin du programme\n");
    exit(0);
}