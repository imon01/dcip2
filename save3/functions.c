#include "functions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>

#include <zconf.h>
#include <locale.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <ifaddrs.h>

#ifndef unix
#define WIN32
#include <windows.h>
#include <winsock.h>
#else
/*
#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
 */
#include <netdb.h>
#endif

struct hostent;
struct sockaddr_in;

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
int sock_init( int pigopt, int qlen, int port, char *addr, struct sockaddr_in conn, struct hostent *host ){


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
            nerror("socket");
            return -1;
        }

        /* Set socket to resuable*/
        if(setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int))== -1){
            nerror("setsockopt");
            return -1;
        }

        /* Bind a local address to the socket */
        if( bind(sd, (struct sockaddr *)&conn, sizeof(conn)) < 0){
            nerror("bind");
            return -1;
        }

        /* Specify size of request queue */
        if (listen(sd, qlen) < 0)  {
            nerror("listen");
            shutdown(sd, 2);
            return -1;
        }


    }

    /* Connecting */
    if(pigopt == 2){
        conn.sin_port = htons((u_short) port);
        host = gethostbyname(addr);
        memcpy(&conn.sin_addr.s_addr, host->h_addr, host->h_length);

        //inet_aton(host->h_addr, &conn.sin_addr);


        /* Create a socket. */
        if((sd = socket(AF_INET, SOCK_STREAM, 0))  < 0){
            nerror("socket");
            return -1;
        }

        /* Connect to remote host*/
        if( (connect(sd, (struct sockaddr * )&conn, sizeof(conn))) < 0) {
            nerror("!connect");
            shutdown(sd, 2);
            return -1;
        }
    }

    return sd;
}/*end socket_init */


char fileRead(const char *filename, char *output[255]) {


    int count = 0;
    char input[255];
    char *line;
    FILE *file = fopen(filename, "r");
    char buf[255] = "",
            *delim = " \n";

    if (!file) {  /* validate file open for reading */
        nerror("error: file open failed");
        return 1;
    }

    if (!fgets (buf, 255, file)) {  /* read one line from file */
        nerror("error: file read failed");
        return 1;
    }

    /* tokenize line with strtok */
    for (char *p = strtok (buf, delim); p; p = strtok (NULL, delim))
        output[count++] = strdup(p);//Store replica
    if (file != stdin) {
        fclose(file);     /* close file if not stdin */
    }
    return count;
}
//

char *strdup(const char *str){
    char *ret = malloc(strlen(str)+1);
    if(ret){
        strcpy(ret, str);
    }
    return ret;
}

/*
*Function:
*           sockettype
*
*Description:
*           performs the respective 
*
*Relavent Arguments:

*Returns :
*           None
*/


void sockettype(char *buf, char *sockid, unsigned char *stype, unsigned char * openld, unsigned char * openrd, int * local, int * remote, icmd * flags, fd_set *masterset){
    int n = 0;
    
    /* Passive socket (0) */
    if( stype){
                bzero(buf, sizeof(buf));
                n = recv(*local, buf, sizeof(buf), 0);

                if (n < 0) {
                    nerror("remote left recv error ");
                }
                if (n == 0) {
                    nerror("remote left connection closed");
                    *openld = 0;
                    
                    /**WARNING: RECONNECTION PSEUDOCODE CASES**/
                    
                    //if( active right && persr, reconnect)
                    //    SET FLAG--> flags->persr = 2;
                    //    Note: reconnection will be attempted at end of SELECT LOOP                                
                    //
                    //else we have a passive right, can't reconnect
                    // display warning: "reconnection error for local right descriptor, is passive"
                    
                    /**NOTE**/
                    /* Since n == 0, okay to remove*/
                    FD_CLR(*local, masterset);
                }

                /*
                * move cursor to window
                * printw
                *
                */
                /* If dsplr is set we print data coming fr0m the left*/
                /*`q*/
                
                winwrite(CMW, "d1");
                if(strcmp(sockid, LEFT) == 0){
                    getyx(sw[ULW], yul, xul);
                    if(buf[0]== 13){
                        yul++;
                        xul= 0;
                    }
                    wmove(sw[ULW], yul, xul);
                    wprintw(sw[ULW], "%c",buf[0]);
                    //scroll(sw[ULW]);
                    update_win(ULW);
                else{                    
                    getyx(sw[BRW], ybr, xbr);
                    if(buf[0]== 13){
                        ybr++;
                        xbr= 0;
                    }
                    wmove(sw[BRW], ybr, xbr);
                    wprintw(sw[BRW], "%c",buf[0]);
                    //scroll(sw[BRW]);
                    update_win(BRW);
                }
                

                /* Loop data right if set*/ 
                if (flags->loopr && *openld) {
                    n = send(*local, buf, sizeof(buf), 0);

                    if (n < 0) {
                        nerror("remote left send error");                        
                    }
                    if (n == 0) {
                        /*NOTE: persl must be handled differently in Piggy3*/
                        nerror("remote left connection closed");
                    }
                    
                    getyx(sw[BLW], ybl, xbl);
                    if(buf[0]== 13){
                        ybl++;
                        xbl= 0;
                    }
                    wmove(sw[BLW], ybl, xbl);
                    wprintw(sw[BLW], "%c",buf[0]);
                    //scroll(sw[BLW]);
                    update_win(BLW);                    
                    bzero(buf, sizeof(buf));
                    
                }

                /* Check if data needs to be forwarded */
                if (*openrd && flags->output) {
                    n = send(*remote, buf, sizeof(buf), 0);

                    if (n < 0) {
                        *openrd = 0;
                        nerror("remote right send error");                        
                    }
                    if (n == 0) {
                        *openrd = 0;
                        /*NOTE: persl must be handled differently in Piggy3*/
                        /* Set reconnect flag if persl is set*/
                        nerror("remote right connection closed 1");
                    }
                    
                    getyx(sw[BRW], ybr, xbr);
                    if(buf[0]== 13){
                        ybr++;
                          xbr= 0;
                    }
                    wmove(sw[BRW], ybr, xbr);
                    wprintw(sw[BRW], "%c",buf[0]);
                    //scroll(sw[BRW]);
                    update_win(BRW);
                    
                    bzero(buf, sizeof(buf));
                }

                /* b.0*/
                /* Check if output is set to left*/
                /**WARNING SOURCE OF CYCLIC DATA PASSING**/
                /**SET ANOTHER  CONDITION "PASSING VARIABLE" **/
                if (*openld && !flags->output && (flags->position != 2)) {
                    /* b.0*/
                    n = send(*local, buf, sizeof(buf), 0);

                    if (n < 0) {
                        *openld = 0;
                        nerror("remote left send error ");
                    }
                    if (n == 0) {
                        *openld = 0;
                        /*NOTE: persl must be handled differently in Piggy3*/                                                
                        /* Set reconnect flag if persl is set*/
                        nerror("remote left connection closed ");
                    }
                    bzero(buf, sizeof(buf));
                }        
    }
    /*Active descriptor (1)*/
    else{
           bzero(buf, sizeof(buf));
            n = recv(*local, buf, sizeof(buf), 0);
            //winwrite(BRW, "2");
            if (n < 0) {
                *openrd = 0;
                nerror("remote right recv error ");                
            }

            if (n == 0) {
                nerror("remote right connection closed 2");
                *openrd = 0;
                
                /**WARNING: RECONNECTION PSEUDOCODE CASES**/
                
                //if( active left && persl, reconnect)
                //    SET FLAG--> flags->persl = 1;
                //    Note: reconnection will be attempted at end of SELECT LOOP                                
                //
                //else we have a passive left, can't reconnect
                // display warning: "reconnection error for local left descriptor, is passive"                
                
                /**NOTE**/
                /* Since n == 0, okay to remove*/
                FD_CLR(*local, masterset);
            }

            winwrite(CMW, "d2");
            /* Display data arriving on right side in BRW*/
            getyx(sw[BRW], ybr, xbr);
            if(buf[0]== 13){
                ybr++;
                xbr= 0;
            }
            
            wmove(sw[BRW], ybr, xbr);
            wprintw(sw[BRW], "%c",buf[0]);
            //scroll(sw[BRW]);
            update_win(BRW);


            /* Check for constant DROPL string*/
            if (strcmp(buf, DROPL) == 0) {
                winwrite(CMW, "remote right side dropn ");
                *openrd = 0;
            }
                /* Check for constant PERSL string*/
            else if (strcmp(buf, PERSL) == 0) {
                winwrite(CMW, "remote right side reconnection ");
                *openrd = 1;
            } else {
                /* If dsprl is set we print data coming frm the right*/                                                    

                if (flags->loopl == 1) {
                    n = send(*remote, buf, sizeof(buf), 0);
                    if (n < 0) {
                        *openrd = 0;
                        nerror("send right error ");                        
                    }
                    if (n == 0) {
                        *openrd = 0;
                        flags->persr = 2;                        
                    }
                    bzero(buf, sizeof(buf));
                }

                /* Data only left forwarded if middle piggy */
                if (*openld && !flags->output) {
                    n = send(*local, buf, sizeof(buf), 0);
                    if (n < 0) {
                        *openld = 0;
                        nerror("send left error ");                        
                    }

                    if (n == 0) {
                        *openld = 0;
                        flags->persl = 2;                        
                    }
                }


                if (*openrd && flags->output) {
                    n = send(*remote, buf, sizeof(buf), 0);
                    if (n < 0) {
                        nerror("send right error ");                        
                    }
                    if (n == 0) {
                        flags->persr = 2;                        
                    }
                    bzero(buf, sizeof(buf));
                }
            }        
    }
    
}


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
*            2  reserved for persl
*            3  reserved for persr
*            4  reserved for dropl
*            5  reserver for dropr
*/
int flagsfunction( icmd  * flags, char * command, int len ,int position, unsigned char * openld, unsigned char * openrd, int * ld, int * rd, struct sockaddr_in left, struct sockaddr_in right, int inputDesignation, unsigned char *ltype, unsigned char *rtype){
        int value = -1;
        winclear(CMW, 1,0);
        
        /* output left (0), output right (1)*/
        if (strncmp(command, "outputl", len) == 0) {

            if(position != 1){
                value = 1;
                flags->output =0;
            }
            else{
                nerror("Cant set head piggy output right\n");
            }
        }


        /* output left (0), output right (1)*/
        if (strncmp(command, "outputr", len) == 0) {

            if(position !=2){
                value = 1;
                flags->output = 1;
            }else{
                nerror("Cant set tail piggy output right");
            }
        }

        /* */
        if (strncmp(command, "output", len) == 0) {
            value = 1;
            if (flags->output) {
                winwrite(CMW, "output = right");                
            }
            else{
                winwrite(CMW, "output = left");                
            }
        }

        /* */
        if (strncmp(command, "dsplr", len) == 0) {

            if(position !=1){
                value = 1;
                flags->dsprl = 0;
                flags->dsplr = 1;
            }
            else{
                nerror("Cant set dsplr for head piggy");
            }
        }

        /* */
        if (strncmp(command, "dsprl", len) == 0) {

            if(position != 2){
                value = 1;
                flags->dsprl = 1;
                flags->dsplr = 0;
            }
            else{
                nerror("Cant set dsprl for tail piggy");
            }
        }

        /* */
        if (strncmp(command, "display", len) == 0){
            value = 1;
            if(flags->dsprl){
                winwrite(CMW, "display right\n");
            }
            else{
                winwrite(CMW, "display left\n");
            }
        }

        /* */
        if (strncmp(command, "persl", len) == 0) {
            value = 2;
            flags->persl = 1;
            *openld = 1;

        }

        /* */
        if (strncmp(command, "persr", len) == 0) {
            value = 3;
            flags->persr = 1;
            *openrd = 1;

        }

        /* */
        if (strncmp(command, "dropl", len) == 0) {
            
            value = 4;                        
            flags->dropl = 1;
            *openld = 0;
            shutdown(*ld, 2);
                        
        }

        /* */
        if (strncmp(command, "dropr", len) == 0) {
            value = 5;
            flags->dropr = 1;
            *openrd = 0;
            shutdown( *rd, 2);
        }

        /* */
        if (strncmp(command, "right", len) == 0){
            value = 1;
            winclear(INW);
            wmove(sw[INW], 1,0);
            wprintw(sw[INW], "%s:%hu",flags->localaddr, flags->llport);
                        
            if(*openrd == 1){
                wprintw(sw[INW], "%s:%hu",flags->localaddr, flags->llport);
            }
            else{
                wprintw(sw[INW], ":*:*");
            }

            wmove(sw[INW], 2, 0);
            if(*rtype){
                wprintw(sw[INW], "CONNECTED");
            }
            else if( !*rtype & *openrd){
                wprintw(sw[INW], "LiSTENING");
            }
            else{                
                waddstr(sw[INW], "DISCONNECTED");
            }
            
            update_win(INW);
        }

        /* left side connection*/
        if (strncmp(command, "left", len) == 0){
            winclear(INW);
            wmove(sw[INW], 1,0);
            value = 1;
            if( *openld ){
                wprintw(sw[INW], "%s:%hu",inet_ntoa(left.sin_addr), left.sin_port);                
            }
            else{
                wprintw(sw[INW], "*:*");                
            }

            wprintw(sw[INW], ":%s:%hu", flags->localaddr, flags->llport);

            wmove(sw[INW], 2, 0);
            if(*ltype){
                wprintw(sw[INW], "CONNECTED");
            }
            else if( !*ltype & *openld){
                wprintw(sw[INW], "LiSTENING");
            }
            else{
                waddstr(sw[INW], "DISCONNECTED");
            }
            
            update_win(INW);
        }

        if (strncmp(command, "loopr", len) == 0) {
            value = 1;
            flags->loopr = 1;  /* Takes data to be written to the right and sends left  */
            flags->output = 0; /* Output becomes left with loopr                        */

        }
        if (strncmp(command, "loopl", len) == 0) {
            value = 1;
            flags->loopl = 1;   /* Takes data to be written to the left and send right  */
            flags->output = 1;  /* Output becomes right                                 */
        }
        if (inputDesignation != -1){
            value = 1;
        }
        if (strncmp(command, "outputl", len) == 0) {

            if(position != 1){
                value = 1;
                flags->output =0;
            }
            else{
                nerror("Cant set head piggy output right");
            }
        }
        if (strncmp(command, "reset", len) == 0) {
            value = 1;
            flags->reset = 1;
        }
    
        return value;
}
/*End flagsfunction*/