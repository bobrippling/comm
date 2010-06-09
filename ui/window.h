#ifndef TERM_H
#define TERM_H

void initterm(void);

void term_setoriginal(void);
void term_setcustom(void);

int ncols(void);
int nrows(void);

void escape_print(const char *);
void escape_movex(signed char);
#define escape_clrtoeol() escape_print("K")


#endif
