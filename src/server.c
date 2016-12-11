#include "../include/utils.h"
#include "../include/server.h"

char *default_path;


void error_msg(const char* msg, int halt_flag) {
    perror(msg);
    if (halt_flag) exit(-1); 
}

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

void add_client(int connfd, pool *p) 
{
    int i;
    p->nready--;

    for (i = 0; i < FD_SETSIZE; i++)  
	{	
		if (p->clientfd[i] < 0) 
		{ 
		   
		    p->clientfd[i] = connfd;                 

		    
		    FD_SET(connfd, &p->read_set); 

		    
		    if (connfd > p->maxfd) 
				p->maxfd = connfd; 
		    
		    if (i > p->maxi)       
				p->maxi = i;      
		    
		    break;
		}
    }
    if (i == FD_SETSIZE) 
		unix_error("add_client error: Too many clients");
}


void download(pool *p,int i)
{
    size_t count;
    off_t remaining = p->ssize[i] - p->off_set[i];

    if (remaining > DOWNLOAD_BUFFER)
        count = DOWNLOAD_BUFFER;
    else
        count = remaining;

    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;

    int err = getpeername(p->clientfd[i], (SA *)&clientaddr, &clientlen);
    if(!err)
        sendfile(p->clientfd[i], p->open_writer_fds[i], &p->off_set[i], count);

    if(err || (p->off_set[i] >= p->ssize[i]))
    {
        printf("2\n");
        p->ssize[i] = 0;
        p->off_set[i] = 0;
        close(p->clientfd[i]);
        close(p->open_writer_fds[i]);
        p->open_writer_fds[i] = -1;
        p->clientfd[i] = -1;
    }
    return;
}


char query[100];
int Order = 1;
void translate(char uri[])
{
	int j=0;
	int sz = strlen(uri);

	for (int i = 0; i < sz; ++i)
	{
		if(uri[i]=='%' && uri[i+1] == '2' && uri[i+2] == '0')
		{
			uri[j]=((char)(uri[i+1]-'0')*16+(uri[i+2]-'0'));
			i+=2;
		}
		else if(uri[i] == '?')
		{
			Order *= -1;
			strcpy(query, uri+i+3);
			break;
		}
		else
		{
			uri[j]=uri[i];
		}
		j++;
	}
	uri[j]=0;
}


typedef struct{

	char *name;
	int size_byte;
	char *type;
	char *lst_mdf;
	int end;
	int dir;
	time_t date;

}cfile;

enum {NAME, SIZE, TYPE, DATE};



void response(char request[], int connfd, pool *p, int index)
{
	struct stat sb;

    char  uri[MAXLINE];
	sscanf(request, "%s %s", uri, uri);
	translate(uri);
				
	char tmp_path[1024];
	strcpy(tmp_path, default_path);
	strcat(tmp_path, uri);

	if(stat(tmp_path, &sb) == -1){

		server_error(connfd, "404", "not found", tmp_path, "Webserver could not find this path");
		close(connfd);
		FD_CLR(connfd, &p->read_set); 
		p->clientfd[index] = -1;          
		return;
	}

	int cmp_cmd = NAME;
	if(strstr(query, "type"))				cmp_cmd = TYPE;
	else if(strstr(query, "size"))			cmp_cmd = SIZE;
	else if(strstr(query, "lastmodified"))	cmp_cmd = DATE;
 	
	switch (sb.st_mode & S_IFMT)
	{
        case S_IFDIR:
            html_content(uri, connfd, cmp_cmd, Order);
            close(connfd);
			FD_CLR(connfd, &p->read_set); 
			p->clientfd[index] = -1;          
		    break;

        case S_IFREG:     
        	add_download(connfd, p, tmp_path, (ull)sb.st_size, index);
       	 	break;

       	 default:
       	 	break;
    }
}


void check_clients(pool *p) 
{
	 
    int i, connfd, n;
	char buf[MAXLINE];
   	char *response_buf;

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) 
    {
		
		connfd = p->clientfd[i];
		if ((connfd > 0) && (FD_ISSET(connfd, &p->read_ready_set))) 
		{ 
			p->nready--;
		
		    int bytes_read = recv(connfd, buf, sizeof(buf), 0); 
			if (bytes_read < 0)
				error_msg("Problem with recv call", 0);
			response(buf, connfd, p, i);
			break;
		}
	}

	for (int i = 0; i<= p->maxi; ++i)
		if(p->open_writer_fds[i] != -1)
			download(p,i);

	return;
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

    while (1) 
    {
		
		pool.read_ready_set = pool.read_set;
		pool.nready = Select(pool.maxfd+1, &pool.read_ready_set, NULL, NULL, &ttime);
		
		if (FD_ISSET(listenfd, &pool.read_ready_set))  
		{
			connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
		    add_client(connfd, &pool); 
		}
		
		check_clients(&pool);
    }

	return 0;
}