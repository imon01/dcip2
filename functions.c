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
#include <net/if.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>

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
*           getip
* 
*Description: 
*           compares value of two integers
* 
*Relavent Arguments:
* 
*Returns : 
*           max of two integers
*/


int sock_init(icmd * flags, int pigopt, int qlen, int port, char *addr, struct sockaddr_in conn, struct hostent *host ){
    
    
        int sd, len,n =0;        
        int flag =1;                                    
        len = sizeof(struct sockaddr_in *);
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
                if( bind(sd, (struct sockaddr *)&conn, sizeof(conn)) < 0){
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
        
        /* Connecting */
        if(pigopt == 2){            
                printf("getting host\n");
                conn.sin_port = htons((u_short) port);
                host = gethostbyname(addr);
                printf("memcpy\n");
                memcpy(&conn.sin_addr.s_addr, host->h_addr, host->h_length);
                
                //inet_aton(host->h_addr, &conn.sin_addr);
                

                /* Create a socket. */
                if((sd = socket(AF_INET, SOCK_STREAM, 0))  < 0){
                    perror("socket");
                    return -1;
                }
                
                /* Connect to remote host*/
                if( (connect(sd, (struct sockaddr * )&conn, sizeof(conn))) < 0) {
                    perror("!connect");
                    shutdown(sd, 2);
                    return -1;
                }
        }
        printf("sock_init ok...\n");
        return sd;    
}/*end socket_init */



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
*            1  valid interactive command 
*/
int flagsfunction(icmd  * flags, char * command, int len ,int position, int * openld, int * openrd, int * ld, int * rd, struct sockaddr_in right, struct sockaddr_in left){    
        int value = -1;
        
        if (strncmp(command, "outputl", len) == 0) {        
            printf("set output to left piggy\n");
            flags->output =0;        
            flags->loopl = 0;
            flags->loopr = 0;            
        }
        
        if (strncmp(command, "outputr", len) == 0) {
            
            value = 1;        
            flags->output = 1;
            flags->loopl = 0;
            flags->loopr = 0;
        }
        
        if (strncmp(command, "output", len) == 0) {
            value = 1;        
            if (flags->output = 0) {
                printf("output = left\n");
            }
            else{
                printf("output = right\n");  
            }
        }
        
        if (strncmp(command, "dsplr", len) == 0) {
            value = 1;
            flags->dsprl = 0;
            flags->dsplr = 1;
            flags->dsplr = 1;
            printf("display left to right stream\n");
        }
        
        if (strncmp(command, "dsprl", len) == 0) {
            value = 1;     
            flags->dsprl = 1;
            flags->dsplr = 0;                    
            printf("display right to left stream\n");
        }
        
        if (strncmp(command, "display", len) == 0) {
            value = 1;
            if(flags->dsplr){
                printf("display right\n");
            }
            else{
                printf("display left\n");
            }        
        }
        
        if (strncmp(command, "dropr", len) == 0) {
            value = 1;
            flags->dropr = 1;
            shutdown( *rd, 2);
            printf("drop right\n");
        }
        
        if (strncmp(command, "dropl", len) == 0) {
            value = 2;
            flags->dropl = 1;        
            printf("drop left\n");
        }
        
        if (strncmp(command, "persl", len) == 0) {
            value = 1;
            flags->persl = 1;
            printf("persl\n");
        }
        
        if (strncmp(command, "persr", len) == 0) {        
            value = 1;
            flags->persr = 1;
            printf("persrl\n");
        }
        
        if (strncmp(command, "right", len) == 0){    
            value = 1;                
            printf("%s:%hu", flags->lladdr, flags->llport);
            if(*openrd == 1){
                printf(":%s:%hu",inet_ntoa(right.sin_addr), right.sin_port);            
            }
            else{
                printf(":*:*");
            }
            
            if(flags->dropr){
                printf("\nDISCONNECTED\n");
            }
            else{
            printf("\nCONNECTED\n");
            }
        }
        
        if (strncmp(command, "left", len) == 0) {                
            value = 1;                
            if(position != 1 && !flags->dropl && *openld == 1){
                printf("%s:%hu",inet_ntoa(right.sin_addr), right.sin_port);            
            }
            else{
                printf("*:*");
            }                
            printf(":%s:%hu", flags->lladdr, flags->llport);        
                    
                        
            if(flags->dropl){
                printf("\nDISCONNECTED\n");
            }
            else{
                printf("\nLISTENING\n");
            }        
        }
        
        if (strncmp(command, "loopr", len) == 0) {
            value = 1;
            flags->loopr = 1;
            printf("loopr\n");
        }
        if (strncmp(command, "loopl", len) == 0) {        
            value = 1;
            flags->loopr = 1;
            printf("loopr\n");
        }    
        return value;
}
/*End flagsfunction*/


char fileRead(const char *filename, char *output[255]) {


    int count = 0;
    char input[255];
    char *line;
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Cannot open file: %s\n", filename);        
    }else{        
        while(count < 255 && fgets(input, sizeof(input), file)) {
            line = strtok(input, "\n");
            if(line)
                output[count++] = strdup(line);//Store replica
        }
        fclose(file);
    }

    return count;
}


char *strdup(const char *str){
    char *ret = malloc(strlen(str)+1);
    if(ret){
        strcpy(ret, str);
    }
    return ret;
}