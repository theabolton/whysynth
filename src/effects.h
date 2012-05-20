/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2005, 2008, 2010 Sean Bolton and others.
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

#ifndef _EFFECTS_H
#define _EFFECTS_H

#include <stdlib.h>

#include <ladspa.h>

#include "whysynth_types.h"

/*
 * effects_start_allocation
 */
static inline void
effects_start_allocation(y_synth_t *synth)
{
    synth->effect_buffer_allocation = 0;
}

/*
 * effects_request_silencing_of_subsequent_allocations
 */
static inline void
effects_request_silencing_of_subsequent_allocations(y_synth_t *synth)
{
    synth->effect_buffer_silence_count = synth->effect_buffer_allocation;
}

/* in effects.c: */
void *effects_request_buffer(y_synth_t *synth, size_t size);
int   effects_setup(y_synth_t *synth);
void  effects_cleanup(y_synth_t *synth);
void  effects_process(y_synth_t *synth, unsigned long sample_count,
                      LADSPA_Data *out_left, LADSPA_Data *out_right);

/* in effect_reverb.c: */
void effect_delay_request_buffers(y_synth_t *synth);
void effect_delay_setup(y_synth_t *synth);
void effect_delay_process(y_synth_t *synth, unsigned long sample_count,
                          LADSPA_Data *out_left, LADSPA_Data *out_right);
void effect_reverb_request_buffers(y_synth_t *synth);
void effect_reverb_setup(y_synth_t *synth);
void effect_reverb_process(y_synth_t *synth, unsigned long sample_count,
                           LADSPA_Data *out_left, LADSPA_Data *out_right);

/* in effect_screverb.c: */
void effect_screverb_request_buffers(y_synth_t *synth);
void effect_screverb_setup(y_synth_t *synth);
void effect_screverb_process(y_synth_t *synth, unsigned long sample_count,
                             LADSPA_Data *out_left, LADSPA_Data *out_right);

#endif /* _EFFECTS_H */
