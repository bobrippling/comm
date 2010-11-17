#ifndef GTRANSFERS_H
#define GTRANSFERS_H

struct transfer
{
	char *fname, *path; // heap
	int is_recv;
	struct transfer *next;
};

void transfers_init(GtkListStore **liststore, GtkWidget *treeWidget);

void transfers_add(GtkListStore *liststore, const char *title, const char *data, int recv);

struct transfer *transfers_get(int row);
void transfers_clear(void);

#endif
