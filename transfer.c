
#include <stdio.h>
#include <time.h>
#include "transfer.h"
#include <limits.h>


//*************************************************CUSTOMIZATION**************************************************

#define PORT "6045"

//Comment out the below macro to accept a port number from user, else the above defined port will be used every time
//the program is run. Custom port numbers need to be used to fire multiple instances of program, and maintain simultaneous
//connections(each connection with unique port number). This is useful for single server device -multiple client device 
//scenario.Fire up the program multiple times into server mode with different port numbers, and then use those port 
//numbers on client devices to connect to server. This gives multiple simultaneous client connections with the server.

#define USE_FIXED_PORT

//Buffer size allocated on server side.
#define BUFFERSIZE 100 

//Buffer type allocated on server side.
typedef int buffer_type;

//Comment the below line to stop printing the received buffer on client side
#define BUFFER_PRINT

//Comment the below line to stop printing most of debug info
#define DEBUG_PRINT

//The stop flag can be set by a top level application/or through a POSIX-LINUX signal to stop the device role
//and for a chance to change mode-server/client. This flag is monitored only by server, and upon seeing stop flag
//being raised, the server sends a message to client to stop it's role, and go for a mode switch.In effect , both
//server and client stop their respective roles, and move to the role selection stage.
volatile int STOP_FLAG; 

//The flag that is monitored by the server to send data to client. After the first data sent, server sends buffer to
//client only when change flag changes. 

//The time needed to complete a buffer transfer after seeing the flip of change
//flag is around ~0.42
volatile int CHANGE_FLAG;

//This flag can be set by a top level application/ or through a POSIX-LINUX signal to the client, to request for a 
//new buffer from the server. Once it is raised on client side, the client sends appropriate message to server
//to be able to handle a new buffer request. This flag is only monitored by the client.
volatile int DISCONTINUE_FLAG;

//The mode selection variable. Set the mode to either of below macros to specify the device role.
volatile int MODE;
#define SERVER_MODE 0
#define CLIENT_MODE 1


//random numbers assigned for ACK and NACK handshaking messages used by server and client for synchronization.
#define ACK 5 
#define NACK 6
#define IDLE 7

//IDLE handshaking delay counter max limit. INT_MAX is a C macro in limits.h which gives max value of integer 
//data type. If you find the delay between raising discontinue flag on client(new buffer request), and the client response 
//to it, to be more than desirable, then reduce the COUNT_MAX value, to reduce the delay between successive IDLE handshaking
//data exchange between server and client. 
#define COUNT_MAX (INT_MAX-100) 


//**********************************************************************************************************************



//*******************************************************NOTES***************************************************************
// 1)   START PROCEDURE:START THE SERVER FIRST, AND THEN THE CLIENT. (This order is VERY necessary)
// 2)   STOP PROCEDURE: SEND a configured linux SIGNAL(currently configured for ctrl+z keyboard combination) 
//                      TO THIS PROGRAM ON *SERVER*; SERVER THEN SENDS A MESSAGE TO CLIENT TO 
//                      stop itself, and reverse role if needed.
// 3)   Minimum one transaction is needed to stop the respective roles and allow for role reversal :( 
//      [This is a disadvantage of a blocking network socket, but non-blocking sockets are more tough to manage]
// 4)   Every transaction implements a ACK/NACK/STOP_MSG handshaking between server and client for synchronization.
// 5)   There will be timing restrictions on The control signals(STOP/CHANGE/DISCONTINUE FLAG) that are given to this program.
//      These signals can be given from a new thread in this program, to keep it asynced with the flow control of 
//      main thread of this program.
// 6)   To gracefully close this system in the current state : Hit Ctrl+Z on the server to quit the roles on server and 
//      client side, and then hit Ctrl+C on both devices respectively to close the programs.
// 7)   Minimum interval between 2 successive change_flag flips, for the program to respond to both the changes , is 
//      effectively the time needed to send the buffer from server to the client, and receive the acknowledgement.
//      The program displays the buffer transmission time after completing a transaction, and also the average transmission time
//      upon quitting server role(for role switch). The program doesn't respond to changes in change_flag during this interval.
//      
//**************************************************************************************************************************




/*
Select the mode of operation . Also installs the signal handlers
*/
void operate_mode(){


           
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
            client_mode();    //Call the client routine.    
        }
    }   
 
}


void server_mode(){


    CHANGE_FLAG =0; //Initialize the change flag for server transmission behaviour.
   
    int errno;
    int listenfd;    
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);


    #ifdef DEBUG_PRINT
        printf("Server mode selected!\n");
    #endif


    #if defined(USE_FIXED_PORT)
        char port[10]=PORT;
    #else
        char port[10];
        printf("Enter the port number:");
        errno = scanf("%s",port);
    #endif
     
    listenfd = Open_listenfd(port);

   
    printf("Successfully started a listening server on  port %s, Now connect to this port from client side!\n",port);
        
    
    int loc_connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        
    send_buffer(loc_connfd);
    Close(loc_connfd);
    Close(listenfd);  
    
}

void client_mode(){
        int errno;
        char getName[MAXBUF];
        int num_bytes;
        char host[30];
        int client_stop_flag=0;

        //Client mode
        #ifdef DEBUG_PRINT
            printf("Client mode selected!\n");
        #endif        
     

        printf("Enter the Host IP address to connect to:");
        errno = scanf("%s",host);


        #if defined(USE_FIXED_PORT)
            char port[10]=PORT;
        #else
            char port[10];
            printf("Enter the port number to connect to:");
            errno = scanf("%s",port);
        #endif
         
      
     
      int remotefd = Open_clientfd(host, port);

    while(1) {

               
        printf("\nEnter The buffer pointer to receive: ");
        errno = scanf("%s", getName); //susceptible to buffer overflow, but whatevs
        strcat(getName, "\n");
        printf("Enter num_bytes asked for ");
        errno = scanf("%d", &num_bytes);
        long long buf_pointer = atoi(getName);

        //Send the buffer pointer in long long format, and num_bytes in int:

        receive_buffer(&buf_pointer, num_bytes, remotefd , &client_stop_flag);
        if(client_stop_flag){
            break;
        }             
    }
    Close(remotefd);

}


/*
 * sendit - sends preset file to client
 */
void send_buffer(int clientfd) {
    //char buf[MAXBUF];
   
    rio_t clientRio;

    Rio_readinitb(&clientRio, clientfd);

    int recvd_resp_indicator,resp_indicator;

    int loc_stop_flag = 0;

    long long pointer;

    double total = 0;
    double count = 0;

    int counter = 0;


    while(1){ //Looper for new buffer request and process it!

    rio_readnb(&clientRio,&pointer,sizeof(long long));


    #ifdef DEBUG_PRINT
        printf("    Server: Client requested pointer %lld\n",pointer);
    #endif  
    

    int size_req;
    rio_readnb(&clientRio,&size_req,sizeof(int));

    
    buffer_type * send_buf_ptr = (buffer_type *)(pointer);


//*********************************************SEND THE BUFFER******************************

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
   
    
    struct timeval start, finish, result;
   
    while(1){
        
        
        int current_value = CHANGE_FLAG;
       
        while(1){
            

            if((current_value)!=CHANGE_FLAG){ //Value of flag changed! send data once and wait for response!
               
                
                gettimeofday(&start, NULL);

                #ifdef DEBUG_PRINT
                    printf("Executing change send\n");
                #endif
                
                resp_indicator = ACK;
                rio_writen(clientfd,&resp_indicator,sizeof(int));
                rio_writen(clientfd,send_buf_ptr,size_req);
                rio_readnb(&clientRio,&recvd_resp_indicator,sizeof(int));
                break;
            }

            if(STOP_FLAG){
               
               #ifdef DEBUG_PRINT
                    printf("Role reversal interrupt!!!\n");
                #endif
               
               loc_stop_flag =1;
               resp_indicator =  NACK;
               rio_writen(clientfd,&resp_indicator,sizeof(int));
               break;
           }

           counter++;
           //Delay counter for idle handshaking to avoid blocking on the sockets!.
           if(counter == COUNT_MAX){
                counter = 0;
                
                resp_indicator = IDLE;
                rio_writen(clientfd,&resp_indicator,sizeof(int));
                rio_readnb(&clientRio,&recvd_resp_indicator,sizeof(int));
                if(recvd_resp_indicator != IDLE)
                    break;

           }



        }

        
        if(loc_stop_flag){
            break; // Break to switch role.
        }

        if(recvd_resp_indicator==ACK){
            #ifdef DEBUG_PRINT
                    printf("Postive ack from client!!! continue further\n");
            #endif            
        }
        else{

            #ifdef DEBUG_PRINT
                printf("New buffer request from client\n");
            #endif            
            break; //execute from new buffer read request block!
        }


        //Measure time needed to 
            gettimeofday(&finish, NULL);            
            timeval_subtract(&result, &finish, &start);
            time_t elapsedSeconds = result.tv_sec;
            long int elapsedMicros = result.tv_usec;
            double elapsed = elapsedSeconds + ((double) elapsedMicros)/1000000.0;

            #ifdef DEBUG_PRINT
                printf("Buffer transmission time: %lf\n",elapsed);
            #endif            
            total +=elapsed;
            count++;

    }

    if(loc_stop_flag){
        break; //Break new buffer request loop to get switch role loop.
    }

}
    
    #ifdef DEBUG_PRINT
            printf("Average buffer transmission time: %lf\n",(total/count));
    #endif 
    
            
}




void receive_buffer(long long *ptr, int num_bytes, int remotefd, int * client_stop_flag) {
    
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

        
        rio_readnb(&remoteRio,&recvd_resp_indicator,sizeof(int));


        if(recvd_resp_indicator==ACK){

            #ifdef DEBUG_PRINT
                printf("Server response positive!, read buffer now\n");
            #endif 
           

            rio_readnb(&remoteRio,int_buffer_2,num_bytes);

            //*********************************CONSUME THE BUFFER(here buffer is printed)********************************
            #ifdef BUFFER_PRINT
            for(int i=0;i<(num_bytes/sizeof(buffer_type));i++){
                
                    printf("%d\n",int_buffer_2[i]);
            }
            #endif
            //***********************************************************************************************************

            if(DISCONTINUE_FLAG==0){
                
                #ifdef DEBUG_PRINT
                    printf("Continue further!!sending to server\n");
                #endif

                //Send ACK  and continue!
                resp_indicator = ACK;
                rio_writen(remotefd,&resp_indicator,sizeof(int)); 
            }
            else{
                
                #ifdef DEBUG_PRINT
                    printf("Discontinue, new buffer request, sending to server!!\n");
                #endif                
                
                //Send NACK and break!
                resp_indicator = NACK;
                rio_writen(remotefd,&resp_indicator,sizeof(int)); 
                break;  
            }

        }
        else if(recvd_resp_indicator==NACK){   //NACK received. switch roles!!

            #ifdef DEBUG_PRINT
                printf("Server sent role reversal request! break!!!!\n");
            #endif 
            
            *client_stop_flag = 1;
            break;
        }
        else{ //IDLE connection handshaking!.

            if(DISCONTINUE_FLAG==0){
                                
                //Send IDLE connection handshaking and continue!
                resp_indicator = IDLE;
                rio_writen(remotefd,&resp_indicator,sizeof(int)); 
            }
            else{

                //Flag raised! discontinue!

                #ifdef DEBUG_PRINT
                    printf("Discontinue, New buffer request, sending to server!!\n");
                #endif                
                
                //Send NACK and break!
                resp_indicator = NACK;
                rio_writen(remotefd,&resp_indicator,sizeof(int)); 
                break;  
            }

        }
    }
    

    Free(int_buffer_2);

    
}

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