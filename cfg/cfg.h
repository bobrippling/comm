#ifndef CFG_H
#define CFG_H

int config_read(void);
int config_write(void);

const char *config_name(void);
const char *config_port(void);
const char *config_lasthost(void);
const char *config_colour(void);

void config_setname(const char *);
void config_setport(const char *);
void config_setlasthost(const char *);
void config_setcolour(const char *);

#endif
