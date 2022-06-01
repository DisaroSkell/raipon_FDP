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
#include <unistd.h>


// Tableau de clients contenant tous les membres connectés
client clients[nb_clients_max*nb_channels_max];

// Sémaphore pour la connexion, gère la disponibilité des places
sem_t sem_clients_max;

// Sémaphore mutex pour protéger l'accès au tableau clients dans les zones critiques
sem_t sem_tab_clients;



// Tableau de channels contenant tous ceux disponibles
channel channels[nb_channels_max];

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
    free(message);
    return resultat;
}

void* traitement_serveur(void * paramspointer){

    traitement_params * params = paramspointer;

    int numclient = params->numclient;

    // Permet de savoir où se situe le client
    int numchan = -1; // Le général n'est pas dans le tableau channels
    char * chan = "Général";
    int posclient = -1; // Position du client dans le tableau occupants du channel

    // On lui souhaite la bienvenue quand même
    envoi_direct(numclient, "Bienvenue dans le Général !\n", "Serveur", -10);

    printf("%d: Client socket = %d\n", numclient, clients[numclient].socket);
    while (1) {
        char * msg = reception_message(numclient, numchan, posclient);

        printf("%d: Message reçu : %s\n", numclient, msg);

        commande cmd = gestion_commande(msg, numclient, numchan, posclient);
        if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "mp") == 0) { // On envoie un mp
            free(cmd.nom_cmd);
            int destinataire = chercher_client(cmd.user, nb_clients_max*nb_channels_max, clients);

            if (destinataire == -1) { // On envoie un feedback d'erreur au client
                envoi_direct(numclient, "Destinataire non trouvé !\n", "Serveur", -10);
            } else {
                while (cmd.message[strlen(cmd.message)-1] != '\n') {
                    char * part1 = (char *) malloc((strlen(cmd.message)+1)*sizeof(char));
                    strcpy(part1, cmd.message);

                    char * part2 = reception_message(numclient, numchan, posclient);

                    free(cmd.message);

                    cmd.message = (char *) malloc((strlen(part1)+strlen(part2)+1)*sizeof(char));
                    strcpy(cmd.message, part1);
                    strcat(cmd.message, part2);

                    free(part1);
                    free(part2);
                }
                cmd.message = censure(cmd.message);
                
                envoi_direct(destinataire, cmd.message, clients[numclient].pseudo, -2);
            }
            free(cmd.message);
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "fin") == 0) {
            free(cmd.nom_cmd);
            // On arrête tout
            envoi_direct(numclient, "Déconnexion en cours...\n", "Serveur", -10);
            
            deconnexion(numclient, numchan, posclient);
            
            pthread_exit(0);
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "manuel") == 0) { // On envoie le manuel
            free(cmd.nom_cmd);

            char * manuel = lire_manuel();
            envoi_direct(numclient, manuel, "Serveur", -10);
            free(manuel);
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "ef") == 0) { // On va connecter deux clients pour un envoi
            free(cmd.nom_cmd);
            int destinataire = chercher_client(cmd.user, nb_clients_max*nb_channels_max, clients);
            free(cmd.user);
            if (destinataire == -1) { // On envoie un feedback d'erreur au client
                envoi_direct(numclient, "Destinataire non trouvé !\n", "Serveur", -10);
            }

            char * taillefichier = (char *) malloc(4 * sizeof(char));
            sprintf(taillefichier, "%d", cmd.taillef);
            char * co = (char *) malloc((3 + strlen(cmd.nomf) + 1 + strlen(taillefichier) + 1) * sizeof(char));
            
            strcpy(co, "/ef ");
            strcat(co, cmd.nomf);
            strcat(co, " ");
            strcat(co, taillefichier);
            strcat(co, " ");
            strcat(co, clients[numclient].IP);
            
            envoi_message(clients[destinataire].socket, co);
            free(taillefichier);
            free(co);
            // free(cmd.nomf); // Il ne veut pas se free
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "rejoindre") == 0) { // On rejoint un channel
            free(cmd.nom_cmd);
            
            int suivant = chercher_channel(cmd.nomf); // Le général n'est pas dans la liste des channels
            if (suivant == -1 && strcmp(cmd.nomf, "Général") != 0) { // On n'a pas trouvé le channel
                envoi_direct(numclient, "Channel inconnu. /channel pour voir les channels disponible.\n", "Serveur", -10);
            } else {
                int place = rejoindre_channel(numclient, suivant);
                if (place == -2) {
                    // Le channel à rejoindre est plein

                    // On envoie "Le channel [nom] est plein.\n"
                    char * pleintxt = (char *) malloc((11 + strlen(cmd.nomf) + 12)*sizeof(char));

                    strcpy(pleintxt,"Le channel ");
                    strcat(pleintxt, cmd.nomf);
                    strcat(pleintxt, " est plein.\n");
                    free(cmd.nomf);

                    envoi_direct(numclient, pleintxt, "Serveur", -10);
                    free(pleintxt);
                } else {
                    free(cmd.nomf);

                    // On va à la destination
                    if (place != -4) {
                        chan = channels[suivant].nom;
                        numchan = suivant;
                        posclient = place;
                        bienvenue(numclient, numchan);
                    } else {
                        envoi_direct(numclient, "Vous avez déjà rejoint ce channel!\n", "Serveur", -10);
                    }
                }
            }
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "quitter") == 0) { // On quitte un channel
            free(cmd.nom_cmd);
            
            if (strcmp(cmd.nomf, "Général") == 0) { // On n'a pas le droit de quitter le général
                envoi_direct(numclient, "Vous n'avez pas le droit de quitter le Général.\n", "Serveur", -10);
            }
            int vaten = chercher_channel(cmd.nomf);
            if (vaten == -1) { // On n'a pas trouvé le channel
                envoi_direct(numclient, "Channel inconnu. /channel pour voir les channels disponible.\n", "Serveur", -10);
            } else {
                aurevoir(numclient, vaten);
                // Client vide
                client init;
                init.socket = 0;
                init.pseudo = "";
                init.IP = "";

                if (sem_wait(&sem_tab_channels) == -1) perror("Erreur blocage sémaphore");
                channels[vaten].occupants[posclient] = init;
                sem_post(&sem_tab_channels);

                if (numchan == vaten) {
                    numchan = -1;
                    chan = "Général";
                    posclient = -1;

                    envoi_direct(numclient, "Vous écrivez maintenant dans le général\n", "Serveur", -10);
                }
            }
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "channel") == 0) { // On change de channel
            free(cmd.nom_cmd);
            if (cmd.taillef == -1) { // On envoie la liste des channels
                envoi_channels(numclient);
            } else {
                if (strcmp(cmd.nomf, "Général") == 0) { // On va écrire dans le général
                    numchan = -1;
                    chan = "Général";
                    posclient = -1;
                    
                    envoi_direct(numclient, "Vous écrivez maintenant dans le général\n", "Serveur", -10);
                } else {
                    int suivant = chercher_channel(cmd.nomf); // On rappelle, le général n'est pas dans le tableau channels
                    if (suivant == -1) { // On n'a pas trouvé le channel
                        envoi_direct(numclient, "Channel inconnu. /channel pour voir les channels disponible.\n", "Serveur", -10);
                    } else {
                        int place = chercher_client(clients[numclient].pseudo, nb_clients_max, channels[suivant].occupants);
                        if (place == -1) {
                            // Le client n'est pas inscrit dans le channel

                            // On envoie "Vous n'êtes pas inscrits dans le channel [nom].\n"
                            char * refus = (char *) malloc((strlen("Vous n'êtes pas inscrits dans le channel ") + strlen(cmd.nomf) + 3)*sizeof(char));

                            strcpy(refus,"Vous n'êtes pas inscrits dans le channel ");
                            strcat(refus, cmd.nomf);
                            strcat(refus, ".\n");
                            free(cmd.nomf);

                            envoi_direct(numclient, refus, "Serveur", -10);
                            free(refus);
                        } else {

                            // On va à la destination
                            chan = channels[suivant].nom;
                            numchan = suivant;
                            posclient = place;
                            
                            // On envoie "Vous écrivez dans le channel [nom].\n"
                            char * acceptation = (char *) malloc((strlen("Vous écrivez dans le channel ") + strlen(cmd.nomf) + 3)*sizeof(char));

                            strcpy(acceptation, "Vous écrivez dans le channel ");
                            strcat(acceptation, cmd.nomf);
                            strcat(acceptation, ".\n");
                            free(cmd.nomf);
                            
                            envoi_direct(numclient, acceptation, "Serveur", -10);
                            free(acceptation);
                        }
                    }
                }
            }
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "cc") == 0) {
            free(cmd.nom_cmd);
            if (chercher_channel(cmd.nomf) != -1 || strcmp(cmd.nomf, "Général") == 0 || strcmp(cmd.nomf, "Direct") == 0) {
                // On n'a pas le droit de créer un autre Général (logique) ou un channel Direct (pour éviter la confusion avec les mp)
                free(cmd.nomf);
                free(cmd.message);
                envoi_direct(numclient, "Channel déjà existant. /channel pour les voir tous.\n", "Serveur", -10);
            } else {
                if (sem_wait(&sem_tab_channels) == -1) perror("Erreur blocage sémaphore");

                int i = chercher_place_chan(nb_channels_max, channels);

                if (i != -1) {
                    channels[i].nom = (char *) malloc(strlen(cmd.nomf)*sizeof(char));
                    strcpy(channels[i].nom, cmd.nomf);

                    channels[i].description = (char *) malloc(strlen(cmd.message)*sizeof(char));
                    strcpy(channels[i].description, cmd.message);
                    sem_post(&sem_tab_channels);

                    envoi_direct(numclient, "Le channel a bien été créé !\n", "Serveur", -10);
                } else {
                    sem_post(&sem_tab_channels);
                    envoi_direct(numclient, "Il y a déjà trop de channels. Action refusée. (désolé)\n", "Serveur", -10);
                }
                free(cmd.nomf);
                free(cmd.message);
            }
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "mc") == 0) {
            free(cmd.nom_cmd);
            if (sem_wait(&sem_tab_channels) == -1) perror("Erreur blocage sémaphore");

            int indice = chercher_channel(cmd.nomf);
            free(cmd.nomf);
            if (indice == -1) {
                free(cmd.message);
                envoi_direct(numclient, "Channel non existant. /channel pour les voir tous.\n", "Serveur", -10);
                sem_post(&sem_tab_channels);
            } else {
                free(channels[indice].description);
                channels[indice].description = (char *) malloc((strlen(cmd.message + 1)*sizeof(char)));
                strcpy(channels[indice].description, cmd.message);
                free(cmd.message);

                sem_post(&sem_tab_channels);

                envoi_direct(numclient, "Le channel a bien été modifié !\n", "Serveur", -10);
            }
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "sc") == 0) {
            free(cmd.nom_cmd);
            if (sem_wait(&sem_tab_channels) == -1) perror("Erreur blocage sémaphore");
            
            int indice = chercher_channel(cmd.nomf);
            free(cmd.nomf);
            if (indice == -1) {
                envoi_direct(numclient, "Channel non existant. /channel pour les voir tous.\n", "Serveur", -10);
                sem_post(&sem_tab_channels);
            } else {
                // Channel vide
                channel blank;
                blank.nom = "";
                blank.description = "";

                // On envoie à tous les occupants du channel pour les notifier
                for(int i = 0; i < nb_clients_max; i++){
                    // On n'envoie pas de message aux client qui n'existent pas
                    if (channels->occupants[i].socket != 0) {
                        envoi_direct(i, "Suppression du channel. Retour au général.\n", "Serveur", indice);
                    }
                }
                
                free(channels[indice].nom);
                free(channels[indice].description);
                channels[indice] = blank;

                sem_post(&sem_tab_channels);

                envoi_direct(numclient, "Le channel a bien été supprimé !\n", "Serveur", -10);
            }
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "membres") == 0) {
            free(cmd.nom_cmd);
            if (cmd.taillef == -1) { // On envoie la liste du channel actuel
                envoi_membres(numclient, numchan);
            } else {
                int chancherche = chercher_channel(cmd.nomf); // On rappelle, le général n'est pas dans le tableau channels
                if (chancherche == -1 && strcmp(cmd.nomf, "Général") != 0) { // On n'a pas trouvé le channel
                    envoi_direct(numclient, "Channel inconnu. /channel pour voir les channels disponible.\n", "Serveur", -10);
                } else {
                    envoi_membres(numclient, chancherche);
                }
            }
        }
        else if (cmd.id_op == 1 && strcmp(cmd.nom_cmd, "turnoff") == 0) {
            fermeture_serveur();
        }
        else if (cmd.id_op == -1) { // On envoie un feedback d'erreur au client
            envoi_direct(numclient, "Commande non reconnue, faites /manuel pour plus d'informations\n", "Serveur", -10);
        }
        else { // On envoie un message à tous les clients du channel
            msg = censure(msg);
            if (strcmp(chan, "Général") == 0) {
                for(int i = 0; i < nb_clients_max*nb_channels_max; i++){
                    // On n'envoie pas de message au client qui envoie, ni aux client qui n'existent pas
                    if (i != numclient && clients[i].socket != 0) { // Les clients ont tous des sockets différents (ou vide)
                        envoi_direct(i, msg, clients[numclient].pseudo, numchan);
                    }
                }
            } else {
                if (chercher_channel(chan) == -1) { // On ramène l'utilisateur dans le général car le channel n'existe plus
                    // On va envoyer "Le channel [nomchannel] n'existe plus, retour dans le général.\n"
                    char * erreur = (char *) malloc((11 + strlen(chan) + 40)*sizeof(char));
                    strcpy(erreur, "Le channel ");
                    strcat(erreur, chan);
                    strcat(erreur, " n'existe plus, retour dans le général.\n");

                    envoi_direct(numclient, erreur, "Serveur", -10);
                    free(erreur);

                    numchan = -1;
                    chan = "Général";
                    posclient = -1;
                } else {
                    // On envoie à tous les occupants du channel actuel
                    for(int i = 0; i < nb_clients_max; i++){
                        // On n'envoie pas de message au client qui envoie, ni aux client qui n'existent pas
                        if (i != posclient && channels[numchan].occupants[i].socket != 0) { // Les clients ont tous des sockets différents (ou vide)
                            envoi_direct(i, msg, clients[numclient].pseudo, numchan);
                        }
                    }
                }
            }
        }
        free(msg); // On libère la mémoire du message 
    }
}

int envoi_direct(int numreceveur, char * msg, char * expediteur, int numchan) {
    int resultat = 0;
    char * chan;
    int socketreceveur;

    if (numchan == -1) {
        chan = "Général";
        socketreceveur = clients[numreceveur].socket;
    } else if (numchan == -2) {
        chan = "Direct";
        socketreceveur = clients[numreceveur].socket;
    } else if (numchan >= 0 && numchan < nb_channels_max) {
        chan = channels[numchan].nom;
        socketreceveur = channels[numchan].occupants[numreceveur].socket;
    } else {
        chan = "";
        socketreceveur = clients[numreceveur].socket;
    }

    int len;
    char * message;

    if (strcmp(chan, "") == 0) {
        // "<" + pseudo + "> : " + msg + "\0"
        len = 1 + strlen(expediteur) + 4 + strlen(msg) + 1;

        message = (char *) malloc(len*sizeof(char));
        
        strcpy(message, "<");
    } else {
        // "[" + chan + "] <" + pseudo + "> : " + msg + "\0"
        len = 1 + strlen(chan) + 3 + strlen(expediteur) + 4 + strlen(msg) + 1;

        message = (char *) malloc(len*sizeof(char));
        
        strcpy(message, "[");
        strcat(message, chan);
        strcat(message, "] <");
    }

    strcat(message, expediteur);
    strcat(message, "> : ");
    strcat(message, msg);

    int sndlen = send(socketreceveur, &len, sizeof(len), 0);
    if (sndlen == -1) {
        perror("Erreur envoi taille message");
        resultat = -1;
    }
    else {
        int sndmsg = send(socketreceveur, message, len, 0);
        if (sndmsg == -1) {
            perror("Erreur envoi message");
            resultat = -1;
        }
        else printf("Message Envoyé au client %d (%s):\n%s\n", socketreceveur, clients[numreceveur].pseudo, message);
    }
    free(message);
    return resultat;
}

char * reception_message(int numclient, int numchan, int posclient) {
    int len;

    // On reçoit la taille du message
    ssize_t rcv_len = recv(clients[numclient].socket, &len, sizeof(len), 0);
    if (rcv_len == -1) {
        perror("Erreur réception taille message");
        envoi_direct(numclient, "Erreur réception taille message\n", "Serveur", -10);
    } 
    if (rcv_len == 0) {
        // On arrête tout
        deconnexion(numclient, numchan, posclient);
    }
    printf("%d: Longueur du message reçu: %d\n", numclient, (int)len);

    // On reçoit le message
    char * msg = (char *) malloc((len)*sizeof(char));
    ssize_t rcv = recv(clients[numclient].socket, msg, len, 0);
    if (rcv == -1){
        perror("Erreur réception message");
        envoi_direct(numclient, "Erreur réception taille message\n", "Serveur", -10);
    } 
    if (rcv == 0) {
        // On arrête tout
        deconnexion(numclient, numchan, posclient);
    }

    return msg;
}

int chercher_client(char * pseudo, int taille, client tabcli[taille]) {
    int resultat = -1;

    if (strcmp(pseudo, "") == 0) {
        perror("Pseudo à chercher vide");
        return -1;
    }

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

int chercher_place_cli(int taille, client tabcli[taille]) {
    int i = 0;

    while (i < taille && tabcli[i].socket !=0) {
        i++;
    }

    return i;
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

int chercher_place_chan(int taille, channel tabchan[taille]) {
    int resultat = -1;
    int i = 0;

    while (i < taille && resultat == -1) {
        if (strcmp(tabchan[i].nom, "") == 0) {
            resultat = i;
        }

        i++;
    }

    return resultat;
}

int rejoindre_channel(int numclient, int numchan) {
    // cf les spécifs pour plus d'info
    int resultat = -1;

    if (numchan == -1) { // On est déjà dans le général
        return resultat;
    }

    if (chercher_client(clients[numclient].pseudo, nb_clients_max, channels[numchan].occupants) != -1) {
        printf("Nous avons affaire à un petit clown :)\n");
        return -4;
    }

    int placelibre = chercher_place_cli(nb_clients_max, channels[numchan].occupants);
    if (placelibre == nb_clients_max) { // Le channel est plein (cf chercher_place)
        resultat = -2;
    } else {
        if (sem_wait(&sem_tab_channels) == -1) perror("Erreur blocage sémaphore");
        channels[numchan].occupants[placelibre] = clients[numclient];
        sem_post(&sem_tab_channels);

        resultat = placelibre;
    }

    return resultat;
}

int bienvenue(int numclient, int numchan) {
    int result = 0;

    // On les stock pour éviter d'y accéder plusieurs fois
    char * pseudo = clients[numclient].pseudo;
    char * nomchan;
    char * descchan;

    if (numchan == -1) {
        nomchan = "Général";
        descchan = "";
    } else {
        nomchan = channels[numchan].nom;
        descchan = channels[numchan].description;
    }

    // On va envoyer "[[nomchannel]] Bienvenue à [pseudo] dans le channel!\n" à tous les membres du channel
    // OU                           "---------------------sur le serveur!\n" mais on prend la plus grand taille
    char * message1 = (char *) malloc((1 + strlen(nomchan) + 14 + strlen(pseudo) + 18)*sizeof(char));
    if (numchan == -1) {
        strcpy(message1, "Bienvenue à ");
        strcat(message1, pseudo);
        strcat(message1, " sur le serveur!\n");
    } else {
        strcpy(message1, "[");
        strcat(message1, nomchan);
        strcat(message1, "] Bienvenue à ");
        strcat(message1, pseudo);
        strcat(message1, " dans le channel!\n");
    }

    char * message2;
    if (numchan == -1) {
        message2 = "Bienvenue sur le serveur !\n";
    } else {
        // On va envoyer "Vous arrivez dans le channel [nom du channel]:\n[description]\n" au client
        message2 = (char *) malloc((29 + strlen(nomchan) + 2 + strlen(descchan) + 1)*sizeof(char));
        strcpy(message2, "Vous arrivez dans le channel ");
        strcat(message2, nomchan);
        strcat(message2, ":\n");
        strcat(message2, descchan);
        strcat(message2, "\n");
    }

    client * tab;

    if (numchan == -1) {
        tab = clients;
    } else {
        tab = channels[numchan].occupants;
    }

    int taille = nb_clients_max;
    if (numchan == -1) {
        taille *= nb_channels_max;
    }

    for (int i = 0; i < taille; i++) {
        if (tab[i].socket != 0) {
            if (strcmp(tab[i].pseudo, clients[numclient].pseudo) == 0) {
                if (envoi_message(tab[i].socket, message2) == -1) {
                    result = -1;
                }
            } else {
                if (envoi_message(tab[i].socket, message1) == -1) {
                    result = -1;
                }
            }
        }
    }

    free(message1);
    if (numchan != -1) {
        free(message2);
    }

    return result;
}

int aurevoir(int numclient, int numchan) {
    int result = 0;

    // On les stock pour éviter d'y accéder plusieurs fois
    char * pseudo = clients[numclient].pseudo;
    char * nomchan = channels[numchan].nom;

    // On va envoyer "[[nomchannel]] [pseudo] a quitté le channel." à tous les membres du channel
    char * message1 = (char *) malloc((1 + strlen(nomchan) + 2 + strlen(pseudo) + 22)*sizeof(char));
    strcpy(message1, "[");
    strcat(message1, nomchan);
    strcat(message1, "] ");
    strcat(message1, pseudo);
    strcat(message1, " a quitté le channel.\n");

    // On va envoyer "Vous quittez le channel [nomchannel]." au client
    char * message2 = (char *) malloc((24 + strlen(nomchan) + 2)*sizeof(char));
    strcpy(message2, "Vous quittez le channel ");
    strcat(message2, nomchan);
    strcat(message2, ".\n");

    for (int i = 0; i < nb_clients_max; i++) {
        if (channels[numchan].occupants[i].socket != 0) {
            if (channels[numchan].occupants[i].socket == clients[numclient].socket) {
                if (envoi_message(channels[numchan].occupants[i].socket, message2) == -1) {
                    result--;
                }
            } else {
                if (envoi_message(channels[numchan].occupants[i].socket, message1) == -1) {
                    result--;
                }
            }
        }
    }

    free(message1);
    free(message2);

    return result;
}

void deconnexion(int numclient, int numchan, int posclient) {
    char * pseudo = clients[numclient].pseudo; // On le stock, on va en avoir besoin plusieurs fois

    printf("Fin du thread du client %d concernant %s\n", numclient, pseudo);

    // Client vide, utilisé pour retirer le client qui se déconnecte de la liste
    client init;
    init.socket = 0;
    init.pseudo = "";
    init.IP = "";

    // On le retire du channel dans lequel il était
    if (numchan != -1) {
        if (sem_wait(&sem_tab_channels) == -1) perror("Erreur blocage sémaphore");
        channels[numchan].occupants[posclient] = init;
        sem_post(&sem_tab_clients);
    } // On ne fait rien s'il était dans le général
    
    // On le retire des clients
    if (sem_wait(&sem_tab_clients) == -1) perror("Erreur blocage sémaphore");
    clients[numclient] = init;
    sem_post(&sem_tab_clients);
    sem_post(&sem_clients_max);

    // On prévient tout le monde qu'il est parti
    // Dans le channel [Général] par <Serveur>: "[pseudo] s'est déconnecté.\n"
    char * message = (char *) malloc((strlen(pseudo) + strlen(" s'est déconnecté.\n")) * sizeof(char));
    strcpy(message, pseudo);
    strcat(message, " s'est déconnecté.\n");

    for(int i = 0; i < nb_clients_max*nb_channels_max; i++){
        // On n'envoie pas de message au client qui envoie, ni aux client qui n'existent pas
        if (i != numclient && clients[i].socket != 0) { // Les clients ont tous des sockets différents (ou vide)
            envoi_direct(i, message, "Serveur", -1);
        }
    }
    
    free(message);
    
    pthread_exit(0);
}

void envoi_membres(int numclient, int numchan) {
    char * nomchan;
    if (numchan == -1) { // On est dans le général
        nomchan = "Général";
    } else {
        nomchan = channels[numchan].nom;
    }

    // On va commencer par envoyer "[Serveur] Voici la liste des membres du channel [nomchannel]:\n"
    char * message = (char *) malloc((48 + strlen(nomchan) + 2)*sizeof(char));
    strcpy(message, "[Serveur] Voici la liste des membres du channel ");
    strcat(message, nomchan);
    strcat(message, ":\n");

    envoi_message(clients[numclient].socket, message);

    char * cli;

    if (numchan == -1) { // On est dans le général
        for (int i = 0; i < nb_clients_max*nb_channels_max; i++) {
            if (clients[i].socket != 0) {
                char * pseudo = clients[i].pseudo;

                if (strcmp(pseudo, clients[numclient].pseudo) == 0) {
                    // On envoie "\t- [pseudo] (Vous)\n"
                    cli = (char *) malloc((3 + strlen(pseudo) + 8)*sizeof(char));
                    strcpy(cli, "\t- ");
                    strcat(cli, pseudo);
                    strcat(cli, " (Vous)\n");
                } else {
                    // On envoie "\t- [pseudo]\n"
                    cli = (char *) malloc((3 + strlen(pseudo) + 1)*sizeof(char));
                    strcpy(cli, "\t- ");
                    strcat(cli, pseudo);
                    strcat(cli, "\n");
                }

                envoi_message(clients[numclient].socket, cli);

                free(cli);
            }
        }
    } else {
        for (int i = 0; i < nb_clients_max; i++) {
            if (channels[numchan].occupants[i].socket != 0) {
                char * pseudo = channels[numchan].occupants[i].pseudo;

                if (strcmp(pseudo, clients[numclient].pseudo) == 0) {
                    // On envoie "\t- [pseudo] (Vous)\n"
                    cli = (char *) malloc((3 + strlen(pseudo) + 8)*sizeof(char));
                    strcpy(cli, "\t- ");
                    strcat(cli, pseudo);
                    strcat(cli, " (Vous)\n");
                } else {
                    // On envoie "\t- [pseudo]\n"
                    cli = (char *) malloc((3 + strlen(pseudo) + 1)*sizeof(char));
                    strcpy(cli, "\t- ");
                    strcat(cli, pseudo);
                    strcat(cli, "\n");
                }

                envoi_message(clients[numclient].socket, cli);

                free(cli);
            }
        }
    }

    free(message);
}

void envoi_channels(int numclient) {
    /*
    Format du message:
    Liste des channels:             (20 caractères)
        - [[nomchan]]: [descchan]   (4 caractères + taille du nom + 3 caractères + taille de la description + 1 caractère)
        - [[nomchan]]: [descchan]   (ces lignes seront présentes nb_channels_max fois)
        ...
    */

    int taillemsg = 20;

    for (int i = 0; i < nb_channels_max; i++) {
        char * nomchan = channels[i].nom;
        if (strcmp(nomchan, "") != 0) {
            taillemsg += (4 + strlen(nomchan) + 3 + strlen(channels[i].description) + 1);
        }
    }

    char * msginfo = (char *) malloc(taillemsg*sizeof(char));
    strcpy(msginfo, "Liste des channels:\n");

    for (int i = 0; i < nb_channels_max; i++) {
        char * nomchan = channels[i].nom;
        if (strcmp(nomchan, "") != 0) {
            strcat(msginfo, "\t- [");
            strcat(msginfo, nomchan);
            strcat(msginfo, "]: ");
            strcat(msginfo, channels[i].description);
            strcat(msginfo, "\n");
        }
    }

    if (strlen(msginfo) > 20) { // On envoie les channels
        envoi_direct(numclient, msginfo, "Serveur", -10);
    } else { // Sinon c'est qu'il n'y en a pas
        envoi_direct(numclient, "Il n'y a aucun channel\n", "Serveur", -10);
    }

    free(msginfo);
}

void sauvegarde_channels() {
    FILE * f = fopen("Public/channels", "w");
    
    if (f == NULL) {
        perror("Fichier channels indisponible");
        fclose(f);
        return;
    }

    for(int i = 0; i < nb_channels_max; i++) {
        if(strcmp(channels[i].nom, "") != 0) {
            fprintf(f, "%s ", channels[i].nom);
            fprintf(f, "%s\n", channels[i].description);
        }
    }

    fclose(f);
    printf("Fin de la sauvegarde des channels\n");
}

void restaurer_channels() {
    // Au cas où le fichier existe pas
    if (access("Public/channels", F_OK) != 0) {
        printf("Pas de fichier channel, rien à restaurer.\n");
        return;
    }
    
    FILE * f = fopen("Public/channels", "rb");

    if (f == NULL) {
        perror("Fichier channels indisponible");
        fclose(f);
        return;
    }

    char * buffer;
    fseek (f, 0, SEEK_END);
    long length = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer = malloc (length);
    if (buffer) {
        fread (buffer, 1, length, f);
    } else {
        perror("Problème dans la lecture du fichier.");
        return;
    }

    char * token;
    char * reste = buffer;
    int i = 0;

    token = strtok_r(reste, "\n", &reste);

    if (sem_wait(&sem_tab_channels) == -1) perror("Erreur blocage sémaphore");

    while (token && i < nb_channels_max) {
        char * ligne = (char *) malloc(strlen(token)*sizeof(char));
        strcpy(ligne, token);

        ligne = strtok(ligne, " ");

        if (ligne == NULL) {
            // On n'a rien sur cette ligne, on ignore
            token = strtok_r(reste, "\n", &reste);
            continue;
        }

        channels[i].nom = (char *) malloc(strlen(ligne)*sizeof(char));
        strcpy(channels[i].nom, ligne);

        ligne = strtok(NULL, " ");

        // La description, c'est la ligne sans le nom du channel
        char * desc = (char *) malloc((strlen(token)-strlen(channels[i].nom))*sizeof(char));
        strcpy(desc, "");

        while(ligne) { // Si on n'a pas de description sur cette ligne, on n'en met pas
            strcat(desc, ligne);
            ligne = strtok(NULL, " ");
            if (ligne) { // On rajoute un espace que s'il reste des choses à rajouter après
                strcat(desc, " ");
            }
        }

        channels[i].description = desc;

        token = strtok_r(reste, "\n", &reste);

        i++;
    }

    sem_post(&sem_tab_channels);

    printf("Les channels:\n");
    for(int i = 0; i < nb_channels_max; i++) {
        printf("\t- [%s]: %s\n", channels[i].nom, channels[i].description);
    }

    free(buffer);

    fclose(f);
    printf("Fin de la restauration des channels\n");
}

char * lire_manuel() {
    char * buffer = 0;
    long length;
    FILE * f = fopen("Public/manuel", "rb");

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
        buffer = (char *) malloc((strlen("Manuel actuellement indisponible.\n")+1)*sizeof(char));
        strcpy(buffer,"Manuel actuellement indisponible.\n");
        return buffer;
    }
}

void fermeture_serveur() {
    for(int i = 0; i < nb_clients_max; i++){
        if (clients[i].socket != 0) {
            envoi_direct(i, "Fermeture du serveur\n", "Serveur", -10);
        }
    }

    printf("Fin du programme\n");
    exit(0);
}

commande gestion_commande(char * slashmsg, int numclient, int numchan, int posclient) {
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
            free(msg);
            free(msgmodif);
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
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                return result;
            }

            result.user = malloc(strlen(token)*sizeof(char));
            strcpy(result.user,token);

            token = strtok(NULL, " "); // On regarde la suite, ici: le message

            if (token == NULL) {
                perror("Vous devez mettre un message après l'utilisateur !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                free(result.user);
                return result;
            }

            // La longueur du message à envoyer, c'est la longueur de la commande sans le slash (msg) sans la commande (result.nom_cmd) et sans l'utilisateur (result.user) (-2 avec les 2 espaces)
            char * mp = (char *) malloc((strlen(msg)-strlen(result.nom_cmd)-strlen(result.user)-2)*sizeof(char));
            strcpy(mp,"");

            while (token != NULL) {
                strcat(mp, token);
                token = strtok(NULL, " ");
                if (token) { // On ajoute l'espace que s'il y a une suite
                    strcat(mp, " ");
                }
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
        else if (strcmp(cmd,"ef") == 0 || strcmp(cmd,"envoifichier") == 0) {
            result.nom_cmd = (char *) malloc(3*sizeof(char));
            strcpy(result.nom_cmd, "ef");

            token = strtok(NULL, " "); // On regarde la suite, ici: le nom de fichier

            if (token == NULL) { // Ne devrait pas arriver, mais on sait jamais
                perror("Vous devez mettre le nom de fichier après /ef !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                return result;
            }
            
            result.nomf = (char *) malloc(strlen(token)*sizeof(char));
            strcpy(result.nomf, token);

            token = strtok(NULL, " "); // On regarde la suite, ici : la taille du fichier

            if (token == NULL) { // Ne devrait pas arriver, mais on sait jamais
                perror("Vous devez mettre la taille du fichier après son nom !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                free(result.nomf);
                return result;
            }

            result.taillef = atoi(token);

            token = strtok(NULL, " "); // On regarde la suite, ici: le pseudo du destinataire

            if (token == NULL) { // Ne devrait pas arriver, mais on sait jamais
                perror("Vous devez mettre le pseudo du destinataire après la taille !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                free(result.nomf);
                return result;
            }

            result.user = (char *) malloc(strlen(token)*sizeof(char));
            strcpy(result.user, token);
        }
        else if (strcmp(cmd,"rejoindre") == 0) {
            result.nom_cmd = (char *) malloc(10*sizeof(char));
            strcpy(result.nom_cmd, "rejoindre");

            token = strtok(NULL, " "); // On regarde la suite, ici: le channel à rejoindre

            // On utilise les mêmes variables que ef
            if (token == NULL) {
                perror("Vous devez mettre le nom du channel après /rejoindre !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                return result;
            } else { // On donne le nom du channel
                // On doit évidemment enlever le \n
                token = strtok(token, "\n");

                if (token == NULL) { // S'il n'y a pas de suite, c'est la situation d'avant
                    perror("Vous devez mettre le nom du channel après /rejoindre !");
                    result.id_op = -1;
                    free(msg);
                    free(msgmodif);
                    free(result.nom_cmd);
                    return result;
                } else {
                    result.nomf = (char *) malloc(strlen(token)*sizeof(char));
                    strcpy(result.nomf, token);
                }
            }
        }
        else if (strcmp(cmd,"quitter") == 0) {
            result.nom_cmd = (char *) malloc(8*sizeof(char));
            strcpy(result.nom_cmd, "quitter");

            token = strtok(NULL, " "); // On regarde la suite, ici: le channel à rejoindre

            // On utilise les mêmes variables que ef
            if (token == NULL) { 
                perror("Vous devez mettre le nom du channel après /quitter !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                return result;
            } else { // On donne le nom du channel
                // On doit évidemment enlever le \n
                token = strtok(token, "\n");

                if (token == NULL) { // S'il n'y a pas de suite, c'est la situation d'avant 
                    perror("Vous devez mettre le nom du channel après /quitter !");
                    result.id_op = -1;
                    free(msg);
                    free(msgmodif);
                    free(result.nom_cmd);
                    return result;
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

            // On utilise les mêmes variables que ef
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
        else if (strcmp(cmd,"creerchannel") == 0 || strcmp(cmd,"cc") == 0) {
            result.nom_cmd = (char *) malloc(3*sizeof(char));
            strcpy(result.nom_cmd, "cc");

            token = strtok(NULL, " "); // On regarde la suite, ici: le nom du channel

            if (token == NULL) {
                perror("Vous devez mettre le channel après /cc !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                return result;
            }

            // On triche un peu pour pas surcharger la struct
            result.nomf = malloc(strlen(token)*sizeof(char));
            strcpy(result.nomf,token);

            token = strtok(NULL, "\0"); // On regarde la suite, ici: la description

            if (token == NULL) {
                perror("Vous devez mettre une description après le nom du channel !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                free(result.nomf);
                return result;
            }

            // La longueur de la description à utiliser, c'est la longueur de la commande sans le slash (msg) sans la commande (result.nom_cmd) et sans le nom du channel (result.nomf) (-2 avec les 2 espaces)
            char * desc = (char *) malloc((strlen(msg)-strlen(result.nom_cmd)-strlen(result.nomf)-2)*sizeof(char));
            
            strcpy(desc, token);

            while (desc[strlen(desc)-1] != '\n') {
                char * part1 = (char *) malloc((strlen(desc)+1)*sizeof(char));
                strcpy(part1, desc);

                char * part2 = reception_message(numclient, numchan, posclient);

                free(desc);

                desc = (char *) malloc((strlen(part1)+strlen(part2)+1)*sizeof(char));
                strcpy(desc, part1);
                strcat(desc, part2);

                free(part1);
                free(part2);
            }

            // On enlève le \n de la fin

            desc = strtok(desc, "\n");

            if (desc == NULL) { // Même situation qu'avant
                perror("Vous devez mettre une description après le nom du channel !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                free(result.nomf);
                return result;
            }

            result.message = desc;
        }
        else if (strcmp(cmd,"modifierchannel") == 0 || strcmp(cmd,"mc") == 0) {
            result.nom_cmd = (char *) malloc(3*sizeof(char));
            strcpy(result.nom_cmd, "mc");

            token = strtok(NULL, " "); // On regarde la suite, ici: le nom du channel

            if (token == NULL) {
                perror("Vous devez mettre le channel après /mc !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                return result;
            }

            // On triche un peu pour pas surcharger la struct
            result.nomf = malloc(strlen(token)*sizeof(char));
            strcpy(result.nomf,token);

            token = strtok(NULL, "\0"); // On regarde la suite, ici: la description

            if (token == NULL) {
                perror("Vous devez mettre une description après le nom du channel !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                free(result.nomf);
                return result;
            }

            // La longueur de la description à utiliser, c'est la longueur de la commande sans le slash (msg) sans la commande (result.nom_cmd) et sans le nom du channel (result.nomf) (-2 avec les 2 espaces)
            char * desc = (char *) malloc((strlen(msg)-strlen(result.nom_cmd)-strlen(result.nomf)-2)*sizeof(char));
            
            strcpy(desc, token);

            while (desc[strlen(desc)-1] != '\n') {
                char * part1 = (char *) malloc((strlen(desc)+1)*sizeof(char));
                strcpy(part1, desc);

                char * part2 = reception_message(numclient, numchan, posclient);

                free(desc);

                desc = (char *) malloc((strlen(part1)+strlen(part2)+1)*sizeof(char));
                strcpy(desc, part1);
                strcat(desc, part2);

                free(part1);
                free(part2);
            }

            result.message = desc;
        }
        else if (strcmp(cmd,"supprimerchannel") == 0 || strcmp(cmd,"sc") == 0) {
            result.nom_cmd = (char *) malloc(3*sizeof(char));
            strcpy(result.nom_cmd, "sc");

            token = strtok(NULL, " "); // On regarde la suite, ici: le nom du channel

            if (token == NULL) {
                perror("Vous devez mettre le channel après /sc !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                return result;
            }

            // On doit enlever le \n
            token = strtok(token, "\n");

            if (token == NULL) { // S'il n'y a pas de suite, c'est la situation d'avant
                perror("Vous devez mettre le channel après /sc !");
                result.id_op = -1;
                free(msg);
                free(msgmodif);
                free(result.nom_cmd);
                return result;
            } else {
                // On triche un peu pour pas surcharger la struct
                result.nomf = (char *) malloc(strlen(token)*sizeof(char));
                strcpy(result.nomf, token);
            }
        }
        else if (strcmp(cmd,"membres") == 0 || strcmp(cmd,"membres\n") == 0) {
            result.nom_cmd = (char *) malloc(7*sizeof(char));
            strcpy(result.nom_cmd, "membres");

            token = strtok(NULL, " "); // On regarde la suite, ici: le channel dont on veut la liste

            // On utilise les mêmes variables que ef
            if (token == NULL) { // S'il n'y en a pas on enverra la liste pour le channel actuel
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
        else if (strcmp(cmd, "turnoff") == 0 || strcmp(cmd,"turnoff\n") == 0) {
            result.nom_cmd = (char *) malloc(strlen("turnoff")*sizeof(char));
            strcpy(result.nom_cmd, "turnoff");
        }
        else {
            perror("Commande non reconnue");
            result.id_op = -1;
            return result;
        }

        free(msg);
        free(msgmodif);
    }
    return result;
}

char * censure(char * message) {
    FILE * censure;
    char * nomcheminCen = (char *) malloc((strlen("Public/Censure")+1)*sizeof(char));
    strcpy(nomcheminCen, "Public/Censure");
    censure = fopen(nomcheminCen, "r");
    if (censure == NULL) {
        perror("Erreur durant la lecture du fichier de mots censurés, non trouvé");
        return message;
    }
    char * m = (char *) malloc(20*sizeof(char));
    while(fgets(m, 20, censure) != NULL) {
        char * token = (char *) malloc((strlen(m))*sizeof(char));
        char * replace = (char *) malloc((strlen(m))*sizeof(char));
        for(int i = 1; i < strlen(m); i++) {
            strcat(replace, "*");
        }
        token = strtok(m, "\n");
        str_replace(message, m, replace);
    }
    fclose(censure);
    free(nomcheminCen);
    free(m);
    return message;
}

void str_replace(char *target, const char *needle, const char *replacement) {
    char buffer[1024] = { 0 };
    char *insert_point = &buffer[0];
    const char *tmp = target;
    size_t needle_len = strlen(needle);
    size_t repl_len = strlen(replacement);

    while (1) {
        const char *p = strstr(tmp, needle);

        // walked past last occurrence of needle; copy remaining part
        if (p == NULL) {
            strcpy(insert_point, tmp);
            break;
        }

        // copy part before needle
        memcpy(insert_point, tmp, p - tmp);
        insert_point += p - tmp;

        // copy replacement string
        memcpy(insert_point, replacement, repl_len);
        insert_point += repl_len;

        // adjust pointers, move on
        tmp = p + needle_len;
    }

    // write altered string back to target
    strcpy(target, buffer);
}

void signal_handle(int sig){
    for(int i = 0; i < nb_clients_max*nb_channels_max; i++){
        if (clients[i].socket != 0) {
            envoi_direct(i, "Fermeture du serveur\n", "Serveur", -10);
        }
    }

    sauvegarde_channels();

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