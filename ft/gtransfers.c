#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>


#include "gtransfers.h"


GtkListStore *treeStore = NULL;

struct transfer
{
	char *fname, *data; // heap
	struct transfer *next;
} *transfers = NULL;


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

void transfers_add(const char *title, const char *data)
{
	GtkTreeIter iter;
	struct transfer *t;

	gtk_list_store_append(treeStore, &iter);

	gtk_list_store_set(treeStore, &iter,
			0, title, -1); /* FIXME: data */

	t = malloc(sizeof(*t));
	if(!t)
		g_error("couldn't allocate %ld bytes", sizeof(*t));

	t->fname = g_strdup(title);
	t->data  = g_strdup(data);
	t->next = transfers;
	transfers = t;
}

const char *transfers_get(int n)
{
	struct transfer *t;

	for(t = transfers; n-- && t; t = t->next);

	if(t)
		return t->data;
	return NULL;
}

void transfers_clear(void)
{
	gtk_list_store_clear(treeStore);
}

