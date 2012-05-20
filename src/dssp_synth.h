/* DSSI Plugin Framework
 *
 * Copyright (C) 2005 Sean Bolton.
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

#ifndef _DSSP_SYNTH_H
#define _DSSP_SYNTH_H

#include <stdlib.h>

#include "whysynth_types.h"

/* in dssp_synth.c: */
int   dssp_voicelist_mutex_lock(y_synth_t *synth);
int   dssp_voicelist_mutex_unlock(y_synth_t *synth);
char *dssi_configure_message(const char *fmt, ...);

#endif /* _DSSP_SYNTH_H */
