#ifndef PRIV_H
#define PRIV_H

struct privchat
{
	GtkWidget *winPriv, *txtMain, *entryTxt, *btnSend;
	char *namedup;
};

struct privchat *priv_new(GtkWidget *winMain, const char *to);

#endif
