/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2005-2006 Sean Bolton.
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

/* -FIX- add some more-efficient versions of the common ranges (-1..+1, -.5..+.5, 0..+1) */
static inline float
random_float(float lower_bound, float range)
{
    /* -FIX- bit-shifting is not the most efficient way to generate noise on some platforms....? */
    return lower_bound + range * ((float)random() / (float)RAND_MAX);
}

/*
 * volume_cv_to_amplitude
 */
static inline float
volume_cv_to_amplitude(float cv)
{
    int i;
    float f;

    cv *= 100.0f;
    if (cv > 127.0f) cv = 127.0f;
    else if (cv < -127.0f) cv = -127.0f;
    i = lrintf(cv - 0.5f);
    f = cv - (float)i;
    return volume_cv_to_amplitude_table[i + 128] + f *
           (volume_cv_to_amplitude_table[i + 129] -
            volume_cv_to_amplitude_table[i + 128]);
}

/*
 * pitch_to_frequency
 */
static inline float
pitch_to_frequency(float pitch)
{
    int i = lrintf(pitch - 0.5f);
    float f = pitch - (float)i;
    i &= 0x7f;
    /* -FIX- this probably isn't accurate enough! */
    return y_pitch[i] + f * (y_pitch[i + 1] - y_pitch[i]);
}

/*
 * y_voice_mod_index
 */
static inline int
y_voice_mod_index(LADSPA_Data *p)
{
    int i = lrintf(*p);

    if (i < 0 || i >= Y_MODS_COUNT)
        return 0;

    return i;
}

/*
 * wavetable_select
 */
static inline void
wavetable_select(struct vosc *vosc, int key)
{
    int i;

    if (key > 256) key = 256;
    vosc->wave_select_key = key;
       
    i = 0;
    while (i < WAVETABLE_MAX_WAVES) {
        if (key <= wavetable[vosc->waveform].wave[i].max_key)
            break;
        i++;
    }
    if ((wavetable[vosc->waveform].wave[i].max_key - key >=
            WAVETABLE_CROSSFADE_RANGE) ||
        wavetable[vosc->waveform].wave[i].max_key == 256) {
        /* printf("vosc %p: new wave index %d (sel = %d) no crossfade\n", vosc, i, key); */
        vosc->wave0 = wavetable[vosc->waveform].wave[i].data;
        vosc->wave1 = wavetable[vosc->waveform].wave[i].data;
        vosc->wavemix0 = 1.0f;
        vosc->wavemix1 = 0.0f;
    } else {
        vosc->wave0 = wavetable[vosc->waveform].wave[i].data;
        vosc->wave1 = wavetable[vosc->waveform].wave[i + 1].data;
        vosc->wavemix0 = (float)(wavetable[vosc->waveform].wave[i].max_key - key + 1) /
                                 (float)(WAVETABLE_CROSSFADE_RANGE + 1);
        vosc->wavemix1 = 1.0f - vosc->wavemix0;
        /* printf("vosc %p: new wave index %d (sel = %d) CROSSFADE = %f/%f\n", vosc, i, key, vosc->wavemix0, vosc->wavemix1); */
    }
}

