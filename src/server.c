#include "../include/utils.h"
#include "../include/server.h"

char *default_path;


void server_error(int connfd, char *num, char *msg, char *cause, char *text)
{
    char buf[MAXLINE];

    sprintf(buf, "HTTP/1.1 %s %s\r\nContent-type: text/html\r\n",num, msg);
    
    sprintf(buf, "<html><title>Error</title>\r\n<body>%s: %s\r\n<hr/><p>%s: %s</p></body></html>\r\n",num, msg, text, cause);

    send(connfd,buf, strlen(buf), 0);
    return;
}

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


char global_path_name[MAXLINE];


void build1(string a, struct stat *st)
{
	char path[MAXLINE];
	strcpy(path, global_path_name);

	if(path[strlen(path)] != '/')
		strcat(path, "/");

	strcat(path, a.text);

	stat(path, st);
}

void build2(string a, string b, struct stat *stA, struct stat *stB)
{
	build1(a, stA);
	build1(b, stB);
}

int get_extension(string a, char *type)
{
	struct stat st;	
	build1(a, &st);

	if((st.st_mode & S_IFMT) == S_IFDIR)
	{
		strcpy(type, "dir");
		return 0;			
	}

	char nametmp[100];
	strcpy(nametmp, a.text);
	int len_name = strlen(nametmp);

	int pos = len_name - 1;
	while(nametmp[pos] != '.'){
		--pos;
		if(pos < 0)
			break;
	}

	if(pos < 0)
	{
		strcpy(type, "unknown");
		return 0;
	}

	int ind;
	for( ind = pos; ind < len_name; ++ind)
		type[ind-pos] = nametmp[ind];
	type[ind-pos] = 0;

	return 0;
}

int check_back(string a, string b)
{
	if(strcmp(a.text, ".") == 0)
		return -1;
	if(strcmp(b.text, ".") == 0)
		return 1;
	if(strcmp(a.text, "..") == 0)
		return -1;
	if(strcmp(b.text, "..") == 0)
		return 1;
	return 0;
}

int cmp_size (string a, string b)
{
	int chbk = check_back(a, b);
	if(chbk != 0)
		return chbk;

	struct stat stA, stB;
	build2(a, b, &stA, &stB);

	if((stA.st_mode & S_IFMT) == S_IFDIR && 
		(stB.st_mode & S_IFMT) == S_IFDIR )
		return strcmp(a.text, b.text) * Order;

	if((stA.st_mode & S_IFMT) == S_IFDIR)
		return -1;

	if((stB.st_mode & S_IFMT) == S_IFDIR)
		return 1;

    if ( stA.st_size == stB.st_size )
		return strcmp(a.text, b.text) * Order;

	return (stA.st_size - stB.st_size) * Order;
}


int cmp_type (string a, string b)
{	
	int chbk = check_back(a, b);
	if(chbk != 0)
		return chbk;

	char typeA[100], typeB[100];
	get_extension(a, typeA);
	get_extension(b, typeB);

    int val = strcmp(typeA, typeB);

    if(!val)
    	return strcmp(a.text, b.text) * Order;
    return val * Order;
}

int cmp_name (string a, string b)
{
	int chbk = check_back(a, b);
	if(chbk != 0)
		return chbk;

    return strcmp(a.text, b.text) * Order;
}

int cmp_date (string a, string b)
{
	int chbk = check_back(a, b);
	if(chbk != 0)
		return chbk;

	struct stat stA, stB;
	build2(a, b, &stA, &stB);

	double seconds = difftime(stA.st_mtime, stB.st_mtime);

    if(seconds < 0)
    	return -1 * Order;
    if(seconds > 0)
    	return 1 * Order;

    return strcmp(a.text, b.text) * Order;
}


void add_download(int connfd ,pool *p,char *path, long long size, int index)
{
	char array[100];
	sprintf(array, "HTTP/1.1 200 OK");
	sprintf(append(array), "Content-Length: %lld\n\n", size);
	printf("%d\n",1);
    FD_CLR(connfd, &p->read_set);

	p->off_set[index] = 0;
	p->ssize[index] = size;
	p->open_writer_fds[index] = open(path, O_RDONLY, 0);
	if(p->open_writer_fds[index] < 0)
	{
		server_error(connfd, "403", "forbidden", path, "Webserver could not read the file");
		p->ssize[index] = 0;
        p->off_set[index] = 0;
        close(p->clientfd[index]);
        close(p->open_writer_fds[index]);
        p->open_writer_fds[index] = -1;
        p->clientfd[index] = -1;
		return;
	}
	
	send(connfd, array, strlen(array), 0);

}

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