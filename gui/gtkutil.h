#ifndef GTKUTIL_H
#define GTKUTIL_H

void addtext(const char *text);
void addtextl(const char *fmt, va_list l);
void addtextf(const char *fmt, ...);

void clientlist_init(void);
void clientlist_add(const char *name);
void clientlist_clear(void);

#endif
