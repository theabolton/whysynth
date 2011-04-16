#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>

#include "src/whysynth_voice_render.c"

#define SEGMENT_LENGTH 2750

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

    // play this output with: play -r 44100 -ts16 output
    fwrite(audio, 2, SEGMENT_LENGTH, output);


    // check return values
}

int main(void) 
{
    // freq: 0-50 from GUI
    // svcf->frequency: ~0.001-0.05 from note value (middle C ~ 0.0055)
    // res: 0-1

    FILE *output = fopen("output","wb");

    float i,r;

    for (i=0; i<10; i++)
        for (r=0; r<1; r += 0.05)
            test_filter(i, 0.0055, r, output);

    fclose(output);

    return 0;
}
