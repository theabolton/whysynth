/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2006, 2007, 2012 Sean Bolton.
 *
 * Based on the public domain implementation of the PADsynth
 * algorithm by Nasca O. Paul.  If you want to understand the
 * algorithm, see his example code, it's much clearer than this
 * optimized version.
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
#include <string.h>
#include <math.h>

#ifdef FFTW_VERSION_2
#include <rfftw.h>
#define fftwf_malloc(x) malloc(x)
#define fftwf_free(x) free(x)
#else
#include <fftw3.h>
#endif

#include "whysynth_types.h"
#include "whysynth.h"
#include "dssp_event.h"
#include "wave_tables.h"
#include "sampleset.h"
#include "padsynth.h"

#include "whysynth_voice_inline.h"

int
padsynth_init(void)
{
    global.padsynth_table_size = -1;
    global.padsynth_outfreqs = NULL;
    global.padsynth_outsamples = NULL;
    global.padsynth_fft_plan = NULL;
    global.padsynth_ifft_plan = NULL;

    /* allocate input FFT buffer */
    global.padsynth_inbuf = (float *)fftwf_malloc(WAVETABLE_POINTS * sizeof(float));
    if (!global.padsynth_inbuf) {
        return 0;
    }

    /* create input FFTW plan */
#ifdef FFTW_VERSION_2
    global.padsynth_fft_plan  = (void *)rfftw_create_plan(WAVETABLE_POINTS,
                                                          FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
#else
    global.padsynth_fft_plan  = (void *)fftwf_plan_r2r_1d(WAVETABLE_POINTS,
                                                          global.padsynth_inbuf,
                                                          global.padsynth_inbuf,
                                                          FFTW_R2HC, FFTW_ESTIMATE);
#endif
    if (!global.padsynth_fft_plan) {
        padsynth_fini();
        return 0;
    }

    return 1;
}

void
padsynth_fini(void)
{
    padsynth_free_temp();

#ifdef FFTW_VERSION_2
    if (global.padsynth_fft_plan)  rfftw_destroy_plan(global.padsynth_fft_plan);
    if (global.padsynth_ifft_plan) rfftw_destroy_plan(global.padsynth_ifft_plan);
#else
    if (global.padsynth_fft_plan)  fftwf_destroy_plan(global.padsynth_fft_plan);
    if (global.padsynth_ifft_plan) fftwf_destroy_plan(global.padsynth_ifft_plan);
#endif
    if (global.padsynth_inbuf)     fftwf_free(global.padsynth_inbuf);
}

void
padsynth_free_temp(void)
{
    if (global.padsynth_outfreqs) {
        fftwf_free(global.padsynth_outfreqs);
        global.padsynth_outfreqs = NULL;
    }
    if (global.padsynth_outsamples) {
        fftwf_free(global.padsynth_outsamples);
        global.padsynth_outsamples = NULL;
    }
}

/* padsynth_sampletable_setup
 *
 * set up a sampleset sample table from a wavetable, possibly inserting
 * additional samples to ensure complete coverage
 */
void
padsynth_sampletable_setup(y_sampleset_t *ss)
{
    int oi, ocount = 0, ni, ncount = 0;
    int this_max, next_max;
    signed short  max_key[WAVETABLE_MAX_WAVES],
                 *source[WAVETABLE_MAX_WAVES];

    for (oi = 0; oi < WAVETABLE_MAX_WAVES; oi++) {
        /* printf("o: %2d %3d %p\n", oi, wavetable[ss->waveform].wave[oi].max_key,
         *        wavetable[ss->waveform].wave[oi].data); */
        if (wavetable[ss->waveform].wave[oi].max_key == 256) {
            ocount = oi + 1;
            break;
        }
    }
    ni = 0;
    for (oi = ocount - 1; oi >= 0; oi--) {
        this_max = wavetable[ss->waveform].wave[oi].max_key;
        next_max = (oi ? wavetable[ss->waveform].wave[oi - 1].max_key : 0);
        while (this_max > next_max) {
            if (ncount >= WAVETABLE_MAX_WAVES) {
                YDB_MESSAGE(YDB_SAMPLE, " padsynth_sampletable_setup WARNING: sample table bloat!\n");
                oi = 0;
                break;
            }
            max_key[ni] = this_max;
            source[ni] = wavetable[ss->waveform].wave[oi].data;
            /* printf("n: %2d %3d %3d %3d %p\n", ni, max_key[ni],
             *        this_max - next_max, ni ? max_key[ni - 1] - max_key[ni] : 0,
             *        source[ni]); */
            ni++; ncount++;
            if (oi == 0 && this_max <= 36)
                break;
            if (this_max > 127) {
                this_max = next_max;
                while (this_max <= 106) this_max += 12;
            } else if (next_max == 0) {
                this_max -= 12;
            } else if (this_max - next_max > 12) {
                int n = (this_max - next_max + 11) / 12;
                this_max -= (this_max - next_max) / n;
            } else
                this_max = next_max;
        }
    }
    for (oi = 0; oi < ncount; oi++) {
        ni = ncount - oi - 1;
        ss->source[oi] = source[ni];
        ss->max_key[oi] = max_key[ni];
        ss->sample[oi] = NULL;
        if (ss->max_key[oi] == 256)
            break;
    }
}

#define M_PI_F ((float)M_PI)

/* Random number generator */
static inline float
RND(void)
{
    return (float)rand() / ((float)RAND_MAX + 1.0f);
}

/* This is the profile of one harmonic
 * In this case is a Gaussian distribution (e^(-x^2))
 * The amplitude is divided by the bandwidth to ensure that the harmonic
 * keeps the same amplitude regardless of the bandwidth */
static inline float
profile(float fi, float bwi) {
    float x = fi / bwi;
    x *= x;
    if (x > 14.71280603f) return 0.0f;  /* this avoids computing the e^(-x^2) where it's results are very close to zero */
    return expf(-x) / bwi;
}

/*
 *  The relF function returns the relative frequency of the N'th harmonic
 *  to the fundamental frequency.
 */
static inline float
relF(int N, float stretch) {
    return ((float)N * (1.0f + (float)(N - 1) * stretch));
};

/*
 *  PADsynth-ize a WhySynth wavetable.
 */
int
padsynth_render(y_sample_t *sample)
{
    int N, i, fc0, nh, plimit_low, plimit_high, rndlim_low, rndlim_high;
    float bw, stretch, bwscale, damping;
    float f, max, samplerate, bw0_Hz, relf;
    float *inbuf = global.padsynth_inbuf;
    float *outfreqs, *smp;

    /* handle the special case where the sample key limit is 256 -- these
     * don't get rendered because they're usually just sine waves, and are
     * played at very high frequency */
    if (sample->max_key == 256) {
        sample->data = (signed short *)malloc((WAVETABLE_POINTS + 8) * sizeof(signed short));
        if (!sample->data)
            return 0;
        sample->data += 4; /* guard points */
        memcpy(sample->data - 4, sample->source - 4,
               (WAVETABLE_POINTS + 8) * sizeof(signed short)); /* including guard points */
        sample->length = WAVETABLE_POINTS;
        sample->period = (float)WAVETABLE_POINTS;
        return 1;
    }

    /* calculate the output table size */
    i = lrintf((float)global.sample_rate * 2.5f);  /* at least 2.5 seconds long -FIX- this should be configurable */
    N = WAVETABLE_POINTS * 2;
    while (N < i) {
        if (N * 5 / 4 >= i) { N = N * 5 / 4; break; }
        if (N * 3 / 2 >= i) { N = N * 3 / 2; break; }
        N <<= 1;
    }

    /* check temporary memory and IFFT plan, allocate if needed */
    if (global.padsynth_table_size != N) {
        padsynth_free_temp();
        if (global.padsynth_ifft_plan) {
#ifdef FFTW_VERSION_2
            rfftw_destroy_plan(global.padsynth_ifft_plan);
#else
            fftwf_destroy_plan(global.padsynth_ifft_plan);
#endif
            global.padsynth_ifft_plan = NULL;
        }
        global.padsynth_table_size = N;
    }
    if (!global.padsynth_outfreqs)
        global.padsynth_outfreqs = (float *)fftwf_malloc(N * sizeof(float));
    if (!global.padsynth_outsamples)
        global.padsynth_outsamples = (float *)fftwf_malloc(N * sizeof(float));
    if (!global.padsynth_outfreqs || !global.padsynth_outsamples)
        return 0;
    outfreqs = global.padsynth_outfreqs;
    smp = global.padsynth_outsamples;
    if (!global.padsynth_ifft_plan)
        global.padsynth_ifft_plan =
#ifdef FFTW_VERSION_2
            (void *)rfftw_create_plan(N, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);
#else
            (void *)fftwf_plan_r2r_1d(N, global.padsynth_outfreqs,
                                      global.padsynth_outsamples,
                                      FFTW_HC2R, FFTW_ESTIMATE);
#endif
    if (!global.padsynth_ifft_plan)
        return 0;

    /* allocate sample memory */
    sample->data = (signed short *)malloc((N + 8) * sizeof(signed short));
    if (!sample->data)
        return 0;
    sample->data += 4; /* guard points */
    sample->length = N;

    /* condition parameters */
    bw = (sample->param1 ? (float)(sample->param1 * 2) : 1.0f); /* partial width: 1, or 2 to 100 cents by 2 */
    stretch = (float)(sample->param2 - 10) / 10.0f;
    stretch *= stretch * stretch / 10.0f;                       /* partial stretch: -10% to +10% */
    switch (sample->param3) {
      default:
      case  0:  bwscale = 1.0f;   break;                        /* width scale: 10% to 250% */
      case  2:  bwscale = 0.5f;   break;
      case  4:  bwscale = 0.25f;  break;
      case  6:  bwscale = 0.1f;   break;
      case  8:  bwscale = 1.5f;   break;
      case 10:  bwscale = 2.0f;   break;
      case 12:  bwscale = 2.5f;   break;
      case 14:  bwscale = 0.75f;  break;
    }
    damping = (float)sample->param4 / 20.0f;
    damping = damping * damping * -6.0f * logf(10.0f) / 20.0f;  /* damping: 0 to -6dB per partial */

    /* obtain spectrum of input wavetable */
    YDB_MESSAGE(YDB_SAMPLE, " padsynth_render: analyzing input table\n");
    for (i = 0; i < WAVETABLE_POINTS; i++)
        inbuf[i] = (float)sample->source[i] / 32768.0f;
#ifdef FFTW_VERSION_2
    rfftw_one((rfftw_plan)global.padsynth_fft_plan, inbuf, inbuf);
#else
    fftwf_execute((const fftwf_plan)global.padsynth_fft_plan);  /* transform inbuf in-place */
#endif
    max = 0.0f;
    if (damping > -1e-3f) { /* no damping */
        for (i = 1; i < WAVETABLE_POINTS / 2; i++) {
            inbuf[i] = sqrtf(inbuf[i] * inbuf[i] +
                             inbuf[WAVETABLE_POINTS - i] * inbuf[WAVETABLE_POINTS - i]);
            if (fabsf(inbuf[i]) > max) max = fabsf(inbuf[i]);
        }
        if (fabsf(inbuf[WAVETABLE_POINTS / 2]) > max) max = fabsf(inbuf[WAVETABLE_POINTS / 2]);
    } else {  /* damping */
        for (i = 1; i < WAVETABLE_POINTS / 2; i++) {
            inbuf[i] = sqrtf(inbuf[i] * inbuf[i] +
                             inbuf[WAVETABLE_POINTS - i] * inbuf[WAVETABLE_POINTS - i]) *
                       expf((float)i * damping);
            if (fabsf(inbuf[i]) > max) max = fabsf(inbuf[i]);
        }
        inbuf[WAVETABLE_POINTS / 2] = 0.0f;  /* lazy */
    }
    if (max < 1e-5f) max = 1e-5f;
    for (i = 1; i <= WAVETABLE_POINTS / 2; i++)
        inbuf[i] /= max;

    /* create new frequency profile */
    YDB_MESSAGE(YDB_SAMPLE, " padsynth_render: creating frequency profile\n");
    memset(outfreqs, 0, N * sizeof(float));

    /* render the fundamental at 4 semitones below the key limit */
    f = 440.0f * y_pitch[sample->max_key - 4];

    /* Find a nominal samplerate close to the real samplerate such that the
     * input partials fall exactly at integer output partials. This ensures
     * that especially the lower partials are not out of tune. Example:
     *    N = 131072
     *    global.samplerate = 44100
     *    f = 261.625565
     *    fi = f / global.samplerate = 0.00593255
     *    fc0 = int(fi * N) = int(777.592) = 778
     * so we find a new 'samplerate' that will result in fi * N being exactly 778:
     *    samplerate = f * N / fc = 44076.8
     */
    fc0 = lrintf(f / (float)global.sample_rate * (float)N);
    sample->period = (float)N / (float)fc0;  /* frames per period */
    samplerate = f * sample->period;
    /* YDB_MESSAGE(YDB_SAMPLE, " padsynth_render: size = %d, f = %f, fc0 = %d, period = %f\n", N, f, fc0, sample->period); */

    bw0_Hz = (powf(2.0f, bw / 1200.0f) - 1.0f) * f;

    /* Find the limits of the harmonics to be used in the output table. These
     * are 20Hz and Nyquist, corrected for the nominal-to-actual sample rate
     * difference, with the latter also corrected  for the 4-semitone shift in
     * the fundamental frequency.
     * lower partial limit:
     *   (20Hz * samplerate / global.sample_rate) / samplerate * N
     * 4-semitone shift:
     *   (2 ^ -12) ^ 4
     * upper partial limit:
     *   ((global.sample_rate / 2) * samplerate / global.sample_rate) / samplerate * N / shift
     */
    plimit_low = lrintf(20.0f / (float)global.sample_rate * (float)N);
    /* plimit_high = lrintf(20000.0f / (float)global.sample_rate * (float)N / 1.25992f); */
    plimit_high = lrintf((float)N / 2 / 1.25992f);
    /* YDB_MESSAGE(YDB_SAMPLE, " padsynth_render: nominal rate = %f, plimit low = %d, plimit high = %d\n", samplerate, plimit_low, plimit_high); */

    rndlim_low = N / 2;
    rndlim_high = 0;
    for (nh = 1; nh <= WAVETABLE_POINTS / 2; nh++) {
        int fc, contributed;
        float bw_Hz;  /* bandwidth of the current harmonic measured in Hz */
        float bwi;
        float fi;
        float plimit_amp;

        if (inbuf[nh] < 1e-5f)
            continue;

        relf = relF(nh, stretch);
        if (relf < 1e-10f)
            continue;

        bw_Hz = bw0_Hz * powf(relf, bwscale);
        
        bwi = bw_Hz / (2.0f * samplerate);
        fi = f * relf / samplerate;
        fc = lrintf(fi * (float)N);
        /* printf("...... kl = %d, nh = %d, fn = %f, fc = %d, bwi*N = %f\n", sample->max_key, nh, f * relf, fc, bwi * (float)N); */

        /* set plimit_amp such that we don't calculate harmonics -100dB or more
         * below the profile peak for this harmonic */
        plimit_amp = profile(0.0f, bwi) * 1e-5f / inbuf[nh];
        /* printf("...... (nh = %d, fc = %d, prof(0) = %e, plimit_amp = %e, amp = %e)\n", nh, fc, profile(0.0f, bwi), plimit_amp, inbuf[nh]); */

        /* scan profile and add partial's contribution to outfreqs */
        contributed = 0;
        for (i = (fc < plimit_high ? fc : plimit_high); i >= plimit_low; i--) {
            float hprofile = profile(((float)i / (float)N) - fi, bwi);
            if (hprofile < plimit_amp) {
                /* printf("...... (i = %d, profile = %e)\n", i, hprofile); */
                break;
            }
            outfreqs[i] += hprofile * inbuf[nh];
            contributed = 1;
        }
        if (contributed && rndlim_low > i + 1) rndlim_low = i + 1;
        contributed = 0;
        for (i = (fc + 1 > plimit_low ? fc + 1 : plimit_low); i <= plimit_high; i++) {
            float hprofile = profile(((float)i / (float)N) - fi, bwi);
            if (hprofile < plimit_amp) {
                /* printf("...... (i = %d, profile = %e)\n", i, hprofile); */
                break;
            }
            outfreqs[i] += hprofile * inbuf[nh];
            contributed = 1;
        }
        if (contributed && rndlim_high < i - 1) rndlim_high = i - 1;
    };
    if (rndlim_low > rndlim_high) {  /* somehow, outfreqs is still empty */
        YDB_MESSAGE(YDB_SAMPLE, " padsynth_render WARNING: empty output table (key limit = %d)\n", sample->max_key);
        rndlim_low = rndlim_high = fc0;
        outfreqs[fc0] = 1.0f;
    }

    /* randomize the phases */
    /* YDB_MESSAGE(YDB_SAMPLE, " padsynth_render: randomizing phases (%d to %d, kl=%d)\n", rndlim_low, rndlim_high, sample->max_key); */
    YDB_MESSAGE(YDB_SAMPLE, " padsynth_render: randomizing phases\n");
    if (rndlim_high >= N / 2) rndlim_high = N / 2 - 1;
    for (i = rndlim_low; i < rndlim_high; i++) {
        float phase = RND() * 2.0f * M_PI_F;
        outfreqs[N - i] = outfreqs[i] * cosf(phase);
        outfreqs[i]     = outfreqs[i] * sinf(phase);
    };

    /* inverse FFT back to time domain */
    YDB_MESSAGE(YDB_SAMPLE, " padsynth_render: performing inverse FFT\n");
#ifdef FFTW_VERSION_2
    rfftw_one((rfftw_plan)global.padsynth_ifft_plan, outfreqs, smp);
#else
    /* remember restrictions on FFTW3 'guru' execute: buffers must be the same
     * sizes, same in-place-ness or out-of-place-ness, and same alignment as
     * when plan was created. */
    fftwf_execute_r2r((const fftwf_plan)global.padsynth_ifft_plan, outfreqs, smp);
#endif

    /* normalize and convert output data */
    YDB_MESSAGE(YDB_SAMPLE, " padsynth_render: normalizing output\n");
    max = 0.0f;
    for (i = 0; i < N; i++) if (fabsf(smp[i]) > max) max = fabsf(smp[i]);
    if (max < 1e-5f) max = 1e-5f;
    max = 32767.0f / max;
    for (i = 0; i < N; i++)
         sample->data[i] = lrintf(smp[i] * max);

    /* copy guard points */
    for (i = -4; i < 0; i++)
        sample->data[i] = sample->data[i + N];
    for (i = 0; i < 4; i++)
        sample->data[N + i] = sample->data[i];

    YDB_MESSAGE(YDB_SAMPLE, " padsynth_render: done\n");

    return 1;
}

/* ==== sampleset oscillator ==== */

float crossfade_coeff0[5] = {
    /* -15.6dB   -7.95dB,     -4dB,   -1.7dB,  -0.41dB */
      0.166667, 0.400312, 0.629961, 0.823074, 0.954108
};

/*
 * sample_select
 */
static inline void
sample_select(y_sosc_t *sosc, struct vosc *vosc, int key)
{
    y_sampleset_t *ss = sosc->sampleset;
    int i;

    if (key > 256) key = 256;
    vosc->wave_select_key = key;
       
    for (i = 0; i < WAVETABLE_MAX_WAVES; i++) {
        if (key <= ss->max_key[i])
            break;
    }
    if (ss->max_key[i] - key >= WAVETABLE_CROSSFADE_RANGE ||
        ss->max_key[i] == 256) {
        /* printf("vosc %p: new sample index %d (sel = %d) no crossfade\n", vosc, i, key); */
        vosc->i0 = i;
        vosc->i1 = i;
        vosc->wavemix0 = 1.0f;
        vosc->wavemix1 = 0.0f;
    } else {
        vosc->i0 = i;
        vosc->i1 = i + 1;
        vosc->wavemix0 = crossfade_coeff0[ss->max_key[i] - key];
        vosc->wavemix1 = crossfade_coeff0[(WAVETABLE_CROSSFADE_RANGE - 1) - (ss->max_key[i] - key)];

        /* printf("vosc %p: new sample index %d (sel = %d) CROSSFADE = %f/%f\n", vosc, i, key, vosc->wavemix0, vosc->wavemix1); */
    }
}

static inline float
bspline_interp(float x, float ym1, float y0, float y1, float y2)
{
    /* 4-point, 3rd-order B-spline (x-form)
     * from "Polynomial Interpolators for High-Quality Resampling of
     * Oversampled Audio" by Olli Niemitalo */
    float ym1py1 = ym1 + y1;
    float c0 = 1/6.0f * ym1py1 + 2/3.0f * y0;
    float c1 = 1/2.0f * (y1 - ym1);
    float c2 = 1/2.0f * ym1py1 - y0;
    float c3 = 1/2.0f * (y0 - y1) + 1/6.0f * (y2 - ym1);
    return ((c3 * x + c2) * x + c1) * x + c0;
}

void
padsynth_oscillator(unsigned long sample_count, y_sosc_t *sosc,
                    y_voice_t *voice, struct vosc *vosc, int index, float w0)
{
    unsigned long sample;
    float w, w_delta,
          level_a, level_a_delta,
          level_b, level_b_delta;
    float f;
    int   i, ready;

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
    level_a       /= 32767.0f;
    level_a_delta /= 32767.0f;
    level_b       = level_a       * *(sosc->level_b);
    level_b_delta = level_a_delta * *(sosc->level_b);
    level_a       *= *(sosc->level_a);
    level_a_delta *= *(sosc->level_a);
    level_a_delta = (level_a_delta - level_a) / (float)sample_count;
    level_b_delta = (level_b_delta - level_b) / (float)sample_count;
    /* -FIX- condition to [0, 1]? */

    if (sosc->sampleset && sosc->sampleset->set_up) {

        i = voice->key + lrintf(*(sosc->pitch));
        if (vosc->mode     != vosc->last_mode ||
            vosc->waveform != vosc->last_waveform ||
            i              != vosc->wave_select_key) {

            /* get sample indices and crossfade from sampleset */
            sample_select(sosc, vosc, i);
            vosc->last_waveform = vosc->waveform;
            /* try to avoid reseting pos0 except when necessary to avoid causing clocks in mono
             * mode with portamento -- but basically it's going to click if it's not the same
             * sample.... */
            if (vosc->mode != vosc->last_mode) {
                vosc->last_mode = vosc->mode;
                if (sosc->sampleset->sample[vosc->i0])
                    vosc->pos0 = vosc->pos1 = (double)random_float(0.0f,
                                                  (float)sosc->sampleset->sample[vosc->i0]->length);
                else
                    vosc->pos0 = vosc->pos1 = 0.0;
            }
        }
        if (sosc->sampleset->sample[vosc->i0] &&
            sosc->sampleset->sample[vosc->i1])
            ready = 1;
        else
            ready = 0;
    } else {
        if (vosc->mode != vosc->last_mode) {
            vosc->last_mode = vosc->mode;
            vosc->pos0 = vosc->pos1 = 0.0;
        }
        vosc->last_waveform = -1;
        ready = 0;
    }

    if (!ready) {
        /* sampleset not ready, render a sine instead */
        float pos = (float)vosc->pos0;

        if (pos > 1.0f) pos = 0.0f;
        level_a       *= 16383.5f; /* scale for sine_wave (float) instead of sample->data (16-bit fixed) */
        level_a_delta *= 16383.5f;
        level_b       *= 16383.5f;
        level_b_delta *= 16383.5f;

        for (sample = 0; sample < sample_count; sample++) {

            pos += w;
            if (pos >= 1.0f) pos -= 1.0f;

            f = pos * (float)SINETABLE_POINTS;
            i = lrintf(f - 0.5f);
            f -= (float)i;
            f = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f;
            voice->osc_bus_a[index]   += level_a * f;
            voice->osc_bus_b[index++] += level_b * f;

            w       += w_delta;
            level_a += level_a_delta;
            level_b += level_b_delta;
        }

        vosc->pos0 = (double)pos;
        return;
    }

    if (sosc->sampleset->param3 & 1) {  /* mono */
      if (vosc->i0 == vosc->i1) { /* no crossfade */

        /* mono without crossfade */
        y_sample_t *s = (y_sample_t *)sosc->sampleset->sample[vosc->i0];
        signed short *data = s->data;
        double pos = vosc->pos0;
        double length = (double)s->length;
        float period = s->period;

        if (pos >= length) pos = 0.0;

        for (sample = 0; sample < sample_count; sample++) {

            i = lrint(pos - 0.5);
            f = (float)(pos - (double)i);
            f = bspline_interp(f, (float)data[i - 1], (float)data[i],
                                  (float)data[i + 1], (float)data[i + 2]);
            voice->osc_bus_a[index]   += level_a * f;
            voice->osc_bus_b[index++] += level_b * f;

            w       += w_delta;
            level_a += level_a_delta;
            level_b += level_b_delta;

            pos += w * period;
            if (pos >= length) pos -= length;
            /* sampleset oscillators do not export sync */
        }

        vosc->pos0 = pos;

      } else {

        /* mono with crossfade */
        y_sample_t *s0 = (y_sample_t *)sosc->sampleset->sample[vosc->i0],
                   *s1 = (y_sample_t *)sosc->sampleset->sample[vosc->i1];
        signed short *data0 = s0->data,
                     *data1 = s1->data;
        double pos0 = vosc->pos0,
               pos1 = vosc->pos1;
        double length0 = (double)s0->length,
               length1 = (double)s1->length;
        float period0 = s0->period,
              period1 = s1->period;
        float a,
              wavemix0 = vosc->wavemix0,
              wavemix1 = vosc->wavemix1;

        if (pos0 >= length0) pos0 = 0.0;
        if (pos1 >= length1) pos1 = 0.0;

        for (sample = 0; sample < sample_count; sample++) {

            i = lrint(pos0 - 0.5);
            f = (float)(pos0 - (double)i);
            a = bspline_interp(f, (float)data0[i - 1], (float)data0[i],
                                  (float)data0[i + 1], (float)data0[i + 2]) * wavemix0;

            i = lrint(pos1 - 0.5);
            f = (float)(pos1 - (double)i);
            a += bspline_interp(f, (float)data1[i - 1], (float)data1[i],
                                  (float)data1[i + 1], (float)data1[i + 2]) * wavemix1;

            voice->osc_bus_a[index]   += level_a * a;
            voice->osc_bus_b[index++] += level_b * a;

            w       += w_delta;
            level_a += level_a_delta;
            level_b += level_b_delta;

            pos0 += w * period0;
            pos1 += w * period1;
            if (pos0 >= length0) pos0 -= length0;
            if (pos1 >= length1) pos1 -= length1;
            /* sampleset oscillators do not export sync */
        }

        vosc->pos0 = pos0;
        vosc->pos1 = pos1;
      }

    } else {

      if (vosc->i0 == vosc->i1) {

        /* stereo without crossfade */
        y_sample_t *s = (y_sample_t *)sosc->sampleset->sample[vosc->i0];
        signed short *data = s->data;
        double posl = vosc->pos0,
               posr,
               length = (double)s->length;
        float period = s->period;

        if (posl >= length) posl = 0.0;
        /* delay the right channel by about one-half the table length, but make
         * it an multiple of the period length to minimize phase cancellation
         * when summed to mono */
        posr = posl + rint(length / 2.0 / (double)period) * (double)period;
        if (posr >= length) posr -= length;

        for (sample = 0; sample < sample_count; sample++) {

            i = lrint(posl - 0.5);
            f = (float)(posl - (double)i);
            f = bspline_interp(f, (float)data[i - 1], (float)data[i],
                                  (float)data[i + 1], (float)data[i + 2]);
            voice->osc_bus_a[index]   += level_a * f;

            i = lrint(posr - 0.5);
            f = (float)(posr - (double)i);
            f = bspline_interp(f, (float)data[i - 1], (float)data[i],
                                  (float)data[i + 1], (float)data[i + 2]);
            voice->osc_bus_b[index++] += level_b * f;

            w       += w_delta;
            level_a += level_a_delta;
            level_b += level_b_delta;

            posl += w * period;
            if (posl >= length) posl -= length;
            posr += w * period;
            if (posr >= length) posr -= length;
            /* sampleset oscillators do not export sync */
        }

        vosc->pos0 = posl;

      } else {

        /* stereo with crossfade */
        y_sample_t *s0 = (y_sample_t *)sosc->sampleset->sample[vosc->i0],
                   *s1 = (y_sample_t *)sosc->sampleset->sample[vosc->i1];
        signed short *data0 = s0->data,
                     *data1 = s1->data;
        double posl0 = vosc->pos0,
               posl1 = vosc->pos1,
               posr0, posr1;
        double length0 = (double)s0->length,
               length1 = (double)s1->length;
        float period0 = s0->period,
              period1 = s1->period;
        float a,
              wavemix0 = vosc->wavemix0,
              wavemix1 = vosc->wavemix1;

        if (posl0 >= length0) posl0 = 0.0;
        if (posl1 >= length1) posl1 = 0.0;
        posr0 = posl0 + rint(length0 / 2.0 / (double)period0) * (double)period0;
        posr1 = posl1 + rint(length1 / 2.0 / (double)period1) * (double)period1;
        if (posr0 >= length0) posr0 -= length0;
        if (posr1 >= length1) posr1 -= length1;

        for (sample = 0; sample < sample_count; sample++) {

            i = lrint(posl0 - 0.5);
            f = (float)(posl0 - (double)i);
            a = bspline_interp(f, (float)data0[i - 1], (float)data0[i],
                                  (float)data0[i + 1], (float)data0[i + 2]) * wavemix0;
            i = lrint(posl1 - 0.5);
            f = (float)(posl1 - (double)i);
            a += bspline_interp(f, (float)data1[i - 1], (float)data1[i],
                                  (float)data1[i + 1], (float)data1[i + 2]) * wavemix1;
            voice->osc_bus_a[index]   += level_a * a;

            i = lrint(posr0 - 0.5);
            f = (float)(posr0 - (double)i);
            a = bspline_interp(f, (float)data0[i - 1], (float)data0[i],
                                  (float)data0[i + 1], (float)data0[i + 2]) * wavemix0;
            i = lrint(posr1 - 0.5);
            f = (float)(posr1 - (double)i);
            a += bspline_interp(f, (float)data1[i - 1], (float)data1[i],
                                  (float)data1[i + 1], (float)data1[i + 2]) * wavemix1;
            voice->osc_bus_b[index++] += level_b * a;

            w       += w_delta;
            level_a += level_a_delta;
            level_b += level_b_delta;

            posl0 += w * period0;
            posr0 += w * period0;
            posl1 += w * period1;
            posr1 += w * period1;
            if (posl0 >= length0) posl0 -= length0;
            if (posr0 >= length0) posr0 -= length0;
            if (posl1 >= length1) posl1 -= length1;
            if (posr1 >= length1) posr1 -= length1;
            /* sampleset oscillators do not export sync */
        }

        vosc->pos0 = posl0;
        vosc->pos1 = posl1;
      }
    }
}

