#define _POSIX_C_SOURCE 200809
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "gcfg.h"
#include "../config.h"

#ifdef _WIN32
char *strdup(const char *);
#endif

#define CFG_GFT_FNAME  "recent_hosts"

#define ITER_HOSTS(i, code) \
	for(i = 0; i < nhosts; i++) \
		code

static const char **hosts = NULL;
static int nhosts = 0;


void cfg_add(const char *host)
{
	const char **new;
	int i;

	ITER_HOSTS(i,
		if(!strcmp(hosts[i], host))
			return;
			);

	new = realloc(hosts, sizeof(*new) * ++nhosts);
	if(!new)
		return;

	hosts = new;
	hosts[nhosts-1] = strdup(host);
}

void cfg_write()
{
	int i;
	FILE *f;

#ifdef _WIN32
	f = fopen("gft.cfg", "w");
#else
	char *home = getenv("HOME"), *tmp;

	if(!home)
		return;

	tmp = g_strdup_printf("%s/" CFG_EXTRA "%s", home, CFG_DOT CFG_GFT_FNAME);
	f = fopen(tmp, "w");
	g_free(tmp);
#endif

	if(!f){
		perror("cfg_add()");
		return;
	}

	ITER_HOSTS(i,
		fprintf(f, "%s\n", hosts[i]);
		);

	fclose(f);
}

void cfg_read(GtkWidget *cboHost)
{
	FILE *f;
	char line[128];

#ifdef _WIN32
	f = fopen("gft.cfg", "r");
#else
	char *home = getenv("HOME"), *tmp;

	if(!home){
		fputs("couldn't get $HOME\n", stderr);
		return;
	}

	tmp = g_strdup_printf("%s/" CFG_EXTRA "%s", home, CFG_DOT CFG_GFT_FNAME);
	f = fopen(tmp, "r");
	g_free(tmp);
#endif

	if(!f){
		if(errno != ENOENT)
			perror("cfg_read()");
		return;
	}

	while(fgets(line, sizeof line, f)){
		char *nl = strchr(line, '\n');
		if(nl)
			*nl = '\0';
		cfg_add(line);
		gtk_combo_box_append_text(GTK_COMBO_BOX(cboHost), line);
	}
	fclose(f);
	gtk_entry_set_text(GTK_ENTRY(GTK_BIN(cboHost)->child), line);
}
