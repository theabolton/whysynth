/* WhySynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004-2012, 2016 Sean Bolton and others.
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

#define _DEFAULT_SOURCE 1
#define _ISOC99_SOURCE  1

#if THREAD_LOCALE_LOCALE_H
#define _XOPEN_SOURCE 700   /* needed for glibc newlocale() support */
#define _GNU_SOURCE         /* ditto, for older versions of glibc */
#endif

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#if THREAD_LOCALE_LOCALE_H
#include <locale.h>
#else
#if THREAD_LOCALE_XLOCALE_H
#include <xlocale.h>
#endif
#endif

#include "whysynth_types.h"
#include "whysynth.h"
#include "whysynth_voice.h"
#include "gui_main.h"
#include "common_data.h"

static int
patch_write_text(FILE *file, char *text, int maxlen)
{
    int i;

    for (i = 0; i < maxlen; i++) {
        if (!text[i]) {
            break;
        } else if (text[i] < 33 || text[i] > 126 ||
                   text[i] == '%') {
            fprintf(file, "%%%02x", (unsigned char)text[i]);
        } else {
            fputc(text[i], file);
        }
    }
    return fprintf(file, "\n"); /* -FIX- error handling... */
}

static int
patch_write_osc(FILE *file, int index, struct posc *osc)
{
    return fprintf(file, "oscY %d %d %d %d %.6g %d %.6g %.6g %.6g %d %.6g %d %.6g %.6g %.6g\n",
                   index, osc->mode, osc->waveform, osc->pitch, osc->detune,
                   osc->pitch_mod_src, osc->pitch_mod_amt, osc->mparam1,
                   osc->mparam2, osc->mmod_src, osc->mmod_amt,
                   osc->amp_mod_src, osc->amp_mod_amt,
                   osc->level_a, osc->level_b);
}

static int
patch_write_vcf(FILE *file, int index, struct pvcf *vcf)
{
    return fprintf(file, "vcfY %d %d %d %.6g %d %.6g %.6g %.6g\n",
                   index, vcf->mode, vcf->source, vcf->frequency,
                   vcf->freq_mod_src, vcf->freq_mod_amt, vcf->qres,
                   vcf->mparam);
}

static int
patch_write_lfo(FILE *file, char which, struct plfo *lfo)
{
    return fprintf(file, "lfoY %c %.6g %d %.6g %d %.6g\n",
                   which, lfo->frequency, lfo->waveform, lfo->delay,
                   lfo->amp_mod_src, lfo->amp_mod_amt);
}

static int
patch_write_eg(FILE *file, char which, struct peg *eg)
{
    return fprintf(file, "egY %c %d %d %.6g %.6g %d %.6g %.6g %d %.6g %.6g %d %.6g %.6g %.6g %.6g %d %.6g\n",
                   which, eg->mode,
                   eg->shape1, eg->time1, eg->level1,
                   eg->shape2, eg->time2, eg->level2,
                   eg->shape3, eg->time3, eg->level3,
                   eg->shape4, eg->time4,
                   eg->vel_level_sens, eg->vel_time_scale, eg->kbd_time_scale,
                   eg->amp_mod_src, eg->amp_mod_amt);
}

int
gui_data_write_patch(FILE *file, y_patch_t *patch)
{
    fprintf(file, "# WhySynth patch\n");
    fprintf(file, "WhySynth patch format %d begin\n", 0);

    fprintf(file, "name ");
    patch_write_text(file, patch->name, 30);

    if (strlen(patch->comment)) {
        fprintf(file, "comment ");
        patch_write_text(file, patch->comment, 60);
    }

    /* -PORTS- */
    patch_write_osc(file, 1, &patch->osc1);
    patch_write_osc(file, 2, &patch->osc2);
    patch_write_osc(file, 3, &patch->osc3);
    patch_write_osc(file, 4, &patch->osc4);
    patch_write_vcf(file, 1, &patch->vcf1);
    patch_write_vcf(file, 2, &patch->vcf2);
    fprintf(file, "mix %.6g %.6g %.6g %.6g %.6g %.6g %.6g %.6g\n",
            patch->busa_level, patch->busa_pan,
            patch->busb_level, patch->busb_pan,
            patch->vcf1_level, patch->vcf1_pan,
            patch->vcf2_level, patch->vcf2_pan);
    fprintf(file, "volume %.6g\n", patch->volume);
    fprintf(file, "effects %d %.6g %.6g %.6g %.6g %.6g %.6g %.6g\n",
            patch->effect_mode, patch->effect_param1, patch->effect_param2,
            patch->effect_param3, patch->effect_param4, patch->effect_param5,
            patch->effect_param6, patch->effect_mix);
    fprintf(file, "glide %.6g\n", patch->glide_time);
    fprintf(file, "bend %d\n", patch->bend_range);
    patch_write_lfo(file, 'g', &patch->glfo);
    patch_write_lfo(file, 'v', &patch->vlfo);
    patch_write_lfo(file, 'm', &patch->mlfo);
    fprintf(file, "mlfo %.6g %.6g\n", patch->mlfo_phase_spread,
            patch->mlfo_random_freq);
    patch_write_eg(file, 'o', &patch->ego);
    patch_write_eg(file, '1', &patch->eg1);
    patch_write_eg(file, '2', &patch->eg2);
    patch_write_eg(file, '3', &patch->eg3);
    patch_write_eg(file, '4', &patch->eg4);
    fprintf(file, "modmix %.6g %d %.6g %d %.6g\n", patch->modmix_bias,
            patch->modmix_mod1_src, patch->modmix_mod1_amt,
            patch->modmix_mod2_src, patch->modmix_mod2_amt);

    fprintf(file, "WhySynth patch end\n");

    return 1;  /* -FIX- error handling yet to be implemented */
}

#if (THREAD_LOCALE_LOCALE_H || THREAD_LOCALE_XLOCALE_H)
/* it would be nice if glibc had fprintf_l() and sscanf_l().... */

static locale_t old_locale = LC_GLOBAL_LOCALE;
static locale_t c_locale   = LC_GLOBAL_LOCALE;

void
y_set_C_numeric_locale(void)
{
    if (c_locale == LC_GLOBAL_LOCALE) {
        c_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
    }
    old_locale = uselocale((locale_t)0);
    uselocale(c_locale);
}

void
y_restore_old_numeric_locale(void)
{
    uselocale(old_locale);
    if (c_locale != LC_GLOBAL_LOCALE) {
        freelocale(c_locale);
        c_locale = LC_GLOBAL_LOCALE;
    }
}
#else
#warning no newlocale()/uselocale() available, patch loading may fail in non-C locales
void y_set_C_numeric_locale(void) { return; }
void y_restore_old_numeric_locale(void) { return; }
#endif

/*
 * gui_data_save
 */
int
gui_data_save(char *filename, int start, int end, char **message)
{
    FILE *fh;
    int i;
    char buffer[20];

    GDB_MESSAGE(GDB_IO, " gui_data_save: attempting to save '%s'\n", filename);

    if ((fh = fopen(filename, "wb")) == NULL) {
        if (message) *message = strdup("could not open file for writing");
        return 0;
    }
    y_set_C_numeric_locale();
    for (i = start; i <= end; i++) {
        if (!gui_data_write_patch(fh, &patches[i])) {
            y_restore_old_numeric_locale();
            fclose(fh);
            if (message) *message = strdup("error while writing file");
            return 0;
        }
    }
    y_restore_old_numeric_locale();
    fclose(fh);

    if (message) {
        snprintf(buffer, 20, "wrote %d patches", end - start + 1);
        *message = strdup(buffer);
    }
    return 1;
}

/*
 * gui_data_save_dirty_patches_to_tmp
 */
int
gui_data_save_dirty_patches_to_tmp(void)
{
    FILE *fh;
    int i;

    if ((fh = fopen(patches_tmp_filename, "wb")) == NULL) {
        return 0;
    }
    y_set_C_numeric_locale();
    for (i = 0; i < patch_count; i++) {
        if (!gui_data_write_patch(fh, &patches[i])) {
            y_restore_old_numeric_locale();
            fclose(fh);
            return 0;
        }
    }
    y_restore_old_numeric_locale();
    fclose(fh);
    return 1;
}

/*
 * gui_data_check_patches_allocation
 */
void
gui_data_check_patches_allocation(int patch_index)
{
    if (patch_index >= patches_allocated) {

        int n = (patch_index + 0x80) & 0xffff80,
            i;

        if (!(patches = (y_patch_t *)realloc(patches, n * sizeof(y_patch_t)))) {
            GDB_MESSAGE(-1, " gui_data_friendly_patches fatal: out of memory!\n");
            exit(1);
        }

        for (i = patches_allocated; i < n; i++) {
            memcpy(&patches[i], &y_init_voice, sizeof(y_patch_t));
        }

        patches_allocated = n;
    }
}

/*
 * gui_data_load
 */
int
gui_data_load(const char *filename, int position, char **message)
{
    FILE *fh;
    int count = 0;
    int index = position;
    char buffer[32];

    GDB_MESSAGE(GDB_IO, " gui_data_load: attempting to load '%s'\n", filename);

    if ((fh = fopen(filename, "rb")) == NULL) {
        if (message) *message = strdup("could not open file for reading");
        return 0;
    }

    while (1) {
        gui_data_check_patches_allocation(index);
        if (!y_data_read_patch(fh, &patches[index]))
            break;
        count++;
        index++;
    }
    fclose(fh);

    if (!count) {
        if (message) *message = strdup("no patches recognized");
        return 0;
    }

    if (position == 0 && count >= patch_count)
        patches_dirty = 0;
    else
        patches_dirty = 1;
    if (index > patch_count)
        patch_count = index;

    if (message) {
        snprintf(buffer, 32, "loaded %d patches", count);
        *message = strdup(buffer);
    }
    return count;
}

/*
 * gui_data_friendly_patches
 *
 * give the new user a default set of good patches to get started with
 */
void
gui_data_friendly_patches(void)
{
    gui_data_check_patches_allocation(y_friendly_patch_count);

    memcpy(patches, y_friendly_patches, y_friendly_patch_count * sizeof(y_patch_t));

    patch_count = y_friendly_patch_count;
}

/*
 * gui_data_patch_compare
 *
 * returns true if two patches are the same
 */
int
gui_data_patch_compare(y_patch_t *patch1, y_patch_t *patch2)
{
    int n;

    if (strcmp(patch1->name, patch2->name))
        return 0;
    if (strcmp(patch1->comment, patch2->comment))
        return 0;

    /* Note: this comparison depends on osc1.mode being the first field in
     * y_patch_t after the comment, and on y_patch_t being packed from
     * osc1.mode through its end. */
    n = offsetof(y_patch_t, osc1.mode);
    return !memcmp((void *)((char *)patch1 + n),
                   (void *)((char *)patch2 + n), sizeof(y_patch_t) - n);
}

#ifdef DEVELOPER

/* ==== Write Patches as 'C'... ==== */

static int
c_write_text(FILE *file, char *text, int maxlen)
{
    int i;

    for (i = 0; i < maxlen; i++) {
        if (!text[i]) {
            break;
        } else if (text[i] < 32 || text[i] > 126 ||
                   text[i] == '"' || text[i] == '\\') {
            fprintf(file, "\\%03o", (unsigned char)text[i]);
        } else {
            fputc(text[i], file);
        }
    }

    return 1;  /* -FIX- error handling... */
}

static int
c_write_osc(FILE *file, struct posc *osc)
{
    return fprintf(file, "        { %d, %d, %d, %.6g, %d, %.6g, %.6g, %.6g, %d, %.6g, %d, %.6g, %.6g, %.6g },\n",
                   osc->mode, osc->waveform, osc->pitch, osc->detune,
                   osc->pitch_mod_src, osc->pitch_mod_amt, osc->mparam1,
                   osc->mparam2, osc->mmod_src, osc->mmod_amt,
                   osc->amp_mod_src, osc->amp_mod_amt,
                   osc->level_a, osc->level_b);
}

static int
c_write_vcf(FILE *file, struct pvcf *vcf)
{
    return fprintf(file, "        { %d, %d, %.6g, %d, %.6g, %.6g, %.6g },\n",
                   vcf->mode, vcf->source, vcf->frequency,
                   vcf->freq_mod_src, vcf->freq_mod_amt, vcf->qres,
                   vcf->mparam);
}

static int
c_write_lfo(FILE *file, struct plfo *lfo)
{
    return fprintf(file, "        { %.6g, %d, %.6g, %d, %.6g },\n",
                   lfo->frequency, lfo->waveform, lfo->delay,
                   lfo->amp_mod_src, lfo->amp_mod_amt);
}

static int
c_write_eg(FILE *file, struct peg *eg)
{
    return fprintf(file, "        { %d, %d, %.6g, %.6g, %d, %.6g, %.6g, %d, %.6g, %.6g, %d, %.6g, %.6g, %.6g, %.6g, %d, %.6g },\n",
                   eg->mode,
                   eg->shape1, eg->time1, eg->level1,
                   eg->shape2, eg->time2, eg->level2,
                   eg->shape3, eg->time3, eg->level3,
                   eg->shape4, eg->time4,
                   eg->vel_level_sens, eg->vel_time_scale, eg->kbd_time_scale,
                   eg->amp_mod_src, eg->amp_mod_amt);
}

/* This writes a patch in C structure format suitable for inclusion in
 * patch_tables.c
 */

int
gui_data_write_patch_as_c(FILE *file, y_patch_t *patch)
{
    fprintf(file, "    {\n");

    fprintf(file, "        \"");
    c_write_text(file, patch->name, 30);
    fprintf(file, "\",\n");

    fprintf(file, "        \"");
    c_write_text(file, patch->comment, 60);
    fprintf(file, "\",\n");

    /* -PORTS- */
    c_write_osc(file, &patch->osc1);
    c_write_osc(file, &patch->osc2);
    c_write_osc(file, &patch->osc3);
    c_write_osc(file, &patch->osc4);
    c_write_vcf(file, &patch->vcf1);
    c_write_vcf(file, &patch->vcf2);
    fprintf(file, "        %.6g, %.6g, %.6g, %.6g, %.6g, %.6g, %.6g, %.6g,\n",
            patch->busa_level, patch->busa_pan,
            patch->busb_level, patch->busb_pan,
            patch->vcf1_level, patch->vcf1_pan,
            patch->vcf2_level, patch->vcf2_pan);
    fprintf(file, "        %.6g,\n", patch->volume);
    fprintf(file, "        %d, %.6g, %.6g, %.6g, %.6g, %.6g, %.6g, %.6g,\n",
            patch->effect_mode, patch->effect_param1, patch->effect_param2,
            patch->effect_param3, patch->effect_param4, patch->effect_param5,
            patch->effect_param6, patch->effect_mix);
    fprintf(file, "        %.6g, %d,\n", patch->glide_time, patch->bend_range);
    c_write_lfo(file, &patch->glfo);
    c_write_lfo(file, &patch->vlfo);
    c_write_lfo(file, &patch->mlfo);
    fprintf(file, "        %.6g, %.6g,\n", patch->mlfo_phase_spread,
            patch->mlfo_random_freq);
    c_write_eg(file, &patch->ego);
    c_write_eg(file, &patch->eg1);
    c_write_eg(file, &patch->eg2);
    c_write_eg(file, &patch->eg3);
    c_write_eg(file, &patch->eg4);
    fprintf(file, "        %.6g, %d, %.6g, %d, %.6g\n", patch->modmix_bias,
            patch->modmix_mod1_src, patch->modmix_mod1_amt,
            patch->modmix_mod2_src, patch->modmix_mod2_amt);

    fprintf(file, "    },\n");

    return 1;  /* -FIX- error handling yet to be implemented */
}

/*
 * gui_data_save_as_c
 */
int
gui_data_save_as_c(char *filename, int start, int end, char **message)
{
    FILE *fh;
    int i;
    char buffer[20];

    GDB_MESSAGE(GDB_IO, " gui_data_save_as_c: attempting to save '%s'\n", filename);

    if ((fh = fopen(filename, "wb")) == NULL) {
        if (message) *message = strdup("could not open file for writing");
        return 0;
    }
    y_set_C_numeric_locale();
    for (i = start; i <= end; i++) {
        if (!gui_data_write_patch_as_c(fh, &patches[i])) {
            y_restore_old_numeric_locale();
            fclose(fh);
            if (message) *message = strdup("error while writing file");
            return 0;
        }
    }
    y_restore_old_numeric_locale();
    fclose(fh);

    if (message) {
        snprintf(buffer, 20, "wrote %d patches", end - start + 1);
        *message = strdup(buffer);
    }
    return 1;
}
#endif /* DEVELOPER */

/* ==== Import Xsynth-DSSI Patches... ==== */

y_patch_t y_init_voice_xsynth_single = {
        /* -PORTS- */
        "-Xsynth-DSSI single init voice",
        "",
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5 },
        { 1, 0, 50, 0, 0, 0, 0 },
        { 0, 0, 50, 0, 0, 0, 0 },
        0, 0.5, 0, 0.5, 1, 0.5, 0, 0.5,
        0.5,
        0, 0, 0, 0, 0, 0, 0, 0,
        0.984375, 2,
        { 1, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0 },
        90, 0,
        { 1, 0, 0.1, 1, 3, 0.001, 1, 2, 0.001, 1, 0, 0.2, 0, 0, 0, 0, 0 },
        { 1, 0, 0.1, 1, 3, 0.1, 1, 0, 0.1, 1, 0, 0.2, 0, 0, 0, 0, 0 },
        { 1, 0, 0.1, 1, 3, 0.1, 1, 0, 0.1, 1, 0, 0.2, 0, 0, 0, 0, 0 },
        { 0, 3, 0.1, 1, 3, 0.1, 1, 3, 0.1, 1, 3, 0.2, 0, 0, 0, 0, 0 },
        { 0, 3, 0.1, 1, 3, 0.1, 1, 3, 0.1, 1, 3, 0.2, 0, 0, 0, 0, 0 },
        1, 0, 0, 0, 0
};

y_patch_t y_init_voice_xsynth_dual = {
        /* -PORTS- */
        "-Xsynth-DSSI dual init voice-",
        "",
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5 },
        { 1, 0, 50, 0, 0, 0, 0 },
        { 1, 1, 50, 0, 0, 0, 0 },
        0, 0.5, 0, 0.5, 0.71, 0.25, 0.71, 0.75,
        0.5,
        0, 0, 0, 0, 0, 0, 0, 0,
        0.984375, 2,
        { 1, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0 },
        90, 0,
        { 1, 0, 0.1, 1, 3, 0.001, 1, 0, 0.001, 1, 0, 0.2, 0, 0, 0, 0, 0 },
        { 1, 0, 0.1, 1, 3, 0.1, 1, 0, 0.1, 1, 0, 0.2, 0, 0, 0, 0, 0 },
        { 1, 0, 0.1, 1, 3, 0.1, 1, 0, 0.1, 1, 0, 0.2, 0, 0, 0, 0, 0 },
        { 0, 3, 0.1, 1, 3, 0.1, 1, 3, 0.1, 1, 3, 0.2, 0, 0, 0, 0, 0 },
        { 0, 3, 0.1, 1, 3, 0.1, 1, 3, 0.1, 1, 3, 0.2, 0, 0, 0, 0, 0 },
        1, 0, 0, 0, 0
};

static void
xi_set_osc(struct posc *osc, float pitch, int waveform, float pw)
{
    float new_detune = logf(pitch) / logf(2) * 12;
    int   new_pitch = floorf(new_detune + 0.5f);

    new_detune -= (float)new_pitch;
    osc->pitch  = new_pitch;
    osc->detune = new_detune;

    switch (waveform) {
      case 0:  /* sine */
        osc->mode = 2;  /* wavetable */
        osc->waveform = 0;
        break;
      case 1:  /* triangle */
        osc->waveform = 3;
        osc->mparam2 = 0.5f;
        break;
      default:
      case 2:  /* sawtooth+ */
        osc->waveform = 0;
        break;
      case 3:  /* sawtooth- */
        osc->waveform = 1;
        break;
      case 4:  /* square */
        osc->waveform = 2;
        osc->mparam2 = 0.5f;
        break;
      case 5:  /* rectangular */
        osc->waveform = 2;
        osc->mparam2 = pw;
        break;
      case 6:  /* var-slope tri */
        osc->waveform = 3;
        osc->mparam2 = pw;
        break;
    }
}

#define XI_EG_TIME_CONSTANT_1  (1.044e-4)
#define XI_EG_TIME_CONSTANT_2  (2.349e-4)

void
xi_set_eg(struct peg *eg, float a, float d, float s, float r, float vs)
{
    eg->time1 = XI_EG_TIME_CONSTANT_1 / a;
    if (eg->time1 < 0.001f) eg->time1 = 0.001f;
    else if (eg->time1 > 20.0f) eg->time1 = 20.0f;

    eg->time3 = XI_EG_TIME_CONSTANT_1 / d * 1.2; /* fudge */
    if (eg->time3 < 0.001f) eg->time3 = 0.001f;
    else if (eg->time3 > 50.0f) eg->time3 = 50.0f;

    eg->level3 = s;

    eg->time4 = XI_EG_TIME_CONSTANT_2 / r;
    if (eg->time4 < 0.001f) eg->time4 = 0.001f;
    else if (eg->time4 > 50.0f) eg->time4 = 50.0f;

    if (vs < 0.5)
        eg->vel_level_sens = vs * 0.8f;
    else
        eg->vel_level_sens = 0.4f + (vs - 0.4f) * 0.6f;
}

#include "wave_tables.h"

int xi_find_wave(char *name)
{
    int i;

    for (i = 0; i < wavetables_count; i++) {
        char *wavename = wavetable[i].name;

        if (strrchr(wavename, '|'))
            wavename = strrchr(wavename, '|') + 1;
        if (!strcmp(wavename, name))
            return i;
    }
    
    fprintf(stderr, "Xsynth-DSSI patch import warning: wave '%s' not found!\n", name);
    return 0; /* punt */
}

static float
xi_amp_to_volume_cv(float a)
{
    if (a > 0.001f)
        return 1.0f + 0.08f * logf(a) / logf(2);
    else
        return a * 200.0f;
}

/*
 * gui_data_read_xsynth_patch
 */
int
gui_data_read_xsynth_patch(FILE *file, y_patch_t *patch, int dual)
{
    int format, i0;
    char buf[256], buf2[91];
    float f0, f1, f2, f3, f4, f5, f6;
    float lfo2osc = 0.0f,
          lfo2vcf = 0.0f,
          eg12osc = 0.0f,
          eg12vcf = 0.0f,
          eg22osc = 0.0f,
          eg22vcf = 0.0f;
    y_patch_t tmp;

    do {
        if (!fgets(buf, 256, file)) return 0;
    } while (y_data_is_comment(buf));

    if (sscanf(buf, " xsynth-dssi patch format %d begin", &format) != 1 ||
        format < 0 || format > 1)
        return 0;

    memcpy(&tmp, dual ? &y_init_voice_xsynth_dual
                      : &y_init_voice_xsynth_single, sizeof(y_patch_t));
    strcpy(tmp.comment, "Imported Xsynth-DSSI patch");

    while (1) {
        
        if (!fgets(buf, 256, file)) return 0;

        /* 'name %20%20<init%20voice>' */
        if (sscanf(buf, " name %90s", buf2) == 1) {
            
            y_data_parse_text(buf2, buf, 30);
            if (dual && strlen(buf) < 27)
                snprintf(tmp.name, 31, "X2: %s", buf);
            else if (strlen(buf) < 28)
                snprintf(tmp.name, 31, "X: %s", buf);
            else
                strncpy(tmp.name, buf, 31);
            continue;

        /* -PORTS- */
        /* 'osc1 1.0 2 0.5' */
        } else if (y_sscanf(buf, " osc1 %f %d %f", &f0, &i0, &f1) == 3) {

            xi_set_osc(&tmp.osc1, f0, i0, f1);
            if (dual)
                xi_set_osc(&tmp.osc3, f0, i0, f1);
            continue;

        /* 'osc2 2.0 5 0.231648' */
        } else if (y_sscanf(buf, " osc2 %f %d %f", &f0, &i0, &f1) == 3) {

            xi_set_osc(&tmp.osc2, f0, i0, f1);
            if (dual)
                xi_set_osc(&tmp.osc4, f0, i0, f1);
            continue;

        /* 'sync 0' */
        } else if (sscanf(buf, " sync %d", &i0) == 1) {

            tmp.osc2.mparam1 = i0 ? 1.0f : 0.0f;
            if (dual)
               tmp.osc4.mparam1 = tmp.osc2.mparam1;
            continue;

        /* 'balance 0.5' */
        } else if (y_sscanf(buf, " balance %f", &f0) == 1) {

            tmp.osc1.level_a = (1.0f - f0) * 1.4f;
            tmp.osc2.level_a =         f0  * 1.4f;
            if (dual) {
                tmp.osc3.level_b = tmp.osc1.level_a;
                tmp.osc4.level_b = tmp.osc2.level_a;
            }
            continue;

        /* 'lfo 0.70426 0 0 0.193056' */
        } else if (y_sscanf(buf, " lfo %f %d %f %f", &tmp.vlfo.frequency, &i0,
                            &f0, &f1) == 4) {
                            // frequency, waveform, amount_o, amount_f
            switch (i0) {
              default:
              case 0:  tmp.vlfo.waveform = xi_find_wave("Sine 1");        break;
              case 1:  tmp.vlfo.waveform = xi_find_wave("LFO Tri");       break;
              case 2:  tmp.vlfo.waveform = xi_find_wave("LFO Saw");       break; /* saw+ */
              case 3:  tmp.vlfo.waveform = xi_find_wave("LFO Saw");       break; /* saw- */
              case 4:  tmp.vlfo.waveform = xi_find_wave("LFO Square");    break;
              case 5:  tmp.vlfo.waveform = xi_find_wave("LFO Rect 1/4");  break;
            }
            if (i0 == 2) {
                lfo2osc = -f0;
                lfo2vcf = -f1;
            } else {
                lfo2osc = f0;
                lfo2vcf = f1;
            }
            continue;

        /* 'eg1 0.0002 0.1 1 1e-04 0 14.5722' */
        } else if (format == 1 &&
                   y_sscanf(buf, " eg1 %f %f %f %f %f %f %f",
                   // eg1_attack_time, eg1_decay_time,
                   // eg1_sustain_level, eg1_release_time,
                   // eg1_vel_sens, eg1_amount_o, eg1_amount_f
                            &f0, &f1, &f2, &f3, &f4, &f5, &f6) == 7) {

            xi_set_eg(&tmp.ego, f0, f1, xi_amp_to_volume_cv(f2),
                                            f3, f4);
            xi_set_eg(&tmp.eg1, f0, f1, f2, f3, f4);
            eg12osc = f5;
            eg12vcf = f6;
            continue;

        /* 'eg2 1e-04 4.4e-05 0 8.3e-05 0 12.5' */
        } else if (format == 1 &&
                   y_sscanf(buf, " eg2 %f %f %f %f %f %f %f",
                            &f0, &f1, &f2, &f3, &f4, &f5, &f6) == 7) {

            xi_set_eg(&tmp.eg2, f0, f1, f2, f3, f4);
            eg22osc = f5;
            eg22vcf = f6;
            continue;

        /* 'eg1 0.100000 0.000051 0.000000 0.000142 0.000000 0.000001' */
        } else if (format == 0 &&
                   y_sscanf(buf, " eg1 %f %f %f %f %f %f",
                   // eg1_attack_time, eg1_decay_time,
                   // eg1_sustain_level, eg1_release_time,
                   // eg1_amount_o, eg1_amount_f
                            &f0, &f1, &f2, &f3, &f5, &f6) == 6) {

            xi_set_eg(&tmp.ego, f0, f1, xi_amp_to_volume_cv(f2),
                                            f3, 0.0f);
            xi_set_eg(&tmp.eg1, f0, f1, f2, f3, 0.0f);
            eg12osc = f5;
            eg12vcf = f6;
            continue;

        /* 'eg2 0.100000 0.100000 1.000000 0.100000 0.000000 0.000001' */
        } else if (format == 0 &&
                   y_sscanf(buf, " eg2 %f %f %f %f %f %f",
                            &f0, &f1, &f2, &f3, &f5, &f6) == 6) {

            xi_set_eg(&tmp.eg2, f0, f1, f2, f3, 0.0f);
            eg22osc = f5;
            eg22vcf = f6;
            continue;

        /* 'vcf 4.91945 0.37905 0' */
        } else if (y_sscanf(buf, " vcf %f %f %d", &f0, &f1, &i0) == 3) {

            tmp.vcf1.frequency = f0;
            tmp.vcf1.qres = f1 / 1.995f;
            tmp.vcf1.mode = i0 + 1;
            if (dual) {
                tmp.vcf2.frequency = f0;
                tmp.vcf2.qres = f1 / 1.995f;
                tmp.vcf2.mode = i0 + 1;
            }
            continue;

        /* 'glide 0.984375' */
        } else if (y_sscanf(buf, " glide %f", &tmp.glide_time) == 1) {

            continue;

        /* 'volume 0.5' */
        } else if (y_sscanf(buf, " volume %f", &tmp.volume) == 1) {

            continue;

        /* 'xsynth-dssi patch end' */
        } else if (sscanf(buf, " xsynth-dssi patch %3s", buf2) == 1 &&
                   !strcmp(buf2, "end")) {

            break; /* finished */

        } else {

            return 0; /* unrecognized line */
        }
    }

    /* try to sort out the oscillator 2 modulation */
    /* -FIX- properly handle the remaining multiple modulation source cases */
    if (fabsf(lfo2osc) < 1e-5 && eg12osc < 1e-5 && eg22osc < 1e-5) {
        /* no oscillator 2 modulation */
    } else if (fabsf(lfo2osc) >= eg12osc && fabsf(lfo2osc) >= eg22osc) { /* lfo strongest */
        tmp.osc2.pitch_mod_src = Y_MOD_VLFO;
        tmp.osc2.pitch_mod_amt = lfo2osc;
        if (dual) {
            tmp.osc4.pitch_mod_src = Y_MOD_VLFO;
            tmp.osc4.pitch_mod_amt = lfo2osc;
        }
    } else if (eg12osc >= fabsf(lfo2osc) && eg12osc >= eg22osc) { /* eg1 strongest */
        tmp.osc2.pitch_mod_src = Y_MOD_EG1;
        tmp.osc2.pitch_mod_amt = eg12osc;
        if (dual) {
            tmp.osc4.pitch_mod_src = Y_MOD_EG1;
            tmp.osc4.pitch_mod_amt = eg12osc;
        }
        if (fabsf(lfo2osc) > 1e-5) {
            tmp.eg1.amp_mod_src = Y_MOD_VLFO_UP;
            tmp.eg1.amp_mod_amt = lfo2osc;
        }
    } else if (eg22osc >= fabsf(lfo2osc) && eg22osc >= eg12osc) { /* eg2 strongest */
        tmp.osc2.pitch_mod_src = Y_MOD_EG2;
        tmp.osc2.pitch_mod_amt = eg22osc;
        if (dual) {
            tmp.osc4.pitch_mod_src = Y_MOD_EG2;
            tmp.osc4.pitch_mod_amt = eg22osc;
        }
        if (fabsf(lfo2osc) > 1e-5) {
            tmp.eg2.amp_mod_src = Y_MOD_VLFO_UP;
            tmp.eg2.amp_mod_amt = lfo2osc;
        }
    } else {
        fprintf(stderr, "'%s': unhandled oscillator 2 modulation case: lfo %f, eg1 %f, eg2 %f\n", tmp.name, lfo2osc, eg12osc, eg22osc);
    }

    /* try to sort out the filter modulation */
    /* -FIX- properly handle the remaining multiple modulation source cases */
    if (fabsf(lfo2vcf) < 1e-5 && eg12vcf < 1e-5 && eg22vcf < 1e-5) {
        /* no filter modulation */
    } else if (fabsf(lfo2vcf) >= eg12vcf && fabsf(lfo2vcf) >= eg22vcf) { /* lfo strongest */
        tmp.vcf1.freq_mod_src = Y_MOD_VLFO;
        tmp.vcf1.freq_mod_amt = lfo2vcf;
        if (dual) {
            tmp.vcf2.freq_mod_src = Y_MOD_VLFO;
            tmp.vcf2.freq_mod_amt = lfo2vcf;
        }
    } else if (eg12vcf >= fabsf(lfo2vcf) && eg12vcf >= eg22vcf) { /* eg1 strongest */
        tmp.vcf1.freq_mod_src = Y_MOD_EG1;
        tmp.vcf1.freq_mod_amt = eg12vcf / 50.0f;
        if (dual) {
            tmp.vcf2.freq_mod_src = Y_MOD_EG1;
            tmp.vcf2.freq_mod_amt = eg12vcf / 50.0f;
        }
        if (fabsf(lfo2vcf) > 1e-5) {
            tmp.eg1.amp_mod_src = Y_MOD_VLFO_UP;
            tmp.eg1.amp_mod_amt = lfo2vcf;
        }
    } else if (eg22vcf >= fabsf(lfo2vcf) && eg22vcf >= eg12vcf) { /* eg2 strongest */
        tmp.vcf1.freq_mod_src = Y_MOD_EG2;
        tmp.vcf1.freq_mod_amt = eg22vcf / 50.0f;
        if (dual) {
            tmp.vcf2.freq_mod_src = Y_MOD_EG2;
            tmp.vcf2.freq_mod_amt = eg22vcf / 50.0f;
        }
        if (fabsf(lfo2vcf) > 1e-5) {
            tmp.eg2.amp_mod_src = Y_MOD_VLFO_UP;
            tmp.eg2.amp_mod_amt = lfo2vcf;
        }
    } else {
        fprintf(stderr, "'%s': unhandled filter modulation case: lfo %f, eg1 %f, eg2 %f\n", tmp.name, lfo2vcf, eg12vcf, eg22vcf);
    }

    memcpy(patch, &tmp, sizeof(y_patch_t));

    return 1;  /* -FIX- error handling yet to be implemented */
}

/*
 * gui_data_import_xsynth
 */
int
gui_data_import_xsynth(const char *filename, int position, int dual, char **message)
{
    FILE *fh;
    int count = 0;
    int index = position;
    char buffer[32];

    GDB_MESSAGE(GDB_IO, " gui_data_import_xsynth: attempting to load '%s'\n", filename);

    if ((fh = fopen(filename, "rb")) == NULL) {
        if (message) *message = strdup("could not open file for reading");
        return 0;
    }

    while (1) {
        gui_data_check_patches_allocation(index);
        if (!gui_data_read_xsynth_patch(fh, &patches[index], dual))
            break;
        count++;
        index++;
    }
    fclose(fh);

    if (!count) {
        if (message) *message = strdup("no patches recognized");
        return 0;
    }

    patches_dirty = 1;  /* always */
    if (index > patch_count)
        patch_count = index;

    if (message) {
        snprintf(buffer, 32, "loaded %d patches", count);
        *message = strdup(buffer);
    }
    return count;
}

/* ==== Interpret K4 Patches... ==== */

/* I don't own a K4, so all of this is just a big guess based on the
 * product manual.  Only the most basic patch parameters are
 * converted, while many others are ignored.  Still, it results in
 * a few interesting conversions, if only as starting points for new
 * WhySynth patches. */

y_patch_t y_init_voice_k4_single = {
        /* -PORTS- */
        "- K4oid single init voice -",
        "",
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5 },
        { 1, 0, 50, 0, 0, 0, 0 },
        { 0, 0, 50, 0, 0, 0, 0 },
        0, 0.5, 0, 0.5, 1, 0.5, 0, 0.5,
        0.5,
        0, 0, 0, 0, 0, 0, 0, 0,
        0.984375, 2,
        { 1, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0 },
        90, 0,
        { 1, 3, 0.002, 1, 3, 0.001, 1, 3, 0, 1, 11, 0.2, 0, 0, 0, 0, 0 },
        { 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        { 0, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        { 0, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        { 0, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        1, 0, 0, 0, 0
};

y_patch_t y_init_voice_k4_dual = {
        /* -PORTS- */
        "- K4oid dual init voice -",
        "",
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5 },
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5 },
        { 1, 0, 50, 0, 0, 0, 0 },
        { 1, 1, 50, 0, 0, 0, 0 },
        0, 0.5, 0, 0.5, 0.71, 0.25, 0.71, 0.75,
        0.5,
        0, 0, 0, 0, 0, 0, 0, 0,
        0.984375, 2,
        { 1, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0 },
        90, 0,
        { 1, 3, 0.002, 1, 3, 0.001, 1, 3, 0, 1, 11, 0.2, 0, 0, 0, 0, 0 },
        { 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        { 0, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        { 0, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        { 0, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        1, 0, 0, 0, 0
};

y_patch_t y_init_voice_k4_twin = {
        /* -PORTS- */
        "- K4oid twin init voice -",
        "",
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5 },
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5 },
        { 1, 0, 50, 0, 0, 0, 0 },
        { 1, 1, 50, 0, 0, 0, 0 },
        0, 0.5, 0, 0.5, 0.71, 0.25, 0.71, 0.75,
        0.5,
        0, 0, 0, 0, 0, 0, 0, 0,
        0.984375, 2,
        { 1, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0 },
        90, 0,
        { 1, 3, 0.002, 1, 3, 0.001, 1, 3, 0, 1, 11, 0.2, 0, 0, 0, 0, 0 },
        { 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        { 0, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        { 0, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        { 0, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        1, 0, 0, 0, 0
};

y_patch_t y_init_voice_k4_double = {
        /* -PORTS- */
        "- K4oid double init voice -",
        "",
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0 },
        { 1, 0, 50, 0, 0, 0, 0 },
        { 1, 2, 50, 0, 0, 0, 0 },
        0, 0.5, 0, 0.5, 0, 0.5, 1, 0.5,
        0.5,
        0, 0, 0, 0, 0, 0, 0, 0,
        0.984375, 2,
        { 1, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0 },
        90, 0,
        { 1, 3, 0.002, 1, 3, 0.001, 1, 3, 0, 1, 11, 0.2, 0, 0, 0, 0, 0 },
        { 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        { 0, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        { 0, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        { 0, 1, 0.1, 1, 1, 0.1, 1, 1, 0.1, 1, 1, 0.2, 0, 0, 0, 0, 0 },
        1, 0, 0, 0, 0
};

unsigned char k4_wave_to_y_wave[96] = {
      0,   1,   2,   3,   4,   5,   6,   7,
      8,  14,  15,  16,  17,  18,  19,  21,
     22,  23,  12,  24,  26,  27,  28,  29,
     30,  31,  40,  42,  46,  47,  54,  56,
     57,  58,  83,  84,  95,  85,  86,  87,
     88,  89,  90,  91,  99, 100, 101, 108,
    114, 109, 110,  71, 118, 102,  98,  73,
     59,  66,  60,  92,  93,  94,  96, 103,
    122,  61, 115, 112, 113,  53,  48,  41,
     44,  43,  62,  63,  67,  69,  68,  70,
    124, 104, 105, 106, 111, 116, 117,  74,
     75,  77,  80,  81,  76,  72,  49,  32,
};

/* EGO needs to be run in gate mode, which leaves 4 egs to work with:
 * - single and dual need 3: two DCO and one DCF
 * - twin needs 6!
 * - double needs 5!
 */

struct k4_env {
    unsigned char p[5];  /* D, A, D, S, R */
};

int
k4_get_eg(int *egs_used, struct k4_env *eg, unsigned char delay, unsigned char attack,
           unsigned char decay, unsigned char sustain, unsigned char release)
{
    int i;
    unsigned char p[5];

    p[0] = delay;
    p[1] = attack;
    p[2] = (sustain == 100 ? 0 : decay);
    p[3] = sustain;
    p[4] = release;

    for (i = 0; i < *egs_used; i++) {
        if (abs(eg[i].p[0] - p[0]) > 2 ||
            abs(eg[i].p[1] - p[1]) > 2 ||
            abs(eg[i].p[2] - p[2]) > 2 ||
            abs(eg[i].p[3] - p[3]) > 2 ||
            abs(eg[i].p[4] - p[4]) > 2)
            continue;
        return i;
    }

    /* i = egs_used */
    eg[i].p[0] = p[0];
    eg[i].p[1] = p[1];
    eg[i].p[2] = p[2];
    eg[i].p[3] = p[3];
    eg[i].p[4] = p[4];

    (*egs_used)++;

    return i;
}

static inline float
k4_eg_time(unsigned char time)
{
    /* This is a complete guess: */
    return expf(logf(2) * ((float)(time - 100) / 10.0f)) * 20.0f;
}

void
k4_set_eg(y_patch_t *patch, int egnum, struct k4_env *eg, unsigned char *longest_release)
{
    struct peg *yeg = &patch->eg1;

    if (egnum < 0) return;

    yeg = &yeg[egnum]; /* now a pointer to eg1, eg2, eg3, or eg4 */

    if (longest_release && eg[egnum].p[4] > *longest_release)
        *longest_release = eg[egnum].p[4];

    if (eg[egnum].p[0] > 0) {
        yeg->mode = 2;  /* AAASR */
        yeg->time1 = k4_eg_time(eg[egnum].p[0]);
        yeg->level1 = 0.0f;
        yeg->time2 = k4_eg_time(eg[egnum].p[1]);
        yeg->level2 = 1.0f;
    } else {
        yeg->mode = 1;  /* ADSR */
        yeg->time1 = k4_eg_time(eg[egnum].p[1]);
    }
    yeg->time3 = k4_eg_time(eg[egnum].p[2]);
    yeg->level3 = (float)eg[egnum].p[3] / 100.0f;
    yeg->time4 = k4_eg_time(eg[egnum].p[4]);
}

void
k4_set_osc(y_patch_t *patch, unsigned char *data, int osc, int *wave, int eg)
{
    struct posc *posc = &patch->osc1;

    posc = &posc[osc];  /* now a pointer to osc1, osc2, osc3, or osc4 */

    posc->waveform = k4_wave_to_y_wave[wave[osc]];
    posc->pitch = (data[42 + osc] & 63) - 24;
    posc->detune = (float)(data[50 + osc] - 50) / 100.0f;
    posc->amp_mod_src = Y_MOD_EG1 + eg;
    posc->amp_mod_amt = 1.0f;
    if (data[54 + osc] & 0x02) {
        posc->pitch_mod_src = Y_MOD_VLFO;
        posc->pitch_mod_amt = 0.0f;  /* not even going to guess (data[23] - 50 => -50 to 50 => ?) */
    }
}

void
k4_set_vcf(y_patch_t *patch, unsigned char *data, int vcf, int eg)
{
    struct pvcf *pvcf = &patch->vcf1;
    float f;

    pvcf = &pvcf[vcf];  /* now a pointer to vcf1 or vcf2 */

    f = (float)(data[102 + vcf]) / 100.0f;
    pvcf->frequency = f * f * f * 50.0f;                             /* guess */
    if (eg >= 0) {
        pvcf->freq_mod_src = Y_MOD_EG1 + eg;
        pvcf->freq_mod_amt = (float)(data[112 + vcf] - 50) / 50.0f;  /* guess */
    } else {
        pvcf->freq_mod_src = 0;
        pvcf->freq_mod_amt = 0.0f;
    }
    pvcf->qres = (float)(data[104 + vcf] & 7);
    if (pvcf->qres > 0.0f)
        pvcf->qres = (pvcf->qres + 3.0f) / 11.0f;                    /* guess */
}

void
k4_set_lfos(y_patch_t *patch, unsigned char *data)
{
    int waveform;

    /* 'Vibrato' => VLFO */
    waveform = (data[14] & 0x30) >> 4;
    switch (waveform) {
        case 0:  patch->vlfo.waveform = xi_find_wave("LFO Tri");       break;
        case 1:  patch->vlfo.waveform = xi_find_wave("LFO Saw");       break; /* -FIX- may be inverted? */
        case 2:  patch->vlfo.waveform = xi_find_wave("LFO Square");    break;
        case 3:  patch->vlfo.waveform = xi_find_wave("LFO Rect 2/4");  break; /* -FIX- should be S/H */
    }
    /* These next two are complete guesses: */
    patch->vlfo.frequency = expf((sqrtf((float)data[16] / 20.0f) - 1.0f) * logf(10.0f));
    patch->vlfo.delay = (float)data[18] / 20.0f;

    /* -FIX- implement 'LFO' => MLFO */
}

/*
 * k4_interpret_patch
 */
int
k4_interpret_patch(int number, unsigned char *data, y_patch_t *patch, int dual)
{
    y_patch_t tmp;
    int i, j;
    int k4_mode;
    int mute[4];
    int wave[4];
    struct k4_env eg[6];
    int egs_used;
    int dco_eg[4], dcf_eg[2];
    unsigned char longest_release;

    k4_mode = data[13] & 3;
    switch (k4_mode) {
      default:
      case 0:  /* normal */
        memcpy(&tmp, dual ? &y_init_voice_k4_dual
                          : &y_init_voice_k4_single, sizeof(y_patch_t));
        j = dual ? 'S' : 's';
        break;
      case 1:  /* twin */
        memcpy(&tmp, &y_init_voice_k4_twin, sizeof(y_patch_t));
        j = 't';
        break;
      case 2:  /* double */
        memcpy(&tmp, &y_init_voice_k4_double, sizeof(y_patch_t));
        j = 'd';
        break;
    }

    /* snprintf(tmp.name, 31, "K4oid %2d%c:           ", number, j); */
    snprintf(tmp.name, 31, "K4oid:           ");
    j = strlen(tmp.name) - 10;
    for (i = 0; i < 10; i++) {
        if (data[i] >= 32 && data[i] < 127)
            tmp.name[j + i] = data[i];
        else
            tmp.name[j + i] = '?';
    }
    while (i > 0 && tmp.name[j + i - 1] == ' ') i--;
    tmp.name[j + i] = 0;
    strcpy(tmp.comment, "(Mis)Interpreted Kawai K4 patch");

    printf("%s", tmp.name);
    switch (k4_mode) {
      default:
      case 0:
        printf(" normal");
        break;
      case 1:
        printf(" twin  ");
        break;
      case 2:
        printf(" double");
        break;
    }

    /* these are opposite of what the manual says */
    mute[0] = (data[14] & 1);
    mute[1] = (data[14] & 2);
    if (k4_mode == 0) {
        mute[2] = 1;
        mute[3] = 1;
    } else {
        mute[2] = (data[14] & 4);
        mute[3] = (data[14] & 8);
    }

    wave[0] = data[38] + ((data[34] & 1) << 7);
    wave[1] = data[39] + ((data[35] & 1) << 7);
    wave[2] = data[40] + ((data[36] & 1) << 7);
    wave[3] = data[41] + ((data[37] & 1) << 7);

    j = 1;
    for (i = 0; i < 4; i++) {
        if (k4_mode == 0 && i >= 2)
            printf("  --- ");
        else if (mute[i])
            printf(" (%3d)", wave[i]);
        else {
            printf("  %3d ", wave[i]);
            /* if (wave[i] == 190) wave[i] = 0;  // where's my swellchoir? */
            if (wave[i] > 95) j = 0;
        }
    }
    if (j) {
        printf("\n");
    } else {
        printf(" - uses PCM wave(s), skipping\n");
        return 0;
    }

    for (i = 0; i < 4; i++)
        if (!mute[i])
            printf("   S%d: %3d %3d %3d %3d %3d\n", i + 1, data[30+i], data[62+i], data[66+i], data[70+i], data[74+i]);
    printf("   F1:     %3d %3d %3d %3d\n", data[116], data[118], data[120], data[122]);
    if (k4_mode == 1) /* twin */
        printf("   F2:     %3d %3d %3d %3d\n", data[117], data[119], data[121], data[123]);

    egs_used = 0;
    dco_eg[0] = dco_eg[1] = dco_eg[2] = dco_eg[3] = dcf_eg[0] = dcf_eg[1] = -1;
    if (!mute[0]) dco_eg[0] = k4_get_eg(&egs_used, eg, data[30], data[62], data[66], data[70], data[74]);
    if (!mute[1]) dco_eg[1] = k4_get_eg(&egs_used, eg, data[31], data[63], data[67], data[71], data[75]);
    if (!mute[2]) dco_eg[2] = k4_get_eg(&egs_used, eg, data[32], data[64], data[68], data[72], data[76]);
    if (!mute[3]) dco_eg[3] = k4_get_eg(&egs_used, eg, data[33], data[65], data[69], data[73], data[77]);
    if (data[112] != 50)  /* if dcf1 eg depth is not zero */
        dcf_eg[0] = k4_get_eg(&egs_used, eg, 0, data[116], data[118], data[120], data[122]);
    if (k4_mode == 1 && data[113] != 50) /* twin and dcf2 depth not zero */
        dcf_eg[1] = k4_get_eg(&egs_used, eg, 0, data[117], data[119], data[121], data[123]);
    if (egs_used > 4) {
        printf("   skipping, patch needs %d envelopes\n", egs_used);
        return 0;
    }
    printf("   Envelope assignments: DCO: %d %d %d %d  DCF: %d %d\n", dco_eg[0], dco_eg[1], dco_eg[2], dco_eg[3], dcf_eg[0], dcf_eg[1]);

    longest_release = 0; /* for ego release time */
    if (mute[0])
        tmp.osc1.mode = 0;
    else {
        k4_set_osc(&tmp, data, 0, wave, dco_eg[0]);
        k4_set_eg(&tmp, dco_eg[0], eg, &longest_release);
    }
    if (mute[1])
        tmp.osc2.mode = 0;
    else {
        k4_set_osc(&tmp, data, 1, wave, dco_eg[1]);
        k4_set_eg(&tmp, dco_eg[1], eg, &longest_release);
    }
    if (k4_mode == 0 || mute[2])
        tmp.osc3.mode = 0;
    else {
        k4_set_osc(&tmp, data, 2, wave, dco_eg[2]);
        k4_set_eg(&tmp, dco_eg[2], eg, &longest_release);
    }
    if (k4_mode == 0 || mute[3])
        tmp.osc4.mode = 0;
    else {
        k4_set_osc(&tmp, data, 3, wave, dco_eg[3]);
        k4_set_eg(&tmp, dco_eg[3], eg, &longest_release);
    }
    tmp.ego.time4 = k4_eg_time(longest_release);
    if (k4_mode == 0 && dual) {
        tmp.osc3 = tmp.osc1;
        tmp.osc4 = tmp.osc2;
        tmp.osc3.level_a = 0.0f;
        tmp.osc3.level_b = tmp.osc1.level_a;
        tmp.osc4.level_a = 0.0f;
        tmp.osc4.level_b = tmp.osc2.level_a;
    }
    k4_set_vcf(&tmp, data, 0, dcf_eg[0]);
    k4_set_eg(&tmp, dcf_eg[0], eg, NULL);
    if (k4_mode == 1) {  /* twin */
        k4_set_vcf(&tmp, data, 1, dcf_eg[1]);
        k4_set_eg(&tmp, dcf_eg[1], eg, NULL);
    } else if (k4_mode == 2) {  /* double */
        tmp.vcf2 = tmp.vcf1;
        tmp.vcf2.source = 2; /* filter 1 output */
    } else if (k4_mode == 0 && dual) {
        tmp.vcf2 = tmp.vcf1;
        tmp.vcf2.source = 1; /* bus b */
    }
    k4_set_lfos(&tmp, data);

    memcpy(patch, &tmp, sizeof(y_patch_t));

    return 1;
}

/*
 * gui_data_interpret_k4
 */
int
gui_data_interpret_k4(const char *filename, int position, int dual, char **message)
{
    FILE *fh;
    long filelength;
    unsigned char *raw_patch_data = NULL;
    int i;
    int count = 0;
    int index = position;
    char buffer[32];

    GDB_MESSAGE(GDB_IO, " gui_data_interpret_k4: attempting to load '%s'\n", filename);

    if ((fh = fopen(filename, "rb")) == NULL) {
        if (message) *message = strdup("could not open file for reading");
        return 0;
    }

    if (fseek(fh, 0, SEEK_END) ||
        (filelength = ftell(fh)) == -1 ||
        fseek(fh, 0, SEEK_SET)) {
        if (message) *message = strdup("couldn't get length of patch file");
        fclose(fh);
        return 0;
    }
    if (filelength == 0) {
        if (message) *message = strdup("patch file has zero length");
        fclose(fh);
        return 0;
    } else if (filelength > 16384) {
        if (message) *message = strdup("patch file is too large");
        fclose(fh);
        return 0;
    }

    if (!(raw_patch_data = (unsigned char *)malloc(filelength))) {
        if (message) *message = strdup("couldn't allocate memory for raw patch file");
        fclose(fh);
        return 0;
    }

    if (fread(raw_patch_data, 1, filelength, fh) != (size_t)filelength) {
        if (message) *message = strdup("short read on patch file");
        free(raw_patch_data);
        fclose(fh);
        return 0;
    }
    fclose(fh);

    /* figure out what kind of file it is */
    if (filelength > 6 &&
        raw_patch_data[0] == 0xf0 &&
        raw_patch_data[1] == 0x40 &&
        (raw_patch_data[2] & 0xf0) == 0x00 &&
        raw_patch_data[3] == 0x22 &&  /* All Patch Data dump */
        raw_patch_data[4] == 0x00 &&
        raw_patch_data[5] == 0x04) {  /* K4 */

        if (filelength != 15123 ||
            raw_patch_data[15122] != 0xf7) {

            if (message) *message = strdup("badly formatted K4 All Patch Data dump!");
            count = 0;

        } else {

            for (i = 0; i < 64; i++) {
                gui_data_check_patches_allocation(index);
                if (k4_interpret_patch(i, raw_patch_data + 8 + i * 131, &patches[index], dual)) {
#if 1 /* normal: */
                    count++;
                    index++;
#else /* duplicate elimination: */
                    {
                        int j,
                            n = offsetof(y_patch_t, osc1.mode);

                        for (j = 0; j < index; j++)
                            if (!memcmp((void *)((char *)(&patches[j]) + n),
                                        (void *)((char *)(&patches[index]) + n), sizeof(y_patch_t) - n))
                                break;
                        if (j == index) {
                            count++;
                            index++;
                        } else {
                            printf("   ****** duplicate patch, skipping ******\n");
                        }
                    }
#endif
                }
            }

            if (count == 0 && message)
                *message = strdup("no compatible patches in patch file");
        }

    } else {

        /* unsuccessful load */
        if (message) *message = strdup("unknown patch bank file format!");
        count = 0;

    }

    free(raw_patch_data);

    if (!count)
        return 0;

    patches_dirty = 1;  /* always */
    if (index > patch_count)
        patch_count = index;

    if (message) {
        snprintf(buffer, 32, "loaded %d patches", count);
        *message = strdup(buffer);
    }
    return count;
}

