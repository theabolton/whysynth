/* WhySynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004-2008, 2010 Sean Bolton and others.
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

#ifndef _GUI_CALLBACKS_H
#define _GUI_CALLBACKS_H

#include <gtk/gtk.h>

#include "whysynth_types.h"

void on_menu_open_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_save_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_import_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_quit_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_edit_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_about_activate(GtkMenuItem *menuitem, gpointer user_data);
gint on_delete_event_wrapper(GtkWidget *widget, GdkEvent *event,
                             gpointer data);
void on_open_file_chooser_response(GtkDialog *dialog, gint response, gpointer data);
void on_position_change(GtkWidget *widget, gpointer data);
void on_save_file_chooser_response(GtkDialog *dialog, gint response, gpointer data);
void on_save_file_range_change(GtkWidget *widget, gpointer data);
void on_import_file_chooser_response(GtkDialog *dialog, gint response, gpointer data);
void on_about_dismiss(GtkWidget *widget, gpointer data);
gint patches_list_sort_func(GtkTreeModel *model, GtkTreeIter *a,
                            GtkTreeIter *b, gpointer data);
void on_patches_selection_changed(GtkTreeSelection *treeselection, gpointer data);
void on_patches_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                              GtkTreeViewColumn *column, gpointer data);
void on_voice_knob_change(GtkWidget *widget, gpointer data);
void on_voice_knob_zero(GtkWidget *widget, gpointer data);
void on_voice_detent_change(GtkWidget *widget, gpointer data);
void on_voice_onoff_toggled(GtkWidget *widget, gpointer data);
void on_voice_combo_change(GtkWidget *widget, gpointer data);
void on_voice_element_copy(GtkWidget *widget, gpointer data);
void on_voice_element_paste(GtkWidget *widget, gpointer data);
void on_test_note_slider_change(GtkWidget *widget, gpointer data);
void on_test_note_mode_toggled(GtkWidget *widget, gpointer data);
void release_test_note(void);
void on_test_note_button_press(GtkWidget *widget, gpointer data);
void on_test_note_toggle_toggled(GtkWidget *widget, gpointer data);
void on_edit_action_button_press(GtkWidget *widget, gpointer data);
void on_edit_save_position_ok(GtkWidget *widget, gpointer data);
void on_edit_save_position_cancel(GtkWidget *widget, gpointer data);
void on_edit_close(GtkWidget *widget, gpointer data);
void on_tuning_change(GtkWidget *widget, gpointer data);
void on_polyphony_change(GtkWidget *widget, gpointer data);
void on_mono_mode_activate(GtkWidget *widget, gpointer data);
void on_glide_mode_activate(GtkWidget *widget, gpointer data);
void on_program_cancel_toggled(GtkWidget *widget, gpointer data);
void display_notice(char *message1, char *message2);
void on_notice_dismiss(GtkWidget *widget, gpointer data);
void update_osc_layout_on_mode_change(int mode_port);
void update_vcf_layout_on_mode_change(int mode_port);
void update_effect_layout_on_mode_change(void);
void update_eg_layout_on_mode_change(int mode_port);
void update_voice_widget(int port, float value, int send_OSC);
void update_voice_widgets_from_patch(y_patch_t *patch);
void update_from_program_select(int program);
float get_value_from_knob(int index);
int  get_value_from_detent(int index);
int  get_value_from_combo(int index);
void update_patch_from_voice_widgets(y_patch_t *patch);
void update_load(const char *value);
void update_polyphony(const char *value);
void update_monophonic(const char *value);
void update_glide(const char *value);
void update_program_cancel(const char *value);
void update_project_directory(const char *value);
void rebuild_patches_list(void);
void update_port_wavetable_counts(void);

#endif  /* _GUI_CALLBACKS_H */

