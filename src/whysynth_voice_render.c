/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004-2007, 2010, 2012 Sean Bolton and others.
 *
 * Portions of this file come from Steve Brookes' Xsynth,
 * copyright (C) 1999 S. J. Brookes.
 * Portions of this file come from Fons Adriaensen's VCO-plugins
 * and MCP-plugins, copyright (C) 2003 Fons Adriaensen.
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
 * Portions of this file may have come from Csound, copyright
 * (C) 1999 Sean Costello, rasmus ekman, et. al.
 * Portions of this file come from Nick Dowell's amSynth,
 * copyright (c) 2001,2002 Nick Dowell.
 * Chamberlin filter refactoring with high-pass and band-reject
 * versions copyright (c) 2011 Luke Andrew.
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

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#include <stdlib.h>
#include <math.h>

#include <ladspa.h>

#include "whysynth.h"
#include "dssp_event.h"
#include "whysynth_voice.h"
#include "wave_tables.h"
#include "agran_oscillator.h"
#include "padsynth.h"

#include "whysynth_voice_inline.h"

#define M_2PI_F (2.0f * (float)M_PI)
#define M_PI_F (float)M_PI

static int   tables_initialized = 0;

float        y_pitch[129];

float        sine_wave[4 + SINETABLE_POINTS + 1];

float        volume_cv_to_amplitude_table[257];

#define VOLUME_TO_AMPLITUDE_SCALE 128

static float volume_to_amplitude_table[4 + VOLUME_TO_AMPLITUDE_SCALE + 2]; /* -FIX- get rid of this */

void
y_init_tables(void)
{
    int i;
    float pexp;
    float volume, volume_exponent;

    if (tables_initialized)
        return;

    /* oscillator waveforms */
    for (i = 0; i <= SINETABLE_POINTS; ++i) {
        sine_wave[i + 4] = sinf(M_2PI_F * (float)i / (float)SINETABLE_POINTS) * 0.5f;
    }
    sine_wave[-1 + 4] = sine_wave[SINETABLE_POINTS - 1 + 4];  /* guard points both ends */

    /* MIDI note to pitch */
    for (i = 0; i <= 128; ++i) {
        pexp = (float)(i - 69) / 12.0f;
        y_pitch[i] = powf(2.0f, pexp);
    }

    /* volume to amplitude
     *
     * This generates a curve which is:
     *  volume_to_amplitude_table[128 + 4] = 0.25 * 3.16...   ~=  -2dB
     *  volume_to_amplitude_table[64 + 4]  = 0.25 * 1.0       ~= -12dB  (yields -18dB per voice nominal)
     *  volume_to_amplitude_table[32 + 4]  = 0.25 * 0.316...  ~= -22dB
     *  volume_to_amplitude_table[16 + 4]  = 0.25 * 0.1       ~= -32dB
     *   etc.
     */
    volume_exponent = 1.0f / (2.0f * log10f(2.0f));
    for (i = 0; i <= VOLUME_TO_AMPLITUDE_SCALE; i++) {
        volume = (float)i / (float)VOLUME_TO_AMPLITUDE_SCALE;
        volume_to_amplitude_table[i + 4] = powf(2.0f * volume, volume_exponent) / 4.0f;
    }
    volume_to_amplitude_table[ -1 + 4] = 0.0f;
    volume_to_amplitude_table[129 + 4] = volume_to_amplitude_table[128 + 4];

#if 0
/* -FIX- may still need this or something like it (vel scaling code in whysynth_voice.c is baroque) */
static float velocity_to_attenuation[128];
    float ol, amp;

    /* velocity to attenuation
     *
     * Creates the velocity to attenuation lookup table, for converting
     * velocities [1, 127] to full-velocity-sensitivity (VS=7) attenuation
     * in quarter decibels.  Modeled after my TX-7's velocity response.*/
    velocity_to_attenuation[0] = 253.9999f; /* -FIX- ? */
    for (i = 1; i < 127; i++) {
        if (i >= 10) {
            ol = (powf(((float)i / 127.0f), 0.32f) - 1.0f) * 100.0f;
            amp = powf(2.0f, ol / 8.0f);
        } else {
            ol = (powf(((float)10 / 127.0f), 0.32f) - 1.0f) * 100.0f;
            amp = powf(2.0f, ol / 8.0f) * (float)i / 10.0f;
        }
        velocity_to_attenuation[i] = log10f(amp) * -80.0f;
    }
    velocity_to_attenuation[127] = 0.0f;
#endif

    /* CV to amplitude
     *
     * This creates the 'control voltage' to amplitude curve, for converting
     * CVs of (-1.27, 1.27) to output amplitudes, as:
     *    CV      amplitude
     *    1.0        1.0     = 0dB
     *    0.92       0.5     = -6dB
     *    0.84       0.25    = -12dB
     *    0.0        0.0     = -infinity dB
     *   -0.92      -0.5     = -6dB
     *   -1.0       -1.0     = 0dB
     * Table indices are scaled by a factor of 100, and are bipolar, with the
     * zero point at index 128.  The curve is modeled after the 'output level'
     * to amplitude curve of the DX-7. */
    volume_cv_to_amplitude_table[128] = 0.0f;
    for (i = 1; i < 6; i++)
        volume_cv_to_amplitude_table[128 + i] = powf(2.0f, -108.0f / 8.0f) * (float)i / 6.0f;
    for ( ; i < 20; i++)
        volume_cv_to_amplitude_table[128 + i] = powf(2.0f, (float)(i * 2 - 120) / 8.0f);
    for ( ; i <= 128; i++)
        volume_cv_to_amplitude_table[128 + i] = powf(2.0f, (float)(i - 100) / 8.0f);
    for (i = 1; i <= 128; i++)
        volume_cv_to_amplitude_table[128 - i] = -volume_cv_to_amplitude_table[128 + i];

    tables_initialized = 1;
}

static inline float
volume(float level)
{
    unsigned char segment;
    float fract;

    level *= (float)VOLUME_TO_AMPLITUDE_SCALE;
    segment = lrintf(level - 0.5f);
    fract = level - (float)segment;

    return volume_to_amplitude_table[segment + 4] + fract *
               (volume_to_amplitude_table[segment + 5] -
                volume_to_amplitude_table[segment + 4]);
}

static inline float
pan_cv_to_amplitude(float cv)
{
    /* equal power panning code after Ardour, copyright (c) 2004 Paul Davis */

    const float scale = -0.831783137; /* 2 - 4 * 10 ^ (-3 / 20) */

    return cv * (scale * cv + 1.0f - scale);
}

/*
 * y_voice_waveform_index
 */
static inline int
y_voice_waveform_index(LADSPA_Data *p)
{
    int i = lrintf(*p);

    if (i < 0 || i >= wavetables_count)
        return 0;

    return i;
}

/* ==== Modulators ==== */

static inline float
y_voice_lfo_get_value(float pos, int waveform)
{
    float f;
    int i;
    signed short *wave = wavetable[waveform].wave[0].data;

    pos *= (float)WAVETABLE_POINTS;
    i = lrintf(pos - 0.5f);
    f = pos - (float)i;
    return ((float)wave[i] + (float)(wave[i + 1] - wave[i]) * f) / 32767.0f;
}

/*
 * y_voice_setup_lfo
 *
 * need not be called only during control tick
 */
void
y_voice_setup_lfo(y_synth_t *synth, y_slfo_t *slfo, struct vlfo *vlfo,
                  float phase, float randfreq,
                  struct vmod *srcmods, struct vmod *destmods)
{
    int mod = y_voice_mod_index(slfo->amp_mod_src),
        waveform = y_voice_waveform_index(slfo->waveform);
    float mult0, mult1;
    struct vmod *bpmod = destmods,
                *upmod = destmods + 1;

    vlfo->freqmult = random_float(1.0f - randfreq * 0.5f, randfreq);
    vlfo->pos = phase + *(slfo->frequency) * vlfo->freqmult / synth->control_rate;
    vlfo->pos = fmodf(vlfo->pos, 1.0f);
    vlfo->delay_count = lrintf(*(slfo->delay) * synth->control_rate);

    mult1 = *(slfo->amp_mod_amt);
    if (mult1 > 0.0f) {
        mult0 = 1.0f - mult1 + mult1 * srcmods[mod].value;
        mult1 = 1.0f - mult1 + mult1 * srcmods[mod].next_value;
    } else {
        mult0 = 1.0f + mult1 * srcmods[mod].value;
        mult1 = 1.0f + mult1 * srcmods[mod].next_value;
    }
    
    if (vlfo->delay_count == 0) {

        bpmod->value = y_voice_lfo_get_value(phase, waveform) * mult0;
        bpmod->next_value = y_voice_lfo_get_value(vlfo->pos, waveform) * mult1;
        bpmod->delta = (bpmod->next_value - bpmod->value) / (float)synth->control_remains;
        upmod->value = (bpmod->value + mult0) * 0.5f;
        upmod->next_value = (bpmod->next_value + mult1) * 0.5f;
        upmod->delta = (upmod->next_value - upmod->value) / (float)synth->control_remains;

    } else {

        if (synth->control_remains != Y_CONTROL_PERIOD) {
            mult0 = (float)(Y_CONTROL_PERIOD - synth->control_remains) /
                          (float)Y_CONTROL_PERIOD;
            vlfo->delay_length = (float)vlfo->delay_count + mult0;
            mult0 = mult0 / vlfo->delay_length;
        } else {
            vlfo->delay_length = (float)vlfo->delay_count;
            vlfo->delay_count--;
            mult0 = 1.0f / vlfo->delay_length;
        }
        mult1 *= mult0;

        bpmod->value = 0.0f;
        bpmod->next_value = y_voice_lfo_get_value(vlfo->pos, waveform) * mult1;
        bpmod->delta = bpmod->next_value / (float)synth->control_remains;
        upmod->value = 0.0f;
        upmod->next_value = (bpmod->next_value + mult1) * 0.5f;
        upmod->delta = upmod->next_value / (float)synth->control_remains;
    }
};

/*
 * y_voice_update_lfo
 *
 * may only be called during control tick
 */
void
y_voice_update_lfo(y_synth_t *synth, y_slfo_t *slfo, struct vlfo *vlfo,
                   struct vmod *srcmods, struct vmod *destmods)
{
    int mod = y_voice_mod_index(slfo->amp_mod_src),
        waveform = y_voice_waveform_index(slfo->waveform);
    float mult;
    struct vmod *bpmod = destmods,
                *upmod = destmods + 1;

    vlfo->pos += *(slfo->frequency) * vlfo->freqmult / synth->control_rate;
    if (vlfo->pos >= 1.0f) vlfo->pos -= 1.0f;

    mult = *(slfo->amp_mod_amt);
    if (mult > 0.0f) {
        mult = 1.0f - mult + mult * srcmods[mod].next_value;
    } else {
        mult = 1.0f + mult * srcmods[mod].next_value;
    }
    if (vlfo->delay_count != 0) {
        mult *= 1.0f - (float)vlfo->delay_count / vlfo->delay_length;
        vlfo->delay_count--;
    }

    bpmod->value = bpmod->next_value;
    bpmod->next_value = y_voice_lfo_get_value(vlfo->pos, waveform) * mult;
    bpmod->delta = (bpmod->next_value - bpmod->value) / (float)Y_CONTROL_PERIOD;
    upmod->value = upmod->next_value;
    upmod->next_value = (bpmod->next_value + mult) * 0.5f;
    upmod->delta = (upmod->next_value - upmod->value) / (float)Y_CONTROL_PERIOD;
}

/*
 * y_voice_eg_set_next_segment
 *
 * assumes a DSSP_EG_RUNNING envelope
 * may only be called during control tick
 */
static void
y_voice_eg_set_next_segment(y_seg_t *seg, y_voice_t *voice, struct veg *veg,
                            struct vmod *destmod)
{
    if (veg->segment >= 3) {

        veg->state = DSSP_EG_FINISHED;
        destmod->value = destmod->next_value = destmod->delta = 0.0f;
        // YDB_MESSAGE(YDB_NOTE, " next_segment: eg %p to finished\n", veg);

    } else if (veg->segment == veg->sustain_segment) {

        int i;
        float mult;
        
        veg->state = DSSP_EG_SUSTAINING;

        i = y_voice_mod_index(seg->amp_mod_src);
        mult = *(seg->amp_mod_amt);
        if (mult > 0.0f) {
            mult = 1.0f - mult + mult * voice->mod[i].next_value;
        } else {
            mult = 1.0f + mult * voice->mod[i].next_value;
        }

        destmod->value = destmod->next_value;
        destmod->next_value = veg->d * mult;
        destmod->delta = (destmod->next_value - destmod->value) / (float)Y_CONTROL_PERIOD;
        // YDB_MESSAGE(YDB_NOTE, " next_segment: eg %p to sustain\n", veg);

    } else {

        int mode = lrintf(*(seg->mode)),
            time, i;
        float f, inv_duration, level, mult;

        veg->segment++;
        destmod->value = destmod->next_value;

        if (veg->segment == 1 && mode == 1) {  /* second segment of simple ADSR */
            time = 1;
            level = veg->level_scale;
        } else {
            time = lrintf(*(seg->time[veg->segment]) * veg->time_scale);
            if (time < 1) time = 1;
            level = *(seg->level[veg->segment]) * veg->level_scale;
        }
        veg->count = time - 1;

        f = veg->target - level;  /* segment delta * -1 */
        i = veg->shape[veg->segment];
        inv_duration = 1.0f / (float)time;

        veg->target = level;
        veg->d = eg_shape_coeffs[i][3] * f + level;
        f *= inv_duration;
        veg->c = eg_shape_coeffs[i][2] * f;
        f *= inv_duration;
        veg->b = eg_shape_coeffs[i][1] * f;
        f *= inv_duration;
        veg->a = eg_shape_coeffs[i][0] * f;

        i = y_voice_mod_index(seg->amp_mod_src);
        mult = *(seg->amp_mod_amt);
        if (mult > 0.0f) {
            mult = 1.0f - mult + mult * voice->mod[i].next_value;
        } else {
            mult = 1.0f + mult * voice->mod[i].next_value;
        }

        f = (float)veg->count;
        destmod->next_value = (((veg->a * f + veg->b) * f + veg->c) * f + veg->d) * mult;
        destmod->delta = (destmod->next_value - destmod->value) / (float)Y_CONTROL_PERIOD;
        
        // YDB_MESSAGE(YDB_NOTE, " next_segment: eg %p to segment %d\n", veg, veg->segment);
    }
}

/*
 * y_voice_update_eg
 *
 * may only be called during control tick
 */
static void
y_voice_update_eg(y_seg_t *seg, y_voice_t *voice, struct veg *veg,
                  struct vmod *destmod)
{
    if (veg->state == DSSP_EG_FINISHED) {

        return;
    
    } else if (veg->state == DSSP_EG_SUSTAINING) {

        int i;
        float mult;
        
        i = y_voice_mod_index(seg->amp_mod_src);
        mult = *(seg->amp_mod_amt);
        if (mult > 0.0f) {
            mult = 1.0f - mult + mult * voice->mod[i].next_value;
        } else {
            mult = 1.0f + mult * voice->mod[i].next_value;
        }

        destmod->value = destmod->next_value;
        destmod->next_value = veg->d * mult;
        destmod->delta = (destmod->next_value - destmod->value) / (float)Y_CONTROL_PERIOD;

    } else if (veg->count) {

        float f, mult;
        int i;

        veg->count--;
        destmod->value = destmod->next_value;

        i = y_voice_mod_index(seg->amp_mod_src);
        mult = *(seg->amp_mod_amt);
        if (mult > 0.0f) {
            mult = 1.0f - mult + mult * voice->mod[i].next_value;
        } else {
            mult = 1.0f + mult * voice->mod[i].next_value;
        }

        f = (float)veg->count;
        destmod->next_value = (((veg->a * f + veg->b) * f + veg->c) * f + veg->d) * mult;
        destmod->delta = (destmod->next_value - destmod->value) / (float)Y_CONTROL_PERIOD;
        // YDB_MESSAGE(YDB_NOTE, " update_eg: %g -> %g by %g\n", destmod->value, destmod->next_value, destmod->delta);

    } else {

        y_voice_eg_set_next_segment(seg, voice, veg, destmod);
    }
}

/*
 * y_voice_check_for_dead
 */
static inline int
y_voice_check_for_dead(y_synth_t *synth, y_voice_t *voice)
{
    if (voice->ego.state == DSSP_EG_FINISHED) {
        /* -FIX- this could also check if eg->segment > eg->sustain_segment, level is already zero, and any subsequent segment levels are zero as well... */
        // YDB_MESSAGE(YDB_NOTE, " eps_voice_check_for_dead: killing voice %p:%d\n", voice, voice->note_id);
        y_voice_off(synth, voice);
        return 1;
    }
    return 0;
}

/*
 * y_mod_update_pressure
 */
static inline void
y_mod_update_pressure(y_synth_t *synth, y_voice_t *voice)
{
    if (fabsf(voice->mod[Y_MOD_PRESSURE].next_value - voice->mod[Y_MOD_PRESSURE].value) > 1e-10) {
        voice->mod[Y_MOD_PRESSURE].delta =
           (voice->mod[Y_MOD_PRESSURE].next_value - voice->mod[Y_MOD_PRESSURE].value) /
               (float)Y_CONTROL_PERIOD;
    }
}

/*
 * y_mod_update_modmix
 */
static inline void
y_mod_update_modmix(y_synth_t *synth, y_voice_t *voice, unsigned long sample_count)
{
    int mod;
    float n = (float)sample_count,
          f = *(synth->modmix_bias);

    mod = y_voice_mod_index(synth->modmix_mod1_src);
    f += *(synth->modmix_mod1_amt) * (voice->mod[mod].next_value + voice->mod[mod].delta * n);
    mod = y_voice_mod_index(synth->modmix_mod2_src);
    f += *(synth->modmix_mod2_amt) * (voice->mod[mod].next_value + voice->mod[mod].delta * n);

    if (f > 2.0f) f = 2.0f;
    else if (f < -2.0f) f = -2.0f;
    voice->mod[Y_MOD_MIX].next_value = f;
    voice->mod[Y_MOD_MIX].delta = (f - voice->mod[Y_MOD_MIX].value) / n;
}

/* ==== Oscillators ==== */

/* Xsynth{,-DSSI} pitch modulation was:
 *     osc2_omega *= (1.0f + eg1 * synth->eg1_amount_o) *
 *                   (1.0f + eg2 * synth->eg2_amount_o) *
 *                   (1.0f + lfo * *(synth->osc2.pitch_mod_amt));
 */

static inline void
blosc_place_step_dd(int index, float phase, float w, float *buffer_a, float scale_a,
                                                     float *buffer_b, float scale_b)
{
    float r, dd;
    int i;

    r = MINBLEP_PHASES * phase / w;
    i = lrintf(r - 0.5f);
    r -= (float)i;
    i &= MINBLEP_PHASE_MASK;  /* port changes can cause i to be out-of-range */
    /* This would be better than the above, but more expensive:
     *  while (i < 0) {
     *    i += MINBLEP_PHASES;
     *    index++;
     *  }
     */

    while (i < MINBLEP_PHASES * DD_PULSE_LENGTH) {
        dd = y_step_dd_table[i].value + r * y_step_dd_table[i].delta;
        buffer_a[index] += scale_a * dd;
        buffer_b[index] += scale_b * dd;
        i += MINBLEP_PHASES;
        index = (index + 1) & OSC_BUS_MASK;
    }
}

static inline void
blosc_place_slope_dd(int index, float phase, float w, float *buffer_a, float slope_delta_a,
                                                      float *buffer_b, float slope_delta_b)
{
    float r, dd;
    int i;

    r = MINBLEP_PHASES * phase / w;
    i = lrintf(r - 0.5f);
    r -= (float)i;
    i &= MINBLEP_PHASE_MASK;  /* port changes can cause i to be out-of-range */

    slope_delta_a *= w;
    slope_delta_b *= w;

    while (i < MINBLEP_PHASES * DD_PULSE_LENGTH) {
        dd = y_slope_dd_table[i] + r * (y_slope_dd_table[i + 1] - y_slope_dd_table[i]);
        buffer_a[index] += slope_delta_a * dd;
        buffer_b[index] += slope_delta_b * dd;
        i += MINBLEP_PHASES;
        index = (index + 1) & OSC_BUS_MASK;
    }
}

/* declare the master and slave versions of the minBLEP and wavetable
 * oscillator functions */
#define BLOSC_MASTER  1
#include "minblep_oscillator.h"
#undef BLOSC_MASTER
#define BLOSC_MASTER  0
#include "minblep_oscillator.h"
#undef BLOSC_MASTER

static int fm_mod_ratio_to_keys[17] = {
    -12, 0, 12, 19, 24, 28, 31, 34, 36, 38, 40, 42, 43, 44, 46, 47, 48
};

static void
fm_wave2sine(unsigned long sample_count, y_sosc_t *sosc, y_voice_t *voice,
             struct vosc *vosc, int index, float w0)
{
    signed short *wave0, *wave1;
    unsigned long sample;
    float cpos = (float)vosc->pos0,
          mpos = (float)vosc->pos1,
          freq_ratio,
          wavemix0, wavemix1,
          w, w_delta,
          mod, mod_delta,
          level_a, level_a_delta,
          level_b, level_b_delta;
    float f;
    int   i;

    i = lrintf(*(sosc->mparam1) * 16.0f);
    freq_ratio = (float)i;
    if (freq_ratio < 1.0f) freq_ratio = 0.5f;
    freq_ratio *= 1.0f + 0.012 * (*(sosc->mparam2) - 0.5f);

    if (vosc->mode != vosc->last_mode)
        cpos = mpos = 0.0f;
    i = voice->key + lrintf(*(sosc->pitch)) + fm_mod_ratio_to_keys[i];
    if (vosc->mode     != vosc->last_mode ||
        vosc->waveform != vosc->last_waveform ||
        i              != vosc->wave_select_key) {

        /* select wave(s) and crossfade from wavetable */
        wavetable_select(vosc, i);
        vosc->last_mode     = vosc->mode;
        vosc->last_waveform = vosc->waveform;
    }

    i = y_voice_mod_index(sosc->pitch_mod_src);
    f = *(sosc->pitch_mod_amt);
    w = 1.0f + f * voice->mod[i].value;
    w_delta = w + f * voice->mod[i].delta * (float)sample_count;
    w_delta *= w0;
    w       *= w0;
    w_delta = (w_delta - w) / (float)sample_count;
    /* -FIX- condition to [0, 0.5)? */

    i = y_voice_mod_index(sosc->mmod_src);
    f = *(sosc->mmod_amt);
    mod = f * voice->mod[i].value;
    mod_delta = volume_cv_to_amplitude(mod + f * voice->mod[i].delta * (float)sample_count);
    mod       = volume_cv_to_amplitude(mod);
    mod       *= 2.089f / 32767.0f;
    mod_delta *= 2.089f / 32767.0f;
    mod_delta = (mod_delta - mod) / (float)sample_count;

    i = y_voice_mod_index(sosc->amp_mod_src);
    f = *(sosc->amp_mod_amt);
    if (f > 0.0f)
        level_a = 1.0f - f + f * voice->mod[i].value;
    else
        level_a = 1.0f + f * voice->mod[i].value;
    level_a_delta = volume_cv_to_amplitude(level_a + f * voice->mod[i].delta * (float)sample_count);
    level_a       = volume_cv_to_amplitude(level_a);
    level_b       = level_a       * *(sosc->level_b);
    level_b_delta = level_a_delta * *(sosc->level_b);
    level_a       *= *(sosc->level_a);
    level_a_delta *= *(sosc->level_a);
    level_a_delta = (level_a_delta - level_a) / (float)sample_count;
    level_b_delta = (level_b_delta - level_b) / (float)sample_count;
    /* -FIX- condition to [0, 1]? */

    wave0 = vosc->wave0;
    wave1 = vosc->wave1;
    wavemix0 = vosc->wavemix0;
    wavemix1 = vosc->wavemix1;

    /* -FIX- optimize for the case of no crossfade */

    for (sample = 0; sample < sample_count; sample++) {

        cpos += w;

        if (cpos >= 1.0f) {
            cpos -= 1.0f;
            voice->osc_sync[sample] = cpos / w;
        } else {
            voice->osc_sync[sample] = -1.0f;
        }

        mpos += w * freq_ratio;

        while (mpos >= 1.0f) mpos -= 1.0f;

        f = mpos * (float)WAVETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;

        f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
            ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
        f *= mod;  /* f is now modulation index, in periods */

        f = (cpos + f) * (float)SINETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;
        i &= (SINETABLE_POINTS - 1);
        f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
        voice->osc_bus_a[index]   += level_a * f;
        voice->osc_bus_b[index++] += level_b * f;

        w       += w_delta;
        mod     += mod_delta;
        level_a += level_a_delta;
        level_b += level_b_delta;
    }

    vosc->pos0 = (double)cpos;
    vosc->pos1 = (double)mpos;
}

static void
fm_sine2wave(unsigned long sample_count, y_sosc_t *sosc, y_voice_t *voice,
             struct vosc *vosc, int index, float w0)
{
    signed short *wave0, *wave1;
    unsigned long sample;
    float cpos = (float)vosc->pos0,
          mpos = (float)vosc->pos1,
          freq_ratio,
          wavemix0, wavemix1,
          w, w_delta,
          mod, mod_delta,
          level_a, level_a_delta,
          level_b, level_b_delta;
    float f;
    int   i;

    if (vosc->mode != vosc->last_mode)
        cpos = mpos = 0.0f;
    i = voice->key + lrintf(*(sosc->pitch));
    if (vosc->mode     != vosc->last_mode ||
        vosc->waveform != vosc->last_waveform ||
        i              != vosc->wave_select_key) {

        /* select wave(s) and crossfade from wavetable */
        wavetable_select(vosc, i);
        vosc->last_mode     = vosc->mode;
        vosc->last_waveform = vosc->waveform;
    }

    i = y_voice_mod_index(sosc->pitch_mod_src);
    f = *(sosc->pitch_mod_amt);
    w = 1.0f + f * voice->mod[i].value;
    w_delta = w + f * voice->mod[i].delta * (float)sample_count;
    w_delta *= w0;
    w       *= w0;
    w_delta = (w_delta - w) / (float)sample_count;
    /* -FIX- condition to [0, 0.5)? */

    freq_ratio = lrintf(*(sosc->mparam1) * 16.0f);
    if (freq_ratio < 1.0f) freq_ratio = 0.5f;
    freq_ratio *= 1.0f + 0.012 * (*(sosc->mparam2) - 0.5f);

    i = y_voice_mod_index(sosc->mmod_src);
    f = *(sosc->mmod_amt);
    mod = f * voice->mod[i].value;
    mod_delta = volume_cv_to_amplitude(mod + f * voice->mod[i].delta * (float)sample_count);
    mod       = volume_cv_to_amplitude(mod);
    mod       *= 2.089f * 2.0f;
    mod_delta *= 2.089f * 2.0f;
    mod_delta = (mod_delta - mod) / (float)sample_count;

    i = y_voice_mod_index(sosc->amp_mod_src);
    f = *(sosc->amp_mod_amt);
    if (f > 0.0f)
        level_a = 1.0f - f + f * voice->mod[i].value;
    else
        level_a = 1.0f + f * voice->mod[i].value;
    level_a_delta = volume_cv_to_amplitude(level_a + f * voice->mod[i].delta * (float)sample_count);
    level_a       = volume_cv_to_amplitude(level_a);
    level_b       = level_a       * *(sosc->level_b);
    level_b_delta = level_a_delta * *(sosc->level_b);
    level_a       *= *(sosc->level_a);
    level_a_delta *= *(sosc->level_a);
    level_a_delta = (level_a_delta - level_a) / (float)sample_count;
    level_b_delta = (level_b_delta - level_b) / (float)sample_count;
    /* -FIX- condition to [0, 1]? */

    wave0 = vosc->wave0;
    wave1 = vosc->wave1;
    wavemix0 = vosc->wavemix0;
    wavemix1 = vosc->wavemix1;

    /* -FIX- optimize for the case of no crossfade */

    for (sample = 0; sample < sample_count; sample++) {

        cpos += w;

        if (cpos >= 1.0f) {
            cpos -= 1.0f;
            voice->osc_sync[sample] = cpos / w;
        } else {
            voice->osc_sync[sample] = -1.0f;
        }

        mpos += w * freq_ratio;

        while (mpos >= 1.0f) mpos -= 1.0f;

        f = mpos * (float)SINETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;

        f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
        f *= mod;  /* f is now modulation index, in periods */

        f = (cpos + f) * (float)WAVETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;
        i &= (WAVETABLE_POINTS - 1);
        f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
            ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
        f /= 65534.0f;
        voice->osc_bus_a[index]   += level_a * f;
        voice->osc_bus_b[index++] += level_b * f;

        w       += w_delta;
        mod     += mod_delta;
        level_a += level_a_delta;
        level_b += level_b_delta;
    }

    vosc->pos0 = (double)cpos;
    vosc->pos1 = (double)mpos;
}

static void
fm_wave2lf(unsigned long sample_count, y_synth_t *synth, y_sosc_t *sosc,
           y_voice_t *voice, struct vosc *vosc, int index, float w0)
{
    signed short *wave0, *wave1;
    unsigned long sample;
    float cpos = (float)vosc->pos0,
          mpos = (float)vosc->pos1,
          lfw, w, w_delta,
          wavemix0, wavemix1,
          mod, mod_delta,
          level_a, level_a_delta,
          level_b, level_b_delta;
    float f;
    int   i;

    lfw = y_pitch[lrintf(*(sosc->mparam1) * 48.0f) + 33]; /* MParam1 = carrier freq, 0.125 to 2 Hz */
    lfw *= synth->deltat;

    if (vosc->mode != vosc->last_mode)
        cpos = mpos = 0.0f;
    i = voice->key + lrintf(*(sosc->pitch));
    if (vosc->mode     != vosc->last_mode ||
        vosc->waveform != vosc->last_waveform ||
        i              != vosc->wave_select_key) {

        /* select wave(s) and crossfade from wavetable */
        wavetable_select(vosc, i);
        vosc->last_mode     = vosc->mode;
        vosc->last_waveform = vosc->waveform;
    }

    i = y_voice_mod_index(sosc->pitch_mod_src);
    f = *(sosc->pitch_mod_amt);
    w = 1.0f + f * voice->mod[i].value;
    w_delta = w + f * voice->mod[i].delta * (float)sample_count;
    w_delta *= w0;
    w       *= w0;
    w_delta = (w_delta - w) / (float)sample_count;
    /* -FIX- condition to [0, 0.5)? */

    i = y_voice_mod_index(sosc->mmod_src);
    f = *(sosc->mmod_amt);
    mod = *(sosc->mparam2) + f * voice->mod[i].value;
    mod_delta = volume_cv_to_amplitude(mod + f * voice->mod[i].delta * (float)sample_count);
    mod       = volume_cv_to_amplitude(mod);
    mod       *= 2.089f / 32767.0f;
    mod_delta *= 2.089f / 32767.0f;
    mod_delta = (mod_delta - mod) / (float)sample_count;

    i = y_voice_mod_index(sosc->amp_mod_src);
    f = *(sosc->amp_mod_amt);
    if (f > 0.0f)
        level_a = 1.0f - f + f * voice->mod[i].value;
    else
        level_a = 1.0f + f * voice->mod[i].value;
    level_a_delta = volume_cv_to_amplitude(level_a + f * voice->mod[i].delta * (float)sample_count);
    level_a       = volume_cv_to_amplitude(level_a);
    level_b       = level_a       * *(sosc->level_b);
    level_b_delta = level_a_delta * *(sosc->level_b);
    level_a       *= *(sosc->level_a);
    level_a_delta *= *(sosc->level_a);
    level_a_delta = (level_a_delta - level_a) / (float)sample_count;
    level_b_delta = (level_b_delta - level_b) / (float)sample_count;
    /* -FIX- condition to [0, 1]? */

    wave0 = vosc->wave0;
    wave1 = vosc->wave1;
    wavemix0 = vosc->wavemix0;
    wavemix1 = vosc->wavemix1;

    /* -FIX- optimize for the case of no crossfade */

    for (sample = 0; sample < sample_count; sample++) {

        cpos += lfw;
        if (cpos >= 1.0f) {
            cpos -= 1.0f;
        }

        mpos += w;
        if (mpos >= 1.0f) {
            mpos -= 1.0f;
            voice->osc_sync[sample] = mpos / w;
        } else {
            voice->osc_sync[sample] = -1.0f;
        }

        f = mpos * (float)WAVETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;
        f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
            ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
        f *= mod;  /* f is now modulation index, in periods */

        f = (cpos + f) * (float)SINETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;
        i &= (SINETABLE_POINTS - 1);
        f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
        voice->osc_bus_a[index]   += level_a * f;
        voice->osc_bus_b[index++] += level_b * f;

        w       += w_delta;
        mod     += mod_delta;
        level_a += level_a_delta;
        level_b += level_b_delta;
    }

    vosc->pos0 = (double)cpos;
    vosc->pos1 = (double)mpos;
}

static void
waveshaper(unsigned long sample_count, y_sosc_t *sosc, y_voice_t *voice,
           struct vosc *vosc, int index, float w0)
{
    signed short *wave;
    unsigned long sample;
    float pos = (float)vosc->pos0,
          w, w_delta,
          mod, mod_delta,
          bias,
          level_a, level_a_delta,
          level_b, level_b_delta;
    float f;
    int   i;

    if (vosc->mode     != vosc->last_mode ||
        vosc->waveform != vosc->last_waveform) {

        /* select wave from wavetable: so that waveshaping is predictable,
         * always use the wave for key 60 (which means that some waveforms will
         * alias badly at higher notes -- -FIX- is there a better way?
         * => probably a 'wave select bias' control would be more useful than
         * the 'phase bias' is). */
        wavetable_select(vosc, 60);
        vosc->last_mode     = vosc->mode;
        vosc->last_waveform = vosc->waveform;
        pos = 0.0f;
    }

    i = y_voice_mod_index(sosc->pitch_mod_src);
    f = *(sosc->pitch_mod_amt);
    w = 1.0f + f * voice->mod[i].value;
    w_delta = w + f * voice->mod[i].delta * (float)sample_count;
    w_delta *= w0;
    w       *= w0;
    w_delta = (w_delta - w) / (float)sample_count;
    /* -FIX- condition to [0, 0.5)? */

    i = y_voice_mod_index(sosc->mmod_src);
    f = *(sosc->mmod_amt);
    mod = *(sosc->mparam2) * 1.4f + f * voice->mod[i].value;
    mod_delta = mod + f * voice->mod[i].delta * (float)sample_count;
    /* mod_delta = volume_cv_to_amplitude(mod + f * voice->mod[i].delta * (float)sample_count);
     * mod       = volume_cv_to_amplitude(mod);
     * linearly scaled modulation seems to work better than the logarithmic above: */
    mod       *= (float)WAVETABLE_POINTS;
    mod_delta *= (float)WAVETABLE_POINTS;
    mod_delta = (mod_delta - mod) / (float)sample_count;

    bias = *(sosc->mparam1) * (float)WAVETABLE_POINTS;

    i = y_voice_mod_index(sosc->amp_mod_src);
    f = *(sosc->amp_mod_amt);
    if (f > 0.0f)
        level_a = 1.0f - f + f * voice->mod[i].value;
    else
        level_a = 1.0f + f * voice->mod[i].value;
    level_a_delta = volume_cv_to_amplitude(level_a + f * voice->mod[i].delta * (float)sample_count);
    level_a       = volume_cv_to_amplitude(level_a);
    level_b       = level_a       * *(sosc->level_b);
    level_b_delta = level_a_delta * *(sosc->level_b);
    level_a       *= *(sosc->level_a);
    level_a_delta *= *(sosc->level_a);
    level_a_delta = (level_a_delta - level_a) / (float)sample_count;
    level_b_delta = (level_b_delta - level_b) / (float)sample_count;
    /* -FIX- condition to [0, 1]? */

    wave = vosc->wave0;

    for (sample = 0; sample < sample_count; sample++) {

        pos += w;

        if (pos >= 1.0f) {
            pos -= 1.0f;
            voice->osc_sync[sample] = pos / w;
        } else {
            voice->osc_sync[sample] = -1.0f;
        }

        f = pos * SINETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;

        f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
        f *= mod;
        f += bias;
        i = lrintf(f - 0.5f);
        f -= (float)i;
        i &= (WAVETABLE_POINTS - 1);
        f = ((float)wave[i] + (float)(wave[i + 1] - wave[i]) * f) / 65534.0f;
        voice->osc_bus_a[index]   += level_a * f;
        voice->osc_bus_b[index++] += level_b * f;

        w       += w_delta;
        mod     += mod_delta;
        level_a += level_a_delta;
        level_b += level_b_delta;
    }

    vosc->pos0 = (double)pos;
}

static void
noise(unsigned long sample_count, y_sosc_t *sosc, y_voice_t *voice,
      struct vosc *vosc, int index, float w)
{
    int mod, sample;
    float f,
          level_a, level_a_delta,
          level_b, level_b_delta,
          c0, c1, c2, q;

    if (vosc->mode != vosc->last_mode) {
        vosc->f0 = 0.0f;
        vosc->f1 = 0.0f;
        vosc->f2 = 0.0f;
        vosc->last_mode = vosc->mode;
    }

    mod = y_voice_mod_index(sosc->amp_mod_src);
    f = *(sosc->amp_mod_amt);
    if (f > 0.0f)
        level_a = 1.0f - f + f * voice->mod[mod].value;
    else
        level_a = 1.0f + f * voice->mod[mod].value;
    level_a_delta = volume_cv_to_amplitude(level_a + f * voice->mod[mod].delta * (float)sample_count);
    level_a       = volume_cv_to_amplitude(level_a);
    level_b       = level_a       * *(sosc->level_b);
    level_b_delta = level_a_delta * *(sosc->level_b);
    level_a       *= *(sosc->level_a);
    level_a_delta *= *(sosc->level_a);
    level_a_delta = (level_a_delta - level_a) / (float)sample_count;
    level_b_delta = (level_b_delta - level_b) / (float)sample_count;
    /* -FIX- condition to [0, 1]? */

    switch (vosc->waveform) {
      default:
      case 0:   /* White */
        for (sample = 0; sample < sample_count; sample++) {
            f = ((float)random() / (float)RAND_MAX) - 0.5f;
            voice->osc_bus_a[index]   += level_a * f;
            voice->osc_bus_b[index++] += level_b * f;
            /* noise oscillators do not export sync */

            level_a += level_a_delta;
            level_b += level_b_delta;
        }
        break;

      case 1:  /* Paul Kellet's "economy" pink filter from Csound */
        c0 = vosc->f0;
        c1 = vosc->f1;
        c2 = vosc->f2;
        for (sample = 0; sample < sample_count; sample++) {
            f = ((float)random() / (float)RAND_MAX) - 0.5f;
            c0 = c0 * 0.99765 + f * 0.0990460;
            c1 = c1 * 0.96300 + f * 0.2965164;
            c2 = c2 * 0.57000 + f * 1.0526913;
            f = c0 + c1 + c2 + f * 0.1848;
            f *= 0.11;  /* (roughly) compensate for gain */

            voice->osc_bus_a[index]   += level_a * f;
            voice->osc_bus_b[index++] += level_b * f;
            /* noise oscillators do not export sync */

            level_a += level_a_delta;
            level_b += level_b_delta;
        }
        vosc->f0 = c0;
        vosc->f1 = c1;
        vosc->f2 = c2;
        break;
        
      case 2:   /* Low-pass */
        /* Chamberlin state-variable with cutoff limiting after Brandt; see comments in vcf_2pole */
        q = 2.0f - *(sosc->mparam2) * 1.995f;
        c2 = 0.115375f * q * q - 0.673851f * q + 1.67588f;
        f = *(sosc->mparam1);
        f = w * f * f * 128.0f;
        if (f > 0.48f) f = 0.48f;
        f = (-5.98261f * f + 7.11034f) * f; /* quick approximation of 2.0f * sinf(M_PI_F * f) */
        if (f > c2) f = c2;                 /* limit f to keep it stable */
        c0 = vosc->f0;
        c1 = vosc->f1;
        for (sample = 0; sample < sample_count; sample++) {

            c1 = c1 + f * c0;
            c2 = (((float)random() / (float)RAND_MAX) - 0.5f) - c1 - q * c0;
            c0 = f * c2 + c0;

            voice->osc_bus_a[index]   += level_a * c1;
            voice->osc_bus_b[index++] += level_b * c1;
            /* noise oscillators do not export sync */

            level_a += level_a_delta;
            level_b += level_b_delta;
        }
        vosc->f0 = c0;
        vosc->f1 = c1;
        break;

      case 3:   /* Band-pass */
        /* Chamberlin state-variable with cutoff limiting after Brandt; see comments in vcf_2pole */
        q = 2.0f - *(sosc->mparam2) * 1.995f;
        c2 = 0.115375f * q * q - 0.673851f * q + 1.67588f;
        f = *(sosc->mparam1);
        f = w * f * f * 128.0f;
        if (f > 0.48f) f = 0.48f;
        f = (-5.98261f * f + 7.11034f) * f; /* quick approximation of 2.0f * sinf(M_PI_F * f) */
        if (f > c2) f = c2;                 /* limit f to keep it stable */
        c0 = vosc->f0;
        c1 = vosc->f1;
        for (sample = 0; sample < sample_count; sample++) {

            c1 = c1 + f * c0;
            c2 = (((float)random() / (float)RAND_MAX) - 0.5f) - c1 - q * c0;
            c0 = f * c2 + c0;

            voice->osc_bus_a[index]   += level_a * c0;
            voice->osc_bus_b[index++] += level_b * c0;
            /* noise oscillators do not export sync */

            level_a += level_a_delta;
            level_b += level_b_delta;
        }
        vosc->f0 = c0;
        vosc->f1 = c1;
        break;
    }
}

static void
phase_distortion(unsigned long sample_count, y_sosc_t *sosc, y_voice_t *voice,
                 struct vosc *vosc, int index, float w0)
{
    unsigned long sample;
    float pos = (float)vosc->pos0,
          dpos, window,
          w, w_delta,
          mod, mod_delta,
          level_a, level_a_delta,
          level_b, level_b_delta;
    float f;
    int   cycle = vosc->i0,
          i;

    if (vosc->mode     != vosc->last_mode
        /* || vosc->waveform != vosc->last_waveform */) {

        vosc->last_mode     = vosc->mode;
        vosc->last_waveform = vosc->waveform;
        pos = 0.0f;
        cycle = 0;
    }

    i = y_voice_mod_index(sosc->pitch_mod_src);
    f = *(sosc->pitch_mod_amt);
    w = 1.0f + f * voice->mod[i].value;
    w_delta = w + f * voice->mod[i].delta * (float)sample_count;
    w_delta *= w0;
    w       *= w0;
    w_delta = (w_delta - w) / (float)sample_count;
    /* -FIX- condition to [0, 0.5)? */

    i = y_voice_mod_index(sosc->mmod_src);
    f = *(sosc->mmod_amt);
    mod = *(sosc->mparam2) + f * voice->mod[i].value;
    mod_delta = mod + f * voice->mod[i].delta * (float)sample_count;
    /* at this point, mod_delta is actually the target value for mod */

    i = y_voice_mod_index(sosc->amp_mod_src);
    f = *(sosc->amp_mod_amt);
    if (f > 0.0f)
        level_a = 1.0f - f + f * voice->mod[i].value;
    else
        level_a = 1.0f + f * voice->mod[i].value;
    level_a_delta = volume_cv_to_amplitude(level_a + f * voice->mod[i].delta * (float)sample_count);
    level_a       = volume_cv_to_amplitude(level_a);
    level_b       = level_a       * *(sosc->level_b);
    level_b_delta = level_a_delta * *(sosc->level_b);
    level_a       *= *(sosc->level_a);
    level_a_delta *= *(sosc->level_a);
    level_a_delta = (level_a_delta - level_a) / (float)sample_count;
    level_b_delta = (level_b_delta - level_b) / (float)sample_count;
    /* -FIX- condition to [0, 1]? */

    if (vosc->waveform < 12) {  /* single waveform */

        switch (vosc->waveform) {

          default:
          case 0:  /* cosine<->saw */
            mod = 0.5f - mod * 0.5f;
            mod_delta = 0.5f - mod_delta * 0.5f;
            if (mod < w) mod = w;
            else if (mod > 1.0f - w) mod = 1.0f - w;
            if (mod_delta < w) mod_delta = w;
            else if (mod_delta > 1.0f - w) mod_delta = 1.0f - w;
            mod_delta = (mod_delta - mod) / (float)sample_count;

            for (sample = 0; sample < sample_count; sample++) {

                pos += w;

                if (pos >= 1.0f) {
                    pos -= 1.0f;
                    voice->osc_sync[sample] = pos / w;
                } else {
                    voice->osc_sync[sample] = -1.0f;
                }

                if (pos < mod) {
                    dpos = pos * 0.5f / mod;
                } else {
                    dpos = 0.5f + (pos - mod) * 0.5f / (1.0f - mod);
                }

                f = dpos * (float)SINETABLE_POINTS;
                i = lrintf(f - 0.5f);
                f -= (float)i;
                i += SINETABLE_POINTS / 4; /* shift to get cosine from sine table */
                i &= (SINETABLE_POINTS - 1);
                f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                voice->osc_bus_a[index]   += level_a * f;
                voice->osc_bus_b[index++] += level_b * f;

                w       += w_delta;
                mod     += mod_delta;
                level_a += level_a_delta;
                level_b += level_b_delta;
            }
            break;

          case 1:  /* cosine<->square */
            /* Hmm, according to Tomás Mulcahy, http://www.madtheory.com/CZ%20article/CZ%20article.htm,
             * the CZ series doesn't output a square wave, but rather a sort of cosine with a spike in
             * it.  The scope trace he shows couldn't sound like a square wave, so I question that, but
             * hey, he's got a CZ and I don't.... */
            mod = 0.5f - mod * 0.5f;
            mod_delta = 0.5f - mod_delta * 0.5f;
            if (mod < w) mod = w;
            else if (mod > 0.5f) mod = 0.5f;
            if (mod_delta < w) mod_delta = w;
            else if (mod_delta > 0.5f) mod_delta = 0.5f;
            mod_delta = (mod_delta - mod) / (float)sample_count;

            for (sample = 0; sample < sample_count; sample++) {

                pos += w;

                if (pos >= 1.0f) {
                    pos -= 1.0f;
                    voice->osc_sync[sample] = pos / w;
                } else {
                    voice->osc_sync[sample] = -1.0f;
                }

                if (pos < mod) {
                    dpos = pos * 0.5f / mod;
                } else if (pos < 0.5f) {
                    dpos = 0.5f;
                } else if (pos < 0.5f + mod) {
                    dpos = (pos - 0.5f) * 0.5f / mod + 0.5f;
                } else {
                    dpos = 1.0f;
                }

                f = dpos * (float)SINETABLE_POINTS;
                i = lrintf(f - 0.5f);
                f -= (float)i;
                i += SINETABLE_POINTS / 4; /* shift to get cosine from sine table */
                i &= (SINETABLE_POINTS - 1);
                f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                voice->osc_bus_a[index]   += level_a * f;
                voice->osc_bus_b[index++] += level_b * f;

                w       += w_delta;
                mod     += mod_delta;
                level_a += level_a_delta;
                level_b += level_b_delta;
            }
            break;

          case 2:  /* cosine<->pulse */
            mod = 1.0f - mod;
            mod_delta = 1.0f - mod_delta;
            if (mod < 4.0f * w) mod = 4.0f * w;
            else if (mod > 1.0f - 4.0f * w) mod = 1.0f - 4.0f * w;
            if (mod_delta < 4.0f * w) mod_delta = 4.0f * w;
            else if (mod_delta > 1.0f - 4.0f * w) mod_delta = 1.0f - 4.0f * w;
            mod_delta = (mod_delta - mod) / (float)sample_count;

            for (sample = 0; sample < sample_count; sample++) {

                pos += w;

                if (pos >= 1.0f) {
                    pos -= 1.0f;
                    voice->osc_sync[sample] = pos / w;
                } else {
                    voice->osc_sync[sample] = -1.0f;
                }

                if (pos < mod) {
                    dpos = pos / mod;
                } else {
                    dpos = 1.0f;
                }

                f = dpos * (float)SINETABLE_POINTS;
                i = lrintf(f - 0.5f);
                f -= (float)i;
                i += SINETABLE_POINTS / 4; /* shift to get cosine from sine table */
                i &= (SINETABLE_POINTS - 1);
                f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                voice->osc_bus_a[index]   += level_a * f;
                voice->osc_bus_b[index++] += level_b * f;

                w       += w_delta;
                mod     += mod_delta;
                level_a += level_a_delta;
                level_b += level_b_delta;
            }
            break;

          case 5:  /* 'Resonant I' (sawtooth window) */
            mod = expf(mod * 6.0f * (float)M_LN2);
            mod_delta = expf(mod_delta * 6.0f * (float)M_LN2);
            if (mod * w > 0.5f) mod = 0.5f / w;
            if (mod_delta * w > 0.5f) mod_delta = 0.5f / w;
            mod_delta = (mod_delta - mod) / (float)sample_count;

            for (sample = 0; sample < sample_count; sample++) {

                pos += w;

                if (pos >= 1.0f) {
                    pos -= 1.0f;
                    voice->osc_sync[sample] = pos / w;
                } else {
                    voice->osc_sync[sample] = -1.0f;
                }

                dpos = pos * mod;
                window = 1.0f - pos;

                f = dpos * (float)SINETABLE_POINTS;
                i = lrintf(f - 0.5f);
                f -= (float)i;
                i += SINETABLE_POINTS / 4; /* shift to get cosine from sine table */
                i &= (SINETABLE_POINTS - 1);
                f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                f = 0.5f - f;
                f *= window;
                f = 0.5f - f;
                voice->osc_bus_a[index]   += level_a * f;
                voice->osc_bus_b[index++] += level_b * f;

                w       += w_delta;
                mod     += mod_delta;
                level_a += level_a_delta;
                level_b += level_b_delta;
            }
            break;

          case 6:  /* 'Resonant II' (triangle window) */
            mod = expf(mod * 6.0f * (float)M_LN2);
            mod_delta = expf(mod_delta * 6.0f * (float)M_LN2);
            if (mod * w > 0.5f) mod = 0.5f / w;
            if (mod_delta * w > 0.5f) mod_delta = 0.5f / w;
            mod_delta = (mod_delta - mod) / (float)sample_count;

            for (sample = 0; sample < sample_count; sample++) {

                pos += w;

                if (pos >= 1.0f) {
                    pos -= 1.0f;
                    voice->osc_sync[sample] = pos / w;
                } else {
                    voice->osc_sync[sample] = -1.0f;
                }

                dpos = pos * mod;
                if (pos < 0.5)
                    window = 2.0f * pos;
                else
                    window = 2.0f * (1.0f - pos);

                f = dpos * (float)SINETABLE_POINTS;
                i = lrintf(f - 0.5f);
                f -= (float)i;
                i += SINETABLE_POINTS / 4; /* shift to get cosine from sine table */
                i &= (SINETABLE_POINTS - 1);
                f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                f = 0.5f - f;
                f *= window;
                f = 0.5f - f;
                voice->osc_bus_a[index]   += level_a * f;
                voice->osc_bus_b[index++] += level_b * f;

                w       += w_delta;
                mod     += mod_delta;
                level_a += level_a_delta;
                level_b += level_b_delta;
            }
            break;

          case 7:  /* 'Resonant III' (trapezoidal window) */
            mod = expf(mod * 6.0f * (float)M_LN2);
            mod_delta = expf(mod_delta * 6.0f * (float)M_LN2);
            if (mod * w > 0.5f) mod = 0.5f / w;
            if (mod_delta * w > 0.5f) mod_delta = 0.5f / w;
            mod_delta = (mod_delta - mod) / (float)sample_count;

            for (sample = 0; sample < sample_count; sample++) {

                pos += w;

                if (pos >= 1.0f) {
                    pos -= 1.0f;
                    voice->osc_sync[sample] = pos / w;
                } else {
                    voice->osc_sync[sample] = -1.0f;
                }

                dpos = pos * mod;
                if (pos < 0.5)  /* -FIX- where should this breakpoint be? */
                    window = 1.0f;
                else
                    window = 2.0f * (1.0f - pos);

                f = dpos * (float)SINETABLE_POINTS;
                i = lrintf(f - 0.5f);
                f -= (float)i;
                i += SINETABLE_POINTS / 4; /* shift to get cosine from sine table */
                i &= (SINETABLE_POINTS - 1);
                f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                f = 0.5f - f;
                f *= window;
                f = 0.5f - f;
                voice->osc_bus_a[index]   += level_a * f;
                voice->osc_bus_b[index++] += level_b * f;

                w       += w_delta;
                mod     += mod_delta;
                level_a += level_a_delta;
                level_b += level_b_delta;
            }
            break;
        }

    } else {  /* alternating waveform ('octave modulation') */

        float mod0, mod0_delta,
              mod1, mod1_delta;
        int   wave0 = (vosc->waveform - 12) % 12,
              wave1 = (vosc->waveform - 12) / 12;

        if (*(sosc->mparam1) < 0.5f)
            f = 1.0f;
        else
            f = 2.0f * (1.0f - *(sosc->mparam1));
        switch (wave0) {
          default:
          case 0:  /* cosine<->saw */
            mod0 = 0.5f - f * mod * 0.5f;
            mod0_delta = 0.5f - f * mod_delta * 0.5f;
            if (mod0 < w) mod0 = w;
            else if (mod0 > 1.0f - w) mod0 = 1.0f - w;
            if (mod0_delta < w) mod0_delta = w;
            else if (mod0_delta > 1.0f - w) mod0_delta = 1.0f - w;
            break;
          case 1:  /* cosine<->square */
            mod0 = 0.5f - f * mod * 0.5f;
            mod0_delta = 0.5f - f * mod_delta * 0.5f;
            if (mod0 < w) mod0 = w;
            else if (mod0 > 0.5f) mod0 = 0.5f;
            if (mod0_delta < w) mod0_delta = w;
            else if (mod0_delta > 0.5f) mod0_delta = 0.5f;
            break;
          case 2:  /* cosine<->pulse */
            mod0 = 1.0f - f * mod;
            mod0_delta = 1.0f - f * mod_delta;
            if (mod0 < 4.0f * w) mod0 = 4.0f * w;
            else if (mod0 > 1.0f - 4.0f * w) mod0 = 1.0f - 4.0f * w;
            if (mod0_delta < 4.0f * w) mod0_delta = 4.0f * w;
            else if (mod0_delta > 1.0f - 4.0f * w) mod0_delta = 1.0f - 4.0f * w;
            break;
          case 5:  /* 'Resonant I' (sawtooth window) */
          case 6:  /* 'Resonant II' (triangle window) */
          case 7:  /* 'Resonant III' (trapezoidal window) */
            mod0 = expf(f * mod * 6.0f * (float)M_LN2);
            mod0_delta = expf(f * mod_delta * 6.0f * (float)M_LN2);
            if (mod0 * w > 0.5f) mod0 = 0.5f / w;
            if (mod0_delta * w > 0.5f) mod0_delta = 0.5f / w;
            break;
        }
        mod0_delta = (mod0_delta - mod0) / (float)sample_count;

        if (*(sosc->mparam1) < 0.5f)
            f = 2.0f * *(sosc->mparam1);
        else
            f = 1.0f;
        switch (wave1) {
          default:
          case 0:  /* cosine<->saw */
            mod1 = 0.5f - f * mod * 0.5f;
            mod1_delta = 0.5f - f * mod_delta * 0.5f;
            if (mod1 < w) mod1 = w;
            else if (mod1 > 1.0f - w) mod1 = 1.0f - w;
            if (mod1_delta < w) mod1_delta = w;
            else if (mod1_delta > 1.0f - w) mod1_delta = 1.0f - w;
            break;
          case 1:  /* cosine<->square */
            mod1 = 0.5f - f * mod * 0.5f;
            mod1_delta = 0.5f - f * mod_delta * 0.5f;
            if (mod1 < w) mod1 = w;
            else if (mod1 > 0.5f) mod1 = 0.5f;
            if (mod1_delta < w) mod1_delta = w;
            else if (mod1_delta > 0.5f) mod1_delta = 0.5f;
            break;
          case 2:  /* cosine<->pulse */
            mod1 = 1.0f - f * mod;
            mod1_delta = 1.0f - f * mod_delta;
            if (mod1 < 4.0f * w) mod1 = 4.0f * w;
            else if (mod1 > 1.0f - 4.0f * w) mod1 = 1.0f - 4.0f * w;
            if (mod1_delta < 4.0f * w) mod1_delta = 4.0f * w;
            else if (mod1_delta > 1.0f - 4.0f * w) mod1_delta = 1.0f - 4.0f * w;
            break;
          case 5:  /* 'Resonant I' (sawtooth window) */
          case 6:  /* 'Resonant II' (triangle window) */
          case 7:  /* 'Resonant III' (trapezoidal window) */
            mod1 = expf(f * mod * 6.0f * (float)M_LN2);
            mod1_delta = expf(f * mod_delta * 6.0f * (float)M_LN2);
            if (mod1 * w > 0.5f) mod1 = 0.5f / w;
            if (mod1_delta * w > 0.5f) mod1_delta = 0.5f / w;
            break;
        }
        mod1_delta = (mod1_delta - mod1) / (float)sample_count;

        sample = 0;
        while (sample < sample_count) {

            if (cycle == 0) {
                mod = mod0;
                mod_delta = mod0_delta;
                i = wave0;
            } else {
                mod = mod1;
                mod_delta = mod1_delta;
                i = wave1;
            }
            switch (i) {

              default:
              case 0:  /* cosine<->saw */
                for (; sample < sample_count; sample++) {

                    if (pos < mod) {
                        dpos = pos * 0.5f / mod;
                    } else {
                        dpos = 0.5f + (pos - mod) * 0.5f / (1.0f - mod);
                    }

                    f = dpos * (float)SINETABLE_POINTS;
                    i = lrintf(f - 0.5f);
                    f -= (float)i;
                    i += SINETABLE_POINTS / 4;  /* shift to get cosine from sine table */
                    i &= (SINETABLE_POINTS - 1);
                    f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                    voice->osc_bus_a[index]   += level_a * f;
                    voice->osc_bus_b[index++] += level_b * f;

                    pos     += w;
                    w       += w_delta;
                    mod     += mod_delta;
                    mod0    += mod0_delta;
                    mod1    += mod1_delta;
                    level_a += level_a_delta;
                    level_b += level_b_delta;

                    if (pos >= 1.0f) {
                        pos -= 1.0f;
                        voice->osc_sync[sample] = pos / w;
                        cycle ^= 1;
                        sample++;
                        break;
                    } else {
                        voice->osc_sync[sample] = -1.0f;
                    }
                }
                break;

              case 1:  /* cosine<->square */
                for (; sample < sample_count; sample++) {

                    if (pos < mod) {
                        dpos = pos * 0.5f / mod;
                    } else if (pos < 0.5f) {
                        dpos = 0.5f;
                    } else if (pos < 0.5f + mod) {
                        dpos = (pos - 0.5f) * 0.5f / mod + 0.5f;
                    } else {
                        dpos = 1.0f;
                    }

                    f = dpos * (float)SINETABLE_POINTS;
                    i = lrintf(f - 0.5f);
                    f -= (float)i;
                    i += SINETABLE_POINTS / 4;  /* shift to get cosine from sine table */
                    i &= (SINETABLE_POINTS - 1);
                    f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                    voice->osc_bus_a[index]   += level_a * f;
                    voice->osc_bus_b[index++] += level_b * f;

                    pos     += w;
                    w       += w_delta;
                    mod     += mod_delta;
                    mod0    += mod0_delta;
                    mod1    += mod1_delta;
                    level_a += level_a_delta;
                    level_b += level_b_delta;

                    if (pos >= 1.0f) {
                        pos -= 1.0f;
                        voice->osc_sync[sample] = pos / w;
                        cycle ^= 1;
                        sample++;
                        break;
                    } else {
                        voice->osc_sync[sample] = -1.0f;
                    }
                }
                break;

              case 2:  /* cosine<->pulse */
                for (; sample < sample_count; sample++) {

                    if (pos < mod) {
                        dpos = pos / mod;
                    } else {
                        dpos = 1.0f;
                    }

                    f = dpos * (float)SINETABLE_POINTS;
                    i = lrintf(f - 0.5f);
                    f -= (float)i;
                    i += SINETABLE_POINTS / 4;  /* shift to get cosine from sine table */
                    i &= (SINETABLE_POINTS - 1);
                    f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                    voice->osc_bus_a[index]   += level_a * f;
                    voice->osc_bus_b[index++] += level_b * f;

                    pos     += w;
                    w       += w_delta;
                    mod     += mod_delta;
                    mod0    += mod0_delta;
                    mod1    += mod1_delta;
                    level_a += level_a_delta;
                    level_b += level_b_delta;

                    if (pos >= 1.0f) {
                        pos -= 1.0f;
                        voice->osc_sync[sample] = pos / w;
                        cycle ^= 1;
                        sample++;
                        break;
                    } else {
                        voice->osc_sync[sample] = -1.0f;
                    }
                }
                break;

              case 5:  /* 'Resonant I' (sawtooth window) */
                for (; sample < sample_count; sample++) {

                    dpos = pos * mod;
                    window = 1.0f - pos;

                    f = dpos * (float)SINETABLE_POINTS;
                    i = lrintf(f - 0.5f);
                    f -= (float)i;
                    i += SINETABLE_POINTS / 4;  /* shift to get cosine from sine table */
                    i &= (SINETABLE_POINTS - 1);
                    f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                    f = 0.5f - f;
                    f *= window;
                    f = 0.5f - f;
                    voice->osc_bus_a[index]   += level_a * f;
                    voice->osc_bus_b[index++] += level_b * f;

                    pos     += w;
                    w       += w_delta;
                    mod     += mod_delta;
                    mod0    += mod0_delta;
                    mod1    += mod1_delta;
                    level_a += level_a_delta;
                    level_b += level_b_delta;

                    if (pos >= 1.0f) {
                        pos -= 1.0f;
                        voice->osc_sync[sample] = pos / w;
                        cycle ^= 1;
                        sample++;
                        break;
                    } else {
                        voice->osc_sync[sample] = -1.0f;
                    }
                }
                break;

              case 6:  /* 'Resonant II' (triangle window) */
                for (; sample < sample_count; sample++) {

                    dpos = pos * mod;
                    if (pos < 0.5)
                        window = 2.0f * pos;
                    else
                        window = 2.0f * (1.0f - pos);

                    f = dpos * (float)SINETABLE_POINTS;
                    i = lrintf(f - 0.5f);
                    f -= (float)i;
                    i += SINETABLE_POINTS / 4;  /* shift to get cosine from sine table */
                    i &= (SINETABLE_POINTS - 1);
                    f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                    f = 0.5f - f;
                    f *= window;
                    f = 0.5f - f;
                    voice->osc_bus_a[index]   += level_a * f;
                    voice->osc_bus_b[index++] += level_b * f;

                    pos     += w;
                    w       += w_delta;
                    mod     += mod_delta;
                    mod0    += mod0_delta;
                    mod1    += mod1_delta;
                    level_a += level_a_delta;
                    level_b += level_b_delta;

                    if (pos >= 1.0f) {
                        pos -= 1.0f;
                        voice->osc_sync[sample] = pos / w;
                        cycle ^= 1;
                        sample++;
                        break;
                    } else {
                        voice->osc_sync[sample] = -1.0f;
                    }
                }
                break;

              case 7:  /* 'Resonant III' (trapezoidal window) */
                for (; sample < sample_count; sample++) {

                    dpos = pos * mod;
                    if (pos < 0.5)  /* -FIX- where should this breakpoint be? */
                        window = 1.0f;
                    else
                        window = 2.0f * (1.0f - pos);

                    f = dpos * (float)SINETABLE_POINTS;
                    i = lrintf(f - 0.5f);
                    f -= (float)i;
                    i += SINETABLE_POINTS / 4;  /* shift to get cosine from sine table */
                    i &= (SINETABLE_POINTS - 1);
                    f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                    f = 0.5f - f;
                    f *= window;
                    f = 0.5f - f;
                    voice->osc_bus_a[index]   += level_a * f;
                    voice->osc_bus_b[index++] += level_b * f;

                    pos     += w;
                    w       += w_delta;
                    mod     += mod_delta;
                    mod0    += mod0_delta;
                    mod1    += mod1_delta;
                    level_a += level_a_delta;
                    level_b += level_b_delta;

                    if (pos >= 1.0f) {
                        pos -= 1.0f;
                        voice->osc_sync[sample] = pos / w;
                        cycle ^= 1;
                        sample++;
                        break;
                    } else {
                        voice->osc_sync[sample] = -1.0f;
                    }
                }
                break;
            }
        }
    }

    vosc->pos0 = (double)pos;
    vosc->i0 = cycle;
}

static void
wt_chorus(unsigned long sample_count, y_synth_t *synth, y_sosc_t *sosc,
          y_voice_t *voice, struct vosc *vosc, int index, float w0)
{
    signed short *wave0, *wave1;
    unsigned long sample;
    float pos0 = (float)vosc->pos0,
          pos1 = (float)vosc->pos1,
          pos2 = vosc->f0,
          pos3 = vosc->f1,
          pos4 = vosc->f2,
          wavemix0, wavemix1,
          w, w_delta, wm0, wm1, wm3, wm4,
          am04, am13,
          level_a, level_a_delta,
          level_b, level_b_delta;
    float f, a;
    int   i;

    wm1 = 0.999f - *(sosc->mparam1) * 0.008f;  /* MParam1 = tuning spread */
    wm3 = 1.0f / wm1;
    wm0 = 0.998f - *(sosc->mparam1) * 0.016f;
    wm4 = 1.0f / wm0;

    f = 1.0f - *(sosc->mmod_amt);              /* MMod AMt = mix profile */
    f *= f;
    am13 = expf(-f * 2.4f);
    am04 = am13 * am13;

    if (vosc->mode != vosc->last_mode) {
        vosc->last_mode = vosc->mode;
        vosc->last_waveform = -1;
        pos2 = random_float(0.0f, 1.0f);
        pos0 = pos2 + 0.8f; if (pos0 >= 1.0f) pos0 -= 1.0f;
        pos1 = pos2 + 0.1f; if (pos1 >= 1.0f) pos1 -= 1.0f;
        pos3 = pos2 + 0.5f; if (pos3 >= 1.0f) pos3 -= 1.0f;
        pos4 = pos2 + 0.2f; if (pos4 >= 1.0f) pos4 -= 1.0f;
    }
    i = voice->key + lrintf(*(sosc->pitch) + *(sosc->mparam2) * WAVETABLE_SELECT_BIAS_RANGE);
    if (vosc->waveform != vosc->last_waveform ||
        i              != vosc->wave_select_key) {

        /* select wave(s) and crossfade from wavetable */
        wavetable_select(vosc, i);
        vosc->last_waveform = vosc->waveform;
    }

    i = y_voice_mod_index(sosc->pitch_mod_src);
    f = *(sosc->pitch_mod_amt);
    w = 1.0f + f * voice->mod[i].value;
    w_delta = w + f * voice->mod[i].delta * (float)sample_count;
    w_delta *= w0;
    w       *= w0;
    w_delta = (w_delta - w) / (float)sample_count;
    /* -FIX- condition to [0, 0.5)? */

    i = y_voice_mod_index(sosc->amp_mod_src);
    f = *(sosc->amp_mod_amt);
    if (f > 0.0f)
        level_a = 1.0f - f + f * voice->mod[i].value;
    else
        level_a = 1.0f + f * voice->mod[i].value;
    level_a_delta = volume_cv_to_amplitude(level_a + f * voice->mod[i].delta * (float)sample_count);
    level_a       = volume_cv_to_amplitude(level_a);
    level_a       /= (65534.0f * 2.0f);
    level_a_delta /= (65534.0f * 2.0f);
    level_b       = level_a       * *(sosc->level_b);
    level_b_delta = level_a_delta * *(sosc->level_b);
    level_a       *= *(sosc->level_a);
    level_a_delta *= *(sosc->level_a);
    level_a_delta = (level_a_delta - level_a) / (float)sample_count;
    level_b_delta = (level_b_delta - level_b) / (float)sample_count;
    /* -FIX- condition to [0, 1]? */

    wave0 = vosc->wave0;
    wave1 = vosc->wave1;
    wavemix0 = vosc->wavemix0;
    wavemix1 = vosc->wavemix1;

    /* -FIX- optimize for the case of no crossfade */

    for (sample = 0; sample < sample_count; sample++) {

        pos0 += w * wm0;
        pos1 += w * wm1;
        pos2 += w;
        pos3 += w * wm3;
        pos4 += w * wm4;
        if (pos0 >= 1.0f) pos0 -= 1.0f;
        if (pos1 >= 1.0f) pos1 -= 1.0f;
        if (pos2 >= 1.0f) {
            pos2 -= 1.0f;
            voice->osc_sync[sample] = pos2 / w;
        } else {
            voice->osc_sync[sample] = -1.0f;
        }
        if (pos3 >= 1.0f) pos3 -= 1.0f;
        if (pos4 >= 1.0f) pos4 -= 1.0f;

        f = pos0 * (float)WAVETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;
        f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
            ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
        a = f * am04;

        f = pos1 * (float)WAVETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;
        f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
            ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
        a += f * am13;

        f = pos2 * (float)WAVETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;
        f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
            ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
        a += f;

        f = pos3 * (float)WAVETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;
        f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
            ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
        a += f * am13;

        f = pos4 * (float)WAVETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;
        f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
            ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
        a += f * am04;

        voice->osc_bus_a[index]   += level_a * a;
        voice->osc_bus_b[index++] += level_b * a;

        w       += w_delta;
        level_a += level_a_delta;
        level_b += level_b_delta;
    }

    vosc->pos0 = (double)pos0;
    vosc->pos1 = (double)pos1;
    vosc->f0   = pos2;
    vosc->f1   = pos3;
    vosc->f2   = pos4;
}

static void
oscillator(unsigned long sample_count, y_synth_t *synth, y_sosc_t *sosc,
           y_voice_t *voice, struct vosc *vosc, int index, float w)

{
    switch (vosc->mode) {
      default:
      case 0: /* disabled */
        break;

      case 1: /* minBLEP */
        if (*(sosc->mparam1) > 0.9f) /* sync on */
           blosc_slave(sample_count, sosc, voice, vosc, index, w);
        else
           blosc_master(sample_count, sosc, voice, vosc, index, w);
        break;

      case 2: /* wavetable */
        if (*(sosc->mparam1) > 0.9f) /* sync on */
            wt_osc_slave(sample_count, sosc, voice, vosc, index, w);
        else
            wt_osc_master(sample_count, sosc, voice, vosc, index, w);
        break;

      case 3: /* async granular */
        agran_oscillator(sample_count, synth, sosc, voice, vosc, index, w);
        break;

      case 4: /* FM Wave->Sine: sine phase modulated by wave */
        fm_wave2sine(sample_count, sosc, voice, vosc, index, w);
        break;

      case 5: /* FM Sine->Wave: wave phase modulated by sine */
        fm_sine2wave(sample_count, sosc, voice, vosc, index, w);
        break;

      case 6: /* waveshaper */
        waveshaper(sample_count, sosc, voice, vosc, index, w);
        break;

      case 7: /* noise */
        noise(sample_count, sosc, voice, vosc, index, w);
        break;

      case Y_OSCILLATOR_MODE_PADSYNTH: /* PADsynth */
        padsynth_oscillator(sample_count, sosc, voice, vosc, index, w);
        break;

      case Y_OSCILLATOR_MODE_PD: /* phase distortion */
        phase_distortion(sample_count, sosc, voice, vosc, index, w);
        break;

      case 10: /* FM Wave->LF Sine: low-frequency sine modulated by wave */
        fm_wave2lf(sample_count, synth, sosc, voice, vosc, index, w);
        break;

      case 11: /* wavetable chorus */
        wt_chorus(sample_count, synth, sosc, voice, vosc, index, w);
        break;
    }
}

/* ==== Filters ==== */

/*
 * parameter 'freq' is normalized, i.e. a fraction of the sample rate, with 0.5 being Nyquist.
 *
 * Xsynth{,-DSSI} used a cutoff frequency calculation of:
 *    freq = M_PI_F * deltat * voice->current_pitch * synth->mod_wheel;  /-* now (0 to 1) * pi *-/
 *    freqkey_b = freq * *(synth->vcf2.frequency);
 *    freqeg1 = freq * *(synth->eg1_amount_f);
 *    freqeg2 = freq * *(synth->eg2_amount_f);
 *    cutoff[sample] = (freqkey_b + freqeg1 * eg1 + freqeg2 * eg2) *
 *                         (1.0f + lfo * *(synth->vcf2.freq_mod_amt));
 *    freqcut = cutoff[sample] * 2.0f ...
 */

/* vcf_off
 */
static inline void
vcf_off(unsigned long sample_count, struct vvcf *vvcf, float *out)
{
    if (vvcf->mode != vvcf->last_mode) {
        vvcf->last_mode = vvcf->mode;
        /* silence the filter output buffer */
        memset(out, 0, Y_CONTROL_PERIOD * sizeof(float));
    }
}

static inline float
stabilize(float freqcut, float freq, float qres)
{
    /* SVF stabilization based on Eli Brandt's work
     * Eli's original 'f3' function for limiting cutoff frequency based on Q:
     *     freqmax = (-qres + sqrt(4.f + qres * qres));
     * My slightly more stable version:
     *     freqmax = (-qres + sqrt(4.f * sqrt(2.f) + qres * qres)) / sqrt(2.f);
     * A quick approximation thereof: */
    float freqmax = (0.115375f * qres - 0.673851f) * qres + 1.67588f;

    freqcut *= freq;

    if (freqcut > 0.48f) freqcut = 0.48f;
    else if (freqcut < 1e-5f) freqcut = 1e-5f;

    freqcut = (-5.98261f * freqcut + 7.11034f) * freqcut; /* quick approximation of 2.0f * sinf(M_PI_F * freqcut) */
    if (freqcut > freqmax) freqcut = freqmax;

    return freqcut;
}

enum _filter_type_t {
    FT_LOWPASS_2POLE,
    FT_LOWPASS_4POLE,
    FT_LOWPASS_4POLE_CLIP,
    FT_HIGHPASS_2POLE,
    FT_HIGHPASS_4POLE,
    FT_BANDPASS,
    FT_BANDREJECT
};

typedef enum _filter_type_t filter_type_t;

// We define all the inner-loops for the Chamberlin filters using #defines
// so that we can keep most of the conditional statements out of the loops.
#define FILTER_LOOP_BANDPASS       FILTER_LOOP_PRELUDE { BANDPASS          UPDATE_FREQCUT }
#define FILTER_LOOP_BANDREJECT     FILTER_LOOP_PRELUDE { BANDREJECT        UPDATE_FREQCUT }
#define FILTER_LOOP_2POLE_HP       FILTER_LOOP_PRELUDE { TWO_POLE_HP       UPDATE_FREQCUT }
#define FILTER_LOOP_4POLE_HP       FILTER_LOOP_PRELUDE { FOUR_POLE_HP      UPDATE_FREQCUT }
#define FILTER_LOOP_2POLE_LP       FILTER_LOOP_PRELUDE { TWO_POLE_LP       UPDATE_FREQCUT }
#define FILTER_LOOP_4POLE_LP       FILTER_LOOP_PRELUDE { FOUR_POLE_LP      UPDATE_FREQCUT }
#define FILTER_LOOP_4POLE_LP_CLIP  FILTER_LOOP_PRELUDE { FOUR_POLE_LP_CLIP UPDATE_FREQCUT }

#define FILTER_LOOP_PRELUDE  for (sample = 0; sample < sample_count; sample++) 
#define UPDATE_FREQCUT       freqcut += freqcut_delta;


#define TWO_POLE_LP                   \
    input = in[sample];               \
    FIRST_FILTER_STAGE                \
    out[sample] = delay2;             \
   
#define FOUR_POLE_LP                  \
    input = in[sample];               \
    FIRST_FILTER_STAGE                \
                                      \
    stage2_input = delay2;            \
    SECOND_FILTER_STAGE               \
                                      \
    out[sample] = delay4;             \

#define BANDPASS                      \
    input = in[sample];               \
    FIRST_FILTER_STAGE                \
                                      \
    stage2_input = delay1;            \
    SECOND_FILTER_STAGE               \
                                      \
    out[sample] = delay3;             \

#define BANDREJECT                    \
    input = in[sample];               \
    FIRST_FILTER_STAGE                \
                                      \
    stage2_input = highpass + delay2; \
    SECOND_FILTER_STAGE               \
                                      \
    out[sample] = highpass + delay4;  \

#define TWO_POLE_HP                   \
    input = in[sample];               \
    FIRST_FILTER_STAGE                \
                                      \
    out[sample] = highpass;           \

#define FOUR_POLE_HP                  \
    input = in[sample];               \
    FIRST_FILTER_STAGE                \
                                      \
    stage2_input = highpass;          \
    SECOND_FILTER_STAGE               \
                                      \
    out[sample] = highpass;           \

#define FOUR_POLE_LP_CLIP             \
    input = in[sample] * gain;        \
    if (input > 0.7f)                 \
        input = 0.7f;                 \
    else if (input < -0.7f)           \
        input = -0.7f;                \
                                      \
    FIRST_FILTER_STAGE                \
                                      \
    stage2_input = delay2 * gain;     \
    if (stage2_input > 0.7f)          \
        stage2_input = 0.7f;          \
    else if (stage2_input < -0.7f)    \
        stage2_input = -0.7f;         \
                                      \
    SECOND_FILTER_STAGE               \
                                      \
    out[sample] = delay4;             \


#define FIRST_FILTER_STAGE                                                        \
    delay2 = delay2 + freqcut * delay1;         /* delay2/4 = lowpass output */   \
    highpass = input - delay2 - qres * delay1;                                    \
    delay1 = freqcut * highpass + delay1;       /* delay1/3 = bandpass output */  \

#define SECOND_FILTER_STAGE                                                       \
    delay4 = delay4 + freqcut * delay3;                                           \
    highpass = stage2_input - delay4 - qres * delay3;                             \
    delay3 = freqcut * highpass + delay3;                                         \


/* vcf_2_4pole
 *
 * 2/4-pole Chamberlin state-variable low-pass filter
 */
static void
vcf_2_4pole(unsigned long sample_count, y_svcf_t *svcf, y_voice_t *voice,
          struct vvcf *vvcf, float freq, filter_type_t type, float *in, float *out)
{
    unsigned long sample;
    int mod;
    float freqcut, freqtmp, freqcut_delta,
          qres, highpass, gain,
          delay1, delay2, delay3, delay4,
          input, stage2_input;

    if (vvcf->last_mode != vvcf->mode) {
        vvcf->delay1 = 0.0f;
        vvcf->delay2 = 0.0f;
        vvcf->delay3 = 0.0f;
        vvcf->delay4 = 0.0f;
        vvcf->last_mode = vvcf->mode;
    }

    if (type == FT_LOWPASS_2POLE)
        qres = 2.0f - *(svcf->qres) * 1.995f;
    else
        qres = 2.0f - *(svcf->qres) * 1.96f;

    mod = y_voice_mod_index(svcf->freq_mod_src);

    freqcut = (*(svcf->frequency) +
                *(svcf->freq_mod_amt) * 50.0f * voice->mod[mod].value);
    freqtmp = freqcut +
                *(svcf->freq_mod_amt) * 50.0f * (float)sample_count * voice->mod[mod].delta;

    freqcut = stabilize(freqcut, freq, qres);
    freqtmp = stabilize(freqtmp, freq, qres);

    freqcut_delta = (freqtmp - freqcut) / (float)sample_count;

    /* gain range: -24dB to +24dB */
    gain = volume_cv_to_amplitude(0.36f + *(svcf->mparam) * 0.64f) * 16.0f;

    delay1 = vvcf->delay1;
    delay2 = vvcf->delay2;
    delay3 = vvcf->delay3;
    delay4 = vvcf->delay4;

    switch (type)
    {
        case FT_BANDPASS:
            FILTER_LOOP_BANDPASS
            break;

        case FT_BANDREJECT:
            FILTER_LOOP_BANDREJECT
            break;

        case FT_HIGHPASS_2POLE:
            FILTER_LOOP_2POLE_HP
            break;

        case FT_HIGHPASS_4POLE:
            FILTER_LOOP_4POLE_HP
            break;

        case FT_LOWPASS_2POLE:
            FILTER_LOOP_2POLE_LP
            break;

        case FT_LOWPASS_4POLE:
            FILTER_LOOP_4POLE_LP
            break;

        case FT_LOWPASS_4POLE_CLIP:
            FILTER_LOOP_4POLE_LP_CLIP
            break;
    }

    vvcf->delay1 = delay1;
    vvcf->delay2 = delay2;
    vvcf->delay3 = delay3;
    vvcf->delay4 = delay4;
}

static inline void
vcf_2pole(unsigned long sample_count, y_svcf_t *svcf, y_voice_t *voice,
          struct vvcf *vvcf, float freq, float *in, float *out)
{
    vcf_2_4pole(sample_count, svcf, voice, vvcf, freq, FT_LOWPASS_2POLE, in, out);
}

static inline void
vcf_4pole(unsigned long sample_count, y_svcf_t *svcf, y_voice_t *voice,
          struct vvcf *vvcf, float freq, float *in, float *out)
{
    vcf_2_4pole(sample_count, svcf, voice, vvcf, freq, FT_LOWPASS_4POLE, in, out);
}

static inline void
vcf_clip4pole(unsigned long sample_count, y_svcf_t *svcf, y_voice_t *voice,
             struct vvcf *vvcf, float freq, float *in, float *out)
{
    vcf_2_4pole(sample_count, svcf, voice, vvcf, freq, FT_LOWPASS_4POLE_CLIP, in, out);
}

static inline void
vcf_bandpass(unsigned long sample_count, y_svcf_t *svcf, y_voice_t *voice,
             struct vvcf *vvcf, float freq, float *in, float *out)
{
    vcf_2_4pole(sample_count, svcf, voice, vvcf, freq, FT_BANDPASS, in, out);
}

static inline void
vcf_bandreject(unsigned long sample_count, y_svcf_t *svcf, y_voice_t *voice,
             struct vvcf *vvcf, float freq, float *in, float *out)
{
    vcf_2_4pole(sample_count, svcf, voice, vvcf, freq, FT_BANDREJECT, in, out);
}

static inline void
vcf_highpass_2pole(unsigned long sample_count, y_svcf_t *svcf, y_voice_t *voice,
             struct vvcf *vvcf, float freq, float *in, float *out)
{
    vcf_2_4pole(sample_count, svcf, voice, vvcf, freq, FT_HIGHPASS_2POLE, in, out);
}

static inline void
vcf_highpass_4pole(unsigned long sample_count, y_svcf_t *svcf, y_voice_t *voice,
             struct vvcf *vvcf, float freq, float *in, float *out)
{
    vcf_2_4pole(sample_count, svcf, voice, vvcf, freq, FT_HIGHPASS_4POLE, in, out);
}

/* vcf_mvclpf
 *
 * Fons Adriaensen's MVCLPF-3
 */
void
vcf_mvclpf(unsigned long sample_count, y_svcf_t *svcf, y_voice_t *voice,
           struct vvcf *vvcf, float freq, float *in, float *out)
{
    unsigned long s;
    int mod;
    float w0, w0d, g0, g1, res, w, x, d,
          delay1, delay2, delay3, delay4, delay5;

    if (vvcf->last_mode != vvcf->mode) {
        vvcf->delay1 = 0.0f;
        vvcf->delay2 = 0.0f;
        vvcf->delay3 = 0.0f;
        vvcf->delay4 = 0.0f;
        vvcf->delay5 = 0.0f;
        vvcf->last_mode = vvcf->mode;
    }

    mod = y_voice_mod_index(svcf->freq_mod_src);
    w0 = (*(svcf->frequency) +
             *(svcf->freq_mod_amt) * 50.0f * voice->mod[mod].value);
    w0d = w0 + *(svcf->freq_mod_amt) * 50.0f *
                   (float)sample_count * voice->mod[mod].delta;
    w0  *= M_PI_F * freq;
    w0d *= M_PI_F * freq;
    if (w0 < 0.0f)
        w0 = 0.0f;
    if (w0d < 0.0f)
        w0d = 0.0f;
    w0d = (w0d - w0) / (float)sample_count;

    /* g0 = dB_to_amplitude(input_gain_in_db) / 2
     * Fons used a port range of -60dB to 10 dB; here we use -18dB to +18dB
     * 0.0 port value -> -18dB / 2 -> g0 = 0.0625
     * 0.5 port value ->   0dB / 2 -> g0 = 0.5
     * 1.0 port value -> +18dB / 2 -> g0 = 4.0 */
    g0 = volume_cv_to_amplitude(0.52f + *(svcf->mparam) * 0.48f) * (8.0f / 2.0f);
    /* g1 = dB_to_amplitude(output_gain_in_db) * 2
     * Fons used a port range of -15dB to 15dB; here it is the inverse of the input gain */
    g1 = 1.0f / g0;

    /* res should be 0 to 1 already */
    res = *(svcf->qres);

    delay1 = vvcf->delay1;
    delay2 = vvcf->delay2;
    delay3 = vvcf->delay3;
    delay4 = vvcf->delay4;
    delay5 = vvcf->delay5;

    for (s = 0; s < sample_count; s++) {

        w = w0;
        w0 += w0d;

        if (w < 0.75f) w *= 1.005f - w * (0.624f - w * (0.65f - w * 0.54f));
        else
	{
	    w *= 0.6748f;
            if (w > 0.82f) w = 0.82f;
	}

        x = in[s] * g0 - (4.3f - 0.2f * w) * res * delay5 + 1e-10f;
        x /= sqrtf(1.0f + x * x);  /* x = tanh(x) */
        d = w * (x  - delay1) / (1.0f + delay1 * delay1);            
        x = delay1 + 0.77f * d;
        delay1 = x + 0.23f * d;        
        d = w * (x  - delay2) / (1.0f + delay2 * delay2);            
        x = delay2 + 0.77f * d;
        delay2 = x + 0.23f * d;        
        d = w * (x  - delay3) / (1.0f + delay3 * delay3);            
        x = delay3 + 0.77f * d;
        delay3 = x + 0.23f * d;        
        d = w * (x  - delay4);
        x = delay4 + 0.77f * d;
        delay4 = x + 0.23f * d;        
        delay5 += 0.85f * (delay4 - delay5);

        x = in[s] * g0 - (4.3f - 0.2f * w) * res * delay5;
        x /= sqrtf(1.0f + x * x);  /* x = tanh(x) */
        d = w * (x  - delay1) / (1.0f + delay1 * delay1);            
        x = delay1 + 0.77f * d;
        delay1 = x + 0.23f * d;        
        d = w * (x  - delay2) / (1.0f + delay2 * delay2);            
        x = delay2 + 0.77f * d;
        delay2 = x + 0.23f * d;        
        d = w * (x  - delay3) / (1.0f + delay3 * delay3);            
        x = delay3 + 0.77f * d;
        delay3 = x + 0.23f * d;        
        d = w * (x  - delay4);
        x = delay4 + 0.77f * d;
        delay4 = x + 0.23f * d;        
        delay5 += 0.85f * (delay4 - delay5);

        out[s] = g1 * delay4;
    }

    vvcf->delay1 = delay1;
    vvcf->delay2 = delay2;
    vvcf->delay3 = delay3;
    vvcf->delay4 = delay4;
    vvcf->delay5 = delay5;
}

/* vcf_amsynth
 *
 * Nick Dowell's amSynth 24 dB/octave resonant low-pass filter.
 */
static void
vcf_amsynth(unsigned long sample_count, y_svcf_t *svcf, y_voice_t *voice,
            struct vvcf *vvcf, float freq, float *in, float *out)
{
    unsigned long sample;
    int mod;
    float freqtmp;

    float r, k, k_delta, k2, bh;  /* These were all doubles in the original */
    float a0, a1, b1, b2;         /* amSynth filter, but I don't hear a     */
    float x, y;                   /* noticeable difference with floats....  */
    float d1, d2, d3, d4;

    if (vvcf->last_mode != vvcf->mode) {
        vvcf->last_mode = vvcf->mode;
        d1 = d2 = d3 = d4 = 0.0f;
    } else {
        d1 = vvcf->delay1;
        d2 = vvcf->delay2;
        d3 = vvcf->delay3;
        d4 = vvcf->delay4;
    }

    /* find coeff values for start and end of this buffer */
    mod = y_voice_mod_index(svcf->freq_mod_src);
    freqtmp = (*(svcf->frequency) +
                  *(svcf->freq_mod_amt) * 50.0f * voice->mod[mod].value);
    freqtmp *= freq;
    if (freqtmp > 0.495f) freqtmp = 0.495f;  /* filter is unstable _AT_ PI */
    else if (freqtmp < 1e-4f) freqtmp = 1e-4f;
    k = tanf(freqtmp * M_PI_F);  /* -FIX- optimizable? */
    freqtmp += freq * *(svcf->freq_mod_amt) * 50.0f *
                   (float)sample_count * voice->mod[mod].delta;
    if (freqtmp > 0.495f) freqtmp = 0.495f;  /* filter is unstable _AT_ PI */
    else if (freqtmp < 1e-4f) freqtmp = 1e-4f;
    k_delta = tanf(freqtmp * M_PI_F);
    k_delta = (k_delta - k) / (float)sample_count;

    r = 2.0f * (1.0f - *(svcf->qres) * 0.97f);
    if (r == 0.0f) r = 0.001f;

    for (sample = 0; sample < sample_count; sample++) {

        k2 = k * k;
        bh = 1.0f + (r * k) + k2;
        a0 = k2 / bh;
        /* a2 = a0; */
        a1 = a0 * 2.0f;
        b1 = 2.0f * (k2 - 1.0f) / -bh;
        b2 = (1.0f - (r * k) + k2) / -bh;

        /* filter (2 cascaded second order filters) */
        x = in[sample];

        /* first 2nd-order unit */
        y = ( a0*x ) + d1;
        d1 = d2 + ( (a1)*x ) + ( (b1)*y );
        d2 = ( /* a2 */ a0*x ) + ( (b2)*y );
        x = y;

        /* and the second */
        y = ( a0*x ) + d3;
        d3 = d4 + ( a1*x ) + ( b1*y );
        d4 = ( /* a2 */ a0*x ) + ( b2*y );
        
        out[sample] = y;

        k += k_delta;
    }

    vvcf->delay1 = d1;
    vvcf->delay2 = d2;
    vvcf->delay3 = d3;
    vvcf->delay4 = d4;
}

/* vcf_resonz
 *
 * resonz from Csound ugsc.c
 *
 * An implementation of the 2-pole, 2-zero reson filter
 * described by Julius O. Smith and James B. Angell in
 * "A Constant Gain Digital Resonator Tuned by a Single
 * Coefficient," Computer Music Journal, Vol. 6, No. 4,
 * Winter 1982, p.36-39. resonz implements the version
 * where the zeros are located at z = 1 and z = -1.
 *
 * -FIX- 'resonance' knob should be labeled 'bandwidth' for this and resonz....
 */
static void
vcf_resonz(unsigned long sample_count, y_svcf_t *svcf, y_voice_t *voice,
           struct vvcf *vvcf, float freq, float *in, float *out)
{
    unsigned long sample;
    int mod;
    float freqtmp, kbw;

    float r, scale; /* radius & scaling factor */
    float c1, c2;   /* filter coefficients */
    float xn, yn, xnm1, xnm2, ynm1, ynm2;

    if (vvcf->last_mode != vvcf->mode) {
        vvcf->delay1 = 0.0f;
        vvcf->delay2 = 0.0f;
        vvcf->delay3 = 0.0f;
        vvcf->delay4 = 0.0f;
        vvcf->last_mode = vvcf->mode;
    }

    mod = y_voice_mod_index(svcf->freq_mod_src);
    freqtmp = (*(svcf->frequency) +
                  *(svcf->freq_mod_amt) * 50.0f * voice->mod[mod].value);
    freq *= freqtmp;
    if (freq > 0.48f) freq = 0.48f;
    else if (freq < 2e-4f) freq = 2e-4f;

    kbw = 1.0f - *(svcf->qres);
    kbw = 0.5f * kbw * kbw * kbw * kbw;
    if (kbw < 6.25e-5f) kbw = 6.25e-5f;

    r = expf(-M_PI_F * kbw);
    c1 = 2.0f * r * cosf(M_2PI_F * freq);
    c2 = r * r;

    /* Normalizing factors derived from equations in Ken Steiglitz,
     * "A Note on Constant-Gain Digital Resonators," Computer
     * Music Journal, vol. 18, no. 4, pp. 8-10, Winter 1982.
     */
    /* -FIX- */
    // if (p->scaletype == 1)
    //   scale = (1.0f - c2) * 0.5f;
    // else if (p->scaletype == 2)
      scale = sqrtf((1.0f - c2) * 0.5f);
    // else scale = 1.0f;

    xnm1 = vvcf->delay1;
    xnm2 = vvcf->delay2;
    ynm1 = vvcf->delay3;
    ynm2 = vvcf->delay4;

    for (sample = 0; sample < sample_count; sample++) {

        xn = in[sample];
        out[sample] = yn = scale * (xn - xnm2) + c1 * ynm1 - c2 * ynm2;
        xnm2 = xnm1;
        xnm1 = xn;
        ynm2 = ynm1;
        ynm1 = yn;
    }

    vvcf->delay1 = xnm1;
    vvcf->delay2 = xnm2;
    vvcf->delay3 = ynm1;
    vvcf->delay4 = ynm2;
}

/*
 * y_voice_render
 *
 * generate the actual sound data for this voice
 */
void
y_voice_render(y_synth_t *synth, y_voice_t *voice,
                    LADSPA_Data *out_left, LADSPA_Data *out_right,
                    unsigned long sample_count, int do_control_update)
{
    unsigned long sample;
    float         deltat = synth->deltat;
    int           osc_index = voice->osc_index;
    float         osc1_omega, osc2_omega, osc3_omega, osc4_omega,
                 *vcf_source;

    /* calculate fundamental pitch of voice */
    voice->current_pitch = *(synth->glide_time) * voice->target_pitch +
                            (1.0f - *(synth->glide_time)) * voice->prev_pitch;    /* portamento */
    if (do_control_update) {
        voice->prev_pitch = voice->current_pitch; /* save pitch for next time */
    }
    voice->current_pitch *= synth->pitch_bend * *(synth->tuning);

    /* condition some frequently-used integer ports */
    voice->osc1.mode        = lrintf(*(synth->osc1.mode));
    voice->osc1.waveform    = y_voice_waveform_index(synth->osc1.waveform);
    voice->osc2.mode        = lrintf(*(synth->osc2.mode));
    voice->osc2.waveform    = y_voice_waveform_index(synth->osc2.waveform);
    voice->osc3.mode        = lrintf(*(synth->osc3.mode));
    voice->osc3.waveform    = y_voice_waveform_index(synth->osc3.waveform);
    voice->osc4.mode        = lrintf(*(synth->osc4.mode));
    voice->osc4.waveform    = y_voice_waveform_index(synth->osc4.waveform);
    voice->vcf1.mode        = lrintf(*(synth->vcf1.mode));
    voice->vcf2.mode        = lrintf(*(synth->vcf2.mode));

    /* update modulators */
    voice->mod[Y_MOD_MODWHEEL] = synth->mod[Y_MOD_MODWHEEL];
    y_mod_update_pressure(synth, voice);
    voice->mod[Y_MOD_GLFO]     = synth->mod[Y_GLOBAL_MOD_GLFO];
    voice->mod[Y_MOD_GLFO_UP]  = synth->mod[Y_GLOBAL_MOD_GLFO_UP];
    y_mod_update_modmix(synth, voice, sample_count);

    /* --- VCO section */

    /* -FIX- this should move into oscillators, so they can do mode-specific things with it (like ignore it?...) */
    osc1_omega = voice->current_pitch * pitch_to_frequency(69.0f + *(synth->osc1.pitch) + *(synth->osc1.detune));
    osc2_omega = voice->current_pitch * pitch_to_frequency(69.0f + *(synth->osc2.pitch) + *(synth->osc2.detune));
    osc3_omega = voice->current_pitch * pitch_to_frequency(69.0f + *(synth->osc3.pitch) + *(synth->osc3.detune));
    osc4_omega = voice->current_pitch * pitch_to_frequency(69.0f + *(synth->osc4.pitch) + *(synth->osc4.detune));

    oscillator(sample_count, synth, &synth->osc1, voice, &voice->osc1, osc_index, deltat * osc1_omega);
    oscillator(sample_count, synth, &synth->osc2, voice, &voice->osc2, osc_index, deltat * osc2_omega);
    oscillator(sample_count, synth, &synth->osc3, voice, &voice->osc3, osc_index, deltat * osc3_omega);
    oscillator(sample_count, synth, &synth->osc4, voice, &voice->osc4, osc_index, deltat * osc4_omega);

    /* --- VCF section */

    voice->osc_bus_a[osc_index] += 1e-20f; /* make sure things don't get too quiet... */
    voice->osc_bus_b[osc_index] += 1e-20f;
    voice->osc_bus_a[osc_index + (sample_count >> 1)] -= 1e-20f;
    voice->osc_bus_b[osc_index + (sample_count >> 1)] -= 1e-20f;

    vcf_source = (*(synth->vcf1.source) < 0.001f) ? voice->osc_bus_a :
                                                    voice->osc_bus_b;
    vcf_source += osc_index;
    switch (lrintf(*(synth->vcf1.mode))) {
      default:
      case 0:
        vcf_off(sample_count, &voice->vcf1, synth->vcf1_out);
        break;
      case 1:
        vcf_2pole(sample_count, &synth->vcf1, voice, &voice->vcf1,
                  deltat * voice->current_pitch,
                  vcf_source, synth->vcf1_out);
        break;
      case 2:
        vcf_4pole(sample_count, &synth->vcf1, voice, &voice->vcf1,
                  deltat * voice->current_pitch,
                  vcf_source, synth->vcf1_out);
        break;
      case 3:
        vcf_mvclpf(sample_count, &synth->vcf1, voice, &voice->vcf1,
                   deltat * voice->current_pitch,
                   vcf_source, synth->vcf1_out);
        break;
      case 4:
        vcf_clip4pole(sample_count, &synth->vcf1, voice, &voice->vcf1,
                      deltat * voice->current_pitch,
                      vcf_source, synth->vcf1_out);
        break;
      case 5:
        vcf_bandpass(sample_count, &synth->vcf1, voice, &voice->vcf1,
                     deltat * voice->current_pitch,
                     vcf_source, synth->vcf1_out);
        break;
      case 6:
        vcf_amsynth(sample_count, &synth->vcf1, voice, &voice->vcf1,
                    deltat * voice->current_pitch,
                    vcf_source, synth->vcf1_out);
        break;
      case 7:
        vcf_resonz(sample_count, &synth->vcf1, voice, &voice->vcf1,
                  deltat * voice->current_pitch,
                  vcf_source, synth->vcf1_out);
        break;
      case 8:
        vcf_highpass_2pole(sample_count, &synth->vcf1, voice, &voice->vcf1,
                    deltat * voice->current_pitch,
                    vcf_source, synth->vcf1_out);
        break;
      case 9:
        vcf_highpass_4pole(sample_count, &synth->vcf1, voice, &voice->vcf1,
                    deltat * voice->current_pitch,
                    vcf_source, synth->vcf1_out);
        break;
      case 10:
        vcf_bandreject(sample_count, &synth->vcf1, voice, &voice->vcf1,
                    deltat * voice->current_pitch,
                    vcf_source, synth->vcf1_out);
        break;
    }

    switch (lrintf(*(synth->vcf2.source))) {
      default:
      case 0:
        vcf_source = voice->osc_bus_a + osc_index;
        break;
      case 1:
        vcf_source = voice->osc_bus_b + osc_index;
        break;
      case 2:
        vcf_source = synth->vcf1_out;
        break;
    }
    switch (lrintf(*(synth->vcf2.mode))) {
      default:
      case 0:
        vcf_off(sample_count, &voice->vcf2, synth->vcf2_out);
        break;
      case 1:
        vcf_2pole(sample_count, &synth->vcf2, voice, &voice->vcf2,
                  deltat * voice->current_pitch,
                  vcf_source, synth->vcf2_out);
        break;
      case 2:
        vcf_4pole(sample_count, &synth->vcf2, voice, &voice->vcf2,
                  deltat * voice->current_pitch,
                  vcf_source, synth->vcf2_out);
        break;
      case 3:
        vcf_mvclpf(sample_count, &synth->vcf2, voice, &voice->vcf2,
                   deltat * voice->current_pitch,
                   vcf_source, synth->vcf2_out);
        break;
      case 4:
        vcf_clip4pole(sample_count, &synth->vcf2, voice, &voice->vcf2,
                      deltat * voice->current_pitch,
                      vcf_source, synth->vcf2_out);
        break;
      case 5:
        vcf_bandpass(sample_count, &synth->vcf2, voice, &voice->vcf2,
                     deltat * voice->current_pitch,
                     vcf_source, synth->vcf2_out);
        break;
      case 6:
        vcf_amsynth(sample_count, &synth->vcf2, voice, &voice->vcf2,
                    deltat * voice->current_pitch,
                    vcf_source, synth->vcf2_out);
        break;
      case 7:
        vcf_resonz(sample_count, &synth->vcf2, voice, &voice->vcf2,
                  deltat * voice->current_pitch,
                  vcf_source, synth->vcf2_out);
        break;
      case 8:
        vcf_highpass_2pole(sample_count, &synth->vcf2, voice, &voice->vcf2,
                    deltat * voice->current_pitch,
                    vcf_source, synth->vcf2_out);
        break;
      case 9:
        vcf_highpass_4pole(sample_count, &synth->vcf2, voice, &voice->vcf2,
                    deltat * voice->current_pitch,
                    vcf_source, synth->vcf2_out);
        break;
      case 10:
        vcf_bandreject(sample_count, &synth->vcf2, voice, &voice->vcf2,
                    deltat * voice->current_pitch,
                    vcf_source, synth->vcf2_out);
        break;
    }

    /* --- VCA section */

    {   float amp_busa_l = *(synth->busa_level) * pan_cv_to_amplitude(1.0f - *(synth->busa_pan)),
              amp_busa_r = *(synth->busa_level) * pan_cv_to_amplitude(       *(synth->busa_pan)),
              amp_busb_l = *(synth->busb_level) * pan_cv_to_amplitude(1.0f - *(synth->busb_pan)),
              amp_busb_r = *(synth->busb_level) * pan_cv_to_amplitude(       *(synth->busb_pan)),
              amp_vcf1_l = *(synth->vcf1_level) * pan_cv_to_amplitude(1.0f - *(synth->vcf1_pan)),
              amp_vcf1_r = *(synth->vcf1_level) * pan_cv_to_amplitude(       *(synth->vcf1_pan)),
              amp_vcf2_l = *(synth->vcf2_level) * pan_cv_to_amplitude(1.0f - *(synth->vcf2_pan)),
              amp_vcf2_r = *(synth->vcf2_level) * pan_cv_to_amplitude(       *(synth->vcf2_pan)),
              vol_out   = volume(*(synth->volume) * synth->cc_volume),
              vca       = vol_out * volume_cv_to_amplitude(voice->mod[Y_MOD_EGO].value),
              vca_delta = vol_out * volume_cv_to_amplitude(voice->mod[Y_MOD_EGO].value +
                                                           voice->mod[Y_MOD_EGO].delta *
                                                               (float)sample_count),
              pan_l     = pan_cv_to_amplitude(1.0f - synth->cc_pan),
              pan_r     = pan_cv_to_amplitude(       synth->cc_pan),
              vca_l     = vca * pan_l,
              vca_r     = vca * pan_r,
              vca_delta_l = vca_delta * pan_l,
              vca_delta_r = vca_delta * pan_r;

        vca_delta_l = (vca_delta_l - vca_l) / (float)sample_count;
        vca_delta_r = (vca_delta_r - vca_r) / (float)sample_count;
          
        for (sample = 0; sample < sample_count; sample++) {
            out_left[sample]  += vca_l *
                                 (amp_busa_l * voice->osc_bus_a[sample + osc_index] +
                                  amp_busb_l * voice->osc_bus_b[sample + osc_index] +
                                  amp_vcf1_l * synth->vcf1_out[sample] +
                                  amp_vcf2_l * synth->vcf2_out[sample]);
            out_right[sample] += vca_r *
                                 (amp_busa_r * voice->osc_bus_a[sample + osc_index] +
                                  amp_busb_r * voice->osc_bus_b[sample + osc_index] +
                                  amp_vcf1_r * synth->vcf1_out[sample] +
                                  amp_vcf2_r * synth->vcf2_out[sample]);
            vca_l += vca_delta_l;
            vca_r += vca_delta_r;

            /* zero oscillator buses for next time around */
            voice->osc_bus_a[sample + osc_index] = 0.0f;
            voice->osc_bus_b[sample + osc_index] = 0.0f;
        }
    }

    osc_index += sample_count;

    if (do_control_update) {
        /* do those things that should be done only once per control-calculation
         * interval (render burst), such as voice check-for-dead, pitch
         * envelope calculations, volume envelope phase transition checks, etc.
         */

        /* -FIX- updating modulators _after_ rendering means a delay of up to 64 samples in
         * the response to those we can't predict (modwheel, pressure, etc.) -- as part of the
         * redesign, they need to be updated on the top side of the control period. */
        voice->mod[Y_MOD_MODWHEEL].value += (float)sample_count * voice->mod[Y_MOD_MODWHEEL].delta;
        voice->mod[Y_MOD_PRESSURE].value += (float)sample_count * voice->mod[Y_MOD_PRESSURE].delta;
        y_voice_update_lfo(synth, &synth->vlfo, &voice->vlfo,  voice->mod, &voice->mod[Y_MOD_VLFO]);
        y_voice_update_lfo(synth, &synth->mlfo, &voice->mlfo0, voice->mod, &voice->mod[Y_MOD_MLFO0]);
        y_voice_update_lfo(synth, &synth->mlfo, &voice->mlfo1, voice->mod, &voice->mod[Y_MOD_MLFO1]);
        y_voice_update_lfo(synth, &synth->mlfo, &voice->mlfo2, voice->mod, &voice->mod[Y_MOD_MLFO2]);
        y_voice_update_lfo(synth, &synth->mlfo, &voice->mlfo3, voice->mod, &voice->mod[Y_MOD_MLFO3]);

        y_voice_update_eg(&synth->ego, voice, &voice->ego, &voice->mod[Y_MOD_EGO]);
        /* check if we've decayed to nothing, turn off voice if so */
        if (y_voice_check_for_dead(synth, voice))
            return; /* we're dead now, so return */

        y_voice_update_eg(&synth->eg1, voice, &voice->eg1, &voice->mod[Y_MOD_EG1]);
        y_voice_update_eg(&synth->eg2, voice, &voice->eg2, &voice->mod[Y_MOD_EG2]);
        y_voice_update_eg(&synth->eg3, voice, &voice->eg3, &voice->mod[Y_MOD_EG3]);
        y_voice_update_eg(&synth->eg4, voice, &voice->eg4, &voice->mod[Y_MOD_EG4]);
        voice->mod[Y_MOD_MIX].value += (float)sample_count * voice->mod[Y_MOD_MIX].delta;

        osc_index &= OSC_BUS_MASK;

    } else {
        /* update modulator values for the incomplete control cycle */
        int i;

        for (i = 1; i < Y_MODS_COUNT; i++)
            voice->mod[i].value += (float)sample_count * voice->mod[i].delta;
    }

    /* save things for next time around */

    /* already saved prev_pitch above */
    voice->osc_index  = osc_index;
}

