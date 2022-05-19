#define taille_pseudo 30 // Taille du pseudo en comptant le \0
#define SIZE 1024 // Taille du buffe

/**
 * @brief Structure qui sera passée au thread de réception client
 */
typedef struct argsrec {
  int socket;
  int fin; // Pointer sur booléen, vrai ssi le programme doit se terminer
}argsrec;

typedef struct argsfichier {
  int socket; // Socket de l'envoi de message
  char * IP;
  char * destinataire; // Pseudo du destinataire
  char * nomf; // Nom du fichier 
  long taillef; // Taille du fichier
  int action; // 0 pour récupérer un fichier, 1 pour l'envoyer
}argsfichier;

/**
 * @brief Le thread de reception de messages du client
 * 
 * @param argpointer Pointeur vers une structure (on utilisera argsrec)
 */
void * thread_reception(void * argpointer);

/**
 * @brief Lecture des messages dans le terminal puis envoi
 * 
 * @param dS Socket du serveur pour l'envoi
 * @return false si le message saisie est "fin\n", true sinon (utilisé pour l'arrêt)
 */
int lecture_message(int dS);

/**
 * @brief Envoi d'un message par le socket (envoi de la taille puis du message)
 * 
 * @param socket Socket du destinataire
 * @param msg Message à envoyer
 * @return 0 si tout se passe bien, -1 si erreur d'envoi
 */
int envoi_message(int socket, char * msg);

/**
 * @brief Affiche le contenu du répertoire Public/ dans le terminal
 */
void print_repertoire();

void * thread_fichier(void * argpointer);

/**
 * @brief Envoie le contenu du fichier Public/nomfichier au socket
 * 
 * @param socket Socket du destinataire
 * @param nomfichier Nom du fichier à envoyer (situé dans Public/)
 */
void envoi_fichier(int socket, char * nomfichier, char * destinataire);

/**
 * @brief Récupère un fichier depuis le socket
 * 
 * @param socket Socket de l'envoyeur
 * @param nomfichier Nom du fichier à recevoir
 * @param taillefichier Taille du fichier à recevoir
 */
void recup_fichier(int socket, char * nomfichier, long taillefichier);

/**
 * @brief Fonction de signal CTRL C client
 */
void signal_handleCli(int sig);

/**
 * @brief Fonction de calcul de la taille d'un entier
 * 
 * @param nb Entier quelconque
 * @return Le nombre de chiffres qui compose l'entier nb
 */
int tailleint(int nb);