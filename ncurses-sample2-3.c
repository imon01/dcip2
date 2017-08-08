// Ncurses Sample 2-3
#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <locale.h>
#define NUMWINS 7
#define RES_BUF_SIZE 80

  WINDOW *w[NUMWINS];
  WINDOW *sw[NUMWINS];

void update_win(int i)
{
  touchwin(w[i]);
  wrefresh(sw[i]);
}

int main(int argc, char *argv[])
{
  int i,j;
  char response[RES_BUF_SIZE];
  int WPOS[NUMWINS][4]= { {16,66,0,0},{16,66,0,66},{16,66,16,0},{16,66,16,66},
    {3,132,32,0},{5,132,35,0},{3,132,40,0} };
  setlocale(LC_ALL,"");       // this has to do with the character set to use
  initscr();         // must always call this (or newterm) to initialize the
                     // library before any routines that deal with windows
                     // or the screen can be called
    
 cbreak();          // this allows use to get characters as they are typed
                     // without the need for the userpressing the enter key 
 noecho();          // this means the characters typed are not 
                     // shown on the screen unless we put them there
 nonl();            // this means don't translate a newline character
                     // to a carraige return linefeed sequence on output
 intrflush(stdscr, FALSE);  // 
 keypad(stdscr, TRUE);       // 
 clear();             // make sure screen is clear before we start

 if (  !(LINES==43) || !(COLS==132) ) // we don't want to have to do arithmetic to figure out
                                      // where to put things on other size screens
  { 
    move(0,0);
    addstr("Piggy3 requires a screen size of 132 columns and 43 rows");
    move(1,0);
    addstr("Set screen size to 132 by 43 and try again");
    move(2,0);
    addstr("Press enter to terminate program");
    refresh();
    getstr(response);            // Pause so we can see the screen 
    endwin();
    exit(EXIT_FAILURE);
    }

 // create the 7 windows and the seven subwindows
 for (i=0;i<NUMWINS;i++)
   {
     w[i]=newwin(WPOS[i][0],WPOS[i][1],WPOS[i][2],WPOS[i][3]);
     sw[i]=subwin(w[i],WPOS[i][0]-2,WPOS[i][1]-2,WPOS[i][2]+1,WPOS[i][3]+1);
     scrollok(sw[i],TRUE);
     wborder(w[i],0,0,0,0,0,0,0,0);
     touchwin(w[i]);
     wrefresh(w[i]);
     wrefresh(sw[i]);
   }

 // Write some stuff to the windows 
 wmove(sw[0],0,0);
 wprintw(sw[0],"This is the window where the data flowing from left to right ");
 wprintw(sw[0],"will be displayed. Notice now we don't need to worry about when we reach the last ");
 wprintw(sw[0]," position in the subwindow, we will just wrap around down to the next line of the subwindow");
 wprintw(sw[0]," and can never mess up the border that is in the parent window");
 wmove(sw[1],4,0);
 waddstr(sw[1],"Data leaving right side");
 wmove(sw[2],4,0);
 waddstr(sw[2],"Data leaving the left side");
 wmove(sw[3],4,0);
 waddstr(sw[3],"Data arriving from the right"); 
 wmove(sw[4],0,0);
 waddstr(sw[4],"Commands");
 wmove(sw[5],0,0);
 waddstr(sw[5],"Input");
 wmove(sw[6],0,0);
 waddstr(sw[6],"Errors");
 
 for (i=0;i<NUMWINS;i++) update_win(i);

  // Place cursor at top corner of window 5
 wmove(sw[4],0,0);  
 wprintw(sw[4],"Press Enter to see the output in the upper left window scroll");
 wgetstr(sw[4],response);            // Pause so we can see the screen
 wmove(sw[4],0,0); 
 wclrtoeol(sw[4]);  
 wprintw(sw[4],"I'm sleeping a tenth of a second between each line");
 update_win(4);
 for (i=0;i<5;i++) 
   {
     wprintw(sw[0],"This is line \t %d of \t 100\n",i);     
     update_win(0);
   } 
 wmove(sw[4],0,0);  
 wclrtoeol(sw[4]);  
 wprintw(sw[4],"All finished. Press Enter to terminate the program.");
 update_win(4);
 wgetstr(sw[4],response); 
/* End screen updating */
 endwin();
 // Bye 
}

