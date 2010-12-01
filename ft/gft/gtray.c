#include <gtk/gtk.h>
#include <string.h>
#ifdef _WIN32
# include <windows.h>
# include <gdk/gdkwin32.h>
#endif

#include "../../gcommon/gtray.h"
#include "gtray.h"
#include "gft.h"

#define ICON_FILE "gft.ico"

static gtray_icon     tray;
static GtkWidget     *menu;
static GtkWidget     *winMain;

void tray_quit(void);
void tray_activated(GObject *tray, gpointer unused);
void tray_popupmenu(GtkStatusIcon *status_icon, guint button,
		guint32 activate_time, gpointer unused);

void tray_activated(GObject *unused0, gpointer unused1)
{
	(void)unused0;
	(void)unused1;
	tray_toggle();
}

void tray_popupmenu(GtkStatusIcon *status_icon, guint button, guint32 activate_time, gpointer unused)
{
	(void)unused;

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL,
			status_icon, /* data passed to tray_activated */
			button, activate_time);
}

void tray_quit()
{
	gft_quit();
}

#ifdef _WIN32
HWND tray_hwnd(void)
{
	HWND parent = FindWindow("Shell_TrayWnd", "");
	return FindWindowEx(parent, 0, "TrayNotifyWnd", NULL);
}
#endif


void tray_toggle()
{
#ifdef _WIN32
	HWND hWnd;
	RECT rc_us, rc_tray;
	RECT *from, *to;

	hWnd = (HWND)gdk_win32_drawable_get_handle(
			winMain->window);

	GetWindowRect(hWnd, &rc_us);
	GetWindowRect(tray_hwnd(), &rc_tray);

	if(gtk_widget_get_visible(winMain)){
		from = &rc_us;
		to   = &rc_tray;
	}else{
		from = &rc_tray;
		to   = &rc_us;
	}

	DrawAnimatedRects(hWnd, IDANI_CAPTION, from, to);
#endif

	gtk_widget_set_visible(winMain,
			!gtk_widget_get_visible(winMain));
}

void tray_balloon(const char *t, const char *m)
{
	gtray_balloon(&tray, t, m/*,*/ );
}

void tray_init(GtkWidget *winMain2, const char *argv_0)
{
	GtkWidget *menu_item_toggle, *menu_item_quit;
#ifndef _WIN32
	char *icon_path, *exe_dir, *slash;

	exe_dir   = g_strdup(argv_0);
	slash     = strrchr(exe_dir, G_DIR_SEPARATOR);
	if(slash){
		*slash = '\0';
		icon_path = g_strdup_printf("%s/%s", exe_dir, ICON_FILE);
	}else
		/* something like argv[0] = "gft" */
		icon_path = g_strdup(ICON_FILE);
#else
	(void)argv_0;
#endif

	winMain = winMain2;

	menu = gtk_menu_new();

	menu_item_toggle = gtk_menu_item_new_with_label("Toggle");
	menu_item_quit   = gtk_menu_item_new_with_label("Quit"  );
	g_signal_connect(G_OBJECT(menu_item_toggle), "activate", G_CALLBACK(tray_toggle), NULL);
	g_signal_connect(G_OBJECT(menu_item_quit),   "activate", G_CALLBACK(tray_quit)  , NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_toggle);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_quit  );
	gtk_widget_show_all(GTK_WIDGET(menu));

#ifdef _WIN32
	{
		HICON icon = LoadIcon(GetModuleHandle(0),
				MAKEINTRESOURCE(IDI_ICON_GFT));
		GdkPixbuf *pbuf;

		if(!icon)
			fprintf(stderr, "LoadIcon(): %d\n", GetLastError());

		pbuf = gdk_win32_icon_to_pixbuf_libgtk_only(icon);

		gtray_new_pixbuf(&tray, pbuf,
				"Comm FT", tray_activated, tray_popupmenu);
	}
#else
	gtray_new_fname(&tray, icon_path,
			"Comm FT", tray_activated, tray_popupmenu);

	g_free(exe_dir);
	g_free(icon_path);
#endif
}

void tray_term()
{
	gtray_visible(&tray, FALSE);
}
