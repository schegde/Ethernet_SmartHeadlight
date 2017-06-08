#include "transfer.h"
#include <stdio.h>


volatile int STOP_FLAG;
volatile int CHANGE_FLAG;
volatile int DISCONTINUE_FLAG;

void sig_handler(int sig_num){

    STOP_FLAG = 1;      
    DISCONTINUE_FLAG = 1;
  
}


void sig_handler2(int sig_num){

    CHANGE_FLAG = (~(CHANGE_FLAG))&(1);
}


int main(int argc, char **argv) {

	  operate_mode();

}