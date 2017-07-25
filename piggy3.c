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

        connectr IP [port]:
            Create a connection to computer with “IP” on their tcp port
            “port” for your “right side” If a port is not specified the current
            value of port for the remote port on the right is used. This may
            have been specified on the command line or may have been
            established via an interactive command. If it has never been set
            than use the default port.
        connectl: IP [port]:
            create a connection to computer with “IP” on their tcp port
            “port” for your “left side.”

        listenl: [port]: Use for left side listen for a connection on your local port port. Use default if no port given.
        listenr [port]: Use for right side listen for a connection on your local port port. Use default if no port given.

        read: filename: Read the contents of file “filename” and write it to the current output direction.

        llport [port]: Bind to local port “port” for a left side connection.
        rlport [port]: Bind to local port “port” for a left side connection.
        lrport [port]: Vccept a connection on the left side only if the remote computer attempting to connect has source port “port”.

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

        reset:
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
#ifndef unix
#define WIN32
#include <windows.h>
#include <winsock.h>
#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#endif

#include "functions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <zconf.h>
#include <termios.h>

#define PROTOPORT 36795 /* default protocol port number, booknumber */
#define QLEN 6 /* size of request queue */
#define MAXSIZE 1000

extern int errno;
char localhost[] = "localhost"; /* default host name */
char *filename = "scriptin.txt"; // set default definition for filename
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
                {"llport",  optional_argument, NULL, 't'},
                {"rraddr",  required_argument, NULL, 'z'},
                {"rrport",  optional_argument, NULL, 'k'},
                {NULL, 0,                      NULL, 0}
        };

char *strdup(const char *str) {
    char *ret = malloc(strlen(str) + 1);
    if (ret)
        strcpy(ret, str);
    return ret;
}

char fileRead(const char *filename, char *output[255]) {


    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        perror("Cannot open file:");
        return 0;
    }
    int count = 0;
    char input[255];
    while (count < 255 && fgets(input, sizeof(input), file)) {
        char *line = strtok(input, "\n");
        if (line)
            output[count++] = strdup(line);//Store replica
    }
    fclose(file);

    return count;
}


int main(int argc, char *argv[]) {

    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

    /* setup ncurses for multiple windows */

    int x, y;
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


    if (!(LINES == 43) || !(COLS == 132)) // we don't want to have to do arithmetic to figure out
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
    for (x = 0; x < NUMWINS; x++) {
        w[x] = newwin(WPOS[x][0], WPOS[x][1], WPOS[x][2], WPOS[x][3]);
        sw[x] = subwin(w[x], WPOS[x][0] - 2, WPOS[x][1] - 2, WPOS[x][2] + 1, WPOS[x][3] + 1);
        scrollok(sw[x], TRUE); // allows window to be automatically scrolled
        wborder(w[x], 0, 0, 0, 0, 0, 0, 0, 0);
        touchwin(w[x]);
        wrefresh(w[x]);
        wrefresh(sw[x]);
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
    waddstr(sw[4], "Commands");

    wmove(sw[5], 0, 0);
    waddstr(sw[5], "Input");

    wmove(sw[6], 0, 0);
    waddstr(sw[6], "Errors");



    for (x = 0; x < NUMWINS; x++) update_win(x);
    // Place cursor at top corner of window 5
    wmove(sw[4], 0, 0);
    wprintw(sw[4], "Press Enter to see the output in the upper left window scroll");
    wgetstr(sw[4], response); // Pause so we can see the screen
    wmove(sw[4], 0, 0);
    wclrtoeol(sw[4]); // clears current line without clobbering borders
    wprintw(sw[4], "I'm sleeping between each line");
    update_win(4);
    for (x = 0; x < 100; x++) {

        // main system loop

        wprintw(sw[0], "This is line \t %d of \t 100\n", x);
        usleep(60000); // allows delay in printing each line
        update_win(0);
    }



    wmove(sw[4], 0, 0);
    wclrtoeol(sw[4]);
    wprintw(sw[4], "All finished. Press Enter to terminate the program.");
    update_win(4);
    wgetstr(sw[4], response);
/* End screen updating */
    endwin();
    // Bye

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef WIN32
    WSADATA wsaData;
WSAStartup(0x0101, &wsaData);
#endif

    static struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    cfmakeraw(&newt);


    /* Use input arguments from loop to set values*/
    int i;
    int n;
    int len;
    int ch;
    int indexptr; /*generic ponter for getopt_long_only API*/
    char *line = NULL; /* read line by line */
    icmd flags;

    flags.noleft = 0;
    flags.noright = 0;
    flags.rraddr = NULL; /* hold addresses of left and right connect IP adresses */
    flags.llport = PROTOPORT; /*left protocol port number */
    flags.rrport = PROTOPORT;  /*right protocol port number */
    int parentrd = -1; /*right and left socket descriptors*/
    int parentld = -1;  /*right and left socket descriptors*/
    int openrd = 0; /* indicates open (1) right connection, otherwise (0)*/
    int openld = 0;    /* indicates open(1) left connection, otherwise (0)*/
    int maxfd;
    int desc = -1;

    struct addrinfo hints, *infoptr; /*used for getting connecting right piggy if give DNS*/
    struct addrinfo *p;
    char hostinfo[256];

    char buf[MAXSIZE]; /* buffer for string the server sends */
    char stdinBuf[MAXSIZE]; /* buffer for standard input insert */

    fd_set readset, masterset;
    int pigopt;
    int descrready;
    int commandLength;
    char command[10];
    char commandArray[1000];
    char charMode;
    char *output[255];

    flags.dsplr = 1; /* display left to right data, default if no display option provided */
    flags.dsprl = 0; /* display right  to left data */
    flags.loopr = 0; /* take data that comes from the left and send it back to the left */
    flags.loopl = 0; /* take data that comes in from the right and send back to the right */

    while ((ch = getopt_long_only(argc, argv, "a::l::r::d::e::f::g::t::k:z:", long_options, &indexptr)) != -1) {
        switch (ch) {
            case 'a':
                // read file
                if (argv[2] != NULL) {

                    char *filename = argv[2];
                    int readLines = fileRead(filename, output);
                    // read from array and pass into flag function
                    for (int x = 0; x < readLines; ++x) {
                        printf("%s\n", output[x]);
                        // print into command window sw[4];
                        n = flagsfunction(flags, output[x], sizeof(buf), flags.position, &desc, &parentrd, right, left,
                                          lconn);

                        if (n < 0) {
                            printf("invalid command\n");
                            // print into error reporting sw[6]
                        }

                        free(output[x]);//Discard after being used
                    }

                }
                    // if none specified read from default filename
                else {

                    int readLines = fileRead("scriptin.txt", output);
                    // read from array and pass into flag function
                    for (int x = 0; x < readLines; ++x) {
                        printf("%s\n", output[x]);
                        // print into command window sw[4];
                        n = flagsfunction(flags, output[x], sizeof(buf), flags.position, &desc, &parentrd, right, left,
                                          lconn);

                        if (n < 0) {
                            printf("invalid command\n");
                            // print into error reporting sw[6]

                        }

                        free(output[x]);//Discard after being used
                    }
                }
                break;
            case 'l':
                flags.noleft = 1;
                printf("noLeft\n");
                // print into IO window sw[4];
                break;

            case 'r':
                flags.noright = 2;
                flags.dsplr = 0;
                flags.dsprl = 1;
                printf("noRight\n");
                break;
            case 'd':
                flags.dsplr = 2;
                printf("display left -> right \n");
                break;

            case 'e':
                flags.dsprl = 1;
                printf("display right -> left \n");
                break;

            case 'f':
                flags.loopr = 1;
                printf("loopr\n");
                break;

            case 'g':
                flags.loopl = 1;
                printf("loopl\n");
                break;

            case 't':
                if (number(argv[optind]) > 0) {
                    flags.llport = atoi(argv[optind]);
                } else {
                    printf("left port not a number: %s\n", argv[optind]);
                    // print into error reporting sw[6]
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                if (flags.llport < 0 || flags.llport > 65535) {
                    // print into error reporting sw[6]
                    printf("left port number out of range: %d\n", flags.llport);
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                break;

            case 'k':
                if (number(argv[optind - 1]) > 0) {
                    flags.rrport = atoi(argv[optind - 1]);
                } else {
                    printf("right port not a number: %s\n", argv[optind - 1]);
                    // print into error reporting sw[6]
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                if (flags.rrport < 0 || flags.rrport > 65535) {
                    printf("right port number out of range: %d\n", flags.rrport);
                    // print into error reporting sw[6]
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }
                /* test for illegal value */
                break;

            case 'z':
                printf("right addrs parse...\n");
                flags.rraddr = argv[optind - 1];

                hints.ai_family = AF_INET;

                n = getaddrinfo(flags.rraddr, NULL, NULL, &infoptr);

                if (n != 0) {
                    printf("rraddr error\n");
                    // print into error reporting sw[6]
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return -1;
                }

                for (p = infoptr; p != NULL; p = p->ai_next) {
                    getnameinfo(p->ai_addr, p->ai_addrlen, hostinfo, sizeof(hostinfo), NULL, 0, NI_NUMERICHOST);
                    flags.rraddr = hostinfo;
                }

                freeaddrinfo(infoptr);

                break;
            case '?':
                fprintf(stderr, "invalid option: -%c\n", optopt);
                // print into error reporting sw[6]
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                return -1;
        }
    }

    printf("All available args parsed...\n");
    flags.position = flags.noleft + flags.noright;
    printf("postion: %d\n", flags.position);
    if (flags.position == 3) {
        printf("Piggy requires at least one connection...\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        exit(1);
    }

    n = flags.dsplr + flags.dsprl;
    if (n == 3) {
        printf("dsplr and dsprl cannot both be set...\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        exit(1);
    }
    /* Display conditions for tail and head piggies*/
    /* Head piggy case, set dsprl flag and clear dsplr*/
    if (flags.noleft == 1) {
        flags.dsplr = 0;
        flags.dsprl = 1;
    }
    /* Tail piggy case, set dslr flag and clear dsprl*/
    if (flags.noright == 2) {
        flags.dsplr == 2;
        flags.dsprl = 0;
    }

    /* A position < 1 implies that the currect piggy is at least*/
    /*  a middle piggy                                          */
    if ((flags.position < 1) & (flags.rraddr == NULL)) {
        printf("Piggy right connection requires right address or DNS...\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        exit(1);
    }

    FD_ZERO(&masterset);
    FD_SET(0, &masterset);

    switch (flags.position) {

        /* Middle piggy */
        case 0:
            printf("Middle piggy\n");

            pigopt = 2;
            parentrd = sock_init(pigopt, 0, flags.rrport, flags.rraddr, right, host);

            if (parentrd < 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }

            pigopt = 1;
            parentld = sock_init(pigopt, QLEN, flags.llport, NULL, left, NULL);

            if (parentld < 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }

            //FD_SET(desc, &masterset);
            FD_SET(parentld, &masterset);
            FD_SET(parentrd, &masterset);
            printf("One right descriptor added to fd_set, right descriptor\n");
            printf("Two left descriptors added, main left listening and left accept\n");
            break;

            /* Head piggy */
        case 1:
            printf("Head piggy\n");

            pigopt = 2;
            parentrd = sock_init(pigopt, 0, flags.rrport, flags.rraddr, right, host);

            if (parentrd < 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }
/**********/
            printf("%s", inet_ntoa(right.sin_addr)); // return the IP) );

/**********/
            openrd = 1;
            printf("One right descriptor added to fd_set, right descriptor\n");
            FD_SET(parentrd, &masterset);
            break;

            /* Tail Piggy*/
        default:
            printf("Tail piggy\n");

            pigopt = 1;
            parentld = sock_init(pigopt, QLEN, flags.llport, NULL, left, NULL);
            openld = 1;
            if (parentld < 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }

            FD_SET(parentld, &masterset);
            printf("One left descriptor added to fd_set, left descriptor \n");
    }
    /*end switch */



    /* Since the piggy position is at least 0 and less than 3*/
    /*  maxfd == parentld or maxfd == parentrd                   */
    maxfd = max(parentld, parentrd);
    printf("All required sockets okay...\n");
    printf("(left %d, desc %d, right %d, maxfd %d)\n", parentld, desc, parentrd, maxfd);

    while (1) {
        memcpy(&readset, &masterset, sizeof(masterset));

        /**/
        n = select(maxfd + 1, &readset, NULL, NULL, NULL);
        if (n < 0) {
            printf("select error \n");
            break;
        }

        /*Reading from standard in*/
        /* When creating the socket descriptors, we
        *  already ensured that parentrd >0 and
        *  therfore don't need addition checks
        *   durring its use.
        */
        if (FD_ISSET(0, &readset)) {
            ch = getchar();

            switch (ch) {
                /*i*/
                case 105:
                    printf("Enter Insert\n");
                    //printf("position%d", parentrd);
                    /* for building program, send all data right*/

                    //while(ch = getchar() != EOF && getchar() != '\n'){
                    while (1) {
                        //while((ch = getchar()) != 27){

                        ch = getchar();

                        if (ch == 27) {
                            break;

                        } else {
                            buf[0] = (char) ch;
                            putchar(ch);
                            if (flags.position <= 1) {
                                if (openrd) {
                                    n = send(parentrd, buf, sizeof(buf), 0);
                                }
                                if (n < 0) {
                                    printf("send parent error");

                                    break;
                                }
                                if (n == 0) {
                                    /* Here, if persr is set, we will attempt*/
                                    /*  reestablish the connection           */
                                    printf("1: right connection closed...\n");
                                    break;
                                }
                                //printf("Message sent successfully...\n");
                            } else {
                                /* Send data to the left, this is a tail piggy */
                                n = send(desc, buf, sizeof(buf), 0);

                                if (n < 0) {
                                    printf("send left error \n");
                                    break;
                                }
                                if (n == 0) {
                                    printf("left connection closed, reestablish later...\n");
                                    break;
                                }

                                //printf("Message sent successfully...\n");
                            }
                        }
                    }
                    printf("\n");

                    break;
                    /* q*/
                case 113:
                    bzero(buf, sizeof(buf));
                    printf("exiting\n");
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return 0;
                    /* : */
                case 58:
                    bzero(buf, sizeof(buf));
                    i = 0;
                    putchar(':');
                    printf("Enter command mode\n");

                    //while( (ch = getchar()) != 10){
                    while (1) {
                        ch = getchar();
                        if (ch == 10) {
                            break;
                        } else {

                            buf[i++] = ch;
                            putchar(ch);

                            //++i;
                        }
                    }
                    //printf("left command\n");
                    // pass buf into file read if begins with source

                    char *bufCommand = buf;
                    //printf("%s\n", bufCommand);
                    char *checker = NULL;


                    checker = strstr(bufCommand, "source");
                    if (checker == bufCommand) {
                        char tokenCommand[1000];
                        char commandOutput[255];


                        // run through tokenizer
                        char delimiter[] = " ";
                        char *word2, *end;
                        int inputLength = strlen(buf);
                        char *inputCopy = (char *) calloc(inputLength + 1, sizeof(char));
                        strncpy(inputCopy, buf, inputLength);
                        strtok_r(inputCopy, delimiter, &end);
                        word2 = strtok_r(NULL, delimiter, &end);

                        // get commands from fileread
                        int readLines = fileRead(word2, output);

                        // read from array and pass into flagfunction
                        for (int x = 0; x < readLines; ++x) {
                            printf("%s\n", output[x]);
                            n = flagsfunction(flags, output[x], sizeof(buf), flags.position, &desc, &parentrd, right,
                                              left, lconn);

                            if (n < 0) {
                                printf("invalid command\n");
                            }

                            free(output[x]);//Discard after being used
                        }


                        if (n < 0) {
                            printf("invalid command\n");
                        }
                        break;
                    } else {


                        printf("\n");
                        printf("%s\n", buf);
                        n = flagsfunction(flags, buf, sizeof(buf), flags.position, &desc, &parentrd, right, left,
                                          lconn);

                        if (n < 0) {
                            printf("invalid command\n");
                        }
                        break;
                    }
            }
        }
        /*End stdin */


        /*
         * Accept incoming left connection
         *
         */
        if (FD_ISSET(parentld, &readset)) {
            len = sizeof(lconn);
            desc = accept(parentld, (struct sockaddr *) &lconn, &len);

            if (desc < 0) {
                printf("left accept error\n");
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                exit(1);
            }

            maxfd = max(maxfd, desc);
            FD_SET(desc, &masterset);
            printf("(left %d, desc %d, right %d, maxfd %d)\n", parentld, desc, parentrd, maxfd);
        }

        /* Incoming left side connection */
        /* When creating the socket descriptors, we
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
                printf("left connection closed, reestablish later...\n");
                FD_CLR(desc, &masterset);
                //break;
            }
            // printf("Message recv successfully...\n");

            if (flags.position == 2) {
                printf("%c", buf[0]);
            }
            /* If dsprl is set we print data coming frm the right*/
            if (flags.dsplr == 1) {
                printf("%c", buf[0]);
                //fputs(buf, stdout);
            }

            if (flags.loopr == 1) {
                n = send(desc, buf, sizeof(buf), 0);
                if (n < 0) {
                    printf("send left error\n");
                    break;
                }
                if (n == 0) {
                    printf("2: right connection closed, reestablish later...\n");
                    break;
                }
                // printf("Message loooped left successfully...\n");
            }
            /* Check if data needs to be forwarded */
            if (flags.loopr != 1 && flags.position <= 1) {
                printf("forwarding message...\n");
                n = send(parentrd, buf, sizeof(buf), 0);

                if (n < 0) {
                    printf("send right error \n");
                    break;
                }
                if (n == 0) {
                    printf("3: right connection closed, reestablish later...\n");
                    break;
                }
            }
        }
        /* End ready  desc*/

        /* Incoming right side connection */
        /* FD_ISSET will be true for the head
        * piggy and middle piggies, therfore we
        * don't check for -noright
        */
        if (FD_ISSET(parentrd, &readset)) {
            // printf("Incoming right side data...\n");
            bzero(buf, sizeof(buf));

            n = recv(parentrd, buf, sizeof(buf), 0);
            if (n < 0) {
                printf("recv right error\n");
                break;
            }
            if (n == 0) {
                openrd = 0;
                //printf("right connection closed, reestablish later...\n");
                if (flags.persr == 1) {
                    printf("reestablish...\n");
                }
                /*do something*/
            } else {
                /* If dsprl is set we print data coming frm the right*/
                if (flags.dsprl == 1) {
                    printf("%c", buf[0]);
                }

                if (flags.loopl == 1) {
                    n = send(parentrd, buf, sizeof(buf), 0);
                    if (n < 0) {
                        printf("send right error\n");
                        break;
                    }
                    if (n == 0) {
                        printf("left connection closed, reestablish later...\n");
                        break;
                    }
                }

                /* Data only right forwarded if middle piggy */
                if (flags.loopl != 1 && flags.position == 0) {
                    n = send(desc, buf, sizeof(buf), 0);
                    if (n < 0) {
                        printf("send left error\n");
                        break;
                    }
                    if (n == 0) {
                        printf("left connection closed, reestablish later...\n");
                        break;
                    }
                }
            }
        }
        /* End ready parentrd*/
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 0;
}/*end main*/
