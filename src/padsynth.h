/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2006 Sean Bolton.
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

#ifndef _PADSYNTH_H
#define _PADSYNTH_H

#include "whysynth_types.h"

int  padsynth_init(void);
void padsynth_fini(void);
void padsynth_free_temp(void);
void padsynth_sampletable_setup(y_sampleset_t *sampleset);
int  padsynth_render(y_sample_t *sample);
void padsynth_oscillator(unsigned long sample_count, y_sosc_t *sosc,
                         y_voice_t *voice, struct vosc *vosc,
                         int index, float w);

#endif /* _PADSYNTH_H */
