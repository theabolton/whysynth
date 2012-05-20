/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004-2005 Sean Bolton and others.
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

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "whysynth_types.h"
#include "whysynth.h"
#include "dssp_synth.h"
#include "whysynth_voice.h"
#include "dssp_event.h"
#include "common_data.h"

/*
 * y_data_check_patches_allocation
 */
void
y_data_check_patches_allocation(y_synth_t *synth, int patch_index)
{
    if (patch_index >= synth->patches_allocated) {

        int n = (patch_index + 0x80) & 0xffff80,
            i;
        y_patch_t *p = (y_patch_t *)malloc(n * sizeof(y_patch_t));

        if (synth->patches) {
            memcpy(p, synth->patches, synth->patches_allocated * sizeof(y_patch_t));
            free(synth->patches);
        }
        synth->patches = p;

        for (i = synth->patches_allocated; i < n; i++) {
            memcpy(&synth->patches[i], &y_init_voice, sizeof(y_patch_t));
        }

        synth->patches_allocated = n;
    }
}

/*
 * y_data_friendly_patches
 *
 * give the new user a default set of good patches to get started with
 */
void
y_data_friendly_patches(y_synth_t *synth)
{
    y_data_check_patches_allocation(synth, y_friendly_patch_count);

    memcpy(synth->patches, y_friendly_patches, y_friendly_patch_count * sizeof(y_patch_t));

    synth->patch_count = y_friendly_patch_count;
}

/*
 * y_data_load
 */
char *
y_data_load(y_synth_t *synth, char *filename)
{
    FILE *fh;
    int count = 0;

    if ((fh = fopen(filename, "rb")) == NULL)
        return dssi_configure_message("load error: could not open file '%s'", filename);

    pthread_mutex_lock(&synth->patches_mutex);

    while (1) {
        y_data_check_patches_allocation(synth, count);
        if (!y_data_read_patch(fh, &synth->patches[count]))
            break;
        count++;
    }
    fclose(fh);

    if (!count) {
        pthread_mutex_unlock(&synth->patches_mutex);
        return dssi_configure_message("load error: no patches recognized in patch file '%s'", filename);
    }
    if (count > synth->patch_count)
        synth->patch_count = count;

    pthread_mutex_unlock(&synth->patches_mutex);

    return NULL; /* success */
}

