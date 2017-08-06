#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <zconf.h>
#include <curses.h>
#include <locale.h>
#include <arpa/inet.h>

/* Window defintion constants*/

#define ULW 0 /* upper left window      */
#define URW 1 /* upper right window     */
#define BLW 2 /* bottom left window     */
#define BRW 3 /* bottom right window    */
#define CMW 4 /* command window  */
#define INW 5 /* inputs window   */
#define ERW 6 /* errors menu     */


#define DEFAULT 36795 /* default protocol port number, booknumber */
#define QLEN 6          /* size of request queue */
#define MAXSIZE 256
#define NUMWINS 7
#define RES_BUF_SIZE 80

/* Window definitions */
#define inLeft 0    /* Top left window */
#define outRight 1  /* Top right window*/
#define outLeft 2   /* Bottom left window*/
# define inRight 3  /* Bottom right window*/



WINDOW *w[NUMWINS];
WINDOW *sw[NUMWINS];
WINDOW wh[NUMWINS];






struct hostent;
struct sockaddr_in;

typedef struct flags{

    int llport;                 /* Left local port, left side lisenting port    */
    int rrport;                 /* Remote right port                            */
    int lrport;                 /* Left accepting conditional port              */
    int rlport;                 /* Right side listening port                    */

    char connectl;
    char connectr;
    char listenl;
    char listenr;


    unsigned char position;
    unsigned char noleft;
    unsigned char noright;
    unsigned char output; /* output left (0), output right (1)*/
    unsigned char dsplr;
    unsigned char dsprl;
    unsigned char reconl;
    unsigned char display;
    unsigned char dropr;
    unsigned char dropl;
    unsigned char persl;
    unsigned char persr;
    unsigned char loopr;
    unsigned char loopl;
    unsigned char reset;
    char lraddr    [100];
    char rraddr    [100];
    char lladdr    [100];
    char localaddr [100];
    char source    [100]; /* Source file*/
}icmd;


int number(char*);

int max(int, int);

char *strdup(const char *);


char fileRead(const char *, char *[]);

int sock_init(int, int, int, char *, struct sockaddr_in , struct hostent *);

int flagsfunction(icmd *, char *, int , int, int *, int *, int *, int *, struct sockaddr_in, struct sockaddr_in, int inputDesignation);

//void connectopt(icmd *, fd_set *,  int *, int *, int *, int *, struct sockaddr_in, struct hosten *);

#endif