#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include <errno.h>
#include <stdarg.h>

/* comm includes */
#ifdef _WIN32
# include <winsock2.h>
#else
# include <arpa/inet.h>

# include <sys/stat.h>
# include <fcntl.h>
#endif

#include "gtkutil.h"
#include "../log/log.h"
#include "../cfg/cfg.h"
#include "../libcomm/comm.h"
#include "../gcommon/glist.h"
#include "gladegen.h"
#include "priv.h"
#include "../config.h"
#include "drawan.h"

#define WIN_MAIN       "winMain"
#define GLADE_XML_FILE "tmp.glade"
#define TIMEOUT    250

#define COLOUR_UNKNOWN "#000000"
#define COLOUR_ERR     "#FF0000"
#define COLOUR_INFO    "#00DD00"
#define COLOUR_MSG     "#0000FF"
#define COLOUR_PRIV    "#000000"

/* prototypes */
static int  getobjects(GtkBuilder *);
static void updatewidgets(void);
static void commcallback(enum comm_callbacktype type, const char *fmt, ...);
static void cfg2txt(void), txt2cfg(void);
static const char *gtk_color_to_rgb(GdkColor *col);
static void        gui_set_colour(void);

G_MODULE_EXPORT gboolean on_btnConnect_clicked        (GtkButton *button, gpointer data);
G_MODULE_EXPORT gboolean on_btnDisconnect_clicked     (GtkButton *button, gpointer data);
G_MODULE_EXPORT gboolean on_btnSend_clicked           (GtkButton *button, gpointer data);
G_MODULE_EXPORT gboolean on_entryName_activate        (GtkEntry *ent,     gpointer data);
G_MODULE_EXPORT gboolean on_entryName_focus_out_event (GtkEntry *ent,     gpointer data);
G_MODULE_EXPORT gboolean on_txtMain_key_press_event   (GtkWidget *widget, GdkEventKey *event, gpointer func_data);
G_MODULE_EXPORT gboolean on_txtMain_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer func_data);
G_MODULE_EXPORT gboolean on_winMain_destroy           (void);
G_MODULE_EXPORT gboolean timeout                      (gpointer data);


/* vars */
static GtkWidget *winMain, *colorseldiag;
static GtkWidget *entryHost, *entryIn, *entryName; /* Gtk_Entry */
static GtkWidget *btnConnect, *btnDisconnect, *btnSend;
static GtkWidget *colorsel;
GtkWidget *vboxMain;
GtkWidget *drawanArea;

static int coloursel_is_text = FALSE;

static comm_t   commt;
static GdkColor var_color;

static GtkListStore *clientlist = NULL;
static GtkWidget *treeClients;

struct
{
	int priv_win;
} config = { 1 };

/* extern'd */
GtkWidget *txtMain; /* GtkTextView */


/* events */
G_MODULE_EXPORT gboolean on_btnClear_clicked(void)
{
	draw_clear();
	comm_draw_clear(&commt);
	return FALSE;
}

G_MODULE_EXPORT gboolean on_btnColour_clicked(void)
{
	coloursel_is_text = FALSE;
	gtk_widget_show(colorseldiag);
	return FALSE;
}

G_MODULE_EXPORT gboolean on_drawwidth_change(void)
{

	return FALSE;
}

G_MODULE_EXPORT gboolean on_winMain_destroy(void)
{
	/* program quit +comm_free takes place at the end of main */
	txt2cfg();
	config_write();

	gtkutil_cleanup();

	draw_term();

	gtk_main_quit(); /* gtk exit here only */
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
			addtext(COLOUR_ERR, "Pray tell, who art thou?\n");
		else
			addtext(COLOUR_ERR, "Pray, to where am I to connect?\n");
		return FALSE;
	}

	if((port = strchr(hostdup, ':')))
		*port++ = '\0';

	/*
	 * TODO
	 *
	 * call comm_connect (async)
	 * call gtk_main()
	 * when comm_callback, if connected/failure,
	 *   then gtk_main_quit()
	 * flow then returns here to handle "connected to..."
	 *
	 * OR
	 *
	 *   #include <gtk/gtkmain.h>
	 *   while(1){
	 *     if(gtk_events_pending())
	 *         gtk_main_iteration();
	 *     comm_checkconnected();
	 *   }
	 *
	 *
	 */
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
		int send = 0;

		if(!*txt){
			addtext(COLOUR_ERR, "My good sir, it is required that you enter text\n");
			return FALSE;
		}

		if(*txt == '/')
			if(!strncmp(txt+1, "su", 2))
				if(txt[3] == '\0')
					comm_su(&commt, ""); /* drop */
				else
					comm_su(&commt, txt+4);
			else if(!strncmp(txt+1, "kick ", 5))
				comm_kick(&commt, txt+6);
			else if(!strncmp(txt+1, "msg ", 4)){
				char *dup = alloca(strlen(txt) + 1);
				char *name, *msg;
				strcpy(dup, txt);
				name = dup + 5;
				msg = strchr(name, ' ');
				if(!msg)
					addtext(COLOUR_ERR, "PrivMsg: Need space and a message after the name\n");
				else{
					*msg++ = '\0';
					comm_privmsg(&commt, name, msg);
				}
			}else
				send = 1;
		else
			send = 1;

		if(send && comm_sendmessage(&commt, txt) <= 0)
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
	if(event->type == GDK_2BUTTON_PRESS){
		/* show colour choser */
		coloursel_is_text = TRUE;
		gtk_widget_show(colorseldiag);
	}

	return FALSE;
}

G_MODULE_EXPORT gboolean on_txtMain_button_press_event(
		GtkWidget *widget, GdkEventButton *event, gpointer func_data)
{
	GtkTextIter iter;
	const char *link;

	UNUSED(widget);
	UNUSED(func_data);

	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(txtMain),
			&iter, event->x, event->y);

	if((link = iterlink(&iter))){
#ifdef _WIN32
		HINSTANCE ret;

		if(
				(int)(ret =
				 ShellExecute(NULL, "open", link, NULL, NULL, SW_SHOW /* 5 */))
				<= 32)
			fprintf(stderr, "ShellExecute() failed: %d\n", ret);
#else
		const char *browser = getenv("BROWSER");
		int devnull;

		if(!browser)
			browser = DEFAULT_BROWSER;

		switch(fork()){
			case -1:
				perror("fork()");
				addtextf(COLOUR_ERR, "fork(): %s\n", strerror(errno));
				break;
			case 0:
				devnull = open("/dev/null", O_APPEND);
				dup2(devnull, STDOUT_FILENO); /* leave open because of perror below? */
				dup2(devnull, STDERR_FILENO);
				close(devnull);

				execlp(browser, browser, link, (char *)NULL);
				perror("execlp()");
				exit(-1);
		}
#endif
	}

	return FALSE;
}

G_MODULE_EXPORT gboolean on_txtMain_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer func_data)
{
	UNUSED(widget);
	UNUSED(event);
	UNUSED(func_data);

	gtk_widget_grab_focus(entryIn);

	if(isprint(event->keyval)){
		const char *txt = gtk_entry_get_text(GTK_ENTRY(entryIn));
		int len = strlen(txt) + 1;
		char *new = alloca(len + 1);

		strcpy(new, txt);
		new[len-1] = event->keyval;
		new[len  ] = '\0';

		gtk_entry_set_text(GTK_ENTRY(entryIn), new);
		gtk_editable_set_position(GTK_EDITABLE(entryIn), -1 /* last */);
	}

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
	UNUSED(widget);

	gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(colorsel), &var_color);

	if(coloursel_is_text)
		gui_set_colour();
	else{
		draw_set_colour(&var_color);
		//config_setdrawcolour(); TODO
	}

	gtk_widget_hide(colorseldiag);

	return FALSE;
}

static void gui_set_colour()
{
	const char *scol;

	gtk_widget_modify_text(GTK_WIDGET(entryIn), GTK_STATE_NORMAL, &var_color);
	/*                ^---: text, base, bg and fg are available */

	scol = gtk_color_to_rgb(&var_color);

	config_setcolour(scol);
	if(comm_state(&commt) == COMM_ACCEPTED)
		comm_colour(&commt, scol);
}

G_MODULE_EXPORT gboolean on_colorsel_cancel_clicked(GtkWidget *widget)
{
	UNUSED(widget);
	gtk_widget_hide(colorseldiag);
	return FALSE;
}


/* textbox events ------------------- */


G_MODULE_EXPORT gboolean on_entryHost_activate(GtkEntry *ent, gpointer data)
{
	return on_entryName_activate(ent, data);
}

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
	char *insertme, *insertmel, *from_name;
	int logadd = 0;
	va_list l;

#define TYPE(e, s, c, l) case e: type_str = s; col = c; logadd = l; break
	switch(type){
		case COMM_DRAW:
		{
			int x1, y1, x2, y2;
			comm_getdrawdata(fmt + 1, &x1, &y1, &x2, &y2);
			draw_brush(drawanArea, x1, y1, x2, y2);
			return;
		}

		case COMM_DRAW_CLEAR:
			draw_clear();
			return;

		TYPE(COMM_INFO,         "Info",         COLOUR_INFO, 1);
		TYPE(COMM_SERVER_INFO,  "Server Info",  COLOUR_INFO, 1);
		TYPE(COMM_RENAME,       "Rename",       COLOUR_INFO, 1);
		TYPE(COMM_CLIENT_CONN,  "Client Comm",  COLOUR_INFO, 1);
		TYPE(COMM_CLIENT_DISCO, "Client Disco", COLOUR_INFO, 1);
		TYPE(COMM_ERR,          "Error",        COLOUR_ERR,  1);

		case COMM_STATE_CHANGE:
			updatewidgets();

			switch(comm_state(&commt)){
				case COMM_DISCONNECTED:
					updatewidgets();
					glist_clear(clientlist);
					break;

				case COMM_ACCEPTED:
					gui_set_colour();
					gtk_widget_grab_focus(entryIn);
					break;

				case COMM_VERSION_WAIT:
				case COMM_NAME_WAIT:
				case COMM_CONNECTING:
					break;
			}
			return;

		case COMM_CLIENT_LIST:
		{
			struct list *l;
			glist_clear(clientlist);
			for(l = comm_clientlist(&commt); l; l = l->next)
				glist_add(clientlist, l->name);
			return;
		}

		case COMM_SELF_RENAME:
			gtk_entry_set_text(GTK_ENTRY(entryName),
					comm_getname(&commt));
			addtextf(COLOUR_INFO, "Renamed to %s\n", comm_getname(&commt));
			return;

		case COMM_MSG:
		case COMM_PRIVMSG:
			logadd = 1;
			col = COLOUR_PRIV;
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

	if(type == COMM_MSG || type == COMM_PRIVMSG){
		from_name = strchr(insertmel, ':');
		if(from_name){
			*from_name = '\0';
			if(!(col = comm_getcolour(&commt, insertmel)))
				/* NULL if they have no colour set */
				col = COLOUR_MSG;
			*from_name = ':';
		}
	}

	if(!col)
		col = COLOUR_UNKNOWN;

	if(type == COMM_PRIVMSG && from_name && config.priv_win){
		// FIXME: check for existing window
		priv_new(winMain, from_name);
	}else{
		addtext(col, insertme);

		if(logadd)
			log_add(insertmel, col);
	}

	g_free(insertme);
	g_free(insertmel);

	switch(type){
		case COMM_MSG:
		case COMM_PRIVMSG:
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
		case COMM_STATE_CHANGE:
		case COMM_DRAW:
		case COMM_DRAW_CLEAR:
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
	static   enum commstate last = COMM_DISCONNECTED;
	register enum commstate cs = comm_state(&commt);

	if(last == cs)
		return;

	switch(last = cs){
		case COMM_DISCONNECTED:
			gtk_widget_set_sensitive(entryHost,      TRUE);
			gtk_widget_set_sensitive(entryName,      TRUE);
			gtk_widget_set_sensitive(entryIn,        FALSE);
			gtk_widget_set_sensitive(btnConnect,     TRUE);
			gtk_widget_set_sensitive(btnDisconnect,  FALSE);
			gtk_widget_set_sensitive(btnSend,        FALSE);
			break;

		case COMM_ACCEPTED:
		case COMM_CONNECTING:
		case COMM_VERSION_WAIT:
		case COMM_NAME_WAIT:
			gtk_widget_set_sensitive(entryHost,      FALSE);
			gtk_widget_set_sensitive(btnConnect,     FALSE);
			gtk_widget_set_sensitive(btnDisconnect,  TRUE);

			gtk_widget_set_sensitive(entryName,      cs == COMM_ACCEPTED);
			gtk_widget_set_sensitive(entryIn,        cs == COMM_ACCEPTED);
			gtk_widget_set_sensitive(btnSend,        cs == COMM_ACCEPTED);
			break;
	}
}

int gui_draw_net(int x1, int y1, int x2, int y2, int col)
{
	if(comm_state(&commt) == COMM_ACCEPTED){
		comm_draw(&commt, x1, y1, x2, y2, col);
		return 0;
	}
	return 1;
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

	GET_WIDGET(drawanArea);
	GET_WIDGET(vboxMain);

	return 0;
#undef GET_WIDGET
}

static void cfg2txt()
{
	const char *host = config_lasthost();
	const char *col  = config_colour();

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

	if(*col){
		if(gdk_color_parse(col, &var_color))
			gui_set_colour();
		else
			fprintf(stderr, "Couldn't parse config colour \"%s\"\n", col);
	}
}

static void txt2cfg()
{
	char *host, *port;
	const char *name;

	name = gtk_entry_get_text(GTK_ENTRY(entryName));

	config_setname(name);

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

int gladegen_init(void)
{
	FILE *f = fopen(GLADE_XML_FILE, "w");
	unsigned int i;

	if(!f)
		return 1;

	for(i = 0; i < sizeof(glade_str_col)/sizeof(char *); i++)
		if(fputs(glade_str_col[i], f) == EOF){
			fclose(f);
			return 1;
		}

	if(fclose(f) == EOF)
		return 1;
	return 0;
}

void gladegen_term(void)
{
	remove(GLADE_XML_FILE);
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
	/* the usual shit */
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if(!debug)
		FreeConsole();
#else
	if(signal(SIGCHLD, SIG_IGN) == SIG_ERR)
		perror("signal(SIGCHLD)");
#endif

	memset(&var_color, '\0', sizeof var_color);

	comm_init(&commt);

	gtk_init(&argc, &argv); /* bail here if !$DISPLAY */

	builder = gtk_builder_new();

	if(gladegen_init())
		return 1;

	if(!gtk_builder_add_from_file(builder, GLADE_XML_FILE, &error)){
		g_warning("gtk_builder_add_from_file(): %s", error->message);
		g_error_free(error);
	}
	gladegen_term();

	if(getobjects(builder))
		return 1;

	gtk_builder_connect_signals(builder, NULL);

	glist_init(&clientlist, treeClients);

	draw_init();

	/* don't need it anymore */
	g_object_unref(G_OBJECT(builder));

	/* signal/timeout setup */
	g_timeout_add(TIMEOUT, timeout, NULL);
	g_signal_connect(G_OBJECT(winMain), "destroy", G_CALLBACK(on_winMain_destroy), NULL);

	updatewidgets();

	cfg2txt();

	gtk_widget_show(winMain);

	gtk_main();

	/* no gtk stuff after here - all invalid */
	if(comm_state(&commt) != COMM_DISCONNECTED)
		comm_close(&commt);

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
