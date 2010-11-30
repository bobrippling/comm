#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef _WIN32
# include <malloc.h>
# include <windows.h>
# include <shlobj.h>
#else
# include <alloca.h>
#endif

#include <stdio.h>
#include <stdarg.h>

#define WIN_MAIN       "winFT"
#define TRAY_ICON      "tray.ico"
#define GLADE_XML_FILE "tmpft.glade"
#define TIMEOUT    500

#define CLOSE() \
	do{ \
		settimeout(0); \
		ft_close(&ft); \
		gstate = STATE_DISCO; \
		cmds(); \
	}while(0)

#define STAY_OPEN() \
	do{ \
		gstate = STATE_CONNECTED; \
		cmds(); \
		settimeout(1); \
	}while(0)

#define URGENT(b) \
	do \
			if(!gtk_window_is_active(GTK_WINDOW(winMain))) \
				gtk_window_set_urgency_hint(GTK_WINDOW(winMain), b); \
	while(0)


#define QUEUE_REM(n) \
	glist_remove(listTransfers, \
			gtk_tree_view_get_model(GTK_TREE_VIEW(treeTransfers)), n)

#define QUEUE(n) \
	do{ \
		glist_add(listTransfers, n); \
		queue_add(&file_queue,   n); \
		gtk_widget_set_sensitive(btnSend, TRUE); \
	}while(0)


#include "../../config.h"

#include "gladegen.h"

#include "../util/gqueue.h"
#include "../../gcommon/glist.h"
#include "gtray.h"

#include "../libft/ft.h"

#include "../../common/gnudefs.h"

#include "gcfg.h"
#include "gtransfers.h"

void cmds(void);
void status(const char *, ...) GCC_PRINTF_ATTRIB(1, 2);
void settimeout(int on);

const char *get_openfolder(void);
void html_expand(char *s);
void shelldir(const char *d);

int callback(struct filetransfer *ft, enum ftstate state,
		size_t bytessent, size_t bytestotal);
int queryback(struct filetransfer *ft, enum ftquery qtype,
		const char *msg, ...);
char *fnameback(struct filetransfer *ft,
		char *fname);

static GtkWidget *btnSend, *btnConnect, *btnListen, *btnClose;
static GtkWidget *btnFileChoice, *btnOpenFolder, *btnClearTransfers;

static GtkWidget *progressft, *lblStatus;
static GtkWidget *cboHost;

static GtkWidget *frmSend;

static GtkWidget    *treeDone,      *treeTransfers;
static GtkListStore *listTransfers, *listDone;

static GtkWidget *winMain;


GtkWidget *btnDirChoice; /* extern'd */


struct filetransfer ft;
struct queue *file_queue = NULL;
int cancelled = 0;
double lastfraction = 0;
enum
{
	STATE_DISCO, STATE_LISTEN, STATE_CONNECTED, STATE_TRANSFER
} gstate;


/* events */
G_MODULE_EXPORT gboolean
on_frmSend_drag_data_received(
		GtkWidget          *widget,
		GdkDragContext     *context,
		gint                x,
		gint                y,
		GtkSelectionData   *datasel,
		guint               type,
		guint               time)
{
	if(datasel && datasel->length >= 0){
		char *dup, *iter;

		if(context->action == GDK_ACTION_ASK)
			context->action = GDK_ACTION_COPY;

		/*
		 * data looks like this:
		 *
		 * (char *){
		 *   "file:///home/rob/Desktop/file1.txt\n\r?"
		 *   "file:///home/rob/Desktop/file%202.txt\n\r?"
		 * };
		 *
		 */

		dup = g_strdup(datasel->data);

		for(iter = strtok(dup, "\r\n"); iter; iter = strtok(NULL, "\r\n"))
			if(!strncmp(iter, "file://", 7)){
				char *p = iter + 7;
				html_expand(p);
				QUEUE(p);
			}else
				fprintf(stderr, "warning: can't add \"%s\" to queue\n", iter);

		g_free(dup);
	}

	return FALSE;
}

G_MODULE_EXPORT gboolean
on_winMain_delete_event(void)
{
	/* return false to allow destroy */
	if(cfg_get_close_to_tray()){
		tray_toggle();
		return TRUE;
	}
	return FALSE;
}

G_MODULE_EXPORT gboolean
on_winMain_destroy(void)
{
	CLOSE();
	cfg_write(); /* must be before gtk shutdown */
	gtk_main_quit(); /* gtk exit here only */
	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnOpenFolder_clicked(void)
{
	shelldir(get_openfolder());
	return FALSE;
}

G_MODULE_EXPORT gboolean
btnClearTransfers_clicked(void)
{
	glist_clear(listDone);
	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnClose_clicked(void)
{
	if(gstate == STATE_CONNECTED || gstate == STATE_LISTEN){
		CLOSE();
		cancelled = 1;
	}else if(gstate == STATE_TRANSFER)
		cancelled = 1;
	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnListen_clicked(void)
{
	char *hostret, *port;
	int iport;

	settimeout(0);

	hostret = gtk_combo_box_get_active_text(GTK_COMBO_BOX(cboHost));

	port = strchr(hostret, ':');
	if(!port)
		port = FT_DEFAULT_PORT;
	else
		port++;

	if(sscanf(port, "%d", &iport) != 1)
		status("Port _number_ ya fool...");
	else{
		if(ft_listen(&ft, iport)){
			status("Couldn't listen: %s", ft_lasterr(&ft));
			CLOSE();
		}else{
			status("Awaiting connection...");
			settimeout(1);
			gstate = STATE_LISTEN;
		}
		cmds();
	}

	g_free(hostret);
	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnConnect_clicked(void)
{
	char *host, *port;

	settimeout(0);

	/* alloced */
	host = gtk_combo_box_get_active_text(GTK_COMBO_BOX(cboHost));

	if(!host){
		status("Couldn't get host");
		return FALSE;
	}else if(!*host){
		status("To where am I to connect? Timbuktu?");
		return FALSE;
	}

	cfg_add(host);

	port = strchr(host, ':');
	if(port)
		*port++ = '\0';

	if(ft_connect(&ft, host, port)){
		status("Couldn't connect: %s", ft_lasterr(&ft));
		CLOSE();
	}else{
		status("Connected to %s", host);
		gstate = STATE_CONNECTED;
		settimeout(1);
	}
	URGENT(1);
	cmds();

	g_free(host);

	return FALSE;
}


G_MODULE_EXPORT gboolean
on_btnDequeue_clicked(void)
{
	char *sel = glist_selected(GTK_TREE_VIEW(treeTransfers));

	if(!sel){
		status("Uh... select a queue'd file if you please");
		return FALSE;
	}

	/* gui list */
	QUEUE_REM(sel);

	/* internal linked list */
	queue_rem(&file_queue, sel);

	if(queue_len(file_queue) == 0)
		gtk_widget_set_sensitive(btnSend, FALSE);

	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnQueue_clicked(void)
{
	const char *fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(btnFileChoice));

	if(!fname){
		status("Need file to queue ya crazy man");
		return FALSE;
	}

	QUEUE(fname);

	return FALSE;
}

G_MODULE_EXPORT gboolean
on_btnSend_clicked(void)
{
	char *item;

	if(!ft_connected(&ft)){
		status("Not connected ya fool!");
		return FALSE;
	}

	settimeout(0);
	lastfraction = 0;

	while((item = queue_next(&file_queue))){
		const char *basename = strrchr(item, G_DIR_SEPARATOR);

		QUEUE_REM(item);

		if(!basename++)
			basename = item;

		if(ft_send(&ft, callback, item)){
			status("Couldn't send %s: %s", basename, ft_lasterr(&ft));
			CLOSE();
			break;
		}else{
			status("Sent %s", basename);
			STAY_OPEN();
		}
	}

	gtk_widget_set_sensitive(btnSend, FALSE);

	return FALSE;
}

G_MODULE_EXPORT gboolean
on_treeDoneTransfers_row_activated(GtkTreeView *tree_view,
		GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	struct transfer *transfer;
	gint *indices;

	(void)tree_view;
	(void)column;
	(void)user_data;

	indices = gtk_tree_path_get_indices(path);

	if(indices){
		transfer = transfers_get(*indices);
		if(transfer){
			if(transfer->is_recv)
				status("Recieved %s", transfer->fname);
			else
				status("Sent %s", transfer->fname);
			shelldir(transfer->path);
		}else
			status("Couldn't get path for %s");
	}

	return FALSE;
}

G_MODULE_EXPORT gboolean
timeout(gpointer data)
{
	(void)data;

	if(gstate == STATE_CONNECTED){
		switch(ft_poll_recv_or_close(&ft)){
			case FT_YES:
				/*
				 * state = STATE_TRANSFER;
				 * cmds();
				 * no need for this - set in the callback straight away
				 */

				if(!ft_poll_connected(&ft)){
					/* connection closed */
					status("Connection closed");
					CLOSE();
					return FALSE; /* timer death */
				}

				if(ft_recv(&ft, callback, queryback, fnameback)){
					status("Couldn't recieve file: %s", ft_lasterr(&ft));
					CLOSE();
				}else{
					/*
					 * status("Recieved %s", ft_truncname(&ft, 32));
					 * don't do ^ here, it's done in the callback
					 */
					STAY_OPEN();
				}
				return FALSE; /* kill timer */

			case FT_ERR:
				status("ft_poll() error: %s", ft_lasterr(&ft));
				CLOSE();
				return FALSE;

			case FT_NO:
				return TRUE; /* keep looking */
		}
		/* unreachable */
	}else{
		enum ftret ftr = ft_accept(&ft, 0);

		if(cancelled){
			status("Cancelled");
			cancelled = 0;
			return FALSE;
		}else
			switch(ftr){
				case FT_ERR:
					status("Couldn't accept connection: %s", ft_lasterr(&ft));
					return FALSE; /* kill timer */

				case FT_YES:
					status("Got connection from %s", ft_remoteaddr(&ft));
					gstate = STATE_CONNECTED;
					URGENT(1);
					cmds();
					/* fall */
				case FT_NO:
					return TRUE; /* make sure timeout keeps getting called */
			}
		/* unreachable */
	}
	/* unreachable (unless bogus enum value) */
	return TRUE;
}

G_MODULE_EXPORT
int on_winMain_focus_in_event(void)
{
	gtk_window_set_urgency_hint(GTK_WINDOW(winMain), FALSE);
	return FALSE;
}

void cmds()
{
	switch(gstate){
		case STATE_DISCO:
			gtk_widget_set_sensitive(cboHost,        TRUE);
			gtk_widget_set_sensitive(btnConnect,     TRUE);
			gtk_widget_set_sensitive(btnListen,      TRUE);
			gtk_widget_set_sensitive(btnClose,       FALSE);
			break;

		case STATE_CONNECTED:
			gtk_widget_set_sensitive(cboHost,        FALSE);
			gtk_widget_set_sensitive(btnConnect,     FALSE);
			gtk_widget_set_sensitive(btnListen,      FALSE);
			gtk_widget_set_sensitive(btnClose,       TRUE);
			break;

		case STATE_LISTEN:
			gtk_widget_set_sensitive(cboHost,        FALSE);
			gtk_widget_set_sensitive(btnConnect,     FALSE);
			gtk_widget_set_sensitive(btnListen,      FALSE);
			gtk_widget_set_sensitive(btnClose,       TRUE);
			break;

		case STATE_TRANSFER:
			gtk_widget_set_sensitive(cboHost,        FALSE);
			gtk_widget_set_sensitive(btnConnect,     FALSE);
			gtk_widget_set_sensitive(btnListen,      FALSE);
			gtk_widget_set_sensitive(btnClose,       TRUE);
			break;
	}

	if(gtk_events_pending())
		gtk_main_iteration();
}

const char *get_openfolder()
{
	static char folder[512];
	char *dname;

	dname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(btnDirChoice));

	if(dname){
		unsigned int len;

		strncpy(folder, dname, sizeof(folder));

		len = strlen(folder);
		if(len >= sizeof(folder)-3)
			fputs("warning: folder name too long\n", stderr);
		else if(folder[len - 1] != G_DIR_SEPARATOR){
			folder[len] = G_DIR_SEPARATOR;
			folder[len+1] = '\0';
		}

		g_free(dname);
		return folder;
	}
	return NULL;
}

void shelldir(const char *d)
{
#ifdef _WIN32
	HINSTANCE ret;

	if(!d)
		d = ".";

	if(d &&
			(int)(ret =
			 ShellExecute(NULL, "open", d, NULL, NULL, SW_SHOW /* 5 */))
			<= 32)
		fprintf(stderr, "ShellExecute(\"%s\") failed: %d\n", d, ret);
#else
	fprintf(stderr, "TODO: btnOpenFolder(\"%s\")\n", d); /* TODO */
#endif
}

void settimeout(int on)
{
	static int id = -1;

	if(on)
		id = g_timeout_add(TIMEOUT, timeout, NULL);
	else if(id != -1){
		g_source_remove(id);
		id = -1;
	}
}

char *fnameback(struct filetransfer *ft, char *fname)
{
	const char *folder = get_openfolder();

	(void)ft;

	if(!folder)
		return fname;

	return g_strdup_printf("%s%s%s", folder,
			folder[strlen(folder)-1] == G_DIR_SEPARATOR ? "" : "/",
			fname);
}

int callback(struct filetransfer *ft, enum ftstate ftst,
		size_t bytessent, size_t bytestotal)
{
	const double fraction = (double)bytessent / (double)bytestotal;

	switch(ftst){
		case FT_WAIT:
			break; /* skip straight to return */

		case FT_SENT:
		case FT_RECIEVED:
		{
			char *stat;

			if(ftst == FT_SENT)
				stat = g_strdup_printf("Sent %s", ft_basename(ft));
			else
				stat = g_strdup_printf("Recieved %s", ft_basename(ft));
			status(stat);
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressft), 1.0f);
			transfers_add(listDone, ft_basename(ft), ft_fname(ft), ftst == FT_RECIEVED);

			tray_balloon("Transfer complete", stat);
			g_free(stat);
			break;
		}

		case FT_BEGIN_SEND:
		case FT_BEGIN_RECV:
			cancelled = 0;
			gstate = STATE_TRANSFER;
			cmds();
			/* fall */

		case FT_RECIEVING:
		case FT_SENDING:
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressft), fraction);
			status("%s: %dK/%dK (%2.2f%%)", ft_basename(ft),
					bytessent / 1024, bytestotal / 1024, 100.0f * fraction);
	}

	while(gtk_events_pending())
		gtk_main_iteration();

	return cancelled;
}

int queryback(struct filetransfer *ft, enum ftquery qtype, const char *msg, ...)
{
	GtkWidget *dialog = gtk_dialog_new(), *label;
	va_list l;
	const char *iter, *percent;
	char *caption;
	int i = 0, formatargs = 0;

	(void)ft;
	(void)qtype;

	if(!dialog)
		return 0;

	va_start(l, msg);
	caption = g_strdup_vprintf(msg, l);
	va_end(l); /* needs reinitialising */

	label = gtk_label_new(caption);
	/*content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_add(GTK_CONTAINER(content_area), label);*/
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);

	/* button time */
	percent = msg-1;
	while(1)
		if((percent = strchr(percent+1, '%'))){
			if(percent[1] != '%')
				formatargs++;
		}else
			break;

	/* walk forwards formatargs times, then we're at the button names */
	va_start(l, msg);
	while(formatargs --> 0)
		va_arg(l, const char *);

	while((iter = va_arg(l, const char *)))
		gtk_dialog_add_button(GTK_DIALOG(dialog), iter, i++);
	va_end(l);

	gtk_widget_show(label);
	gtk_window_set_urgency_hint(GTK_WINDOW(dialog), TRUE);
	i = gtk_dialog_run(GTK_DIALOG(dialog));
	g_free(caption);
	gtk_widget_destroy(dialog);

	return i;
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

static int getobjects(GtkBuilder *b)
{
	GtkWidget *hboxHost;
	GtkWidget *scrollDone, *scrollQueue;

#define GET_WIDGET(x) \
	if(!((x) = GTK_WIDGET(gtk_builder_get_object(b, #x)))){ \
		fputs("Error: Couldn't get Gtk Widget \"" #x "\", bailing\n", stderr); \
		return 1; \
	}

	GET_WIDGET(btnSend);
	GET_WIDGET(btnConnect);
	GET_WIDGET(btnListen);
	GET_WIDGET(btnClose);
	GET_WIDGET(btnFileChoice);
	GET_WIDGET(winMain);
	GET_WIDGET(progressft);
	GET_WIDGET(lblStatus);
	GET_WIDGET(btnOpenFolder);
	GET_WIDGET(btnClearTransfers);
	GET_WIDGET(btnDirChoice);

	GET_WIDGET(treeDone);
	GET_WIDGET(treeTransfers);

	GET_WIDGET(frmSend);

	GET_WIDGET(hboxHost); /* FIXME: memleak? */
	/* create one with text as the column stuff */
	cboHost = gtk_combo_box_entry_new_text();
	gtk_container_add(GTK_CONTAINER(hboxHost), cboHost);
	gtk_widget_set_visible(cboHost, TRUE);
	/*gtk_box_reorder_child(GTK_BOX(hboxHost), cboHost, 0);*/

#if 0
	GET_WIDGET(scrollQueue);
	GET_WIDGET(scrollDone);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollDone),  GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollQueue), GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
#endif

	return 0;
#undef GET_WIDGET
}

int gladegen_init(void)
{
	FILE *f = fopen(GLADE_XML_FILE, "w");
	unsigned int i;

	if(!f)
		return 1;

	for(i = 0; i < sizeof(glade_str_gft)/sizeof(char *); i++)
		if(fputs(glade_str_gft[i], f) == EOF){
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

void html_expand(char *s)
{
	char *p = s;

	do{
		char *fin;
		unsigned int n;

		p = strchr(p, '%');
		if(!p)
			break;

		if(sscanf(p+1, "%2x", &n) == 1){
			*p = n;

			memmove(p + 1, p + 3, strlen(p + 2));
		}

	}while(1);
}

void drag_init(void)
{
	static GtkTargetEntry target_list[] = {
		/* datatype, GtkTargetFlags, datatype (custom) */
		{ "text/uri-list", 0, 0 }
	};

	gtk_drag_dest_set(
			frmSend,
			GTK_DEST_DEFAULT_ALL,
			target_list, sizeof(target_list)/sizeof(target_list[0]),
			GDK_ACTION_COPY);

	g_signal_connect(G_OBJECT(frmSend),
			"drag-data-received", G_CALLBACK(on_frmSend_drag_data_received), NULL);
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

		if(!debug){
			fputs("gft: debug off\n", stderr);
			FreeConsole();
		}
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

	if(!gtk_builder_add_from_file(builder, GLADE_XML_FILE, &error)){
		g_warning("%s", error->message);
		/*g_free(error);*/
		return 1;
	}
	gladegen_term();

	if(getobjects(builder))
		return 1;

	gtk_builder_connect_signals(builder, NULL);

	glist_init(&listTransfers, treeTransfers);

	/* don't need it anymore */
	g_object_unref(G_OBJECT(builder));

	/* signal setup */
	g_signal_connect(G_OBJECT(winMain), "delete-event" ,       G_CALLBACK(on_winMain_delete_event),    NULL);
	g_signal_connect(G_OBJECT(winMain), "destroy",             G_CALLBACK(on_winMain_destroy),         NULL);

	cfg_read(cboHost);
	tray_init(winMain, *argv);
	transfers_init(&listDone, treeDone);

	{
#ifdef _WIN32
		char path[MAX_PATH];
		if(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, path) == S_OK)
#else
		char *path = getenv("HOME");
		if(path)
#endif
			gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(btnFileChoice), path);
	}

	gtk_widget_set_sensitive(btnSend, FALSE);
	drag_init();

	cmds();
	gtk_widget_show(winMain);
	gtk_main();

	tray_term();

	return 0;
}
