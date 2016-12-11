#include "../include/utils.h"

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


int main(int argc, char **argv)
{
	struct timeval ttime;
	ttime.tv_sec = 0;
	ttime.tv_usec = 50;

    int listenfd, connfd, port; 
    socklen_t clientlen = sizeof(struct sockaddr_in);
   
    struct sockaddr_in clientaddr;
    static pool pool;

	return 0;
}