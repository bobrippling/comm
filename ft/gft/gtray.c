#include <gtk/gtk.h>

#include "../../common/gtray.h"

#define ICON_FILE "gft.ico"

static GtkStatusIcon *tray;
static GtkWidget     *menu;
static GtkWidget     *winMain;

void tray_toggle(void);
void tray_quit(  void);

void tray_activated(GObject *tray, gpointer unused);
void tray_popupmenu(GtkStatusIcon *status_icon, guint button,
		guint32 activate_time, gpointer unused);

void tray_activated(GObject *tray, gpointer unused)
{
	/* TODO */
	printf("activated! %p, %p\n", (void *)tray, (void *)unused);
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
	gtk_main_quit();
}

void tray_toggle()
{
	gtk_widget_set_visible(winMain, !gtk_widget_get_visible(winMain));
}

void tray_init(GtkWidget *winMain2)
{
	GtkWidget *menu_item_toggle, *menu_item_quit;

	winMain = winMain2;

	menu = gtk_menu_new();

	menu_item_toggle = gtk_menu_item_new_with_label("Toggle");
	menu_item_quit   = gtk_menu_item_new_with_label("Quit"  );
	g_signal_connect(G_OBJECT(menu_item_toggle), "activate", G_CALLBACK(tray_toggle), NULL);
	g_signal_connect(G_OBJECT(menu_item_quit),   "activate", G_CALLBACK(tray_quit)  , NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_toggle);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_quit  );
	gtk_widget_show_all(GTK_WIDGET(menu));

	tray = gtray_new_fname(ICON_FILE,
			"Comm FT", tray_activated, tray_popupmenu);
}

void tray_term()
{
	gtray_visible(tray, FALSE);
}
