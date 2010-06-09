#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
/* window size */
#include <sys/types.h>
#include <sys/ioctl.h>

/*
 * http://en.wikibooks.org/wiki/Serial_Programming/termios
 *
 * ECHO      - echo chars
 * ICANON    - enable erase, kill, werase and rprint special chars (^W?)
 * ECHONL    - echo newline
 * ISIG      - enable interrupt, quit and suspend
 * HUP       - send SIGHUP when last process closes tty
 * TOSTOP    - send SIGSTP when bg process tries to write
 * EOF char  - char sends EOF
 *
 * COOKED    - not raw
 */

/*
#define FLAGS  ISIG    |\
              ~ECHO    |\
              ~ECHONL  |\
               TOSTOP  |\
              ~ECHOE   |\
              ~ECHOK   |\
              ~IEXTEN  |\
              ~ECHOCTL |\
              ~ECHOPRT
*/

#define TERM_ATTR(x) tcsetattr(STDIN_FILENO, TCSANOW, &x)

#include "term.h"

static void singlechar(int, struct termios *);
static void echo(int, struct termios *);

static struct termios origattr, customattr;
static const char terminal_escape[] = { 0x1B, 0x5B, 0x0 }; /* aka something[ */


void initterm()
{
    if(tcgetattr(STDIN_FILENO, &customattr)){
        perror("tcgetattr()");
        return;
    }
    
    memcpy(&origattr, &customattr, sizeof(struct termios));
    
    /*
    t.c_lflag &= FLAGS;
    
    t.c_cc[VMIN]  = 1;
    t.c_cc[VTIME] = 0;
    */
    
    singlechar(1, &customattr);
    echo(0, &customattr);
    
    if(TERM_ATTR(customattr))
        perror("tcsetattr()");
}

void term_setoriginal()
{
    TERM_ATTR(origattr);
}

void term_setcustom()
{
    TERM_ATTR(customattr);
}

/*
static void printflags(struct termios *t)
{
#define PRINT_FLAG(x) if( (x & t->c_lflag) == x )\
                          puts(#x);
    
    PRINT_FLAG(ISIG);
    PRINT_FLAG(~ECHO);
    PRINT_FLAG(~ECHONL);
    PRINT_FLAG( TOSTOP);
    PRINT_FLAG(~ECHOE);
    PRINT_FLAG(~ECHOK);
    PRINT_FLAG(~IEXTEN);
}
*/

static void singlechar(int i, struct termios *t)
{
    if(i){
        t->c_cc[VMIN]  = sizeof(char); /* minumum number of bytes to be read before returning from read() */
        t->c_lflag &= ~ICANON;
    }else{
        t->c_cc[VMIN] = 0;
        t->c_lflag |= ICANON;
    }
}


static void echo(int i, struct termios *t)
{
    if(i)
        t->c_lflag |= ECHO;
    else
        t->c_lflag &= ~ECHO;
}

void escape_print(const char *esc)
{
    fputs(terminal_escape, stdout);
    fputs(esc, stdout);
}

void escape_movex(signed char i)
{
    static char cmd[5]; /* largest a signed char can be is 255 - 3. +1 for the letter, +1 for null */
    
    /* A - up, B - down, C - right, D - left */
    if(i > 0)
        sprintf(cmd, "%dC", i);
    else
        sprintf(cmd, "%dD", -i);
    
    escape_print(cmd);
}

int ncols()
{
    struct winsize ws;
    ioctl(1, TIOCGWINSZ, &ws);
    return ws.ws_col;
}

int nrows()
{
    struct winsize ws;
    ioctl(1, TIOCGWINSZ, &ws);
    return ws.ws_row;
}
