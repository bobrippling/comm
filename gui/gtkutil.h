#ifndef GTKUTIL_H
#define GTKUTIL_H

void addtext( const char *col, const char *text);
void addtextl(const char *col, const char *fmt, va_list l);
void addtextf(const char *col, const char *fmt, ...);

void clientlist_init(void);
void clientlist_add(const char *name);
void clientlist_clear(void);

#endif
