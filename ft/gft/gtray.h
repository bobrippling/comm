#ifndef TRAY_H
#define TRAY_H

void tray_init(GtkWidget *winMain2, const char *exepath);
void tray_term(void);
void tray_balloon(const char *title, const char *msg);

#endif
