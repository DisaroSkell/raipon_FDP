#define nb_clients_max 3 // Nombre de client maximum pouvant être accueillis sur un channel en simultané
#define nb_channels_max 2 // Nombre de channels maximum pouvant être actifs en même temps
#define taille_pseudo 30 // Taille du pseudo en comptant le \0
#define SIZE 1024 // Taille du buffer

/**
 * @brief Structure pour un client (socket et pseudo)
 */
typedef struct client {
  int socket;
  char * pseudo;
  char * IP;
  int port;
}client;

/**
 * @brief Structure pour un channel (nom, description et occupants)
 */
typedef struct channel {
  char * nom;
  char * description;
  client occupants[nb_clients_max];
}channel;

/**
 * @brief Structure pour le thread de relai de message pour chaque client
 */
typedef struct traitement_params {
  int numclient;
}traitement_params;

/**
 * @brief Structure pour une commande
 */
typedef struct commande {
  // L'id de la commande: -1 = commande / non reconnue; 0 = pas de commande; 1 = "/";
  int id_op;

  char * nom_cmd; // Le nom de la commande qui suit l'opérateur

  // Pour la commande mp
  char * message; // Message à envoyer
  char * user; // Destinataire

  // Pour les commandes ef et rf
  char * nomf; // Nom du fichier
  int taillef; // Taille du fichier
}commande;

/**
 * @brief Envoi d'un message par le socket (envoi de la taille puis du message)
 * 
 * @param socket Socket du destinataire
 * @param msg Message à envoyer
 * @return 0 si tout se passe bien, -1 si erreur d'envoi
 */
int envoi_message(int socket, char * msg);

/**
 * @brief Thread de traitement serveur, un thead par client
 * 
 * @param paramspointer Pointeur sur la structure de paramètres (traitement_params)
 */
void * traitement_serveur(void * paramspointer);

/**
 * @brief Envoi d'un message à un utilisateur par son indice dans le tableau clients
 * Envoi de la taille, puis du message
 * Forme du message : "[channel] <pseudo> : message"
 * 
 * @param numreceveur Indice du receveur dans le tableau clients
 * @param msg Message à envoyer
 * @param envoyeur Pseudo de l'envoyeur
 * @param numchan Indice du channel où écrit l'envoyeur dans le tableau channels (-1 = Général; -2 = Direct; autre c'est rien)
 * @return 0 si tout se passe bien, -1 si erreur d'envoi
 */
int envoi_direct(int numreceveur, char * msg, char * envoyeur, int numchan);

/**
 * @brief Reception d'un message du client numclient dans le tableau clients
 * Reception de la taille, puis du message
 * Utilise la fonction deconnexion()
 * 
 * @param numclient Indice du client dans le tableau
 * @param numchan Index du channel dans le tableau channels
 * @param posclient Index du client dans le tableau des occupant du channel numchan
 * @return Le message reçu
 */
char * reception_message(int numclient, int numchan, int posclient);

/**
 * @brief Cherche un client dans le tableau tabcli par son pseudo. Renvoie une erreur si deux clients ont le même pseudo.
 * 
 * @param pseudo Pseudo de la personne recherchée
 * @param taille Taille du tableau qui suit
 * @param tabcli Tableau de clients, chaque client est supposé soit "vide" soit unique
 * @return -1 si non trouvé, l'index dans le tableau clients sinon.
 */
int chercher_client(char * pseudo, int taille, client tabcli[taille]);

/**
 * @brief Cherche une place dans le tableau clients
 * 
 * @param taille Taille du tableau qui suit
 * @param tabcli Tableau de clients, chaque client est supposé soit "vide" soit unique
 * @return Si aucune place n'est trouvée la taille du tableau, sinon l'index de la première place trouvée
 */
int chercher_place_cli(int taille, client tabcli[taille]);

/**
 * @brief Cherche un channel dans le tableau channels par nom. Renvoie une erreur si deux channels ont le même nom.
 * 
 * @param nom Nom du channel recherché
 * @return -1 si non trouvé, l'index dans le tableau channels sinon
 */
int chercher_channel(char * nom);

/**
 * @brief Cherche une place dans le tableau channels
 * 
 * @param taille Taille du tableau qui suit
 * @param tabchan Tableau de channels, chaque channel est supposé soit "vide" soit unique
 * @return Si aucune place n'est trouvée la taille du tableau, sinon l'index de la première place trouvée
 */
int chercher_place_chan(int taille, channel tabcchan[taille]);


/**
 * @brief Déplace un utilisateur d'un channel à un autre. Modifie le tableau channels.
 * 
 * @param numclient Index du client dans le tableau clients
 * @param numchan1 Index du channel de départ dans le tableau channels
 * @param numchan2 Index du channel d'arrivée dans le tableau channels
 * @return -1 si on va vers le général, -2 si numchan2 est plein, sinon renvoie la position du client dans le tableau des occupants du channel
 */
int changer_channel(int numclient, int numchan1, int numchan2);

/**
 * @brief Envoie un message de bienvenue dans le channel pour prévenir l'arrivée du client.
 * Envoie un message spécial de bienvenue au client avec la description du channel.
 * 
 * @param numclient Index du client dans le tableau clients
 * @param numchan Index du channel dans le tableau channels
 * @return -1 si problème d'envoi
 */
int bienvenue(int numclient, int numchan);

/**
 * @brief Envoie un message notifiant le départ du client dans le channel
 * 
 * @param numclient Index du client dans le tableau clients
 * @param numchan Index du channel dans le tableau channels
 * @return -1 si problème d'envoi
 */
int aurevoir(int numclient, int numchan);

/**
 * @brief Met fin au thread de gestion client pour déconnecter le client proprement
 * 
 * @param numclient Index du client dans le tableau clients
 * @param numchan Index du channel dans le tableau channels
 * @param posclient Index du client dans le tableau des occupant du channel numchan; -1 si c'est dans le Général
 */
void deconnexion(int numclient, int numchan, int posclient);

/**
 * @brief Envoie la liste des membres du channel au client
 * 
 * @param numclient Index du client dans le tableau clients
 * @param numchan Index du channel dans le tableau channels
 */
void envoi_membres(int numclient, int numchan);

/**
 * @brief Envoie tous les channels et leur description au client
 * 
 * @param numclient Indice du client dans le tableau
 */
void envoi_channels(int numclient);

/**
 * @brief Sauvegarde le nom et la description des channels dans Public/channels
 */
void sauvegarde_channels();

/**
 * @brief Restaure le nom et la description des channels depuis Public/channels 
 */
void restaurer_channels();

/**
 * @brief Consulte le fichier Public/manuel (source: https://www.codegrepper.com/code-examples/c/c+read+file+into+string)
 * 
 * @return Le manuel lu en String
 */
char * lire_manuel();

/**
 * @brief Envoie le contenu du répertoire Public (inspiré de: https://askcodez.com/implementation-de-la-commande-ls-al-en-c.html)
 * 
 * @param numclient Indice du client dans le tableau
 */
void envoi_repertoire(int numclient);

/**
 * @brief Récupère un fichier envoyé par le client
 * 
 * @param socket Socket du client
 * @param nomfichier Nom du fichier à recevoir
 * @param taillefichier Taille du fichier à recevoir
 */
void recup_fichier(int socket, char * nomfichier, long taillefichier);

/**
 * @brief Envoi d'un fichier à un client
 * 
 * @param numclient Indice du client dans le tableau
 * @param nomfichier Nom du fichier à recevoir
 */
void envoi_fichier(int numclient, char * nomfichier);

/**
 * @brief Analyse le message pour y reconnaître une commande (utilise reception message)
 * 
 * @param msg Message à analyser
 * @param numclient Indice du client dans le tableau
 * @param numchan Index du channel dans le tableau channels
 * @param posclient Index du client dans le tableau des occupant du channel numchan
 * @return Un objet commande, cf la struct pour plus d'info de retour
 */
commande gestion_commande(char * msg, int numclient, int numchan, int posclient);

/**
 * @brief Gestion du signal CTRL C serveur
 */
void signal_handle(int sig);

/**
 * @brief Fonction de calcul de la taille d'un entier
 * 
 * @param nb Entier quelconque
 * @return Le nombre de chiffres qui compose l'entier nb
 */
int tailleint(int nb);