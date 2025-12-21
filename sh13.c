#include <SDL.h>        
#include <SDL_image.h>        
#include <SDL_ttf.h>         
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/*Le jeu SH13 fonctionne de la manière suivante :
  - Chaque joueur a une main de cartes et doit deviner l'objet, le joueur et le mobile du crime.
  - Les joueurs peuvent se connecter à un serveur pour jouer en ligne.
  - Le serveur gère les connexions des clients et la logique du jeu.
*/


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

pthread_t thread_serveur_tcp_id;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char gbuffer[256];
char gServerIpAddress[256];
int gServerPort;
char gClientIpAddress[256];
int gClientPort;
char gName[256];
char gNames[4][256];
int gId;
int joueurSel;
int objetSel;
int guiltSel;
int guiltGuess[13];
int tableCartes[4][8];
int b[3];
int goEnabled;
int connectEnabled;

char *nbobjets[]={"5","5","5","5","4","3","3","3"};
char *nbnoms[]={"Sebastian Moran", "irene Adler", "inspector Lestrade",
  "inspector Gregson", "inspector Baynes", "inspector Bradstreet",
  "inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
  "Mrs. Hudson", "Mary Morstan", "James Moriarty"};

volatile int synchro;

void *fn_serveur_tcp(void *arg)
{
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd<0)
	{
		printf("sockfd error\n");
		exit(1);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = gClientPort;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("bind error\n");
		exit(1);
	}

	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	while (1) // boucle infinie du thread serveur tcp
	{
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); 
		if (newsockfd < 0)
		{
			printf("accept error\n");
			exit(1);
		}

		bzero(gbuffer,256); // met toute les valeurs du buffer a 0
		n = read(newsockfd,gbuffer,255); // ecrit dans gbuffer
		if (n < 0)
		{
			printf("read error\n");
			exit(1);
		}
		//printf("%s",gbuffer);

		synchro=1; // indique au thread principal qu'un message est dispo

		while (synchro); // attend que le thread principal consomme le message

	}
}

void sendMessageToServer(char *ipAddress, int portno, char *mess)
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char sendbuffer[1024];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    server = gethostbyname(ipAddress);
    if (server == NULL) 
	{
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
	{
		printf("ERROR connecting\n");
		exit(1);
	}

	snprintf(sendbuffer, sizeof(sendbuffer), "%s\n", mess);
	n = write(sockfd,sendbuffer,strlen(sendbuffer));

    close(sockfd);
}

int main(int argc, char ** argv)
{
	int ret;
	int i,j;
	int joueurElimine[4]={0,0,0,0};
	int joueur_courant;

    int quit = 0;
    SDL_Event event;
	int mx,my;
	char sendBuffer[1024];
	char lname[256];
	int id;

	if (argc<6) // verifie les arguments
	{
		print_boxed_title("Usage:", RED);
		printf("<app> <Main server ip address> <Main server port> <Client ip address> <Client port> <player name>\n");
		exit(1);
	}

	strcpy(gServerIpAddress,argv[1]); // adresse ip du serveur principal
	gServerPort=atoi(argv[2]); // port du serveur principal en int
	strcpy(gClientIpAddress,argv[3]); // adresse ip du client
	gClientPort=atoi(argv[4]); // port du client en int
	strcpy(gName,argv[5]); // nom du joueur

    SDL_Init(SDL_INIT_VIDEO); // Initialize SDL2
	TTF_Init(); // Initialize SDL2_ttf
 
    SDL_Window * window = SDL_CreateWindow("SDL2 SH13", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, 0); // creer une fenetre SDL
 
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);  // creer un renderer SDL

    SDL_Surface *deck[13],*objet[8],*gobutton,*connectbutton,*logo_su,*logo_ps; /*declaration des surfaces SDL_Surface 
	afin de stocker les images, c'est comme si un tableau stockait des images*/

	deck[0] = IMG_Load("SH13_0.png"); // load toute les images dans des surfaces SDL_Surface
	deck[1] = IMG_Load("SH13_1.png");
	deck[2] = IMG_Load("SH13_2.png");
	deck[3] = IMG_Load("SH13_3.png");
	deck[4] = IMG_Load("SH13_4.png");
	deck[5] = IMG_Load("SH13_5.png");
	deck[6] = IMG_Load("SH13_6.png");
	deck[7] = IMG_Load("SH13_7.png");
	deck[8] = IMG_Load("SH13_8.png");
	deck[9] = IMG_Load("SH13_9.png");
	deck[10] = IMG_Load("SH13_10.png");
	deck[11] = IMG_Load("SH13_11.png");
	deck[12] = IMG_Load("SH13_12.png");

	objet[0] = IMG_Load("SH13_pipe_120x120.png");
	objet[1] = IMG_Load("SH13_ampoule_120x120.png");
	objet[2] = IMG_Load("SH13_poing_120x120.png");
	objet[3] = IMG_Load("SH13_couronne_120x120.png");
	objet[4] = IMG_Load("SH13_carnet_120x120.png");
	objet[5] = IMG_Load("SH13_collier_120x120.png");
	objet[6] = IMG_Load("SH13_oeil_120x120.png");
	objet[7] = IMG_Load("SH13_crane_120x120.png");

	gobutton = IMG_Load("gobutton.png"); // load des boutons
	connectbutton = IMG_Load("connectbutton.png"); // load des boutons
	logo_su = IMG_Load("logo_su.png"); // load logo su
	logo_ps = IMG_Load("logo_ps.png"); // load logo ps

	strcpy(gNames[0],"-"); // initialise les noms des joueurs a "-"
	strcpy(gNames[1],"-");
	strcpy(gNames[2],"-");
	strcpy(gNames[3],"-");

	joueurSel=-1; // aucune selection au debut
	objetSel=-1; // aucune selection au debut
	guiltSel=-1; // aucune selection au debut

	b[0]=-1; // les trois images que chaque joueur reçois n'affiche rien au début
	b[1]=-1;
	b[2]=-1;

	for (i=0;i<13;i++)
	{
		guiltGuess[i]=0; // guilt est l'une des trois possibilité pdt un tour cad accusé qlqn
	}
	for (i=0;i<4;i++)
	{
		for (j=0;j<8;j++)
		{
			tableCartes[i][j]=-1; 
		}
	}
	goEnabled=0; // le bouton go n'est pas actif au debut
	connectEnabled=1; // le bouton connect est actif au debut
    // transforme les surfaces en textures pour l'affichage
    SDL_Texture *texture_deck[13],*texture_gobutton,*texture_connectbutton,*texture_objet[8],*texture_logo_su,*texture_logo_ps;

	for (i=0;i<13;i++)
	{
		texture_deck[i] = SDL_CreateTextureFromSurface(renderer, deck[i]); // transforme les surfaces en textures pour l'affichage
	}
	for (i=0;i<8;i++)
	{
		texture_objet[i] = SDL_CreateTextureFromSurface(renderer, objet[i]); // transforme les surfaces en textures pour l'affichage
	}

    texture_gobutton = SDL_CreateTextureFromSurface(renderer, gobutton); // transforme les surfaces en textures pour l'affichage
    texture_connectbutton = SDL_CreateTextureFromSurface(renderer, connectbutton); // transforme les surfaces en textures pour l'affichage
	texture_logo_su = SDL_CreateTextureFromSurface(renderer, logo_su); // transforme les surfaces en textures pour l'affichage
	texture_logo_ps = SDL_CreateTextureFromSurface(renderer, logo_ps); // transforme les surfaces en textures pour l'affichage
	// Chargement de la police de caractères
    TTF_Font* Sans = TTF_OpenFont("sans.ttf", 15); 
    TTF_Font* SansBig = TTF_OpenFont("sans.ttf", 30); 
    printf("Sans=%p\n",Sans); // ecrit l'adresse de la police dans le terminal

	/* Creation du thread serveur tcp. */
	printf ("Creation du thread serveur tcp !\n"); // ecrit dans le terminal
	synchro=0; // indique qu'aucun message n'est dispo
	ret = pthread_create ( & thread_serveur_tcp_id, NULL, fn_serveur_tcp, NULL); // cree le thread serveur tcp

    while (!quit) // boucle principale SDL 
    {
		if (SDL_PollEvent(&event))
		{
			//printf("un event\n");
			switch (event.type) // traite les evenements
			{
				case SDL_QUIT: // fermeture de la fenetre
					quit = 1;
					joueurElimine[gId] = 1;
					sprintf(sendBuffer,"E %d",gId);
					sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);
					break;
				case  SDL_MOUSEBUTTONDOWN: // un clic de souris
					SDL_GetMouseState( &mx, &my ); //renvoie la position de la souris
					//printf("mx=%d my=%d\n",mx,my);
					if ((mx<200) && (my<50) && (connectEnabled==1))
					{
						snprintf(sendBuffer, sizeof(sendBuffer), "C %s %d %s", gClientIpAddress, gClientPort, gName);

						// RAJOUTER DU CODE ICI
						sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);
						// fin du code à rajouter
						connectEnabled=0; // desactive le bouton connect
					}
					else if ((mx>=0) && (mx<200) && (my>=90) && (my<330))
					{
						joueurSel=(my-90)/60;
						guiltSel=-1;
					}
					else if ((mx>=200) && (mx<680) && (my>=0) && (my<90))
					{
						objetSel=(mx-200)/60;
						guiltSel=-1;
					}
					else if ((mx>=100) && (mx<250) && (my>=350) && (my<740))
					{
						joueurSel=-1;
						objetSel=-1;
						guiltSel=(my-350)/30;
					}
					else if ((mx>=250) && (mx<300) && (my>=350) && (my<740))
					{
						int ind=(my-350)/30;
						guiltGuess[ind]=1-guiltGuess[ind];
					}
					else if ((mx>=500) && (mx<700) && (my>=350) && (my<450) && (goEnabled==1))
					{
						printf("go! joueur=%d objet=%d guilt=%d\n",joueurSel, objetSel, guiltSel);
						if (guiltSel!=-1)
						{
							sprintf(sendBuffer,"G %d %d",gId, guiltSel);

							// RAJOUTER DU CODE ICI
							sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer); // on envoie au serveur qu'il y a un guilt

						}
						else if ((objetSel!=-1) && (joueurSel==-1))
						{
							sprintf(sendBuffer,"O %d %d",gId, objetSel);

							// RAJOUTER DU CODE ICI
							sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer); // demande qui a des cartes avec l'objetSel

						}
						else if ((objetSel!=-1) && (joueurSel!=-1))
						{
							sprintf(sendBuffer,"S %d %d %d",gId, joueurSel,objetSel);

							// RAJOUTER DU CODE ICI
							sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);// demande a un joueur qui a des cartes avec l'objetSel
						}
					}
					else
					{
						joueurSel=-1;
						objetSel=-1;
						guiltSel=-1;
					}
					break;
				case  SDL_MOUSEMOTION:
					SDL_GetMouseState( &mx, &my ); //renvoie la position de la souris
					break;
			}
		}

        if (synchro==1) // un message est dispo du thread serveur tcp
        {
			printf("consomme |%s|\n",gbuffer);
			switch (gbuffer[0])
			{
				// Message 'I' : le joueur recoit son Id
				case 'I':
					// RAJOUTER DU CODE ICI
					printf("Id recu: %s\n", gbuffer+2);
					gId = atoi(gbuffer+2);
					break;
				// Message 'L' : le joueur recoit la liste des joueurs
				case 'L':
					// RAJOUTER DU CODE ICI
					printf("La liste des joueurs connecté est la suivante : %s\n", gbuffer+2);
					sscanf(gbuffer+2,"%s %s %s %s", gNames[0], gNames[1], gNames[2], gNames[3]);
					break;
				// Message 'D' : le joueur recoit ses trois cartes
				case 'D': // ss doute actualiser le tableau b[]
					// RAJOUTER DU CODE ICI
					printf("Cartes recues: %s\n", gbuffer+2);
					// extraire les trois cartes du message
					sscanf(gbuffer+2," %d %d %d",&b[0],&b[1],&b[2]);
					break;
				// Message 'M' : le joueur recoit le n° du joueur courant
				// Cela permet d'affecter goEnabled pour autoriser l'affichage du bouton go
				case 'M':
					// RAJOUTER DU CODE ICI
					printf("Joueur courant: %s\n", gbuffer+2);
					joueur_courant = atoi(gbuffer+2);
					if (atoi(gbuffer+2) == gId)
						goEnabled = 1; // autorise le bouton go
					else
						goEnabled = 0; // desactive le bouton go
					break;
				// Message 'V' : le joueur recoit une valeur de tableCartes
				case 'V':
					// RAJOUTER DU CODE ICI
					int temp;
					sscanf(gbuffer+2,"%d %d", &i, &j);
					sscanf(gbuffer+5,"%d", &temp);
					if (tableCartes[i][j] == -1) {
						tableCartes[i][j] = temp;
						printf("tableCartes[%d][%d] = %d\n", i, j, temp);
					}
					else if (temp != 100)
					{
						tableCartes[i][j] = temp;
						printf("tableCartes[%d][%d] = %d\n", i, j, temp);
					}
					break;
				case 'E': // un joueur est éliminé
					sscanf(gbuffer + 2,"%d", &id);
					snprintf(sendBuffer, sizeof(sendBuffer), "Le joueur %s est éliminé", gNames[id]);
					print_boxed_title(sendBuffer, RED);
					joueurElimine[id] = 1;
					for (int i = 0 ; i < 4 ; i ++)
					{
						if (joueurElimine[i] == 0)
							break;
						if (i == 3)
						{
							sprintf(sendBuffer, "Tous les joueurs sont éliminés. Fin de la partie.");
							print_boxed_title(sendBuffer, RED);
							quit = 1;
						}
					}			
					break;
				case 'T': // un joueur a gagné
					sscanf(gbuffer + 2,"%d", &id);
					snprintf(sendBuffer, sizeof(sendBuffer), "Le joueur %s a gagné la partie", gNames[id]);
					print_boxed_title(sendBuffer, GREEN);
					quit = 1;
					break;
				case 'F': // la partie est finie car un joueur a quitté la partie
					sscanf(gbuffer + 2,"%d", &id);
					snprintf(sendBuffer, sizeof(sendBuffer), "Le joueur %s a quitté la partie. Fin de la partie.", gNames[id]);
					print_boxed_title(sendBuffer, RED);
					quit = 1;
					break;
			}
			synchro = 0; // indique au thread serveur tcp que le message a été consommé
        }

        SDL_Rect dstrect_grille = { 512-250, 10, 500, 350 };
        SDL_Rect dstrect_image = { 0, 0, 500, 330 };
        SDL_Rect dstrect_image1 = { 0, 340, 250, 330/2 };

		SDL_SetRenderDrawColor(renderer, 255, 230, 230, 230);
		SDL_Rect rect = {0, 0, 1024, 768}; 
		SDL_RenderFillRect(renderer, &rect);

		if (joueurSel!=-1) // surligner le joueur selectionné
		{
			SDL_SetRenderDrawColor(renderer, 255, 180, 180, 255);
			SDL_Rect rect1 = {0, 90+joueurSel*60, 200 , 60}; 
			SDL_RenderFillRect(renderer, &rect1);
		}	

		if (objetSel!=-1) // surligner l'objet selectionné
		{
			SDL_SetRenderDrawColor(renderer, 180, 255, 180, 255);
			SDL_Rect rect1 = {200+objetSel*60, 0, 60 , 90}; 
			SDL_RenderFillRect(renderer, &rect1);
		}	

		if (guiltSel!=-1) // surligner la personne selectionnée
		{
			SDL_SetRenderDrawColor(renderer, 180, 180, 255, 255);
			SDL_Rect rect1 = {100, 350+guiltSel*30, 150 , 30}; 
			SDL_RenderFillRect(renderer, &rect1);
		}	

		{
			SDL_Rect dstrect_pipe = { 210, 10, 40, 40 };
			SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
			SDL_Rect dstrect_ampoule = { 270, 10, 40, 40 };
			SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
			SDL_Rect dstrect_poing = { 330, 10, 40, 40 };
			SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
			SDL_Rect dstrect_couronne = { 390, 10, 40, 40 };
			SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
			SDL_Rect dstrect_carnet = { 450, 10, 40, 40 };
			SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect_carnet);
			SDL_Rect dstrect_collier = { 510, 10, 40, 40 };
			SDL_RenderCopy(renderer, texture_objet[5], NULL, &dstrect_collier);
			SDL_Rect dstrect_oeil = { 570, 10, 40, 40 };
			SDL_RenderCopy(renderer, texture_objet[6], NULL, &dstrect_oeil);
			SDL_Rect dstrect_crane = { 630, 10, 40, 40 };
			SDL_RenderCopy(renderer, texture_objet[7], NULL, &dstrect_crane);
		}

        SDL_Color col1 = {0, 0, 0};
        for (i=0;i<8;i++) // afficher le nombre d'objets
        {
			SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, nbobjets[i], col1);
			SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

			SDL_Rect Message_rect; //create a rect
			Message_rect.x = 230+i*60;  //controls the rect's x coordinate 
			Message_rect.y = 50; // controls the rect's y coordinte
			Message_rect.w = surfaceMessage->w; // controls the width of the rect
			Message_rect.h = surfaceMessage->h; // controls the height of the rect

			SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
			SDL_DestroyTexture(Message);
			SDL_FreeSurface(surfaceMessage);
        }

        for (i=0;i<13;i++) // afficher les noms des cartes
        {
			SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, nbnoms[i], col1);
			SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

			SDL_Rect Message_rect;
			Message_rect.x = 105;
			Message_rect.y = 350+i*30;
			Message_rect.w = surfaceMessage->w;
			Message_rect.h = surfaceMessage->h;

			SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
			SDL_DestroyTexture(Message);
			SDL_FreeSurface(surfaceMessage);
        }

		for (i=0;i<4;i++)
		{
			for (j=0;j<8;j++)
			{
				if (tableCartes[i][j]!=-1)
				{
					char mess[10];
					if (tableCartes[i][j]==100)
					{
						sprintf(mess,"*");
					}
					else
					{
						sprintf(mess,"%d",tableCartes[i][j]);
					}
					SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, mess, col1);
					SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

					SDL_Rect Message_rect;
					Message_rect.x = 230+j*60;
					Message_rect.y = 110+i*60;
					Message_rect.w = surfaceMessage->w;
					Message_rect.h = surfaceMessage->h;

					SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
					SDL_DestroyTexture(Message);
					SDL_FreeSurface(surfaceMessage);
				}
			}
		}


		// Sebastian Moran
		{
			SDL_Rect dstrect_crane = { 0, 350, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[7], NULL, &dstrect_crane);
		}
		{
			SDL_Rect dstrect_poing = { 30, 350, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
		}
		// Irene Adler
		{
			SDL_Rect dstrect_crane = { 0, 380, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[7], NULL, &dstrect_crane);
		}
		{
			SDL_Rect dstrect_ampoule = { 30, 380, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
		}
		{
			SDL_Rect dstrect_collier = { 60, 380, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[5], NULL, &dstrect_collier);
		}
		// Inspector Lestrade
		{
			SDL_Rect dstrect_couronne = { 0, 410, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
		}
		{
			SDL_Rect dstrect_oeil = { 30, 410, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[6], NULL, &dstrect_oeil);
		}
		{
			SDL_Rect dstrect_carnet = { 60, 410, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect_carnet);
		}
		// Inspector Gregson 
		{
			SDL_Rect dstrect_couronne = { 0, 440, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
		}
		{
			SDL_Rect dstrect_poing = { 30, 440, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
		}
		{
			SDL_Rect dstrect_carnet = { 60, 440, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect_carnet);
		}
		// Inspector Baynes 
		{
			SDL_Rect dstrect_couronne = { 0, 470, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
		}
		{
			SDL_Rect dstrect_ampoule = { 30, 470, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
		}
		// Inspector Bradstreet
		{
			SDL_Rect dstrect_couronne = { 0, 500, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
		}
		{
			SDL_Rect dstrect_poing = { 30, 500, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
		}
		// Inspector Hopkins 
		{
			SDL_Rect dstrect_couronne = { 0, 530, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
		}
		{
			SDL_Rect dstrect_pipe = { 30, 530, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
		}
		{
			SDL_Rect dstrect_oeil = { 60, 530, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[6], NULL, &dstrect_oeil);
		}
		// Sherlock Holmes 
		{
			SDL_Rect dstrect_pipe = { 0, 560, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
		}
		{
			SDL_Rect dstrect_ampoule = { 30, 560, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
		}
		{
			SDL_Rect dstrect_poing = { 60, 560, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
		}
		// John Watson 
		{
			SDL_Rect dstrect_pipe = { 0, 590, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
		}
		{
			SDL_Rect dstrect_oeil = { 30, 590, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[6], NULL, &dstrect_oeil);
		}
		{
			SDL_Rect dstrect_poing = { 60, 590, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
		}
		// Mycroft Holmes
		{
			SDL_Rect dstrect_pipe = { 0, 620, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
		}
		{
			SDL_Rect dstrect_ampoule = { 30, 620, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
		}
		{
			SDL_Rect dstrect_carnet = { 60, 620, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect_carnet);
		}
		// Mrs. Hudson
		{
			SDL_Rect dstrect_pipe = { 0, 650, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
		}
		{
			SDL_Rect dstrect_collier = { 30, 650, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[5], NULL, &dstrect_collier);
		}
		// Mary Morstan
		{
			SDL_Rect dstrect_carnet = { 0, 680, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect_carnet);
		}
		{
			SDL_Rect dstrect_collier = { 30, 680, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[5], NULL, &dstrect_collier);
		}
		// James Moriarty
		{
			SDL_Rect dstrect_crane = { 0, 710, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[7], NULL, &dstrect_crane);
		}
		{
			SDL_Rect dstrect_ampoule = { 30, 710, 30, 30 };
			SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
		}

		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

		// Afficher les suppositions
		for (i=0;i<13;i++)
		{
			if (guiltGuess[i])
			{
				SDL_RenderDrawLine(renderer, 250,350+i*30,300,380+i*30);
				SDL_RenderDrawLine(renderer, 250,380+i*30,300,350+i*30);
			}
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderDrawLine(renderer, 0,30+60,680,30+60);
		SDL_RenderDrawLine(renderer, 0,30+120,680,30+120);
		SDL_RenderDrawLine(renderer, 0,30+180,680,30+180);
		SDL_RenderDrawLine(renderer, 0,30+240,680,30+240);
		SDL_RenderDrawLine(renderer, 0,30+300,680,30+300);

		SDL_RenderDrawLine(renderer, 200,0,200,330);
		SDL_RenderDrawLine(renderer, 260,0,260,330);
		SDL_RenderDrawLine(renderer, 320,0,320,330);
		SDL_RenderDrawLine(renderer, 380,0,380,330);
		SDL_RenderDrawLine(renderer, 440,0,440,330);
		SDL_RenderDrawLine(renderer, 500,0,500,330);
		SDL_RenderDrawLine(renderer, 560,0,560,330);
		SDL_RenderDrawLine(renderer, 620,0,620,330);
		SDL_RenderDrawLine(renderer, 680,0,680,330);

		for (i=0;i<14;i++)
		{
			SDL_RenderDrawLine(renderer, 0,350+i*30,300,350+i*30);
		}
		SDL_RenderDrawLine(renderer, 100,350,100,740);
		SDL_RenderDrawLine(renderer, 250,350,250,740);
		SDL_RenderDrawLine(renderer, 300,350,300,740);

		//SDL_RenderCopy(renderer, texture_grille, NULL, &dstrect_grille);
		if (b[0]!=-1)
		{
			SDL_Rect dstrect = { 750, 0, 1000/4, 660/4 };
			SDL_RenderCopy(renderer, texture_deck[b[0]], NULL, &dstrect);
		}
		if (b[1]!=-1)
		{
			SDL_Rect dstrect = { 750, 200, 1000/4, 660/4 };
			SDL_RenderCopy(renderer, texture_deck[b[1]], NULL, &dstrect);
		}
		if (b[2]!=-1)
		{
			SDL_Rect dstrect = { 750, 400, 1000/4, 660/4 };
			SDL_RenderCopy(renderer, texture_deck[b[2]], NULL, &dstrect);
		}

		// Le bouton go
		if (goEnabled==1)
		{
			SDL_Rect dstrect = { 500, 350, 200, 150 }; // {x, y, w, h}
			SDL_RenderCopy(renderer, texture_gobutton, NULL, &dstrect);
		}
		// Le bouton connect
		if (connectEnabled==1)
		{
			SDL_Rect dstrect = { 0, 0, 200, 50 };
			SDL_RenderCopy(renderer, texture_connectbutton, NULL, &dstrect);
		}

		//SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
		//SDL_RenderDrawLine(renderer, 0, 0, 200, 200);

		// Affichage des logos
		int w_tex, h_tex;
		// On récupère la taille réelle de la texture
		SDL_QueryTexture(texture_logo_su, NULL, NULL, &w_tex, &h_tex);

		// On définit la largeur voulue
		int largeur_voulue = 200;
		// On calcule la hauteur proportionnelle (on multiplie avant de diviser pour garder la précision)
		int hauteur_auto = (largeur_voulue * h_tex) / w_tex;

		SDL_Rect le_logo_su = { 350, 570, largeur_voulue, hauteur_auto };
		SDL_RenderCopy(renderer, texture_logo_su, NULL, &le_logo_su);
		// Même chose pour le logo PS
		SDL_QueryTexture(texture_logo_ps, NULL, NULL, &w_tex, &h_tex);
		int hauteur_auto_ps = (largeur_voulue * h_tex) / w_tex;
		SDL_Rect le_logo_ps = { 350, 600 + hauteur_auto, largeur_voulue, hauteur_auto_ps };
		SDL_RenderCopy(renderer, texture_logo_ps, NULL, &le_logo_ps);

		//affiche le joueur courant
		snprintf(sendBuffer, sizeof(sendBuffer), "Le joueur courant est %s", gNames[joueur_courant]);

		SDL_Color colCurrentPlayer = {0, 0, 255}; // Bleu
		SDL_Surface* surfaceMessage = TTF_RenderText_Solid(SansBig, sendBuffer, colCurrentPlayer);
		SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

		SDL_Rect Message_rect;
		Message_rect.x = 580;
		Message_rect.y = 600;
		Message_rect.w = surfaceMessage->w;
		Message_rect.h = surfaceMessage->h;

		SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
		SDL_DestroyTexture(Message);
		SDL_FreeSurface(surfaceMessage);

		//affiche Qui je suis
		snprintf(sendBuffer, sizeof(sendBuffer), "Je suis %s", gNames[gId]);

		surfaceMessage = TTF_RenderText_Solid(SansBig, sendBuffer, colCurrentPlayer);
		Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

		Message_rect.x = 580;
		Message_rect.y = 650;
		Message_rect.w = surfaceMessage->w;
		Message_rect.h = surfaceMessage->h;

		SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
		SDL_DestroyTexture(Message);
		SDL_FreeSurface(surfaceMessage);


		SDL_Color col = {0, 0, 0};
		for (i=0;i<4;i++)
		{
			if (strlen(gNames[i])>0)
			{
				SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, gNames[i], col);
				SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

				SDL_Rect Message_rect; //create a rect
				Message_rect.x = 10;  //controls the rect's x coordinate 
				Message_rect.y = 110+i*60; // controls the rect's y coordinte
				Message_rect.w = surfaceMessage->w; // controls the width of the rect
				Message_rect.h = surfaceMessage->h; // controls the height of the rect

				SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
				SDL_DestroyTexture(Message);
				SDL_FreeSurface(surfaceMessage);
			}
		}
		SDL_RenderPresent(renderer);
	}
    SDL_DestroyTexture(texture_deck[0]);
    SDL_DestroyTexture(texture_deck[1]);
    SDL_FreeSurface(deck[0]);
    SDL_FreeSurface(deck[1]);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
 
    SDL_Quit();
 
    return 0;
}
