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

#ifndef _SAMPLESET_H
#define _SAMPLESET_H

#include <ladspa.h>

#include "whysynth_types.h"
#include "whysynth_voice.h"
#include "wave_tables.h"

struct _y_sample_t {
    y_sample_t    *next;

    volatile int   ref_count;

    int            mode;
    signed short  *source;
    int            max_key;
    int            param1,
                   param2,
                   param3,
                   param4;

    signed short  *data;
    int            length;
    float          period;
};

struct _y_sampleset_t {
    y_sampleset_t *next;

    volatile int   ref_count;
    volatile int   rendered;
    volatile int   set_up;

    int            mode,
                   waveform,
                   param1,
                   param2,
                   param3,
                   param4;

    signed short  *source[WAVETABLE_MAX_WAVES],
                   max_key[WAVETABLE_MAX_WAVES];
    volatile y_sample_t
                  *sample[WAVETABLE_MAX_WAVES];
};

int  sampleset_init(void);
int  sampleset_instantiate(y_synth_t *synth);
void sampleset_cleanup(y_synth_t *synth);
void sampleset_fini(void);

void sampleset_free(y_sampleset_t *ss);
y_sample_t *sampleset_find_sample(y_sampleset_t *ss, int index);

void *sampleset_worker_function(void *arg);

void sampleset_check_oscillators(y_synth_t *synth);
y_sampleset_t *sampleset_setup(y_sosc_t *sosc, int mode, int waveform,
                               int param1, int param2, int param3, int param4);
void sampleset_release(y_sampleset_t *sampleset);

#endif /* _SAMPLESET_H */
