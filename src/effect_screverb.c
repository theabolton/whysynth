/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2008, 2010, 2016 Sean Bolton and others.
 *
 * Nearly all of this code comes from the file reverbsc.c in
 * Csound 5.08, copyright (c) 2005 Istvan Varga. Comments from
 * the original source:
 *
 *     8 delay line FDN reverb, with feedback matrix based upon
 *     physical modeling scattering junction of 8 lossless waveguides
 *     of equal characteristic impedance. Based on Julius O. Smith III,
 *     "A New Approach to Digital Reverberation using Closed Waveguide
 *     Networks," Proceedings of the International Computer Music
 *     Conference 1985, p. 47-53 (also available as a seperate
 *     publication from CCRMA), as well as some more recent papers by
 *     Smith and others.
 *
 *     Csound orchestra version coded by Sean Costello, October 1999
 *
 *     C implementation (C) 2005 Istvan Varga
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

#include <math.h>

#include <ladspa.h>

#include "whysynth_types.h"
#include "dssp_event.h"
#include "effects.h"

#define DEFAULT_SRATE   44100.0
// #define MIN_SRATE       5000.0
// #define MAX_SRATE       1000000.0
// #define MAX_PITCHMOD    20.0
#define DELAYPOS_SHIFT  28
#define DELAYPOS_SCALE  0x10000000
#define DELAYPOS_MASK   0x0FFFFFFF

/* reverbParams[n][0] = delay time (in seconds)                     */
/* reverbParams[n][1] = random variation in delay time (in seconds) */
/* reverbParams[n][2] = random variation frequency (in 1/sec)       */
/* reverbParams[n][3] = random seed (0 - 32767)                     */

static const double reverbParams[8][4] = {
    { (2473.0 / DEFAULT_SRATE), 0.0010, 3.100,  1966.0 },
    { (2767.0 / DEFAULT_SRATE), 0.0011, 3.500, 29491.0 },
    { (3217.0 / DEFAULT_SRATE), 0.0017, 1.110, 22937.0 },
    { (3557.0 / DEFAULT_SRATE), 0.0006, 3.973,  9830.0 },
    { (3907.0 / DEFAULT_SRATE), 0.0010, 2.341, 20643.0 },
    { (4127.0 / DEFAULT_SRATE), 0.0011, 1.897, 22937.0 },
    { (2143.0 / DEFAULT_SRATE), 0.0017, 0.891, 29491.0 },
    { (1933.0 / DEFAULT_SRATE), 0.0006, 3.221, 14417.0 }
};

static const double outputGain  = 0.35;
static const double jpScale     = 0.25;

typedef struct {
    int         writePos;
    int         bufferSize;
    int         readPos;
    int         readPosFrac;
    int         readPosFrac_inc;
    int         seedVal;
    int         randLine_cnt;
    double      filterState;
    float       *buf;
} delayLine;

typedef struct {
    double     dampFact;
    float      prv_LPFreq;
    delayLine  delayLines[8];
} SC_REVERB;

static int
delay_line_max_samples(y_synth_t *synth, int n)
{
    double  maxDel;

    maxDel = reverbParams[n][0];
    /* was: maxDel += (reverbParams[n][1] * (double) *(p->iPitchMod) * 1.125); */
    maxDel += (reverbParams[n][1] * 10.0 * 1.125);
    return (int) (maxDel * (double) synth->sample_rate + 16.5);
}

static double
pitch_mod(y_synth_t *synth)
{
    double m = (double) *(synth->effect_param6);

    if (m < 0.8)
        return (m / 0.8);
    else
        return 1.0 + (m - 0.8) * 45.0;
}

static void
next_random_lineseg(y_synth_t *synth, SC_REVERB *p, delayLine *lp, int n)
{
    double  prvDel, nxtDel, phs_incVal;

    /* update random seed */
    if (lp->seedVal < 0)
        lp->seedVal += 0x10000;
    lp->seedVal = (lp->seedVal * 15625 + 1) & 0xFFFF;
    if (lp->seedVal >= 0x8000)
        lp->seedVal -= 0x10000;
    /* length of next segment in samples */
    lp->randLine_cnt = (int) (((double)synth->sample_rate / reverbParams[n][2]) + 0.5);
    prvDel = (double) lp->writePos;
    prvDel -= ((double) lp->readPos
               + ((double) lp->readPosFrac / (double) DELAYPOS_SCALE));
    while (prvDel < 0.0)
        prvDel += (double) lp->bufferSize;
    prvDel = prvDel / (double)synth->sample_rate;    /* previous delay time in seconds */
    nxtDel = (double) lp->seedVal * reverbParams[n][1] / 32768.0;
    /* next delay time in seconds */
    /* was: nxtDel = reverbParams[n][0] + (nxtDel * (double) *(p->iPitchMod)); */
    nxtDel = reverbParams[n][0] + (nxtDel * pitch_mod(synth));
    /* calculate phase increment per sample */
    phs_incVal = (prvDel - nxtDel) / (double) lp->randLine_cnt;
    phs_incVal = phs_incVal * (double)synth->sample_rate + 1.0;
    lp->readPosFrac_inc = (int) (phs_incVal * DELAYPOS_SCALE + 0.5);
}

static void
init_delay_line(y_synth_t *synth, SC_REVERB *p, delayLine *lp, int n)
{
    double  readPos;

    /* delay line bufferSize and buffer address already set up */
    lp->writePos = 0;
    /* set random seed */
    lp->seedVal = (int) (reverbParams[n][3] + 0.5);
    /* set initial delay time */
    readPos = (double) lp->seedVal * reverbParams[n][1] / 32768.0;
    /* was: readPos = reverbParams[n][0] + (readPos * (double) *(p->iPitchMod)); */
    readPos = reverbParams[n][0] + (readPos * pitch_mod(synth));
    readPos = (double) lp->bufferSize - (readPos * (double)synth->sample_rate);
    lp->readPos = (int) readPos;
    readPos = (readPos - (double) lp->readPos) * (double) DELAYPOS_SCALE;
    lp->readPosFrac = (int) (readPos + 0.5);
    /* initialise first random line segment */
    next_random_lineseg(synth, p, lp, n);
    /* delay line already cleared to zero */
}

void
effect_screverb_request_buffers(y_synth_t *synth)
{
    SC_REVERB *p = (SC_REVERB *)effects_request_buffer(synth, sizeof(SC_REVERB));
    int i, nBytes;

    memset(p, 0, sizeof(SC_REVERB));

    effects_request_silencing_of_subsequent_allocations(synth);

    /* calculate the number of bytes to allocate */
    nBytes = 0;
    for (i = 0; i < 8; i++) {
        p->delayLines[i].bufferSize = delay_line_max_samples(synth, i);
        nBytes = p->delayLines[i].bufferSize * sizeof(float);
        nBytes = (nBytes + 15) & (~15);
        p->delayLines[i].buf = (float *) effects_request_buffer(synth, nBytes);
    }
}

void
effect_screverb_setup(y_synth_t *synth)
{
    SC_REVERB *p = (SC_REVERB *)synth->effect_buffer;
    int i;

    /* set up delay lines */
    for (i = 0; i < 8; i++)
        init_delay_line(synth, p, &p->delayLines[i], i);
    p->dampFact = 1.0;
    p->prv_LPFreq = -1.0f;
}

void
effect_screverb_process(y_synth_t *synth, unsigned long sample_count,
                        LADSPA_Data *out_left, LADSPA_Data *out_right)
{
    SC_REVERB *p = (SC_REVERB *)synth->effect_buffer;
    float      wet, dry;
    double     ainL, ainR, aoutL, aoutR, feedback;
    double     vm1, v0, v1, v2, am1, a0, a1, a2, frac;
    delayLine *lp;
    int        i, n, readPos;

    wet = *(synth->effect_mix);
    dry = 1.0f - wet;

// *(synth->effect_param4) = kfblvl  = kFeedBack
// *(synth->effect_param5) = kfco    = kLPFreq
// *(synth->effect_param6) = ipitchm = iPitchMod

    /* calculate tone filter coefficient if frequency changed */
    if (fabs(*(synth->effect_param5) - p->prv_LPFreq) > 1e-7f) {
        p->prv_LPFreq = *(synth->effect_param5);
        /* was: p->dampFact = 2.0 - cos(p->prv_LPFreq * TWOPI / p->sampleRate); */
        p->dampFact = 2.0 - cos((double) p->prv_LPFreq * M_PI);
        p->dampFact = p->dampFact - sqrt(p->dampFact * p->dampFact - 1.0);
    }
    feedback = sqrt(*(synth->effect_param4));
    /* update delay lines */
    for (i = 0; i < sample_count; i++) {
        /* DC blocker */
        synth->dc_block_l_ynm1 = synth->voice_bus_l[i] - synth->dc_block_l_xnm1 +
                                     synth->dc_block_r * synth->dc_block_l_ynm1;
        ainL = (double)synth->dc_block_l_ynm1;
        synth->dc_block_l_xnm1 = synth->voice_bus_l[i];
        synth->dc_block_r_ynm1 = synth->voice_bus_r[i] - synth->dc_block_r_xnm1 +
                                     synth->dc_block_r * synth->dc_block_r_ynm1;
        ainR = (double)synth->dc_block_r_ynm1;
        synth->dc_block_r_xnm1 = synth->voice_bus_r[i];
        /* calculate "resultant junction pressure" and mix to input signals */
        aoutL = 0.0;
        for (n = 0; n < 8; n++)
            aoutL += p->delayLines[n].filterState;
        aoutL *= jpScale;
        ainR += aoutL;
        ainL += aoutL;
        aoutL = aoutR = 0.0;
        /* loop through all delay lines */
        for (n = 0; n < 8; n++) {
            lp = &p->delayLines[n];
            /* send input signal and feedback to delay line */
            lp->buf[lp->writePos] = (float) ((n & 1 ? ainR : ainL)
                                             - lp->filterState);
            if (++lp->writePos >= lp->bufferSize)
                lp->writePos -= lp->bufferSize;
            /* read from delay line with cubic interpolation */
            if (lp->readPosFrac >= DELAYPOS_SCALE) {
                lp->readPos += (lp->readPosFrac >> DELAYPOS_SHIFT);
                lp->readPosFrac &= DELAYPOS_MASK;
            }
            if (lp->readPos >= lp->bufferSize)
                lp->readPos -= lp->bufferSize;
            readPos = lp->readPos;
            frac = (double) lp->readPosFrac * (1.0 / (double) DELAYPOS_SCALE);
            /* calculate interpolation coefficients */
            a2 = frac * frac; a2 -= 1.0; a2 *= (1.0 / 6.0);
            a1 = frac; a1 += 1.0; a1 *= 0.5; am1 = a1 - 1.0;
            a0 = 3.0 * a2; a1 -= a0; am1 -= a2; a0 -= frac;
            /* read four samples for interpolation */
            if (readPos > 0 && readPos < (lp->bufferSize - 2)) {
                vm1 = (double) (lp->buf[readPos - 1]);
                v0 = (double) (lp->buf[readPos]);
                v1 = (double) (lp->buf[readPos + 1]);
                v2 = (double) (lp->buf[readPos + 2]);
            } else {
                /* at buffer wrap-around, need to check index */
                if (--readPos < 0) readPos += lp->bufferSize;
                vm1 = (double) lp->buf[readPos];
                if (++readPos >= lp->bufferSize) readPos -= lp->bufferSize;
                v0 = (double) lp->buf[readPos];
                if (++readPos >= lp->bufferSize) readPos -= lp->bufferSize;
                v1 = (double) lp->buf[readPos];
                if (++readPos >= lp->bufferSize) readPos -= lp->bufferSize;
                v2 = (double) lp->buf[readPos];
            }
            v0 = (am1 * vm1 + a0 * v0 + a1 * v1 + a2 * v2) * frac + v0;
            /* update buffer read position */
            lp->readPosFrac += lp->readPosFrac_inc;
            /* apply feedback gain and lowpass filter */
            v0 *= feedback;
            v0 = (lp->filterState - v0) * p->dampFact + v0;
            lp->filterState = v0;
            /* mix to output */
            if (n & 1)
                aoutR += v0;
            else
                aoutL += v0;
            /* start next random line segment if current one has reached endpoint */
            if (--(lp->randLine_cnt) <= 0)
                next_random_lineseg(synth, p, lp, n);
        }
        out_left[i]  = wet * (float) (aoutL * outputGain) + dry * synth->voice_bus_l[i];
        out_right[i] = wet * (float) (aoutR * outputGain) + dry * synth->voice_bus_r[i];
    }
}

