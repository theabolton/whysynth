/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2006, 2007 Sean Bolton.
 *
 * Interthread communication from ardour, copyright (C) 1999-
 * 2002 Paul Davis.
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

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <math.h>
#include <pthread.h>

#include "whysynth_types.h"
#include "whysynth.h"
#include "whysynth_ports.h"
#include "dssp_event.h"
#include "wave_tables.h"
#include "sampleset.h"
#include "padsynth.h"

/* ==== utility routines ==== */

static inline void
signal_worker_thread(void)
{
    char c;

    if (write(global.sampleset_pipe_fd[1], &c, 1) != 1) {
        YDB_MESSAGE(-1, " sampleset signal_worker_thread ERROR: cannot write pipe: %s\n", strerror(errno));
    }
}

static inline void
wait_for_signal(void)
{
    struct pollfd pfd;

    pfd.fd = global.sampleset_pipe_fd[0];
    pfd.events = POLLIN|POLLHUP|POLLERR;

    while (1) {
        if (poll(&pfd, 1, -1) >= 0)
            break;
        if (errno == EINTR)
            continue;  /* gdb at work, perhaps */
        YDB_MESSAGE(-1, " sampleset wait_for_signal ERROR: poll failed: %s\n", strerror(errno));
        sleep(1);
        return; /* bail */
    }

    if (pfd.revents & ~POLLIN) {
        YDB_MESSAGE(-1, " sampleset wait_for_signal ERROR: poll returned error\n");
        sleep(1);
        return; /* bail */
    }

    if (pfd.revents & POLLIN) {
        /* empty the pipe of all current requests */
        char buf[16];
        while (1) {
            size_t nread = read (global.sampleset_pipe_fd[0], &buf, sizeof (buf));

            if (nread > 0) {
                if ((size_t) nread < sizeof (buf)) {
                    break;
                } else {
                    continue;
                }
            } else if (nread == 0) {
                    break;
            } else if (errno == EAGAIN) {
                    break;
            } else {
                YDB_MESSAGE(-1, " sampleset wait_for_signal: ERROR reading from signal pipe\n");
                sleep(1);
                return; /* bail */
            }
        }
    }
}

void
sampleset_free(y_sampleset_t *ss)
{
    int i;
    y_sampleset_t *t, *prev;

    if (ss->set_up) {
        for (i = 0; i < WAVETABLE_MAX_WAVES; i++) {
            if (ss->sample[i])
                ss->sample[i]->ref_count--;
            if (ss->max_key[i] == 256)
                break;
        }
    }

    prev = NULL;
    for (t = global.active_sampleset_list; t; t = t->next) {
        if (t == ss) {
            if (prev) {
                prev->next = ss->next;
            } else {
                global.active_sampleset_list = ss->next;
            }
            break;
        }
        prev = t;
    }

    ss->next = global.free_sampleset_list;
    global.free_sampleset_list = ss;
}

y_sample_t *
sampleset_find_sample(y_sampleset_t *ss, int index)
{
    y_sample_t *s;

    if (ss->mode == Y_OSCILLATOR_MODE_PADSYNTH) {
        int param3 = ss->param3 & ~1;

        for (s = global.active_sample_list; s; s = s->next) {
            if (s->mode == Y_OSCILLATOR_MODE_PADSYNTH &&
                s->source == ss->source[index] &&
                s->max_key == ss->max_key[index] &&
                s->param1 == ss->param1 &&
                s->param2 == ss->param2 &&
                s->param3 == param3 &&
                s->param4 == ss->param4) {
                YDB_MESSAGE(YDB_SAMPLE, " sampleset_find_sample: matched sample %p to sampleset %p %d %d:%d=>%p@%d %d %d %d %d\n", s, ss, ss->mode, ss->waveform, index, ss->source[index], ss->max_key[index], ss->param1, ss->param2, param3, ss->param4);
                return s;
            }
        }
        /* YDB_MESSAGE(YDB_SAMPLE, " sampleset_find_sample: no match found for sampleset %p %d %d:%d=>%p %d %d %d %d\n", ss, ss->mode, ss->waveform, index, ss->source[index], ss->param1, ss->param2, param3, ss->param4); */
    }

    return NULL;
}

void
sampleset_dummy_sampletable_setup(y_sampleset_t *ss)
{
    int i;

    for (i = 0; i < WAVETABLE_MAX_WAVES; i++) {
        ss->source[i] = wavetable[ss->waveform].wave[i].data;
        ss->max_key[i] = wavetable[ss->waveform].wave[i].max_key;
        ss->sample[i] = NULL;
        if (ss->max_key[i] == 256)
            break;
    }
}

/* this just illustrates the minimum a render function needs to do */
int
sampleset_dummy_render(y_sample_t *sample)
{
    signed short *data = (signed short *)malloc((WAVETABLE_POINTS + 8) * sizeof(signed short));
    int i;

    if (!data)
        return 0;

    /* create (copy) sample data */
    memcpy(data + 4, sample->source, WAVETABLE_POINTS * sizeof(signed short));
    sample->data = data + 4;
    /* copy guard points */
    for (i = -4; i < 0; i++)
        sample->data[i] = sample->data[i + WAVETABLE_POINTS];
    for (i = 0; i < 4; i++)
        sample->data[WAVETABLE_POINTS + i] = sample->data[i];
    /* set int   length */
    sample->length = WAVETABLE_POINTS;
    /* set float period */
    sample->period = (float)WAVETABLE_POINTS;

    return 1;
}

/* ==== sampleset initialization and cleanup ==== */

int
sampleset_init(void)
{
    pthread_mutex_init(&global.sampleset_mutex, NULL);
    global.sampleset_pipe_fd[0] = -1;
    global.sampleset_pipe_fd[1] = -1;
    global.worker_thread_started = 0;
    global.worker_thread_done = 0;
    global.samplesets_allocated = 0;
    global.active_sampleset_list = NULL;
    global.free_sampleset_list = NULL;
    global.samples_allocated = 0;
    global.active_sample_list = NULL;
    global.free_sample_list = NULL;

    if (!padsynth_init())
        return 0;

    /* open unnamed pipe for interthread signaling */
    if (pipe(global.sampleset_pipe_fd)) {
        YDB_MESSAGE(-1, " sampleset_init: could not open signal pipe: %s\n", strerror(errno));
        padsynth_fini();
        return 0;
    }
    if (fcntl(global.sampleset_pipe_fd[0], F_SETFL, O_NONBLOCK) ||
        fcntl(global.sampleset_pipe_fd[1], F_SETFL, O_NONBLOCK)) {
        YDB_MESSAGE(-1, " sampleset_init: could not make signal pipe nonblocking: %s\n", strerror(errno));
        close(global.sampleset_pipe_fd[0]);
        close(global.sampleset_pipe_fd[1]);
        padsynth_fini();
        return 0;
    }

    /* create non-realtime worker thread */
    /* -FIX- optionally set this nice or low-priority SCHED_FIFO or SCHED_RR? */
    if (pthread_create(&global.worker_thread, NULL, sampleset_worker_function, NULL)) {
        YDB_MESSAGE(-1, " sampleset_init: could not create worker thread: %s\n", strerror(errno));
        close(global.sampleset_pipe_fd[0]);
        close(global.sampleset_pipe_fd[1]);
        padsynth_fini();
        return 0;
    }
    global.worker_thread_started = 1;
    YDB_MESSAGE(YDB_SAMPLE, " sampleset_init: worker thread started\n");

    return 1;
}

int
sampleset_instantiate(y_synth_t *synth)
{
    /* make sure enough samplesets and samples are allocated for instance count */
    while (global.samplesets_allocated < global.instance_count * 4 + 1) {
        y_sampleset_t *ss = (y_sampleset_t *)calloc(1, sizeof(y_sampleset_t));
        if (!ss)
            return 0;
        ss->next = global.free_sampleset_list;
        global.free_sampleset_list = ss;
        global.samplesets_allocated++;
    }

    while (global.samples_allocated < global.instance_count * 4 * WAVETABLE_MAX_WAVES + 1) {
        y_sample_t *s = (y_sample_t *)calloc(1, sizeof(y_sample_t));
        if (!s)
            return 0;
        s->next = global.free_sample_list;
        global.free_sample_list = s;
        global.samples_allocated++;
    }

    return 1;
}

void
sampleset_cleanup(y_synth_t *synth)
{
    if (synth->osc1.sampleset ||
        synth->osc2.sampleset ||
        synth->osc3.sampleset ||
        synth->osc4.sampleset) {

        pthread_mutex_lock(&global.sampleset_mutex);

        if (synth->osc1.sampleset) sampleset_release(synth->osc1.sampleset);
        if (synth->osc2.sampleset) sampleset_release(synth->osc2.sampleset);
        if (synth->osc3.sampleset) sampleset_release(synth->osc3.sampleset);
        if (synth->osc4.sampleset) sampleset_release(synth->osc4.sampleset);

        signal_worker_thread();
        pthread_mutex_unlock(&global.sampleset_mutex);
    }
}

void
sampleset_fini(void)
{
    y_sampleset_t *ss;
    y_sample_t *s;

    /* signal non-realtime worker thread to exit, and join it */
    if (global.worker_thread_started) {
        global.worker_thread_done = 1;
        signal_worker_thread();
        YDB_MESSAGE(YDB_SAMPLE, " sampleset_fini: waiting for worker thread to exit\n");
        pthread_join(global.worker_thread, NULL);
    }

    /* close interthread signaling pipe */
    if (global.sampleset_pipe_fd[0] >= 0) close(global.sampleset_pipe_fd[0]);
    if (global.sampleset_pipe_fd[1] >= 0) close(global.sampleset_pipe_fd[1]);

    /* free all sampleset resources */
    while (global.active_sampleset_list) {
        ss = global.active_sampleset_list;
        YDB_MESSAGE(YDB_SAMPLE, " sampleset_fini: quitting with active sampleset %p\n", ss);
        global.active_sampleset_list = ss->next;
        free(ss);
    }
    while (global.free_sampleset_list) {
        ss = global.free_sampleset_list;
        global.free_sampleset_list = ss->next;
        free(ss);
    }
    while (global.active_sample_list) {
        s = global.active_sample_list;
        YDB_MESSAGE(YDB_SAMPLE, " sampleset_fini: quitting with active sample %p\n", s);
        global.active_sample_list = s->next;
        free(s->data - 4);
        free(s);
    }
    while (global.free_sample_list) {
        s = global.free_sample_list;
        global.free_sample_list = s->next;
        free(s);
    }

    padsynth_fini();

    YDB_MESSAGE(YDB_SAMPLE, " sampleset_fini: done.\n");
}

/* ==== non-realtime worker thread ==== */

void *
sampleset_worker_function(void *arg)
{
    int render_needed,
        render_index = 0,
        i;
    y_sampleset_t *ss, *render_ss = NULL;
    y_sample_t *sample, *needs_freeing_sample_list, *prev;

    /* -FIX- ardour has:
     *    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, 0);
     *    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);
     * plus the SCHED_FIFO setting. Why the cancel settings?
     */

    while (!global.worker_thread_done) {
        wait_for_signal();

        YDB_MESSAGE(YDB_SAMPLE, " sampleset_worker_function: what needs to be done?\n");

        pthread_mutex_lock(&global.sampleset_mutex);

        /* loop until no samples needing to be rendered are found */
        do {
            /* scan the sampleset list, assigning any already-rendered samples */
            YDB_MESSAGE(YDB_SAMPLE, " sampleset_worker_function: beginning render check\n");
            render_needed = 0;
            for (ss = global.active_sampleset_list; ss; ss = ss->next) {
                int all_samples_rendered;

                if (ss->rendered)
                    continue;
                else if (!ss->set_up) {
                    if (ss->mode == Y_OSCILLATOR_MODE_PADSYNTH)
                        padsynth_sampletable_setup(ss);
                    else
                        sampleset_dummy_sampletable_setup(ss);
                    ss->set_up = 1;
                }

                all_samples_rendered = 1;
                for (i = 0; i < WAVETABLE_MAX_WAVES; i++) {
                    if (ss->sample[i] == NULL) {
                        sample = sampleset_find_sample(ss, i);
                        if (sample) {
                            sample->ref_count++;
                            ss->sample[i] = sample;
                        } else {
                            render_needed = 1;
                            render_ss = ss;
                            render_index = i;
                            all_samples_rendered = 0;
                        }
                    }
                    if (ss->max_key[i] == 256) {
                        if (all_samples_rendered)
                            ss->rendered = 1;
                        break;
                    }
                }
            }

            /* if we found a sample in need of rendering, bump the reference
             * count on its sampleset to make sure it doesn't get garbage
             * collected */
            if (render_needed)
                render_ss->ref_count++;

            /* all rendered samples should be assigned, so garbage collect now */
            YDB_MESSAGE(YDB_SAMPLE, " sampleset_worker_function: beginning garbage collect\n");
            ss = global.active_sampleset_list;
            while (ss) {
                if (ss->ref_count == 0) {
                    y_sampleset_t *t = ss;
                    ss = ss->next;
                    sampleset_free(t);
                    YDB_MESSAGE(YDB_SAMPLE, " sampleset_worker_function: freeing unused sampleset %p\n", t);
                } else
                    ss = ss->next;
            }
            needs_freeing_sample_list = NULL;
            prev = NULL;
            sample = global.active_sample_list;
            while (sample) {
                if (sample->ref_count == 0) {
                    y_sample_t *t = sample;
                    if (prev)
                        prev->next = sample->next;
                    else
                        global.active_sample_list = sample->next;
                    sample = sample->next;
                    t->next = needs_freeing_sample_list;
                    needs_freeing_sample_list = t;
                } else {
                    prev = sample;
                    sample = sample->next;
                }
            }

            /* free unused samples */
            if (needs_freeing_sample_list) {
                YDB_MESSAGE(YDB_SAMPLE, " sampleset_worker_function: freeing unused samples\n");

                pthread_mutex_unlock(&global.sampleset_mutex);

                for (sample = needs_freeing_sample_list; sample; sample = sample->next) {
                    YDB_MESSAGE(YDB_SAMPLE, " sampleset_worker_function: freeing unused sample %p\n", sample);
                    free(sample->data - 4);
                }
                
                pthread_mutex_lock(&global.sampleset_mutex);

                while (needs_freeing_sample_list) {
                    sample = needs_freeing_sample_list;
                    needs_freeing_sample_list = sample->next;
                    sample->next = global.free_sample_list;
                    global.free_sample_list = sample;
                }
            }

            /* render the sample */
            if (render_needed) {
                int rc;

                YDB_MESSAGE(YDB_SAMPLE, " sampleset_worker_function: beginning render\n");

                sample = global.free_sample_list;
                if (sample == NULL) {
                    YDB_MESSAGE(YDB_SAMPLE, " sampleset_worker_function ERROR: no free samples!\n");
                    render_needed = 0;
                    break;
                }
                global.free_sample_list = sample->next;

                sample->ref_count = 0;
                sample->mode     = render_ss->mode;
                sample->source   = render_ss->source[render_index];
                sample->max_key  = render_ss->max_key[render_index];
                sample->param1   = render_ss->param1;
                sample->param2   = render_ss->param2;
                sample->param3   = (render_ss->mode == Y_OSCILLATOR_MODE_PADSYNTH ?
                                        render_ss->param3 & ~1 : render_ss->param3);
                sample->param4   = render_ss->param4;

                YDB_MESSAGE(YDB_SAMPLE, " sampleset_worker_function: ready to render %p as %d %d:%d=>%p@%d %d %d %d %d\n", sample, sample->mode, render_ss->waveform, render_index, sample->source, sample->max_key, sample->param1, sample->param2, sample->param3, sample->param4);

                render_ss->ref_count--; /* now the sampleset can be freed */

                pthread_mutex_unlock(&global.sampleset_mutex);

                if (sample->mode == Y_OSCILLATOR_MODE_PADSYNTH) {
                    rc = padsynth_render(sample);
                } else {
                    rc = sampleset_dummy_render(sample);
                }

                pthread_mutex_lock(&global.sampleset_mutex);

                if (rc) {
                    sample->next = global.active_sample_list;
                    global.active_sample_list = sample;
                    YDB_MESSAGE(YDB_SAMPLE, " sampleset_worker_function: sample %p render succeeded!\n", sample);
                } else {
                    sample->next = global.free_sample_list;
                    global.free_sample_list = sample;
                    YDB_MESSAGE(YDB_SAMPLE, " sampleset_worker_function ERROR: sample %p render failed!\n", sample);
                    render_needed = 0;  /* bail */
                }
            }
        } while (render_needed && !global.worker_thread_done);

        padsynth_free_temp();

        pthread_mutex_unlock(&global.sampleset_mutex);

        YDB_MESSAGE(YDB_SAMPLE, " sampleset_worker_function: all done for now.\n");

    } /* while (!done) */

    return NULL;        
}

/* ==== realtime support routines ==== */

static inline void
sampleset_check_oscillator(y_synth_t *synth, y_sosc_t *sosc,
                           int *changed)
{
    int mode = lrintf(*(sosc->mode));

    if (mode == Y_OSCILLATOR_MODE_PADSYNTH) {
        int waveform = lrintf(*(sosc->waveform)),
            param1 = lrintf(*(sosc->mparam1) * 50.0f),
            param2 = lrintf(*(sosc->mparam2) * 20.0f),
            param3 = lrintf(*(sosc->mmod_src)),
            param4 = lrintf(*(sosc->mmod_amt) * 20.0f);

        if (param3 > 15) param3 = 0;

        if (sosc->sampleset) {
            y_sampleset_t *ss = sosc->sampleset;
            /* check oscillator parameters for changes */
            if (mode != ss->mode || waveform != ss->waveform ||
                param1 != ss->param1 || param2 != ss->param2 ||
                param3 != ss->param3 || param4 != ss->param4) {

                if (*changed || !pthread_mutex_trylock(&global.sampleset_mutex)) {
                    *changed = 1;
                    /* YDB_MESSAGE(YDB_SAMPLE, " sampleset_check_oscillator: change on oscillator %p\n", sosc); */
                    sampleset_release(sosc->sampleset);
                    sosc->sampleset = sampleset_setup(sosc, mode, waveform,
                                                      param1, param2, param3, param4);
                }
            }
        } else { /* set up new sampleset */
            if (*changed || !pthread_mutex_trylock(&global.sampleset_mutex)) {
                *changed = 1;
                /* YDB_MESSAGE(YDB_SAMPLE, " sampleset_check_oscillator: new for oscillator %p\n", sosc); */
                sosc->sampleset = sampleset_setup(sosc, mode, waveform,
                                                  param1, param2, param3, param4);
            }
        }
    } else {
        if (sosc->sampleset) { /* free sampleset resource we are no longer using */
            if (*changed || !pthread_mutex_trylock(&global.sampleset_mutex)) {
                *changed = 1;
                /* YDB_MESSAGE(YDB_SAMPLE, " sampleset_check_oscillator: freeing for oscillator %p\n", sosc); */
                sampleset_release(sosc->sampleset);
                sosc->sampleset = NULL;
            }
        }
    }
}

void
sampleset_check_oscillators(y_synth_t *synth)
{
    int changed = 0;

    sampleset_check_oscillator(synth, &synth->osc1, &changed);
    sampleset_check_oscillator(synth, &synth->osc2, &changed);
    sampleset_check_oscillator(synth, &synth->osc3, &changed);
    sampleset_check_oscillator(synth, &synth->osc4, &changed);

    if (changed) {
        signal_worker_thread();
        pthread_mutex_unlock(&global.sampleset_mutex);
    }
}

/*
 * sampleset_setup
 *
 * Search for a matching active sampleset, and increment its reference count
 * if found.  Otherwise, set up an immediately-ready temporary sampleset,
 * which will then have its samples rendered by the non-realtime worker thread.
 * The sampleset mutex must be locked before calling this.
 */
y_sampleset_t *
sampleset_setup(y_sosc_t *sosc, int mode, int waveform, int param1, int param2,
                int param3, int param4)
{
    y_sampleset_t *ss;

    for (ss = global.active_sampleset_list; ss; ss = ss->next) {
        if (mode == ss->mode && waveform == ss->waveform &&
            param1 == ss->param1 && param2 == ss->param2 &&
            param3 == ss->param3 && param4 == ss->param4) {
            ss->ref_count++;
            /* YDB_MESSAGE(YDB_SAMPLE, " sampleset_setup: found sampleset %p, ref_count now %d\n", ss, ss->ref_count); */
            return ss;
        }
    }

    /* no existing sampleset matches, so create a temporary one */
    ss = global.free_sampleset_list;
    if (ss == NULL) {
        YDB_MESSAGE(YDB_SAMPLE, " sampleset_setup ERROR: no free samplesets!\n");
        return NULL;
    }
    global.free_sampleset_list = ss->next;

    /* YDB_MESSAGE(YDB_SAMPLE, " sampleset_setup: setting up new %d %d %d %d %d %d\n", mode, waveform, param1, param2, param3, param4); */

    ss->ref_count = 1;
    ss->rendered = 0;
    ss->set_up = 0;

    ss->mode     = mode;
    ss->waveform = waveform;
    ss->param1   = param1;
    ss->param2   = param2;
    ss->param3   = param3;
    ss->param4   = param4;

    ss->next = global.active_sampleset_list;
    global.active_sampleset_list = ss;

    return ss;
}

/*
 * sampleset_release
 *
 * Release a sampleset: decrement its reference count, and if that becomes
 * zero, free the sampleset. The sampleset mutex must be locked before calling
 * this.
 */
void
sampleset_release(y_sampleset_t *ss)
{
    if (--ss->ref_count == 0) {
        /* YDB_MESSAGE(YDB_SAMPLE, " sampleset_release: freeing sampleset %p\n", ss); */

        sampleset_free(ss);

    } else {
        /* YDB_MESSAGE(YDB_SAMPLE, " sampleset_release: sampleset %p ref_count now %d\n", ss, ss->ref_count); */
    }
}

