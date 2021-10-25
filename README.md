# Projet CNAM Socket

## Lancement

Commande de compilation :
gcc -Wall -o cavalier_GUI cavalier_GUI.c $(pkg-config --cflags --libs gtk+-3.0)

Lancement projet avec <num_port> le port TCP d'écoute :
./cavalier_GUI <num_port>


## Objectif

Réalisation d’un jeu de chasse au cavalier à 2 joueurs en utilisant l’API Socket en langage C pour faire communiquer le programme de deux joueurs exécutés localement sur la même machine.


## Règles du jeu

Le jeu se joue sur un échiquier de 8x8. Les deux cavaliers sont disposés initialement sur deux cases d’angles diagonalement opposées. Le cavalier blanc aura le rôle de poursuivant et le cavalier noir le rôle de poursuivi. 

L’objectif pour le cavalier blanc est d’atteindre la position du cavalier noir. Les joueurs jouent à tour de  rôle, et c’est le cavalier noir qui commence. Les cavaliers se déplacent comme au jeu d’échec vers une case libre.

Après un déplacement, un pion rouge est déposé sur la case précédemment occupée par le cavalier lors du dernier déplacement. Ainsi au fur et à mesure de l’évolution de la partie, le nombre de cases libres diminue et le nombre de déplacements possibles se réduit. 

La partie se termine dans l’une de ces trois situations :
1. Le cavalier blanc gagne si :
  - c’est à son tour de jouer et il peut atteindre la case du cavalier noir en un coup,
  - ou le cavalier noir ne peut plus se déplacer.
2. Le cavalier noir gagne si :
   - le cavalier noir est à l’abri derrière une barrière de pions rouge que le cavalier blanc ne peut franchir
   - ou le cavalier blanc ne peut plus se déplacer.
3. Il y a égalité si les deux cavaliers sont dans l’impossibilité simultanée de se déplacer.

## Architecture logicielle

L’approche la plus simple pour réaliser ce projet est de choisir une architecture Peer-to-Peer pour les échanges entre les deux joueurs. A savoir, le programme de chaque joueur aura à la fois une partie cliente et une partie serveur pour les échanges de messages liés aux coups des joueurs.

Une seconde partie de ce projet consistera à introduire un serveur de jeu stockant les informations sur les joueurs (adresse et numéro de port d’un autre joueur), auquel chaque joueur pourra se connecter pour démarrer une nouvelle partie. 

Pour cette partie du projet, vous opterez pour une architecture Client/Serveur. Les communications devront être effectuées en utilisant l'interface Sockets. Nous utiliserons le protocole de transport TCP afin de s’assurer de la bonne transmission et de l’ordre des messages. Pour implémenter le projet, vous devrez utiliser le langage C et l'API socket.h. 