#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>

#include "gqueue.h"

static struct queue *queue_new(const char *item);

/*
 * warning: uses g_error to die if alloc fails
 */

static struct queue *queue_new(const char *item)
{
	struct queue *q = malloc(sizeof *q);
	if(!q)
		g_error("couldn't allocate %ld bytes",
				(unsigned long)sizeof *q);

	q->item = g_strdup(item);
	q->next = NULL;

	return q;
}

char *queue_next(struct queue **qp)
{
	char *item;
	struct queue *delme;

	if(!*qp)
		return NULL;

	delme = *qp;
	item = (*qp)->item;

	*qp = (*qp)->next;

	free(delme);

	return item;
}

void queue_add(struct queue **qp, const char *item)
{
	if(!*qp){
		*qp = queue_new(item);
	}else{
		struct queue *q;

		q = *qp;
		while(q->next)
			q = q->next;

		q->next = queue_new(item);
	}
}

void queue_rem(struct queue **qp, const char *item)
{
	struct queue *q, *prev = NULL;

	for(q = *qp; q; prev = q, q = q->next){
		if(!strcmp(q->item, item)){

			free(q->item);
			if(prev){
				prev->next = q->next;
				free(q);
			}else{
				/* *qp == q */
				prev = q;
				*qp = q->next;
				free(prev);
			}
			return;
		}
	}

	g_warning("queue_rem: couldn't find \"%s\"", item);
}

int queue_len(struct queue *q)
{
	int i = 0;
	for(; q; q = q->next, i++);
	return i;
}

int queue_has(struct queue *q, const char *s)
{
	for(; q; q = q->next)
		if(!strcmp(q->item, s))
			return 1;
	return 0;
}
