#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <alloca.h>

#include "gtkutil.h"

#define PROPERTY_URL "url"

#define WARN(s) fprintf(stderr, __FILE__ ":%d:" s "\n", __LINE__)

static void addtag(const char *name, const char *col);
static int havetag(GtkTextBuffer *buffa, const char *name);
static int isurlchar(char c);
static void insertlink(GtkTextBuffer *buffa, GtkTextIter *iter, const char *text, int textlen);
static void insert_with_links(GtkTextBuffer *buffa, GtkTextIter *iter, const char *col, const char *text);

/* funcs */
extern GtkWidget    *txtMain;
extern GtkWidget    *treeClients;
static GtkListStore *treeStore;

static void addtag(const char *name, const char *col)
{
	GtkTextBuffer *buffa = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtMain));

	if(!buffa){
		WARN("!buffa");
		return;
	}

	gtk_text_buffer_create_tag(buffa, name,
		"foreground", col, NULL);
}

static int havetag(GtkTextBuffer *buffa, const char *name)
{
	GtkTextTagTable *table;

	table = gtk_text_buffer_get_tag_table(buffa);
	if(!table){
		WARN("!table");
		return 1;
	}

	return !!gtk_text_tag_table_lookup(table, name);
}

static void insertlink(GtkTextBuffer *buffa, GtkTextIter *iter, const char *text, int textlen)
{
	/*if(!havetag(buffa, LINK_TAG_NAME))*/
	GtkTextTag *urltag;
	char *dup = g_strndup(text, textlen), *nl;

	if(!dup){
		WARN("!dup");
		return;
	}

	urltag = gtk_text_buffer_create_tag(buffa, NULL, /* anon */
	                        "foreground", "blue",
	                        "underline", PANGO_UNDERLINE_SINGLE,
	                        NULL);

	if((nl = strchr(dup, '\n')))
		*nl = '\0';

	/* tag the address on */
	g_object_set_data(G_OBJECT(urltag), PROPERTY_URL, dup);

	gtk_text_buffer_insert_with_tags(buffa, iter, text, textlen, urltag, NULL);
}

static int isurlchar(char c)
{
	/*
	 * search for [^a-z0-9._+#=?&:;%/!,-]
	 */
	if(('a' <= c && c <= 'z') ||
		 ('A' <= c && c <= 'Z') ||
		 ('0' <= c && c <= '9') ||
		 ('+' <= c && c <= ';'))
		return 1;

	/* [+,-./] handled above */

	switch(c){
		case '!': case '#': case '%': case '&':
		case ':': case ';': case '=':
		case '?': case '_':
			return 1;
	}

	return 0;
}

static void insert_with_links(GtkTextBuffer *buffa, GtkTextIter *iter,
		const char *col, const char *text)
{
	const char *link;
	int linkiswww;

	while(
			(linkiswww = 0, link = strstr(text, "://")) ||
			(linkiswww = 1, link = strstr(text, "www"))
			){

		if(link > text && (linkiswww || isalpha(link[-1]))){
			const char *linkend = link + 1;
			int len;

			if(!linkiswww){
				while(link > text && isalpha(*--link));
				link++;
			}

			/* add previous text, since link > text */
			gtk_text_buffer_insert_with_tags_by_name(buffa,
					iter, text, len = link - text, col, NULL);
			text += len;


			while(isurlchar(*linkend))
				linkend++;

			insertlink(buffa, iter, link, len = linkend - link);
			text += len;
		}else
			link++;
	}

	if(*text)
		gtk_text_buffer_insert_with_tags_by_name(buffa,
				iter, text, -1, col, NULL);
}

const char *iterlink(GtkTextIter *iter)
{
	GtkTextBuffer *buffa = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtMain));
	GSList *list = NULL, *lp = NULL;
	const char *ret = NULL;

	if(!buffa){
		WARN("!buffa");
		return NULL;
	}

	list = gtk_text_iter_get_tags(iter); /* needs freeing */
	for(lp = list; lp; lp = lp->next){
		GtkTextTag *tag = lp->data;
		const char *url = g_object_get_data(G_OBJECT(tag), PROPERTY_URL);
		if(!url)
			continue;
		ret = url;
	}

	if(list)
		g_slist_free(list);

	return ret;
}

void addtext(const char *col, const char *text)
{
	GtkTextIter    iter;
	GtkTextBuffer *buffa;
	GtkTextView   *view;
	GtkTextMark   *insert_mark;

	buffa = gtk_text_view_get_buffer(view = GTK_TEXT_VIEW(txtMain));
	if(!buffa){
		WARN("!buffa");
		return;
	}

	if(!havetag(buffa, col))
		addtag(col, col);

	memset(&iter, '\0', sizeof iter);
	gtk_text_buffer_get_end_iter(buffa, &iter);
	/*gtk_text_buffer_insert(buffa, &iter, text, -1 * nul-term * ); */

	insert_with_links(buffa, &iter, col, text);

	/* colour */
	/* iter is still the start of the insertion
	gtk_text_buffer_get_end_iter(buffa, &enditer);
	gtk_text_buffer_apply_tag(buffa, tag, &iter, &enditer); */


	/* autoscroll */
	gtk_text_buffer_get_end_iter(buffa, &iter);

	/* get the current (cursor) mark name */
	insert_mark = gtk_text_buffer_get_insert(buffa);

	/* move mark and selection bound to the end */
	gtk_text_buffer_place_cursor(buffa, &iter);

	gtk_text_view_scroll_to_mark(view, insert_mark,
			0.0, TRUE, 0.0, 1.0);
	/* ..., ..., margin, use_align, xalign, yalign */
}

void addtextl(const char *col, const char *fmt, va_list l)
{
	char *insertme = g_strdup_vprintf(fmt, l);
	addtext(col, insertme);
	free(insertme);
}

void addtextf(const char *col, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	addtextl(col, fmt, l);
	va_end(l);
}

/*
 * list business
 * http://scentric.net/tutorial/sec-treemodel-rowref.html
 */

void clientlist_init()
{
	GtkTreeModel    *model;
	GtkCellRenderer *renderer;

	treeStore = gtk_list_store_new(1, G_TYPE_STRING);
	model = GTK_TREE_MODEL(treeStore);

	/* Name column */
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeClients),
			-1,
			"Name", renderer,
			"text", 0 /* column index */,
			NULL);

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeClients), model);

	g_object_unref(model);
	/* don't do this: g_object_unref(renderer);*/
}

void clientlist_add(const char *name)
{
	GtkTreeIter iter;

	gtk_list_store_append(treeStore, &iter);

	gtk_list_store_set(treeStore, &iter,
			0, name, -1);
}

void clientlist_clear()
{
	gtk_list_store_clear(treeStore);
}

/* GtkTextTagTableForeach */
static void gtkutil_cleanup_tagtable(GtkTextTag *tag, gpointer data)
{
	char *url = g_object_get_data(G_OBJECT(tag), PROPERTY_URL);

	UNUSED(data);

	if(url)
		g_free(url);
		/*g_object_set_data(G_OBJECT(tag), PROPERTY_URL, NULL);*/
}

void gtkutil_cleanup()
{
	GtkTextBuffer *buffa;
	GtkTextTagTable *table;

	buffa = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtMain));
	if(!buffa){
		WARN("!buffa");
		return;
	}

	table = gtk_text_buffer_get_tag_table(buffa);
	if(table)
		gtk_text_tag_table_foreach(table, gtkutil_cleanup_tagtable, table);
	else
		WARN("!table");
}
