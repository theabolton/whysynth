/* gtkknob.c - libwhy GtkKnob knob widget
 *
 * Copyright (C) 2008, 2010 Sean Bolton
 *
 * Parts of this code come from GTK+, both the library source and
 * the example programs.  Other bits come from gAlan 0.2.0,
 * copyright (C) 1999 Tony Garnock-Jones.
 *
 * gtkknob-cairo_or_pixmap_selectable.c
 *
 * - This version requires GTK+ 2.0 or later.
 * - This version can use fast but ugly server-side pixmaps to draw
 *   the knobs.
 * - On GTK+ 2.8 or later, this can also use cairo to draw nice but
 *   relatively slower anti-aliased knobs.
 * - On GTK+ 2.8 or later, cairo is the default, but this can be
 *   changed using the function gtk_knob_set_fast_rendering().
 * -FIX- It would be nice to add a property "fast-rendering" that
 * could be easily set....
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#define _ISOC99_SOURCE
#define _BSD_SOURCE

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>

#include "gtkknob.h"

#define KNOB_SIZE               32
#define HALF_KNOB_SIZE          (KNOB_SIZE / 2)

#define UPDATE_DELAY_LENGTH    300


enum {
    STATE_IDLE,
    STATE_PRESSED,
    STATE_DRAGGING
};

static GtkWidgetClass *parent_class = NULL;

#if GTK_CHECK_VERSION(2, 8, 0)
static int prefer_cairo = TRUE;
#else /* GTK_CHECK_VERSION(2, 8, 0) */
static int prefer_cairo = FALSE;
#endif /* GTK_CHECK_VERSION(2, 8, 0) */

static struct {
    int refcount;
    GdkPixmap *pixmap;
    GdkPixmap *insensitive_pixmap;
    GdkBitmap *mask;
    GdkGC     *mask_gc;
    GdkGC     *red_gc;
#if GTK_CHECK_VERSION(2, 8, 0)
    cairo_surface_t *knob_surface;
    cairo_surface_t *insensitive_knob_surface;
    cairo_pattern_t *shadow_pattern;
#endif
} common;

G_DEFINE_TYPE (GtkKnob, gtk_knob, GTK_TYPE_WIDGET);

static void     gtk_knob_class_init    (GtkKnobClass *klass);
static void     gtk_knob_init          (GtkKnob *knob);
static void     gtk_knob_destroy       (GtkObject *object);
static void     gtk_knob_finalize      (GObject   *object);
static void     gtk_knob_realize       (GtkWidget *widget);
static void     gtk_knob_size_request  (GtkWidget *widget, GtkRequisition *requisition);
static void     gtk_knob_size_allocate (GtkWidget *widget, GtkAllocation  *allocation);
static gint     gtk_knob_button_press  (GtkWidget *widget, GdkEventButton *event);
static gint     gtk_knob_button_release(GtkWidget *widget, GdkEventButton *event);
static gint     gtk_knob_motion_notify (GtkWidget *widget, GdkEventMotion *event);
static gint     gtk_knob_timer_callback(GtkKnob *knob);
static void     gtk_knob_update_mouse  (GtkKnob *knob, gint x, gint y, gboolean absolute);
static void     gtk_knob_adjustment_value_changed (GtkAdjustment *adjustment,
                                                   gpointer       data);
static void     gtk_knob_adjustment_changed (GtkAdjustment *adjustment,
                                             gpointer       data);
static gboolean gtk_knob_expose        (GtkWidget *knob, GdkEventExpose *event);
static void     gtk_knob_common_initialize(GtkWidget *widget);
static void     gtk_knob_common_finalize(void);


static void
gtk_knob_class_init (GtkKnobClass *class)
{
    GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
    GtkObjectClass *object_class = (GtkObjectClass*) class;
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

    parent_class = g_type_class_peek_parent (class);

    gobject_class->finalize = gtk_knob_finalize;
    object_class->destroy = gtk_knob_destroy;

    widget_class->realize = gtk_knob_realize;
    widget_class->size_request = gtk_knob_size_request;
    widget_class->size_allocate = gtk_knob_size_allocate;
    widget_class->expose_event = gtk_knob_expose;
    widget_class->button_press_event = gtk_knob_button_press;
    widget_class->button_release_event = gtk_knob_button_release;
    widget_class->motion_notify_event = gtk_knob_motion_notify;

    common.refcount = 0;
    common.pixmap = NULL;
    common.insensitive_pixmap = NULL;
    common.mask = NULL;
    common.mask_gc = NULL;
    common.red_gc = NULL;
#if GTK_CHECK_VERSION(2, 8, 0)
    common.knob_surface = NULL;
    common.insensitive_knob_surface = NULL;
    common.shadow_pattern = NULL;
#endif
}


static void
gtk_knob_init (GtkKnob *knob)
{
    knob->adjustment = NULL;
    knob->policy = GTK_UPDATE_CONTINUOUS;
    knob->state = STATE_IDLE;
    knob->center_x = 0;
    knob->center_y = 0;
    knob->saved_x = 0;
    knob->saved_y = 0;
    knob->old_value = 0.0;
    knob->timer = 0;

    common.refcount++;
}


GtkWidget*
gtk_knob_new (GtkAdjustment *adjustment)
{
    GtkKnob *knob = g_object_new (GTK_TYPE_KNOB, NULL);

    gtk_knob_set_adjustment(knob, adjustment);

    return GTK_WIDGET(knob);
}


static void
gtk_knob_finalize (GObject *object)
{
    /* GtkKnob *knob = GTK_KNOB (object); */

    common.refcount--;
    if (common.refcount == 0) gtk_knob_common_finalize();

    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


static void
gtk_knob_destroy (GtkObject *object)
{
    GtkKnob *knob = GTK_KNOB(object);

    if (knob->adjustment) {
        g_signal_handlers_disconnect_by_func (knob->adjustment,
                                              gtk_knob_adjustment_changed,
                                              knob);
        g_signal_handlers_disconnect_by_func (knob->adjustment,
                                              gtk_knob_adjustment_value_changed,
                                              knob);
        g_object_unref(G_OBJECT(knob->adjustment));
        knob->adjustment = NULL;
    }
    if (knob->timer) {
        g_source_remove(knob->timer);
        knob->timer = 0;
    }

    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


void
gtk_knob_set_fast_rendering(gboolean setting)
{
#if GTK_CHECK_VERSION(2, 8, 0)
    prefer_cairo = (setting ? FALSE : TRUE);
#else
    prefer_cairo = FALSE;
#endif
}


GtkAdjustment*
gtk_knob_get_adjustment (GtkKnob *knob)
{
    g_return_val_if_fail (GTK_IS_KNOB (knob), NULL);

    if (!knob->adjustment)
        gtk_knob_set_adjustment (knob, NULL);

    return knob->adjustment;
}


void
gtk_knob_set_adjustment (GtkKnob *knob, GtkAdjustment *adjustment)
{
    g_return_if_fail (GTK_IS_KNOB (knob));

    if (!adjustment)
        adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    else
        g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

    if (knob->adjustment) {
        g_signal_handlers_disconnect_by_func (knob->adjustment,
                                              gtk_knob_adjustment_changed,
                                              knob);
        g_signal_handlers_disconnect_by_func (knob->adjustment,
                                              gtk_knob_adjustment_value_changed,
                                              knob);
        g_object_unref(G_OBJECT(knob->adjustment));
    }

    knob->adjustment = adjustment;
    g_object_ref(G_OBJECT(knob->adjustment));
    gtk_object_sink(GTK_OBJECT(knob->adjustment)); /* deprecated, but g_object_ref_sink() requires glib 2.10 or later */

    g_signal_connect(G_OBJECT(adjustment), "changed",
                     G_CALLBACK(gtk_knob_adjustment_changed), (gpointer) knob);
    g_signal_connect(G_OBJECT(adjustment), "value_changed",
                     G_CALLBACK(gtk_knob_adjustment_value_changed), (gpointer) knob);

    gtk_knob_adjustment_changed (adjustment, knob);
}


void
gtk_knob_set_update_policy(GtkKnob *knob, GtkUpdateType policy)
{
    g_return_if_fail (GTK_IS_KNOB (knob));

    knob->policy = policy;
}


static void
gtk_knob_realize (GtkWidget *widget)
{
    GtkKnob *knob;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (GTK_IS_KNOB (widget));

    knob = GTK_KNOB (widget);
    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);
    attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK |
                                GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                                GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK;

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
    gdk_window_set_user_data (widget->window, knob);

    widget->style = gtk_style_attach (widget->style, widget->window);
    gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

    gtk_knob_common_initialize(widget);
}


static void
gtk_knob_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
    requisition->width = KNOB_SIZE;
    requisition->height = KNOB_SIZE;
}


static void
gtk_knob_size_allocate (GtkWidget     *widget,
                        GtkAllocation *allocation)
{
    g_return_if_fail (GTK_IS_KNOB (widget));
    g_return_if_fail (allocation != NULL);

    widget->allocation = *allocation;
 
    if (GTK_WIDGET_REALIZED (widget)) {
        gdk_window_move_resize (widget->window,
                                allocation->x, allocation->y,
                                allocation->width, allocation->height);
    }
}


static gint
gtk_knob_button_press (GtkWidget *widget, GdkEventButton *event)
{
    GtkKnob *knob;

    g_return_val_if_fail(GTK_IS_KNOB(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    knob = GTK_KNOB(widget);

    switch (knob->state) {
      case STATE_IDLE:
        switch (event->button) {
          case 1:
          case 3:
            gtk_grab_add(widget);
            knob->state = STATE_PRESSED;
            knob->center_x = widget->allocation.width / 2;
            knob->center_y = widget->allocation.height / 2;
            knob->saved_x = event->x;
            knob->saved_y = event->y;
            break;

          default:
            break;
        }
        break;

      default:
        break;
    }

    return FALSE;
}


static gint
gtk_knob_button_release (GtkWidget *widget, GdkEventButton *event)
{
    GtkKnob *knob;
    gfloat value;

    g_return_val_if_fail(GTK_IS_KNOB(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    knob = GTK_KNOB(widget);

    switch (knob->state) {
      case STATE_PRESSED:
        gtk_grab_remove(widget);
        knob->state = STATE_IDLE;

        switch (event->button) {
          case 1:
            value = knob->adjustment->value - knob->adjustment->page_increment;
            value = MAX(MIN(value, knob->adjustment->upper),
                        knob->adjustment->lower);
            if (knob->adjustment->value != value) {
                knob->adjustment->value = value;
                g_signal_emit_by_name(G_OBJECT(knob->adjustment), "value_changed");
            }
            break;

          case 3:
            value = knob->adjustment->value + knob->adjustment->page_increment;
            value = MAX(MIN(value, knob->adjustment->upper),
                        knob->adjustment->lower);
            if (knob->adjustment->value != value) {
                knob->adjustment->value = value;
                g_signal_emit_by_name(G_OBJECT(knob->adjustment), "value_changed");
            }
            break;

          default:
            break;
        }
        break;

      case STATE_DRAGGING:
        gtk_grab_remove(widget);
        knob->state = STATE_IDLE;

        if (knob->policy != GTK_UPDATE_CONTINUOUS && knob->old_value != knob->adjustment->value)
            g_signal_emit_by_name(G_OBJECT(knob->adjustment), "value_changed");

        break;

      default:
        break;
    }

    return FALSE;
}


static gint
gtk_knob_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
    GtkKnob *knob;
    GdkModifierType mods;
    gint x, y;

    g_return_val_if_fail(GTK_IS_KNOB(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    knob = GTK_KNOB(widget);

    x = event->x;
    y = event->y;

    if (event->is_hint || (event->window != widget->window))
        gdk_window_get_pointer(widget->window, &x, &y, &mods);

    switch (knob->state) {
      case STATE_PRESSED:
        knob->state = STATE_DRAGGING;
        /* fall through */

      case STATE_DRAGGING:
        if (mods & GDK_BUTTON1_MASK) {
            gtk_knob_update_mouse(knob, x, y, TRUE);
            return TRUE;
        } else if (mods & GDK_BUTTON3_MASK) {
            gtk_knob_update_mouse(knob, x, y, FALSE);
            return TRUE;
        }
        break;

      default:
        break;
    }

    return FALSE;
}


static gint
gtk_knob_timer_callback (GtkKnob *knob)
{
    g_return_val_if_fail(GTK_IS_KNOB(knob), FALSE);

    if (knob->policy == GTK_UPDATE_DELAYED)
        g_signal_emit_by_name(G_OBJECT(knob->adjustment), "value_changed");

    return FALSE;       /* don't keep running this timer */
}


static void
gtk_knob_update_mouse(GtkKnob *knob, gint x, gint y, gboolean absolute)
{
    gfloat old_value, new_value;
    gdouble angle;
    gint dv, dh;

    g_return_if_fail(GTK_IS_KNOB(knob));

    old_value = knob->adjustment->value;

    if (absolute) {

        angle = atan2(-y + knob->center_y, x - knob->center_x);
        angle /= M_PI;
        if (angle < -0.5)
            angle += 2;

        new_value = -(2.0/3.0) * (angle - 1.25);   /* map [1.25pi, -0.25pi] onto [0, 1] */
        new_value *= knob->adjustment->upper - knob->adjustment->lower;
        new_value += knob->adjustment->lower;

    } else {

        dv = knob->center_y - y; /* inverted cartesian graphics coordinate system */
        dh = x - knob->center_x;

        if (abs(dv) < HALF_KNOB_SIZE && abs(dh) < HALF_KNOB_SIZE) {
            
            /* translate close-in motion into fine adjustment */
            dv = knob->saved_y - y;
            dh = x - knob->saved_x;
            new_value = knob->adjustment->value +
                        (float)(dv + dh) * knob->adjustment->step_increment;

        } else if (abs(dv) > abs(dh)) {

            /* translate up-and-down motion into fine adjustment */
            dv = knob->saved_y - y;
            new_value = knob->adjustment->value +
                        (float)dv * knob->adjustment->step_increment;

        } else {

            /* translate side-to-side motion into coarse adjustment */
            dh = x - knob->saved_x;
            new_value = knob->adjustment->value +
                        (float)dh * (knob->adjustment->upper -
                                     knob->adjustment->lower) / 300.0f;

        }
        knob->saved_x = x;
        knob->saved_y = y;
    }

    new_value = MAX(MIN(new_value, knob->adjustment->upper),
                    knob->adjustment->lower);

    knob->adjustment->value = new_value;

    if (knob->adjustment->value != old_value) {

        if (knob->policy == GTK_UPDATE_CONTINUOUS)
            g_signal_emit_by_name(G_OBJECT(knob->adjustment), "value_changed");
        else {
            gtk_widget_queue_draw (GTK_WIDGET(knob));

            if (knob->policy == GTK_UPDATE_DELAYED) {
                if (knob->timer)
                    g_source_remove(knob->timer);

                knob->timer = g_timeout_add (UPDATE_DELAY_LENGTH,
                                             (GSourceFunc) gtk_knob_timer_callback,
                                             (gpointer) knob);
          }
        }
    }
}


static void
gtk_knob_adjustment_changed(GtkAdjustment *adjustment, gpointer data)
{
    GtkKnob *knob;

    g_return_if_fail(adjustment != NULL);
    g_return_if_fail(data != NULL);

    knob = GTK_KNOB(data);

    knob->old_value = adjustment->value;

    gtk_widget_queue_draw (GTK_WIDGET(knob));
}

static void
gtk_knob_adjustment_value_changed (GtkAdjustment *adjustment, gpointer data)
{
    GtkKnob *knob;

    g_return_if_fail(adjustment != NULL);
    g_return_if_fail(data != NULL);

    knob = GTK_KNOB(data);

    if (knob->old_value != adjustment->value) {
        knob->old_value = adjustment->value;

        gtk_widget_queue_draw (GTK_WIDGET(knob));
    }
}


static void
gtk_knob_paint_xlib(GtkWidget *widget, int center_x, int center_y)
{
    GtkKnob *knob = GTK_KNOB(widget);
    gfloat dx, dy;

    gdk_window_clear_area(widget->window, 0, 0, widget->allocation.width, widget->allocation.height);

    center_x -= HALF_KNOB_SIZE;
    center_y -= HALF_KNOB_SIZE;
    gdk_gc_set_clip_origin(common.mask_gc, center_x, center_y);

    dx = knob->adjustment->value - knob->adjustment->lower;
    dy = knob->adjustment->upper - knob->adjustment->lower;

    if (dy != 0 && GTK_WIDGET_IS_SENSITIVE (widget)) {
        dx = MIN(MAX(dx / dy, 0), 1);
        dx = -1.5 * dx + 1.25;

        gdk_draw_arc (widget->window, common.red_gc, TRUE,
                      1 + center_x , 1 + center_y,
                      KNOB_SIZE - 4, KNOB_SIZE - 2,
                      dx * 180.f * 64.f, (1.25f - dx) * 180.f * 64.f);
        gdk_draw_pixmap(widget->window, common.mask_gc, common.pixmap,
                        0, 0, center_x, center_y, KNOB_SIZE, KNOB_SIZE);

        dx *= M_PI;
        dy = -sin(dx) * 11 + 15.5;
        dx = cos(dx) * 11 + 15.5;

        gdk_draw_line(widget->window, widget->style->white_gc,
                      15 + center_x, 16 + center_y,
                      dx + center_x, dy + center_y);
    } else {
        gdk_draw_pixmap(widget->window, common.mask_gc, common.insensitive_pixmap,
                        0, 0, center_x, center_y, KNOB_SIZE, KNOB_SIZE);
    }
}


#if GTK_CHECK_VERSION(2, 8, 0)
static void
gtk_knob_paint_cairo(GtkWidget *widget, cairo_t *cr)
{
    GtkKnob *knob = GTK_KNOB(widget);
    gfloat value, range;

    if (GTK_WIDGET_IS_SENSITIVE (widget)) {

        range = knob->adjustment->upper - knob->adjustment->lower;
        if (fabs(range) < 1e-37) {
            value = 0.0;
        } else {
            value = MAX(MIN(knob->adjustment->value, knob->adjustment->upper),
                        knob->adjustment->lower);
            value = value - knob->adjustment->lower;
            value /= range;  /* now scaled 0.0 to 1.0 */
        }

        /* draw the red arc */
        cairo_set_source_rgb (cr, 1.0, 0, 0);
        cairo_move_to (cr, HALF_KNOB_SIZE, HALF_KNOB_SIZE);
        cairo_arc (cr, HALF_KNOB_SIZE, HALF_KNOB_SIZE, (KNOB_SIZE * 0.47),
                   M_PI * 0.75, M_PI * 0.75 + value * (1.5 * M_PI));
        cairo_close_path (cr);
        cairo_fill (cr);

        /* draw the knob shadow */
        cairo_set_source (cr, common.shadow_pattern);
        cairo_paint (cr);

        /* paint the knob and reference points over what we have so far */
        cairo_set_source_surface (cr, common.knob_surface, 0, 0);
        cairo_paint (cr);

        /* add cursor to knob */
        cairo_translate (cr, HALF_KNOB_SIZE, HALF_KNOB_SIZE);
        cairo_rotate (cr, M_PI * 0.75 + value * (1.5 * M_PI));
        cairo_set_line_width (cr, KNOB_SIZE / 32.0 * 1.5);
        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        cairo_move_to (cr, KNOB_SIZE * 0.06, 0);
        cairo_line_to (cr, KNOB_SIZE * 0.30, 0);
        cairo_stroke (cr);

    } else {  /* insensitive */
        cairo_set_source_surface (cr, common.insensitive_knob_surface, 0, 0);
        cairo_paint (cr);
    }
}
#endif /* GTK_CHECK_VERSION(2, 8, 0) */


static gboolean
gtk_knob_expose (GtkWidget *widget, GdkEventExpose *event)
{
    int center_x, center_y;
#if GTK_CHECK_VERSION(2, 8, 0)
    int l, t, r, b;
    cairo_t *cr;
#endif /* GTK_CHECK_VERSION(2, 8, 0) */

    g_return_val_if_fail(GTK_IS_KNOB(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    /* minimize the clip area to just what we'll actually draw (32x32 at most) */
    center_x = widget->allocation.width / 2;
    center_y = widget->allocation.height / 2;

    if (prefer_cairo) {
#if GTK_CHECK_VERSION(2, 8, 0)
        if (common.knob_surface == NULL)
            gtk_knob_common_initialize(widget);
            
        if ((center_x - HALF_KNOB_SIZE >= event->area.x + event->area.width)  ||
            (center_x + HALF_KNOB_SIZE <= event->area.x)                      ||
            (center_y - HALF_KNOB_SIZE >= event->area.y + event->area.height) ||
            (center_y + HALF_KNOB_SIZE <= event->area.y)) {
            /* clip area is empty */
            return FALSE;
        }
        l = MAX(event->area.x,                      center_x - HALF_KNOB_SIZE);
        r = MIN(event->area.x + event->area.width,  center_x + HALF_KNOB_SIZE);
        t = MAX(event->area.y,                      center_y - HALF_KNOB_SIZE);
        b = MIN(event->area.y + event->area.height, center_y + HALF_KNOB_SIZE);
        cr = gdk_cairo_create (widget->window);
        cairo_rectangle (cr, l, t, r - l, b - t);
        cairo_clip (cr);
        cairo_translate(cr, center_x - HALF_KNOB_SIZE, center_y - HALF_KNOB_SIZE);

        gtk_knob_paint_cairo(widget, cr);

        cairo_destroy (cr);
#endif /* GTK_CHECK_VERSION(2, 8, 0) */
    } else {
        if (common.pixmap == NULL)
            gtk_knob_common_initialize(widget);

        gtk_knob_paint_xlib(widget, center_x, center_y);
    }

    return FALSE;
}


/* forward declarations of XPM data below */
static char * knob_xpm[];
static char * insensitive_knob_xpm[];

static void
gtk_knob_common_initialize_xlib(GtkWidget *widget)
{
    GdkColor color = { 0, 0xffff, 0, 0 };

    common.pixmap = gdk_pixmap_create_from_xpm_d(widget->window, &common.mask,
                                                 &widget->style->bg[GTK_STATE_NORMAL],
                                                 knob_xpm);
    common.insensitive_pixmap = gdk_pixmap_create_from_xpm_d(widget->window, &common.mask,
                                                 &widget->style->bg[GTK_STATE_NORMAL],
                                                 insensitive_knob_xpm);

    common.mask_gc = gdk_gc_new(widget->window);
    gdk_gc_copy(common.mask_gc, widget->style->bg_gc[GTK_STATE_NORMAL]);
    gdk_gc_set_clip_mask(common.mask_gc, common.mask);

    common.red_gc = gdk_gc_new(widget->window);
    gdk_gc_copy(common.red_gc, widget->style->bg_gc[GTK_STATE_NORMAL]);
    gdk_colormap_alloc_color(gtk_widget_get_colormap (widget), &color, FALSE, TRUE);
    gdk_gc_set_foreground(common.red_gc, &color);
}

static void
gtk_knob_common_finalize_xlib(void)
{
    if (common.pixmap) {
        gdk_pixmap_unref(common.pixmap);
        common.pixmap = NULL;
    }
    if (common.insensitive_pixmap) {
        gdk_pixmap_unref(common.insensitive_pixmap);
        common.insensitive_pixmap = NULL;
    }
    if (common.mask) {
        gdk_bitmap_unref(common.mask);
        common.mask = NULL;
    }
    if (common.mask_gc) {
        gdk_gc_unref(common.mask_gc);
        common.mask_gc = NULL;
    }
    if (common.red_gc) {
        gdk_gc_unref(common.red_gc);
        common.red_gc = NULL;
    }
}

#if GTK_CHECK_VERSION(2, 8, 0)

/* forward declarations of PNG data below */
static size_t        knob_c_png_length;
static unsigned char knob_c_png[];
static size_t        knob_i_png_length;
static unsigned char knob_i_png[];

struct cairo_read_func_closure {
    size_t         length;
    size_t         offset;
    unsigned char *data;
};

static cairo_status_t
cairo_read_func(void *closure, unsigned char *data, unsigned int length)
{
    struct cairo_read_func_closure *cl = (struct cairo_read_func_closure *)closure;
    size_t l = MIN(length, cl->length - cl->offset);

    memcpy(data, cl->data + cl->offset, l);

    cl->offset += l;

    return CAIRO_STATUS_SUCCESS;
}

static void
gtk_knob_common_initialize_cairo(void)
{
    struct cairo_read_func_closure cl;
    cairo_pattern_t *pat;

    /* create knob surfaces */
    cl.length = knob_c_png_length;
    cl.offset = 0;
    cl.data = knob_c_png;
    common.knob_surface = cairo_image_surface_create_from_png_stream(cairo_read_func, &cl);
    cl.length = knob_i_png_length;
    cl.offset = 0;
    cl.data = knob_i_png;
    common.insensitive_knob_surface = cairo_image_surface_create_from_png_stream(cairo_read_func, &cl);

    /* create the shadow pattern */
    pat = cairo_pattern_create_radial (KNOB_SIZE * 0.61, KNOB_SIZE * 0.61, 0.0,
                                       KNOB_SIZE * 0.61, KNOB_SIZE * 0.61, KNOB_SIZE * 0.4);  /* 0.375 */
    cairo_pattern_add_color_stop_rgba (pat, 0.75, 0, 0, 0, 0.5);
    cairo_pattern_add_color_stop_rgba (pat, 0.90, 0, 0, 0, 0.1);
    cairo_pattern_add_color_stop_rgba (pat, 1.00, 0, 0, 0, 0);
    common.shadow_pattern = pat;
}

static void
gtk_knob_common_finalize_cairo(void)
{
    if (common.knob_surface) {
        cairo_surface_destroy (common.knob_surface);
        common.knob_surface = NULL;
    }
    if (common.insensitive_knob_surface) {
        cairo_surface_destroy (common.insensitive_knob_surface);
        common.insensitive_knob_surface = NULL;
    }
    if (common.shadow_pattern) {
        cairo_pattern_destroy (common.shadow_pattern);
        common.shadow_pattern = NULL;
    }
}

#endif  /* GTK_CHECK_VERSION(2, 8, 0) */


static void
gtk_knob_common_initialize(GtkWidget *widget)
{
#if GTK_CHECK_VERSION(2, 8, 0)
    if (prefer_cairo) {
        gtk_knob_common_initialize_cairo();
        gtk_knob_common_finalize_xlib();
    } else {
        gtk_knob_common_initialize_xlib(widget);
        gtk_knob_common_finalize_cairo();
    }
#else /* GTK_CHECK_VERSION(2, 8, 0) */
    gtk_knob_common_initialize_xlib(widget);
#endif /* GTK_CHECK_VERSION(2, 8, 0) */
}

static void
gtk_knob_common_finalize(void)
{
    gtk_knob_common_finalize_xlib();
#if GTK_CHECK_VERSION(2, 8, 0)
    gtk_knob_common_finalize_cairo();
#endif /* GTK_CHECK_VERSION(2, 8, 0) */
}


static char * knob_xpm[] = {
"32 32 106 2",
"  	c None",
". 	c #FCFCFD",
"+ 	c #0A0A20",
"@ 	c #6B6B9E",
"# 	c #8989B3",
"$ 	c #66669A",
"% 	c #AEAECC",
"& 	c #AAAACA",
"* 	c #A6A6C7",
"= 	c #9C9CC0",
"- 	c #9494BB",
"; 	c #8E8EB7",
"> 	c #8C8CB6",
", 	c #7F7FAD",
"' 	c #A0A0C3",
") 	c #B5B5D1",
"! 	c #B0B0CE",
"~ 	c #8181AE",
"{ 	c #7C7CAA",
"] 	c #7575A5",
"^ 	c #595991",
"/ 	c #9696BC",
"( 	c #8383AF",
"_ 	c #7A7AA9",
": 	c #7171A3",
"< 	c #7070A2",
"[ 	c #626298",
"} 	c #42427E",
"| 	c #9292BA",
"1 	c #6E6EA0",
"2 	c #616197",
"3 	c #5F5F95",
"4 	c #585890",
"5 	c #464682",
"6 	c #2E2E6D",
"7 	c #9A9ABF",
"8 	c #69699D",
"9 	c #5B5B92",
"0 	c #4C4C86",
"a 	c #363674",
"b 	c #222262",
"c 	c #5E5E94",
"d 	c #52528B",
"e 	c #3F3F7C",
"f 	c #282868",
"g 	c #A2A2C4",
"h 	c #43437F",
"i 	c #2D2D6C",
"j 	c #1F1F5E",
"k 	c #333371",
"l 	c #202060",
"m 	c #30306F",
"n 	c #1D1D58",
"o 	c #53538C",
"p 	c #454581",
"q 	c #1C1C54",
"r 	c #19194C",
"s 	c #50508A",
"t 	c #3B3B79",
"u 	c #232363",
"v 	c #181848",
"w 	c #15153F",
"x 	c #4A4A85",
"y 	c #353573",
"z 	c #1C1C56",
"A 	c #14143E",
"B 	c #121237",
"C 	c #2F2F6E",
"D 	c #121236",
"E 	c #7777A7",
"F 	c #292969",
"G 	c #161642",
"H 	c #0E0E2C",
"I 	c #13133A",
"J 	c #0C0C26",
"K 	c #393977",
"L 	c #0E0E2B",
"M 	c #4F4F89",
"N 	c #161644",
"O 	c #0A0A1F",
"P 	c #474783",
"Q 	c #41417D",
"R 	c #3E3E7B",
"S 	c #54548D",
"T 	c #171747",
"U 	c #0A0A1E",
"V 	c #080819",
"W 	c #1E1E5A",
"X 	c #0F0F2D",
"Y 	c #09091C",
"Z 	c #070717",
"` 	c #2B2B6A",
" .	c #343472",
"..	c #252565",
"+.	c #19194B",
"@.	c #0D0D29",
"#.	c #070715",
"$.	c #1A1A4F",
"%.	c #101032",
"&.	c #0D0D28",
"*.	c #0F0F2F",
"=.	c #101030",
"-.	c #0B0B22",
";.	c #09091B",
">.	c #060614",
",.	c #060612",
"                                                                ",
"                              + +                               ",
"                              + +                               ",
"                                                                ",
"                            @ # # $                             ",
"                      % & * = - ; > # # ,                       ",
"                  ' ) ! & = ; # ~ , { { ] @ ^                   ",
"                % ) ) * / ( _ : < @ @ @ @ [ ^ }                 ",
"              & ) ) * | { 1 $ 2 2 3 3 3 3 3 4 5 6               ",
"            7 ) ) & | _ 8 3 3 3 3 3 3 3 3 3 9 0 a b             ",
"            & % * | { $ 3 3 3 3 3 3 3 3 3 3 c d e f             ",
"          ' & g | { $ 3 3 3 3 3 3 3 3 3 3 3 3 4 h i j           ",
"          * * / , @ 3 3 3 3 3 3 3 3 3 3 3 3 3 9 0 k l           ",
"          g ' > ] 2 3 3 3 3 3 3 3 3 3 3 3 3 3 ^ 0 m n           ",
"        1 ' = ( 1 3 3 3 3 3 3 3 3 3 3 3 3 3 9 o p i q r         ",
"        > = / , @ 3 3 3 3 3 3 3 3 3 3 3 3 c ^ s t u v w         ",
"        # 7 ; { $ 3 3 3 3 3 3 3 3 3 3 3 3 9 4 x y z A B         ",
"        3 > ( ] 2 3 3 3 3 3 3 3 3 3 3 3 3 ^ d 5 C r D B         ",
"          E E @ 3 3 3 3 3 3 3 3 3 3 3 3 9 4 s e F G H           ",
"          [ [ 3 c c 3 3 3 3 3 3 3 3 3 3 9 d 5 a j I J           ",
"          c ^ o o 9 9 3 3 3 3 c 9 9 9 ^ d 5 K u G L +           ",
"            s x x s 4 9 3 3 c 9 4 o s M x a u N H O             ",
"            P Q R h M S 9 9 ^ o M P h e k b T L U V             ",
"              K k a t h 0 M 0 x h K m f W G X Y Z               ",
"                ` i ` i 6 k  .m ` ..n +.B @.Y #.                ",
"          + +     l q r +.v $.$.v w %.&.U Z #.    + +           ",
"          + +         B *.=.=.=.@.-.;.>.,.        + +           ",
"                            %.J O J                             ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                "};

static char * insensitive_knob_xpm[] = {
"32 32 82 1",
" 	c None",
".	c #777777",
"+	c #B8B8B8",
"@	c #C7C7C7",
"#	c #B5B5B5",
"$	c #D9D9D9",
"%	c #D7D7D7",
"&	c #D5D5D5",
"*	c #D0D0D0",
"=	c #CCCCCC",
"-	c #CACACA",
";	c #C8C8C8",
">	c #C2C2C2",
",	c #D2D2D2",
"'	c #DCDCDC",
")	c #DADADA",
"!	c #C3C3C3",
"~	c #C0C0C0",
"{	c #BDBDBD",
"]	c #AFAFAF",
"^	c #CDCDCD",
"/	c #C4C4C4",
"(	c #BBBBBB",
"_	c #BABABA",
":	c #B3B3B3",
"<	c #A3A3A3",
"[	c #CBCBCB",
"}	c #B9B9B9",
"|	c #B2B2B2",
"1	c #AEAEAE",
"2	c #A5A5A5",
"3	c #989898",
"4	c #CFCFCF",
"5	c #B7B7B7",
"6	c #B0B0B0",
"7	c #A8A8A8",
"8	c #9C9C9C",
"9	c #919191",
"0	c #B1B1B1",
"a	c #ABABAB",
"b	c #A1A1A1",
"c	c #959595",
"d	c #D3D3D3",
"e	c #8F8F8F",
"f	c #9A9A9A",
"g	c #909090",
"h	c #999999",
"i	c #8D8D8D",
"j	c #ACACAC",
"k	c #A4A4A4",
"l	c #8B8B8B",
"m	c #888888",
"n	c #AAAAAA",
"o	c #9F9F9F",
"p	c #929292",
"q	c #878787",
"r	c #838383",
"s	c #A7A7A7",
"t	c #8C8C8C",
"u	c #808080",
"v	c #BEBEBE",
"w	c #848484",
"x	c #7C7C7C",
"y	c #818181",
"z	c #797979",
"A	c #9E9E9E",
"B	c #858585",
"C	c #A2A2A2",
"D	c #868686",
"E	c #767676",
"F	c #757575",
"G	c #8E8E8E",
"H	c #737373",
"I	c #969696",
"J	c #9B9B9B",
"K	c #939393",
"L	c #7A7A7A",
"M	c #8A8A8A",
"N	c #7E7E7E",
"O	c #7D7D7D",
"P	c #787878",
"Q	c #727272",
"                                ",
"               ..               ",
"               ..               ",
"                                ",
"              +@@#              ",
"           $%&*=-;@@>           ",
"         ,')%*-@!>~~{+]         ",
"        $''&^/~(_++++:]<        ",
"       %''&[~}#::|||||123       ",
"      4''%[~5|||||||||6789      ",
"      %$&[~#||||||||||0abc      ",
"     ,%d[~#||||||||||||1<3e     ",
"     &&^>+|||||||||||||67fg     ",
"     d,;{:|||||||||||||]7hi     ",
"    },*/}|||||||||||||6jk3lm    ",
"    ;*^>+||||||||||||0]nopqr    ",
"    @4-~#||||||||||||61s8tru    ",
"    |;/{:||||||||||||]a2hmuu    ",
"     vv+||||||||||||61nbcwx     ",
"     ::|00||||||||||6a28eyz     ",
"     0]jj66||||0666]a2Apwx.     ",
"      nssn16||061jnns8pBx.      ",
"      2Cb<nj66]jn2<bf9DxEF      ",
"       Af8o<7n7s<AhcGwxEH       ",
"        I3I33fJhIKimuLEH        ",
"     ..  glmmqMMqrNLEHH  ..     ",
"     ..    uOOOOLPFQQ    ..     ",
"              Nz.z              ",
"                                ",
"                                ",
"                                ",
"                                "};

#if GTK_CHECK_VERSION(2, 8, 0)

static size_t        knob_c_png_length = 1082;
static unsigned char knob_c_png[1082] =
  "\211PNG\15\12\32\12\0\0\0\15IHDR\0\0\0\40\0\0\0\40\10\6\0\0\0szz\364"
  "\0\0\0\1sRGB\0\256\316\34\351\0\0\0\6bKGD\0\377\0\377\0\377\240\275\247"
  "\223\0\0\3\342IDATX\303\355VM\213\34U\24=\357\325Gw'0\321\4\304\30w\371"
  "\15a\\d\231a6\1]E\30W\16\10.t'\"nD\307\215\333\254$\213\350&\270\226"
  "\20\2\223E>6\306e\230\14\202\211`23NwM\367t}\276z_\367=\27\325\335d\222"
  "\350twt!\346@AQE\335{\352\336s\356}\300\177\34~t\315\15\216\377;\330"
  "<\37\335\272\365\313\345$\311\227\262L\236\320\332\40\216\303\301\261"
  "c\235\33\27.,\256\376k\4n\336|\360q\232\346\27\215!XK\20BAJ\3\255-\234"
  "\363\40\"\0@U)xO\77\257\255\275\373\326\77F`}\375\2760\206:\306\20\264"
  "\266\310s\201<\257a\214\205\224\346\0\1\245\14\352ZCk[\\\272\364\301"
  "\302\13\23\270v\355\336D\345Y&\220\246\25\6\203\2u\255Q\327\32\336{\30"
  "c\17|#\245\206\326\2044-p\365\352gln\2O'\357\367\13$I\212<\27\220R\303"
  "\230\346\257\235s\7\2032\6\"\207\252R\350\367S\334\271\263\306f\266\341"
  "\372\372}1\276o\2\25\350v\207\30\14\12dY\5\245\14\234s\317$\37#\14\3"
  "\264Z\21:\235\26\226\227\277\334\236\211\300\355\333\17>MS\321Q\312\40"
  "\313\4\272\335\24I\222\242\337\317\40\204\234J\264A\20\240\325\212q\344"
  "H\7E\241N\235\77\377\325\367S\23\330\331\31|\3\0e)'eO\222\24J\231\351"
  "\355\305\200(\12\21\206\1\242(D\257\227\255\314\320\2\17k\11e)1\30\24"
  "\30\16\313\231\222\3\36\336{p\316\320\351\304\210\343\10D>\232\212\300"
  "\365\353\33\227\255m\354&\204\206\20\22B\250\331\26\204\307\310\232\16"
  "\336\3D\4\"\211s\347>\371\361\31\255<\375\240(\304\222sM\5\352ZM\304"
  "6+\32\201\2J\351\21\21\13k\353\327\16%\40\245=1\256\200\265\16Dn\276"
  "\31\3170\262\242\204\20""5\254-\261\277\257\337<\224@\303\336CJ\3\"z"
  "f\310\314\322\6\"\202R\32J\25""0\246\6c\366p\15(e\6y^\243\252$\2242\223"
  "a3\17\234s\320\332\200H\200H\0""0\371\241\4\30\303\215\275\275\14e\331"
  "\210o\236\3763\326LCk\233\12Z[\203\250F\247\23<\234j\24\257\256~\353"
  "\1L\26\3148\250\367\323$g\340\234\3039\217~\77\303\366\366.\312\3627"
  "(\325\307\326\326]6\225\6\232\3367k\227s\216v;\236\370\332\373\306\343"
  "\1779Z9\7c\14E!\220$\373\250\353\77`L\6\316\235\231z\20\2359s\372\243"
  "q\377\211\34\254u\10\202\0\214q\4A\0\316\331s\313>~\227\246%vv\22d\331"
  "\357\220\262\7kK,,DWf\332\206++\27\207E!^a\214#\212B\34=\332A\20p\204"
  "!\7\21M*\301\30\3cM(k\35\322\264D\2577@\226m\241\256\267`m\2010\364\335"
  "G\217\356\235\234y\35//\177\355\211\10a\30\"\212\"\264Z\21\332\355h\264"
  "n\351\200\320\2242\250*\211<\37\242\252v\241T\2\255\207pNc8\34\276\241"
  "Tow\256\3\311\331\263\237\373\40\10\340\234\7\347M\307\342\270\221N\223"
  "\2749\226)U\302\332\22R\356\301\230\34\326\26pN\242\333\375u\256\3\211"
  "\177\362\375\322\322\27\333\275\336\356)\"9\32""2\26\234\267F^W\260\266"
  "\2061\305\304\357D\22q\34\364\36\77\336|\375y\361f&\320T\342\303\357"
  "\372\375\235\367\244,#\306\330\23\216Qp\256\271\210$\30200\307\217\277"
  "zec\343\247\367\377.\336\13\35\313\27\27\337\371\241,\325i!\206'\235"
  "S\10C\226\267\333\361\303\315\315\273o\343%^bF\374\11\251\215\305RV\321"
  "\250V\0\0\0\0IEND\256B`\202";

static size_t        knob_i_png_length = 819;
static unsigned char knob_i_png[819] =
  "\211PNG\15\12\32\12\0\0\0\15IHDR\0\0\0\40\0\0\0\40\10\6\0\0\0szz\364"
  "\0\0\0\1sRGB\0\256\316\34\351\0\0\2\355IDATX\303\355W\273N#A\20\254\331"
  "]\303O8#'u@d\211\4\221\372\3\10\220\34\20\\\200D\300\27\40K\266d\210"
  "\20\374\7!1\21\22\"[9\363\37\354kf\272{\346\22\317j\15\346\3305Gp:Zr"
  "fOWUwW\267\201\1779\306\343\361\343x<~\374\312\33\21\376\367P\333\374"
  "\350\365\365\365\210\210\6\314\334\27\21""8\347\226\336\373\247\203\203"
  "\203\207o\3\220\246\351>3O\234s\273\336{Xk!\"X\1\200\367\36\0@D\246("
  "\212\213\343\343\343\227\277\6\40M\323{\0{!\2411\246\6\300\314\360\336"
  "\3039\7\0\20\21\20\21\312\262\\\214F\243\323/\3H\323\264\356rc\14\252"
  "\252BUU`f\20\321Z\362\20D\4\"B\236\347899\31n\15\340m\362\242(P\24\5"
  "\2141`f\210\10\0\324\3627CDP\226%\262,\303\331\331\331\260\363\30\256"
  "d\7\0Xk\353\344UUAk]K\277)9\0(\245\20\3071\222$\301l6\273\357\4\40M\323"
  "}k\355\36""3\303\30\203<\317k\0\326\332v\6\23EH\222\4;;;\210\343xo6\233"
  "\355\267\6\240\265\236\274e^\24\5\230\271\323\210\305q\\+\341\234\233"
  "\264\6\240\224\252G\255)y\347\31W\12\275^/\0\331m\5\340\371\371\371("
  "\214[\350\346\266\262\277\215\320\37\336{\210\10&\223\311Q\33\5\6\241"
  "\213\231\271n\266\256\341\234[3*\347\34\210h\360\366{\311\6\324\375M"
  "\16\267\15{\347\34\254\265\260\326\202\210\40\"\375O\1""4\3478\260\330"
  "\206}xCD`\255\5""3\2773\254\215%\320Z/\265\326k^\277\15\373&\201\306"
  ";\313O\1\224e\371T\226e-[\327\22\4\226!y\350\243\225u\77}\12\340\360"
  "\360\360\241i\271\333D\230\240P\377\240\300\325\325\325C\253\36p\316"
  "\31\21\331\15\362'IR\273\333G\214\233\322\33c\326v\207\265\26\3169\323"
  "\332\210D\344\"x@`\262i\353\205)a\346\272dD\204\252\252\220e\31\262,"
  "CUU\40\"\0\270h\15`4\32\2750\363\"$7\306\2041\252g:\324\267\371\35\255"
  "5\262,[[\\+\366\213\351t\372\322y\35\337\335\335=23\242(\2527[\257\327"
  "C\24Ek\354\233j5o\206P\377\371|>\334\372\40\271\271\271yTJ\301{\17\245"
  "\24\224R\210\242\10J\251\2651\13\15\27\256\245\240\330\237\222\177\10"
  "\40\334\372\267\267\267C\0\230N\247\367D\264'\"\365\15\20\307\361;\263"
  "\11\335\277j\336\305|>\77\335\364^\347\377\5\347\347\347\247\0~i\255"
  "M\250m\236\347k\237\306\251f\242(\372\25\222\177\313Y~yyy\244\224\32"
  "Xk\373\253%\263\4\360t}}\375\200\237\370\211\216\361\33\353\274\40}\314"
  "\227E/\0\0\0\0IEND\256B`\202";

#endif /* GTK_CHECK_VERSION(2, 8, 0) */

