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
*       lraddr [IP]:
            When the left is put into passive mode (via a listenl command)
            accept a connection on the left side only if the remote computer
            attempting to connect has IP address “IP” If the left is placed in
            active mode (trying to connect) use this as the address to connect to.
*       rraddr [IP]:
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


extern int errno;
char localhost[] = "localhost";         /* default host name */
char *filename = "scriptin.txt";        /* Set default definition for filename */
struct hostent *host;                   /* Pointer to a host table entry */
struct sockaddr_in left;                /* Structure to hold left address */
struct sockaddr_in right;               /* Structure to hold right address */
struct sockaddr_in lconn;               /* Structure for left passive connection */
struct sockaddr_in rconn;               /* Structure for right passive connection*/




char *output[MAXSIZE];
int inputDesignation = -1;

void update_win(int i) {
    touchwin(w[i]);
    wrefresh(sw[i]);
}

void win_clear(int win){
    wmove(sw[win], 0,0);
    wclrtoeol(sw[win]);
    update_win(win);
}

void nerror(char *str) {
    wmove(sw[ERW], 0, 0);
    wclrtoeol(sw[ERW]);
    waddstr(sw[ERW], str);
    update_win(ERW);
}

void winwrite(int win, char *str) {
    win_clear(win);
    waddstr(sw[win], str);
    update_win(win);
}


/* add string to a window */
void wAddstr(int z, char c[255]);




/* closes windows completely */
void GUIshutdown(char *response) {
    winwrite(CMW, "All finished. Press Enter to terminate the program.");
    wgetstr(sw[CMW], response);
    endwin();
    echo();
}


void winclear(int win, int y, int x){
    wmove(sw[win], y, x);
    wclrtoeol(sw[win]);
    update_win(win);
}

void reset_displays(){

    int i = 4;

    for(; i< 7; i++){
        win_clear(i);
    }

    waddstr(sw[INW], "Data Entry:");
    waddstr(sw[ERW], "Errors:");
    waddstr(sw[CMW], "Commands:");

    i = 4;
    for(; i< 7; i++){
        update_win(i);
    }
}


/* clear the windows and set all variables to default */
void resetWindows() {

    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    /* Clear screen before starting */
    clear();
    w[0] = newwin(0, 0, 0, 0);

    for (int i = 0; i < NUMWINS; i++) {
        touchwin(w[i]);
        wrefresh(w[i]);
        wrefresh(sw[i]);
    }

    icmd *flags;

    flags->noleft = 0;
    flags->noright = 0;
    bzero(flags->rraddr, sizeof(flags->rraddr));    /* Right connecting address                                           */
    bzero(flags->lraddr, sizeof(flags->rraddr));    /* Left connected address                                             */
    bzero(flags->localaddr, sizeof(flags->rraddr)); /* Local address                                                      */
    flags->llport = DEFAULT;                        /* left protocol port number                                          */
    flags->rrport = DEFAULT;                        /* right protocol port number                                         */
    flags->dsplr = 1;                               /* display left to right data, default if no display option provided  */
    flags->dsprl = 0;                               /* display right  to left data                                        */
    flags->loopr = 0;                               /* take data that comes from the left and send it back to the left    */
    flags->loopl = 0;                               /* take data that comes in from the right and send back to the right  */
    flags->output = 1;

    flags->connectl = 0;
    flags->connectr = 0;
    flags->listenl = 0;
    flags->listenr = 0;
    flags->reset = 0;


}

/* inputs requiring 2nd word in command */
const char *writtenInputs[] = {
        "connectl",
        "connectr",
        "listenl",
        "listenr",
        "llport",
        "rrport",
        "lrport",
        "rlport",
        "lladdr",
        "rraddr",
};

static struct option long_options[] ={
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


char *twoWordCommand(char cbuf[80], icmd *flags){

    win_clear(INW);

    char delimiter[] = " ";
    char *checker = NULL;
    //char *inputCheck = cbuf;
    char *word2, *end;
    char *inputCopy;
    int inputLength;



    inputLength = strlen(cbuf);
    inputCopy = (char *) calloc(inputLength + 1, sizeof(char));

    /* inputs requiring 2nd word in command */
    const char *writtenInputs[] = {"connectl", "connectr", "listenl", "listenr", "llport", "rrport", "lrport", "rlport", "lladdr", "rraddr"};

    /* check if command is of specific set of commands which require 2nd word in command to be read, other than source */
    for(int l = 0; l < 10; l++) {
        //update_win(CMW);
        checker = strstr(cbuf, writtenInputs[l]);
        if (checker == cbuf) {
            strncpy(inputCopy, cbuf, inputLength);
            strtok_r(inputCopy, delimiter, &end);
            word2 = strtok_r(NULL, delimiter, &end);

            int inputDesignation = l;

            /* set flags for values to 2nd word in command */
            // delimiter or inputCheck probably input

            // connectl
            if (inputDesignation == 0) {

                flags->connectl = *word2;
                waddstr(sw[INW], "connectl ");
                wprintw(sw[INW], word2);
            }
                // connectr
            else if (inputDesignation == 1) {
                flags->connectr = *word2;
                waddstr(sw[INW], "connectr ");
                wprintw(sw[INW], word2);
            }
                //listenl
            else if (inputDesignation == 2) {
                flags->listenl = *word2;
                waddstr(sw[INW], "listenl ");
                wprintw(sw[INW], word2);
            }
                // listenr
            else if (inputDesignation == 3) {
                flags->listenr = *word2;
                waddstr(sw[INW], "listenr ");
                wprintw(sw[INW], word2);
            }
                // llport
            else if (inputDesignation == 4) {
                flags->llport = (int) *word2;
                waddstr(sw[INW], "llport ");
                wprintw(sw[INW], word2);

                // rrport
            } else if (inputDesignation == 5) {
                flags->rrport = (int) *word2;
                waddstr(sw[INW], "rrport ");
                wprintw(sw[INW], word2);
            }
                // lrport
            else if (inputDesignation == 6) {
                flags->lrport = (int) *word2;
                waddstr(sw[INW], "lrport ");
                wprintw(sw[INW], word2);
            }
                // rlport
            else if (inputDesignation == 7) {
                flags->rlport = (int) *word2;
                waddstr(sw[INW], "rlport ");
                wprintw(sw[INW], word2);
            }
                // lladdr
            else if (inputDesignation == 8) {
                flags->lladdr[100] = *word2;
                waddstr(sw[INW], "lladdr ");
                wprintw(sw[INW], word2);
            }
                // rraddr
            else if (inputDesignation == 9) {
                flags->rraddr[100] = *word2;
                waddstr(sw[INW], "rraddr ");
                wprintw(sw[INW], word2);
            }


            return inputCopy;
        }
    }
    return NULL;
}

int fileCommand(char cbuf[80]) {


    char delimiter[] = " ";
    char *checker = NULL;
    //char *inputCheck = cbuf;
    char *word2, *end;
    int inputLength;
    int readCommandLines;
    char *inputCopy = (char *) calloc(inputLength + 1, sizeof(char));

    /* clear output array */
    memset(output, 0, sizeof output);


    /* check for source [filename.txt] */
    checker = strstr(cbuf, "source");
    if (checker == cbuf) {


        strncpy(inputCopy, cbuf, inputLength);
        strtok_r(inputCopy, delimiter, &end);
        word2 = strtok_r(NULL, delimiter, &end);


        /* Get commands from fileread*/
        readCommandLines = fileRead(word2, output);

        /* Read from array and pass into flagfunction */




        return readCommandLines;
    }
    return 0;
}
/***************************************************************************************************************************************************/
/***************************************************************************************************************************************************/
/**                                                                  START MAIN                                                                   **/
/***************************************************************************************************************************************************/
/***************************************************************************************************************************************************/
/**find@tag: SM0**/
/*
*Function:
*       main
*
*Description:
*       Piggybacking socket connections
*
*
*Returns:
*       None, integer
*
*/
int main(int argc, char *argv[]) {


    /***************************************************/
    /* Use input arguments from loop to set values     */
    /***************************************************/
    int i, n, x, len, ch;
    int written     = 0;
    int maxfd;                          /* Max descriptor                          */
    int pigopt;                         /* Piggy position indicating variable      */
    int indexptr;                       /* Generic ponter for getopt_long_only API */
    int descl       = -1;               /* Left accepted descriptor                */
    int descr       = -1;               /* Left accepted descriptor                */
    int parentrd    = -1;               /* Main left  descriptors                  */
    int parentld    = -1;               /* Main right descriptors                  */
    int templeft    =  0;               /* Left sending identifier descriptor      */
    int tempright   =  0;               /* Right sending identifier descriptor     */
    char errorstr[32];                  /* Error buf                               */
    char buf[MAXSIZE];                  /* Buffer for string the server sends      */
    char *output[MAXSIZE];
    char cbuf[RES_BUF_SIZE];
    unsigned char lefttype;             /* (0) passive left, else (1) active       */
    unsigned char righttype;            /* (0) passive right, else (1) active      */

    /***************************************************/
    /* Control flow variables                          */
    /***************************************************/
    unsigned char openrd = 0;                   /* (1) indicates open right connection, otherwise (0)*/
    unsigned char openld = 0;                   /* (1) indicates open left  connection, otherwise (0)*/
    unsigned char fowardr = 0;
    unsigned char fowardl = 0;

    /***************************************************/
    /* Remote and host information variables           */
    /***************************************************/
    struct addrinfo hints, *infoptr;            /* used for getting connecting right piggy if give DNS*/
    struct addrinfo *p;
    struct in_addr ip;
    struct hostent *lhost;
    char hostinfo[256];
    char hostname[256];


    /***************************************************/
    /* File Descriptor sets                            */
    /***************************************************/
    fd_set readset, masterset;


    /***************************************************/
    /* Init descriptor set                             */
    /***************************************************/
    FD_ZERO(&masterset);
    FD_SET(0, &masterset);

    /***************************************************/
    /* File related variables                          */
    /***************************************************/
    char *inputCheck = buf;
    char *checker = NULL;
    int readLines;
    int fileRequested = 0;
    char *word2, *end;
    char delimiter[] = " ";
    int readCommandLines;
    int inputLength = 0;
    char *inputCopy;


    /***************************************************/
    /* Ncurses windows variables                       */
    /***************************************************/
    int a, c;
    char response[RES_BUF_SIZE];
    int WPOS[NUMWINS][4] = {
            {16,  66,   0,   0},
            {16,  66,   0,  66},
            {16,  66,  16,   0},
            {16,  66,  16,  66},
            { 3,  132, 32,   0},
            { 5,  132, 35,   0},
            { 3,  132, 40,   0}
    };
    
    
    /***************************************************/
    /* Setup ncurses for multiple windows              */
    /***************************************************/
    setlocale(LC_ALL, "");                      /* Use to local character set*/
    initscr();
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    /* Clear screen before starting */
    clear();
    w[0] = newwin(0, 0, 0, 0);

    
    /****************************************************/
    /* Check for correct terminal size, 132x43 required */
    /***************************************************/
    if (LINES != 43 || COLS != 132) {
        move(0, 0);
        addstr("Piggy3 requires a screen size of 132 columns and 43 rows");
        move(1, 0);
        addstr("Set screen size to 132 by 43 and try again");
        move(2, 0);
        addstr("Press enter to terminate program");
        mvprintw(3, 0, "%dx%d\n", COLS, LINES);
        refresh();
        getstr(response);
        endwin();
        exit(EXIT_FAILURE);
    }


    /***************************************************/
    /* Create the 7 windows and the seven subwindows  */
    /***************************************************/
    for (i = 0; i < NUMWINS; i++) {
        w[i] = newwin(WPOS[i][0], WPOS[i][1], WPOS[i][2], WPOS[i][3]);
        sw[i] = subwin(w[i], WPOS[i][0] - 2, WPOS[i][1] - 2, WPOS[i][2] + 1, WPOS[i][3] + 1);
        scrollok(sw[i], TRUE); 
        wborder(w[i], 0, 0, 0, 0, 0, 0, 0, 0);
        touchwin(w[i]);
        wrefresh(w[i]);
        wrefresh(sw[i]);
    }


    /***************************************************/
    /* Create the 7 windows and the seven subwindows  */
    /***************************************************/
    for (i = 0; i < NUMWINS; i++) {
        w[i] = newwin(WPOS[i][0], WPOS[i][1], WPOS[i][2], WPOS[i][3]);
        sw[i] = subwin(w[i], WPOS[i][0] - 2, WPOS[i][1] - 2, WPOS[i][2] + 1, WPOS[i][3] + 1);
        scrollok(sw[i], TRUE); 
        wborder(w[i], 0, 0, 0, 0, 0, 0, 0, 0);
        touchwin(w[i]);
        wrefresh(w[i]);
        wrefresh(sw[i]);
    }


    /***********************************************/
    /* Windows                                     */
    /***********************************************/
    wmove(sw[INW], 0, 0);
    waddstr(sw[INW], "Data Entry: ");
    wmove(sw[ERW], 0, 0);
    waddstr(sw[ERW], "Errors: ");
    wmove(sw[CMW], 0, 0);
    waddstr(sw[CMW], "Commands: ");

    for (i = 4; i < NUMWINS; i++) {
        update_win(i);
    }


    /***********************************************/
    /*  Flag variables init_init                   */
    /***********************************************/
    icmd *flags;
    flags = malloc(sizeof(icmd));

    flags->noleft = 0;
    flags->noright = 0;
    bzero(flags->rraddr, sizeof(flags->rraddr));    /* Right connecting address                                           */
    bzero(flags->lraddr, sizeof(flags->rraddr));    /* Left connected address                                             */
    bzero(flags->localaddr, sizeof(flags->rraddr)); /* Local address                                                      */
    flags->llport = DEFAULT;                        /* left protocol port number                                          */
    flags->rrport = DEFAULT;                        /* right protocol port number                                         */
    flags->dsplr = 1;                               /* display left to right data, default if no display option provided  */
    flags->dsprl = 0;                               /* display right  to left data                                        */
    flags->loopr = 0;                               /* take data that comes from the left and send it back to the left    */
    flags->loopl = 0;                               /* take data that comes in from the right and send back to the right  */
    flags->output = 1;
    flags->reset = 0;



    /*********************************/
    /*  Parsing argv[]               */
    /*********************************/

    while ((ch = getopt_long_only(argc, argv, "a::l::r::d::e::f::g::h::i::t::k:z:", long_options, &indexptr)) != -1) {
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
                        n = flagsfunction(flags, output[x], sizeof(buf), flags->position, &openld, &openrd, &descl,
                                          &parentrd, right, lconn, inputDesignation);

                        if (n < 0) {
                            nerror("invalid command");
                        }
                        free(output[x]);
                    }

                }
                    /* if none specified read from default filename*/
                else {
                    readLines = fileRead("scriptin.txt", output);
                    /* read from array and pass into flag function  */
                    for (x = 0; x < readLines; ++x) {
                        n = flagsfunction(flags, output[x], sizeof(buf), flags->position, &openld, &openrd, &descl,
                                          &parentrd, right, lconn, inputDesignation);

                        if (n < 0) {
                            nerror("invalid command");
                        }
                        free(output[x]);
                    }
                }
                break;
            case 'l':
                openld = 0;
                flags->noleft = 1;
                waddstr(sw[4], "no left ");
                update_win(4);
                break;

            case 'r':
                openrd = 0;
                flags->noright = 2;
                waddstr(sw[4], "no right ");
                update_win(4);
                break;
            case 'd':
                flags->dsplr = 2;
                /* No change, default is dsplr*/
                break;

            case 'e':
                flags->dsprl = 1;
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
                waddstr(sw[4], "persl ");
                update_win(4);
                break;

            case 'h':
                flags->persr = 1;
                waddstr(sw[4], "persr ");
                update_win(4);
            case 't':
                if (number(argv[optind]) > 0) {
                    flags->llport = atoi(argv[optind]);
                } else {
                    nerror(" left port not a number");
                    GUIshutdown(response);
                    return -1;
                }
                if (flags->llport < 0 || flags->llport > 88889) {
                    nerror(" left port number out of range");
                    GUIshutdown(response);
                    return -1;
                }
                break;

            case 'k':
                if (number(argv[optind - 1]) > 0) {
                    flags->rrport = atoi(argv[optind]);
                } else {
                    nerror(" right port not a number ");
                    GUIshutdown(response);
                    return -1;
                }
                if (flags->rrport < 0 || flags->rrport > 88889) {
                    nerror(" right port number out of range");
                    GUIshutdown(response);
                    return -1;
                }
                /* test for illegal value */
                break;

            case 'z':
                strncpy(flags->rraddr, argv[optind - 1], sizeof(flags->rraddr));
                hints.ai_family = AF_INET;
                n = getaddrinfo(flags->rraddr, NULL, NULL, &infoptr);

                if (n != 0) {
                    nerror(" rraddr error");
                    GUIshutdown(response);
                    return -1;
                }

                for (p = infoptr; p != NULL; p = p->ai_next) {
                    getnameinfo(p->ai_addr, p->ai_addrlen, hostinfo, sizeof(hostinfo), NULL, 0, NI_NUMERICHOST);
                    strcpy(flags->rraddr, hostinfo);
                }

                freeaddrinfo(infoptr);
                break;

            case '?':
                nerror("No valid command");
                GUIshutdown(response);
                return -1;
        }
    }
    /* end switch statement */


    /**************************************************/
    /*  Adjusting program variables and correct flags */
    /**************************************************/

    /* If head piggy selected, it requires a right address*/
    if (flags->noleft && (flags->rraddr[0] == '0')) {
        nerror("Head piggy requires a right address...\n");
        GUIshutdown(response);
        return -1;
    }

    /* Checking for minimum program requirements*/
    flags->position = flags->noleft + flags->noright;
    if (flags->position == 3) {
        //printf("Piggy requires at least one connection...\n");
        nerror("Piggy requires at least one connection...\n");
        GUIshutdown(response);
        return -1;
    }

    /* Checking if display flags are appropiately set*/
    n = flags->dsplr + flags->dsprl;
    if (n == 3) {
        nerror("dsplr and dsprl cannot both be set...");
        GUIshutdown(response);
        return -1;
    }


    /* Head piggy, exit if dsplr and noleft*/
    if(flags->noleft && flags->dsplr == 2){
        nerror("dsplr and noleft cannot both be set...");
        GUIshutdown(response);
        return -1;
    }


    /* Tail piggy, exit if dsprl and noright*/
    if(flags->noright && flags->dsprl){
        nerror("dsprl and noright cannot both be set...");
        GUIshutdown(response);
        return -1;
    }


    /* A position < 1 implies that the currect piggy is at least*/
    /*  a middle piggy                                          */
    if ((flags->position < 1) & (flags->rraddr[0] == '\0')) {
        nerror("Piggy right connection requires right address or DNS...\n");
        GUIshutdown(response);
        return -1;
    }



    /*********************************/
    /*    Getting local IP address   */
    /*********************************/
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        nerror("gethostname, local machine error");
        GUIshutdown(response);
        return -1;
    }

    lhost = gethostbyname(hostname);
    if (lhost == NULL) {
        nerror("gethostbyname, local machine error");
        GUIshutdown(response);
        return -1;
    }

    ip = *(struct in_addr *) lhost->h_addr_list[0];
    strcpy(flags->localaddr, inet_ntoa(ip));



/**WARNING: GENERALIZE THE PIGGY POSTIONS, SO EACH CAN STAND ALONE. THAT IS, EACH PIGGY TYPE CAN EXIST W/O CONNECTIONS
 * GENERALIZE THE PIGGY DESCRIPTORS
 **/
    /*********************************/
    /*  Piggy setup                  */
    /*********************************/
    /* pigspo*/
    switch (flags->position) {

        /*
         * Middle piggy
         */
        case 0:


            pigopt = 2;
            parentrd = sock_init(pigopt, 0, flags->rrport, flags->rraddr, right, host);
            if (parentrd < 0) {
                nerror("socket_init");
                GUIshutdown(response);
                exit(1);
            }

            pigopt = 1;
            parentld = sock_init(pigopt, QLEN, flags->llport, NULL, left, NULL);
            if (parentld < 0) {
                nerror("socket_init");
                GUIshutdown(response);
                exit(1);
            }

            lefttype  = 0;
            righttype = 1;
            openrd   = 1;
            openld   = 1;
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
                nerror("socket_init");
                GUIshutdown(response);
                exit(1);
            }

            openrd = 1;
            openld = 0;
            lefttype = NULLCONNECTION;
            righttype = 1;
            FD_SET(parentrd, &masterset);
            break;

            /*
            * Tail Piggy
            */
        default:


            pigopt = 1;
            parentld = sock_init(pigopt, QLEN, flags->llport, NULL, left, NULL);

            if (parentld < 0) {
                nerror("socket_init");
                GUIshutdown(response);
                exit(1);
            }

            flags->output = 0;
            openld = 1;
            openrd = 0;
            lefttype = 0;
            righttype = NULLCONNECTION;
            FD_SET(parentld, &masterset);
    }
    /*end switch */

    
    /***************************************************/
    /* Init windows cursor postion                     */
    /***************************************************/
    yul = 0; xul = 0;                           /* Top left window position variables           */
    yur = 0; xur = 0;                           /* Top right window position variables          */
    ybl = 0; xbl = 0;                           /* Bottom left window position variables        */
    ybr = 0; xbr = 0;                           /* Bottom right window position variables       */
    ycm = 0; xcm = 0;                           /* Command window position variables            */
    getyx(sw[ULW], yul, xul);
    getyx(sw[URW], yur, xur);
    getyx(sw[BLW], ybl, xbl);
    getyx(sw[BRW], ybr, xbr);
    getyx(sw[CMW], ycm, xcm);
    

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

    while (1) {
        memcpy(&readset, &masterset, sizeof(masterset));

        /**/
        n = select(maxfd + 1, &readset, NULL, NULL, NULL);
        if (n < 0) {
            nerror("select error < 0");
        }

        if (n == 0) {
            nerror("select error == 0");
        }

        /*****************************************************************/
        /* Standard in descriptor ready (@tag:  0FD)                     */
        /*****************************************************************/

        /* Notes:
        * 
        *
        *
        *
        * 
        */

        if (FD_ISSET(0, &readset)) {
            win_clear(CMW);
            bzero(buf, sizeof(buf));

            tempright = descr > 0 ? descr : parentrd; 
            templeft  = descl > 0 ? descl : parentld;
            
            noecho();
            c = wgetch(sw[CMW]);
            switch (c) {
                /*******************************/
                /* Insert mode                 */
                /*******************************/
                case 105:
                    if (openrd || openld) {
                        win_clear(URW);
                        win_clear(BLW);
                        win_clear(INW);
                        winwrite(INW, "Insert: ");
                        echo();

                        while (1) {
                            c = wgetch(sw[INW]);

                            if (c != 27) {

                                wclrtoeol(sw[INW]);
                                putchar(c);
                                buf[0] = c;
                                buf[1] = '\0';
                                echo();

                                /* Requires further work to ignore characters not in printable range */
                                if(c == 8){
                                    noecho();
                                    nocbreak();
                                    delch();
                                    delch();
                                    cbreak();
                                    refresh();
                                }



                                /* Preconditions for sending data to the right, output == 1 */
                                /* Data should be displayed in URW window*/
                                /* `w */
                                if (flags->output && openrd){
                                    n = send(tempright, buf, sizeof(buf), 0);


                                    if (n < 0) {
                                        openrd = 0;
                                        nerror("right send error");
                                    }
                                    if (n == 0) {
                                        openrd = 0;
                                        /* Here, if persr is set, we will attempt*/
                                        /*  reestablish the connection           */
                                        if(flags->persr){
                                            flags->reconl = 1;
                                        }
                                        nerror("right send error, connection closed");
                                    }

                                    /*  */
                                    if(c == 13){

                                        yur++;
                                        xur = 0;
                                        wmove(sw[URW], yur, xur);
                                        wrefresh(sw[URW]);
                                        /**BOOKMARK, ODD BEHAVIOR WITH REMOTE RECV DATA DISPLAY**/
                                        /**COMMENT FOR NOW**/
                                        //scroll(sw[ULW]);
                                    }
                                    wprintw(sw[URW], buf);
                                    update_win(URW);
                                }
                                
                                /* Preconditions for sending data to the left, output == 0 */
                                else if (!flags->output && openld) {
                                    n = send(templeft, buf, sizeof(buf), 0);

                                    if (n < 0) {
                                        openld = 0;
                                        nerror("left send error ");
                                    }

                                    if (n == 0 && flags->persl) {
                                        //some state??
                                    }
                                    if ( n == 0){
                                        openld = 0;
                                        if( flags->persl){
                                            /**WARNING: PSEUDOCODE**/
                                            //SET FLAGS->RECONL IFF LEFT IS ACTIVE
                                            //ELSE GIVE WARNING MESSAGE, CLEAR RECONL FLAG FOR LEFT

                                        }
                                    }
                                    /*  */
                                    if(c == 13){
                                        ybl++;
                                        xbl = 0;
                                        wmove(sw[BLW], ybl, xbl);
                                        wrefresh(sw[BLW]);
                                        /**BOOKMARK, ODD BEHAVIOR WITH REMOTE RECV DATA DISPLAY**/
                                        /**COMMENT FOR NOW**/
                                        //scroll(sw[ULW]);
                                    }
                                    wprintw(sw[BLW], buf);
                                    update_win(BLW);
                                }
                                else {
                                    if (!openld & !flags->output) {
                                        nerror("left connection closed ");
                                        /* usleep used for debugging*/
                                        usleep(5000);
                                        wgetstr(sw[INW], response);
                                    }
                                    if (!openrd & flags->output == 1) {
                                        nerror("right connection closed ");

                                        /* usleep used for debugging*/
                                        usleep(5000);
                                        wgetstr(sw[INW], response);
                                    }
                                }
                            }
                            
                            /* ESCAPE key has been hit*/
                            else {
                                /**WE COULD reset the local windows to starting state**/
                                /**Display window modes**/
                                noecho();
                                waddstr(sw[INW], "press : to enter command mode, or i to insert or q to quit");
                                update_win(INW);
                                win_clear(INW);
                                break;
                            }
                        }/* End input loop*/
                    }/* End of at least one socket is open*/
                    else {
                        nerror("no open sockets");
                        wgetstr(sw[INW], response);
                    }
                    break;

                    /*******************************/
                    /*  Quitting                   */
                    /*******************************/
                case 113:
                    bzero(buf, sizeof(buf));


                    /**WARNING: THESE TEST CASES MUST BE CHANGED ONCE WE GENERALIZED THE DESCRIPTORS BEHAVIOR**/
//                         if( parentld> 0){
//                             shutdown(parentld, 1);
//                         }
//                         if( descl > 0){
//                             shutdown(descl, 2);
//                         }
//                         if( parentrd> 0){
//                             shutdown(parentrd, 2);
//                         }
                    winwrite(CMW, "Exiting");
                    GUIshutdown(response);
                    endwin();

                    return 1;

                    /*******************************/
                    /* Interactive commands        */
                    /*******************************/
                default:
                    bzero(cbuf, sizeof(cbuf));

                    /*`1*/
                    i = 0;
                    winwrite(CMW, ":");
                    echo();
                    nocbreak;


                    if( c >31 && c < 127  || c ==8){
                        waddch(sw[CMW], c);
                    }

                    update_win(CMW);

                    while (1) {
                        win_clear(BRW);
                        wprintw(sw[BRW], "%d",c);
                        update_win(BRW);

                        /* clear input window before new command requested */
                        wclrtoeol(sw[INW]);
                        update_win(INW);

                        /* Needs to be updated, requires addition conditional variable */
                        if(c == 8){
                            delch();
                            delch();
                        }

                        if( c >31 && c < 127){
                            cbuf[i]= (char) c;
                            i++;
                            wclrtoeol(sw[CMW]);
                            update_win(CMW);
                        }

                            /* Enter has been hit*/
                        else if(c == 13){
                            /* PROCESS COMMAND HERE, USER COMMAND STORED IN CBUF*/

                            winwrite(CMW, cbuf);
                            break;
                        }
                        else{
                            noecho();
                            win_clear(CMW);
                            winwrite(CMW, "not a printable character");
                            break;
                        }
                        c = wgetch(sw[CMW]);
                    }
                    /*End accepting commands */

                    noecho();
                    if(i == RES_BUF_SIZE){
                        winwrite(ERW, "buffer full");
                    }

                    wmove(sw[CMW], 0,0);
                    wclrtoeol(sw[CMW]);
                    update_win(CMW);
                    noecho();




/**WARNING: REMEMBER TO INSERT I.C. FUNCTIONALITY HERE**/
                    /**WARNING: REMEMBER TO INSERT I.C. FUNCTIONALITY HERE**/
/**WARNING: REMEMBER TO INSERT I.C. FUNCTIONALITY HERE**/
                    /**WARNING: REMEMBER TO INSERT I.C. FUNCTIONALITY HERE**/
/**WARNING: REMEMBER TO INSERT I.C. FUNCTIONALITY HERE**/
                    /**WARNING: REMEMBER TO INSERT I.C. FUNCTIONALITY HERE**/
/**WARNING: REMEMBER TO INSERT I.C. FUNCTIONALITY HERE**/


                    /* two word commands */
                    char *commandWord1 = twoWordCommand(cbuf, flags);

                    /* file commands */
                    int readCommandLines = fileCommand(cbuf);

                    if(commandWord1 != NULL){
                        n = flagsfunction(flags, commandWord1, sizeof(cbuf), flags->position, &openld, &openrd, &descl,
                                          &parentrd, right, lconn, inputDesignation);
                    } else if(readCommandLines != 0){
                        for (int z = 0; z < readCommandLines; ++z) {

                            winwrite(CMW, output[z]);

                            n = flagsfunction(flags, output[z], sizeof(cbuf), flags->position, &openld, &openrd, &descl,
                                              &parentrd, right, lconn, inputDesignation);
                        }

                    }else{
                        n = flagsfunction(flags, cbuf, sizeof(cbuf), flags->position, &openld, &openrd, &descl,
                                          &parentrd, right, lconn, inputDesignation);
                    }


                    /* process commmands */

                    if(flags->reset == 1) {
                        resetWindows();
                        break;
                    } else {

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
                                if (flags->position != 1) {
                                    bzero(buf, sizeof(buf));
                                    strcpy(buf, PERSL);
                                    n = send(descl, buf, sizeof(buf), 0);

                                    if (n < 0) {
                                        openld = 0;
                                    } else {
                                        FD_SET(descl, &masterset);
                                        nerror("connection established");
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

                                    winwrite(CMW, "right side reconnecting");
                                    pigopt = 2;
                                    parentrd = sock_init(pigopt, 0, flags->rrport, flags->rraddr, right, host);

                                    if (parentrd > 0) {

                                        winwrite(CMW, "connection established");
                                        openrd = 1;
                                        openld = 0;
                                        maxfd = max(descl, parentld);
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
                                if (descl > 0) {
                                    bzero(buf, sizeof(buf));
                                    strcpy(buf, DROPL);

                                    n = send(descl, buf, sizeof(buf), 0);
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
                                nerror("invalid command");

                        }/* End switch case*/
                    }/* end else */
            }
            /**END MAIN 0-FD SWITCH**/

            //reset_displays();
        }/*End stdin */


        /*****************************************************************/
        /* Left Side Accept (Find @tag: LSA)                             */
        /*****************************************************************/
        /*Notes:
        *   Passive left socket, descl will be set
        *   lefttype == 0
        * 
        * 
        * 
        */
        if (!lefttype & FD_ISSET(parentld, &readset)) {
            len = sizeof(lconn);
            descl = accept(parentld, (struct sockaddr *) &lconn, &len);
            if (descl < 0) {
                nerror("left accept error");
                endwin();
                return -1;
            }

            flags->llport = (int) ntohs(lconn.sin_port);
            strcpy(flags->lladdr, inet_ntoa(lconn.sin_addr));
            maxfd = max(maxfd, descl);
            FD_SET(descl, &masterset);
            reset_displays();
            winwrite(CMW, "Left passive connection established");
        }


        /*****************************************************************/
        /* Right Side Accept (Find @tag: RSA)                             */
        /*****************************************************************/
        /*Notes:
        *   Passive right socket, descr will be set
        *   righttype == 0
        * 
        * 
        * 
        */
        if (!righttype & FD_ISSET(parentrd, &readset)) {
            len = sizeof(rconn);
            descr = accept(parentrd, (struct sockaddr *) &rconn, &len);
            if (descr < 0) {
                nerror("left accept error");
                endwin();
                return -1;
            }

            flags->rrport = (int) ntohs(rconn.sin_port);
            strcpy(flags->rraddr, inet_ntoa(rconn.sin_addr));
            maxfd = max(maxfd, descr);
            FD_SET(descr, &masterset);
            reset_displays();
            winwrite(CMW, "Right passive connection established");
        }




        /*****************************************************************/
        /* Left Listeing Descriptor (find @tag: LLD)                     */
        /*****************************************************************/


        /* Notes:
        *    Descl only set with passive left
        * 
        */
        if (FD_ISSET(descl, &readset)) {
            /** REQUIRED VARIABLES = { masterset, opendld, openrd, descl, buffer, flags} **/

            if(righttype){
                sockettype(buf, &righttype, &openld, &openrd, &descl, &parentrd, flags, &masterset);
            }
            else{
                sockettype(buf, &righttype, &openld, &openrd, &descl, &descr, flags, &masterset);
            }
            reset_displays();
        }


        /*****************************************************************/
        /* Right Listeing Descriptor (find @tag: RLD)                    */
        /*****************************************************************/


        /* Notes:
        *   Descr only set with passive right
        */
        if (FD_ISSET(descr, &readset)) {
            /** REQUIRED VARIABLES = { masterset, opendld, openrd, descl, buffer, flags} **/


            if( lefttype){
                sockettype(buf, &lefttype, &openld, &openrd, &descr, &parentld, flags, &masterset);
            }
            else{
                sockettype(buf, &lefttype, &openld, &openrd, &descr, &descl, flags, &masterset);
            }
            reset_displays();
        }

        /*****************************************************************/
        /* RIGHT ACTIVE DESCRIPTROR (find @tag: RAD)                     */
        /*****************************************************************/

        /*
        *
        * Notes:
        *   Active right socket, use parentrd
        * 
        * 
        */
        if (righttype & FD_ISSET(parentrd, &readset) ) {
            /** REQUIRED VARIABLES = { masterset, opendld, openrd, parentrd, buffer, flags} **/


            /*Case: active left*/
            if(lefttype){
                sockettype(buf, &lefttype, &openld, &openrd, &parentrd, &parentld, flags, &masterset);
            }
                /*Case: passive left*/
            else{
                sockettype(buf, &lefttype, &openld, &openrd, &parentrd, &descl, flags, &masterset);
            }
            reset_displays();
        }/* End ready parentrd*/



        /*****************************************************************/
        /* LEFT ACTIVE DESCRIPTROR (find @tag: RSD)                       */
        /*****************************************************************/

        /*
        *
        * Notes:
        *   
        *   
        *   
        */
        if (lefttype & FD_ISSET(parentld, &readset) ) {
            /** REQUIRED VARIABLES = { masterset, opendld, openrd, parentrd, buffer, flags} **/


            /*Case: active left*/
            if(righttype){
                sockettype(buf, &righttype, &openld, &openrd, &parentld, &parentrd, flags, &masterset);
            }
                /*Case: passive left*/
            else{
                sockettype(buf, &righttype, &openld, &openrd, &parentld, &descr, flags, &masterset);
            }
            reset_displays();
        }/* End ready parentrd*/



        /******************************************/
        /* LEFT & RIGHT RECONNECTION DONE HERE    */
        /******************************************/

        /*
        *Notes:
        *  -Try right reconnection if unsuccessful
        *  -persr is only ever set to 2 when a reconnect
        *      fails after calling flagsfunction
        */

        if (flags->persr == 2) {
            winwrite(CMW, "right side reconnecting ");

            pigopt = 2;
            parentrd = sock_init(pigopt, 0, flags->rrport, flags->rraddr, right, host);

            if (parentrd > 0) {
                flags->persr = 1;
                openrd = 1;
                maxfd = max(descl, parentld);
                maxfd = max(maxfd, parentrd);
                FD_SET(parentrd, &masterset);
            }
            reset_displays();
        }
    }
    /***************************************************/
    /**END SELECT LOOP                                **/
    /***************************************************/

    /* End screen updating */
    endwin();
    echo();

    /**WARNING: THESE TEST CASES MUST BE CHANGED ONCE WE GENERALIZED THE DESCRIPTORS BEHAVIOR**/
    
    /**MAYBE**/
    /**MAYBE**/
    /**MAYBE**/
    
    if( parentld> 0){
        shutdown(parentld, 1);
    }
    if( descl > 0){
        shutdown(descl, 2);
    }
    if(descr > 0){
        shutdown(descr, 2);
    }

    if( parentrd> 0){
        shutdown(parentrd, 2);
    }

    return 0;


}/*end main*/