#ifndef DRAWAN_H
#define DRAWAN_H

void draw_init(void), draw_term(void);
void draw_brush(GtkWidget *widget,
		gdouble x, gdouble y, gdouble x2, gdouble y2);

#endif
