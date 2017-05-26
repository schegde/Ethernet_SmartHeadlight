
#include <stdio.h>
#include <time.h>
#include "transfer.h"


//*********************************CUSTOMIZATION**********************************
//Buffer size.
#define BUFFERSIZE 100 

//Buffer type
typedef int buffer_type;


#define SERVER_MODE 0
#define CLIENT_MODE 1

int MODE;

#define BUFFER_PRINT

//This can be set by a top level application to break the listening loop, and for a chance to change mode
int STOP_FLAG = 0;  
//********************************************************************************




// void sendit(int fd);
// void clienterror(int fd, char *cause, char *errnum, 
//      char *shortmsg, char *longmsg);
// void send_func(void *vargp);

// void receive_file(char *getName, int num_bytes, char *host, char *port);
// void listen_func(void *vargp);
// void remove_newline_ch(char *line);
// //int Open_w(const char *pathname, int flags, mode_t mode);
// int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);




/*
 * 
 * main - binds to specified port and listens for new connections,
 * spawning threads to handle their requests
 * 
 */
int main(int argc, char **argv) {

   // STOP_FLAG = 0;
   
    int errno;
    char getName[MAXBUF];// copyName[MAXBUF]; 
    int num_bytes;
    
    /* simply ignore sigpipe signals, the most elegant solution */
    Signal(SIGPIPE, SIG_IGN);

    while(1){

    int MODE;
    printf("Select the mode you want to operate in? Hit 1 for client mode and 0 for server mode:");
    errno = scanf("%d",&MODE);
    

    //Declare and initialize the buffer.
    if(MODE==SERVER_MODE){
    char port[10];
    printf("Server mode selected!\n");
    buffer_type * buffer = (buffer_type *)malloc(BUFFERSIZE*sizeof(buffer_type));

    for(int i=0;i<BUFFERSIZE;i++){
      buffer[i] = i;
      
    }

    printf("The pointer to the buffer allocated is:%lld\n",(long long)buffer);
    printf("Enter the listening port for the server:");
    errno = scanf("%s",port);
    listen_func(port);
    free(buffer);
    }

    else{
      printf("Client mode selected!\n");
      char host[30], port[10];
      printf("Enter the Host IP address to connect to:");
      errno = scanf("%s",host);
      printf("Enter the port on the host to connect to:");
      errno = scanf("%s",port); 

    while(1) {

        if(STOP_FLAG)
          break;
        
        printf("\nEnter The buffer pointer to receive: ");
        errno = scanf("%s", getName); //susceptible to buffer overflow, but whatevs
        strcat(getName, "\n");
        printf("Enter num_bytes asked for ");
        errno = scanf("%d", &num_bytes);
        receive_file(getName, num_bytes, host, port);
        
    }

    }
    }   
 
     
}

void mode_selector(int mode_loc){

  MODE = mode_loc;

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
        if(STOP_FLAG)
          break;
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
    //int length, filefd;
    rio_t clientRio;

    //int int_buf[ASK_SIZE];

    Rio_readinitb(&clientRio, clientfd);

    //first, get the Buffer pointer we want. 

    rio_readlineb(&clientRio, buf, MAXBUF);
    remove_newline_ch(buf);

    long long pointer = atoi(buf);

    printf("    Server: Client requested pointer %lld\n",pointer);
    //  printf("Dereferencing this gives:%d\n",*((int *)pointer));


    int size_req;
    rio_readnb(&clientRio,&size_req,sizeof(int));


    // if((filefd = Open_w(buf, O_RDONLY, DEF_MODE)) < 0) { //error opening file
    //     strcpy(buf,"error\n");
    //     printf("%s\n", buf);
    //     rio_writen(clientfd, buf, strlen(buf));
    //     return;
    // };
    
    strcpy(buf,"success\n");
    rio_writen(clientfd,buf,strlen(buf));


    // for(int i = 0;i<size_req;i++){
    //    int_buf[i] = *(((int *)(pointer))+i);
    // }
    buffer_type * send_buf_ptr = (buffer_type *)(pointer);

    rio_writen(clientfd,send_buf_ptr,size_req);

    
    // while ((length = rio_readn(filefd, buf, MAXBUF)) != 0) {
    //     rio_writen(clientfd, buf, length);
    // }

    //finally close open file descriptor
    //Close(filefd);
    printf("    Server: Client received buffer\n");
}


void remove_newline_ch(char *line) {
    int new_line = strlen(line) -1;
    if (line[new_line] == '\n')
        line[new_line] = '\0';
}

void receive_file(char *getName, int num_bytes, char *host, char *port) {
    int remotefd; //, filefd, length;
    char buf[MAXBUF];
    rio_t remoteRio;
    struct timeval start, finish, result;
    gettimeofday(&start, NULL);

    remotefd = Open_clientfd(host, port);

    Rio_readinitb(&remoteRio, remotefd);

    //Request the buffer pointer.
    rio_writen(remotefd, getName, strlen(getName));

    rio_writen(remotefd, &num_bytes,sizeof(int));
    
    //then get back if the file was found or not
    Rio_readlineb(&remoteRio, buf, MAXLINE);
    printf("Response: %s", buf);

    if(!strcmp(buf,"error\n")) {//file not found
        printf("Client: Error getting file from server\n");
        Close(remotefd);
        return;
    }
    

    buffer_type int_buffer_2[num_bytes/sizeof(buffer_type)];
    rio_readnb(&remoteRio,int_buffer_2,num_bytes);


//***************************************************PRINT THE BUFFER****************************************
    #ifdef BUFFER_PRINT
    for(int i=0;i<(num_bytes/sizeof(buffer_type));i++){
        
            printf("%d\n",int_buffer_2[i]);
    }
    #endif
//***********************************************************************************************************

    
    Close(remotefd);

    //print some stats about our transfer
    gettimeofday(&finish, NULL);
    timeval_subtract(&result, &finish, &start);
    time_t elapsedSeconds = result.tv_sec;
    long int elapsedMicros = result.tv_usec;
    double elapsed = elapsedSeconds + ((double) elapsedMicros)/1000000.0;

    double fileSize = num_bytes*sizeof(int);
    double speed = fileSize / elapsed;


    printf("Elapsed: %lf seconds\n", elapsed);
    printf("Filesize: %0lf Bytes, %3lf kiloBytes, %lf megaBytes\n", fileSize, fileSize/1000, fileSize/1000000);
    printf("Speed: %0lf B/s, %3lf kB/s, %lf mB/s\n", speed, speed/1000, speed/1000000);

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