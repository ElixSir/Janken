// V1 : fcts session, et connexion
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define CHECK(sts, msg) if ((sts)==-1) {perror(msg); exit(-1);}
#define MAX_BUFF	512

							// ne pas oublier d'exlure les ports : Assigned Ports

typedef char message_t[MAX_BUFF];

void serveur (char ip[], int port);
void client (const char * MSG);

int requetethread2 = 0;
int cas;

int fin=1;

pthread_t thread;
pthread_t thread2;
int seserveur;
int sdserveur;
void pthreadserveur();

int nombreClients = 0;

struct client{
char pseudo[20];
char IP[16];
char port[7];
char etat[10];
};


void communicationClientFonction(char MSG[], char IP[], int port );

struct client clients[7];

static void signalHandler(int);

int sessionSrv(char ip[], int port);
void sendreponse();

int main () {

	    struct sigaction newActionn;
	    newActionn.sa_handler = signalHandler;
	    sigemptyset(&newActionn.sa_mask);
	   // newAction.sa_flags = SA_SIGINFO;
	    sigaction(SIGINT, &newActionn, NULL);

	char ip[16];
	int port;
	printf("[Janken] Entrez une adresse ip pour l'écoute du serveur : ");
	scanf("%s", ip);
	printf("[Janken] Entrez un port pour l'écoute du serveur : ");
	scanf("%d", &port);
	seserveur = sessionSrv(ip, port);
	pthread_create(&thread, NULL, (void *)pthreadserveur, NULL);
	pthread_create(&thread2, NULL, (void *)sendreponse, NULL);
	printf("Attente des threads\n");
	pthread_join(thread, NULL);
	pthread_join(thread2, NULL);
	printf("Fin de l'application\n");
	return 0;
}

int sessionSrv(char ip[], int port) {
	int se /*socket Ã©coute*/;
	struct sockaddr_in seAdr;

	// CrÃ©ation dâ€™une socket INET/STREAM : nÃ©cessite une connexion
	CHECK(se = socket(PF_INET, SOCK_STREAM, 0),"-- PB : socket()");
	printf("[SERVER]:CrÃ©ation de la socket d'Ã©coute [%d]\n", se);
	// PrÃ©paration dâ€™un adressage pour une socket INET
	seAdr.sin_family = PF_INET;
	seAdr.sin_port = htons(port);				// htons() : network order	
	seAdr.sin_addr.s_addr = inet_addr(ip);
	//seAdr.sin_addr.s_addr = INADDR_ANY; 
	// adresse effective
	memset(&(seAdr.sin_zero), 0, 8);
	// Association de la socket d'Ã©coute avec lâ€™adresse d'Ã©coute
	CHECK(bind(se, (struct sockaddr *)&seAdr, sizeof(seAdr)),"Erreur ip/port");
	printf("[SERVER]:Association de la socket [%d] avec l'adresse [%s:%d]\n", se,
				inet_ntoa(seAdr.sin_addr), ntohs(seAdr.sin_port));
	// Mise de la socket Ã  l'Ã©coute
	CHECK(listen(se, 2), "--PB : listen()");	// 5 est le nb de clients mis en attente
	// Boucle permanente (1 serveur est un daemon)
	printf("[SERVER]:Ecoute de demande de connexion (2 max) sur le canal [%d] d'adresse [%s:%d]\n", se, inet_ntoa(seAdr.sin_addr), ntohs(seAdr.sin_port));
	return se;
}
int acceptClt(int se, struct sockaddr_in *cltAdr) {
	int sd;
	socklen_t lenCltAdr=sizeof(*cltAdr);
	// Attente d'une connexion client : accept() cÃ´tÃ© serveur & connect cÃ´tÃ© client
	// AprÃ¨s acceptation d'une connexion client, 
	// il ya crÃ©ation d'une socket de dialoque
	printf("[SERVER]:Attente d'une connexion client\n");
	CHECK(sd=accept(se, (struct sockaddr *)cltAdr, &lenCltAdr),"-- PB : accept()");
	printf("[SERVER]:Acceptation de connexion du client [%s:%d]\n",
			inet_ntoa(cltAdr->sin_addr), ntohs(cltAdr->sin_port));
	return sd;
}
void dialSrv2Clt(int sd, struct sockaddr_in *cltAdr) {
	// Dialogue avec le client
	// Ici, lecture d'une reqÃªte et envoi d'une rÃ©ponse
	message_t buff;
	int req;	

	memset(buff, 0, MAX_BUFF);
	printf("\t[SERVER]:Attente de rÃ©ception d'une requÃªte\n");
	CHECK (recv(sd, buff, MAX_BUFF, 0), "PB-- recv()");
	printf("\t[SERVER]:RequÃªte reÃ§ue : ##%s##\n", buff);
	printf("\t\t[SERVER]:du client d'adresse [%s:%d]\n", inet_ntoa(cltAdr->sin_addr), ntohs(cltAdr->sin_port));
	

   	 // La définitions de séparateurs connus.
   	 const char * separators = ";";

   	 // On cherche à récupérer, un à un, tous les mots (token) de la phrase
    	// et on commence par le premier.
   	 char * strToken = strtok ( buff, separators );
   	 int index=0;
   	 
   	 char pseudo[20];
   	 char type[7];
   	 char ip[16];
   	 char port[6];
   	 
   	 
   	 while ( strToken != NULL ) {
    	    
    	    if(index == 0){
    	    	strcpy(type, strToken);
    	    }
    	    
    	    if(index==1){
		strcpy(pseudo, strToken);	
    	    }
    	    
    	    if(index==2){
    	    	strcpy(ip, strToken);
    	    }
    	    	
    	    	
    	    if(index==3){
    	    	strcpy(port, strToken);
    	    }
    	    	
    	    
    	    index++;
     	   strToken = strtok ( NULL, separators );
   	 }
   	 
   	 
   	   if(type[0] == 'a' && type[0] == 't'){
   	 		for(int i=0;i<nombreClients;i++){
			if(strcmp(ip, clients[i].IP) == 0 && strcmp(port, clients[i].port) == 0){
				if(strcmp(clients[i].etat, "recherche") == 0){
					sprintf(clients[i].etat,"attente");
					cas = 1;
					requetethread2 = 1;
					printf("Etat client mis a jour\n");
					CHECK(shutdown(sd, SHUT_WR),"-- PB : shutdown() dialsrv2client");
					sleep(1);
					return;
				}
				if(strcmp(clients[i].etat, "attente") == 0){
					sprintf(clients[i].etat,"recherche");
					printf("Etat client mis a jour\n");
					cas = 1;
					requetethread2 = 1;
					CHECK(shutdown(sd, SHUT_WR),"-- PB : shutdown() dialsrv2client");
					sleep(1);
					return;
				}
				
			}
		}
   	 
   	 }
   	 
   	 if(type[0] == 's' && type[0] == 'u'){
   	 
   	 		for(int i=0;i<nombreClients;i++){
			if(strcmp(ip, clients[i].IP) == 0 && strcmp(port, clients[i].port) == 0){
				for(int j=i;j<nombreClients-1;j++){
					sprintf(clients[j].IP,"%s", clients[j+1].IP);
					sprintf(clients[j].pseudo,"%s", clients[j+1].pseudo);
					sprintf(clients[j].port,"%s", clients[j+1].port);
				}
					 nombreClients--;
				   	 printf("Suppréssion d'un client\n");
					 CHECK(shutdown(sd, SHUT_WR),"-- PB : shutdown() dialsrv2client");
					 sleep(1);
					 return;
			}
		}
   	 
   	 }
   	 
   	 
   	 
	if(type[0] == 'c' && type[1] == 'l'){
	
	if(nombreClients == 8){ printf("Refu d'un client\n"); char *req = "refus"; CHECK(send(sd, req, strlen(req) +1,0), "PB -- send()"); CHECK(shutdown(sd, SHUT_WR),"-- PB : shutdown()");sleep(1); return;} // securité
	
		
	
	
		if(nombreClients >= 1){
			sprintf(clients[nombreClients].pseudo,"%s", pseudo);
			sprintf(clients[nombreClients].IP,"%s", ip);
			sprintf(clients[nombreClients].port,"%s", port);
			sprintf(clients[nombreClients].etat,"recherche");
			nombreClients++;
			// Send requête attente
			
			cas = 1;
			requetethread2 = 1;
			printf("Second Client, envoi client 1\n");
		}
		
		if(nombreClients == 0){ // Si c'est le premier client l'ajouter a la liste et lui dire d'attendre
			sprintf(clients[nombreClients].pseudo,"%s", pseudo);
			sprintf(clients[nombreClients].IP,"%s", ip);
			sprintf(clients[nombreClients].port,"%s", port);
			sprintf(clients[nombreClients].etat,"attente");
			nombreClients++;
			printf("Envoyéici %s %s\n", clients[0].IP, clients[0].port);
			// send requête clt0
			cas = 2;
			requetethread2 = 1;
			printf("Premier client, requete en attente\n");
		}
		

		
	}
   	 
   	 if(type[0] == 'f' && type[1] == 'i' && type[2] == 'n'){
   	 	nombreClients = 0;
   	 	memset(clients, 0, 2);
   	 }
   	 
   	 printf("Requete type %s avec comme pseudo %s comme ip %s et port %s\n", type,pseudo,ip,port);
	CHECK(shutdown(sd, SHUT_WR),"-- PB : shutdown() dialsrv2client");
	sleep(1);
	// utiliser les getsockopts pour dÃ©terminer si le client a envoyÃ© qq chose
}



void sendreponse(){

	while(fin){
	
		if(requetethread2){
			requetethread2 = 0;
		
			if(cas == 1){
			
				// Connection
				
					int taille = strlen("serveur;1;");
					for(int i=0; i < nombreClients; i++){
						if(strcmp(clients[i].etat, "attente")==0){
					 	taille = taille + strlen(clients[i].pseudo) +1 + strlen(clients[i].IP) + 1 + strlen(clients[i].port) +1;
					 	}
					}
					taille = taille +1; // Ajout du caractère de fin \0 
					char req[taille];
					int clientattente = 0;
					for(int i=0;i<nombreClients;i++){
						if(strcmp(clients[i].etat, "attente")==0){
						clientattente++;
						}
					}
					
					sprintf(req, "serveur;%i;", clientattente);
					for(int i=0; i< nombreClients; i++){
					if(strcmp(clients[i].etat, "attente")==0){
						sprintf(req, "%s%i;%s;%s;%s;", req,i,clients[i].pseudo,clients[i].IP,clients[i].port);
						}
					}
					
				//CHECK(send(sd, req, strlen(req) +1,0), "PB -- send()");
				for(int i=0; i< nombreClients; i++){
					communicationClientFonction(req, clients[i].IP, atoi(clients[i].port) ); // Réponse du client (-1 car nbC a était incrémenté)
					printf("Taille : %i\n", taille);
					
				}
				printf("Envoyé1 %s\n", req);
				
				
			}
			
			if(cas == 2){
				
			// Send requête client 1
			char *req = "serveur;attente";
			//CHECK(send(sd, req, strlen(req) +1,0), "PB -- send()");
			communicationClientFonction(req, clients[0].IP, atoi(clients[0].port) );
			printf("Envoyé5 %s %s\n", clients[0].IP, clients[0].port);
			}
			
			

			cas = 0;
		
		}
		sleep(1);
	}


}


void pthreadserveur(){

struct sockaddr_in cltAdrSrv;
	while (fin) {
		// attente de connexion d'un client et crÃ©ation d'une socket de dialogue
		sdserveur=acceptClt(seserveur, &cltAdrSrv);

			// dialogue avec le client connectÃ©
			dialSrv2Clt(sdserveur, &cltAdrSrv);
	
				
	} 

	pthread_exit(0);



}


static void signalHandler(int numSig){

switch(numSig){
	case SIGINT:
		printf("\nLe programme se ferme\n");
		fin = 0;
		sleep(1);
		CHECK(close(sdserveur), "Pb close sd");
		CHECK(close(seserveur), "Pb close se");
		exit(0);
		break;
		
	case SIGQUIT:
	printf("\nLe programme se ferme\n");
		fin = 0;
		sleep(1);
		CHECK(close(sdserveur), "Pb close sd");
		CHECK(close(seserveur), "Pb close se");
		exit(0);
		break;
}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////CLT/////////////////////////////////////////////////////////

void dialClt2Srv(int sad, const char * MSG) {

	printf("\t[CLIENT]:Envoi d'une requête sur [%d]\n", sad);
	CHECK(send(sad, MSG, strlen(MSG)+1, 0),"-- PB : send()");
	printf("\t\t[CLIENT]:requête envoyée : ##%s##\n", MSG);


}
int sessionClt(void) {
	int sad;
	// Création d’une socket INET/STREAM d'appel et de dialogue
	CHECK(sad = socket(PF_INET, SOCK_STREAM, 0),"-- PB : socket()");
	printf("[CLIENT]:Création de la socket d'appel et de dialogue [%d]\n", sad);		
	// Le client n'a pas besoin d'avoir une adresse mais il peut
	// ICI, Pas de préparation ni d'association
	return sad;
}
void connectSrv(int sad, char ip[],int port) {
	struct sockaddr_in srvAdr;
	// le client doit fournir l'adresse du serveur
	srvAdr.sin_family = PF_INET;
	srvAdr.sin_port = htons(port);		
	srvAdr.sin_addr.s_addr = inet_addr(ip);
	memset(&(srvAdr.sin_zero), 0, 8);
	// demande connexion 
	CHECK(connect(sad, (struct sockaddr *)&srvAdr, sizeof(srvAdr)),"-- PB : connect()");
	printf("[CLIENT]:Connexion effectuée avec le serveur [%s:%d] par le canal [%d]\n",inet_ntoa(srvAdr.sin_addr), ntohs(srvAdr.sin_port), sad);	
}


void communicationClientFonction(char MSG[], char IP[] , int port )
{
	//const char * MSG="Client;Elie;127.0.0.10;22000";
    	int sad; //socket appel et dialogue;
	printf("Communication");
	// Mise en place du socket d'appel PF_INET/STREAM adressée ou non
	sad=sessionClt();
	// Connexion avec un serveur
	// la socket d'appel devient une socket de dialogue (connectée)
	connectSrv(sad, IP, port);
	// Dialogue du client avec le serveur
	dialClt2Srv(sad, MSG);
	// Fermeture de la socket de dialogue
	//getchar();
	CHECK(close(sad),"-- PB : close()");
	//pthread_exit(EXIT_SUCCESS);
}


