#include <gtk/gtk.h>
#include <stdlib.h>

#include "priv.h"

G_MODULE_EXPORT gboolean on_btnSend_click(GtkWidget *win, GtkWidget *btn);
G_MODULE_EXPORT gboolean on_winPriv_destroy(GtkWidget *win);

struct privchat *priv_new(GtkWidget *winMain, const char *to)
{
	static int TEMP = 0;

	struct privchat *p = malloc(sizeof *p);
	GtkWidget *vbox, *hbox;

	if(!TEMP){
		TEMP = 1;
		setvbuf(stdout, NULL, _IONBF, 0);
	}

	if(!p)
		g_error("couldn't allocated %ld bytes", sizeof *p);

	p->namedup  = g_strdup(to);
	p->winPriv  = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	p->txtMain  = gtk_text_view_new();
	p->entryTxt = gtk_entry_new();
	p->btnSend  = gtk_button_new();

	hbox = gtk_hbox_new(FALSE, 0);
	vbox = gtk_vbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), p->entryTxt, TRUE, TRUE, 0); /* FIXME: TRUE vs FALSE */
	gtk_box_pack_start(GTK_BOX(hbox), p->btnSend,  TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), p->txtMain, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox      , TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(p->winPriv), vbox);

	gtk_window_set_transient_for(GTK_WINDOW(p->winPriv), GTK_WINDOW(winMain));

	gtk_window_set_destroy_with_parent(GTK_WINDOW(p->winPriv), TRUE);

	g_signal_connect(G_OBJECT(winMain),    "destroy", G_CALLBACK(on_winPriv_destroy), NULL);
	g_signal_connect(G_OBJECT(p->btnSend), "clicked", G_CALLBACK(on_btnSend_click),   NULL);

	gtk_widget_show_all(p->winPriv);

	printf("new privchat window for \"%s\", %p\n", to, (void *)p->winPriv);

	return p;
}

/* events */

G_MODULE_EXPORT gboolean
on_winPriv_destroy(GtkWidget *win)
{
	printf("on_winPriv_destroy::win: %p\n", (void *)win);
	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnSend_click(GtkWidget *win, GtkWidget *btn)
{
	printf("on_btnSend_click, win: %p, btn: %p\n", (void *)win, (void *)btn);
	return FALSE;
}
