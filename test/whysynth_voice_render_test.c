#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "src/whysynth_voice_render.c"

// change SEGMENT_LENGTH to a large vaule (3000+) when you want to 
// check the filter output by ear
#define SEGMENT_LENGTH 128

#define SWEEP_STEPS 10

// These defines give us a header for a 44.1kHz 16 bit mono WAV file

#define WAV_HEADER  { \
  0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,  0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, \
  0x44, 0xac, 0x00, 0x00, 0x88, 0x58, 0x01, 0x00,  0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61 \
}
#define WAV_HEADER_SIZE 32

typedef void (*filter_func_t)(unsigned long, y_svcf_t *, y_voice_t *, struct vvcf *, float , float *, float *);

// Parameter ranges:
//
// freq: 0-50 from GUI
// key_mod: ~0.001-0.05 from note value (middle C ~ 0.0055)
// res: 0-1
void generate_segment(filter_func_t filter, float freq, float key_mod, float drive, float res, FILE *output)
{
    int i;
    float fzero = 0.0f;

    struct vvcf  vcf_state = {0, 0, 0.0, 0.0, 0.0, 0.0, 0.0};
    y_voice_t   *voice     = malloc(sizeof(y_voice_t));
    y_svcf_t    *param     = malloc(sizeof(y_svcf_t));

    assert(voice != NULL && param != NULL);

    param->frequency = &key_mod;
    param->qres = &res;

    param->mode = &fzero;
    param->source = &fzero;
    param->freq_mod_src = &fzero;
    param->freq_mod_amt = &fzero;
    param->mparam = &drive;

    float in[SEGMENT_LENGTH], out[SEGMENT_LENGTH];

    for (i = 0; i < SEGMENT_LENGTH; i++)
    {
        in[i] = ((float)random() / (float)RAND_MAX) - 0.5f;
        out[i] = 0;
    }

    filter(SEGMENT_LENGTH, param, voice, &vcf_state, freq, (float *)&in, (float *)&out);

    int16_t audio[SEGMENT_LENGTH];

    for (i = 0; i < SEGMENT_LENGTH; i++)
        audio[i] = (int16_t)roundf(out[i] * 17000 );

    assert(fwrite(audio, 2, SEGMENT_LENGTH, output) == SEGMENT_LENGTH);
}

FILE *create_wav(uint32_t data_size, char *filename)
{
    FILE *output = fopen(filename,"wb");

    int riff_size = WAV_HEADER_SIZE + data_size + 2;
    fwrite("RIFF", 1, 4, output);
    fwrite(&riff_size, 4, 1, output);

    uint8_t header[] = WAV_HEADER;
    fwrite(&header, 1, 32, output);

    fwrite(&data_size, 4, 1, output);

    return output;
}

static void sweep(filter_func_t filter, float drive, FILE *output)
{
    float freq, r;

    for (r=0; r < 0.9; r += (1.0/SWEEP_STEPS))
        for (freq=0; freq < SWEEP_STEPS; freq++)
            generate_segment(filter, freq, 0.0055, drive, r, output);
}

void sweep_filter(filter_func_t filter, bool sweep_drive, char *filename)
{
    srand(0);

    uint32_t data_size = SEGMENT_LENGTH * SWEEP_STEPS * SWEEP_STEPS * 2;

    if (sweep_drive)
        data_size *= SWEEP_STEPS;

    FILE *output = create_wav(data_size, filename);

    float drive = 0;

    // generate test segments by sweeping the frequency, resonance and optionally drive.
    if (sweep_drive)
        for (drive=0; drive < 1.0; drive += (1.0/SWEEP_STEPS))
            sweep(filter, drive, output);
    else
        sweep(filter, drive, output);


    fclose(output);
}

int main(void) 
{
    sweep_filter(&vcf_2pole,     false, "vcf_2pole_sweep.wav");
    sweep_filter(&vcf_4pole,     false, "vcf_4pole_sweep.wav");
    sweep_filter(&vcf_mvclpf,    false, "vcf_mvclpf_sweep.wav");
    sweep_filter(&vcf_clip4pole, true,  "vcf_clip4pole_sweep.wav");
    sweep_filter(&vcf_bandpass,  false, "vcf_bandpass_sweep.wav");
    sweep_filter(&vcf_amsynth,   false, "vcf_amsynth_sweep.wav");

    return 0;
}
