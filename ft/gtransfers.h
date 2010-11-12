#ifndef GTRANSFERS_H
#define GTRANSFERS_H

struct transfer
{
	char *fname, *path; // heap
	int is_recv;
	struct transfer *next;
};

void transfers_init(void);
void transfers_add(const char *title, const char *data, int recv);
struct transfer *transfers_get(int row);
void transfers_clear(void);

#endif
