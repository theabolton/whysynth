/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004-2006 Sean Bolton and others.
 *
 * Portions of this file may have come from Chris Cannam and Steve
 * Harris's public domain DSSI example code.
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

#ifndef _WHYSYNTH_H
#define _WHYSYNTH_H

/* ==== debugging ==== */

/* Y_DEBUG bits */
#define YDB_DSSI    1   /* DSSI interface */
#define YDB_AUDIO   2   /* audio output */
#define YDB_NOTE    4   /* note on/off, voice allocation */
#define YDB_DATA    8   /* plugin patchbank handling */
#define YDB_SAMPLE 16   /* non-realtime-rendered sampleset handling */
#define GDB_MAIN   32   /* GUI main program flow */
#define GDB_OSC    64   /* GUI OSC handling */
#define GDB_IO    128   /* GUI patch file input/output */
#define GDB_GUI   256   /* GUI GUI callbacks, updating, etc. */

/* If you want debug information, define Y_DEBUG to the YDB_* bits you're
 * interested in getting debug information about, bitwise-ORed together.
 * Otherwise, leave it undefined. */
// #define Y_DEBUG (1+8+16+32+64)

#ifdef Y_DEBUG

#include <stdio.h>
#define Y_DEBUG_INIT(x)
#define YDB_MESSAGE(type, fmt...) { if (Y_DEBUG & type) fprintf(stderr, "whysynth.so" fmt); }
#define GDB_MESSAGE(type, fmt...) { if (Y_DEBUG & type) fprintf(stderr, "WhySynth_gtk" fmt); }
// -FIX-:
// #include "message_buffer.h"
// #define Y_DEBUG_INIT(x)  mb_init(x)
// #define YDB_MESSAGE(type, fmt...) { \-
//     if (Y_DEBUG & type) { \-
//         char _m[256]; \-
//         snprintf(_m, 255, fmt); \-
//         add_message(_m); \-
//     } \-
// }

#else  /* !Y_DEBUG */

#define YDB_MESSAGE(type, fmt...)
#define GDB_MESSAGE(type, fmt...)
#define Y_DEBUG_INIT(x)

#endif  /* Y_DEBUG */

/* ==== end of debugging ==== */

#define Y_MAX_POLYPHONY     64
#define Y_DEFAULT_POLYPHONY 12

#endif /* _WHYSYNTH_H */

