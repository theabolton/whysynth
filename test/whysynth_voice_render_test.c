#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "src/whysynth_voice_render.c"

#define SEGMENT_LENGTH 2750
#define FREQ_STEPS 10
#define RES_STEPS 20
#define DATA_SIZE (SEGMENT_LENGTH * FREQ_STEPS * RES_STEPS * 2)
#define WAV_HEADER  { \
0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,  0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, \
0x44, 0xac, 0x00, 0x00, 0x88, 0x58, 0x01, 0x00,  0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61 \
}
#define WAV_HEADER_SIZE 32
#define WAV_FILESIZE (10 + WAV_HEADER_SIZE + DATA_SIZE)

void test_filter(float freq, float key_mod, float res, FILE *output)
{
    int i;
    float fzero = 0.0f;

    struct vvcf  vcf_state = {0, 0, 0.0, 0.0, 0.0, 0.0, 0.0};
    y_voice_t   *voice     = malloc(sizeof(y_voice_t));
    y_svcf_t    *param     = malloc(sizeof(y_svcf_t));

    param->frequency = &key_mod;
    param->qres = &res;

    param->mode = &fzero;
    param->source = &fzero;
    param->freq_mod_src = &fzero;
    param->freq_mod_amt = &fzero;
    param->mparam = &fzero;

    float in[SEGMENT_LENGTH]; 

    // TODO only call this for automated tests to prevent audible patterns forming
    // when verifying the output by ear
    //srand(0);

    for (i = 0; i<SEGMENT_LENGTH; i++)
        in[i] = ((float)random() / (float)RAND_MAX) - 0.5f;

    float out[SEGMENT_LENGTH] = {0};

    vcf_2pole(SEGMENT_LENGTH, param, voice, &vcf_state, freq, (float *)&in, (float *)&out);

    int16_t audio[SEGMENT_LENGTH];
    for (i = 0; i < SEGMENT_LENGTH; i++)
        audio[i] = (int16_t)roundf(out[i] * 30000 );

    fwrite(audio, 2, SEGMENT_LENGTH, output);


    // TODO check return values
}

int main(void) 
{
    // freq: 0-50 from GUI
    // svcf->frequency: ~0.001-0.05 from note value (middle C ~ 0.0055)
    // res: 0-1

    FILE *output = fopen("filter_sweep_test.wav","wb");

    int riff_size = WAV_FILESIZE - 8;
    fwrite("RIFF", 1, 4, output);
    fwrite(&riff_size, 4, 1, output);

    uint8_t header[] = WAV_HEADER;
    fwrite(&header, 1, 32, output);

    uint32_t data_size = DATA_SIZE;
    fwrite(&data_size, 4, 1, output);

    float freq,r;

    for (freq=0; freq < FREQ_STEPS; freq++)
        for (r=0; r < 1; r += (1.0/RES_STEPS))
            test_filter(freq, 0.0055, r, output);

    fclose(output);

    return 0;
}
