/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004-2010 Sean Bolton and others.
 *
 * Portions of this file may have come from Steve Brookes'
 * Xsynth, copyright (C) 1999 S. J. Brookes.
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
 * Portions of this file may have come from Chris Cannam and Steve
 * Harris's public domain DSSI example code.
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

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#include <ladspa.h>

#include "whysynth.h"
#include "dssp_synth.h"
#include "dssp_event.h"
#include "whysynth_voice.h"
#include "common_data.h"
#include "sampleset.h"
#include "effects.h"

/*
 * y_synth_clear_held_keys
 */
static inline void
y_synth_clear_held_keys(y_synth_t *synth)
{
    int i;

    for (i = 0; i < 8; i++) synth->held_keys[i] = -1;
}

/*
 * y_voice_remove_held_key
 */
static inline void
y_voice_remove_held_key(y_synth_t *synth, unsigned char key)
{
    int i;

    /* check if this key is in list of held keys; if so, remove it and
     * shift the other keys up */
    for (i = 7; i >= 0; i--) {
        if (synth->held_keys[i] == key)
            break;
    }
    if (i >= 0) {
        for (; i < 7; i++) {
            synth->held_keys[i] = synth->held_keys[i + 1];
        }
        synth->held_keys[7] = -1;
    }
}

/*
 * y_synth_all_voices_off
 *
 * stop processing all notes immediately
 */
void
y_synth_all_voices_off(y_synth_t *synth)
{
    int i;
    y_voice_t *voice;

    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (_PLAYING(voice)) {
            y_voice_off(synth, voice);
        }
    }
    y_synth_clear_held_keys(synth);
}

/*
 * y_synth_note_off
 *
 * handle a note off message
 */
void
y_synth_note_off(y_synth_t *synth, unsigned char key, unsigned char rvelocity)
{
    int i;
    y_voice_t *voice;

    y_voice_remove_held_key(synth, key);

    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (synth->monophonic ? (_PLAYING(voice)) :
                                (_ON(voice) && (voice->key == key))) {
            YDB_MESSAGE(YDB_NOTE, " y_synth_note_off: key %d rvel %d voice %d note id %d\n", key, rvelocity, i, voice->note_id);
            y_voice_note_off(synth, voice, key, rvelocity);
        }
    }
}

/*
 * y_synth_all_notes_off
 *
 * put all notes into the released state
 */
void
y_synth_all_notes_off(y_synth_t* synth)
{
    int i;
    y_voice_t *voice;

    /* reset the sustain controller */
    synth->cc[MIDI_CTL_SUSTAIN] = 0;
    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (_ON(voice) || _SUSTAINED(voice)) {
            y_voice_release_note(synth, voice);
        }
    }
}

/*
 * y_synth_free_voice_by_kill
 *
 * selects a voice for killing. the selection algorithm is a refinement
 * of the algorithm previously in fluid_synth_alloc_voice.
 */
static y_voice_t*
y_synth_free_voice_by_kill(y_synth_t *synth)
{
    int i;
    int best_prio = 10001;
    int this_voice_prio;
    y_voice_t *voice;
    int best_voice_index = -1;

    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
    
        /* safeguard against an available voice. */
        if (_AVAILABLE(voice))
            return voice;
    
        /* Determine, how 'important' a voice is.
         * Start with an arbitrary number */
        this_voice_prio = 10000;

        if (_RELEASED(voice)) {
            /* This voice is in the release phase. Consider it much less
             * important than a voice which is still held. */
            this_voice_prio -= 2000;
        } else if (_SUSTAINED(voice)) {
            /* The sustain pedal is held down, and this voice is still "on"
             * because of this even though it has received a note off.
             * Consider it less important than voices which have not yet
             * received a note off. This decision is somewhat subjective, but
             * usually the sustain pedal is used to play 'more-voices-than-
             * fingers', and if so, it won't hurt as much to kill one of those
             * voices. */
             this_voice_prio -= 1000;
        };

        /* We are not enthusiastic about releasing voices, which have just been
         * started.  Otherwise hitting a chord may result in killing notes
         * belonging to that very same chord.  So subtract the age of the voice
         * from the priority - an older voice is just a little bit less
         * important than a younger voice. */
        this_voice_prio -= (synth->note_id - voice->note_id);
    
        /* -FIX- not yet implemented:
         * /= take a rough estimate of loudness into account. Louder voices are more important. =/
         * if (voice->volenv_section != FLUID_VOICE_ENVATTACK){
         *     this_voice_prio += voice->volenv_val*1000.;
         * };
         */

        /* check if this voice has less priority than the previous candidate. */
        if (this_voice_prio < best_prio)
            best_voice_index = i,
            best_prio = this_voice_prio;
    }

    if (best_voice_index < 0)
        return NULL;

    voice = synth->voice[best_voice_index];
    YDB_MESSAGE(YDB_NOTE, " y_synth_free_voice_by_kill: no available voices, killing voice %d note id %d\n", best_voice_index, voice->note_id);
    y_voice_off(synth, voice);
    return voice;
}

/*
 * y_synth_alloc_voice
 */
static y_voice_t *
y_synth_alloc_voice(y_synth_t* synth, unsigned char key)
{
    int i;
    y_voice_t* voice;

    /* If there is another voice on the same key, advance it
     * to the release phase to keep our CPU usage low. */
    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (voice->key == key && (_ON(voice) || _SUSTAINED(voice))) {
            y_voice_release_note(synth, voice);
        }
    }

    /* check if there's an available voice */
    voice = NULL;
    for (i = 0; i < synth->voices; i++) {
        if (_AVAILABLE(synth->voice[i])) {
            voice = synth->voice[i];
            break;
        }
    }

    /* No success yet? Then stop a running voice. */
    if (voice == NULL) {
        voice = y_synth_free_voice_by_kill(synth);
    }

    if (voice == NULL) {
        YDB_MESSAGE(YDB_NOTE, " y_synth_alloc_voice: failed to allocate a voice (key=%d)\n", key);
        return NULL;
    }

    YDB_MESSAGE(YDB_NOTE, " y_synth_alloc_voice: key %d voice %p\n", key, voice);
    return voice;
}

/*
 * y_synth_note_on
 */
void
y_synth_note_on(y_synth_t *synth, unsigned char key, unsigned char velocity)
{
    y_voice_t* voice;

    if (key > 127 || velocity > 127)
        return;  /* MidiKeys 1.6b3 sends bad notes.... */

    if (synth->monophonic) {

        voice = synth->voice[0];
        if (_PLAYING(synth->voice[0])) {
            YDB_MESSAGE(YDB_NOTE, " y_synth_note_on: retriggering mono voice on new key %d\n", key);
        }

    } else { /* polyphonic mode */

        voice = y_synth_alloc_voice(synth, key);
        if (voice == NULL)
            return;

    }

    voice->note_id  = synth->note_id++;

    y_voice_note_on(synth, voice, key, velocity);
}

/*
 * y_synth_key_pressure
 */
void
y_synth_key_pressure(y_synth_t *synth, unsigned char key, unsigned char pressure)
{
    int i;
    y_voice_t* voice;

    /* save it for future voices */
    synth->key_pressure[key] = pressure;
    
    /* check if any playing voices need updating */
    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (_PLAYING(voice) && voice->key == key) {
            y_voice_update_pressure_mod(synth, voice);
        }
    }
}

/*
 * y_synth_damp_voices
 *
 * advance all sustained voices to the release phase (note that this does not
 * clear the sustain controller.)
 */
void
y_synth_damp_voices(y_synth_t* synth)
{
    int i;
    y_voice_t* voice;

    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (_SUSTAINED(voice)) {
            y_voice_release_note(synth, voice);
        }
    }
}

/*
 * y_synth_update_wheel_mod
 */
void
y_synth_update_wheel_mod(y_synth_t* synth)
{
    synth->mod_wheel = (float)(synth->cc[MIDI_CTL_MSB_MODWHEEL] * 128 +
                               synth->cc[MIDI_CTL_LSB_MODWHEEL]) / 16256.0f;
    if (synth->mod_wheel > 1.0f)
        synth->mod_wheel = 1.0f;
    synth->mod[Y_MOD_MODWHEEL].next_value = synth->mod_wheel;
    /* don't need to check if any playing voices need updating, because it's global */
}

/*
 * y_synth_update_volume
 */
void
y_synth_update_volume(y_synth_t* synth)
{
    synth->cc_volume = (float)(synth->cc[MIDI_CTL_MSB_MAIN_VOLUME] * 128 +
                               synth->cc[MIDI_CTL_LSB_MAIN_VOLUME]) / 16256.0f;
    if (synth->cc_volume > 1.0f)
        synth->cc_volume = 1.0f;
    /* don't need to check if any playing voices need updating, because it's global */
}

/*
 * y_synth_update_pan
 */
void
y_synth_update_pan(y_synth_t* synth)
{
    /* <= 1 hard left, per http://www.midi.org/techspecs/rp36.php */
    synth->cc_pan = (float)((synth->cc[MIDI_CTL_MSB_PAN] - 1) * 128 +
                            synth->cc[MIDI_CTL_LSB_PAN]) / 16128.0f;
    if (synth->cc_pan > 1.0f)
        synth->cc_pan = 1.0f;
    else if (synth->cc_pan < 0.0f)
        synth->cc_pan = 0.0f;
    /* don't need to check if any playing voices need updating, because it's global */
}

/*
 * y_synth_control_change
 */
void
y_synth_control_change(y_synth_t *synth, unsigned int param, signed int value)
{
    synth->cc[param] = value;

    switch (param) {

      case MIDI_CTL_MSB_MODWHEEL:
      case MIDI_CTL_LSB_MODWHEEL:
        y_synth_update_wheel_mod(synth);
        break;

      case MIDI_CTL_MSB_MAIN_VOLUME:
      case MIDI_CTL_LSB_MAIN_VOLUME:
        y_synth_update_volume(synth);
        break;

      case MIDI_CTL_MSB_PAN:
      case MIDI_CTL_LSB_PAN:
        y_synth_update_pan(synth);
        break;

      case MIDI_CTL_SUSTAIN:
        YDB_MESSAGE(YDB_NOTE, " y_synth_control_change: got sustain control of %d\n", value);
        if (value < 64)
            y_synth_damp_voices(synth);
        break;

      case MIDI_CTL_ALL_SOUNDS_OFF:
        y_synth_all_voices_off(synth);
        break;

      case MIDI_CTL_RESET_CONTROLLERS:
        y_synth_init_controls(synth);
        break;

      case MIDI_CTL_ALL_NOTES_OFF:
        y_synth_all_notes_off(synth);
        break;

      /* what others should we respond to? */

      /* these we ignore (let the host handle):
       *  BANK_SELECT_MSB
       *  BANK_SELECT_LSB
       *  DATA_ENTRY_MSB
       *  NRPN_MSB
       *  NRPN_LSB
       *  RPN_MSB
       *  RPN_LSB
       * -FIX- no! we need RPN (0, 0) Pitch Bend Sensitivity!
       */
    }
}

/*
 * y_synth_channel_pressure
 */
void
y_synth_channel_pressure(y_synth_t *synth, signed int pressure)
{
    int i;
    y_voice_t* voice;

    /* save it for future voices */
    synth->channel_pressure = pressure;

    /* update global modulator */
    synth->pressure = (float)pressure / 127.0f;
    synth->mod[Y_MOD_PRESSURE].next_value = synth->pressure;

    /* check if any playing voices need updating */
    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (_PLAYING(voice)) {
            y_voice_update_pressure_mod(synth, voice);
        }
    }
}

/*
 * y_synth_pitch_bend
 */
void
y_synth_pitch_bend(y_synth_t *synth, signed int value)
{
    synth->pitch_wheel = value; /* ALSA pitch bend is already -8192 - 8191 */
    /* -FIX- this could probably use the (future) pitch mod LUT (or mabye exp2ap()?): */
    if (value == 0)
        synth->pitch_bend = 1.0f;
    else {
        if (value == 8191) value = 8192;
        synth->pitch_bend = exp((float)(value * lrintf(*(synth->bend_range))) /
                                (float)(8192 * 12) * M_LN2);
    }
    /* don't need to check if any playing voices need updating, because it's global */
}

/*
 * y_synth_init_controls
 */
void
y_synth_init_controls(y_synth_t *synth)
{
    int i;
    y_voice_t* voice;

    /* if sustain was on, we need to damp any sustained voices */
    if (Y_SYNTH_SUSTAINED(synth)) {
        synth->cc[MIDI_CTL_SUSTAIN] = 0;
        y_synth_damp_voices(synth);
    }

    for (i = 0; i < 128; i++) {
        synth->key_pressure[i] = 0;
        synth->cc[i] = 0;
    }
    synth->channel_pressure = 0;
    synth->pitch_wheel = 0;
    synth->cc[7] = 127;                  /* full volume */
    synth->cc[10] = 64;                  /* dead center */

    y_synth_update_wheel_mod(synth);
    y_synth_update_volume(synth);
    y_synth_update_pan(synth);
    synth->pitch_bend = 1.0f;

    /* check if any playing voices need updating */
    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (_PLAYING(voice)) {
            y_voice_update_pressure_mod(synth, voice);
        }
    }
}

/*
 * y_synth_select_patch
 */
void
y_synth_select_patch(y_synth_t *synth, unsigned long patch)
{
    if (patch >= synth->patch_count) return;

    if (synth->program_cancel)
        y_synth_all_voices_off(synth);

    y_voice_set_ports(synth, &synth->patches[patch]);
}

/*
 * y_synth_set_program_descriptor
 */
int
y_synth_set_program_descriptor(y_synth_t *synth, DSSI_Program_Descriptor *pd,
                               unsigned long patch)
{
    if (patch >= synth->patch_count) {
        return 0;
    }
    pd->Bank = 0;
    pd->Program = patch;
    pd->Name = synth->patches[patch].name;
    return 1;

}

/*
 * y_synth_handle_load
 */
char *
y_synth_handle_load(y_synth_t *synth, const char *value)
{
    char *file, *rv;

    YDB_MESSAGE(YDB_DATA, " y_synth_handle_load: attempting to load '%s'\n", value);

    if (!(file = y_data_locate_patch_file(value, synth->project_dir))) {
	return dssi_configure_message("load error: could not find file '%s'",
				      value);
    }

    if ((rv = y_data_load(synth, file)) != NULL) {
        free(file);
        return rv;
    }

    if (strcmp(file, value)) {
	rv = dssi_configure_message("warning: patch file '%s' not found, loaded '%s' instead",
                                    value, file);
	free(file);
	return rv;
    } else {
	free(file);
	return NULL;
    }
}

/*
 * y_synth_handle_monophonic
 */
char *
y_synth_handle_monophonic(y_synth_t *synth, const char *value)
{
    int mode = -1;

    if (!strcmp(value, "on")) mode = Y_MONO_MODE_ON;
    else if (!strcmp(value, "once")) mode = Y_MONO_MODE_ONCE;
    else if (!strcmp(value, "both")) mode = Y_MONO_MODE_BOTH;
    else if (!strcmp(value, "off"))  mode = Y_MONO_MODE_OFF;

    if (mode == -1) {
        return dssi_configure_message("error: monophonic value not recognized");
    }

    if (mode == Y_MONO_MODE_OFF) {  /* polyphonic mode */

        synth->monophonic = 0;
        synth->voices = synth->polyphony;

    } else {  /* one of the monophonic modes */

        dssp_voicelist_mutex_lock(synth);

        if (!synth->monophonic) y_synth_all_voices_off(synth);
        synth->monophonic = mode;
        synth->voices = 1;

        dssp_voicelist_mutex_unlock(synth);
    }

    return NULL;
}

/*
 * y_synth_handle_polyphony
 */
char *
y_synth_handle_polyphony(y_synth_t *synth, const char *value)
{
    int polyphony = atoi(value);
    int i;
    y_voice_t *voice;

    if (polyphony < 1 || polyphony > Y_MAX_POLYPHONY) {
        return dssi_configure_message("error: polyphony value out of range");
    }
    /* set the new limit */
    synth->polyphony = polyphony;

    if (!synth->monophonic) {
        synth->voices = polyphony;

        /* turn off any voices above the new limit */

        dssp_voicelist_mutex_lock(synth);

        for (i = polyphony; i < Y_MAX_POLYPHONY; i++) {
            voice = synth->voice[i];
            if (_PLAYING(voice)) {
                if (synth->held_keys[0] != -1)
                    y_synth_clear_held_keys(synth);
                y_voice_off(synth, voice);
            }
        }

        dssp_voicelist_mutex_unlock(synth);
    }

    return NULL;
}

/*
 * y_synth_handle_glide
 */
char *
y_synth_handle_glide(y_synth_t *synth, const char *value)
{
    int mode = -1;

    if (!strcmp(value, "legato"))        mode = Y_GLIDE_MODE_LEGATO;
    else if (!strcmp(value, "initial"))  mode = Y_GLIDE_MODE_INITIAL;
    else if (!strcmp(value, "always"))   mode = Y_GLIDE_MODE_ALWAYS;
    else if (!strcmp(value, "leftover")) mode = Y_GLIDE_MODE_LEFTOVER;
    else if (!strcmp(value, "off"))      mode = Y_GLIDE_MODE_OFF;

    if (mode == -1) {
        return dssi_configure_message("error: glide value not recognized");
    }

    synth->glide = mode;

    return NULL;
}

/*
 * y_synth_handle_program_cancel
 */
char *
y_synth_handle_program_cancel(y_synth_t *synth, const char *value)
{
    synth->program_cancel = strcmp(value, "off") ? 1 : 0;

    return NULL;
}

/*
 * y_synth_handle_project_dir
 */
char *
y_synth_handle_project_dir(y_synth_t *synth, const char *value)
{
    if (synth->project_dir) free(synth->project_dir);
    if (!value) synth->project_dir = NULL;
    else synth->project_dir = strdup(value);
    return NULL;
}


/*
 * y_synth_render_voices
 */
void
y_synth_render_voices(y_synth_t *synth,
                      LADSPA_Data *out_left, LADSPA_Data *out_right,
                      unsigned long sample_count, int do_control_update)
{
    unsigned long i;
    y_voice_t* voice;

    /* check for sampleset (non-realtime-rendered) resource changes */
    sampleset_check_oscillators(synth);

    /* silence the pre-effect buffer */
    for (i = 0; i < sample_count; i++)
        synth->voice_bus_l[i] = synth->voice_bus_r[i] = 0.0f;

#if defined(Y_DEBUG) && (Y_DEBUG & YDB_AUDIO)
out_left[0]  += 0.10f; /* add a 'buzz' to output so there's something audible even when quiescent */
out_right[0] += 0.10f;
#endif /* defined(Y_DEBUG) && (Y_DEBUG & YDB_AUDIO) */

    /* pre-render global modulator updates */
    if (fabsf(synth->mod[Y_MOD_MODWHEEL].next_value - synth->mod[Y_MOD_MODWHEEL].value) > 1e-10) {
        synth->mod[Y_MOD_MODWHEEL].delta =
           (synth->mod[Y_MOD_MODWHEEL].next_value - synth->mod[Y_MOD_MODWHEEL].value) /
               (float)Y_CONTROL_PERIOD;
    }
    if (fabsf(synth->mod[Y_MOD_PRESSURE].next_value - synth->mod[Y_MOD_PRESSURE].value) > 1e-10) {
        synth->mod[Y_MOD_PRESSURE].delta =
           (synth->mod[Y_MOD_PRESSURE].next_value - synth->mod[Y_MOD_PRESSURE].value) /
               (float)Y_CONTROL_PERIOD;
    }

    /* render each active voice */
    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
    
        if (_PLAYING(voice)) {
            y_voice_render(synth, voice, synth->voice_bus_l, synth->voice_bus_r,
                           sample_count, do_control_update);
        }
    }

    /* post-render global modulator updates */
    if (do_control_update) {
        synth->mod[Y_MOD_MODWHEEL].value += (float)sample_count * synth->mod[Y_MOD_MODWHEEL].delta;
        synth->mod[Y_MOD_PRESSURE].value += (float)sample_count * synth->mod[Y_MOD_PRESSURE].delta;
        y_voice_update_lfo(synth, &synth->glfo, &synth->glfo_vlfo,
                           synth->mod, &synth->mod[Y_GLOBAL_MOD_GLFO]);
    } else {
        /* update modulator values for the incomplete control cycle */
        for (i = 1; i < Y_GLOBAL_MODS_COUNT; i++)
            synth->mod[i].value += (float)sample_count * synth->mod[i].delta;
    }

    /* effect processing */
    synth->voice_bus_l[0] += 1e-20f; /* ''' bubbling of the quantum foam ,,, */
    synth->voice_bus_r[0] += 1e-20f;
    synth->voice_bus_l[sample_count >> 1] -= 1e-20f;
    synth->voice_bus_r[sample_count >> 1] -= 1e-20f;
    if (lrintf(*(synth->effect_mode)) == 0) {  /* 'Off', or DC-filter only */
        float r = synth->dc_block_r,
              l_xnm1 = synth->dc_block_l_xnm1,
              l_ynm1 = synth->dc_block_l_ynm1,
              r_xnm1 = synth->dc_block_r_xnm1,
              r_ynm1 = synth->dc_block_r_ynm1;
        for (i = 0; i < sample_count; i++) {
            l_ynm1 = synth->voice_bus_l[i] - l_xnm1 + r * l_ynm1;
            l_xnm1 = synth->voice_bus_l[i];
            out_left[i] = l_ynm1;
            r_ynm1 = synth->voice_bus_r[i] - r_xnm1 + r * r_ynm1;
            r_xnm1 = synth->voice_bus_r[i];
            out_right[i] = r_ynm1;
        }
        synth->dc_block_l_xnm1 = l_xnm1;
        synth->dc_block_l_ynm1 = l_ynm1;
        synth->dc_block_r_xnm1 = r_xnm1;
        synth->dc_block_r_ynm1 = r_ynm1;

        synth->last_effect_mode = 0;
    } else
        effects_process(synth, sample_count, out_left, out_right);  /* other effects */
}

