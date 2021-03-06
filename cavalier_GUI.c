
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <inttypes.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <gtk/gtk.h>


#define MAXDATASIZE 256
#define NOIR 0
#define BLANC 1

struct coup {
    uint16_t colonne;
    uint16_t ligne;
};


/******************************/
/***** VARIABLES GLOBALES *****/
/******************************/

// tableau associe au damier , couleurs possibles 0 : pour noir, 1 : pour blanc, 3 : pour pion
int damier[8][8];

// 0 : pour noir, 1 : pour blanc
int couleur_j = 0;
int couleur_a = 0;

// emplacement du joueur
struct coup localisation;

// numero port passé lors de l'appel
int port;
// Info sur adversaire
char *addr_j2, *port_j2;

// Id du thread fils gerant connexion socket
pthread_t thr_id;
// descripteurs de socket
int sockfd, newsockfd=-1;
// taille adresse
int addr_size;
// structure pour stocker adresse adversaire
struct sockaddr *their_addr;

// ensembles de socket pour toutes les sockets actives avec select
fd_set master, read_fds, write_fds;

// utilise pour select
int fdmax;

struct addrinfo hints, *servinfo, *p;



/****************************************************************/
/***** VARIABLES GLOBALES ASSOCIEES A L'INTERFACE GRAPHIQUE *****/
/****************************************************************/

GtkBuilder  *  p_builder   = NULL;
GError      *  p_err       = NULL;



/************************************/
/***** ENTETE FONCTIONS DE BASE *****/
/************************************/

/* Fonction permettant afficher image pion dans case du damier (indiqué par sa colonne et sa ligne) */
void affiche_pion(int col, int lig);

/* Fonction permettant afficher image cavalier noir dans case du damier (indiqué par sa colonne et sa ligne) */
void affiche_cav_noir(int col, int lig);

/* Fonction permettant afficher image cavalier blanc dans case du damier (indiqué par sa colonne et sa ligne) */
void affiche_cav_blanc(int col, int lig);

/* Fonction transformant coordonnees du damier graphique en indexes pour matrice du damier */
void coord_to_indexes(const gchar *coord, int *col, int *lig);

/* Fonction appelee lors du clique sur une case du damier */
static void coup_joueur(GtkWidget *p_case);

/* Fonction retournant texte du champs adresse du serveur de l'interface graphique */
char *lecture_addr_serveur(void);

/* Fonction retournant texte du champs port du serveur de l'interface graphique */
char *lecture_port_serveur(void);

/* Fonction retournant texte du champs login de l'interface graphique */
char *lecture_login(void);

/* Fonction retournant texte du champs adresse du cadre Joueurs de l'interface graphique */
char *lecture_addr_adversaire(void);

/* Fonction retournant texte du champs port du cadre Joueurs de l'interface graphique */
char *lecture_port_adversaire(void);

/* Fonction affichant boite de dialogue si partie gagnee */
void affiche_fenetre_gagne(void);

/* Fonction affichant boite de dialogue si partie perdue */
void affiche_fenetre_perdu(void);

/* Fonction appelee lors du clique du bouton Se connecter */
static void clique_connect_serveur(GtkWidget *b);

/* Fonction desactivant bouton demarrer partie */
void disable_button_start(void);

/* Fonction appelee lors du clique du bouton Demarrer partie */
static void clique_connect_adversaire(GtkWidget *b);

/* Fonction desactivant les cases du damier */
void gele_damier(void);

/* Fonction activant les cases du damier */
void degele_damier(void);

/* Fonction permettant d'initialiser le plateau de jeu */
void init_interface_jeu(void);

/* Fonction reinitialisant la liste des joueurs sur l'interface graphique */
void reset_liste_joueurs(void);

/* Fonction permettant d'ajouter un joueur dans la liste des joueurs sur l'interface graphique */
void affich_joueur(char *login, char *adresse, char *port);

/* Fonction permettant d'effectuer des opérations récurentes sur les descripteurs */
void set_and_clear_fds(int fd_signal);

/* Fonction permettant d'initialiser une socket et de la bind */
void init_bind_socket(int *fd_sock, const char *port);

/* Fonction permettant d'initialiser une socket et de tenter une connexion dessus */
void init_connect_socket(int *fd_sock);



/*************************************/
/***** ENTETE FONCTIONS AJOUTEES *****/
/*************************************/

/* Fonction permettant de connaitre toutes les cases où le joueur qui joue peut se déplacer sur le damier */
int** get_case_jouable(int col, int lig);

/* Fonction permettant d'ajouter les cases où le joueur qui joue peut se déplacer sur le damier */
void add_case_jouable(int** case_j);

/* Fonction permettant afficher image case jouable dans case du damier (indiqué par sa colonne et sa ligne) */
void affiche_case_jouable(int col, int lig);

/* Fonction permettant de supprimer les anciens cases jouable par le joueur sur le damier */
void remove_old_case_jouable();

/* Fonction permettant de connaitre les cordonnées du joueur donnée en paramètre */
struct coup get_coord_joueur(int couleur);

/* Fonction permettant afficher image case vide dans case du damier (indiqué par sa colonne et sa ligne) */
void affiche_case_vide(int col, int lig);

/* Fonction permettant de vérifier si le joueur peut se déplacer ou non */
int verifie_joueur_bloque(int couleur);

/* Fonction permettant de vérifier si c'est le cavalier blanc qui a gagné */
void verifie_cav_blanc_gagne();

/* Fonction permettant de vérifier si c'est le cavalier noir qui a gagné */
void verifie_cav_noir_gagne();

/* Fonction permettant de vérifier si il y a égalité */
void verifie_egalite();

/* Fonction affichant boite de dialogue si partie egalité */
void affiche_fenetre_egalite(void);

/* Fonction appelee lorsque l'on recoit le coup de l'adversaire */
static void coup_adversaire(int col, int lig);



/*****************************/
/***** FONCTIONS DE BASE *****/
/*****************************/

/* Fonction transforme coordonnees du damier graphique en indexes pour matrice du damier */
void coord_to_indexes(const gchar *coord, int *col, int *lig) {
  char *c;
  
  c=malloc(3*sizeof(char));
  
  c=strncpy(c, coord, 1);
  c[1]='\0';
  
  if(strcmp(c, "A")==0)
  {
    *col=0;
  }
  if(strcmp(c, "B")==0)
  {
    *col=1;
  }
  if(strcmp(c, "C")==0)
  {
    *col=2;
  }
  if(strcmp(c, "D")==0)
  {
    *col=3;
  }
  if(strcmp(c, "E")==0)
  {
    *col=4;
  }
  if(strcmp(c, "F")==0)
  {
    *col=5;
  }
  if(strcmp(c, "G")==0)
  {
    *col=6;
  }
  if(strcmp(c, "H")==0)
  {
    *col=7;
  }
    
  *lig=atoi(coord+1)-1;
}

/* Fonction transforme coordonnees du damier graphique en indexes pour matrice du damier */
void indexes_to_coord(int col, int lig, char *coord) {
  char c;

  if(col==0)
  {
    c='A';
  }
  if(col==1)
  {
    c='B';
  }
  if(col==2)
  {
    c='C';
  }
  if(col==3)
  {
    c='D';
  }
  if(col==4)
  {
    c='E';
  }
  if(col==5)
  {
    c='F';
  }
  if(col==6)
  {
    c='G';
  }
  if(col==7)
  {
    c='H';
  }
    
  sprintf(coord, "%c%d\0", c, lig+1);
}

/* Fonction permettant afficher image pion dans case du damier (indiqué par sa colonne et sa ligne) */
void affiche_pion(int col, int lig) {
    char * coord;
    
    coord=malloc(3*sizeof(char));
    indexes_to_coord(col, lig, coord);
    
    gtk_image_set_from_file(GTK_IMAGE(gtk_builder_get_object(p_builder, coord)), "UI_Glade/case_pion.png");
}

/* Fonction permettant afficher image cavalier noir dans case du damier (indiqué par sa colonne et sa ligne) */
void affiche_cav_noir(int col, int lig) {
    char * coord;
    
    coord=malloc(3*sizeof(char));
    indexes_to_coord(col, lig, coord);
    
    gtk_image_set_from_file(GTK_IMAGE(gtk_builder_get_object(p_builder, coord)), "UI_Glade/case_cav_noir.png");
}

/* Fonction permettant afficher image cavalier blanc dans case du damier (indiqué par sa colonne et sa ligne) */
void affiche_cav_blanc(int col, int lig) {
    char * coord;
    
    coord=malloc(3*sizeof(char));
    indexes_to_coord(col, lig, coord);
    
    gtk_image_set_from_file(GTK_IMAGE(gtk_builder_get_object(p_builder, coord)), "UI_Glade/case_cav_blanc.png");
}

/* Fonction appelee lors du clique sur une case du damier */
static void coup_joueur(GtkWidget *p_case) {
  int col, lig, type_msg, nb_piece, score;
  char buf[MAXDATASIZE];
  
  // Traduction coordonnees damier en indexes matrice damier
  coord_to_indexes(gtk_buildable_get_name(GTK_BUILDABLE(gtk_bin_get_child(GTK_BIN(p_case)))), &col, &lig);

  // Degele le damier
  degele_damier();
  
  // Vérifier le status de la partie (gagné, perdu, égalité)
  verifie_egalite();

  if (couleur_j == 0) {
    verifie_cav_noir_gagne();
  }
  else {
    verifie_cav_blanc_gagne();
  }

  int** case_j = get_case_jouable(localisation.colonne, localisation.ligne);

  // Vérifie que la case cliquée correspond à une case jouable
  if (case_j[col][lig] == 1) {

    // Supprimer les indications des case qui pouvait être joué
    remove_old_case_jouable(case_j);

    if (couleur_j == BLANC) {
      // Afficher cavalier blanc dans la case cliquée
      affiche_cav_blanc(col, lig);
      damier[col][lig] = 1;
    }
    else {
      // Afficher cavalier noir dans la case cliquée
      affiche_cav_noir(col, lig);
      damier[col][lig] = NOIR;
    }

    // Afficher pion à l'ancien emplacement du joueur
    affiche_pion(localisation.colonne, localisation.ligne);
    damier[localisation.colonne][localisation.ligne] = 3;

    // Gele le damier
    //gele_damier();

    // Envoyer à l'adversaire les anciennes et les nouvelles coordonnées du joueur actuel
    bzero(buf, MAXDATASIZE);
    snprintf(buf, MAXDATASIZE, "%u %u %u %u", htons((uint16_t)localisation.colonne), htons((uint16_t)localisation.ligne), htons((uint16_t)col), htons((uint16_t)lig));

    if (send(newsockfd, &buf, strlen(buf), 0) == -1)
    {
        perror("ERREUR: Envoi des données");
        close(newsockfd);
    }

    // Changer la localisation actuelle du joueur
    localisation.colonne=col;
    localisation.ligne=lig;

    // Afficher les cases jouables par le joueur
    add_case_jouable(get_case_jouable(localisation.colonne, localisation.ligne));

  } 
}

/* Fonction retournant texte du champs adresse du serveur de l'interface graphique */
char *lecture_addr_serveur(void) {
  GtkWidget *entry_addr_srv;
  
  entry_addr_srv = (GtkWidget *) gtk_builder_get_object(p_builder, "entry_adr");
  
  return (char *)gtk_entry_get_text(GTK_ENTRY(entry_addr_srv));
}

/* Fonction retournant texte du champs port du serveur de l'interface graphique */
char *lecture_port_serveur(void) {
  GtkWidget *entry_port_srv;
  
  entry_port_srv = (GtkWidget *) gtk_builder_get_object(p_builder, "entry_port");
  
  return (char *)gtk_entry_get_text(GTK_ENTRY(entry_port_srv));
}

/* Fonction retournant texte du champs login de l'interface graphique */
char *lecture_login(void) {
  GtkWidget *entry_login;
  
  entry_login = (GtkWidget *) gtk_builder_get_object(p_builder, "entry_login");
  
  return (char *)gtk_entry_get_text(GTK_ENTRY(entry_login));
}

/* Fonction retournant texte du champs adresse du cadre Joueurs de l'interface graphique */
char *lecture_addr_adversaire(void) {
  GtkWidget *entry_addr_j2;
  
  entry_addr_j2 = (GtkWidget *) gtk_builder_get_object(p_builder, "entry_addr_j2");
  
  return (char *)gtk_entry_get_text(GTK_ENTRY(entry_addr_j2));
}

/* Fonction retournant texte du champs port du cadre Joueurs de l'interface graphique */
char *lecture_port_adversaire(void) {
  GtkWidget *entry_port_j2;
  
  entry_port_j2 = (GtkWidget *) gtk_builder_get_object(p_builder, "entry_port_j2");
  
  return (char *)gtk_entry_get_text(GTK_ENTRY(entry_port_j2));
}

/* Fonction affichant boite de dialogue si partie gagnee */
void affiche_fenetre_gagne(void) {
  GtkWidget *dialog;
    
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  
  dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object(p_builder, "window1")), flags, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "Fin de la partie.\n\n Vous avez gagné!!!");
  gtk_dialog_run(GTK_DIALOG (dialog));
  
  gtk_widget_destroy(dialog);
}

/* Fonction affichant boite de dialogue si partie perdue */
void affiche_fenetre_perdu(void) {
  GtkWidget *dialog;
    
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  
  dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object(p_builder, "window1")), flags, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "Fin de la partie.\n\n Vous avez perdu!");
  gtk_dialog_run(GTK_DIALOG (dialog));
  
  gtk_widget_destroy(dialog);
}

/* Fonction appelee lors du clique du bouton Se connecter */
static void clique_connect_serveur(GtkWidget *b) {
  /***** TO DO *****/
}

/* Fonction desactivant bouton demarrer partie */
void disable_button_start(void) {
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object (p_builder, "button_start"), FALSE);
}

/* Fonction traitement signal bouton Demarrer partie */
static void clique_connect_adversaire(GtkWidget *b) {
  if(newsockfd==-1)
  {
    // Deactivation bouton demarrer partie
    gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object (p_builder, "button_start"), FALSE);
    
    // Recuperation adresse et port adversaire au format chaines caracteres
    addr_j2=lecture_addr_adversaire();
    port_j2=lecture_port_adversaire();
    
    printf("[Port joueur : %d] Adresse j2 lue : %s\n",port, addr_j2);
    printf("[Port joueur : %d] Port j2 lu : %s\n", port, port_j2);

    pthread_kill(thr_id, SIGUSR1);
    
  }
}

/* Fonction desactivant les cases du damier */
void gele_damier(void) {
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA1"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB1"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC1"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD1"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE1"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF1"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG1"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH1"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA2"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB2"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC2"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD2"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE2"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF2"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG2"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH2"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA3"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB3"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC3"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD3"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE3"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF3"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG3"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH3"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA4"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB4"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC4"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD4"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE4"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF4"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG4"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH4"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA5"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB5"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC5"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD5"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE5"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF5"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG5"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH5"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA6"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB6"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC6"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD6"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE6"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF6"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG6"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH6"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA7"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB7"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC7"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD7"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE7"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF7"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG7"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH7"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA8"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB8"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC8"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD8"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE8"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF8"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG8"), FALSE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH8"), FALSE);
}

/* Fonction activant les cases du damier */
void degele_damier(void) {
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA1"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB1"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC1"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD1"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE1"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF1"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG1"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH1"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA2"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB2"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC2"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD2"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE2"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF2"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG2"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH2"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA3"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB3"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC3"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD3"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE3"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF3"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG3"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH3"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA4"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB4"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC4"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD4"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE4"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF4"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG4"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH4"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA5"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB5"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC5"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD5"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE5"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF5"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG5"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH5"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA6"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB6"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC6"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD6"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE6"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF6"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG6"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH6"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA7"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB7"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC7"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD7"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE7"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF7"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG7"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH7"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA8"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB8"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC8"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD8"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE8"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF8"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG8"), TRUE);
  gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH8"), TRUE);
}

/* Fonction permettant d'initialiser le plateau de jeu */
void init_interface_jeu(void) {
  // Initilisation du damier (A1=cavalier_noir, H8=cavalier_blanc)
  affiche_cav_blanc(7,7);
  damier[7][7]=1;
  affiche_cav_noir(0,0);
  damier[0][0]=0;  

  // Initialisation du plateau de chaque joueur
  if (couleur_j == 0) {
      localisation.colonne=0;
      localisation.ligne=0;
      degele_damier();
      add_case_jouable(get_case_jouable(localisation.colonne, localisation.ligne));
  }
  else {
      localisation.colonne=7;
      localisation.ligne=7;
      gele_damier();
  }  
}

/* Fonction reinitialisant la liste des joueurs sur l'interface graphique */
void reset_liste_joueurs(void) {
  GtkTextIter start, end;
  
  gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(p_builder, "textview_joueurs")))), &start);
  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(p_builder, "textview_joueurs")))), &end);
  
  gtk_text_buffer_delete(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(p_builder, "textview_joueurs")))), &start, &end);
}

/* Fonction permettant d'ajouter un joueur dans la liste des joueurs sur l'interface graphique */
void affich_joueur(char *login, char *adresse, char *port) {
  const gchar *joueur;
  
  joueur=g_strconcat(login, " - ", adresse, " : ", port, "\n", NULL);
  
  gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(p_builder, "textview_joueurs")))), joueur, strlen(joueur));
}

/* Fonction permettant d'effectuer des opérations récurentes sur les descripteurs */
void set_and_clear_fds(int fd_signal) {
    FD_SET(newsockfd, &master);
    fdmax = newsockfd > fdmax ? newsockfd : fdmax;

    FD_CLR(sockfd, &master);
    close(sockfd);

    FD_CLR(fd_signal, &master);
    close(fd_signal);
}

/* Fonction permettant d'initialiser une socket et de la bind */
void init_bind_socket(int *fd_sock, const char *port) {
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int connected = 0;
    while (connected != 1)
    {
        printf("Trying port [%s]\n", port);
        int status = getaddrinfo(NULL, port, &hints, &servinfo);
        if (status != 0)
        {
            fprintf(stderr, "getaddrinfo : %s\n", gai_strerror(status));
            exit(1);
        }

        for (p = servinfo; p != NULL; p->ai_next)
        {
            if ((*fd_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            {
                perror("Error while creating socket...");
                exit(0);
            }

            if (bind(*fd_sock, p->ai_addr, p->ai_addrlen) == -1)
            {
                close(*fd_sock);
                perror("Error while binding socket...");
                
                int val;

                val = atoi(port);
                val++;
                sprintf((char *restrict)port, "%d", val);

                break;
            }
            connected = 1;
            break;
        }
        if (p == NULL)
        {
            fprintf(stderr, "error while binding, exit\n");
            exit(1);
        }
        freeaddrinfo(servinfo);
    }
}

/* Fonction permettant d'initialiser une socket et de tenter une connexion dessus */
void init_connect_socket(int *fd_sock) {
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(addr_j2, port_j2, &hints, &servinfo);
    if (status != 0)
    {
        fprintf(stderr, "getaddrinfo : %s\n", gai_strerror(status));
    }

    for (p = servinfo; p != NULL; p->ai_next)
    {
        if ((*fd_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("Error while creating client socket...\n");
            break;
        }
        printf("socket: %d\n", *fd_sock);
        if (connect(*fd_sock, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(*fd_sock);
            perror("Error while binding client socket...");
            break;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "error while binding, exit\n");
        exit(1);
    }
    freeaddrinfo(servinfo);
}

/* Fonction exécutée par le thread gérant les communications à travers la socket */
static void * f_com_socket(void *p_arg) {
  int i, nbytes, old_col, old_lig, new_col, new_lig;

  sigset_t signal_mask;
  
  char buf[MAXDATASIZE], *tmp, *p_parse, *saveptr;
  int len, bytes_sent, t_msg_recu;

  int fd_signal;

  uint16_t type_msg, col_j2;
  uint16_t ucol, ulig;

  struct coup mouvement_joueur;

  saveptr = malloc(sizeof(char));

  /* Association descripteur au signal SIGUSR1 */
  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGUSR1);

  if (sigprocmask(SIG_BLOCK, &signal_mask, NULL) == -1) {
      printf("[Port joueur %d] Erreur sigprocmask\n", port);
      return 0;
  }

  fd_signal = signalfd(-1, &signal_mask, 0);

  if (fd_signal == -1) {
      printf("[Port joueur %d] Erreur signalfd\n", port);
      return 0;
  }

  /* Ajout descripteur du signal dans ensemble de descripteur utilisé avec fonction select */
  FD_SET(fd_signal, &master);

  if (fd_signal > fdmax) {
      fdmax = fd_signal;
  }
  
  while(1) {
    read_fds=master;	// copie des ensembles
    
    if(select(fdmax+1, &read_fds, &write_fds, NULL, NULL)==-1) {
      perror("Problème avec select");
      gtk_widget_set_sensitive((GtkWidget *)gtk_builder_get_object(p_builder, "button_start"), TRUE);
      exit(4);
    }
    
    printf("[Port joueur %d] Entree dans boucle for\n", port);
    for(i=0; i<=fdmax; i++) {
      printf("[Port joueur %d] newsockfd=%d, iteration %d boucle for\n", port, newsockfd, i);

      if(FD_ISSET(i, &read_fds)) {
        if (i == fd_signal) {
          /* Cas où on se connecte au joueur adverse */
          printf("Connexion avec l'adversaire\n");

          if (newsockfd == -1)
          {
            printf("test 1");
            init_connect_socket(&newsockfd);

            set_and_clear_fds(fd_signal);

            couleur_j = NOIR;
            couleur_a = BLANC;
            init_interface_jeu();
          }
        }

        // Acceptation connexion adversaire
        if(i==sockfd) { 
	  
          if (newsockfd == -1) {
              addr_size = sizeof(their_addr);
              if ((newsockfd = accept(sockfd, their_addr, (socklen_t *)&addr_size)) == -1)
              {
                  perror("Connexion refusée");
                  return NULL;
              }
              set_and_clear_fds(fd_signal);
              init_interface_jeu();
          }

          gtk_widget_set_sensitive((GtkWidget *)gtk_builder_get_object(p_builder, "button_start"), FALSE);

        }
        // Reception et traitement des messages du joueur adverse
        else { 
          printf("\ntest 7 => %d \n", i);
          if (i == newsockfd) {
            // vider buffer
            bzero(buf, MAXDATASIZE);
            nbytes = recv(newsockfd, buf, MAXDATASIZE, 0);

            printf("\n%s\n", buf);

            old_col = atoi(strtok_r(buf, " ", &saveptr));
            old_lig = atoi(strtok_r(NULL, " ", &saveptr));

            new_col = atoi(strtok_r(buf, " ", &saveptr));
            new_lig = atoi(strtok_r(NULL, " ", &saveptr));

            sscanf(new_col, "%hu", (unsigned short int *)&(mouvement_joueur.colonne));
            sscanf(new_lig, "%hu", (unsigned short int *)&(mouvement_joueur.ligne));

            mouvement_joueur.colonne = ntohs(mouvement_joueur.colonne);
            mouvement_joueur.ligne = ntohs(mouvement_joueur.ligne);

            coup_adversaire(mouvement_joueur.colonne, mouvement_joueur.ligne);
          }
        }
      }
    }
  }
  
  return NULL;
}



/******************************/
/***** FONCTIONS AJOUTEES *****/
/******************************/

/* Fonction permettant d'ajouter les cases où le joueur qui joue peut se déplacer sur le damier */
void add_case_jouable(int** case_j) {
  int i,j;

  for(i=0; i<8; i++)
  {
    for(j=0; j<8; j++)
    {
      if (case_j[i][j] == 1) {
        affiche_case_jouable(i,j);
      }
    }  
  }

}

/* Fonction permettant de connaitre toutes les cases où le joueur qui joue peut se déplacer sur le damier */
int** get_case_jouable(int col, int lig) {
  int i,j;

  // malloc case_j variable array 2d
  int* values = calloc(8*8, sizeof(int));
  int** case_j = malloc(8*sizeof(int*));
  for (int i=0; i<8; ++i) {
      case_j[i] = values + i*8;
  }
  
  // Initialise val to 0
  for(i=0; i<8; i++)
  {
    for(j=0; j<8; j++)
    {
      case_j[i][j]=0; 
    }
  }

  // Vérifier déplacement par le haut
  if (lig >= 2) {
    // Vérifier déplacement en haut à droite
    if (col <= 6 ) {
      if (damier[col+1][lig-2] == -1) {
        case_j[col+1][lig-2]=1;
      }
    }

    // Vérifier déplacement en haut à gauche
    if (col >= 1) {
      if (damier[col-1][lig-2] == -1) {
        case_j[col-1][lig-2]=1;
      }
    }
  }
  
  // Vérifier déplacement par le bas
  if (lig <= 5) {
    // Vérifier déplacement en bas à droite
    if (col <= 6) {
      if (damier[col+1][lig+2] == -1) {
        case_j[col+1][lig+2]=1;
      }
    }

    // Vérifier déplacement en bas à gauche
    if (col >= 1) {
      if (damier[col-1][lig+2] == -1) {
        case_j[col-1][lig+2]=1;
      }
    }
  }
  
  // Vérifier déplacement par la gauche
  if (col >= 2) {
    // Vérifier déplacement à gauche en haut
    if (lig >= 1 ) {
      if (damier[col-2][lig-1] == -1) {
        case_j[col-2][lig-1]=1;
      }
    }

    // Vérifier déplacement à gauche en bas
    if (lig <= 6) {
      if (damier[col-2][lig+1] == -1) {
        case_j[col-2][lig+1]=1;
      }
    }
  }

  // Vérifier déplacement par la droite
  if (col <= 5) {
    // Vérifier déplacement à droite en haut
    if (lig >= 1 ) {
      if (damier[col+2][lig-1] == -1) {
        case_j[col+2][lig-1]=1;
      }
    }

    // Vérifier déplacement à droite en bas
    if (lig <= 6) {
      if (damier[col+2][lig+1] == -1) {
        case_j[col+2][lig+1]=1;
      }
    }
  }

  return case_j;
}

/* Fonction permettant afficher image case jouable dans case du damier (indiqué par sa colonne et sa ligne) */
void affiche_case_jouable(int col, int lig) {
    char * coord;
    
    coord=malloc(3*sizeof(char));
    indexes_to_coord(col, lig, coord);
    
    gtk_image_set_from_file(GTK_IMAGE(gtk_builder_get_object(p_builder, coord)), "UI_Glade/case_jouable.png");
}

/* Fonction permettant de supprimer les anciens cases jouable par le joueur sur le damier */
void remove_old_case_jouable(int** case_j) {
  int i,j;

  for(i=0; i<8; i++)
  {
    for(j=0; j<8; j++)
    {
      if (case_j[i][j] == 1) {
        affiche_case_vide(i,j);
      }
    }  
  }

}

/* Fonction permettant de connaitre les cordonnées du joueur donnée en paramètre */
struct coup get_coord_joueur(int couleur) {
  int i,j;
  struct coup coord;

  for(i=0; i<8; i++)
  {
    for(j=0; j<8; j++)
    {
      if (damier[i][j] == couleur) {
        coord.colonne=i;
        coord.ligne=j;
      }
    }  
  }

  return coord;
}

/* Fonction permettant de vérifier si le joueur peut se déplacer ou non */
int verifie_joueur_bloque(int couleur) {
  int i, j;
  int bloque=1;

  int ** coup_jouable = get_case_jouable(get_coord_joueur(couleur).colonne, get_coord_joueur(couleur).ligne);
  for(i=0; i<8; i++)
  {
    for(j=0; j<8; j++)
    {
      if (coup_jouable[i][j] == 1) {
        bloque=0;
      }
    }  
  }

  return bloque;
}

/* Fonction permettant afficher image case vide dans case du damier (indiqué par sa colonne et sa ligne) */
void affiche_case_vide(int col, int lig) {
    char * coord;
    
    coord=malloc(3*sizeof(char));
    indexes_to_coord(col, lig, coord);
    
    gtk_image_set_from_file(GTK_IMAGE(gtk_builder_get_object(p_builder, coord)), "UI_Glade/case_def.png");
}

/* Fonction permettant de vérifier si c'est le cavalier blanc qui a gagné */
void verifie_cav_blanc_gagne() {
  // Vérifie si le cavalier blanc peut prendre la place du cavalier noir 
  struct coup blanc = get_coord_joueur(BLANC);

  int col = blanc.colonne;
  int lig = blanc.ligne;

  // Vérifier déplacement par le haut
  if (lig >= 2) {
    // Vérifier déplacement en haut à droite
    if (col <= 6 ) {
      if (damier[col+1][lig-2] == 0) {
        affiche_fenetre_gagne();
      }
    }

    // Vérifier déplacement en haut à gauche
    if (col >= 1) {
      if (damier[col-1][lig-2] == 0) {
        affiche_fenetre_gagne();
      }
    }
  }
  
  // Vérifier déplacement par le bas
  if (lig <= 5) {
    // Vérifier déplacement en bas à droite
    if (col <= 6) {
      if (damier[col+1][lig+2] == 0) {
        affiche_fenetre_gagne();
      }
    }

    // Vérifier déplacement en bas à gauche
    if (col >= 1) {
      if (damier[col-1][lig+2] == 0) {
        affiche_fenetre_gagne();
      }
    }
  }
  
  // Vérifier déplacement par la gauche
  if (col >= 2) {
    // Vérifier déplacement à gauche en haut
    if (lig >= 1 ) {
      if (damier[col-2][lig-1] == 0) {
        affiche_fenetre_gagne();
      }
    }

    // Vérifier déplacement à gauche en bas
    if (lig <= 6) {
      if (damier[col-2][lig+1] == 0) {
        affiche_fenetre_gagne();
      }
    }
  }

  // Vérifier déplacement par la droite
  if (col <= 5) {
    // Vérifier déplacement à droite en haut
    if (lig >= 1 ) {
      if (damier[col+2][lig-1] == 0) {
        affiche_fenetre_gagne();
      }
    }

    // Vérifier déplacement à droite en bas
    if (lig <= 6) {
      if (damier[col+2][lig+1] == 0) {
        affiche_fenetre_gagne();
      }
    }
  }

  // Vérifier si cavalier noir bloqué
  if (verifie_joueur_bloque(NOIR)) {
    affiche_fenetre_gagne();
  }
}

/* Fonction permettant de vérifier si c'est le cavalier noir qui a gagné */
void verifie_cav_noir_gagne() {
  // Vérifier si le cavalier noir est protégé derrière une barrière de pion rouge


  // Vérifier si le cavalier blanc bloqué
  if (verifie_joueur_bloque(BLANC)) {
    affiche_fenetre_gagne();
  }
}

/* Fonction permettant de vérifier si il y a égalité */
void verifie_egalite() {
  // Vérifier si le cavalier noir et le cavalier blanc sont bloqués
  if (verifie_joueur_bloque(NOIR) && verifie_joueur_bloque(BLANC)) {
    affiche_fenetre_egalite();
  }

}

/* Fonction affichant boite de dialogue si partie egalité */
void affiche_fenetre_egalite(void) {
  GtkWidget *dialog;
    
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  
  dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object(p_builder, "window1")), flags, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "Fin de la partie.\n\n égalité!");
  gtk_dialog_run(GTK_DIALOG (dialog));
  
  gtk_widget_destroy(dialog);
}

/* Fonction appelee lorsque l'on recoit le coup de l'adversaire */
static void coup_adversaire(int col, int lig) {
    affiche_cav_noir(col, lig);
    printf("\ncol=%d  lig=%d\n");

    degele_damier();
}



/****************/
/***** MAIN *****/
/****************/

int main(int argc, char ** argv) {
   int i, j, ret;
   int thread;

   if(argc!=2) {
    printf("\nPrototype : ./cavalier_GUI num_port\n\n");
    exit(1);
   }
   
   /* Initialisation de GTK+ */
   gtk_init (& argc, & argv);
   
   /* Creation d'un nouveau GtkBuilder */
   p_builder = gtk_builder_new();
 
   if (p_builder != NULL) {
      /* Chargement du XML dans p_builder */
      gtk_builder_add_from_file (p_builder, "UI_Glade/Cavalier.glade", & p_err);
 
      if (p_err == NULL) {
         /* Recuparation d'un pointeur sur la fenetre. */
         GtkWidget * p_win = (GtkWidget *) gtk_builder_get_object (p_builder, "window1");

 
         /* Gestion evenement clic pour chacune des cases du damier */
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
         
         /* Gestion clic boutons interface */
         g_signal_connect(gtk_builder_get_object(p_builder, "button_connect"), "clicked", G_CALLBACK(clique_connect_serveur), NULL);
         g_signal_connect(gtk_builder_get_object(p_builder, "button_start"), "clicked", G_CALLBACK(clique_connect_adversaire), NULL);
         
         /* Gestion clic bouton fermeture fenetre */
         g_signal_connect_swapped(G_OBJECT(p_win), "destroy", G_CALLBACK(gtk_main_quit), NULL);
         
         
         /* Recuperation numero port donne en parametre */
         port=atoi(argv[1]);
          
         /* Initialisation du damier de jeu */
         for(i=0; i<8; i++) {
           for(j=0; j<8; j++) {
             damier[i][j]=-1; 
           }  
         }

        // Initialisation de l'interface de jeu
        init_interface_jeu();

        // Initialisation socket et autres objets, et création thread pour communications avec joueur adverse
        init_bind_socket(&sockfd, argv[1]);

        // Attend une connexion sur la socket définit dans la variable sockfd
        listen(sockfd, 1);

        // Initialise les descripteurs de fichier à 0
        FD_ZERO(&master);
        FD_ZERO(&read_fds);

        // Définit la variable master avec la socket définit dans sockfd
        FD_SET(sockfd, &master);
        
        read_fds = master;
        fdmax = sockfd;

        // creation thread pour communication avec joueur adverse
        thread = pthread_create(&thr_id, NULL, f_com_socket, NULL);
        if (thread != 0) {
            fprintf(stderr, "ERREUR : Creation du thread %s\n", strerror(thread));
            exit(1);
        }
        
         gtk_widget_show_all(p_win);
         gtk_main();
      }
      else {
         /* Affichage du message d'erreur de GTK+ */
         g_error ("%s", p_err->message);
         g_error_free (p_err);
      }
   }

   return EXIT_SUCCESS;
}
