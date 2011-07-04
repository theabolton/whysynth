/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004-2010, 2012 Sean Bolton and others.
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

#ifndef _WHYSYNTH_PORTS_H
#define _WHYSYNTH_PORTS_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ladspa.h>

/* -PORTS- */
#define Y_PORT_OUTPUT_LEFT          0
#define Y_PORT_OUTPUT_RIGHT         1

#define Y_PORT_OSC1_MODE            2
#define Y_PORT_OSC1_WAVEFORM        3
#define Y_PORT_OSC1_PITCH           4
#define Y_PORT_OSC1_DETUNE          5
#define Y_PORT_OSC1_PITCH_MOD_SRC   6
#define Y_PORT_OSC1_PITCH_MOD_AMT   7
#define Y_PORT_OSC1_MPARAM1         8   /* sync / grain lz / mod freq ratio / ws phase offset / slave freq / noise cutoff freq */
#define Y_PORT_OSC1_MPARAM2         9   /* pulsewidth / wave sel bias / grain spread / mod freq detune / mod amount bias */
#define Y_PORT_OSC1_MMOD_SRC       10   /* (pw | mod index | slave freq ) mod source / grain envelope */
#define Y_PORT_OSC1_MMOD_AMT       11   /* (pw | mod index | slave freq ) mod amount / grain pitch distribution */
#define Y_PORT_OSC1_AMP_MOD_SRC    12
#define Y_PORT_OSC1_AMP_MOD_AMT    13
#define Y_PORT_OSC1_LEVEL_A        14
#define Y_PORT_OSC1_LEVEL_B        15

#define Y_PORT_OSC2_MODE           16
#define Y_PORT_OSC2_WAVEFORM       17
#define Y_PORT_OSC2_PITCH          18
#define Y_PORT_OSC2_DETUNE         19
#define Y_PORT_OSC2_PITCH_MOD_SRC  20
#define Y_PORT_OSC2_PITCH_MOD_AMT  21
#define Y_PORT_OSC2_MPARAM1        22
#define Y_PORT_OSC2_MPARAM2        23
#define Y_PORT_OSC2_MMOD_SRC       24
#define Y_PORT_OSC2_MMOD_AMT       25
#define Y_PORT_OSC2_AMP_MOD_SRC    26
#define Y_PORT_OSC2_AMP_MOD_AMT    27
#define Y_PORT_OSC2_LEVEL_A        28
#define Y_PORT_OSC2_LEVEL_B        29

#define Y_PORT_OSC3_MODE           30
#define Y_PORT_OSC3_WAVEFORM       31
#define Y_PORT_OSC3_PITCH          32
#define Y_PORT_OSC3_DETUNE         33
#define Y_PORT_OSC3_PITCH_MOD_SRC  34
#define Y_PORT_OSC3_PITCH_MOD_AMT  35
#define Y_PORT_OSC3_MPARAM1        36
#define Y_PORT_OSC3_MPARAM2        37
#define Y_PORT_OSC3_MMOD_SRC       38
#define Y_PORT_OSC3_MMOD_AMT       39
#define Y_PORT_OSC3_AMP_MOD_SRC    40
#define Y_PORT_OSC3_AMP_MOD_AMT    41
#define Y_PORT_OSC3_LEVEL_A        42
#define Y_PORT_OSC3_LEVEL_B        43

#define Y_PORT_OSC4_MODE           44
#define Y_PORT_OSC4_WAVEFORM       45
#define Y_PORT_OSC4_PITCH          46
#define Y_PORT_OSC4_DETUNE         47
#define Y_PORT_OSC4_PITCH_MOD_SRC  48
#define Y_PORT_OSC4_PITCH_MOD_AMT  49
#define Y_PORT_OSC4_MPARAM1        50
#define Y_PORT_OSC4_MPARAM2        51
#define Y_PORT_OSC4_MMOD_SRC       52
#define Y_PORT_OSC4_MMOD_AMT       53
#define Y_PORT_OSC4_AMP_MOD_SRC    54
#define Y_PORT_OSC4_AMP_MOD_AMT    55
#define Y_PORT_OSC4_LEVEL_A        56
#define Y_PORT_OSC4_LEVEL_B        57

#define Y_PORT_VCF1_MODE           58
#define Y_PORT_VCF1_SOURCE         59
#define Y_PORT_VCF1_FREQUENCY      60
/* -FIX- add kbd tracking? */
#define Y_PORT_VCF1_FREQ_MOD_SRC   61
#define Y_PORT_VCF1_FREQ_MOD_AMT   62
#define Y_PORT_VCF1_QRES           63
#define Y_PORT_VCF1_MPARAM         64

#define Y_PORT_VCF2_MODE           65
#define Y_PORT_VCF2_SOURCE         66
#define Y_PORT_VCF2_FREQUENCY      67
#define Y_PORT_VCF2_FREQ_MOD_SRC   68
#define Y_PORT_VCF2_FREQ_MOD_AMT   69
#define Y_PORT_VCF2_QRES           70
#define Y_PORT_VCF2_MPARAM         71

#define Y_PORT_BUSA_LEVEL          72
#define Y_PORT_BUSA_PAN            73
#define Y_PORT_BUSB_LEVEL          74
#define Y_PORT_BUSB_PAN            75
#define Y_PORT_VCF1_LEVEL          76
#define Y_PORT_VCF1_PAN            77
#define Y_PORT_VCF2_LEVEL          78
#define Y_PORT_VCF2_PAN            79
#define Y_PORT_VOLUME              80

/* how many effects parameters? gverb needs 6 (5 with 'reflections/tail mix') */
#define Y_PORT_EFFECT_MODE         81
#define Y_PORT_EFFECT_PARAM1       82
#define Y_PORT_EFFECT_PARAM2       83
#define Y_PORT_EFFECT_PARAM3       84
#define Y_PORT_EFFECT_PARAM4       85
#define Y_PORT_EFFECT_PARAM5       86
#define Y_PORT_EFFECT_PARAM6       87
#define Y_PORT_EFFECT_MIX          88

#define Y_PORT_GLIDE_TIME          89
#define Y_PORT_BEND_RANGE          90

#define Y_PORT_GLFO_FREQUENCY      91
#define Y_PORT_GLFO_WAVEFORM       92
#define Y_PORT_GLFO_AMP_MOD_SRC    93
#define Y_PORT_GLFO_AMP_MOD_AMT    94

#define Y_PORT_VLFO_FREQUENCY      95
#define Y_PORT_VLFO_WAVEFORM       96
#define Y_PORT_VLFO_DELAY          97
#define Y_PORT_VLFO_AMP_MOD_SRC    98
#define Y_PORT_VLFO_AMP_MOD_AMT    99

#define Y_PORT_MLFO_FREQUENCY     100
#define Y_PORT_MLFO_WAVEFORM      101
#define Y_PORT_MLFO_DELAY         102
#define Y_PORT_MLFO_AMP_MOD_SRC   103
#define Y_PORT_MLFO_AMP_MOD_AMT   104
#define Y_PORT_MLFO_PHASE_SPREAD  105
#define Y_PORT_MLFO_RANDOM_FREQ   106

#define Y_PORT_EGO_MODE           107
#define Y_PORT_EGO_SHAPE1         108
#define Y_PORT_EGO_TIME1          109
#define Y_PORT_EGO_LEVEL1         110
#define Y_PORT_EGO_SHAPE2         111
#define Y_PORT_EGO_TIME2          112
#define Y_PORT_EGO_LEVEL2         113
#define Y_PORT_EGO_SHAPE3         114
#define Y_PORT_EGO_TIME3          115
#define Y_PORT_EGO_LEVEL3         116
#define Y_PORT_EGO_SHAPE4         117
#define Y_PORT_EGO_TIME4          118
#define Y_PORT_EGO_VEL_LEVEL_SENS 119
#define Y_PORT_EGO_VEL_TIME_SCALE 120
#define Y_PORT_EGO_KBD_TIME_SCALE 121
#define Y_PORT_EGO_AMP_MOD_SRC    122
#define Y_PORT_EGO_AMP_MOD_AMT    123

#define Y_PORT_EG1_MODE           124
#define Y_PORT_EG1_SHAPE1         125
#define Y_PORT_EG1_TIME1          126
#define Y_PORT_EG1_LEVEL1         127
#define Y_PORT_EG1_SHAPE2         128
#define Y_PORT_EG1_TIME2          129
#define Y_PORT_EG1_LEVEL2         130
#define Y_PORT_EG1_SHAPE3         131
#define Y_PORT_EG1_TIME3          132
#define Y_PORT_EG1_LEVEL3         133
#define Y_PORT_EG1_SHAPE4         134
#define Y_PORT_EG1_TIME4          135
#define Y_PORT_EG1_VEL_LEVEL_SENS 136
#define Y_PORT_EG1_VEL_TIME_SCALE 137
#define Y_PORT_EG1_KBD_TIME_SCALE 138
#define Y_PORT_EG1_AMP_MOD_SRC    139
#define Y_PORT_EG1_AMP_MOD_AMT    140

#define Y_PORT_EG2_MODE           141
#define Y_PORT_EG2_SHAPE1         142
#define Y_PORT_EG2_TIME1          143
#define Y_PORT_EG2_LEVEL1         144
#define Y_PORT_EG2_SHAPE2         145
#define Y_PORT_EG2_TIME2          146
#define Y_PORT_EG2_LEVEL2         147
#define Y_PORT_EG2_SHAPE3         148
#define Y_PORT_EG2_TIME3          149
#define Y_PORT_EG2_LEVEL3         150
#define Y_PORT_EG2_SHAPE4         151
#define Y_PORT_EG2_TIME4          152
#define Y_PORT_EG2_VEL_LEVEL_SENS 153
#define Y_PORT_EG2_VEL_TIME_SCALE 154
#define Y_PORT_EG2_KBD_TIME_SCALE 155
#define Y_PORT_EG2_AMP_MOD_SRC    156
#define Y_PORT_EG2_AMP_MOD_AMT    157

#define Y_PORT_EG3_MODE           158
#define Y_PORT_EG3_SHAPE1         159
#define Y_PORT_EG3_TIME1          160
#define Y_PORT_EG3_LEVEL1         161
#define Y_PORT_EG3_SHAPE2         162
#define Y_PORT_EG3_TIME2          163
#define Y_PORT_EG3_LEVEL2         164
#define Y_PORT_EG3_SHAPE3         165
#define Y_PORT_EG3_TIME3          166
#define Y_PORT_EG3_LEVEL3         167
#define Y_PORT_EG3_SHAPE4         168
#define Y_PORT_EG3_TIME4          169
#define Y_PORT_EG3_VEL_LEVEL_SENS 170
#define Y_PORT_EG3_VEL_TIME_SCALE 171
#define Y_PORT_EG3_KBD_TIME_SCALE 172
#define Y_PORT_EG3_AMP_MOD_SRC    173
#define Y_PORT_EG3_AMP_MOD_AMT    174

#define Y_PORT_EG4_MODE           175
#define Y_PORT_EG4_SHAPE1         176
#define Y_PORT_EG4_TIME1          177
#define Y_PORT_EG4_LEVEL1         178
#define Y_PORT_EG4_SHAPE2         179
#define Y_PORT_EG4_TIME2          180
#define Y_PORT_EG4_LEVEL2         181
#define Y_PORT_EG4_SHAPE3         182
#define Y_PORT_EG4_TIME3          183
#define Y_PORT_EG4_LEVEL3         184
#define Y_PORT_EG4_SHAPE4         185
#define Y_PORT_EG4_TIME4          186
#define Y_PORT_EG4_VEL_LEVEL_SENS 187
#define Y_PORT_EG4_VEL_TIME_SCALE 188
#define Y_PORT_EG4_KBD_TIME_SCALE 189
#define Y_PORT_EG4_AMP_MOD_SRC    190
#define Y_PORT_EG4_AMP_MOD_AMT    191

#define Y_PORT_MODMIX_BIAS        192
#define Y_PORT_MODMIX_MOD1_SRC    193
#define Y_PORT_MODMIX_MOD1_AMT    194
#define Y_PORT_MODMIX_MOD2_SRC    195
#define Y_PORT_MODMIX_MOD2_AMT    196

#define Y_PORT_TUNING             197

#define Y_PORTS_COUNT  198

#define Y_PORT_TYPE_OUTPUT       0  /* control output */
#define Y_PORT_TYPE_BOOLEAN      1
#define Y_PORT_TYPE_INTEGER      2  /* integer using spinbutton */
#define Y_PORT_TYPE_LINEAR       3
#define Y_PORT_TYPE_LOGARITHMIC  4
#define Y_PORT_TYPE_LOGSCALED    5  /* logarithmic scaling, but may include zero */
#define Y_PORT_TYPE_BPLOGSCALED  6  /* dual logarithmic scaling, must be symmetric around zero */
#define Y_PORT_TYPE_COMBO        7  /* integer using combobox */

#define Y_COMBO_TYPE_OSC_MODE      0
#define Y_COMBO_TYPE_OSC_WAVEFORM  1
#define Y_COMBO_TYPE_WT_WAVEFORM   2
#define Y_COMBO_TYPE_MOD_SRC       3
#define Y_COMBO_TYPE_MMOD_SRC      4
#define Y_COMBO_TYPE_FILTER_MODE   5
#define Y_COMBO_TYPE_FILTER_SRC    6
#define Y_COMBO_TYPE_EFFECT_MODE   7
#define Y_COMBO_TYPE_EG_MODE       8
#define Y_COMBO_TYPE_EG_SHAPE      9
#define Y_COMBO_TYPE_COUNT  10

#define Y_COMBOMODEL_TYPE_OSC_MODE          0
#define Y_COMBOMODEL_TYPE_WAVETABLE         1
#define Y_COMBOMODEL_TYPE_MINBLEP_WAVEFORM  2
#define Y_COMBOMODEL_TYPE_NOISE_WAVEFORM    3
#define Y_COMBOMODEL_TYPE_MOD_SRC           4
#define Y_COMBOMODEL_TYPE_GLFO_MOD_SRC      5
#define Y_COMBOMODEL_TYPE_GRAIN_ENV         6
#define Y_COMBOMODEL_TYPE_PADSYNTH_MODE     7
#define Y_COMBOMODEL_TYPE_FILTER_MODE       8
#define Y_COMBOMODEL_TYPE_FILTER1_SRC       9
#define Y_COMBOMODEL_TYPE_FILTER2_SRC      10
#define Y_COMBOMODEL_TYPE_EFFECT_MODE      11
#define Y_COMBOMODEL_TYPE_EG_MODE          12
#define Y_COMBOMODEL_TYPE_EG_SHAPE         13
#define Y_COMBOMODEL_TYPE_PD_WAVEFORM      14
#define Y_COMBOMODEL_TYPE_COUNT   15

/* -FIX- enumerate them all: */
#define Y_OSCILLATOR_MODE_MINBLEP   1
#define Y_OSCILLATOR_MODE_AGRAN     3
#define Y_OSCILLATOR_MODE_NOISE     7
#define Y_OSCILLATOR_MODE_PADSYNTH  8
#define Y_OSCILLATOR_MODE_PD        9

#define Y_OSCILLATOR_MODE_COUNT    11
#define Y_FILTER_MODE_COUNT        12
#define Y_EFFECT_MODE_COUNT         3

#define Y_MOD_ONE        0
#define Y_MOD_MODWHEEL   1
#define Y_MOD_PRESSURE   2 /* channel for GLFO, channel+key for others */

#define Y_GLOBAL_MOD_GLFO     3
#define Y_GLOBAL_MOD_GLFO_UP  4
#define Y_GLOBAL_MODS_COUNT   5

#define Y_MOD_KEY        3
#define Y_MOD_VELOCITY   4
/* -FIX- add key gate, release velocity, ... */
#define Y_MOD_GLFO       5
#define Y_MOD_GLFO_UP    6
#define Y_MOD_VLFO       7
#define Y_MOD_VLFO_UP    8
#define Y_MOD_MLFO0      9
#define Y_MOD_MLFO0_UP  10
#define Y_MOD_MLFO1     11
#define Y_MOD_MLFO1_UP  12
#define Y_MOD_MLFO2     13
#define Y_MOD_MLFO2_UP  14
#define Y_MOD_MLFO3     15
#define Y_MOD_MLFO3_UP  16
#define Y_MOD_EGO       17
#define Y_MOD_EG1       18
#define Y_MOD_EG2       19
#define Y_MOD_EG3       20
#define Y_MOD_EG4       21
#define Y_MOD_MIX       22
/* -FIX- add custom MIDI or LADSPA mods? */

#define Y_MODS_COUNT    23

struct y_port_descriptor {

    LADSPA_PortDescriptor          port_descriptor;
    char *                         name;
    LADSPA_PortRangeHintDescriptor hint_descriptor;
    LADSPA_Data                    lower_bound;
    LADSPA_Data                    upper_bound;
    int                            type;
    float                          scale;  /* steepness of logarithmic scaling for '0LOG' and 'BPLOG' knob types */
    int                            subtype;

};

extern struct y_port_descriptor y_port_description[];

#endif /* _WHYSYNTH_PORTS_H */

