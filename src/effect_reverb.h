/* DSSI Plugin Framework
 *
 * Copyright (C) 2005 Sean Bolton and others.
 *
 * Most of this code comes from the Plate2x2 reverb in CAPS 0.2.3,
 * copyright (c) 2002-4 Tim Goetze.
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

#ifndef _EFFECT_REVERB_H
#define _EFFECT_REVERB_H

#include <math.h>

#include "whysynth_types.h"

struct OnePoleLP
{
    float a0, b1, y1;
};

static inline void
OnePoleLP_reset(struct OnePoleLP *this)
{
    this->y1 = 0.0f;
}

// static inline void
// OnePoleLP_set_f (struct OnePoleLP *this, double fc)
// {
//     OnePoleLP_set(this, exp (-2 * M_PI * fc));
// }

static inline void
OnePoleLP_set(struct OnePoleLP *this, double d)
{
    this->a0 = (float)d;
    this->b1 = (float)(1. - d);
}

static inline float
OnePoleLP_process(struct OnePoleLP *this, float x)
{
    return this->y1 = this->a0 * x + this->b1 * this->y1;
}

// static inline void
// OnePoleLP_decay (struct OnePoleLP *this, double d)
// {
//     this->a0 *= d;
//     this->b1 = 1. - this->a0;
// }

static inline int
next_power_of_2 (int n)
{
    int m = 1;

    while (m < n)
        m <<= 1;

    return m;
}

struct Delay
{
    int size;
    float * data;
    int read, write;
};

static inline void
Delay_init (struct Delay *this, y_synth_t *synth, int n)
{
    this->size = next_power_of_2 (n);
    this->data = (float *) effects_request_buffer(synth, this->size * sizeof(float));
    this->size -= 1;
    this->write = n;
    this->read = 0;
}

static inline void
Delay_reset(struct Delay *this)
{
    /* effect buffer should have been zeroed before we get here */
    /* memset (this->data, 0, (this->size + 1) * sizeof (float)); */
}

static inline float
Delay_peek(struct Delay *this, int i)
{
    return this->data[(this->write - i) & this->size];
}

static inline void 
Delay_put (struct Delay *this, float x)
{
    this->data [this->write] = x;
    this->write = (this->write + 1) & this->size;
}

static inline float
Delay_get(struct Delay *this)
{
    float x = this->data [this->read];
    this->read = (this->read + 1) & this->size;
    return x;
}

static inline float
Delay_putget (struct Delay *this, float x)
{
    Delay_put (this, x);
    return Delay_get(this);
}

/* fractional lookup, linear interpolation */
static inline float
Delay_get_at (struct Delay *this, float f)
{
    int n = lrintf(f - 0.5f);

    f -= (float)n;

    /* one multiply less, but slower on this 1.7 amd:
     *   register d_sample xn = (*this) [n];
     *   return xn + f * ((*this) [n + 1] - xn);
     * rather like this ... */
    return (1.0f - f) * Delay_peek(this, n) + f * Delay_peek(this, n + 1);
}

/* fractional lookup, cubic interpolation */
// statis inline float
// Delay_get_cubic (struct Delay *this, float f)
// {
//     float x_1, x0, x1, x2;
//     register float a, b, c;
//     int n = lrintf(f - 0.5f);
//
//     f -= (float)n;
//
//     x_1 = Delay_peek(this, n - 1);
//     x0 = Delay_peek(this, n);
//     x1 = Delay_peek(this, n + 1);
//     x2 = Delay_peek(this, n + 2);
//
//     /* float quicker than double here */
//     a = (3 * (x0 - x1) - x_1 + x2) * .5;
//     b = 2 * x1 + x_1 - (5 * x0 + x2) * .5;
//     c = (x1 - x_1) * .5;
//
//     return x0 + (((a * f) + b) * f + c) * f;
// }

static inline float
Lattice_process(struct Delay *delay, float x, double d)
{
    float y = Delay_get(delay);
    x -= d * y;
    Delay_put(delay, x);
    return d * x + y;
};

struct Sine
{
    int z;
    float y[2];
    float b;
};

static inline void
Sine_set_f (struct Sine *this, double w, double phase)
{
    this->b = 2. * cos (w);
    this->y[0] = sin (phase - w);
    this->y[1] = sin (phase - w * 2.);
    this->z = 0;
}

static inline void
Sine_set_f3 (struct Sine *this, double f, double fs, double phase)
{
    Sine_set_f (this, f * M_PI / fs, phase);
}

/* advance and return 1 sample */
static inline double
Sine_get(struct Sine *this)
{
    register double s = this->b * this->y[this->z]; 
    this->z ^= 1;
    s -= this->y[this->z];
    return this->y[this->z] = s;
}

struct ModLattice
{
    float n0, width;

    struct Delay delay;
    struct Sine lfo;
};

static inline void
ModLattice_init (struct ModLattice *this, y_synth_t *synth, int n, int w)
{
    this->n0 = n;
    this->width = w;
    Delay_init(&this->delay, synth, n + w);
}

static inline void
ModLattice_reset(struct ModLattice *this)
{
    Delay_reset(&this->delay);
}

static inline float
ModLattice_process (struct ModLattice *this, float x, double d)
{
    /* TODO: try all-pass interpolation */
    float y = Delay_get_at(&this->delay, this->n0 + this->width * Sine_get(&this->lfo));
    x += d * y;
    Delay_put(&this->delay, x);
    return y - d * x; /* note sign */
}

struct Plate
{
    double fs;

    float indiff1, indiff2, dediff1, dediff2;

    struct {
        struct OnePoleLP bandwidth;
        struct Delay lattice[4];
    } input;

    struct {
        struct ModLattice mlattice[2];
        struct Delay lattice[2];
        struct Delay delay[4];
        struct OnePoleLP damping[2];
        int taps[12];
    } tank;
};

struct DualDelay
{
    int max_delay;

    struct Delay delay_l,
                 delay_r;
    struct OnePoleLP damping_l,
                     damping_r;
};

#endif /* _EFFECT_REVERB_H */
