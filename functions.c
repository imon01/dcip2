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
    winclear(INW, 1,0);

    /* output left (0), output right (1)*/
    if (strncmp(command, "outputl", len) == 0) {
        value = 1;
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
        value = 1;
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
            wprintw(sw[INW], "output = right");
            update_win(INW);
        }
        else{
            wprintw(sw[INW], "output = left");
            update_win(INW);
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
        wmove(sw[INW], 0,0);
        wprintw(sw[INW], "%s:%hu",flags->localaddr, flags->llport);

        if(*openrd == 1){
            wprintw(sw[INW], "%s:%hu",flags->localaddr, flags->llport);
        }
        else{
            wprintw(sw[INW], ":*:*");
        }

        wmove(sw[INW], 1, 0);
        if(*openrd){
            wprintw(sw[INW], "CONNECTED");
        }
        else{
            waddstr(sw[INW], "DISCONNECTED");
        }
        update_win(INW);
    }

    /* left side connection*/
    if (strncmp(command, "left", len) == 0){
        winclear(INW);
        wmove(sw[INW], 0,0);
        value = 1;
        if( *openld ){
            wprintw(sw[INW], "%s:%hu",inet_ntoa(left.sin_addr), left.sin_port);
        }
        else{
            wprintw(sw[INW], "*:*");
        }

        wprintw(sw[INW], ":%s:%hu", flags->localaddr, flags->llport);

        wmove(sw[INW], 1, 0);
        if( *openld){
            wprintw(sw[INW], "LISTENING");
        }
        else{
            wprintw(sw[INW], "DISCONNECTED");
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
    if (strncmp(command, "connectl", len) == 0) {
        value = 1;
    }
    if (strncmp(command, "connectr", len) == 0) {
        value = 1;
    }
    if (strncmp(command, "listenl", len) == 0) {
        value = 1;
    }
    if (strncmp(command, "listenr", len) == 0) {
        value = 1;
    }
    if (strncmp(command, "llport", len) == 0) {
        value = 1;
    }
    if (strncmp(command, "rrport", len) == 0) {
        value = 1;
    }
    if (strncmp(command, "rlport", len) == 0) {
        value = 1;
    }
    if (strncmp(command, "lrport", len) == 0) {
        value = 1;
    }
    if (strncmp(command, "lraddr", len) == 0) {
        value = 1;
    }
    if (strncmp(command, "rraddr", len) == 0) {
        value = 1;
    }
    return value;
}
/*End flagsfunction*/


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

char *strdup(const char *str){
    char *ret = malloc(strlen(str)+1);
    if(ret){
        strcpy(ret, str);
    }
    return ret;
}


/*
*Function:
*           flags_init
*
*Description:
*           initializes all data fields
*
*Relavent Arguments:

*Returns :
*           None
*/

void flags_init(icmd * flags){
    
    flags->llport = DEFAULT;                   /* left protocol port number                                           */
    flags->rrport = DEFAULT;                   /* right protocol port number                                          */
    flags->lrport = DEFAULT;
    flags->rlport = DEFAULT;

    flags->position = 0;
    flags->noleft = 0;
    flags->noright = 0;
    flags->output = 1;
    flags->dsplr = 1;                           /* display left to right data, default if no display option provided  */
    flags->dsprl = 0;                           /* display right  to left data                                        */
    flags->reconl = 0;
    flags->display = 0;
    flags->loopr = 0;                           /* take data that comes from the left and send it back to the left    */
    flags->loopl = 0;                           /* take data that comes in from the right and send back to the right  */
    flags->reset = 0;    
    
    bzero(flags->rraddr, sizeof(flags->rraddr));    /* Right connecting address                                       */
    bzero(flags->lraddr, sizeof(flags->rraddr));    /* Left connected address                                         */
    bzero(flags->localaddr, sizeof(flags->rraddr));    /* Local address                                               */
    bzero(flags->connectl, sizeof(flags->connectl));
    bzero(flags->connectr, sizeof(flags->connectr));
    
    bzero(flags->listenl, sizeof(flags->listenl));
    bzero(flags->listenr, sizeof(flags->listenr));
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

void sockettype(char *buf, unsigned char *stype, unsigned char * openld, unsigned char * openrd, int * local, int * remote, icmd * flags, fd_set *masterset, char * sockid){
    
    
    int n = 0;
    int winr, wins;
    int y, x;
    unsigned char loop = 0;
    int * loopdesc;
    unsigned char *open1, *open2;
    char ebufr1[64];
    char ebufr2[64];  
    char ebufs1[64];
    char ebufs2[64];
    
    
    
    if(strcmp(sockid, LEFT) == 0){
        
        winr =  ULW;
        wins = BLW;
        loop = flags->loopl;
        
        getyx(sw[ULW], yul, xul);
        y = yul;
        x = xul;
            
        open1    = openld;
        open2    = openrd;
        loopdesc = local;
        strcpy( ebufr1, "remote left recv error");
        strcpy( ebufr2, "remote left connection closed");
        strcpy( ebufs1,"remote right send error");
        strcpy( ebufs2, "remote right connection closed");
        
        
    }
    else{
        
        winwrite(ERW, "strcmp(sockid, RIGHT) == 0");
        winr =  BRW;
        wins = URW;
        loop = flags->loopr;
        getyx(sw[BRW], ybr, xbr);
        y = ybr;
        x = xbr;
                
        open1    =  openrd;
        open2    =  openld;        
        loopdesc =  local;
        strcpy( ebufr1, "remote right send error");
        strcpy( ebufr2, "remote right connection closed");
        strcpy( ebufs1, "remote left send error");
        strcpy( ebufs2, "remote left connection closed");
    }        
    
    bzero(buf, sizeof(buf));
    n = recv(*local, buf, sizeof(buf), 0);
        
    if (n < 0) {
        *open1 = 0;
        nerror(ebufr1);
    }
    if (n == 0) {
        *open1 = 0;
        nerror(ebufr2);        
        
        /**WARNING: RECONNECTION PSEUDOCODE CASES**/
        // WARNING: GENERALIZE RECONNECIOTN
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
        
    if(buf[0]== 13){
        y++;                    
        x = 0;
    }
    
     
    wmove(sw[winr], y, x);
    wprintw(sw[winr], "%c",buf[0]);
    //scroll(sw[winr]);    
    update_win(winr);
    
    
    
    winwrite(CMW, "e6");
    winwrite(ERW, (char *) loop);
    
    /* Loop data if set*/
    if (loop) {
        winwrite( CMW, " e7.a");
        n = send(*loopdesc, buf, sizeof(buf), 0);        
        winwrite( CMW, " e7.a");
        if (n < 0) {
            *open2 = 0;
            nerror(ebufs1);                        
        }
        if (n == 0) {
            *open2 = 0;
            /*NOTE: persl must be handled differently in Piggy3*/
            nerror(ebufs2);
        }
        
        getyx(sw[wins], y, x);
        if(buf[0]== 13){
            y++;
            x= 0;
        }
        winwrite( CMW, " e7.b");
        wmove(sw[wins], y, x);
        wprintw(sw[wins], "%c",buf[0]);
        //scroll(sw[wins]);
        update_win(wins);
        bzero(buf, sizeof(buf));
        
        winwrite( CMW, " e7");
        
    }        


    /* Check if data needs to be forwarded */    
    if (*open2 && flags->output) {
        n = send(*remote, buf, sizeof(buf), 0);

        if (n < 0) {
            *open2 = 0;
            nerror(ebufs1);                        
        }
        if (n == 0) {
            *open2 = 0;
            /*NOTE: persl must be handled differently in Piggy3*/
            /* Set reconnect flag if persl is set*/
            nerror(ebufs2);
        }
        
        getyx(sw[wins], y, x);
        if(buf[0]== 13){
            y++;
            x= 0;
        }
        wmove(sw[wins], y, x);
        wprintw(sw[wins], "%c",buf[0]);
        //scroll(sw[wins]);
        update_win(wins);
        
        bzero(buf, sizeof(buf));
        
        winwrite( CMW, " e8");
    }
    
    /* Check if output is set to left*/
    /**WARNING SOURCE OF CYCLIC DATA PASSING**/
    /**SET ANOTHER  CONDITION "PASSING VARIABLE" **/
    if ( !flags->output && (flags->position != 2)  && *openld & !stype ) {
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
        winwrite( CMW, " e9");
    }        
    
}
/* End socktype*/