#include <gtk/gtk.h>

#define WIN_MAIN   "winFT"
#define TIMEOUT    250

#include "gladegen.h"

GtkWidget *btnSend, *btnConnect, *btnListen;
GtkWidget *winMain;

/* events */
G_MODULE_EXPORT gboolean on_winMain_destroy(void)
{
	gtk_main_quit(); /* gtk exit here only */
	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnListen_clicked(GtkButton *button, gpointer data)
{
	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnConnect_clicked(GtkButton *button, gpointer data)
{
	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnSend_clicked(GtkButton *button, gpointer data)
{
	return FALSE;
}

static int getobjects(GtkBuilder *b)
{
#define GET_WIDGET(x) \
	if(!((x) = GTK_WIDGET(gtk_builder_get_object(b, #x)))){ \
		fputs("Error: Couldn't get Gtk Widget \"" #x "\", bailing\n", stderr); \
		return 1; \
	}
	GET_WIDGET(winMain);
	GET_WIDGET(btnSend);
	GET_WIDGET(btnConnect);
	GET_WIDGET(btnListen);

	return 0;
#undef GET_WIDGET
}

int main(int argc, char **argv)
{
	GError     *error = NULL;
	GtkBuilder *builder;

	if(argc != 1){
		fprintf(stderr, "Usage: %s\n", *argv);
		return 1;
	}

	gtk_init(&argc, &argv); /* bail here if !$DISPLAY */

	builder = gtk_builder_new();

	if(gladegen_init())
		return 1;

	if(!gtk_builder_add_from_file(builder, GFT_GLADE, &error)){
		g_warning("%s", error->message);
		/*g_free(error);*/
		return 1;
	}
	gladegen_term();

	if(getobjects(builder))
		return 1;

	gtk_builder_connect_signals(builder, NULL);

	/* don't need it anymore */
	g_object_unref(G_OBJECT(builder));

	/* signal setup */
	g_signal_connect(G_OBJECT(winMain), "destroy", G_CALLBACK(on_winMain_destroy), NULL);

	gtk_widget_show(winMain);

	gtk_main();

	return 0;
}
