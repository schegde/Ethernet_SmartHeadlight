
#include <stdio.h>
#include <time.h>
#include "transfer.h"


//*********************************CUSTOMIZATION**********************************



//Port number used for connections, should be a number in string format.
#define PORT "6045" 

//Buffer size.
#define BUFFERSIZE 100 

//Buffer type
typedef int buffer_type;


#define SERVER_MODE 0
#define CLIENT_MODE 1

//Comment the below line to stop printing the received buffer on client side
#define BUFFER_PRINT

//Comment the below line to stop printing most of debug info
#define DEBUG_PRINT

//This can be set by a top level application to break the listening loop, and for a chance to change mode
int STOP_FLAG;  

int CHANGE_FLAG;

int MODE;


//********************************************************************************



/*
Signal handler for switching the roles between Server and client.
Currently configured to respond to  SIGTSTP Linux signal which gets generated from 
(Ctrl+Z) keyboard combination while program is running.
To respond to a signal sent by another process, send the appropriate signal to this program
And install this handler for that signal in main();

*/
void sig_handler(int sig_num){

    STOP_FLAG = 1;      //Interrupted by an external signal.
  
}

/*
 * 
 * main - binds to specified port and listens for new connections,
 * spawning threads to handle their requests
 * 
 */

int main(int argc, char **argv) {

    
    Signal(SIGPIPE, SIG_IGN);

    //Installing the signal handler for SIGTSSTP signal. Change to appropriate signal, if 
    //another process will interrupt this program flow.

    Signal(SIGTSTP, sig_handler); //Ctrl-Z for interruption!

    CHANGE_FLAG =1; //Specify the change flag for server transmission behaviour.

    operate_mode();
       
}


void operate_mode(){
    
    while(1){

        STOP_FLAG = 0; //Reset the flag, this is a new role start!

   
        printf("Select the mode you want to operate in? Hit 1 for client mode and 0 for server mode:");
        errno = scanf("%d",&MODE);
    

        
        if(MODE==SERVER_MODE){
            server_mode();
        }
        else{
            client_mode();
        
        }
    }   
 
}


void server_mode(){

    //Server Mode
    int errno;
    char port[10]=PORT;
    int listenfd, *connfd;    
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);


    #ifdef DEBUG_PRINT
        printf("Server mode selected!\n");
    #endif

    //*************************ALLOCATE BUFFERS and INITIALIZE THEM HERE*********************//
    //Allocate and initialize the buffers on server side !!!
    buffer_type * buffer;
    buffer = (buffer_type *)malloc(BUFFERSIZE*sizeof(buffer_type));

    for(int i=0;i<BUFFERSIZE;i++){
      buffer[i] = i;
      
    }
    #ifdef DEBUG_PRINT
        printf("The pointer to the buffer allocated is:%lld\n",(long long)buffer);
    #endif

    //**********************************************************************************

     
    listenfd = Open_listenfd(port);

   
    printf("Successfully started a listening server on  port %s, Now connect to this port from client side!\n",port);
        
  
    
    while (1) {
        if(STOP_FLAG)
          break;
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        int loc_connfd = *connfd;
        Free(connfd);
        send_file(loc_connfd);
        Close(loc_connfd);
        
    }
    
    free(buffer);
    
}

void client_mode(){
        int errno;
        char getName[MAXBUF];
        int num_bytes;
        char host[30];
        char port[10]=PORT;

        //Client mode
        #ifdef DEBUG_PRINT
            printf("Client mode selected!\n");
        #endif        
     
      printf("Enter the Host IP address to connect to:");
      errno = scanf("%s",host);
      // printf("Enter the port on the host to connect to:");
      // errno = scanf("%s",port); 

    while(1) {

               
        printf("\nEnter The buffer pointer to receive: ");
        errno = scanf("%s", getName); //susceptible to buffer overflow, but whatevs
        strcat(getName, "\n");
        printf("Enter num_bytes asked for ");
        errno = scanf("%d", &num_bytes);
        long long buf_pointer = atoi(getName);

        //Send the buffer pointer in long long format, and num_bytes in int:
        receive_file(&buf_pointer, num_bytes, host, port);

        if(STOP_FLAG)
          break;
        
    }

}



/*
 * sendit - sends preset file to client
 */
void send_file(int clientfd) {
    char buf[MAXBUF];
   
    rio_t clientRio;

    Rio_readinitb(&clientRio, clientfd);

    long long pointer;
    rio_readnb(&clientRio,&pointer,sizeof(long long));


    #ifdef DEBUG_PRINT
        printf("    Server: Client requested pointer %lld\n",pointer);
    #endif  
    

    int size_req;
    rio_readnb(&clientRio,&size_req,sizeof(int));

    
    strcpy(buf,"success\n");
    rio_writen(clientfd,buf,strlen(buf));

    buffer_type * send_buf_ptr = (buffer_type *)(pointer);

    rio_writen(clientfd,send_buf_ptr,size_req);

    #ifdef DEBUG_PRINT
        printf("    Server: Client received buffer\n");
    #endif  
        
}


void remove_newline_ch(char *line) {
    int new_line = strlen(line) -1;
    if (line[new_line] == '\n')
        line[new_line] = '\0';
}

void receive_file(long long *ptr, int num_bytes, char *host, char *port) {
    int remotefd; //, filefd, length;
    char buf[MAXBUF];
    rio_t remoteRio;

    #ifdef DEBUG_PRINT
        struct timeval start, finish, result;
        gettimeofday(&start, NULL);
    #endif

    remotefd = Open_clientfd(host, port);

    Rio_readinitb(&remoteRio, remotefd);

    //Request the buffer pointer.
    rio_writen(remotefd, ptr,sizeof(long long));

    rio_writen(remotefd, &num_bytes,sizeof(int));
    
    //then get back if the file was found or not
    Rio_readlineb(&remoteRio, buf, MAXLINE);

    #ifdef DEBUG_PRINT
        printf("Response: %s", buf);
    #endif  
    
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

    #ifdef DEBUG_PRINT
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
    #endif 
    

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