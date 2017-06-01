

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
void receive_file(long long * pointer, int num_bytes, char *host, char *port);
void remove_newline_ch(char *line);
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);

#endif