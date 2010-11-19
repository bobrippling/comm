#ifndef GTRAY_H
#define GTRAY_H


GtkStatusIcon *gtray_new_pixbuf(GdkPixbuf *,
		const char *tooltip,
		void (*activated)(GObject *tray, gpointer),
		void (*popupmenu)(GtkStatusIcon *tray, guint button, guint32 time, gpointer)
		);

GtkStatusIcon *gtray_new_fname(const char *,
		const char *tooltip,
		void (*activated)(GObject *tray, gpointer),
		void (*popupmenu)(GtkStatusIcon *tray, guint button, guint32 time, gpointer)
		);

#define gtray_visible(tray, b) \
	gtk_status_icon_set_visible(tray, b)

#endif
