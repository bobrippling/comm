#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "gtkutil.h"

/* funcs */
extern GtkWidget    *txtMain;
extern GtkWidget    *treeClients;
static GtkListStore *treeStore;

void addtext(const char *text)
{
	GtkTextBuffer *buffa;
	GtkTextView   *view;
	GtkTextIter    iter;

	buffa = gtk_text_view_get_buffer(view = GTK_TEXT_VIEW(txtMain));
	if(!buffa)
		return;

	memset(&iter, '\0', sizeof iter);
	gtk_text_buffer_get_iter_at_offset(buffa, &iter, -1 /* end */);
	gtk_text_buffer_insert(            buffa, &iter, text, -1 /* nul-term */ );

	/* ..., ..., margin, use_align, xalign, yalign */
	gtk_text_view_scroll_to_iter(view, &iter, 0.0, FALSE, 0, 0);
}

void addtextl(const char *fmt, va_list l)
{
	char *insertme = g_strdup_vprintf(fmt, l);
	addtext(insertme);
	free(insertme);
}

void addtextf(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	addtextl(fmt, l);
	va_end(l);
}

/*
 * list business
 * http://scentric.net/tutorial/sec-treemodel-rowref.html
 */

void clientlist_init()
{
	GtkTreeModel    *model;
	GtkCellRenderer *renderer;

	treeStore = gtk_list_store_new(1, G_TYPE_STRING);
	model = GTK_TREE_MODEL(treeStore);

	/* Name column */
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeClients),
			-1,
			"Name", renderer,
			"text", 0 /* column index */,
			NULL);

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeClients), model);

	g_object_unref(model);
	/* don't do this: g_object_unref(renderer);*/
}

void clientlist_add(const char *name)
{
	GtkTreeIter iter;

	gtk_list_store_append(treeStore, &iter);

	gtk_list_store_set(treeStore, &iter,
			0, name, -1);
}

void clientlist_clear()
{
	gtk_list_store_clear(treeStore);
}
