void envoi(int dS) {
    printf("Entrez un message\n");
    char * m = (char *) malloc(30*sizeof(char));
    fgets( m, 30*sizeof(char), stdin ); 
    size_t len= strlen(m)+1;
    int snd2 = send(dS, &len, sizeof(len),0);
    if (snd2 == -1){perror("Erreur envoi taille message");}
    int snd = send(dS, m, len , 0) ;
    if (snd == -1){perror("Erreur envoi message");}
    printf("Message Envoyé \n");
}

void reception(int dS)
    size_t len = 0;
    ssize_t rcv_len = recv(dSC, &len, sizeof(len), 0) ;
    if (rcv_len == -1){perror("Erreur réception taille message");}
    printf("%d\n",(int)len);
    char * msg = (char *) malloc((len)*sizeof(char));
    ssize_t rcv = recv(dSC, msg, len, 0) ;
    if (rcv == -1){perror("Erreur réception message");}
    printf("Message reçu : %s\n", msg);
}