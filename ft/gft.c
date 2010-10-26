#include <gtk/gtk.h>
#include <string.h>
#include <alloca.h>

#include <stdio.h>
#include <stdarg.h>

#define WIN_MAIN   "winFT"
#define TIMEOUT    250

#include "libft/ft.h"
#include "gladegen.h"

void status(const char *, ...);
int callback(struct filetransfer *ft, enum ftstate state,
		size_t bytessent, size_t bytestotal);
void settimeout(int on);

GtkWidget *btnSend, *btnConnect, *btnListen;
GtkWidget *winMain;
GtkWidget *progressft, *lblStatus;
GtkWidget *entryFile, *cboHost;

struct filetransfer ft;
double lastfraction = 0;


/* events */
G_MODULE_EXPORT gboolean on_winMain_destroy(void)
{
	ft_close(&ft);
	gtk_main_quit(); /* gtk exit here only */
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
		if(ft_listen(&ft, iport))
			status("Couldn't listen: %s", ft_lasterr(&ft));
		else{
			status("Awaiting connection...");
			settimeout(1);
		}
		// TODO: cmds
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

	if(ft_connect(&ft, host, port))
		status("Couldn't connect: %s", ft_lasterr(&ft));
	else
		status("Connected to %s", host);
	// TODO: cmds

	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnSend_clicked(void)
{
	const char *fname = gtk_entry_get_text(GTK_ENTRY(entryFile));

	settimeout(0);

	lastfraction = 0;

	if(ft_send(&ft, callback, fname))
		status("Couldn't send %s: %s", fname, ft_lasterr(&ft));
	else
		status("Sent %s", fname);

	return FALSE;
}

G_MODULE_EXPORT gboolean
timeout(gpointer data)
{
	(void)data;

	if(ft_accept(&ft, 0)){
		const char *err = ft_lasterr(&ft);
		if(err){
			status("Couldn't accept connection: %s", err);
			return FALSE;
		}/* else timeout for accept() */
	}else{
		status("Got connection");
		if(ft_recv(&ft, callback))
			status("Couldn't recveive file: %s", ft_lasterr(&ft));
		else
			status("Recieved %s", "TODO"); /* TODO */
		ft_close(&ft);
		return FALSE; /* kill timer */
	}

	return TRUE;
}

void settimeout(int on)
{
	static int id = -1;

	if(on)
		g_timeout_add(100, timeout, NULL);
	else if(id != -1){
		g_source_remove(id);
		id = -1;
	}
}

int callback(struct filetransfer *ft, enum ftstate state,
		size_t bytessent, size_t bytestotal)
{
	const double fraction = (double)bytessent / (double)bytestotal;

	if(state == FT_WAIT){
		status("waiting for inital data...");
		return;
	}

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressft), fraction);
	status("%s: %dK/%dK (%2.2f%%)", ft_fname(ft),
			bytessent / 1024, bytestotal / 1024, 100.0f * fraction);

	while(gtk_events_pending())
		gtk_main_iteration();

	return 0; // TODO: cancel
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
	GET_WIDGET(winMain);
	GET_WIDGET(cboHost);
	GET_WIDGET(entryFile);
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
