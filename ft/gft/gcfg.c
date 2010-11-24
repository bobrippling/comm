#define _POSIX_C_SOURCE 200809L
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "gcfg.h"
#include "../../config.h"

#ifndef strdup
char *strdup(const char *);
#endif

#define CFG_GFT_FNAME  "gft.conf"

#define ITER_HOSTS(i, code) \
	for(i = 0; i < nhosts; i++) \
		code

static const char **hosts = NULL;
static int nhosts = 0;


void cfg_add(const char *host)
{
	const char **new, *start;
	char *dup;
	int i;

	start = host;
	while(isspace(*start))
		start++;

	if(!*start)
		return;

	ITER_HOSTS(i,
		if(!strcmp(hosts[i], host))
			return;
			);

	new = realloc(hosts, sizeof(*new) * ++nhosts);
	if(!new)
		return;

	hosts = new;
	hosts[nhosts-1] = dup = strdup(host);

	while(*dup && !isspace(*dup))
		dup++;
	if(isspace(*dup))
		*dup = '\0';
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

	/* mkdir (assuming ~/.config exists) */
	if(!f){
		char *slash = strrchr(tmp, '/');
		if(slash){
			*slash = '\0';
			if(mkdir(tmp, 0755) && errno != EEXIST){
				perror("mkdir(CFG)");
				g_free(tmp);
				return;
			}
			*slash = '/';
			f = fopen(tmp, "w");
		}
	}

	g_free(tmp);
#endif

	if(!f){
		perror("cfg_write()");
		return;
	}

	{
		extern GtkWidget *btnDirChoice;
		char *dname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(btnDirChoice));

		if(dname){
			fprintf(f, "DIR %s\n", dname);
			g_free(dname);
		}
	}

	ITER_HOSTS(i,
		fprintf(f, "HOST %s\n", hosts[i]);
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

		if(!strncmp(line, "HOST ", 5)){
			char *const h = line + 5;
			cfg_add(h);
			gtk_combo_box_append_text(GTK_COMBO_BOX(cboHost), h);
			gtk_entry_set_text(GTK_ENTRY(GTK_BIN(cboHost)->child), h);
		}else if(!strncmp(line, "DIR ", 4)){
			extern GtkWidget *btnDirChoice;
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(btnDirChoice), line + 4);
		}else
			fprintf(stderr, "Invalid config line: %s\n", line);
	}
	fclose(f);
}
