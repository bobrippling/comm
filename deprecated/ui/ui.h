#ifndef UI_H
#define UI_H

/* callbacks */
void ui_message(const char *, ...);
void ui_info(const char *, ...);

void ui_gotclient(const char *);

void ui_warning(const char *, ...);
void ui_error(const char *, ...);
void ui_perror(const char *);

int ui_doevents(void);

int  ui_init(const char *host, int port);
void ui_term(void);

#endif
