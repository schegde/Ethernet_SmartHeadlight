

#ifndef _TRANSFER_FILE_
#define _TRANSFER_FILE_

#include "csapp.h"

extern volatile int STOP_FLAG;
extern volatile int CHANGE_FLAG;
extern volatile int DISCONTINUE_FLAG;


void operate_mode();
void server_mode();
void client_mode();
void send_buffer(int fd);
void receive_buffer(long long * pointer, int num_bytes, int remotefd,int * client_stop_flag);
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);

#endif