#ifndef GCFG_H
#define GCFG_H

int  cfg_add(const char *); /* returns 1 on _new_ entry */
void cfg_read(GtkWidget *cboHost);
void cfg_write(void);


#define GET_AND_SET(n) \
	int  cfg_get_##n(); \
	void cfg_set_##n(int i);

#undef GET_AND_SET

#endif
