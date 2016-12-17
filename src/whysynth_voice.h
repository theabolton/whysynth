/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004-2008, 2010, 2016 Sean Bolton and others.
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

#ifndef _WHYSYNTH_VOICE_H
#define _WHYSYNTH_VOICE_H

#define _DEFAULT_SOURCE 1
#define _ISOC99_SOURCE  1

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include <dssi.h>

#include "whysynth_types.h"
#include "whysynth_ports.h"

/* control-calculation period, in samples; also the maximum size of a rendering
 * burst: */
#define Y_CONTROL_PERIOD        64

/* minBLEP constants */
/* minBLEP table oversampling factor (must be a power of two): */
#define MINBLEP_PHASES          64
/* MINBLEP_PHASES minus one: */
#define MINBLEP_PHASE_MASK      63
/* length in samples of (truncated) discontinuity deltas: */
#define DD_PULSE_LENGTH         64
/* delay between start of DD pulse and the discontinuity, in samples: */
#define DD_SAMPLE_DELAY          4
/* OSC_BUS_LENGTH must be a power of two, equal to or greater than
 * Y_CONTROL_PERIOD plus DD_PULSE_LENGTH: */
#define OSC_BUS_LENGTH         128
#define OSC_BUS_MASK          (OSC_BUS_LENGTH - 1)

/* Length of sine wave table for FM and waveshaper oscillators (must be
 * a power of two) */
#define SINETABLE_POINTS      1024

#define WAVETABLE_SELECT_BIAS_RANGE  60.0f

/* ==== y_patch_t ==== */

/* -PORTS- */
struct posc
{
    int    mode;
    int    waveform;
    int    pitch;
    float  detune;
    int    pitch_mod_src;
    float  pitch_mod_amt;
    float  mparam1;       /* sync / grain lz / mod freq ratio / ws phase offset / slave freq / noise cutoff freq */
    float  mparam2;       /* pulsewidth / wave sel bias / grain spread / mod freq detune / mod amount bias */
    int    mmod_src;      /* (pw | mod index | slave freq ) mod source / grain envelope */
    float  mmod_amt;      /* (pw | mod index | slave freq ) mod amount / grain pitch distribution */
    int    amp_mod_src;
    float  amp_mod_amt;
    float  level_a;
    float  level_b;
};

struct pvcf
{
    int    mode;
    int    source;
    float  frequency;
    int    freq_mod_src;
    float  freq_mod_amt;
    float  qres;
    float  mparam;
};

struct plfo
{
    float  frequency;
    int    waveform;
    float  delay;
    int    amp_mod_src;
    float  amp_mod_amt;
};

struct peg
{
    int    mode;
    int    shape1;
    float  time1;
    float  level1;
    int    shape2;
    float  time2;
    float  level2;
    int    shape3;
    float  time3;
    float  level3;
    int    shape4;
    float  time4;
    float  vel_level_sens;
    float  vel_time_scale;
    float  kbd_time_scale;
    int    amp_mod_src;
    float  amp_mod_amt;
};

struct _y_patch_t
{
    char          name[31];
    char          comment[61];

    struct posc   osc1,
                  osc2,
                  osc3,
                  osc4;
    struct pvcf   vcf1,
                  vcf2;

    float         busa_level;
    float         busa_pan;
    float         busb_level;
    float         busb_pan;
    float         vcf1_level;
    float         vcf1_pan;
    float         vcf2_level;
    float         vcf2_pan;
    float         volume;

    int           effect_mode;
    float         effect_param1;
    float         effect_param2;
    float         effect_param3;
    float         effect_param4;
    float         effect_param5;
    float         effect_param6;
    float         effect_mix;

    float         glide_time;
    int           bend_range;

    struct plfo   glfo,
                  vlfo,
                  mlfo;
    float         mlfo_phase_spread;
    float         mlfo_random_freq;

    struct peg    ego,
                  eg1,
                  eg2,
                  eg3,
                  eg4;

    float         modmix_bias;
    int           modmix_mod1_src;
    float         modmix_mod1_amt;
    int           modmix_mod2_src;
    float         modmix_mod2_amt;
};

/* ==== y_voice_t ==== */

enum y_voice_status
{
    Y_VOICE_OFF,       /* silent: is not processed by render loop */
    Y_VOICE_ON,        /* has not received a note off event */
    Y_VOICE_SUSTAINED, /* has received note off, but sustain controller is on */
    Y_VOICE_RELEASED   /* had note off, not sustained, in final decay phase of envelopes */
};

/* -PORTS- */
struct vosc
{
    /* copies of LADSPA ports, copied in each render burst */
    int           mode,        /* oscillator mode; conditioned to integer */
                  waveform;    /* waveform; conditioned to integer */

    /* persistent voice state */
    /* -- all oscillators */
    int           last_mode,
                  last_waveform;
    double        pos0,        /* wavetable phase or sample position */
                  pos1;
    /* -- wavetable, async granular, FM, waveshaper, PADsynth */
    int           wave_select_key;
    signed short *wave0,       /* pointer to lower wavetable */
                 *wave1;       /* pointer to upper wavetable */
    float         wavemix0,    /* crossfade: lower table/sample amplitude */
                  wavemix1;    /* crossfade: upper table/sample amplitude */
    /* -- async granular */
    grain_t      *grain_list;  /* active grain list */
    /* -- shared storage for miscellaneous state */
    int           i0,          /* agran next_onset; PADsynth lower sample index */
                  i1;          /* minBLEP bp_high;  PADsynth upper sample index */
    float         f0,          /* minBLEP out; noise filter state; wt chorus pos2 */
                  f1,          /* noise filter state; wt chorus pos3 */
                  f2;          /* noise filter state; wt chorus pos4 */
};

struct vvcf
{
    int   mode,
          last_mode;
    float delay1,
          delay2,
          delay3,
          delay4,
          delay5;
};

struct vlfo
{
    float pos;          /* LFO phase, 0 to 1 */
    float freqmult;     /* random frequency multiplier, 0.5 to 1.5 */
    float delay_length; /* onset delay length, in control ticks */
    int   delay_count;  /* control ticks remaining in delay */
};

enum dssp_eg_state {
    DSSP_EG_FINISHED,
    DSSP_EG_RUNNING,
    DSSP_EG_SUSTAINING
};

struct veg
{
    int   shape[4];
    int   sustain_segment;  /* 2 for ADSR or AAASR, 1 for AASRR, 0 for ASRRR, -1 for One-Shot */

    int   state;            /* enum dssp_eg_state (finished, running, sustaining) */
    int   segment;          /* 0 to 3 */
    int   count;            /* control ticks until end of this phase */
    float time_scale;       /* amount to scale envelope times due to velocity time scaling and keyboard time scaling, multiplied by control rate */
    float level_scale;      /* amount to scale envelope levels due to velocity level sensitivity */
    float target;           /* segment target level */
    float a, b, c, d;       /* segment shape function coefficients */
};

struct vmod
{
    float value;
    float next_value;  /* value at next control tick */
    float delta;
};

/*
 * y_voice_t
 */
struct _y_voice_t
{
    unsigned int  note_id;

    unsigned char status;
    unsigned char key;
    unsigned char velocity;
    unsigned char rvelocity;   /* the note-off velocity */

    /* translated controller values */
    float         pressure;

    /* persistent voice state */
    float         prev_pitch,
                  target_pitch,
                  current_pitch;
    struct vosc   osc1,
                  osc2,
                  osc3,
                  osc4;
    struct vvcf   vcf1,
                  vcf2;
    struct vlfo   /* glfo is in y_synth_t */
                  vlfo,
                  mlfo0,
                  mlfo1,
                  mlfo2,
                  mlfo3;
    struct veg    ego,
                  eg1,
                  eg2,
                  eg3,
                  eg4;
    struct vmod   mod[Y_MODS_COUNT];

    /* buffers */
    int           osc_index;                        /* shared index into osc_bus_{a,b} */
    float         osc_sync[Y_CONTROL_PERIOD];       /* buffer for sync subsample offsets */
    float         osc_bus_a[OSC_BUS_LENGTH],
                  osc_bus_b[OSC_BUS_LENGTH];
};

#define _PLAYING(voice)    ((voice)->status != Y_VOICE_OFF)
#define _ON(voice)         ((voice)->status == Y_VOICE_ON)
#define _SUSTAINED(voice)  ((voice)->status == Y_VOICE_SUSTAINED)
#define _RELEASED(voice)   ((voice)->status == Y_VOICE_RELEASED)
#define _AVAILABLE(voice)  ((voice)->status == Y_VOICE_OFF)

extern float y_pitch[129];

typedef struct { float value, delta; } float_value_delta;
extern float_value_delta y_step_dd_table[];

extern float y_slope_dd_table[];

/* in whysynth_voice.c */
extern float eg_shape_coeffs[][4];
y_voice_t *y_voice_new(y_synth_t *synth);
void       y_voice_note_on(y_synth_t *synth, y_voice_t *voice,
                           unsigned char key, unsigned char velocity);
void       y_voice_note_off(y_synth_t *synth, y_voice_t *voice,
                            unsigned char key, unsigned char rvelocity);
void       y_voice_release_note(y_synth_t *synth, y_voice_t *voice);
void       y_voice_set_ports(y_synth_t *synth, y_patch_t *patch);
void       y_voice_update_pressure_mod(y_synth_t *synth, y_voice_t *voice);

/* in whysynth_voice_render.c */
extern float sine_wave[4 + SINETABLE_POINTS + 1];
extern float volume_cv_to_amplitude_table[257];
void y_init_tables(void);
void y_voice_setup_lfo(y_synth_t *synth, y_slfo_t *slfo, struct vlfo *vlfo,
                       float phase, float randfreq,
                       struct vmod *srcmods, struct vmod *destmod);
void y_voice_update_lfo(y_synth_t *synth, y_slfo_t *slfo, struct vlfo *vlfo,
                        struct vmod *srcmods, struct vmod *destmod);
void y_voice_render(y_synth_t *synth, y_voice_t *voice,
                    LADSPA_Data *out_left, LADSPA_Data *out_right,
                    unsigned long sample_count, int do_control_update);

/* in agran_oscillator.c */
void free_active_grains(y_synth_t *synth, y_voice_t *voice);

/* ==== inline functions ==== */

/*
 * y_voice_off
 * 
 * Purpose: Turns off a voice immediately, meaning that it is not processed
 * anymore by the render loop.
 */
static inline void
y_voice_off(y_synth_t *synth, y_voice_t* voice)
{
    voice->status = Y_VOICE_OFF;

    /* silence the oscillator buffers for the next use */
    memset(voice->osc_bus_a, 0, OSC_BUS_LENGTH * sizeof(float));
    memset(voice->osc_bus_b, 0, OSC_BUS_LENGTH * sizeof(float));

    /* free any still-active grains */
    if (voice->osc1.grain_list || voice->osc2.grain_list ||
        voice->osc3.grain_list || voice->osc4.grain_list)
        free_active_grains(synth, voice);

    /* -FIX- decrement active voice count? */
}

/*
 * y_voice_start_voice
 */
static inline void
y_voice_start_voice(y_voice_t *voice)
{
    voice->status = Y_VOICE_ON;
    /* -FIX- increment active voice count? */
}

#endif /* _WHYSYNTH_VOICE_H */

