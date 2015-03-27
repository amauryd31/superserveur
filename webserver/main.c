# include <stdio.h>
# include <string.h>
#include  <sys/types.h>  
#include  <sys/socket.h>
#include  <arpa/inet.h>
#include  <unistd.h>
#include  "socket.h"
#include  <sys/types.h>
#include  <sys/wait.h>
#include  <unistd.h>
#include  <stdlib.h>
#include  <sys/stat.h>
#include  <fcntl.h>


int http_version = 2;
const char * document_root = "/home/infoetu/dumetza/public_html" ;
int socket_client = 0;

int main () {

	int socket_serveur =  creer_serveur(8080);
	if (socket_serveur == -1) {
		return -1;
	}
	
	initialiser_signaux();
	while(1){

	        socket_client = accept (socket_serveur , NULL, NULL);
		if (socket_client == -1) {
			perror ("accept");
			return -1;
		}

		FILE * client = fdopen (socket_client, "w+");
		if(client == NULL) {
			perror("fdopen \n");
			exit(1);
		}

	
		 char * buff = malloc(8000);
		 int f = fork();
		 if (f == -1) {
       			 perror("fork");
  	 	 } else if (f== 0) {
       		   	 sleep(1);
  		   	 while (1) {
				fgets_or_exit(buff,256, client);
		   	}
         	 }

		  else {
			 close(socket_client);
		  }
		
	}
	exit(0);
	return 0;
}


void initialiser_signaux (void) {

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		perror (" signal ");
	}
	struct sigaction sa ;
	sa.sa_handler = traitement_signal ;
	sigemptyset (& sa . sa_mask);
	sa.sa_flags = SA_RESTART ;
	if (sigaction (SIGCHLD , &sa , NULL) == -1) {
		perror ("sigaction(SIGCHLD)");
	}
}

void traitement_signal (int sig) {
	printf ("Signal %d recu \n", sig);
	int status;
	while (waitpid(-1, &status, WNOHANG) >0)
	{
		if (WIFSIGNALED(status))
		{
			if (WTERMSIG(status) == SIGSEGV)
			{
				printf("Erreur de segmentation !\n");
			}
		}
	}
}



	int ligne_vide(char*buff){
		if (strcmp(buff,"\r\n") ==0)
			return 1;
		else if (strcmp(buff,"\n") ==0)
			return 1;
		return 0;
	}

	// Partie 6.1
	char * fgets_or_exit (char * buff , int size , FILE * client) {
		const char * message_bienvenue = "Bonjour , bienvenue sur mon serveur \n Je suis ligne 1 \n je suis ligne 2 \n et moi ligne 3 \n Je suis ligne 4\n" ;
		http_request request;
		if (fgets(buff,size, client) != NULL) {
			
			
			skip_headers (client) ;
			if (parse_http_request (buff ,&request) == 0) {
				send_response ( client , 400 , " Bad Request " , " Bad request \r \n ", request);
			}

			else if (request.method == HTTP_UNSUPPORTED) {
				send_response ( client , 405 , " Method Not Allowed " , " Method Not Allowed \r \n ", request);
			}
			else{ 

				int des = check_and_open (request.url , document_root);
				if(des == -1){
				  	send_response ( client , 404 , " Not Found " , " Not Found \r \n ", request);
				}
				else{

				  send_response ( client , 200 , " OK " , message_bienvenue, request);
				  send_content(client, get_type(get_ext(request.url)));
				  send_file(client, des);
				  
				}
				
				
			}
		}	

		else {
			printf("Connexion terminee \n");
			exit(0);
		}

	return buff;
	}

// Partie 6.2 : permet d'analyser la 1ere ligne de la requete
int parse_http_request (const char * buff , http_request * request) {
	
	int nb_espace = 0;
	int tmp =0;
	int indice_trois = 0;
	int indice_deux = 0;
	char * mot_trois = malloc(8);
	char *requete_url = malloc(32);
	
	while (buff[tmp] != '\0') {

		if (nb_espace ==2) {
			mot_trois[indice_trois] = buff[tmp];
			indice_trois ++;
		}
	
		
		if (nb_espace ==1) {
			if(buff[tmp]!=0){
			requete_url[indice_deux] = buff[tmp];
			}
			indice_deux ++;
		}
		
		if (buff[tmp] == ' ') {
			nb_espace ++;
		}

	tmp ++;

	}
		
	// si GET
	if (buff[0] == 'G' && buff[1] == 'E' && buff [2] == 'T' && nb_espace ==2) {
		request->method=HTTP_GET;
	}
	// SI post, put, delete ou autre
	else  {
		printf("Methode = %s \n", buff);
		request->method=HTTP_UNSUPPORTED;
		request->major_version=mot_trois[5] - '0';
		request->minor_version=mot_trois[7] - '0';
		return 1;
	}

	request->major_version=mot_trois[5] - '0';
	request->minor_version=mot_trois[7] - '0';
	

	requete_url[strlen(requete_url)-1] = '\0' ;
	if (strcmp(requete_url, "/")==0) {
		requete_url = "/index.html ";
		printf("CORRIGE \n");
	}
	request->url=requete_url;
	printf("url = %s \n", request->url);

	return 1;

}

// PARTIE 6.3 : Analyse des entetes
void skip_headers (FILE * client) {
	char * buff =  malloc(8000);
	while (strcmp(buff,"\r\n") !=0 && strcmp(buff,"\n") !=0) {
		fgets(buff,256, client);
	}
}

// PARTIE 6.4 : Reponse au client

void send_status ( FILE * client ,  int code , const char * reason_phrase, http_request request) {
	fprintf(client, "%s %d %d %s %s \n"," HTTP/1.",request.minor_version,  code, " : ", reason_phrase);
	fflush(client);
}

void send_response ( FILE * client , int code , const char * reason_phrase ,const char * message_body, http_request request){
	fprintf(client, "%s %d %d %s %s %s \n"," HTTP/1.",request.minor_version,  code, " : ", reason_phrase, message_body);
	fflush(client);
}

// PARTIE 7.1 : Ouverture du fichier souhaite

char * rewrite_url (char * url) {

	char  * res = "";
	int tmp = 0;
	while(url[tmp] != '\0' && tmp != -1){
		if(url[tmp] != '?'){
			res[tmp] = url[tmp];	
		}
		
		else {
			tmp = -1;
		}
	tmp ++;	

	}

	return res;
}


int check_and_open (const char * url , const char * document_root) {

	int fd;
	char *path;
	struct stat *s;
	s = malloc(sizeof(struct stat));
	int status;
	path = (char*)malloc(strlen(url)+strlen(document_root)+1);
	
	strcpy(path, document_root);
	strcat(path, url);
	
	//path[strlen(path)-1] = '\0' ;
	printf("av st path = %s \n", path); 
	status = stat(path, s);
	
	if (status == -1) {
		
		printf("a1 \n");
		printf("path = \"%s\"  \n", path);  
		perror("stat");
		return -1;
	}
	if (S_ISDIR (s->st_mode)) printf("dossier.");
	if (S_ISREG (s->st_mode)) printf("fichier. ");

	
	fd = open(path, O_RDONLY); 
	
	if (fd < 0)
	{
		printf("b1 \n");
		printf("path = \"%s\"  \n", path); 
		perror("open");
		return -1;
	}
	
	return fd;
}


// PARTIE 7.2 : Transmission du fichier

int get_file_size(int fd) {
  struct stat buf;
  fstat(fd, &buf);
  return buf.st_size;
}


int copy(int in, int out) {


  int nb_lus, nb_ecrits;
              char tampon[512];
	      if (in == -1) {
	       perror("Fichier nul");
	       return -1;
	      }
              if (out == -1) {
		perror("Fichier nul");
		return -1;
	      }
              while (nb_lus >0) {
                      nb_lus = read(in, tampon, 512);
                      if (nb_lus <= 0)
                              break;
                      nb_ecrits = write(out, tampon, nb_lus);
                      if (nb_ecrits != nb_lus)
                              perror("Probleme d'ecriture");
             };
	      return 0;

}


struct mimext typesmime[] = {
	{ ".jpg", "image/jpeg" },
	{ ".html", "text/html" },
	{ NULL, NULL }
};


const char * get_ext(char * url)
{
	return strrchr(url, '.');
}

const char * get_type(const char * ext)
{
	int i;
	for (i = 0; typesmime[i].ext != NULL; i++)
	{
		if (strcmp(ext, typesmime[i].ext) == 0)
			return typesmime[i].mime;
	}
	return "text/plain";
}

void send_content(FILE * client, const char * type)
{
	fprintf(client, "Content-Type: %s\r\n", type);
	fflush(client);
}

void send_file(FILE * client, int fd) {
	fprintf(client, "Content-Length: %d\r\n\r\n", get_file_size(fd));
	fflush(client);
	copy(fd, fileno(client));
}
