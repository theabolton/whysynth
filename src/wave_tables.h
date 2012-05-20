/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2005-2007 Sean Bolton.
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

#ifndef _WAVE_TABLES_H
#define _WAVE_TABLES_H

#define WAVETABLE_POINTS  1024

#define WAVETABLE_CROSSFADE_RANGE     5

#define WAVETABLE_MAX_WAVES  14 /* 'Formant' 1, 2 and 3, and 'Kick' each have 13, but PADsynth expands 'Formant 1' to 14 */

struct wave
{
    unsigned short max_key;    /* MIDI key number */
    unsigned short wavetable;  /* true for wavetable waves, false for sampleset samples */
    signed short * data;
};

struct wavetable
{
    char *      name;
#ifdef Y_GUI
    int         priority;
#endif
#ifdef Y_PLUGIN
    struct wave wave[WAVETABLE_MAX_WAVES];
#endif
};

extern int wavetables_count;

extern struct wavetable wavetable[];

void wave_tables_set_count(void);

#endif /* _WAVE_TABLES_H */
