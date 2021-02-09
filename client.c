/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/*                   E N T Ê T E S    S T A N D A R D S                     */
/* ------------------------------------------------------------------------ */

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <wait.h>
#include <ctype.h>
#include <termios.h>
//Ip SERVEUR Port du serveur
//Pseudo Client2 Port2 Ip2
//Soit CPseudo Port Ip
//Choix (ciseaux papier pierre)

//Une partie client et une partie serveur et une partie contact serveur client

/* ------------------------------------------------------------------------ */
/*              C O N S T A N T E S     S Y M B O L I Q U E S               */
/* ------------------------------------------------------------------------ */

#define PORT_SRV 27015
#define MAX_BUFF 512
#define ADDR_SRV "127.0.0.1"

/* ------------------------------------------------------------------------ */
/*              D É F I N I T I O N S   D E   T Y P E S                     */
/* ------------------------------------------------------------------------ */

typedef void *(*pf_t)(void *);

typedef struct
{
	int idClt;
	char addrClt[16];
	char coupAdverse[16];
	int portClt;
	char pseudo[16];
	int score;
} clt_adverse_t;

typedef struct
{
	int id;
	char pseudo[16];
	char addrClt[16];
	int portClt;
} clt_t;

typedef char message_t[MAX_BUFF];

/* ------------------------------------------------------------------------ */
/*                      M A C R O - F O N C T I O N S                       */
/* ------------------------------------------------------------------------ */

#define CHECK(status, msg)  \
	if (-1 == (status))     \
	{                       \
		perror(msg);        \
		exit(EXIT_FAILURE); \
	}

/* ------------------------------------------------------------------------ */
/*            P R O T O T Y P E S    D E    F O N C T I O N S               */
/* ------------------------------------------------------------------------ */
void requestServer(char requete[]);
void communicationServeurFonction();
void serveurClientFonction();
void dialogueAutreClientFonction();
void dialClt2Srv(int sad, const char *MSG);
void connectSrv(int sad);
int sessionClt(void);
void pthreadserveur();
int sessionSrv(char ip[], int port);
int acceptClt(int se, struct sockaddr_in *cltAdr);
void dialSrv2Clt(int sd, struct sockaddr_in *cltAdr);
static void signalHandler(int numSig);
void communicationClientFonction(char msg[]);
void connectClt(int sad);
void choixAdversaire(int nbClient, clt_t clients[], char choixJoueur[], char requete[]);


int port_srv = PORT_SRV;
char addr_srv[20] = ADDR_SRV;
sem_t ecritureInformations;
int fin;
int seserveur;
int sdserveur;
char ip[20];
char ippublic[20];
int port;
int monScore;
int rechercheAdversaire;

int supressionServeur;
char pseudo[30];
clt_adverse_t cltAdverse;
char monCoup[15];
int requeteATraiter;
char champs[31][20]; //Affiche 7 clients maximum de la liste de serveur
/* ------------------------------------------------------------------------ */

int main(int argc, char *argv[])
{

	struct sigaction newActionn;
	newActionn.sa_handler = signalHandler;
	sigemptyset(&newActionn.sa_mask);
	// newAction.sa_flags = SA_SIGINFO;
	sigaction(SIGINT, &newActionn, NULL);

	pthread_t communicationServeur;
	pthread_t serveurClient;
	pthread_t dialogueAutreClient;
	int monScore = 0;
	cltAdverse.score = 0;
	requeteATraiter = 0;
	fin = 0;
	rechercheAdversaire = 1;
	supressionServeur = 0;

	// Informations rentrées par l'utilisateur
	if (argc > 1)
	{
		char *port = argv[1];
		port_srv = atoi(port);
	}

	printf("Quel est l'adresse ip du serveur à contacter ? \n");
	scanf("%s", addr_srv); // On demande d'entrer l'adresse avec fgets

	printf("[Janken] Entrez une adresse ip pour l'écoute du serveur (privée) : ");
	scanf("%s", ip);
	printf("[Janken] Entrez votre adresse publique  : ");
	scanf("%s", ippublic);

	//Test plus rapide
	/*strcpy(addr_srv, ADDR_SRV);
	strcpy(ip, ADDR_SRV);
	strcpy(ippublic, ADDR_SRV);*/

	printf("[Janken] Entrez un port pour l'écoute du serveur : ");
	scanf("%d", &port);

	printf("[Janken] Entrez votre pseudo : ");
	scanf("%s", pseudo);

	//Initialisation

	CHECK(sem_init(&ecritureInformations, 0, 1), "PB init ecritureInformations");

	//Lancement des threads

	// Requête serveur
	CHECK(pthread_create(&communicationServeur, NULL, (pf_t)communicationServeurFonction, NULL), "pthread_create(communicationServeur)");

	// Partie serveur du client
	seserveur = sessionSrv(ip, port);
	CHECK(pthread_create(&serveurClient, NULL, (pf_t)serveurClientFonction, NULL), "pthread_create(serveurClient)");

	//Partie dialogue avec l'autre client
	CHECK(pthread_create(&dialogueAutreClient, NULL, (pf_t)dialogueAutreClientFonction, NULL), "pthread_create(dialogueAutreClient)");

	//Attente de terminaison des threads

	CHECK(pthread_join(communicationServeur, NULL), "pthread_join(communicationServeur)");
	CHECK(pthread_join(serveurClient, NULL), "pthread_join(serveurClient)");
	CHECK(pthread_join(dialogueAutreClient, NULL), "pthread_join(dialogueAutreClient)");

	printf("Au revoir\n");

	return 0;
}

/* ------------------------------------------------------------------------ */

/*

Il se connecte au serveur d'enregistrement, envoi unique de la requête
En donnant son pseudo son IP et son PORT

*/

void communicationServeurFonction()
{
	char requete[20];

	//Requête initiale pour enregistrement dans le serveur

	sprintf(requete, "client;%s;%s;%d", pseudo, ippublic, port);

	requestServer(requete);

	while (fin == 0)
	{
		if (supressionServeur == 1)
		{ //Suppression du client dans la table du serveur
			//sup;pseudo;ip;port
			supressionServeur = 0;
			sprintf(requete, "sup;%s;%s;%d", pseudo, ippublic, port);
			printf("on se supprime des clients du serveur\n");
			requestServer(requete);
		}
		
		sleep(3);
	}

	pthread_exit(EXIT_SUCCESS);
}

/* ------------------------------------------------------------------------ */

void serveurClientFonction()
{

	struct sockaddr_in cltAdrSrv;

	while (fin == 0)
	{
		// attente de connexion d'un client et crÃ©ation d'une socket de dialogue
		sdserveur = acceptClt(seserveur, &cltAdrSrv);

		// dialogue avec le client connectÃ©
		dialSrv2Clt(sdserveur, &cltAdrSrv);
	}
	
	pthread_exit(EXIT_SUCCESS);
}

/* ------------------------------------------------------------------------ */

void dialogueAutreClientFonction()
{ //dialogue avec l'autre client

	char requete[20];
	char choix[5];
	char choixJoueur[5];
	int nbClient;
	int k;
	clt_t clients[20];
	int j;

	while (fin == 0)
	{
		while (requeteATraiter == 0)
		{
			sleep(2);
		}
		requeteATraiter = 0;
	
		//réception de la requête

		if (champs[0][0] == 's' && champs[0][1] == 'e')
		{

			if (champs[1][0] == 'a' && champs[2][0] == '\0') 
			{												 //serveur;attente

				printf("Attente d'un autre joueur\n");
				rechercheAdversaire = 0;
			}
			else if (isdigit(champs[1][0])) 
			{//server;nbClient;id;client;ip;port;id
				nbClient = atoi(champs[1]);
				memset(clients, 0, sizeof(clients));
				j = 0;

				for (int i = 0; i < nbClient; i++)
				{
					k = 4 * i + 2; //indice du premier paramètre du nouveau client

					if (strcmp(champs[k + 2], ip) != 0 || atoi(champs[k + 3]) != port)
					{
						clients[j].id = j;
						strcpy(clients[j].pseudo, champs[k + 1]);
						strcpy(clients[j].addrClt, champs[k + 2]);
						clients[j].portClt = atoi(champs[k + 3]);

						j++;
					}
				}

				if (rechercheAdversaire)
				{

					choixAdversaire(nbClient, clients, choixJoueur, requete);
				}
			}
			
		}
		else if (champs[0][0] == 'g' && champs[0][1] == 'o')
		{ //go;pseudo;ip;port

			printf("\n\n");
			printf("Voulez vous jouer avec %s (O/n) ? \n", champs[1]);
			scanf("%s", choix);
			if (choix[0] == 'n')
			{
				printf("Joueur refusé\n");

				strcpy(cltAdverse.pseudo, champs[1]);
				strcpy(cltAdverse.addrClt, champs[2]);
				cltAdverse.portClt = atoi(champs[3]);

				sprintf(requete, "non");
				communicationClientFonction(requete);
			}
			else
			{
				

				strcpy(cltAdverse.pseudo, champs[1]);
				strcpy(cltAdverse.addrClt, champs[2]);
				cltAdverse.portClt = atoi(champs[3]);
				

				//envoi de requête au serveur pour suppression de la liste
				
				supressionServeur = 1;
				//envoi au client adverse jeux;pierre/feuille/ciseaux
				printf("[Janken] Quel coup jouez vous (pierre/feuille/ciseaux) ? : \n");
				scanf("%s", monCoup);
				sprintf(requete, "jeux;1;%s", monCoup);
				communicationClientFonction(requete);
			}
		}
		else if (champs[0][0] == 'n' && champs[0][1] == 'o' && champs[0][2] == 'n')
		{
			printf("L'invitation a été refusée par le joueur\n");

			choixAdversaire(nbClient, clients, choixJoueur, requete);
		}
		else if (champs[0][0] == 'j') 
		{//jeux;1/2;pierre/feuille/ciseaux 

			rechercheAdversaire = 0;

			strcpy(cltAdverse.coupAdverse, champs[2]);
			printf("L'adversaire a joué un coup\n");
			//cas 1, l'adversaire a joué son coup
			if (champs[1][0] == '1')
			{
				printf("[Janken] Quel coup jouez vous (pierre/feuille/ciseaux) ? : \n");
				scanf("%s", monCoup);

				//envoi du coup
				sprintf(requete, "jeux;2;%s", monCoup);
				communicationClientFonction(requete);
			}

			if ((monCoup[0] == 'c' && cltAdverse.coupAdverse[0] == 'p') || (monCoup[0] == 'p' && cltAdverse.coupAdverse[0] == 'f') || (monCoup[0] == 'f' && cltAdverse.coupAdverse[0] == 'c'))
			{ //Toutes les possibilitées perdantes
				printf("Perdu\n");
				cltAdverse.score++;
			}
			else if (monCoup[0] == 'c' && cltAdverse.coupAdverse[0] == 'f' || (monCoup[0] == 'p' && cltAdverse.coupAdverse[0] == 'c') || (monCoup[0] == 'f' && cltAdverse.coupAdverse[0] == 'p'))
			{ //Toutes les possibilitées gagnantes
				printf("Gagné !\n");
				monScore++;
			}
			else
			{
				printf("Aucun participant ne gagne\n");
			}

			printf("Le score pour l'adversaire est de \t%d  -  %d \n", cltAdverse.score, monScore);
			if (cltAdverse.score == 3 || monScore == 3)
			{
				printf("Fin du jeu\n");
				if (cltAdverse.score == 3)
				{
					printf("Vous avez perdu le match\n");
				}
				else
				{
					printf("Vous avez gagné le match !\n");
				}

				printf("Voulez-vous rejouer ? (O/n)\n");
				scanf("%s", choix);
				if (choix[0] == 'n') 
				{

					fin = 1;
				}
				else
				{
					sprintf(requete, "client;%s;%s;%d", pseudo, ippublic, port);

					requestServer(requete);
				}
			}
			else if (champs[1][0] == '1')
			{
				printf("Nouvelle manche\n");
				//Envoi de la requête pour la manche suivante
				printf("[Janken] Quel coup jouez vous (pierre/feuille/ciseaux) ? : \n");
				scanf("%s", monCoup);
				sprintf(requete, "jeux;1;%s", monCoup);
				communicationClientFonction(requete);
			}

			
		}
	}
	

	pthread_exit(EXIT_SUCCESS);
}

/* ------------------------------------------------------------------------ */

/* P A R T I E      C L I E N T 	*/


void choixAdversaire(int nbClient, clt_t clients[], char choixJoueur[], char requete[])
{
	int nbAdversaire = nbClient;
	int joueur;
	printf("Il y a %d adversaires disponibles :\n\n", nbAdversaire);

	for (int i = 0; i < nbAdversaire; i++)
	{
		printf("%d\t%s\t%s\t%d\n", clients[i].id, clients[i].pseudo, clients[i].addrClt, clients[i].portClt);
	}

	printf("Voulez-vous attendre qu'un joueur vous choisisse ? (O/n)\n");
	scanf("%s", choixJoueur);
	if (choixJoueur[0] == 'n' && nbAdversaire != 0)
	{
		printf("Avec quel joueur voulez vous jouer ?\n");
		scanf("%s", choixJoueur);
		//scanf("%d", &choixJoueur);//met en pause si on reçoit une requête, problème
		printf("Vous avez choisi %s\n", choixJoueur);
		joueur = atoi(choixJoueur);
		printf("Demande de jeux avec %s\n", clients[joueur].pseudo);

		strcpy(cltAdverse.pseudo, clients[joueur].pseudo);
		strcpy(cltAdverse.addrClt, clients[joueur].addrClt);
		cltAdverse.portClt = clients[joueur].portClt;

		sprintf(requete, "go;%s;%s;%d", pseudo, ippublic, port);
		communicationClientFonction(requete);
	}
	else if (nbAdversaire == 0)
	{
		printf("Aucun adversaire pour le moment\n");
		sleep(10);
		choixAdversaire(nbClient, clients, choixJoueur, requete);
	}
	else
	{ //attente pseudo ip port
		printf("Mise en attente\n");
		rechercheAdversaire=0;
		sprintf(requete, "attente;%s;%s;%d", pseudo, ippublic, port);
		requestServer(requete);
	}
}

void requestServer(char requete[])
{

	int sad; //socket appel et dialogue;

	// Mise en place du socket d'appel PF_INET/STREAM adressée ou non
	sad = sessionClt();
	// Connexion avec un serveur
	// la socket d'appel devient une socket de dialogue (connectée)
	connectSrv(sad);
	// Dialogue du client avec le serveur
	dialClt2Srv(sad, requete);
	// Fermeture de la socket de dialogue
	getchar();
	CHECK(close(sad), "-- PB : close()");
}
void communicationClientFonction(char msg[])
{ 

	int sad; //socket appel et dialogue;
	
	// Mise en place du socket d'appel PF_INET/STREAM adressée ou non
	sad = sessionClt();
	// Connexion avec un serveur
	// la socket d'appel devient une socket de dialogue (connectée)
	connectClt(sad);
	// Dialogue du client avec le serveur
	dialClt2Srv(sad, msg);
	// Fermeture de la socket de dialogue
	
	CHECK(close(sad), "-- PB : close()");
}

void connectClt(int sad)
{
	struct sockaddr_in cltAdr;
	// le client doit fournir l'adresse du serveur
	cltAdr.sin_family = PF_INET;
	cltAdr.sin_port = htons(cltAdverse.portClt);
	cltAdr.sin_addr.s_addr = inet_addr(cltAdverse.addrClt);
	memset(&(cltAdr.sin_zero), 0, 8);
	
	CHECK(connect(sad, (struct sockaddr *)&cltAdr, sizeof(cltAdr)), "-- PB : connect()");
	
}

void dialClt2Srv(int sad, const char *MSG)
{
	struct sockaddr_in sadAdr;
	socklen_t lenSadAdr;
	message_t buff;
	// Dialogue du client avec le serveur : while(..) { envoiRequete(); attenteReponse();}
	// Ici on va se contenter d'envoyer un message et de recevoir une réponse
	// Envoi d'un message à un destinaire avec \0
	
	CHECK(send(sad, MSG, strlen(MSG) + 1, 0), "-- PB : send()");
	
}
int sessionClt(void)
{
	int sad;
	// Création d’une socket INET/STREAM d'appel et de dialogue
	CHECK(sad = socket(PF_INET, SOCK_STREAM, 0), "-- PB : socket()");
	
	// Le client n'a pas besoin d'avoir une adresse mais il peut
	// ICI, Pas de préparation ni d'association
	return sad;
}
void connectSrv(int sad)
{
	struct sockaddr_in srvAdr;
	// le client doit fournir l'adresse du serveur
	srvAdr.sin_family = PF_INET;
	srvAdr.sin_port = htons(port_srv);
	srvAdr.sin_addr.s_addr = inet_addr(addr_srv);
	memset(&(srvAdr.sin_zero), 0, 8);
	// demande connexion
	CHECK(connect(sad, (struct sockaddr *)&srvAdr, sizeof(srvAdr)), "-- PB : connect()");
	
}

/* P A R T I E      S E R V E U R 	*/

int sessionSrv(char ip[], int port)
{

	int se /*socket Ã©coute*/;
	struct sockaddr_in seAdr;

	// CrÃ©ation dâ€™une socket INET/STREAM : nÃ©cessite une connexion
	CHECK(se = socket(PF_INET, SOCK_STREAM, 0), "-- PB : socket()");
	
	// PrÃ©paration dâ€™un adressage pour une socket INET
	seAdr.sin_family = PF_INET;
	seAdr.sin_port = htons(port); // htons() : network order
	//seAdr.sin_addr.s_addr = inet_addr(ip);
	seAdr.sin_addr.s_addr = inet_addr(ip);
	// adresse effective
	memset(&(seAdr.sin_zero), 0, 8);
	// Association de la socket d'Ã©coute avec lâ€™adresse d'Ã©coute
	CHECK(bind(se, (struct sockaddr *)&seAdr, sizeof(seAdr)), "Erreur ip/port");
	
	// inet_ntoa(seAdr.sin_addr), ntohs(seAdr.sin_port));
	// Mise de la socket Ã  l'Ã©coute
	CHECK(listen(se, 2), "--PB : listen()"); // 5 est le nb de clients mis en attente
	// Boucle permanente (1 serveur est un daemon)
	
	return se;
}
int acceptClt(int se, struct sockaddr_in *cltAdr)
{
	int sd;
	socklen_t lenCltAdr = sizeof(*cltAdr);
	// Attente d'une connexion client : accept() cÃ´tÃ© serveur & connect cÃ´tÃ© client
	// AprÃ¨s acceptation d'une connexion client,
	// il ya crÃ©ation d'une socket de dialoque

	CHECK(sd = accept(se, (struct sockaddr *)cltAdr, &lenCltAdr), "-- PB : accept()");
	
	
	return sd;
}
void dialSrv2Clt(int sd, struct sockaddr_in *cltAdr)
{
	// Dialogue avec le client
	// Ici, lecture d'une reqÃªte et envoi d'une rÃ©ponse
	message_t buff;

	int i = 0;
	// La définitions de séparateurs connus.

	memset(buff, 0, MAX_BUFF);
	
	CHECK(recv(sd, buff, MAX_BUFF, 0), "PB-- recv()");
	

	const char *separators = ";";
	char *strToken = strtok(buff, separators);
	// On cherche à récupérer, un à un, tous les mots (token) de la phrase
	// et on commence par le premier.
	while (strToken != NULL)
	{
		

		strcpy(champs[i], strToken);

		i++;
		strToken = strtok(NULL, separators);
	}

	requeteATraiter = 1;

	CHECK(shutdown(sd, SHUT_WR), "-- PB : shutdown()");
	sleep(1);

	
}

/* S I G N A L     H A N D L E R */
static void signalHandler(int numSig)
{
	char requete[20];
	switch (numSig)
	{
	case SIGINT:
		CHECK(close(sdserveur), "Pb close sd");
		CHECK(close(seserveur), "Pb close se");
		printf("on ferme les sockets\n");

		sprintf(requete, "sup;%s;%s;%d", pseudo, ippublic, port);
		printf("on se supprime des clients du serveur\n");
		requestServer(requete);

		exit(0);
		break;

	case SIGQUIT:
		printf("\nLe programme se ferme\n");

		break;
	}
}
