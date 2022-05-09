#define nb_client_max 3
#define taille_pseudo 30 // taille du pseudo en comptant le \0

typedef struct client {
  int socket;
  char * pseudo;
}client;

typedef struct traitement_params {
  int numclient;
}traitement_params;

typedef struct commande {
  // L'id de la commande: -1 = commande / non reconnue; 0 = pas de commande; 1 = "/";
  int id_op;
  char * nom_cmd; // Le nom de la commande qui suit l'opérateur
  char * message;
  char * user;
}commande;

// Envoi d'un message par le socket (envoi de la taille puis du message)
int envoi_message(int socket, char * msg);

// Thread de traitement serveur, un thead par client
void * traitement_serveur(void * paramspointer);

// Envoi d'un message à un utilisateur par son indice dans le tableau clients
// Envoi de la taille, puis du message
// Forme du message : "<pseudo> : message"
int envoi_direct(int numreceveur, char * msg, char * envoyeur);

// Reception d'un message du client numclient dans le tableau clients
// Reception de la taille, puis du message
char * reception_message(int numclient);

// Cherche un client dans le tableau clients
// Renvoie: -1 si non trouvé, l'index dans le tableau clients sinon.
int chercher_client(char * pseudo);

// Cherche une place dans le tableau clients
// Renvoie: l'index de la première place trouvée, la taille du tableau si aucune place n'est trouvée
int chercher_place();

// Renvoie le manuel lu dans Public/manuel sous forme de String
char * lire_manuel();

// Renvoie un objet commande, cf la struct pour plus d'info de retour
commande gestion_commande(char * msg);

// Gestion du signal CTRL C serveur
void signal_handle(int sig);