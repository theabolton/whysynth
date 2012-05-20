/* WhySynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2005-2008, 2010 Sean Bolton.
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

#ifdef Y_BOGUS_MLOCKALL
#include <string.h>
#endif

#include "wave_tables.h"

void
wave_tables_set_count(void)
{
#ifdef Y_BOGUS_MLOCKALL
    int i, j;
    signed short scratch[4];

    for (i = 0; wavetable[i].name; i++) {
        /* Mac OS X (as of Tiger, anyway) doesn't have a functioning mlockall(),
         * so we read (some of) the data for each wave to make sure it is paged
         * into memory, in the hopes of minimizing page faults later. */
        j = 0;
        while (1) {
            struct wave *w = &wavetable[i].wave[j];
            memcpy(scratch, w->data, sizeof(scratch));
            memcpy(scratch, w->data + 512, sizeof(scratch));
            memcpy(scratch, w->data + 1032 - sizeof(scratch), sizeof(scratch));
            if (wavetable[i].wave[j].max_key == 256)
                break;
            j++;
        }
    }
#else
    int i = 0;

    while (wavetable[i].name) i++;
#endif

    wavetables_count = i;
}

#ifdef Y_PLUGIN
extern signed short yw_sin_1_data[1032];
extern signed short yw_sin_2_data[1032];
extern signed short yw_sin_3_data[1032];
extern signed short yw_sin_4_data[1032];
extern signed short yw_sin_5_data[1032];
extern signed short yw_sin_6_data[1032];
extern signed short yw_sin_7_data[1032];
extern signed short yw_sin_8_data[1032];
extern signed short yw_sin_9_data[1032];
extern signed short yw_lfo_rect13_data[1032];
extern signed short yw_lfo_rect14_data[1032];
extern signed short yw_lfo_rect16_data[1032];
extern signed short yw_lfo_rect24_data[1032];
extern signed short yw_lfo_s_h_1_data[1032];
extern signed short yw_lfo_s_h_2_data[1032];
extern signed short yw_lfo_s_h_3_data[1032];
extern signed short yw_lfo_s_h_4_data[1032];
extern signed short yw_lfo_saw_data[1032];
extern signed short yw_lfo_square_data[1032];
extern signed short yw_lfo_tri_data[1032];
extern signed short yw_ws_cheby_t5_data[1032];
extern signed short k4_10_4_data[1032];
extern signed short k4_10_5_data[1032];
extern signed short k4_10_6_data[1032];
extern signed short k4_11_3_data[1032];
extern signed short k4_11_4_data[1032];
extern signed short k4_11_5_data[1032];
extern signed short k4_11_6_data[1032];
extern signed short k4_12_1_data[1032];
extern signed short k4_12_2_data[1032];
extern signed short k4_12_3_data[1032];
extern signed short k4_12_4_data[1032];
extern signed short k4_12_5_data[1032];
extern signed short k4_12_6_data[1032];
extern signed short k4_13_1_data[1032];
extern signed short k4_13_2_data[1032];
extern signed short k4_13_3_data[1032];
extern signed short k4_13_4_data[1032];
extern signed short k4_13_5_data[1032];
extern signed short k4_13_6_data[1032];
extern signed short k4_14_1_data[1032];
extern signed short k4_14_2_data[1032];
extern signed short k4_14_3_data[1032];
extern signed short k4_14_4_data[1032];
extern signed short k4_14_5_data[1032];
extern signed short k4_14_6_data[1032];
extern signed short k4_15_1_data[1032];
extern signed short k4_15_2_data[1032];
extern signed short k4_15_3_data[1032];
extern signed short k4_15_4_data[1032];
extern signed short k4_15_5_data[1032];
extern signed short k4_15_6_data[1032];
extern signed short k4_16_1_data[1032];
extern signed short k4_16_2_data[1032];
extern signed short k4_16_3_data[1032];
extern signed short k4_16_4_data[1032];
extern signed short k4_16_5_data[1032];
extern signed short k4_16_6_data[1032];
extern signed short k4_17_1_data[1032];
extern signed short k4_17_2_data[1032];
extern signed short k4_17_3_data[1032];
extern signed short k4_17_4_data[1032];
extern signed short k4_17_5_data[1032];
extern signed short k4_17_6_data[1032];
extern signed short k4_18_1_data[1032];
extern signed short k4_18_2_data[1032];
extern signed short k4_18_3_data[1032];
extern signed short k4_18_4_data[1032];
extern signed short k4_18_5_data[1032];
extern signed short k4_18_6_data[1032];
extern signed short k4_19_3_data[1032];
extern signed short k4_19_4_data[1032];
extern signed short k4_19_5_data[1032];
extern signed short k4_19_6_data[1032];
extern signed short k4_20_1_data[1032];
extern signed short k4_20_2_data[1032];
extern signed short k4_20_3_data[1032];
extern signed short k4_20_4_data[1032];
extern signed short k4_20_5_data[1032];
extern signed short k4_20_6_data[1032];
extern signed short k4_21_1_data[1032];
extern signed short k4_21_2_data[1032];
extern signed short k4_21_3_data[1032];
extern signed short k4_21_4_data[1032];
extern signed short k4_21_5_data[1032];
extern signed short k4_21_6_data[1032];
extern signed short k4_22_1_data[1032];
extern signed short k4_22_2_data[1032];
extern signed short k4_22_3_data[1032];
extern signed short k4_22_4_data[1032];
extern signed short k4_22_5_data[1032];
extern signed short k4_22_6_data[1032];
extern signed short k4_23_1_data[1032];
extern signed short k4_23_2_data[1032];
extern signed short k4_23_3_data[1032];
extern signed short k4_23_4_data[1032];
extern signed short k4_23_5_data[1032];
extern signed short k4_23_6_data[1032];
extern signed short k4_24_1_data[1032];
extern signed short k4_24_2_data[1032];
extern signed short k4_24_3_data[1032];
extern signed short k4_24_4_data[1032];
extern signed short k4_24_5_data[1032];
extern signed short k4_24_6_data[1032];
extern signed short k4_25_1_data[1032];
extern signed short k4_25_2_data[1032];
extern signed short k4_25_3_data[1032];
extern signed short k4_25_4_data[1032];
extern signed short k4_25_5_data[1032];
extern signed short k4_25_6_data[1032];
extern signed short k4_26_1_data[1032];
extern signed short k4_26_2_data[1032];
extern signed short k4_26_3_data[1032];
extern signed short k4_26_4_data[1032];
extern signed short k4_26_5_data[1032];
extern signed short k4_26_6_data[1032];
extern signed short k4_27_3_data[1032];
extern signed short k4_28_3_data[1032];
extern signed short k4_28_4_data[1032];
extern signed short k4_28_5_data[1032];
extern signed short k4_28_6_data[1032];
extern signed short k4_29_1_data[1032];
extern signed short k4_29_2_data[1032];
extern signed short k4_29_3_data[1032];
extern signed short k4_29_4_data[1032];
extern signed short k4_29_5_data[1032];
extern signed short k4_29_6_data[1032];
extern signed short k4_30_1_data[1032];
extern signed short k4_30_2_data[1032];
extern signed short k4_30_3_data[1032];
extern signed short k4_30_4_data[1032];
extern signed short k4_30_5_data[1032];
extern signed short k4_30_6_data[1032];
extern signed short k4_31_1_data[1032];
extern signed short k4_31_2_data[1032];
extern signed short k4_31_3_data[1032];
extern signed short k4_31_4_data[1032];
extern signed short k4_31_5_data[1032];
extern signed short k4_31_6_data[1032];
extern signed short k4_32_1_data[1032];
extern signed short k4_32_2_data[1032];
extern signed short k4_32_3_data[1032];
extern signed short k4_32_4_data[1032];
extern signed short k4_32_5_data[1032];
extern signed short k4_32_6_data[1032];
extern signed short k4_33_1_data[1032];
extern signed short k4_33_2_data[1032];
extern signed short k4_33_3_data[1032];
extern signed short k4_33_4_data[1032];
extern signed short k4_33_5_data[1032];
extern signed short k4_33_6_data[1032];
extern signed short k4_34_1_data[1032];
extern signed short k4_34_2_data[1032];
extern signed short k4_34_3_data[1032];
extern signed short k4_34_4_data[1032];
extern signed short k4_34_5_data[1032];
extern signed short k4_34_6_data[1032];
extern signed short k4_35_2_data[1032];
extern signed short k4_35_3_data[1032];
extern signed short k4_35_4_data[1032];
extern signed short k4_35_5_data[1032];
extern signed short k4_35_6_data[1032];
extern signed short k4_36_2_data[1032];
extern signed short k4_36_3_data[1032];
extern signed short k4_36_4_data[1032];
extern signed short k4_36_5_data[1032];
extern signed short k4_36_6_data[1032];
extern signed short k4_37_1_data[1032];
extern signed short k4_37_2_data[1032];
extern signed short k4_37_3_data[1032];
extern signed short k4_37_4_data[1032];
extern signed short k4_37_5_data[1032];
extern signed short k4_37_6_data[1032];
extern signed short k4_38_2_data[1032];
extern signed short k4_38_3_data[1032];
extern signed short k4_38_4_data[1032];
extern signed short k4_38_5_data[1032];
extern signed short k4_38_6_data[1032];
extern signed short k4_39_1_data[1032];
extern signed short k4_39_2_data[1032];
extern signed short k4_39_3_data[1032];
extern signed short k4_39_4_data[1032];
extern signed short k4_39_5_data[1032];
extern signed short k4_39_6_data[1032];
extern signed short k4_40_1_data[1032];
extern signed short k4_40_2_data[1032];
extern signed short k4_40_3_data[1032];
extern signed short k4_40_4_data[1032];
extern signed short k4_40_5_data[1032];
extern signed short k4_40_6_data[1032];
extern signed short k4_41_2_data[1032];
extern signed short k4_41_3_data[1032];
extern signed short k4_41_4_data[1032];
extern signed short k4_41_5_data[1032];
extern signed short k4_41_6_data[1032];
extern signed short k4_42_1_data[1032];
extern signed short k4_42_2_data[1032];
extern signed short k4_42_3_data[1032];
extern signed short k4_42_4_data[1032];
extern signed short k4_42_5_data[1032];
extern signed short k4_42_6_data[1032];
extern signed short k4_43_2_data[1032];
extern signed short k4_43_3_data[1032];
extern signed short k4_43_4_data[1032];
extern signed short k4_43_5_data[1032];
extern signed short k4_43_6_data[1032];
extern signed short k4_44_1_data[1032];
extern signed short k4_44_2_data[1032];
extern signed short k4_44_3_data[1032];
extern signed short k4_44_4_data[1032];
extern signed short k4_44_5_data[1032];
extern signed short k4_44_6_data[1032];
extern signed short k4_45_1_data[1032];
extern signed short k4_45_2_data[1032];
extern signed short k4_45_3_data[1032];
extern signed short k4_45_4_data[1032];
extern signed short k4_45_5_data[1032];
extern signed short k4_45_6_data[1032];
extern signed short k4_46_1_data[1032];
extern signed short k4_46_2_data[1032];
extern signed short k4_46_3_data[1032];
extern signed short k4_46_4_data[1032];
extern signed short k4_46_5_data[1032];
extern signed short k4_46_6_data[1032];
extern signed short k4_47_1_data[1032];
extern signed short k4_47_2_data[1032];
extern signed short k4_47_3_data[1032];
extern signed short k4_47_4_data[1032];
extern signed short k4_47_5_data[1032];
extern signed short k4_47_6_data[1032];
extern signed short k4_48_2_data[1032];
extern signed short k4_48_3_data[1032];
extern signed short k4_48_4_data[1032];
extern signed short k4_48_5_data[1032];
extern signed short k4_48_6_data[1032];
extern signed short k4_49_1_data[1032];
extern signed short k4_49_2_data[1032];
extern signed short k4_49_3_data[1032];
extern signed short k4_49_4_data[1032];
extern signed short k4_49_5_data[1032];
extern signed short k4_49_6_data[1032];
extern signed short k4_50_3_data[1032];
extern signed short k4_50_4_data[1032];
extern signed short k4_50_5_data[1032];
extern signed short k4_50_6_data[1032];
extern signed short k4_51_1_data[1032];
extern signed short k4_51_2_data[1032];
extern signed short k4_51_3_data[1032];
extern signed short k4_51_4_data[1032];
extern signed short k4_51_5_data[1032];
extern signed short k4_51_6_data[1032];
extern signed short k4_52_1_data[1032];
extern signed short k4_52_2_data[1032];
extern signed short k4_52_3_data[1032];
extern signed short k4_52_4_data[1032];
extern signed short k4_52_5_data[1032];
extern signed short k4_52_6_data[1032];
extern signed short k4_53_1_data[1032];
extern signed short k4_53_2_data[1032];
extern signed short k4_53_3_data[1032];
extern signed short k4_53_4_data[1032];
extern signed short k4_53_5_data[1032];
extern signed short k4_53_6_data[1032];
extern signed short k4_54_1_data[1032];
extern signed short k4_54_2_data[1032];
extern signed short k4_54_3_data[1032];
extern signed short k4_54_4_data[1032];
extern signed short k4_54_5_data[1032];
extern signed short k4_54_6_data[1032];
extern signed short k4_55_1_data[1032];
extern signed short k4_55_2_data[1032];
extern signed short k4_55_3_data[1032];
extern signed short k4_55_4_data[1032];
extern signed short k4_55_5_data[1032];
extern signed short k4_55_6_data[1032];
extern signed short k4_56_3_data[1032];
extern signed short k4_56_4_data[1032];
extern signed short k4_56_5_data[1032];
extern signed short k4_56_6_data[1032];
extern signed short k4_57_1_data[1032];
extern signed short k4_57_2_data[1032];
extern signed short k4_57_3_data[1032];
extern signed short k4_57_4_data[1032];
extern signed short k4_57_5_data[1032];
extern signed short k4_57_6_data[1032];
extern signed short k4_58_1_data[1032];
extern signed short k4_58_2_data[1032];
extern signed short k4_58_3_data[1032];
extern signed short k4_58_4_data[1032];
extern signed short k4_58_5_data[1032];
extern signed short k4_58_6_data[1032];
extern signed short k4_59_1_data[1032];
extern signed short k4_59_2_data[1032];
extern signed short k4_59_3_data[1032];
extern signed short k4_59_4_data[1032];
extern signed short k4_59_5_data[1032];
extern signed short k4_59_6_data[1032];
extern signed short k4_60_3_data[1032];
extern signed short k4_60_4_data[1032];
extern signed short k4_60_5_data[1032];
extern signed short k4_60_6_data[1032];
extern signed short k4_61_1_data[1032];
extern signed short k4_61_2_data[1032];
extern signed short k4_61_3_data[1032];
extern signed short k4_61_4_data[1032];
extern signed short k4_61_5_data[1032];
extern signed short k4_61_6_data[1032];
extern signed short k4_62_2_data[1032];
extern signed short k4_62_3_data[1032];
extern signed short k4_62_4_data[1032];
extern signed short k4_62_5_data[1032];
extern signed short k4_62_6_data[1032];
extern signed short k4_63_1_data[1032];
extern signed short k4_63_2_data[1032];
extern signed short k4_63_3_data[1032];
extern signed short k4_63_4_data[1032];
extern signed short k4_63_5_data[1032];
extern signed short k4_63_6_data[1032];
extern signed short k4_64_2_data[1032];
extern signed short k4_64_3_data[1032];
extern signed short k4_64_4_data[1032];
extern signed short k4_64_5_data[1032];
extern signed short k4_64_6_data[1032];
extern signed short k4_65_1_data[1032];
extern signed short k4_65_2_data[1032];
extern signed short k4_65_3_data[1032];
extern signed short k4_65_4_data[1032];
extern signed short k4_65_5_data[1032];
extern signed short k4_65_6_data[1032];
extern signed short k4_66_3_data[1032];
extern signed short k4_66_4_data[1032];
extern signed short k4_66_5_data[1032];
extern signed short k4_66_6_data[1032];
extern signed short k4_67_2_data[1032];
extern signed short k4_67_3_data[1032];
extern signed short k4_67_4_data[1032];
extern signed short k4_67_5_data[1032];
extern signed short k4_67_6_data[1032];
extern signed short k4_68_1_data[1032];
extern signed short k4_68_2_data[1032];
extern signed short k4_68_3_data[1032];
extern signed short k4_68_4_data[1032];
extern signed short k4_68_5_data[1032];
extern signed short k4_68_6_data[1032];
extern signed short k4_69_1_data[1032];
extern signed short k4_69_2_data[1032];
extern signed short k4_69_3_data[1032];
extern signed short k4_69_4_data[1032];
extern signed short k4_69_5_data[1032];
extern signed short k4_69_6_data[1032];
extern signed short k4_70_1_data[1032];
extern signed short k4_70_2_data[1032];
extern signed short k4_70_3_data[1032];
extern signed short k4_70_4_data[1032];
extern signed short k4_70_5_data[1032];
extern signed short k4_70_6_data[1032];
extern signed short k4_71_2_data[1032];
extern signed short k4_71_3_data[1032];
extern signed short k4_71_4_data[1032];
extern signed short k4_71_5_data[1032];
extern signed short k4_71_6_data[1032];
extern signed short k4_72_1_data[1032];
extern signed short k4_72_2_data[1032];
extern signed short k4_72_3_data[1032];
extern signed short k4_72_4_data[1032];
extern signed short k4_72_5_data[1032];
extern signed short k4_72_6_data[1032];
extern signed short k4_73_1_data[1032];
extern signed short k4_73_2_data[1032];
extern signed short k4_73_3_data[1032];
extern signed short k4_73_4_data[1032];
extern signed short k4_73_5_data[1032];
extern signed short k4_73_6_data[1032];
extern signed short k4_74_1_data[1032];
extern signed short k4_74_2_data[1032];
extern signed short k4_74_3_data[1032];
extern signed short k4_74_4_data[1032];
extern signed short k4_74_5_data[1032];
extern signed short k4_74_6_data[1032];
extern signed short k4_75_1_data[1032];
extern signed short k4_75_2_data[1032];
extern signed short k4_75_3_data[1032];
extern signed short k4_75_4_data[1032];
extern signed short k4_75_5_data[1032];
extern signed short k4_75_6_data[1032];
extern signed short k4_76_1_data[1032];
extern signed short k4_76_2_data[1032];
extern signed short k4_76_3_data[1032];
extern signed short k4_76_4_data[1032];
extern signed short k4_76_5_data[1032];
extern signed short k4_76_6_data[1032];
extern signed short k4_77_1_data[1032];
extern signed short k4_77_2_data[1032];
extern signed short k4_77_3_data[1032];
extern signed short k4_77_4_data[1032];
extern signed short k4_77_5_data[1032];
extern signed short k4_77_6_data[1032];
extern signed short k4_78_1_data[1032];
extern signed short k4_78_2_data[1032];
extern signed short k4_78_3_data[1032];
extern signed short k4_78_4_data[1032];
extern signed short k4_78_5_data[1032];
extern signed short k4_78_6_data[1032];
extern signed short k4_79_1_data[1032];
extern signed short k4_79_2_data[1032];
extern signed short k4_79_3_data[1032];
extern signed short k4_79_4_data[1032];
extern signed short k4_79_5_data[1032];
extern signed short k4_79_6_data[1032];
extern signed short k4_80_1_data[1032];
extern signed short k4_80_2_data[1032];
extern signed short k4_80_3_data[1032];
extern signed short k4_80_4_data[1032];
extern signed short k4_80_5_data[1032];
extern signed short k4_80_6_data[1032];
extern signed short k4_81_1_data[1032];
extern signed short k4_81_2_data[1032];
extern signed short k4_81_3_data[1032];
extern signed short k4_81_4_data[1032];
extern signed short k4_81_5_data[1032];
extern signed short k4_81_6_data[1032];
extern signed short k4_82_1_data[1032];
extern signed short k4_82_2_data[1032];
extern signed short k4_82_3_data[1032];
extern signed short k4_82_4_data[1032];
extern signed short k4_82_5_data[1032];
extern signed short k4_82_6_data[1032];
extern signed short k4_83_1_data[1032];
extern signed short k4_83_2_data[1032];
extern signed short k4_83_3_data[1032];
extern signed short k4_83_4_data[1032];
extern signed short k4_83_5_data[1032];
extern signed short k4_83_6_data[1032];
extern signed short k4_84_1_data[1032];
extern signed short k4_84_2_data[1032];
extern signed short k4_84_3_data[1032];
extern signed short k4_84_4_data[1032];
extern signed short k4_84_5_data[1032];
extern signed short k4_84_6_data[1032];
extern signed short k4_85_1_data[1032];
extern signed short k4_85_2_data[1032];
extern signed short k4_85_3_data[1032];
extern signed short k4_85_4_data[1032];
extern signed short k4_85_5_data[1032];
extern signed short k4_85_6_data[1032];
extern signed short k4_86_1_data[1032];
extern signed short k4_86_2_data[1032];
extern signed short k4_86_3_data[1032];
extern signed short k4_86_4_data[1032];
extern signed short k4_86_5_data[1032];
extern signed short k4_86_6_data[1032];
extern signed short k4_87_1_data[1032];
extern signed short k4_87_2_data[1032];
extern signed short k4_87_3_data[1032];
extern signed short k4_87_4_data[1032];
extern signed short k4_87_5_data[1032];
extern signed short k4_87_6_data[1032];
extern signed short k4_88_1_data[1032];
extern signed short k4_88_2_data[1032];
extern signed short k4_88_3_data[1032];
extern signed short k4_88_4_data[1032];
extern signed short k4_88_5_data[1032];
extern signed short k4_88_6_data[1032];
extern signed short k4_89_1_data[1032];
extern signed short k4_89_2_data[1032];
extern signed short k4_89_3_data[1032];
extern signed short k4_89_4_data[1032];
extern signed short k4_89_5_data[1032];
extern signed short k4_89_6_data[1032];
extern signed short k4_90_1_data[1032];
extern signed short k4_90_2_data[1032];
extern signed short k4_90_3_data[1032];
extern signed short k4_90_4_data[1032];
extern signed short k4_90_5_data[1032];
extern signed short k4_90_6_data[1032];
extern signed short k4_91_1_data[1032];
extern signed short k4_91_2_data[1032];
extern signed short k4_91_3_data[1032];
extern signed short k4_91_4_data[1032];
extern signed short k4_91_5_data[1032];
extern signed short k4_91_6_data[1032];
extern signed short k4_92_2_data[1032];
extern signed short k4_92_3_data[1032];
extern signed short k4_92_4_data[1032];
extern signed short k4_92_5_data[1032];
extern signed short k4_92_6_data[1032];
extern signed short k4_93_1_data[1032];
extern signed short k4_93_2_data[1032];
extern signed short k4_93_3_data[1032];
extern signed short k4_93_4_data[1032];
extern signed short k4_93_5_data[1032];
extern signed short k4_94_2_data[1032];
extern signed short k4_94_3_data[1032];
extern signed short k4_94_4_data[1032];
extern signed short k4_94_5_data[1032];
extern signed short k4_94_6_data[1032];
extern signed short k4_95_1_data[1032];
extern signed short k4_95_2_data[1032];
extern signed short k4_95_3_data[1032];
extern signed short k4_95_4_data[1032];
extern signed short k4_95_5_data[1032];
extern signed short k4_95_6_data[1032];
extern signed short k4_96_1_data[1032];
extern signed short k4_96_2_data[1032];
extern signed short k4_96_3_data[1032];
extern signed short k4_96_4_data[1032];
extern signed short k4_96_5_data[1032];
extern signed short k4_96_6_data[1032];
extern signed short sq80_formant_1_2_data[1032];
extern signed short sq80_formant_2_3_data[1032];
extern signed short sq80_formant_3_4_data[1032];
extern signed short sq80_formant_4_5_data[1032];
extern signed short sq80_formant_5_6_data[1032];
extern signed short sq80_formant_6_7_data[1032];
extern signed short sq80_formant_8_9_data[1032];
extern signed short sq80_noise1_16_data[1032];
extern signed short sq80_noise2_16_data[1032];
extern signed short sq80_noise3_16_data[1032];
extern signed short sq80_pulsesynth2_6_data[1032];
extern signed short sq80_pulsesynth3_7_data[1032];
extern signed short sq80_pulsesynth4_8_data[1032];
extern signed short sq80_pulsesynth5_9_data[1032];
extern signed short sq80_02_5_data[1032];
extern signed short sq80_03_7_data[1032];
extern signed short sq80_04_8_data[1032];
extern signed short sq80_05_9_data[1032];
extern signed short sq80_06_a_data[1032];
extern signed short sq80_07_b_data[1032];
extern signed short sq80_08_c_data[1032];
extern signed short sq80_09_7_data[1032];
extern signed short sq80_0a_8_data[1032];
extern signed short sq80_0b_9_data[1032];
extern signed short sq80_0c_a_data[1032];
extern signed short sq80_0d_a_data[1032];
extern signed short sq80_0e_b_data[1032];
extern signed short sq80_0e_c_data[1032];
extern signed short sq80_0f_9_data[1032];
extern signed short sq80_10_a_data[1032];
extern signed short sq80_11_b_data[1032];
extern signed short sq80_11_c_data[1032];
extern signed short sq80_12_9_data[1032];
extern signed short sq80_13_a_data[1032];
extern signed short sq80_14_c_data[1032];
extern signed short sq80_16_e_data[1032];
extern signed short sq80_17_e_data[1032];
extern signed short sq80_18_d_data[1032];
extern signed short sq80_19_d_data[1032];
extern signed short sq80_1a_d_data[1032];
extern signed short sq80_1b_9_data[1032];
extern signed short sq80_1c_b_data[1032];
extern signed short sq80_1d_5_data[1032];
extern signed short sq80_1d_6_data[1032];
extern signed short sq80_1e_7_data[1032];
extern signed short sq80_1f_3_data[1032];
extern signed short sq80_1f_4_data[1032];
extern signed short sq80_1f_5_data[1032];
extern signed short sq80_1f_6_data[1032];
extern signed short sq80_1f_7_data[1032];
extern signed short sq80_1f_8_data[1032];
extern signed short sq80_1f_9_data[1032];
extern signed short sq80_1f_a_data[1032];
extern signed short sq80_1f_b_data[1032];
extern signed short sq80_1f_c_data[1032];
extern signed short sq80_1f_d_data[1032];
extern signed short sq80_21_4_data[1032];
extern signed short sq80_22_5_data[1032];
extern signed short sq80_23_6_data[1032];
extern signed short sq80_24_b_data[1032];
extern signed short sq80_24_c_data[1032];
extern signed short sq80_25_5_data[1032];
extern signed short sq80_26_7_data[1032];
extern signed short sq80_27_7_data[1032];
extern signed short sq80_28_8_data[1032];
extern signed short sq80_29_9_data[1032];
extern signed short sq80_2a_b_data[1032];
extern signed short sq80_2a_c_data[1032];
extern signed short sq80_2b_5_data[1032];
extern signed short sq80_2c_6_data[1032];
extern signed short sq80_2d_7_data[1032];
extern signed short sq80_2e_8_data[1032];
extern signed short sq80_2f_9_data[1032];
extern signed short sq80_30_a_data[1032];
extern signed short sq80_31_a_data[1032];
extern signed short sq80_31_b_data[1032];
extern signed short sq80_32_a_data[1032];
extern signed short sq80_32_b_data[1032];
extern signed short sq80_32_c_data[1032];
extern signed short sq80_33_b_data[1032];
extern signed short sq80_33_c_data[1032];
extern signed short sq80_34_4_data[1032];
extern signed short sq80_35_5_data[1032];
extern signed short sq80_36_6_data[1032];
extern signed short sq80_36_7_data[1032];
extern signed short sq80_37_9_data[1032];
extern signed short sq80_38_b_data[1032];
extern signed short sq80_39_c_data[1032];
extern signed short sq80_3a_6_data[1032];
extern signed short sq80_3b_7_data[1032];
extern signed short sq80_3b_8_data[1032];
extern signed short sq80_3c_8_data[1032];
extern signed short sq80_3c_9_data[1032];
extern signed short sq80_3d_9_data[1032];
extern signed short sq80_3d_a_data[1032];
extern signed short sq80_3e_a_data[1032];
extern signed short sq80_3e_b_data[1032];
extern signed short sq80_3f_8_data[1032];
extern signed short sq80_40_9_data[1032];
extern signed short sq80_40_a_data[1032];
extern signed short sq80_41_b_data[1032];
extern signed short sq80_41_c_data[1032];
extern signed short sq80_42_7_data[1032];
extern signed short sq80_43_8_data[1032];
extern signed short sq80_44_9_data[1032];
extern signed short sq80_45_a_data[1032];
extern signed short sq80_45_b_data[1032];
extern signed short sq80_46_4_data[1032];
extern signed short sq80_47_5_data[1032];
extern signed short sq80_48_6_data[1032];
extern signed short sq80_49_9_data[1032];
extern signed short sq80_4a_b_data[1032];
extern signed short sq80_4b_c_data[1032];
extern signed short sq80_4c_8_data[1032];
extern signed short sq80_4d_9_data[1032];
extern signed short sq80_4e_a_data[1032];
extern signed short sq80_4f_b_data[1032];
extern signed short sq80_50_c_data[1032];
extern signed short sq80_51_d_data[1032];
extern signed short sq80_68_6_data[1032];
extern signed short sq80_68_7_data[1032];
extern signed short sq80_68_8_data[1032];
extern signed short sq80_69_9_data[1032];
extern signed short sq80_69_b_data[1032];
extern signed short sq80_6d_4_data[1032];
extern signed short sq80_6d_5_data[1032];
extern signed short sq80_6e_5_data[1032];
extern signed short sq80_6e_6_data[1032];
extern signed short sq80_6f_6_data[1032];
extern signed short sq80_6f_7_data[1032];
extern signed short sq80_70_7_data[1032];
extern signed short sq80_71_9_data[1032];
extern signed short sq80_72_9_data[1032];
extern signed short sq80_72_a_data[1032];
extern signed short sq80_73_b_data[1032];
extern signed short sq80_74_c_data[1032];
extern signed short sq80_76_7_data[1032];
extern signed short sq80_76x_7_data[1032];
extern signed short sq80_76x_8_data[1032];
extern signed short sq80_76x_9_data[1032];
extern signed short sq80_76x_a_data[1032];
extern signed short sq80_77_b_data[1032];
extern signed short sq80_77_c_data[1032];
extern signed short sq80_78_7_data[1032];
extern signed short sq80_78_9_data[1032];
extern signed short sq80_7b_9_data[1032];
extern signed short sq80_7b_a_data[1032];
extern signed short sq80_7c_b_data[1032];
extern signed short sq80_7c_c_data[1032];
extern signed short sq80_7d_9_data[1032];
extern signed short sq80_7e_a_data[1032];
extern signed short sq80_7f_b_data[1032];
extern signed short sq80_7f_c_data[1032];
extern signed short sq80_80_6_data[1032];
extern signed short sq80_80_7_data[1032];
extern signed short sq80_80_8_data[1032];
extern signed short sq80_81_5_data[1032];
extern signed short sq80_81_6_data[1032];
extern signed short sq80_82_7_data[1032];
extern signed short sq80_82_8_data[1032];
extern signed short sq80_83_9_data[1032];
extern signed short sq80_84_a_data[1032];
extern signed short sq80_84_b_data[1032];
extern signed short sq80_84_c_data[1032];
extern signed short sq80_85_7_data[1032];
extern signed short sq80_85_8_data[1032];
extern signed short sq80_86_9_data[1032];
extern signed short sq80_87_8_data[1032];
extern signed short sq80_88_a_data[1032];
extern signed short sq80_88_b_data[1032];
extern signed short sq80_88_c_data[1032];
extern signed short sq80_89_5_data[1032];
extern signed short sq80_89_6_data[1032];
extern signed short sq80_8a_7_data[1032];
extern signed short sq80_8b_8_data[1032];
extern signed short sq80_8c_a_data[1032];
extern signed short sq80_8c_b_data[1032];
extern signed short tx81z_2_8_data[1032];
extern signed short tx81z_2_9_data[1032];
extern signed short tx81z_2_10_data[1032];
extern signed short tx81z_2_11_data[1032];
extern signed short tx81z_2_12_data[1032];
extern signed short tx81z_3_8_data[1032];
extern signed short tx81z_3_9_data[1032];
extern signed short tx81z_3_10_data[1032];
extern signed short tx81z_3_11_data[1032];
extern signed short tx81z_3_12_data[1032];
extern signed short tx81z_4_8_data[1032];
extern signed short tx81z_4_9_data[1032];
extern signed short tx81z_4_10_data[1032];
extern signed short tx81z_4_11_data[1032];
extern signed short tx81z_4_12_data[1032];
extern signed short tx81z_5_8_data[1032];
extern signed short tx81z_5_9_data[1032];
extern signed short tx81z_5_10_data[1032];
extern signed short tx81z_5_11_data[1032];
extern signed short tx81z_5_12_data[1032];
extern signed short tx81z_6_7_data[1032];
extern signed short tx81z_6_8_data[1032];
extern signed short tx81z_6_9_data[1032];
extern signed short tx81z_6_10_data[1032];
extern signed short tx81z_6_11_data[1032];
extern signed short tx81z_6_12_data[1032];
extern signed short tx81z_7_8_data[1032];
extern signed short tx81z_7_9_data[1032];
extern signed short tx81z_7_10_data[1032];
extern signed short tx81z_7_11_data[1032];
extern signed short tx81z_7_12_data[1032];
extern signed short tx81z_8_7_data[1032];
extern signed short tx81z_8_8_data[1032];
extern signed short tx81z_8_9_data[1032];
extern signed short tx81z_8_10_data[1032];
extern signed short tx81z_8_11_data[1032];
extern signed short tx81z_8_12_data[1032];
extern signed short bristol_jagged_8_data[1032];
extern signed short bristol_jagged_9_data[1032];
extern signed short bristol_jagged_10_data[1032];
extern signed short bristol_jagged_11_data[1032];
extern signed short bristol_jagged_12_data[1032];
#endif /* Y_PLUGIN */

int wavetables_count;

struct wavetable wavetable[] =
{
/* ==== Simple sinus-based waveforms ==== */

    { "Sines|Sine 1",  /* K4 wave 0 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_sin_1_data + 4 },
      }
#endif
    },
    { "Sines|Sine 2",  /* K4 wave 1 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 120, 1, yw_sin_2_data + 4 },
        { 256, 1, yw_sin_1_data + 4 },
      }
#endif
    },
    { "Sines|Sine 3",  /* K4 wave 2 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 117, 1, yw_sin_3_data + 4 },
        { 256, 1, yw_sin_1_data + 4 },
      }
#endif
    },
    { "Sines|Sine 4",  /* K4 wave 3 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 112, 1, yw_sin_4_data + 4 },
        { 120, 1, yw_sin_2_data + 4 },
        { 256, 1, yw_sin_1_data + 4 },
      }
#endif
    },
    { "Sines|Sine 5",  /* K4 wave 4 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 108, 1, yw_sin_5_data + 4 },
        { 117, 1, yw_sin_3_data + 4 },
        { 256, 1, yw_sin_1_data + 4 },
      }
#endif
    },
    { "Sines|Sine 6",  /* K4 wave 5 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 105, 1, yw_sin_6_data + 4 },
        { 120, 1, yw_sin_2_data + 4 },
        { 256, 1, yw_sin_1_data + 4 },
      }
#endif
    },
    { "Sines|Sine 7",  /* K4 wave 6 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 103, 1, yw_sin_7_data + 4 },
        { 117, 1, yw_sin_3_data + 4 },
        { 256, 1, yw_sin_1_data + 4 },
      }
#endif
    },
    { "Sines|Sine 8",  /* K4 wave 7 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 100, 1, yw_sin_8_data + 4 },
        { 112, 1, yw_sin_4_data + 4 },
        { 120, 1, yw_sin_2_data + 4 },
        { 256, 1, yw_sin_1_data + 4 },
      }
#endif
    },
    { "Sines|Sine 9",  /* K4 wave 8 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  98, 1, yw_sin_9_data + 4 },
        { 108, 1, yw_sin_5_data + 4 },
        { 117, 1, yw_sin_3_data + 4 },
        { 256, 1, yw_sin_1_data + 4 },
      }
#endif
    },
    { "Sines|Sine 1+2",  /* SQ-80 waveform 30 'octave' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        { 119, 1, sq80_17_e_data + 4 },  /* 'octave'  0-14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Sines|Sine 1+2+3",  /* SQ-80 waveform 31 'oct+5' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        { 111, 1, sq80_1a_d_data + 4 },  /* 'oct+5'  0-13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'oct+5' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Sines|Sine 1+3",  /* SQ-80 waveform 33 'triang' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        { 111, 1, sq80_19_d_data + 4 },  /* 'triang'  0-13 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },

/* ==== 'Analog' waveforms ==== */

    { "Analog|Triangle",  /* K4 wave 18 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  66, 1, k4_19_3_data + 4 }, /* Triangle */
        {  78, 1, k4_19_4_data + 4 }, /* Triangle */
        {  90, 1, k4_19_5_data + 4 }, /* Triangle */
        { 127, 1, k4_19_6_data + 4 }, /* Triangle */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Saw 0",  /* SQ-80 waveform 32 'saw__2' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 111, 1, sq80_18_d_data + 4 },  /* 'saw__2'  0-13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'saw__2' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Saw 1",  /* K4 wave 9 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  78, 1, k4_10_4_data + 4 }, /* Saw 1 */
        {  90, 1, k4_10_5_data + 4 }, /* Saw 1 */
        { 127, 1, k4_10_6_data + 4 }, /* Saw 1 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Saw 2",  /* K4 wave 10 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  66, 1, k4_11_3_data + 4 }, /* Saw 2 */
        {  78, 1, k4_11_4_data + 4 }, /* Saw 2 */
        {  90, 1, k4_11_5_data + 4 }, /* Saw 2 */
        { 127, 1, k4_11_6_data + 4 }, /* Saw 2 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Saw 3",  /* K4 wave 11 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_12_1_data + 4 }, /* Saw 3 */
        {  54, 1, k4_12_2_data + 4 }, /* Saw 3 */
        {  66, 1, k4_12_3_data + 4 }, /* Saw 3 */
        {  78, 1, k4_12_4_data + 4 }, /* Saw 3 */
        {  90, 1, k4_12_5_data + 4 }, /* Saw 3 */
        { 127, 1, k4_12_6_data + 4 }, /* Saw 3 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Saw 4",  /* K4 wave 12 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_13_1_data + 4 }, /* Saw 4 */
        {  54, 1, k4_13_2_data + 4 }, /* Saw 4 */
        {  66, 1, k4_13_3_data + 4 }, /* Saw 4 */
        {  78, 1, k4_13_4_data + 4 }, /* Saw 4 */
        {  90, 1, k4_13_5_data + 4 }, /* Saw 4 */
        { 127, 1, k4_13_6_data + 4 }, /* Saw 4 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Saw 5",  /* K4 wave 13 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_14_1_data + 4 }, /* Saw 5 */
        {  54, 1, k4_14_2_data + 4 }, /* Saw 5 */
        {  66, 1, k4_14_3_data + 4 }, /* Saw 5 */
        {  78, 1, k4_14_4_data + 4 }, /* Saw 5 */
        {  90, 1, k4_14_5_data + 4 }, /* Saw 5 */
        { 127, 1, k4_14_6_data + 4 }, /* Saw 5 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Saw 6",  /* K4 wave 14 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_15_1_data + 4 }, /* Saw 6 */
        {  54, 1, k4_15_2_data + 4 }, /* Saw 6 */
        {  66, 1, k4_15_3_data + 4 }, /* Saw 6 */
        {  78, 1, k4_15_4_data + 4 }, /* Saw 6 */
        {  90, 1, k4_15_5_data + 4 }, /* Saw 6 */
        { 127, 1, k4_15_6_data + 4 }, /* Saw 6 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Saw 6.5",  /* SQ-80 waveform 0 'saw' */
    /* -FIX- Now that this has the proper key ranges, it's really very close
     * to 'Saw 6'.  Replace 'Saw 6' with this when next reorganizing the
     * wavetables. */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  39, 1, sq80_34_4_data + 4 },  /* 'saw'  0-4 */
        {  47, 1, sq80_35_5_data + 4 },  /* 'saw'  5 */
        {  55, 1, sq80_36_6_data + 4 },  /* 'saw'  6 */
        {  63, 1, sq80_36_7_data + 4 },  /* 'saw'  7 */
        {  79, 1, sq80_37_9_data + 4 },  /* 'saw'  8-9 */
        {  95, 1, sq80_38_b_data + 4 },  /* 'saw' 10-11 */
        { 103, 1, sq80_39_c_data + 4 },  /* 'saw' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'saw' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'saw' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Saw 7",  /* K4 wave 15 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_16_1_data + 4 }, /* Saw 7 */
        {  54, 1, k4_16_2_data + 4 }, /* Saw 7 */
        {  66, 1, k4_16_3_data + 4 }, /* Saw 7 */
        {  78, 1, k4_16_4_data + 4 }, /* Saw 7 */
        {  90, 1, k4_16_5_data + 4 }, /* Saw 7 */
        { 127, 1, k4_16_6_data + 4 }, /* Saw 7 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Saw 8",  /* K4 wave 16 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_17_1_data + 4 }, /* Saw 8 */
        {  54, 1, k4_17_2_data + 4 }, /* Saw 8 */
        {  66, 1, k4_17_3_data + 4 }, /* Saw 8 */
        {  78, 1, k4_17_4_data + 4 }, /* Saw 8 */
        {  90, 1, k4_17_5_data + 4 }, /* Saw 8 */
        { 127, 1, k4_17_6_data + 4 }, /* Saw 8 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Pulse",  /* K4 wave 17 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_18_1_data + 4 }, /* Pulse */
        {  54, 1, k4_18_2_data + 4 }, /* Pulse */
        {  66, 1, k4_18_3_data + 4 }, /* Pulse */
        {  78, 1, k4_18_4_data + 4 }, /* Pulse */
        {  90, 1, k4_18_5_data + 4 }, /* Pulse */
        { 127, 1, k4_18_6_data + 4 }, /* Pulse */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Square",  /* K4 wave 19 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_20_1_data + 4 }, /* Square */
        {  54, 1, k4_20_2_data + 4 }, /* Square */
        {  66, 1, k4_20_3_data + 4 }, /* Square */
        {  78, 1, k4_20_4_data + 4 }, /* Square */
        {  90, 1, k4_20_5_data + 4 }, /* Square */
        { 127, 1, k4_20_6_data + 4 }, /* Square */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Square 2",  /* SQ-80 waveform 3 'square' */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  39, 1, sq80_46_4_data + 4 },  /* 'square'  0-4 */
        {  47, 1, sq80_47_5_data + 4 },  /* 'square'  5 */
        {  55, 1, sq80_48_6_data + 4 },  /* 'square'  6-7 */
        {  79, 1, sq80_49_9_data + 4 },  /* 'square'  8-9 */
        {  95, 1, sq80_4a_b_data + 4 },  /* 'square' 10-11 */
        { 103, 1, sq80_4b_c_data + 4 },  /* 'square' 12 */
        { 111, 1, sq80_19_d_data + 4 },  /* 'square' 13 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Rectangular 1",  /* K4 wave 20 */
#ifdef Y_GUI
      3
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_21_1_data + 4 }, /* Rectangular 1 */
        {  54, 1, k4_21_2_data + 4 }, /* Rectangular 1 */
        {  66, 1, k4_21_3_data + 4 }, /* Rectangular 1 */
        {  78, 1, k4_21_4_data + 4 }, /* Rectangular 1 */
        {  90, 1, k4_21_5_data + 4 }, /* Rectangular 1 */
        { 127, 1, k4_21_6_data + 4 }, /* Rectangular 1 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Rectangular 2",  /* K4 wave 21 */
#ifdef Y_GUI
      3
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_22_1_data + 4 }, /* Rectangular 2 */
        {  54, 1, k4_22_2_data + 4 }, /* Rectangular 2 */
        {  66, 1, k4_22_3_data + 4 }, /* Rectangular 2 */
        {  78, 1, k4_22_4_data + 4 }, /* Rectangular 2 */
        {  90, 1, k4_22_5_data + 4 }, /* Rectangular 2 */
        { 127, 1, k4_22_6_data + 4 }, /* Rectangular 2 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Rectangular 3",  /* K4 wave 22 */
#ifdef Y_GUI
      3
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_23_1_data + 4 }, /* Rectangular 3 */
        {  54, 1, k4_23_2_data + 4 }, /* Rectangular 3 */
        {  66, 1, k4_23_3_data + 4 }, /* Rectangular 3 */
        {  78, 1, k4_23_4_data + 4 }, /* Rectangular 3 */
        {  90, 1, k4_23_5_data + 4 }, /* Rectangular 3 */
        { 127, 1, k4_23_6_data + 4 }, /* Rectangular 3 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Rectangular 4",  /* K4 wave 23 */
#ifdef Y_GUI
      3
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_24_1_data + 4 }, /* Rectangular 4 */
        {  54, 1, k4_24_2_data + 4 }, /* Rectangular 4 */
        {  66, 1, k4_24_3_data + 4 }, /* Rectangular 4 */
        {  78, 1, k4_24_4_data + 4 }, /* Rectangular 4 */
        {  90, 1, k4_24_5_data + 4 }, /* Rectangular 4 */
        { 127, 1, k4_24_6_data + 4 }, /* Rectangular 4 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Rectangular 5",  /* K4 wave 24 */
#ifdef Y_GUI
      3
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_25_1_data + 4 }, /* Rectangular 5 */
        {  54, 1, k4_25_2_data + 4 }, /* Rectangular 5 */
        {  66, 1, k4_25_3_data + 4 }, /* Rectangular 5 */
        {  78, 1, k4_25_4_data + 4 }, /* Rectangular 5 */
        {  90, 1, k4_25_5_data + 4 }, /* Rectangular 5 */
        { 127, 1, k4_25_6_data + 4 }, /* Rectangular 5 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Rectangular 6",  /* K4 wave 25 */
#ifdef Y_GUI
      3
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_26_1_data + 4 }, /* Rectangular 6 */
        {  54, 1, k4_26_2_data + 4 }, /* Rectangular 6 */
        {  66, 1, k4_26_3_data + 4 }, /* Rectangular 6 */
        {  78, 1, k4_26_4_data + 4 }, /* Rectangular 6 */
        {  90, 1, k4_26_5_data + 4 }, /* Rectangular 6 */
        { 127, 1, k4_26_6_data + 4 }, /* Rectangular 6 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },

/* ==== Digital/new synth waveforms ==== */

    { "Digital|Synth",  /* K4 wave 95 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_96_1_data + 4 }, /* Synth */
        {  54, 1, k4_96_2_data + 4 }, /* Synth */
        {  66, 1, k4_96_3_data + 4 }, /* Synth */
        {  78, 1, k4_96_4_data + 4 }, /* Synth */
        {  90, 1, k4_96_5_data + 4 }, /* Synth */
        { 127, 1, k4_96_6_data + 4 }, /* Synth */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Digital|Synth 1",  /* SQ-80 waveform 16 'synth1' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  79, 1, sq80_0f_9_data + 4 },  /* 'synth1'  0-9 */
        {  87, 1, sq80_10_a_data + 4 },  /* 'synth1' 10 */
        {  95, 1, sq80_11_b_data + 4 },  /* 'synth1' 11 */
        { 103, 1, sq80_11_c_data + 4 },  /* 'synth1' 12 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'synth1' 13-14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Digital|Synth 2",  /* SQ-80 waveform 17 'synth2' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  79, 1, sq80_12_9_data + 4 },  /* 'synth2'  0-9 */
        {  87, 1, sq80_13_a_data + 4 },  /* 'synth2' 10 */
        { 103, 1, sq80_14_c_data + 4 },  /* 'synth2' 11-12 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Digital|Synth 3",  /* SQ-80 waveform 18 'synth3' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  79, 1, sq80_1b_9_data + 4 },  /* 'synth3'  0-9 */
        {  95, 1, sq80_1c_b_data + 4 },  /* 'synth3' 10-11 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'synth3' 12-13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'synth3' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Digital|Primes",  /* SQ-80 waveform 27 'prime' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  95, 1, sq80_1c_b_data + 4 },  /* 'prime'  0-11 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'prime' 12-13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'prime' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Digital|Alien",  /* SQ-80 waveform 48 'alien' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  47, 1, sq80_81_5_data + 4 },  /* 'alien'  0-5 */
        {  55, 1, sq80_81_6_data + 4 },  /* 'alien'  6 */
        {  63, 1, sq80_82_7_data + 4 },  /* 'alien'  7 */
        {  71, 1, sq80_82_8_data + 4 },  /* 'alien'  8 */
        {  79, 1, sq80_83_9_data + 4 },  /* 'alien'  9 */
        {  87, 1, sq80_84_a_data + 4 },  /* 'alien' 10 */
        {  95, 1, sq80_84_b_data + 4 },  /* 'alien' 11 */
        { 103, 1, sq80_84_c_data + 4 },  /* 'alien' 12 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'alien' 13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'alien' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Digital|Digital 1",  /* SQ-80 waveform 45 'digit1' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  63, 1, sq80_85_7_data + 4 },  /* 'digit1'  0-7 */
        {  71, 1, sq80_85_8_data + 4 },  /* 'digit1'  8 */
        {  86, 1, sq80_86_9_data + 4 },  /* 'digit1'  9+ */
        {  95, 1, sq80_07_b_data + 4 },  /* 'digit1' 11 */
        { 103, 1, sq80_08_c_data + 4 },  /* 'digit1' 12 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'digit1' 13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'digit1' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Digital|PulseSynth",
    /* The first four samples here duplicate //christian's use of the wrong
     * start and length for his 'pulse' extractions. */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  55, 1, sq80_pulsesynth2_6_data + 4 },  /* 'PulseSynth' 0-6 */
        {  63, 1, sq80_pulsesynth3_7_data + 4 },  /* 'PulseSynth' 7 */
        {  71, 1, sq80_pulsesynth4_8_data + 4 },  /* 'PulseSynth' 8 */
        {  79, 1, sq80_pulsesynth5_9_data + 4 },  /* 'PulseSynth' 9 */
        {  95, 1, sq80_07_b_data + 4 },  /* 'pulse' 11 */
        { 103, 1, sq80_08_c_data + 4 },  /* 'pulse' 12 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'pulse' 13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'pulse' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },

/* ==== Brass ==== */

    { "Brass|Pure Horn L",  /* K4 wave 26 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 127, 1, k4_27_3_data + 4 }, /* Pure Horn L */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Brass|Pure Horn H",  /* K4 wave 71 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_72_1_data + 4 }, /* Pure Horn H */
        {  54, 1, k4_72_2_data + 4 }, /* Pure Horn H */
        {  66, 1, k4_72_3_data + 4 }, /* Pure Horn H */
        {  78, 1, k4_72_4_data + 4 }, /* Pure Horn H */
        {  90, 1, k4_72_5_data + 4 }, /* Pure Horn H */
        { 127, 1, k4_72_6_data + 4 }, /* Pure Horn H */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Brass|Punch Brass",  /* K4 wave 27 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  66, 1, k4_28_3_data + 4 }, /* Punch Brass 1 */
        {  78, 1, k4_28_4_data + 4 }, /* Punch Brass 1 */
        {  90, 1, k4_28_5_data + 4 }, /* Punch Brass 1 */
        { 127, 1, k4_28_6_data + 4 }, /* Punch Brass 1 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Brass|Punch Brass 2",  /* K4 wave 73 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_74_1_data + 4 }, /* Punch Brass 2 */
        {  54, 1, k4_74_2_data + 4 }, /* Punch Brass 2 */
        {  66, 1, k4_74_3_data + 4 }, /* Punch Brass 2 */
        {  78, 1, k4_74_4_data + 4 }, /* Punch Brass 2 */
        {  90, 1, k4_74_5_data + 4 }, /* Punch Brass 2 */
        { 127, 1, k4_74_6_data + 4 }, /* Punch Brass 2 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Brass|Fat Brass",  /* K4 wave 72 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_73_1_data + 4 }, /* Fat Brass */
        {  54, 1, k4_73_2_data + 4 }, /* Fat Brass */
        {  66, 1, k4_73_3_data + 4 }, /* Fat Brass */
        {  78, 1, k4_73_4_data + 4 }, /* Fat Brass */
        {  90, 1, k4_73_5_data + 4 }, /* Fat Brass */
        { 127, 1, k4_73_6_data + 4 }, /* Fat Brass */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Brass|Soft Brass",  /* SQ-80 waveform 43 'brass' excerpt */
    /* This is the SQ-80 waveform 43 'brass' with the lowest sample omitted,
     * which matches the 'Soft Brass' patch of the original 20051005 WhySynth
     * release. */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  79, 1, sq80_7b_9_data + 4 },  /* 'brass'  0-9 */
        {  87, 1, sq80_7b_a_data + 4 },  /* 'brass' 10 */
        {  95, 1, sq80_7c_b_data + 4 },  /* 'brass' 11 */
        { 103, 1, sq80_7c_c_data + 4 },  /* 'brass' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'brass' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'brass' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },

/* ==== Woodwind ==== */

    { "Woodwind|Oboe 1",  /* K4 wave 28 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_29_1_data + 4 }, /* Oboe 1 */
        {  54, 1, k4_29_2_data + 4 }, /* Oboe 1 */
        {  66, 1, k4_29_3_data + 4 }, /* Oboe 1 */
        {  78, 1, k4_29_4_data + 4 }, /* Oboe 1 */
        {  90, 1, k4_29_5_data + 4 }, /* Oboe 1 */
        { 127, 1, k4_29_6_data + 4 }, /* Oboe 1 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Woodwind|Oboe 2",  /* K4 wave 29 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_30_1_data + 4 }, /* Oboe 2 */
        {  54, 1, k4_30_2_data + 4 }, /* Oboe 2 */
        {  66, 1, k4_30_3_data + 4 }, /* Oboe 2 */
        {  78, 1, k4_30_4_data + 4 }, /* Oboe 2 */
        {  90, 1, k4_30_5_data + 4 }, /* Oboe 2 */
        { 127, 1, k4_30_6_data + 4 }, /* Oboe 2 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Woodwind|Oboe 3",  /* K4 wave 70 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, k4_71_2_data + 4 }, /* Oboe 3 */
        {  66, 1, k4_71_3_data + 4 }, /* Oboe 3 */
        {  78, 1, k4_71_4_data + 4 }, /* Oboe 3 */
        {  90, 1, k4_71_5_data + 4 }, /* Oboe 3 */
        { 127, 1, k4_71_6_data + 4 }, /* Oboe 3 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Woodwind|Harmonica",  /* K4 wave 94 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_95_1_data + 4 }, /* Harmonica */
        {  54, 1, k4_95_2_data + 4 }, /* Harmonica */
        {  66, 1, k4_95_3_data + 4 }, /* Harmonica */
        {  78, 1, k4_95_4_data + 4 }, /* Harmonica */
        {  90, 1, k4_95_5_data + 4 }, /* Harmonica */
        { 127, 1, k4_95_6_data + 4 }, /* Harmonica */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Woodwind|Reed",  /* SQ-80 waveform 14 'reed' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  47, 1, sq80_1d_5_data + 4 },  /* 'reed'  0-5 */
        {  55, 1, sq80_1d_6_data + 4 },  /* 'reed'  6 */
        {  63, 1, sq80_1e_7_data + 4 },  /* 'reed'  7 */
        {  71, 1, sq80_4c_8_data + 4 },  /* 'reed'  8 */
        {  79, 1, sq80_4d_9_data + 4 },  /* 'reed'  9 */
        {  87, 1, sq80_4e_a_data + 4 },  /* 'reed' 10 */
        {  95, 1, sq80_4f_b_data + 4 },  /* 'reed' 11 */
        { 103, 1, sq80_50_c_data + 4 },  /* 'reed' 12 */
        { 111, 1, sq80_51_d_data + 4 },  /* 'reed' 13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'reed' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Woodwind|Reed 2",  /* SQ-80 waveform 34 'reed_2' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  79, 1, sq80_30_a_data + 4 },  /* 'reed_2'  0-9 */
        {  87, 1, sq80_31_a_data + 4 },  /* 'reed_2' 10 */
        {  95, 1, sq80_32_b_data + 4 },  /* 'reed_2' 11 */
        { 103, 1, sq80_33_c_data + 4 },  /* 'reed_2' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'reed_2' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'reed_2' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Woodwind|Reed 3",  /* SQ-80 waveform 35 'reed_3' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  87, 1, sq80_8c_a_data + 4 },  /* 'reed_3'  0-10 */
        {  95, 1, sq80_8c_b_data + 4 },  /* 'reed_3' 11 */
        { 103, 1, sq80_08_c_data + 4 },  /* 'reed_3' 12 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'reed_3' 13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'reed_3' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Woodwind|Quena",  /* K4 wave 69 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_70_1_data + 4 }, /* Quena */
        {  54, 1, k4_70_2_data + 4 }, /* Quena */
        {  66, 1, k4_70_3_data + 4 }, /* Quena */
        {  78, 1, k4_70_4_data + 4 }, /* Quena */
        {  90, 1, k4_70_5_data + 4 }, /* Quena */
        { 127, 1, k4_70_6_data + 4 }, /* Quena */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },

/* ==== Piano/Keyboard ==== */

    { "Keyboard|Grand Piano",  /* K4 wave 30 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_31_1_data + 4 }, /* Classic Grand */
        {  54, 1, k4_31_2_data + 4 }, /* Classic Grand */
        {  66, 1, k4_31_3_data + 4 }, /* Classic Grand */
        {  78, 1, k4_31_4_data + 4 }, /* Classic Grand */
        {  90, 1, k4_31_5_data + 4 }, /* Classic Grand */
        { 127, 1, k4_31_6_data + 4 }, /* Classic Grand */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Piano Hi",  /* SQ-80 waveform 9 'piano' excerpt */
    /* This is the SQ-80 'piano' wavetable without the lowest three
     * waves, which were four cycles long, and were left out of the
     * first WhySynth release.  See 'Piano Full' for more information. */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  63, 1, sq80_42_7_data + 4 },  /* 'piano'  0-7 */
        {  71, 1, sq80_43_8_data + 4 },  /* 'piano'  8 */
        {  79, 1, sq80_44_9_data + 4 },  /* 'piano'  9 */
        {  87, 1, sq80_45_a_data + 4 },  /* 'piano' 10 */
        {  95, 1, sq80_45_b_data + 4 },  /* 'piano' 11 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'piano' 12-13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'piano' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|E.Piano 1",  /* K4 wave 31 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_32_1_data + 4 }, /* EP 1 */
        {  54, 1, k4_32_2_data + 4 }, /* EP 1 */
        {  66, 1, k4_32_3_data + 4 }, /* EP 1 */
        {  78, 1, k4_32_4_data + 4 }, /* EP 1 */
        {  90, 1, k4_32_5_data + 4 }, /* EP 1 */
        { 127, 1, k4_32_6_data + 4 }, /* EP 1 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|E.Piano 2",  /* K4 wave 32 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_33_1_data + 4 }, /* EP 2 */
        {  54, 1, k4_33_2_data + 4 }, /* EP 2 */
        {  66, 1, k4_33_3_data + 4 }, /* EP 2 */
        {  78, 1, k4_33_4_data + 4 }, /* EP 2 */
        {  90, 1, k4_33_5_data + 4 }, /* EP 2 */
        { 127, 1, k4_33_6_data + 4 }, /* EP 2 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|E.Piano 3",  /* K4 wave 33 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_34_1_data + 4 }, /* EP 3 */
        {  54, 1, k4_34_2_data + 4 }, /* EP 3 */
        {  66, 1, k4_34_3_data + 4 }, /* EP 3 */
        {  78, 1, k4_34_4_data + 4 }, /* EP 3 */
        {  90, 1, k4_34_5_data + 4 }, /* EP 3 */
        { 127, 1, k4_34_6_data + 4 }, /* EP 3 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|E.Piano 4",  /* K4 wave 56 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_57_1_data + 4 }, /* EP 4 */
        {  54, 1, k4_57_2_data + 4 }, /* EP 4 */
        {  66, 1, k4_57_3_data + 4 }, /* EP 4 */
        {  78, 1, k4_57_4_data + 4 }, /* EP 4 */
        {  90, 1, k4_57_5_data + 4 }, /* EP 4 */
        { 127, 1, k4_57_6_data + 4 }, /* EP 4 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|E.Piano 5",  /* K4 wave 58 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_59_1_data + 4 }, /* EP 5 */
        {  54, 1, k4_59_2_data + 4 }, /* EP 5 */
        {  66, 1, k4_59_3_data + 4 }, /* EP 5 */
        {  78, 1, k4_59_4_data + 4 }, /* EP 5 */
        {  90, 1, k4_59_5_data + 4 }, /* EP 5 */
        { 127, 1, k4_59_6_data + 4 }, /* EP 5 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|E.Piano 6",  /* K4 wave 65 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  66, 1, k4_66_3_data + 4 }, /* EP 6 */
        {  78, 1, k4_66_4_data + 4 }, /* EP 6 */
        {  90, 1, k4_66_5_data + 4 }, /* EP 6 */
        { 127, 1, k4_66_6_data + 4 }, /* EP 6 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|E.Piano 7",  /* K4 wave 74 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_75_1_data + 4 }, /* EP 7 */
        {  54, 1, k4_75_2_data + 4 }, /* EP 7 */
        {  66, 1, k4_75_3_data + 4 }, /* EP 7 */
        {  78, 1, k4_75_4_data + 4 }, /* EP 7 */
        {  90, 1, k4_75_5_data + 4 }, /* EP 7 */
        { 127, 1, k4_75_6_data + 4 }, /* EP 7 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|E.Piano 8",  /* K4 wave 75 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_76_1_data + 4 }, /* EP 8 */
        {  54, 1, k4_76_2_data + 4 }, /* EP 8 */
        {  66, 1, k4_76_3_data + 4 }, /* EP 8 */
        {  78, 1, k4_76_4_data + 4 }, /* EP 8 */
        {  90, 1, k4_76_5_data + 4 }, /* EP 8 */
        { 127, 1, k4_76_6_data + 4 }, /* EP 8 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|E.Piano 9",  /* SQ-80 waveform 10 'el_pno' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  71, 1, sq80_3f_8_data + 4 },  /* 'el_pno'  0-8 */
        {  79, 1, sq80_40_9_data + 4 },  /* 'el_pno'  9 */
        {  87, 1, sq80_40_a_data + 4 },  /* 'el_pno' 10 */
        {  95, 1, sq80_41_b_data + 4 },  /* 'el_pno' 11 */
        { 103, 1, sq80_41_c_data + 4 },  /* 'el_pno' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'el_pno' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'el_pno' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Clav",  /* SQ-80 waveform 42 'clav' */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  47, 1, sq80_89_5_data + 4 },  /* 'clav'  0-5 */
        {  55, 1, sq80_89_6_data + 4 },  /* 'clav'  6 */
        {  63, 1, sq80_8a_7_data + 4 },  /* 'clav'  7 */
        {  77, 1, sq80_8b_8_data + 4 },  /* 'clav'  8+ */
        {  87, 1, sq80_8c_a_data + 4 },  /* 'clav' 10 */
        {  95, 1, sq80_8c_b_data + 4 },  /* 'clav' 11 */
        { 103, 1, sq80_08_c_data + 4 },  /* 'clav' 12 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'clav' 13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'clav' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Syn Clavi 1",  /* K4 wave 57 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_58_1_data + 4 }, /* Syn Clavi 1 */
        {  54, 1, k4_58_2_data + 4 }, /* Syn Clavi 1 */
        {  66, 1, k4_58_3_data + 4 }, /* Syn Clavi 1 */
        {  78, 1, k4_58_4_data + 4 }, /* Syn Clavi 1 */
        {  90, 1, k4_58_5_data + 4 }, /* Syn Clavi 1 */
        { 127, 1, k4_58_6_data + 4 }, /* Syn Clavi 1 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Syn Clavi 2",  /* K4 wave 76 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_77_1_data + 4 }, /* Syn Clavi 2 */
        {  54, 1, k4_77_2_data + 4 }, /* Syn Clavi 2 */
        {  66, 1, k4_77_3_data + 4 }, /* Syn Clavi 2 */
        {  78, 1, k4_77_4_data + 4 }, /* Syn Clavi 2 */
        {  90, 1, k4_77_5_data + 4 }, /* Syn Clavi 2 */
        { 127, 1, k4_77_6_data + 4 }, /* Syn Clavi 2 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Harpsichord L",  /* K4 wave 78 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_79_1_data + 4 }, /* Harpsichord L */
        {  54, 1, k4_79_2_data + 4 }, /* Harpsichord L */
        {  66, 1, k4_79_3_data + 4 }, /* Harpsichord L */
        {  78, 1, k4_79_4_data + 4 }, /* Harpsichord L */
        {  90, 1, k4_79_5_data + 4 }, /* Harpsichord L */
        { 127, 1, k4_79_6_data + 4 }, /* Harpsichord L */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Harpsichord M",  /* K4 wave 77 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_78_1_data + 4 }, /* Harpsichord M */
        {  54, 1, k4_78_2_data + 4 }, /* Harpsichord M */
        {  66, 1, k4_78_3_data + 4 }, /* Harpsichord M */
        {  78, 1, k4_78_4_data + 4 }, /* Harpsichord M */
        {  90, 1, k4_78_5_data + 4 }, /* Harpsichord M */
        { 127, 1, k4_78_6_data + 4 }, /* Harpsichord M */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Harpsichord H",  /* K4 wave 79 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_80_1_data + 4 }, /* Harpsichord H */
        {  54, 1, k4_80_2_data + 4 }, /* Harpsichord H */
        {  66, 1, k4_80_3_data + 4 }, /* Harpsichord H */
        {  78, 1, k4_80_4_data + 4 }, /* Harpsichord H */
        {  90, 1, k4_80_5_data + 4 }, /* Harpsichord H */
        { 127, 1, k4_80_6_data + 4 }, /* Harpsichord H */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Marimba",  /* K4 wave 51 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_52_1_data + 4 }, /* Marimba */
        {  54, 1, k4_52_2_data + 4 }, /* Marimba */
        {  66, 1, k4_52_3_data + 4 }, /* Marimba */
        {  78, 1, k4_52_4_data + 4 }, /* Marimba */
        {  90, 1, k4_52_5_data + 4 }, /* Marimba */
        { 127, 1, k4_52_6_data + 4 }, /* Marimba */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Marimba Attack",  /* K4 wave 93 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, k4_94_2_data + 4 }, /* Marimba Attack */
        {  66, 1, k4_94_3_data + 4 }, /* Marimba Attack */
        {  78, 1, k4_94_4_data + 4 }, /* Marimba Attack */
        {  90, 1, k4_94_5_data + 4 }, /* Marimba Attack */
        { 127, 1, k4_94_6_data + 4 }, /* Marimba Attack */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Xylophone",  /* K4 wave 55 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  66, 1, k4_56_3_data + 4 }, /* Xylophone */
        {  78, 1, k4_56_4_data + 4 }, /* Xylophone */
        {  90, 1, k4_56_5_data + 4 }, /* Xylophone */
        { 127, 1, k4_56_6_data + 4 }, /* Xylophone */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Vibraphone Attack",  /* K4 wave 87 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_88_1_data + 4 }, /* Vibraphone Attack */
        {  54, 1, k4_88_2_data + 4 }, /* Vibraphone Attack */
        {  66, 1, k4_88_3_data + 4 }, /* Vibraphone Attack */
        {  78, 1, k4_88_4_data + 4 }, /* Vibraphone Attack */
        {  90, 1, k4_88_5_data + 4 }, /* Vibraphone Attack */
        { 127, 1, k4_88_6_data + 4 }, /* Vibraphone Attack */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Vibraphone 1",  /* K4 wave 88 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_89_1_data + 4 }, /* Vibraphone 1 */
        {  54, 1, k4_89_2_data + 4 }, /* Vibraphone 1 */
        {  66, 1, k4_89_3_data + 4 }, /* Vibraphone 1 */
        {  78, 1, k4_89_4_data + 4 }, /* Vibraphone 1 */
        {  90, 1, k4_89_5_data + 4 }, /* Vibraphone 1 */
        { 127, 1, k4_89_6_data + 4 }, /* Vibraphone 1 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Vibraphone 2",  /* K4 wave 92 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_93_1_data + 4 }, /* Vibraphone 2 */
        {  54, 1, k4_93_2_data + 4 }, /* Vibraphone 2 */
        {  66, 1, k4_93_3_data + 4 }, /* Vibraphone 2 */
        {  78, 1, k4_93_4_data + 4 }, /* Vibraphone 2 */
        {  90, 1, k4_93_5_data + 4 }, /* Vibraphone 2 */  /* -FIX- drops several octaves suddenly */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Horn Vibe",  /* K4 wave 89 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_90_1_data + 4 }, /* Horn Vibe */
        {  54, 1, k4_90_2_data + 4 }, /* Horn Vibe */
        {  66, 1, k4_90_3_data + 4 }, /* Horn Vibe */
        {  78, 1, k4_90_4_data + 4 }, /* Horn Vibe */
        {  90, 1, k4_90_5_data + 4 }, /* Horn Vibe */
        { 127, 1, k4_90_6_data + 4 }, /* Horn Vibe */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Bell",  /* SQ-80 waveform 1 'bell' */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  63, 1, sq80_09_7_data + 4 },  /* 'bell'  0-7 */
        {  71, 1, sq80_0a_8_data + 4 },  /* 'bell'  8 */
        {  79, 1, sq80_0b_9_data + 4 },  /* 'bell'  9 */
        {  87, 1, sq80_0c_a_data + 4 },  /* 'bell' 10 */
        {  95, 1, sq80_24_b_data + 4 },  /* 'bell' 11 */
        { 103, 1, sq80_24_c_data + 4 },  /* 'bell' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'bell' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'bell' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Bell 2",  /* SQ-80 waveform 47 'bell_2' */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  63, 1, sq80_78_7_data + 4 },  /* 'bell_2'  0-7 */
        {  79, 1, sq80_78_9_data + 4 },  /* 'bell_2'  8-9 */
        {  95, 1, sq80_77_b_data + 4 },  /* 'bell_2' 10-11 */
        { 103, 1, sq80_77_c_data + 4 },  /* 'bell_2' 12 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Steel Drum 1",  /* K4 wave 90 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_91_1_data + 4 }, /* Steel Drum 1 */
        {  54, 1, k4_91_2_data + 4 }, /* Steel Drum 1 */
        {  66, 1, k4_91_3_data + 4 }, /* Steel Drum 1 */
        {  78, 1, k4_91_4_data + 4 }, /* Steel Drum 1 */
        {  90, 1, k4_91_5_data + 4 }, /* Steel Drum 1 */
        { 127, 1, k4_91_6_data + 4 }, /* Steel Drum 1 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Steel Drum 2",  /* K4 wave 91 */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, k4_92_2_data + 4 }, /* Steel Drum 2 */
        {  66, 1, k4_92_3_data + 4 }, /* Steel Drum 2 */
        {  78, 1, k4_92_4_data + 4 }, /* Steel Drum 2 */
        {  90, 1, k4_92_5_data + 4 }, /* Steel Drum 2 */
        { 127, 1, k4_92_6_data + 4 }, /* Steel Drum 2 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },

/* ==== Organ ==== */

    { "Organ|E.Organ",  /* SQ-80 waveform 15 'organ' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  87, 1, sq80_0d_a_data + 4 },  /* 'organ'  0-10 */
        {  95, 1, sq80_0e_b_data + 4 },  /* 'organ' 11 */
        { 103, 1, sq80_0e_c_data + 4 },  /* 'organ' 12 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'organ' 13-14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Organ|E.Organ 1",  /* K4 wave 34 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, k4_35_2_data + 4 }, /* E.Organ 1 */
        {  66, 1, k4_35_3_data + 4 }, /* E.Organ 1 */
        {  78, 1, k4_35_4_data + 4 }, /* E.Organ 1 */
        {  90, 1, k4_35_5_data + 4 }, /* E.Organ 1 */
        { 127, 1, k4_35_6_data + 4 }, /* E.Organ 1 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|E.Organ 2",  /* K4 wave 35 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, k4_36_2_data + 4 }, /* E.Organ 2 */
        {  66, 1, k4_36_3_data + 4 }, /* E.Organ 2 */
        {  78, 1, k4_36_4_data + 4 }, /* E.Organ 2 */
        {  90, 1, k4_36_5_data + 4 }, /* E.Organ 2 */
        { 127, 1, k4_36_6_data + 4 }, /* E.Organ 2 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|E.Organ 3",  /* K4 wave 37 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, k4_38_2_data + 4 }, /* E.Organ 3 */
        {  66, 1, k4_38_3_data + 4 }, /* E.Organ 3 */
        {  78, 1, k4_38_4_data + 4 }, /* E.Organ 3 */
        {  90, 1, k4_38_5_data + 4 }, /* E.Organ 3 */
        { 127, 1, k4_38_6_data + 4 }, /* E.Organ 3 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|E.Organ 4",  /* K4 wave 38 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_39_1_data + 4 }, /* E.Organ 4 */
        {  54, 1, k4_39_2_data + 4 }, /* E.Organ 4 */
        {  66, 1, k4_39_3_data + 4 }, /* E.Organ 4 */
        {  78, 1, k4_39_4_data + 4 }, /* E.Organ 4 */
        {  90, 1, k4_39_5_data + 4 }, /* E.Organ 4 */
        { 127, 1, k4_39_6_data + 4 }, /* E.Organ 4 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|E.Organ 5",  /* K4 wave 39 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_40_1_data + 4 }, /* E.Organ 5 */
        {  54, 1, k4_40_2_data + 4 }, /* E.Organ 5 */
        {  66, 1, k4_40_3_data + 4 }, /* E.Organ 5 */
        {  78, 1, k4_40_4_data + 4 }, /* E.Organ 5 */
        {  90, 1, k4_40_5_data + 4 }, /* E.Organ 5 */
        { 127, 1, k4_40_6_data + 4 }, /* E.Organ 5 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|E.Organ 6",  /* K4 wave 40 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, k4_41_2_data + 4 }, /* E.Organ 6 */
        {  66, 1, k4_41_3_data + 4 }, /* E.Organ 6 */
        {  78, 1, k4_41_4_data + 4 }, /* E.Organ 6 */
        {  90, 1, k4_41_5_data + 4 }, /* E.Organ 6 */
        { 127, 1, k4_41_6_data + 4 }, /* E.Organ 6 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|E.Organ 7",  /* K4 wave 41 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_42_1_data + 4 }, /* E.Organ 7 */
        {  54, 1, k4_42_2_data + 4 }, /* E.Organ 7 */
        {  66, 1, k4_42_3_data + 4 }, /* E.Organ 7 */
        {  78, 1, k4_42_4_data + 4 }, /* E.Organ 7 */
        {  90, 1, k4_42_5_data + 4 }, /* E.Organ 7 */
        { 127, 1, k4_42_6_data + 4 }, /* E.Organ 7 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|E.Organ 8",  /* K4 wave 42 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, k4_43_2_data + 4 }, /* E.Organ 8 */
        {  66, 1, k4_43_3_data + 4 }, /* E.Organ 8 */
        {  78, 1, k4_43_4_data + 4 }, /* E.Organ 8 */
        {  90, 1, k4_43_5_data + 4 }, /* E.Organ 8 */
        { 127, 1, k4_43_6_data + 4 }, /* E.Organ 8 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|E.Organ 9",  /* K4 wave 43 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_44_1_data + 4 }, /* E.Organ 9 */
        {  54, 1, k4_44_2_data + 4 }, /* E.Organ 9 */
        {  66, 1, k4_44_3_data + 4 }, /* E.Organ 9 */
        {  78, 1, k4_44_4_data + 4 }, /* E.Organ 9 */
        {  90, 1, k4_44_5_data + 4 }, /* E.Organ 9 */
        { 127, 1, k4_44_6_data + 4 }, /* E.Organ 9 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|E.Organ 10",  /* K4 wave 59 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  66, 1, k4_60_3_data + 4 }, /* E.Organ 10 */
        {  78, 1, k4_60_4_data + 4 }, /* E.Organ 10 */
        {  90, 1, k4_60_5_data + 4 }, /* E.Organ 10 */
        { 127, 1, k4_60_6_data + 4 }, /* E.Organ 10 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|E.Organ 11",  /* K4 wave 60 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_61_1_data + 4 }, /* E.Organ 11 */
        {  54, 1, k4_61_2_data + 4 }, /* E.Organ 11 */
        {  66, 1, k4_61_3_data + 4 }, /* E.Organ 11 */
        {  78, 1, k4_61_4_data + 4 }, /* E.Organ 11 */
        {  90, 1, k4_61_5_data + 4 }, /* E.Organ 11 */
        { 127, 1, k4_61_6_data + 4 }, /* E.Organ 11 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|E.Organ 12",  /* K4 wave 61 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, k4_62_2_data + 4 }, /* E.Organ 12 */
        {  66, 1, k4_62_3_data + 4 }, /* E.Organ 12 */
        {  78, 1, k4_62_4_data + 4 }, /* E.Organ 12 */
        {  90, 1, k4_62_5_data + 4 }, /* E.Organ 12 */
        { 127, 1, k4_62_6_data + 4 }, /* E.Organ 12 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|Positif",  /* K4 wave 36 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_37_1_data + 4 }, /* Positif */
        {  54, 1, k4_37_2_data + 4 }, /* Positif */
        {  66, 1, k4_37_3_data + 4 }, /* Positif */
        {  78, 1, k4_37_4_data + 4 }, /* Positif */
        {  90, 1, k4_37_5_data + 4 }, /* Positif */
        { 127, 1, k4_37_6_data + 4 }, /* Positif */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Organ|Big Pipe",  /* K4 wave 62 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_63_1_data + 4 }, /* Big Pipe */
        {  54, 1, k4_63_2_data + 4 }, /* Big Pipe */
        {  66, 1, k4_63_3_data + 4 }, /* Big Pipe */
        {  78, 1, k4_63_4_data + 4 }, /* Big Pipe */
        {  90, 1, k4_63_5_data + 4 }, /* Big Pipe */
        { 127, 1, k4_63_6_data + 4 }, /* Big Pipe */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },

/* ==== Strings ==== */

    { "Strings|String",  /* SQ-80 waveform 44 'string' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  55, 1, sq80_80_6_data + 4 },  /* 'string'  0-6 */
        {  63, 1, sq80_80_7_data + 4 },  /* 'string'  7 */
        {  71, 1, sq80_80_8_data + 4 },  /* 'string'  8 */
        {  79, 1, sq80_7d_9_data + 4 },  /* 'string'  9 */
        {  87, 1, sq80_7e_a_data + 4 },  /* 'string' 10 */
        {  95, 1, sq80_7f_b_data + 4 },  /* 'string' 11 */
        { 103, 1, sq80_7f_c_data + 4 },  /* 'string' 12 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'string' 13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'string' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Strings|Cello",  /* K4 wave 54 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_55_1_data + 4 }, /* Cello */
        {  54, 1, k4_55_2_data + 4 }, /* Cello */
        {  66, 1, k4_55_3_data + 4 }, /* Cello */
        {  78, 1, k4_55_4_data + 4 }, /* Cello */
        {  90, 1, k4_55_5_data + 4 }, /* Cello */
        { 127, 1, k4_55_6_data + 4 }, /* Cello */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },

/* ==== Guitar ==== */

    { "Guitar|Classic Guitar",  /* K4 wave 44 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_45_1_data + 4 }, /* Classic Guitar */
        {  54, 1, k4_45_2_data + 4 }, /* Classic Guitar */
        {  66, 1, k4_45_3_data + 4 }, /* Classic Guitar */
        {  78, 1, k4_45_4_data + 4 }, /* Classic Guitar */
        {  90, 1, k4_45_5_data + 4 }, /* Classic Guitar */
        { 127, 1, k4_45_6_data + 4 }, /* Classic Guitar */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Guitar|Steel Strings",  /* K4 wave 45 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_46_1_data + 4 }, /* Steel Strings */
        {  54, 1, k4_46_2_data + 4 }, /* Steel Strings */
        {  66, 1, k4_46_3_data + 4 }, /* Steel Strings */
        {  78, 1, k4_46_4_data + 4 }, /* Steel Strings */
        {  90, 1, k4_46_5_data + 4 }, /* Steel Strings */
        { 127, 1, k4_46_6_data + 4 }, /* Steel Strings */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Guitar|Harp",  /* K4 wave 46 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_47_1_data + 4 }, /* Harp */
        {  54, 1, k4_47_2_data + 4 }, /* Harp */
        {  66, 1, k4_47_3_data + 4 }, /* Harp */
        {  78, 1, k4_47_4_data + 4 }, /* Harp */
        {  90, 1, k4_47_5_data + 4 }, /* Harp */
        { 127, 1, k4_47_6_data + 4 }, /* Harp */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Guitar|Glass Harp",  /* K4 wave 53 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_54_1_data + 4 }, /* Glass Harp */
        {  54, 1, k4_54_2_data + 4 }, /* Glass Harp */
        {  66, 1, k4_54_3_data + 4 }, /* Glass Harp */
        {  78, 1, k4_54_4_data + 4 }, /* Glass Harp */
        {  90, 1, k4_54_5_data + 4 }, /* Glass Harp */
        { 127, 1, k4_54_6_data + 4 }, /* Glass Harp */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Guitar|Glass Harp 2",  /* K4 wave 63 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, k4_64_2_data + 4 }, /* Glass Harp 2 */
        {  66, 1, k4_64_3_data + 4 }, /* Glass Harp 2 */
        {  78, 1, k4_64_4_data + 4 }, /* Glass Harp 2 */
        {  90, 1, k4_64_5_data + 4 }, /* Glass Harp 2 */
        { 127, 1, k4_64_6_data + 4 }, /* Glass Harp 2 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Guitar|Koto",  /* K4 wave 81 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_82_1_data + 4 }, /* Koto */
        {  54, 1, k4_82_2_data + 4 }, /* Koto */
        {  66, 1, k4_82_3_data + 4 }, /* Koto */
        {  78, 1, k4_82_4_data + 4 }, /* Koto */
        {  90, 1, k4_82_5_data + 4 }, /* Koto */
        { 127, 1, k4_82_6_data + 4 }, /* Koto */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Guitar|Sitar L",  /* K4 wave 82 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_83_1_data + 4 }, /* Sitar L */
        {  54, 1, k4_83_2_data + 4 }, /* Sitar L */
        {  66, 1, k4_83_3_data + 4 }, /* Sitar L */
        {  78, 1, k4_83_4_data + 4 }, /* Sitar L */
        {  90, 1, k4_83_5_data + 4 }, /* Sitar L */
        { 127, 1, k4_83_6_data + 4 }, /* Sitar L */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Guitar|Sitar H",  /* K4 wave 83 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_84_1_data + 4 }, /* Sitar H */
        {  54, 1, k4_84_2_data + 4 }, /* Sitar H */
        {  66, 1, k4_84_3_data + 4 }, /* Sitar H */
        {  78, 1, k4_84_4_data + 4 }, /* Sitar H */
        {  90, 1, k4_84_5_data + 4 }, /* Sitar H */
        { 127, 1, k4_84_6_data + 4 }, /* Sitar H */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },

/* ==== Bass ==== */

    { "Bass|E.Bass",  /* SQ-80 waveform 8 'bass' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  63, 1, sq80_27_7_data + 4 },  /* 'bass'  0-7 */
        {  71, 1, sq80_28_8_data + 4 },  /* 'bass'  8 */
        {  85, 1, sq80_29_9_data + 4 },  /* 'bass'  9+ */
        {  95, 1, sq80_2a_b_data + 4 },  /* 'bass' 11 */
        { 103, 1, sq80_2a_c_data + 4 },  /* 'bass' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'bass' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'bass' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Bass|Wood Bass",  /* K4 wave 47 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, k4_48_2_data + 4 }, /* Wood Bass */
        {  66, 1, k4_48_3_data + 4 }, /* Wood Bass */
        {  78, 1, k4_48_4_data + 4 }, /* Wood Bass */
        {  90, 1, k4_48_5_data + 4 }, /* Wood Bass */
        { 127, 1, k4_48_6_data + 4 }, /* Wood Bass */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Bass|Digi Bass",  /* K4 wave 49 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  66, 1, k4_50_3_data + 4 }, /* Digi Bass */
        {  78, 1, k4_50_4_data + 4 }, /* Digi Bass */
        {  90, 1, k4_50_5_data + 4 }, /* Digi Bass */
        { 127, 1, k4_50_6_data + 4 }, /* Digi Bass */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Bass|Finger Bass",  /* K4 wave 50 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_51_1_data + 4 }, /* Finger Bass */
        {  54, 1, k4_51_2_data + 4 }, /* Finger Bass */
        {  66, 1, k4_51_3_data + 4 }, /* Finger Bass */
        {  78, 1, k4_51_4_data + 4 }, /* Finger Bass */
        {  90, 1, k4_51_5_data + 4 }, /* Finger Bass */
        { 127, 1, k4_51_6_data + 4 }, /* Finger Bass */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Bass|Pick Bass",  /* K4 wave 84 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_85_1_data + 4 }, /* Pick Bass */
        {  54, 1, k4_85_2_data + 4 }, /* Pick Bass */
        {  66, 1, k4_85_3_data + 4 }, /* Pick Bass */
        {  78, 1, k4_85_4_data + 4 }, /* Pick Bass */
        {  90, 1, k4_85_5_data + 4 }, /* Pick Bass */
        { 127, 1, k4_85_6_data + 4 }, /* Pick Bass */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Bass|Syn Bass 1",  /* K4 wave 67 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_68_1_data + 4 }, /* Syn Bass 1 */
        {  54, 1, k4_68_2_data + 4 }, /* Syn Bass 1 */
        {  66, 1, k4_68_3_data + 4 }, /* Syn Bass 1 */
        {  78, 1, k4_68_4_data + 4 }, /* Syn Bass 1 */
        {  90, 1, k4_68_5_data + 4 }, /* Syn Bass 1 */
        { 127, 1, k4_68_6_data + 4 }, /* Syn Bass 1 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Bass|Syn Bass 2",  /* K4 wave 68 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_69_1_data + 4 }, /* Syn Bass 2 */
        {  54, 1, k4_69_2_data + 4 }, /* Syn Bass 2 */
        {  66, 1, k4_69_3_data + 4 }, /* Syn Bass 2 */
        {  78, 1, k4_69_4_data + 4 }, /* Syn Bass 2 */
        {  90, 1, k4_69_5_data + 4 }, /* Syn Bass 2 */
        { 127, 1, k4_69_6_data + 4 }, /* Syn Bass 2 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Bass|Syn Bass 3",  /* K4 wave 48 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_49_1_data + 4 }, /* Syn Bass 3 */
        {  54, 1, k4_49_2_data + 4 }, /* Syn Bass 3 */
        {  66, 1, k4_49_3_data + 4 }, /* Syn Bass 3 */
        {  78, 1, k4_49_4_data + 4 }, /* Syn Bass 3 */
        {  90, 1, k4_49_5_data + 4 }, /* Syn Bass 3 */
        { 127, 1, k4_49_6_data + 4 }, /* Syn Bass 3 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Bass|Syn Bass 4",  /* K4 wave 66 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, k4_67_2_data + 4 }, /* Syn Bass 4 */
        {  66, 1, k4_67_3_data + 4 }, /* Syn Bass 4 */
        {  78, 1, k4_67_4_data + 4 }, /* Syn Bass 4 */
        {  90, 1, k4_67_5_data + 4 }, /* Syn Bass 4 */
        { 127, 1, k4_67_6_data + 4 }, /* Syn Bass 4 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Bass|Syn Bass 5",  /* K4 wave 85 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_86_1_data + 4 }, /* Syn Bass 5 */
        {  54, 1, k4_86_2_data + 4 }, /* Syn Bass 5 */
        {  66, 1, k4_86_3_data + 4 }, /* Syn Bass 5 */
        {  78, 1, k4_86_4_data + 4 }, /* Syn Bass 5 */
        {  90, 1, k4_86_5_data + 4 }, /* Syn Bass 5 */
        { 127, 1, k4_86_6_data + 4 }, /* Syn Bass 5 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Bass|Syn Bass 6",  /* K4 wave 86 */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_87_1_data + 4 }, /* Syn Bass 6 */
        {  54, 1, k4_87_2_data + 4 }, /* Syn Bass 6 */
        {  66, 1, k4_87_3_data + 4 }, /* Syn Bass 6 */
        {  78, 1, k4_87_4_data + 4 }, /* Syn Bass 6 */
        {  90, 1, k4_87_5_data + 4 }, /* Syn Bass 6 */
        { 127, 1, k4_87_6_data + 4 }, /* Syn Bass 6 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },

/* ==== Voice ==== */

    { "Voice|Syn Voice",  /* K4 wave 52 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_53_1_data + 4 }, /* Syn Voice */
        {  54, 1, k4_53_2_data + 4 }, /* Syn Voice */
        {  66, 1, k4_53_3_data + 4 }, /* Syn Voice */
        {  78, 1, k4_53_4_data + 4 }, /* Syn Voice */
        {  90, 1, k4_53_5_data + 4 }, /* Syn Voice */
        { 127, 1, k4_53_6_data + 4 }, /* Syn Voice */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Voice|Formant 1",  /* SQ-80 waveform 19 'formt1' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  15, 1, sq80_2b_5_data + 4 },  /* 'formt1'  0-1 */
        {  23, 1, sq80_2c_6_data + 4 },  /* 'formt1'  2 */
        {  31, 1, sq80_2d_7_data + 4 },  /* 'formt1'  3 */
        {  39, 1, sq80_2e_8_data + 4 },  /* 'formt1'  4 */
        {  47, 1, sq80_2f_9_data + 4 },  /* 'formt1'  5 */
        {  55, 1, sq80_30_a_data + 4 },  /* 'formt1'  6 */
        {  63, 1, sq80_31_a_data + 4 },  /* 'formt1'  7 */
        {  71, 1, sq80_32_a_data + 4 },  /* 'formt1'  8 */
        {  95, 1, sq80_33_b_data + 4 },  /* 'formt1'  9-11 */
        { 103, 1, sq80_33_c_data + 4 },  /* 'formt1' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'formt1' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'formt1' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Voice|Formant Mellow",
    /* This is a set of the SQ-80 'formt' waves which //christian trimmed to
     * incorrect lengths, but the result is useful.... */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  16, 1, sq80_formant_1_2_data + 4 },
        {  25, 1, sq80_formant_2_3_data + 4 },
        {  34, 1, sq80_formant_3_4_data + 4 },
        {  43, 1, sq80_formant_4_5_data + 4 },
        {  52, 1, sq80_formant_5_6_data + 4 },
        {  61, 1, sq80_formant_6_7_data + 4 },
        { 102, 1, sq80_formant_8_9_data + 4 },
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Voice|Voice 2",  /* SQ-80 waveform 12 'voice2' */
#ifdef Y_GUI
      4
#endif
#ifdef Y_PLUGIN
      {
        {  47, 1, sq80_25_5_data + 4 },  /* 'voice2'  0-5 */
        {  55, 1, sq80_3a_6_data + 4 },  /* 'voice2'  6 */
        {  63, 1, sq80_26_7_data + 4 },  /* 'voice2'  7 */
        {  71, 1, sq80_3b_8_data + 4 },  /* 'voice2'  8 */
        {  79, 1, sq80_3c_9_data + 4 },  /* 'voice2'  9 */
        {  87, 1, sq80_3d_a_data + 4 },  /* 'voice2' 10 */
        {  95, 1, sq80_3e_b_data + 4 },  /* 'voice2' 11 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'voice2' 12-14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },

/* ==== (Periodic) Noise ==== */

    { "Noise|Random",  /* K4 wave 64 */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_65_1_data + 4 }, /* Random */
        {  54, 1, k4_65_2_data + 4 }, /* Random */
        {  66, 1, k4_65_3_data + 4 }, /* Random */
        {  78, 1, k4_65_4_data + 4 }, /* Random */
        {  90, 1, k4_65_5_data + 4 }, /* Random */
        { 127, 1, k4_65_6_data + 4 }, /* Random */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Noise|Random HF",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 127, 1, k4_65_1_data + 4 }, /* Random */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Noise|Buzz",  /* K4 wave 80 ('E.Organ 13'?!) */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  42, 1, k4_81_1_data + 4 }, /* E.Organ 13 */
        {  54, 1, k4_81_2_data + 4 }, /* E.Organ 13 */
        {  66, 1, k4_81_3_data + 4 }, /* E.Organ 13 */
        {  78, 1, k4_81_4_data + 4 }, /* E.Organ 13 */
        {  90, 1, k4_81_5_data + 4 }, /* E.Organ 13 */
        { 127, 1, k4_81_6_data + 4 }, /* E.Organ 13 */
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Noise|Grit 2",  /* SQ-80 waveform 37 'grit_2' */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  39, 1, sq80_6d_4_data + 4 },  /* 'grit_2'  0-4 */
        {  47, 1, sq80_6e_5_data + 4 },  /* 'grit_2'  5 */
        {  55, 1, sq80_6f_6_data + 4 },  /* 'grit_2'  6 */
        {  63, 1, sq80_70_7_data + 4 },  /* 'grit_2'  7 */
        {  71, 1, sq80_71_9_data + 4 },  /* 'grit_2'  8 */
        {  79, 1, sq80_72_9_data + 4 },  /* 'grit_2'  9 */
        {  87, 1, sq80_73_b_data + 4 },  /* 'grit_2' 10 */
        { 103, 1, sq80_74_c_data + 4 },  /* 'grit_2' 11-12 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'grit_2' 13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'grit_2' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Noise|Noise 1",
#ifdef Y_GUI
      4
#endif
#ifdef Y_PLUGIN
      {
        { 127, 1, sq80_noise1_16_data + 4 },
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Noise|Noise 2",
#ifdef Y_GUI
      4
#endif
#ifdef Y_PLUGIN
      {
        { 127, 1, sq80_noise2_16_data + 4 },
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Noise|Noise 3",
#ifdef Y_GUI
      4
#endif
#ifdef Y_PLUGIN
      {
        { 127, 1, sq80_noise3_16_data + 4 },
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },

/* ==== TX81Z Waveforms ==== */

    { "TX81Z|TX81Z Wave 2",  /* square-tri-ish */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  70, 1, tx81z_2_8_data + 4 },
        {  79, 1, tx81z_2_9_data + 4 },
        {  88, 1, tx81z_2_10_data + 4 },
        {  97, 1, tx81z_2_11_data + 4 },
        { 106, 1, tx81z_2_12_data + 4 },
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "TX81Z|TX81Z Wave 3",  /* soft saw-ish */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  70, 1, tx81z_3_8_data + 4 },
        {  79, 1, tx81z_3_9_data + 4 },
        {  88, 1, tx81z_3_10_data + 4 },
        {  97, 1, tx81z_3_11_data + 4 },
        { 106, 1, tx81z_3_12_data + 4 },
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "TX81Z|TX81Z Wave 4",  /* slightly less soft saw-ish */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  70, 1, tx81z_4_8_data + 4 },
        {  79, 1, tx81z_4_9_data + 4 },
        {  88, 1, tx81z_4_10_data + 4 },
        {  97, 1, tx81z_4_11_data + 4 },
        { 106, 1, tx81z_4_12_data + 4 },
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "TX81Z|TX81Z Wave 5",  /* 40%-rect-ish? */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  70, 1, tx81z_5_8_data + 4 },
        {  79, 1, tx81z_5_9_data + 4 },
        {  88, 1, tx81z_5_10_data + 4 },
        {  97, 1, tx81z_5_11_data + 4 },
        { 106, 1, tx81z_5_12_data + 4 },
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "TX81Z|TX81Z Wave 6",  /* med-soft saw-ish */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  61, 1, tx81z_6_7_data + 4 },
        {  70, 1, tx81z_6_8_data + 4 },
        {  79, 1, tx81z_6_9_data + 4 },
        {  88, 1, tx81z_6_10_data + 4 },
        {  97, 1, tx81z_6_11_data + 4 },
        { 106, 1, tx81z_6_12_data + 4 },
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "TX81Z|TX81Z Wave 7",  /* soft-saw/square-ish */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  70, 1, tx81z_7_8_data + 4 },
        {  79, 1, tx81z_7_9_data + 4 },
        {  88, 1, tx81z_7_10_data + 4 },
        {  97, 1, tx81z_7_11_data + 4 },
        { 106, 1, tx81z_7_12_data + 4 },
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "TX81Z|TX81Z Wave 8",  /* brighter saw-ish */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  61, 1, tx81z_8_7_data + 4 },
        {  70, 1, tx81z_8_8_data + 4 },
        {  79, 1, tx81z_8_9_data + 4 },
        {  88, 1, tx81z_8_10_data + 4 },
        {  97, 1, tx81z_8_11_data + 4 },
        { 106, 1, tx81z_8_12_data + 4 },
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },

/* ==== Polynomials for the waveshaper ==== */

    /* -FIX- Need to add more someday.... */
    { "Waveshaper|WS Cheby T5",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_ws_cheby_t5_data + 4 },
      }
#endif
    },

/* ==== Non-band-limited waveforms for LFO ==== */

    { "LFO|LFO S/H 1",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_lfo_s_h_1_data + 4 },
      }
#endif
    },
    { "LFO|LFO S/H 2",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_lfo_s_h_2_data + 4 },
      }
#endif
    },
    { "LFO|LFO S/H 3",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_lfo_s_h_3_data + 4 },
      }
#endif
    },
    { "LFO|LFO S/H 4",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_lfo_s_h_4_data + 4 },
      }
#endif
    },
    { "LFO|LFO Saw",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_lfo_saw_data + 4 },
      }
#endif
    },
    { "LFO|LFO Tri",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_lfo_tri_data + 4 },
      }
#endif
    },
    { "LFO|LFO Rect 1/3",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_lfo_rect13_data + 4 },
      }
#endif
    },
    { "LFO|LFO Rect 1/4",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_lfo_rect14_data + 4 },
      }
#endif
    },
    { "LFO|LFO Rect 1/6",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_lfo_rect16_data + 4 },
      }
#endif
    },
    { "LFO|LFO Rect 2/4",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_lfo_rect24_data + 4 },
      }
#endif
    },
    { "LFO|LFO Square",
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        { 256, 1, yw_lfo_square_data + 4 },
      }
#endif
    },

/* ==== Waveforms added with the 20051231 release ==== */

    { "Analog|Jagged Saw",  /* Bristol jagged edged ramp */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  70, 1, bristol_jagged_8_data + 4 },
        {  79, 1, bristol_jagged_9_data + 4 },
        {  88, 1, bristol_jagged_10_data + 4 },
        {  97, 1, bristol_jagged_11_data + 4 },
        { 106, 1, bristol_jagged_12_data + 4 },
        { 256, 1, yw_sin_1_data + 4 }, /* end-of-waves marker */
      }
#endif
    },
    { "Sines|Sine 1+2+3+4",  /* SQ-80 waveform 26 '4_octs' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  95, 1, sq80_0e_b_data + 4 },  /* '4_octs'  0-11 */
        { 103, 1, sq80_0e_c_data + 4 },  /* '4_octs' 12 */
        { 119, 1, sq80_17_e_data + 4 },  /* '4_octs' 13-14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Pulse 2",  /* SQ-80 waveform 4 'pulse' */
    /* This has approximately the same power spectrum as the K4 pulse wave,
     * but not the coherent phases, so it's a smoother wave than the K4
     * Dirac pulse. */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  54, 1, sq80_02_5_data + 4 },  /* 'pulse'  0-5+ */
        {  63, 1, sq80_03_7_data + 4 },  /* 'pulse'  7 */
        {  71, 1, sq80_04_8_data + 4 },  /* 'pulse'  8 */
        {  79, 1, sq80_05_9_data + 4 },  /* 'pulse'  9 */
        {  87, 1, sq80_06_a_data + 4 },  /* 'pulse' 10 */
        {  95, 1, sq80_07_b_data + 4 },  /* 'pulse' 11 */
        { 103, 1, sq80_08_c_data + 4 },  /* 'pulse' 12 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'pulse' 13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'pulse' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Soft Pulse",  /* SQ-80 waveform 24 'pulse2' */
    /* This is a subset of SQ-80 waveform 4 'pulse' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  95, 1, sq80_07_b_data + 4 },  /* 'pulse2'  0-11 */
        { 103, 1, sq80_08_c_data + 4 },  /* 'pulse2' 12 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'pulse2' 13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'pulse2' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Analog|Soft Square",  /* SQ-80 waveform 25 'sqr__2' */
    /* This is a subset of SQ-80 waveform 3 'square' */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        { 103, 1, sq80_4b_c_data + 4 },  /* 'sqr__2'  0-12 */
        { 111, 1, sq80_19_d_data + 4 },  /* 'sqr__2' 13 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Digital|Digital 2",  /* SQ-80 waveform 46 'digit2' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  78, 1, sq80_87_8_data + 4 },  /* 'digit2'  0-8+ */
        {  87, 1, sq80_88_a_data + 4 },  /* 'digit2' 10 */
        {  95, 1, sq80_88_b_data + 4 },  /* 'digit2' 11 */
        { 103, 1, sq80_88_c_data + 4 },  /* 'digit2' 12 */
        { 111, 1, sq80_19_d_data + 4 },  /* 'digit2' 13 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Brass|Medium Brass",  /* SQ-80 waveform 43 'brass' with 256-byte sample 0x76 */
    /* This is the SQ-80 waveform 43 'brass' as specified in the osrom,
     * except that I've used //christian's 256-frame length for sample 0x76,
     * instead of the osrom-specified 512. This gives a harmonic profile more
     * similar to samples 0x7b and 0x7c. */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  63, 1, sq80_76_7_data + 4 },  /* 'brass'  0-7 */
        {  79, 1, sq80_7b_9_data + 4 },  /* 'brass'  8-9 */
        {  87, 1, sq80_7b_a_data + 4 },  /* 'brass' 10 */
        {  95, 1, sq80_7c_b_data + 4 },  /* 'brass' 11 */
        { 103, 1, sq80_7c_c_data + 4 },  /* 'brass' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'brass' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'brass' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Brass|Hard Brass",  /* SQ-80 raw sample 0x76 */
    /* This is raw sample 0x76 with the osrom-specified length of 512, over
     * a range of 0-87.  It's very bright, somewhat thin, and rather sax-
     * like in the low registers. */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  63, 1, sq80_76x_7_data + 4 },  /* 'brass'  0-7 */
        {  71, 1, sq80_76x_8_data + 4 },  /* 'brass'  8 */
        {  79, 1, sq80_76x_9_data + 4 },  /* 'brass'  9 */
        {  87, 1, sq80_76x_a_data + 4 },  /* 'brass' 10 */
        {  95, 1, sq80_7c_b_data + 4 },  /* 'brass' 11 */
        { 103, 1, sq80_7c_c_data + 4 },  /* 'brass' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'brass' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'brass' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Woodwind|Kick",  /* SQ-80 waveform 13 'kick' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  31, 1, sq80_1f_3_data + 4 },  /* 'kick'  0-3 */
        {  39, 1, sq80_1f_4_data + 4 },  /* 'kick'  4 */
        {  47, 1, sq80_1f_5_data + 4 },  /* 'kick'  5 */
        {  55, 1, sq80_1f_6_data + 4 },  /* 'kick'  6 */
        {  63, 1, sq80_1f_7_data + 4 },  /* 'kick'  7 */
        {  71, 1, sq80_1f_8_data + 4 },  /* 'kick'  8 */
        {  79, 1, sq80_1f_9_data + 4 },  /* 'kick'  9 */
        {  87, 1, sq80_1f_a_data + 4 },  /* 'kick' 10 */
        {  95, 1, sq80_1f_b_data + 4 },  /* 'kick' 11 */
        { 103, 1, sq80_1f_c_data + 4 },  /* 'kick' 12 */
        { 111, 1, sq80_1f_d_data + 4 },  /* 'kick' 13 */
        { 119, 1, yw_sin_2_data + 4 },   /* 'kick' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Piano Full",  /* SQ-80 waveform 9 'piano' */
    /* This is as close as we can get to the original piano waveform. The
     * lowest three waves (21 through 23) were originally four cycles long,
     * but here they've each been folded in to a single cycle. */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  39, 1, sq80_21_4_data + 4 },  /* 'piano'  0-4 */
        {  47, 1, sq80_22_5_data + 4 },  /* 'piano'  5 */
        {  55, 1, sq80_23_6_data + 4 },  /* 'piano'  6 */
        {  63, 1, sq80_42_7_data + 4 },  /* 'piano'  7 */
        {  71, 1, sq80_43_8_data + 4 },  /* 'piano'  8 */
        {  79, 1, sq80_44_9_data + 4 },  /* 'piano'  9 */
        {  87, 1, sq80_45_a_data + 4 },  /* 'piano' 10 */
        {  95, 1, sq80_45_b_data + 4 },  /* 'piano' 11 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'piano' 12-13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'piano' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|E.Piano 10",  /* SQ-80 waveform 29 'e_pno2' */
    /* This is a subset of SQ-80 waveform 10 'el_pno'. */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  95, 1, sq80_41_b_data + 4 },  /* 'e_pno2'  0-11 */
        { 103, 1, sq80_41_c_data + 4 },  /* 'e_pno2' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'e_pno2' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'e_pno2' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Keyboard|Chime",  /* SQ-80 waveform 53 'chime' */
#ifdef Y_GUI
      2
#endif
#ifdef Y_PLUGIN
      {
        {  55, 1, sq80_68_6_data + 4 },  /* 'chime'  0-6 */
        {  63, 1, sq80_68_7_data + 4 },  /* 'chime'  7 */
        {  71, 1, sq80_68_8_data + 4 },  /* 'chime'  8 */
        {  79, 1, sq80_69_9_data + 4 },  /* 'chime'  9 */
        {  95, 1, sq80_69_b_data + 4 },  /* 'chime' 10-11 */
        { 103, 1, yw_sin_6_data + 4 },   /* 'chime' 12 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Bass|E.Bass 2",  /* SQ-80 waveform 28 'bass_2' */
    /* This is a subset of SQ-80 waveform 8 'bass' */
#ifdef Y_GUI
      0
#endif
#ifdef Y_PLUGIN
      {
        {  85, 1, sq80_29_9_data + 4 },  /* 'bass_2'  0-9+ */
        {  95, 1, sq80_2a_b_data + 4 },  /* 'bass_2' 11 */
        { 103, 1, sq80_2a_c_data + 4 },  /* 'bass_2' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'bass_2' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'bass_2' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Voice|Formant 2",  /* SQ-80 waveform 20 'formt2' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  23, 1, sq80_2b_5_data + 4 },  /* 'formt2'  0-2 */
        {  31, 1, sq80_2c_6_data + 4 },  /* 'formt2'  3 */
        {  39, 1, sq80_2d_7_data + 4 },  /* 'formt2'  4 */
        {  47, 1, sq80_2e_8_data + 4 },  /* 'formt2'  5 */
        {  55, 1, sq80_2f_9_data + 4 },  /* 'formt2'  6 */
        {  63, 1, sq80_30_a_data + 4 },  /* 'formt2'  7 */
        {  71, 1, sq80_31_a_data + 4 },  /* 'formt2'  8 */
        {  79, 1, sq80_32_a_data + 4 },  /* 'formt2'  9 */
        {  95, 1, sq80_33_b_data + 4 },  /* 'formt2' 10-11 */
        { 103, 1, sq80_33_c_data + 4 },  /* 'formt2' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'formt2' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'formt2' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Voice|Formant 3",  /* SQ-80 waveform 21 'formt3' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  31, 1, sq80_2b_5_data + 4 },  /* 'formt3'  0-3 */
        {  39, 1, sq80_2c_6_data + 4 },  /* 'formt3'  4 */
        {  47, 1, sq80_2d_7_data + 4 },  /* 'formt3'  5 */
        {  55, 1, sq80_2e_8_data + 4 },  /* 'formt3'  6 */
        {  63, 1, sq80_2f_9_data + 4 },  /* 'formt3'  7 */
        {  71, 1, sq80_30_a_data + 4 },  /* 'formt3'  8 */
        {  79, 1, sq80_31_a_data + 4 },  /* 'formt3'  9 */
        {  87, 1, sq80_32_a_data + 4 },  /* 'formt3' 10 */
        {  95, 1, sq80_33_b_data + 4 },  /* 'formt3' 11 */
        { 103, 1, sq80_33_c_data + 4 },  /* 'formt3' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'formt3' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'formt3' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Voice|Formant 4",  /* SQ-80 waveform 22 'formt4' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  39, 1, sq80_2b_5_data + 4 },  /* 'formt4'  0-4 */
        {  47, 1, sq80_2c_6_data + 4 },  /* 'formt4'  5 */
        {  55, 1, sq80_2d_7_data + 4 },  /* 'formt4'  6 */
        {  63, 1, sq80_2e_8_data + 4 },  /* 'formt4'  7 */
        {  71, 1, sq80_2f_9_data + 4 },  /* 'formt4'  8 */
        {  79, 1, sq80_30_a_data + 4 },  /* 'formt4'  9 */
        {  87, 1, sq80_31_a_data + 4 },  /* 'formt4' 10 */
        {  95, 1, sq80_32_b_data + 4 },  /* 'formt4' 11 */
        { 103, 1, sq80_33_c_data + 4 },  /* 'formt4' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'formt4' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'formt4' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Voice|Formant 5",  /* SQ-80 waveform 23 'formt5' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  47, 1, sq80_2b_5_data + 4 },  /* 'formt5'  0-5 */
        {  55, 1, sq80_2c_6_data + 4 },  /* 'formt5'  6 */
        {  63, 1, sq80_2d_7_data + 4 },  /* 'formt5'  7 */
        {  71, 1, sq80_2e_8_data + 4 },  /* 'formt5'  8 */
        {  79, 1, sq80_2f_9_data + 4 },  /* 'formt5'  9 */
        {  87, 1, sq80_30_a_data + 4 },  /* 'formt5' 10 */
        {  95, 1, sq80_31_b_data + 4 },  /* 'formt5' 11 */
        { 103, 1, sq80_32_c_data + 4 },  /* 'formt5' 12 */
        { 111, 1, sq80_18_d_data + 4 },  /* 'formt5' 13 */
        { 119, 1, sq80_16_e_data + 4 },  /* 'formt5' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Voice|Voice 1",  /* SQ-80 waveform 11 'voice1' */
    /* 25 25 25 25 25 3a 26 3b 3c 3d 3e 3e 17 17 17 15 */
    /* 05 05 05 05 05 06 07 07 08 09 0a 0b 0e 0e 0e 0f */
#ifdef Y_GUI
      3
#endif
#ifdef Y_PLUGIN
      {
        {  39, 1, sq80_25_5_data + 4 },  /* 'voice1'  0-4 */
        {  47, 1, sq80_3a_6_data + 4 },  /* 'voice1'  5 */
        {  55, 1, sq80_26_7_data + 4 },  /* 'voice1'  6 */
        {  63, 1, sq80_3b_7_data + 4 },  /* 'voice1'  7 */
        {  71, 1, sq80_3c_8_data + 4 },  /* 'voice1'  8 */
        {  79, 1, sq80_3d_9_data + 4 },  /* 'voice1'  9 */
        {  87, 1, sq80_3e_a_data + 4 },  /* 'voice1' 10 */
        {  95, 1, sq80_3e_b_data + 4 },  /* 'voice1' 11 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'voice1' 12-14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Noise|Grit 1",  /* SQ-80 waveform 36 'grit_1' */
#ifdef Y_GUI
      1
#endif
#ifdef Y_PLUGIN
      {
        {  39, 1, sq80_6e_5_data + 4 },  /* 'grit_1'  0-4 */
        {  47, 1, sq80_6f_6_data + 4 },  /* 'grit_1'  5 */
        {  55, 1, sq80_70_7_data + 4 },  /* 'grit_1'  6 */
        {  63, 1, sq80_71_9_data + 4 },  /* 'grit_1'  7 */
        {  71, 1, sq80_72_9_data + 4 },  /* 'grit_1'  8 */
        {  79, 1, sq80_73_b_data + 4 },  /* 'grit_1'  9 */
        {  87, 1, sq80_74_c_data + 4 },  /* 'grit_1' 10 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'grit_1' 11-13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'grit_1' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },
    { "Noise|Grit 3",  /* SQ-80 waveform 38 'grit_3' */
#ifdef Y_GUI
      3
#endif
#ifdef Y_PLUGIN
      {
        {  39, 1, sq80_6d_4_data + 4 },  /* 'grit_3'  0-4 */
        {  47, 1, sq80_6d_5_data + 4 },  /* 'grit_3'  5 */
        {  55, 1, sq80_6e_6_data + 4 },  /* 'grit_3'  6 */
        {  63, 1, sq80_6f_7_data + 4 },  /* 'grit_3'  7 */
        {  70, 1, sq80_70_7_data + 4 },  /* 'grit_3'  7+ */
        {  79, 1, sq80_71_9_data + 4 },  /* 'grit_3'  9 */
        {  87, 1, sq80_72_a_data + 4 },  /* 'grit_3' 10 */
        {  95, 1, sq80_73_b_data + 4 },  /* 'grit_3' 11 */
        { 103, 1, sq80_74_c_data + 4 },  /* 'grit_3' 12 */
        { 111, 1, sq80_1a_d_data + 4 },  /* 'grit_3' 13 */
        { 119, 1, sq80_17_e_data + 4 },  /* 'grit_3' 14 */
        { 256, 1, yw_sin_1_data + 4 },   /* end-of-waves marker */
      }
#endif
    },

    { (char *)0 } /* end-of-wavetable marker */
};

