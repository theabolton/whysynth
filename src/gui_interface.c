/* WhySynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004-2017 Sean Bolton
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "gtkknob.h"

#include "whysynth.h"
#include "whysynth_ports.h"
#include "gui_callbacks.h"
#include "gui_interface.h"
#include "gui_images.h"
#include "agran_oscillator.h"
#include "wave_tables.h"

GtkWidget *main_window;
GtkObject *main_test_note_key_adj;
GtkObject *main_test_note_velocity_adj;

GtkWidget *patches_list;

GtkWidget *about_window;
GtkWidget *about_label;

GtkWidget *open_file_chooser;
GtkObject *open_file_position_spin_adj;
GtkWidget *open_file_position_name_label;

GtkWidget *save_file_chooser;
GtkObject *save_file_start_spin_adj;
GtkWidget *save_file_start_name;
GtkObject *save_file_end_spin_adj;
GtkWidget *save_file_end_name;
GtkWidget *save_file_mode_combo;

GtkWidget *import_file_chooser;
GtkObject *import_file_position_spin_adj;
GtkWidget *import_file_position_name_label;
GtkWidget *import_file_position_dual_button;

GtkWidget *notice_window;
GtkWidget *notice_label_1;
GtkWidget *notice_label_2;

GtkWidget *edit_window;
GtkObject *edit_test_note_key_adj;
GtkObject *edit_test_note_velocity_adj;
GtkWidget *edit_test_note_button;
GtkWidget *edit_test_note_toggle;

GtkWidget *edit_save_position_window;
GtkObject *edit_save_position_spin_adj;
GtkWidget *edit_save_position_name_label;

GtkWidget *name_entry;
GtkWidget *category_entry;
GtkWidget *comment_entry;

GtkObject *tuning_adj;
GtkObject *polyphony_adj;
GtkWidget *monophonic_option_menu;
GtkWidget *glide_option_menu;
GtkWidget *program_cancel_button;

struct voice_widgets voice_widgets[Y_PORTS_COUNT];

GtkTreeStore *combomodel[Y_COMBOMODEL_TYPE_COUNT];
GQuark combomodel_id_to_path_quark;

static GtkCellRenderer *combo_renderer;
GQuark combo_value_quark;
GQuark combo_combomodel_type_quark;

static char *
make_window_title(const char *tag, const char *text)
{
    char *title = (char *)malloc(strlen(tag) + strlen(text) + 2);
    sprintf(title, "%s %s", tag, text);
    return title;
}

static void
set_window_title(GtkWidget *window, const char *tag, const char *text)
{
    char *title = make_window_title(tag, text);
    gtk_window_set_title (GTK_WINDOW (window), title);
    free(title);
}

static GtkWidget *
patches_list_new(void)
{
    GtkListStore *store = gtk_list_store_new(PATCHES_LIST_COLUMNS,
                                             G_TYPE_UINT,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING);
    GtkTreeSortable   *sortable = GTK_TREE_SORTABLE(store);
    GtkWidget         *view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;

    gtk_tree_sortable_set_sort_func(sortable, PATCHES_LIST_COL_NUMBER, patches_list_sort_func,
                                    GINT_TO_POINTER(PATCHES_LIST_COL_NUMBER), NULL);
    gtk_tree_sortable_set_sort_func(sortable, PATCHES_LIST_COL_CATEGORY, patches_list_sort_func,
                                    GINT_TO_POINTER(PATCHES_LIST_COL_CATEGORY), NULL);
    gtk_tree_sortable_set_sort_func(sortable, PATCHES_LIST_COL_NAME, patches_list_sort_func,
                                    GINT_TO_POINTER(PATCHES_LIST_COL_NAME), NULL);
    gtk_tree_sortable_set_sort_column_id(sortable, PATCHES_LIST_COL_NUMBER, GTK_SORT_ASCENDING);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_renderer_set_alignment(renderer, 1.0, 0.5);
    column = gtk_tree_view_column_new_with_attributes("ProgNo", renderer,
                                                      "text", PATCHES_LIST_COL_NUMBER, NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(view), column, -1);
    gtk_tree_view_column_set_sort_column_id(column, PATCHES_LIST_COL_NUMBER);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Category", renderer,
                                                      "text", PATCHES_LIST_COL_CATEGORY, NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(view), column, -1);
    gtk_tree_view_column_set_sort_column_id(column, PATCHES_LIST_COL_CATEGORY);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Name", renderer,
                                                      "text", PATCHES_LIST_COL_NAME, NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(view), column, -1);
    gtk_tree_view_column_set_sort_column_id(column, PATCHES_LIST_COL_NAME);

    gtk_widget_show_all(view);

    return view;
}

void
create_main_window (const char *tag)
{
    GtkWidget *vbox1;
    GtkWidget *menubar1;
    GtkWidget *file1;
    GtkWidget *file1_menu;
    GtkWidget *menu_open;
    GtkWidget *menu_save;
    GtkWidget *menu_import_xsynth;
    GtkWidget *menu_import_k4;
    GtkWidget *separator1;
    GtkWidget *menu_quit;
    GtkWidget *edit1;
    GtkWidget *edit1_menu;
    GtkWidget *menu_edit;
    GtkWidget *help1;
    GtkWidget *help1_menu;
    GtkWidget *menu_about;
    GtkWidget *notebook1;
    GtkWidget *scrolledwindow1;
    GtkWidget *patches_tab_label;
    GtkWidget *test_note_frame;
    GtkWidget *test_note_table;
    GtkWidget *test_note_key_label;
    GtkWidget *test_note_velocity_label;
    GtkWidget *test_note_key;
    GtkWidget *test_note_velocity;
    GtkWidget *test_note_button;
    GtkWidget *label54;
    GtkWidget *frame14;
    GtkWidget *configuration_table;
    GtkWidget *tuning_spin;
    GtkWidget *polyphony;
    GtkWidget *mono_mode_off;
    GtkWidget *mono_mode_on;
    GtkWidget *mono_mode_once;
    GtkWidget *mono_mode_both;
    GtkWidget *optionmenu5_menu;
    GtkWidget *glide_mode_label;
    GtkWidget *glide_mode_legato;
    GtkWidget *glide_mode_initial;
    GtkWidget *glide_mode_always;
  GtkWidget *glide_mode_leftover;
  GtkWidget *glide_mode_off;
    GtkWidget *glide_menu;
    GtkWidget *label43;
    GtkWidget *label44;
    GtkWidget *label45;
  GtkWidget *frame15;
  GtkWidget *logo_pixmap;
  GtkWidget *label47;
  GtkWidget *configuration_tab_label;
    GtkAccelGroup *accel_group;
    GdkPixbuf *icon;

    if ((icon = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                         "whysynth", 32, 0, NULL)) != NULL) {
        gtk_window_set_default_icon(icon);
        g_object_unref(icon);
    }

    accel_group = gtk_accel_group_new ();

    main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (main_window), "main_window", main_window);
    gtk_window_set_title (GTK_WINDOW (main_window), tag);
    gtk_widget_realize(main_window);  /* window must be realized for create_*_pixmap() */

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (main_window), vbox1);

  menubar1 = gtk_menu_bar_new ();
  gtk_widget_ref (menubar1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menubar1", menubar1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menubar1);
  gtk_box_pack_start (GTK_BOX (vbox1), menubar1, FALSE, FALSE, 0);

  file1 = gtk_menu_item_new_with_label ("File");
  gtk_widget_ref (file1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "file1", file1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (file1);
  gtk_container_add (GTK_CONTAINER (menubar1), file1);

  file1_menu = gtk_menu_new ();
  gtk_widget_ref (file1_menu);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "file1_menu", file1_menu,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (file1), file1_menu);

  menu_open = gtk_menu_item_new_with_label ("Open Patch Bank...");
  gtk_widget_ref (menu_open);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_open", menu_open,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menu_open);
  gtk_container_add (GTK_CONTAINER (file1_menu), menu_open);
  gtk_widget_add_accelerator (menu_open, "activate", accel_group,
                              GDK_O, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  menu_save = gtk_menu_item_new_with_label ("Save Patch Bank...");
  gtk_widget_ref (menu_save);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_save", menu_save,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menu_save);
  gtk_container_add (GTK_CONTAINER (file1_menu), menu_save);
  gtk_widget_add_accelerator (menu_save, "activate", accel_group,
                              GDK_S, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  separator1 = gtk_menu_item_new ();
  gtk_widget_ref (separator1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "separator1", separator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (separator1);
  gtk_container_add (GTK_CONTAINER (file1_menu), separator1);
  gtk_widget_set_sensitive (separator1, FALSE);

  menu_import_xsynth = gtk_menu_item_new_with_label ("Import Xsynth-DSSI Patches...");
  gtk_widget_ref (menu_import_xsynth);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_import_xsynth", menu_import_xsynth,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menu_import_xsynth);
  gtk_container_add (GTK_CONTAINER (file1_menu), menu_import_xsynth);

  menu_import_k4 = gtk_menu_item_new_with_label ("(Mis)Interpret K4 Patches...");
  gtk_widget_ref (menu_import_k4);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_import_k4", menu_import_k4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menu_import_k4);
  gtk_container_add (GTK_CONTAINER (file1_menu), menu_import_k4);

  separator1 = gtk_menu_item_new ();
  gtk_widget_ref (separator1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "separator1", separator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (separator1);
  gtk_container_add (GTK_CONTAINER (file1_menu), separator1);
  gtk_widget_set_sensitive (separator1, FALSE);

  menu_quit = gtk_menu_item_new_with_label ("Quit");
  gtk_widget_ref (menu_quit);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_quit", menu_quit,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menu_quit);
  gtk_container_add (GTK_CONTAINER (file1_menu), menu_quit);
  gtk_widget_add_accelerator (menu_quit, "activate", accel_group,
                              GDK_Q, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  edit1 = gtk_menu_item_new_with_label ("Edit");
  gtk_widget_ref (edit1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "edit1", edit1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (edit1);
  gtk_container_add (GTK_CONTAINER (menubar1), edit1);
  gtk_menu_item_right_justify (GTK_MENU_ITEM (edit1));

  edit1_menu = gtk_menu_new ();
  gtk_widget_ref (edit1_menu);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "edit1_menu", edit1_menu,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (edit1), edit1_menu);

  menu_edit = gtk_menu_item_new_with_label ("Edit Patch...");
  gtk_widget_ref (menu_edit);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_edit", menu_edit,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menu_edit);
  gtk_container_add (GTK_CONTAINER (edit1_menu), menu_edit);
  gtk_widget_add_accelerator (menu_edit, "activate", accel_group,
                              GDK_E, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  help1 = gtk_menu_item_new_with_label ("About");
  gtk_widget_ref (help1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "help1", help1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (help1);
  gtk_container_add (GTK_CONTAINER (menubar1), help1);
  gtk_menu_item_right_justify (GTK_MENU_ITEM (help1));

  help1_menu = gtk_menu_new ();
  gtk_widget_ref (help1_menu);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "help1_menu", help1_menu,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (help1), help1_menu);

  menu_about = gtk_menu_item_new_with_label ("About WhySynth");
  gtk_widget_ref (menu_about);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_about", menu_about,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menu_about);
  gtk_container_add (GTK_CONTAINER (help1_menu), menu_about);

  notebook1 = gtk_notebook_new ();
  gtk_widget_ref (notebook1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "notebook1", notebook1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notebook1);
  gtk_box_pack_start (GTK_BOX (vbox1), notebook1, TRUE, TRUE, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "scrolledwindow1", scrolledwindow1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_container_add (GTK_CONTAINER (notebook1), scrolledwindow1);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  patches_list = patches_list_new();
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), patches_list);

  patches_tab_label = gtk_label_new ("Patches");
  gtk_widget_ref (patches_tab_label);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "patches_tab_label", patches_tab_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (patches_tab_label);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 0), patches_tab_label);

    /* Configuration tab */
  frame14 = gtk_frame_new ("Configuration");
  gtk_widget_ref (frame14);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame14", frame14,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame14);
  gtk_container_add (GTK_CONTAINER (notebook1), frame14);

  configuration_table = gtk_table_new (3, 6, FALSE);
  gtk_widget_ref (configuration_table);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "configuration_table", configuration_table,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (configuration_table);
  gtk_container_add (GTK_CONTAINER (frame14), configuration_table);
  gtk_container_set_border_width (GTK_CONTAINER (configuration_table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (configuration_table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (configuration_table), 5);

  label54 = gtk_label_new ("Tuning");
  gtk_widget_ref (label54);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label54", label54,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label54);
  gtk_table_attach (GTK_TABLE (configuration_table), label54, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label54), 0, 0.5);

  tuning_adj = gtk_adjustment_new (440, 415.3, 467.2, 0.1, 1, 0);
  tuning_spin = gtk_spin_button_new (GTK_ADJUSTMENT (tuning_adj), 1, 1);
  gtk_widget_ref (tuning_spin);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "tuning_spin", tuning_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (tuning_spin);
  gtk_table_attach (GTK_TABLE (configuration_table), tuning_spin, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (tuning_spin), TRUE);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (tuning_spin), GTK_UPDATE_IF_VALID);

  label43 = gtk_label_new ("Polyphony");
  gtk_widget_ref (label43);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label43", label43,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label43);
  gtk_table_attach (GTK_TABLE (configuration_table), label43, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label43), 0, 0.5);

  polyphony_adj = gtk_adjustment_new (Y_DEFAULT_POLYPHONY, 1, Y_MAX_POLYPHONY, 1, 10, 0);
  polyphony = gtk_spin_button_new (GTK_ADJUSTMENT (polyphony_adj), 1, 0);
  gtk_widget_ref (polyphony);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "polyphony", polyphony,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (polyphony);
  gtk_table_attach (GTK_TABLE (configuration_table), polyphony, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label44 = gtk_label_new ("Monophonic Mode");
  gtk_widget_ref (label44);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label44", label44,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label44);
  gtk_table_attach (GTK_TABLE (configuration_table), label44, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label44), 0, 0.5);

    monophonic_option_menu = gtk_option_menu_new ();
    gtk_widget_ref (monophonic_option_menu);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "monophonic_option_menu", monophonic_option_menu,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (monophonic_option_menu);
    gtk_table_attach (GTK_TABLE (configuration_table), monophonic_option_menu, 1, 2, 2, 3,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    optionmenu5_menu = gtk_menu_new ();
    mono_mode_off = gtk_menu_item_new_with_label ("Off");
    gtk_widget_show (mono_mode_off);
    gtk_menu_append (GTK_MENU (optionmenu5_menu), mono_mode_off);
    mono_mode_on = gtk_menu_item_new_with_label ("On");
    gtk_widget_show (mono_mode_on);
    gtk_menu_append (GTK_MENU (optionmenu5_menu), mono_mode_on);
    mono_mode_once = gtk_menu_item_new_with_label ("Once");
    gtk_widget_show (mono_mode_once);
    gtk_menu_append (GTK_MENU (optionmenu5_menu), mono_mode_once);
    mono_mode_both = gtk_menu_item_new_with_label ("Both");
    gtk_widget_show (mono_mode_both);
    gtk_menu_append (GTK_MENU (optionmenu5_menu), mono_mode_both);
    gtk_option_menu_set_menu (GTK_OPTION_MENU (monophonic_option_menu), optionmenu5_menu);

    glide_mode_label = gtk_label_new ("Glide Mode");
    gtk_widget_ref (glide_mode_label);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "glide_mode_label", glide_mode_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (glide_mode_label);
    gtk_table_attach (GTK_TABLE (configuration_table), glide_mode_label, 0, 1, 3, 4,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (glide_mode_label), 0, 0.5);

    glide_option_menu = gtk_option_menu_new ();
    gtk_widget_ref (glide_option_menu);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "glide_option_menu", glide_option_menu,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (glide_option_menu);
    gtk_table_attach (GTK_TABLE (configuration_table), glide_option_menu, 1, 2, 3, 4,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    glide_menu = gtk_menu_new ();
    glide_mode_legato = gtk_menu_item_new_with_label ("Legato Only");
    gtk_widget_show (glide_mode_legato);
    gtk_menu_append (GTK_MENU (glide_menu), glide_mode_legato);
    glide_mode_initial = gtk_menu_item_new_with_label ("Non-legato Only");
    gtk_widget_show (glide_mode_initial);
    gtk_menu_append (GTK_MENU (glide_menu), glide_mode_initial);
    glide_mode_always = gtk_menu_item_new_with_label ("Always");
    gtk_widget_show (glide_mode_always);
    gtk_menu_append (GTK_MENU (glide_menu), glide_mode_always);
    glide_mode_leftover = gtk_menu_item_new_with_label ("Leftover");
    gtk_widget_show (glide_mode_leftover);
    gtk_menu_append (GTK_MENU (glide_menu), glide_mode_leftover);
    glide_mode_off = gtk_menu_item_new_with_label ("Off");
    gtk_widget_show (glide_mode_off);
    gtk_menu_append (GTK_MENU (glide_menu), glide_mode_off);
    gtk_option_menu_set_menu (GTK_OPTION_MENU (glide_option_menu), glide_menu);

  label45 = gtk_label_new ("Cancel Notes On\n"
                           "Program Change");
  gtk_widget_ref (label45);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label45", label45,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label45);
  gtk_table_attach (GTK_TABLE (configuration_table), label45, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label45), 0, 0.5);

    program_cancel_button = gtk_check_button_new ();
    gtk_widget_ref (program_cancel_button);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "program_cancel_button", program_cancel_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(program_cancel_button), TRUE);
    gtk_widget_show (program_cancel_button);
    gtk_table_attach (GTK_TABLE (configuration_table), program_cancel_button, 1, 2, 4, 5,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

  frame15 = gtk_frame_new (NULL);
  gtk_widget_ref (frame15);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame15", frame15,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame15);
  gtk_table_attach (GTK_TABLE (configuration_table), frame15, 0, 3, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);

  logo_pixmap = create_logo_pixmap (main_window);
  gtk_widget_ref (logo_pixmap);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "logo_pixmap", logo_pixmap,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (logo_pixmap);
  gtk_container_add (GTK_CONTAINER (frame15), logo_pixmap);
  gtk_misc_set_padding (GTK_MISC (logo_pixmap), 2, 2);

  label47 = gtk_label_new ("     ");
  gtk_widget_ref (label47);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label47", label47,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label47);
  gtk_table_attach (GTK_TABLE (configuration_table), label47, 2, 3, 0, 4,
                    (GtkAttachOptions) (GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label47), 0, 0.5);

    configuration_tab_label = gtk_label_new ("Configuration");
    gtk_widget_ref (configuration_tab_label);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "configuration_tab_label", configuration_tab_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (configuration_tab_label);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 1), configuration_tab_label);

    test_note_frame = gtk_frame_new ("Test Note");
    gtk_widget_ref (test_note_frame);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "main test_note_frame", test_note_frame,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_frame);
    gtk_container_set_border_width (GTK_CONTAINER (test_note_frame), 5);

    test_note_table = gtk_table_new (3, 3, FALSE);
    gtk_widget_ref (test_note_table);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "test_note_table", test_note_table,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_table);
    gtk_container_add (GTK_CONTAINER (test_note_frame), test_note_table);
    gtk_container_set_border_width (GTK_CONTAINER (test_note_table), 2);
    gtk_table_set_row_spacings (GTK_TABLE (test_note_table), 1);
    gtk_table_set_col_spacings (GTK_TABLE (test_note_table), 5);
  
    test_note_key_label = gtk_label_new ("key");
    gtk_widget_ref (test_note_key_label);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "main test_note_key_label", test_note_key_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_key_label);
    gtk_table_attach (GTK_TABLE (test_note_table), test_note_key_label, 0, 1, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (test_note_key_label), 0, 0.5);
  
    test_note_velocity_label = gtk_label_new ("velocity");
    gtk_widget_ref (test_note_velocity_label);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "main test_note_velocity_label", test_note_velocity_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_velocity_label);
    gtk_table_attach (GTK_TABLE (test_note_table), test_note_velocity_label, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (test_note_velocity_label), 0, 0.5);
  
    test_note_button = gtk_button_new_with_label ("Send Test Note");
    gtk_widget_ref (test_note_button);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "main_test_note_button", test_note_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_button);
    gtk_table_attach (GTK_TABLE (test_note_table), test_note_button, 2, 3, 0, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 4, 0);

    main_test_note_key_adj = gtk_adjustment_new (60, 12, 120, 1, 12, 12);
    test_note_key = gtk_hscale_new (GTK_ADJUSTMENT (main_test_note_key_adj));
    gtk_widget_ref (test_note_key);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "main_test_note_key", test_note_key,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_key);
    gtk_table_attach (GTK_TABLE (test_note_table), test_note_key, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_scale_set_value_pos (GTK_SCALE (test_note_key), GTK_POS_RIGHT);
    gtk_scale_set_digits (GTK_SCALE (test_note_key), 0);
    gtk_range_set_update_policy (GTK_RANGE (test_note_key), GTK_UPDATE_DELAYED);

    main_test_note_velocity_adj = gtk_adjustment_new (96, 1, 137, 1, 10, 10);
    test_note_velocity = gtk_hscale_new (GTK_ADJUSTMENT (main_test_note_velocity_adj));
    gtk_widget_ref (test_note_velocity);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "main_test_note_velocity", test_note_velocity,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_velocity);
    gtk_table_attach (GTK_TABLE (test_note_table), test_note_velocity, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_scale_set_value_pos (GTK_SCALE (test_note_velocity), GTK_POS_RIGHT);
    gtk_scale_set_digits (GTK_SCALE (test_note_velocity), 0);
    gtk_range_set_update_policy (GTK_RANGE (test_note_velocity), GTK_UPDATE_DELAYED);

    gtk_box_pack_start (GTK_BOX (vbox1), test_note_frame, FALSE, FALSE, 0);

    gtk_signal_connect(GTK_OBJECT(main_window), "destroy",
                       GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (main_window), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_menu_quit_activate);

    gtk_signal_connect (GTK_OBJECT (menu_open), "activate",
                        GTK_SIGNAL_FUNC (on_menu_open_activate),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (menu_import_xsynth), "activate",
                        GTK_SIGNAL_FUNC (on_menu_import_activate),
                        (gpointer)"xsynth");
    gtk_signal_connect (GTK_OBJECT (menu_import_k4), "activate",
                        GTK_SIGNAL_FUNC (on_menu_import_activate),
                        (gpointer)"k4");
    gtk_signal_connect (GTK_OBJECT (menu_save), "activate",
                        GTK_SIGNAL_FUNC (on_menu_save_activate),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (menu_quit), "activate",
                        GTK_SIGNAL_FUNC (on_menu_quit_activate),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (menu_edit), "activate",
                        GTK_SIGNAL_FUNC (on_menu_edit_activate),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (menu_about), "activate",
                        GTK_SIGNAL_FUNC (on_menu_about_activate),
                        NULL);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(patches_list))), "changed",
                     G_CALLBACK(on_patches_selection_changed),
                     NULL); /* single-click */
    g_signal_connect(G_OBJECT(patches_list), "row-activated",
                     G_CALLBACK(on_patches_row_activated),
                     NULL); /* double-click */

    /* connect test note widgets */
    gtk_signal_connect (GTK_OBJECT (main_test_note_key_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_test_note_slider_change),
                        GINT_TO_POINTER(0));
    gtk_signal_connect (GTK_OBJECT (main_test_note_velocity_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_test_note_slider_change),
                        GINT_TO_POINTER(1));
    gtk_signal_connect (GTK_OBJECT (test_note_button), "pressed",
                        GTK_SIGNAL_FUNC (on_test_note_button_press),
                        GINT_TO_POINTER(1));
    gtk_signal_connect (GTK_OBJECT (test_note_button), "released",
                        GTK_SIGNAL_FUNC (on_test_note_button_press),
                        GINT_TO_POINTER(0));

    /* connect synth configuration widgets */
    gtk_signal_connect (GTK_OBJECT (tuning_adj), "value_changed",
                        GTK_SIGNAL_FUNC(on_tuning_change),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (polyphony))),
                        "value_changed", GTK_SIGNAL_FUNC(on_polyphony_change),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (mono_mode_off), "activate",
                        GTK_SIGNAL_FUNC (on_mono_mode_activate),
                        (gpointer)"off");
    gtk_signal_connect (GTK_OBJECT (mono_mode_on), "activate",
                        GTK_SIGNAL_FUNC (on_mono_mode_activate),
                        (gpointer)"on");
    gtk_signal_connect (GTK_OBJECT (mono_mode_once), "activate",
                        GTK_SIGNAL_FUNC (on_mono_mode_activate),
                        (gpointer)"once");
    gtk_signal_connect (GTK_OBJECT (mono_mode_both), "activate",
                        GTK_SIGNAL_FUNC (on_mono_mode_activate),
                        (gpointer)"both");
    gtk_signal_connect (GTK_OBJECT (glide_mode_legato), "activate",
                        GTK_SIGNAL_FUNC (on_glide_mode_activate),
                        (gpointer)"legato");
    gtk_signal_connect (GTK_OBJECT (glide_mode_initial), "activate",
                        GTK_SIGNAL_FUNC (on_glide_mode_activate),
                        (gpointer)"initial");
    gtk_signal_connect (GTK_OBJECT (glide_mode_always), "activate",
                        GTK_SIGNAL_FUNC (on_glide_mode_activate),
                        (gpointer)"always");
    gtk_signal_connect (GTK_OBJECT (glide_mode_leftover), "activate",
                        GTK_SIGNAL_FUNC (on_glide_mode_activate),
                        (gpointer)"leftover");
    gtk_signal_connect (GTK_OBJECT (glide_mode_off), "activate",
                        GTK_SIGNAL_FUNC (on_glide_mode_activate),
                        (gpointer)"off");
    gtk_signal_connect (GTK_OBJECT (program_cancel_button), "toggled",
                        GTK_SIGNAL_FUNC (on_program_cancel_toggled),
                        NULL);

    gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_group);
}

void
create_about_window (const char *tag)
{
    GtkWidget *frame1;
    GtkWidget *vbox2;
    GtkWidget *about_pixmap;
    GtkWidget *closeabout;

    about_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (about_window), "about_window", about_window);
    gtk_window_set_title (GTK_WINDOW (about_window), "About WhySynth");
    gtk_widget_realize(about_window);  /* window must be realized for create_about_pixmap() */

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox2);
    gtk_object_set_data_full (GTK_OBJECT (about_window), "vbox2", vbox2,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox2);
    gtk_container_add (GTK_CONTAINER (about_window), vbox2);

    frame1 = gtk_frame_new (NULL);
    gtk_widget_ref (frame1);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "frame1", frame1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (frame1);
    gtk_box_pack_start (GTK_BOX (vbox2), frame1, FALSE, FALSE, 0);

    about_pixmap = (GtkWidget *)create_about_pixmap (about_window);
    gtk_widget_ref (about_pixmap);
    gtk_object_set_data_full (GTK_OBJECT (about_window), "about_pixmap", about_pixmap,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (about_pixmap);
    gtk_container_add (GTK_CONTAINER (frame1), about_pixmap);
    gtk_misc_set_padding (GTK_MISC (about_pixmap), 5, 5);

    about_label = gtk_label_new ("Some message\ngoes here");
    gtk_widget_ref (about_label);
    gtk_object_set_data_full (GTK_OBJECT (about_window), "about_label", about_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (about_label);
    gtk_box_pack_start (GTK_BOX (vbox2), about_label, FALSE, FALSE, 0);
    // gtk_label_set_line_wrap (GTK_LABEL (about_label), TRUE);
    gtk_label_set_justify (GTK_LABEL (about_label), GTK_JUSTIFY_CENTER);
    gtk_misc_set_padding (GTK_MISC (about_label), 5, 5);

    closeabout = gtk_button_new_with_label ("Dismiss");
    gtk_widget_ref (closeabout);
    gtk_object_set_data_full (GTK_OBJECT (about_window), "closeabout", closeabout,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (closeabout);
    gtk_box_pack_start (GTK_BOX (vbox2), closeabout, FALSE, FALSE, 0);

    gtk_signal_connect (GTK_OBJECT (about_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (about_window), "delete_event",
                        GTK_SIGNAL_FUNC (on_delete_event_wrapper),
                        (gpointer)on_about_dismiss);
    gtk_signal_connect (GTK_OBJECT (closeabout), "clicked",
                        GTK_SIGNAL_FUNC (on_about_dismiss),
                        NULL);
}

void
create_open_file_chooser (const char *tag)
{
    char *title = make_window_title(tag, "Open Patch Bank");
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *position_spin;

    open_file_chooser = gtk_file_chooser_dialog_new(title,
                                                    GTK_WINDOW (main_window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_OPEN,
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);
    free(title);
    /* no need to block other actions -- make window nonmodal and nontransient */
    gtk_window_set_transient_for(GTK_WINDOW (open_file_chooser), NULL);
    gtk_window_set_modal(GTK_WINDOW (open_file_chooser), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW (open_file_chooser), GDK_WINDOW_TYPE_HINT_NORMAL);
    gtk_window_set_decorated(GTK_WINDOW (open_file_chooser), TRUE);
    gtk_file_chooser_set_show_hidden (GTK_FILE_CHOOSER (open_file_chooser), TRUE);

    gtk_signal_connect (GTK_OBJECT (open_file_chooser), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (open_file_chooser), "delete_event",
                        (GtkSignalFunc)gtk_widget_hide_on_delete, NULL);
    gtk_signal_connect (GTK_OBJECT (open_file_chooser), "response", 
                        (GtkSignalFunc) on_open_file_chooser_response,
                        NULL);

    vbox = gtk_vbox_new (FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 8);

    label = gtk_label_new ("Program Number at which to begin loading:\n"
                           "(existing patches will be overwritten)");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 2);

    open_file_position_spin_adj = gtk_adjustment_new (0, 0, 511, 1, 10, 0);
    position_spin = gtk_spin_button_new (GTK_ADJUSTMENT (open_file_position_spin_adj), 1, 0);
    gtk_box_pack_start (GTK_BOX (hbox), position_spin, FALSE, FALSE, 10);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (position_spin), TRUE);

    open_file_position_name_label = gtk_label_new ("default voice");
    gtk_box_pack_start (GTK_BOX (hbox), open_file_position_name_label, FALSE, FALSE, 2);
    gtk_label_set_justify (GTK_LABEL (open_file_position_name_label), GTK_JUSTIFY_LEFT);

    gtk_widget_show_all (vbox);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(open_file_chooser), vbox);

    gtk_signal_connect (GTK_OBJECT (open_file_position_spin_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_position_change),
                        (gpointer)open_file_position_name_label);
}

void
create_save_file_chooser (const char *tag)
{
    char *title = make_window_title(tag, "Save Patch Bank");
    GtkWidget *vbox;
    GtkWidget *label4;
    GtkWidget *table;
    GtkWidget *label5;
    GtkWidget *label6;
    GtkWidget *save_file_start_spin;
    GtkWidget *save_file_end_spin;
    GtkWidget *hbox;
    GtkWidget *label;

    save_file_chooser = gtk_file_chooser_dialog_new(title,
                                                    GTK_WINDOW (main_window),
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_SAVE,
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);
    free(title);
    /* no need to block other actions -- make window nonmodal and nontransient */
    gtk_window_set_transient_for(GTK_WINDOW (save_file_chooser), NULL);
    gtk_window_set_modal(GTK_WINDOW (save_file_chooser), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW (save_file_chooser), GDK_WINDOW_TYPE_HINT_NORMAL);
    gtk_window_set_decorated(GTK_WINDOW (save_file_chooser), TRUE);
    /* perform overwrite confirmation */
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (save_file_chooser),
                                                    TRUE);
    gtk_file_chooser_set_show_hidden (GTK_FILE_CHOOSER (save_file_chooser), TRUE);

    gtk_signal_connect (GTK_OBJECT (save_file_chooser), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (save_file_chooser), "delete_event",
                        (GtkSignalFunc)gtk_widget_hide_on_delete, NULL);
    gtk_signal_connect (GTK_OBJECT (save_file_chooser), "response", 
                        (GtkSignalFunc) on_save_file_chooser_response,
                        NULL);

    vbox = gtk_vbox_new (FALSE, 0);

    label4 = gtk_label_new ("Select the Program Numbers for the range of patches you wish to save:");
    gtk_box_pack_start (GTK_BOX (vbox), label4, TRUE, TRUE, 0);
    gtk_misc_set_padding (GTK_MISC (label4), 0, 6);
    gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

    table = gtk_table_new (2, 3, FALSE);
    gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
    gtk_table_set_col_spacings (GTK_TABLE (table), 2);

    label5 = gtk_label_new ("Start Program Number");
    gtk_table_attach (GTK_TABLE (table), label5, 0, 1, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);

    label6 = gtk_label_new ("End Program (inclusive)");
    gtk_table_attach (GTK_TABLE (table), label6, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

    save_file_start_spin_adj = gtk_adjustment_new (0, 0, 511, 1, 10, 0);
    save_file_start_spin = gtk_spin_button_new (GTK_ADJUSTMENT (save_file_start_spin_adj), 1, 0);
    gtk_table_attach (GTK_TABLE (table), save_file_start_spin, 1, 2, 0, 1,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 6, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (save_file_start_spin), TRUE);

    save_file_end_spin_adj = gtk_adjustment_new (511, 0, 511, 1, 10, 0);
    save_file_end_spin = gtk_spin_button_new (GTK_ADJUSTMENT (save_file_end_spin_adj), 1, 0);
    gtk_table_attach (GTK_TABLE (table), save_file_end_spin, 1, 2, 1, 2,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (save_file_end_spin), TRUE);

    save_file_start_name = gtk_label_new ("(unset)");
    gtk_table_attach (GTK_TABLE (table), save_file_start_name, 2, 3, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify (GTK_LABEL (save_file_start_name), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment (GTK_MISC (save_file_start_name), 0, 0.5);

    save_file_end_name = gtk_label_new ("(unset)");
    gtk_table_attach (GTK_TABLE (table), save_file_end_name, 2, 3, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (save_file_end_name), 0, 0.5);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 6);

    label = gtk_label_new ("Patch File Format:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    save_file_mode_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(save_file_mode_combo),
                                   "Current (version 1)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(save_file_mode_combo),
                                   "Backward-compatible (version 0)");
#ifdef DEVELOPER
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(save_file_mode_combo), "'C' Source Code");
#endif /* DEVELOPER */
    gtk_combo_box_set_active(GTK_COMBO_BOX(save_file_mode_combo), 0);
    gtk_box_pack_start (GTK_BOX (hbox), save_file_mode_combo, FALSE, FALSE, 6);
    
    gtk_widget_show_all (vbox);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(save_file_chooser), vbox);

    gtk_signal_connect (GTK_OBJECT (save_file_start_spin_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_save_file_range_change),
                        GINT_TO_POINTER(0));
    gtk_signal_connect (GTK_OBJECT (save_file_end_spin_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_save_file_range_change),
                        GINT_TO_POINTER(1));
}

void
create_import_file_chooser (void)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *position_spin;

    import_file_chooser = gtk_file_chooser_dialog_new("WhySynth - Import Patches",
                                                    GTK_WINDOW (main_window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_OPEN,
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);
    /* no need to block other actions -- make window nonmodal and not transient */
    gtk_window_set_transient_for(GTK_WINDOW (import_file_chooser), NULL);
    gtk_window_set_modal(GTK_WINDOW (import_file_chooser), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW (import_file_chooser), GDK_WINDOW_TYPE_HINT_NORMAL);
    gtk_window_set_decorated(GTK_WINDOW (import_file_chooser), TRUE);
    gtk_file_chooser_set_show_hidden (GTK_FILE_CHOOSER (import_file_chooser), TRUE);

    gtk_signal_connect (GTK_OBJECT (import_file_chooser), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (import_file_chooser), "delete_event",
                        (GtkSignalFunc)gtk_widget_hide_on_delete, NULL);
    gtk_signal_connect (GTK_OBJECT (import_file_chooser), "response", 
                        (GtkSignalFunc) on_import_file_chooser_response,
                        NULL);

    vbox = gtk_vbox_new (FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 8);

    label = gtk_label_new ("Program Number at which to begin importing:\n"
                           "(existing patches will be overwritten)");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 2);

    import_file_position_spin_adj = gtk_adjustment_new (0, 0, 511, 1, 10, 0);
    position_spin = gtk_spin_button_new (GTK_ADJUSTMENT (import_file_position_spin_adj), 1, 0);
    gtk_box_pack_start (GTK_BOX (hbox), position_spin, FALSE, FALSE, 10);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (position_spin), TRUE);

    import_file_position_name_label = gtk_label_new ("default voice");
    gtk_box_pack_start (GTK_BOX (hbox), import_file_position_name_label, FALSE, FALSE, 2);
    gtk_label_set_justify (GTK_LABEL (import_file_position_name_label), GTK_JUSTIFY_LEFT);

    import_file_position_dual_button = gtk_check_button_new_with_label("Create 'dual' patches");
    gtk_box_pack_start (GTK_BOX (vbox), import_file_position_dual_button, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (import_file_position_dual_button), 6);

    gtk_widget_show_all (vbox);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(import_file_chooser), vbox);

    gtk_signal_connect (GTK_OBJECT (import_file_position_spin_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_position_change),
                        (gpointer)import_file_position_name_label);
}

void
create_notice_window (const char *tag)
{
  GtkWidget *vbox3;
  GtkWidget *hbox1;
  GtkWidget *notice_dismiss;

    notice_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (notice_window), "notice_window", notice_window);
    set_window_title(notice_window, tag, "Notice");
    gtk_window_set_position (GTK_WINDOW (notice_window), GTK_WIN_POS_MOUSE);
    gtk_window_set_modal (GTK_WINDOW (notice_window), TRUE);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox3);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "vbox3", vbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox3);
  gtk_container_add (GTK_CONTAINER (notice_window), vbox3);

  notice_label_1 = gtk_label_new ("Some message\ngoes here");
  gtk_widget_ref (notice_label_1);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "notice_label_1", notice_label_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notice_label_1);
  gtk_box_pack_start (GTK_BOX (vbox3), notice_label_1, TRUE, TRUE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (notice_label_1), TRUE);
  gtk_misc_set_padding (GTK_MISC (notice_label_1), 10, 5);

  notice_label_2 = gtk_label_new ("more text\ngoes here");
  gtk_widget_ref (notice_label_2);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "notice_label_2", notice_label_2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notice_label_2);
  gtk_box_pack_start (GTK_BOX (vbox3), notice_label_2, FALSE, FALSE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (notice_label_2), TRUE);
  gtk_misc_set_padding (GTK_MISC (notice_label_2), 10, 5);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox1);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "hbox1", hbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox1, FALSE, FALSE, 0);

  notice_dismiss = gtk_button_new_with_label ("Dismiss");
  gtk_widget_ref (notice_dismiss);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "notice_dismiss", notice_dismiss,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notice_dismiss);
  gtk_box_pack_start (GTK_BOX (hbox1), notice_dismiss, TRUE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (notice_dismiss), 7);

    gtk_signal_connect (GTK_OBJECT (notice_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (notice_window), "delete_event",
                        GTK_SIGNAL_FUNC (on_delete_event_wrapper),
                        (gpointer)on_notice_dismiss);
    gtk_signal_connect (GTK_OBJECT (notice_dismiss), "clicked",
                        GTK_SIGNAL_FUNC (on_notice_dismiss),
                        NULL);
}

struct y_osc_modes_t y_osc_modes[Y_OSCILLATOR_MODE_COUNT + 2] = {
   /* name                     pri  id  mparam1 top       mp1-left mp1-rt mparam2-top        mp2-lt  mp2-rt                   mmod src top          mmod amt top         */
    { "Off",                    0,  0,  NULL,             "0",     "1",   NULL,              "0",    "1",    /* Off        */ NULL,                 NULL,                },
    { "minBLEP",              'm',  1,  "Sync",           "Off",   "On",  NULL,              "0",    "1",    /* minBLEP    */ NULL,                 NULL,                },
    { "Wavecycle",            'w',  2,  "Sync",           "Off",   "On",  "Wave Sel Bias",   "0",    "+60",  /* Wavecycle  */ NULL,                 NULL,                },
    { "Async Granular",       'a',  Y_OSCILLATOR_MODE_AGRAN,
                                        "Grain Lz",       "0.001", "20",  "Grain Spread",    "0",    "1",    /* AsyncGran  */ "Grain Envelope",     "Grain Freq Dist",   },
    { "FM|FM Wave->Sine",       0,  4,  "Mod Freq Ratio", "0.5",   "16",  "Mod Freq Detune", "-",    "+",    /* FM Wv->Sin */ "Mod Index Source",   "Mod Index Amount",  },
    { "FM|FM Sine->Wave",       1,  5,  "Mod Freq Ratio", "0.5",   "16",  "Mod Freq Detune", "-",    "+",    /* FM Sin->Wv */ "Mod Index Source",   "Mod Index Amount",  },
    { "Waveshaper",           'x',  6,  "Phase Bias",     "0",     "1",   "Mod Amt Bias",    "0",    "1",    /* Waveshaper */ "Mod Amt Source",     "Mod Amt Amount",    },
    { "Noise",                'n',  7,  NULL,             "0",     "1",   NULL,              "0",    "1",    /* Noise      */ NULL,                 NULL,                },
    { "PADsynth",             'p',  Y_OSCILLATOR_MODE_PADSYNTH,
                                        "Partial Width",  "1",     "100", "Partial Stretch", "-10%", "+10%", /* PADsynth   */ "Width Scale / Mode", "Damping",           },
    { "Phase Distortion",     'p',  9,  "Mod Balance",    "1st",   "2nd", "Bias",            "0",    "1",    /* Phase Dist */ "Mod Source",         "Mod Amount",        },
    { "FM|FM Wave->LF Sine",    2, 10,  "Low Frequency",  "1/8Hz", "2Hz", "Mod Index Bias",  "0",    "1",    /* FM Wv->LF  */ "Mod Index Source",   "Mod Index Amount",  },
    { "Wavecycle Chorus",     'w', 11,  "Tuning Spread",  "0",     "1",   "Wave Sel Bias",   "0",    "+60",  /* WaveChorus */ NULL,                 "Chorus Depth",      },
    { NULL }
};

struct y_vcf_modes_t y_vcf_modes[Y_FILTER_MODE_COUNT + 2] = {
   /* name                        pri  id  qres         mparam */
    { "Off",                       0,  0,  NULL,        NULL     },
    { "Low Pass: Xsynth 2-pole",   1,  1,  "Resonance", NULL     },
    { "Low Pass: Xsynth 4-pole",   2,  2,  "Resonance", NULL     },
    { "Low Pass: Fons' MVC LPF-3", 3,  3,  "Resonance", "Drive"  },
    { "Low Pass: Clipping 4-pole", 5,  4,  "Resonance", "Drive"  },
    { "Band Pass: 4-pole",         6,  5,  "Resonance", NULL     },
    { "Low Pass: amSynth 4-pole",  4,  6,  "Resonance", NULL     },
    { "Band Pass: Csound resonz",  7,  7,  "Bandwidth", NULL     }, /* actually, "1-2*Bandwidth/Samplerate" */
    { "High Pass: 2-pole",         8,  8,  "Resonance", NULL     },
    { "High Pass: 4-pole",         9,  9,  "Resonance", NULL     },
    { "Band Reject: 4-pole",      10, 10,  "Resonance", NULL     },
    { NULL }
};

struct y_effect_modes_t y_effect_modes[Y_EFFECT_MODE_COUNT + 2] = {
   /* name              pri  id  mparam1  mparam2      mparam3         mparam4        mparam5           mparam6 */
    { "Off",             0,  0,  NULL,    NULL,        NULL,           NULL,          NULL,             NULL },
    { "Plate Reverb",    0,  1,  NULL,    NULL,        NULL,           "Bandwidth",   "Tail",           "Damping" },
    { "Dual Delay",      0,  2,  NULL,    "Feedback",  "Feed Across",  "Left Delay",  "Right Delay",    "Damping" },
    { "SC Reverb",       0,  3,  NULL,    NULL,        NULL,           "Feedback",    "Low Pass Freq",  "Pitch Mod" },
    { NULL }
};

static GtkTreeStore *
cmodel_new(void)
{
    GtkTreeStore *m = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT); /* name, id, sort priority */
    g_object_set_qdata(G_OBJECT(m), combomodel_id_to_path_quark, g_ptr_array_new());

    return m;
}

static void
_cmodel_add(GtkTreeStore *m, const char *path, int sort, int id, GtkTreeIter *parent_iter)
{
    const char *dp;
    int len;
    GtkTreeModel *model = GTK_TREE_MODEL(m);
    GtkTreeIter iter, insert_after_iter;
    int found, insert_after_set;

    /* find length of top element of path */
    dp = strchr(path, '|');
    if (dp == NULL)
        len = strlen(path);
    else
        len = dp - path;
    if (len == 0) {
        fprintf(stderr, "cmodel_add: menu element has zero length (path = '%s'), aborting!\n", path);
        exit(1);
    }
    /* find top element of path in list */
    found = FALSE;
    insert_after_set = FALSE;
    if (gtk_tree_model_iter_children(model, &iter, parent_iter)) {
        do {
            gchar *mname;
            gint msort;

            gtk_tree_model_get(model, &iter, 0, &mname, 2, &msort, -1);
            if (!strncmp(path, mname, len) && mname[len] == '\0') {
                found = TRUE;
                g_free(mname);
                break;
            } else if (msort <= sort) {
                insert_after_iter = iter;
                insert_after_set = TRUE;
            }
            g_free(mname);
        } while (gtk_tree_model_iter_next(model, &iter));
    }
    if (found) {
        if (dp != NULL) {  /* this is a branch node */
            return _cmodel_add(m, dp + 1, sort, id, &iter); /* recurse to next lower level */
        } else {
            fprintf(stderr, "cmodel_add: leaf already exists (path='%s'), aborting!\n", path);
            exit(1);
        }
    } else {  /* didn't find it */
        if (dp != NULL && dp[1] != '\0') {
            fprintf(stderr, "cmodel_add: trying to add to non-existent branch '%s', aborting!\n", path);
            exit(1);
        }
        /* insert into model */
        if (insert_after_set) {
            gtk_tree_store_insert_after(m, &iter, parent_iter, &insert_after_iter);
        } else {
            gtk_tree_store_prepend(m, &iter, parent_iter);
        }
        if (dp != NULL) { /* set branch node */
            char *name = g_strndup(path, len);

            gtk_tree_store_set(m, &iter, 0, name, 1, id, 2, sort, -1);
            g_free(name);
        } else { /* set leaf node */
            gtk_tree_store_set(m, &iter, 0, path, 1, id, 2, sort, -1);
        }
    }
}

static void
cmodel_add(GtkTreeStore *m, const char *path, int sort, int id)
{
    _cmodel_add(m, path, sort, id, NULL);
}

static gboolean
cmodel_finish_foreach_func(GtkTreeModel *model,
                           GtkTreePath *path,
                           GtkTreeIter *iter,
                           gpointer data)
{
    int id;
    const char *name;
    GPtrArray *id_to_path = (GPtrArray *)data;

    if (gtk_tree_model_iter_has_child (model, iter))
        return FALSE; /* skip branch nodes */

    gtk_tree_model_get(model, iter, 0, &name, 1, &id, -1);

    if (id_to_path->len <= id) {
        g_ptr_array_set_size(id_to_path, id + 1);
    }
    g_ptr_array_index(id_to_path, id) = gtk_tree_path_to_string(path);

    return FALSE;
}

static void
cmodel_finish(GtkTreeStore *m, int type)
{
    GPtrArray *id_to_path = g_object_get_qdata(G_OBJECT(m), combomodel_id_to_path_quark);

    gtk_tree_model_foreach(GTK_TREE_MODEL(m), cmodel_finish_foreach_func, (gpointer)id_to_path);

    combomodel[type] = m;
}

static void   /* a GtkCellLayoutDataFunc for combos */
cmodel_only_leaves_sensitive(GtkCellLayout   *cell_layout,
                             GtkCellRenderer *cell,
                             GtkTreeModel    *tree_model,
                             GtkTreeIter     *iter,
                             gpointer         data)
{
    gboolean sensitive;

    sensitive = !gtk_tree_model_iter_has_child (tree_model, iter);

    g_object_set (cell, "sensitive", sensitive, NULL);
}

void
create_edit_combo_models(void)
{
    GtkTreeStore *m;
    int i;

    combomodel_id_to_path_quark = g_quark_from_string ("combomodel_id_to_path");
    combo_renderer = gtk_cell_renderer_text_new();
    combo_value_quark = g_quark_from_string ("combo_value");
    combo_combomodel_type_quark = g_quark_from_string ("combomodel_type");

    /* Oscillator modes (Y_COMBOMODEL_TYPE_OSC_MODE) */
    m = cmodel_new();

    cmodel_add(m, "FM|", 'f', 0);
    for (i = 0; y_osc_modes[i].name != NULL; i++) {
        struct y_osc_modes_t *om = &y_osc_modes[i];
        cmodel_add(m, om->name, om->priority, om->id);
    }
    if (i != Y_OSCILLATOR_MODE_COUNT + 1) {
        GDB_MESSAGE(-1, " create_edit_combo_models(): oscillator mode count mismatch!\n");
        /* exit(1); */
    }

    cmodel_finish(m, Y_COMBOMODEL_TYPE_OSC_MODE);

    /* Wavetables (Y_COMBOMODEL_TYPE_WAVETABLE) */
    m = cmodel_new();

    cmodel_add(m, "Sines|",                 0,  0);
    cmodel_add(m, "Analog|",                0,  0);
    cmodel_add(m, "Digital|",               0,  0);
    cmodel_add(m, "Brass|",                 0,  0);
    cmodel_add(m, "Woodwind|",              0,  0);
    cmodel_add(m, "Keyboard|",              0,  0);
    cmodel_add(m, "Organ|",                 0,  0);
    cmodel_add(m, "Strings|",               0,  0);
    cmodel_add(m, "Guitar|",                0,  0);
    cmodel_add(m, "Bass|",                  0,  0);
    cmodel_add(m, "Voice|",                 0,  0);
    cmodel_add(m, "Noise|",                 0,  0);
    cmodel_add(m, "TX81Z|",                 0,  0);
    cmodel_add(m, "Waveshaper|",            0,  0);
    cmodel_add(m, "LFO|",                   0,  0);
    for (i = 0; i < wavetables_count; i++) {
        cmodel_add(m, wavetable[i].name, wavetable[i].priority, i);
    }

    cmodel_finish(m, Y_COMBOMODEL_TYPE_WAVETABLE);

    /* minBLEP waveforms (Y_COMBOMODEL_TYPE_MINBLEP_WAVEFORM) */
    m = cmodel_new();

    cmodel_add(m, "Sawtooth+",   0, 0);
    cmodel_add(m, "Sawtooth-",   0, 1);
    cmodel_add(m, "Rectangular", 0, 2);
    cmodel_add(m, "Triangular",  0, 3);
    cmodel_add(m, "Clipped Saw", 0, 5);
    cmodel_add(m, "S/H Noise",   0, 4);

    cmodel_finish(m, Y_COMBOMODEL_TYPE_MINBLEP_WAVEFORM);

    /* Noise waveforms (Y_COMBOMODEL_TYPE_NOISE_WAVEFORM) */
    m = cmodel_new();

    cmodel_add(m, "White",     0, 0);
    cmodel_add(m, "Pink",      0, 1);
    cmodel_add(m, "Low-Pass",  0, 2);
    cmodel_add(m, "Band-Pass", 0, 3);

    cmodel_finish(m, Y_COMBOMODEL_TYPE_NOISE_WAVEFORM);

    /* Phase distortion (Y_COMBOMODEL_TYPE_PD_WAVEFORM) */
    m = cmodel_new();

    cmodel_add(m, "Sawtooth",          0, 0);
    cmodel_add(m, "Square",            0, 1);
    cmodel_add(m, "Pulse",             0, 2);
    /* double-sine */
    /* saw-pulse */
    cmodel_add(m, "CZ 'Resonant I'",   0, 5);
    cmodel_add(m, "CZ 'Resonant II'",  0, 6);
    cmodel_add(m, "CZ 'Resonant III'", 0, 7);
    cmodel_add(m, "Sawtooth & ...|",                  0, 0);
    cmodel_add(m, "Sawtooth & ...|Double Saw",        0, 0 + 0*12 + 12);
    cmodel_add(m, "Sawtooth & ...|Saw & Square",      0, 0 + 1*12 + 12);
    cmodel_add(m, "Sawtooth & ...|Saw & Pulse",       0, 0 + 2*12 + 12);
    cmodel_add(m, "Sawtooth & ...|Saw & Reso I",      0, 0 + 5*12 + 12);
    cmodel_add(m, "Sawtooth & ...|Saw & Reso II",     0, 0 + 6*12 + 12);
    cmodel_add(m, "Sawtooth & ...|Saw & Reso III",    0, 0 + 7*12 + 12);
    cmodel_add(m, "Square & ...|",                    0, 0);
    cmodel_add(m, "Square & ...|Double Square",       0, 1 + 1*12 + 12);
    cmodel_add(m, "Square & ...|Square & Pulse",      0, 1 + 2*12 + 12);
    cmodel_add(m, "Square & ...|Square & Reso I",     0, 1 + 5*12 + 12);
    cmodel_add(m, "Square & ...|Square & Reso II",    0, 1 + 6*12 + 12);
    cmodel_add(m, "Square & ...|Square & Reso III",   0, 1 + 7*12 + 12);
    cmodel_add(m, "Pulse & ...|",                     0, 0);
    cmodel_add(m, "Pulse & ...|Double Pulse",         0, 2 + 2*12 + 12);
    cmodel_add(m, "Pulse & ...|Pulse & Reso I",       0, 2 + 5*12 + 12);
    cmodel_add(m, "Pulse & ...|Pulse & Reso II",      0, 2 + 6*12 + 12);
    cmodel_add(m, "Pulse & ...|Pulse & Reso III",     0, 2 + 7*12 + 12);
    cmodel_add(m, "Reso I & ...|",                    0, 0);
    cmodel_add(m, "Reso I & ...|Double Reso I",       0, 5 + 5*12 + 12);
    cmodel_add(m, "Reso I & ...|Reso I & Reso II",    0, 5 + 6*12 + 12);
    cmodel_add(m, "Reso I & ...|Reso I & Reso III",   0, 5 + 7*12 + 12);
    cmodel_add(m, "Reso II & ...|",                   0, 0);
    cmodel_add(m, "Reso II & ...|Double Reso II",     0, 6 + 6*12 + 12);
    cmodel_add(m, "Reso II & ...|Reso II & Reso III", 0, 6 + 7*12 + 12);
    cmodel_add(m, "Double Reso III",                  0, 7 + 7*12 + 12);

    cmodel_finish(m, Y_COMBOMODEL_TYPE_PD_WAVEFORM);

    /* Modulation sources (Y_COMBOMODEL_TYPE_MOD_SRC) */
    m = cmodel_new();

    cmodel_add(m, "Constant On",     0,  0);
    cmodel_add(m, "Mod Wheel",       0,  1);
    cmodel_add(m, "Pressure",        0,  2);
    cmodel_add(m, "Key",             0,  3);
    cmodel_add(m, "Velocity",        0,  4);
    cmodel_add(m, "GLFO Bipolar",    0,  5);
    cmodel_add(m, "GLFO Unipolar",   0,  6);
    cmodel_add(m, "VLFO Bipolar",    0,  7);
    cmodel_add(m, "VLFO Unipolar",   0,  8);
    cmodel_add(m, "MLFO 0 Bipolar",  0,  9);
    cmodel_add(m, "MLFO 0 Unipolar", 0, 10);
    cmodel_add(m, "MLFO 1 Bipolar",  0, 11);
    cmodel_add(m, "MLFO 1 Unipolar", 0, 12);
    cmodel_add(m, "MLFO 2 Bipolar",  0, 13);
    cmodel_add(m, "MLFO 2 Unipolar", 0, 14);
    cmodel_add(m, "MLFO 3 Bipolar",  0, 15);
    cmodel_add(m, "MLFO 3 Unipolar", 0, 16);
    cmodel_add(m, "EGO",             0, 17);
    cmodel_add(m, "EG1",             0, 18);
    cmodel_add(m, "EG2",             0, 19);
    cmodel_add(m, "EG3",             0, 20);
    cmodel_add(m, "EG4",             0, 21);
    cmodel_add(m, "ModMix",          0, 22);

    cmodel_finish(m, Y_COMBOMODEL_TYPE_MOD_SRC);

    /* Global modulation sources (Y_COMBOMODEL_TYPE_GLFO_MOD_SRC) */
    m = cmodel_new();

    cmodel_add(m, "Constant On",     0,  0);
    cmodel_add(m, "Mod Wheel",       0,  1);
    cmodel_add(m, "Pressure",        0,  2);

    cmodel_finish(m, Y_COMBOMODEL_TYPE_GLFO_MOD_SRC);

    /* Async granular grain envelopes (Y_COMBOMODEL_TYPE_GRAIN_ENV) */
    m = cmodel_new();

    for (i = 0; i < AG_GRAIN_ENVELOPE_COUNT; i++) {
        cmodel_add(m, grain_envelope_descriptors[i].name, 0, i);
    }

    cmodel_finish(m, Y_COMBOMODEL_TYPE_GRAIN_ENV);

    /* PADsynth mode (Y_COMBOMODEL_TYPE_PADSYNTH_MODE) */
    m = cmodel_new();

    cmodel_add(m, "100% / Stereo",    3,  0);
    cmodel_add(m, "100% / Mono",      3,  1);
    cmodel_add(m, "50% / Stereo",     2,  2);
    cmodel_add(m, "50% / Mono",       2,  3);
    cmodel_add(m, "25% / Stereo",     1,  4);
    cmodel_add(m, "25% / Mono",       1,  5);
    cmodel_add(m, "10% / Stereo",     0,  6);
    cmodel_add(m, "10% / Mono",       0,  7);
    cmodel_add(m, "150% / Stereo",    4,  8);
    cmodel_add(m, "150% / Mono",      4,  9);
    cmodel_add(m, "200% / Stereo",    5, 10);
    cmodel_add(m, "200% / Mono",      5, 11);
    cmodel_add(m, "250% / Stereo",    6, 12);
    cmodel_add(m, "250% / Mono",      6, 13);
    cmodel_add(m, "75% / Stereo",     2, 14);
    cmodel_add(m, "75% / Mono",       2, 15);

    cmodel_finish(m, Y_COMBOMODEL_TYPE_PADSYNTH_MODE);

    /* Filter mode (Y_COMBOMODEL_TYPE_FILTER_MODE) */
    m = cmodel_new();

    for (i = 0; y_vcf_modes[i].name != NULL; i++) {
        struct y_vcf_modes_t *vm = &y_vcf_modes[i];
        cmodel_add(m, vm->name, vm->priority, vm->id);
    }
    if (i != Y_FILTER_MODE_COUNT + 1) {
        GDB_MESSAGE(-1, " create_edit_combo_models(): filter mode count mismatch!\n");
        /* exit(1); */
    }

    cmodel_finish(m, Y_COMBOMODEL_TYPE_FILTER_MODE);

    /* Filter sources (Y_COMBOMODEL_TYPE_FILTER_SRC) */
    m = cmodel_new();

    cmodel_add(m, "Bus A", 0, 0);
    cmodel_add(m, "Bus B", 0, 1);

    cmodel_finish(m, Y_COMBOMODEL_TYPE_FILTER1_SRC);

    m = cmodel_new();

    cmodel_add(m, "Bus A", 0, 0);
    cmodel_add(m, "Bus B", 0, 1);
    cmodel_add(m, "Filter 1 Out", 0, 2);

    cmodel_finish(m, Y_COMBOMODEL_TYPE_FILTER2_SRC);

    /* Effect mode (Y_COMBOMODEL_TYPE_EFFECT_MODE) */
    m = cmodel_new();

    for (i = 0; y_effect_modes[i].name != NULL; i++) {
        struct y_effect_modes_t *em = &y_effect_modes[i];
        cmodel_add(m, em->name, em->priority, em->id);
    }
    if (i != Y_EFFECT_MODE_COUNT + 1) {
        GDB_MESSAGE(-1, " create_edit_combo_models(): effect mode count mismatch!\n");
        /* exit(1); */
    }

    cmodel_finish(m, Y_COMBOMODEL_TYPE_EFFECT_MODE);

    /* EG mode (Y_COMBOMODEL_TYPE_EG_MODE) */
    m = cmodel_new();

    cmodel_add(m, "Off",             0, 0);
    cmodel_add(m, "ADSR",            0, 1);
    cmodel_add(m, "AAASR",           0, 2);
    cmodel_add(m, "AASRR",           0, 3);
    cmodel_add(m, "ASRRR",           0, 4);
    cmodel_add(m, "One-Shot",        0, 5);

    cmodel_finish(m, Y_COMBOMODEL_TYPE_EG_MODE);

    /* EG shape (Y_COMBOMODEL_TYPE_EG_SHAPE) */
    m = cmodel_new();

    cmodel_add(m, "Lead +3",       0,  0);
    cmodel_add(m, "Lead +2",       0,  1);
    cmodel_add(m, "Lead +1",       0,  2);
    cmodel_add(m, "Linear",        0,  3);
    cmodel_add(m, "Lag -1",        0,  4);
    cmodel_add(m, "Lag -2",        0,  5);
    cmodel_add(m, "Lag -3",        0,  6);
    cmodel_add(m, "\"S\" Lead",    0,  7);
    cmodel_add(m, "\"S\" Mid",     0,  8);
    cmodel_add(m, "\"S\" Lag",     0,  9);
    cmodel_add(m, "Jump",          0, 10);
    cmodel_add(m, "Hold",          0, 11);

    cmodel_finish(m, Y_COMBOMODEL_TYPE_EG_SHAPE);
}

GtkWidget *
create_edit_tab_and_table(char *text, int cols, int rows, GtkWidget *window, GtkWidget *notebook)
{
    GtkWidget *table;
    GtkWidget *tab_label;

    table = gtk_table_new (cols, rows, FALSE);
    gtk_widget_ref (table);
    gtk_object_set_data_full (GTK_OBJECT (window), "edit tab table", table,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (table);
    gtk_container_set_border_width (GTK_CONTAINER (table), 4);
    // gtk_table_set_col_spacings (GTK_TABLE (table), 2);

    tab_label = gtk_label_new (text);
    gtk_widget_ref (tab_label);
    gtk_object_set_data_full (GTK_OBJECT (window), "edit tab label", tab_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (tab_label);
    gtk_misc_set_alignment (GTK_MISC (tab_label), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (tab_label), 2, 2);

    gtk_notebook_append_page(GTK_NOTEBOOK (notebook), table, tab_label);

    return table;
}

GtkWidget *
create_edit_place_label_in_table(char *text,
                                 GtkWidget *window, GtkWidget *table,
                                 int x0, int x1, int y0, int y1)
{
    GtkWidget *label;

    label = gtk_label_new (text);
    gtk_widget_ref (label);
    gtk_object_set_data_full (GTK_OBJECT (window), "edit table label", label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (label), 2, 2);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_table_attach (GTK_TABLE (table), label, x0, x1, y0, y1,
                              (GtkAttachOptions) (GTK_FILL),
                              (GtkAttachOptions) (0), 2, 2);
    return label;
}

void
create_edit_place_knob_in_table(int port,
                                char *toptext, char *ltext, char *rtext,
                                GtkWidget *window, GtkWidget *table,
                                int x0, int x1, int y0, int y1)
{
    struct y_port_descriptor *ypd = &y_port_description[port];
    GtkWidget *wframe, *wtable, *wlabel, *widget;

    wframe = gtk_frame_new (NULL);
    gtk_widget_ref (wframe);
    gtk_object_set_data_full (GTK_OBJECT (window), "edit table simple knob frame", wframe,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (wframe);
    gtk_table_attach (GTK_TABLE (table), wframe, x0, x1, y0, y1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    wtable = gtk_table_new (3, 3, FALSE);
    gtk_widget_ref (wtable);
    gtk_object_set_data_full (GTK_OBJECT (window), "edit table simple knob table", wtable,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (wtable);
    gtk_container_set_border_width (GTK_CONTAINER (wtable), 2);
    gtk_table_set_row_spacings (GTK_TABLE (wtable), 0);
    gtk_table_set_col_spacings (GTK_TABLE (wtable), 0);
    gtk_container_add (GTK_CONTAINER (wframe), wtable);

    if (toptext) {
        wlabel = gtk_label_new (toptext);
        voice_widgets[port].top_label = wlabel;
        gtk_widget_ref (wlabel);
        gtk_object_set_data_full (GTK_OBJECT (window), "edit table simple knob top label", wlabel,
                                  (GtkDestroyNotify) gtk_widget_unref);
        gtk_widget_show (wlabel);
        gtk_misc_set_alignment (GTK_MISC (wlabel), 0, 0.5);
        gtk_misc_set_padding (GTK_MISC (wlabel), 3, 2);
        gtk_label_set_line_wrap (GTK_LABEL (wlabel), TRUE);
        gtk_table_attach (GTK_TABLE (wtable), wlabel, 0, 3, 0, 1,
                                  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                                  (GtkAttachOptions) (0), 0, 0);
    }

    if (ypd->type == Y_PORT_TYPE_LINEAR || ypd->type == Y_PORT_TYPE_PAN) {
        voice_widgets[port].adjustment = gtk_adjustment_new (ypd->lower_bound, ypd->lower_bound, ypd->upper_bound, (ypd->upper_bound - ypd->lower_bound) / 1000.0f, 1, 0);
    } else { /* Y_PORT_TYPE_LOGARITHMIC, Y_PORT_TYPE_LOGSCALED, or Y_PORT_TYPE_BPLOGSCALED */
        voice_widgets[port].adjustment = gtk_adjustment_new (0.0f, 0.0f, 1.0f, 1.0f / 1000.0f, 1, 0);
    }
    widget = gtk_knob_new (GTK_ADJUSTMENT (voice_widgets[port].adjustment));
    voice_widgets[port].widget = widget;
    gtk_widget_ref (widget);
    gtk_object_set_data_full (GTK_OBJECT (window), "edit table simple knob", widget,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (widget);
    gtk_table_attach (GTK_TABLE (wtable), widget, 1, 2, 1, 2,
                              (GtkAttachOptions) (0),
                              (GtkAttachOptions) (0), 0, 0);
    gtk_signal_connect (GTK_OBJECT (voice_widgets[port].adjustment), "value_changed",
                        GTK_SIGNAL_FUNC (on_voice_knob_change),
                        GINT_TO_POINTER(port));

    if (ypd->type == Y_PORT_TYPE_BPLOGSCALED ||
        (ypd->type == Y_PORT_TYPE_LINEAR && ypd->lower_bound < 0.0f && ypd->upper_bound > 0.0f) ||
        ypd->type == Y_PORT_TYPE_PAN) {
        widget = gtk_button_new ();
        gtk_widget_ref (widget);
        gtk_object_set_data_full (GTK_OBJECT (window), "zero button", widget,
                              (GtkDestroyNotify) gtk_widget_unref);
        gtk_widget_show (widget);
        gtk_table_attach (GTK_TABLE (wtable), widget, 1, 2, 2, 3,
                          (GtkAttachOptions) (0),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_signal_connect (GTK_OBJECT (widget), "pressed",
                            GTK_SIGNAL_FUNC (on_voice_knob_zero),
                            GINT_TO_POINTER(port));
    }

    if (ltext == NULL || *ltext != '\0') { /* Place left value label, unless ltext points to empty string */
        if (ltext) { /* use supplied label */
            wlabel = gtk_label_new (ltext);
        } else {     /* use port lower bound */
            char buf[256];

            sprintf(buf, "%.5g", ypd->lower_bound);
            wlabel = gtk_label_new (buf);
        }
        voice_widgets[port].label1 = wlabel;
        gtk_widget_ref (wlabel);
        gtk_object_set_data_full (GTK_OBJECT (window), "edit table simple knob left label", wlabel,
                                  (GtkDestroyNotify) gtk_widget_unref);
        gtk_widget_show (wlabel);
        gtk_misc_set_alignment (GTK_MISC (wlabel), 1, 0.5);
        gtk_misc_set_padding (GTK_MISC (wlabel), 1, 0);
        gtk_table_attach (GTK_TABLE (wtable), wlabel, 0, 1, 2, 3,
                          (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
    }

    if (rtext == NULL || *rtext != '\0') { /* Place right value label, unless rtext points to empty string */
        if (rtext) { /* use supplied label */
            wlabel = gtk_label_new (rtext);
        } else {     /* use port upper bound */
            char buf[256];

            sprintf(buf, "%.5g", ypd->upper_bound);
            wlabel = gtk_label_new (buf);
        }
        voice_widgets[port].label2 = wlabel;
        gtk_widget_ref (wlabel);
        gtk_object_set_data_full (GTK_OBJECT (window), "edit table simple knob right label", wlabel,
                                  (GtkDestroyNotify) gtk_widget_unref);
        gtk_widget_show (wlabel);
        gtk_misc_set_alignment (GTK_MISC (wlabel), 0, 0.5);
        gtk_misc_set_padding (GTK_MISC (wlabel), 1, 0);
        gtk_table_attach (GTK_TABLE (wtable), wlabel, 2, 3, 2, 3,
                          (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
    }
}

void
create_edit_place_detent_in_table(int port, char *toptext,
                                  GtkWidget *window, GtkWidget *table,
                                  int x0, int x1, int y0, int y1)
{
    struct y_port_descriptor *ypd = &y_port_description[port];
    GtkWidget *wframe, *wvbox, *wlabel, *widget;

    wframe = gtk_frame_new (NULL);
    gtk_widget_ref (wframe);
    gtk_object_set_data_full (GTK_OBJECT (window), "edit table detent frame", wframe,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (wframe);
    gtk_table_attach (GTK_TABLE (table), wframe, x0, x1, y0, y1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    wvbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (wvbox);
    gtk_object_set_data_full (GTK_OBJECT (window), "edit table detent vbox", wvbox,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (wvbox);
    gtk_container_set_border_width (GTK_CONTAINER (wvbox), 2);
    gtk_container_add (GTK_CONTAINER (wframe), wvbox);

    if (toptext) {
        wlabel = gtk_label_new (toptext);
        voice_widgets[port].top_label = wlabel;
        gtk_widget_ref (wlabel);
        gtk_object_set_data_full (GTK_OBJECT (window), "edit table detent top label", wlabel,
                                  (GtkDestroyNotify) gtk_widget_unref);
        gtk_widget_show (wlabel);
        gtk_misc_set_alignment (GTK_MISC (wlabel), 0, 0.5);
        gtk_misc_set_padding (GTK_MISC (wlabel), 3, 2);
        gtk_label_set_line_wrap (GTK_LABEL (wlabel), TRUE);
        gtk_box_pack_start (GTK_BOX (wvbox), wlabel, FALSE, FALSE, 0);
    }

    voice_widgets[port].adjustment = gtk_adjustment_new (ypd->lower_bound, ypd->lower_bound, ypd->upper_bound, 1, 1, 0);
    widget = gtk_spin_button_new (GTK_ADJUSTMENT (voice_widgets[port].adjustment), 1, 0);
    voice_widgets[port].widget = widget;
    gtk_widget_ref (widget);
    gtk_object_set_data_full (GTK_OBJECT (window), "edit table detent spin", widget,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (widget);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (widget), TRUE);
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (widget), GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (widget), TRUE);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), TRUE);
    gtk_box_pack_start (GTK_BOX (wvbox), widget, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (voice_widgets[port].adjustment), "value_changed",
                        GTK_SIGNAL_FUNC (on_voice_detent_change),
                        GINT_TO_POINTER(port));
}

void
create_edit_place_combo_in_table(int port, char *toptext, int combomodel_type,
                                 GtkWidget *table,
                                 int x0, int x1, int y0, int y1)
{
    GtkWidget *wframe, *wvbox, *wlabel, *wcombo;

    wframe = gtk_frame_new (NULL);
    gtk_widget_show (wframe);
    gtk_table_attach (GTK_TABLE (table), wframe, x0, x1, y0, y1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    wvbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (wvbox);
    gtk_container_set_border_width (GTK_CONTAINER (wvbox), 2);
    gtk_container_add (GTK_CONTAINER (wframe), wvbox);

    if (toptext) {
        wlabel = gtk_label_new (toptext);
        voice_widgets[port].top_label = wlabel;
        gtk_widget_show (wlabel);
        gtk_misc_set_alignment (GTK_MISC (wlabel), 0, 0.5);
        gtk_misc_set_padding (GTK_MISC (wlabel), 3, 2);
        gtk_label_set_line_wrap (GTK_LABEL (wlabel), TRUE);
        gtk_box_pack_start (GTK_BOX (wvbox), wlabel, FALSE, FALSE, 0);
    }

    wcombo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(combomodel[combomodel_type]));
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(wcombo), combo_renderer, FALSE);
    gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(wcombo), combo_renderer,
                                       cmodel_only_leaves_sensitive, NULL, NULL);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(wcombo), combo_renderer, "text", 0);
    g_object_set_qdata(G_OBJECT(wcombo), combo_value_quark, GINT_TO_POINTER(0));
    g_object_set_qdata(G_OBJECT(wcombo), combo_combomodel_type_quark,
                       GINT_TO_POINTER(combomodel_type));
    voice_widgets[port].widget = wcombo;
    gtk_widget_show(wcombo);
    gtk_box_pack_start (GTK_BOX(wvbox), wcombo, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(wcombo), "changed", G_CALLBACK(on_voice_combo_change),
                     GINT_TO_POINTER(port));
}

void
create_edit_place_copy_paste_buttons(int port,
                                     GtkWidget *window, GtkWidget *table,
                                     int x0, int x1, int y0, int y1)
{
    GtkWidget *vbox;
    GtkWidget *copy_button;
    GtkWidget *paste_button;

    vbox = gtk_vbox_new (TRUE, 0);
    gtk_widget_ref (vbox);
    gtk_object_set_data_full (GTK_OBJECT (window), "edit copy/paste vbox", vbox,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_table_attach (GTK_TABLE (table), vbox, x0, x1, y0, y1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    copy_button = gtk_button_new_with_label ("Copy");
    gtk_widget_ref (copy_button);
    gtk_object_set_data_full (GTK_OBJECT (window), "copy_button", copy_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (copy_button);
    gtk_box_pack_start (GTK_BOX (vbox), copy_button, TRUE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (copy_button), "clicked",
                        GTK_SIGNAL_FUNC (on_voice_element_copy),
                        GINT_TO_POINTER(port));

    paste_button = gtk_button_new_with_label ("Paste");
    gtk_widget_ref (paste_button);
    gtk_object_set_data_full (GTK_OBJECT (window), "paste_button", paste_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (paste_button);
    gtk_box_pack_start (GTK_BOX (vbox), paste_button, TRUE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (paste_button), "clicked",
                        GTK_SIGNAL_FUNC (on_voice_element_paste),
                        GINT_TO_POINTER(port));
}

void
create_edit_window (const char *tag)
{
    int port;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *notebook;
    GtkWidget *osc1_table;
    GtkWidget *osc2_table;
    GtkWidget *osc3_table;
    GtkWidget *osc4_table;
    GtkWidget *filter_table;
    GtkWidget *mix_table;
    GtkWidget *effect_table;
    GtkWidget *vseparator;
    GtkWidget *lfo_table;
    GtkWidget *eg_table;
    GtkWidget *misc_table;
    GtkWidget *copy_button;
    GtkWidget *paste_button;
    GtkWidget *hseparator;
    GtkWidget *edit_action_frame;
    GtkWidget *edit_action_table;
    GtkWidget *test_note_frame;
    GtkWidget *test_note_table;
    GtkWidget *test_note_key_label;
    GtkWidget *test_note_velocity_label;
    GtkWidget *test_note_key;
    GtkWidget *test_note_velocity;
    GtkWidget *test_note_mode_button;
    GtkWidget *edit_save_changes;
    GtkWidget *edit_close;

    /* create combo models */
    create_edit_combo_models();

    /* edit window */
    edit_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (edit_window),
                         "edit_window", edit_window);
    set_window_title(edit_window, tag, "Patch Edit");
    gtk_signal_connect (GTK_OBJECT (edit_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (edit_window), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_edit_close);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "vbox",
                              vbox, (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (edit_window), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

    /* name, category and comment */
    table = gtk_table_new (4, 2, FALSE);
    gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (table), 2);
    gtk_table_set_row_spacings (GTK_TABLE (table), 1);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);

    label = gtk_label_new ("Patch Name");
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                      GTK_FILL, 0, 0, 0);

    name_entry = gtk_entry_new_with_max_length(30);
    gtk_table_attach (GTK_TABLE (table), name_entry, 1, 2, 0, 1,
                      GTK_FILL | GTK_EXPAND, 0, 0, 0);

    label = gtk_label_new ("Category");
    gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                      GTK_FILL, 0, 0, 0);

    category_entry = gtk_entry_new_with_max_length(10);
    gtk_table_attach (GTK_TABLE (table), category_entry, 3, 4, 0, 1,
                      GTK_FILL, 0, 0, 0);

    label = gtk_label_new ("Comment");
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                      GTK_FILL, 0, 0, 0);

    comment_entry = gtk_entry_new_with_max_length(60);
    gtk_table_attach (GTK_TABLE (table), comment_entry, 1, 4, 1, 2,
                      GTK_FILL | GTK_EXPAND, 0, 0, 0);
    gtk_widget_show_all (table);

    /* separator */
    hseparator = gtk_hseparator_new ();
    gtk_widget_ref (hseparator);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "hseparator", hseparator,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hseparator);
    gtk_box_pack_start (GTK_BOX (vbox), hseparator, FALSE, FALSE, 2);

    /* notebook */
    notebook = gtk_notebook_new ();
    gtk_widget_ref (notebook);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "notebook", notebook,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (notebook);
    gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 2);

    for (port = 0; port < Y_PORTS_COUNT; port++) {
        voice_widgets[port].widget = NULL;
        voice_widgets[port].adjustment = NULL;
        voice_widgets[port].top_label = NULL;
        voice_widgets[port].label1 = NULL;
        voice_widgets[port].label2 = NULL;
        voice_widgets[port].last_mode = -1;
    }

    /* oscillator 1 */
    osc1_table = create_edit_tab_and_table("Osc1", 6, 3, edit_window, notebook);
    create_edit_place_label_in_table("Oscillator 1\n'Osc1'",                edit_window, osc1_table, 0, 1, 0, 1);
    create_edit_place_copy_paste_buttons(Y_PORT_OSC1_MODE,                  edit_window, osc1_table, 0, 1, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_OSC1_MODE, "Mode", Y_COMBOMODEL_TYPE_OSC_MODE,
                                                                            osc1_table, 1, 2, 0, 1);
    create_edit_place_detent_in_table(Y_PORT_OSC1_PITCH,    "Pitch",        edit_window, osc1_table, 2, 3, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_OSC1_DETUNE, "Detune", "-50", "+50",
                                                                            edit_window, osc1_table, 3, 4, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_OSC1_PITCH_MOD_SRC, "Pitch Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                            osc1_table, 4, 5, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_OSC1_PITCH_MOD_AMT, "Pitch Mod Amount", NULL, NULL,
                                                                            edit_window, osc1_table, 5, 6, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_OSC1_WAVEFORM, "Waveform", Y_COMBOMODEL_TYPE_WAVETABLE,
                                                                            osc1_table, 1, 2, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC1_MPARAM1, "MParam1", NULL, NULL,
                                                                            edit_window, osc1_table, 2, 3, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC1_MPARAM2, "MParam2", NULL, NULL,
                                                                            edit_window, osc1_table, 3, 4, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_OSC1_MMOD_SRC, "MMod Source", Y_COMBOMODEL_TYPE_GRAIN_ENV,
                                                                            osc1_table, 4, 5, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC1_MMOD_AMT, "MMod Amount", NULL, NULL,
                                                                            edit_window, osc1_table, 5, 6, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC1_LEVEL_A, "BusA Send Level", NULL, NULL,
                                                                            edit_window, osc1_table, 2, 3, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_OSC1_LEVEL_B, "BusB Send Level", NULL, NULL,
                                                                            edit_window, osc1_table, 3, 4, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_OSC1_AMP_MOD_SRC, "Amp Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                            osc1_table, 4, 5, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_OSC1_AMP_MOD_AMT, "Amp Mod Amount", NULL, NULL,
                                                                            edit_window, osc1_table, 5, 6, 2, 3);

    /* oscillator 2 */
    osc2_table = create_edit_tab_and_table("Osc2", 6, 3, edit_window, notebook);
    create_edit_place_label_in_table("Oscillator 2\n'Osc2'",                edit_window, osc2_table, 0, 1, 0, 1);
    create_edit_place_copy_paste_buttons(Y_PORT_OSC2_MODE,                  edit_window, osc2_table, 0, 1, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_OSC2_MODE, "Mode", Y_COMBOMODEL_TYPE_OSC_MODE,
                                                                            osc2_table, 1, 2, 0, 1);
    create_edit_place_detent_in_table(Y_PORT_OSC2_PITCH,    "Pitch",        edit_window, osc2_table, 2, 3, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_OSC2_DETUNE, "Detune", "-50", "+50",
                                                                            edit_window, osc2_table, 3, 4, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_OSC2_PITCH_MOD_SRC, "Pitch Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                            osc2_table, 4, 5, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_OSC2_PITCH_MOD_AMT, "Pitch Mod Amount", NULL, NULL,
                                                                            edit_window, osc2_table, 5, 6, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_OSC2_WAVEFORM, "Waveform", Y_COMBOMODEL_TYPE_WAVETABLE,
                                                                            osc2_table, 1, 2, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC2_MPARAM1, "MParam1", NULL, NULL,
                                                                            edit_window, osc2_table, 2, 3, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC2_MPARAM2, "MParam2", NULL, NULL,
                                                                            edit_window, osc2_table, 3, 4, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_OSC2_MMOD_SRC, "MMod Source", Y_COMBOMODEL_TYPE_GRAIN_ENV,
                                                                            osc2_table, 4, 5, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC2_MMOD_AMT, "MMod Amount", NULL, NULL,
                                                                            edit_window, osc2_table, 5, 6, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC2_LEVEL_A, "BusA Send Level", NULL, NULL,
                                                                            edit_window, osc2_table, 2, 3, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_OSC2_LEVEL_B, "BusB Send Level", NULL, NULL,
                                                                            edit_window, osc2_table, 3, 4, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_OSC2_AMP_MOD_SRC, "Amp Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                            osc2_table, 4, 5, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_OSC2_AMP_MOD_AMT, "Amp Mod Amount", NULL, NULL,
                                                                            edit_window, osc2_table, 5, 6, 2, 3);

    /* oscillator 3 */
    osc3_table = create_edit_tab_and_table("Osc3", 6, 3, edit_window, notebook);
    create_edit_place_label_in_table("Oscillator 3\n'Osc3'",                edit_window, osc3_table, 0, 1, 0, 1);
    create_edit_place_copy_paste_buttons(Y_PORT_OSC3_MODE,                  edit_window, osc3_table, 0, 1, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_OSC3_MODE, "Mode", Y_COMBOMODEL_TYPE_OSC_MODE,
                                                                            osc3_table, 1, 2, 0, 1);
    create_edit_place_detent_in_table(Y_PORT_OSC3_PITCH,    "Pitch",        edit_window, osc3_table, 2, 3, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_OSC3_DETUNE, "Detune", "-50", "+50",
                                                                            edit_window, osc3_table, 3, 4, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_OSC3_PITCH_MOD_SRC, "Pitch Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                            osc3_table, 4, 5, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_OSC3_PITCH_MOD_AMT, "Pitch Mod Amount", NULL, NULL,
                                                                            edit_window, osc3_table, 5, 6, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_OSC3_WAVEFORM, "Waveform", Y_COMBOMODEL_TYPE_WAVETABLE,
                                                                            osc3_table, 1, 2, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC3_MPARAM1, "MParam1", NULL, NULL,
                                                                            edit_window, osc3_table, 2, 3, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC3_MPARAM2, "MParam2", NULL, NULL,
                                                                            edit_window, osc3_table, 3, 4, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_OSC3_MMOD_SRC, "MMod Source", Y_COMBOMODEL_TYPE_GRAIN_ENV,
                                                                            osc3_table, 4, 5, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC3_MMOD_AMT, "MMod Amount", NULL, NULL,
                                                                            edit_window, osc3_table, 5, 6, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC3_LEVEL_A, "BusA Send Level", NULL, NULL,
                                                                            edit_window, osc3_table, 2, 3, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_OSC3_LEVEL_B, "BusB Send Level", NULL, NULL,
                                                                            edit_window, osc3_table, 3, 4, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_OSC3_AMP_MOD_SRC, "Amp Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                            osc3_table, 4, 5, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_OSC3_AMP_MOD_AMT, "Amp Mod Amount", NULL, NULL,
                                                                            edit_window, osc3_table, 5, 6, 2, 3);

    /* oscillator 4 */
    osc4_table = create_edit_tab_and_table("Osc4", 6, 3, edit_window, notebook);
    create_edit_place_label_in_table("Oscillator 4\n'Osc4'",                edit_window, osc4_table, 0, 1, 0, 1);
    create_edit_place_copy_paste_buttons(Y_PORT_OSC4_MODE,                  edit_window, osc4_table, 0, 1, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_OSC4_MODE, "Mode", Y_COMBOMODEL_TYPE_OSC_MODE,
                                                                            osc4_table, 1, 2, 0, 1);
    create_edit_place_detent_in_table(Y_PORT_OSC4_PITCH,    "Pitch",        edit_window, osc4_table, 2, 3, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_OSC4_DETUNE, "Detune", "-50", "+50",
                                                                            edit_window, osc4_table, 3, 4, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_OSC4_PITCH_MOD_SRC, "Pitch Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                            osc4_table, 4, 5, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_OSC4_PITCH_MOD_AMT, "Pitch Mod Amount", NULL, NULL,
                                                                            edit_window, osc4_table, 5, 6, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_OSC4_WAVEFORM, "Waveform", Y_COMBOMODEL_TYPE_WAVETABLE,
                                                                            osc4_table, 1, 2, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC4_MPARAM1, "MParam1", NULL, NULL,
                                                                            edit_window, osc4_table, 2, 3, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC4_MPARAM2, "MParam2", NULL, NULL,
                                                                            edit_window, osc4_table, 3, 4, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_OSC4_MMOD_SRC, "MMod Source", Y_COMBOMODEL_TYPE_GRAIN_ENV,
                                                                            osc4_table, 4, 5, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC4_MMOD_AMT, "MMod Amount", NULL, NULL,
                                                                            edit_window, osc4_table, 5, 6, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_OSC4_LEVEL_A, "BusA Send Level", NULL, NULL,
                                                                            edit_window, osc4_table, 2, 3, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_OSC4_LEVEL_B, "BusB Send Level", NULL, NULL,
                                                                            edit_window, osc4_table, 3, 4, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_OSC4_AMP_MOD_SRC, "Amp Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                            osc4_table, 4, 5, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_OSC4_AMP_MOD_AMT, "Amp Mod Amount", NULL, NULL,
                                                                            edit_window, osc4_table, 5, 6, 2, 3);

    /* filters */
    filter_table = create_edit_tab_and_table("Filters", 8, 2, edit_window, notebook);
    create_edit_place_label_in_table("Filter 1", edit_window, filter_table, 0, 1, 0, 1);
    create_edit_place_label_in_table("Filter 2", edit_window, filter_table, 0, 1, 1, 2);

    create_edit_place_combo_in_table(Y_PORT_VCF1_MODE, "Mode", Y_COMBOMODEL_TYPE_FILTER_MODE,
                                                                            filter_table, 1, 2, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_VCF1_SOURCE, "Source", Y_COMBOMODEL_TYPE_FILTER1_SRC,
                                                                            filter_table, 2, 3, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_VCF1_FREQUENCY, "Frequency", NULL, NULL,
                                                                            edit_window, filter_table, 3, 4, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_VCF1_FREQ_MOD_SRC, "Freq Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                            filter_table, 4, 5, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_VCF1_FREQ_MOD_AMT, "Freq Mod Amt", NULL, NULL,
                                                                            edit_window, filter_table, 5, 6, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_VCF1_QRES, "Resonance", NULL, NULL,
                                                                            edit_window, filter_table, 6, 7, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_VCF1_MPARAM, "Drive", NULL, NULL,
                                                                            edit_window, filter_table, 7, 8, 0, 1);

    create_edit_place_combo_in_table(Y_PORT_VCF2_MODE, "Mode", Y_COMBOMODEL_TYPE_FILTER_MODE,
                                                                            filter_table, 1, 2, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_VCF2_SOURCE, "Source", Y_COMBOMODEL_TYPE_FILTER2_SRC,
                                                                            filter_table, 2, 3, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_VCF2_FREQUENCY, "Frequency", NULL, NULL,
                                                                            edit_window, filter_table, 3, 4, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_VCF2_FREQ_MOD_SRC, "Freq Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                            filter_table, 4, 5, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_VCF2_FREQ_MOD_AMT, "Freq Mod Amt", NULL, NULL,
                                                                            edit_window, filter_table, 5, 6, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_VCF2_QRES, "Resonance", NULL, NULL,
                                                                            edit_window, filter_table, 6, 7, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_VCF2_MPARAM, "Drive", NULL, NULL,
                                                                            edit_window, filter_table, 7, 8, 1, 2);
    hbox = gtk_hbox_new (TRUE, 0);
    gtk_widget_ref (hbox);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "edit filter copy/paste hbox", hbox,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hbox);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_table_attach (GTK_TABLE (filter_table), hbox, 1, 5, 2, 3,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 5);

    copy_button = gtk_button_new_with_label ("Copy Filter1");
    gtk_widget_ref (copy_button);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "vcf1_copy_button", copy_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (copy_button);
    gtk_box_pack_start (GTK_BOX (hbox), copy_button, TRUE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (copy_button), "clicked",
                        GTK_SIGNAL_FUNC (on_voice_element_copy),
                        GINT_TO_POINTER(Y_PORT_VCF1_MODE));

    paste_button = gtk_button_new_with_label ("Paste Filter1");
    gtk_widget_ref (paste_button);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "vcf1_paste_button", paste_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (paste_button);
    gtk_box_pack_start (GTK_BOX (hbox), paste_button, TRUE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (paste_button), "clicked",
                        GTK_SIGNAL_FUNC (on_voice_element_paste),
                        GINT_TO_POINTER(Y_PORT_VCF1_MODE));

    copy_button = gtk_button_new_with_label ("Copy Filter2");
    gtk_widget_ref (copy_button);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "vcf2_copy_button", copy_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (copy_button);
    gtk_box_pack_start (GTK_BOX (hbox), copy_button, TRUE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (copy_button), "clicked",
                        GTK_SIGNAL_FUNC (on_voice_element_copy),
                        GINT_TO_POINTER(Y_PORT_VCF2_MODE));

    paste_button = gtk_button_new_with_label ("Paste Filter2");
    gtk_widget_ref (paste_button);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "vcf2_paste_button", paste_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (paste_button);
    gtk_box_pack_start (GTK_BOX (hbox), paste_button, TRUE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (paste_button), "clicked",
                        GTK_SIGNAL_FUNC (on_voice_element_paste),
                        GINT_TO_POINTER(Y_PORT_VCF2_MODE));

    /* output mix */
    mix_table = create_edit_tab_and_table("Mix", 6, 3, edit_window, notebook);
    create_edit_place_label_in_table("Bus A",         edit_window, mix_table, 1, 2, 0, 1);
    create_edit_place_label_in_table("Bus B",         edit_window, mix_table, 2, 3, 0, 1);
    create_edit_place_label_in_table("Filter 1",      edit_window, mix_table, 3, 4, 0, 1);
    create_edit_place_label_in_table("Filter 2",      edit_window, mix_table, 4, 5, 0, 1);
    create_edit_place_label_in_table("Master Volume", edit_window, mix_table, 5, 6, 0, 1);
    create_edit_place_label_in_table("Level",         edit_window, mix_table, 0, 1, 1, 2);
    create_edit_place_label_in_table("Pan",           edit_window, mix_table, 0, 1, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_BUSA_LEVEL, NULL, NULL, NULL, edit_window, mix_table, 1, 2, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_BUSA_PAN,   NULL, "L",  "R",  edit_window, mix_table, 1, 2, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_BUSB_LEVEL, NULL, NULL, NULL, edit_window, mix_table, 2, 3, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_BUSB_PAN,   NULL, "L",  "R",  edit_window, mix_table, 2, 3, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_VCF1_LEVEL, NULL, NULL, NULL, edit_window, mix_table, 3, 4, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_VCF1_PAN,   NULL, "L",  "R",  edit_window, mix_table, 3, 4, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_VCF2_LEVEL, NULL, NULL, NULL, edit_window, mix_table, 4, 5, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_VCF2_PAN,   NULL, "L",  "R",  edit_window, mix_table, 4, 5, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_VOLUME,     NULL, NULL, NULL, edit_window, mix_table, 5, 6, 1, 2);

    /* effect */
    effect_table = create_edit_tab_and_table("Effect", 6, 3, edit_window, notebook);
    create_edit_place_label_in_table("Effect",                              edit_window, effect_table, 0, 1, 0, 1);
    create_edit_place_copy_paste_buttons(Y_PORT_EFFECT_MODE,                edit_window,   effect_table, 0, 1, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_EFFECT_MODE, "Effect Mode", Y_COMBOMODEL_TYPE_EFFECT_MODE,
                                                                            effect_table, 1, 2, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EFFECT_MIX, "Mix", "Dry", "Wet", edit_window, effect_table, 1, 2, 1, 2);

    vseparator = gtk_vseparator_new ();
    gtk_widget_ref (vseparator);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "vseparator", vseparator,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vseparator);
    gtk_table_attach (GTK_TABLE (effect_table), vseparator, 2, 3, 0, 2,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (GTK_FILL), 5, 0);

    create_edit_place_knob_in_table(Y_PORT_EFFECT_PARAM1, "Param 1", NULL, NULL, edit_window, effect_table, 3, 4, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EFFECT_PARAM2, "Param 2", NULL, NULL, edit_window, effect_table, 4, 5, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EFFECT_PARAM3, "Param 3", NULL, NULL, edit_window, effect_table, 5, 6, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EFFECT_PARAM4, "Param 4", NULL, NULL, edit_window, effect_table, 3, 4, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EFFECT_PARAM5, "Param 5", NULL, NULL, edit_window, effect_table, 4, 5, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EFFECT_PARAM6, "Param 6", NULL, NULL, edit_window, effect_table, 5, 6, 1, 2);

    /* LFOs */
    lfo_table = create_edit_tab_and_table("LFOs", 8, 4, edit_window, notebook);
    create_edit_place_label_in_table("Frequency",          edit_window, lfo_table, 1, 2, 0, 1);
    create_edit_place_label_in_table("Waveform",           edit_window, lfo_table, 2, 3, 0, 1);
    create_edit_place_label_in_table("Amp Mod Source",     edit_window, lfo_table, 3, 4, 0, 1);
    create_edit_place_label_in_table("Amp Mod Amt",        edit_window, lfo_table, 4, 5, 0, 1);
    create_edit_place_label_in_table("Delay",              edit_window, lfo_table, 5, 6, 0, 1);
    create_edit_place_label_in_table("Phase Spread",       edit_window, lfo_table, 6, 7, 0, 1);
    create_edit_place_label_in_table("Random Freq",        edit_window, lfo_table, 7, 8, 0, 1);
    create_edit_place_label_in_table("Global LFO\n'GLFO'", edit_window, lfo_table, 0, 1, 1, 2);
    create_edit_place_label_in_table("Voice  LFO\n'VLFO'", edit_window, lfo_table, 0, 1, 2, 3);
    create_edit_place_label_in_table("Multi-Phase\nLFO 'MLFO'", edit_window, lfo_table, 0, 1, 3, 4);
    create_edit_place_knob_in_table(Y_PORT_GLFO_FREQUENCY, NULL, NULL, NULL,    edit_window, lfo_table, 1, 2, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_GLFO_WAVEFORM, NULL, Y_COMBOMODEL_TYPE_WAVETABLE,
                                                                                lfo_table, 2, 3, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_GLFO_AMP_MOD_SRC, NULL, Y_COMBOMODEL_TYPE_GLFO_MOD_SRC,
                                                                                lfo_table, 3, 4, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_GLFO_AMP_MOD_AMT, NULL, NULL, NULL,  edit_window, lfo_table, 4, 5, 1, 2);

    create_edit_place_knob_in_table(Y_PORT_VLFO_FREQUENCY, NULL, NULL, NULL,    edit_window, lfo_table, 1, 2, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_VLFO_WAVEFORM, NULL, Y_COMBOMODEL_TYPE_WAVETABLE,
                                                                                lfo_table, 2, 3, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_VLFO_AMP_MOD_SRC, NULL, Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                                lfo_table, 3, 4, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_VLFO_AMP_MOD_AMT, NULL, NULL, NULL,  edit_window, lfo_table, 4, 5, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_VLFO_DELAY, NULL, NULL, NULL,        edit_window, lfo_table, 5, 6, 2, 3);
                                                                   
    create_edit_place_knob_in_table(Y_PORT_MLFO_FREQUENCY, NULL, NULL, NULL,    edit_window, lfo_table, 1, 2, 3, 4);
    create_edit_place_combo_in_table(Y_PORT_MLFO_WAVEFORM, NULL, Y_COMBOMODEL_TYPE_WAVETABLE,
                                                                                lfo_table, 2, 3, 3, 4);
    create_edit_place_combo_in_table(Y_PORT_MLFO_AMP_MOD_SRC, NULL, Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                                lfo_table, 3, 4, 3, 4);
    create_edit_place_knob_in_table(Y_PORT_MLFO_AMP_MOD_AMT, NULL, NULL, NULL,  edit_window, lfo_table, 4, 5, 3, 4);
    create_edit_place_knob_in_table(Y_PORT_MLFO_DELAY, NULL, NULL, NULL,        edit_window, lfo_table, 5, 6, 3, 4);
    create_edit_place_knob_in_table(Y_PORT_MLFO_PHASE_SPREAD, NULL, NULL, NULL, edit_window, lfo_table, 6, 7, 3, 4);
    create_edit_place_knob_in_table(Y_PORT_MLFO_RANDOM_FREQ, NULL, NULL, NULL,  edit_window, lfo_table, 7, 8, 3, 4);

    /* EGO */
    eg_table = create_edit_tab_and_table("EGO", 8, 3, edit_window, notebook);
    create_edit_place_label_in_table("Output\nEnvelope\nGenerator \n'EGO'",              edit_window, eg_table, 0, 1, 0, 1);
    create_edit_place_copy_paste_buttons(Y_PORT_EGO_MODE,                                edit_window, eg_table, 0, 1, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_EGO_MODE, "Mode", Y_COMBOMODEL_TYPE_EG_MODE,     eg_table, 1, 2, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EGO_VEL_LEVEL_SENS, "Vel->Level", NULL, NULL, edit_window, eg_table, 2, 3, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EGO_VEL_TIME_SCALE, "Vel->Time", NULL, NULL,  edit_window, eg_table, 1, 2, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EGO_KBD_TIME_SCALE, "Kbd->Time", NULL, NULL,  edit_window, eg_table, 2, 3, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_EGO_AMP_MOD_SRC, "Amp Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                                         eg_table, 1, 2, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EGO_AMP_MOD_AMT, "Amp Mod Amt", NULL, NULL,   edit_window, eg_table, 2, 3, 2, 3);

    vseparator = gtk_vseparator_new ();
    gtk_widget_ref (vseparator);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "vseparator", vseparator,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vseparator);
    gtk_table_attach (GTK_TABLE (eg_table), vseparator, 3, 4, 0, 3,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (GTK_FILL), 5, 0);

    create_edit_place_combo_in_table(Y_PORT_EGO_SHAPE1, "Attack 1 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 4, 5, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EGO_SHAPE2, "Attack 2 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 5, 6, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EGO_SHAPE3, "Attack 3 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 6, 7, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EGO_SHAPE4, "Release Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 7, 8, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EGO_TIME1,  "Attack 1 Time",  NULL, NULL, edit_window, eg_table, 4, 5, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EGO_TIME2,  "Attack 2 Time",  NULL, NULL, edit_window, eg_table, 5, 6, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EGO_TIME3,  "Attack 3 Time",  NULL, NULL, edit_window, eg_table, 6, 7, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EGO_TIME4,  "Release Time",   NULL, NULL, edit_window, eg_table, 7, 8, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EGO_LEVEL1, "Attack 1 Level", NULL, NULL, edit_window, eg_table, 4, 5, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EGO_LEVEL2, "Attack 2 Level", NULL, NULL, edit_window, eg_table, 5, 6, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EGO_LEVEL3, "Sustain Level ", NULL, NULL, edit_window, eg_table, 6, 7, 2, 3);

    /* EG1*/
    eg_table = create_edit_tab_and_table("EG1", 8, 3, edit_window, notebook);
    create_edit_place_label_in_table("Envelope\nGenerator \n'EG1'",                      edit_window, eg_table, 0, 1, 0, 1);
    create_edit_place_copy_paste_buttons(Y_PORT_EG1_MODE,                                edit_window, eg_table, 0, 1, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_EG1_MODE, "Mode", Y_COMBOMODEL_TYPE_EG_MODE,     eg_table, 1, 2, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EG1_VEL_LEVEL_SENS, "Vel->Level", NULL, NULL, edit_window, eg_table, 2, 3, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EG1_VEL_TIME_SCALE, "Vel->Time", NULL, NULL,  edit_window, eg_table, 1, 2, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG1_KBD_TIME_SCALE, "Kbd->Time", NULL, NULL,  edit_window, eg_table, 2, 3, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_EG1_AMP_MOD_SRC, "Amp Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                                         eg_table, 1, 2, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EG1_AMP_MOD_AMT, "Amp Mod Amt", NULL, NULL,   edit_window, eg_table, 2, 3, 2, 3);

    vseparator = gtk_vseparator_new ();
    gtk_widget_ref (vseparator);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "vseparator", vseparator,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vseparator);
    gtk_table_attach (GTK_TABLE (eg_table), vseparator, 3, 4, 0, 3,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (GTK_FILL), 5, 0);

    create_edit_place_combo_in_table(Y_PORT_EG1_SHAPE1, "Attack 1 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 4, 5, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EG1_SHAPE2, "Attack 2 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 5, 6, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EG1_SHAPE3, "Attack 3 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 6, 7, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EG1_SHAPE4, "Release Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 7, 8, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EG1_TIME1,  "Attack 1 Time",  NULL, NULL, edit_window, eg_table, 4, 5, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG1_TIME2,  "Attack 2 Time",  NULL, NULL, edit_window, eg_table, 5, 6, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG1_TIME3,  "Attack 3 Time",  NULL, NULL, edit_window, eg_table, 6, 7, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG1_TIME4,  "Release Time",   NULL, NULL, edit_window, eg_table, 7, 8, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG1_LEVEL1, "Attack 1 Level", NULL, NULL, edit_window, eg_table, 4, 5, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EG1_LEVEL2, "Attack 2 Level", NULL, NULL, edit_window, eg_table, 5, 6, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EG1_LEVEL3, "Sustain Level ", NULL, NULL, edit_window, eg_table, 6, 7, 2, 3);

    /* EG2*/
    eg_table = create_edit_tab_and_table("EG2", 8, 3, edit_window, notebook);
    create_edit_place_label_in_table("Envelope\nGenerator \n'EG2'",                      edit_window, eg_table, 0, 1, 0, 1);
    create_edit_place_copy_paste_buttons(Y_PORT_EG2_MODE,                                edit_window, eg_table, 0, 1, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_EG2_MODE, "Mode", Y_COMBOMODEL_TYPE_EG_MODE,     eg_table, 1, 2, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EG2_VEL_LEVEL_SENS, "Vel->Level", NULL, NULL, edit_window, eg_table, 2, 3, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EG2_VEL_TIME_SCALE, "Vel->Time", NULL, NULL,  edit_window, eg_table, 1, 2, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG2_KBD_TIME_SCALE, "Kbd->Time", NULL, NULL,  edit_window, eg_table, 2, 3, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_EG2_AMP_MOD_SRC, "Amp Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                                         eg_table, 1, 2, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EG2_AMP_MOD_AMT, "Amp Mod Amt", NULL, NULL,   edit_window, eg_table, 2, 3, 2, 3);

    vseparator = gtk_vseparator_new ();
    gtk_widget_ref (vseparator);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "vseparator", vseparator,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vseparator);
    gtk_table_attach (GTK_TABLE (eg_table), vseparator, 3, 4, 0, 3,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (GTK_FILL), 5, 0);

    create_edit_place_combo_in_table(Y_PORT_EG2_SHAPE1, "Attack 1 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 4, 5, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EG2_SHAPE2, "Attack 2 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 5, 6, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EG2_SHAPE3, "Attack 3 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 6, 7, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EG2_SHAPE4, "Release Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 7, 8, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EG2_TIME1,  "Attack 1 Time",  NULL, NULL, edit_window, eg_table, 4, 5, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG2_TIME2,  "Attack 2 Time",  NULL, NULL, edit_window, eg_table, 5, 6, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG2_TIME3,  "Attack 3 Time",  NULL, NULL, edit_window, eg_table, 6, 7, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG2_TIME4,  "Release Time",   NULL, NULL, edit_window, eg_table, 7, 8, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG2_LEVEL1, "Attack 1 Level", NULL, NULL, edit_window, eg_table, 4, 5, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EG2_LEVEL2, "Attack 2 Level", NULL, NULL, edit_window, eg_table, 5, 6, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EG2_LEVEL3, "Sustain Level ", NULL, NULL, edit_window, eg_table, 6, 7, 2, 3);

    /* EG3*/
    eg_table = create_edit_tab_and_table("EG3", 8, 3, edit_window, notebook);
    create_edit_place_label_in_table("Envelope\nGenerator \n'EG3'",                      edit_window, eg_table, 0, 1, 0, 1);
    create_edit_place_copy_paste_buttons(Y_PORT_EG3_MODE,                                edit_window, eg_table, 0, 1, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_EG3_MODE, "Mode", Y_COMBOMODEL_TYPE_EG_MODE,     eg_table, 1, 2, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EG3_VEL_LEVEL_SENS, "Vel->Level", NULL, NULL, edit_window, eg_table, 2, 3, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EG3_VEL_TIME_SCALE, "Vel->Time", NULL, NULL,  edit_window, eg_table, 1, 2, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG3_KBD_TIME_SCALE, "Kbd->Time", NULL, NULL,  edit_window, eg_table, 2, 3, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_EG3_AMP_MOD_SRC, "Amp Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                                         eg_table, 1, 2, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EG3_AMP_MOD_AMT, "Amp Mod Amt", NULL, NULL,   edit_window, eg_table, 2, 3, 2, 3);

    vseparator = gtk_vseparator_new ();
    gtk_widget_ref (vseparator);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "vseparator", vseparator,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vseparator);
    gtk_table_attach (GTK_TABLE (eg_table), vseparator, 3, 4, 0, 3,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (GTK_FILL), 5, 0);

    create_edit_place_combo_in_table(Y_PORT_EG3_SHAPE1, "Attack 1 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 4, 5, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EG3_SHAPE2, "Attack 2 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 5, 6, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EG3_SHAPE3, "Attack 3 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 6, 7, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EG3_SHAPE4, "Release Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 7, 8, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EG3_TIME1,  "Attack 1 Time",  NULL, NULL, edit_window, eg_table, 4, 5, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG3_TIME2,  "Attack 2 Time",  NULL, NULL, edit_window, eg_table, 5, 6, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG3_TIME3,  "Attack 3 Time",  NULL, NULL, edit_window, eg_table, 6, 7, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG3_TIME4,  "Release Time",   NULL, NULL, edit_window, eg_table, 7, 8, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG3_LEVEL1, "Attack 1 Level", NULL, NULL, edit_window, eg_table, 4, 5, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EG3_LEVEL2, "Attack 2 Level", NULL, NULL, edit_window, eg_table, 5, 6, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EG3_LEVEL3, "Sustain Level ", NULL, NULL, edit_window, eg_table, 6, 7, 2, 3);

    /* EG4*/
    eg_table = create_edit_tab_and_table("EG4", 8, 3, edit_window, notebook);
    create_edit_place_label_in_table("Envelope\nGenerator \n'EG4'",                      edit_window, eg_table, 0, 1, 0, 1);
    create_edit_place_copy_paste_buttons(Y_PORT_EG4_MODE,                                edit_window, eg_table, 0, 1, 2, 3);
    create_edit_place_combo_in_table(Y_PORT_EG4_MODE, "Mode", Y_COMBOMODEL_TYPE_EG_MODE,     eg_table, 1, 2, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EG4_VEL_LEVEL_SENS, "Vel->Level", NULL, NULL, edit_window, eg_table, 2, 3, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EG4_VEL_TIME_SCALE, "Vel->Time", NULL, NULL,  edit_window, eg_table, 1, 2, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG4_KBD_TIME_SCALE, "Kbd->Time", NULL, NULL,  edit_window, eg_table, 2, 3, 1, 2);
    create_edit_place_combo_in_table(Y_PORT_EG4_AMP_MOD_SRC, "Amp Mod Source", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                                         eg_table, 1, 2, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EG4_AMP_MOD_AMT, "Amp Mod Amt", NULL, NULL,   edit_window, eg_table, 2, 3, 2, 3);

    vseparator = gtk_vseparator_new ();
    gtk_widget_ref (vseparator);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "vseparator", vseparator,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vseparator);
    gtk_table_attach (GTK_TABLE (eg_table), vseparator, 3, 4, 0, 3,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (GTK_FILL), 5, 0);

    create_edit_place_combo_in_table(Y_PORT_EG4_SHAPE1, "Attack 1 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 4, 5, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EG4_SHAPE2, "Attack 2 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 5, 6, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EG4_SHAPE3, "Attack 3 Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 6, 7, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_EG4_SHAPE4, "Release Shape", Y_COMBOMODEL_TYPE_EG_SHAPE,
                                                                                     eg_table, 7, 8, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_EG4_TIME1,  "Attack 1 Time",  NULL, NULL, edit_window, eg_table, 4, 5, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG4_TIME2,  "Attack 2 Time",  NULL, NULL, edit_window, eg_table, 5, 6, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG4_TIME3,  "Attack 3 Time",  NULL, NULL, edit_window, eg_table, 6, 7, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG4_TIME4,  "Release Time",   NULL, NULL, edit_window, eg_table, 7, 8, 1, 2);
    create_edit_place_knob_in_table(Y_PORT_EG4_LEVEL1, "Attack 1 Level", NULL, NULL, edit_window, eg_table, 4, 5, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EG4_LEVEL2, "Attack 2 Level", NULL, NULL, edit_window, eg_table, 5, 6, 2, 3);
    create_edit_place_knob_in_table(Y_PORT_EG4_LEVEL3, "Sustain Level ", NULL, NULL, edit_window, eg_table, 6, 7, 2, 3);

    /* Miscellaneous */
    misc_table = create_edit_tab_and_table("Miscellaneous", 5, 3, edit_window, notebook);
    create_edit_place_knob_in_table(Y_PORT_MODMIX_BIAS, "ModMix Bias", NULL, NULL,   edit_window, misc_table, 0, 1, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_MODMIX_MOD1_SRC, "ModMix Mod 1 Src", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                                     misc_table, 1, 2, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_MODMIX_MOD1_AMT, "ModMix Mod 1 Amt", NULL, NULL,
                                                                                     edit_window, misc_table, 2, 3, 0, 1);
    create_edit_place_combo_in_table(Y_PORT_MODMIX_MOD2_SRC, "ModMix Mod 2 Src", Y_COMBOMODEL_TYPE_MOD_SRC,
                                                                                     misc_table, 3, 4, 0, 1);
    create_edit_place_knob_in_table(Y_PORT_MODMIX_MOD2_AMT, "ModMix Mod 2 Amt", NULL, NULL,
                                                                                     edit_window, misc_table, 4, 5, 0, 1);

    hseparator = gtk_hseparator_new ();
    gtk_widget_ref (hseparator);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "hseparator", hseparator,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hseparator);
    gtk_table_attach (GTK_TABLE (misc_table), hseparator, 0, 5, 1, 2,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (GTK_FILL), 0, 5);

    create_edit_place_knob_in_table(Y_PORT_GLIDE_TIME, "Glide Rate", NULL, NULL, edit_window, misc_table, 0, 1, 2, 3);
    create_edit_place_detent_in_table(Y_PORT_BEND_RANGE, "Bend Range",           edit_window, misc_table, 1, 2, 2, 3);

    /* test note widgets */
    test_note_frame = gtk_frame_new ("Test Note");
    gtk_widget_ref (test_note_frame);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "edit test_note_frame", test_note_frame,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_frame);
    // gtk_container_set_border_width (GTK_CONTAINER (test_note_frame), 5);
    gtk_box_pack_start (GTK_BOX (vbox), test_note_frame, FALSE, FALSE, 0);

    test_note_table = gtk_table_new (4, 3, FALSE);
    gtk_widget_ref (test_note_table);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "test_note_table", test_note_table,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_table);
    gtk_container_add (GTK_CONTAINER (test_note_frame), test_note_table);
    gtk_container_set_border_width (GTK_CONTAINER (test_note_table), 2);
    gtk_table_set_row_spacings (GTK_TABLE (test_note_table), 1);
    gtk_table_set_col_spacings (GTK_TABLE (test_note_table), 5);

    test_note_key_label = gtk_label_new ("key");
    gtk_widget_ref (test_note_key_label);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "edit test_note_key_label", test_note_key_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_key_label);
    gtk_table_attach (GTK_TABLE (test_note_table), test_note_key_label, 0, 1, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (test_note_key_label), 0, 0.5);
  
    test_note_velocity_label = gtk_label_new ("velocity");
    gtk_widget_ref (test_note_velocity_label);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "edit test_note_velocity_label", test_note_velocity_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_velocity_label);
    gtk_table_attach (GTK_TABLE (test_note_table), test_note_velocity_label, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (test_note_velocity_label), 0, 0.5);

    test_note_mode_button = gtk_check_button_new ();
    gtk_widget_ref (test_note_mode_button);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "test_note_mode_button", test_note_mode_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_mode_button);
    gtk_table_attach (GTK_TABLE (test_note_table), test_note_mode_button, 2, 3, 0, 2,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 4, 0);

    edit_test_note_button = gtk_button_new_with_label (" Send Test Note");
    gtk_widget_ref (edit_test_note_button);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "edit_test_note_button", edit_test_note_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (edit_test_note_button);
    gtk_table_attach (GTK_TABLE (test_note_table), edit_test_note_button, 3, 4, 0, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 4, 0);

    edit_test_note_toggle = gtk_toggle_button_new_with_label ("Toggle Test Note");
    gtk_widget_ref (edit_test_note_toggle);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "edit_test_note_toggle", edit_test_note_toggle,
                              (GtkDestroyNotify) gtk_widget_unref);
    /* gtk_widget_show (edit_test_note_toggle);  -- initially hidden */
    gtk_table_attach (GTK_TABLE (test_note_table), edit_test_note_toggle, 3, 4, 0, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 4, 0);

    edit_test_note_key_adj = gtk_adjustment_new (60, 12, 120, 1, 12, 12);
    test_note_key = gtk_hscale_new (GTK_ADJUSTMENT (edit_test_note_key_adj));
    gtk_widget_ref (test_note_key);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "edit_test_note_key", test_note_key,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_key);
    gtk_table_attach (GTK_TABLE (test_note_table), test_note_key, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_scale_set_value_pos (GTK_SCALE (test_note_key), GTK_POS_RIGHT);
    gtk_scale_set_digits (GTK_SCALE (test_note_key), 0);
    gtk_range_set_update_policy (GTK_RANGE (test_note_key), GTK_UPDATE_DELAYED);

    edit_test_note_velocity_adj = gtk_adjustment_new (96, 1, 137, 1, 10, 10);
    test_note_velocity = gtk_hscale_new (GTK_ADJUSTMENT (edit_test_note_velocity_adj));
    gtk_widget_ref (test_note_velocity);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "edit_test_note_velocity", test_note_velocity,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_velocity);
    gtk_table_attach (GTK_TABLE (test_note_table), test_note_velocity, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_scale_set_value_pos (GTK_SCALE (test_note_velocity), GTK_POS_RIGHT);
    gtk_scale_set_digits (GTK_SCALE (test_note_velocity), 0);
    gtk_range_set_update_policy (GTK_RANGE (test_note_velocity), GTK_UPDATE_DELAYED);

    /* connect test note widgets */
    gtk_signal_connect (GTK_OBJECT (edit_test_note_key_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_test_note_slider_change),
                        GINT_TO_POINTER(2));
    gtk_signal_connect (GTK_OBJECT (edit_test_note_velocity_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_test_note_slider_change),
                        GINT_TO_POINTER(3));
    gtk_signal_connect (GTK_OBJECT (test_note_mode_button), "toggled",
                        GTK_SIGNAL_FUNC (on_test_note_mode_toggled),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (edit_test_note_button), "pressed",
                        GTK_SIGNAL_FUNC (on_test_note_button_press),
                        GINT_TO_POINTER(1));
    gtk_signal_connect (GTK_OBJECT (edit_test_note_button), "released",
                        GTK_SIGNAL_FUNC (on_test_note_button_press),
                        GINT_TO_POINTER(0));
    gtk_signal_connect (GTK_OBJECT (edit_test_note_toggle), "toggled",
                        GTK_SIGNAL_FUNC (on_test_note_toggle_toggled),
                        NULL);

    /* edit action widgets */
    edit_action_frame = gtk_frame_new ("Edit Action");
    gtk_widget_ref (edit_action_frame);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "edit_action_frame", edit_action_frame,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (edit_action_frame);
    // gtk_container_set_border_width (GTK_CONTAINER (edit_action_frame), 5);
    gtk_box_pack_start (GTK_BOX (vbox), edit_action_frame, FALSE, FALSE, 0);

    edit_action_table = gtk_table_new (5, 1, FALSE);
    gtk_widget_ref (edit_action_table);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "edit_action_table", edit_action_table,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (edit_action_table);
    gtk_container_add (GTK_CONTAINER (edit_action_frame), edit_action_table);
    gtk_container_set_border_width (GTK_CONTAINER (edit_action_table), 2);
    gtk_table_set_row_spacings (GTK_TABLE (edit_action_table), 1);
    gtk_table_set_col_spacings (GTK_TABLE (edit_action_table), 5);

    edit_save_changes = gtk_button_new_with_label ("Save Changes");
    gtk_widget_ref (edit_save_changes);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "edit_save_changes", edit_save_changes,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (edit_save_changes);
    gtk_table_attach (GTK_TABLE (edit_action_table), edit_save_changes, 3, 4, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_signal_connect (GTK_OBJECT (edit_save_changes), "clicked",
                        GTK_SIGNAL_FUNC (on_edit_action_button_press),
                        (gpointer)"save");

    edit_close = gtk_button_new_with_label ("Close");
    gtk_widget_ref (edit_close);
    gtk_object_set_data_full (GTK_OBJECT (edit_window), "edit_close", edit_close,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (edit_close);
    gtk_table_attach (GTK_TABLE (edit_action_table), edit_close, 4, 5, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_signal_connect (GTK_OBJECT (edit_close), "clicked",
                        GTK_SIGNAL_FUNC (on_edit_close),
                        NULL);
}

void
create_edit_save_position_window (const char *tag)
{
  GtkWidget *vbox4;
  GtkWidget *position_text_label;
  GtkWidget *hbox2;
  GtkWidget *label50;
  GtkWidget *position_spin;
  GtkWidget *hbox3;
  GtkWidget *position_cancel;
  GtkWidget *position_ok;

    edit_save_position_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (edit_save_position_window),
                       "edit_save_position_window", edit_save_position_window);
    set_window_title(edit_save_position_window, tag, "Edit Save Position");

  vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox4);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window), "vbox4",
                            vbox4, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox4);
  gtk_container_add (GTK_CONTAINER (edit_save_position_window), vbox4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox4), 6);

  position_text_label = gtk_label_new ("Select the Program Number to which you "
                                       "wish to save your changed patch "
                                       "(existing patches will be overwritten):");
  gtk_widget_ref (position_text_label);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                            "position_text_label", position_text_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (position_text_label);
  gtk_box_pack_start (GTK_BOX (vbox4), position_text_label, TRUE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (position_text_label), GTK_JUSTIFY_FILL);
  gtk_label_set_line_wrap (GTK_LABEL (position_text_label), TRUE);
  gtk_misc_set_padding (GTK_MISC (position_text_label), 0, 6);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox2);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window), "hbox2",
                            hbox2, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox2, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2), 6);

  label50 = gtk_label_new ("Program Number");
  gtk_widget_ref (label50);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window), "label50",
                            label50, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label50);
  gtk_box_pack_start (GTK_BOX (hbox2), label50, FALSE, TRUE, 2);

  edit_save_position_spin_adj = gtk_adjustment_new (0, 0, 511, 1, 10, 0);
  position_spin = gtk_spin_button_new (GTK_ADJUSTMENT (edit_save_position_spin_adj), 1, 0);
  gtk_widget_ref (position_spin);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                            "position_spin", position_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (position_spin);
  gtk_box_pack_start (GTK_BOX (hbox2), position_spin, FALSE, FALSE, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (position_spin), TRUE);

  edit_save_position_name_label = gtk_label_new ("default voice");
  gtk_widget_ref (edit_save_position_name_label);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                            "edit_save_position_name_label",
                            edit_save_position_name_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (edit_save_position_name_label);
  gtk_box_pack_start (GTK_BOX (hbox2), edit_save_position_name_label, FALSE, FALSE, 2);
  gtk_label_set_justify (GTK_LABEL (edit_save_position_name_label), GTK_JUSTIFY_LEFT);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox3);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window), "hbox3", hbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox3);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox3, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox3), 6);

  position_cancel = gtk_button_new_with_label ("Cancel");
  gtk_widget_ref (position_cancel);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                            "position_cancel", position_cancel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (position_cancel);
  gtk_box_pack_start (GTK_BOX (hbox3), position_cancel, TRUE, FALSE, 12);

  position_ok = gtk_button_new_with_label ("Save");
  gtk_widget_ref (position_ok);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                            "position_ok", position_ok,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (position_ok);
  gtk_box_pack_end (GTK_BOX (hbox3), position_ok, TRUE, FALSE, 12);

    gtk_signal_connect (GTK_OBJECT (edit_save_position_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (edit_save_position_window), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_edit_save_position_cancel);
    gtk_signal_connect (GTK_OBJECT (edit_save_position_spin_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_position_change),
                        (gpointer)edit_save_position_name_label);
    gtk_signal_connect (GTK_OBJECT (position_ok), "clicked",
                        (GtkSignalFunc)on_edit_save_position_ok,
                        NULL);
    gtk_signal_connect (GTK_OBJECT (position_cancel), "clicked",
                        (GtkSignalFunc)on_edit_save_position_cancel,
                        NULL);
}

void
create_windows(const char *instance_tag)
{
    char tag[50];

    /* build a nice identifier string for the window titles */
    if (strlen(instance_tag) == 0) {
        strcpy(tag, "WhySynth");
    } else if (strstr(instance_tag, "WhySynth") ||
               strstr(instance_tag, "whysynth")) {
        if (strlen(instance_tag) > 49) {
            snprintf(tag, 50, "...%s", instance_tag + strlen(instance_tag) - 46); /* hope the unique info is at the end */
        } else {
            strcpy(tag, instance_tag);
        }
    } else {
        if (strlen(instance_tag) > 40) {
            snprintf(tag, 50, "WhySynth ...%s", instance_tag + strlen(instance_tag) - 37);
        } else {
            snprintf(tag, 50, "WhySynth %s", instance_tag);
        }
    }

    create_main_window(tag);
    create_about_window(tag);
    create_open_file_chooser(tag);
    create_save_file_chooser(tag);
    create_import_file_chooser();
    create_edit_window(tag);
    create_edit_save_position_window(tag);
    create_notice_window(tag);
}

