#ifndef GCFG_H
#define GCFG_H

void cfg_add(const char *);
void cfg_read(GtkWidget *cboHost);
void cfg_write(void);


#define GET_AND_SET(n) \
	int cfg_get_##n(); \
	int cfg_set_##n(int i);

GET_AND_SET(close_to_tray)

#undef GET_AND_SET

#endif
