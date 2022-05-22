#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions_serveur.h"
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>


// Tableau de clients contenant tous les membres connectés
client clients[nb_clients_max*nb_channels_max];

// Sémaphore pour la connexion, gère la disponibilité des places
sem_t sem_clients_max;

// Sémaphore mutex pour protéger l'accès au tableau clients dans les zones critiques
sem_t sem_tab_clients;



// Tableau de channels contenant tous ceux disponibles
channel channels[nb_channels_max];

// Sémaphore pour la création de channel, gère la disponibilité des places
sem_t sem_channels_max;

// Sémaphore mutex pour protéger l'accès au tableau channels dans les zones critiques
sem_t sem_tab_channels;

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

    // Permet de savoir où se situe le client
    int numchan = -1; // Le général n'est pas dans le tableau channels
    char * chan = "Général";

    // On lui souhaite la bienvenue quand même
    envoi_direct(numclient, "Bienvenue dans le Général !", "Serveur", "Serveur");

    // client vide, utilisé pour remplacé ce client dans clients en cas de fermeture du thread
    client init;
    init.socket = 0;
    init.pseudo = "";

    printf("%d: Client socket = %d\n", numclient, clients[numclient].socket);
    while (1) {
        char * msg = reception_message(numclient);

        printf("%d: Message reçu : %s\n", numclient, msg);

        commande cmd = gestion_commande(msg);
        if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "mp") == 0) { // On envoie un mp
            int destinataire = chercher_client(cmd.user, nb_clients_max*nb_channels_max, clients);

            if (destinataire == -1) { // On envoie un feedback d'erreur au client
                envoi_direct(numclient, "Destinataire non trouvé !\n", "Serveur", "Serveur");
            } else {
                while (strchr(cmd.message, '\n') == NULL) {
                    strcat(cmd.message, reception_message(numclient));
                }

                envoi_direct(destinataire, cmd.message, clients[numclient].pseudo, "Direct");
            }
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "fin") == 0) {
            // On arrête tout
            envoi_direct(numclient, "Déconnexion en cours...\n", "Serveur", "Serveur");
            
            printf("%d: Fin du thread\n", numclient);
            
            if (sem_wait(&sem_tab_clients) == -1) perror("Erreur blocage sémaphore");
            clients[numclient] = init;
            sem_post(&sem_tab_clients);
            sem_post(&sem_clients_max);
            
            pthread_exit(0);
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "manuel") == 0) { // On envoie le manuel
            envoi_direct(numclient, lire_manuel(), "Serveur", "Serveur");
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "ef") == 0) { // On récupère un fichier de la part du client
            recup_fichier(clients[numclient].socket, cmd.nomf, cmd.taillef);
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "rf") == 0) { // On envoie un fichier au client
            if (cmd.taillef == -1) { // C'est comme ça qu'on détecte si aucun fichier n'est mis quand on est un shalg :)
                envoi_repertoire(numclient);
            } else {
                envoi_fichier(numclient, cmd.nomf);
            }
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "channel") == 0) { // On change de channel
            if (cmd.taillef == -1) { // On envoie la liste des channels
                for (int i = 0; i < nb_channels_max; i++) {
                    // On va faire une fonction d'envoi spécial
                    if (strcmp(channels[i].nom, "") != 0) {
                        envoi_direct(numclient, channels[i].nom, "Serveur", "Serveur");
                    }
                }
            } else {
                int suivant = chercher_channel(cmd.nomf); // On rappelle, le général n'est pas dans le tableau channels
                if (suivant == -1 && strcmp(cmd.nomf, "Général") != 0) { // On n'a pas trouvé le channel
                    envoi_direct(numclient, "Channel inconnu. /channel pour voir les channels disponible.", "Serveur", "Serveur");
                } else {
                    if (changer_channel(numclient, numchan, suivant) == -1) {
                        // Le channel suivant est plein

                        // On envoie "Le channel [nom] est plein."
                        char * pleintxt = (char *) malloc((11 + strlen(cmd.nomf) + 11)*sizeof(char));

                        strcpy(pleintxt,"Le channel ");
                        strcat(pleintxt, cmd.nomf);
                        strcat(pleintxt, " est plein.");

                        envoi_direct(numclient, pleintxt, "Serveur", "Serveur");
                    } else {
                        // Prévenir qu'on quitte le channel
                        numchan = suivant;
                        chan = (char *) malloc(strlen(cmd.nomf)*sizeof(char));
                        strcpy(chan, cmd.nomf);
                        if (numchan != -1) {
                            bienvenue(numclient, numchan);
                        }
                    }
                }
            }
        }
        else if (cmd.id_op == -1) { // On envoie un feedback d'erreur au client
            envoi_direct(numclient, "Commande non reconnue, faites /manuel pour plus d'informations\n", "Serveur", "Serveur");
        }
        else { // On envoie un message à tous les clients du channel
            if (strcmp(chan, "Général") == 0) {
                for(int i = 0; i < nb_clients_max*nb_channels_max; i++){
                    // On n'envoie pas de message au client qui envoie, ni aux client qui n'existent pas
                    if (i != numclient && clients[i].socket != 0) { // Les clients ont tous des sockets différents (ou vide)
                        envoi_direct(i, msg, clients[numclient].pseudo, chan);
                    }
                }
            } else {
                if (chercher_channel(chan) == -1) { // On ramène l'utilisateur dans le général car le channel n'existe plus
                    // On va envoyer "Le channel [nomchannel] n'existe plus, retour dans le général."
                    char * erreur = (char *) malloc((11 + strlen(chan) + 39)*sizeof(char));
                    strcpy(erreur, "Le channel ");
                    strcat(erreur, chan);
                    strcat(erreur, " n'existe plus, retour dans le général.");

                    envoi_direct(numclient, erreur, "Serveur", "Serveur");

                    numchan = -1;
                    chan = "Général";
                }
                // On envoie à tous les occupants du channel actuel
                for(int i = 0; i < nb_clients_max; i++){
                    // On n'envoie pas de message au client qui envoie, ni aux client qui n'existent pas
                    if (i != numclient && channels->occupants[i].socket != 0) { // Les clients ont tous des sockets différents (ou vide)
                        envoi_direct(i, msg, channels->occupants[numclient].pseudo, chan);
                    }
                }
            }
        }
        free(msg); // On libère la mémoire du message 
    }
}

int envoi_direct(int numreceveur, char * msg, char * expediteur, char * chan) {
    int resultat = 0;

    // "[" + chan + "] <" + pseudo + "> : " + msg + "\0"
    int len = 1 + strlen(chan) + 3 + strlen(expediteur) + 4 + strlen(msg) + 1;

    char * message = (char *) malloc(len*sizeof(char));
    
    strcpy(message, "[");
    strcat(message, chan);
    strcat(message, "] <");
    strcat(message, expediteur);
    strcat(message, "> : ");
    strcat(message, msg);

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

char * reception_message(int numclient) {
    // client vide, utilisé pour remplacé ce client dans clients en cas de fermeture du thread
    client init;
    init.socket = 0;
    init.pseudo = "";

    int len;

    // On reçoit la taille du message
    ssize_t rcv_len = recv(clients[numclient].socket, &len, sizeof(len), 0);
    if (rcv_len == -1) perror("Erreur réception taille message");
    if (rcv_len == 0) {
        // On arrête tout
        printf("Non connecté au client, fin du thread\n");
        
        if (sem_wait(&sem_tab_clients) == -1) perror("Erreur blocage sémaphore");
        clients[numclient] = init;
        sem_post(&sem_tab_clients);
        sem_post(&sem_clients_max);
        
        pthread_exit(0);
    }
    printf("%d: Longueur du message reçu: %d\n", numclient, (int)len);

    // On reçoit le message
    char * msg = (char *) malloc((len)*sizeof(char));
    ssize_t rcv = recv(clients[numclient].socket, msg, len, 0);
    if (rcv == -1) perror("Erreur réception message");
    if (rcv == 0) {
        // On arrête tout
        printf("Non connecté au client, fin du thread\n");
        
        if (sem_wait(&sem_tab_clients) == -1) perror("Erreur blocage sémaphore");
        clients[numclient] = init;
        sem_post(&sem_tab_clients);
        sem_post(&sem_clients_max);
        
        pthread_exit(0);
    }

    return msg;
}

int chercher_client(char * pseudo, int taille, client tabcli[taille]) {
    int resultat = -1;

    for(int i = 0; i < taille; i++){
        if (strcmp(tabcli[i].pseudo, pseudo) == 0) {
            if (resultat != -1) {
                perror("Plusieurs personnes ont le même pseudo !\n");
                return resultat;
            }
            resultat = i;
        }
    }

    return resultat;
}

int chercher_place(int taille, client tabcli[taille]) {
    int resultat = -1;
    int i = 0;

    while (i < taille && resultat == -1) {
        if (tabcli[i].socket == 0) {
            resultat = i;
        }

        i++;
    }

    return resultat;
}

int chercher_channel(char * nom) {
    int resultat = -1;

    for(int i = 0; i < nb_channels_max; i++){
        if (strcmp(channels[i].nom, nom) == 0) {
            if (resultat != -1) {
                printf("Plusieurs channels ont le même nom !\n");
                return resultat;
            }
            resultat = i;
        }
    }

    return resultat;
}

int changer_channel(int numclient, int numchan1, int numchan2) {
    // Client vide, on va en avoir besoin
    client init;
    init.socket = 0;
    init.pseudo = "";

    int result = 0;

    if (numchan1 == numchan2) {
        printf("Nous avons affaire à un petit clown :)\n");
        return -2;
    }

    if (numchan2 !=-1) {
        int placelibre = chercher_place(nb_clients_max, channels[numchan2].occupants);
        if (placelibre == nb_channels_max) { // Le channel est plein !!
            result = -1;
        } else {
            sem_wait(&sem_tab_channels);
            channels[numchan2].occupants[placelibre] = clients[numclient];
            sem_post(&sem_tab_channels);
        }
    } // On ne fait rien si on est dans le général

    if (numchan1 != -1 && result != -1) {
        sem_wait(&sem_tab_channels);
        int placelibre = chercher_client(clients[numclient].pseudo, nb_clients_max, channels[numchan1].occupants);
        channels[numchan1].occupants[placelibre] = init;
        sem_post(&sem_tab_channels);
    } // On ne fait rien si on est dans le général ou si on a un channel plein

    return result;
}

int bienvenue(int numclient, int numchan) {
    int result = 0;

    // On les stock pour éviter d'y accéder plusieurs fois
    char * pseudo = clients[numclient].pseudo;
    char * nomchan = channels[numchan].nom;
    char * descchan = channels[numchan].description;

    // On va envoyer "[[nomchannel]] Bienvenue à [pseudo] dans le channel!" à tous les membres du channel
    char * message1 = (char *) malloc((1 + strlen(nomchan) + 14 + strlen(pseudo) + 17)*sizeof(char));
    strcpy(message1, "[");
    strcat(message1, nomchan);
    strcat(message1, "] Bienvenue à ");
    strcat(message1, pseudo);
    strcat(message1, " dans le channel!");

    // On va envoyer "Vous arrivez dans le channel [nom du channel]:\n[description]" au client
    char * message2 = (char *) malloc((29 + strlen(nomchan) + 2 + strlen(descchan))*sizeof(char));
    strcpy(message2, "Vous arrivez dans le channel ");
    strcat(message2, nomchan);
    strcat(message2, ":\n");
    strcat(message2, descchan);

    for (int i = 0; i < nb_clients_max; i++) {
        if (clients[i].socket != 0) {
            if (i == numclient) {
                if (envoi_message(clients[i].socket, message2) == -1) {
                    result = -1;
                }
            } else {
                if (envoi_message(clients[i].socket, message1) == -1) {
                    result = -1;
                }
            }
        }
    }

    return result;
}

char * lire_manuel() {
    char * buffer = 0;
    long length;
    FILE * f = fopen ("Public/manuel", "rb");

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
        return "Manuel actuellement indisponible.\n";
    }
}

void envoi_repertoire(int numclient) {
    DIR *mydir;
    struct dirent *myfile;
    struct stat mystat;
    char * msg;
    mydir = opendir("Public");
    while ((myfile = readdir(mydir)) != NULL) {
        msg = (char *) malloc(30*sizeof(char));
        stat(myfile->d_name, &mystat);
        sprintf(msg,"%ld %s",mystat.st_size,myfile->d_name);
        printf("%s\n",msg);
        envoi_direct(numclient,msg, "Serveur", "Serveur");
        free(msg);
    }
}

void recup_fichier(int socket, char * nomfichier, long taillefichier) {
    FILE *fp;
    char buffer[SIZE];

    // On met le chemin relatif du fichier dans un string
    char * cheminf = (char *) malloc((strlen(nomfichier)+7)*sizeof(char));
    strcpy(cheminf, "Public/");
    strcat(cheminf, nomfichier);
    
    fp = fopen(cheminf, "w");
    
    ssize_t rcv_f;
    while (ftell(fp) < taillefichier) { // On s'arrête quand on a reçu l'équivalent de la taille du fichier
        rcv_f = recv(socket, buffer, SIZE, 0);
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
    printf("Fin de la réception du fichier %s\n", nomfichier);
}

void envoi_fichier(int numclient, char * nomfichier) {
    FILE *fp;

    // On met le chemin relatif du fichier dans un string
    char * nomchemin = (char *) malloc((strlen(nomfichier)+7)*sizeof(char));
    strcpy(nomchemin, "Public/");
    strcat(nomchemin, nomfichier);

    fp = fopen(nomchemin, "r");
    if (fp == NULL) {
        perror("Erreur durant la lecture du fichier");
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

    // La commande sera "/rf [nom fichier] [taille fichier]\0"
    // D'où les additions:
    char * commande = (char *) malloc((3 + strlen(nomfichier) + 1 + tailleint(taillefichier) + 1) * sizeof(char));

    strcpy(commande, "/ef ");
    strcat(commande, nomfichier);
    strcat(commande, " ");

    char * taillef = (char *) malloc((tailleint(taillefichier) + 1) * sizeof(char));
    sprintf(taillef, "%ld", taillefichier);
    strcat(commande, taillef);
    
    envoi_message(clients[numclient].socket, commande);

    // Après avoir envoyé la commande, on envoie le fichier
    while(fgets(data, SIZE, fp) != NULL) {
        ssize_t envoi = send(clients[numclient].socket, data, SIZE, 0);
        if (envoi == -1) {
            perror("Erreur dans l'envoi du fichier");
            return;
        }

        bzero(data, SIZE);
    }

    printf("Fichier envoyé à %s !\n", clients[numclient].pseudo);
}

commande gestion_commande(char * slashmsg) {
    commande result;
    result.id_op = 0;
    result.nom_cmd = "";
    result.message = "";
    result.user = "";
    result.nomf = "";
    result.taillef = 0;

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
                strcat(mp, token);
                strcat(mp, " ");

                token = strtok(NULL, " ");
            }

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
        else if (strcmp(cmd,"ef") == 0) {
            result.nom_cmd = (char *) malloc(3*sizeof(char));
            strcpy(result.nom_cmd, "ef");

            token = strtok(NULL, " "); // On regarde la suite, ici: le nom de fichier

            if (token == NULL) { // Ne devrait pas arriver, mais on sait jamais
                perror("Vous devez mettre le nom de fichier après /ef !");
                result.id_op = -1;
                return result;
            }
            
            result.nomf = (char *) malloc(strlen(token)*sizeof(char));
            strcpy(result.nomf, token);

            token = strtok(NULL, " "); // On regarde la suite, ici : la taille du fichier

            if (token == NULL) { // Ne devrait pas arriver, mais on sait jamais
                perror("Vous devez mettre la taille du fichier après son nom !");
                result.id_op = -1;
                return result;
            }

            result.taillef = atoi(token);
        }
        else if (strcmp(cmd,"rf") == 0 || strcmp(cmd,"rf\n") == 0) {
            result.nom_cmd = (char *) malloc(3*sizeof(char));
            strcpy(result.nom_cmd, "rf");

            token = strtok(NULL, " "); // On regarde la suite, ici: le nom de fichier

            if (token == NULL) { // S'il n'y en a pas on enverra la liste des fichiers
                result.taillef = -1;
            } else { // On donne le nom du fichier
                // On doit évidemment enlever le \n
                token = strtok(token, "\n");

                if (token == NULL) { // S'il n'y a pas de suite, c'est la situation d'avant
                    result.taillef = -1;
                } else {
                    result.nomf = (char *) malloc(strlen(token)*sizeof(char));
                    strcpy(result.nomf, token);
                }
            }
        }
        else if (strcmp(cmd,"channel") == 0 || strcmp(cmd,"channel\n") == 0) {
            result.nom_cmd = (char *) malloc(8*sizeof(char));
            strcpy(result.nom_cmd, "channel");

            token = strtok(NULL, " "); // On regarde la suite, ici: le channel à rejoindre

            // On utilise les mêmes variables que ef et rf
            if (token == NULL) { // S'il n'y en a pas on enverra la liste des channels
                result.taillef = -1;
            } else { // On donne le nom du channel
                // On doit évidemment enlever le \n
                token = strtok(token, "\n");

                if (token == NULL) { // S'il n'y a pas de suite, c'est la situation d'avant
                    result.taillef = -1;
                } else {
                    result.nomf = (char *) malloc(strlen(token)*sizeof(char));
                    strcpy(result.nomf, token);
                }
            }
        }
        else {
            perror("Commande non reconnue");
            result.id_op = -1;
            return result;
        }
    }
    return result;
}

void signal_handle(int sig){
    for(int i = 0; i < nb_clients_max*nb_channels_max; i++){
        if (clients[i].socket != 0) {
            envoi_direct(i, "Fermeture du serveur\n", "Serveur", "Serveur");
        }
    }

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