#define taille_pseudo 30 // taille du pseudo en comptant le \0

/**
 * @brief Structure qui sera passée au thread de réception client
 */
typedef struct argsrec {
  int socket;
  int fin; // Pointer sur booléen, vrai ssi le programme doit se terminer
}argsrec;

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

void envoi_repertoire(int socket);

void envoi_fichier(int socket, char * nomfichier, int taillefichier);

/**
 * @brief Fonction de signal CTRL C client
 */
void signal_handleCli(int sig);