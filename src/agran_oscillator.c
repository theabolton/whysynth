/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2005-2007, 2016, 2018 Sean Bolton and others.
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

#define _DEFAULT_SOURCE 1
#define _ISOC99_SOURCE  1

#include <stdlib.h>
#include <math.h>

#include "whysynth.h"
#include "dssp_event.h"
#include "whysynth_voice.h"
#include "wave_tables.h"
#include "agran_oscillator.h"

#include "whysynth_voice_inline.h"

static inline void
free_osc_active_grain_list(y_synth_t *synth, struct vosc *osc)
{
    if (osc->grain_list) {
        grain_t *first, *last;

        first = osc->grain_list;
        last = first;
        while (last->next) last = last->next;
        last->next = synth->free_grain_list;
        synth->free_grain_list = first;
        osc->grain_list = NULL;
    }
}

/*
 * agran_oscillator
 *
 * asynchronous granular synthesis parameters:
 *    duration is set by envelope (mmod src)
 *    lz is set directly          (mparam1)
 *    density is lz / duration
 *    (lz = density * duration)
 *    (duration = lz / density)
 */
void
agran_oscillator(unsigned long sample_count,
                 y_synth_t *synth, y_sosc_t *sosc,
                 y_voice_t *voice, struct vosc *vosc, int index, float w)
{
    grain_t *grain, *prev;
    grain_envelope_data_t *envelope;
    int   i,
          next_onset = vosc->i0;
    float level_a, level_b, level_a_delta, level_b_delta;

    i = lrintf(*(sosc->mmod_src));
    if (i < 0 || i >= AG_GRAIN_ENVELOPE_COUNT)
        envelope = &global.grain_envelope[0];
    else
        envelope = &global.grain_envelope[i];

    i = voice->key + lrintf(*(sosc->pitch));
    if (vosc->mode != vosc->last_mode) {

        if (vosc->grain_list)
            free_osc_active_grain_list(synth, vosc);

        /* cause new-grain loop below to create several initial grains. */
        next_onset = (int)(envelope->length - Y_CONTROL_PERIOD) / -2;

        /* select wave(s) and crossfade from wavetable */
        wavetable_select(vosc, i);
        
        vosc->last_mode = vosc->mode;
        vosc->last_waveform = vosc->waveform;

    } else if (vosc->waveform != vosc->last_waveform ||
               i              != vosc->wave_select_key) {

        wavetable_select(vosc, i);
        vosc->last_waveform = vosc->waveform;
    }

    while (next_onset < (int)sample_count) {
        float grain_freq_dist = 0.5f * *(sosc->mmod_amt) * *(sosc->mmod_amt),
              grain_lz = *(sosc->mparam1) * 20.0f,
              grain_spread = *(sosc->mparam2);

        /* Generate a new grain (or grains) with a random period, and add it to
         * the active grain list. */
        if (synth->free_grain_list == NULL) {
            /* int x=0; grain=vosc->grain_list; while(grain) { x++; grain=grain->next; }
             * fprintf(stderr, "agran_oscillator: free grain list exhausted (oscillator has %d grains in list)!\n", x); */
            goto no_free_grains;
        }
        grain = synth->free_grain_list;
        synth->free_grain_list = grain->next;
        grain->next = vosc->grain_list;
        vosc->grain_list = grain;

        grain->env_pos = Y_CONTROL_PERIOD - next_onset;
        if (next_onset >= 0) {
            grain->wave_pos = -w * next_onset;
            while (grain->wave_pos < 0.0f) grain->wave_pos += 1.0f;
        } else {
            grain->wave_pos = fmodf(-w * next_onset, 1.0f);
        }
        if (grain_freq_dist < 1e-4) {
            grain->w = w;
        } else {
            grain->w = random_float(-1.0f, 2.0f);
            grain->w *= fabsf(grain->w);
            grain->w = w * (1.0f + grain_freq_dist * grain->w); /* -FIX- does not center on frequency */
        }

        /* Set the onset of the next grain. */
        if (grain_lz < 1e-3f) grain_lz = 1e-3f; /* prevent division by zero */
        next_onset += lrintf((float)(envelope->length - Y_CONTROL_PERIOD) / grain_lz *
                                     (1.0f + grain_spread * random_float(-0.5f, 1.0f)));
        if (next_onset > 192000)
            next_onset = 192000;                /* prevent overlong onset times */

        /* fprintf(stderr, "-- next_onset now %d (lz %f, density %f)\n", vosc->next_onset, grain_lz, density); */
    }
    next_onset -= sample_count;
  no_free_grains:
    vosc->i0 = next_onset;

    /* Calculate modulators */
    if (vosc->grain_list) {
        int mod = y_voice_mod_index(sosc->amp_mod_src);
        float amt = *(sosc->amp_mod_amt);

        if (amt > 0.0f)
            level_a = 1.0f - amt + amt * voice->mod[mod].value;
        else
            level_a = 1.0f + amt * voice->mod[mod].value;
        level_a_delta = volume_cv_to_amplitude(level_a + amt * voice->mod[mod].delta * (float)sample_count);
        level_a       = volume_cv_to_amplitude(level_a);
        level_b       = level_a       * *(sosc->level_b) / 65534.0f;
        level_b_delta = level_a_delta * *(sosc->level_b) / 65534.0f;
        level_a       *= *(sosc->level_a) / 65534.0f;
        level_a_delta *= *(sosc->level_a) / 65534.0f;
        level_a_delta = (level_a_delta - level_a) / (float)sample_count;
        level_b_delta = (level_b_delta - level_b) / (float)sample_count;
        /* -FIX- condition to [0, 1]? */
    } else {
        level_a = level_b = level_a_delta = level_b_delta = 0.0f; /* shut GCC up */
    }

    /* Walk the grain list, and render each grain for this burst. */
    grain = vosc->grain_list;
    prev = NULL;
    while (grain) {
        unsigned long sample, tmp_count;
        signed short *wave0 = vosc->wave0,
                     *wave1 = vosc->wave1;
        float wave_pos = grain->wave_pos,
              wavemix0 = vosc->wavemix0,
              wavemix1 = vosc->wavemix1;
        float f, level_a_tmp, level_b_tmp;

        if (envelope->length < grain->env_pos) /* as a result of gr_envelope change */
            tmp_count = 0;
        else
            tmp_count = envelope->length - grain->env_pos;

        if (sample_count >= tmp_count) {
            grain_t *next;

            /* render grain to end of envelope, then free grain */
            level_a_tmp = level_a;
            level_b_tmp = level_b;
            if (wave0 == wave1) {
                for (sample = 0; sample < tmp_count; sample++) {

                    wave_pos += grain->w;

                    if (wave_pos >= 1.0f) {
                        wave_pos -= 1.0f;
                        /* async granular oscillators do not export sync */
                    }

                    f = wave_pos * (float)WAVETABLE_POINTS;
                    i = lrintf(f - 0.5f);
                    f -= (float)i;

                    f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f);
                    f *= envelope->data[grain->env_pos + sample];
                    voice->osc_bus_a[index + sample] += level_a_tmp * f;
                    voice->osc_bus_b[index + sample] += level_b_tmp * f;

                    level_a_tmp += level_a_delta;
                    level_b_tmp += level_b_delta;
                }
            } else {
                for (sample = 0; sample < tmp_count; sample++) {

                    wave_pos += grain->w;

                    if (wave_pos >= 1.0f) {
                        wave_pos -= 1.0f;
                        /* async granular oscillators do not export sync */
                    }

                    f = wave_pos * (float)WAVETABLE_POINTS;
                    i = lrintf(f - 0.5f);
                    f -= (float)i;

                    f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
                        ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
                    f *= envelope->data[grain->env_pos + sample];
                    voice->osc_bus_a[index + sample] += level_a_tmp * f;
                    voice->osc_bus_b[index + sample] += level_b_tmp * f;

                    level_a_tmp += level_a_delta;
                    level_b_tmp += level_b_delta;
                }
            }

            grain->wave_pos = wave_pos;
            grain->env_pos += tmp_count;

            next = grain->next;

            if (prev)
                prev->next = next;
            else
                vosc->grain_list = next;
            grain->next = synth->free_grain_list;
            synth->free_grain_list = grain;
            grain = next;

        } else {
            /* render sample_count samples */

            level_a_tmp = level_a;
            level_b_tmp = level_b;
            if (wave0 == wave1) {
                for (sample = 0; sample < sample_count; sample++) {

                    wave_pos += grain->w;

                    if (wave_pos >= 1.0f) {
                        wave_pos -= 1.0f;
                        /* async granular oscillators do not export sync */
                    }

                    f = wave_pos * (float)WAVETABLE_POINTS;
                    i = lrintf(f - 0.5f);
                    f -= (float)i;

                    f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f);
                    f *= envelope->data[grain->env_pos + sample];
                    voice->osc_bus_a[index + sample] += level_a_tmp * f;
                    voice->osc_bus_b[index + sample] += level_b_tmp * f;

                    level_a_tmp += level_a_delta;
                    level_b_tmp += level_b_delta;
                }
            } else {
                for (sample = 0; sample < sample_count; sample++) {

                    wave_pos += grain->w;

                    if (wave_pos >= 1.0f) {
                        wave_pos -= 1.0f;
                        /* async granular oscillators do not export sync */
                    }

                    f = wave_pos * (float)WAVETABLE_POINTS;
                    i = lrintf(f - 0.5f);
                    f -= (float)i;

                    f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
                        ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
                    f *= envelope->data[grain->env_pos + sample];
                    voice->osc_bus_a[index + sample] += level_a_tmp * f;
                    voice->osc_bus_b[index + sample] += level_b_tmp * f;

                    level_a_tmp += level_a_delta;
                    level_b_tmp += level_b_delta;
                }
            }

            grain->wave_pos = wave_pos;
            grain->env_pos += sample_count;

            prev = grain;
            grain = grain->next;
        }
    }
}

int
new_grain_array(y_synth_t *synth, int grain_count)
{
    /* assumes voicelist is locked and any existing grains are freed */
    int i;

    if (synth->grains)
        free(synth->grains);

    synth->free_grain_list = NULL;

    synth->grains = (grain_t *)calloc(grain_count, sizeof(grain_t));

    if (!synth->grains)
        return 0;

    synth->free_grain_list = synth->grains;
    for (i = 1; i < grain_count; i++)
        synth->grains[i - 1].next = &synth->grains[i];

    return 1;
}

void
free_grain_envelopes(grain_envelope_data_t *env)
{
    int e;

    for (e = 0; e < AG_GRAIN_ENVELOPE_COUNT; e++) {
        if (env[e].data)
            free(env[e].data);
    }
    free(env);
}

grain_envelope_data_t *
create_grain_envelopes(unsigned long sample_rate)
{
    grain_envelope_descriptor_t *desc;
    grain_envelope_data_t *env;
    int e, i;

    env = (grain_envelope_data_t *)calloc(AG_GRAIN_ENVELOPE_COUNT,
                                          sizeof(grain_envelope_data_t));
    if (!env)
        return NULL;

    for (e = 0; e < AG_GRAIN_ENVELOPE_COUNT; e++) {
        desc = &grain_envelope_descriptors[e];
        env[e].length = lrintf((float)sample_rate * desc->length / 1000.0f) ;
        env[e].data = malloc((env[e].length + Y_CONTROL_PERIOD) * sizeof(float));
        if (!env[e].data)
            goto out_of_memory;

        /* zero pre-onset segment */
        for (i = 0; i < Y_CONTROL_PERIOD; i++)
           env[e].data[i] = 0.0f;

        /* create grain envelope starting at offset Y_CONTROL_PERIOD */
        switch (desc->type) {
          default:
          case AG_ENVELOPE_RECTANGULAR:
            for (i = 0; i < env[e].length; i++)
                env[e].data[Y_CONTROL_PERIOD + i] = 1.0f;
            break;

          case AG_ENVELOPE_LINEAR:
            {
                const int envLength = lrintf((desc->rate * (float)env[e].length));
                const float c = 1.0f / (float)envLength;
                for (i = 0; i < envLength; i++) {
                    env[e].data[Y_CONTROL_PERIOD + i] = (float)i * c;
                }
                for (i = envLength; i < env[e].length - envLength; i++) {
                    env[e].data[Y_CONTROL_PERIOD + i] = 1.0f;
                }
                for (i = env[e].length - envLength; i < env[e].length; i++) {
                    env[e].data[Y_CONTROL_PERIOD + i] = (float)(env[e].length - i) * c;
                }
            }
            break;

          case AG_ENVELOPE_TRIANGULAR:
            {
                const int env1Length = lrintf((desc->rate * (float)env[e].length));
                const int env2Length = env[e].length - env1Length;
                const float c1 = 1.0f / (float)env1Length;
                const float c2 = 1.0f / (float)env2Length;
                for (i = 0; i < env1Length; i++) {
                    env[e].data[Y_CONTROL_PERIOD + i] = (float)i * c1;
                }
                for (i = env1Length; i < env[e].length; i++) {
                    env[e].data[Y_CONTROL_PERIOD + i] = 1.0f - (float)(i - env1Length) * c2;
                }
            }
            break;

          case AG_ENVELOPE_GAUSSIAN:
            {
                const float minAmpl = desc->rate;
                /* Calculate scale factor so that the envelope amplitude has 
                 * decreased to minAmpl at the start and at the end of the grain, i.e
                 * solve exp(-sqr(t)/2) = minAmpl */
                const float scale = 2.0f * sqrtf(-2.0f * logf(minAmpl)) / (float)env[e].length;
                const int peak = (env[e].length - 1) / 2;
                for (i = 0; i <= peak; i++) {
                  const float t = (float)i * scale;
                  const float ampl = expf(-t*t/2);
                  env[e].data[Y_CONTROL_PERIOD + peak + i] = ampl;
                  env[e].data[Y_CONTROL_PERIOD + peak - i] = ampl;
                }
                if (env[e].length % 2 == 0) { /* if length is even, zero last frame of buffer */
                    env[e].data[Y_CONTROL_PERIOD + env[e].length - 1] = 0.0f;
                }
            }
            break;

          case AG_ENVELOPE_ROADSIAN: /* Gaussian start and end with rectangular middle */
            {
                int gaussianLength = lrintf((desc->rate * (float)env[e].length));

                if (gaussianLength) {
                    for (i = 1; i <= gaussianLength; i++) {
                        float t = (float)i / (float)gaussianLength * 4.70158f; /* yields -96dB edges */
                        const float ampl = expf(-t * t / 2.0f);
                        env[e].data[Y_CONTROL_PERIOD + gaussianLength - i] = ampl;
                        env[e].data[Y_CONTROL_PERIOD + env[e].length - 1
                                                     - gaussianLength + i] = ampl;
                    }
                }
                for (i = gaussianLength; i < env[e].length - gaussianLength; i++) {
                    env[e].data[Y_CONTROL_PERIOD + i] = 1.0f;
                }
            }
            break;
        }

        env[e].length += Y_CONTROL_PERIOD;
    }
    return env;

  out_of_memory:
    for (; e >= 0; e--) {
        if (env[e].data)
            free(env[e].data);
    }
    free(env);
    return NULL;
}

void
free_active_grains(y_synth_t *synth, y_voice_t *voice)
{
    free_osc_active_grain_list(synth, &voice->osc1);
    free_osc_active_grain_list(synth, &voice->osc2);
    free_osc_active_grain_list(synth, &voice->osc3);
    free_osc_active_grain_list(synth, &voice->osc4);
}

