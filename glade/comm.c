#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdarg.h>

/* comm includes */
#include <poll.h>
#include <arpa/inet.h>

#include "gtkutil.h"
#include <comm.h>

#define COMM_GLADE "main.glade"
#define WIN_MAIN   "winMain"
#define TIMEOUT    250
#define UNUSED(n) do (void)(n); while(0)

/* prototypes */
/* TODO */

/* vars */
GtkWidget  *winmain;
GtkWidget  *entryHost, *entryIn, *entryName; /* Gtk_Entry */
GtkWidget  *txtMain; /* GtkTextView */

comm_t commt;


/* events */
G_MODULE_EXPORT void on_close(void)
{
	if(comm_state(&commt) != COMM_DISCONNECTED)
		comm_close(&commt);
	gtk_main_quit();
}

G_MODULE_EXPORT void on_btnDisconnect_clicked(GtkButton *button, gpointer data)
{
	UNUSED(button);
	UNUSED(data);

	if(comm_state(&commt) != COMM_DISCONNECTED)
		comm_close(&commt);
}

G_MODULE_EXPORT void on_btnConnect_clicked(GtkButton *button, gpointer data)
{
	const char *host = gtk_entry_get_text(GTK_ENTRY(entryHost));
	const char *name = gtk_entry_get_text(GTK_ENTRY(entryName));

	UNUSED(button);
	UNUSED(data);


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
}

G_MODULE_EXPORT void on_btnSend_clicked(GtkButton *button, gpointer data)
{
	UNUSED(button);
	UNUSED(data);

	if(comm_state(&commt) == COMM_ACCEPTED){
		if(comm_sendmessage(&commt, gtk_entry_get_text(GTK_ENTRY(entryIn))))
			addtextf("error sending message: %s\n", comm_lasterr(&commt));
	}else
		addtext("not connected!\n");
}

static void commcallback(enum comm_callbacktype type, const char *fmt, ...)
{
	const char *type_str = "unknown";
	char *insertme, *insertmel;
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

	va_start(l, fmt);
	insertmel = g_strdup_vprintf(fmt, l);
	va_end(l);

	insertme = g_strconcat(type_str, ": ", insertmel, "\n", NULL);
	g_free(insertmel);

	addtext(insertme);

	g_free(insertme);
}

G_MODULE_EXPORT gboolean timeout(gpointer data)
{
	UNUSED(data);

	if(comm_recv(&commt, &commcallback)){
		addtextf("disconnected: %s\n", comm_lasterr(&commt));
		on_btnDisconnect_clicked(NULL, NULL);
	}

	return TRUE; /* must be ~0 */
}

/* end of new hotness, start of old+busted */

static int getobjects(GtkBuilder *b)
{
#define GET_WIDGET(x) if(!((x) = GTK_WIDGET(gtk_builder_get_object(b, #x)))) return 1
	GET_WIDGET(entryHost);
	GET_WIDGET(entryIn);
	GET_WIDGET(entryName);
	GET_WIDGET(txtMain);
	/*(GtkWidget *)g_object_get_data(G_OBJECT(winmain), "entryIn");*/

	return 0;
#undef GET_WIDGET
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
