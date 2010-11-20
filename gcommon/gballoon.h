#ifndef GBALLOON_H
#define GBALLOON_H

GtkWidget *gballoon_show(const char *title, const char *msg, int timeout,
		void (*destroy_func)(gpointer), void *destroy_data);

#endif
