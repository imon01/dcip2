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
const char * DROPL = "REMOTE-LEFT-DROP";
char * filename = "scriptin.txt"; // set default definition for filename
struct hostent *host; /* pointer to a host table entry */
struct addrinfo hints, *infoptr; /*used for getting connecting right piggy if give DNS*/
struct sockaddr_in left; /* structure to hold left address */
struct sockaddr_in right; /* structure to hold right address */
struct sockaddr_in lconn; /* structure to hold left connnecting address */

/* ncurses libraries ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <curses.h>
#include <locale.h>

#define NUMWINS 7
#define RES_BUF_SIZE 80
WINDOW *w[NUMWINS];
WINDOW *sw[NUMWINS];
WINDOW wh[NUMWINS];

void update_win(int i) {
    touchwin(w[i]);
    wrefresh(sw[i]);
}

// definitions
#define inLeft 0 // top left window
#define outRight 1 // top right window
#define outLeft 2 // bottom left window
# define inRight 3 // bottom right window

/* add string to a window */
void wAddstr(int z, char c[255]);


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


static struct option long_options[] =
        {
                {"s",       optional_argument, NULL, 'a'},
                {"noleft",  optional_argument, NULL, 'l'},
                {"noright", optional_argument, NULL, 'r'},
                {"dsplr",   optional_argument, NULL, 'd'},
                {"dsprl",   optional_argument, NULL, 'e'},
                {"loopr",   optional_argument, NULL, 'f'},
                {"loopl",   optional_argument, NULL, 'g'},
                {"persr",   optional_argument, NULL, 'h'},
                {"persl",   optional_argument, NULL, 'i'},
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

    static struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);
    //tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    cfmakeraw(&newt);


    /* Use input arguments from loop to set values*/
    int i;
    int n;
    int len;
    int ch;
    int indexptr; /*generic ponter for getopt_long_only API*/

    int maxfd;
    int desc     = -1;
    int parentrd = -1; /*right and left socket descriptors*/
    int parentld = -1;  /*right and left socket descriptors*/
    int openrd   =  1; /* indicates open (1) right connection, otherwise (0)*/
    int openld   =  1;    /* indicates open(1) left connection, otherwise (0)*/

    struct addrinfo hints, *infoptr; /*used for getting connecting right piggy if give DNS*/
    struct addrinfo *p;
    struct in_addr ip;
    struct hostent* lhost;
    char hostinfo[256];
    char hostname[256];
    char buf[MAXSIZE]; /* buffer for string the server sends */
    //char stdinBuf[MAXSIZE]; /* buffer for standard input insert */

    fd_set readset, masterset;
    int pigopt;
    char *output[255];

    char *bufCommand = buf;
    char *checker = NULL;


    int readLines;
    int fileRequested = 0;

    char *word2, *end;
    char delimiter[] = " ";
    int readCommandLines;
    int inputLength = 0;


    icmd  * flags;
    flags = malloc(sizeof(icmd));


    flags->noleft  = 0;
    flags->noright = 0;
    flags->rraddr  = NULL; /* hold addresses of left and right connect IP adresses */
    flags->llport  = PROTOPORT; /*left protocol port number */
    flags->rrport  = PROTOPORT;  /*right protocol port number */
    flags->dsplr   = 1; /* display left to right data, default if no display option provided */
    flags->dsprl   = 0; /* display right  to left data */
    flags->loopr   = 0; /* take data that comes from the left and send it back to the left */
    flags->loopl   = 0; /* take data that comes in from the right and send back to the right */
    flags->output  = 1;
    flags->reset   = 0;

    /* setup ncurses for multiple windows */

    int a;
    char response[RES_BUF_SIZE];
    int WPOS[NUMWINS][4] = {{16, 66,  0,  0},
                            {16, 66,  0,  66},
                            {16, 66,  16, 0},
                            {16, 66,  16, 66},
                            {3,  132, 32, 0},
                            {5,  132, 35, 0},
                            {3,  132, 40, 0}};
    setlocale(LC_ALL, ""); // this has to do with the character set to use

    initscr(); // must always call this (or newterm) to initialize the
    // library before any routines that deal with windows
    // or the screen can be called

    cbreak(); // this allows use to get characters as they are typed
    // without the need for the user pressing the enter key

    noecho(); // this means the characters typed are not
    // shown on the screen unless we put them there

    nonl(); // this means don't translate a newline character
    // to a carraige return linefeed sequence on output

    intrflush(stdscr, FALSE); //
    keypad(stdscr, TRUE); //

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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    /*********************************/
    /*    Getting local IP address   */
    /*********************************/
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        printf("gethostname, local machine error\n");
        tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
        return -1;
    }

    lhost = gethostbyname(hostname);
    if (lhost == NULL) {
        printf("gethostbyname, local machine error\n");
        tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
        return -1;
    }

    ip = *(struct in_addr*)lhost->h_addr_list[0];
    flags->lladdr = inet_ntoa(ip);
    /*********************************/
    /* End getting local IP address  */
    /*********************************/



    /*********************************/
    /*  Parsing argv[]               */
    /*********************************/

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    while ((ch = getopt_long_only(argc, argv, "a::l::r::d::e::f::g::h::t::k:z:", long_options, &indexptr)) != -1) {
        switch (ch) {
            case 'a':
                /* read file */
                for (int comm = 0; argv[comm] != '\0'; comm++) {
                    // check if filename included
                    if (strstr(argv[comm], ".txt") != NULL) {
                        fileRequested = 1;
                        filename = argv[comm];
                    }
                }

                if (fileRequested) {

                    readLines = fileRead(filename, output);
                    /* read from array and pass into flag function*/
                    for (int x = 0; x < readLines; ++x) {
                        printf("%s\n", output[x]);
                        n = flagsfunction(flags, output[x], sizeof(buf), flags->position, &openld, &openrd, &desc,
                                          &parentrd, right, lconn);

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
                    // read from array and pass into flag function
                    for (int x = 0; x < readLines; ++x) {
                        printf("%s\n", output[x]);
                        n = flagsfunction(flags, output[x], sizeof(buf), flags->position, &openld, &openrd, &desc,
                                          &parentrd, right, lconn);

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
                //printf("noLeft\n");
                waddstr(sw[4], "noleft ");
                update_win(4);
                break;

            case 'r':
                openrd = 0;
                flags->noright = 2;
                flags->dsplr = 0;
                flags->dsprl = 1;
                //printf("noRight\n");
                waddstr(sw[4], "noright ");
                update_win(4);
                break;
            case 'd':
                flags->dsplr = 2;
                //printf("display left -> right \n");
                waddstr(sw[4], "dsplr ");
                update_win(4);
                break;

            case 'e':
                flags->dsprl = 1;
                //printf("display right -> left \n");
                waddstr(sw[4], "dsprl ");
                update_win(4);
                break;

            case 'f':
                flags->loopr = 1;
                flags->output = 1;
                //printf("loopr\n");
                waddstr(sw[4], "loopr ");
                update_win(4);
                break;

            case 'g':
                flags->loopl = 1;
                flags->output = 0;
                //printf("loopl\n");
                waddstr(sw[4], "loopl ");
                update_win(4);
                break;
            case 'h':
                flags->persr = 1;
                //openrd = 1;
                printf("persr\n");

            case 't':
                if (number(argv[optind]) > 0) {
                    flags->llport = atoi(argv[optind]);
                } else {
                    //printf("left port not a number: %s\n", argv[optind]);
                    waddstr(sw[6]," left port not a number");
                    update_win(6);
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                if (flags->llport < 0 || flags->llport > 88889) {
                    // print into error reporting sw[6]
                    //printf("left port number out of range: %d\n", flags.llport);
                    waddstr(sw[6]," left port number out of range");
                    update_win(6);
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                break;

            case 'k':
                if (number(argv[optind - 1]) > 0) {
                    flags->rrport = atoi(argv[optind - 1]);
                } else {
                    //printf("right port not a number: %s\n", argv[optind - 1]);
                    // print into error reporting sw[6]
                    waddstr(sw[6]," right port not a number ");
                    waddstr(sw[6],"argv[optind - 1] ");
                    update_win(6);
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                if (flags->rrport < 0 || flags->rrport > 88889) {
                    //printf("right port number out of range: %d\n", flags.rrport);
                    waddstr(sw[6]," right port number out of range");
                    update_win(6);
                    // print into error reporting sw[6]
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                /* test for illegal value */
                break;

            case 'z':
                //printf("right addrs parse...\n");
                waddstr(sw[5],"right addrs parse...");
                update_win(5);

                flags->rraddr = argv[optind - 1];

                hints.ai_family = AF_INET;

                n = getaddrinfo(flags->rraddr, NULL, NULL, &infoptr);

                if (n != 0) {
                    //printf("rraddr error\n");
                    // print into error reporting sw[6]
                    waddstr(sw[6]," rraddr error");
                    update_win(6);
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
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
            default:break;
        }
    }

    /* end switch statement */


    //printf("All available args parsed...\n");
    //waddstr(sw[5]," All available args parsed... ");
    //update_win(5);

    /* after switch positioning */
    /* Check for minimum program requirments */

    flags->position = flags->noleft + flags->noright;
    printf("postion: %d\n", flags->position);
    if (flags->position == 3) {
        //printf("Piggy requires at least one connection...\n");
        waddstr(sw[6]," Piggy requires at least one connection... ");
        update_win(6);
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        exit(1);
    }

    n = flags->dsplr + flags->dsprl;
    if (n == 3) {
        //printf("dsplr and dsprl cannot both be set...\n");
        waddstr(sw[6]," dsplr and dsprl cannot both be set... ");
        update_win(6);
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        exit(1);
    }
    /* Display conditions for tail and head piggies*/
    /* Head piggy case, set dsprl flag and clear dsplr*/
    if (flags->noleft == 1) {
        flags->dsplr = 0;
        flags->dsprl = 1;
    }
    /* Tail piggy case, set dslr flag and clear dsprl*/
    if (flags->noright == 2) {
        flags->dsplr == 2;
        flags->dsprl = 0;
    }

    /* A position < 1 implies that the current piggy is at least*/
    /*  a middle piggy                                          */
    if ((flags->position < 1) & (flags->rraddr == NULL)) {
        //printf("Piggy right connection requires right address or DNS...\n");
        waddstr(sw[6]," Piggy right connection requires right address or DNS... ");
        update_win(6);
        GUIshutdown(response);
        /*
        wmove(sw[4], 0, 0);
        wclrtoeol(sw[4]);
        endwin();
        echo();
        */

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        exit(1);
    }

    FD_ZERO(&masterset);
    FD_SET(0, &masterset);


    /*********************************/
    /*  Piggy setup                  */
    /*********************************/
    switch (flags->position){

        /*
         * Middle piggy
         */
        case 0:
            //printf("Middle piggy\n");
            waddstr(sw[5],"Middle Piggy ");
            update_win(5);


            pigopt =2;
            parentrd = sock_init(flags, pigopt, 0, flags->rrport, flags->rraddr, right, host);
            if( parentrd < 0){
                tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }

            pigopt = 1;
            parentld = sock_init(flags, pigopt, QLEN, flags->llport, NULL, left, NULL);
            if( parentld < 0){
                tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }


            openrd = 1;
            openld = 1;
            FD_SET(parentld, &masterset);
            FD_SET(parentrd, &masterset);
            //printf("One right descriptor added to fd_set, right descriptor\n");
            //printf("Two left descriptors added, main left listening and left accept\n");
            waddstr(sw[5],"One right descriptor added to fd_set, right descriptor ");
            waddstr(sw[5],"Two left descriptors added, main left listening and left accept ");
            update_win(5);
            break;

            /*
             * Head piggy
             */
        case 1:
            //printf("Head piggy\n");
            waddstr(sw[5],"Head Piggy ");
            update_win(5);


            pigopt = 2;
            parentrd = sock_init(flags,pigopt, 0, flags->rrport, flags->rraddr, right, host);

            if( parentrd < 0){
                tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }

            openrd = 1;
            openld = 0;
            //printf("One right descriptor added to fd_set, right descriptor\n");
            waddstr(sw[5],"One right descriptor added to fd_set, right descriptor ");
            update_win(5);
            FD_SET(parentrd, &masterset);
            break;

            /*
             * Tail Piggy
             */
        default:
            //printf("Tail piggy\n");
            waddstr(sw[5],"Tail Piggy\n");
            update_win(5);

            pigopt = 1;
            parentld = sock_init(flags, pigopt, QLEN, flags->llport, NULL, left, NULL);
            openld = 1;

            if( parentld < 0){
                tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }

            FD_SET(parentld, &masterset);
            //printf("One left descriptor added to fd_set, left descriptor \n");
            waddstr(sw[5],"One left descriptor added to fd_set, left descriptor ");
            update_win(5);
    }
    /*end switch */


    /****************************************************/
    /*  Main loop performing network interaction        */
    /****************************************************/


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

        /* Standard in descriptor ready
        *
        * Notes:
        *   When creating the socket descriptors,
        *   we already ensured that parentrd > 0,
        *   therfore don't need addition checks
        *   durring its use.
        */

        if (FD_ISSET(0, &readset)) {
            ch = getchar();
            //ch = wgetch(w[4]);

            switch(ch){
                /*i*/

                case 105:
                    if(openrd || openld){
                        wrefresh(w[5]);
                        //printf("Enter Insert\n");
                        waddstr(sw[5],"Enter Insert ");
                        update_win(5);
                        nocbreak();
                        echo();

                        while(1){
                            ch = getchar();
                            ch = wgetch(w[5]);

                            if(ch == 27){
                                //printf("\n");
                                waddstr(sw[4],"\n");
                                update_win(4);
                                break;
                            }else{
                                buf[0] = (char) ch;
                                /* Preconditions for sending data to the right, output == 0 */
                                if(flags->output && openrd){
                                    n = send(parentrd, buf, sizeof(buf), 0);
                                    if( n < 0){
                                        //printf("send parent error");
                                        waddstr(sw[6],"send parent error ");
                                        update_win(6);
                                        break;
                                    }
                                    if(n == 0){
                                        /* Here, if persr is set, we will attempt*/
                                        /*  reestablish the connection           */
                                        //printf("right connection closed...\n");
                                        waddstr(sw[5],"right connection closed... ");
                                        update_win(5);
                                        break;
                                    }
                                }

                                /* Preconditions for sending data to the left, output == 0 */
                                if( !flags->output && openld){
                                    n = send(desc, buf, sizeof(buf), 0);

                                    if( n< 0){
                                        //printf("send left error \n");
                                        waddstr(sw[6],"send left error ");
                                        update_win(6);
                                        break;
                                    }
                                    if(n ==0){
                                        //printf("left connection closed, reestablish later...\n");
                                        waddstr(sw[6],"left connection closed, reestablish later ");
                                        update_win(6);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    else{
                        //printf("no open sockets..\n");
                        waddstr(sw[6],"no open sockets ");
                        update_win(6);
                    }
                    break;
                    /* q quit*/
                case 113:
                    bzero(buf, sizeof(buf));
                    //printf("exiting\n");
                    waddstr(sw[5],"exiting ");
                    update_win(5);
                    GUIshutdown(response);
                    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
                    return 0;
                    /* : interactive commands*/
                case 58:
                    bzero(buf, sizeof(buf));
                    i = 0;
                    putchar(':');
                    move(0,0);
                    //printf("Enter command mode ");
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
                    char *inputCopy = (char *) calloc(inputLength + 1, sizeof(char));

                    checker = strstr(bufCommand, "source");
                    if (checker == bufCommand) {
                        strncpy(inputCopy, buf, inputLength);
                        strtok_r(inputCopy, delimiter, &end);
                        word2 = strtok_r(NULL, delimiter, &end);

                        /* Get commands from fileread*/
                        readCommandLines = fileRead(word2, output);

                        /* Read from array and pass into flagfunction */
                        for (int x = 0; x < readCommandLines; ++x) {
                            //printf("%s\n", output[x]);
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
                                    if ((flags->position < 2) && !openld) {
                                        FD_SET(desc, &masterset);
                                    }
                                    break;

                                    /* persr, make reconnection if necessary*/
                                case 3:
                                    if (flags->position < 2 && !FD_ISSET(parentrd, &masterset)) {
                                        printf("right side reconnecting.. ");
                                        pigopt = 2;
                                        parentrd = sock_init(flags, pigopt, 0, flags->rrport, flags->rraddr, right,
                                                             host);

                                        if (parentrd > 0) {
                                            openrd = 1;
                                            openld = 0;
                                            //printf("(left %d, desc %d, right %d, maxfd %d)\n", parentld, desc, parentrd, maxfd);
                                            maxfd = max(desc, parentld);
                                            maxfd = max(maxfd, parentrd);
                                            FD_SET(parentrd, &masterset);
                                            //printf("One right descriptor added to fd_set, right descriptor\n");
                                            waddstr(sw[5],"One right descriptor added to fd_set, right descriptor ");
                                            update_win(5);
                                            //printf("(left %d, desc %d, right %d, maxfd %d)\n", parentld, desc, parentrd, maxfd);
                                        } else {
                                            flags->persr = 2;
                                        }
                                    }
                                    break;

                                    /* dropl*/
                                case 4:
                                    if( desc > 0 ){
                                        buf[0]= '\0';
                                        strncat(buf, "REMOTE-LEFT-DROP", sizeof(buf));
                                        n = send(desc, buf, sizeof(buf), 0);
                                        openld = 0;
                                        if(n < 0){
                                            continue;
                                        }
                                        FD_CLR(desc, & masterset);
                                    }
                                    break;

                                    /* dropr*/
                                case 5:
                                    /* parentrd socket already closed in flagsfunction*/
                                    if(parentrd > 0){
                                        FD_CLR(parentrd, &masterset);
                                    }
                                    break;

                                default:

                                    //printf("invalid command\n");
                                    waddstr(sw[5],"invalid command ");
                                    update_win(5);
                            }
                            free(output[x]);
                        }

                        break;
                    }/* End reading commands from bufCommand*/
                    else{
                        n = flagsfunction(flags, buf, sizeof(buf), flags->position, &openld, &openrd, &desc, &parentrd, lconn, right);

                        switch(n){
                            /* valid command*/
                            case 1:
                                break;
                                /* persl*/
                            case 2:
                                if( (flags->position < 2) && !openld){
                                    FD_SET(desc, &masterset);
                                }
                                break;

                                /* persr, make reconnection if necessary*/
                            case 3:
                                if(flags->position < 2 && !FD_ISSET(parentrd, &masterset) ){
                                    //printf("right side reconnecting..\n ");
                                    waddstr(sw[5],"right side reconnecting ");
                                    update_win(5);
                                    pigopt = 2;
                                    parentrd = sock_init(flags,pigopt, 0, flags->rrport, flags->rraddr, right, host);

                                    if(parentrd > 0){
                                        openrd = 1;
                                        openld = 0;
                                        //printf("(left %d, desc %d, right %d, maxfd %d)\n", parentld, desc, parentrd, maxfd);
                                        maxfd = max(desc, parentld);
                                        maxfd = max(maxfd, parentrd);
                                        FD_SET(parentrd, &masterset);
                                        //printf("One right descriptor added to fd_set, right descriptor\n");
                                        waddstr(sw[5],"One right descriptor added to fd_set, right descriptor ");
                                        update_win(5);
                                        //printf("(left %d, desc %d, right %d, maxfd %d)\n", parentld, desc, parentrd, maxfd);
                                    }
                                    else{
                                        flags->persr = 2;
                                    }
                                }
                                break;

                                /* dropl*/
                            case 4:
                                if( desc > 0 ){
                                    buf[0]= '\0';
                                    strncat(buf, "REMOTE-LEFT-DROP", sizeof(buf));
                                    n = send(desc, buf, sizeof(buf), 0);

                                    if(n < 0){
                                        continue;
                                    }
                                    FD_CLR(desc, & masterset);
                                }
                                break;
                                /* dropr*/
                            case 5:

                                /* parentrd socket already closed in flagsfunction*/
                                if(parentrd > 0){
                                    FD_CLR(parentrd, &masterset);
                                }
                                break;
                            default:
                                //printf("invalid command\n");
                                waddstr(sw[6],"invalid command ");
                                update_win(6);
                        }

                        break;
                    }
            }
        }
        /*End stdin */


        /*
         * Left side accepting descriptor ready
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

            maxfd = max(maxfd, desc);
            FD_SET(desc, &masterset);
            //printf("(left %d, desc %d, right %d, maxfd %d)\n", parentld, desc, parentrd, maxfd);
        }

        /* Left side descriptor ready
        *
        * Notes:
        *   When creating the socket descriptors, we
        *   already ensured that desc >0; therfore
        *   don't need addition checks during its use.
        *   Also only the tail and middle piggies have
        *   left connections; therefore, we can assuredly
        *   use position with desc.
        */

        if( FD_ISSET(desc, &readset)){
            bzero(buf, sizeof(buf));
            n = recv(desc, buf, sizeof(buf), 0);
            printf("%c\n", buf[0]);
            mvwaddch(w[0], 0, 0, buf[0] );
            if(n < 0){
                //printf("recv left error \n");
                waddstr(sw[6],"recv left error ");
                update_win(6);

                break;
            }
            if(n ==0){
                //printf("left connection closed, reestablish later...\n");
                waddstr(sw[5],"left connection closed, reestablish later... ");
                update_win(5);
                FD_CLR(desc, &masterset);
            }

/*
 * move cursor to window
 * printw
 * */
            /* If dsplr is set we print data coming frm the right*/
            if(flags->dsplr == 1){

                printf("%c", buf[0]);
                // move window and display char;
                //mvwaddch(w[3],w[3]-1, 1,buf[0]);
                //printw(buf[0]);

            }

            /* Loop data right if set*/
            if(flags->loopr ){
                n = send(desc, buf, sizeof(buf), 0);

                if(n < 0){
                    //printf("send left error\n");
                    waddstr(sw[6],"send left error ");
                    update_win(6);
                    break;
                }
                if( n == 0){
                    /* Set reconnect flag if persl is set*/
                    if(flags->persl){
                        flags->reconl = 1;
                    }
                    break;
                }
            }

            /* Check if data needs to be forwarded */
            if(openrd && !flags->loopr){
                n = send(parentrd, buf, sizeof(buf), 0);

                if(n < 0){
                    openrd = 0;
                    //printf("send right error \n");
                    waddstr(sw[6],"send right error ");
                    update_win(6);
                    break;
                }
                if( n == 0){
                    openrd = 0;
                    /* Set reconnect flag if persl is set*/
                    if(flags->persl){
                        flags->reconl = 1;
                    }
                    break;
                }
            }
        }
        /* End ready  desc*/

        /* Right side descriptor ready
        *
        * Notes:
        *   FD_ISSET will be true for the head
        *   piggy and middle piggies, therfore we
        *   don't check for -noright
        */

        if( FD_ISSET(parentrd, &readset)){
            bzero(buf, sizeof(buf));
            n = recv(parentrd, buf, sizeof(buf), 0);
            if(n < 0){
                openrd = 0;
                //printf("recv right error\n");
                waddstr(sw[6],"recv left error ");
                update_win(6);


                break;
            }

            if( n == 0){
                openrd =0;
                if(flags->persr ==1){
                    /* Reestablishing done at end of loop*/
                    flags->persr = 2;
                }
                break;
            }
                /* Check for constant string*/
            else if( strncmp(buf, DROPL, sizeof(buf) ) ){
                openrd = 0;
                //printf("Right connection closed...\n");
                waddstr(sw[5],"Right connection closed...");
                update_win(5);

            }
            else{
//
                /* If dsprl is set we print data coming frm the right*/
                if(flags->dsprl){
                    printf("%c", buf[0]);
                }

                if(flags->loopl ==1){
                    n = send(parentrd, buf, sizeof(buf), 0);
                    if(n < 0){
                        //printf("send right error\n");
                        waddstr(sw[6],"send right error ");
                        update_win(6);
                        break;
                    }
                    if( n == 0){
                        //printf("left connection closed, reestablish later...\n");
                        waddstr(sw[5],"left connection closed reestablish later... ");
                        update_win(5);
                        break;
                    }
                }

                /* Data only left forwarded if middle piggy */
                if( !flags->loopl && openld){
                    n = send(desc, buf, sizeof(buf), 0);
                    if(n < 0){
                        openld = 0;
                        //printf("send left error\n");
                        waddstr(sw[6],"send left error ");
                        update_win(6);
                        break;
                    }

                    if( n == 0){
                        flags->persl = 2;
                        //printf("left connection closed, reestablish later...\n");
                        waddstr(sw[5],"left connection closed reestablish later... ");
                        update_win(5);
                        break;
                    }
                }
            }
        }/* End ready parentrd*/

        /* Try right reconnection if unsuccessful*/
        /* persr is only ever set to 2 when a reconnect
         * fails after calling flagsfunction
         */
        if(flags->persr == 2){
            //printf("right side reconnecting..\n ");
            waddstr(sw[5],"left connection closed reestablish later... ");
            update_win(5);
            pigopt = 2;

            parentrd = sock_init(flags,pigopt, 0, flags->rrport, flags->rraddr, right, host);
            if(parentrd >0 ){
                flags->persr = 1;
                openrd = 1;
                //printf("(left %d, desc %d, right %d, maxfd %d)\n", parentld, desc, parentrd, maxfd);
                maxfd = max(desc, parentld);
                maxfd = max(maxfd, parentrd);
                FD_SET(parentrd, &masterset);
                //printf("One right descriptor added to fd_set, right descriptor\n");
                //printf("(left %d, desc %d, right %d, maxfd %d)\n", parentld, desc, parentrd, maxfd);
            }
        }

        if(flags->reconl){
            //printf("left side reconnecting...\n ");
            waddstr(sw[5],"left connection reconnecting..... ");
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