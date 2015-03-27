enum http_method {
HTTP_GET ,
HTTP_UNSUPPORTED ,
};

typedef struct
{
enum http_method method ;
int major_version ;
int minor_version ;
char * url ;
} http_request ;

struct mimext {
	const char * ext;
	const char * mime;
};


int creer_serveur (int port );
const char * get_ext(char * url);
void send_content(FILE * client, const char * mime);
const char * get_type(const char * ext);
void send_file(FILE * client, int fd);
void initialiser_signaux (void);
void traitement_signal (int sig );
int requete_valide (char *buff);
int ligne_vide (char *buff);
char * fgets_or_exit ( char * buffer , int size , FILE * stream);
int parse_http_request (const char * buff , http_request * request);
void send_status ( FILE * pointeur_fdopen ,  int code , const char * reason_phrase, http_request request);
void send_response ( FILE * client , int code , const char * reason_phrase ,const char * message_body, http_request request);
int check_and_open ( const char * url , const char * document_root );
char * rewrite_url ( char * url );
int get_file_size(int fd);
int copy(int in, int out);
void skip_headers (FILE * client) ;
