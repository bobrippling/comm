#include <gtk/gtk.h>
#ifdef _WIN32
/*# include <gdk/win32/gdkwin32.h>*/
# define _WIN32_IE 0x500
# include <windows.h>
# include <shellapi.h>
# include <gdk/gdkwin32.h>

/*# include "gwinlist.h"*/
#endif

#include "gtray.h"
#include "gballoon.h"


void gtray_new_fname(
		gtray_icon *tray_icon,
		const char *fname,
		const char *tooltip,
		void (*activated)(GObject *tray, gpointer),
		void (*popupmenu)(GtkStatusIcon *tray, guint button, guint32 time, gpointer))
{
	GdkPixbuf *iconpixbuf = gdk_pixbuf_new_from_file(fname, NULL);

	if(!iconpixbuf)
		fprintf(stderr, "gtray_new_fname: couldn't load icon %s\n", fname);

	gtray_new_pixbuf(tray_icon, iconpixbuf, tooltip, activated, popupmenu);
}

void gtray_new_pixbuf(
		gtray_icon *tray_icon,
		GdkPixbuf *pixbuf,
		const char *tooltip,
		void (*activated)(GObject *tray, gpointer),
		void (*popupmenu)(GtkStatusIcon *tray, guint button, guint32 time, gpointer))
{
#ifdef USE_WIN_ICON
	/* tray_icon is NOTIFYICONDATA */
	tray_icon->hWnd   = 0;
	tray_icon->uID    = 0;
	tray_icon->uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	tray_icon->uCallbackMessage = WM_MOUSEMOVE; /* TODO */
	tray_icon->hIcon  = gdk_win32_pixbuf_to_hicon_libgtk_only(pixbuf);

	if(!Shell_NotifyIcon(NIM_ADD, tray_icon))
		fprintf(stderr, "Shell_NotifyIcon failed (setup)\n");
#else
	GtkStatusIcon *tray;

	tray = gtk_status_icon_new_from_pixbuf(pixbuf);

	if(pixbuf)
		g_object_unref(pixbuf);

	if(tooltip)
		gtk_status_icon_set_tooltip(tray, tooltip);

	if(activated)
		g_signal_connect(tray, "activate",   GTK_SIGNAL_FUNC(activated), NULL);

	if(popupmenu)
		g_signal_connect(tray, "popup-menu", GTK_SIGNAL_FUNC(popupmenu), NULL);

	*tray_icon = tray;
#endif
}

void gtray_balloon(gtray_icon *icondata,
		const char *title, const char *msg)
{
#ifdef USE_WIN_ICON
#define CPY(d, s) \
		strncpy(d, s, sizeof(d) / sizeof(d[0]))

	CPY(icondata->szInfo,      msg);
	CPY(icondata->szInfoTitle, title);

	icondata->uTimeout = 2000;
	icondata->dwInfoFlags = NIIF_NONE; /* INFO, WARNING, ERROR */
	icondata->uFlags = NIF_INFO;

	/*printf("Shell_NotifyIcon(NIM_MODIFY,\n"
				 "  { .szInfo = \"%s\", .szInfoTitle = \"%s\",\n"
				 "    .hwnd   = %ld, ....\n",
				 icondata.szInfo, icondata.szInfoTitle,
				 icondata.hWnd);*/

	if(!Shell_NotifyIcon(NIM_MODIFY, icondata))
		fprintf(stderr, "Shell_NotifyIcon failed\n");
#else
	/*printf("gtray_balloon: TODO!\n");*/
	gballoon_show(title, msg, 2000, NULL, NULL);
#endif
}

void gtray_visible(gtray_icon *tray, int vis)
{
#ifdef USE_WIN_ICON
	fprintf(stderr, "TODO: icon visible\n");
#else
	gtk_status_icon_set_visible(*tray, vis);
#endif
}
