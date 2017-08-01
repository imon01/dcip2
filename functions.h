#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

struct hostent;
struct sockaddr_in;

typedef struct flags{
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
    int llport;
    int rrport;
    char localaddr [100];
    char rraddr[100];   
    char lladdr[100];
    char source[100]; /* Source file*/   
}icmd;


int number(char*);

int max(int, int);

char *strdup(const char *);


char fileRead(const char *, char *[]);

int sock_init(int, int, int, char *, struct sockaddr_in , struct hostent *);

int flagsfunction(icmd *, char *, int , int, int *, int *, int *, int *, struct sockaddr_in, struct sockaddr_in);

//void connectopt(icmd *, fd_set *,  int *, int *, int *, int *, struct sockaddr_in, struct hosten *);

#endif
    
    
 