#ifndef DRAWAN_H
#define DRAWAN_H

void draw_init(void), draw_term(void);
void draw_brush(GtkWidget *widget, int x, int y, int x2, int y2);

void draw_clear(void);
void draw_set_colour(GdkColor *);

#endif
