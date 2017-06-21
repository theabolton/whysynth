/* WhySynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004-2017 Sean Bolton and others.
 *
 * Portions of this file may have come from Steve Brookes'
 * Xsynth, copyright (C) 1999 S. J. Brookes.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define _DEFAULT_SOURCE 1
#define _ISOC99_SOURCE  1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <gtk/gtk.h>
#include <lo/lo.h>

#include "whysynth_types.h"
#include "whysynth.h"
#include "whysynth_ports.h"
#include "whysynth_voice.h"
#include "gui_main.h"
#include "gui_callbacks.h"
#include "gui_images.h"
#include "gui_interface.h"
#include "common_data.h"
#include "wave_tables.h"

static unsigned char test_note_noteon_key = 60;
static int           test_note_noteoff_key = -1;
static unsigned char test_note_velocity = 96;

static int    open_path_set = 0;
static int    save_path_set = 0;
static char  *import_mode;

static struct posc osc_clipboard;
static int         osc_clipboard_valid = 0;
static struct peg  eg_clipboard;
static int         eg_clipboard_valid = 0;
static struct pvcf vcf_clipboard;
static int         vcf_clipboard_valid = 0;
static struct {
        int    effect_mode;
        float  effect_param1;
        float  effect_param2;
        float  effect_param3;
        float  effect_param4;
        float  effect_param5;
        float  effect_param6;
        float  effect_mix;
    }              effect_clipboard;
static int         effect_clipboard_valid = 0;

void
set_file_chooser_path(GtkFileChooser *file_chooser, const char *filename)
{
    char *folder = strdup(filename);
    char *last_slash = strrchr(folder, '/');

    GDB_MESSAGE(GDB_GUI, ": set_file_chooser_path called with '%s'\n", filename);

    if (last_slash && last_slash != folder) {
        *last_slash = '\0';
        gtk_file_chooser_set_current_folder (file_chooser, folder);
    }
    free(folder);
}

void
on_menu_open_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtk_widget_hide(import_file_chooser);
    gtk_widget_hide(save_file_chooser);

    (GTK_ADJUSTMENT(open_file_position_spin_adj))->value = (float)patch_count;
    (GTK_ADJUSTMENT(open_file_position_spin_adj))->upper = (float)patch_count;
    gtk_signal_emit_by_name (GTK_OBJECT (open_file_position_spin_adj), "value_changed");

    gtk_widget_show(open_file_chooser);
}


void
on_menu_save_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtk_widget_hide(open_file_chooser);
    gtk_widget_hide(import_file_chooser);

    (GTK_ADJUSTMENT(save_file_start_spin_adj))->upper = (float)(patch_count - 1);
    (GTK_ADJUSTMENT(save_file_end_spin_adj))->value = (float)(patch_count - 1);
    (GTK_ADJUSTMENT(save_file_end_spin_adj))->upper = (float)(patch_count - 1);
    gtk_signal_emit_by_name (GTK_OBJECT (save_file_start_spin_adj), "value_changed");
    gtk_signal_emit_by_name (GTK_OBJECT (save_file_end_spin_adj), "value_changed");

    gtk_widget_show(save_file_chooser);
}


void
on_menu_import_activate         (GtkMenuItem     *menuitem,
                                 gpointer         user_data)
{
    gtk_widget_hide(open_file_chooser);
    gtk_widget_hide(save_file_chooser);

    import_mode = (char *)user_data;
    if (!strcmp(import_mode, "xsynth")) {
        gtk_window_set_title(GTK_WINDOW(import_file_chooser), "WhySynth - Import Xsynth-DSSI Patches");
    } else if (!strcmp(import_mode, "k4")) {
        gtk_window_set_title(GTK_WINDOW(import_file_chooser), "WhySynth - Import Kawai K4 Patches");
    } else {
        gtk_window_set_title(GTK_WINDOW(import_file_chooser), "WhySynth - Import Patches");
    }

    (GTK_ADJUSTMENT(import_file_position_spin_adj))->value = (float)patch_count;
    (GTK_ADJUSTMENT(import_file_position_spin_adj))->upper = (float)patch_count;
    gtk_signal_emit_by_name (GTK_OBJECT (import_file_position_spin_adj), "value_changed");

    gtk_widget_show(import_file_chooser);
}


void
on_menu_quit_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    /* -FIX- if any patches changed, ask "are you sure?" or "save changes?" */
    gtk_main_quit();
}


void
on_menu_edit_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (!GTK_WIDGET_MAPPED(edit_window))
        gtk_widget_show(edit_window);
    else
        gdk_window_raise(edit_window->window);
}


void
on_menu_about_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char buf[256];
#ifdef HAVE_CONFIG_H
    snprintf(buf, 256, "WhySynth version: " VERSION "\n"
#else
    snprintf(buf, 256,
#endif
                       "plugin URL: %s\n"
                       "host URL: %s\n", osc_self_url, osc_host_url);
    gtk_label_set_text (GTK_LABEL (about_label), buf);
    gtk_widget_show(about_window);
}

gint
on_delete_event_wrapper( GtkWidget *widget, GdkEvent *event, gpointer data )
{
    void (*handler)(GtkWidget *, gpointer) = (void (*)(GtkWidget *, gpointer))data;

    /* call our 'close', 'dismiss' or 'cancel' callback (which must not need the user data) */
    (*handler)(widget, NULL);

    /* tell GTK+ to NOT emit 'destroy' */
    return TRUE;
}

void
on_open_file_chooser_response(GtkDialog *dialog, gint response, gpointer data)
{
    int position = lrintf(GTK_ADJUSTMENT(open_file_position_spin_adj)->value);
    char *filename, *message;

    gtk_widget_hide(open_file_chooser);

    switch (response) {

      case GTK_RESPONSE_ACCEPT:
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        GDB_MESSAGE(GDB_GUI, ": on_open_file_chooser_response called with 'accept', file '%s', position %d\n",
                    filename, position);

        open_path_set = 1;
        if (!save_path_set)
            set_file_chooser_path(GTK_FILE_CHOOSER(save_file_chooser), filename);

        if (gui_data_load(filename, position, &message)) {

            /* successfully loaded at least one patch */
            rebuild_patches_list();
            display_notice("Open Patch File succeeded:", message);

            if (patches_dirty) {

                /* our patch bank is dirty, so we need to save a temporary copy
                 * for the plugin to load */
                if (gui_data_save_dirty_patches_to_tmp()) {
                    lo_send(osc_host_address, osc_configure_path, "ss", "load",
                            patches_tmp_filename);
                    last_configure_load_was_from_tmp = 1;
                } else {
                    display_notice("Open Patch File error:", "couldn't save temporary patch bank");
                }
            
            } else {

                /* patches is clean after the load, so tell the plugin to
                 * load the same file */

                lo_send(osc_host_address, osc_configure_path, "ss", "load",
                        filename);

                /* clean up old temporary file, if any */
                if (last_configure_load_was_from_tmp) {
                    unlink(patches_tmp_filename);
                }
                last_configure_load_was_from_tmp = 0;
            }

        } else {  /* didn't load anything successfully */

            display_notice("Open Patch File failed:", message);

        }
        free(message);
        g_free(filename);
        break;

      case GTK_RESPONSE_CANCEL:
        GDB_MESSAGE(GDB_GUI, ": on_open_file_chooser_response called with 'cancel'\n");
        break;

      default:
        GDB_MESSAGE(GDB_GUI, ": on_open_file_chooser_response called with %d\n", response);
        break;
    }
}

/*
 * on_position_change
 *
 * used by both the open file position and edit save position dialogs
 * data is a pointer to the dialog's patch name label
 */
void
on_position_change(GtkWidget *widget, gpointer data)
{
    int position = lrintf(GTK_ADJUSTMENT(widget)->value);
    GtkWidget *label = (GtkWidget *)data;

    if (position >= patch_count) {
        gtk_label_set_text (GTK_LABEL (label), "(empty)");
    } else {
        gtk_label_set_text (GTK_LABEL (label), patches[position].name);
    }
}

void
on_save_file_chooser_response(GtkDialog *dialog, gint response, gpointer data)
{
    char *filename, *message;

    int save_file_start = lrintf(GTK_ADJUSTMENT(save_file_start_spin_adj)->value);
    int save_file_end   = lrintf(GTK_ADJUSTMENT(save_file_end_spin_adj)->value);

    gtk_widget_hide(save_file_chooser);

    switch (response) {

      case GTK_RESPONSE_ACCEPT:
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        GDB_MESSAGE(GDB_GUI, ": on_save_file_chooser_response called with 'accept', file '%s', "
                    "start %d, end %d\n", filename, save_file_start, save_file_end);

        save_path_set = 1;
        if (!open_path_set)
            set_file_chooser_path(GTK_FILE_CHOOSER(open_file_chooser), filename);

#ifdef DEVELOPER
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (save_file_c_mode_button))) {
            if (gui_data_save_as_c(filename, save_file_start, save_file_end, &message)) {

                display_notice("Save Patches as 'C' succeeded:", message);

            } else {  /* problem with save */

                display_notice("Save Patches as 'C' failed:", message);

            }
        } else {
#endif /* DEVELOPER */
            if (gui_data_save(filename, save_file_start, save_file_end, &message)) {

                display_notice("Save Patch File succeeded:", message);

                if (save_file_start == 0 && save_file_end + 1 == patch_count) {
            
                    patches_dirty = 0;
                    lo_send(osc_host_address, osc_configure_path, "ss", "load",
                            filename);
                    /* clean up old temporary file, if any */
                    if (last_configure_load_was_from_tmp) {
                        unlink(patches_tmp_filename);
                    }
                    last_configure_load_was_from_tmp = 0;
                }

            } else {  /* problem with save */

                display_notice("Save Patch File failed:", message);

            }
#ifdef DEVELOPER
        }
#endif /* DEVELOPER */
        free(message);
        g_free(filename);
        break;

      case GTK_RESPONSE_CANCEL:
        GDB_MESSAGE(GDB_GUI, ": on_save_file_chooser_response called with 'cancel'\n");
        break;

      default:
        GDB_MESSAGE(GDB_GUI, ": on_save_file_chooser_response called with %d\n", response);
        break;
    }
}

/*
 * on_save_file_range_change
 */
void
on_save_file_range_change(GtkWidget *widget, gpointer data)
{
    int which = GPOINTER_TO_INT(data);
    int start = lrintf(GTK_ADJUSTMENT(save_file_start_spin_adj)->value);
    int end   = lrintf(GTK_ADJUSTMENT(save_file_end_spin_adj)->value);

    if (which == 0) {  /* start */
        if (end < start) {
            (GTK_ADJUSTMENT(save_file_end_spin_adj))->value = (float)start;
            gtk_signal_emit_by_name (GTK_OBJECT (save_file_end_spin_adj), "value_changed");
        }
        gtk_label_set_text (GTK_LABEL (save_file_start_name), patches[start].name);
    } else { /* end */
        if (end < start) {
            (GTK_ADJUSTMENT(save_file_start_spin_adj))->value = (float)end;
            gtk_signal_emit_by_name (GTK_OBJECT (save_file_start_spin_adj), "value_changed");
        }
        gtk_label_set_text (GTK_LABEL (save_file_end_name), patches[end].name);
    }
}

void
on_import_file_chooser_response(GtkDialog *dialog, gint response, gpointer data)
{
    int position = lrintf(GTK_ADJUSTMENT(import_file_position_spin_adj)->value);
    int dual = GTK_TOGGLE_BUTTON (import_file_position_dual_button)->active ? 1 : 0;
    char *filename, *message;
    int result;

    gtk_widget_hide(import_file_chooser);

    switch (response) {

      case GTK_RESPONSE_ACCEPT:
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        GDB_MESSAGE(GDB_GUI, ": on_import_file_chooser_response called with 'accept', "
                    "file '%s', position %d, dual %d\n",
                    filename, position, dual);

        if (!strcmp(import_mode, "xsynth")) {
            result = gui_data_import_xsynth(filename, position, dual, &message);
        } else if (!strcmp(import_mode, "k4")) {
            result = gui_data_interpret_k4(filename, position, dual, &message);
        } else {
            result = 0;
            message = "unknown import type";
        }

        if (result) {
            /* successfully imported at least one patch */
            rebuild_patches_list();
            display_notice("Import Patches succeeded:", message);

            /* patch bank is always dirty after a successful import, so we need
             * to save a temporary copy for the plugin to load */
            if (gui_data_save_dirty_patches_to_tmp()) {
                lo_send(osc_host_address, osc_configure_path, "ss", "load",
                        patches_tmp_filename);
                last_configure_load_was_from_tmp = 1;
            } else {
                display_notice("Import Patches error:", "couldn't save temporary patch bank");
            }

        } else {  /* didn't import anything successfully */

            display_notice("Import Patches failed:", message);

        }
        free(message);
        g_free(filename);
        break;

      case GTK_RESPONSE_CANCEL:
        GDB_MESSAGE(GDB_GUI, ": on_import_file_chooser_response called with 'cancel'\n");
        break;

      default:
        GDB_MESSAGE(GDB_GUI, ": on_import_file_chooser_response called with %d\n", response);
        break;
    }
}

void
on_about_dismiss( GtkWidget *widget, gpointer data )
{
    gtk_widget_hide(about_window);
}

/* ========== Patch List ========== */

/* Comparison function for the patch list model sorting. */
gint
patches_list_sort_func(GtkTreeModel *model,
                       GtkTreeIter  *a,
                       GtkTreeIter  *b,
                       gpointer      userdata)
{
    static gint secondary_sort_on_name = FALSE;  /* -FIX- hidden state */
    gint sortcol = GPOINTER_TO_INT(userdata);
    gint ret;
    gchar *a_name, *b_name;

    /* Set secondary_sort_on_name if the current order is by number or name, otherwise, if the
     * current order is by category, leave it at its previous value. */
    if (sortcol == PATCHES_LIST_COL_NUMBER) {
        secondary_sort_on_name = FALSE;
    } else if (sortcol == PATCHES_LIST_COL_NAME) {
        secondary_sort_on_name = TRUE;
    }

    /* 'ProgNo' and 'Name' sort only on themselves. 'Category' will use a secondary key of 'ProgNo'
     * or 'Name' depending on whether secondary_sort_on_name is true. The control flow here is a
     * slightly non-obvious fall-through based on that. */
    if (sortcol == PATCHES_LIST_COL_CATEGORY) {
        gchar *a_category, *b_category;

        gtk_tree_model_get(model, a, PATCHES_LIST_COL_CATEGORY, &a_category, -1);
        gtk_tree_model_get(model, b, PATCHES_LIST_COL_CATEGORY, &b_category, -1);
        ret = g_utf8_collate(a_category, b_category);
        g_free(a_category);
        g_free(b_category);
        if (ret != 0) {
            return ret;
        }
        /* fall through to sort by either number or name secondary key */
    }
    if ((sortcol == PATCHES_LIST_COL_NUMBER) ||
        (sortcol == PATCHES_LIST_COL_CATEGORY && !secondary_sort_on_name)) {
        /* sort (or sub-sort) on patch number */
        guint a_number, b_number;

        gtk_tree_model_get(model, a, PATCHES_LIST_COL_NUMBER, &a_number, -1);
        gtk_tree_model_get(model, b, PATCHES_LIST_COL_NUMBER, &b_number, -1);
        return a_number - b_number;
    }
    /* sort on name */
    gtk_tree_model_get(model, a, PATCHES_LIST_COL_NAME, &a_name, -1);
    gtk_tree_model_get(model, b, PATCHES_LIST_COL_NAME, &b_name, -1);
    // !FIX! case insensitive, elide punctuation? (then if equal, recompare originals for more stability)
    // => no, leave punctuation
    // => g_utf_strup(), g_utf8_strdown(), g_utf8_casefold(),
    // https://developer.gnome.org/glib/stable/glib-Unicode-Manipulation.html#g-utf8-collate
    ret = g_utf8_collate(a_name, b_name);
    g_free(a_name);
    g_free(b_name);
    return ret;
}

static gboolean
find_program_in_list_store(GtkTreeIter *iter, int program)
{
    /* An O(n) search for the requested program within the sorted GtkListStore. Probably using
     * GtkTreeModelSort would be more efficient, but also more complicated.
     * `program` must be a valid program number. Sets the `iter` to point to the list store row
     * containing the patch, and returns TRUE. Should never return FALSE. */
    GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(patches_list));
    int index;

    if (!gtk_tree_model_get_iter_first(store, iter))
         return FALSE;
    do {
        gtk_tree_model_get(store, iter, PATCHES_LIST_COL_NUMBER, &index, -1);
        if (index == program)
            return TRUE;
    } while (gtk_tree_model_iter_next(store, iter));

    return FALSE;
}

void
on_patches_selection_changed(GtkTreeSelection *selection,
                             gpointer data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
        guint index;

        gtk_tree_model_get(model, &iter, PATCHES_LIST_COL_NUMBER, &index, -1);

        GDB_MESSAGE(GDB_GUI, " on_patches_selection_changed: patch %d selected\n", index);

        /* set all the patch edit widgets to match */
        update_voice_widgets_from_patch(&patches[index]);

        lo_send(osc_host_address, osc_program_path, "ii", index / 128, index % 128);
    }
}

void
on_patches_row_activated(GtkTreeView       *tree_view,
                         GtkTreePath       *path,
                         GtkTreeViewColumn *column,
                         gpointer           user_data)
{
    /* gint *indicies = gtk_tree_path_get_indices(path); */
    /* gint row = indicies[0]; */

    GDB_MESSAGE(GDB_GUI, " on_patches_row_activated\n");

    /* User double-clicked on a row. The first click will have triggered
     * on_patches_selection_changed() above, so all we need to do here is make sure the
     * window is shown */
    on_menu_edit_activate(NULL, NULL);
}

/* ========== Patch Edit Window ========== */

void
on_voice_knob_change( GtkWidget *widget, gpointer data )
{
    int index = GPOINTER_TO_INT(data);
    float value;

    value = get_value_from_knob(index);
    
    GDB_MESSAGE(GDB_GUI, " on_voice_knob_change: knob %d changed to %10.6f => %10.6f\n",
            index, GTK_ADJUSTMENT(widget)->value, value);

    lo_send(osc_host_address, osc_control_path, "if", index, value);
}

void
on_voice_knob_zero(GtkWidget *widget, gpointer data)
{
    int port = GPOINTER_TO_INT(data);
    struct y_port_descriptor *ypd = &y_port_description[port];

    if (ypd->type == Y_PORT_TYPE_PAN) {
        GDB_MESSAGE(GDB_GUI, " on_voice_knob_zero: setting knob %d to 0.5\n", port);
        update_voice_widget(port, 0.5f, TRUE);
    } else {
        GDB_MESSAGE(GDB_GUI, " on_voice_knob_zero: setting knob %d to 0\n", port);
        update_voice_widget(port, 0.0f, TRUE);
    }
}

void
check_for_layout_update_on_port_change(int index)
{
    /* update GUI layouts in response to mode changes */
    switch (index) {
      case Y_PORT_OSC1_MODE:
      case Y_PORT_OSC2_MODE:
      case Y_PORT_OSC3_MODE:
      case Y_PORT_OSC4_MODE:
        update_osc_layout_on_mode_change(index);
        break;

      case Y_PORT_OSC1_WAVEFORM:
      case Y_PORT_OSC2_WAVEFORM:
      case Y_PORT_OSC3_WAVEFORM:
      case Y_PORT_OSC4_WAVEFORM:
        update_osc_layout_on_mode_change(index - Y_PORT_OSC1_WAVEFORM + Y_PORT_OSC1_MODE);
        break;

      case Y_PORT_VCF1_MODE:
      case Y_PORT_VCF2_MODE:
        update_vcf_layout_on_mode_change(index);
        break;

      case Y_PORT_EFFECT_MODE:
        update_effect_layout_on_mode_change();
        break;

      case Y_PORT_EGO_MODE:
      case Y_PORT_EG1_MODE:
      case Y_PORT_EG2_MODE:
      case Y_PORT_EG3_MODE:
      case Y_PORT_EG4_MODE:
        update_eg_layout_on_mode_change(index);
        break;

      default:
        break;
    }
}

void
on_voice_detent_change( GtkWidget *widget, gpointer data )
{
    int index = GPOINTER_TO_INT(data);
    int value = lrintf(GTK_ADJUSTMENT(widget)->value);

    GDB_MESSAGE(GDB_GUI, " on_voice_detent_change: detent %d changed to %d\n",
            index, value);

    lo_send(osc_host_address, osc_control_path, "if", index, (float)value);
}

void
on_voice_onoff_toggled( GtkWidget *widget, gpointer data )
{
    int index = GPOINTER_TO_INT(data);
    int state = GTK_TOGGLE_BUTTON (widget)->active;

    GDB_MESSAGE(GDB_GUI, " on_voice_onoff_toggled: button %d changed to %s\n",
                index, (state ? "on" : "off"));

    lo_send(osc_host_address, osc_control_path, "if", index, (state ? 1.0f : 0.0f));
}

void
on_voice_combo_change( GtkWidget *widget, gpointer data )
{
    int index = GPOINTER_TO_INT(data);
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    int value = 0;  /* default to 0 if no row is active */

    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
        gtk_tree_model_get(model, &iter, 1, &value, -1);
    /* The combo needs to always store the port's value (for any valid port value),
     * yet the tree model that the combo box is currently using may not be able to
     * represent all values as active rows.  So, we store the value in a qdata
     * associated with the combo box. */
    g_object_set_qdata(G_OBJECT(widget), combo_value_quark, GINT_TO_POINTER(value));

    GDB_MESSAGE(GDB_GUI, " on_voice_combo_change: combo %d changed to %d\n", index, value);

    lo_send(osc_host_address, osc_control_path, "if", index, (float)value);

    check_for_layout_update_on_port_change(index);
}

void
on_voice_element_copy(GtkWidget *widget, gpointer data)
{
    int index = GPOINTER_TO_INT(data),
        offset;

    GDB_MESSAGE(GDB_GUI, " on_voice_element_copy: copying element beginning at port %d\n", index);

    switch (index) {
      /* -PORTS- */
      case Y_PORT_OSC1_MODE:
      case Y_PORT_OSC2_MODE:
      case Y_PORT_OSC3_MODE:
      case Y_PORT_OSC4_MODE:
        offset = index - Y_PORT_OSC1_MODE;
        osc_clipboard.mode          = get_value_from_combo(offset + Y_PORT_OSC1_MODE);
        osc_clipboard.waveform      = get_value_from_combo(offset + Y_PORT_OSC1_WAVEFORM);
        osc_clipboard.pitch         = get_value_from_detent(offset + Y_PORT_OSC1_PITCH);
        osc_clipboard.detune        = get_value_from_knob(offset + Y_PORT_OSC1_DETUNE);
        osc_clipboard.pitch_mod_src = get_value_from_combo(offset + Y_PORT_OSC1_PITCH_MOD_SRC);
        osc_clipboard.pitch_mod_amt = get_value_from_knob(offset + Y_PORT_OSC1_PITCH_MOD_AMT);
        osc_clipboard.mparam1       = get_value_from_knob(offset + Y_PORT_OSC1_MPARAM1);
        osc_clipboard.mparam2       = get_value_from_knob(offset + Y_PORT_OSC1_MPARAM2);
        osc_clipboard.mmod_src      = get_value_from_combo(offset + Y_PORT_OSC1_MMOD_SRC);
        osc_clipboard.mmod_amt      = get_value_from_knob(offset + Y_PORT_OSC1_MMOD_AMT);
        osc_clipboard.amp_mod_src   = get_value_from_combo(offset + Y_PORT_OSC1_AMP_MOD_SRC);
        osc_clipboard.amp_mod_amt   = get_value_from_knob(offset + Y_PORT_OSC1_AMP_MOD_AMT);
        osc_clipboard.level_a       = get_value_from_knob(offset + Y_PORT_OSC1_LEVEL_A);
        osc_clipboard.level_b       = get_value_from_knob(offset + Y_PORT_OSC1_LEVEL_B);
        osc_clipboard_valid = 1;
        break;

      case Y_PORT_VCF1_MODE:
      case Y_PORT_VCF2_MODE:
        offset = index - Y_PORT_VCF1_MODE;
        vcf_clipboard.mode          = get_value_from_combo(offset + Y_PORT_VCF1_MODE);
        vcf_clipboard.source        = get_value_from_combo(offset + Y_PORT_VCF1_SOURCE);
        vcf_clipboard.frequency     = get_value_from_knob(offset + Y_PORT_VCF1_FREQUENCY);
        vcf_clipboard.freq_mod_src  = get_value_from_combo(offset + Y_PORT_VCF1_FREQ_MOD_SRC);
        vcf_clipboard.freq_mod_amt  = get_value_from_knob(offset + Y_PORT_VCF1_FREQ_MOD_AMT);
        vcf_clipboard.qres          = get_value_from_knob(offset + Y_PORT_VCF1_QRES);
        vcf_clipboard.mparam        = get_value_from_knob(offset + Y_PORT_VCF1_MPARAM);
        vcf_clipboard_valid = 1;
        break;

      case Y_PORT_EFFECT_MODE:
        effect_clipboard.effect_mode   = get_value_from_combo(Y_PORT_EFFECT_MODE);
        effect_clipboard.effect_param1 = get_value_from_knob(Y_PORT_EFFECT_PARAM1);
        effect_clipboard.effect_param2 = get_value_from_knob(Y_PORT_EFFECT_PARAM2);
        effect_clipboard.effect_param3 = get_value_from_knob(Y_PORT_EFFECT_PARAM3);
        effect_clipboard.effect_param4 = get_value_from_knob(Y_PORT_EFFECT_PARAM4);
        effect_clipboard.effect_param5 = get_value_from_knob(Y_PORT_EFFECT_PARAM5);
        effect_clipboard.effect_param6 = get_value_from_knob(Y_PORT_EFFECT_PARAM6);
        effect_clipboard.effect_mix    = get_value_from_knob(Y_PORT_EFFECT_MIX);
        effect_clipboard_valid = 1;
        break;

      case Y_PORT_EGO_MODE:
      case Y_PORT_EG1_MODE:
      case Y_PORT_EG2_MODE:
      case Y_PORT_EG3_MODE:
      case Y_PORT_EG4_MODE:
        offset = index - Y_PORT_EGO_MODE;
        eg_clipboard.mode           = get_value_from_combo(offset + Y_PORT_EGO_MODE);
        eg_clipboard.shape1         = get_value_from_combo(offset + Y_PORT_EGO_SHAPE1);
        eg_clipboard.time1          = get_value_from_knob(offset + Y_PORT_EGO_TIME1);
        eg_clipboard.level1         = get_value_from_knob(offset + Y_PORT_EGO_LEVEL1);
        eg_clipboard.shape2         = get_value_from_combo(offset + Y_PORT_EGO_SHAPE2);
        eg_clipboard.time2          = get_value_from_knob(offset + Y_PORT_EGO_TIME2);
        eg_clipboard.level2         = get_value_from_knob(offset + Y_PORT_EGO_LEVEL2);
        eg_clipboard.shape3         = get_value_from_combo(offset + Y_PORT_EGO_SHAPE3);
        eg_clipboard.time3          = get_value_from_knob(offset + Y_PORT_EGO_TIME3);
        eg_clipboard.level3         = get_value_from_knob(offset + Y_PORT_EGO_LEVEL3);
        eg_clipboard.shape4         = get_value_from_combo(offset + Y_PORT_EGO_SHAPE4);
        eg_clipboard.time4          = get_value_from_knob(offset + Y_PORT_EGO_TIME4);
        eg_clipboard.vel_level_sens = get_value_from_knob(offset + Y_PORT_EGO_VEL_LEVEL_SENS);
        eg_clipboard.vel_time_scale = get_value_from_knob(offset + Y_PORT_EGO_VEL_TIME_SCALE);
        eg_clipboard.kbd_time_scale = get_value_from_knob(offset + Y_PORT_EGO_KBD_TIME_SCALE);
        eg_clipboard.amp_mod_src    = get_value_from_combo(offset + Y_PORT_EGO_AMP_MOD_SRC);
        eg_clipboard.amp_mod_amt    = get_value_from_knob(offset + Y_PORT_EGO_AMP_MOD_AMT);
        eg_clipboard_valid = 1;
        break;

      default:
        break;
    }
}

void
on_voice_element_paste(GtkWidget *widget, gpointer data)
{
    int index = GPOINTER_TO_INT(data),
        offset;

    GDB_MESSAGE(GDB_GUI, " on_voice_element_paste: paste requested for element beginning at port %d\n", index);

    switch (index) {
      /* -PORTS- */
      case Y_PORT_OSC1_MODE:
      case Y_PORT_OSC2_MODE:
      case Y_PORT_OSC3_MODE:
      case Y_PORT_OSC4_MODE:
        if (!osc_clipboard_valid)
            return;
        offset = index - Y_PORT_OSC1_MODE;
        update_voice_widget(offset + Y_PORT_OSC1_MODE,          (float)osc_clipboard.mode,        TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_WAVEFORM,      (float)osc_clipboard.waveform,    TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_PITCH,         (float)osc_clipboard.pitch,       TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_DETUNE,        osc_clipboard.detune,             TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_PITCH_MOD_SRC, (float)osc_clipboard.pitch_mod_src, TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_PITCH_MOD_AMT, osc_clipboard.pitch_mod_amt,      TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_MPARAM1,       osc_clipboard.mparam1,            TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_MPARAM2,       osc_clipboard.mparam2,            TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_MMOD_SRC,      (float)osc_clipboard.mmod_src,    TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_MMOD_AMT,      osc_clipboard.mmod_amt,           TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_AMP_MOD_SRC,   (float)osc_clipboard.amp_mod_src, TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_AMP_MOD_AMT,   osc_clipboard.amp_mod_amt,        TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_LEVEL_A,       osc_clipboard.level_a,            TRUE);
        update_voice_widget(offset + Y_PORT_OSC1_LEVEL_B,       osc_clipboard.level_b,            TRUE);
        break;

      case Y_PORT_VCF1_MODE:
      case Y_PORT_VCF2_MODE:
        if (!vcf_clipboard_valid)
            return;
        offset = index - Y_PORT_VCF1_MODE;
        update_voice_widget(offset + Y_PORT_VCF1_MODE,         (float)vcf_clipboard.mode,         TRUE);
        update_voice_widget(offset + Y_PORT_VCF1_SOURCE,       (float)vcf_clipboard.source,       TRUE);
        update_voice_widget(offset + Y_PORT_VCF1_FREQUENCY,    vcf_clipboard.frequency,           TRUE);
        update_voice_widget(offset + Y_PORT_VCF1_FREQ_MOD_SRC, (float)vcf_clipboard.freq_mod_src, TRUE);
        update_voice_widget(offset + Y_PORT_VCF1_FREQ_MOD_AMT, vcf_clipboard.freq_mod_amt,        TRUE);
        update_voice_widget(offset + Y_PORT_VCF1_QRES,         vcf_clipboard.qres,                TRUE);
        update_voice_widget(offset + Y_PORT_VCF1_MPARAM,       vcf_clipboard.mparam,              TRUE);
        break;

      case Y_PORT_EFFECT_MODE:
        if (!effect_clipboard_valid)
            return;
        update_voice_widget(Y_PORT_EFFECT_MODE,   (float)effect_clipboard.effect_mode, TRUE);
        update_voice_widget(Y_PORT_EFFECT_PARAM1, effect_clipboard.effect_param1,      TRUE);
        update_voice_widget(Y_PORT_EFFECT_PARAM2, effect_clipboard.effect_param2,      TRUE);
        update_voice_widget(Y_PORT_EFFECT_PARAM3, effect_clipboard.effect_param3,      TRUE);
        update_voice_widget(Y_PORT_EFFECT_PARAM4, effect_clipboard.effect_param4,      TRUE);
        update_voice_widget(Y_PORT_EFFECT_PARAM5, effect_clipboard.effect_param5,      TRUE);
        update_voice_widget(Y_PORT_EFFECT_PARAM6, effect_clipboard.effect_param6,      TRUE);
        update_voice_widget(Y_PORT_EFFECT_MIX,    effect_clipboard.effect_mix,         TRUE);
        break;

      case Y_PORT_EGO_MODE:
      case Y_PORT_EG1_MODE:
      case Y_PORT_EG2_MODE:
      case Y_PORT_EG3_MODE:
      case Y_PORT_EG4_MODE:
        if (!eg_clipboard_valid)
            return;
        offset = index - Y_PORT_EGO_MODE;
        update_voice_widget(offset + Y_PORT_EGO_MODE,           (float)eg_clipboard.mode,        TRUE);
        update_voice_widget(offset + Y_PORT_EGO_SHAPE1,         (float)eg_clipboard.shape1,      TRUE);
        update_voice_widget(offset + Y_PORT_EGO_TIME1,          eg_clipboard.time1,              TRUE);
        update_voice_widget(offset + Y_PORT_EGO_LEVEL1,         eg_clipboard.level1,             TRUE);
        update_voice_widget(offset + Y_PORT_EGO_SHAPE2,         (float)eg_clipboard.shape2,      TRUE);
        update_voice_widget(offset + Y_PORT_EGO_TIME2,          eg_clipboard.time2,              TRUE);
        update_voice_widget(offset + Y_PORT_EGO_LEVEL2,         eg_clipboard.level2,             TRUE);
        update_voice_widget(offset + Y_PORT_EGO_SHAPE3,         (float)eg_clipboard.shape3,      TRUE);
        update_voice_widget(offset + Y_PORT_EGO_TIME3,          eg_clipboard.time3,              TRUE);
        update_voice_widget(offset + Y_PORT_EGO_LEVEL3,         eg_clipboard.level3,             TRUE);
        update_voice_widget(offset + Y_PORT_EGO_SHAPE4,         (float)eg_clipboard.shape4,      TRUE);
        update_voice_widget(offset + Y_PORT_EGO_TIME4,          eg_clipboard.time4,              TRUE);
        update_voice_widget(offset + Y_PORT_EGO_VEL_LEVEL_SENS, eg_clipboard.vel_level_sens,     TRUE);
        update_voice_widget(offset + Y_PORT_EGO_VEL_TIME_SCALE, eg_clipboard.vel_time_scale,     TRUE);
        update_voice_widget(offset + Y_PORT_EGO_KBD_TIME_SCALE, eg_clipboard.kbd_time_scale,     TRUE);
        update_voice_widget(offset + Y_PORT_EGO_AMP_MOD_SRC,    (float)eg_clipboard.amp_mod_src, TRUE);
        update_voice_widget(offset + Y_PORT_EGO_AMP_MOD_AMT,    eg_clipboard.amp_mod_amt,        TRUE);
        break;

      default:
        break;
    }
}

void
on_test_note_slider_change(GtkWidget *widget, gpointer data)
{
    unsigned char value = lrintf(GTK_ADJUSTMENT(widget)->value);

    /* synchronize main and edit sliders */
    switch (GPOINTER_TO_INT(data)) {
      case 0: /* main key */

        test_note_noteon_key = value;

        GTK_ADJUSTMENT(edit_test_note_key_adj)->value = test_note_noteon_key;
        /* emit "value_changed" to get the widget to redraw itself, but don't call 
         * this callback again */
        g_signal_handlers_block_by_func(G_OBJECT(edit_test_note_key_adj),
                                        on_test_note_slider_change, GINT_TO_POINTER(2));
        gtk_signal_emit_by_name (GTK_OBJECT (edit_test_note_key_adj), "value_changed");
        g_signal_handlers_unblock_by_func(G_OBJECT(edit_test_note_key_adj),
                                          on_test_note_slider_change, GINT_TO_POINTER(2));

        GDB_MESSAGE(GDB_GUI, " on_test_note_slider_change: new test note key %d from main window\n", test_note_noteon_key);
        break;

      case 1: /* main velocity */

        test_note_velocity = value;

        GTK_ADJUSTMENT(edit_test_note_velocity_adj)->value = test_note_velocity;
        /* emit "value_changed" to get the widget to redraw itself, but don't call 
         * this callback again */
        g_signal_handlers_block_by_func(G_OBJECT(edit_test_note_velocity_adj),
                                        on_test_note_slider_change, GINT_TO_POINTER(3));
        gtk_signal_emit_by_name (GTK_OBJECT (edit_test_note_velocity_adj), "value_changed");
        g_signal_handlers_unblock_by_func(G_OBJECT(edit_test_note_velocity_adj),
                                          on_test_note_slider_change, GINT_TO_POINTER(3));

        GDB_MESSAGE(GDB_GUI, " on_test_note_slider_change: new test note velocity %d from main window\n", test_note_velocity);
        break;

      case 2: /* edit key */

        test_note_noteon_key = value;

        GTK_ADJUSTMENT(main_test_note_key_adj)->value = test_note_noteon_key;
        /* emit "value_changed" to get the widget to redraw itself, but don't call 
         * this callback again */
        g_signal_handlers_block_by_func(G_OBJECT(main_test_note_key_adj),
                                        on_test_note_slider_change, GINT_TO_POINTER(0));
        gtk_signal_emit_by_name (GTK_OBJECT (main_test_note_key_adj), "value_changed");
        g_signal_handlers_unblock_by_func(G_OBJECT(main_test_note_key_adj),
                                          on_test_note_slider_change, GINT_TO_POINTER(0));

        GDB_MESSAGE(GDB_GUI, " on_test_note_slider_change: new test note key %d from edit window\n", test_note_noteon_key);
        break;

      case 3: /* edit velocity */

        test_note_velocity = value;

        GTK_ADJUSTMENT(main_test_note_velocity_adj)->value = test_note_velocity;
        /* emit "value_changed" to get the widget to redraw itself, but don't call 
         * this callback again */
        g_signal_handlers_block_by_func(G_OBJECT(main_test_note_velocity_adj),
                                        on_test_note_slider_change, GINT_TO_POINTER(1));
        gtk_signal_emit_by_name (GTK_OBJECT (main_test_note_velocity_adj), "value_changed");
        g_signal_handlers_unblock_by_func(G_OBJECT(main_test_note_velocity_adj),
                                          on_test_note_slider_change, GINT_TO_POINTER(1));

        GDB_MESSAGE(GDB_GUI, " on_test_note_slider_change: new test note velocity %d from edit window\n", test_note_velocity);
        break;
    }
}

void
on_test_note_mode_toggled(GtkWidget *widget, gpointer data)
{
    int state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    if (state) {
        gtk_widget_hide(edit_test_note_button);
        gtk_widget_show(edit_test_note_toggle);
    } else {
        gtk_widget_show(edit_test_note_button);
        gtk_widget_hide(edit_test_note_toggle);
    }
}

static void
send_midi(unsigned char b0, unsigned char b1, unsigned char b2)
{
    unsigned char midi[4];

    midi[0] = 0;
    midi[1] = b0;
    midi[2] = b1;
    midi[3] = b2;
    lo_send(osc_host_address, osc_midi_path, "m", midi);
}

void
release_test_note(void)
{
    if (test_note_noteoff_key >= 0) {
        send_midi(0x80, test_note_noteoff_key, 0x40);
        test_note_noteoff_key = -1;
    }
}

void
on_test_note_button_press(GtkWidget *widget, gpointer data)
{
    /* here we just set the state of the test note toggle button, which may
     * cause a call to on_test_note_toggle_toggled() below, which will send
     * the actual MIDI message. */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edit_test_note_toggle),
                                 GPOINTER_TO_INT(data) != 0);
}

void
on_test_note_toggle_toggled(GtkWidget *widget, gpointer data)
{
    int state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(edit_test_note_toggle));

    GDB_MESSAGE(GDB_GUI, " on_test_note_toggle_toggled: state is now %s\n",
                state ? "active" : "inactive");

    if (state) {  /* button pressed */

        if (test_note_noteoff_key < 0) {
            send_midi(0x90, test_note_noteon_key, test_note_velocity);
            test_note_noteoff_key = test_note_noteon_key;
        }

    } else { /* button released */

        release_test_note();

    }
}

void
on_edit_action_button_press(GtkWidget *widget, gpointer data)
{
    int i;

    GDB_MESSAGE(GDB_GUI, " on_edit_action_button_press: '%s' clicked\n", (char *)data);

    if (!strcmp(data, "save")) {

        /* find the last non-init-voice patch, and set the save position to the
         * following patch */
        for (i = patch_count;
             i > 0 && gui_data_patch_compare(&patches[i - 1], &y_init_voice);
             i--);
        (GTK_ADJUSTMENT(edit_save_position_spin_adj))->value = (float)i;
        (GTK_ADJUSTMENT(edit_save_position_spin_adj))->upper = (float)patch_count;
        gtk_signal_emit_by_name (GTK_OBJECT (edit_save_position_spin_adj), "value_changed");
    
        gtk_widget_show(edit_save_position_window);

    }
}

void
on_edit_save_position_ok( GtkWidget *widget, gpointer data )
{
    int position = lrintf(GTK_ADJUSTMENT(edit_save_position_spin_adj)->value);
    GtkTreeIter iter;

    gtk_widget_hide(edit_save_position_window);

    GDB_MESSAGE(GDB_GUI, " on_edit_save_position_ok: position %d\n", position);

    /* set the patch to match all the edit widgets */
    gui_data_check_patches_allocation(position);
    update_patch_from_voice_widgets(&patches[position]);
    patches_dirty = 1;
    if (position == patch_count) patch_count++;
    /* update view */
    find_program_in_list_store(&iter, position);
    gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(patches_list))), &iter,
                       PATCHES_LIST_COL_CATEGORY, "y",
                       PATCHES_LIST_COL_NAME, patches[position].name,
                       -1);

    /* our patch bank is now dirty, so we need to save a temporary copy
     * for the plugin to load */
    if (gui_data_save_dirty_patches_to_tmp()) {
        lo_send(osc_host_address, osc_configure_path, "ss", "load",
                patches_tmp_filename);
        last_configure_load_was_from_tmp = 1;
    } else {
        display_notice("Patch Edit Save Changes error:", "couldn't save temporary patch bank");
    }
}

void
on_edit_save_position_cancel( GtkWidget *widget, gpointer data )
{
    GDB_MESSAGE(GDB_GUI, " on_edit_save_position_cancel called\n");
    gtk_widget_hide(edit_save_position_window);
}

void
on_edit_close(GtkWidget *widget, gpointer data)
{
    gtk_widget_hide(edit_window);
}

void
on_tuning_change(GtkWidget *widget, gpointer data)
{
    float value = GTK_ADJUSTMENT(widget)->value;

    GDB_MESSAGE(GDB_GUI, " on_tuning_change: tuning set to %10.6f\n", value);

    lo_send(osc_host_address, osc_control_path, "if", Y_PORT_TUNING, value);
}

void
on_polyphony_change(GtkWidget *widget, gpointer data)
{
    int polyphony = lrintf(GTK_ADJUSTMENT(widget)->value);
    char buffer[4];
    
    GDB_MESSAGE(GDB_GUI, " on_polyphony_change: polyphony set to %d\n", polyphony);

    snprintf(buffer, 4, "%d", polyphony);
    lo_send(osc_host_address, osc_configure_path, "ss", "polyphony", buffer);
}

void
on_mono_mode_activate(GtkWidget *widget, gpointer data)
{
    char *mode = data;

    GDB_MESSAGE(GDB_GUI, " on_mono_mode_activate: monophonic mode '%s' selected\n", mode);

    lo_send(osc_host_address, osc_configure_path, "ss", "monophonic", mode);
}

void
on_glide_mode_activate(GtkWidget *widget, gpointer data)
{
    char *mode = data;

    GDB_MESSAGE(GDB_GUI, " on_glide_mode_activate: glide mode '%s' selected\n", mode);

    lo_send(osc_host_address, osc_configure_path, "ss", "glide", mode);
}

void
on_program_cancel_toggled(GtkWidget *widget, gpointer data)
{
    const char *state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) ? "on" : "off";

    GDB_MESSAGE(GDB_GUI, " on_program_cancel_toggled: program cancel now '%s'\n", state);

    lo_send(osc_host_address, osc_configure_path, "ss", "program_cancel", state);
}

void
display_notice(char *message1, char *message2)
{
    gtk_label_set_text (GTK_LABEL (notice_label_1), message1);
    gtk_label_set_text (GTK_LABEL (notice_label_2), message2);
    gtk_widget_show(notice_window);
}

void
on_notice_dismiss( GtkWidget *widget, gpointer data )
{
    gtk_widget_hide(notice_window);
}

int
combo_get_value(int port)
{
    GtkComboBox *combo = GTK_COMBO_BOX(voice_widgets[port].widget);

    return GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(combo), combo_value_quark));
}

void
combo_set_active_row(int port, int value)
{
    GtkComboBox *combo = GTK_COMBO_BOX(voice_widgets[port].widget);
    GtkTreeModel *model = gtk_combo_box_get_model(combo);
    GPtrArray *id_to_path = g_object_get_qdata(G_OBJECT(model), combomodel_id_to_path_quark);
    GtkTreeIter iter;

    g_signal_handlers_block_by_func(combo, on_voice_combo_change, GINT_TO_POINTER(port));
    if (value >= 0 && value < id_to_path->len &&
        gtk_tree_model_get_iter_from_string(model, &iter, g_ptr_array_index(id_to_path, value))) {
        gtk_combo_box_set_active_iter(combo, &iter);
    } else {
        gtk_combo_box_set_active(combo, -1);
    }
    g_signal_handlers_unblock_by_func(combo, on_voice_combo_change, GINT_TO_POINTER(port));
}

void
combo_set_value(int port, int value)
{
    GtkComboBox *combo = GTK_COMBO_BOX(voice_widgets[port].widget);

    /* Store the value: associated it with the widget as a qdata. See
     * on_voice_combo_change() for further description. */
    g_object_set_qdata(G_OBJECT(combo), combo_value_quark, GINT_TO_POINTER(value));

    combo_set_active_row(port, value);
}

void
combo_set_combomodel_type(int port, int combomodel_type)
{
    GtkComboBox *combo = GTK_COMBO_BOX(voice_widgets[port].widget);
    GtkTreeModel *model = GTK_TREE_MODEL(combomodel[combomodel_type]);
    int value = GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(combo), combo_value_quark));

    gtk_combo_box_set_model(GTK_COMBO_BOX(combo), model);
    /* we've left the combobox without an active row, set set it now */
    combo_set_active_row(port, value);
}

char *osc_noise_mparam1_top_labels[] = {  /* noise wave-specific labels */
    NULL,
    NULL,
    "Cutoff Freq",
    "Center Freq"
};

char *osc_minblep_mparam2_top_labels[] = {   /* minBLEP wave-specific labels */
    NULL,
    NULL,
    "Pulsewidth",
    "Slope",
    "Pulsewidth",
    "Toothwidth"
};

char *osc_noise_mparam2_top_labels[] = {  /* noise wave-specific labels */
    NULL,
    NULL,
    "Resonance",
    "Resonance"
};

char *osc_minblep_mmod_src_top_labels[] = {  /* minBLEP wave-specific labels */
    NULL,
    NULL,
    "PW Mod Source",
    "Slope Mod Source",
    "PW Mod Source",
    "TW Mod Source"
};

char *osc_minblep_mmod_amt_top_labels[] = {  /* minBLEP wave-specific labels */
    NULL,
    NULL,
    "PW Mod Amount",
    "Slope Mod Amount",
    "PW Mod Amount",
    "TW Mod Amount"
};

static void
update_top_label(int port, char *text)
{
    int sensitive;

    if (text) {
        gtk_label_set_text (GTK_LABEL (voice_widgets[port].top_label), text);
        sensitive = TRUE;
    } else {
        gtk_label_set_text (GTK_LABEL (voice_widgets[port].top_label), " ");
        sensitive = FALSE;
    }
    gtk_widget_set_sensitive (voice_widgets[port].widget, sensitive);
}

#define MINBLEP_WAVES_COUNT 6  /* -FIX- why the hell is this here? */
void
update_osc_layout_on_mode_change(int mode_port)
{
    int osc_mode = get_value_from_combo(mode_port);
    int wave_port = mode_port - Y_PORT_OSC1_MODE + Y_PORT_OSC1_WAVEFORM;
    int osc_wave = get_value_from_combo(wave_port);
    int port;
    char *text;

    if (osc_mode < 0 || osc_mode > Y_OSCILLATOR_MODE_COUNT)
        osc_mode = 0;
    if (osc_wave < 0 || osc_wave >= wavetables_count)
        osc_wave = 0;

    if (osc_mode != voice_widgets[mode_port].last_mode ||
        ((osc_mode == Y_OSCILLATOR_MODE_MINBLEP ||
          osc_mode == Y_OSCILLATOR_MODE_NOISE ||
          osc_mode == Y_OSCILLATOR_MODE_PD) &&
         osc_wave != voice_widgets[wave_port].last_mode)) { /* minBLEP and noise have wave-dependent layout */

        int combomodel_type;

        voice_widgets[mode_port].last_mode = osc_mode;
        voice_widgets[wave_port].last_mode = osc_wave;

        if (osc_mode == Y_OSCILLATOR_MODE_MINBLEP)
            combomodel_type = Y_COMBOMODEL_TYPE_MINBLEP_WAVEFORM;
        else if (osc_mode == Y_OSCILLATOR_MODE_NOISE)
            combomodel_type = Y_COMBOMODEL_TYPE_NOISE_WAVEFORM;
        else if (osc_mode == Y_OSCILLATOR_MODE_PD)
            combomodel_type = Y_COMBOMODEL_TYPE_PD_WAVEFORM;
        else
            combomodel_type = Y_COMBOMODEL_TYPE_WAVETABLE;

        combo_set_combomodel_type(wave_port, combomodel_type);

        port = mode_port - Y_PORT_OSC1_MODE + Y_PORT_OSC1_MPARAM1; /* MParam1 */
        if (osc_mode == Y_OSCILLATOR_MODE_NOISE && osc_wave < 4)  /* noise */
            update_top_label(port, osc_noise_mparam1_top_labels[osc_wave]);
        else if (osc_mode == Y_OSCILLATOR_MODE_PD && osc_wave < 12)  /* phase distortion */
            update_top_label(port, NULL);  /* no mod balance on single waveforms */
        else
            update_top_label(port, y_osc_modes[osc_mode].mparam1_top_label);
        text = y_osc_modes[osc_mode].mparam1_left_label;
        gtk_label_set_text (GTK_LABEL (voice_widgets[port].label1), text);
        text = y_osc_modes[osc_mode].mparam1_right_label;
        gtk_label_set_text (GTK_LABEL (voice_widgets[port].label2), text);

        port = mode_port - Y_PORT_OSC1_MODE + Y_PORT_OSC1_MPARAM2; /* MParam2 */
        if (osc_mode == Y_OSCILLATOR_MODE_MINBLEP && osc_wave < MINBLEP_WAVES_COUNT)  /* minBLEP */
            update_top_label(port, osc_minblep_mparam2_top_labels[osc_wave]);
        else if (osc_mode == Y_OSCILLATOR_MODE_NOISE && osc_wave < 4)  /* noise */
            update_top_label(port, osc_noise_mparam2_top_labels[osc_wave]);
        else
            update_top_label(port, y_osc_modes[osc_mode].mparam2_top_label);
        text = y_osc_modes[osc_mode].mparam2_left_label;
        gtk_label_set_text (GTK_LABEL (voice_widgets[port].label1), text);
        text = y_osc_modes[osc_mode].mparam2_right_label;
        gtk_label_set_text (GTK_LABEL (voice_widgets[port].label2), text);

        port = mode_port - Y_PORT_OSC1_MODE + Y_PORT_OSC1_MMOD_SRC; /* MMod Src */
        if (osc_mode == Y_OSCILLATOR_MODE_AGRAN)
            combomodel_type = Y_COMBOMODEL_TYPE_GRAIN_ENV;
        else if (osc_mode == Y_OSCILLATOR_MODE_PADSYNTH)
            combomodel_type = Y_COMBOMODEL_TYPE_PADSYNTH_MODE;
        else
            combomodel_type = Y_COMBOMODEL_TYPE_MOD_SRC;
        combo_set_combomodel_type(port, combomodel_type);
        if (osc_mode == Y_OSCILLATOR_MODE_MINBLEP && osc_wave < MINBLEP_WAVES_COUNT)  /* minBLEP */
            update_top_label(port, osc_minblep_mmod_src_top_labels[osc_wave]);
        else
            update_top_label(port, y_osc_modes[osc_mode].mmod_src_top_label);

        port = mode_port - Y_PORT_OSC1_MODE + Y_PORT_OSC1_MMOD_AMT; /* MMod Amt */
        if (osc_mode == Y_OSCILLATOR_MODE_MINBLEP && osc_wave < MINBLEP_WAVES_COUNT)  /* minBLEP */
            update_top_label(port, osc_minblep_mmod_amt_top_labels[osc_wave]);
        else
            update_top_label(port, y_osc_modes[osc_mode].mmod_amt_top_label);
    }
}

void
update_vcf_layout_on_mode_change(int mode_port)
{
    int vcf_mode = get_value_from_combo(mode_port);
    int port;

    if (vcf_mode < 0 || vcf_mode > Y_FILTER_MODE_COUNT)
        vcf_mode = 0;

    if (vcf_mode != voice_widgets[mode_port].last_mode) {

        voice_widgets[mode_port].last_mode = vcf_mode;

        port = mode_port - Y_PORT_VCF1_MODE + Y_PORT_VCF1_QRES;   /* QRes */
        update_top_label(port, y_vcf_modes[vcf_mode].qres_top_label);
        port = mode_port - Y_PORT_VCF1_MODE + Y_PORT_VCF1_MPARAM; /* MParam */
        update_top_label(port, y_vcf_modes[vcf_mode].mparam_top_label);
        // text = vcf_mparam_left_labels[vcf_mode];
        // gtk_label_set_text (GTK_LABEL (voice_widgets[port].label1), text);
        // text = vcf_mparam_right_labels[vcf_mode];
        // gtk_label_set_text (GTK_LABEL (voice_widgets[port].label2), text);
    }
}

void
update_effect_layout_on_mode_change(void)
{
    const int mode_port = Y_PORT_EFFECT_MODE;
    int mode = get_value_from_combo(mode_port);

    if (mode < 0 || mode > Y_EFFECT_MODE_COUNT)
        mode = 0;

    if (mode != voice_widgets[mode_port].last_mode) {

        voice_widgets[mode_port].last_mode = mode;

        update_top_label(Y_PORT_EFFECT_PARAM1, y_effect_modes[mode].mparam1_top_label);
        // text = effect_param1_left_labels[mode];
        // gtk_label_set_text (GTK_LABEL (voice_widgets[port].label1), text);
        // text = effect_param1_right_labels[mode];
        // gtk_label_set_text (GTK_LABEL (voice_widgets[port].label2), text);

        update_top_label(Y_PORT_EFFECT_PARAM2, y_effect_modes[mode].mparam2_top_label);
        update_top_label(Y_PORT_EFFECT_PARAM3, y_effect_modes[mode].mparam3_top_label);
        update_top_label(Y_PORT_EFFECT_PARAM4, y_effect_modes[mode].mparam4_top_label);
        update_top_label(Y_PORT_EFFECT_PARAM5, y_effect_modes[mode].mparam5_top_label);
        update_top_label(Y_PORT_EFFECT_PARAM6, y_effect_modes[mode].mparam6_top_label);
    }
}

char *eg_shape1_top_labels[] = {
    NULL,
    "Attack Shape",
    "Attack 1 Shape",
    "Attack 1 Shape",
    "Attack Shape",
    "Shape 1"
};

char *eg_time1_top_labels[] = {
    NULL,
    "Attack Time",
    "Attack 1 Time",
    "Attack 1 Time",
    "Attack Time",
    "Time 1"
};

char *eg_level1_top_labels[] = {
    NULL,
    NULL,
    "Attack 1 Level",
    "Attack Level",
    "Sustain Level",
    "Level 1"
};

char *eg_shape2_top_labels[] = {
    NULL,
    NULL,
    "Attack 2 Shape",
    "Attack 2 Shape",
    "Release 1 Shape",
    "Shape 2"
};

char *eg_time2_top_labels[] = {
    NULL,
    NULL,
    "Attack 2 Time",
    "Attack 2 Time",
    "Release 1 Time",
    "Time 2"
};

char *eg_level2_top_labels[] = {
    NULL,
    NULL,
    "Attack 2 Level",
    "Sustain Level",
    "Release 1 Level",
    "Level 2"
};

char *eg_shape3_top_labels[] = {
    NULL,
    "Decay Shape",
    "Attack 3 Shape",
    "Release 1 Shape",
    "Release 2 Shape",
    "Shape 3"
};

char *eg_time3_top_labels[] = {
    NULL,
    "Decay Time",
    "Attack 3 Time",
    "Release 1 Time",
    "Release 2 Time",
    "Time 3"
};

char *eg_level3_top_labels[] = {
    NULL,
    "Sustain Level",
    "Sustain Level",
    "Release Level",
    "Release 2 Level",
    "Level 3"
};

char *eg_shape4_top_labels[] = {
    NULL,
    "Release Shape",
    "Release Shape",
    "Release 2 Shape",
    "Release 3 Shape",
    "Shape 4"
};

char *eg_time4_top_labels[] = {
    NULL,
    "Release Time",
    "Release Time",
    "Release 2 Time",
    "Release 3 Time",
    "Time 4"
};

void
update_eg_layout_on_mode_change(int mode_port)
{
    int mode = get_value_from_combo(mode_port);
    int port;

    if (mode < 0 || mode > 11)
        mode = 0;

    if (mode != voice_widgets[mode_port].last_mode) {

        voice_widgets[mode_port].last_mode = mode;

        port = mode_port - Y_PORT_EGO_MODE + Y_PORT_EGO_SHAPE1;
        update_top_label(port, eg_shape1_top_labels[mode]);
        port = mode_port - Y_PORT_EGO_MODE + Y_PORT_EGO_TIME1;
        update_top_label(port, eg_time1_top_labels[mode]);
        port = mode_port - Y_PORT_EGO_MODE + Y_PORT_EGO_LEVEL1;
        update_top_label(port, eg_level1_top_labels[mode]);
        port = mode_port - Y_PORT_EGO_MODE + Y_PORT_EGO_SHAPE2;
        update_top_label(port, eg_shape2_top_labels[mode]);
        port = mode_port - Y_PORT_EGO_MODE + Y_PORT_EGO_TIME2;
        update_top_label(port, eg_time2_top_labels[mode]);
        port = mode_port - Y_PORT_EGO_MODE + Y_PORT_EGO_LEVEL2;
        update_top_label(port, eg_level2_top_labels[mode]);
        port = mode_port - Y_PORT_EGO_MODE + Y_PORT_EGO_SHAPE3;
        update_top_label(port, eg_shape3_top_labels[mode]);
        port = mode_port - Y_PORT_EGO_MODE + Y_PORT_EGO_TIME3;
        update_top_label(port, eg_time3_top_labels[mode]);
        port = mode_port - Y_PORT_EGO_MODE + Y_PORT_EGO_LEVEL3;
        update_top_label(port, eg_level3_top_labels[mode]);
        port = mode_port - Y_PORT_EGO_MODE + Y_PORT_EGO_SHAPE4;
        update_top_label(port, eg_shape4_top_labels[mode]);
        port = mode_port - Y_PORT_EGO_MODE + Y_PORT_EGO_TIME4;
        update_top_label(port, eg_time4_top_labels[mode]);
    }
}

void
update_voice_widget(int port, float value, int send_OSC)
{
    struct y_port_descriptor *ypd;
    GtkAdjustment *adj;
    GtkWidget *widget;
    float cval;
    int dval;

    if (port <= Y_PORT_OUTPUT_RIGHT || port >= Y_PORTS_COUNT) {
        return;
    }

    ypd = &y_port_description[port];
    if (value < ypd->lower_bound)
        value = ypd->lower_bound;
    else if (value > ypd->upper_bound)
        value = ypd->upper_bound;

    if (port == Y_PORT_TUNING) {  /* handle tuning specially, since it's not stored in patch */
        (GTK_ADJUSTMENT(tuning_adj))->value = value;
        /* emit "value_changed" to get the widget to redraw itself, but don't call 
         * on_tuning_change(): */
        g_signal_handlers_block_by_func(tuning_adj, on_tuning_change, NULL);
        gtk_signal_emit_by_name (tuning_adj, "value_changed");
        g_signal_handlers_unblock_by_func(tuning_adj, on_tuning_change, NULL);
        /* if requested, send update to DSSI host */
        if (send_OSC)
            lo_send(osc_host_address, osc_control_path, "if", port, value);
        return;
    }

    switch (ypd->type) {

      case Y_PORT_TYPE_BOOLEAN:
        dval = (value > 0.0001f ? 1 : 0);
        GDB_MESSAGE(GDB_GUI, " update_voice_widget: change of '%s' to %f => %d\n", ypd->name, value, dval);
        widget = (GtkWidget *)voice_widgets[port].widget;
        /* update the widget, but don't call on_voice_onoff_toggled(): */
        g_signal_handlers_block_by_func(widget, on_voice_onoff_toggled, GINT_TO_POINTER(port));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dval);
        g_signal_handlers_unblock_by_func(widget, on_voice_onoff_toggled, GINT_TO_POINTER(port));
        break;

      case Y_PORT_TYPE_INTEGER:
        dval = lrintf(value);
        GDB_MESSAGE(GDB_GUI, " update_voice_widget: change of '%s' to %f => %d\n", ypd->name, value, dval);
        adj = (GtkAdjustment *)voice_widgets[port].adjustment;
        adj->value = (float)dval;
        /* emit "value_changed" to get the widget to redraw itself, but don't call 
         * on_voice_detent_change(): */
        g_signal_handlers_block_by_func(adj, on_voice_detent_change, GINT_TO_POINTER(port));
        gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
        g_signal_handlers_unblock_by_func(adj, on_voice_detent_change, GINT_TO_POINTER(port));
        break;

      case Y_PORT_TYPE_LINEAR:
      case Y_PORT_TYPE_PAN:
        GDB_MESSAGE(GDB_GUI, " update_voice_widget: change of '%s' to %f\n", ypd->name, value);
        adj = (GtkAdjustment *)voice_widgets[port].adjustment;
        adj->value = value;
        /* emit "value_changed" to get the widget to redraw itself, but don't call 
         * on_voice_knob_change(): */
        g_signal_handlers_block_by_func(adj, on_voice_knob_change, GINT_TO_POINTER(port));
        gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
        g_signal_handlers_unblock_by_func(adj, on_voice_knob_change, GINT_TO_POINTER(port));
        break;

      case Y_PORT_TYPE_LOGARITHMIC:
        cval = logf(ypd->lower_bound);
        cval = (logf(value) - cval) / (logf(ypd->upper_bound) - cval);
        if (cval < 1.0e-6f)
            cval = 0.0f;
        else if (cval > 1.0f - 1.0e-6f)
            cval = 1.0f;
        GDB_MESSAGE(GDB_GUI, " update_voice_widget: change of '%s' to %f => %f\n", ypd->name, value, cval);
        adj = (GtkAdjustment *)voice_widgets[port].adjustment;
        adj->value = cval;
        /* emit "value_changed" to get the widget to redraw itself, but don't call 
         * on_voice_knob_change(): */
        g_signal_handlers_block_by_func(adj, on_voice_knob_change, GINT_TO_POINTER(port));
        gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
        g_signal_handlers_unblock_by_func(adj, on_voice_knob_change, GINT_TO_POINTER(port));
        break;

      case Y_PORT_TYPE_LOGSCALED:
        cval = (value - ypd->lower_bound) / (ypd->upper_bound - ypd->lower_bound);
        cval = powf(cval, 1.0f / ypd->scale);
        if (cval < 1.0e-6f)
            cval = 0.0f;
        else if (cval > 1.0f - 1.0e-6f)
            cval = 1.0f;
        GDB_MESSAGE(GDB_GUI, " update_voice_widget: change of '%s' to %f => %f\n", ypd->name, value, cval);
        adj = (GtkAdjustment *)voice_widgets[port].adjustment;
        adj->value = cval;
        /* emit "value_changed" to get the widget to redraw itself, but don't call 
         * on_voice_knob_change(): */
        g_signal_handlers_block_by_func(adj, on_voice_knob_change, GINT_TO_POINTER(port));
        gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
        g_signal_handlers_unblock_by_func(adj, on_voice_knob_change, GINT_TO_POINTER(port));
        break;

      case Y_PORT_TYPE_BPLOGSCALED:
        if (value > 0.0f) {
            cval = value / ypd->upper_bound;
            cval = powf(cval, 1.0f / ypd->scale);
            cval = 0.5f + 0.5f * cval;
        } else if (value < 0.0f) {
            cval = value / ypd->lower_bound;
            cval = powf(cval, 1.0f / ypd->scale);
            cval = 0.5f - 0.5f * cval;
        } else /* 0 */
            cval = 0.5f;
        if (cval < 1.0e-6f)
            cval = 0.0f;
        else if (cval > 1.0f - 1.0e-6f)
            cval = 1.0f;
        GDB_MESSAGE(GDB_GUI, " update_voice_widget: change of '%s' to %f => %f\n", ypd->name, value, cval);
        adj = (GtkAdjustment *)voice_widgets[port].adjustment;
        adj->value = cval;
        /* emit "value_changed" to get the widget to redraw itself, but don't call 
         * on_voice_knob_change(): */
        g_signal_handlers_block_by_func(adj, on_voice_knob_change, GINT_TO_POINTER(port));
        gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
        g_signal_handlers_unblock_by_func(adj, on_voice_knob_change, GINT_TO_POINTER(port));
        break;

      case Y_PORT_TYPE_COMBO:
        dval = lrintf(value);
        GDB_MESSAGE(GDB_GUI, " update_voice_widget: change of combo '%s' to %f => %d\n", ypd->name, value, dval);
        widget = voice_widgets[port].widget;
        combo_set_value(port, dval);  /* does not cause call to on_voice_combo_change() */
        check_for_layout_update_on_port_change(port);
        break;

      default:
        GDB_MESSAGE(GDB_GUI, " update_voice_widget WARNING: unhandled change of '%s' to %f!\n", ypd->name, value);
        break;
    }

    /* if requested, send update to DSSI host */
    if (send_OSC)
        lo_send(osc_host_address, osc_control_path, "if", port, value);
}

void
update_voice_widgets_from_patch(y_patch_t *patch)
{
    /* -PORTS- */
    update_voice_widget(Y_PORT_OSC1_MODE,          (float)patch->osc1.mode,          FALSE);
    update_voice_widget(Y_PORT_OSC1_WAVEFORM,      (float)patch->osc1.waveform,      FALSE);
    update_voice_widget(Y_PORT_OSC1_PITCH,         (float)patch->osc1.pitch,         FALSE);
    update_voice_widget(Y_PORT_OSC1_DETUNE,        patch->osc1.detune,               FALSE);
    update_voice_widget(Y_PORT_OSC1_PITCH_MOD_SRC, (float)patch->osc1.pitch_mod_src, FALSE);
    update_voice_widget(Y_PORT_OSC1_PITCH_MOD_AMT, patch->osc1.pitch_mod_amt,        FALSE);
    update_voice_widget(Y_PORT_OSC1_MPARAM1,       patch->osc1.mparam1,              FALSE);
    update_voice_widget(Y_PORT_OSC1_MPARAM2,       patch->osc1.mparam2,              FALSE);
    update_voice_widget(Y_PORT_OSC1_MMOD_SRC,      (float)patch->osc1.mmod_src,      FALSE);
    update_voice_widget(Y_PORT_OSC1_MMOD_AMT,      patch->osc1.mmod_amt,             FALSE);
    update_voice_widget(Y_PORT_OSC1_AMP_MOD_SRC,   (float)patch->osc1.amp_mod_src,   FALSE);
    update_voice_widget(Y_PORT_OSC1_AMP_MOD_AMT,   patch->osc1.amp_mod_amt,          FALSE);
    update_voice_widget(Y_PORT_OSC1_LEVEL_A,       patch->osc1.level_a,              FALSE);
    update_voice_widget(Y_PORT_OSC1_LEVEL_B,       patch->osc1.level_b,              FALSE);

    update_voice_widget(Y_PORT_OSC2_MODE,          (float)patch->osc2.mode,          FALSE);
    update_voice_widget(Y_PORT_OSC2_WAVEFORM,      (float)patch->osc2.waveform,      FALSE);
    update_voice_widget(Y_PORT_OSC2_PITCH,         (float)patch->osc2.pitch,         FALSE);
    update_voice_widget(Y_PORT_OSC2_DETUNE,        patch->osc2.detune,               FALSE);
    update_voice_widget(Y_PORT_OSC2_PITCH_MOD_SRC, (float)patch->osc2.pitch_mod_src, FALSE);
    update_voice_widget(Y_PORT_OSC2_PITCH_MOD_AMT, patch->osc2.pitch_mod_amt,        FALSE);
    update_voice_widget(Y_PORT_OSC2_MPARAM1,       patch->osc2.mparam1,              FALSE);
    update_voice_widget(Y_PORT_OSC2_MPARAM2,       patch->osc2.mparam2,              FALSE);
    update_voice_widget(Y_PORT_OSC2_MMOD_SRC,      (float)patch->osc2.mmod_src,      FALSE);
    update_voice_widget(Y_PORT_OSC2_MMOD_AMT,      patch->osc2.mmod_amt,             FALSE);
    update_voice_widget(Y_PORT_OSC2_AMP_MOD_SRC,   (float)patch->osc2.amp_mod_src,   FALSE);
    update_voice_widget(Y_PORT_OSC2_AMP_MOD_AMT,   patch->osc2.amp_mod_amt,          FALSE);
    update_voice_widget(Y_PORT_OSC2_LEVEL_A,       patch->osc2.level_a,              FALSE);
    update_voice_widget(Y_PORT_OSC2_LEVEL_B,       patch->osc2.level_b,              FALSE);

    update_voice_widget(Y_PORT_OSC3_MODE,          (float)patch->osc3.mode,          FALSE);
    update_voice_widget(Y_PORT_OSC3_WAVEFORM,      (float)patch->osc3.waveform,      FALSE);
    update_voice_widget(Y_PORT_OSC3_PITCH,         (float)patch->osc3.pitch,         FALSE);
    update_voice_widget(Y_PORT_OSC3_DETUNE,        patch->osc3.detune,               FALSE);
    update_voice_widget(Y_PORT_OSC3_PITCH_MOD_SRC, (float)patch->osc3.pitch_mod_src, FALSE);
    update_voice_widget(Y_PORT_OSC3_PITCH_MOD_AMT, patch->osc3.pitch_mod_amt,        FALSE);
    update_voice_widget(Y_PORT_OSC3_MPARAM1,       patch->osc3.mparam1,              FALSE);
    update_voice_widget(Y_PORT_OSC3_MPARAM2,       patch->osc3.mparam2,              FALSE);
    update_voice_widget(Y_PORT_OSC3_MMOD_SRC,      (float)patch->osc3.mmod_src,      FALSE);
    update_voice_widget(Y_PORT_OSC3_MMOD_AMT,      patch->osc3.mmod_amt,             FALSE);
    update_voice_widget(Y_PORT_OSC3_AMP_MOD_SRC,   (float)patch->osc3.amp_mod_src,   FALSE);
    update_voice_widget(Y_PORT_OSC3_AMP_MOD_AMT,   patch->osc3.amp_mod_amt,          FALSE);
    update_voice_widget(Y_PORT_OSC3_LEVEL_A,       patch->osc3.level_a,              FALSE);
    update_voice_widget(Y_PORT_OSC3_LEVEL_B,       patch->osc3.level_b,              FALSE);

    update_voice_widget(Y_PORT_OSC4_MODE,          (float)patch->osc4.mode,          FALSE);
    update_voice_widget(Y_PORT_OSC4_WAVEFORM,      (float)patch->osc4.waveform,      FALSE);
    update_voice_widget(Y_PORT_OSC4_PITCH,         (float)patch->osc4.pitch,         FALSE);
    update_voice_widget(Y_PORT_OSC4_DETUNE,        patch->osc4.detune,               FALSE);
    update_voice_widget(Y_PORT_OSC4_PITCH_MOD_SRC, (float)patch->osc4.pitch_mod_src, FALSE);
    update_voice_widget(Y_PORT_OSC4_PITCH_MOD_AMT, patch->osc4.pitch_mod_amt,        FALSE);
    update_voice_widget(Y_PORT_OSC4_MPARAM1,       patch->osc4.mparam1,              FALSE);
    update_voice_widget(Y_PORT_OSC4_MPARAM2,       patch->osc4.mparam2,              FALSE);
    update_voice_widget(Y_PORT_OSC4_MMOD_SRC,      (float)patch->osc4.mmod_src,      FALSE);
    update_voice_widget(Y_PORT_OSC4_MMOD_AMT,      patch->osc4.mmod_amt,             FALSE);
    update_voice_widget(Y_PORT_OSC4_AMP_MOD_SRC,   (float)patch->osc4.amp_mod_src,   FALSE);
    update_voice_widget(Y_PORT_OSC4_AMP_MOD_AMT,   patch->osc4.amp_mod_amt,          FALSE);
    update_voice_widget(Y_PORT_OSC4_LEVEL_A,       patch->osc4.level_a,              FALSE);
    update_voice_widget(Y_PORT_OSC4_LEVEL_B,       patch->osc4.level_b,              FALSE);

    update_voice_widget(Y_PORT_VCF1_MODE,          (float)patch->vcf1.mode,          FALSE);
    update_voice_widget(Y_PORT_VCF1_SOURCE,        (float)patch->vcf1.source,        FALSE);
    update_voice_widget(Y_PORT_VCF1_FREQUENCY,     patch->vcf1.frequency,            FALSE);
    update_voice_widget(Y_PORT_VCF1_FREQ_MOD_SRC,  (float)patch->vcf1.freq_mod_src,  FALSE);
    update_voice_widget(Y_PORT_VCF1_FREQ_MOD_AMT,  patch->vcf1.freq_mod_amt,         FALSE);
    update_voice_widget(Y_PORT_VCF1_QRES,          patch->vcf1.qres,                 FALSE);
    update_voice_widget(Y_PORT_VCF1_MPARAM,        patch->vcf1.mparam,               FALSE);

    update_voice_widget(Y_PORT_VCF2_MODE,          (float)patch->vcf2.mode,          FALSE);
    update_voice_widget(Y_PORT_VCF2_SOURCE,        (float)patch->vcf2.source,        FALSE);
    update_voice_widget(Y_PORT_VCF2_FREQUENCY,     patch->vcf2.frequency,            FALSE);
    update_voice_widget(Y_PORT_VCF2_FREQ_MOD_SRC,  (float)patch->vcf2.freq_mod_src,  FALSE);
    update_voice_widget(Y_PORT_VCF2_FREQ_MOD_AMT,  patch->vcf2.freq_mod_amt,         FALSE);
    update_voice_widget(Y_PORT_VCF2_QRES,          patch->vcf2.qres,                 FALSE);
    update_voice_widget(Y_PORT_VCF2_MPARAM,        patch->vcf2.mparam,               FALSE);

    update_voice_widget(Y_PORT_BUSA_LEVEL,         patch->busa_level,                FALSE);
    update_voice_widget(Y_PORT_BUSA_PAN,           patch->busa_pan,                  FALSE);
    update_voice_widget(Y_PORT_BUSB_LEVEL,         patch->busb_level,                FALSE);
    update_voice_widget(Y_PORT_BUSB_PAN,           patch->busb_pan,                  FALSE);
    update_voice_widget(Y_PORT_VCF1_LEVEL,         patch->vcf1_level,                FALSE);
    update_voice_widget(Y_PORT_VCF1_PAN,           patch->vcf1_pan,                  FALSE);
    update_voice_widget(Y_PORT_VCF2_LEVEL,         patch->vcf2_level,                FALSE);
    update_voice_widget(Y_PORT_VCF2_PAN,           patch->vcf2_pan,                  FALSE);
    update_voice_widget(Y_PORT_VOLUME,             patch->volume,                    FALSE);

    update_voice_widget(Y_PORT_EFFECT_MODE,        (float)patch->effect_mode,        FALSE);
    update_voice_widget(Y_PORT_EFFECT_PARAM1,      patch->effect_param1,             FALSE);
    update_voice_widget(Y_PORT_EFFECT_PARAM2,      patch->effect_param2,             FALSE);
    update_voice_widget(Y_PORT_EFFECT_PARAM3,      patch->effect_param3,             FALSE);
    update_voice_widget(Y_PORT_EFFECT_PARAM4,      patch->effect_param4,             FALSE);
    update_voice_widget(Y_PORT_EFFECT_PARAM5,      patch->effect_param5,             FALSE);
    update_voice_widget(Y_PORT_EFFECT_PARAM6,      patch->effect_param6,             FALSE);
    update_voice_widget(Y_PORT_EFFECT_MIX,         patch->effect_mix,                FALSE);

    update_voice_widget(Y_PORT_GLIDE_TIME,         patch->glide_time,                FALSE);
    update_voice_widget(Y_PORT_BEND_RANGE,         (float)patch->bend_range,         FALSE);

    update_voice_widget(Y_PORT_GLFO_FREQUENCY,     patch->glfo.frequency,            FALSE);
    update_voice_widget(Y_PORT_GLFO_WAVEFORM,      (float)patch->glfo.waveform,      FALSE);
    update_voice_widget(Y_PORT_GLFO_AMP_MOD_SRC,   (float)patch->glfo.amp_mod_src,   FALSE);
    update_voice_widget(Y_PORT_GLFO_AMP_MOD_AMT,   patch->glfo.amp_mod_amt,          FALSE);

    update_voice_widget(Y_PORT_VLFO_FREQUENCY,     patch->vlfo.frequency,            FALSE);
    update_voice_widget(Y_PORT_VLFO_WAVEFORM,      (float)patch->vlfo.waveform,      FALSE);
    update_voice_widget(Y_PORT_VLFO_DELAY,         patch->vlfo.delay,                FALSE);
    update_voice_widget(Y_PORT_VLFO_AMP_MOD_SRC,   (float)patch->vlfo.amp_mod_src,   FALSE);
    update_voice_widget(Y_PORT_VLFO_AMP_MOD_AMT,   patch->vlfo.amp_mod_amt,          FALSE);

    update_voice_widget(Y_PORT_MLFO_FREQUENCY,     patch->mlfo.frequency,            FALSE);
    update_voice_widget(Y_PORT_MLFO_WAVEFORM,      (float)patch->mlfo.waveform,      FALSE);
    update_voice_widget(Y_PORT_MLFO_DELAY,         patch->mlfo.delay,                FALSE);
    update_voice_widget(Y_PORT_MLFO_AMP_MOD_SRC,   (float)patch->mlfo.amp_mod_src,   FALSE);
    update_voice_widget(Y_PORT_MLFO_AMP_MOD_AMT,   patch->mlfo.amp_mod_amt,          FALSE);
    update_voice_widget(Y_PORT_MLFO_PHASE_SPREAD,  patch->mlfo_phase_spread,         FALSE);
    update_voice_widget(Y_PORT_MLFO_RANDOM_FREQ,   patch->mlfo_random_freq,          FALSE);

    update_voice_widget(Y_PORT_EGO_MODE,           (float)patch->ego.mode,           FALSE);
    update_voice_widget(Y_PORT_EGO_SHAPE1,         (float)patch->ego.shape1,         FALSE);
    update_voice_widget(Y_PORT_EGO_TIME1,          patch->ego.time1,                 FALSE);
    update_voice_widget(Y_PORT_EGO_LEVEL1,         patch->ego.level1,                FALSE);
    update_voice_widget(Y_PORT_EGO_SHAPE2,         (float)patch->ego.shape2,         FALSE);
    update_voice_widget(Y_PORT_EGO_TIME2,          patch->ego.time2,                 FALSE);
    update_voice_widget(Y_PORT_EGO_LEVEL2,         patch->ego.level2,                FALSE);
    update_voice_widget(Y_PORT_EGO_SHAPE3,         (float)patch->ego.shape3,         FALSE);
    update_voice_widget(Y_PORT_EGO_TIME3,          patch->ego.time3,                 FALSE);
    update_voice_widget(Y_PORT_EGO_LEVEL3,         patch->ego.level3,                FALSE);
    update_voice_widget(Y_PORT_EGO_SHAPE4,         (float)patch->ego.shape4,         FALSE);
    update_voice_widget(Y_PORT_EGO_TIME4,          patch->ego.time4,                 FALSE);
    update_voice_widget(Y_PORT_EGO_VEL_LEVEL_SENS, patch->ego.vel_level_sens,        FALSE);
    update_voice_widget(Y_PORT_EGO_VEL_TIME_SCALE, patch->ego.vel_time_scale,        FALSE);
    update_voice_widget(Y_PORT_EGO_KBD_TIME_SCALE, patch->ego.kbd_time_scale,        FALSE);
    update_voice_widget(Y_PORT_EGO_AMP_MOD_SRC,    (float)patch->ego.amp_mod_src,    FALSE);
    update_voice_widget(Y_PORT_EGO_AMP_MOD_AMT,    patch->ego.amp_mod_amt,           FALSE);
                                                                                        
    update_voice_widget(Y_PORT_EG1_MODE,           (float)patch->eg1.mode,           FALSE);
    update_voice_widget(Y_PORT_EG1_SHAPE1,         (float)patch->eg1.shape1,         FALSE);
    update_voice_widget(Y_PORT_EG1_TIME1,          patch->eg1.time1,                 FALSE);
    update_voice_widget(Y_PORT_EG1_LEVEL1,         patch->eg1.level1,                FALSE);
    update_voice_widget(Y_PORT_EG1_SHAPE2,         (float)patch->eg1.shape2,         FALSE);
    update_voice_widget(Y_PORT_EG1_TIME2,          patch->eg1.time2,                 FALSE);
    update_voice_widget(Y_PORT_EG1_LEVEL2,         patch->eg1.level2,                FALSE);
    update_voice_widget(Y_PORT_EG1_SHAPE3,         (float)patch->eg1.shape3,         FALSE);
    update_voice_widget(Y_PORT_EG1_TIME3,          patch->eg1.time3,                 FALSE);
    update_voice_widget(Y_PORT_EG1_LEVEL3,         patch->eg1.level3,                FALSE);
    update_voice_widget(Y_PORT_EG1_SHAPE4,         (float)patch->eg1.shape4,         FALSE);
    update_voice_widget(Y_PORT_EG1_TIME4,          patch->eg1.time4,                 FALSE);
    update_voice_widget(Y_PORT_EG1_VEL_LEVEL_SENS, patch->eg1.vel_level_sens,        FALSE);
    update_voice_widget(Y_PORT_EG1_VEL_TIME_SCALE, patch->eg1.vel_time_scale,        FALSE);
    update_voice_widget(Y_PORT_EG1_KBD_TIME_SCALE, patch->eg1.kbd_time_scale,        FALSE);
    update_voice_widget(Y_PORT_EG1_AMP_MOD_SRC,    (float)patch->eg1.amp_mod_src,    FALSE);
    update_voice_widget(Y_PORT_EG1_AMP_MOD_AMT,    patch->eg1.amp_mod_amt,           FALSE);
                                                                                        
    update_voice_widget(Y_PORT_EG2_MODE,           (float)patch->eg2.mode,           FALSE);
    update_voice_widget(Y_PORT_EG2_SHAPE1,         (float)patch->eg2.shape1,         FALSE);
    update_voice_widget(Y_PORT_EG2_TIME1,          patch->eg2.time1,                 FALSE);
    update_voice_widget(Y_PORT_EG2_LEVEL1,         patch->eg2.level1,                FALSE);
    update_voice_widget(Y_PORT_EG2_SHAPE2,         (float)patch->eg2.shape2,         FALSE);
    update_voice_widget(Y_PORT_EG2_TIME2,          patch->eg2.time2,                 FALSE);
    update_voice_widget(Y_PORT_EG2_LEVEL2,         patch->eg2.level2,                FALSE);
    update_voice_widget(Y_PORT_EG2_SHAPE3,         (float)patch->eg2.shape3,         FALSE);
    update_voice_widget(Y_PORT_EG2_TIME3,          patch->eg2.time3,                 FALSE);
    update_voice_widget(Y_PORT_EG2_LEVEL3,         patch->eg2.level3,                FALSE);
    update_voice_widget(Y_PORT_EG2_SHAPE4,         (float)patch->eg2.shape4,         FALSE);
    update_voice_widget(Y_PORT_EG2_TIME4,          patch->eg2.time4,                 FALSE);
    update_voice_widget(Y_PORT_EG2_VEL_LEVEL_SENS, patch->eg2.vel_level_sens,        FALSE);
    update_voice_widget(Y_PORT_EG2_VEL_TIME_SCALE, patch->eg2.vel_time_scale,        FALSE);
    update_voice_widget(Y_PORT_EG2_KBD_TIME_SCALE, patch->eg2.kbd_time_scale,        FALSE);
    update_voice_widget(Y_PORT_EG2_AMP_MOD_SRC,    (float)patch->eg2.amp_mod_src,    FALSE);
    update_voice_widget(Y_PORT_EG2_AMP_MOD_AMT,    patch->eg2.amp_mod_amt,           FALSE);
                                                                                        
    update_voice_widget(Y_PORT_EG3_MODE,           (float)patch->eg3.mode,           FALSE);
    update_voice_widget(Y_PORT_EG3_SHAPE1,         (float)patch->eg3.shape1,         FALSE);
    update_voice_widget(Y_PORT_EG3_TIME1,          patch->eg3.time1,                 FALSE);
    update_voice_widget(Y_PORT_EG3_LEVEL1,         patch->eg3.level1,                FALSE);
    update_voice_widget(Y_PORT_EG3_SHAPE2,         (float)patch->eg3.shape2,         FALSE);
    update_voice_widget(Y_PORT_EG3_TIME2,          patch->eg3.time2,                 FALSE);
    update_voice_widget(Y_PORT_EG3_LEVEL2,         patch->eg3.level2,                FALSE);
    update_voice_widget(Y_PORT_EG3_SHAPE3,         (float)patch->eg3.shape3,         FALSE);
    update_voice_widget(Y_PORT_EG3_TIME3,          patch->eg3.time3,                 FALSE);
    update_voice_widget(Y_PORT_EG3_LEVEL3,         patch->eg3.level3,                FALSE);
    update_voice_widget(Y_PORT_EG3_SHAPE4,         (float)patch->eg3.shape4,         FALSE);
    update_voice_widget(Y_PORT_EG3_TIME4,          patch->eg3.time4,                 FALSE);
    update_voice_widget(Y_PORT_EG3_VEL_LEVEL_SENS, patch->eg3.vel_level_sens,        FALSE);
    update_voice_widget(Y_PORT_EG3_VEL_TIME_SCALE, patch->eg3.vel_time_scale,        FALSE);
    update_voice_widget(Y_PORT_EG3_KBD_TIME_SCALE, patch->eg3.kbd_time_scale,        FALSE);
    update_voice_widget(Y_PORT_EG3_AMP_MOD_SRC,    (float)patch->eg3.amp_mod_src,    FALSE);
    update_voice_widget(Y_PORT_EG3_AMP_MOD_AMT,    patch->eg3.amp_mod_amt,           FALSE);
                                                                                        
    update_voice_widget(Y_PORT_EG4_MODE,           (float)patch->eg4.mode,           FALSE);
    update_voice_widget(Y_PORT_EG4_SHAPE1,         (float)patch->eg4.shape1,         FALSE);
    update_voice_widget(Y_PORT_EG4_TIME1,          patch->eg4.time1,                 FALSE);
    update_voice_widget(Y_PORT_EG4_LEVEL1,         patch->eg4.level1,                FALSE);
    update_voice_widget(Y_PORT_EG4_SHAPE2,         (float)patch->eg4.shape2,         FALSE);
    update_voice_widget(Y_PORT_EG4_TIME2,          patch->eg4.time2,                 FALSE);
    update_voice_widget(Y_PORT_EG4_LEVEL2,         patch->eg4.level2,                FALSE);
    update_voice_widget(Y_PORT_EG4_SHAPE3,         (float)patch->eg4.shape3,         FALSE);
    update_voice_widget(Y_PORT_EG4_TIME3,          patch->eg4.time3,                 FALSE);
    update_voice_widget(Y_PORT_EG4_LEVEL3,         patch->eg4.level3,                FALSE);
    update_voice_widget(Y_PORT_EG4_SHAPE4,         (float)patch->eg4.shape4,         FALSE);
    update_voice_widget(Y_PORT_EG4_TIME4,          patch->eg4.time4,                 FALSE);
    update_voice_widget(Y_PORT_EG4_VEL_LEVEL_SENS, patch->eg4.vel_level_sens,        FALSE);
    update_voice_widget(Y_PORT_EG4_VEL_TIME_SCALE, patch->eg4.vel_time_scale,        FALSE);
    update_voice_widget(Y_PORT_EG4_KBD_TIME_SCALE, patch->eg4.kbd_time_scale,        FALSE);
    update_voice_widget(Y_PORT_EG4_AMP_MOD_SRC,    (float)patch->eg4.amp_mod_src,    FALSE);
    update_voice_widget(Y_PORT_EG4_AMP_MOD_AMT,    patch->eg4.amp_mod_amt,           FALSE);

    update_voice_widget(Y_PORT_MODMIX_BIAS,        patch->modmix_bias,               FALSE);
    update_voice_widget(Y_PORT_MODMIX_MOD1_SRC,    (float)patch->modmix_mod1_src,    FALSE);
    update_voice_widget(Y_PORT_MODMIX_MOD1_AMT,    patch->modmix_mod1_amt,           FALSE);
    update_voice_widget(Y_PORT_MODMIX_MOD2_SRC,    (float)patch->modmix_mod2_src,    FALSE);
    update_voice_widget(Y_PORT_MODMIX_MOD2_AMT,    patch->modmix_mod2_amt,           FALSE);

    gtk_entry_set_text(GTK_ENTRY(name_entry), patch->name);
    gtk_entry_set_text(GTK_ENTRY(comment_entry), patch->comment);
}

void
update_from_program_select(int program)
{
    if (program < patch_count) {
        GtkTreeIter iter;

        GDB_MESSAGE(GDB_GUI, " update_from_program_select: received program %d\n", program);

        find_program_in_list_store(&iter, program);
        /* update selected row, but don't call on_patches_selection_changed() */
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(patches_list));
        g_signal_handlers_block_by_func(selection, on_patches_selection_changed, NULL);
        gtk_tree_selection_select_iter(selection, &iter);
        /* emit cursor-changed signal to get view to update */
        g_signal_emit_by_name(patches_list, "cursor-changed");
        /* -FIX- it would be nice to move the GtkScrolledWindow so that the selected row is
         * in view. Anybody know how to do that easily? */
        g_signal_handlers_unblock_by_func(selection, on_patches_selection_changed, NULL);

        update_voice_widgets_from_patch(&patches[program]);

    } else {  /* out of range */

        /* gtk_tree_selection_unselect_all(selection); */

    }
}

float
get_value_from_knob(int index)
{
    struct y_port_descriptor *ypd = &y_port_description[index];
    float value = GTK_ADJUSTMENT(voice_widgets[index].adjustment)->value;

    switch (ypd->type) {

      case Y_PORT_TYPE_LINEAR:
      case Y_PORT_TYPE_PAN:
        return value;

      case Y_PORT_TYPE_LOGARITHMIC:
        return expf(logf(ypd->upper_bound) * value +
                    logf(ypd->lower_bound) * (1.0f - value));

      case Y_PORT_TYPE_LOGSCALED:
        return powf(value, ypd->scale) * (ypd->upper_bound - ypd->lower_bound)
                   + ypd->lower_bound;

      case Y_PORT_TYPE_BPLOGSCALED:
        if (value > 0.5f) {
            return powf(2.0f * (value - 0.5f), ypd->scale) * ypd->upper_bound;
        } else if (value < 0.5f) {
            return powf(2.0f * (0.5f - value), ypd->scale) * ypd->lower_bound;
        } else /* 0.5 */
            return 0.0f;

      default:
        GDB_MESSAGE(GDB_GUI, " get_value_from_knob WARNING: unhandled knob type %d for port %d!\n", ypd->type, index);
        return 0.0f;
    }
}

int
get_value_from_detent(int index)
{
    return lrintf(GTK_ADJUSTMENT(voice_widgets[index].adjustment)->value);
}

int
get_value_from_combo(int index)
{
    return combo_get_value(index);
}

// static int
// get_value_from_onoff(int index)
// {
//     return (GTK_TOGGLE_BUTTON (voice_widget[index])->active ? 1 : 0);
// }

void
update_patch_from_voice_widgets(y_patch_t *patch)
{
    int i;

    /* -PORTS- */
    patch->osc1.mode          = get_value_from_combo(Y_PORT_OSC1_MODE);
    patch->osc1.waveform      = get_value_from_combo(Y_PORT_OSC1_WAVEFORM);
    patch->osc1.pitch         = get_value_from_detent(Y_PORT_OSC1_PITCH);
    patch->osc1.detune        = get_value_from_knob(Y_PORT_OSC1_DETUNE);
    patch->osc1.pitch_mod_src = get_value_from_combo(Y_PORT_OSC1_PITCH_MOD_SRC);
    patch->osc1.pitch_mod_amt = get_value_from_knob(Y_PORT_OSC1_PITCH_MOD_AMT);
    patch->osc1.mparam1       = get_value_from_knob(Y_PORT_OSC1_MPARAM1);
    patch->osc1.mparam2       = get_value_from_knob(Y_PORT_OSC1_MPARAM2);
    patch->osc1.mmod_src      = get_value_from_combo(Y_PORT_OSC1_MMOD_SRC);
    patch->osc1.mmod_amt      = get_value_from_knob(Y_PORT_OSC1_MMOD_AMT);
    patch->osc1.amp_mod_src   = get_value_from_combo(Y_PORT_OSC1_AMP_MOD_SRC);
    patch->osc1.amp_mod_amt   = get_value_from_knob(Y_PORT_OSC1_AMP_MOD_AMT);
    patch->osc1.level_a       = get_value_from_knob(Y_PORT_OSC1_LEVEL_A);
    patch->osc1.level_b       = get_value_from_knob(Y_PORT_OSC1_LEVEL_B);

    patch->osc2.mode          = get_value_from_combo(Y_PORT_OSC2_MODE);
    patch->osc2.waveform      = get_value_from_combo(Y_PORT_OSC2_WAVEFORM);
    patch->osc2.pitch         = get_value_from_detent(Y_PORT_OSC2_PITCH);
    patch->osc2.detune        = get_value_from_knob(Y_PORT_OSC2_DETUNE);
    patch->osc2.pitch_mod_src = get_value_from_combo(Y_PORT_OSC2_PITCH_MOD_SRC);
    patch->osc2.pitch_mod_amt = get_value_from_knob(Y_PORT_OSC2_PITCH_MOD_AMT);
    patch->osc2.mparam1       = get_value_from_knob(Y_PORT_OSC2_MPARAM1);
    patch->osc2.mparam2       = get_value_from_knob(Y_PORT_OSC2_MPARAM2);
    patch->osc2.mmod_src      = get_value_from_combo(Y_PORT_OSC2_MMOD_SRC);
    patch->osc2.mmod_amt      = get_value_from_knob(Y_PORT_OSC2_MMOD_AMT);
    patch->osc2.amp_mod_src   = get_value_from_combo(Y_PORT_OSC2_AMP_MOD_SRC);
    patch->osc2.amp_mod_amt   = get_value_from_knob(Y_PORT_OSC2_AMP_MOD_AMT);
    patch->osc2.level_a       = get_value_from_knob(Y_PORT_OSC2_LEVEL_A);
    patch->osc2.level_b       = get_value_from_knob(Y_PORT_OSC2_LEVEL_B);

    patch->osc3.mode          = get_value_from_combo(Y_PORT_OSC3_MODE);
    patch->osc3.waveform      = get_value_from_combo(Y_PORT_OSC3_WAVEFORM);
    patch->osc3.pitch         = get_value_from_detent(Y_PORT_OSC3_PITCH);
    patch->osc3.detune        = get_value_from_knob(Y_PORT_OSC3_DETUNE);
    patch->osc3.pitch_mod_src = get_value_from_combo(Y_PORT_OSC3_PITCH_MOD_SRC);
    patch->osc3.pitch_mod_amt = get_value_from_knob(Y_PORT_OSC3_PITCH_MOD_AMT);
    patch->osc3.mparam1       = get_value_from_knob(Y_PORT_OSC3_MPARAM1);
    patch->osc3.mparam2       = get_value_from_knob(Y_PORT_OSC3_MPARAM2);
    patch->osc3.mmod_src      = get_value_from_combo(Y_PORT_OSC3_MMOD_SRC);
    patch->osc3.mmod_amt      = get_value_from_knob(Y_PORT_OSC3_MMOD_AMT);
    patch->osc3.amp_mod_src   = get_value_from_combo(Y_PORT_OSC3_AMP_MOD_SRC);
    patch->osc3.amp_mod_amt   = get_value_from_knob(Y_PORT_OSC3_AMP_MOD_AMT);
    patch->osc3.level_a       = get_value_from_knob(Y_PORT_OSC3_LEVEL_A);
    patch->osc3.level_b       = get_value_from_knob(Y_PORT_OSC3_LEVEL_B);

    patch->osc4.mode          = get_value_from_combo(Y_PORT_OSC4_MODE);
    patch->osc4.waveform      = get_value_from_combo(Y_PORT_OSC4_WAVEFORM);
    patch->osc4.pitch         = get_value_from_detent(Y_PORT_OSC4_PITCH);
    patch->osc4.detune        = get_value_from_knob(Y_PORT_OSC4_DETUNE);
    patch->osc4.pitch_mod_src = get_value_from_combo(Y_PORT_OSC4_PITCH_MOD_SRC);
    patch->osc4.pitch_mod_amt = get_value_from_knob(Y_PORT_OSC4_PITCH_MOD_AMT);
    patch->osc4.mparam1       = get_value_from_knob(Y_PORT_OSC4_MPARAM1);
    patch->osc4.mparam2       = get_value_from_knob(Y_PORT_OSC4_MPARAM2);
    patch->osc4.mmod_src      = get_value_from_combo(Y_PORT_OSC4_MMOD_SRC);
    patch->osc4.mmod_amt      = get_value_from_knob(Y_PORT_OSC4_MMOD_AMT);
    patch->osc4.amp_mod_src   = get_value_from_combo(Y_PORT_OSC4_AMP_MOD_SRC);
    patch->osc4.amp_mod_amt   = get_value_from_knob(Y_PORT_OSC4_AMP_MOD_AMT);
    patch->osc4.level_a       = get_value_from_knob(Y_PORT_OSC4_LEVEL_A);
    patch->osc4.level_b       = get_value_from_knob(Y_PORT_OSC4_LEVEL_B);

    patch->vcf1.mode          = get_value_from_combo(Y_PORT_VCF1_MODE);
    patch->vcf1.source        = get_value_from_combo(Y_PORT_VCF1_SOURCE);
    patch->vcf1.frequency     = get_value_from_knob(Y_PORT_VCF1_FREQUENCY);
    patch->vcf1.freq_mod_src  = get_value_from_combo(Y_PORT_VCF1_FREQ_MOD_SRC);
    patch->vcf1.freq_mod_amt  = get_value_from_knob(Y_PORT_VCF1_FREQ_MOD_AMT);
    patch->vcf1.qres          = get_value_from_knob(Y_PORT_VCF1_QRES);
    patch->vcf1.mparam        = get_value_from_knob(Y_PORT_VCF1_MPARAM);

    patch->vcf2.mode          = get_value_from_combo(Y_PORT_VCF2_MODE);
    patch->vcf2.source        = get_value_from_combo(Y_PORT_VCF2_SOURCE);
    patch->vcf2.frequency     = get_value_from_knob(Y_PORT_VCF2_FREQUENCY);
    patch->vcf2.freq_mod_src  = get_value_from_combo(Y_PORT_VCF2_FREQ_MOD_SRC);
    patch->vcf2.freq_mod_amt  = get_value_from_knob(Y_PORT_VCF2_FREQ_MOD_AMT);
    patch->vcf2.qres          = get_value_from_knob(Y_PORT_VCF2_QRES);
    patch->vcf2.mparam        = get_value_from_knob(Y_PORT_VCF2_MPARAM);

    patch->busa_level         = get_value_from_knob(Y_PORT_BUSA_LEVEL);
    patch->busa_pan           = get_value_from_knob(Y_PORT_BUSA_PAN);
    patch->busb_level         = get_value_from_knob(Y_PORT_BUSB_LEVEL);
    patch->busb_pan           = get_value_from_knob(Y_PORT_BUSB_PAN);
    patch->vcf1_level         = get_value_from_knob(Y_PORT_VCF1_LEVEL);
    patch->vcf1_pan           = get_value_from_knob(Y_PORT_VCF1_PAN);
    patch->vcf2_level         = get_value_from_knob(Y_PORT_VCF2_LEVEL);
    patch->vcf2_pan           = get_value_from_knob(Y_PORT_VCF2_PAN);
    patch->volume             = get_value_from_knob(Y_PORT_VOLUME);

    patch->effect_mode        = get_value_from_combo(Y_PORT_EFFECT_MODE);
    patch->effect_param1      = get_value_from_knob(Y_PORT_EFFECT_PARAM1);
    patch->effect_param2      = get_value_from_knob(Y_PORT_EFFECT_PARAM2);
    patch->effect_param3      = get_value_from_knob(Y_PORT_EFFECT_PARAM3);
    patch->effect_param4      = get_value_from_knob(Y_PORT_EFFECT_PARAM4);
    patch->effect_param5      = get_value_from_knob(Y_PORT_EFFECT_PARAM5);
    patch->effect_param6      = get_value_from_knob(Y_PORT_EFFECT_PARAM6);
    patch->effect_mix         = get_value_from_knob(Y_PORT_EFFECT_MIX);

    patch->glide_time         = get_value_from_knob(Y_PORT_GLIDE_TIME);
    patch->bend_range         = get_value_from_detent(Y_PORT_BEND_RANGE);

    patch->glfo.frequency     = get_value_from_knob(Y_PORT_GLFO_FREQUENCY);
    patch->glfo.waveform      = get_value_from_combo(Y_PORT_GLFO_WAVEFORM);
    patch->glfo.delay         = 0.0f;
    patch->glfo.amp_mod_src   = get_value_from_combo(Y_PORT_GLFO_AMP_MOD_SRC);
    patch->glfo.amp_mod_amt   = get_value_from_knob(Y_PORT_GLFO_AMP_MOD_AMT);

    patch->vlfo.frequency     = get_value_from_knob(Y_PORT_VLFO_FREQUENCY);
    patch->vlfo.waveform      = get_value_from_combo(Y_PORT_VLFO_WAVEFORM);
    patch->vlfo.delay         = get_value_from_knob(Y_PORT_VLFO_DELAY);
    patch->vlfo.amp_mod_src   = get_value_from_combo(Y_PORT_VLFO_AMP_MOD_SRC);
    patch->vlfo.amp_mod_amt   = get_value_from_knob(Y_PORT_VLFO_AMP_MOD_AMT);

    patch->mlfo.frequency     = get_value_from_knob(Y_PORT_MLFO_FREQUENCY);
    patch->mlfo.waveform      = get_value_from_combo(Y_PORT_MLFO_WAVEFORM);
    patch->mlfo.delay         = get_value_from_knob(Y_PORT_MLFO_DELAY);
    patch->mlfo.amp_mod_src   = get_value_from_combo(Y_PORT_MLFO_AMP_MOD_SRC);
    patch->mlfo.amp_mod_amt   = get_value_from_knob(Y_PORT_MLFO_AMP_MOD_AMT);
    patch->mlfo_phase_spread  = get_value_from_knob(Y_PORT_MLFO_PHASE_SPREAD);
    patch->mlfo_random_freq   = get_value_from_knob(Y_PORT_MLFO_RANDOM_FREQ);

    patch->ego.mode           = get_value_from_combo(Y_PORT_EGO_MODE);
    patch->ego.shape1         = get_value_from_combo(Y_PORT_EGO_SHAPE1);
    patch->ego.time1          = get_value_from_knob(Y_PORT_EGO_TIME1);
    patch->ego.level1         = get_value_from_knob(Y_PORT_EGO_LEVEL1);
    patch->ego.shape2         = get_value_from_combo(Y_PORT_EGO_SHAPE2);
    patch->ego.time2          = get_value_from_knob(Y_PORT_EGO_TIME2);
    patch->ego.level2         = get_value_from_knob(Y_PORT_EGO_LEVEL2);
    patch->ego.shape3         = get_value_from_combo(Y_PORT_EGO_SHAPE3);
    patch->ego.time3          = get_value_from_knob(Y_PORT_EGO_TIME3);
    patch->ego.level3         = get_value_from_knob(Y_PORT_EGO_LEVEL3);
    patch->ego.shape4         = get_value_from_combo(Y_PORT_EGO_SHAPE4);
    patch->ego.time4          = get_value_from_knob(Y_PORT_EGO_TIME4);
    patch->ego.vel_level_sens = get_value_from_knob(Y_PORT_EGO_VEL_LEVEL_SENS);
    patch->ego.vel_time_scale = get_value_from_knob(Y_PORT_EGO_VEL_TIME_SCALE);
    patch->ego.kbd_time_scale = get_value_from_knob(Y_PORT_EGO_KBD_TIME_SCALE);
    patch->ego.amp_mod_src    = get_value_from_combo(Y_PORT_EGO_AMP_MOD_SRC);
    patch->ego.amp_mod_amt    = get_value_from_knob(Y_PORT_EGO_AMP_MOD_AMT);

    patch->eg1.mode           = get_value_from_combo(Y_PORT_EG1_MODE);
    patch->eg1.shape1         = get_value_from_combo(Y_PORT_EG1_SHAPE1);
    patch->eg1.time1          = get_value_from_knob(Y_PORT_EG1_TIME1);
    patch->eg1.level1         = get_value_from_knob(Y_PORT_EG1_LEVEL1);
    patch->eg1.shape2         = get_value_from_combo(Y_PORT_EG1_SHAPE2);
    patch->eg1.time2          = get_value_from_knob(Y_PORT_EG1_TIME2);
    patch->eg1.level2         = get_value_from_knob(Y_PORT_EG1_LEVEL2);
    patch->eg1.shape3         = get_value_from_combo(Y_PORT_EG1_SHAPE3);
    patch->eg1.time3          = get_value_from_knob(Y_PORT_EG1_TIME3);
    patch->eg1.level3         = get_value_from_knob(Y_PORT_EG1_LEVEL3);
    patch->eg1.shape4         = get_value_from_combo(Y_PORT_EG1_SHAPE4);
    patch->eg1.time4          = get_value_from_knob(Y_PORT_EG1_TIME4);
    patch->eg1.vel_level_sens = get_value_from_knob(Y_PORT_EG1_VEL_LEVEL_SENS);
    patch->eg1.vel_time_scale = get_value_from_knob(Y_PORT_EG1_VEL_TIME_SCALE);
    patch->eg1.kbd_time_scale = get_value_from_knob(Y_PORT_EG1_KBD_TIME_SCALE);
    patch->eg1.amp_mod_src    = get_value_from_combo(Y_PORT_EG1_AMP_MOD_SRC);
    patch->eg1.amp_mod_amt    = get_value_from_knob(Y_PORT_EG1_AMP_MOD_AMT);

    patch->eg2.mode           = get_value_from_combo(Y_PORT_EG2_MODE);
    patch->eg2.shape1         = get_value_from_combo(Y_PORT_EG2_SHAPE1);
    patch->eg2.time1          = get_value_from_knob(Y_PORT_EG2_TIME1);
    patch->eg2.level1         = get_value_from_knob(Y_PORT_EG2_LEVEL1);
    patch->eg2.shape2         = get_value_from_combo(Y_PORT_EG2_SHAPE2);
    patch->eg2.time2          = get_value_from_knob(Y_PORT_EG2_TIME2);
    patch->eg2.level2         = get_value_from_knob(Y_PORT_EG2_LEVEL2);
    patch->eg2.shape3         = get_value_from_combo(Y_PORT_EG2_SHAPE3);
    patch->eg2.time3          = get_value_from_knob(Y_PORT_EG2_TIME3);
    patch->eg2.level3         = get_value_from_knob(Y_PORT_EG2_LEVEL3);
    patch->eg2.shape4         = get_value_from_combo(Y_PORT_EG2_SHAPE4);
    patch->eg2.time4          = get_value_from_knob(Y_PORT_EG2_TIME4);
    patch->eg2.vel_level_sens = get_value_from_knob(Y_PORT_EG2_VEL_LEVEL_SENS);
    patch->eg2.vel_time_scale = get_value_from_knob(Y_PORT_EG2_VEL_TIME_SCALE);
    patch->eg2.kbd_time_scale = get_value_from_knob(Y_PORT_EG2_KBD_TIME_SCALE);
    patch->eg2.amp_mod_src    = get_value_from_combo(Y_PORT_EG2_AMP_MOD_SRC);
    patch->eg2.amp_mod_amt    = get_value_from_knob(Y_PORT_EG2_AMP_MOD_AMT);

    patch->eg3.mode           = get_value_from_combo(Y_PORT_EG3_MODE);
    patch->eg3.shape1         = get_value_from_combo(Y_PORT_EG3_SHAPE1);
    patch->eg3.time1          = get_value_from_knob(Y_PORT_EG3_TIME1);
    patch->eg3.level1         = get_value_from_knob(Y_PORT_EG3_LEVEL1);
    patch->eg3.shape2         = get_value_from_combo(Y_PORT_EG3_SHAPE2);
    patch->eg3.time2          = get_value_from_knob(Y_PORT_EG3_TIME2);
    patch->eg3.level2         = get_value_from_knob(Y_PORT_EG3_LEVEL2);
    patch->eg3.shape3         = get_value_from_combo(Y_PORT_EG3_SHAPE3);
    patch->eg3.time3          = get_value_from_knob(Y_PORT_EG3_TIME3);
    patch->eg3.level3         = get_value_from_knob(Y_PORT_EG3_LEVEL3);
    patch->eg3.shape4         = get_value_from_combo(Y_PORT_EG3_SHAPE4);
    patch->eg3.time4          = get_value_from_knob(Y_PORT_EG3_TIME4);
    patch->eg3.vel_level_sens = get_value_from_knob(Y_PORT_EG3_VEL_LEVEL_SENS);
    patch->eg3.vel_time_scale = get_value_from_knob(Y_PORT_EG3_VEL_TIME_SCALE);
    patch->eg3.kbd_time_scale = get_value_from_knob(Y_PORT_EG3_KBD_TIME_SCALE);
    patch->eg3.amp_mod_src    = get_value_from_combo(Y_PORT_EG3_AMP_MOD_SRC);
    patch->eg3.amp_mod_amt    = get_value_from_knob(Y_PORT_EG3_AMP_MOD_AMT);

    patch->eg4.mode           = get_value_from_combo(Y_PORT_EG4_MODE);
    patch->eg4.shape1         = get_value_from_combo(Y_PORT_EG4_SHAPE1);
    patch->eg4.time1          = get_value_from_knob(Y_PORT_EG4_TIME1);
    patch->eg4.level1         = get_value_from_knob(Y_PORT_EG4_LEVEL1);
    patch->eg4.shape2         = get_value_from_combo(Y_PORT_EG4_SHAPE2);
    patch->eg4.time2          = get_value_from_knob(Y_PORT_EG4_TIME2);
    patch->eg4.level2         = get_value_from_knob(Y_PORT_EG4_LEVEL2);
    patch->eg4.shape3         = get_value_from_combo(Y_PORT_EG4_SHAPE3);
    patch->eg4.time3          = get_value_from_knob(Y_PORT_EG4_TIME3);
    patch->eg4.level3         = get_value_from_knob(Y_PORT_EG4_LEVEL3);
    patch->eg4.shape4         = get_value_from_combo(Y_PORT_EG4_SHAPE4);
    patch->eg4.time4          = get_value_from_knob(Y_PORT_EG4_TIME4);
    patch->eg4.vel_level_sens = get_value_from_knob(Y_PORT_EG4_VEL_LEVEL_SENS);
    patch->eg4.vel_time_scale = get_value_from_knob(Y_PORT_EG4_VEL_TIME_SCALE);
    patch->eg4.kbd_time_scale = get_value_from_knob(Y_PORT_EG4_KBD_TIME_SCALE);
    patch->eg4.amp_mod_src    = get_value_from_combo(Y_PORT_EG4_AMP_MOD_SRC);
    patch->eg4.amp_mod_amt    = get_value_from_knob(Y_PORT_EG4_AMP_MOD_AMT);

    patch->modmix_bias        = get_value_from_knob(Y_PORT_MODMIX_BIAS);
    patch->modmix_mod1_src    = get_value_from_combo(Y_PORT_MODMIX_MOD1_SRC);
    patch->modmix_mod1_amt    = get_value_from_knob(Y_PORT_MODMIX_MOD1_AMT);
    patch->modmix_mod2_src    = get_value_from_combo(Y_PORT_MODMIX_MOD2_SRC);
    patch->modmix_mod2_amt    = get_value_from_knob(Y_PORT_MODMIX_MOD2_AMT);

    strncpy(patch->name, gtk_entry_get_text(GTK_ENTRY(name_entry)), 30);
    patch->name[30] = 0;
    /* trim trailing spaces */
    i = strlen(patch->name);
    while(i && patch->name[i - 1] == ' ') i--;
    patch->name[i] = 0;

    strncpy(patch->comment, gtk_entry_get_text(GTK_ENTRY(comment_entry)), 60);
    patch->comment[60] = 0;
    /* trim trailing spaces */
    i = strlen(patch->comment);
    while(i && patch->comment[i - 1] == ' ') i--;
    patch->comment[i] = 0;
}

void
update_load(const char *value)
{
    char *path;

    GDB_MESSAGE(GDB_OSC, " update_load: received request to load '%s'\n", value);

    path = y_data_locate_patch_file(value, project_directory);

    if (path) {

        char *message;
        int result = gui_data_load(path, 0, &message);

        if (result) {

            /* successfully loaded at least one patch */
            patch_count = result;
            patches_dirty = 0;
            rebuild_patches_list();

            /* clean up old temporary file, if any */
            if (last_configure_load_was_from_tmp) {
                unlink(patches_tmp_filename);
            }
            last_configure_load_was_from_tmp = 0;

            /* -FIX- could set file chooser paths */

            /* Note: if the project directory was used to find the patch file,
             * this does not inform the user here (other than the path widget
             * displaying the substituted path), because the plugin has probably
             * passed an error to the host stating the same thing. */
        } else {
            free(path);
            GDB_MESSAGE(GDB_OSC, " update_load: gui_data_load() failed with '%s'!\n", message);
            display_notice("Unable to load the patch file requested by the host!", (char *)value);
        }
        free(message);

    } else {

        GDB_MESSAGE(GDB_OSC, " update_load: could not locate patch file!\n");
        display_notice("Unable to locate the patch file requested by the host!", (char *)value);

    }
}

void
update_polyphony(const char *value)
{
    int poly = atoi(value);

    GDB_MESSAGE(GDB_OSC, ": update_polyphony called with '%s'\n", value);

    if (poly > 0 && poly < Y_MAX_POLYPHONY) {

        GTK_ADJUSTMENT(polyphony_adj)->value = (float)poly;
        /* emit "value_changed" to get the widget to redraw itself, but don't call 
         * on_polyphony_change(): */
        g_signal_handlers_block_by_func(polyphony_adj, on_polyphony_change, NULL);
        gtk_signal_emit_by_name (GTK_OBJECT (polyphony_adj), "value_changed");
        g_signal_handlers_unblock_by_func(polyphony_adj, on_polyphony_change, NULL);
    }
}

void
update_monophonic(const char *value)
{
    int index;

    GDB_MESSAGE(GDB_OSC, ": update_monophonic called with '%s'\n", value);

    if (!strcmp(value, "off")) {
        index = 0;
    } else if (!strcmp(value, "on")) {
        index = 1;
    } else if (!strcmp(value, "once")) {
        index = 2;
    } else if (!strcmp(value, "both")) {
        index = 3;
    } else {
        return;
    }

    gtk_option_menu_set_history(GTK_OPTION_MENU (monophonic_option_menu),
                                index);  /* updates optionmenu current selection,
                                          * without needing to send it a signal */
}

void
update_glide(const char *value)
{
    int index;

    GDB_MESSAGE(GDB_OSC, ": update_glide called with '%s'\n", value);

    if (!strcmp(value, "legato")) {
        index = 0;
    } else if (!strcmp(value, "initial")) {
        index = 1;
    } else if (!strcmp(value, "always")) {
        index = 2;
    } else if (!strcmp(value, "leftover")) {
        index = 3;
    } else if (!strcmp(value, "off")) {
        index = 4;
    } else {
        return;
    }

    gtk_option_menu_set_history(GTK_OPTION_MENU (glide_option_menu),
                                index);  /* updates optionmenu current selection,
                                          * without needing to send it a signal */
}

void
update_program_cancel(const char *value)
{
    GDB_MESSAGE(GDB_OSC, ": update_program_cancel called with '%s'\n", value);

    /* update the widget, but don't call on_program_cancel_toggled(): */
    g_signal_handlers_block_by_func(program_cancel_button, on_program_cancel_toggled, NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(program_cancel_button),
                                 strcmp(value, "off") ? 1 : 0);
    g_signal_handlers_unblock_by_func(program_cancel_button, on_program_cancel_toggled, NULL);
}

void
update_project_directory(const char *value)
{
    GDB_MESSAGE(GDB_OSC, ": update_project_directory called with '%s'\n", value);

        if (project_directory) {
            gtk_file_chooser_remove_shortcut_folder(GTK_FILE_CHOOSER(open_file_chooser),
                                                    project_directory, NULL);
            gtk_file_chooser_remove_shortcut_folder(GTK_FILE_CHOOSER(save_file_chooser),
                                                    project_directory, NULL);
            gtk_file_chooser_remove_shortcut_folder(GTK_FILE_CHOOSER(import_file_chooser),
                                                    project_directory, NULL);
            free(project_directory);
        }
        project_directory = strdup(value);
        gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(open_file_chooser),
                                             project_directory, NULL);
        gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(save_file_chooser),
                                             project_directory, NULL);
        gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(import_file_chooser),
                                             project_directory, NULL);
}

void
rebuild_patches_list(void)
{
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(patches_list)));
    GtkTreeSortable *sortable = GTK_TREE_SORTABLE(store);
    GtkTreeIter iter;
    gint sort_column_id;
    GtkSortType order;
    int i;

    GDB_MESSAGE(GDB_GUI, ": rebuild_patches_list called\n");

    /* removed store from view to prevent updating on each append */
    g_object_ref(store);
    gtk_tree_view_set_model(GTK_TREE_VIEW(patches_list), NULL);
    /* turn off sorting while rebuilding */
    gtk_tree_sortable_get_sort_column_id(sortable, &sort_column_id, &order);
    gtk_tree_sortable_set_sort_column_id(sortable, GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                                         GTK_SORT_ASCENDING);
    gtk_list_store_clear(store);
    if (patch_count == 0) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           PATCHES_LIST_COL_NUMBER, 0,
                           PATCHES_LIST_COL_CATEGORY, "",
                           PATCHES_LIST_COL_NAME, "default voice",
                           -1);
    } else {
        for (i = 0; i < patch_count; i++) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               PATCHES_LIST_COL_NUMBER, i,
                               PATCHES_LIST_COL_CATEGORY, "x",
                               PATCHES_LIST_COL_NAME, patches[i].name,
                               -1);
        }
    }
    /* restore sort order */
    gtk_tree_sortable_set_sort_column_id(sortable, sort_column_id, order);
    /* add store back to view */
    gtk_tree_view_set_model(GTK_TREE_VIEW(patches_list), GTK_TREE_MODEL(store));
    g_object_unref(store);
}

void
update_port_wavetable_counts(void)
{
    int port;

    wave_tables_set_count();

    for (port = 0; port < Y_PORTS_COUNT; port++) {
        if (y_port_description[port].type == Y_PORT_TYPE_COMBO &&
            (y_port_description[port].subtype == Y_COMBO_TYPE_OSC_WAVEFORM ||
             y_port_description[port].subtype == Y_COMBO_TYPE_WT_WAVEFORM))
            y_port_description[port].upper_bound = (float)wavetables_count - 1;
    }
}

