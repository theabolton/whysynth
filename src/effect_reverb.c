/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2005, 2010 Sean Bolton and others.
 *
 * Nearly all of the Plate reverb code comes from the Plate2x2
 * reverb in CAPS 0.2.3, copyright (c) 2002-4 Tim Goetze.
 * The Dual Delay is original, using Tim's delay and filter
 * objects.
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

#include <string.h>
#include <math.h>

#include <ladspa.h>

#include "whysynth_types.h"
#include "dssp_event.h"
#include "effects.h"
#include "effect_reverb.h"

/* ==== Plate Reverb ==== */

static float _Plate_l[] = {
    0.004771345048889486, 0.0035953092974026408, 
    0.01273478713752898, 0.0093074829474816042, 
    0.022579886428547427, 0.030509727495715868, 
    0.14962534861059779, 0.060481838647894894, 0.12499579987231611, 
    0.14169550754342933, 0.089244313027116023, 0.10628003091293972
};

static float _Plate_t[] = {
    0.0089378717113000241, 0.099929437854910791, 0.064278754074123853, 
    0.067067638856221232, 0.066866032727394914, 0.006283391015086859, 
    0.01186116057928161, 0.12187090487550822, 0.041262054366452743, 
    0.089815530392123921, 0.070931756325392295, 0.011256342192802662
};

void
effect_reverb_request_buffers(y_synth_t *synth)
{
    struct Plate *plate = (struct Plate *)effects_request_buffer(synth, sizeof(struct Plate));
    int i;

    memset(plate, 0, sizeof(struct Plate));

    plate->fs = (double)synth->sample_rate;

    effects_request_silencing_of_subsequent_allocations(synth);

#   define L(i) ((int) (_Plate_l[i] * plate->fs))
    /* lh */
    Delay_init(&plate->input.lattice[0], synth, L(0));
    Delay_init(&plate->input.lattice[1], synth, L(1));
    
    /* rh */
    Delay_init(&plate->input.lattice[2], synth, L(2));
    Delay_init(&plate->input.lattice[3], synth, L(3));

    /* modulated, width about 12 samples @ 44.1 */
    ModLattice_init(&plate->tank.mlattice[0], synth, L(4), (int) (0.00040322707570310132 * plate->fs));
    ModLattice_init(&plate->tank.mlattice[1], synth, L(5), (int) (0.00040322707570310132 * plate->fs));

    /* lh */
    Delay_init(&plate->tank.delay[0],   synth, L(6));
    Delay_init(&plate->tank.lattice[0], synth, L(7));
    Delay_init(&plate->tank.delay[1],   synth, L(8));

    /* rh */
    Delay_init(&plate->tank.delay[2],   synth, L(9));
    Delay_init(&plate->tank.lattice[1], synth, L(10));
    Delay_init(&plate->tank.delay[3],   synth, L(11));
#   undef L

#   define T(i) ((int) (_Plate_t[i] * plate->fs))

    for (i = 0; i < 12; ++i)
        plate->tank.taps[i] = T(i);
#   undef T
    
    /* tuned for soft attack, ambience */
    plate->indiff1 = .742;
    plate->indiff2 = .712;

    plate->dediff1 = .723;
    plate->dediff2 = .729;
}

void effect_reverb_setup(y_synth_t *synth)
{
    struct Plate *plate = (struct Plate *)synth->effect_buffer;
    int i;

    OnePoleLP_reset(&plate->input.bandwidth);

    for (i = 0; i < 4; ++i)
    {
        Delay_reset(&plate->input.lattice[i]);
        Delay_reset(&plate->tank.delay[i]);
    }

    for (i = 0; i < 2; ++i)
    {
        ModLattice_reset(&plate->tank.mlattice[i]);
        Delay_reset(&plate->tank.lattice[i]);
        OnePoleLP_reset(&plate->tank.damping[i]);
    }
    
    Sine_set_f3(&plate->tank.mlattice[0].lfo, 1.2, plate->fs, 0);
    Sine_set_f3(&plate->tank.mlattice[1].lfo, 1.2, plate->fs, .5 * M_PI);
}

static inline void
Plate_process (struct Plate *plate, float x, float decay, float * _xl, float * _xr)
{
    register float xl, xr;

    x = OnePoleLP_process(&plate->input.bandwidth, x);

    /* lh */
    x = Lattice_process(&plate->input.lattice[0], x, plate->indiff1);
    x = Lattice_process(&plate->input.lattice[1], x, plate->indiff1);
    
    /* rh */
    x = Lattice_process(&plate->input.lattice[2], x, plate->indiff2);
    x = Lattice_process(&plate->input.lattice[3], x, plate->indiff2);

    /* summation point */
    xl = x + decay * Delay_get(&plate->tank.delay[3]);
    xr = x + decay * Delay_get(&plate->tank.delay[1]);

    /* lh */
    xl = ModLattice_process(&plate->tank.mlattice[0], xl, plate->dediff1);
    xl = Delay_putget(&plate->tank.delay[0], xl);
    xl = OnePoleLP_process(&plate->tank.damping[0], xl);
    xl *= decay;
    xl = Lattice_process(&plate->tank.lattice[0], xl, plate->dediff2);
    Delay_put(&plate->tank.delay[1], xl);

    /* rh */
    xr = ModLattice_process(&plate->tank.mlattice[1], xr, plate->dediff1);
    xr = Delay_putget(&plate->tank.delay[2], xr);
    xr = OnePoleLP_process(&plate->tank.damping[1], xr);
    xr *= decay;
    xr = Lattice_process(&plate->tank.lattice[1], xr, plate->dediff2);
    Delay_put(&plate->tank.delay[3], xr);

    /* gather output */
    xl  = .6 * Delay_peek(&plate->tank.delay[2], plate->tank.taps[0]);
    xl += .6 * Delay_peek(&plate->tank.delay[2], plate->tank.taps[1]);
    xl -= .6 * Delay_peek(&plate->tank.lattice[1], plate->tank.taps[2]);
    xl += .6 * Delay_peek(&plate->tank.delay[3], plate->tank.taps[3]);
    xl -= .6 * Delay_peek(&plate->tank.delay[0], plate->tank.taps[4]);
    xl += .6 * Delay_peek(&plate->tank.lattice[0], plate->tank.taps[5]);

    xr  = .6 * Delay_peek(&plate->tank.delay[0], plate->tank.taps[6]);
    xr += .6 * Delay_peek(&plate->tank.delay[0], plate->tank.taps[7]);
    xr -= .6 * Delay_peek(&plate->tank.lattice[0], plate->tank.taps[8]);
    xr += .6 * Delay_peek(&plate->tank.delay[1], plate->tank.taps[9]);
    xr -= .6 * Delay_peek(&plate->tank.delay[2], plate->tank.taps[10]);
    xr += .6 * Delay_peek(&plate->tank.lattice[1], plate->tank.taps[11]);

    *_xl = xl;
    *_xr = xr;
}

void
effect_reverb_process(y_synth_t *synth, unsigned long frames,
                      LADSPA_Data *out_left, LADSPA_Data *out_right)
{
    struct Plate *plate = (struct Plate *)synth->effect_buffer;
    double d, damp;
    float decay, blend, dry;
    int i;

    blend = *(synth->effect_mix);
    dry = 1.0f - blend;

    /* originally:
     *   d = *(synth->effect_param4) * 0.994f + 0.005f;
     *   OnePoleLP_set(&plate->input.bandwidth, exp(-M_PI * (1. - d)));
     * a quick approximation of the above: */
    d = *(synth->effect_param4);
    d = ((1.26595 * d - 0.614577) * d + 0.305691) * d + 0.0422856;
    OnePoleLP_set(&plate->input.bandwidth, d);                       /* "bandwidth" */

    decay = *(synth->effect_param5) * 0.749f;                        /* "tail" */

    d = *(synth->effect_param6) * 0.9995f + 0.0005f;
    damp = exp(-M_PI * d);                                           /* "damping" */
    OnePoleLP_set(&plate->tank.damping[0], damp);
    OnePoleLP_set(&plate->tank.damping[1], damp);

    for (i = 0; i < frames; ++i)
    {
        float sl, sr, x, xl, xr;

        /* DC blocker */
        sl = synth->voice_bus_l[i] - synth->dc_block_l_xnm1 +
                 synth->dc_block_r * synth->dc_block_l_ynm1;
        synth->dc_block_l_ynm1 = sl;
        synth->dc_block_l_xnm1 = synth->voice_bus_l[i];
        sr = synth->voice_bus_r[i] - synth->dc_block_r_xnm1 +
                 synth->dc_block_r * synth->dc_block_r_ynm1;
        synth->dc_block_r_ynm1 = sr;
        synth->dc_block_r_xnm1 = synth->voice_bus_r[i];

        /* Plate2x2 reverb */
        x = (sl + sr) * 0.5f;

        Plate_process (plate, x, decay, &xl, &xr);

        out_left[i]  = blend * xl + dry * sl;
        out_right[i] = blend * xr + dry * sr;
    }
}

/* ==== Dual Delay ==== */

void
effect_delay_request_buffers(y_synth_t *synth)
{
    struct DualDelay *delay = (struct DualDelay *)effects_request_buffer(synth, sizeof(struct DualDelay));

    memset(delay, 0, sizeof(struct DualDelay));

    delay->max_delay = lrintf(2.0f * synth->sample_rate);

    effects_request_silencing_of_subsequent_allocations(synth);

    /* !FIX! buffers actually allocated are next power of two, which wastes a good chunk of memory
     * (like 334k to 548k; it's okay for the reverb because the buffers are relatively small, but
     * here they're much larger.) */
    Delay_init(&delay->delay_l, synth, delay->max_delay);
    Delay_init(&delay->delay_r, synth, delay->max_delay);
}

void effect_delay_setup(y_synth_t *synth)
{
    struct DualDelay *delay = (struct DualDelay *)synth->effect_buffer;

    Delay_reset(&delay->delay_l);
    Delay_reset(&delay->delay_r);
    OnePoleLP_reset(&delay->damping_l);
    OnePoleLP_reset(&delay->damping_r);
}

void
effect_delay_process(y_synth_t *synth, unsigned long frames,
                     LADSPA_Data *out_left, LADSPA_Data *out_right)
{
    struct DualDelay *delay = (struct DualDelay *)synth->effect_buffer;
    float wet, dry, fb, fa, fia, damping;
    int i, delay_l, delay_r;

    wet = *(synth->effect_mix);
    dry = 1.0f - wet;

    fb = *(synth->effect_param2);
    fa = *(synth->effect_param3);
    fia = 1.0f - fa;

    delay_l = lrintf(*(synth->effect_param4) * 2.0f * synth->sample_rate);
    if (delay_l < 1) delay_l = 1;
    else if (delay_l > delay->max_delay) delay_l = delay->max_delay;
    delay_r = lrintf(*(synth->effect_param5) * 2.0f * synth->sample_rate);
    if (delay_r < 1) delay_r = 1;
    else if (delay_r > delay->max_delay) delay_r = delay->max_delay;

    damping = *(synth->effect_param6);
    if (damping < 1e-3) {

        for (i = 0; i < frames; ++i) {
            float sl, sr, il, ir, xl, xr;

            /* DC blocker */
            sl = synth->voice_bus_l[i] - synth->dc_block_l_xnm1 +
                     synth->dc_block_r * synth->dc_block_l_ynm1;
            synth->dc_block_l_ynm1 = sl;
            synth->dc_block_l_xnm1 = synth->voice_bus_l[i];
            sr = synth->voice_bus_r[i] - synth->dc_block_r_xnm1 +
                     synth->dc_block_r * synth->dc_block_r_ynm1;
            synth->dc_block_r_ynm1 = sr;
            synth->dc_block_r_xnm1 = synth->voice_bus_r[i];

            /* Dual delay */
            xl = Delay_peek(&delay->delay_l, delay_l);
            xr = Delay_peek(&delay->delay_r, delay_r);
            il = sl + xl * fb;
            ir = sr + xr * fb;
            Delay_put(&delay->delay_l, fia * il + fa * ir);
            Delay_put(&delay->delay_r, fia * ir + fa * il);

            out_left[i]  = wet * xl + dry * sl;
            out_right[i] = wet * xr + dry * sr;
        }
    } else {

        damping = exp(-M_PI * (damping * 0.9995f + 0.0005f));
        OnePoleLP_set(&delay->damping_l, damping);
        OnePoleLP_set(&delay->damping_r, damping);

        for (i = 0; i < frames; ++i) {
            float sl, sr, il, ir, xl, xr;

            /* DC blocker */
            sl = synth->voice_bus_l[i] - synth->dc_block_l_xnm1 +
                     synth->dc_block_r * synth->dc_block_l_ynm1;
            synth->dc_block_l_ynm1 = sl;
            synth->dc_block_l_xnm1 = synth->voice_bus_l[i];
            sr = synth->voice_bus_r[i] - synth->dc_block_r_xnm1 +
                     synth->dc_block_r * synth->dc_block_r_ynm1;
            synth->dc_block_r_ynm1 = sr;
            synth->dc_block_r_xnm1 = synth->voice_bus_r[i];

            /* Dual delay */
            xl = Delay_peek(&delay->delay_l, delay_l);
            xr = Delay_peek(&delay->delay_r, delay_r);
            il = sl + xl * fb;
            ir = sr + xr * fb;
            il = OnePoleLP_process(&delay->damping_l, il);
            ir = OnePoleLP_process(&delay->damping_r, ir);
            Delay_put(&delay->delay_l, fia * il + fa * ir);
            Delay_put(&delay->delay_r, fia * ir + fa * il);

            out_left[i]  = wet * xl + dry * sl;
            out_right[i] = wet * xr + dry * sr;
        }
    }
}

