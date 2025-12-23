#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#include <netdb.h>
#include <arpa/inet.h>

// Définitions des codes d'échappement ANSI pour les couleurs et les styles
#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define UNDERLINE   "\033[4m"
#define REVERSE     "\033[7m"

// Couleurs de texte
#define BLACK       "\033[30m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"

// Couleurs d'arrière-plan (Background)
#define BG_BLACK    "\033[40m"
#define BG_RED      "\033[41m"
#define BG_GREEN    "\033[42m"
#define BG_YELLOW   "\033[43m"
#define BG_BLUE     "\033[44m"
#define BG_MAGENTA  "\033[45m"
#define BG_CYAN     "\033[46m"
#define BG_WHITE    "\033[47m"

/**
 * @brief Affiche un message stylé dans le terminal.
 * * @param text Le message à afficher.
 * @param fg_color La couleur du premier plan (ex: BLUE).
 * @param bg_color La couleur d'arrière-plan (ex: BG_YELLOW).
 * @param style Le style (ex: BOLD ou "" pour aucun).
 */
void print_styled(const char *text, const char *fg_color, const char *bg_color, const char *style) {
    // 1. Applique le style (Gras, Souligné, etc.)
    // 2. Applique la couleur de l'arrière-plan
    // 3. Applique la couleur du premier plan (texte)
    // 4. Affiche le texte
    // 5. Réinitialise le style et la couleur du terminal
    printf("%s%s%s%s%s\n", style, bg_color, fg_color, text, RESET);
}

/**
 * @brief Affiche un message avec une bordure simple pour le mettre en évidence.
 * * @param text Le message à afficher.
 * @param fg_color La couleur du texte.
 */
void print_boxed_title(const char *text, const char *fg_color) {
    size_t len = strlen(text);
    char border[len + 5]; // +5 pour l'espace, le padding, et le caractère null

    // Crée la ligne de bordure
    for (size_t i = 0; i < len + 4; i++) {
        border[i] = '='; // Vous pouvez utiliser différents caractères (ex: '#', '*')
    }
    border[len + 4] = '\0';

    // Affiche la bordure supérieure
    printf("%s%s%s\n", fg_color, BOLD, border);
    
    // Affiche le message
    printf("%s  %s  %s\n", fg_color, text, RESET);
    
    // Affiche la bordure inférieure et réinitialise
    printf("%s%s%s%s\n", fg_color, BOLD, border, RESET);
}

struct _client
{
        char ipAddress[40];
        int port;
        char name[40];
} tcpClients[4]; // supporte au maximum 4 clients
int nbClients; // nombre de clients actuellement connectes
int fsmServer; // si fsmServer==0 j'attends les connexions des joueurs
			   // si fsmServer==1 le jeu est en cours
int deck[13]={0,1,2,3,4,5,6,7,8,9,10,11,12}; // les cartes du jeu
int tableCartes[4][8]; // matrice du serveur contenant pour chaque joueur (ligne) le nombre de cartes de chaque type (colonne)
char *nomcartes[]=
{"Sebastian Moran", "irene Adler", "inspector Lestrade",
  "inspector Gregson", "inspector Baynes", "inspector Bradstreet",
  "inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
  "Mrs. Hudson", "Mary Morstan", "James Moriarty"};
char *nomsymboles[]={"pipe","ampoule","poing","couronne","carnet","collier","oeil","crane"};
int joueurCourant; // id du joueur dont c'est le tour
int joueursElimines[4] = {0, 0, 0, 0}; // tableau indiquant quels joueurs sont elimines

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void melangerDeck() // mélange le tableau des 13 cartes de façon aléatoire
{
        int i;
        int index1,index2,tmp;

        for (i=0;i<1000;i++)
        {
                index1=rand()%13;
                index2=rand()%13;

                tmp=deck[index1];
                deck[index1]=deck[index2];
                deck[index2]=tmp;
        }
}

void createTable() // remplit la matrice tableCartes en fonction des cartes distribuées aux joueurs
{
	// Le joueur 0 possede les cartes d'indice 0,1,2
	// Le joueur 1 possede les cartes d'indice 3,4,5 
	// Le joueur 2 possede les cartes d'indice 6,7,8 
	// Le joueur 3 possede les cartes d'indice 9,10,11 
	// Le coupable est la carte d'indice 12
	int i,j,c;

	for (i=0;i<4;i++)
		for (j=0;j<8;j++)
			tableCartes[i][j]=0;

	for (i=0;i<4;i++)
	{
		for (j=0;j<3;j++)
		{
			c=deck[i*3+j];
			switch (c)
			{
				case 0: // Sebastian Moran
					tableCartes[i][7]++;
					tableCartes[i][2]++;
					break;
				case 1: // Irene Adler
					tableCartes[i][7]++;
					tableCartes[i][1]++;
					tableCartes[i][5]++;
					break;
				case 2: // Inspector Lestrade
					tableCartes[i][3]++;
					tableCartes[i][6]++;
					tableCartes[i][4]++;
					break;
				case 3: // Inspector Gregson 
					tableCartes[i][3]++;
					tableCartes[i][2]++;
					tableCartes[i][4]++;
					break;
				case 4: // Inspector Baynes 
					tableCartes[i][3]++;
					tableCartes[i][1]++;
					break;
				case 5: // Inspector Bradstreet 
					tableCartes[i][3]++;
					tableCartes[i][2]++;
					break;
				case 6: // Inspector Hopkins 
					tableCartes[i][3]++;
					tableCartes[i][0]++;
					tableCartes[i][6]++;
					break;
				case 7: // Sherlock Holmes 
					tableCartes[i][0]++;
					tableCartes[i][1]++;
					tableCartes[i][2]++;
					break;
				case 8: // John Watson 
					tableCartes[i][0]++;
					tableCartes[i][6]++;
					tableCartes[i][2]++;
					break;
				case 9: // Mycroft Holmes 
					tableCartes[i][0]++;
					tableCartes[i][1]++;
					tableCartes[i][4]++;
					break;
				case 10: // Mrs. Hudson 
					tableCartes[i][0]++;
					tableCartes[i][5]++;
					break;
				case 11: // Mary Morstan 
					tableCartes[i][4]++;
					tableCartes[i][5]++;
					break;
				case 12: // James Moriarty 
					tableCartes[i][7]++;
					tableCartes[i][1]++;
					break;
			}
		}
	}
} 

void printDeck() // affiche le contenu du deck et de la tableCartes
{
        int i,j;

        for (i=0;i<13;i++)
			printf("%d %s\n",deck[i],nomcartes[deck[i]]);

	for (i=0;i<4;i++)
	{
		for (j=0;j<8;j++)
		{
			printf("%2.2d ",tableCartes[i][j]);
		}
		puts(""); // équivalent de printf("\n");
	}
}

void printClients() // affiche la liste des clients connectés
{
        int i;

        for (i=0;i<nbClients;i++)
                printf("%d: %s %5.5d %s\n",i,tcpClients[i].ipAddress,
                        tcpClients[i].port,
                        tcpClients[i].name);
}

int findClientByName(char *name) // recherche un client par son nom et retourne son indice dans tcpClients
{
        int i;

        for (i=0;i<nbClients;i++)
                if (strcmp(tcpClients[i].name,name)==0)
                        return i;
        return -1;
}

void sendMessageToClient(char *clientip,int clientport,char *mess) // envoie un message mess au client d'adresse clientip et de port clientport
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    server = gethostbyname(clientip);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(clientport);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        {
                printf("ERROR connecting\n");
                exit(1);
        }

        sprintf(buffer,"%s\n",mess);
        n = write(sockfd,buffer,strlen(buffer));

    close(sockfd);
}

void broadcastMessage(char *mess) // envoie un message mess a tous les clients connectes (le server principale doit prevenir tt le monde)
{
	int i;

	for (i=0;i<nbClients;i++)
	{
		sendMessageToClient(tcpClients[i].ipAddress, tcpClients[i].port, mess);
	}
}

int main(int argc, char *argv[]) // serveur de jeu
{
	while (1)
	{
		print_boxed_title("Nouvelle Partie", CYAN);
		// on réinitialise toutes les variables pour une nouvelle partie
		nbClients = 0;
		fsmServer = 0;
		for (int i=0; i<4; i++)
		{
			joueursElimines[i] = 0;
		}

		srand(time(NULL));
		int sockfd, newsockfd, portno;
		socklen_t clilen;
		char buffer[256];
		struct sockaddr_in serv_addr, cli_addr;
		int n;
		int i;

		char com;
		char clientIpAddress[256], clientName[256];
		int clientPort;
		int id;
		char reply[256];


		if (argc < 2) {
			fprintf(stderr,"ERROR, no port provided\n");
			exit(1);
		}
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) 
		error("ERROR opening socket");

		int opt = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) // permets de réutiliser l'adresse du socket immédiatement après la fermeture
			error("ERROR setsockopt");

		bzero((char *) &serv_addr, sizeof(serv_addr));
		portno = atoi(argv[1]);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(portno);
		if (bind(sockfd, (struct sockaddr *) &serv_addr,
				sizeof(serv_addr)) < 0) 
				error("ERROR on binding");
		listen(sockfd,5);
		clilen = sizeof(cli_addr);

		printDeck();
		melangerDeck();
		createTable();
		printDeck();
		joueurCourant=0;

		for (i=0;i<4;i++)
		{
			strcpy(tcpClients[i].ipAddress,"localhost");
			tcpClients[i].port=-1;
			strcpy(tcpClients[i].name,"-");
		}
		int JeunEnCours=1;
		while (JeunEnCours)
		{    
			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
			if (newsockfd < 0) 
			{
				error("ERROR on accept");
			}
			bzero(buffer,256);
			n = read(newsockfd,buffer,255);
			if (n < 0) 
			{
				error("ERROR reading from socket");
			}

			printf("Received packet from %s:%d\nData: [%s]\n\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), buffer);

			if (fsmServer==0)
			{
				switch (buffer[0])
				{
					case 'C':
						sscanf(buffer,"%c %s %d %s", &com, clientIpAddress, &clientPort, clientName);
						printf("COM=%c ipAddress=%s port=%d name=%s\n",com, clientIpAddress, clientPort, clientName);

						// fsmServer==0 alors j'attends les connexions de tous les joueurs
						strcpy(tcpClients[nbClients].ipAddress,clientIpAddress);
						tcpClients[nbClients].port=clientPort;
						strcpy(tcpClients[nbClients].name,clientName);
						nbClients++;

						printClients();

						// rechercher l'id du joueur qui vient de se connecter

						id=findClientByName(clientName);
						printf("id=%d\n",id);

						// lui envoyer un message personnel pour lui communiquer son id

						sprintf(reply,"I %d",id);
						sendMessageToClient(tcpClients[id].ipAddress,
								tcpClients[id].port,
								reply);

						// Envoyer un message broadcast pour communiquer a tout le monde la liste des joueurs actuellement
						// connectes

						sprintf(reply,"L %s %s %s %s", tcpClients[0].name, tcpClients[1].name, tcpClients[2].name, tcpClients[3].name);
						broadcastMessage(reply);

						// Si le nombre de joueurs atteint 4, alors on peut lancer le jeu

						if (nbClients==4)
						{
							// On envoie ses cartes au joueur 0, ainsi que la ligne qui lui correspond dans tableCartes
							// RAJOUTER DU CODE ICI

							sprintf(reply,"D %d %d %d",deck[0],deck[1],deck[2]);
							sendMessageToClient(tcpClients[0].ipAddress,
									tcpClients[0].port,
									reply);
							for (int j=0;j<8;j++)
							{
								sprintf(reply,"V 0 %d %d", j, tableCartes[0][j]);
								sendMessageToClient(tcpClients[0].ipAddress,
										tcpClients[0].port,
										reply);
							}
							// On envoie ses cartes au joueur 1, ainsi que la ligne qui lui correspond dans tableCartes
							// RAJOUTER DU CODE ICI

							sprintf(reply,"D %d %d %d",deck[3],deck[4],deck[5]);
							sendMessageToClient(tcpClients[1].ipAddress,
									tcpClients[1].port,
									reply);
							for (int j=0;j<8;j++)
							{
								sprintf(reply,"V 1 %d %d", j, tableCartes[1][j]);
								sendMessageToClient(tcpClients[1].ipAddress,
										tcpClients[1].port,
										reply);
							}
							// On envoie ses cartes au joueur 2, ainsi que la ligne qui lui correspond dans tableCartes
							// RAJOUTER DU CODE ICI

							sprintf(reply,"D %d %d %d",deck[6],deck[7],deck[8]);
							sendMessageToClient(tcpClients[2].ipAddress,
									tcpClients[2].port,
									reply);
							for (int j=0;j<8;j++)
							{
								sprintf(reply,"V 2 %d %d", j, tableCartes[2][j]);
								sendMessageToClient(tcpClients[2].ipAddress,
										tcpClients[2].port,
										reply);
							}

							// On envoie ses cartes au joueur 3, ainsi que la ligne qui lui correspond dans tableCartes
							// RAJOUTER DU CODE ICI

							sprintf(reply,"D %d %d %d",deck[9],deck[10],deck[11]);
							sendMessageToClient(tcpClients[3].ipAddress,
									tcpClients[3].port,
									reply);

							for (int j=0;j<8;j++)
							{
								sprintf(reply,"V 3 %d %d", j, tableCartes[3][j]);
								sendMessageToClient(tcpClients[3].ipAddress,
										tcpClients[3].port,
										reply);
							}

							// On envoie enfin un message a tout le monde pour definir qui est le joueur courant=0
							// RAJOUTER DU CODE ICI

							sprintf(reply,"M %d",joueurCourant);
							broadcastMessage(reply);
							fsmServer=1;
						}
						break;
				}
			}
			else if (fsmServer==1)
			{
				int id_demandeur, symbole, id_receveur, guilt;
				switch (buffer[0])
				{
					case 'G': // guilt
						// RAJOUTER DU CODE ICI
						sscanf(buffer + 2,"%d %d", &id_demandeur, &guilt);
						printf("%s dit que %s est le coupable\n", tcpClients[id_demandeur].name, nomcartes[guilt]);
						if (guilt == deck[12])
						{
							sprintf(reply,"Victoire de %s",tcpClients[id_demandeur].name);
							print_boxed_title(reply, GREEN);
							sprintf(reply,"T %d",id_demandeur);
							broadcastMessage(reply);
							JeunEnCours = 0;
						} else 
						{
							sprintf(reply,"Élimination de %s",tcpClients[id_demandeur].name);
							print_boxed_title(reply, RED);
							sprintf(reply,"E %d",id_demandeur);
							broadcastMessage(reply);
						}
						joueursElimines[id_demandeur] = 1;
						i = 0;
						do {
							joueurCourant = (joueurCourant + 1) % 4;
							i++;
						} while (joueursElimines[joueurCourant] == 1 && i < 4);
						if (i == 4)
						{
							print_boxed_title("Tous les joueurs sont éliminés. Fin de la partie.", RED);
							JeunEnCours = 0;
							continue;
						}
						sprintf(reply,"M %d",joueurCourant);
						broadcastMessage(reply);


						break;
					case 'O': // un joueur fait demande qui a des cartes avec le symbole ...
						// RAJOUTER DU CODE ICI
						sscanf(buffer,"%c %d %d", &com, &id_demandeur, &symbole);
						printf("%s demande qui a des cartes avec le symbole %s\n", tcpClients[id_demandeur].name, nomsymboles[symbole]);
						for (int i = 0; i < 4; i++) {
							if (i != id_demandeur)
							{
								if (tableCartes[i][symbole] > 0) {
									sprintf(reply,"V %d %d %d",i, symbole, 100); // 100 signifie "j'en ai"
									for (int j=0;j<4;j++)
									{
										if (j != i)
										{
											sendMessageToClient(tcpClients[j].ipAddress,
													tcpClients[j].port,
													reply);
										}
									}
								} else if (tableCartes[i][symbole] == 0) {
									sprintf(reply,"V %d %d %d",i, symbole, 0); // 0 signifie "j'en ai pas"
									for (int j=0;j<4;j++)
									{
										if (j != i)
										{
											sendMessageToClient(tcpClients[j].ipAddress,
													tcpClients[j].port,
													reply);
										}
									}
								}
							}
							printf("Joueur %s a %d cartes avec le symbole %s\n", tcpClients[i].name, tableCartes[i][symbole], nomsymboles[symbole]);
						}
						i = 0;
						do {
							joueurCourant = (joueurCourant + 1) % 4;
							i++;
						} while (joueursElimines[joueurCourant] == 1 && i < 4);
						if (i == 4)
						{
							print_boxed_title("Tous les joueurs sont éliminés. Fin de la partie.", RED);
							JeunEnCours = 0;
							continue;
						}
						sprintf(reply,"M %d",joueurCourant);
						broadcastMessage(reply);

						break;
					case 'S': // un joueur demande a un joueur t'as combien de cartes avec le symbole ...
						// RAJOUTER DU CODE ICI
						sscanf(buffer + 2,"%d %d %d", &id_demandeur, &id_receveur, &symbole);
						printf("%s demande à %s combien il a de cartes avec le symbole %s\n", tcpClients[id_demandeur].name, tcpClients[id_receveur].name, nomsymboles[symbole]);
						sprintf(reply,"V %d %d %d",id_receveur, symbole, tableCartes[id_receveur][symbole]);
						for (int j=0;j<4;j++)
						{
							if (j != id_receveur)
							{
								sendMessageToClient(tcpClients[j].ipAddress,
										tcpClients[j].port,
										reply);
							}
						}
						i = 0;
						do {
							joueurCourant = (joueurCourant + 1) % 4;
							i++;
						} while (joueursElimines[joueurCourant] == 1 && i < 4);
						if (i == 4)
						{
							print_boxed_title("Tous les joueurs sont éliminés. Fin de la partie.", RED);
							JeunEnCours = 0;
							continue;
						}
						sprintf(reply,"M %d",joueurCourant);
						broadcastMessage(reply);
						break;
					case 'E': // un joueur à quitter la partie
						sscanf(buffer + 2,"%d", &id);
						sprintf(reply,"Le joueur %s à quitté la partie", tcpClients[id].name);
						print_boxed_title(reply, RED);
						JeunEnCours = 0;
						sprintf(reply,"F %d",id);
						broadcastMessage(reply);
						break;
					default:
						break;
				}
			}
			close(newsockfd);
		}
		close(sockfd);
	}
	return 0; 
}
