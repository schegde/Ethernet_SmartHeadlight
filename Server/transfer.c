/*
 * transfer.c
 * 
 * Name: Alec Foster 
 * andrew id: ajfoster
 *
 * Basic server/client for sending images or other binary data over ethernet
 * 
 */

#include <stdio.h>
#include <time.h>
#include "csapp.h"


void sendit(int fd);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void *send_thread(void *vargp);
void *receive_thread(void *vargp);
void *listen_thread(void *vargp);
void receive_file(char *getName, char *copyName, char *host, char *port);
void remove_newline_ch(char *line);
int Open_w(const char *pathname, int flags, mode_t mode);
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);
static unsigned get_file_size (const char * file_name);


/*
 * 
 * main - binds to specified port and listens for new connections,
 * spawning threads to handle their requests
 * 
 */
int main(int argc, char **argv) {
    pthread_t tid;
    int errno;
    char getName[MAXBUF], copyName[MAXBUF]; //

    /* Check command line args */
    if (argc != 4) {
		fprintf(stderr, "usage: %s <host> <receive port> <send port>\n", argv[0]);
		exit(1);
    }

    /* simply ignore sigpipe signals, the most elegant solution */
    Signal(SIGPIPE, SIG_IGN);

    //spawn a listener thread so that clients can connect to receive files
    
    Pthread_create(&tid, NULL, listen_thread, argv[3]);

    //now do client stuff below
    
    while(1) {
        printf("\nEnter a file to receive: ");
        errno = scanf("%s", getName); //susceptible to buffer overflow, but whatevs
        strcat(getName, "\n");
        //printf("You Entered: %s", getName);
        printf("Enter a new file name: ");
        errno = scanf("%s", copyName);
        //printf("You Entered: %s\n\n", copyName);
        receive_file(getName, copyName, argv[1], argv[2]);
    }
    
}

void *listen_thread(void *vargp){
    int listenfd, *connfd;
    pthread_t tid;
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);

    char *port = vargp;
    
    Pthread_detach(pthread_self());

    //bind to port
    listenfd = Open_listenfd(port);
    while (1) {
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Pthread_create(&tid, NULL, send_thread, connfd);
    }
}

void receive_file(char *getName, char *copyName, char *host, char *port) {
    int remotefd, filefd, length;
    char buf[MAXBUF];
    rio_t remoteRio;
    struct timeval start, finish, result;
    gettimeofday(&start, NULL);

    remotefd = Open_clientfd(host, port);
    Rio_readinitb(&remoteRio, remotefd);

    //first write our request to the server
    rio_writen(remotefd, getName, strlen(getName));

    
    //then get back if the file was found or not
    Rio_readlineb(&remoteRio, buf, MAXLINE);
    printf("Response: %s", buf);
    if(!strcmp(buf,"error\n")) {//file not found
        printf("Client: Error getting file from server\n");
        Close(remotefd);
        return;
    }
    

    //now actually do some copying
    filefd = Open(copyName, O_WRONLY|O_CREAT|O_TRUNC, DEF_MODE);

    while ((length = rio_readnb(&remoteRio, buf, MAXBUF)) != 0) {
        rio_writen(filefd, buf, length);
    }
    Close(filefd);
    Close(remotefd);

    //print some stats about our transfer
    gettimeofday(&finish, NULL);
    timeval_subtract(&result, &finish, &start);
    time_t elapsedSeconds = result.tv_sec;
    long int elapsedMicros = result.tv_usec;
    double elapsed = elapsedSeconds + ((double) elapsedMicros)/1000000.0;

    double fileSize = (double) get_file_size(copyName);
    double speed = fileSize / elapsed;


    printf("Elapsed: %lf seconds\n", elapsed);
    printf("Filesize: %0lf Bytes, %3lf kiloBytes, %lf megaBytes\n", fileSize, fileSize/1000, fileSize/1000000);
    printf("Speed: %0lf B/s, %3lf kB/s, %lf mB/s\n", speed, speed/1000, speed/1000000);

}

/*
 * 
 * transfer_thread - transfers requested file to client and closes when done
 * 
 */
void *send_thread(void *vargp) {
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    sendit(connfd);
    Close(connfd);
    return NULL;
}

void *receive_thread(void *vargp) {
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    //receiveit(connfd);
    Close(connfd);
    return NULL;
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


/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, 
         char *shortmsg, char *longmsg) {
    char buf[MAXBUF], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Ratchet Proxy</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
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


/***********************************
* Other People's Code *
************************************/


/* This routine returns the size of the file it is called with. */

static unsigned get_file_size (const char *file_name) {
    struct stat sb;
    if (stat (file_name, & sb) != 0) {
        fprintf (stderr, "'stat' failed for '%s': %s.\n",
                 file_name, strerror (errno));
        exit (EXIT_FAILURE);
    }
    return sb.st_size;
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