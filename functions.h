#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

struct hostent;
struct sockaddr_in;
struct commands;

typedef struct flags{
    int position;
    int noleft;
    int noright;
    int outputl;
    int outputr;
    int output;
    int dsplr;
    int dsprl;
    int display;
    int dropr;
    int dropl;
    int persl;
    int persr;
    int source;
    int right;
    int left;
    int loopr;
    int loopl;
    char * rraddr;
    int llport;
    int rrport;

}icmd;




int number(char*);

int sock_init(int, int, int, char *, struct sockaddr_in, struct hostent *);

int max(int, int);

int flagsfunction(icmd, char *, int , int  , int *, int *, struct sockaddr_in, struct sockaddr_in, struct sockaddr_in);



#endif

