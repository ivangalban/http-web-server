#include "../include/utils.h"
#include "../include/server.h"

char *default_path;



void init_pool(int listenfd, pool *p)
{
  
    int i;
    p->maxi = -1;                  
    for (i = 0; i < FD_SETSIZE; i++)  
	{
		p->off_set[i] = 0;
		p->ssize[i] = 0;
		p->open_writer_fds[i] = -1;
		p->clientfd[i] = -1;        
    
	    p->maxfd = listenfd;            
   
	    FD_ZERO(&p->read_set);
	    FD_SET(listenfd, &p->read_set); 
	}
}


int main(int argc, char **argv)
{
	struct timeval ttime;
	ttime.tv_sec = 0;
	ttime.tv_usec = 50;

    int listenfd, connfd, port; 
    socklen_t clientlen = sizeof(struct sockaddr_in);
   
    struct sockaddr_in clientaddr;
    static pool pool;

    port = atoi(argv[1]);
    default_path = argv[2];

    if ((argc != 3) || (opendir(default_path) == NULL) || (port < MINPORT) || (port > MAXPORT))
    {
		fprintf(stderr, "mounting error\n");
		exit(0);
    }

    listenfd = open_listenfd(port);

    if(listenfd < 0)
    {
    	fprintf(stderr, "mounting error\n");
		exit(0);
    }

    init_pool(listenfd, &pool); 

	return 0;
}