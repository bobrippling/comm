#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdarg.h>

/* comm includes */
#ifdef _WIN32
# include <winsock2.h>
#else
# include <arpa/inet.h>
#endif

#include "gtkutil.h"
#include "gladegen.h"
#include "../libcomm/comm.h"

#define WIN_MAIN   "winMain"
#define TIMEOUT    250
#define UNUSED(n) do (void)(n); while(0)

/* prototypes */
static int  getobjects(GtkBuilder *);
static void updatewidgets(void);
static void commcallback(enum comm_callbacktype type, const char *fmt, ...);
G_MODULE_EXPORT void on_btnConnect_clicked(   GtkButton *button, gpointer data);
G_MODULE_EXPORT void on_btnDisconnect_clicked(GtkButton *button, gpointer data);
G_MODULE_EXPORT void on_btnSend_clicked(      GtkButton *button, gpointer data);
G_MODULE_EXPORT void on_winMain_destroy(void);
G_MODULE_EXPORT gboolean timeout(gpointer data);

/* vars */
GtkWidget *winMain;
GtkWidget *entryHost, *entryIn, *entryName; /* Gtk_Entry */
GtkWidget *txtMain; /* GtkTextView */
GtkWidget *btnConnect, *btnDisconnect, *btnSend;
GtkWidget *treeClients;

comm_t commt;

/* events */
G_MODULE_EXPORT void on_winMain_destroy(void)
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
	updatewidgets();
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

	if(comm_connect(&commt, host, NULL /* TODO: custom port */, name))
		addtextf("Couldn't connect to %s: %s\n", host, comm_lasterr(&commt));
	else
		addtextf("Connected to %s\n", host);
	updatewidgets();
}

G_MODULE_EXPORT void on_btnSend_clicked(GtkButton *button, gpointer data)
{
	UNUSED(button);
	UNUSED(data);

	if(comm_state(&commt) == COMM_ACCEPTED){
		const char *txt = gtk_entry_get_text(GTK_ENTRY(entryIn));
		if(!strlen(txt)){
			addtext("need text!\n");
			return;
		}

		if(comm_sendmessage(&commt, txt))
			addtextf("error sending message: %s\n", comm_lasterr(&commt));
		gtk_entry_set_text(GTK_ENTRY(entryIn), "");
	}else
		addtext("not connected!\n");
	updatewidgets();
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
		TYPE(COMM_SERVER_INFO, "server info");
		TYPE(COMM_ERR,  "err");
		TYPE(COMM_RENAME,  "rename");
		TYPE(COMM_CLIENT_CONN,  "client_conn");
		TYPE(COMM_CLIENT_DISCO, "client_disco");

		case COMM_CLIENT_LIST:
		{
			struct list *l;
			clientlist_clear();
			for(l = comm_clientlist(&commt); l; l = l->next)
				clientlist_add(l->name);
			return;
		}

		case COMM_CAN_SEND:
			comm_rels(&commt);
			return;

		case COMM_CLOSED:
			updatewidgets();
			clientlist_clear();
			return;
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

	if(comm_state(&commt) != COMM_DISCONNECTED){
		if(comm_recv(&commt, &commcallback)){
			addtextf("disconnected: %s\n", comm_lasterr(&commt));
			comm_close(&commt);
		}
		updatewidgets();
	}

	return TRUE; /* must be ~0 */
}

static void updatewidgets(void)
{
	register enum commstate cs = comm_state(&commt);

	switch(cs){
		case COMM_DISCONNECTED:
			gtk_widget_set_sensitive(entryHost,      TRUE);
			gtk_widget_set_sensitive(entryName,      TRUE);
			gtk_widget_set_sensitive(entryIn,        FALSE);
			gtk_widget_set_sensitive(btnConnect,     TRUE);
			gtk_widget_set_sensitive(btnDisconnect,  FALSE);
			gtk_widget_set_sensitive(btnSend,        FALSE);
			break;

		case CONN_CONNECTING:
		case COMM_VERSION_WAIT:
		case COMM_NAME_WAIT:
		case COMM_ACCEPTED:
			gtk_widget_set_sensitive(entryHost,      FALSE);
			gtk_widget_set_sensitive(entryName,      FALSE);
			gtk_widget_set_sensitive(btnConnect,     FALSE);
			gtk_widget_set_sensitive(btnDisconnect,  TRUE);

			gtk_widget_set_sensitive(entryIn,        cs == COMM_ACCEPTED);
			gtk_widget_set_sensitive(btnSend,        cs == COMM_ACCEPTED);
			break;

	}
}

static int getobjects(GtkBuilder *b)
{
#define GET_WIDGET(x) \
	if(!((x) = GTK_WIDGET(gtk_builder_get_object(b, #x)))){ \
		fputs("Error: Couldn't get Gtk Widget \"" #x "\", bailing\n", stderr); \
		return 1; \
	}
	/*(GtkWidget *)g_object_get_data(G_OBJECT(winMain), "entryIn");*/
	GET_WIDGET(entryHost);
	GET_WIDGET(entryIn);
	GET_WIDGET(entryName);
	GET_WIDGET(txtMain);

	GET_WIDGET(btnConnect);
	GET_WIDGET(btnDisconnect);
	GET_WIDGET(btnSend);

	GET_WIDGET(treeClients);

	GET_WIDGET(winMain);

	return 0;
#undef GET_WIDGET
}

int main(int argc, char **argv)
{
	GError     *error = NULL;
	GtkBuilder *builder;
	int i;

	for(i = 1; i < argc; i++)
#if _WIN32
		if(!strcmp(argv[i], "-d"))
			FreeConsole();
		else
#endif
			goto usage;

	comm_init(&commt);

	gtk_init(&argc, &argv); /* bail here if !$DISPLAY */

	builder = gtk_builder_new();

	if(gladegen_init())
		return 1;

	if(!gtk_builder_add_from_file(builder, COMM_GLADE, &error)){
		g_warning("%s", error->message);
		/*g_free(error);*/
		return 1;
	}
	gladegen_term();

	if(getobjects(builder))
		return 1;

	gtk_builder_connect_signals(builder, NULL);

	clientlist_init();

	/* don't need it anymore */
	g_object_unref(G_OBJECT(builder));

	/* signal/timeout setup */
	g_timeout_add(TIMEOUT, timeout, NULL);
	g_signal_connect(G_OBJECT(winMain), "destroy", G_CALLBACK(on_winMain_destroy), NULL);

	updatewidgets();

	gtk_widget_show(winMain);

	gtk_main();

	return 0;
usage:
	fprintf(stderr, "Usage: %s [-d]\n"
		"  -d: Debug (Keep console window open)\n",
		*argv);
	return 1;
}
