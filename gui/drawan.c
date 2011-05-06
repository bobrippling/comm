#include <gtk/gtk.h>
#include <math.h>

static GdkPixmap *pixmap = NULL; /* backbuffer */
static int last_x = -1, last_y = -1;

#define DRAW_SIZE 2

void
draw_brush(GtkWidget *widget, gdouble x, gdouble y, gdouble x2, gdouble y2)
{
#if 0
	gdk_draw_line()
	gdk_draw_rectangle()
	gdk_draw_arc()
	gdk_draw_polygon()
	gdk_draw_string()
	gdk_draw_text()
	gdk_draw_pixmap()
	gdk_draw_bitmap()
	gdk_draw_image()
	gdk_draw_points()
	gdk_draw_segments()

	http://www.gtk.org/tutorial1.2/gtk_tut-23.html
#endif

#ifdef RECT_METHOD
	GdkRectangle update_rect;

	update_rect.x = x - 5;
	update_rect.y = y - 5;
	update_rect.width = 10;
	update_rect.height = 10;
	gdk_draw_rectangle(pixmap,
		widget->style->black_gc,
		TRUE,
		update_rect.x, update_rect.y,
		update_rect.width, update_rect.height);

	gtk_widget_draw(widget, &update_rect);
#else
	if(last_x == -1)
		return;

	gdk_draw_line(
			pixmap,
			widget->style->black_gc,
			x, y, x2, y2);

	gtk_widget_queue_draw_area(widget,
			MIN(x,   x2),
			MIN(y,   y2),
			fabs(x - x2) + DRAW_SIZE,
			fabs(y - y2) + DRAW_SIZE
			);
#endif
}

void
draw_brush_send(GtkWidget *widget, gdouble x, gdouble y, gdouble x2, gdouble y2)
{
	int got_draw(int, int, int, int, int);

	if(!got_draw(x, y, x2, y2, 0))
		draw_brush(widget, x, y, x2, y2);
}

gboolean
on_drawan_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	(void)event;
	(void)data;

	if(pixmap){
		gdk_pixmap_unref(pixmap);
		fprintf(stderr, "TODO: resize pixmap\n");
	}

	pixmap = gdk_pixmap_new(widget->window,
		widget->allocation.width,
		widget->allocation.height,
		-1);

	gdk_draw_rectangle(pixmap,
		widget->style->white_gc,
		TRUE,
		0, 0,
		widget->allocation.width,
		widget->allocation.height);

  return TRUE;
}

gint
on_drawan_expose(GtkWidget *widget, GdkEventExpose *event)
{
  gdk_draw_pixmap(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
			pixmap,
			event->area.x, event->area.y,
			event->area.x, event->area.y,
			event->area.width, event->area.height);

  return FALSE;
}

gint
on_drawan_button_press(GtkWidget *widget, GdkEventButton *event)
{
	if(event->button == 1 && pixmap != NULL)
		draw_brush_send(widget, event->x, event->y, last_x, last_y);

	return TRUE;
}

gint
on_drawan_button_release()
{
	last_x = last_y = -1;
	return FALSE;
}

gint
on_drawan_motion(GtkWidget *widget, GdkEventMotion *event)
{
	int x, y;
	GdkModifierType state;

	if(event->is_hint){
		gdk_window_get_pointer(event->window, &x, &y, &state);
	}else{
		x = event->x;
		y = event->y;
		state = event->state;
	}

	if(state & GDK_BUTTON1_MASK && pixmap != NULL)
		draw_brush_send(widget, x, y, last_x, last_y);

	last_x = x;
	last_y = y;

	return TRUE;
}

void draw_init()
{
	extern GtkWidget *drawanArea;

	gtk_widget_set_events(
			drawanArea,
			gtk_widget_get_events(drawanArea)
			| GDK_EXPOSURE_MASK
			| GDK_LEAVE_NOTIFY_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_POINTER_MOTION_MASK
			| GDK_POINTER_MOTION_HINT_MASK);
	gtk_signal_connect(
			GTK_OBJECT(drawanArea),
			"expose_event",
			(GtkSignalFunc) on_drawan_expose,
			NULL);
}

void draw_term()
{
}
