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
#include "../log/log.h"
#include "../cfg/cfg.h"
#include "../libcomm/comm.h"
#include "../config.h"

#define WIN_MAIN   "winMain"
#define TIMEOUT    250
#define UNUSED(n) do (void)(n); while(0)

/* On the TODO */
#define COLOUR_UNKNOWN "#000000"
#define COLOUR_ERR     "#FF0000"
#define COLOUR_INFO    "#00DD00"
#define COLOUR_MSG     "#0000FF"

/* prototypes */
static int  getobjects(GtkBuilder *);
static void updatewidgets(void);
static void commcallback(enum comm_callbacktype type, const char *fmt, ...);
static void cfg2txt(void), txt2cfg(void);
static const char *gtk_color_to_rgb(GdkColor *col);

G_MODULE_EXPORT gboolean on_btnConnect_clicked        (GtkButton *button, gpointer data);
G_MODULE_EXPORT gboolean on_btnDisconnect_clicked     (GtkButton *button, gpointer data);
G_MODULE_EXPORT gboolean on_btnSend_clicked           (GtkButton *button, gpointer data);
G_MODULE_EXPORT gboolean on_entryName_activate        (GtkEntry *ent,     gpointer data);
G_MODULE_EXPORT gboolean on_entryName_focus_out_event (GtkEntry *ent,     gpointer data);
G_MODULE_EXPORT gboolean on_winMain_destroy           (void);
G_MODULE_EXPORT gboolean timeout                      (gpointer data);


/* vars */
GtkWidget *winMain, *colorseldiag;
GtkWidget *entryHost, *entryIn, *entryName; /* Gtk_Entry */
GtkWidget *txtMain; /* GtkTextView */
GtkWidget *btnConnect, *btnDisconnect, *btnSend;
GtkWidget *treeClients;
GtkWidget *colorsel;

comm_t commt;

/* events */
G_MODULE_EXPORT gboolean on_winMain_destroy(void)
{
	if(comm_state(&commt) != COMM_DISCONNECTED)
		comm_close(&commt);

	txt2cfg();
	config_write();

	gtk_main_quit();
	return FALSE;
}

G_MODULE_EXPORT int on_winMain_focus_in_event(GtkWindow *win, gpointer data)
{
	UNUSED(win);
	UNUSED(data);
	gtk_window_set_urgency_hint(GTK_WINDOW(winMain), FALSE);
	return FALSE;
}


/* buttans ------------- */


G_MODULE_EXPORT gboolean on_btnDisconnect_clicked(GtkButton *button, gpointer data)
{
	UNUSED(button);
	UNUSED(data);

	if(comm_state(&commt) != COMM_DISCONNECTED)
		comm_close(&commt);
	updatewidgets();
	return FALSE;
}

G_MODULE_EXPORT gboolean on_btnConnect_clicked(GtkButton *button, gpointer data)
{
	const char *host = gtk_entry_get_text(GTK_ENTRY(entryHost));
	const char *name = gtk_entry_get_text(GTK_ENTRY(entryName));
	char *hostdup = alloca(strlen(host)+1);
	char *port = NULL;

	UNUSED(button);
	UNUSED(data);

	strcpy(hostdup, host);

	if(!*name || !*hostdup){
		if(!*name)
			addtext(COLOUR_ERR, "Need name\n");
		else
			addtext(COLOUR_ERR, "Need host\n");
		return FALSE;
	}

	if((port = strchr(hostdup, ':')))
		*port++ = '\0';

	if(comm_connect(&commt, hostdup, port, name))
		addtextf(COLOUR_ERR, "Couldn't connect to %s:%s: %s\n",
				hostdup, port ? port : DEFAULT_PORT, comm_lasterr(&commt));
	else
		addtextf(COLOUR_INFO, "Connected to %s\n", hostdup);
	updatewidgets();
	return FALSE;
}

G_MODULE_EXPORT gboolean on_btnSend_clicked(GtkButton *button, gpointer data)
{
	UNUSED(button);
	UNUSED(data);

	if(comm_state(&commt) == COMM_ACCEPTED){
		const char *txt = gtk_entry_get_text(GTK_ENTRY(entryIn));
		if(!*txt){
			addtext(COLOUR_ERR, "Need text!\n");
			return FALSE;
		}

		if(comm_sendmessage(&commt, txt) <= 0)
			addtextf(COLOUR_ERR, "Error sending message: %s\n", comm_lasterr(&commt));
		gtk_entry_set_text(GTK_ENTRY(entryIn), "");
	}else
		addtext(COLOUR_ERR, "Not connected!\n");
	updatewidgets();

	return FALSE;
}

G_MODULE_EXPORT gboolean on_entryIn_button_press_event(GtkWidget *widget,
		GdkEventButton *event, gpointer func_data)
{
	UNUSED(widget);
	UNUSED(func_data);

	/* GDK_3BUTTON_PRESS for triple click - TODO: SU */
	if(event->type == GDK_2BUTTON_PRESS)
		/* show colour choser */
		gtk_widget_show(colorseldiag);

	return FALSE;
}

static const char *gtk_color_to_rgb(GdkColor *col)
{
  static char colour_string[8] = "#"; /* #RRGGBB */

  /* 0..65536 -> scale to 0..256 */
  snprintf(colour_string + 1, sizeof colour_string, "%.2X", col->red   / 256);
  snprintf(colour_string + 3, sizeof colour_string, "%.2X", col->green / 256);
  snprintf(colour_string + 5, sizeof colour_string, "%.2X", col->blue  / 256);

	return colour_string;
}

G_MODULE_EXPORT gboolean on_colorsel_ok_clicked(GtkWidget *widget)
{
#if 0
	GdkColor color;
	GtkRcrcstyle rcstyle;

	gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(colorsel), &color);

	rcstyle = gtk_rc_style_new();
	rcstyle->fg[GTK_STATE_NORMAL]             = color; /* struct cpy */
	rcstyle->color_flags[GTK_STATE_NORMAL]   |= GTK_RC_FG;

/*rcstyle->bg[GTK_STATE_NORMAL]             = color2;
	rcstyle->base[GTK_STATE_NORMAL]           = color2;
	rcstyle->text[GTK_STATE_SELECTED]         = color;
	rcstyle->color_flags[GTK_STATE_NORMAL]   |= GTK_RC_BG;
	rcstyle->color_flags[GTK_STATE_NORMAL]   |= GTK_RC_BASE;
	rcstyle->color_flags[GTK_STATE_SELECTED] |= GTK_RC_TEXT;*/

	gtk_widget_modify_style(GTK_WIDGET(entryIn), rcstyle);

	gtk_rc_style_undef(rcstyle);
#endif
	GdkColor color;

	UNUSED(widget);

	gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(colorsel), &color);
	gtk_widget_modify_text(GTK_WIDGET(entryIn), GTK_STATE_NORMAL, &color);
	/*                ^---: text, base, bg and fg are available */

	comm_colour(&commt, gtk_color_to_rgb(&color));
	gtk_widget_hide(colorseldiag);

	return FALSE;
}

G_MODULE_EXPORT gboolean on_colorsel_cancel_clicked(GtkWidget *widget)
{
	UNUSED(widget);
	gtk_widget_hide(colorseldiag);
	return FALSE;
}


/* textbox events ------------------- */


G_MODULE_EXPORT gboolean on_entryName_activate(GtkEntry *ent, gpointer data)
{
	UNUSED(ent);
	UNUSED(data);

	if(comm_state(&commt) == COMM_ACCEPTED)
		/* rename */
		return on_entryName_focus_out_event(NULL, NULL);
	else
		return on_btnConnect_clicked(NULL, NULL);
}

G_MODULE_EXPORT gboolean on_entryName_focus_out_event(GtkEntry *ent, gpointer data)
{
	UNUSED(ent);
	UNUSED(data);

	if(comm_state(&commt) == COMM_ACCEPTED){
		const char *newname = gtk_entry_get_text(GTK_ENTRY(entryName));

		if(!*newname)
			addtext(COLOUR_ERR, "Need name\n");
		else if(strcmp(newname, comm_getname(&commt))){
			comm_rename(&commt, newname);
			addtextf(COLOUR_INFO, "Attempting rename to \"%s\"...\n", newname);
		}else
			return FALSE; /* names are the same, just lost focus */

		/* temp revert/reset */
		gtk_entry_set_text(GTK_ENTRY(entryName), comm_getname(&commt));
	}
	return FALSE; /* pass signal on */
}


/* callbacks/non-events ------------- */


static void commcallback(enum comm_callbacktype type, const char *fmt, ...)
{
	const char *type_str = NULL, *col = NULL;
	char *insertme, *insertmel;
	va_list l;

#define TYPE(e, s, c) case e: type_str = s; col = c; break
	switch(type){
		TYPE(COMM_INFO,         "Info",         COLOUR_INFO);
		TYPE(COMM_SERVER_INFO,  "Server Info",  COLOUR_INFO);
		TYPE(COMM_RENAME,       "Rename",       COLOUR_INFO);
		TYPE(COMM_CLIENT_CONN,  "Client Comm",  COLOUR_INFO);
		TYPE(COMM_CLIENT_DISCO, "Client Disco", COLOUR_INFO);
		TYPE(COMM_ERR,          "Error",        COLOUR_ERR);

		case COMM_CLIENT_LIST:
		{
			struct list *l;
			clientlist_clear();
			for(l = comm_clientlist(&commt); l; l = l->next)
				clientlist_add(l->name);
			return;
		}

		case COMM_SELF_RENAME:
			gtk_entry_set_text(GTK_ENTRY(entryName),
					comm_getname(&commt));
			addtextf(COLOUR_INFO, "Renamed to %s\n", comm_getname(&commt));
			return;

		case COMM_CLOSED:
			updatewidgets();
			clientlist_clear();
			return;

		case COMM_MSG:
			break;
	}
#undef TYPE

	va_start(l, fmt);
	insertmel = g_strdup_vprintf(fmt, l);
	va_end(l);

	if(type_str)
		insertme = g_strconcat(type_str, ": ", insertmel, "\n", NULL);
	else
		insertme = g_strconcat(insertmel, "\n", NULL);

	if(type == COMM_MSG){
		char *colon = strchr(insertmel, ':');
		if(colon){
			*colon = '\0';
			if(!(col = comm_getcolour(&commt, insertmel)))
				/* NULL if they have no colour set */
				col = COLOUR_MSG;
			*colon = ':';
		}
	}
	if(!col)
		col = COLOUR_UNKNOWN;
	addtext(col, insertme);

	if(type == COMM_MSG)
		log_add(insertmel);

	g_free(insertme);
	g_free(insertmel);

	switch(type){
		case COMM_MSG:
		case COMM_INFO:
		case COMM_SERVER_INFO:
		case COMM_ERR:
		case COMM_RENAME:
		case COMM_CLIENT_CONN:
		case COMM_CLIENT_DISCO:
			if(!gtk_window_is_active(GTK_WINDOW(winMain)))
				gtk_window_set_urgency_hint(GTK_WINDOW(winMain), TRUE);

		case COMM_SELF_RENAME:
		case COMM_CLIENT_LIST:
		case COMM_CLOSED:
			break;
	}
}

G_MODULE_EXPORT gboolean timeout(gpointer data)
{
	UNUSED(data);

	if(comm_state(&commt) != COMM_DISCONNECTED){
		if(comm_recv(&commt, &commcallback)){
			addtextf(COLOUR_INFO, "Disconnected: %s\n", comm_lasterr(&commt));
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
			gtk_widget_set_sensitive(btnConnect,     FALSE);
			gtk_widget_set_sensitive(btnDisconnect,  TRUE);

			gtk_widget_set_sensitive(entryName,      cs == COMM_ACCEPTED);
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

	GET_WIDGET(colorseldiag);
	GET_WIDGET(colorsel);

	return 0;
#undef GET_WIDGET
}

static void cfg2txt()
{
	const char *host = config_lasthost();

	gtk_entry_set_text(GTK_ENTRY(entryName), config_name());

	if(*host){
		const char *port = config_port();

		if(*port){
			char *hp;

			hp = g_strconcat(host, ":", port, NULL);
			gtk_entry_set_text(GTK_ENTRY(entryHost), hp);
			g_free(hp);
		}else
			gtk_entry_set_text(GTK_ENTRY(entryHost), host);
	}
}

static void txt2cfg()
{
	char *host, *port;

	config_setname(gtk_entry_get_text(GTK_ENTRY(entryName)));

	host = g_strdup(gtk_entry_get_text(GTK_ENTRY(entryHost)));
	if(host){
		port = strchr(host, ':');
		if(port){
			*port++ = '\0';
			config_setport(port);
		}else
			config_setport("");

		config_setlasthost(host);
		g_free(host);
	}
}

int main(int argc, char **argv)
{
	GError     *error = NULL;
	GtkBuilder *builder;
	int i;
#ifdef _WIN32
	int debug = 0;
#endif

	for(i = 1; i < argc; i++)
#ifdef _WIN32
		if(!strcmp(argv[i], "-d"))
			debug = 1;
		else
#endif
			goto usage;

	if(config_read())
		return 1;

	if(log_init()){
		perror("log_init()");
		return 1;
	}

#ifdef _WIN32
	if(!debug)
		FreeConsole();
#endif

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

	cfg2txt();

	gtk_widget_show(winMain);

	gtk_main();

	return 0;
usage:
	fprintf(stderr,
#ifdef _WIN32
		"Usage: %s [-d]\n"
		"  -d: Debug (Keep console window open)\n"
#else
		"Usage: %s\n"
#endif
		, *argv);
	return 1;
}
