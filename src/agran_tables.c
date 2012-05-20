/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2005 Sean Bolton.
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

#include "agran_oscillator.h"

grain_envelope_descriptor_t grain_envelope_descriptors[AG_GRAIN_ENVELOPE_COUNT] = {
      { "1s Gaussian",         AG_ENVELOPE_GAUSSIAN, 1.523e-8, 1000 }, /* '0.01' matches MSS, '1.523e-8' matches CSound's gen20 Gaussian envelope, '1.585e-5' is -96dB */
      { "1s Roadsian",         AG_ENVELOPE_ROADSIAN,     0.25, 1000 },
      { "1/2s Gaussian",       AG_ENVELOPE_GAUSSIAN, 1.523e-8, 500 },
      { "1/2s Roadsian",       AG_ENVELOPE_ROADSIAN,     0.25, 500 },
      { "1/4s Gaussian",       AG_ENVELOPE_GAUSSIAN, 1.523e-8, 250 },
      { "1/4s Roadsian",       AG_ENVELOPE_ROADSIAN,     0.25, 250 },
      { "1/8s Gaussian",       AG_ENVELOPE_GAUSSIAN, 1.523e-8, 125 },
      { "1/8s Roadsian",       AG_ENVELOPE_ROADSIAN,     0.25, 125 },
      { "90ms Gaussian",       AG_ENVELOPE_GAUSSIAN, 1.523e-8, 90 },
      { "90ms Roadsian",       AG_ENVELOPE_ROADSIAN,     0.25, 90 },
      { "64ms Gaussian",       AG_ENVELOPE_GAUSSIAN, 1.523e-8, 64 },
      { "64ms Roadsian",       AG_ENVELOPE_ROADSIAN,     0.25, 64 },
      { "64ms Rectangular",    AG_ENVELOPE_ROADSIAN,     0.25, 64 },
      { "45ms Gaussian",       AG_ENVELOPE_GAUSSIAN, 1.523e-8, 45 },
      { "45ms Roadsian",       AG_ENVELOPE_ROADSIAN,     0.25, 45 },
      { "45ms Rectangular",    AG_ENVELOPE_ROADSIAN,     0.25, 45 },
      { "32ms Gaussian",       AG_ENVELOPE_GAUSSIAN, 1.523e-8, 32 },
      { "32ms Roadsian",       AG_ENVELOPE_ROADSIAN,     0.25, 32 },
      { "32ms Rectangular",    AG_ENVELOPE_ROADSIAN,     0.25, 32 },
      { "23ms Gaussian",       AG_ENVELOPE_GAUSSIAN, 1.523e-8, 23 },
      { "23ms Roadsian",       AG_ENVELOPE_ROADSIAN,     0.25, 23 },
      { "23ms Rectangular",    AG_ENVELOPE_ROADSIAN,     0.25, 23 },
      { "16ms Gaussian",       AG_ENVELOPE_GAUSSIAN, 1.523e-8, 16 },
      { "16ms Roadsian",       AG_ENVELOPE_ROADSIAN,     0.25, 16 },
      { "16ms Rectangular",    AG_ENVELOPE_ROADSIAN,     0.25, 16 },
      { "11ms Gaussian",       AG_ENVELOPE_GAUSSIAN, 1.523e-8, 11 },
      { "11ms Roadsian",       AG_ENVELOPE_ROADSIAN,     0.25, 11 },
      { "11ms Rectangular",    AG_ENVELOPE_ROADSIAN,     0.25, 11 },
      { "8ms Gaussian",        AG_ENVELOPE_GAUSSIAN, 1.523e-8, 8 },
      { "8ms Roadsian",        AG_ENVELOPE_ROADSIAN,     0.25, 8 },
      { "8ms Rectangular",     AG_ENVELOPE_ROADSIAN,     0.25, 8 },
};

