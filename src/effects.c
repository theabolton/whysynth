/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2005-2008, 2010 Sean Bolton and others.
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
#include <math.h>

#include <ladspa.h>

#include "whysynth_types.h"
#include "dssp_event.h"
#include "effects.h"

/* Okay, this is kinda goofy, the result of having changed my mind at least
 * three times....
 *
 * Basically the idea was to allow y_instantiate() a way to query effects as to
 * their memory needs, then allocate one buffer to be shared between different
 * effects.
 *
 * Each effect needs a function effect_<name>_request_buffers(), which is
 * called during instantiate() to query the effect's memory needs, and again
 * from run_synth() any time the effect type changes.
 *
 * The first time this function is called (during instantiate()) is a "dry run",
 * and anything written to the allocated memory will be lost.  Subsequent times
 * this function is called will be to actually set up the memory for running
 * the effect.
 *
 * This function must call effects_request_buffer() for each piece of memory it
 * needs. This function (effect_<name>_request_buffers()) may access anything
 * within the first 4Kb of memory requested, but must not assume anything beyond
 * that will exist (since it won't the first time the function is called).
 *
 * At some point while allocating buffers, this function may call
 * effects_request_silencing_of_subsequent_allocations().  Any buffers allocated
 * after this call will be silenced before the effect_<name>_process() function
 * is called.
 */

/*
 * effects_request_buffer
 */
void *
effects_request_buffer(y_synth_t *synth, size_t size)
{
    void *p = (void *)((char *)(synth->effect_buffer) + synth->effect_buffer_allocation);

    synth->effect_buffer_allocation += size;
    if (synth->effect_buffer_highwater < synth->effect_buffer_allocation)
        synth->effect_buffer_highwater = synth->effect_buffer_allocation;
    /* printf("allocation = %ld, highwater = %ld\n", synth->effect_buffer_allocation, synth->effect_buffer_highwater); */
    return p;
}

/*
 * effects_setup
 */
int
effects_setup(y_synth_t *synth)
{
    synth->effect_buffer = (void *)malloc(4096);
    if (!synth->effect_buffer)
        return 0;

    synth->effect_buffer_highwater = 0;
    effects_start_allocation(synth);
    effect_reverb_request_buffers(synth);
    effects_start_allocation(synth);
    effect_delay_request_buffers(synth);
    effects_start_allocation(synth);
    effect_screverb_request_buffers(synth);

    if (synth->effect_buffer_highwater > 4096) {
        free(synth->effect_buffer);
        synth->effect_buffer = (void *)calloc(1, synth->effect_buffer_highwater);
        if (!synth->effect_buffer)
            return 0;
    }

    return 1;
}

/*
 * effects_cleanup
 */
void
effects_cleanup(y_synth_t *synth)
{
    if (synth->effect_buffer) free(synth->effect_buffer);
    synth->effect_buffer = NULL;
}

/*
 * effects_process
 */
void 
effects_process(y_synth_t *synth, unsigned long sample_count,
                LADSPA_Data *out_left, LADSPA_Data *out_right)
{
    unsigned long i;
    int current_effect_mode = lrintf(*(synth->effect_mode)); /* will not be 0 */

    if (current_effect_mode != synth->last_effect_mode) {

        synth->last_effect_mode = current_effect_mode;

        effects_start_allocation(synth);
        /* assume we need to silence entire allocation unless effect says otherwise */
        effects_request_silencing_of_subsequent_allocations(synth);
        switch(current_effect_mode) {
          case 1:  /* Tim Goetze's Plate2x2 reverb */
            effect_reverb_request_buffers(synth);
            effect_reverb_setup(synth);
            break;

          case 2:  /* Dual delay */
            effect_delay_request_buffers(synth);
            effect_delay_setup(synth);
            break;

          case 3:  /* Sean Costello's reverb */
            effect_screverb_request_buffers(synth);
            effect_screverb_setup(synth);
            break;
        }
    }

    if (synth->effect_buffer_silence_count) /* buffer is still dirty */
        current_effect_mode = 0;

    switch(current_effect_mode) {

      case 0:  /* effect buffer is still dirty, so silence some of it after running the DC blocker */
        {   float r = synth->dc_block_r,
                  l_xnm1 = synth->dc_block_l_xnm1,
                  l_ynm1 = synth->dc_block_l_ynm1,
                  r_xnm1 = synth->dc_block_r_xnm1,
                  r_ynm1 = synth->dc_block_r_ynm1;
            float dry = 1.0f - *(synth->effect_mix);

            for (i = 0; i < sample_count; i++) {
                l_ynm1 = synth->voice_bus_l[i] - l_xnm1 + r * l_ynm1;
                l_xnm1 = synth->voice_bus_l[i];
                out_left[i] = l_ynm1 * dry;
                r_ynm1 = synth->voice_bus_r[i] - r_xnm1 + r * r_ynm1;
                r_xnm1 = synth->voice_bus_r[i];
                out_right[i] = r_ynm1 * dry;
            }
            synth->dc_block_l_xnm1 = l_xnm1;
            synth->dc_block_l_ynm1 = l_ynm1;
            synth->dc_block_r_xnm1 = r_xnm1;
            synth->dc_block_r_ynm1 = r_ynm1;

            i = synth->effect_buffer_allocation - synth->effect_buffer_silence_count;
            if (i > 8 * sample_count * sizeof(float)) {
                i = 8 * sample_count * sizeof(float);
                memset(synth->effect_buffer + synth->effect_buffer_silence_count, 0, i);
                synth->effect_buffer_silence_count += i;
            } else {
                memset(synth->effect_buffer + synth->effect_buffer_silence_count, 0, i);
                synth->effect_buffer_silence_count = 0;
            }
        }
        break;

      case 1:  /* Tim Goetze's Plate2x2 reverb */
        effect_reverb_process(synth, sample_count, out_left, out_right);
        break;

      case 2:  /* Dual delay */
        effect_delay_process(synth, sample_count, out_left, out_right);
        break;

      case 3:  /* Sean Costello's reverb */
        effect_screverb_process(synth, sample_count, out_left, out_right);
        break;
    }
}

