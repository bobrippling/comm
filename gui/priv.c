#include "priv.h"

struct privchat *priv_new(GtkWidget *winMain, const char *to)
{
	struct privchat *p = malloc(sizeof *p);
	GError     *error = NULL;
	GtkBuilder *builder;

	if(!p)
		g_error("couldn't allocated %ld bytes", sizeof *p);

	p->namedup = g_strdup(to);

	builder = gtk_builder_new();

	if(gladegen_init()) // TODO
		return 1;

	if(!gtk_builder_add_from_file(builder, GLADE_XML_FILE, &error)){
		g_warning("%s", error->message);
		/*g_free(error);*/
		return 1;
	}
	gladegen_term();

	if(getobjects(builder))
		return 1;

#define GET_WIDGET(x) \
	if(!((p->x) = GTK_WIDGET(gtk_builder_get_object(b, #x)))){ \
		fputs("Error: Couldn't get Gtk Widget \"" #x "\", bailing\n", stderr); \
		return 1; \
	}
	GET_WIDGET(winPriv);
	GET_WIDGET(txtMain);
	GET_WIDGET(entrytxt);
	GET_WIDGET(btnSend);
#undef GET_WIDGET

	gtk_builder_connect_signals(builder, NULL);

	/* don't need it anymore */
	g_object_unref(G_OBJECT(builder));

	gtk_window_set_transient_for(      p->winPriv, winMain);
	gtk_window_set_destroy_with_parent(p->winPriv, TRUE);

	return p;
}
