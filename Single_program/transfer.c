
#include <stdio.h>
#include <time.h>
#include "transfer.h"
#include <pthread.h>


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

int DISCONTINUE_FLAG;

int MODE;

#define ACK 5 //random numbers assigned for ACK and NACK and STOP_MSG
#define NACK 6
#define STOP_MSG 7




//***************************************************NOTES*************************************
// 1)   START PROCEDURE:START THE SERVER FIRST, AND THEN THE CLIENT. (This order is VERY necessary)
// 2)   STOP PROCEDURE: SEND a configured linux SIGNAL(currently configured for ctrl+z keyboard combination) 
//                      TO THIS PROGRAM ON *SERVER*; SERVER THEN SENDS A MESSAGE TO CLIENT TO 
//                      stop itself, and reverse role if needed.
// 3)   Minimum one transaction is needed to stop the respective roles and allow for role reversal :( 
//      [This is a disadvantage of a blocking network socket, but non-blocking sockets are more tough to manage]
// 4)   Every transaction implements a ACK/NACK/STOP_MSG handshaking between server and client for synchronization.
// 5)   There will be timing restrictions on The control signals(STOP/CHANGE/DISCONTINUE FLAG) are given to this program.
//      These signals can be given from a new thread in this program, to keep it asynced with the flow control of 
//      main thread of this program.
// 6)  ONCE DISCONTINUE SIGNAL IS ISSUED ON CLIENT SIDE, PROGRAM RESPONDS TO IT ONLY AFTER SERVER SENDS A BUFFER TO IT.
//     I/E THE CHANGE FLAG MUST BE CHANGED ONCE AFTER DISCONTINUE SIGNAL IS ISSUED ON CLIENT SIDE! again a result of 
//     blocking implementation of sockets and buffered reads on the socket. A read on the socket is blocking until 
//     specified number of bytes are read out into user space.
// 7)  To gracefully close this system in the current state : Hit Ctrl+Z on the server to quit the roles on server and 
//      client side, and then hit Ctrl+C on both devices respectively to close the programs (Quitting the roles de-registers
//      the signal handlers to use Ctrl+C normally.)
//*********************************************************************************************


/*
Signal handler for switching the roles between Server and client.
Currently configured to respond to  SIGTSTP Linux signal which gets generated from 
(Ctrl+Z) keyboard combination while program is running.
To respond to a signal sent by another process, send the appropriate signal to this program
And install this handler for that signal in main();

*/
void sig_handler(int sig_num){

    STOP_FLAG = 1;      //Interrupted by an external signal.
    DISCONTINUE_FLAG = 1;
  
  //File descriptor cleanups and malloced cleanups! connection file desctiptors in client and server
}

void sig_handler2(int sig_num){

    CHANGE_FLAG = (~(CHANGE_FLAG))&(1);
}

/*
 * 
 * main - binds to specified port and listens for new connections,
 * spawning threads to handle their requests
 * 
 */

// void control_thread(){




// }

int main(int argc, char **argv) {

    // pthread_t tid;
    // pthread_create(&tid,NULL,control_thread,NULL);
    // pthread_detach(tid); 

    operate_mode();

}


void operate_mode(){


    Signal(SIGPIPE, SIG_IGN);

    //Installing the signal handler for SIGTSSTP signal. Change to appropriate signal, if 
    //another process will interrupt this program flow.

    Signal(SIGTSTP, sig_handler); //Ctrl-Z for interruption!
   

    
    
    while(1){


        STOP_FLAG = 0; //Reset the flag, this is a new role start!

   
        printf("Select the mode you want to operate in? Hit 1 for client mode and 0 for server mode:");
        errno = scanf("%d",&MODE);
    

        
        if(MODE==SERVER_MODE){

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

            //******************************************************************************************

            server_mode(); //Call the server routine!

            free(buffer); 
            //CAUTION!!!!
            //This program frees the allocated buffer to avoid memory leaks! Remove this if the buffer needs
            //to be used later .
            
        }
        else{
            client_mode();        
        }
    }   
 
}


void server_mode(){

    Signal(SIGINT, sig_handler2);

    CHANGE_FLAG =0; //Specify the change flag for server transmission behaviour.

    //Server Mode
    int errno;
    char port[10]=PORT;
    int listenfd;    
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);


    #ifdef DEBUG_PRINT
        printf("Server mode selected!\n");
    #endif


     
    listenfd = Open_listenfd(port);

   
    printf("Successfully started a listening server on  port %s, Now connect to this port from client side!\n",port);
        
    
    int loc_connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        
    send_file(loc_connfd);
    Close(loc_connfd);
    Close(listenfd);  

    Signal(SIGINT, SIG_DFL);
}

void client_mode(){
        int errno;
        char getName[MAXBUF];
        int num_bytes;
        char host[30];
        char port[10]=PORT;
        int client_stop_flag=0;

        //Client mode
        #ifdef DEBUG_PRINT
            printf("Client mode selected!\n");
        #endif        
     
      printf("Enter the Host IP address to connect to:");
      errno = scanf("%s",host);
      // printf("Enter the port on the host to connect to:");
      // errno = scanf("%s",port); 

      int remotefd = Open_clientfd(host, port);

    while(1) {

               
        printf("\nEnter The buffer pointer to receive: ");
        errno = scanf("%s", getName); //susceptible to buffer overflow, but whatevs
        strcat(getName, "\n");
        printf("Enter num_bytes asked for ");
        errno = scanf("%d", &num_bytes);
        long long buf_pointer = atoi(getName);

        //Send the buffer pointer in long long format, and num_bytes in int:

        receive_file(&buf_pointer, num_bytes, remotefd , &client_stop_flag);
        if(client_stop_flag){
            break;
        }             
    }
    Close(remotefd);

}



/*
 * sendit - sends preset file to client
 */
void send_file(int clientfd) {
    //char buf[MAXBUF];
   
    rio_t clientRio;

    Rio_readinitb(&clientRio, clientfd);

    int recvd_resp_indicator,resp_indicator;

    int loc_stop_flag = 0;

    long long pointer;


    while(1){ //Looper for new buffer request and process it!

    rio_readnb(&clientRio,&pointer,sizeof(long long));


    #ifdef DEBUG_PRINT
        printf("    Server: Client requested pointer %lld\n",pointer);
    #endif  
    

    int size_req;
    rio_readnb(&clientRio,&size_req,sizeof(int));

    
    buffer_type * send_buf_ptr = (buffer_type *)(pointer);


//*********************************************SEND THE BUFFER******************************
    // Send this buffer whenever change_flag changes, after every sent command wait for ACK. If NACK, switch to above code
    //to receive a new buffer request and number of bytes request. All looping while blocks should also poll for the STOP_FLAG 
    //to switch the roles. NOTE!!!! remove the STOP flag looping in previous calling function, and use it here.

    resp_indicator = ACK;
    rio_writen(clientfd,&resp_indicator,sizeof(int));

    //Irrespective of change flag , send the buffer for one time anyway!
    rio_writen(clientfd,send_buf_ptr,size_req);

    //Read the response from the client to see whether to continue/stop 
    rio_readnb(&clientRio,&recvd_resp_indicator,sizeof(int));

    if(recvd_resp_indicator==NACK){
        continue;
        //QUIT REST OF CODE, START FROM new buffer request!
    }
    // if(STOP_FLAG){
    //     resp_indicator = NACK;
    //     rio_writen(clientfd,&resp_indicator,sizeof(int));
    //     break;
    // }



//***********************************************GOOD TRY*************************************

    while(1){

        int current_value = CHANGE_FLAG;
        printf(" ");

        while(1){
            printf("\n");

            if((current_value)!=CHANGE_FLAG){ //Value of flag changed! send data once and wait for response!
                //***************************NOTE************************************
                //Time this as a constraint for change flag transition allowed times!
                printf("Executing change send\n");
                resp_indicator = ACK;
                rio_writen(clientfd,&resp_indicator,sizeof(int));
                rio_writen(clientfd,send_buf_ptr,size_req);
                rio_readnb(&clientRio,&recvd_resp_indicator,sizeof(int));
                break;
            }

            if(STOP_FLAG){
               printf("Role reversal interrupt!!!\n");
               loc_stop_flag =1;
               resp_indicator =  NACK;
               rio_writen(clientfd,&resp_indicator,sizeof(int));
               break;
           }
        }

        
        if(loc_stop_flag){
            break; // Break to switch role.
        }

        if(recvd_resp_indicator==ACK){
            printf("Postive ack from client!!! continue further\n");
        }
        else{
            printf("New buffer request from client\n");
            break; //execute from new buffer read request block!
        }

    }

    if(loc_stop_flag){
        break; //Break new buffer request loop to get switch role loop.
    }

}


    //************************************************************************************************

    #ifdef DEBUG_PRINT
        printf("    Server: Client received buffer\n");
    #endif  
        
}


void remove_newline_ch(char *line) {
    int new_line = strlen(line) -1;
    if (line[new_line] == '\n')
        line[new_line] = '\0';
}

void receive_file(long long *ptr, int num_bytes, int remotefd, int * client_stop_flag) {
    
    //char buf[MAXBUF];
    rio_t remoteRio;

    DISCONTINUE_FLAG =0;

    int resp_indicator;
    int recvd_resp_indicator;

    Rio_readinitb(&remoteRio, remotefd);

    //Request the buffer pointer.
    rio_writen(remotefd, ptr,sizeof(long long));

    //Request the number of bytes.
    rio_writen(remotefd, &num_bytes,sizeof(int));
    


    //*****************************************READ THE BUFFER********************************************
    buffer_type * int_buffer_2 = (buffer_type *)malloc(num_bytes);

    

    while(1){ 

        printf("Here to read server response!\n");
        rio_readnb(&remoteRio,&recvd_resp_indicator,sizeof(int));


        if(recvd_resp_indicator==ACK){
            printf("Server response positive!, read buffer now\n");

            rio_readnb(&remoteRio,int_buffer_2,num_bytes);

            //*********************************CONSUME THE BUFFER(here buffer is printed)********************************
            #ifdef BUFFER_PRINT
            for(int i=0;i<(num_bytes/sizeof(buffer_type));i++){
                
                    printf("%d\n",int_buffer_2[i]);
            }
            #endif
            //***********************************************************************************************************

            if(DISCONTINUE_FLAG==0){
                //Send ACK  and continue!
                printf("Continue further!!sending to server\n");
                resp_indicator = ACK;
                rio_writen(remotefd,&resp_indicator,sizeof(int)); 
            }
            else{
                //Send NACK and break!
                printf("Discontinue, new buffer request, sending to server!!\n");
                
                resp_indicator = NACK;
                rio_writen(remotefd,&resp_indicator,sizeof(int)); 
                break;  
            }

        }
        else{   //NACK received. switch roles!!
            printf("Server sent role reversal request! break!!!!\n");
            *client_stop_flag = 1;
            break;
        }
    }
    

    //******************Send ACK to keep continuing receiving or send NACK to get a chance to ask for a new buffer->
    //based on a continue flag. if continue flag =0 send ACK. if continue flag = 1 send NACK.

    Free(int_buffer_2);

 
   
}


