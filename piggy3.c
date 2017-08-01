/*
Program:
        Piggy2.c
Authors:
        Shahob Mousavi
        Isaias Mondar

Description:
        Program functins as a middle connector between a client and server end for transfering data.
        User specifies piggy as a head or tail to determine if the pig will accept information either as specified on command line connection, or from a file input.
        If user does not specify as a heard or tail, the pig will act as an intermidiary to connect to a rightmost connection and listen for incoming connections on the left side.

Arguments:
        -noleft: indicates that the pig will act as the head of a connection taking in input from the user to pass along.
            - Only display data going from left to right
            - Increment bozo counter

        -noright: indicates that the pig will act as the tail connection.
            - Only display data going from right to left

        cannot specify both noleft and noright.

        -laddr determines address port for the the server to accept
        -raddr determines what port on the right side return machine to connect to

        -dsplr If pig has left and right side, then only left to right data is displayed
            - default if neither specified
        -dsprl If pig has left and right side, then only right to left data is displayed

        -persl make the left side persistent connection
        -persr make the right side a persistent connection

        -loopr;  take data that comes from the left and send it back to the left
        -loopl;  take data that comes in from the right and send back to the right


        Vi command ordering
            -[Left] local IP:local port:remoteIP:remote port.
            -[Right] remote IP:remote port :local IP:local port.


Note:
        In function "left_init", we get a seg-fault error when we remove the third (last) parameter. So for this piggy1
        assignmnet, we included the third parameter--mostly because we're still learning how to program in C.
*/
#include "functions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <zconf.h>

#include <termios.h>

#include <arpa/inet.h>


#include <net/if.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <locale.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <ctype.h>
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


#define PROTOPORT 36795 /* default protocol port number, booknumber */
#define QLEN 6 /* size of request queue */
#define MAXSIZE 256


/* For Windows OS ?*/
#ifdef WIN32
WSADATA wsaData;
WSAStartup(0x0101, &wsaData);
#endif

extern int errno;
char localhost[] = "localhost"; /* default host name */
const char *DROPL = "REMOTE-LEFT-DROP";
const char *PERSL = "REMOTE-LEFT-CONN";
char *filename = "scriptin.txt"; // set default definition for filename
struct hostent *host; /* pointer to a host table entry */
//struct addrinfo hints, *infoptr; /*used for getting connecting right piggy if give DNS*/
struct sockaddr_in left; /* structure to hold left address */
struct sockaddr_in right; /* structure to hold right address */
struct sockaddr_in lconn; /* structure to hold left connnecting address */

static struct option long_options[] =
        {
                {"s",       optional_argument, NULL, 'a'},
                {"noleft",  optional_argument, NULL, 'l'},
                {"noright", optional_argument, NULL, 'r'},
                {"dsplr",   optional_argument, NULL, 'd'},
                {"dsprl",   optional_argument, NULL, 'e'},
                {"loopr",   optional_argument, NULL, 'f'},
                {"loopl",   optional_argument, NULL, 'g'},
                {"persl",   optional_argument, NULL, 'i'},
                {"persr",   optional_argument, NULL, 'h'},
                {"llport",  optional_argument, NULL, 't'},
                {"rraddr",  required_argument, NULL, 'z'},
                {"rrport",  optional_argument, NULL, 'k'},
                {NULL, 0,                      NULL, 0}
        };


/*
*Function:
*       main
*
*Description:
*       Piggybacking socket connections
*
*
*Returns:
*       Nothing of relevence to program specification
*
*/
int main(int argc, char *argv[]) {


    /***********************************************/
    /* Termios variables and functionalities       */
    /***********************************************/
    static struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    cfmakeraw(&newt);

    /***********************************************/
    /* Use input arguments from loop to set values */
    /***********************************************/
    int i, n, x, len, ch;
    int maxfd;          /* max descriptor                          */
    int pigopt;         /* piggy position indicating variable      */
    int indexptr;       /* generic ponter for getopt_long_only API */
    int desc = -1;      /* left accepted descriptor                */
    int parentrd = -1;  /* main left  descriptors                  */
    int parentld = -1;  /* main right descriptors                  */
    char buf[MAXSIZE];  /* buffer for string the server sends      */
    char *output[MAXSIZE];
    /***********************************************/
    /* Control flow variables                      */
    /***********************************************/
    int openrd = 0;     /* (1) indicates open right connection, otherwise (0)*/
    int openld = 0;     /* (1) indicates open left  connection, otherwise (0)*/

    
    /***********************************************/
    /* Remote and host information variables       */
    /***********************************************/
    struct addrinfo hints, *infoptr; /* used for getting connecting right piggy if give DNS*/
    struct addrinfo *p;
    struct in_addr ip;
    struct hostent *lhost;
    char hostinfo[256];
    char hostname[256];    

    
    /***********************************************/
    /* File Descriptor sets                         */
    /***********************************************/    
    fd_set readset, masterset;    

    
    /***********************************************/
    /* File related variables                      */
    /***********************************************/    
    char *bufCommand = buf;
    char *checker = NULL;
    int readLines;
    int fileRequested = 0;
    char *word2, *end;
    char delimiter[] = " ";
    int readCommandLines;
    int inputLength = 0;

    /*********************************/
    /*  Flag variables init_init     */
    /*********************************/    

    icmd *flags;
    flags = malloc(sizeof(icmd));

    flags->noleft    = 0;
    flags->noright   = 0;
    bzero(flags->rraddr, sizeof(flags->rraddr));    /* Right connecting address                                           */
    bzero(flags->rraddr, sizeof(flags->rraddr));    /* Left connected address                                             */
    bzero(flags->rraddr, sizeof(flags->rraddr));    /* Local address                                                      */
    flags->llport    = PROTOPORT;                   /* left protocol port number                                          */
    flags->rrport    = PROTOPORT;                   /* right protocol port number                                         */
    flags->dsplr     = 1;                           /* display left to right data, default if no display option provided  */   
    flags->dsprl     = 0;                           /* display right  to left data                                        */
    flags->loopr     = 0;                           /* take data that comes from the left and send it back to the left    */
    flags->loopl     = 0;                           /* take data that comes in from the right and send back to the right  */
    flags->output    = 1;



    /*********************************/
    /*  Parsing argv[]               */
    /*********************************/
    while ((ch = getopt_long_only(argc, argv, "a::l::r::d::e::f::g::h::t::k:z:", long_options, &indexptr)) != -1) {        
        switch (ch) {
            case 'a':
                /* read file */
                for (int comm = 0; argv[comm] != '\0'; comm++) {
                    /* Check if filename included */
                    /* check if filename included */
                    if (strstr(argv[comm], ".txt") != NULL) {
                        fileRequested = 1;
                        filename = argv[comm];
                    }
                }

                if (fileRequested) {

                    readLines = fileRead(filename, output);
                    /* read from array and pass into flag function*/
                    for (x = 0; x < readLines; ++x) {
                       // printf("%s\n", output[x]);
                        n = flagsfunction(flags, output[x], sizeof(buf), flags->position, &openld, &openrd, &desc, &parentrd, right, lconn);

                        if (n < 0) {
                            printf("invalid command\n");
                        }
                        /* Discard after being used*/
                        free(output[x]);
                    }

                }
                    /* if none specified read from default filename*/
                else {
                    readLines = fileRead("scriptin.txt", output);
                    /* read from array and pass into flag function  */                    
                    for (x = 0; x < readLines; ++x) {                        
                        n = flagsfunction(flags, output[x], sizeof(buf), flags->position, &openld, &openrd, &desc, &parentrd, right, lconn);

                        if (n < 0) {
                            printf("invalid command\n");
                        }
                        free(output[x]);
                    }
                }
                break;
                
            case 'l':
                openld = 0;
                flags->noleft = 1;                
                break;

            case 'r':
                openrd = 0;
                flags->noright = 2;
                flags->dsplr = 1;
                flags->dsprl = 0; 
                flags->output = 0;
                break;
                
            case 'd': 
                /* Dsplr is default therefore nothing to be done*/                                
                break;

            case 'e':
                flags->dsprl = 1;
                flags->dsplr = 0;                
                break;

            case 'f':
                flags->loopr = 1;
                flags->output = 1;
                break;

            case 'g':
                flags->loopl = 1;
                flags->output = 0;                
                break;
            
            case 'i':
                flags->persl = 1;
                break;
                
            case 'h':
                flags->persr = 1;
                break;
                
            case 't':
                /* Test for illegal value */
                if (number(argv[optind]) > 0) {
                    flags->llport = atoi(argv[optind]);
                } else {
                    printf("left port not a number: %s\n", argv[optind]);
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                if (flags->llport < 0 || flags->llport > 88889) {
                    printf("left port number out of range: %d\n", flags->llport);
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                break;

            case 'k':
                /* Test for illegal value */
                if (number(argv[optind ]) > 0) {
                    flags->rrport = atoi(argv[optind]);
                } else {
                    printf("right port not a number: %s\n", argv[optind - 1]);
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                if (flags->rrport < 0 || flags->rrport > 88889) {
                    printf("right port number out of range: %d\n", flags->rrport);
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }                
                break;

            case 'z':                
                strcpy(flags->rraddr, argv[optind - 1]);

                hints.ai_family = AF_INET;
                n = getaddrinfo(flags->rraddr, NULL, NULL, &infoptr);

                if (n != 0) {
                    printf("rraddr error\n");
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }

                for (p = infoptr; p != NULL; p = p->ai_next) {
                    getnameinfo(p->ai_addr, p->ai_addrlen, hostinfo, sizeof(hostinfo), NULL, 0, NI_NUMERICHOST);
                    strcpy( flags->rraddr,  hostinfo);
                }

                freeaddrinfo(infoptr);
                break;
                
            case '?':
                fprintf(stderr, "invalid option: -%c\n", optopt);
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                return -1;
        }
    }

    
    
    /**************************************************/
    /*  Adjusting program variables and correct flags */
    /**************************************************/
    
    /* If head piggy selected, it requires a right address*/
    if(flags->noleft && ( flags->rraddr[0] == '\0') ){
        printf("Head piggy requires a right address...\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return -1;        
    }
    
    /* Checking for minimum program requirements*/
    flags->position = flags->noleft + flags->noright;
    if (flags->position == 3) {
        printf("Piggy requires at least one connection...\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return -1;
        exit(1);
    }

    /* Checking if display flags are appropiately set*/
    n = flags->dsplr + flags->dsprl;
    if (n == 3) {
        printf("dsplr and dsprl cannot both be set...\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return -1;
        exit(1);
    }
    
    /* Head piggy, set dsprl and clear dsplr*/
    if (flags->noleft == 1) {
        flags->dsplr = 0;
        flags->dsprl = 1;
    }
    
    /* Tail piggy case, set dsplr flag and clear dsprl*/
    if (flags->noright == 2) {
        flags->dsplr == 2;
        flags->dsprl = 0;
    }

    /* A position < 1 implies that the currect piggy is at least*/
    /*  a middle piggy                                          */
    if ((flags->position < 1) & (flags->rraddr[0] == '\0')) {
        printf("Piggy right connection requires right address or DNS...\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
         return -1;
        exit(1);

    }
    

    
    /*********************************/
    /*    Getting local IP address   */
    /*********************************/
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        printf("gethostname, local machine error\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return -1;
    }

    lhost = gethostbyname(hostname);
    if (lhost == NULL) {
        printf("gethostbyname, local machine error\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return -1;
    }

    ip = *(struct in_addr *) lhost->h_addr_list[0];
    strcpy( flags->localaddr, inet_ntoa(ip));
    printf("local address: %s\n", flags->localaddr );



    
    /*********************************/
    /* Descriptor set init           */
    /*********************************/    
    FD_ZERO(&masterset);
    FD_SET(0, &masterset);

    

    /*********************************/
    /*  Piggy setup                  */
    /*********************************/
    switch (flags->position) {

        /*
         * Middle piggy
         */
        case 0:
            

            pigopt = 2;
            parentrd = sock_init( pigopt, 0, flags->rrport, flags->rraddr, right, host);
            if (parentrd < 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }

            pigopt = 1;
            parentld = sock_init(pigopt, QLEN, flags->llport, NULL, left, NULL);
            if (parentld < 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }


            openrd = 1;
            openld = 1;
            FD_SET(parentld, &masterset);
            FD_SET(parentrd, &masterset);            
            break;

        /*
        * Head piggy
        */
        case 1:
            

            pigopt = 2;            
            parentrd = sock_init(pigopt, 0, flags->rrport, flags->rraddr, right, host);

            if (parentrd < 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }

            openrd = 1;
            openld = 0;        
            FD_SET(parentrd, &masterset);
            break;

        /*
        * Tail Piggy
        */
        default:
            

            pigopt = 1;
            parentld = sock_init(pigopt, QLEN, flags->llport, NULL, left, NULL);            

            if (parentld < 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }
            
            openld = 1;
            openrd = 0;            
            FD_SET(parentld, &masterset);
           // printf("One left descriptor added to fd_set, left descriptor \n");
    }
    /*end switch */

    
    /************************************************************/
    /************************************************************/
    /*                          SELECT                          */
    /*                                                          */
    /*  Main loop performing network interaction (@tag s3l)     */
    /*                                                          */
    /************************************************************/
    /************************************************************/

    

            
    /*  maxfd == parentld or maxfd == parentrd               */    
    maxfd = max(parentld, parentrd);
    printf("All required sockets okay...\n");    

    while (1) {
        /* The size of masterset is dynamic throughout runtime */
        memcpy(&readset, &masterset, sizeof(masterset));

        /* Seeing which descriptors are ready*/
        n = select(maxfd + 1, &readset, NULL, NULL, NULL);
        if (n < 0) {
            printf("select error \n");
            break;
        }
        if(n == 0){
            printf("select == %d\n", n );
        }

        /*****************************************************************/
        /* Standard in descriptor ready (@tag:  0FD)                     */        
        /*****************************************************************/
    
        /* Notes:
        *   When creating the socket descriptors,
        *   we already ensured that parentrd > 0,;
        *   therfore don't need addition checks
        *   durring its use.
        */
        if (FD_ISSET(0, &readset)) {
            ch = getchar();

            switch (ch) {
                
                /*
                *   Insert mode
                */
                case 105:
                    if (openrd || openld) {
                        bzero(buf, sizeof(buf));
                        printf("Enter Insert\n");
                        
                        while ( (ch = getchar()) != 27 ) {

                                if(ch != 10){
                                    putchar(ch);
                                    buf[i] = (char) ch;
                                    ++i;
                                }else{
                                    printf("\n");
                                    buf[i] = '\0';
                                    i =0;
                                    /* Preconditions for sending data to the right, output == 1 */
                                    if (flags->output && openrd) {
                                        n = send(parentrd, buf, sizeof(buf), 0);
                                        bzero(buf, sizeof(buf));
                                        
                                        if (n < 0) {
                                            printf("right send error");
                                            break;
                                        }
                                        if (n == 0) {
                                            /* Here, if persr is set, we will attempt*/
                                            /*  reestablish the connection           */
                                            flags->reconl = 1;                                        
                                            break;
                                        }
                                        
                                    }
                                    
                                    /* Preconditions for sending data to the left, output == 0 */
                                    else if (!flags->output && openld ) {
                                        n = send(desc, buf, sizeof(buf), 0);
                                        bzero(buf, sizeof(buf));

                                        if (n < 0) {
                                            printf("left send error \n");
                                            break;
                                        }
                                        if(n == 0 && flags->persl) {
                                            break;
                                        }
                                        
                                    }
                                    else{
                                        if(!openld & !flags->output){
                                            printf("left connection closed\n");
                                        }
                                        if( !openrd & flags->output == 1){
                                            printf("left connection closed\n");
                                        }
                                    }                                    
                                }                                
                        }/* End input loop*/
                    }/* End of at least one socket is open*/ 
                    else {
                        printf("no open sockets..\n");
                    }
                    break;
                    
                /* 
                 *  Quitting
                 */
                case 113:
                    bzero(buf, sizeof(buf));
                    printf("exiting\n");
                    FD_ZERO(&masterset);
/*                    

                    if(FD_ISSET(desc, &masterset)){
                        shutdown(desc,2);
                    }
                    
                    if(FD_ISSET(parentld, &masterset)){
                        shutdown(parentld,1);
                    }                    
                    
                    if(FD_ISSET(parentrd, &masterset)){
                        shutdown(parentrd,2);
                    }*/
                    
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return 1;
                
                /* 
                 * Interactive commands
                 */
                case 58:
                    bzero(buf, sizeof(buf));
                    i = 0;
                    putchar(':');
                    printf("Enter command mode\n");

                    while (1) {
                        ch = getchar();
                        
                        /* Checking for return character */
                        if (ch == 10) {
                            break;
                        } else {
                            buf[i++] = ch;
                            putchar(ch);
                        }
                    }
                    
                    printf("\n");
                    inputLength = strlen(buf);
                    char *inputCopy = (char *) calloc(inputLength + 1, sizeof(char));
                    
                    checker = strstr(buf, "source");
                    if (checker == buf) {                        
                            strncpy(inputCopy, buf, inputLength);
                            strtok_r(inputCopy, delimiter, &end);
                            word2 = strtok_r(NULL, delimiter, &end);

                            /* Get commands from fileread*/
                            readCommandLines = fileRead(word2, output);

                            /* Read from array and pass into flagfunction */
                            for (x = 0; x < readCommandLines; ++x) {
                                printf("%s\n", output[x]);
                                n = flagsfunction(flags, output[x], sizeof(buf), flags->position, &openld, &openrd, &desc, &parentrd, lconn, right);
                                

                                switch (n) {
                                    /*
                                    *  valid command
                                    */
                                    case 1:
                                        break;

                                    /*
                                    *  persl
                                    */
                                    case 2:
                                        if ( flags->position != 1 ) {                                                                                
                                            bzero(buf, sizeof(buf));
                                            strcpy(buf, PERSL);
                                            n = send(desc, buf, sizeof(buf), 0);
                                                                                    
                                            if (n < 0) {                                        
                                                openld = 0;
                                            }else{
                                                FD_SET(desc, &masterset);

                                                printf("connection established\n");
                                                openld = 1;
                                            }
                                        }
                                        bzero(buf, sizeof(buf));
                                        break;

                                    /*
                                    *  persr, make reconnection if necessary
                                    */
                                    case 3:
                                        if (flags->position < 2 && !FD_ISSET(parentrd, &masterset)) {
                                            printf("right side reconnecting..\n ");
                                            pigopt = 2;
                                            parentrd = sock_init(pigopt, 0, flags->rrport, flags->rraddr, right, host);

                                            if (parentrd > 0) {

                                                printf("connection restablished\n");
                                                openrd = 1;
                                                openld = 0;                                                                            
                                                maxfd = max(desc, parentld);
                                                maxfd = max(maxfd, parentrd);
                                                FD_SET(parentrd, &masterset);                                        
                                            } else {
                                                flags->persr = 2;
                                            }
                                        }
                                        break;

                                    /*
                                    *  dropl
                                    */
                                    case 4:
                                        
                                        /*
                                        * Notes:
                                        *   -valid of piggies with postion 0 & 2
                                        *   -Openld closed
                                        *   -dropl behavior: message sent to connecting piggy
                                        *   -clear output left
                                        */
                                        if (desc > 0) {
                                            bzero(buf, sizeof(buf));                                     
                                            strcpy(buf, DROPL);
                                            printf("dropl %s\n", buf);
                                            n = send(desc, buf, sizeof(buf), 0);
                                            openld = 0;
                                            if (n < 0) {
                                                continue;
                                            }
        //                                    FD_CLR(desc, &masterset);
                                        }
                                        bzero(buf, sizeof(buf));
                                        break;

                                    /* 
                                    *  dropr
                                    */
                                    case 5:
                                        /* parentrd socket already closed in flagsfunction*/
                                        if (parentrd > 0) {
                                            FD_CLR(parentrd, &masterset);
                                        }
                                        break;

                                    default:
                                        printf("invalid command\n");
                            }/* End switch case*/
                            /* ???? */
                            break;
                        }
                    }
                    /* End reading commands from bufCommand*/
                    else {
                        n = flagsfunction(flags, buf, sizeof(buf), flags->position, &openld, &openrd, &desc, &parentrd, lconn, right);

                        switch (n) {
                            /*
                            *  valid command
                            */
                            case 1:
                                break;

                            /*
                            *  persl
                            */
                            case 2:
                                if ( flags->position != 1 ) {                                                                                
                                    bzero(buf, sizeof(buf));
                                    strcpy(buf, PERSL);
                                    n = send(desc, buf, sizeof(buf), 0);
                                                                            
                                    if (n < 0) {                                        
                                        openld = 0;
                                    }else{
                                        FD_SET(desc, &masterset);

                                        printf("connection established\n");
                                        openld = 1;
                                    }
                                }
                                bzero(buf, sizeof(buf));
                                break;

                            /*
                            *  persr, make reconnection if necessary
                            */
                            case 3:
                                if (flags->position < 2 && !FD_ISSET(parentrd, &masterset)) {
                                    printf("right side reconnecting..\n ");
                                    pigopt = 2;
                                    parentrd = sock_init(pigopt, 0, flags->rrport, flags->rraddr, right, host);

                                    if (parentrd > 0) {

                                        printf("connection restablished\n");
                                        openrd = 1;
                                        openld = 0;                                                                            
                                        maxfd = max(desc, parentld);
                                        maxfd = max(maxfd, parentrd);
                                        FD_SET(parentrd, &masterset);                                        
                                    } else {
                                        flags->persr = 2;
                                    }
                                }
                                break;

                            /*
                            *  dropl
                            */
                            case 4:
                                
                                /*
                                * Notes:
                                *   -valid of piggies with postion 0 & 2
                                *   -Openld closed
                                *   -dropl behavior: message sent to connecting piggy
                                *   -clear output left
                                */
                                if (desc > 0) {
                                    bzero(buf, sizeof(buf));                                     
                                    strcpy(buf, DROPL);
                                    printf("dropl %s\n", buf);
                                    n = send(desc, buf, sizeof(buf), 0);
                                    openld = 0;
                                    if (n < 0) {
                                        continue;
                                    }
//                                    FD_CLR(desc, &masterset);
                                }
                                bzero(buf, sizeof(buf));
                                break;

                            /* 
                            *  dropr
                            */
                            case 5:
                                /* parentrd socket already closed in flagsfunction*/
                                if (parentrd > 0) {
                                    FD_CLR(parentrd, &masterset);
                                }
                                break;

                            default:
                                printf("invalid command\n");
                    }/* End switch case*/
                    break;
                }/* End else single interactive command string*/
            }/* End stdin descriptor switch*/
        }/*End stdin */


        /*
         * LEFT SIDE ACCEPT DESCRIPTOR
         * LDA
         * 
         * 
         * 
         * 
         * Notes:
         *  Accept incoming left connection
         */
        if (FD_ISSET(parentld, &readset)) {
            len = sizeof(lconn);
            desc = accept(parentld, (struct sockaddr *) &lconn, &len);
            
            if (desc < 0) {
                printf("left accept error\n");
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }
                        
            
 	    flags->llport = (int) ntohs(lconn.sin_port);
	    strcpy( flags->lladdr, inet_ntoa(lconn.sin_addr));
            
            maxfd = max(maxfd, desc);
            FD_SET(desc, &masterset);                        
            printf("connection established (%s : %hu)\n", flags->lladdr, flags->llport);
            
        }

        /*****************************************************************/
        /* Left Listeing Descriptor (@tag: LLD)                          */
        /*****************************************************************/
        
        
        /* Notes:
        *   When creating the socket descriptors, we
        *   already ensured that desc >0; therfore
        *   don't need addition checks during its use.
        *   Also only the tail and middle piggies have
        *   left connections; therefore, we can assuredly
        *   use position with desc.
        */
        if (FD_ISSET(desc, &readset)) {
            bzero(buf, sizeof(buf));
            n = recv(desc, buf, sizeof(buf), 0);
            
            if (n < 0) {
                printf("recv left error \n");
                break;
            }
            if (n == 0) {                
                FD_CLR(desc, &masterset);
                openld = 0;
                break;
            }

    
            /* If dsplr is set we print data coming frm the right*/
            if (flags->dsplr) {
                printf("%s\n", buf);
            }
            
            /* Loop data right if set*/
            if (flags->loopr && openld){
                n = send(desc, buf, sizeof(buf), 0);

                if (n < 0) {
                    printf("-||---send left error\n");                    
                }
                if (n == 0) {
                    /* Set reconnect flag if persl is set*/
                    if (flags->persl) {
                        flags->reconl = 1;
                    }                    
                }                
                bzero(buf, sizeof(buf));
            }

            /* Check if data needs to be forwarded */
            if (openrd && flags->output ) {
                n = send(parentrd, buf, sizeof(buf), 0);

                if (n < 0) {
                    openrd = 0;
                    printf("~.~.~.~send right error \n");                    
                }
                if (n == 0) {
                    openrd = 0;
                    /* Set reconnect flag if persl is set*/
                    if (flags->persr) {
                        flags->persr =  2;
                    }
                }
                bzero(buf, sizeof(buf));
            }
            
            /* Check if output is set to left*/
            if(openld && !flags->output){
                n = send(desc, buf, sizeof(buf), 0);

                if (n < 0) {
                    openld = 0;
                    printf("<>.<>.<> DESC send left error \n");
                }
                if (n == 0) {
                    openld = 0;
                    /* Set reconnect flag if persl is set*/
                    if (flags->persl) {
                        flags->reconl = 1;
                    }
                }
                bzero(buf, sizeof(buf));
            }
            
        }
        /* End ready  desc*/


        /*****************************************************************/
        /* RIGHT SIDE DESCRIPTROR (@tag: RDD)                            */
        /*****************************************************************/
        
         
        /*
        * 
        * Notes:
        *   FD_ISSET will be true for the head
        *   piggy and middle piggies, therfore we
        *   don't check for -noright,
        */
        if (FD_ISSET(parentrd, &readset)) {
            bzero(buf, sizeof(buf));            
            n = recv(parentrd, buf, sizeof(buf), 0);            
            
            if (n < 0) {
                openrd = 0;
                printf("right recv right error\n");
                break;
            }            
            if (n == 0) {
                openrd = 0;
                    /* Reestablishing done at end of loop*/
                    flags->persr = 2;
                
                break;
            }
            
            /* Check for constant DROPL string*/
            if ( strcmp(buf, DROPL ) == 0 ) {
                printf("remote right side dropn\n");
                openrd = 0;
            } 
            /* Check for constant PERSL string*/
            else if ( strcmp(buf, PERSL)  == 0 ) {
                printf("remote right side reconnection\n");
                openrd = 1;                
            }
            else {
                /* If dsprl is set we print data coming from the right*/                
                if (flags->dsprl) {
                    printf("%s\n", buf);
                }

                if (flags->loopl == 1) {
                    n = send(parentrd, buf, sizeof(buf), 0);
                    if (n < 0) {
                        printf("send right error\n");
                        break;
                    }
                    if (n == 0) {
                        flags->persr = 2;                        
                        break;
                    }
                    bzero(buf, sizeof(buf));
                }

                /* Data only left forwarded if middle piggy */
                if(openld && !flags->output) {
                    n = send(desc, buf, sizeof(buf), 0);
                    if (n < 0) {
                        openld = 0;
                        printf("send left error\n");
                        break;
                    }

                    if (n == 0) {
                        flags->persl = 2;                        
                        break;
                    }
                }
                bzero(buf, sizeof(buf));
            }
        }/* End ready parentrd*/

        
        
        /*****************************************/
        /* Left and right reconnection if set    */
        /*****************************************/
        
        /* 
        *Notes:
        *  -Try right reconnection if unsuccessful
        *  -persr is only ever set to 2 when a reconnect
        *      fails after calling flagsfunction
        */
        if (flags->persr == 2) {
            printf("right side reconnecting..\n ");
            pigopt = 2;
            parentrd = sock_init(pigopt, 0, flags->rrport, flags->rraddr, right, host);
            
            if (parentrd > 0) {
                flags->persr = 1;
                openrd = 1;                
                maxfd = max(desc, parentld);
                maxfd = max(maxfd, parentrd);
                FD_SET(parentrd, &masterset);
            }
        }
        
        /* Dummy condition for persl "reconnection" */
        if (flags->reconl) {
            printf("left side reconnecting attempt...\n ");
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 0;
}/*end main*/
