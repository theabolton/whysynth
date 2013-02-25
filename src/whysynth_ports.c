/* WhySynth DSSI software synthesizer plugin and GUI
 *
 * Copyright (C) 2004-2010, 2013 Sean Bolton and others.
 *
 * Portions of this file may have come from Steve Brookes'
 * Xsynth, copyright (C) 1999 S. J. Brookes.
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

#include <ladspa.h>

#include "whysynth_ports.h"
#include "agran_oscillator.h"

struct y_port_descriptor y_port_description[Y_PORTS_COUNT] = {

#define PD_OUT     (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO)
#define PD_IN      (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL)
#define HD_MIN     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM)
#define HD_LOW     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW)
#define HD_MID     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE)
#define HD_HI      (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_HIGH)
#define HD_MAX     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM)
#define HD_440     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_440)
#define HD_LOG     (LADSPA_HINT_LOGARITHMIC)
#define HD_DETENT  (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_MINIMUM)
#define HD_DETENT0 (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0)
#define HD_SWITCH  (LADSPA_HINT_TOGGLED | LADSPA_HINT_DEFAULT_0)
#define YPT_OUT    Y_PORT_TYPE_OUTPUT
#define YPT_BOOL   Y_PORT_TYPE_BOOLEAN
#define YPT_INT    Y_PORT_TYPE_INTEGER
#define YPT_LIN    Y_PORT_TYPE_LINEAR
#define YPT_LOG    Y_PORT_TYPE_LOGARITHMIC
#define YPT_0LOG   Y_PORT_TYPE_LOGSCALED
#define YPT_BPLOG  Y_PORT_TYPE_BPLOGSCALED
#define YPT_COMBO  Y_PORT_TYPE_COMBO
#define YPT_PAN    Y_PORT_TYPE_PAN

/* last mod source */
#define MOD_LIM (Y_MODS_COUNT-1)
/* greater of last mod source or last grain envelope */
#define OSC_MMS (AG_GRAIN_ENVELOPE_COUNT-1)
/* last (useable) global mod */
#define GMOD_LIM  (Y_GLOBAL_MOD_GLFO-1)

    /* -PORTS- */
    { PD_OUT, "Output Left",         0,               0.0f,     0.0f,   0,         0. },
    { PD_OUT, "Output Right",        0,               0.0f,     0.0f,   0,         0. },

    { PD_IN,  "Osc1 Mode",           HD_DETENT,       0.0f,  Y_OSCILLATOR_MODE_COUNT,
                                                                      YPT_COMBO,   0., Y_COMBO_TYPE_OSC_MODE },
    { PD_IN,  "Osc1 Waveform",       HD_DETENT,       0.0f,    95.0f, YPT_COMBO,   0., Y_COMBO_TYPE_OSC_WAVEFORM },
    { PD_IN,  "Osc1 Pitch",          HD_DETENT0,    -36.0f,    36.0f,   YPT_INT,   0. },
    { PD_IN,  "Osc1 Detune",         HD_MID,         -0.5f,     0.5f, YPT_BPLOG,   1.4 },
    { PD_IN,  "Osc1 Pitch Mod Src",  HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "Osc1 Pitch Mod Amt",  HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },
    { PD_IN,  "Osc1 MParam 1",       HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Osc1 MParam 2",       HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Osc1 MMod Src",       HD_DETENT,       0.0f,  OSC_MMS, YPT_COMBO,   0., Y_COMBO_TYPE_MMOD_SRC },
    { PD_IN,  "Osc1 MMod Amt",       HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Osc1 Amp Mod Src",    HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "Osc1 Amp Mod Amt",    HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },
    { PD_IN,  "Osc1->BusA Level",    HD_MID,          0.0f,     2.0f,  YPT_0LOG,   2. },
    { PD_IN,  "Osc1->BusB Level",    HD_MID,          0.0f,     2.0f,  YPT_0LOG,   2. },

    { PD_IN,  "Osc2 Mode",           HD_DETENT,       0.0f,  Y_OSCILLATOR_MODE_COUNT,
                                                                      YPT_COMBO,   0., Y_COMBO_TYPE_OSC_MODE },
    { PD_IN,  "Osc2 Waveform",       HD_DETENT,       0.0f,    95.0f, YPT_COMBO,   0., Y_COMBO_TYPE_OSC_WAVEFORM },
    { PD_IN,  "Osc2 Pitch",          HD_DETENT0,    -36.0f,    36.0f,   YPT_INT,   0. },
    { PD_IN,  "Osc2 Detune",         HD_MID,         -0.5f,     0.5f, YPT_BPLOG,   1.4 },
    { PD_IN,  "Osc2 Pitch Mod Src",  HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "Osc2 Pitch Mod Amt",  HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },
    { PD_IN,  "Osc2 MParam 1",       HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Osc2 MParam 2",       HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Osc2 MMod Src",       HD_DETENT,       0.0f,  OSC_MMS, YPT_COMBO,   0., Y_COMBO_TYPE_MMOD_SRC },
    { PD_IN,  "Osc2 MMod Amt",       HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Osc2 Amp Mod Src",    HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "Osc2 Amp Mod Amt",    HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },
    { PD_IN,  "Osc2->BusA Level",    HD_MID,          0.0f,     2.0f,  YPT_0LOG,   2. },
    { PD_IN,  "Osc2->BusB Level",    HD_MID,          0.0f,     2.0f,  YPT_0LOG,   2. },

    { PD_IN,  "Osc3 Mode",           HD_DETENT,       0.0f,  Y_OSCILLATOR_MODE_COUNT,
                                                                      YPT_COMBO,   0., Y_COMBO_TYPE_OSC_MODE },
    { PD_IN,  "Osc3 Waveform",       HD_DETENT,       0.0f,    95.0f, YPT_COMBO,   0., Y_COMBO_TYPE_OSC_WAVEFORM },
    { PD_IN,  "Osc3 Pitch",          HD_DETENT0,    -36.0f,    36.0f,   YPT_INT,   0. },
    { PD_IN,  "Osc3 Detune",         HD_MID,         -0.5f,     0.5f, YPT_BPLOG,   1.4 },
    { PD_IN,  "Osc3 Pitch Mod Src",  HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "Osc3 Pitch Mod Amt",  HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },
    { PD_IN,  "Osc3 MParam 1",       HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Osc3 MParam 2",       HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Osc3 MMod Src",       HD_DETENT,       0.0f,  OSC_MMS, YPT_COMBO,   0., Y_COMBO_TYPE_MMOD_SRC },
    { PD_IN,  "Osc3 MMod Amt",       HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Osc3 Amp Mod Src",    HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "Osc3 Amp Mod Amt",    HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },
    { PD_IN,  "Osc3->BusA Level",    HD_MID,          0.0f,     2.0f,  YPT_0LOG,   2. },
    { PD_IN,  "Osc3->BusB Level",    HD_MID,          0.0f,     2.0f,  YPT_0LOG,   2. },

    { PD_IN,  "Osc4 Mode",           HD_DETENT,       0.0f,  Y_OSCILLATOR_MODE_COUNT,
                                                                      YPT_COMBO,   0., Y_COMBO_TYPE_OSC_MODE },
    { PD_IN,  "Osc4 Waveform",       HD_DETENT,       0.0f,    95.0f, YPT_COMBO,   0., Y_COMBO_TYPE_OSC_WAVEFORM },
    { PD_IN,  "Osc4 Pitch",          HD_DETENT0,    -36.0f,    36.0f,   YPT_INT,   0. },
    { PD_IN,  "Osc4 Detune",         HD_MID,         -0.5f,     0.5f, YPT_BPLOG,   1.4 },
    { PD_IN,  "Osc4 Pitch Mod Src",  HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "Osc4 Pitch Mod Amt",  HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },
    { PD_IN,  "Osc4 MParam 1",       HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Osc4 MParam 2",       HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Osc4 MMod Src",       HD_DETENT,       0.0f,  OSC_MMS, YPT_COMBO,   0., Y_COMBO_TYPE_MMOD_SRC },
    { PD_IN,  "Osc4 MMod Amt",       HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Osc4 Amp Mod Src",    HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "Osc4 Amp Mod Amt",    HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },
    { PD_IN,  "Osc4->BusA Level",    HD_MID,          0.0f,     2.0f,  YPT_0LOG,   2. },
    { PD_IN,  "Osc4->BusB Level",    HD_MID,          0.0f,     2.0f,  YPT_0LOG,   2. },

    { PD_IN,  "Filter1 Mode",        HD_DETENT,       0.0f,  Y_FILTER_MODE_COUNT,
                                                                      YPT_COMBO,   0., Y_COMBO_TYPE_FILTER_MODE },
    { PD_IN,  "Filter1 Source",      HD_DETENT,       0.0f,     1.0f, YPT_COMBO,   0., Y_COMBO_TYPE_FILTER_SRC },
    { PD_IN,  "Filter1 Frequency",   HD_MAX,          0.0f,    50.0f,   YPT_LIN,   0. },
    { PD_IN,  "Filter1 Frq Mod Src", HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "Filter1 Frq Mod Amt", HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },
    { PD_IN,  "Filter1 Resonance",   HD_LOW,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Filter1 MParameter",  HD_MID,          0.0f,     1.0f,   YPT_LIN,   0. },

    { PD_IN,  "Filter2 Mode",        HD_DETENT,       0.0f,  Y_FILTER_MODE_COUNT,
                                                                      YPT_COMBO,   0., Y_COMBO_TYPE_FILTER_MODE },
    { PD_IN,  "Filter2 Source",      HD_DETENT,       0.0f,     2.0f, YPT_COMBO,   0., Y_COMBO_TYPE_FILTER_SRC },
    { PD_IN,  "Filter2 Frequency",   HD_MAX,          0.0f,    50.0f,   YPT_LIN,   0. },
    { PD_IN,  "Filter2 Frq Mod Src", HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "Filter2 Frq Mod Amt", HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },
    { PD_IN,  "Filter2 Resonance",   HD_LOW,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Filter2 MParameter",  HD_MID,          0.0f,     1.0f,   YPT_LIN,   0. },

    { PD_IN,  "BusA->Out Level",     HD_MIN,          0.0f,     2.0f,  YPT_0LOG,   2. },
    { PD_IN,  "BusA->Out Pan",       HD_MID,          0.0f,     1.0f,   YPT_PAN,   0. },
    { PD_IN,  "BusB->Out Level",     HD_MIN,          0.0f,     2.0f,  YPT_0LOG,   2. },
    { PD_IN,  "BusB->Out Pan",       HD_MID,          0.0f,     1.0f,   YPT_PAN,   0. },
    { PD_IN,  "Filter1->Out Level",  HD_MID,          0.0f,     2.0f,  YPT_0LOG,   2. },
    { PD_IN,  "Filter1->Out Pan",    HD_LOW,          0.0f,     1.0f,   YPT_PAN,   0. },
    { PD_IN,  "Filter2->Out Level",  HD_MID,          0.0f,     2.0f,  YPT_0LOG,   2. },
    { PD_IN,  "Filter2->Out Pan",    HD_HI,           0.0f,     1.0f,   YPT_PAN,   0. },
    { PD_IN,  "Volume",              HD_LOW,          0.0f,     1.0f,   YPT_LIN,   0. },

    { PD_IN,  "Effect Mode",         HD_DETENT,       0.0f,  Y_EFFECT_MODE_COUNT,
                                                                      YPT_COMBO,   0., Y_COMBO_TYPE_EFFECT_MODE },
    { PD_IN,  "Effect Param 1",      HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Effect Param 2",      HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Effect Param 3",      HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Effect Param 4",      HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Effect Param 5",      HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Effect Param 6",      HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "Effect Wet/Dry Mix",  HD_MID,          0.0f,     1.0f,   YPT_LIN,   0. },

    { PD_IN,  "Glide Rate",          HD_MIN | HD_LOG, 0.002f,   1.0f,   YPT_LOG,   0. },  // -FIX- this needs to be adjusted for different cx rates! XXX change to 'Glide Time'
    { PD_IN,  "Bend Range",          HD_DETENT,       0.0f,    12.0f,   YPT_INT,   0. },

    { PD_IN,  "GLFO Frequency",      HD_MID | HD_LOG, 0.05f,   20.0f,   YPT_LOG,   0. },
    { PD_IN,  "GLFO Waveform",       HD_DETENT,       0.0f,    95.0f, YPT_COMBO,   0., Y_COMBO_TYPE_WT_WAVEFORM },
    { PD_IN,  "GLFO Amp Mod Src",    HD_DETENT,       0.0f, GMOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "GLFO Amp Mod Amt",    HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },

    { PD_IN,  "VLFO Frequency",      HD_MID | HD_LOG, 0.05f,   20.0f,   YPT_LOG,   0. },
    { PD_IN,  "VLFO Waveform",       HD_DETENT,       0.0f,    95.0f, YPT_COMBO,   0., Y_COMBO_TYPE_WT_WAVEFORM },
    { PD_IN,  "VLFO Delay",          HD_MIN,          0.0f,    10.0f,  YPT_0LOG,   1.6 },
    { PD_IN,  "VLFO Amp Mod Src",    HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "VLFO Amp Mod Amt",    HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },

    { PD_IN,  "MLFO Frequency",      HD_MID | HD_LOG, 0.05f,   20.0f,   YPT_LOG,   0. },
    { PD_IN,  "MLFO Waveform",       HD_DETENT,       0.0f,    95.0f, YPT_COMBO,   0., Y_COMBO_TYPE_WT_WAVEFORM },
    { PD_IN,  "MLFO Delay",          HD_MIN,          0.0f,    10.0f,  YPT_0LOG,   1.6 },
    { PD_IN,  "MLFO Amp Mod Src",    HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "MLFO Amp Mod Amt",    HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },
    { PD_IN,  "MLFO Phase Spread",   HD_MID,          0.0f,   180.0f,   YPT_LIN,   0. },
    { PD_IN,  "MLFO Random Freq",    HD_MIN,          0.0f,     1.0f,  YPT_0LOG,   1.5 },

    { PD_IN,  "EGO Mode",            HD_DETENT,       1.0f,     5.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_MODE },
    { PD_IN,  "EGO Shape 1",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EGO Time 1",          HD_MIN,          0.0f,    20.0f,  YPT_0LOG,   3. },
/* -FIX- make EG levels bipolar */
    { PD_IN,  "EGO Level 1",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EGO Shape 2",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EGO Time 2",          HD_MIN,          0.0f,    20.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EGO Level 2",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EGO Shape 3",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EGO Time 3",          HD_MIN,          0.0f,    50.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EGO Level 3",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EGO Shape 4",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EGO Time 4",          HD_MIN,          0.0f,    50.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EGO Vel Level Sens",  HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EGO Vel Time Scale",  HD_MID,         -1.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EGO Kbd Time Scale",  HD_MID,         -1.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EGO Amp Mod Src",     HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "EGO Amp Mod Amt",     HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },

    { PD_IN,  "EG1 Mode",            HD_DETENT,       0.0f,     5.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_MODE },
    { PD_IN,  "EG1 Shape 1",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG1 Time 1",          HD_MIN,          0.0f,    20.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG1 Level 1",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG1 Shape 2",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG1 Time 2",          HD_MIN,          0.0f,    20.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG1 Level 2",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG1 Shape 3",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG1 Time 3",          HD_MIN,          0.0f,    50.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG1 Level 3",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG1 Shape 4",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG1 Time 4",          HD_MIN,          0.0f,    50.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG1 Vel Level Sens",  HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG1 Vel Time Scale",  HD_MID,         -1.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG1 Kbd Time Scale",  HD_MID,         -1.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG1 Amp Mod Src",     HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "EG1 Amp Mod Amt",     HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },

    { PD_IN,  "EG2 Mode",            HD_DETENT,       0.0f,     5.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_MODE },
    { PD_IN,  "EG2 Shape 1",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG2 Time 1",          HD_MIN,          0.0f,    20.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG2 Level 1",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG2 Shape 2",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG2 Time 2",          HD_MIN,          0.0f,    20.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG2 Level 2",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG2 Shape 3",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG2 Time 3",          HD_MIN,          0.0f,    50.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG2 Level 3",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG2 Shape 4",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG2 Time 4",          HD_MIN,          0.0f,    50.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG2 Vel Level Sens",  HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG2 Vel Time Scale",  HD_MID,         -1.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG2 Kbd Time Scale",  HD_MID,         -1.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG2 Amp Mod Src",     HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "EG2 Amp Mod Amt",     HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },

    { PD_IN,  "EG3 Mode",            HD_DETENT,       0.0f,     5.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_MODE },
    { PD_IN,  "EG3 Shape 1",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG3 Time 1",          HD_MIN,          0.0f,    20.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG3 Level 1",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG3 Shape 2",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG3 Time 2",          HD_MIN,          0.0f,    20.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG3 Level 2",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG3 Shape 3",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG3 Time 3",          HD_MIN,          0.0f,    50.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG3 Level 3",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG3 Shape 4",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG3 Time 4",          HD_MIN,          0.0f,    50.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG3 Vel Level Sens",  HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG3 Vel Time Scale",  HD_MID,         -1.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG3 Kbd Time Scale",  HD_MID,         -1.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG3 Amp Mod Src",     HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "EG3 Amp Mod Amt",     HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },

    { PD_IN,  "EG4 Mode",            HD_DETENT,       0.0f,     5.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_MODE },
    { PD_IN,  "EG4 Shape 1",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG4 Time 1",          HD_MIN,          0.0f,    20.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG4 Level 1",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG4 Shape 2",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG4 Time 2",          HD_MIN,          0.0f,    20.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG4 Level 2",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG4 Shape 3",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG4 Time 3",          HD_MIN,          0.0f,    50.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG4 Level 3",         HD_MAX,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG4 Shape 4",         HD_DETENT,       0.0f,    11.0f, YPT_COMBO,   0., Y_COMBO_TYPE_EG_SHAPE },
    { PD_IN,  "EG4 Time 4",          HD_MIN,          0.0f,    50.0f,  YPT_0LOG,   3. },
    { PD_IN,  "EG4 Vel Level Sens",  HD_MIN,          0.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG4 Vel Time Scale",  HD_MID,         -1.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG4 Kbd Time Scale",  HD_MID,         -1.0f,     1.0f,   YPT_LIN,   0. },
    { PD_IN,  "EG4 Amp Mod Src",     HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "EG4 Amp Mod Amt",     HD_MID,         -1.0f,     1.0f, YPT_BPLOG,   2. },

    { PD_IN,  "ModMix Bias",         HD_MID,         -1.4f,     1.4f, YPT_BPLOG,   2. },
    { PD_IN,  "ModMix Mod1 Src",     HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "ModMix Mod1 Amt",     HD_MID,         -1.4f,     1.4f, YPT_BPLOG,   2. },
    { PD_IN,  "ModMix Mod2 Src",     HD_DETENT,       0.0f,  MOD_LIM, YPT_COMBO,   0., Y_COMBO_TYPE_MOD_SRC },
    { PD_IN,  "ModMix Mod2 Amt",     HD_MID,         -1.4f,     1.4f, YPT_BPLOG,   2. },

    // debug: { PD_IN,  "XXXXXXX",             HD_440,        415.3f,   466.2f,   YPT_LIN,   415.3,466.2,0. },

    { PD_IN,  "Tuning",              HD_440,        415.3f,   466.2f,   YPT_LIN,   0. }
#undef PD_OUT
#undef PD_IN
#undef HD_MIN
#undef HD_LOW
#undef HD_MID
#undef HD_HI
#undef HD_MAX
#undef HD_440
#undef HD_LOG
#undef HD_DETENT
#undef HD_DETENT0
#undef HD_SWITCH
#undef YPT_OUT
#undef YPT_BOOL
#undef YPT_INT
#undef YPT_LIN
#undef YPT_LOG
#undef YPT_0LOG
#undef YPT_BPLOG
#undef YPT_COMBO
#undef YPT_PAN
};
