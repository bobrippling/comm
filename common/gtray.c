#include <gtk/gtk.h>

#include "gtray.h"

GtkStatusIcon *gtray_new_fname(const char *fname,
		const char *tooltip,
		void (*activated)(GObject *tray, gpointer),
		void (*popupmenu)(GtkStatusIcon *tray, guint button, guint32 time, gpointer))
{
	GdkPixbuf *icon = gdk_pixbuf_new_from_file(fname, NULL);

	if(!icon)
		fprintf(stderr, "gtray_new_fname: couldn't load icon %s\n", fname);

	return gtray_new_pixbuf(icon, tooltip, activated, popupmenu);
}

GtkStatusIcon *gtray_new_pixbuf(GdkPixbuf *pixbuf,
		const char *tooltip,
		void (*activated)(GObject *tray, gpointer),
		void (*popupmenu)(GtkStatusIcon *tray, guint button, guint32 time, gpointer))
{
	GtkStatusIcon *tray = gtk_status_icon_new_from_pixbuf(pixbuf);

	if(pixbuf)
		g_object_unref(pixbuf);

	if(tooltip)
		gtk_status_icon_set_tooltip(tray, tooltip);

	if(activated)
		g_signal_connect(tray, "activate",   GTK_SIGNAL_FUNC(activated), NULL);

	if(popupmenu)
		g_signal_connect(tray, "popup-menu", GTK_SIGNAL_FUNC(popupmenu), NULL);

	return tray;
}
