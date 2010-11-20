#include <gtk/gtk.h>
#include <stdlib.h>

#include "gballoon.h"

#define WIDTH  150
#define HEIGHT  50

#define DECREMENT  0.02f
#define WAIT_ITERS 40

struct balloon
{
	GtkWidget *window;
	gdouble opacity;
	unsigned int wait;
};


static gboolean balloon_kill(gpointer data)
{
	struct balloon *b = (struct balloon *)data;

	if(b->wait == 0){
		gtk_window_set_opacity(GTK_WINDOW(b->window), b->opacity);

		if((b->opacity -= DECREMENT) <= 0.1f){
			gtk_widget_destroy(b->window);
			free(b);
			return FALSE; /* kill timer */
		}
	}else
		b->wait--;
	return TRUE;
}


GtkWidget *gballoon_show(const char *title, const char *msg, int timeout,
		void (*destroy_func)(gpointer), void *destroy_data)
{
	GtkWidget *window = gtk_window_new(GTK_WINDOW_POPUP);
	GtkWidget *label  = gtk_label_new(msg);
	struct balloon *b = malloc(sizeof *b);

	if(!b)
		g_error("couldn't allocate %ld bytes", sizeof *b);

	b->window     = window;
	b->opacity    = 1.0f;
	b->wait       = WAIT_ITERS;

	gtk_widget_set_size_request(window, WIDTH, HEIGHT);
	gtk_window_set_title(GTK_WINDOW(window), title);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	/*gtk_window_set_decorated(GTK_WINDOW(window), FALSE);*/
	gtk_window_move(GTK_WINDOW(window),
			gdk_screen_width() - WIDTH, gdk_screen_height() - HEIGHT);

	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_container_add(GTK_CONTAINER(window), label);

	gtk_widget_show_all(window);

	g_timeout_add(timeout * DECREMENT, balloon_kill, b);

	if(destroy_func)
		g_signal_connect(G_OBJECT(window), "destroy",
				G_CALLBACK(destroy_func), destroy_data);

	return window;
}
