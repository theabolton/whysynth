/* WhySynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004-2005 Sean Bolton and others.
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

#ifndef _GUI_MAIN_H
#define _GUI_MAIN_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lo/lo.h>

#include "whysynth_types.h"

extern char *     osc_host_url;
extern char *     osc_self_url;
extern lo_address osc_host_address;
extern char *     osc_configure_path;
extern char *     osc_control_path;
extern char *     osc_exiting_path;
extern char *     osc_hide_path;
extern char *     osc_midi_path;
extern char *     osc_program_path;
extern char *     osc_quit_path;
extern char *     osc_show_path;
extern char *     osc_update_path;

extern unsigned int patch_count;
extern unsigned int patches_allocated;
extern int          patches_dirty;
extern y_patch_t   *patches;
extern char         patches_tmp_filename[];
extern char        *project_directory;

extern int last_configure_load_was_from_tmp;

#endif /* _GUI_MAIN_H */

