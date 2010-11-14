#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#define SIZEOF_CAST "%ld"

#include "gtransfers.h"


GtkListStore *treeStore = NULL;
struct transfer *transfers = NULL;

void transfers_init(void)
{
	extern GtkWidget *treeTransfers;
	GtkTreeModel    *model;
	GtkCellRenderer *renderer;

	treeStore = gtk_list_store_new(1, G_TYPE_STRING);
	model = GTK_TREE_MODEL(treeStore);

	/* Name column */
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeTransfers),
			-1,
			"File", renderer,
			"text", 0 /* column index */,
			NULL);

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeTransfers), model);

	g_object_unref(model);
	/* don't do this: g_object_unref(renderer);*/
}

void transfers_add(const char *title,
		const char *data, int is_recv)
{
	GtkTreeIter iter;
	struct transfer *t, *last;
	char *tmp;

	gtk_list_store_append(treeStore, &iter);

	tmp = alloca(strlen(title) + 8); /* should be 7 but oh well */
	strcpy(tmp, is_recv ? "Recv: " : "Sent: ");
	strcat(tmp, title);

	gtk_list_store_set(treeStore, &iter,
			0, tmp, -1);

	t = malloc(sizeof(*t));
	if(!t)
		g_error("couldn't allocate " SIZEOF_CAST " bytes", (long unsigned int)sizeof(*t));

	t->fname = g_strdup(title);
	t->path  = g_strdup(data);
	t->is_recv = is_recv;
	t->next = NULL;

	/* have to add to the end for transfers_get() */
	if(!transfers)
		transfers = t;
	else{
		for(last = transfers; last->next; last = last->next);
		last->next = t;
	}
}

struct transfer *transfers_get(int n)
{
	struct transfer *t;

	for(t = transfers; n-- && t; t = t->next);

	return t;
}

void transfers_clear(void)
{
	gtk_list_store_clear(treeStore);
}

