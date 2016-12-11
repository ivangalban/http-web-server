

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