#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions_client.h"
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>

int socketServeur;
char * IPServeur;
int portServeur;

void * thread_reception(void * argpointer){

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
            if (memcmp(msg, "/ef ", strlen("/ef ")) == 0) {

                char * msgmodif = (char *) malloc((strlen(msg)-4)*sizeof(char));
                strcpy(msgmodif, strtok(msg, "/")); // On enlève le début du message
                
                char * token = strtok(msgmodif, " "); // On s'en fiche un peu du début
                token = strtok(NULL, " "); // On regarde le nom du fichier

                if (token == NULL) { // Ne devrait pas arriver, mais on sait jamais
                    perror("Il manque un nom de fichier après /ef !");
                    continue;;
                }
                
                char * nomf = (char *) malloc(strlen(token)*sizeof(char));
                strcpy(nomf, token);

                token = strtok(NULL, " "); // On regarde la suite, ici : la taille du fichier

                if (token == NULL) { // Ne devrait pas arriver, mais on sait jamais
                    perror("Il manque la taille du fichier après son nom !");
                    continue;
                }

                int taillef = atoi(token);

                pthread_t t;
                argsfichier argsf;

                ssize_t rcvIP = recv(args->socket, &(argsf.IP), (3*sizeof(int)), 0);
                if (rcvIP == -1) {
                    perror("Erreur réception numéro client");
                    exit(0);
                }
                if (rcvIP == 0) {
                    printf("Non connecté au serveur, fin du thread\n");
                    exit(0);
                }

                argsf.socket = args->socket;
                argsf.nomf = nomf;
                argsf.taillef = taillef;
                argsf.action = 0;
                int thread = pthread_create(&t,NULL,thread_fichier,&argsf);
            } else {
                printf("\n");
                printf("%s", msg);
            }
        }
    }
}

int lecture_message(int dS) {
    printf("Entrez un message\n");

    // On lit le message dans le terminal
    int taille = 30;
    char * m = (char *) malloc(taille*sizeof(char));
    fgets(m, taille*sizeof(char), stdin);

    if (strcmp(m, "/envoifichier\n") == 0 || strcmp(m, "/ef\n") == 0) {
        // On montre au client les fichiers disponibles à l'envoi
        print_repertoire();
    } else if (strncmp(m, "/envoificher ", 13) == 0 || strncmp(m, "/ef ", 4) == 0) {
        // On envoie un ficher
        char * token = strtok(m, " ");
        token = strtok(NULL, " "); // On regarde le nom de fichier qui est censé se situer juste après le nom de commande

        if (token == NULL) {
            // On est gentil, on fait comme s'il demandait la liste des fichiers :)
            print_repertoire();
        }
        else {
            argsfichier argsf;
            
            token = strtok(token, "\n");
            pthread_t t;

            argsf.socket = dS;
            argsf.nomf = token;
            argsf.action = 1;
            int thread = pthread_create(&t,NULL,thread_fichier,&argsf);
        }
    }
    else envoi_message(dS, m);

    return strcmp(m, "/fin\n") != 0;
}

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

void print_repertoire() {
    DIR *mydir;
    struct dirent *myfile;
    struct stat mystat;
    mydir = opendir("Public");
    while ((myfile = readdir(mydir)) != NULL) {
        stat(myfile->d_name, &mystat);
        printf("%ld",mystat.st_size);
        printf(" %s\n", myfile->d_name);
    }
}

void * thread_fichier(void * argpointer) {
    argsfichier * args = argpointer;

    if (args->action == 1) { // Le client envoie le fichier
        envoi_fichier(args->socket, args->nomf, args->destinataire);
        pthread_exit(0);
    }

    // Le client reçoit le fichier
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1){
        perror("Erreur dans la création de la socket");
        exit(0);
    }
    printf("Socket Créé\n");

    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    inet_pton(AF_INET,args->IP,&(aS.sin_addr));
    aS.sin_port = htons(8000);
    socklen_t lgA = sizeof(struct sockaddr_in);

    int co = connect(dS, (struct sockaddr *) &aS, lgA);
    if (co == -1){
        perror("Erreur envoi demmande de connexion");
        exit(0);
    }

    printf("Socket Connecté\n");

    recup_fichier(dS, args->nomf, args->taillef);

    pthread_exit(0);
}

void envoi_fichier(int dS, char * nomfichier, char * destinataire) {
    FILE *fp;

    // On met le chemin relatif du fichier dans un string
    char * nomchemin = (char *) malloc((strlen(nomfichier)+7)*sizeof(char));
    strcpy(nomchemin, "Public/");
    strcat(nomchemin, nomfichier);

    fp = fopen(nomchemin, "r");
    if (fp == NULL) {
        perror("Erreur durant la lecture du fichier, non trouvé");
        return;
    }

    char data[SIZE] = {0};

    long int taillefichier;

    if (fp) {
        fseek (fp, 0, SEEK_END);
        taillefichier = ftell (fp);
        fseek (fp, 0, SEEK_SET);
    } else {
        perror("Problème dans la lecture du fichier");
    }

    char * commande = (char *) malloc((3 + strlen(nomfichier) + 1 + tailleint(taillefichier) + 1) * sizeof(char));

    strcpy(commande, "/ef ");
    strcat(commande, destinataire);
    strcat(commande, " ");
    strcat(commande, nomfichier);
    strcat(commande, " ");

    char * taillef = (char *) malloc((tailleint(taillefichier) + 1) * sizeof(char));
    sprintf(taillef, "%ld", taillefichier);
    strcat(commande, taillef);
    
    envoi_message(dS, commande);

    // On crée une nouvelle socket de type serveur

    int sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock == -1){
            perror("Erreur dans la création de la socket");
            exit(0);
        }
        printf("Socket Créé\n");

        struct sockaddr_in ad;
        ad.sin_family = AF_INET;
        ad.sin_addr.s_addr = INADDR_ANY;
        ad.sin_port = htons(8000);
        int bd = bind(sock, (struct sockaddr*)&ad, sizeof(ad));
        if (bd == -1){
            perror("Erreur nommage de la socket");
            exit(0);
        }

        printf("Socket Nommé\n");

        int lstn = listen(sock, 7);
        if (lstn == -1){
            perror("Erreur passage en écoute");
            exit(0);
        }

        printf("Mode écoute\n");

        struct sockaddr_in aC;
        socklen_t lg = sizeof(struct sockaddr_in);

        int dSC = accept(sock, (struct sockaddr*) &aC,&lg);
        if (dSC == -1){
            perror("Erreur connexion non acceptée");
            exit(0);
            }

    // On envoie le fichier
    while(fgets(data, SIZE, fp) != NULL) {
        ssize_t envoi = send(dSC, data, SIZE, 0);
        if (envoi == -1) {
            perror("Erreur dans l'envoi du fichier");
            return;
        }

        bzero(data, SIZE);
    }

    printf("Fichier envoyé !");
}

void recup_fichier(int dS, char * nomf, long taillef) {
    
    FILE *fp;
    char buffer[SIZE];

    // On met le chemin relatif du fichier dans un string
    char * cheminf = (char *) malloc((strlen(nomf)+7)*sizeof(char));
    strcpy(cheminf, "Public/");
    strcat(cheminf, nomf);
    
    fp = fopen(cheminf, "w");
    
    ssize_t rcv_f;
    while (ftell(fp) < taillef) { // On s'arrête quand on a reçu l'équivalent de la taille du fichier
        rcv_f = recv(dS, buffer, SIZE, 0);
        if (rcv_f == -1) {
            perror("Erreur de reception du fichier !");
            return;
        }
        if (rcv_f == 0) {
            perror("Erreur de connexion au client !");
            return;
        }

        fprintf(fp, "%s", buffer);
    }

    fclose(fp);
    printf("Fin de la réception du fichier %s\n", nomf);
}

void signal_handleCli(int sig){
    envoi_message(socketServeur, "/fin\n");

    printf("Fin du programme\n");
    exit(0);
}

int tailleint(int nb) {
    int nombre = nb;
    int resultat = 1;

    // On compte les chiffres en divisant par 10 à chaque fois
    while (nombre >= 10) {
        nombre = nombre/10;
        resultat++;
    }

    return resultat;
}