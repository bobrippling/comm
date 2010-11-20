#include <gtk/gtk.h>
#include <string.h>

#include "glist.h"

/*
 * list business
 * http://scentric.net/tutorial/sec-treemodel-rowref.html
 */

void glist_init(GtkListStore **ls, GtkWidget *treeview)
{
	GtkTreeModel     *model;
	GtkCellRenderer  *renderer;

	*ls = gtk_list_store_new(1, G_TYPE_STRING);
	model = GTK_TREE_MODEL(*ls);

	/* Name column */
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
			-1,
			"Name", renderer,
			"text", 0 /* column index */,
			NULL);

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), model);

	g_object_unref(model);
	/* don't do this: g_object_unref(renderer);*/
}

void glist_add(GtkListStore *ls, const char *name)
{
	GtkTreeIter iter;

	gtk_list_store_append(ls, &iter);
	gtk_list_store_set(ls, &iter,
			0, name, -1);
}

void glist_clear(GtkListStore *ls)
{
	gtk_list_store_clear(ls);
}

void glist_remove(GtkListStore *ls, GtkTreeModel *tree, const char *item)
{
	GtkTreeIter iter;

	if(gtk_tree_model_get_iter_first(tree, &iter)){
		do{
			char *tmp;

			gtk_tree_model_get(tree, &iter, 0, &tmp, -1);

			if(!strcmp(tmp, item)){
				gtk_list_store_remove(ls, &iter);
				break;
			}
		}while(gtk_tree_model_iter_next(tree, &iter));
	}else
		g_warning("glist_remove: couldn't get iter_first");
}

char *glist_selected(GtkTreeView *tv)
{
	GtkTreeIter       iter;
	GtkTreeModel     *model = gtk_tree_view_get_model(tv);
	GtkTreeSelection *sel   = gtk_tree_view_get_selection(tv);
	char             *data  = NULL;

	if(gtk_tree_selection_get_selected(sel, NULL, &iter))
		gtk_tree_model_get(model, &iter, 0, &data, -1);

	return data;
}
