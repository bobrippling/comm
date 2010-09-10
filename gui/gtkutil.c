#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "gtkutil.h"

/* funcs */
extern GtkWidget *txtMain;

void addtext(const char *text)
{
	GtkTextBuffer *buffa;
	GtkTextIter iter;

	buffa = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtMain));
	if(!buffa)
		return;

	memset(&iter, '\0', sizeof iter);
	gtk_text_buffer_get_iter_at_offset(buffa, &iter, -1 /* end */);
	gtk_text_buffer_insert(            buffa, &iter, text, -1 /* nul-term */ );
}

void addtextl(const char *fmt, va_list l)
{
	char *insertme = g_strdup_vprintf(fmt, l);
	addtext(insertme);
	free(insertme);
}

void addtextf(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	addtextl(fmt, l);
	va_end(l);
}