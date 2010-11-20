#ifndef GTRAY_H
#define GTRAY_H

#ifdef _WIN32
/*# define USE_WIN_ICON*/
#endif

#ifdef USE_WIN_ICON
typedef NOTIFYICONDATA  gtray_icon;
#else
typedef GtkStatusIcon  *gtray_icon;
#endif

void gtray_new_fname(
		gtray_icon *,
		const char *fname,
		const char *tooltip,
		void (*activated)(GObject *tray, gpointer),
		void (*popupmenu)(GtkStatusIcon *tray, guint button, guint32 time, gpointer)
		);

void gtray_new_pixbuf(
		gtray_icon *,
		GdkPixbuf *,
		const char *tooltip,
		void (*activated)(GObject *tray, gpointer),
		void (*popupmenu)(GtkStatusIcon *tray, guint button, guint32 time, gpointer)
		);

void gtray_visible(gtray_icon *, int vis);

void gtray_balloon(gtray_icon *icondata,
		const char *title, const char *msg);

#endif
