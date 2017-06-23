/* WhySynth DSSI software synthesizer plugin and GUI
 *
 * Copyright (C) 2004-2007, 2010, 2012 Sean Bolton and others.
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

#ifndef _COMMON_DATA_H
#define _COMMON_DATA_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>

#include "whysynth_types.h"
#include "whysynth_voice.h"

/* in common_data.c: */
extern y_patch_t y_init_voice;

int   y_sscanf(const char *str, const char *format, ...);
int   y_data_is_comment(char *buf);
void  y_ensure_valid_utf8(char *str, int maxlen);
void  y_data_parse_text(const char *buf, char *name, int maxlen);
int   y_data_read_patch(FILE *file, y_patch_t *patch);
char *y_data_locate_patch_file(const char *origpath, const char *project_dir);

/* in gui_data.c: */
int  gui_data_write_patch(FILE *file, y_patch_t *patch, int format);
int  gui_data_save(char *filename, int start, int end, int format,
                   char **message);
int  gui_data_save_dirty_patches_to_tmp(void);
void gui_data_check_patches_allocation(int patch_index);
int  gui_data_load(const char *filename, int position, char **message);
void gui_data_friendly_patches(void);
int  gui_data_write_patch_as_c(FILE *file, y_patch_t *patch);
int  gui_data_save_as_c(char *filename, int start, int end, char **message);
int  gui_data_import_xsynth(const char *filename, int position, int dual,
                            char **message);
int  gui_data_interpret_k4(const char *filename, int position, int dual,
                           char **message);

/* in patch_tables.c: */
extern int       y_friendly_patch_count;
extern y_patch_t y_friendly_patches[];

/* in whysynth_data.c: */
void  y_data_friendly_patches(y_synth_t *synth);
char *y_data_load(y_synth_t *synth, char *filename);

#endif /* _COMMON_DATA_H */

