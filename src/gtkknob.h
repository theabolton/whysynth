/* WhySynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2008 Sean Bolton
 *
 * Parts of this code come from GTK+, both the library source and
 * the example programs.  Other bits come from gAlan 0.2.0,
 * copyright (C) 1999 Tony Garnock-Jones.
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

#ifndef __GTK_KNOB_H__
#define __GTK_KNOB_H__

#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_KNOB            (gtk_knob_get_type ())
#define GTK_KNOB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_KNOB, GtkKnob))
#define GTK_KNOB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_KNOB, GtkKnobClass))
#define GTK_IS_KNOB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_KNOB))
#define GTK_IS_KNOB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_KNOB))
#define GTK_KNOB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_KNOB, GtkKnobClass))

typedef struct _GtkKnob       GtkKnob;
typedef struct _GtkKnobClass  GtkKnobClass;

struct _GtkKnob
{
    GtkWidget widget;

    GtkAdjustment *adjustment;

    guint  policy;   /* update policy (GTK_UPDATE_[CONTINUOUS/DELAYED/DISCONTINUOUS]) */
    guint  state;
    gint   center_x;
    gint   center_y;
    gint   saved_x;
    gint   saved_y;
    gfloat old_value;

    guint32 timer;   /* ID of update timer, or 0 if none */
};

struct _GtkKnobClass
{
    GtkWidgetClass parent_class;
};


GType          gtk_knob_get_type (void) G_GNUC_CONST;
GtkWidget     *gtk_knob_new (GtkAdjustment *adjustment);

void           gtk_knob_set_fast_rendering(gboolean setting);

GtkAdjustment *gtk_knob_get_adjustment(GtkKnob *knob);
void           gtk_knob_set_adjustment(GtkKnob *knob, GtkAdjustment *adjustment);

void           gtk_knob_set_update_policy(GtkKnob *knob, GtkUpdateType  policy);
GtkUpdateType  gtk_knob_get_update_policy(GtkKnob *knob);

G_END_DECLS

#endif /* __GTK_KNOB_H__ */

