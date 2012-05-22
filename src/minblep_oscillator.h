/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004-2008, 2010, 2012 Sean Bolton.
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

/* Ah, the gentle tedium of moving all possible conditionals outside
 * the inner loops!
 *
 * This file gets included twice from whysynth_voice_render.c, to
 * define both master (sync generating) and slave (hard syncing)
 * oscillator functions.
 *
 * The classic-waveform minBLEP functions (oscillator mode 1) are
 * declared first, followed by the wavetable functions (oscillator
 * mode 2), which also use minBLEPs when syncing for alias reduction.
 *
 * As a example of how the BLOSC_THIS macro works, if BLOSC_MASTER is
 * defined when this file is included, then:
 *     BLOSC_THIS(sine, ...)
 * gets replaced with:
 *     blosc_mastersine(...)
 */

#if BLOSC_MASTER
/* #define BLOSC_THIS(x, ...) blosc_master##x(__VA_ARGS__) */
#define BLOSC_THIS(x, args...) blosc_master##x(args)
#else
#define BLOSC_THIS(x, args...) blosc_slave##x(args)
#endif

/* ==== minBLEP oscillators ==== */

/* ---- blosc_*saw functions ---- */

static void
BLOSC_THIS(saw, unsigned long sample_count, y_sosc_t *sosc,
           y_voice_t *voice, struct vosc *vosc, int index, float w0)
{
    unsigned long sample;
    int mod;
    float pos = (float)vosc->pos0,
          amt,
          w, w_delta,
          gain_a, gain_a_delta,
          gain_b, gain_b_delta;

    if (vosc->last_waveform != vosc->waveform) {

        /* this would be the cleanest startup:
         *   pos = 0.5f - w;
         *   blosc_place_slope_dd(index, 0.0f, w, voice->osc_bus_a, -gain_a,
         *                                        voice->osc_bus_b, -gain_b);
         * but we have to match the phase of the original Xsynth code: */
        pos = 0.0f;

        vosc->last_waveform = vosc->waveform;
    }

    /* -FIX- what if we didn't ramp pitch? */
    mod = y_voice_mod_index(sosc->pitch_mod_src);
    amt = *(sosc->pitch_mod_amt);
    w = 1.0f + amt * voice->mod[mod].value;
    w_delta = w + amt * voice->mod[mod].delta * (float)sample_count;
    w_delta *= w0;
    w *= w0;
    w_delta = (w_delta - w) / (float)sample_count;
    /* -FIX- condition to [0, 0.5)? */

    mod = y_voice_mod_index(sosc->amp_mod_src);
    amt = *(sosc->amp_mod_amt);
    if (amt > 0.0f)
        gain_a = 1.0f - amt + amt * voice->mod[mod].value;
    else
        gain_a = 1.0f + amt * voice->mod[mod].value;
    gain_a_delta = volume_cv_to_amplitude(gain_a + amt * voice->mod[mod].delta * (float)sample_count);
    gain_a       = volume_cv_to_amplitude(gain_a);
    if (vosc->waveform == 0) {
        gain_a       = -gain_a;
        gain_a_delta = -gain_a_delta;
    }
    gain_b       = gain_a       * *(sosc->level_b);
    gain_b_delta = gain_a_delta * *(sosc->level_b);
    gain_a       *= *(sosc->level_a);
    gain_a_delta *= *(sosc->level_a);
    gain_a_delta = (gain_a_delta - gain_a) / (float)sample_count;
    gain_b_delta = (gain_b_delta - gain_b) / (float)sample_count;
    /* -FIX- condition to [0, +/-1]? */

    for (sample = 0; sample < sample_count; sample++) {

        pos += w;

#if !BLOSC_MASTER
        if (voice->osc_sync[sample] >= 0.0f) { /* sync to master */

            float eof_offset = voice->osc_sync[sample] * w;
            float pos_at_reset = pos - eof_offset;
            pos = eof_offset;

            /* place any DD that may have occurred in subsample before reset */
            if (pos_at_reset >= 1.0f) {
                pos_at_reset -= 1.0f;
                blosc_place_step_dd(index, pos_at_reset + eof_offset, w,
                                    voice->osc_bus_a, gain_a,
                                    voice->osc_bus_b, gain_b);
            }

            /* now place reset DD */
            blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a * pos_at_reset,
                                               voice->osc_bus_b, gain_b * pos_at_reset);
        } else
#endif /* slave */
        if (pos >= 1.0f) {
            pos -= 1.0f;
#if BLOSC_MASTER
            voice->osc_sync[sample] = pos / w;
#endif /* master */
            blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a,
                                               voice->osc_bus_b, gain_b);
#if BLOSC_MASTER
        } else {
            voice->osc_sync[sample] = -1.0f;
#endif /* master */
        }
        voice->osc_bus_a[(index + DD_SAMPLE_DELAY) & OSC_BUS_MASK] += gain_a * (0.5f - pos);
        voice->osc_bus_b[(index + DD_SAMPLE_DELAY) & OSC_BUS_MASK] += gain_b * (0.5f - pos);

        index++;

        w += w_delta;
        gain_a += gain_a_delta;
        gain_b += gain_b_delta;
    }

    vosc->pos0 = (double)pos;
}

/* ---- blosc_*rect functions ---- */

static void
BLOSC_THIS(rect, unsigned long sample_count, y_sosc_t *sosc,
           y_voice_t *voice, struct vosc *vosc, int index, float w0)
{
    unsigned long sample;
    int   mod,
          bp_high = vosc->i1;  /* true when in 'high' state */
    float pos = (float)vosc->pos0,
          amt,
          w, w_delta,
          pw, pw_delta,
          gain_a, gain_a_delta,
          gain_b, gain_b_delta,
          out = (bp_high ? 0.5f : -0.5f);

    if (vosc->last_waveform != vosc->waveform) {

        pos = 0.0f;
        /* For variable-width pulse, we could do DC compensation with:
         *     out = 0.5f * (1.0f - pw);
         * but that doesn't work well with highly modulated hard sync.  Instead,
         * we keep things in the range [-0.5f, 0.5f]. */
        bp_high = 1;
        out = 0.5;
        /* if we valued alias-free startup over low startup time, we could do:
         *   pos -= w;
         *   blosc_place_step_dd(index, 0.0f, w, voice->osc_bus_a, 0.5f * gain_a,
         *                                       voice->osc_bus_b, 0.5f * gain_b); */

        vosc->last_waveform = vosc->waveform;
    }

    /* -FIX- what if we didn't ramp pitch? */
    mod = y_voice_mod_index(sosc->pitch_mod_src);
    amt = *(sosc->pitch_mod_amt);
    w = 1.0f + amt * voice->mod[mod].value;
    w_delta = w + amt * voice->mod[mod].delta * (float)sample_count;
    w_delta *= w0;
    w *= w0;
    w_delta = (w_delta - w) / (float)sample_count;
    /* -FIX- condition to [0, 0.5)? */

    /* -FIX- what if we didn't ramp pulsewidth? */
    mod = y_voice_mod_index(sosc->mmod_src);
    amt = *(sosc->mmod_amt);
    pw = *(sosc->mparam2) + amt * voice->mod[mod].value;
    pw_delta = pw + amt * voice->mod[mod].delta * (float)sample_count;
    if (pw < w) pw = w;  /* w is sample phase width */
    else if (pw > 1.0f - w) pw = 1.0f - w;
    if (pw_delta < w) pw_delta = w;
    else if (pw_delta > 1.0f - w) pw_delta = 1.0f - w;
    pw_delta = (pw_delta - pw) / (float)sample_count;

    mod = y_voice_mod_index(sosc->amp_mod_src);
    amt = *(sosc->amp_mod_amt);
    if (amt > 0.0f)
        gain_a = 1.0f - amt + amt * voice->mod[mod].value;
    else
        gain_a = 1.0f + amt * voice->mod[mod].value;
    gain_a_delta = volume_cv_to_amplitude(gain_a + amt * voice->mod[mod].delta * (float)sample_count);
    gain_a       = volume_cv_to_amplitude(gain_a);
    gain_b       = gain_a       * *(sosc->level_b);
    gain_b_delta = gain_a_delta * *(sosc->level_b);
    gain_a       *= *(sosc->level_a);
    gain_a_delta *= *(sosc->level_a);
    gain_a_delta = (gain_a_delta - gain_a) / (float)sample_count;
    gain_b_delta = (gain_b_delta - gain_b) / (float)sample_count;
    /* -FIX- condition to [0, +/-1]? */

    for (sample = 0; sample < sample_count; sample++) {

        pos += w;

#if !BLOSC_MASTER
        if (voice->osc_sync[sample] >= 0.0f) { /* sync to master */

            float eof_offset = voice->osc_sync[sample] * w;
            float pos_at_reset = pos - eof_offset;
            pos = eof_offset;

            /* place any DDs that may have occurred in subsample before reset */
            if (bp_high) {
                if (pos_at_reset >= pw) {
                    blosc_place_step_dd(index, pos_at_reset - pw + eof_offset, w,
                                        voice->osc_bus_a, -gain_a,
                                        voice->osc_bus_b, -gain_b);
                    bp_high = 0;
                    out = -0.5f;
                }
                if (pos_at_reset >= 1.0f) {
                    pos_at_reset -= 1.0f;
                    blosc_place_step_dd(index, pos_at_reset + eof_offset, w,
                                        voice->osc_bus_a, gain_a,
                                        voice->osc_bus_b, gain_b);
                    bp_high = 1;
                    out = 0.5f;
                }
            } else {
                if (pos_at_reset >= 1.0f) {
                    pos_at_reset -= 1.0f;
                    blosc_place_step_dd(index, pos_at_reset + eof_offset, w,
                                        voice->osc_bus_a, gain_a,
                                        voice->osc_bus_b, gain_b);
                    bp_high = 1;
                    out = 0.5f;
                }
                if (bp_high && pos_at_reset >= pw) {
                    blosc_place_step_dd(index, pos_at_reset - pw + eof_offset, w,
                                        voice->osc_bus_a, -gain_a,
                                        voice->osc_bus_b, -gain_b);
                    bp_high = 0;
                    out = -0.5f;
                }
            }

            /* now place reset DD */
            if (!bp_high) {
                blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a,
                                                   voice->osc_bus_b, gain_b);
                bp_high = 1;
                out = 0.5f;
            }
            if (pos >= pw) {
                blosc_place_step_dd(index, pos - pw, w, voice->osc_bus_a, -gain_a,
                                                        voice->osc_bus_b, -gain_b);
                bp_high = 0;
                out = -0.5f;
            }
        } else
#endif /* slave */
        if (bp_high) {
            if (pos >= pw) {
                blosc_place_step_dd(index, pos - pw, w, voice->osc_bus_a, -gain_a,
                                                        voice->osc_bus_b, -gain_b);
                bp_high = 0;
                out = -0.5f;
            }
            if (pos >= 1.0f) {
                pos -= 1.0f;
#if BLOSC_MASTER
                voice->osc_sync[sample] = pos / w;
#endif /* master */
                blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a,
                                                   voice->osc_bus_b, gain_b);
                bp_high = 1;
                out = 0.5f;
#if BLOSC_MASTER
            } else {
                voice->osc_sync[sample] = -1.0f;
#endif /* master */
            }
        } else {
            if (pos >= 1.0f) {
                pos -= 1.0f;
#if BLOSC_MASTER
                voice->osc_sync[sample] = pos / w;
#endif /* master */
                blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a,
                                                   voice->osc_bus_b, gain_b);
                bp_high = 1;
                out = 0.5f;
#if BLOSC_MASTER
            } else {
                voice->osc_sync[sample] = -1.0f;
#endif /* master */
            }
            if (bp_high && pos >= pw) {
                blosc_place_step_dd(index, pos - pw, w, voice->osc_bus_a, -gain_a,
                                                        voice->osc_bus_b, -gain_b);
                bp_high = 0;
                out = -0.5f;
            }
        }
        voice->osc_bus_a[(index + DD_SAMPLE_DELAY) & OSC_BUS_MASK] += out * gain_a;
        voice->osc_bus_b[(index + DD_SAMPLE_DELAY) & OSC_BUS_MASK] += out * gain_b;

        index++;

        w += w_delta;
        pw += pw_delta;
        gain_a += gain_a_delta;
        gain_b += gain_b_delta;
    }

    vosc->pos0 = (double)pos;
    vosc->i1 = bp_high;
}

/* ---- blosc_*tri functions ---- */

static void
BLOSC_THIS(tri, unsigned long sample_count, y_sosc_t *sosc,
           y_voice_t *voice, struct vosc *vosc, int index, float w0)
{
    unsigned long sample;
    int   mod,
          bp_high = vosc->i1;  /* true when slope is positive */
    float pos = (float)vosc->pos0,
          amt, out, slope_delta,
          w, w_delta,
          pw, pw_delta,
          gain_a, gain_a_delta,
          gain_b, gain_b_delta;

    /* -FIX- what if we didn't ramp pitch? */
    mod = y_voice_mod_index(sosc->pitch_mod_src);
    amt = *(sosc->pitch_mod_amt);
    w = 1.0f + amt * voice->mod[mod].value;
    w_delta = w + amt * voice->mod[mod].delta * (float)sample_count;
    w_delta *= w0;
    w *= w0;
    w_delta = (w_delta - w) / (float)sample_count;
    /* -FIX- condition to [0, 0.5)? */

    /* -FIX- what if we didn't ramp slope? (could move slope_delta calculation back here) */
    mod = y_voice_mod_index(sosc->mmod_src);
    amt = *(sosc->mmod_amt);
    pw = *(sosc->mparam2) + amt * voice->mod[mod].value;
    pw_delta = pw + amt * voice->mod[mod].delta * (float)sample_count;
    if (pw < w) pw = w;  /* w is sample phase width */
    else if (pw > 1.0f - w) pw = 1.0f - w;
    if (pw_delta < w) pw_delta = w;
    else if (pw_delta > 1.0f - w) pw_delta = 1.0f - w;
    pw_delta = (pw_delta - pw) / (float)sample_count;

    if (vosc->last_waveform != vosc->waveform) {

        /* Setting pos to 0.5 * pw here starts the waveform at a zero crossing
         * for least startup noise, and maintains compatibility with the original
         * Xsynth (which WhySynth inherited through Xsynth-DSSI; now, it's more
         * important to not change the sound of existing patches.)  This does
         * cause a problem with certain patches where this wave is used in a
         * hard-synced slave oscillator: when the master resets the first time,
         * the slave resets to pos = 0, which means that for the first master
         * cycle, the slave will be about one quarter phase ahead of subsequent
         * cycles, producing an audible click or thump. If we didn't need to
         * preserve the sound of existing patches, we could do one of three
         * things: set pos to 0 here, change the sync code below to reset to
         * pos = 0.5 * pw , or change all the sync-generating oscillators to
         * generate a sync pulse immediately on startup. */
        pos = 0.5f * pw;
        /* if we valued alias-free startup over low startup time, we could do:
         *   pos -= w;
         *   blosc_place_slope_dd(voice->osc_audio, index, 0.0f, w, gain * 1.0f / *pw); */
        bp_high = 1;

        vosc->last_waveform = vosc->waveform;
    }

    mod = y_voice_mod_index(sosc->amp_mod_src);
    amt = *(sosc->amp_mod_amt);
    if (amt > 0.0f)
        gain_a = 1.0f - amt + amt * voice->mod[mod].value;
    else
        gain_a = 1.0f + amt * voice->mod[mod].value;
    gain_a_delta = volume_cv_to_amplitude(gain_a + amt * voice->mod[mod].delta * (float)sample_count);
    gain_a       = volume_cv_to_amplitude(gain_a);
    gain_b       = gain_a       * *(sosc->level_b);
    gain_b_delta = gain_a_delta * *(sosc->level_b);
    gain_a       *= *(sosc->level_a);
    gain_a_delta *= *(sosc->level_a);
    gain_a_delta = (gain_a_delta - gain_a) / (float)sample_count;
    gain_b_delta = (gain_b_delta - gain_b) / (float)sample_count;
    /* -FIX- condition to [0, +/-1]? */

    for (sample = 0; sample < sample_count; sample++) {

        pos += w;

#if !BLOSC_MASTER
        if (voice->osc_sync[sample] >= 0.0f) { /* sync to master */

            float eof_offset = voice->osc_sync[sample] * w;
            float pos_at_reset = pos - eof_offset;
            pos = eof_offset;

            /* place any DDs that may have occurred in subsample before reset */
            if (bp_high) {
                out = -0.5f + pos_at_reset / pw;
                if (pos_at_reset >= pw) {
                    out = 0.5f - (pos_at_reset - pw) / (1.0f - pw);
                    slope_delta = (-1.0f / pw - 1.0f / (1.0f - pw)); /* -FIX- move this back up? */
                    blosc_place_slope_dd(index, pos_at_reset - pw + eof_offset, w,
                                         voice->osc_bus_a, gain_a * slope_delta, /* -FIX- change this back to '-slope_delta' if you do... */
                                         voice->osc_bus_b, gain_b * slope_delta);
                    bp_high = 0;
                }
                if (pos_at_reset >= 1.0f) {
                    pos_at_reset -= 1.0f;
                    out = -0.5f + pos_at_reset / pw;
                    slope_delta = (1.0f / pw + 1.0f / (1.0f - pw));
                    blosc_place_slope_dd(index, pos_at_reset + eof_offset, w,
                                         voice->osc_bus_a, gain_a * slope_delta,
                                         voice->osc_bus_b, gain_b * slope_delta);
                    bp_high = 1;
                }
            } else {
                out = 0.5f - (pos_at_reset - pw) / (1.0f - pw);
                if (pos_at_reset >= 1.0f) {
                    pos_at_reset -= 1.0f;
                    out = -0.5f + pos_at_reset / pw;
                    slope_delta = (1.0f / pw + 1.0f / (1.0f - pw));
                    blosc_place_slope_dd(index, pos_at_reset + eof_offset, w,
                                         voice->osc_bus_a, gain_a * slope_delta,
                                         voice->osc_bus_b, gain_b * slope_delta);
                    bp_high = 1;
                }
                if (bp_high && pos_at_reset >= pw) {
                    out = 0.5f - (pos_at_reset - pw) / (1.0f - pw);
                    slope_delta = (-1.0f / pw - 1.0f / (1.0f - pw));
                    blosc_place_slope_dd(index, pos_at_reset - pw + eof_offset, w,
                                         voice->osc_bus_a, gain_a * slope_delta,
                                         voice->osc_bus_b, gain_b * slope_delta);
                    bp_high = 0;
                }
            }

            /* now place reset DDs */
            if (!bp_high) {
                slope_delta = (1.0f / pw + 1.0f / (1.0f - pw));
                blosc_place_slope_dd(index, pos, w, voice->osc_bus_a, gain_a * slope_delta,
                                                    voice->osc_bus_b, gain_b * slope_delta);
            }
            blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a * (-0.5f - out),
                                               voice->osc_bus_b, gain_b * (-0.5f - out));
            out = -0.5f + pos / pw;
            bp_high = 1;
            if (pos >= pw) {
                out = 0.5f - (pos - pw) / (1.0f - pw);
                slope_delta = (-1.0f / pw - 1.0f / (1.0f - pw));
                blosc_place_slope_dd(index, pos - pw, w, voice->osc_bus_a, gain_a * slope_delta,
                                                         voice->osc_bus_b, gain_b * slope_delta);
                bp_high = 0;
            }
        } else
#endif /* slave */
        if (bp_high) {
            out = -0.5f + pos / pw;
            if (pos >= pw) {
                out = 0.5f - (pos - pw) / (1.0f - pw);
                slope_delta = (-1.0f / pw - 1.0f / (1.0f - pw));
                blosc_place_slope_dd(index, pos - pw, w, voice->osc_bus_a, gain_a * slope_delta,
                                                         voice->osc_bus_b, gain_b * slope_delta);
                bp_high = 0;
            }
            if (pos >= 1.0f) {
                pos -= 1.0f;
#if BLOSC_MASTER
                voice->osc_sync[sample] = pos / w;
#endif /* master */
                out = -0.5f + pos / pw;
                slope_delta = (1.0f / pw + 1.0f / (1.0f - pw));
                blosc_place_slope_dd(index, pos, w, voice->osc_bus_a, gain_a * slope_delta,
                                                    voice->osc_bus_b, gain_b * slope_delta);
                bp_high = 1;
#if BLOSC_MASTER
            } else {
                voice->osc_sync[sample] = -1.0f;
#endif /* master */
            }
        } else {
            out = 0.5f - (pos - pw) / (1.0f - pw);
            if (pos >= 1.0f) {
                pos -= 1.0f;
#if BLOSC_MASTER
                voice->osc_sync[sample] = pos / w;
#endif /* master */
                out = -0.5f + pos / pw;
                slope_delta = (1.0f / pw + 1.0f / (1.0f - pw));
                blosc_place_slope_dd(index, pos, w, voice->osc_bus_a, gain_a * slope_delta,
                                                    voice->osc_bus_b, gain_b * slope_delta);
                bp_high = 1;
#if BLOSC_MASTER
            } else {
                voice->osc_sync[sample] = -1.0f;
#endif /* master */
            }
            if (bp_high && pos >= pw) {
                out = 0.5f - (pos - pw) / (1.0f - pw);
                slope_delta = (-1.0f / pw - 1.0f / (1.0f - pw));
                blosc_place_slope_dd(index, pos - pw, w, voice->osc_bus_a, gain_a * slope_delta,
                                                         voice->osc_bus_b, gain_b * slope_delta);
                bp_high = 0;
            }
        }
        voice->osc_bus_a[(index + DD_SAMPLE_DELAY) & OSC_BUS_MASK] += gain_a * out;
        voice->osc_bus_b[(index + DD_SAMPLE_DELAY) & OSC_BUS_MASK] += gain_b * out;

        index++;

        w += w_delta;
        pw += pw_delta;
        gain_a += gain_a_delta;
        gain_b += gain_b_delta;
    }

    vosc->pos0 = (double)pos;
    vosc->i1 = bp_high;
}

/* ---- blosc_*noise functions ---- */

static void
BLOSC_THIS(noise, unsigned long sample_count, y_sosc_t *sosc,
           y_voice_t *voice, struct vosc *vosc, int index, float w0)
{
    unsigned long sample;
    int   mod,
          bp_high = vosc->i1;
    float pos = (float)vosc->pos0,
          amt,
          w, w_delta,
          pw, pw_delta,
          gain_a, gain_a_delta,
          gain_b, gain_b_delta,
          newout, out = vosc->f0;

    if (vosc->last_waveform != vosc->waveform) {

        pos = 0.0f;
        bp_high = 1;
        out = random_float(-0.5f, 1.0f);
        vosc->f0 = out;

        vosc->last_waveform = vosc->waveform;
    }

    /* -FIX- what if we didn't ramp pitch? */
    mod = y_voice_mod_index(sosc->pitch_mod_src);
    amt = *(sosc->pitch_mod_amt);
    w = 1.0f + amt * voice->mod[mod].value;
    w_delta = w + amt * voice->mod[mod].delta * (float)sample_count;
    w_delta *= w0;
    w *= w0;
    w_delta = (w_delta - w) / (float)sample_count;
    /* -FIX- condition to [0, 0.5)? */

    /* -FIX- what if we didn't ramp pulsewidth? */
    mod = y_voice_mod_index(sosc->mmod_src);
    amt = *(sosc->mmod_amt);
    pw = *(sosc->mparam2) + amt * voice->mod[mod].value;
    pw_delta = pw + amt * voice->mod[mod].delta * (float)sample_count;
    if (pw < w) pw = w;  /* w is sample phase width */
    else if (pw > 1.0f - w) pw = 1.0f - w;
    if (pw_delta < w) pw_delta = w;
    else if (pw_delta > 1.0f - w) pw_delta = 1.0f - w;
    pw_delta = (pw_delta - pw) / (float)sample_count;

    mod = y_voice_mod_index(sosc->amp_mod_src);
    amt = *(sosc->amp_mod_amt);
    if (amt > 0.0f)
        gain_a = 1.0f - amt + amt * voice->mod[mod].value;
    else
        gain_a = 1.0f + amt * voice->mod[mod].value;
    gain_a_delta = volume_cv_to_amplitude(gain_a + amt * voice->mod[mod].delta * (float)sample_count);
    gain_a       = volume_cv_to_amplitude(gain_a);
    gain_b       = gain_a       * *(sosc->level_b);
    gain_b_delta = gain_a_delta * *(sosc->level_b);
    gain_a       *= *(sosc->level_a);
    gain_a_delta *= *(sosc->level_a);
    gain_a_delta = (gain_a_delta - gain_a) / (float)sample_count;
    gain_b_delta = (gain_b_delta - gain_b) / (float)sample_count;
    /* -FIX- condition to [0, +/-1]? */

    for (sample = 0; sample < sample_count; sample++) {

        pos += w;

#if !BLOSC_MASTER
        if (voice->osc_sync[sample] >= 0.0f) { /* sync to master */

            float eof_offset = voice->osc_sync[sample] * w;
            float pos_at_reset = pos - eof_offset;
            pos = eof_offset;

            /* place any DDs that may have occurred in subsample before reset */
            if (bp_high) {
                if (pos_at_reset >= pw) {
                    blosc_place_step_dd(index, pos_at_reset - pw + eof_offset, w,
                                        voice->osc_bus_a, -2.0f * out * gain_a,
                                        voice->osc_bus_b, -2.0f * out * gain_b);
                    bp_high = 0;
                    out = -out;
                }
                if (pos_at_reset >= 1.0f) {
                    pos_at_reset -= 1.0f;
                    newout = random_float(-0.5f, 1.0f);
                    blosc_place_step_dd(index, pos_at_reset + eof_offset, w,
                                        voice->osc_bus_a, gain_a * (newout - out),
                                        voice->osc_bus_b, gain_b * (newout - out));
                    bp_high = 1;
                    out = newout;
                }
            } else {
                if (pos_at_reset >= 1.0f) {
                    pos_at_reset -= 1.0f;
                    newout = random_float(-0.5f, 1.0f);
                    blosc_place_step_dd(index, pos_at_reset + eof_offset, w,
                                        voice->osc_bus_a, gain_a * (newout - out),
                                        voice->osc_bus_b, gain_b * (newout - out));
                    bp_high = 1;
                    out = newout;
                }
                if (bp_high && pos_at_reset >= pw) {
                    blosc_place_step_dd(index, pos_at_reset - pw + eof_offset, w,
                                        voice->osc_bus_a, -2.0f * out * gain_a,
                                        voice->osc_bus_b, -2.0f * out * gain_b);
                    bp_high = 0;
                    out = -out;
                }
            }

            /* now place reset DD */
            if (!bp_high) {
                newout = random_float(-0.5f, 1.0f);
                blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a * (newout - out),
                                                   voice->osc_bus_b, gain_b * (newout - out));
                bp_high = 1;
                out = newout;
            }
            if (pos >= pw) {
                blosc_place_step_dd(index, pos - pw, w, voice->osc_bus_a, -2.0f * out * gain_a,
                                                        voice->osc_bus_b, -2.0f * out * gain_b);
                bp_high = 0;
                out = -out;
            }
        } else
#endif /* slave */
        if (bp_high) {
            if (pos >= pw) {
                blosc_place_step_dd(index, pos - pw, w, voice->osc_bus_a, -2.0f * out * gain_a,
                                                        voice->osc_bus_b, -2.0f * out * gain_b);
                bp_high = 0;
                out = -out;
            }
            if (pos >= 1.0f) {
                pos -= 1.0f;
#if BLOSC_MASTER
                voice->osc_sync[sample] = pos / w;
#endif /* master */
                newout = random_float(-0.5f, 1.0f);
                blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a * (newout - out),
                                                   voice->osc_bus_b, gain_b * (newout - out));
                bp_high = 1;
                out = newout;
#if BLOSC_MASTER
            } else {
                voice->osc_sync[sample] = -1.0f;
#endif /* master */
            }
        } else {
            if (pos >= 1.0f) {
                pos -= 1.0f;
#if BLOSC_MASTER
                voice->osc_sync[sample] = pos / w;
#endif /* master */
                newout = random_float(-0.5f, 1.0f);
                blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a * (newout - out),
                                                   voice->osc_bus_b, gain_b * (newout - out));
                bp_high = 1;
                out = newout;
#if BLOSC_MASTER
            } else {
                voice->osc_sync[sample] = -1.0f;
#endif /* master */
            }
            if (bp_high && pos >= pw) {
                blosc_place_step_dd(index, pos - pw, w, voice->osc_bus_a, -2.0f * out * gain_a,
                                                        voice->osc_bus_b, -2.0f * out * gain_b);
                bp_high = 0;
                out = -out;
            }
        }
        voice->osc_bus_a[(index + DD_SAMPLE_DELAY) & OSC_BUS_MASK] += out * gain_a;
        voice->osc_bus_b[(index + DD_SAMPLE_DELAY) & OSC_BUS_MASK] += out * gain_b;

        index++;

        w += w_delta;
        pw += pw_delta;
        gain_a += gain_a_delta;
        gain_b += gain_b_delta;
    }

    vosc->pos0 = (double)pos;
    vosc->i1 = bp_high;
    vosc->f0 = out;
}

/* ---- blosc_*clipsaw functions ---- */

static void
BLOSC_THIS(clipsaw, unsigned long sample_count, y_sosc_t *sosc,
           y_voice_t *voice, struct vosc *vosc, int index, float w0)
{
    unsigned long sample;
    int   mod,
          state = vosc->i1;  /* true in flat "clipped" part of wave */
    float pos = (float)vosc->pos0,
          amt, out,
          w, w_delta,
          pw, pw_delta,
          gain_a, gain_a_delta,
          gain_b, gain_b_delta;

    if (vosc->last_waveform != vosc->waveform) {

        pos = 0.0f;
        state = 0;

        vosc->last_waveform = vosc->waveform;
    }

    /* -FIX- what if we didn't ramp pitch? */
    mod = y_voice_mod_index(sosc->pitch_mod_src);
    amt = *(sosc->pitch_mod_amt);
    w = 1.0f + amt * voice->mod[mod].value;
    w_delta = w + amt * voice->mod[mod].delta * (float)sample_count;
    w_delta *= w0;
    w *= w0;
    w_delta = (w_delta - w) / (float)sample_count;
    /* -FIX- condition to [0, 0.5)? */

    /* -FIX- what if we didn't ramp slope? (could move slope_delta calculation back here) */
    mod = y_voice_mod_index(sosc->mmod_src);
    amt = *(sosc->mmod_amt);
    pw = *(sosc->mparam2) + amt * voice->mod[mod].value;
    pw_delta = pw + amt * voice->mod[mod].delta * (float)sample_count;
    if (pw < w) pw = w;  /* w is sample phase width */
    else if (pw > 1.0f - w) pw = 1.0f - w;
    if (pw_delta < w) pw_delta = w;
    else if (pw_delta > 1.0f - w) pw_delta = 1.0f - w;
    pw_delta = (pw_delta - pw) / (float)sample_count;

    mod = y_voice_mod_index(sosc->amp_mod_src);
    amt = *(sosc->amp_mod_amt);
    if (amt > 0.0f)
        gain_a = 1.0f - amt + amt * voice->mod[mod].value;
    else
        gain_a = 1.0f + amt * voice->mod[mod].value;
    gain_a_delta = volume_cv_to_amplitude(gain_a + amt * voice->mod[mod].delta * (float)sample_count);
    gain_a       = volume_cv_to_amplitude(gain_a);
    gain_b       = gain_a       * *(sosc->level_b);
    gain_b_delta = gain_a_delta * *(sosc->level_b);
    gain_a       *= *(sosc->level_a);
    gain_a_delta *= *(sosc->level_a);
    gain_a_delta = (gain_a_delta - gain_a) / (float)sample_count;
    gain_b_delta = (gain_b_delta - gain_b) / (float)sample_count;
    /* -FIX- condition to [0, +/-1]? */

    for (sample = 0; sample < sample_count; sample++) {

        pos += w;

#if !BLOSC_MASTER
        if (voice->osc_sync[sample] >= 0.0f) { /* sync to master */

            float eof_offset = voice->osc_sync[sample] * w;
            float pos_at_reset = pos - eof_offset;
            pos = eof_offset;

            /* place any DDs that may have occurred in subsample before reset */
            if (state) {
                out = -0.5f;
                if (pos_at_reset >= 1.0f) {
                    pos_at_reset -= 1.0f;
                    out = 0.5f - pos_at_reset / pw;
                    blosc_place_step_dd(index, pos_at_reset + eof_offset, w,
                                        voice->osc_bus_a, gain_a,
                                        voice->osc_bus_b, gain_b);
                    blosc_place_slope_dd(index, pos_at_reset + eof_offset, w,
                                         voice->osc_bus_a, -gain_a / pw,
                                         voice->osc_bus_b, -gain_b / pw);
                    state = 0;
                }
                if (!state && pos_at_reset >= pw) {
                    out = -0.5f;
                    blosc_place_slope_dd(index, pos_at_reset - pw + eof_offset, w,
                                         voice->osc_bus_a, gain_a / pw,
                                         voice->osc_bus_b, gain_b / pw);
                    state = 1;
                }
            } else {
                out = 0.5f - pos_at_reset / pw;
                if (pos_at_reset >= pw) {
                    out = -0.5f;
                    blosc_place_slope_dd(index, pos_at_reset - pw + eof_offset, w,
                                         voice->osc_bus_a, gain_a / pw,
                                         voice->osc_bus_b, gain_b / pw);
                    state = 1;
                }
                if (pos_at_reset >= 1.0f) {
                    pos_at_reset -= 1.0f;
                    out = 0.5f - pos_at_reset / pw;
                    blosc_place_step_dd(index, pos_at_reset + eof_offset, w,
                                        voice->osc_bus_a, gain_a,
                                        voice->osc_bus_b, gain_b);
                    blosc_place_slope_dd(index, pos_at_reset + eof_offset, w,
                                         voice->osc_bus_a, -gain_a / pw,
                                         voice->osc_bus_b, -gain_b / pw);
                    state = 0;
                }
            }

            /* now place reset DDs */
            if (state) {
                blosc_place_slope_dd(index, pos, w, voice->osc_bus_a, -gain_a / pw,
                                                    voice->osc_bus_b, -gain_b / pw);
            }
            blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a * (0.5f - out),
                                               voice->osc_bus_b, gain_b * (0.5f - out));
            out = 0.5f - pos / pw;
            state = 0;
            if (pos >= pw) {
                out = -0.5f;
                blosc_place_slope_dd(index, pos - pw, w, voice->osc_bus_a, gain_a / pw,
                                                         voice->osc_bus_b, gain_b / pw);
                state = 1;
            }
        } else
#endif /* slave */
        if (state) {  /* second half of waveform: chopped off portion */
            out = -0.5f;
            if (pos >= 1.0f) {
                pos -= 1.0f;
#if BLOSC_MASTER
                voice->osc_sync[sample] = pos / w;
#endif /* master */
                out = 0.5f - pos / pw;
		blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a,
                                                   voice->osc_bus_b, gain_b);
		blosc_place_slope_dd(index, pos, w, voice->osc_bus_a, -gain_a / pw,
                                                    voice->osc_bus_b, -gain_b / pw);
                state = 0;
#if BLOSC_MASTER
            } else {
                voice->osc_sync[sample] = -1.0f;
#endif /* master */
            }
            if (!state && pos >= pw) {
                out = -0.5f;
		blosc_place_slope_dd(index, pos - pw, w, voice->osc_bus_a, gain_a / pw,
                                                         voice->osc_bus_b, gain_b / pw);
                state = 1;
            }
	} else {  /* first half of waveform : descending saw */
            out = 0.5f - pos / pw;
            if (pos >= pw) {
                out = -0.5f;
		blosc_place_slope_dd(index, pos - pw, w, voice->osc_bus_a, gain_a / pw,
                                                         voice->osc_bus_b, gain_b / pw);
                state = 1;
            }
            if (pos >= 1.0f) {
                pos -= 1.0f;
#if BLOSC_MASTER
                voice->osc_sync[sample] = pos / w;
#endif /* master */
                out = 0.5f - pos / pw;
		blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a,
                                                   voice->osc_bus_b, gain_b);
		blosc_place_slope_dd(index, pos, w, voice->osc_bus_a, -gain_a / pw,
                                                    voice->osc_bus_b, -gain_b / pw);
                state = 0;
#if BLOSC_MASTER
            } else {
                voice->osc_sync[sample] = -1.0f;
#endif /* master */
            }
        }
        voice->osc_bus_a[(index + DD_SAMPLE_DELAY) & OSC_BUS_MASK] += gain_a * out;
        voice->osc_bus_b[(index + DD_SAMPLE_DELAY) & OSC_BUS_MASK] += gain_b * out;

        index++;

        w += w_delta;
        pw += pw_delta;
        gain_a += gain_a_delta;
        gain_b += gain_b_delta;
    }

    vosc->pos0 = (double)pos;
    vosc->i1 = state;
}

/* ---- blosc_* dispatch function ---- */

static void
BLOSC_THIS(, unsigned long sample_count, y_sosc_t *sosc,
           y_voice_t *voice, struct vosc *osc, int index, float w)
{
    if (osc->mode != osc->last_mode) {
        osc->last_mode = osc->mode;
        osc->last_waveform = -1;
    }

    switch (osc->waveform) {
      default:
      case 0:                                             /* up sawtooth wave */
      case 1:                                           /* down sawtooth wave */
        BLOSC_THIS(saw, sample_count, sosc, voice, osc, index, w);
        break;
      case 2:                         /* variable-duty-cycle rectangular wave */
        BLOSC_THIS(rect, sample_count, sosc, voice, osc, index, w);
        break;
      case 3:                                 /* variable-slope triangle wave */
        BLOSC_THIS(tri, sample_count, sosc, voice, osc, index, w);
        break;
      case 4:                   /* random-amplitude, var-d-c rectangular wave */
        BLOSC_THIS(noise, sample_count, sosc, voice, osc, index, w);
        break;
      case 5:          /* saw with variable-length flat segment between teeth */
        BLOSC_THIS(clipsaw, sample_count, sosc, voice, osc, index, w);
        break;
    }
}

#undef BLOSC_THIS

/* ==== wavetable oscillators ==== */

#if BLOSC_MASTER
static void
wt_osc_master(unsigned long sample_count, y_sosc_t *sosc, y_voice_t *voice,
              struct vosc *vosc, int index, float w0)
#else
static void
wt_osc_slave (unsigned long sample_count, y_sosc_t *sosc, y_voice_t *voice,
              struct vosc *vosc, int index, float w0)
#endif
{
    signed short *wave0, *wave1;
    unsigned long sample;
    float pos = (float)vosc->pos0,
          w, w_delta,
          wavemix0, wavemix1,
          gain_a, gain_a_delta,
          gain_b, gain_b_delta;
    float f;
    int   i;

    if (vosc->mode != vosc->last_mode)
        pos = 0.0f;
    i = voice->key + lrintf(*(sosc->pitch) + *(sosc->mparam2) * WAVETABLE_SELECT_BIAS_RANGE);
    if (vosc->mode     != vosc->last_mode ||
        vosc->waveform != vosc->last_waveform ||
        i              != vosc->wave_select_key) {

        /* select wave(s) and crossfade from wavetable */
        wavetable_select(vosc, i);
        vosc->last_mode     = vosc->mode;
        vosc->last_waveform = vosc->waveform;
    }

    /* -FIX- what if we didn't ramp pitch? */
    i = y_voice_mod_index(sosc->pitch_mod_src);
    f = *(sosc->pitch_mod_amt);
    w = 1.0f + f * voice->mod[i].value;
    w_delta = w + f * voice->mod[i].delta * (float)sample_count;
    w_delta *= w0;
    w *= w0;
    w_delta = (w_delta - w) / (float)sample_count;
    /* -FIX- condition to [0, 0.5)? */

    i = y_voice_mod_index(sosc->amp_mod_src);
    f = *(sosc->amp_mod_amt);
    if (f > 0.0f)
        gain_a = 1.0f - f + f * voice->mod[i].value;
    else
        gain_a = 1.0f + f * voice->mod[i].value;
    gain_a_delta = volume_cv_to_amplitude(gain_a + f * voice->mod[i].delta * (float)sample_count);
    gain_a       = volume_cv_to_amplitude(gain_a);
    gain_b       = gain_a       * *(sosc->level_b);
    gain_b_delta = gain_a_delta * *(sosc->level_b);
    gain_a       *= *(sosc->level_a);
    gain_a_delta *= *(sosc->level_a);
    gain_a_delta = (gain_a_delta - gain_a) / (float)sample_count;
    gain_b_delta = (gain_b_delta - gain_b) / (float)sample_count;
    /* -FIX- condition to [0, 1]? */

    wave0 = vosc->wave0;
    wave1 = vosc->wave1;
    wavemix0 = vosc->wavemix0;
    wavemix1 = vosc->wavemix1;

    /* -FIX- optimize for the case of no crossfade */

    for (sample = 0; sample < sample_count; sample++) {

        pos += w;

#if !BLOSC_MASTER
        if (voice->osc_sync[sample] >= 0.0f) { /* sync to master */

            float eof_offset = voice->osc_sync[sample] * w;
            float pos_at_reset = pos - eof_offset;
            float out, slope;
            pos = eof_offset;

            if (pos_at_reset >= 1.0f) {
                pos_at_reset -= 1.0f;
            }

            /* calculate amplitude change at reset point and place step DD */
            f = pos_at_reset * (float)WAVETABLE_POINTS;
            i = lrintf(f - 0.5f);
            f -= (float)i;
            f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
                ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
            out = f / -65534.0f;
            f = pos * (float)WAVETABLE_POINTS;
            i = lrintf(f - 0.5f);
            f -= (float)i;
            f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
                ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
            out += f / 65534.0f;
            blosc_place_step_dd(index, pos, w, voice->osc_bus_a, gain_a * out,
                                               voice->osc_bus_b, gain_b * out);

            /* if possible, calculate slope change at reset point and place slope DD */
            if (vosc->waveform == 0) {  /* sine wave */
                f = pos_at_reset * SINETABLE_POINTS;
                i = lrintf(f - 0.5f);
                f -= (float)i;
                i = (i + SINETABLE_POINTS / 4) & (SINETABLE_POINTS - 1);
                slope = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
                blosc_place_slope_dd(index, pos, w, voice->osc_bus_a, gain_a * M_2PI_F * (0.5f - slope),
                                                    voice->osc_bus_b, gain_b * M_2PI_F * (0.5f - slope));
            }
        } else
#endif /* slave */
        if (pos >= 1.0f) {
            pos -= 1.0f;
#if BLOSC_MASTER
            voice->osc_sync[sample] = pos / w;
        } else {
            voice->osc_sync[sample] = -1.0f;
#endif /* master */
        }

        f = pos * (float)WAVETABLE_POINTS;
        i = lrintf(f - 0.5f);
        f -= (float)i;

        f = ((float)wave0[i] + (float)(wave0[i + 1] - wave0[i]) * f) * wavemix0 +
            ((float)wave1[i] + (float)(wave1[i + 1] - wave1[i]) * f) * wavemix1;
        f /= 65534.0f;
        voice->osc_bus_a[(index + DD_SAMPLE_DELAY) & OSC_BUS_MASK] += gain_a * f;
        voice->osc_bus_b[(index + DD_SAMPLE_DELAY) & OSC_BUS_MASK] += gain_b * f;

        index++;

        w += w_delta;
        gain_a += gain_a_delta;
        gain_b += gain_b_delta;
    }

    vosc->pos0 = (double)pos;
}

