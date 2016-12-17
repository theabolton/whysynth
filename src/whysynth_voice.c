/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004-2007, 2010, 2016 Sean Bolton and others.
 *
 * Portions of this file may have come from Steve Brookes'
 * Xsynth, copyright (C) 1999 S. J. Brookes.
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
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

#include "whysynth_types.h"
#include "whysynth.h"
#include "dssp_event.h"
#include "whysynth_voice.h"

#include "wave_tables.h"
#include "whysynth_voice_inline.h"

/*
 * y_voice_new
 */
y_voice_t *
y_voice_new(y_synth_t *synth)
{
    y_voice_t *voice;

    voice = (y_voice_t *)calloc(sizeof(y_voice_t), 1);
    if (voice) {
        voice->status = Y_VOICE_OFF;
    }
    return voice;
}

float eg_shape_coeffs[][4] = {
    {   1,     0,    0, 0 },  /*  0 lead, x^3 */
    {   0,     1,    0, 0 },  /*  1 lead, x^2 */
    {   0,   0.5,  0.5, 0 },  /*  2 lead, x/2 + x^2/2 */
    {   0,     0,    1, 0 },  /*  3 linear */
    {   0,  -0.5,  1.5, 0 },  /*  4 lag,  1 - ((1-x)/2 + (1-x)^2/2), or 3x/2 - x^2/2 */
    {   0,    -1,    2, 0 },  /*  5 lag,  1 - (1 - x)^2, or 2x - x^2 */
    {   1,    -3,    3, 0 },  /*  6 lag,  1 - (1 - x)^3, or 3x - 3x^2 + x^3 */
    {   2,    -2,    1, 0 },  /*  7 "S" lead ('reverb' decay) */
    { 2.5, -3.75, 2.25, 0 },  /*  8 "S" mid */
    {   2,    -4,    3, 0 },  /*  9 "S" lag */
    {   0,     0,    0, 0 },  /* 10 lead, "jump", immediate to endpoint */
    {   0,     0,    0, 1 },  /* 11 lag,  "hold", hold startpoint */
};

/*
 * y_eg_setup
 *
 * need not be called only during control tick
 */
static inline void
y_eg_setup(y_synth_t *synth, y_seg_t *seg, y_voice_t *voice, struct veg *veg,
           float start, struct vmod *destmod)
{
    int mode = lrintf(*(seg->mode)),
        time, i;
    float f, inv_duration, level, mult0, mult1;

    if (mode == 0) {  /* Off */
        veg->state = DSSP_EG_FINISHED;
        destmod->value = 0.0f;
        destmod->next_value = 0.0f;
        destmod->delta = 0.0f;
        return;
    }

    veg->shape[0] = lrintf(*(seg->shape[0]));
    veg->shape[1] = lrintf(*(seg->shape[1]));
    veg->shape[2] = lrintf(*(seg->shape[2]));
    veg->shape[3] = lrintf(*(seg->shape[3]));
    if (veg->shape[0] < 0 || veg->shape[0] > 11) veg->shape[0] = 0;
    if (veg->shape[1] < 0 || veg->shape[1] > 11) veg->shape[1] = 0;
    if (veg->shape[2] < 0 || veg->shape[2] > 11) veg->shape[2] = 0;
    if (veg->shape[3] < 0 || veg->shape[3] > 11) veg->shape[3] = 0;

    /* calculate segment lengths, adjusted for 'velocity time scaling' and 'keyboard time scaling' */
    if (fabs(*(seg->kbd_time_scale)) < 1e-5 &&
        fabs(*(seg->vel_time_scale)) < 1e-5) {
        f = 1.0f;
    } else {
        f = 69.0f - *(seg->kbd_time_scale) * (float)(voice->key - 60) -
                    *(seg->vel_time_scale) * (float)(voice->velocity - 72);
        if (f < 33.0f) f = 33.0f;         /* 0.125 to ... */
        else if (f > 105.0f) f = 105.0f;  /* ... 8 oughta be enough range */
        f = pitch_to_frequency(f);
    }
    veg->time_scale = f * synth->control_rate;
    time = lrintf(*(seg->time[0]) * veg->time_scale);
    if (time < 1) time = 1;

    /* calculate segment endpoint levels, adjusted for velocity
     * curve at vel_level_sens=0.5 modeled after my TX-7 (gnuplot makes me wet) */
    /* -FIX- this better feel really damn good, or it needs to be replaced with something faster */
    if (voice->velocity == 127)
        f = 1.0f;
    else {
        float f0, s = *(seg->vel_level_sens) * 2.0f;

        f = (float)voice->velocity / 127.0f;
        f0 = (((-3.0f * f + 7.4f) * f - 6.8f) * f + 3.4) * f;
        if (s < 1.0f)
            f = 1.0f - s + s * f0;
        else
            f = f0 * (2.0f - s) + f * f * (s - 1.0f);
    }
    veg->level_scale = f;

    level = *(seg->level[0]) * f;

    if (mode == 1) { /* simple ADSR */
        level = f;
        veg->shape[1] = 3;  /* linear */
        veg->sustain_segment = 2;
    } else
        veg->sustain_segment = 4 - mode;

    veg->state = DSSP_EG_RUNNING;
    veg->segment = 0;

    if (synth->control_remains != Y_CONTROL_PERIOD) {
        veg->count = time;
        inv_duration = 1.0f / ((float)time + 
                                   (float)(Y_CONTROL_PERIOD - synth->control_remains) /
                                       (float)Y_CONTROL_PERIOD);
    } else {
        veg->count = time - 1;
        inv_duration = 1.0f / (float)time;
    }

    i = veg->shape[0];
    f = start - level;  /* segment delta * -1 */

    veg->target = level;
    veg->d = eg_shape_coeffs[i][3] * f + level;
    f *= inv_duration;
    veg->c = eg_shape_coeffs[i][2] * f;
    f *= inv_duration;
    veg->b = eg_shape_coeffs[i][1] * f;
    f *= inv_duration;
    veg->a = eg_shape_coeffs[i][0] * f;

    i = y_voice_mod_index(seg->amp_mod_src);
    mult1 = *(seg->amp_mod_amt);
    if (mult1 > 0.0f) {
        mult0 = 1.0f - mult1 + mult1 * voice->mod[i].value;
        mult1 = 1.0f - mult1 + mult1 * voice->mod[i].next_value;
    } else {
        mult0 = 1.0f + mult1 * voice->mod[i].value;
        mult1 = 1.0f + mult1 * voice->mod[i].next_value;
    }

    destmod->value = start * mult0;
    f = (float)veg->count;
    destmod->next_value = (((veg->a * f + veg->b) * f + veg->c) * f + veg->d) * mult1;
    destmod->delta = (destmod->next_value - destmod->value) / (float)synth->control_remains;
}

/*
 * y_eg_start
 */
static inline void
y_eg_start(y_synth_t *synth, y_seg_t *seg, y_voice_t *voice, struct veg *veg,
           struct vmod *mod)
{
    y_eg_setup(synth, seg, voice, veg, 0.0f, mod);
}

/*
 * y_eg_restart
 */
static inline void
y_eg_restart(y_synth_t *synth, y_seg_t *seg, y_voice_t *voice, struct veg *veg,
             struct vmod *mod)
{
    float f = (float)veg->count +
                  (float)(Y_CONTROL_PERIOD - synth->control_remains) /
                      (float)Y_CONTROL_PERIOD;

    f = ((veg->a * f + veg->b) * f + veg->c) * f + veg->d; /* current envelope value before modulation */

    y_eg_setup(synth, seg, voice, veg, f, mod);
}

/*
 * y_voice_restart_egs
 */
static void
y_voice_restart_egs(y_synth_t *synth, y_voice_t *voice)
{
    y_eg_restart(synth, &synth->ego, voice, &voice->ego, &voice->mod[Y_MOD_EGO]);
    y_eg_restart(synth, &synth->eg1, voice, &voice->eg1, &voice->mod[Y_MOD_EG1]);
    y_eg_restart(synth, &synth->eg2, voice, &voice->eg2, &voice->mod[Y_MOD_EG2]);
    y_eg_restart(synth, &synth->eg3, voice, &voice->eg3, &voice->mod[Y_MOD_EG3]);
    y_eg_restart(synth, &synth->eg4, voice, &voice->eg4, &voice->mod[Y_MOD_EG4]);
}

/*
 * y_eg_release
 *
 * need not be called only during control tick
 */
static inline void
y_eg_release(y_synth_t *synth, y_seg_t *seg, y_voice_t *voice, struct veg *veg,
             struct vmod *destmod)
{
    int mode = lrintf(*(seg->mode)),
        time, i;
    float f, inv_duration, level, mult;

    if (veg->state == DSSP_EG_FINISHED || veg->sustain_segment < 0) /* finished, 'Off', or 'One-Shot' */
        return;

    veg->state = DSSP_EG_RUNNING;
    veg->segment = veg->sustain_segment + 1;

    if (veg->segment == 1 && mode == 1) {  /* second segment of simple ADSR */
        time = 1;
        level = veg->level_scale;
    } else {
        time = lrintf(*(seg->time[veg->segment]) * veg->time_scale);
        if (time < 1) time = 1;
        level = *(seg->level[veg->segment]) * veg->level_scale;
    }

    if (synth->control_remains != Y_CONTROL_PERIOD) {
        f = (float)(Y_CONTROL_PERIOD - synth->control_remains) / (float)Y_CONTROL_PERIOD;
        inv_duration = 1.0f / ((float)time + f);
        f += (float)veg->count;
        veg->count = time;
    } else {
        f = (float)veg->count;
        inv_duration = 1.0f / (float)time;
        veg->count = time - 1;
    }

    f = ((veg->a * f + veg->b) * f + veg->c) * f + veg->d; /* current envelope value before modulation */
    f = f - level;                                         /* segment delta * -1 */
    i = veg->shape[veg->segment];

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
        mult = 1.0f - mult + mult * voice->mod[i].value;
    } else {
        mult = 1.0f + mult * voice->mod[i].value;
    }

    f = (float)veg->count;
    destmod->next_value = (((veg->a * f + veg->b) * f + veg->c) * f + veg->d) * mult;
    destmod->delta = (destmod->next_value - destmod->value) / (float)synth->control_remains;

    // YDB_MESSAGE(YDB_NOTE, " set_release_segment: eg %p, duration %f, count %d, value %f, delta %f\n", eg, duration, eg->count, mod->value, mod->delta);
}

/*
 * y_voice_release_egs
 */
static inline void
y_voice_release_egs(y_synth_t *synth, y_voice_t *voice)
{
    y_eg_release(synth, &synth->ego, voice, &voice->ego, &voice->mod[Y_MOD_EGO]);
    y_eg_release(synth, &synth->eg1, voice, &voice->eg1, &voice->mod[Y_MOD_EG1]);
    y_eg_release(synth, &synth->eg2, voice, &voice->eg2, &voice->mod[Y_MOD_EG2]);
    y_eg_release(synth, &synth->eg3, voice, &voice->eg3, &voice->mod[Y_MOD_EG3]);
    y_eg_release(synth, &synth->eg4, voice, &voice->eg4, &voice->mod[Y_MOD_EG4]);
}

/*
 * y_voice_note_on
 */
void
y_voice_note_on(y_synth_t *synth, y_voice_t *voice,
                     unsigned char key, unsigned char velocity)
{
    int i;

    voice->key      = key;
    voice->velocity = velocity;

    if (!synth->monophonic || !(_ON(voice) || _SUSTAINED(voice))) {

        /* brand-new voice, or monophonic voice in release phase; set
         * everything up */
        // YDB_MESSAGE(YDB_NOTE, " y_voice_note_on in polyphonic/new section: key %d, mono %d, old status %d\n", key, synth->monophonic, voice->status);

        voice->target_pitch = y_pitch[key];
        switch(synth->glide) {
          case Y_GLIDE_MODE_LEGATO:
            if (synth->held_keys[0] >= 0) {
                voice->prev_pitch = y_pitch[synth->held_keys[0]];
            } else {
                voice->prev_pitch = voice->target_pitch;
            }
            break;

          case Y_GLIDE_MODE_INITIAL:
            if (synth->held_keys[0] >= 0) {
                voice->prev_pitch = voice->target_pitch;
            } else {
                voice->prev_pitch = synth->last_noteon_pitch;
            }
            break;

          case Y_GLIDE_MODE_ALWAYS:
            if (synth->held_keys[0] >= 0) {
                voice->prev_pitch = y_pitch[synth->held_keys[0]];
            } else {
                voice->prev_pitch = synth->last_noteon_pitch;
            }
            break;

          case Y_GLIDE_MODE_LEFTOVER:
            /* leave voice->prev_pitch at whatever it was */
            break;

          default:
          case Y_GLIDE_MODE_OFF:
            voice->prev_pitch = voice->target_pitch;
            break;
        }
        if (!_PLAYING(voice)) { /* new voice */
            voice->osc1.last_mode = -1;
            voice->osc2.last_mode = -1;
            voice->osc3.last_mode = -1;
            voice->osc4.last_mode = -1;
            voice->vcf1.last_mode = -1;
            voice->vcf2.last_mode = -1;
            /* modulators */
            voice->mod[Y_MOD_ONE].value = 1.0f;
            voice->mod[Y_MOD_ONE].next_value = 1.0f;
            voice->mod[Y_MOD_ONE].delta = 0.0f;
            for (i = 1; i < Y_MODS_COUNT; i++) { /* prevent forward-referencing mods from reading garbage */
                voice->mod[i].value = 0.0f;
                voice->mod[i].next_value = 0.0f;
                voice->mod[i].delta = 0.0f;
            }
            /* Y_MOD_MODWHEEL set in y_voice_render() */
            /* Y_MOD_PRESSURE set in y_voice_render() */
            voice->mod[Y_MOD_KEY].value = (float)(key - 60) / 60.0f; /* -FIX- why should KEY be bipolar while VELOCITY is unipolar? */
            voice->mod[Y_MOD_KEY].next_value = voice->mod[Y_MOD_KEY].value;
            voice->mod[Y_MOD_KEY].delta = 0.0f;
            voice->mod[Y_MOD_VELOCITY].value = (float)velocity / 127.0f;
            voice->mod[Y_MOD_VELOCITY].next_value = voice->mod[Y_MOD_VELOCITY].value;
            voice->mod[Y_MOD_VELOCITY].delta = 0.0f;
            /* Y_MOD_GLFO set in y_voice_render() */
            y_voice_setup_lfo(synth, &synth->vlfo, &voice->vlfo, 0.0f, 0.0f,
                              voice->mod, &voice->mod[Y_MOD_VLFO]);
            y_voice_setup_lfo(synth, &synth->mlfo, &voice->mlfo0,
                              0.0f, *(synth->mlfo_random_freq),
                              voice->mod, &voice->mod[Y_MOD_MLFO0]);
            y_voice_setup_lfo(synth, &synth->mlfo, &voice->mlfo1,
                              *(synth->mlfo_phase_spread) / 360.0f,
                              *(synth->mlfo_random_freq),
                              voice->mod, &voice->mod[Y_MOD_MLFO1]);
            y_voice_setup_lfo(synth, &synth->mlfo, &voice->mlfo2,
                              2.0f * *(synth->mlfo_phase_spread) / 360.0f,
                              *(synth->mlfo_random_freq),
                              voice->mod, &voice->mod[Y_MOD_MLFO2]);
            y_voice_setup_lfo(synth, &synth->mlfo, &voice->mlfo3,
                              3.0f * *(synth->mlfo_phase_spread) / 360.0f,
                              *(synth->mlfo_random_freq),
                              voice->mod, &voice->mod[Y_MOD_MLFO3]);
            y_eg_start(synth, &synth->ego, voice, &voice->ego, &voice->mod[Y_MOD_EGO]);
            y_eg_start(synth, &synth->eg1, voice, &voice->eg1, &voice->mod[Y_MOD_EG1]);
            y_eg_start(synth, &synth->eg2, voice, &voice->eg2, &voice->mod[Y_MOD_EG2]);
            y_eg_start(synth, &synth->eg3, voice, &voice->eg3, &voice->mod[Y_MOD_EG3]);
            y_eg_start(synth, &synth->eg4, voice, &voice->eg4, &voice->mod[Y_MOD_EG4]);
            /* Y_MOD_MIX set in y_voice_render() */
            voice->osc_index = Y_CONTROL_PERIOD - synth->control_remains;

        } else { /* monophonic voice in release phase, retrigger EGs */

            y_voice_restart_egs(synth, voice);
        }

        y_voice_update_pressure_mod(synth, voice);

    } else {

        /* synth is monophonic, and we're modifying a playing voice */
        // YDB_MESSAGE(YDB_NOTE, " y_voice_note_on in monophonic section: old key %d => new key %d\n", synth->held_keys[0], key);

        /* set new pitch */
        voice->target_pitch = y_pitch[key];
        if (synth->glide == Y_GLIDE_MODE_INITIAL ||
            synth->glide == Y_GLIDE_MODE_OFF)
            voice->prev_pitch = voice->target_pitch;

        /* if in 'on' or 'both' modes, and key has changed, then re-trigger EGs */
        if ((synth->monophonic == Y_MONO_MODE_ON ||
             synth->monophonic == Y_MONO_MODE_BOTH) &&
            (synth->held_keys[0] < 0 || synth->held_keys[0] != key)) {

            y_voice_restart_egs(synth, voice);
        }

        /* all other variables stay what they are */

    }
    synth->last_noteon_pitch = voice->target_pitch;

    /* add new key to the list of held keys */

    /* check if new key is already in the list; if so, move it to the
     * top of the list, otherwise shift the other keys down and add it
     * to the top of the list. */
    for (i = 0; i < 7; i++) {
        if (synth->held_keys[i] == key)
            break;
    }
    for (; i > 0; i--) {
        synth->held_keys[i] = synth->held_keys[i - 1];
    }
    synth->held_keys[0] = key;

    if (!_PLAYING(voice)) {

        y_voice_start_voice(voice);

    } else if (!_ON(voice)) {  /* must be Y_VOICE_SUSTAINED or Y_VOICE_RELEASED */

        voice->status = Y_VOICE_ON;

    }
}

/*
 * y_voice_note_off
 */
void
y_voice_note_off(y_synth_t *synth, y_voice_t *voice,
                      unsigned char key, unsigned char rvelocity)
{
    // YDB_MESSAGE(YDB_NOTE, " y_voice_note_off: called for voice %p, key %d\n", voice, key);

    /* save release velocity */
    voice->rvelocity = rvelocity;

    if (synth->monophonic) {  /* monophonic mode */

        if (synth->held_keys[0] >= 0) {

            /* still some keys held */

            if (voice->key != synth->held_keys[0]) {

                /* most-recently-played key has changed */
                voice->key = synth->held_keys[0];
                // YDB_MESSAGE(YDB_NOTE, " note-off in monophonic section: changing pitch to %d\n", voice->key);
                voice->target_pitch = y_pitch[voice->key];
                if (synth->glide == Y_GLIDE_MODE_INITIAL ||
                    synth->glide == Y_GLIDE_MODE_OFF)
                    voice->prev_pitch = voice->target_pitch;

                /* if mono mode is 'both', re-trigger EGs */
                if (synth->monophonic == Y_MONO_MODE_BOTH && !_RELEASED(voice)) {
                    y_voice_restart_egs(synth, voice);
                }

            }

        } else {  /* no keys still held */

            if (Y_SYNTH_SUSTAINED(synth)) {

                /* no more keys in list, but we're sustained */
                // YDB_MESSAGE(YDB_NOTE, " note-off in monophonic section: sustained with no held keys\n");
                if (!_RELEASED(voice))
                    voice->status = Y_VOICE_SUSTAINED;

            } else {  /* not sustained */

                /* no more keys in list, so turn off note */
                // YDB_MESSAGE(YDB_NOTE, " note-off in monophonic section: turning off voice %p\n", voice);
                y_voice_release_egs(synth, voice);
                voice->status = Y_VOICE_RELEASED;

            }
        }

    } else {  /* polyphonic mode */

        if (Y_SYNTH_SUSTAINED(synth)) {

            if (!_RELEASED(voice))
                voice->status = Y_VOICE_SUSTAINED;

        } else {  /* not sustained */

            y_voice_release_egs(synth, voice);
            voice->status = Y_VOICE_RELEASED;

        }
    }
}

/*
 * y_voice_release_note
 */
void
y_voice_release_note(y_synth_t *synth, y_voice_t *voice)
{
    // YDB_MESSAGE(YDB_NOTE, " y_voice_release_note: turning off voice %p\n", voice);
    if (_ON(voice)) {
        /* dummy up a release velocity */
        voice->rvelocity = 64;
    }
    y_voice_release_egs(synth, voice);
    voice->status = Y_VOICE_RELEASED;
}

/*
 * y_voice_set_ports
 */
void
y_voice_set_ports(y_synth_t *synth, y_patch_t *patch)
{
    /* -PORTS- */
    *(synth->osc1.mode)          = (float)patch->osc1.mode;
    *(synth->osc1.waveform)      = (float)patch->osc1.waveform;
    *(synth->osc1.pitch)         = (float)patch->osc1.pitch;
    *(synth->osc1.detune)        = patch->osc1.detune;
    *(synth->osc1.pitch_mod_src) = (float)patch->osc1.pitch_mod_src;
    *(synth->osc1.pitch_mod_amt) = patch->osc1.pitch_mod_amt;
    *(synth->osc1.mparam1)       = patch->osc1.mparam1;
    *(synth->osc1.mparam2)       = patch->osc1.mparam2;
    *(synth->osc1.mmod_src)      = (float)patch->osc1.mmod_src;
    *(synth->osc1.mmod_amt)      = patch->osc1.mmod_amt;
    *(synth->osc1.amp_mod_src)   = (float)patch->osc1.amp_mod_src;
    *(synth->osc1.amp_mod_amt)   = patch->osc1.amp_mod_amt;
    *(synth->osc1.level_a)       = patch->osc1.level_a;
    *(synth->osc1.level_b)       = patch->osc1.level_b;

    *(synth->osc2.mode)          = (float)patch->osc2.mode;
    *(synth->osc2.waveform)      = (float)patch->osc2.waveform;
    *(synth->osc2.pitch)         = (float)patch->osc2.pitch;
    *(synth->osc2.detune)        = patch->osc2.detune;
    *(synth->osc2.pitch_mod_src) = (float)patch->osc2.pitch_mod_src;
    *(synth->osc2.pitch_mod_amt) = patch->osc2.pitch_mod_amt;
    *(synth->osc2.mparam1)       = patch->osc2.mparam1;
    *(synth->osc2.mparam2)       = patch->osc2.mparam2;
    *(synth->osc2.mmod_src)      = (float)patch->osc2.mmod_src;
    *(synth->osc2.mmod_amt)      = patch->osc2.mmod_amt;
    *(synth->osc2.amp_mod_src)   = (float)patch->osc2.amp_mod_src;
    *(synth->osc2.amp_mod_amt)   = patch->osc2.amp_mod_amt;
    *(synth->osc2.level_a)       = patch->osc2.level_a;
    *(synth->osc2.level_b)       = patch->osc2.level_b;

    *(synth->osc3.mode)          = (float)patch->osc3.mode;
    *(synth->osc3.waveform)      = (float)patch->osc3.waveform;
    *(synth->osc3.pitch)         = (float)patch->osc3.pitch;
    *(synth->osc3.detune)        = patch->osc3.detune;
    *(synth->osc3.pitch_mod_src) = (float)patch->osc3.pitch_mod_src;
    *(synth->osc3.pitch_mod_amt) = patch->osc3.pitch_mod_amt;
    *(synth->osc3.mparam1)       = patch->osc3.mparam1;
    *(synth->osc3.mparam2)       = patch->osc3.mparam2;
    *(synth->osc3.mmod_src)      = (float)patch->osc3.mmod_src;
    *(synth->osc3.mmod_amt)      = patch->osc3.mmod_amt;
    *(synth->osc3.amp_mod_src)   = (float)patch->osc3.amp_mod_src;
    *(synth->osc3.amp_mod_amt)   = patch->osc3.amp_mod_amt;
    *(synth->osc3.level_a)       = patch->osc3.level_a;
    *(synth->osc3.level_b)       = patch->osc3.level_b;

    *(synth->osc4.mode)          = (float)patch->osc4.mode;
    *(synth->osc4.waveform)      = (float)patch->osc4.waveform;
    *(synth->osc4.pitch)         = (float)patch->osc4.pitch;
    *(synth->osc4.detune)        = patch->osc4.detune;
    *(synth->osc4.pitch_mod_src) = (float)patch->osc4.pitch_mod_src;
    *(synth->osc4.pitch_mod_amt) = patch->osc4.pitch_mod_amt;
    *(synth->osc4.mparam1)       = patch->osc4.mparam1;
    *(synth->osc4.mparam2)       = patch->osc4.mparam2;
    *(synth->osc4.mmod_src)      = (float)patch->osc4.mmod_src;
    *(synth->osc4.mmod_amt)      = patch->osc4.mmod_amt;
    *(synth->osc4.amp_mod_src)   = (float)patch->osc4.amp_mod_src;
    *(synth->osc4.amp_mod_amt)   = patch->osc4.amp_mod_amt;
    *(synth->osc4.level_a)       = patch->osc4.level_a;
    *(synth->osc4.level_b)       = patch->osc4.level_b;

    *(synth->vcf1.mode)         = (float)patch->vcf1.mode;
    *(synth->vcf1.source)       = (float)patch->vcf1.source;
    *(synth->vcf1.frequency)    = patch->vcf1.frequency;
    *(synth->vcf1.freq_mod_src) = (float)patch->vcf1.freq_mod_src;
    *(synth->vcf1.freq_mod_amt) = patch->vcf1.freq_mod_amt;
    *(synth->vcf1.qres)         = patch->vcf1.qres;
    *(synth->vcf1.mparam)       = patch->vcf1.mparam;

    *(synth->vcf2.mode)         = (float)patch->vcf2.mode;
    *(synth->vcf2.source)       = (float)patch->vcf2.source;
    *(synth->vcf2.frequency)    = patch->vcf2.frequency;
    *(synth->vcf2.freq_mod_src) = (float)patch->vcf2.freq_mod_src;
    *(synth->vcf2.freq_mod_amt) = patch->vcf2.freq_mod_amt;
    *(synth->vcf2.qres)         = patch->vcf2.qres;
    *(synth->vcf2.mparam)       = patch->vcf2.mparam;

    *(synth->busa_level)        = patch->busa_level;
    *(synth->busa_pan)          = patch->busa_pan;
    *(synth->busb_level)        = patch->busb_level;
    *(synth->busb_pan)          = patch->busb_pan;
    *(synth->vcf1_level)        = patch->vcf1_level;
    *(synth->vcf1_pan)          = patch->vcf1_pan;
    *(synth->vcf2_level)        = patch->vcf2_level;
    *(synth->vcf2_pan)          = patch->vcf2_pan;
    *(synth->volume)            = patch->volume;

    *(synth->effect_mode)       = (float)patch->effect_mode;
    *(synth->effect_param1)     = patch->effect_param1;
    *(synth->effect_param2)     = patch->effect_param2;
    *(synth->effect_param3)     = patch->effect_param3;
    *(synth->effect_param4)     = patch->effect_param4;
    *(synth->effect_param5)     = patch->effect_param5;
    *(synth->effect_param6)     = patch->effect_param6;
    *(synth->effect_mix)        = patch->effect_mix;
                                                           
    *(synth->glide_time)        = patch->glide_time;
    *(synth->bend_range)        = (float)patch->bend_range;
                                                           
    *(synth->glfo.frequency)    = patch->glfo.frequency;
    *(synth->glfo.waveform)     = (float)patch->glfo.waveform;
    /* synth->glfo.delay always points to a 0.0f */
    *(synth->glfo.amp_mod_src)  = (float)patch->glfo.amp_mod_src;
    *(synth->glfo.amp_mod_amt)  = patch->glfo.amp_mod_amt;
                                                           
    *(synth->vlfo.frequency)    = patch->vlfo.frequency;
    *(synth->vlfo.waveform)     = (float)patch->vlfo.waveform;
    *(synth->vlfo.delay)        = patch->vlfo.delay;
    *(synth->vlfo.amp_mod_src)  = (float)patch->vlfo.amp_mod_src;
    *(synth->vlfo.amp_mod_amt)  = patch->vlfo.amp_mod_amt;
                                                           
    *(synth->mlfo.frequency)    = patch->mlfo.frequency;
    *(synth->mlfo.waveform)     = (float)patch->mlfo.waveform;
    *(synth->mlfo.delay)        = patch->mlfo.delay;
    *(synth->mlfo.amp_mod_src)  = (float)patch->mlfo.amp_mod_src;
    *(synth->mlfo.amp_mod_amt)  = patch->mlfo.amp_mod_amt;
    *(synth->mlfo_phase_spread) = patch->mlfo_phase_spread;
    *(synth->mlfo_random_freq)  = patch->mlfo_random_freq;

    *(synth->ego.mode)           = (float)patch->ego.mode;
    *(synth->ego.shape[0])       = (float)patch->ego.shape1;
    *(synth->ego.time[0])        = patch->ego.time1;
    *(synth->ego.level[0])       = patch->ego.level1;
    *(synth->ego.shape[1])       = (float)patch->ego.shape2;
    *(synth->ego.time[1])        = patch->ego.time2;
    *(synth->ego.level[1])       = patch->ego.level2;
    *(synth->ego.shape[2])       = (float)patch->ego.shape3;
    *(synth->ego.time[2])        = patch->ego.time3;
    *(synth->ego.level[2])       = patch->ego.level3;
    *(synth->ego.shape[3])       = (float)patch->ego.shape4;
    *(synth->ego.time[3])        = patch->ego.time4;
    *(synth->ego.vel_level_sens) = patch->ego.vel_level_sens;
    *(synth->ego.vel_time_scale) = patch->ego.vel_time_scale;
    *(synth->ego.kbd_time_scale) = patch->ego.kbd_time_scale;
    *(synth->ego.amp_mod_src)    = (float)patch->ego.amp_mod_src;
    *(synth->ego.amp_mod_amt)    = patch->ego.amp_mod_amt;

    *(synth->eg1.mode)           = (float)patch->eg1.mode;
    *(synth->eg1.shape[0])       = (float)patch->eg1.shape1;
    *(synth->eg1.time[0])        = patch->eg1.time1;
    *(synth->eg1.level[0])       = patch->eg1.level1;
    *(synth->eg1.shape[1])       = (float)patch->eg1.shape2;
    *(synth->eg1.time[1])        = patch->eg1.time2;
    *(synth->eg1.level[1])       = patch->eg1.level2;
    *(synth->eg1.shape[2])       = (float)patch->eg1.shape3;
    *(synth->eg1.time[2])        = patch->eg1.time3;
    *(synth->eg1.level[2])       = patch->eg1.level3;
    *(synth->eg1.shape[3])       = (float)patch->eg1.shape4;
    *(synth->eg1.time[3])        = patch->eg1.time4;
    *(synth->eg1.vel_level_sens) = patch->eg1.vel_level_sens;
    *(synth->eg1.vel_time_scale) = patch->eg1.vel_time_scale;
    *(synth->eg1.kbd_time_scale) = patch->eg1.kbd_time_scale;
    *(synth->eg1.amp_mod_src)    = (float)patch->eg1.amp_mod_src;
    *(synth->eg1.amp_mod_amt)    = patch->eg1.amp_mod_amt;

    *(synth->eg2.mode)           = (float)patch->eg2.mode;
    *(synth->eg2.shape[0])       = (float)patch->eg2.shape1;
    *(synth->eg2.time[0])        = patch->eg2.time1;
    *(synth->eg2.level[0])       = patch->eg2.level1;
    *(synth->eg2.shape[1])       = (float)patch->eg2.shape2;
    *(synth->eg2.time[1])        = patch->eg2.time2;
    *(synth->eg2.level[1])       = patch->eg2.level2;
    *(synth->eg2.shape[2])       = (float)patch->eg2.shape3;
    *(synth->eg2.time[2])        = patch->eg2.time3;
    *(synth->eg2.level[2])       = patch->eg2.level3;
    *(synth->eg2.shape[3])       = (float)patch->eg2.shape4;
    *(synth->eg2.time[3])        = patch->eg2.time4;
    *(synth->eg2.vel_level_sens) = patch->eg2.vel_level_sens;
    *(synth->eg2.vel_time_scale) = patch->eg2.vel_time_scale;
    *(synth->eg2.kbd_time_scale) = patch->eg2.kbd_time_scale;
    *(synth->eg2.amp_mod_src)    = (float)patch->eg2.amp_mod_src;
    *(synth->eg2.amp_mod_amt)    = patch->eg2.amp_mod_amt;

    *(synth->eg3.mode)           = (float)patch->eg3.mode;
    *(synth->eg3.shape[0])       = (float)patch->eg3.shape1;
    *(synth->eg3.time[0])        = patch->eg3.time1;
    *(synth->eg3.level[0])       = patch->eg3.level1;
    *(synth->eg3.shape[1])       = (float)patch->eg3.shape2;
    *(synth->eg3.time[1])        = patch->eg3.time2;
    *(synth->eg3.level[1])       = patch->eg3.level2;
    *(synth->eg3.shape[2])       = (float)patch->eg3.shape3;
    *(synth->eg3.time[2])        = patch->eg3.time3;
    *(synth->eg3.level[2])       = patch->eg3.level3;
    *(synth->eg3.shape[3])       = (float)patch->eg3.shape4;
    *(synth->eg3.time[3])        = patch->eg3.time4;
    *(synth->eg3.vel_level_sens) = patch->eg3.vel_level_sens;
    *(synth->eg3.vel_time_scale) = patch->eg3.vel_time_scale;
    *(synth->eg3.kbd_time_scale) = patch->eg3.kbd_time_scale;
    *(synth->eg3.amp_mod_src)    = (float)patch->eg3.amp_mod_src;
    *(synth->eg3.amp_mod_amt)    = patch->eg3.amp_mod_amt;

    *(synth->eg4.mode)           = (float)patch->eg4.mode;
    *(synth->eg4.shape[0])       = (float)patch->eg4.shape1;
    *(synth->eg4.time[0])        = patch->eg4.time1;
    *(synth->eg4.level[0])       = patch->eg4.level1;
    *(synth->eg4.shape[1])       = (float)patch->eg4.shape2;
    *(synth->eg4.time[1])        = patch->eg4.time2;
    *(synth->eg4.level[1])       = patch->eg4.level2;
    *(synth->eg4.shape[2])       = (float)patch->eg4.shape3;
    *(synth->eg4.time[2])        = patch->eg4.time3;
    *(synth->eg4.level[2])       = patch->eg4.level3;
    *(synth->eg4.shape[3])       = (float)patch->eg4.shape4;
    *(synth->eg4.time[3])        = patch->eg4.time4;
    *(synth->eg4.vel_level_sens) = patch->eg4.vel_level_sens;
    *(synth->eg4.vel_time_scale) = patch->eg4.vel_time_scale;
    *(synth->eg4.kbd_time_scale) = patch->eg4.kbd_time_scale;
    *(synth->eg4.amp_mod_src)    = (float)patch->eg4.amp_mod_src;
    *(synth->eg4.amp_mod_amt)    = patch->eg4.amp_mod_amt;

    *(synth->modmix_bias)        = patch->modmix_bias;
    *(synth->modmix_mod1_src)    = (float)patch->modmix_mod1_src;
    *(synth->modmix_mod1_amt)    = patch->modmix_mod1_amt;
    *(synth->modmix_mod2_src)    = (float)patch->modmix_mod2_src;
    *(synth->modmix_mod2_amt)    = patch->modmix_mod2_amt;
}

/*
 * y_voice_update_pressure_mod
 */
void
y_voice_update_pressure_mod(y_synth_t *synth, y_voice_t *voice)
{
    unsigned char kp = synth->key_pressure[voice->key];
    unsigned char cp = synth->channel_pressure;
    float p;

    /* add the channel and key pressures together in a way that 'feels' good */
    if (kp > cp) {
        p = (float)kp / 127.0f;
        p += (1.0f - p) * ((float)cp / 127.0f);
    } else {
        p = (float)cp / 127.0f;
        p += (1.0f - p) * ((float)kp / 127.0f);
    }
    voice->pressure = p;
    voice->mod[Y_MOD_PRESSURE].next_value = p;
}

