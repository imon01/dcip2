/*
Program:
        Piggy3.c
Authors:
        Shahob Mousavi
        Isaias Mondar

Description:
        Program functions as a middle connector between a client and server end for transfering data.
        User specifies piggy as a head or tail to determine if the pig will accept information either as specified on
        command line connection, or from a file input.
        If user does not specify as a heard or tail, the pig will act as an intermidiary to connect to a rightmost
        connection and listen for incoming connections on the left side.
        Piggy3 makes use of multiple gui windows to display data moving from left to right and vice versa.

Arguments:

    Command Line Parameters

        -rraddr value:
            This is used to specify the address of the right side, i.e. the node you should connect to.
            Unlike the lraddr option “*” is never valid for an rraddr.
            Also, note that there is no default IP address for the right side.
            A rraddr must always be specified unless -noright is given.

        -noleft:
            Head. No incoming left side connection. If -noleft is given then any -lraddr option is ignored.

        -noright:
            Indicates that the pig will act as the tail connection.
            If -noright is given then any -rraddr option is ignored.
            Cannot specify both noleft and noright.

        -llport [value]:
            What local port address should be used for the left side connection. This is the port you listen on.
            Port you give to bind before you call listen.

        -rrport [value]:
            The port address you should connect to on the node being connected to on the right side.

        -rlport [value]:
            The port to bind to on the computer piggy is running on for making the right side connection.
            The source port for the right side connection.

        -loopr:
            When looping on the right side we should see the “looped” data leaving from the upper right box
            and also see it entering the lower right box.
            Take the data that would be written out to the right side and
            inject it into the data stream arriving from the right side.

        -loopl:
            When looping on the left side we should see the “looped” data leaving from the lower left box
            and also see it entering the uppper left box.
            Take the data that would be written out to the left side and
            inject it into the data stream arriving from the left side.
            */
/*


    Interactive Commands

        i: Enter insert mode. Program should start in command mode.

        esc: Exit insert mode and return to command mode. Esc entered while in command mode is ignored.

        outputl: Set the output direction to left. Determines where data typed from the keyboard in input mode is sent.
        outputr: Set the output direction to right. Determines where data typed from the keyboard in input mode is sent.

        output: Show what direction the output is set to (left of right).

        left: Show the currently connected “tcp pair” for the left side.
        right: Show the currently connected “tcp pair” for the right side.
            Command ordering
            [left] local IP:local port:remoteIP:remote port.
            [right] remote IP:remote port :local IP:local port.

        loopr: When looping on the right side we should see the “looped” data leaving from the upper right box and also see it entering the lower right box.
        loopl: When looping on the left side we should see the “looped” data leaving from the lower left box and also see it entering the uppper left box.

        dropr: Drop right side connection.
        dropl: Drop left side connection.

*       connectr IP [port]:
            Create a connection to computer with “IP” on their tcp port
            “port” for your “right side” If a port is not specified the current
            value of port for the remote port on the right is used. This may
            have been specified on the command line or may have been
            established via an interactive command. If it has never been set
            than use the default port.
*       connectl: IP [port]:
            create a connection to computer with “IP” on their tcp port
            “port” for your “left side.”

*       listenl: [port]: Use for left side listen for a connection on your local port port. Use default if no port given.
*       listenr [port]: Use for right side listen for a connection on your local port port. Use default if no port given.

        read: filename: Read the contents of file “filename” and write it to the current output direction.

*       llport [port]: Bind to local port “port” for a left side connection.
*       rlport [port]: Bind to local port “port” for a right side connection.
*       lrport [port]: Accept a connection on the left side only if the remote computer attempting to connect has source port “port”.

        lraddr [IP]:
            When the left is put into passive mode (via a listenl command)
            accept a connection on the left side only if the remote computer
            attempting to connect has IP address “IP” If the left is placed in
            active mode (trying to connect) use this as the address to connect to.
        rraddr [IP]:
            If the right is set to passive mode to accept a connection on the
            right side, allow it only if the remote computer attempting to
            connect has IP address “IP”. If the right is placed in active mode
            (trying to make a connection) use this as the address to connect to.

*       reset:
            Act as if the program is starting up from the beginning again and
            do whatever was requested by the command line parameters. Any
            active connections or passive opens are dropped and reset. The
            screen should be cleared and redrawn. The program
            should be in exactly the same state it was in at startup after
            processing and acting on the command line parameters.

        q: Terminate the program. When quitting the program be sure to set
           the terminal back into the state it was in before your piggy started
           using the endwin call in ncurses.

        persl: make the left side persistent connection
        persr: make the right side a persistent connection


        Structure:
            declerations
            main
            setup ncurses
            create sockets

*/

#include "functions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <zconf.h>
#include <curses.h>
#include <locale.h>
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

/* For Windows OS*/
#ifdef WIN32
WSADATA wsaData;
WSAStartup(0x0101, &wsaData);
#endif


#define PROTOPORT 36795 /* default protocol port number, booknumber */
#define QLEN 6          /* size of request queue */
#define MAXSIZE 256
#define NUMWINS 7
#define RES_BUF_SIZE 80

/* Window definitions */
#define inLeft 0    /* Top left window */
#define outRight 1  /* Top right window*/
#define outLeft 2   /* Bottom left window*/
# define inRight 3  /* Bottom right window*/


extern int errno;
char localhost[] = "localhost";         /* default host name */
const char *DROPL = "REMOTE-LEFT-DROP";
const char *PERSL = "REMOTE-LEFT-CONN";
char *filename = "scriptin.txt";        /* set default definition for filename */
struct hostent *host;                   /* pointer to a host table entry */
struct sockaddr_in left;                /* structure to hold left address */
struct sockaddr_in right;               /* structure to hold right address */
struct sockaddr_in lconn;               /* structure to hold left connnecting address */
WINDOW *w[NUMWINS];
WINDOW *sw[NUMWINS];
WINDOW wh[NUMWINS];

void update_win(int i) {
    touchwin(w[i]);
    wrefresh(sw[i]);
}


/* add string to a window */
void wAddstr(int z, char c[255]);


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


void GUIshutdown(char * response) {
    wmove(sw[4], 0, 0);
    wclrtoeol(sw[4]);
    wprintw(sw[4], "All finished. Press Enter to terminate the program.");
    update_win(4);
    wgetstr(sw[4], response);
    /* End screen updating */
    endwin();
    echo();
}

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
    char *inputCopy;
    
    /***********************************************/
    /* Ncurses windows variables                   */
    /***********************************************/
    
    int a;
    char response[RES_BUF_SIZE];
    int WPOS[NUMWINS][4] = {{16, 66,  0,  0},
                            {16, 66,  0,  66},
                            {16, 66,  16, 0},
                            {16, 66,  16, 66},
                            {3,  132, 32, 0},
                            {5,  132, 35, 0},
                            {3,  132, 40, 0}};    


    icmd *flags;
    flags = malloc(sizeof(icmd));

    flags->noleft = 0;
    flags->noright = 0;
    flags->rraddr = NULL;      /* hold addresses of left and right connect IP adresses */
    flags->llport = PROTOPORT; /* left protocol port number */
    flags->rrport = PROTOPORT; /* right protocol port number */
    flags->dsplr = 1;          /* display left to right data, default if no display option provided */\
    flags->dsprl = 0;          /* display right  to left data */
    flags->loopr = 0;          /* take data that comes from the left and send it back to the left */
    flags->loopl = 0;          /* take data that comes in from the right and send back to the right */
    flags->output = 1;
    flags->reset  = 0;

    /* setup ncurses for multiple windows */
    setlocale(LC_ALL, ""); // this has to do with the character set to use
    initscr();
    cbreak(); 
    noecho(); 
    nonl(); 

    intrflush(stdscr, FALSE); 
    keypad(stdscr, TRUE); 

    /* Clear screen before starting */
    clear();
    w[0] = newwin(0,0,0,0);


    if (LINES != 43 || COLS != 132) // we don't want to have to do arithmetic to figure out
        // where to put things on other size screens
    {
        move(0, 0);
        addstr("Piggy3 requires a screen size of 132 columns and 43 rows");
        move(1, 0);
        addstr("Set screen size to 132 by 43 and try again");
        move(2, 0);
        addstr("Press enter to terminate program");
        move(3,0);
        wprintf("%dx%d\n", COLS, LINES);
        refresh();
        getstr(response); // Pause so we can see the screen
        endwin();
        exit(EXIT_FAILURE);
    }

    // create the 7 windows and the seven subwindows
    for (a = 0; a < NUMWINS; a++) {
        w[a] = newwin(WPOS[a][0], WPOS[a][1], WPOS[a][2], WPOS[a][3]);
        sw[a] = subwin(w[a], WPOS[a][0] - 2, WPOS[a][1] - 2, WPOS[a][2] + 1, WPOS[a][3] + 1);
        scrollok(sw[a], TRUE); // allows window to be automatically scrolled
        wborder(w[a], 0, 0, 0, 0, 0, 0, 0, 0);
        touchwin(w[a]);
        wrefresh(w[a]);
        wrefresh(sw[a]);
    }
    // Write some stuff to the windows

    /* sw[0] = upper left window, sw[1] = upper right window, sw[2] = bottom left window, sw[3] = bottom right window */
    /* sw[4] = bottom command window, sw[5] = bottom inputs window, sw[7] = bottom errors menu. */

    wmove(sw[0], 0, 0);
    wprintw(sw[0], "This is the window where the data flowing from left to right ");
    wprintw(sw[0], "will be displayed. Notice now we don't need to worry about when we reach the last ");
    wprintw(sw[0], " position in the subwindow, we will just wrap around down to the next line of the subwindow");
    wprintw(sw[0], " and can never mess up the border that is in the parent window");

    wprintw(sw[inLeft], " Data coming from the left, head piggy\n");


    wmove(sw[1], 4, 0);
    waddstr(sw[1], "Data leaving right side");
    wmove(sw[2], 4, 0);
    waddstr(sw[2], "Data leaving the left side");
    wmove(sw[3], 4, 0);
    waddstr(sw[3], "Data arriving from the right");
    wmove(sw[4], 0, 0);
    waddstr(sw[4], "Commands: ");
    wmove(sw[5], 0, 0);
    waddstr(sw[5], "Data Entry: ");
    wmove(sw[6], 0, 0);
    waddstr(sw[6], "Errors: ");

    for (int a = 0; a < NUMWINS; a++) update_win(a);
    // Place cursor at top corner of window 5
    wmove(sw[4], 0, 0);
    wprintw(sw[4], "Press Enter to see the output in the upper left window scroll");
    wgetstr(sw[4], response); // Pause so we can see the screen
    wmove(sw[4], 0, 0);
    wclrtoeol(sw[4]); // clears current line without clobbering borders
    //wprintw(sw[4], "sleeping between each line");
    update_win(4);


    /*********************************/
    /*    Getting local IP address   */
    /*********************************/
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        printf("gethostname, local machine error\n");
        endwin();
        return -1;
    }

    lhost = gethostbyname(hostname);
    if (lhost == NULL) {
        printf("gethostbyname, local machine error\n");
        endwin();
        return -1;
    }

    ip = *(struct in_addr *) lhost->h_addr_list[0];
    flags->localaddr = inet_ntoa(ip);
        

    waddstr(sw[4], "local address ");
    waddstr(sw[4], flags->localaddr);
    update_win(4);




    /*********************************/
    /*  Parsing argv[]               */
    /*********************************/
    
    while ((ch = getopt_long_only(argc, argv, "a::l::r::d::e::f::g::h::t::k:z:", long_options, &indexptr)) != -1) {
        switch (ch) {
            case 'a':
                /* read file */
                for (int comm = 0; argv[comm] != '\0'; comm++) {                    
                    if (strstr(argv[comm], ".txt") != NULL) {
                        fileRequested = 1;
                        filename = argv[comm];
                    }
                }

                if (fileRequested) {
                    readLines = fileRead(filename, output);
                    
                    /* read from array and pass into flag function*/
                    for (x = 0; x < readLines; ++x) {                        
                        n = flagsfunction(flags, output[x], sizeof(buf), flags->position, &openld, &openrd, &desc, &parentrd, right, lconn);

                        if (n < 0) {
                            printf(" invalid command ");
                        }                        
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
                            printf(" invalid command ");
                        }
                        free(output[x]);
                    }
                }
                break;
            case 'l':
                openld = 0;
                flags->noleft = 1;                
                waddstr(sw[4], "noleft ");
                update_win(4);
                break;

            case 'r':
                openrd = 0;
                flags->noright = 2;
                flags->dsplr = 0;
                flags->dsprl = 1;                
                waddstr(sw[4], " noRight ");
                update_win(4);
                break;
            case 'd':
                waddstr(sw[4], "dsplr ");
                update_win(4);
                break;

            case 'e':
                flags->dsprl = 1;
                flags->dsplr = 0;
                waddstr(sw[4], "dsprl ");
                update_win(4);
                break;

            case 'f':
                flags->loopr = 1;
                flags->output = 1;                
                waddstr(sw[4], "loopr ");
                update_win(4);
                break;

            case 'g':
                flags->loopl = 1;
                flags->output = 0;                
                waddstr(sw[4], "loopl ");
                update_win(4);
                break;

            case 'i':
                flags->persl = 1;
                break;

            case 'h':
                flags->persr = 1;
                waddstr(sw[4], "persr ");
                update_win(4);

            case 't':
                if (number(argv[optind]) > 0) {
                    flags->llport = atoi(argv[optind]);
                } else {
                    waddstr(sw[6]," left port not a number");
                    update_win(6);
                    endwin();
                    return -1;
                }
                if (flags->llport < 0 || flags->llport > 88889) {
                    waddstr(sw[6]," left port number out of range");
                    update_win(6);
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                break;

            case 'k':
                if (number(argv[optind - 1]) > 0) {
                    flags->rrport = atoi(argv[optind]);
                } else {
                    waddstr(sw[6]," right port not a number ");
                    waddstr(sw[6], argv[optind]);
                    update_win(6);
                    endwin();
                    return -1;
                }
                if (flags->rrport < 0 || flags->rrport > 88889) {                    
                    waddstr(sw[6]," right port number out of range");
                    update_win(6);
                    endwin();
                    return -1;
                }
                /* test for illegal value */
                break;

            case 'z':                
                waddstr(sw[5],"right addrs parse...");
                update_win(5);

                flags->rraddr = argv[optind - 1];

                hints.ai_family = AF_INET;

                n = getaddrinfo(flags->rraddr, NULL, NULL, &infoptr);

                if (n != 0) {
                    waddstr(sw[6]," rraddr error");
                    update_win(6);                
                    return -1;
                }

                for (p = infoptr; p != NULL; p = p->ai_next) {
                    getnameinfo(p->ai_addr, p->ai_addrlen, hostinfo, sizeof(hostinfo), NULL, 0, NI_NUMERICHOST);
                    flags->rraddr = hostinfo;
                }

                freeaddrinfo(infoptr);
                break;

            case '?':
                //fprintf(stderr, "invalid option: -%c\n", optopt);
                // print into error reporting sw[6]
                waddstr(sw[6],"No valid command");
                update_win(6);
                GUIshutdown(response);
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                return -1;
        }
    }
    /* end switch statement */


    /**************************************************/
    /*  Adjusting program variables and correct flags */
    /**************************************************/

    /* If head piggy selected, it requires a right address*/
    if(flags->noleft && ( flags->rraddr[0] == '0') ){
        //printf("Head piggy requires a right address...\n");
        waddstr(sw[6],"Head piggy requires a right address...");
        update_win(6);
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return -1;
    }

    /* Checking for minimum program requirements*/
    flags->position = flags->noleft + flags->noright;
    if (flags->position == 3) {
        //printf("Piggy requires at least one connection...\n");
        waddstr(sw[6],"Piggy requires at least one connection...");
        update_win(6);
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return -1;
        exit(1);
    }

    /* Checking if display flags are appropiately set*/
    n = flags->dsplr + flags->dsprl;
    if (n == 3) {
        //printf("dsplr and dsprl cannot both be set...\n");
        waddstr(sw[6],"dsplr and dsprl cannot both be set...");
        update_win(6);
        endwin();        
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
    if ((flags->position < 1) & (flags->rraddr == NULL)) {
        //printf("Piggy right connection requires right address or DNS...\n");
        waddstr(sw[6],"Piggy right connection requires right address or DNS...");
        update_win(6);
        endwin();        
        exit(1);

    }

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
                endwin();
                exit(1);
            }

            pigopt = 1;
            parentld = sock_init(pigopt, QLEN, flags->llport, NULL, left, NULL);
            if (parentld < 0) {
                endwin();
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


    /* Since the piggy position is at least 0 and less than 3*/
    /*  maxfd == parentld or maxfd == parentrd               */
    maxfd = max(parentld, parentrd);
    //printf("All required sockets okay...\n");
    waddstr(sw[5],"All required sockets ok ");
    update_win(5);
    //printf("(left %d, desc %d, right %d, maxfd %d)\n", parentld, desc, parentrd, maxfd);

    while (1) {
        memcpy(&readset, &masterset, sizeof(masterset));

        /**/
        n = select(maxfd + 1, &readset, NULL, NULL, NULL);
        if (n < 0) {
            //printf("select error \n");
            waddstr(sw[6],"select error ");
            update_win(6);
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
            //ch = getchar();
            ch = wgetch(w[5]);
            wrefresh(w[5]);

            switch(ch){
                /*i*/

                case 105:
                    if(openrd || openld){
                        bzero(buf, sizeof(buf));
                        wrefresh(w[5]);
                        update_win(5);
                        //printf("Enter Insert\n");
                        werase(w[5]);
                        wmove(sw[5], 0, 0);
                        waddstr(sw[5],"Enter Insert ");
                        update_win(5);
                        nocbreak();
                        echo();

                        while ( (ch = getchar()) != 27 ) {

                            if(ch != 10){
                                putchar(ch);
                                buf[i] = (char) ch;
                                ++i;

                                nocbreak();
                                echo();


                            }else{
                                waddstr(sw[5],"\n");
                                update_win(5);
                                buf[i] = '\0';
                                i =0;
                                /* Preconditions for sending data to the right, output == 1 */
                                if (flags->output && openrd) {
                                    n = send(parentrd, buf, sizeof(buf), 0);
                                    bzero(buf, sizeof(buf));

                                    if (n < 0) {
                                        waddstr(sw[6],"right send error ");
                                        update_win(6);
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
                                        waddstr(sw[6],"left send error ");
                                        update_win(6);
                                        break;
                                    }
                                    if(n == 0 && flags->persl) {
                                        break;
                                    }
                                    //printf("1 left send\n");
                                    waddstr(sw[5],"1 left send ");
                                    update_win(5);
                                }
                                else{
                                    if(!openld & !flags->output){
                                        //printf("left connection closed\n");
                                        waddstr(sw[6],"left connection closed ");
                                        update_win(6);
                                    }
                                    if( !openrd & flags->output == 1){
                                        //printf("left connection closed\n");
                                        waddstr(sw[6],"left connection closed ");
                                        update_win(6);
                                    }
                                }
                            }
                        }/* End input loop*/
                    }/* End of at least one socket is open*/
                    else {
                        //printf("no open sockets..\n");
                        waddstr(sw[6],"no open sockets ");
                        update_win(6);
                    }
                    break;

                    /*
                     *  Quitting
                     */
                case 113:
                    bzero(buf, sizeof(buf));
                    //printf("exiting\n");
                    waddstr(sw[5],"exiting ");
                    update_win(5);
                    GUIshutdown(response);
                    FD_ZERO(&masterset);


                    if(FD_ISSET(desc, &masterset)){
                        shutdown(desc,2);
                    }

                    if(FD_ISSET(parentld, &masterset)){
                        shutdown(parentld,1);
                    }

                    if(FD_ISSET(parentrd, &masterset)){
                        shutdown(parentrd,2);
                    }
                    
                    endwin();
                    return 1;

                    /*
                     * Interactive commands
                     */
                case 58:
                    bzero(buf, sizeof(buf));
                    i = 0;
                    putchar(':');
                    move(0,0);
                    
                    waddstr(sw[4],"Enter Command mode ");
                    update_win(4);
                    nocbreak();
                    //echo();
                    //mvwaddch(w[5],wh[5]-1,1,":");
                    while (1) {
                        ch = getchar();
                        if (ch == 10) {
                            break;
                        } else {
                            buf[i++] = ch;
                            putchar(ch);
                            waddch(w[4], ch);
                        }
                    }
                    cbreak();
                    noecho();


                    inputLength = strlen(buf);
                    inputCopy = (char *) calloc(inputLength + 1, sizeof(char));

                    checker = strstr(bufCommand, "source");
                    if (checker == bufCommand) {
                        strncpy(inputCopy, buf, inputLength);
                        strtok_r(inputCopy, delimiter, &end);
                        word2 = strtok_r(NULL, delimiter, &end);

                        /* Get commands from fileread*/
                        readCommandLines = fileRead(word2, output);

                        /* Read from array and pass into flagfunction */
                        for (x = 0; x < readCommandLines; ++x) {
                            
                            waddstr(w[4], output[x]);
                            update_win(4);

                            n = flagsfunction(flags, output[x], sizeof(buf), flags->position, &openld, &openrd, &desc,
                                              &parentrd, lconn, right);

                            switch (n) {

                                /* valid command*/
                                case 1:
                                    break;

                                    /* persl*/
                                case 2:
                                    if ( flags->position != 1 ) {
                                        bzero(buf, sizeof(buf));
                                        strcpy(buf, PERSL);
                                        n = send(desc, buf, sizeof(buf), 0);

                                        if (n < 0) {
                                            openld = 0;
                                        }else{
                                            FD_SET(desc, &masterset);
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
                                        //printf("right side reconnecting.. ");
                                        waddstr(w[5], "right side reconnecting...");
                                        update_win(5);
                                        pigopt = 2;
                                        parentrd = sock_init(pigopt, 0, flags->rrport, flags->rraddr, right, host);

                                        if (parentrd > 0) {

                                            printf("connection restablished\n");
                                            waddstr(w[5], "connection restablished");
                                            update_win(5);
                                            openrd = 1;
                                            openld = 0;
                                            maxfd = max(desc, parentld);
                                            maxfd = max(maxfd, parentrd);
                                            FD_SET(parentrd, &masterset);
                                        } else {
                                            flags->persr = 2;
                                        }
                                    }
                                    bzero(buf, sizeof(buf));
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
                                        n = send(desc, buf, sizeof(buf), 0);
                                        openld = 0;
                                        if (n < 0) {
                                            continue;
                                        }
                                        FD_CLR(desc, &masterset);
                                    }
                                    break;

                                    /*
                                     *  dropr
                                     */
                                case 5:
                                    /* parentrd socket already closed in flagsfunction*/
                                    if (parentrd > 0) {
                                        FD_CLR(parentrd, &masterset);
                                    }
                                    bzero(buf, sizeof(buf));
                                    break;

                                default:
                                    bzero(buf, sizeof(buf));
                                    printf("invalid command\n");
                                    waddstr(w[6], "invalid command");
                                    update_win(6);


                            }
                            free(output[x]);
                        }

                        break;
                    }
                        /* End reading commands from bufCommand*/
                    else{
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

                                        //printf("connection established\n");
                                        waddstr(w[6], "connection established");
                                        update_win(6);
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
                                    //printf("right side reconnecting..\n ");
                                    waddstr(w[5], "right side reconnecting");
                                    update_win(5);
                                    pigopt = 2;
                                    parentrd = sock_init(pigopt, 0, flags->rrport, flags->rraddr, right, host);

                                    if (parentrd > 0) {

                                        //printf("connection restablished\n");
                                        waddstr(w[5], "connection established");
                                        update_win(5);
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
                                    //printf("dropl %s\n", buf);
                                    waddstr(w[5], "dropl");
                                    update_win(5);

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
                                //printf("invalid command\n");
                                waddstr(w[6], "invalis command");
                                update_win(6);
                        }/* End switch case*/
                        break;
                    }/* End else single interactive command string*/
            }/* End stdin descriptor loop*/
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
                //printf("left accept error\n");
                waddstr(sw[6],"left accept error ");
                update_win(6);
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }

            flags->llport = (int) ntohs(lconn.sin_port);
            flags->lladdr = inet_ntoa(lconn.sin_addr);
            maxfd = max(maxfd, desc);
            FD_SET(desc, &masterset);
            //printf("connection established\n");
            waddstr(sw[5],"connection established ");
            update_win(5);
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

            if(n < 0){
                //printf("recv left error \n");
                waddstr(sw[6],"recv left error ");
                update_win(6);

                break;
            }
            if (n == 0) {
                FD_CLR(desc, &masterset);
                openld = 0;
                break;
            }

/*
 * move cursor to window
 * printw
 * */
            /* If dsplr is set we print data coming frm the right*/
            if (flags->dsplr) {
                printf("%s\n", buf);
            }

            /* Loop data right if set*/
            if (flags->loopr && openld){
                n = send(desc, buf, sizeof(buf), 0);

                if(n < 0){
                    //printf("send left error\n");
                    waddstr(sw[6],"-||---send left error");
                    update_win(6);
                    break;
                }
                if( n == 0){
                    /* Set reconnect flag if persl is set*/
                    if(flags->persl){
                        flags->reconl = 1;
                    }
                }
                bzero(buf, sizeof(buf));
            }

            /* Check if data needs to be forwarded */
            if (openrd && flags->output ) {
                n = send(parentrd, buf, sizeof(buf), 0);

                if(n < 0){
                    openrd = 0;
                    //printf("send right error \n");
                    waddstr(sw[6],"~.~.~.~send right error ");
                    update_win(6);
                    break;
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
                    waddstr(sw[6],"<>.<>.<> DESC send left error ");
                    update_win(6);
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

        if( FD_ISSET(parentrd, &readset)){
            bzero(buf, sizeof(buf));
            n = recv(parentrd, buf, sizeof(buf), 0);
            if(n < 0){
                openrd = 0;
                //printf("recv right error\n");
                waddstr(sw[6],"recv right error ");
                update_win(6);
                break;
            }

            if( n == 0){
                openrd =0;
                    flags->persr = 2;

                break;
            }

            /* Check for constant DROPL string*/
            if ( strcmp(buf, DROPL ) == 0 ) {
                //printf("remote right side dropn\n");
                waddstr(sw[5],"remote right side dropn ");
                update_win(5);

                openrd = 0;
            }
                /* Check for constant PERSL string*/
            else if ( strcmp(buf, PERSL)  == 0 ) {
                //printf("remote right side reconnection\n");
                waddstr(sw[5],"remote right side reconnection ");
                update_win(5);
                openrd = 1;
            }
            else{
// window
                /* If dsprl is set we print data coming frm the right*/
                if (flags->dsprl) {
                    printf("%s\n", buf);
                }

                if(flags->loopl ==1){
                    n = send(parentrd, buf, sizeof(buf), 0);
                    if(n < 0){
                        //printf("send right error\n");
                        waddstr(sw[6],"send right error ");
                        update_win(6);
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
                        //printf("send left error\n");
                        waddstr(sw[6],"send left error ");
                        update_win(6);
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
            //printf("right side reconnecting..\n ");
            waddstr(sw[5],"right side reconnecting ");
            update_win(5);
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
            //printf("left side reconnecting attempt...\n ");
            waddstr(sw[5],"left side reconnecting attempt... ");
            update_win(5);
        }
    }



        /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
    // main ncurse loop
    for (int x = 0; x < 100; x++) {

        // main system loop



        wprintw(sw[0], "This is line \t %d of \t 100\n", x);
        usleep(60000); // allows delay in printing each line
        update_win(0);
    }
*/

GUIshutdown(response);
/* End screen updating */
    endwin();
    echo();
    // Bye

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 0;


}/*end main*/