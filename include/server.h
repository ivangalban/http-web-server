
#define MINPORT 0
#define MAXPORT 65535
#define	MAXLINE	 8192  /* max text line length */
#define DOWNLOAD_BUFFER 8192

typedef struct 
{ 
    int maxfd;        

    fd_set read_set;  
    fd_set read_ready_set; 

    int nready;        
    int maxi;         

    int clientfd[FD_SETSIZE];    
   
   	ull ssize[FD_SETSIZE];
    off_t off_set[FD_SETSIZE];
   	int open_writer_fds[FD_SETSIZE];
} pool;



void init_pool(int listenfd, pool *p);
void add_client(int, pool *);
void check_clients(pool *);
void download(pool *, int);
void error_msg(const char *, int);
void response(char [], int, pool *, int);