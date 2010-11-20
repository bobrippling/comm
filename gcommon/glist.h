#ifndef GLIST_H
#define GLIST_H

void glist_init( GtkListStore **, GtkWidget *treeview);
void glist_add(  GtkListStore *, const char *);
void glist_clear(GtkListStore *);
void glist_remove(GtkListStore *ls, GtkTreeModel *tree, const char *item);

char *glist_selected(GtkTreeView *tv);

#endif
