

#ifndef _TRANSFER_FILE_
#define _TRANSFER_FILE_

#include "csapp.h"

extern int STOP_FLAG;
extern int READY;


void sendit(int fd);
void clienterror(int fd, char *cause, char *errnum, 
     char *shortmsg, char *longmsg);
void send_func(void *vargp);

void receive_file(char *getName, int num_bytes, char *host, char *port);
void listen_func(void *vargp);
void remove_newline_ch(char *line);
//int Open_w(const char *pathname, int flags, mode_t mode);
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);
void mode_selector(int mode);



#endif