#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "drawan.h"

static cairo_surface_t *surface = NULL;

static int last_x = -1, last_y = -1;

GdkColor draw_colour;

#define DRAW_SIZE 2

#define TRACE(x) fputs(x "\n", stderr)

void
draw_brush(GtkWidget *widget, int x, int y, int x2, int y2)
{
	GdkRectangle update_rect;
	cairo_t *cr;

	fprintf(stderr, "draw_brush()\n");

	update_rect.x      = x - DRAW_SIZE;
	update_rect.y      = y - DRAW_SIZE;
	update_rect.width  = 2 * DRAW_SIZE;
	update_rect.height = 2 * DRAW_SIZE;

	/* Paint to the surface, where we store our state */
	cr = cairo_create(surface);

	gdk_cairo_rectangle(cr, &update_rect); // FIXME
	cairo_fill(cr);

	cairo_destroy(cr);

	/* Now invalidate the affected region of the drawing area. */
	gdk_window_invalidate_rect(gtk_widget_get_window(widget), &update_rect, FALSE);
}

void draw_set_colour(GdkColor *col)
{
	// TODO

}


static void
draw_brush_send(GtkWidget *widget, int x, int y)
{
	int gui_draw_net(int, int, int, int, int);

	if(last_x == -1) last_x = x;
	if(last_y == -1) last_y = y;

	if(!gui_draw_net(x, y, last_x, last_y, 0)){ /* check we're connected, etc etc */
		draw_brush(widget, x, y, last_x, last_y);
		last_x = x;
		last_y = y;
	}
}

G_MODULE_EXPORT gboolean
on_drawan_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	GtkAllocation allocation;
	cairo_t *cr;

	TRACE("configure");

	(void)event;
	(void)data;

	if(surface)
		cairo_surface_destroy(surface);

	gtk_widget_get_allocation(widget, &allocation);
	surface = gdk_window_create_similar_surface(
			gtk_widget_get_window(widget),
			CAIRO_CONTENT_COLOR,
			allocation.width, allocation.height);

	/* Initialize the surface to white */
	cr = cairo_create(surface);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	cairo_destroy(cr);

	/* We've handled the configure event, no need for further processing. */
	return TRUE;
}

G_MODULE_EXPORT gint
on_drawan_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	(void)widget;
	(void)data;

	TRACE("draw");

	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_paint(cr);

  return FALSE;
}

G_MODULE_EXPORT gint
on_drawan_button_press(GtkWidget *widget, GdkEventButton *event)
{
	TRACE("press");

	if(event->button == 1 && surface)
		draw_brush_send(widget, event->x, event->y);
		return TRUE;
	else
		return FALSE;
}

G_MODULE_EXPORT gint
on_drawan_button_release()
{
	TRACE("release");

	last_x = last_y = -1;
	return FALSE;
}

G_MODULE_EXPORT gint
on_drawan_motion(GtkWidget *widget, GdkEventMotion *event)
{
	int x, y;
	GdkModifierType state;

	TRACE("motion");

	if(!surface)
		return FALSE;

	gdk_window_get_pointer(event->window, &x, &y, &state);

	if(state & GDK_BUTTON1_MASK)
		draw_brush_send(widget, x, y);

	return TRUE;
}

void draw_clear()
{
	extern GtkWidget *drawanArea;

}

void draw_init()
{
	extern GtkWidget *drawanArea;

	memset(&draw_colour, 0, sizeof draw_colour);

#if 0
	drawanArea = gtk_drawing_area_new();
	gtk_widget_set_size_request(drawanArea, 120, 120);
	gtk_container_add(GTK_CONTAINER(vboxMain), drawanArea);
#endif

	TRACE("draw_init");

	/* surface signals */
	g_signal_connect(drawanArea, "draw",                  G_CALLBACK(on_drawan_draw),      NULL);
	g_signal_connect(drawanArea, "configure-event",       G_CALLBACK(on_drawan_configure), NULL);

	/* events */
	g_signal_connect(drawanArea, "motion-notify-event",   G_CALLBACK(on_drawan_motion),         NULL);
	g_signal_connect(drawanArea, "button-press-event",    G_CALLBACK(on_drawan_button_press),   NULL);
	g_signal_connect(drawanArea, "button-release-event",  G_CALLBACK(on_drawan_button_release), NULL);

	gtk_widget_set_events(
			drawanArea,
			gtk_widget_get_events(drawanArea)
			| GDK_LEAVE_NOTIFY_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_POINTER_MOTION_MASK
			| GDK_POINTER_MOTION_HINT_MASK
			);

	TRACE("draw_init done");
}

void draw_term()
{
	if(surface)
		cairo_surface_destroy(surface);
	surface = NULL;
}
