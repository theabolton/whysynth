/* DSSI Plugin Framework
 *
 * Copyright (C) 2005-2007, 2010, 2016 Sean Bolton and others.
 *
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>

#include <ladspa.h>
#include <dssi.h>

#include "whysynth_types.h"
#include "whysynth.h"
#include "whysynth_ports.h"
#include "dssp_event.h"
#include "common_data.h"
#include "whysynth_voice.h"
#include "agran_oscillator.h"
#include "wave_tables.h"
#include "sampleset.h"
#include "effects.h"

static pthread_mutex_t global_mutex;
y_global_t             global;

static LADSPA_Descriptor *y_LADSPA_descriptor = NULL;
static DSSI_Descriptor   *y_DSSI_descriptor = NULL;

static void y_cleanup(LADSPA_Handle instance);
static void y_run_synth(LADSPA_Handle instance, unsigned long sample_count,
                             snd_seq_event_t *events, unsigned long event_count);

/* ---- mutual exclusion ---- */

static inline int
dssp_voicelist_mutex_trylock(y_synth_t *synth)
{
    int rc;

    /* Attempt the mutex lock */
    rc = pthread_mutex_trylock(&synth->voicelist_mutex);
    if (rc) {
        synth->voicelist_mutex_grab_failed = 1;
        return rc;
    }
    /* Clean up if a previous mutex grab failed */
    if (synth->voicelist_mutex_grab_failed) {
        y_synth_all_voices_off(synth);
        synth->voicelist_mutex_grab_failed = 0;
    }
    return 0;
}

int
dssp_voicelist_mutex_lock(y_synth_t *synth)
{
    return pthread_mutex_lock(&synth->voicelist_mutex);
}

int
dssp_voicelist_mutex_unlock(y_synth_t *synth)
{
    return pthread_mutex_unlock(&synth->voicelist_mutex);
}

/* ---- LADSPA interface ---- */

/*
 * y_instantiate
 *
 * implements LADSPA (*instantiate)()
 */
static LADSPA_Handle
y_instantiate(const LADSPA_Descriptor *descriptor, unsigned long sample_rate)
{
    y_synth_t *synth = (y_synth_t *)calloc(1, sizeof(y_synth_t));
    int i;
    static float static_zero = 0.0f;

    if (!synth) return NULL;

    pthread_mutex_lock(&global_mutex);

    if (global.initialized) {

        if (sample_rate != global.sample_rate) {
            /* all instances must share same sample rate */
            pthread_mutex_unlock(&global_mutex);
            free(synth);
            return NULL;
        }

        global.instance_count++;

    } else {

        global.sample_rate = sample_rate;
        global.grain_envelope = create_grain_envelopes(sample_rate);
        if (!global.grain_envelope) {
            YDB_MESSAGE(-1, " y_instantiate: out of memory!\n");
            free(synth);
            return NULL;
        }
        if (!sampleset_init()) {
            YDB_MESSAGE(-1, " y_instantiate: sampleset_setup() failed!\n");
            free(synth);
            return NULL;
        }
        global.instance_count = 1;
        global.initialized = 1;
    }

    pthread_mutex_unlock(&global_mutex);

    /* do any per-instance one-time initialization here */
    for (i = 0; i < Y_MAX_POLYPHONY; i++) {
        synth->voice[i] = y_voice_new(synth);
        if (!synth->voice[i]) {
            // YDB_MESSAGE(-1, " y_instantiate: out of memory!\n");
            y_cleanup(synth);
            return NULL;
        }
    }

    if (!new_grain_array(synth, AG_DEFAULT_GRAIN_COUNT)) {
        YDB_MESSAGE(-1, " y_instantiate: out of memory!\n");
        y_cleanup(synth);
        return NULL;
    }

    if (!sampleset_instantiate(synth)) {
        YDB_MESSAGE(-1, " y_instantiate: out of memory!\n");
        y_cleanup(synth);
        return NULL;
    }

    synth->sample_rate = (float)sample_rate;
    synth->control_rate = (float)sample_rate / (float)Y_CONTROL_PERIOD;
    synth->deltat = 1.0f / synth->sample_rate;

    if (!effects_setup(synth)) {
        YDB_MESSAGE(-1, " y_instantiate: out of memory!\n");
        y_cleanup(synth);
        return NULL;
    }

    synth->polyphony = Y_DEFAULT_POLYPHONY;
    synth->voices = Y_DEFAULT_POLYPHONY;
    synth->monophonic = 0;
    synth->glide = 0;
    synth->last_noteon_pitch = 0.0f;
    pthread_mutex_init(&synth->voicelist_mutex, NULL);
    synth->voicelist_mutex_grab_failed = 0;
    pthread_mutex_init(&synth->patches_mutex, NULL);
    synth->patch_count = 0;
    synth->patches_allocated = 0;
    synth->patches = NULL;
    synth->pending_patch_change = -1;
    synth->program_cancel = 1;
    synth->project_dir = NULL;
    synth->osc1.sampleset = NULL;
    synth->osc2.sampleset = NULL;
    synth->osc3.sampleset = NULL;
    synth->osc4.sampleset = NULL;
    synth->glfo.delay = &static_zero;
    synth->ego.level[3] = &static_zero;
    synth->eg1.level[3] = &static_zero;
    synth->eg2.level[3] = &static_zero;
    synth->eg3.level[3] = &static_zero;
    synth->eg4.level[3] = &static_zero;
    synth->mod[Y_MOD_ONE].value = 1.0f;
    synth->mod[Y_MOD_ONE].next_value = 1.0f;
    synth->mod[Y_MOD_ONE].delta = 0.0f;
    synth->dc_block_r = 1.0f - (2.0f * 3.141593f * 20.0f/* Hz */ / (float)sample_rate); /* DC blocker cutoff */
    y_data_friendly_patches(synth);
    y_synth_init_controls(synth);

    return (LADSPA_Handle)synth;
}

/*
 * y_connect_port
 *
 * implements LADSPA (*connect_port)()
 */
static void
y_connect_port(LADSPA_Handle instance, unsigned long port, LADSPA_Data *data)
{
    y_synth_t *synth = (y_synth_t *)instance;

    switch (port) {
      /* -PORTS- */
      case Y_PORT_OUTPUT_LEFT:        synth->output_left        = data;  break;
      case Y_PORT_OUTPUT_RIGHT:       synth->output_right       = data;  break;

      case Y_PORT_OSC1_MODE:          synth->osc1.mode          = data;  break;
      case Y_PORT_OSC1_WAVEFORM:      synth->osc1.waveform      = data;  break;
      case Y_PORT_OSC1_PITCH:         synth->osc1.pitch         = data;  break;
      case Y_PORT_OSC1_DETUNE:        synth->osc1.detune        = data;  break;
      case Y_PORT_OSC1_PITCH_MOD_SRC: synth->osc1.pitch_mod_src = data;  break;
      case Y_PORT_OSC1_PITCH_MOD_AMT: synth->osc1.pitch_mod_amt = data;  break;
      case Y_PORT_OSC1_MPARAM1:       synth->osc1.mparam1       = data;  break;
      case Y_PORT_OSC1_MPARAM2:       synth->osc1.mparam2       = data;  break;
      case Y_PORT_OSC1_MMOD_SRC:      synth->osc1.mmod_src      = data;  break;
      case Y_PORT_OSC1_MMOD_AMT:      synth->osc1.mmod_amt      = data;  break;
      case Y_PORT_OSC1_AMP_MOD_SRC:   synth->osc1.amp_mod_src   = data;  break;
      case Y_PORT_OSC1_AMP_MOD_AMT:   synth->osc1.amp_mod_amt   = data;  break;
      case Y_PORT_OSC1_LEVEL_A:       synth->osc1.level_a       = data;  break;
      case Y_PORT_OSC1_LEVEL_B:       synth->osc1.level_b       = data;  break;

      case Y_PORT_OSC2_MODE:          synth->osc2.mode          = data;  break;
      case Y_PORT_OSC2_WAVEFORM:      synth->osc2.waveform      = data;  break;
      case Y_PORT_OSC2_PITCH:         synth->osc2.pitch         = data;  break;
      case Y_PORT_OSC2_DETUNE:        synth->osc2.detune        = data;  break;
      case Y_PORT_OSC2_PITCH_MOD_SRC: synth->osc2.pitch_mod_src = data;  break;
      case Y_PORT_OSC2_PITCH_MOD_AMT: synth->osc2.pitch_mod_amt = data;  break;
      case Y_PORT_OSC2_MPARAM1:       synth->osc2.mparam1       = data;  break;
      case Y_PORT_OSC2_MPARAM2:       synth->osc2.mparam2       = data;  break;
      case Y_PORT_OSC2_MMOD_SRC:      synth->osc2.mmod_src      = data;  break;
      case Y_PORT_OSC2_MMOD_AMT:      synth->osc2.mmod_amt      = data;  break;
      case Y_PORT_OSC2_AMP_MOD_SRC:   synth->osc2.amp_mod_src   = data;  break;
      case Y_PORT_OSC2_AMP_MOD_AMT:   synth->osc2.amp_mod_amt   = data;  break;
      case Y_PORT_OSC2_LEVEL_A:       synth->osc2.level_a       = data;  break;
      case Y_PORT_OSC2_LEVEL_B:       synth->osc2.level_b       = data;  break;

      case Y_PORT_OSC3_MODE:          synth->osc3.mode          = data;  break;
      case Y_PORT_OSC3_WAVEFORM:      synth->osc3.waveform      = data;  break;
      case Y_PORT_OSC3_PITCH:         synth->osc3.pitch         = data;  break;
      case Y_PORT_OSC3_DETUNE:        synth->osc3.detune        = data;  break;
      case Y_PORT_OSC3_PITCH_MOD_SRC: synth->osc3.pitch_mod_src = data;  break;
      case Y_PORT_OSC3_PITCH_MOD_AMT: synth->osc3.pitch_mod_amt = data;  break;
      case Y_PORT_OSC3_MPARAM1:       synth->osc3.mparam1       = data;  break;
      case Y_PORT_OSC3_MPARAM2:       synth->osc3.mparam2       = data;  break;
      case Y_PORT_OSC3_MMOD_SRC:      synth->osc3.mmod_src      = data;  break;
      case Y_PORT_OSC3_MMOD_AMT:      synth->osc3.mmod_amt      = data;  break;
      case Y_PORT_OSC3_AMP_MOD_SRC:   synth->osc3.amp_mod_src   = data;  break;
      case Y_PORT_OSC3_AMP_MOD_AMT:   synth->osc3.amp_mod_amt   = data;  break;
      case Y_PORT_OSC3_LEVEL_A:       synth->osc3.level_a       = data;  break;
      case Y_PORT_OSC3_LEVEL_B:       synth->osc3.level_b       = data;  break;

      case Y_PORT_OSC4_MODE:          synth->osc4.mode          = data;  break;
      case Y_PORT_OSC4_WAVEFORM:      synth->osc4.waveform      = data;  break;
      case Y_PORT_OSC4_PITCH:         synth->osc4.pitch         = data;  break;
      case Y_PORT_OSC4_DETUNE:        synth->osc4.detune        = data;  break;
      case Y_PORT_OSC4_PITCH_MOD_SRC: synth->osc4.pitch_mod_src = data;  break;
      case Y_PORT_OSC4_PITCH_MOD_AMT: synth->osc4.pitch_mod_amt = data;  break;
      case Y_PORT_OSC4_MPARAM1:       synth->osc4.mparam1       = data;  break;
      case Y_PORT_OSC4_MPARAM2:       synth->osc4.mparam2       = data;  break;
      case Y_PORT_OSC4_MMOD_SRC:      synth->osc4.mmod_src      = data;  break;
      case Y_PORT_OSC4_MMOD_AMT:      synth->osc4.mmod_amt      = data;  break;
      case Y_PORT_OSC4_AMP_MOD_SRC:   synth->osc4.amp_mod_src   = data;  break;
      case Y_PORT_OSC4_AMP_MOD_AMT:   synth->osc4.amp_mod_amt   = data;  break;
      case Y_PORT_OSC4_LEVEL_A:       synth->osc4.level_a       = data;  break;
      case Y_PORT_OSC4_LEVEL_B:       synth->osc4.level_b       = data;  break;

      case Y_PORT_VCF1_MODE:          synth->vcf1.mode          = data;  break;
      case Y_PORT_VCF1_SOURCE:        synth->vcf1.source        = data;  break;
      case Y_PORT_VCF1_FREQUENCY:     synth->vcf1.frequency     = data;  break;
      case Y_PORT_VCF1_FREQ_MOD_SRC:  synth->vcf1.freq_mod_src  = data;  break;
      case Y_PORT_VCF1_FREQ_MOD_AMT:  synth->vcf1.freq_mod_amt  = data;  break;
      case Y_PORT_VCF1_QRES:          synth->vcf1.qres          = data;  break;
      case Y_PORT_VCF1_MPARAM:        synth->vcf1.mparam        = data;  break;

      case Y_PORT_VCF2_MODE:          synth->vcf2.mode          = data;  break;
      case Y_PORT_VCF2_SOURCE:        synth->vcf2.source        = data;  break;
      case Y_PORT_VCF2_FREQUENCY:     synth->vcf2.frequency     = data;  break;
      case Y_PORT_VCF2_FREQ_MOD_SRC:  synth->vcf2.freq_mod_src  = data;  break;
      case Y_PORT_VCF2_FREQ_MOD_AMT:  synth->vcf2.freq_mod_amt  = data;  break;
      case Y_PORT_VCF2_QRES:          synth->vcf2.qres          = data;  break;
      case Y_PORT_VCF2_MPARAM:        synth->vcf2.mparam        = data;  break;

      case Y_PORT_BUSA_LEVEL:         synth->busa_level         = data;  break;
      case Y_PORT_BUSA_PAN:           synth->busa_pan           = data;  break;
      case Y_PORT_BUSB_LEVEL:         synth->busb_level         = data;  break;
      case Y_PORT_BUSB_PAN:           synth->busb_pan           = data;  break;
      case Y_PORT_VCF1_LEVEL:         synth->vcf1_level         = data;  break;
      case Y_PORT_VCF1_PAN:           synth->vcf1_pan           = data;  break;
      case Y_PORT_VCF2_LEVEL:         synth->vcf2_level         = data;  break;
      case Y_PORT_VCF2_PAN:           synth->vcf2_pan           = data;  break;
      case Y_PORT_VOLUME:             synth->volume             = data;  break;

      case Y_PORT_EFFECT_MODE:        synth->effect_mode        = data;  break;
      case Y_PORT_EFFECT_PARAM1:      synth->effect_param1      = data;  break;
      case Y_PORT_EFFECT_PARAM2:      synth->effect_param2      = data;  break;
      case Y_PORT_EFFECT_PARAM3:      synth->effect_param3      = data;  break;
      case Y_PORT_EFFECT_PARAM4:      synth->effect_param4      = data;  break;
      case Y_PORT_EFFECT_PARAM5:      synth->effect_param5      = data;  break;
      case Y_PORT_EFFECT_PARAM6:      synth->effect_param6      = data;  break;
      case Y_PORT_EFFECT_MIX:         synth->effect_mix         = data;  break;

      case Y_PORT_GLIDE_TIME:         synth->glide_time         = data;  break;
      case Y_PORT_BEND_RANGE:         synth->bend_range         = data;  break;

      case Y_PORT_GLFO_FREQUENCY:     synth->glfo.frequency     = data;  break;
      case Y_PORT_GLFO_WAVEFORM:      synth->glfo.waveform      = data;  break;
      /* synth->glfo.delay always points to a 0.0f */
      case Y_PORT_GLFO_AMP_MOD_SRC:   synth->glfo.amp_mod_src   = data;  break;
      case Y_PORT_GLFO_AMP_MOD_AMT:   synth->glfo.amp_mod_amt   = data;  break;

      case Y_PORT_VLFO_FREQUENCY:     synth->vlfo.frequency     = data;  break;
      case Y_PORT_VLFO_WAVEFORM:      synth->vlfo.waveform      = data;  break;
      case Y_PORT_VLFO_DELAY:         synth->vlfo.delay         = data;  break;
      case Y_PORT_VLFO_AMP_MOD_SRC:   synth->vlfo.amp_mod_src   = data;  break;
      case Y_PORT_VLFO_AMP_MOD_AMT:   synth->vlfo.amp_mod_amt   = data;  break;

      case Y_PORT_MLFO_FREQUENCY:     synth->mlfo.frequency     = data;  break;
      case Y_PORT_MLFO_WAVEFORM:      synth->mlfo.waveform      = data;  break;
      case Y_PORT_MLFO_DELAY:         synth->mlfo.delay         = data;  break;
      case Y_PORT_MLFO_AMP_MOD_SRC:   synth->mlfo.amp_mod_src   = data;  break;
      case Y_PORT_MLFO_AMP_MOD_AMT:   synth->mlfo.amp_mod_amt   = data;  break;
      case Y_PORT_MLFO_PHASE_SPREAD:  synth->mlfo_phase_spread  = data;  break;
      case Y_PORT_MLFO_RANDOM_FREQ:   synth->mlfo_random_freq   = data;  break;

      case Y_PORT_EGO_MODE:           synth->ego.mode           = data;  break;
      case Y_PORT_EGO_SHAPE1:         synth->ego.shape[0]       = data;  break;
      case Y_PORT_EGO_TIME1:          synth->ego.time[0]        = data;  break;
      case Y_PORT_EGO_LEVEL1:         synth->ego.level[0]       = data;  break;
      case Y_PORT_EGO_SHAPE2:         synth->ego.shape[1]       = data;  break;
      case Y_PORT_EGO_TIME2:          synth->ego.time[1]        = data;  break;
      case Y_PORT_EGO_LEVEL2:         synth->ego.level[1]       = data;  break;
      case Y_PORT_EGO_SHAPE3:         synth->ego.shape[2]       = data;  break;
      case Y_PORT_EGO_TIME3:          synth->ego.time[2]        = data;  break;
      case Y_PORT_EGO_LEVEL3:         synth->ego.level[2]       = data;  break;
      case Y_PORT_EGO_SHAPE4:         synth->ego.shape[3]       = data;  break;
      case Y_PORT_EGO_TIME4:          synth->ego.time[3]        = data;  break;
      /* synth->ego.level[3] always points to a 0.0f */
      case Y_PORT_EGO_VEL_LEVEL_SENS: synth->ego.vel_level_sens = data;  break;
      case Y_PORT_EGO_VEL_TIME_SCALE: synth->ego.vel_time_scale = data;  break;
      case Y_PORT_EGO_KBD_TIME_SCALE: synth->ego.kbd_time_scale = data;  break;
      case Y_PORT_EGO_AMP_MOD_SRC:    synth->ego.amp_mod_src    = data;  break;
      case Y_PORT_EGO_AMP_MOD_AMT:    synth->ego.amp_mod_amt    = data;  break;

      case Y_PORT_EG1_MODE:           synth->eg1.mode           = data;  break;
      case Y_PORT_EG1_SHAPE1:         synth->eg1.shape[0]       = data;  break;
      case Y_PORT_EG1_TIME1:          synth->eg1.time[0]        = data;  break;
      case Y_PORT_EG1_LEVEL1:         synth->eg1.level[0]       = data;  break;
      case Y_PORT_EG1_SHAPE2:         synth->eg1.shape[1]       = data;  break;
      case Y_PORT_EG1_TIME2:          synth->eg1.time[1]        = data;  break;
      case Y_PORT_EG1_LEVEL2:         synth->eg1.level[1]       = data;  break;
      case Y_PORT_EG1_SHAPE3:         synth->eg1.shape[2]       = data;  break;
      case Y_PORT_EG1_TIME3:          synth->eg1.time[2]        = data;  break;
      case Y_PORT_EG1_LEVEL3:         synth->eg1.level[2]       = data;  break;
      case Y_PORT_EG1_SHAPE4:         synth->eg1.shape[3]       = data;  break;
      case Y_PORT_EG1_TIME4:          synth->eg1.time[3]        = data;  break;
      /* synth->eg1.level[3] always points to a 0.0f */
      case Y_PORT_EG1_VEL_LEVEL_SENS: synth->eg1.vel_level_sens = data;  break;
      case Y_PORT_EG1_VEL_TIME_SCALE: synth->eg1.vel_time_scale = data;  break;
      case Y_PORT_EG1_KBD_TIME_SCALE: synth->eg1.kbd_time_scale = data;  break;
      case Y_PORT_EG1_AMP_MOD_SRC:    synth->eg1.amp_mod_src    = data;  break;
      case Y_PORT_EG1_AMP_MOD_AMT:    synth->eg1.amp_mod_amt    = data;  break;

      case Y_PORT_EG2_MODE:           synth->eg2.mode           = data;  break;
      case Y_PORT_EG2_SHAPE1:         synth->eg2.shape[0]       = data;  break;
      case Y_PORT_EG2_TIME1:          synth->eg2.time[0]        = data;  break;
      case Y_PORT_EG2_LEVEL1:         synth->eg2.level[0]       = data;  break;
      case Y_PORT_EG2_SHAPE2:         synth->eg2.shape[1]       = data;  break;
      case Y_PORT_EG2_TIME2:          synth->eg2.time[1]        = data;  break;
      case Y_PORT_EG2_LEVEL2:         synth->eg2.level[1]       = data;  break;
      case Y_PORT_EG2_SHAPE3:         synth->eg2.shape[2]       = data;  break;
      case Y_PORT_EG2_TIME3:          synth->eg2.time[2]        = data;  break;
      case Y_PORT_EG2_LEVEL3:         synth->eg2.level[2]       = data;  break;
      case Y_PORT_EG2_SHAPE4:         synth->eg2.shape[3]       = data;  break;
      case Y_PORT_EG2_TIME4:          synth->eg2.time[3]        = data;  break;
      /* synth->eg2.level[3] always points to a 0.0f */
      case Y_PORT_EG2_VEL_LEVEL_SENS: synth->eg2.vel_level_sens = data;  break;
      case Y_PORT_EG2_VEL_TIME_SCALE: synth->eg2.vel_time_scale = data;  break;
      case Y_PORT_EG2_KBD_TIME_SCALE: synth->eg2.kbd_time_scale = data;  break;
      case Y_PORT_EG2_AMP_MOD_SRC:    synth->eg2.amp_mod_src    = data;  break;
      case Y_PORT_EG2_AMP_MOD_AMT:    synth->eg2.amp_mod_amt    = data;  break;

      case Y_PORT_EG3_MODE:           synth->eg3.mode           = data;  break;
      case Y_PORT_EG3_SHAPE1:         synth->eg3.shape[0]       = data;  break;
      case Y_PORT_EG3_TIME1:          synth->eg3.time[0]        = data;  break;
      case Y_PORT_EG3_LEVEL1:         synth->eg3.level[0]       = data;  break;
      case Y_PORT_EG3_SHAPE2:         synth->eg3.shape[1]       = data;  break;
      case Y_PORT_EG3_TIME2:          synth->eg3.time[1]        = data;  break;
      case Y_PORT_EG3_LEVEL2:         synth->eg3.level[1]       = data;  break;
      case Y_PORT_EG3_SHAPE3:         synth->eg3.shape[2]       = data;  break;
      case Y_PORT_EG3_TIME3:          synth->eg3.time[2]        = data;  break;
      case Y_PORT_EG3_LEVEL3:         synth->eg3.level[2]       = data;  break;
      case Y_PORT_EG3_SHAPE4:         synth->eg3.shape[3]       = data;  break;
      case Y_PORT_EG3_TIME4:          synth->eg3.time[3]        = data;  break;
      /* synth->eg3.level[3] always points to a 0.0f */
      case Y_PORT_EG3_VEL_LEVEL_SENS: synth->eg3.vel_level_sens = data;  break;
      case Y_PORT_EG3_VEL_TIME_SCALE: synth->eg3.vel_time_scale = data;  break;
      case Y_PORT_EG3_KBD_TIME_SCALE: synth->eg3.kbd_time_scale = data;  break;
      case Y_PORT_EG3_AMP_MOD_SRC:    synth->eg3.amp_mod_src    = data;  break;
      case Y_PORT_EG3_AMP_MOD_AMT:    synth->eg3.amp_mod_amt    = data;  break;

      case Y_PORT_EG4_MODE:           synth->eg4.mode           = data;  break;
      case Y_PORT_EG4_SHAPE1:         synth->eg4.shape[0]       = data;  break;
      case Y_PORT_EG4_TIME1:          synth->eg4.time[0]        = data;  break;
      case Y_PORT_EG4_LEVEL1:         synth->eg4.level[0]       = data;  break;
      case Y_PORT_EG4_SHAPE2:         synth->eg4.shape[1]       = data;  break;
      case Y_PORT_EG4_TIME2:          synth->eg4.time[1]        = data;  break;
      case Y_PORT_EG4_LEVEL2:         synth->eg4.level[1]       = data;  break;
      case Y_PORT_EG4_SHAPE3:         synth->eg4.shape[2]       = data;  break;
      case Y_PORT_EG4_TIME3:          synth->eg4.time[2]        = data;  break;
      case Y_PORT_EG4_LEVEL3:         synth->eg4.level[2]       = data;  break;
      case Y_PORT_EG4_SHAPE4:         synth->eg4.shape[3]       = data;  break;
      case Y_PORT_EG4_TIME4:          synth->eg4.time[3]        = data;  break;
      /* synth->eg4.level[3] always points to a 0.0f */
      case Y_PORT_EG4_VEL_LEVEL_SENS: synth->eg4.vel_level_sens = data;  break;
      case Y_PORT_EG4_VEL_TIME_SCALE: synth->eg4.vel_time_scale = data;  break;
      case Y_PORT_EG4_KBD_TIME_SCALE: synth->eg4.kbd_time_scale = data;  break;
      case Y_PORT_EG4_AMP_MOD_SRC:    synth->eg4.amp_mod_src    = data;  break;
      case Y_PORT_EG4_AMP_MOD_AMT:    synth->eg4.amp_mod_amt    = data;  break;

      case Y_PORT_MODMIX_BIAS:        synth->modmix_bias        = data;  break;
      case Y_PORT_MODMIX_MOD1_SRC:    synth->modmix_mod1_src    = data;  break;
      case Y_PORT_MODMIX_MOD1_AMT:    synth->modmix_mod1_amt    = data;  break;
      case Y_PORT_MODMIX_MOD2_SRC:    synth->modmix_mod2_src    = data;  break;
      case Y_PORT_MODMIX_MOD2_AMT:    synth->modmix_mod2_amt    = data;  break;

      case Y_PORT_TUNING:             synth->tuning             = data;  break;

      default:
        break;
    }
}

/*
 * y_activate
 *
 * implements LADSPA (*activate)()
 */
static void
y_activate(LADSPA_Handle instance)
{
    y_synth_t *synth = (y_synth_t *)instance;

    synth->control_remains = 0;
    synth->note_id = 0;
    y_voice_setup_lfo(synth, &synth->glfo, &synth->glfo_vlfo, 0.0f, 0.0f,
                      synth->mod, &synth->mod[Y_GLOBAL_MOD_GLFO]);
    y_synth_all_voices_off(synth);
}

/*
 * y_ladspa_run_wrapper
 *
 * implements LADSPA (*run)() by calling y_run_synth() with no events
 */
static void
y_ladspa_run_wrapper(LADSPA_Handle instance, unsigned long sample_count)
{
    y_run_synth(instance, sample_count, NULL, 0);
}

// optional:
//  void (*run_adding)(LADSPA_Handle Instance,
//                     unsigned long SampleCount);
//  void (*set_run_adding_gain)(LADSPA_Handle Instance,
//                              LADSPA_Data   Gain);

/*
 * y_deactivate
 *
 * implements LADSPA (*deactivate)()
 */
void
y_deactivate(LADSPA_Handle instance)
{
    y_synth_t *synth = (y_synth_t *)instance;

    y_synth_all_voices_off(synth);  /* stop all sounds immediately */
}

/*
 * y_cleanup
 *
 * implements LADSPA (*cleanup)()
 */
static void
y_cleanup(LADSPA_Handle instance)
{
    y_synth_t *synth = (y_synth_t *)instance;
    int i;

    for (i = 0; i < Y_MAX_POLYPHONY; i++)
        if (synth->voice[i]) free(synth->voice[i]);
    if (synth->patches) free(synth->patches);
    if (synth->grains) free(synth->grains);
    if (synth->project_dir) free(synth->project_dir);
    sampleset_cleanup(synth);
    effects_cleanup(synth);
    free(synth);
    pthread_mutex_lock(&global_mutex);
    if (--global.instance_count == 0) {
        sampleset_fini();
        free_grain_envelopes(global.grain_envelope);
        global.initialized = 0;
    }
    pthread_mutex_unlock(&global_mutex);
}

/* ---- DSSI interface ---- */

/*
 * dssi_configure_message
 */
char *
dssi_configure_message(const char *fmt, ...)
{
    va_list args;
    char buffer[256];

    va_start(args, fmt);
    vsnprintf(buffer, 256, fmt, args);
    va_end(args);
    return strdup(buffer);
}

/*
 * y_configure
 *
 * implements DSSI (*configure)()
 */
char *
y_configure(LADSPA_Handle instance, const char *key, const char *value)
{
    YDB_MESSAGE(YDB_DSSI, " y_configure called with '%s' and '%s'\n", key, value);

    if (!strcmp(key, "load")) {

        return y_synth_handle_load((y_synth_t *)instance, value);

    } else if (!strcmp(key, "polyphony")) {

        return y_synth_handle_polyphony((y_synth_t *)instance, value);

    } else if (!strcmp(key, "monophonic")) {

        return y_synth_handle_monophonic((y_synth_t *)instance, value);

    } else if (!strcmp(key, "glide")) {

        return y_synth_handle_glide((y_synth_t *)instance, value);

    } else if (!strcmp(key, "program_cancel")) {

        return y_synth_handle_program_cancel((y_synth_t *)instance, value);

    } else if (!strcmp(key, DSSI_PROJECT_DIRECTORY_KEY)) {

        return y_synth_handle_project_dir((y_synth_t *)instance, value);

    }
    return strdup("error: unrecognized configure key");
}

/*
 * y_get_program
 *
 * implements DSSI (*get_program)()
 */
const DSSI_Program_Descriptor *
y_get_program(LADSPA_Handle instance, unsigned long index)
{
    y_synth_t *synth = (y_synth_t *)instance;
    static DSSI_Program_Descriptor pd;

    YDB_MESSAGE(YDB_DSSI, " y_get_program called with %lu\n", index);

    if (index < synth->patch_count) {
        y_synth_set_program_descriptor(synth, &pd, index);
        pd.Bank = index / 128;
        pd.Program = index % 128;
        return &pd;
    }
    return NULL;
}

/*
 * y_select_program
 *
 * implements DSSI (*select_program)()
 */
void
y_select_program(LADSPA_Handle handle, unsigned long bank,
                      unsigned long program)
{
    y_synth_t *synth = (y_synth_t *)handle;

    YDB_MESSAGE(YDB_DSSI, " y_select_program called with %lu and %lu\n", bank, program);

    /* ignore invalid program requests */
    if (program >= 128)
        return;
    program = bank * 128 + program;
    if (program >= synth->patch_count)
        return;

    /* Attempt the patch mutex, return if lock fails. */
    if (pthread_mutex_trylock(&synth->patches_mutex)) {
        synth->pending_patch_change = program;
        return;
    }

    y_synth_select_patch(synth, program);

    pthread_mutex_unlock(&synth->patches_mutex);
}

/*
 * dssp_handle_pending_patch_change
 */
static inline void
dssp_handle_pending_patch_change(y_synth_t *synth)
{
    /* Attempt the patch mutex, return if lock fails. */
    if (pthread_mutex_trylock(&synth->patches_mutex))
        return;

    if (synth->pending_patch_change < synth->patch_count)
        y_synth_select_patch(synth, synth->pending_patch_change);
    synth->pending_patch_change = -1;

    pthread_mutex_unlock(&synth->patches_mutex);
}

/*
 * y_get_midi_controller
 *
 * implements DSSI (*get_midi_controller_for_port)()
 */
int
y_get_midi_controller(LADSPA_Handle instance, unsigned long port)
{
    YDB_MESSAGE(YDB_DSSI, " y_get_midi_controller called for port %lu\n", port);
    switch (port) {
      case Y_PORT_GLIDE_TIME:
        return DSSI_CC(MIDI_CTL_MSB_PORTAMENTO_TIME);
#ifdef USE_GARETTS_CONTROLLER_ASSIGNMENTS
      /* Ideally, MIDI controller assignments will someday be run-time
       * configurable.  Until then, you have to roll your own.  Here's
       * a setup that Garett Shulman uses with a Doepfer Pocket Fader
       * to sort-of emulate a Juno 106: */
      case Y_PORT_OSC1_LEVEL_A:
        return DSSI_CC(0x30);  /* General Purpose Controller #1 */
      case Y_PORT_OSC2_LEVEL_A:
        return DSSI_CC(0x32);  /* General Purpose Controller #3 */
      case Y_PORT_OSC2_MPARAM2:
        return DSSI_CC(0x33);  /* General Purpose Controller #4 */
      case Y_PORT_OSC2_MMOD_AMT:
        return DSSI_CC(0x34);  /* Undefined */
      case Y_PORT_OSC3_LEVEL_A:
        return DSSI_CC(0x31);  /* General Purpose Controller #2 */
      /* case amp mod source amount for all OSC:
       *   return DSSI_CC(0x35);
       * case frequency for all OSC
       *   return DSSI_CC(0x36); */
      case Y_PORT_VCF1_FREQUENCY:
        return DSSI_CC(0x37);
      /* case Y_PORT_VCF1_FREQ_MOD_AMT:
       *   return DSSI_CC(0x37); */
      case Y_PORT_VCF1_QRES:
        return DSSI_CC(0x38);
      /* case Y_PORT_VCF1_MPARAM:
       *   return DSSI_CC(0x39); */
      case Y_PORT_GLFO_FREQUENCY:
        return DSSI_CC(0x3b);
      case Y_PORT_EG1_TIME1:
        return DSSI_CC(0x3c);
      case Y_PORT_EG1_TIME3:
        return DSSI_CC(0x3d);
      case Y_PORT_EG1_LEVEL3:
        return DSSI_CC(0x3e);
      case Y_PORT_EG1_TIME4:
        return DSSI_CC(0x3f);
      case Y_PORT_MODMIX_MOD1_AMT:
        return DSSI_CC(0x39);
      case Y_PORT_MODMIX_MOD2_AMT:
        return DSSI_CC(0x3a);
#endif
      default:
        break;
    }

    return DSSI_NONE;
}

/*
 * y_handle_event
 */
static inline void
y_handle_event(y_synth_t *synth, snd_seq_event_t *event)
{
    YDB_MESSAGE(YDB_DSSI, " y_handle_event called with event type %d\n", event->type);

    switch (event->type) {
      case SND_SEQ_EVENT_NOTEOFF:
        y_synth_note_off(synth, event->data.note.note, event->data.note.velocity);
        break;
      case SND_SEQ_EVENT_NOTEON:
        if (event->data.note.velocity > 0)
           y_synth_note_on(synth, event->data.note.note, event->data.note.velocity);
        else
           y_synth_note_off(synth, event->data.note.note, 64); /* shouldn't happen, but... */
        break;
      case SND_SEQ_EVENT_KEYPRESS:
        y_synth_key_pressure(synth, event->data.note.note, event->data.note.velocity);
        break;
      case SND_SEQ_EVENT_CONTROLLER:
        y_synth_control_change(synth, event->data.control.param, event->data.control.value);
        break;
      case SND_SEQ_EVENT_CHANPRESS:
        y_synth_channel_pressure(synth, event->data.control.value);
        break;
      case SND_SEQ_EVENT_PITCHBEND:
        y_synth_pitch_bend(synth, event->data.control.value);
        break;
      /* SND_SEQ_EVENT_PGMCHANGE - shouldn't happen */
      /* SND_SEQ_EVENT_SYSEX - shouldn't happen */
      /* SND_SEQ_EVENT_CONTROL14? */
      /* SND_SEQ_EVENT_NONREGPARAM? */
      /* SND_SEQ_EVENT_REGPARAM? */
      default:
        break;
    }
}

/*
 * y_run_synth
 *
 * implements DSSI (*run_synth)()
 */
static void
y_run_synth(LADSPA_Handle instance, unsigned long sample_count,
                 snd_seq_event_t *events, unsigned long event_count)
{
    y_synth_t *synth = (y_synth_t *)instance;
    unsigned long samples_done = 0;
    unsigned long event_index = 0;
    unsigned long burst_size;

    /* attempt the mutex, return only silence if lock fails. */
    if (dssp_voicelist_mutex_trylock(synth)) {
        memset(synth->output_left,  0, sizeof(LADSPA_Data) * sample_count);
        memset(synth->output_right, 0, sizeof(LADSPA_Data) * sample_count);
        return;
    }

    if (synth->pending_patch_change > -1)
        dssp_handle_pending_patch_change(synth);

    while (samples_done < sample_count) {
        if (!synth->control_remains)
            synth->control_remains = Y_CONTROL_PERIOD;

        /* process any ready events */
	while (event_index < event_count
	       && samples_done == events[event_index].time.tick) {
            y_handle_event(synth, &events[event_index]);
            event_index++;
        }

        /* calculate the sample count (burst_size) for the next
         * y_voice_render() call to be the smallest of:
         * - control calculation quantization size (Y_CONTROL_PERIOD, in
         *     samples)
         * - the number of samples remaining in an already-begun control cycle
         *     (synth->control_remains)
         * - the number of samples until the next event is ready
         * - the number of samples left in this run
         */
        burst_size = Y_CONTROL_PERIOD;
        if (synth->control_remains < burst_size) {
            /* we're still in the middle of a control cycle, so reduce the
             * burst size to end when the cycle ends */
            burst_size = synth->control_remains;
        }
        if (event_index < event_count
            && events[event_index].time.tick - samples_done < burst_size) {
            /* reduce burst size to end when next event is ready */
            burst_size = events[event_index].time.tick - samples_done;
        }
        if (sample_count - samples_done < burst_size) {
            /* reduce burst size to end at end of this run */
            burst_size = sample_count - samples_done;
        }

        /* render the burst */
        y_synth_render_voices(synth, synth->output_left + samples_done,
                                   synth->output_right + samples_done, burst_size,
                                   (burst_size == synth->control_remains));
        samples_done += burst_size;
        synth->control_remains -= burst_size;
    }
#if defined(Y_DEBUG) && (Y_DEBUG & YDB_AUDIO)
*synth->output_left  += 0.10f; /* add a 'buzz' to output so there's something audible even when quiescent */
*synth->output_right += 0.10f;
#endif /* defined(Y_DEBUG) && (Y_DEBUG & YDB_AUDIO) */

    dssp_voicelist_mutex_unlock(synth);
}

// optional:
//    void (*run_synth_adding)(LADSPA_Handle    Instance,
//                             unsigned long    SampleCount,
//                             snd_seq_event_t *Events,
//                             unsigned long    EventCount);

/* ---- export ---- */

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
        return y_LADSPA_descriptor;
    default:
        return NULL;
    }
}

const DSSI_Descriptor *dssi_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
        return y_DSSI_descriptor;
    default:
        return NULL;
    }
}

#ifdef __GNUC__
__attribute__((constructor)) void init()
#else
void _init()
#endif
{
    int i;
    char **port_names;
    LADSPA_PortDescriptor *port_descriptors;
    LADSPA_PortRangeHint *port_range_hints;

    Y_DEBUG_INIT("whysynth.so");

    pthread_mutex_init(&global_mutex, NULL);
    global.initialized = 0;
    y_init_tables();
    wave_tables_set_count();

    y_LADSPA_descriptor =
        (LADSPA_Descriptor *) malloc(sizeof(LADSPA_Descriptor));
    if (y_LADSPA_descriptor) {
        y_LADSPA_descriptor->UniqueID = 2187;
        y_LADSPA_descriptor->Label = "WhySynth";
        y_LADSPA_descriptor->Properties = 0;
#ifdef HAVE_CONFIG_H
        y_LADSPA_descriptor->Name = "WhySynth " VERSION " DSSI plugin";
#else
        y_LADSPA_descriptor->Name = "WhySynth DSSI plugin";
#endif
        y_LADSPA_descriptor->Maker = "Sean Bolton <musound AT jps DOT net>";
        y_LADSPA_descriptor->Copyright = "GNU General Public License version 2 or later";
        y_LADSPA_descriptor->PortCount = Y_PORTS_COUNT;

        port_descriptors = (LADSPA_PortDescriptor *)
                                calloc(y_LADSPA_descriptor->PortCount, sizeof
                                                (LADSPA_PortDescriptor));
        y_LADSPA_descriptor->PortDescriptors =
            (const LADSPA_PortDescriptor *) port_descriptors;

        port_range_hints = (LADSPA_PortRangeHint *)
                                calloc(y_LADSPA_descriptor->PortCount, sizeof
                                                (LADSPA_PortRangeHint));
        y_LADSPA_descriptor->PortRangeHints =
            (const LADSPA_PortRangeHint *) port_range_hints;

        port_names = (char **) calloc(y_LADSPA_descriptor->PortCount, sizeof(char *));
        y_LADSPA_descriptor->PortNames = (const char **) port_names;

        for (i = 0; i < Y_PORTS_COUNT; i++) {
            port_descriptors[i] = y_port_description[i].port_descriptor;
            port_names[i]       = y_port_description[i].name;
            port_range_hints[i].HintDescriptor = y_port_description[i].hint_descriptor;
            port_range_hints[i].LowerBound     = y_port_description[i].lower_bound;
            if (y_port_description[i].type == Y_PORT_TYPE_COMBO &&
                (y_port_description[i].subtype == Y_COMBO_TYPE_OSC_WAVEFORM ||
                 y_port_description[i].subtype == Y_COMBO_TYPE_WT_WAVEFORM)) {
                port_range_hints[i].UpperBound = (float)wavetables_count - 1;
            } else {
                port_range_hints[i].UpperBound = y_port_description[i].upper_bound;
            }
        }

        y_LADSPA_descriptor->instantiate = y_instantiate;
        y_LADSPA_descriptor->connect_port = y_connect_port;
        y_LADSPA_descriptor->activate = y_activate;
        y_LADSPA_descriptor->run = y_ladspa_run_wrapper;
        y_LADSPA_descriptor->run_adding = NULL;
        y_LADSPA_descriptor->set_run_adding_gain = NULL;
        y_LADSPA_descriptor->deactivate = y_deactivate;
        y_LADSPA_descriptor->cleanup = y_cleanup;
    }

    y_DSSI_descriptor = (DSSI_Descriptor *) malloc(sizeof(DSSI_Descriptor));
    if (y_DSSI_descriptor) {
        y_DSSI_descriptor->DSSI_API_Version = 1;
        y_DSSI_descriptor->LADSPA_Plugin = y_LADSPA_descriptor;
        y_DSSI_descriptor->configure = y_configure;
        y_DSSI_descriptor->get_program = y_get_program;
        y_DSSI_descriptor->select_program = y_select_program;
        y_DSSI_descriptor->get_midi_controller_for_port = y_get_midi_controller;
        y_DSSI_descriptor->run_synth = y_run_synth;
        y_DSSI_descriptor->run_synth_adding = NULL;
        y_DSSI_descriptor->run_multiple_synths = NULL;
        y_DSSI_descriptor->run_multiple_synths_adding = NULL;
    }
}

#ifdef __GNUC__
__attribute__((destructor)) void fini()
#else
void _fini()
#endif
{
    if (y_LADSPA_descriptor) {
        free((LADSPA_PortDescriptor *) y_LADSPA_descriptor->PortDescriptors);
        free((char **) y_LADSPA_descriptor->PortNames);
        free((LADSPA_PortRangeHint *) y_LADSPA_descriptor->PortRangeHints);
        free(y_LADSPA_descriptor);
    }
    if (y_DSSI_descriptor) {
        free(y_DSSI_descriptor);
    }
}

