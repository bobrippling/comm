#include <gtk/gtk.h>
#include <string.h>
#ifdef _WIN32
# include <malloc.h>
# include <windows.h>
#else
# include <alloca.h>
#endif

#include <stdio.h>
#include <stdarg.h>

#define WIN_MAIN   "winFT"
#define TIMEOUT    250

#define CLOSE() \
	do{ \
		ft_close(&ft); \
		state = STATE_DISCO; \
		cmds(); \
	}while(0)

#include "libft/ft.h"
#include "gladegen.h"

void cmds(void);
void status(const char *, ...);
int callback(struct filetransfer *ft, enum ftstate state,
		size_t bytessent, size_t bytestotal);
void settimeout(int on);

GtkWidget *btnSend, *btnConnect, *btnListen, *btnCancel;
GtkWidget *btnFileChoice;
GtkWidget *winMain;
GtkWidget *progressft, *lblStatus;
GtkWidget *cboHost;

struct filetransfer ft;
int cancelled = 0;
double lastfraction = 0;
enum
{
	STATE_DISCO, STATE_LISTEN, STATE_CONNECTED, STATE_TRANSFER
} state;


/* events */
G_MODULE_EXPORT gboolean on_winMain_destroy(void)
{
	CLOSE();
	gtk_main_quit(); /* gtk exit here only */
	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnCancel_clicked(void)
{
	if(state == STATE_CONNECTED || state == STATE_LISTEN){
		CLOSE();
		cancelled = 1;
		cmds();
	}else if(state == STATE_TRANSFER)
		cancelled = 1;
	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnListen_clicked(void)
{
	const char *hostret, *port;
	int iport;

	settimeout(0);

	hostret = gtk_combo_box_get_active_text(GTK_COMBO_BOX(cboHost));

	port = strchr(hostret, ':');
	if(!port)
		port = DEFAULT_PORT;
	else
		port++;

	if(sscanf(port, "%d", &iport) != 1)
		status("Invalid port number");
	else{
		if(ft_listen(&ft, iport)){
			status("Couldn't listen: %s", ft_lasterr(&ft));
			CLOSE();
		}else{
			status("Awaiting connection...");
			settimeout(1);
			state = STATE_LISTEN;
		}
		cmds();
	}

	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnConnect_clicked(void)
{
	const char *hostret;
	char *host, *port;

	settimeout(0);

	hostret = gtk_combo_box_get_active_text(GTK_COMBO_BOX(cboHost));

	if(!*hostret){
		status("Need host");
		return FALSE;
	}

	host = alloca(strlen(hostret)+1);
	strcpy(host, hostret);

	port = strchr(host, ':');
	if(port)
		*port++ = '\0';

	if(ft_connect(&ft, host, port)){
		status("Couldn't connect: %s", ft_lasterr(&ft));
		CLOSE();
	}else{
		status("Connected to %s", host);
		state = STATE_CONNECTED;
	}
	cmds();

	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnSend_clicked(void)
{
	const char *fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(btnFileChoice));

	settimeout(0);

	lastfraction = 0;

	if(ft_send(&ft, callback, fname)){
		status("Couldn't send %s: %s", fname, ft_lasterr(&ft));
		CLOSE();
	}else
		status("Sent %s", fname);

	return FALSE;
}

G_MODULE_EXPORT gboolean
timeout(gpointer data)
{
	(void)data;

	if(ft_accept(&ft, 0)){
		if(cancelled){
			status("Cancelled");
			cancelled = 0;
			return FALSE;
		}else
			if(ft_haderror(&ft)){
				status("Couldn't accept connection: %s", ft_lasterr(&ft));
				return FALSE; /* kill timer */
			}/* else timeout for accept() */
	}else{
		status("Got connection");
		if(ft_recv(&ft, callback))
			status("Couldn't recveive file: %s", ft_lasterr(&ft));
		else
			status("Recieved %s", "TODO"); /* TODO */
		CLOSE();
		return FALSE; /* kill timer */
	}

	return TRUE;
}

void cmds()
{
	switch(state){
		case STATE_DISCO:
			gtk_widget_set_sensitive(cboHost,        TRUE);
			gtk_widget_set_sensitive(btnConnect,     TRUE);
			gtk_widget_set_sensitive(btnListen,      TRUE);
			gtk_widget_set_sensitive(btnFileChoice,  TRUE);
			gtk_widget_set_sensitive(btnSend,        FALSE);
			gtk_widget_set_sensitive(btnCancel,      FALSE);
			break;

		case STATE_CONNECTED:
			gtk_widget_set_sensitive(cboHost,        FALSE);
			gtk_widget_set_sensitive(btnConnect,     FALSE);
			gtk_widget_set_sensitive(btnListen,      FALSE);
			gtk_widget_set_sensitive(btnFileChoice,  TRUE);
			gtk_widget_set_sensitive(btnSend,        TRUE);
			gtk_widget_set_sensitive(btnCancel,      TRUE);
			break;

		case STATE_LISTEN:
			gtk_widget_set_sensitive(cboHost,        FALSE);
			gtk_widget_set_sensitive(btnConnect,     FALSE);
			gtk_widget_set_sensitive(btnListen,      FALSE);
			gtk_widget_set_sensitive(btnFileChoice,  FALSE);
			gtk_widget_set_sensitive(btnSend,        FALSE);
			gtk_widget_set_sensitive(btnCancel,      TRUE);
			break;

		case STATE_TRANSFER:
			gtk_widget_set_sensitive(cboHost,        FALSE);
			gtk_widget_set_sensitive(btnConnect,     FALSE);
			gtk_widget_set_sensitive(btnListen,      FALSE);
			gtk_widget_set_sensitive(btnFileChoice,  FALSE);
			gtk_widget_set_sensitive(btnSend,        FALSE);
			gtk_widget_set_sensitive(btnCancel,      TRUE);
			break;
	}

	if(gtk_events_pending())
		gtk_main_iteration();
}

void settimeout(int on)
{
	static int id = -1;

	if(on)
		id = g_timeout_add(100, timeout, NULL);
	else if(id != -1){
		g_source_remove(id);
		id = -1;
	}
}

int callback(struct filetransfer *ft, enum ftstate state,
		size_t bytessent, size_t bytestotal)
{
	const double fraction = (double)bytessent / (double)bytestotal;

	if(state == FT_BEGIN){
		cancelled = 0;
		state = STATE_TRANSFER;
		cmds();
	}

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressft), fraction);
	status("%s: %dK/%dK (%2.2f%%)", ft_fname(ft),
			bytessent / 1024, bytestotal / 1024, 100.0f * fraction);

	while(gtk_events_pending())
		gtk_main_iteration();

	return cancelled; // TODO: cancel
}


static int getobjects(GtkBuilder *b)
{
#define GET_WIDGET(x) \
	if(!((x) = GTK_WIDGET(gtk_builder_get_object(b, #x)))){ \
		fputs("Error: Couldn't get Gtk Widget \"" #x "\", bailing\n", stderr); \
		return 1; \
	}

	GET_WIDGET(btnSend);
	GET_WIDGET(btnConnect);
	GET_WIDGET(btnListen);
	GET_WIDGET(btnCancel);
	GET_WIDGET(btnFileChoice);
	GET_WIDGET(winMain);
	GET_WIDGET(cboHost);
	GET_WIDGET(progressft);
	GET_WIDGET(lblStatus);

	return 0;
#undef GET_WIDGET
}

void status(const char *fmt, ...)
{
	va_list l;
	char buffer[2048];

	va_start(l, fmt);
	if(vsnprintf(buffer, sizeof buffer, fmt, l) >=
			(signed)sizeof buffer){
		va_end(l);
		fputs("gft: warning: vsnprintf() string too large for buffer\n",
				stderr);
		return;
	}
	va_end(l);

	gtk_label_set_text(GTK_LABEL(lblStatus), buffer);
}

int main(int argc, char **argv)
{
	GError     *error = NULL;
	GtkBuilder *builder;

	ft_zero(&ft);
	gtk_init(&argc, &argv); /* bail here if !$DISPLAY */

#ifdef _WIN32
	{
		int debug = 0;

		if(argc == 2){
			if(!strcmp(argv[1], "-d")){
				debug = 1;
				fprintf(stderr, "%s: debug on\n",
						*argv);
			}else{
usage:
				fprintf(stderr, "Usage: %s [-d]\n",
						*argv);
				return 1;
			}
		}else if(argc != 1)
			goto usage;

		if(!debug)
			FreeConsole();
	}
#else
	if(argc != 1){
		fprintf(stderr, "Usage: %s\n", *argv);
		return 1;
	}
#endif

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
	cmds();

	gtk_main();

	return 0;
}
