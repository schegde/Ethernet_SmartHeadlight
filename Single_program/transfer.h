

#ifndef _TRANSFER_FILE_
#define _TRANSFER_FILE_

#include "csapp.h"

extern int STOP_FLAG;
extern int CHANGE_FLAG;
//extern int READY;


void operate_mode();
void server_mode();
void client_mode();
void send_file(int fd);
void receive_file(long long * pointer, int num_bytes, int remotefd,int * client_stop_flag);
void remove_newline_ch(char *line);

#endif