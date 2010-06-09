#ifndef STRINGS_H
#define STRINGS_H

#define IS_BACKSPACE(x) ((x) == 127 || (x) == 0407)

char *catstrings(int, ...);
int addtostring(int, int *, int *, char **);
char *basename2(char *);
char *dirname2(char *);

#endif

