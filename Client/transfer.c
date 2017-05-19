
#include <stdio.h>
#include <time.h>
#include "csapp.h"


#define ASK_SIZE 30

void receive_file(char *getName, int array_size, char *host, char *port);
void remove_newline_ch(char *line);
int Open_w(const char *pathname, int flags, mode_t mode);
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);
//static unsigned get_file_size (const char * file_name);


/*
 * 
 * main - binds to specified port and listens for new connections,
 * spawning threads to handle their requests
 * 
 */
int main(int argc, char **argv) {
    
    int errno;
    char getName[MAXBUF]; //
    int array_size;

    /* Check command line args */
    if (argc != 3) {
		fprintf(stderr, "usage: %s <IP address of remote device> <Port on remote device to connect to>\n", argv[0]);
		exit(1);
    }

    /* simply ignore sigpipe signals, the most elegant solution */
    Signal(SIGPIPE, SIG_IGN);

     
    while(1) {
        printf("\nEnter The buffer pointer to receive: ");
        errno = scanf("%s", getName); //susceptible to buffer overflow, but whatevs
        strcat(getName, "\n");
        //printf("You Entered: %s", getName);
        printf("Enter array_size asked for ");
        errno = scanf("%d", &array_size);
        //printf("You Entered: %s\n\n", copyName);
        receive_file(getName, array_size, argv[1], argv[2]);
    }
    
}


void receive_file(char *getName, int array_size, char *host, char *port) {
    int remotefd; //, filefd, length;
    char buf[MAXBUF];
    rio_t remoteRio;
    struct timeval start, finish, result;
    gettimeofday(&start, NULL);

    remotefd = Open_clientfd(host, port);

    Rio_readinitb(&remoteRio, remotefd);

    //first write our request to the server
    rio_writen(remotefd, getName, strlen(getName));

    rio_writen(remotefd, &array_size,sizeof(int));
    
    //then get back if the file was found or not
    Rio_readlineb(&remoteRio, buf, MAXLINE);
    printf("Response: %s", buf);

    if(!strcmp(buf,"error\n")) {//file not found
        printf("Client: Error getting file from server\n");
        Close(remotefd);
        return;
    }
    
    int int_buffer_2[array_size];
    rio_readnb(&remoteRio,int_buffer_2,sizeof(int)*array_size);

    for(int i=0;i<array_size;i++){
        // if(int_buffer[i]!=i)
            printf("%d\n",int_buffer_2[i]);
    }

    //now actually do some copying


    //filefd = Open(copyName, O_WRONLY|O_CREAT|O_TRUNC, DEF_MODE);

    // while ((length = rio_readnb(&remoteRio, buf, MAXBUF)) != 0) {
    //     rio_writen(filefd, buf, length);
    // }

   // Close(filefd);
    Close(remotefd);

    //print some stats about our transfer
    gettimeofday(&finish, NULL);
    timeval_subtract(&result, &finish, &start);
    time_t elapsedSeconds = result.tv_sec;
    long int elapsedMicros = result.tv_usec;
    double elapsed = elapsedSeconds + ((double) elapsedMicros)/1000000.0;

    double fileSize = array_size*sizeof(int);
    double speed = fileSize / elapsed;


    printf("Elapsed: %lf seconds\n", elapsed);
    printf("Filesize: %0lf Bytes, %3lf kiloBytes, %lf megaBytes\n", fileSize, fileSize/1000, fileSize/1000000);
    printf("Speed: %0lf B/s, %3lf kB/s, %lf mB/s\n", speed, speed/1000, speed/1000000);

}




void remove_newline_ch(char *line) {
    int new_line = strlen(line) -1;
    if (line[new_line] == '\n')
        line[new_line] = '\0';
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


/***********************************
* Other People's Code *
************************************/


/* This routine returns the size of the file it is called with. */

// static unsigned get_file_size (const char *file_name) {
//     struct stat sb;
//     if (stat (file_name, & sb) != 0) {
//         fprintf (stderr, "'stat' failed for '%s': %s.\n",
//                  file_name, strerror (errno));
//         exit (EXIT_FAILURE);
//     }
//     return sb.st_size;
// }

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