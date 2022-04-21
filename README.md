# raipon_FDP
*Il faut répondre petit chenapan ^^*

Raipon FDP est une messagerie codée en C faite dans le cadre du cursus Informatique et Gestion de Polytech Montpellier.

Codé par Louis VAN DER PUTTE, Anaëlle Danton et Alma Sorrentino

## Compilation de l'application

Pour compiler le fichier client : gcc -o client client.c fonctions.c -lpthread

Pour compiler le fichier serveur : gcc -o serveur serveur.c fonctions.c -lpthread

## Lancement de l'application

Pour lancer le fichier client : ./client [Pseudonyme] [IPServeur] [port]

Pour lancer le fichier serveur : ./serveur [port]