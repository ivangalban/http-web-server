#include "../src/utils.c"


void unix_error(char *);
int open_listenfd(int);
int Select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
int Accept(int, struct sockaddr *, socklen_t *);