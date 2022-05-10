#define nb_client_max 3 // Nombre de client maximum pouvant être accueillis sur la messagerie en simultané
#define taille_pseudo 30 // taille du pseudo en comptant le \0

/**
 * @brief Structure pour un client (socket et pseudo)
 */
typedef struct client {
  int socket;
  char * pseudo;
}client;

/**
 * @brief Structure pour le thread de relai de message pour chaque client
 */
typedef struct traitement_params {
  int numclient;
}traitement_params;

/**
 * @brief Structure pour une commande: id, nom, message, destinataire (dans le cadre du mp)
 */
typedef struct commande {
  // L'id de la commande: -1 = commande / non reconnue; 0 = pas de commande; 1 = "/";
  int id_op;
  char * nom_cmd; // Le nom de la commande qui suit l'opérateur
  char * message;
  char * user;
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
 * Forme du message : "<pseudo> : message"
 * 
 * @param numreceveur Indice du receveur dans le tableau clients
 * @param msg Message à envoyer
 * @param envoyeur Pseudo de l'envoyeur
 * @return 0 si tout se passe bien, -1 si erreur d'envoi
 */
int envoi_direct(int numreceveur, char * msg, char * envoyeur);

/**
 * @brief Reception d'un message du client numclient dans le tableau clients
 * Reception de la taille, puis du message
 * 
 * @param numclient Indice du client dans le tableau
 * @return Le message reçu
 */
char * reception_message(int numclient);

/**
 * @brief Cherche un client dans le tableau clients par son pseudo
 * 
 * @param pseudo Pseudo de la personne recherchée
 * @return -1 si non trouvé, l'index dans le tableau clients sinon.
 */
int chercher_client(char * pseudo);

/**
 * @brief Cherche une place dans le tableau clients
 * 
 * @return Si aucune place n'est trouvée la taille du tableau, sinon l'index de la première place trouvée
 */
int chercher_place();

/**
 * @brief Consulte le fichier Public/manuel (source: https://www.codegrepper.com/code-examples/c/c+read+file+into+string)
 * 
 * @return Le manuel lu en String
 */
char * lire_manuel();

/**
 * @brief Envoie le contenu du répertoire Public (source: https://askcodez.com/implementation-de-la-commande-ls-al-en-c.html)
 * 
 * @param numclient Indice du client dans le tableau
 */
void envoi_repertoire(int numclient);

/**
 * @brief Analyse le message pour y reconnaître une commande
 * 
 * @param msg Message à analyser
 * @return Un objet commande, cf la struct pour plus d'info de retour
 */
commande gestion_commande(char * msg);

/**
 * @brief Gestion du signal CTRL C serveur
 */
void signal_handle(int sig);