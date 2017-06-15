#include "transfer.h"
#include <stdio.h>


volatile int STOP_FLAG;
volatile int CHANGE_FLAG;
volatile int DISCONTINUE_FLAG;

/* 
Signal handler for SIGTSTP signal given to this program. On server side , this leads to termination of roles on both 
server and client,and allows for a role switch mode.
On the client side, this handler leads to client asking for a new buffer to the server.
Currently configured to respond to  SIGTSTP Linux signal which gets generated from 
(Ctrl+Z) keyboard combination while program is running.
To respond to a signal sent by another process, send the appropriate signal to this program
And install this handler for that signal in main();
*/

void sig_handler1(int sig_num){

    STOP_FLAG = 1;      
    DISCONTINUE_FLAG = 1;
  
}


// Signal handler for the SIGQUIT signal given on the server side. This handler toggles the change flag, which leads 
// to the server sending the buffer that the client had requested.
// Every toggle leads to one server-to-client buffer transfer. Hit Ctrl+\ (backslash) on server side to generate this 
// signal through the terminal, and send buffer to client.

void sig_handler2(int sig_num){

    CHANGE_FLAG = (~(CHANGE_FLAG))&(1);
}


int main(int argc, char **argv) {

	Signal(SIGPIPE, SIG_IGN);

	//Installing the signal handler for SIGTSSTP signal. Change to appropriate signal, if 
    //another signal is used to interrupt this program flow.
    Signal(SIGTSTP, sig_handler1); //Ctrl-Z for interruption!

    //Installing the signal handler for SIGINT signal. Change to appropriate signal, if 
    //another process will interrupt this program flow.
    Signal(SIGQUIT, sig_handler2); //Ctrl-\ for interruption!

	operate_mode();

}