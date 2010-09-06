#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdarg.h>

/* comm includes */
#include <poll.h>
#include <arpa/inet.h>

#include "util.h"
#include "../../libcomm/comm.h"

#define COMM_GLADE "main.glade"
#define WIN_MAIN   "winMain"
#define TIMEOUT    250
#define UNUSED(n) do (void)(n); while(0)

/* prototypes */
/*TODO*/

/* vars */
GtkWidget  *winmain;
GtkWidget  *entryHost, *entryIn, *entryName; /* Gtk_Entry */
GtkWidget  *txtMain; /* GtkTextView */

comm_t commt;


/* funcs */

void addtext(const char *text)
{
	GtkTextBuffer *buffa;
	GtkTextIter iter;

	buffa = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtMain));
	if(!buffa)
		return;

	memset(&iter, '\0', sizeof iter);
	gtk_text_buffer_get_iter_at_offset(buffa, &iter, -1 /* end */);
	gtk_text_buffer_insert(            buffa, &iter, text, -1 /* nul-term */ );
}

void addtextlf(const char *fmt, va_list l, const char *fmt2, ...)
{
	char *insertmel, *insertmef, *insertme;
	va_list l2;

	insertmel = g_strdup_vprintf(fmt, l);

	va_start(l2, fmt2);
	insertmef = g_strdup_vprintf(fmt2, l2);
	va_end(l2);

	insertme = g_strconcat(insertmel, insertmef, NULL);
	g_free(insertmel);
	g_free(insertmef);

	addtext(insertme);

	g_free(insertme);
}

void addtextl(const char *fmt, va_list l)
{
	char *insertme = g_strdup_vprintf(fmt, l);
	addtext(insertme);
	free(insertme);
}

void addtextf(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	addtextl(fmt, l);
	va_end(l);
}

G_MODULE_EXPORT void on_btnConnect_clicked(GtkButton *button, gpointer data)
{
	UNUSED(button);
	UNUSED(data);

	if(entryIn && entryHost){
		const char *host = gtk_entry_get_text(GTK_ENTRY(entryHost));
		const char *name = gtk_entry_get_text(GTK_ENTRY(entryName));

		if(!strlen(name) || !strlen(host)){
			if(!strlen(name))
				addtext("need name\n");
			else
				addtext("need host\n");
			return;
		}

		if(comm_connect(&commt, host, -1, name))
			addtextf("Couldn't connect to %s: %s\n", host, comm_lasterr(&commt));
		else
			addtextf("Connected to %s\n", host);

	}else
		g_warning("couldn't find entryHost/entryIn");
}

G_MODULE_EXPORT void on_btnSend_clicked(GtkButton *button, gpointer data)
{
	UNUSED(button);
	UNUSED(data);

	if(entryIn)
		printf("text: \"%s\"\n", gtk_entry_get_text(GTK_ENTRY(entryIn)));
	else
		g_warning("couldn't find entryIn");
}

static void commcallback(enum comm_callbacktype type, const char *fmt, ...)
{
	const char *type_str = "unknown";
	va_list l;

#define TYPE(e, s) case e: type_str = s; break
	switch(type){
		TYPE(COMM_MSG,  "message");
		TYPE(COMM_INFO, "info");
		TYPE(COMM_ERR,  "err");
		TYPE(COMM_RENAME,  "rename");
		TYPE(COMM_CLIENT_CONN,  "client_conn");
		TYPE(COMM_CLIENT_DISCO, "client_disco");
		TYPE(COMM_CLIENT_LIST,  "client_list");
	}
#undef TYPE

	addtextlf(fmt, l, "%s: ", type_str);
}

G_MODULE_EXPORT gboolean timeout(gpointer data)
{
	UNUSED(data);

	if(comm_recv(&commt, &commcallback)){
		/* TODO: disco */
		/*comm_close(&commt);*/
	}

	return TRUE; /* must be `true' */
}

static int getobjects(GtkBuilder *b)
{
#define GET_WIDGET(x) GTK_WIDGET(gtk_builder_get_object(b, x))
	entryHost = GET_WIDGET("entryHost");
	entryIn   = GET_WIDGET("entryIn");
	entryName = GET_WIDGET("entryName");
	txtMain   = GET_WIDGET("txtMain");
	/*(GtkWidget *)g_object_get_data(G_OBJECT(winmain), "entryIn");*/

	if(!entryHost || !entryIn || !txtMain)
		return 1;
	return 0;
}

int main(int argc, char **argv)
{
	GError     *error = NULL;
	GtkBuilder *builder;

	comm_init(&commt);

	/* Init GTK+ */
	gtk_init(&argc, &argv);

	/* Create new GtkBuilder object */
	builder = gtk_builder_new();

	if(!gtk_builder_add_from_file(builder, COMM_GLADE, &error)){
		g_warning("%s", error->message);
		/*g_free(error);*/
		return 1;
	}

	/* Get main window pointer from UI */
	winmain = GTK_WIDGET(gtk_builder_get_object(builder, WIN_MAIN));

	/* Connect signals */
	gtk_builder_connect_signals(builder, NULL);

	if(getobjects(builder)){
		perror("couldn't get gtk object(s)");
		return 1;
	}

	/* Destroy builder, since we don't need it anymore */
	g_object_unref(G_OBJECT(builder));

	g_timeout_add(TIMEOUT, timeout, NULL);

	/* Show window - all other widgets are automatically shown by GtkBuilder */
	gtk_widget_show(winmain);

	/* Start main loop */
	gtk_main();

	return 0;
}