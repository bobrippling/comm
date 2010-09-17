#ifndef CFG_H
#define CFG_H

#define MAX_PORT_LEN 16
#define MAX_PASS_LEN 32
#define MAX_DESC_LEN 64

int  cfg_read(FILE *f);
void cfg_end(void);

#endif
