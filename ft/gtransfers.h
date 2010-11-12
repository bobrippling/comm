#ifndef GTRANSFERS_H
#define GTRANSFERS_H

void transfers_init(void);
void transfers_add(const char *title, const char *data);
const char *transfers_get(int row);
void transfers_clear(void);

#endif
