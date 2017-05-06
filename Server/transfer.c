
#include <stdio.h>
#include <time.h>
#include "csapp.h"


void sendit(int fd);
void send_func(void *vargp);
void listen_func(void *vargp);

void remove_newline_ch(char *line);
int Open_w(const char *pathname, int flags, mode_t mode);
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);



/*
 * 
 * main - binds to specified port and listens for new connections,
 * spawning threads to handle their requests
 * 
 */
int main(int argc, char **argv) {
   
    int errno;
   // char getName[MAXBUF], copyName[MAXBUF]; //

    /* Check command line args */
    if (argc != 2) {
		fprintf(stderr, "usage: %s <server listening port>\n", argv[0]);
		exit(1);
    }

    /* simply ignore sigpipe signals, the most elegant solution */
    Signal(SIGPIPE, SIG_IGN);

    //Start a listening port so that client can connect to receive files
    
    listen_func(argv[1]);
     
}

void listen_func(void *vargp){
    int listenfd, *connfd;
    
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);

    char *port = vargp;
    
    
    //bind to port
    listenfd = Open_listenfd(port);

    printf("Successfully started a listening server on  port %s, Now connect to this port from client side!\n",port);
    
    while (1) {
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        send_func(connfd);
    }
}


/*
 * 
 * transfer_thread - transfers requested file to client and closes when done
 * 
 */
void send_func(void *vargp) {
    int connfd = *((int *)vargp);
   
    Free(vargp);
    sendit(connfd);
    Close(connfd);
}

/*
 * sendit - sends preset file to client
 */
void sendit(int clientfd) {
    char buf[MAXBUF];
    int length, filefd;
    rio_t clientRio;

    Rio_readinitb(&clientRio, clientfd);

    //first, get the name of the file we want

    rio_readlineb(&clientRio, buf, MAXBUF);
    remove_newline_ch(buf);
    printf("    Server: Client requested %s\n",buf);

    if((filefd = Open_w(buf, O_RDONLY, DEF_MODE)) < 0) { //error opening file
        strcpy(buf,"error\n");
        printf("%s\n", buf);
        rio_writen(clientfd, buf, strlen(buf));
        return;
    };
    strcpy(buf,"success\n");
    rio_writen(clientfd,buf,strlen(buf));
    
    while ((length = rio_readn(filefd, buf, MAXBUF)) != 0) {
        rio_writen(clientfd, buf, length);
    }

    //finally close open file descriptor
    Close(filefd);
    printf("    Server: Client received file\n");
}


void remove_newline_ch(char *line) {
    int new_line = strlen(line) -1;
    if (line[new_line] == '\n')
        line[new_line] = '\0';
}



/**********************************
 * Wrappers for Open rewritten
 **********************************/

 int Open_w(const char *pathname, int flags, mode_t mode) 
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
        fprintf(stderr, "Open error: %s\n", strerror(errno));
    return rc;
}



/* Subtract the ‘struct timeval’ values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0. 
   Credit goes to https://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html */

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y) {
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}
