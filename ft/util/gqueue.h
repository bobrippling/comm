#ifndef QUEUE_H
#define QUEUE_H

struct queue
{
	char *item;
	struct queue *next;
};


char *queue_next(struct queue **);
void  queue_add( struct queue **, const char *);

void  queue_rem( struct queue **, const char *);

int   queue_len( struct queue *);

#endif
