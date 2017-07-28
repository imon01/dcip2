#include "functions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <zconf.h>
#include <locale.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>

#ifndef unix
#define WIN32
#include <windows.h>
#include <winsock.h>
#else
#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif
/*
*Function:
*       number
*
*Description:
*       Checks if all characters are digits in string. Ingores '-'.
*
*
*Returns:
*       pseudo boolean if the string is all digits 1, else -1
*/
struct hostent;
struct sockaddr_in;
int number(char*str){

    int i = 0;
    int value = 1;
    int len = strlen(str);

    for (; i< len; ++i) {

        if (!isdigit(str[i])){
            if(str[i]=='-'){
                continue;
            }else{
                value = -1;
                break;
            }
        }
    }

    return value;
}//end number


/*
*Function:
*           sock_init
*
*Description:
*           Creates the appropriate socket descriptor for a piggy
*
*Relavent Arguments:
*           pigopt 1 -- listen socket
*           pigopt 2 --connect socket
*
*Returns :
*           socket descriptor
*/




int sock_init(int pigopt, int qlen,int port, char *addr, struct sockaddr_in conn, struct hostent *host){

    int sd;
    int len;
    int flag =1;
    len = sizeof(conn);
    memset( (char*)&conn, 0, len);
    conn.sin_family = AF_INET;


    /* left socket, passive listen*/
    if(pigopt == 1){
        conn.sin_addr.s_addr = INADDR_ANY;
        conn.sin_port = htons((u_short)  port);

        /* Create a socket */
        if ( (sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            perror("socket");
            return -1;
        }

        /* Set socket to resuable*/
        if(setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int))== -1){
            perror("setsockopt");
            return -1;
        }

        /* Bind a local address to the socket */
        if( bind(sd, (struct sockaddr *)&conn, len) < 0){
            perror("bind");
            return -1;
        }

        /* Specify size of request queue */
        if (listen(sd, qlen) < 0)  {
            perror("listen");
            shutdown(sd, 2);
            return -1;
        }
    }

    if(pigopt == 2){
        printf("getting host\n");
        conn.sin_port = htons((u_short) port);
        host = gethostbyname(addr);
        printf("memcpy\n");
        memcpy(&conn.sin_addr.s_addr, host->h_addr, host->h_length);

        /* Create a socket. */
        if((sd = socket(AF_INET, SOCK_STREAM, 0))  < 0){
            perror("socket");
            return -1;
        }

        /* Connect to remote host*/
        if( (connect(sd, (struct sockaddr * )&conn, len)) < 0) {
            perror("connect");
            shutdown(sd, 2);
            return -1;
        }
    }
    printf("sock_init ok...\n");
    return sd;
}//end socket_init


/*
*Function:
*           max
*
*Description:
*           compares value of two integers
*
*Relavent Arguments:
*
*Returns :
*           max of two integers
*/
int max(int a, int b){
    int value = b;
    if(a > b){
        value = a;
    }

    return value;

}//end max



/*
*Function:
*           flagsfunction
*
*Description:
*           matches the a string to a valid interactive command
*
*Relavent Arguments:
*
*Returns :
*           -1  invalid interactive command option
*            1  interactive command match
*/
int flagsfunction(icmd opt, char * command, int len ,int position, int * ld, int * rd, struct sockaddr_in right, struct sockaddr_in left, struct sockaddr_in conn){

    int value = -1;
    if (strncmp(command, "outputl", len) == 0) {
        printf("set output to left piggy\n");
        opt.outputl =1;
        opt.outputr =0;

    }
    if (strncmp(command, "outputr", len) == 0) {
        printf("set output to right piggy\n");
        value = 1;
        opt.outputl =0;
        opt.outputr =1;
    }
    if (strncmp(command, "output", len) == 0) {
        value = 1;

        if (opt.outputl = 1) {
            printf("output=left\n");
        } else if (opt.outputr = 1) {
            printf("output=right\n");
        } else {
            printf("not outputting");
        }

    }
    if (strncmp(command, "dsplr", len) == 0) {
        value = 1;
        opt.dsprl = 0;
        opt.dsplr = 1;
        printf("display left to right stream\n");
    }
    if (strncmp(command, "dsprl", len) == 0) {
        value = 1;
        opt.dsprl = 1;
        opt.dsplr = 0;
        printf("display right to left stream\n");
    }
    if (strncmp(command, "display", len) == 0) {
        value = 1;
        printf("show what direction display is\n");
    }
    if (strncmp(command, "dropr", len) == 0) {
        value = 1;
        opt.dropr = 1;
        shutdown( *rd, 2);
        printf("drop right\n");
    }
    if (strncmp(command, "dropl", len) == 0) {
        value = 1;
        opt.dropl = 1;
        shutdown( *ld, 2);
        printf("drop left\n");
    }
    if (strncmp(command, "persl", len) == 0) {
        value = 1;
        opt.persl = 1;
        printf("persl\n");
    }
    if (strncmp(command, "persr", len) == 0) {
        value = 1;
        opt.persr = 1;
        printf("persrl\n");
    }
    if (strncmp(command, "right", len) == 0){
        value = 1;

        printf("%s",inet_ntoa(conn.sin_addr));
        printf(":%hu",conn.sin_port);
        if(position !=2){
            printf(":%s",inet_ntoa(left.sin_addr));
            printf(":%hu",left.sin_port);
        }
        else{
            printf(":*:*");
        }

        if(position ==0){
            printf("\nDISCONNECTED\n");
        }
        if(position == 1){
            printf("\nCONNECTED\n");
        }
        if(position == 2){
            printf("\nLISTENING\n");
        }
    }

    if (strncmp(command, "left", len) == 0) {
        value = 1;

        if(position != 1 ){
            printf("%s",inet_ntoa(left.sin_addr));
            printf(":%hu",left.sin_port);
        }
        else{
            printf("*:*");
        }
        printf(":%s",inet_ntoa(conn.sin_addr));
        printf(":%hu",conn.sin_port);


        if(position ==0){
            printf("\nDISCONNECTED\n");
        }
        if(position == 1){
            printf("\nCONNECTED\n");
        }
        if(position == 2){
            printf("\nLISTENING\n");
        }
    }
    if (strncmp(command, "loopr", len) == 0) {
        value = 1;
        opt.loopr = 1;
        printf("loopr\n");
    }
    if (strncmp(command, "loopl", len) == 0) {
        value = 1;
        opt.loopr = 1;
        printf("loopr\n");
    }
    return value;
}
