#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <time.h>

#define LISTENQ  1024  /* second argument to listen() */

typedef struct sockaddr SA;
typedef unsigned long long ull;

/* $end open_client_read_fd */

/*  
 * open_listenfd - open and return a listening socket on port
 *     Returns -1 and sets errno on Unix error.
 */
/* $begin open_listenfd */
int open_listenfd(int port){
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;
 
    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
		   (const void *)&optval , sizeof(int)) < 0)
	return -1;

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    
    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
		return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
		return -1;
    return listenfd;
}
/* $end open_listenfd */

void unix_error(char *msg){/* unix-style error */
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

int Select(int  n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout){
    int rc;
    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
	   unix_error("Select error");
    return rc;
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen){
    int rc;
    if ((rc = accept(s, addr, addrlen)) < 0)
		unix_error("Accept error");
    return rc;
}