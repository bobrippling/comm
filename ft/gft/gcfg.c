#define _POSIX_C_SOURCE 200809L
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#include "gcfg.h"
#include "gft.h"
#include "../../config.h"

#ifndef strdup
char *strdup(const char *);
#endif

#define CFG_GFT_FNAME  "gft.cfg"

struct list
{
	char *str;
	struct list *next;
} *hosts = NULL;

int cfg_add(const char *host)
{
	struct list *iter, *prev = NULL;

	for(iter = hosts; iter; prev = iter, iter = iter->next)
		if(!strcmp(iter->str, host)){
			if(prev){
				prev->next = iter->next;
				iter->next = hosts;
				hosts = iter;
			}/* else already head of the list */

			return 0;
		}

	iter = malloc(sizeof *iter);
	if(!iter)
		return 0;

	iter->next = hosts;
	iter->str  = strdup(host);
	hosts = iter;

	return 1;
}

void cfg_add_rev(const char *host)
{
	struct list *new, *last;

	for(last = hosts; last && last->next; last = last->next);

	if(last)
		new = last->next = malloc(sizeof *new);
	else
		new = hosts      = malloc(sizeof *new);

	if(!new)
		return;

	new->str  = strdup(host);
	new->next = NULL;
}

void cfg_write()
{
	FILE *f;

#ifdef _WIN32
	f = fopen(CFG_GFT_FNAME, "w");
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

	{
		struct list *iter;
		for(iter = hosts; iter; iter = iter->next)
			fprintf(f, "HOST %s\n", iter->str);
	}


	{
		extern GtkWidget *chkTray;
		fprintf(f, "CLOSE_TRAY %d\n",
#ifdef CFG_USE_RADIO
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkTray))
#else
				gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkTray))
#endif
				);
	}

	fclose(f);
}

void cfg_read(GtkWidget *cboHost)
{
	FILE *f;
	char line[128];

#ifdef _WIN32
	f = fopen(CFG_GFT_FNAME, "r");
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

			if(!hosts)
				gtk_entry_set_text(GTK_ENTRY(GTK_BIN(cboHost)->child), h);

			cfg_add_rev(h);
			gtk_combo_box_append_text(GTK_COMBO_BOX(cboHost), h);

		}else if(!strncmp(line, "DIR ", 4)){
			extern GtkWidget *btnDirChoice;
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(btnDirChoice), line + 4);

		}else if(!strncmp(line, "CLOSE_TRAY ", 11)){
			extern GtkWidget *chkTray;
			int val = 1;

			if(sscanf(line + 11, "%d", &val) != 1)
				fprintf(stderr, "Invalid config number: %s\n", line);
			else
#ifdef CFG_USE_RADIO
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkTray), val);
#else
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(chkTray), val);
#endif
		}else
			fprintf(stderr, "Invalid config line: %s\n", line);
	}
	fclose(f);
}


#define GET_AND_SET(n) \
	int  cfg_get_##n(     ) { return bool_##n; } \
	void cfg_set_##n(int i) { bool_##n = i;    }
