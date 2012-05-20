/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2005 Sean Bolton and others.
 *
 * Portions of this file come from MSS, copyright (C) 2002 Mats
 * Olsson.
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

#ifndef _AGRAN_OSCILLATOR_H
#define _AGRAN_OSCILLATOR_H

#include "whysynth.h"
#include "whysynth_voice.h"

#define AG_DEFAULT_GRAIN_COUNT  (Y_MAX_POLYPHONY * 10)

#define AG_GRAIN_ENVELOPE_COUNT 31

enum grain_envelope_type {
    AG_ENVELOPE_RECTANGULAR,
    AG_ENVELOPE_LINEAR,
    AG_ENVELOPE_TRIANGULAR,
    AG_ENVELOPE_GAUSSIAN,
    AG_ENVELOPE_ROADSIAN   /* Curtis, not Randy */
};

/* Paraphrased from MSS:
 *
 * "For the AG_ENVELOPE_LINEAR envelope the grain_envelope_decriptor_t.rate is
 * the portion of the grain length needed to linearly increase the envelope
 * amplitude from 0 to 1 when the grain starts (and correspondingly decrease
 * envelope amplitude from 1 to 0 when the grain ends).
 *
 * For the AG_ENVELOPE_TRIANGULAR envelope the grain_envelope_decriptor_t.rate
 * is the portion of the grain length needed to linearly increase the
 * envelope amplitude from 0 to 1 when the grain starts. The rest of the grain
 * length the envelope amplitude is then linearly decreased from 1 to 0 (this
 * means that for a 0.5 grain_envelope_decriptor_t.rate the linear and
 * triangular envelopes are identical).
 *
 * For the AG_ENVELOPE_GAUSSIAN envelope the grain_envelope_decriptor_t.rate
 * is the amplitude at the start and the end of the grain (relative to the 1.0
 * in the middle)."
 *
 * AG_ENVELOPE_ROADSIAN is like the linear envelope, but with Gaussian attack
 * and release instead.
 */

struct _grain_envelope_descriptor_t
{
    const char *             name;
    enum grain_envelope_type type;
    float                    rate;
    float                    length; /* in milliseconds */
};

typedef struct _grain_envelope_descriptor_t grain_envelope_descriptor_t;

struct _grain_envelope_data_t
{
    unsigned long length; /* in samples */
    float *       data;
};

struct _grain_t
{
    struct _grain_t *next;
    int              env_pos;  /* index into envelope */
    float            wave_pos; /* wavetable wave phase */
    float            w;        /* per-sample phase increment for this grain */
};

extern grain_envelope_descriptor_t grain_envelope_descriptors[AG_GRAIN_ENVELOPE_COUNT];

void agran_oscillator(unsigned long sample_count,
                      y_synth_t *synth, y_sosc_t *sosc,
                      y_voice_t *voice, struct vosc *vosc, int index, float w);
int  new_grain_array(y_synth_t *synth, int grain_count);
grain_envelope_data_t *
     create_grain_envelopes(unsigned long sample_rate);
void free_grain_envelopes(grain_envelope_data_t *envelopes);
void free_active_grains(y_synth_t *synth, y_voice_t *voice);

#endif /* _AGRAN_OSCILLATOR_H */
