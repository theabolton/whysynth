/* WhySynth DSSI software synthesizer plugin and GUI
 *
 * Copyright (C) 2004-2006, 2010 Sean Bolton and others.
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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <inttypes.h>
#include <locale.h>

#include "whysynth_types.h"
#include "whysynth.h"
#include "whysynth_voice.h"

y_patch_t y_init_voice = {
    "  <-->",
    "",
    /* -PORTS- */
    { 1, 0, 0, 0.0f, 0, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0, 0.0f, 0.5f, 0.5f },  /* osc1 */
    { 0, 0, 0, 0.0f, 0, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0, 0.0f, 0.5f, 0.5f },  /* osc2 */
    { 0, 0, 0, 0.0f, 0, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0, 0.0f, 0.5f, 0.5f },  /* osc3 */
    { 0, 0, 0, 0.0f, 0, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0, 0.0f, 0.5f, 0.5f },  /* osc4 */
    { 1, 0, 50.0f, 0, 0.0f, 0.0f, 0.0f },                                  /* vcf1 */
    { 0, 0, 50.0f, 0, 0.0f, 0.0f, 0.0f },                                  /* vcf2 */
    0.0f, 0.2f, 0.0f, 0.8f, 0.5f, 0.5f, 0.5f, 0.5f,                        /* mix */
    0.5f,                                                                  /* volume */
    0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,                           /* effects */
    0.984375f, 2,                                                          /* glide / bend */
    { 1.0f, 0, 0.0f, 0, 0.0f },                                            /* glfo */
    { 1.0f, 0, 0.0f, 0, 0.0f },                                            /* vlfo */
    { 1.0f, 0, 0.0f, 0, 0.0f }, 90.0f, 0.0f,                               /* mlfo */
    { 1, 3, 0.004, 1, 3, 0.001, 1, 3, 0.001, 1, 3, 0.2, 0, 0, 0, 0, 0 },   /* ego */
    { 0, 3, 0.1, 1, 3, 0.1, 1, 3, 0.1, 1, 3, 0.2, 0, 0, 0, 0, 0 },         /* eg1 */
    { 0, 3, 0.1, 1, 3, 0.1, 1, 3, 0.1, 1, 3, 0.2, 0, 0, 0, 0, 0 },         /* eg2 */
    { 0, 3, 0.1, 1, 3, 0.1, 1, 3, 0.1, 1, 3, 0.2, 0, 0, 0, 0, 0 },         /* eg3 */
    { 0, 3, 0.1, 1, 3, 0.1, 1, 3, 0.1, 1, 3, 0.2, 0, 0, 0, 0, 0 },         /* eg4 */
    1.0f, 0, 0.0f, 0, 0.0f                                                 /* modmix */
};

int
y_data_is_comment(char *buf)  /* line is blank, whitespace, or first non-whitespace character is '#' */
{
    int i = 0;

    while (buf[i]) {
        if (buf[i] == '#') return 1;
        if (buf[i] == '\n') return 1;
        if (buf[i] != ' ' && buf[i] != '\t') return 0;
        i++;
    }
    return 1;
}

void
y_data_parse_text(const char *buf, char *name, int maxlen)
{
    int i = 0, o = 0;
    unsigned int t;

    while (buf[i] && o < maxlen) {
        if (buf[i] < 33 || buf[i] > 126) {
            break;
        } else if (buf[i] == '%') {
            if (buf[i + 1] && buf[i + 2] && sscanf(buf + i + 1, "%2x", &t) == 1) {
                name[o++] = (char)t;
                i += 3;
            } else {
                break;
            }
        } else {
            name[o++] = buf[i++];
        }
    }
    /* trim trailing spaces */
    while (o && name[o - 1] == ' ') o--;
    name[o] = '\0';
}

/* y_sscanf.c - dual-locale sscanf
 *
 * Like the standard sscanf(3), but this version accepts floating point
 * numbers containing either the locale-specified decimal point or the
 * "C" locale decimal point (".").  That said, there are important
 * differences between this and a standard sscanf(3):
 * 
 *  - this function only skips whitespace when there is explicit ' ' in
 *      the format,
 *  - it doesn't return EOF like sscanf(3), just the number of sucessful
 *      conversions,
 *  - it doesn't match character classes,
 *  - there is only one length modifier: 'l', and it only applies to 'd'
 *      (for int64_t) and 'f' (for double),
 *  - '%s' with no field-width specified is an error,
 *  - there are no 'i, 'o', 'p', or 'u' conversions, similarly
 *  - use 'f' instead of 'a', 'A', 'e', 'E', 'F', 'g', or 'G', and
 *  - '%f' doesn't do hexadecimal, infinity or NaN.
 */

static int
_is_whitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}

static int
_is_digit(char c)
{
    return (c >= '0' && c <= '9');
}

static int
_is_hexdigit(char c)
{
    return ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F'));
}

static int y_atof(const char *buffer, double *result); /* forward */

static int
y_vsscanf(const char *buffer, const char *format, va_list ap)
{
    char fc;
    const char *bp = buffer;
    int conversions = 0;

    while ((fc = *format++)) {

        if (_is_whitespace(fc)) { /* skip whitespace */

            while (_is_whitespace(*bp))
                bp++;

        } else if (fc == '%') { /* a conversion */

            int skip = 0;   /* '*' no-store modifier */
            int width = 0;  /* field width */
            int big = 0;    /* storage length modifier */

            if (*format == '*') {
                skip = 1;
                format++;
            }
            while (_is_digit(*format)) {
                width = width * 10 + (*format - '0');
                format++;
            }
            if (*format == 'l') {
                big = 1;
                format++;
            }

            if (*format == '%') {  /* '%' - literal percent character */

                if (*bp == '%')
                    bp++;
                else
                    break;
                format++;

            } else if (*format == 'c') {  /* 'c' - one or more characters */

                int i;
                char *sp = va_arg(ap, char *);

                if (width == 0) width = 1;
                for (i = 0; i < width && *bp != 0; i++, bp++)
                    if (!skip) *sp++ = *bp;
                if (i > 0 && !skip)
                    conversions++;
                format++;

            } else if (*format == 'd') {  /* 'd' - 32 or 64 bit signed decimal integer */

                int negative = 0;
                int i;
                int64_t n = 0;

                if (*bp == '-') {
                    negative = 1;
                    bp++;
                }
                for (i = negative; (width == 0 || i < width) && _is_digit(*bp); i++, bp++)
                    n = n * 10 + (*bp - '0');
                if (i == negative) /* no digits converted */
                    break;
                if (negative) n = -n;
                if (!skip) {
                    if (big)
                        *va_arg(ap, int64_t *) = n;
                    else
                        *va_arg(ap, int32_t *) = n;
                    conversions++;
                }
                format++;

            } else if (*format == 'f') {  /* 'f' - float or double */

                double d;
                int n = y_atof(bp, &d);
                if (n == 0)  /* no digits converted */
                    break;
                if (!skip) {
                    if (big)
                        *va_arg(ap, double *) = d;
                    else
                        *va_arg(ap, float *) = (float)d;
                    conversions++;
                }
                bp += n;
                format++;

            } else if (*format == 'n') {  /* 'n' - store number of characters scanned so far */

                if (!skip) *va_arg(ap, int *) = bp - buffer;
                format++;

            } else if (*format == 's') {  /* 's' - string of non-whitespace characters */

                int i;
                char *sp = va_arg(ap, char *);
                if (width == 0)
                    break; /* must specify a width */
                for (i = 0; i < width && *bp != 0 && !_is_whitespace(*bp); i++, bp++)
                    if (!skip) *sp++ = *bp;
                if (i > 0) {
                    if (!skip) {
                        *sp = 0;
                        conversions++;
                    }
                } else
                    break;  /* conversion failed */
                format++;

            } else if (*format == 'x') {  /* 'x' - 32 or 64 bit signed hexidecimal integer */
                int i;
                int64_t n = 0;

                for (i = 0; (width == 0 || i < width) && _is_hexdigit(*bp); i++, bp++) {
                    n = n * 16;
                    if (*bp >= 'a') n += *bp - 'a' + 10;
                    else if (*bp >= 'A') n += *bp - 'A' + 10;
                    else n += *bp - '0';
                }
                if (i == 0) /* no digits converted */
                    break;
                if (!skip) {
                    if (big)
                        *va_arg(ap, int64_t *) = n;
                    else
                        *va_arg(ap, int32_t *) = n;
                    conversions++;
                }
                format++;

            } else {
                break; /* bad conversion character */
            }

        } else if (fc == *bp) { /* a literal match */

            bp++;

        } else {  /* match fail */
            break;
        }
    }

    return conversions;
}

/* The following function is based on sqlite3AtoF() from sqlite 3.6.18.
 * The sqlite author disclaims copyright to the source code from which
 * this was adapted. */
static int
y_atof(const char *z, double *pResult){
  const char *zBegin = z;
  /* sign * significand * (10 ^ (esign * exponent)) */
  int sign = 1;   /* sign of significand */
  int64_t s = 0;  /* significand */
  int d = 0;      /* adjust exponent for shifting decimal point */
  int esign = 1;  /* sign of exponent */
  int e = 0;      /* exponent */
  double result;
  int nDigits = 0;
  struct lconv *lc = localeconv();
  int dplen = strlen(lc->decimal_point);

  /* skip leading spaces */
  /* while( _is_whitespace(*z) ) z++; */
  /* get sign of significand */
  if( *z=='-' ){
    sign = -1;
    z++;
  }else if( *z=='+' ){
    z++;
  }
  /* skip leading zeroes */
  while( z[0]=='0' ) z++, nDigits++;

  /* copy max significant digits to significand */
  while( _is_digit(*z) && s<((INT64_MAX-9)/10) ){
    s = s*10 + (*z - '0');
    z++, nDigits++;
  }
  /* skip non-significant significand digits
  ** (increase exponent by d to shift decimal left) */
  while( _is_digit(*z) ) z++, nDigits++, d++;

  /* if decimal point is present */
  if( *z=='.' || !strncmp(z, lc->decimal_point, dplen) ) {
    if (*z=='.')
      z++;
    else
      z += dplen;
    /* copy digits from after decimal to significand
    ** (decrease exponent by d to shift decimal right) */
    while( _is_digit(*z) && s<((INT64_MAX-9)/10) ){
      s = s*10 + (*z - '0');
      z++, nDigits++, d--;
    }
    /* skip non-significant digits */
    while( _is_digit(*z) ) z++, nDigits++;
  } else if (nDigits == 0)
      return 0;  /* no significand digits converted */

  /* if exponent is present */
  if( *z=='e' || *z=='E' ){
    int eDigits = 0;
    z++;
    /* get sign of exponent */
    if( *z=='-' ){
      esign = -1;
      z++;
    }else if( *z=='+' ){
      z++;
    }
    /* copy digits to exponent */
    while( _is_digit(*z) ){
      e = e*10 + (*z - '0');
      z++, eDigits++;
    }
    if (eDigits == 0)
        return 0; /* malformed exponent */
  }

  /* adjust exponent by d, and update sign */
  e = (e*esign) + d;
  if( e<0 ) {
    esign = -1;
    e *= -1;
  } else {
    esign = 1;
  }

  /* if 0 significand */
  if( !s ) {
    /* In the IEEE 754 standard, zero is signed.
    ** Add the sign if we've seen at least one digit */
    result = (sign<0 && nDigits) ? -(double)0 : (double)0;
  } else {
    /* attempt to reduce exponent */
    if( esign>0 ){
      while( s<(INT64_MAX/10) && e>0 ) e--,s*=10;
    }else{
      while( !(s%10) && e>0 ) e--,s/=10;
    }

    /* adjust the sign of significand */
    s = sign<0 ? -s : s;

    /* if exponent, scale significand as appropriate
    ** and store in result. */
    if( e ){
      double scale = 1.0;
      /* attempt to handle extremely small/large numbers better */
      if( e>307 && e<342 ){
        while( e%308 ) { scale *= 1.0e+1; e -= 1; }
        if( esign<0 ){
          result = s / scale;
          result /= 1.0e+308;
        }else{
          result = s * scale;
          result *= 1.0e+308;
        }
      }else{
        /* 1.0e+22 is the largest power of 10 than can be 
        ** represented exactly. */
        while( e%22 ) { scale *= 1.0e+1; e -= 1; }
        while( e>0 ) { scale *= 1.0e+22; e -= 22; }
        if( esign<0 ){
          result = s / scale;
        }else{
          result = s * scale;
        }
      }
    } else {
      result = (double)s;
    }
  }

  /* store the result */
  *pResult = result;

  /* return number of characters used */
  return (int)(z - zBegin);
}

int
y_sscanf(const char *str, const char *format, ...)
{
    va_list ap;
    int conversions;

    va_start(ap, format);
    conversions = y_vsscanf(str, format, ap);
    va_end(ap);

    return conversions;
}

/* end of y_sscanf.c */

int
y_data_read_patch(FILE *file, y_patch_t *patch)
{
    int i;
    char c, buf[256], buf2[180];
    y_patch_t tmp;

    do {
        if (!fgets(buf, 256, file)) return 0;
    } while (y_data_is_comment(buf));

    if (sscanf(buf, " WhySynth patch format %d begin", &i) != 1 ||
        i != 0)
        return 0;

    memcpy(&tmp, &y_init_voice, sizeof(y_patch_t));

    while (1) {
        
        if (!fgets(buf, 256, file)) return 0;

        /* 'name %20%20<init%20voice>' */
        if (sscanf(buf, " name %90s", buf2) == 1) {
            
            y_data_parse_text(buf2, tmp.name, 30);
            continue;

        /* 'comment %20%20<init%20voice>' */
        } else if (sscanf(buf, " comment %180s", buf2) == 1) {
            
            y_data_parse_text(buf2, tmp.comment, 60);
            continue;

        /* -PORTS- */
        /* 'oscY 1 1 0 0 0 0 0 0 0.5 0 0 0 0 0.5 0.5' */
        /* 'oscY 2 0 0 0 0 0 0 0 0.5 0 0 0 0 0.5 0.5' */
        /* 'oscY 3 0 0 0 0 0 0 0 0.5 0 0 0 0 0.5 0.5' */
        /* 'oscY 4 0 0 0 0 0 0 0 0.5 0 0 0 0 0.5 0.5' */
        } else if (sscanf(buf, " oscY %d", &i) == 1) {

            struct posc *osc;

            switch (i) {
              case 1: osc = &tmp.osc1; break;
              case 2: osc = &tmp.osc2; break;
              case 3: osc = &tmp.osc3; break;
              case 4: osc = &tmp.osc4; break;
              default:
                return 0;
            }
            if (y_sscanf(buf, " oscY %d %d %d %d %f %d %f %f %f %d %f %d %f %f %f",
                         &i, &osc->mode, &osc->waveform, &osc->pitch,
                         &osc->detune, &osc->pitch_mod_src, &osc->pitch_mod_amt,
                         &osc->mparam1, &osc->mparam2, &osc->mmod_src,
                         &osc->mmod_amt, &osc->amp_mod_src, &osc->amp_mod_amt,
                         &osc->level_a, &osc->level_b) != 15)
                return 0;

            continue;

        /* 'vcfY 1 1 0 50 0 0 0 0' */
        /* 'vcfY 2 0 0 50 0 0 0 0' */
        } else if (sscanf(buf, " vcfY %d", &i) == 1) {

            struct pvcf *vcf;

            switch (i) {
              case 1: vcf = &tmp.vcf1; break;
              case 2: vcf = &tmp.vcf2; break;
              default:
                return 0;
            }
            if (y_sscanf(buf, " vcfY %d %d %d %f %d %f %f %f",
                         &i, &vcf->mode, &vcf->source, &vcf->frequency,
                         &vcf->freq_mod_src, &vcf->freq_mod_amt, &vcf->qres,
                         &vcf->mparam) != 8)
                return 0;

            continue;

        /* 'mix 0 0.2 0 0.8 0.5 0.5 0.5 0.5' */
        } else if (y_sscanf(buf, " mix %f %f %f %f %f %f %f %f",
                            &tmp.busa_level, &tmp.busa_pan,
                            &tmp.busb_level, &tmp.busb_pan,
                            &tmp.vcf1_level, &tmp.vcf1_pan,
                            &tmp.vcf2_level, &tmp.vcf2_pan) == 8) {

            continue;

        /* 'volume 0.5' */
        } else if (y_sscanf(buf, " volume %f", &tmp.volume) == 1) {

            continue;

        /* 'effects 0 0 0 0 0' */
        } else if (y_sscanf(buf, " effects %d %f %f %f %f %f %f %f",
                            &tmp.effect_mode, &tmp.effect_param1,
                            &tmp.effect_param2, &tmp.effect_param3,
                            &tmp.effect_param4, &tmp.effect_param5,
                            &tmp.effect_param6, &tmp.effect_mix) == 8) {

            continue;

        /* 'glide 0.984375' */
        } else if (y_sscanf(buf, " glide %f", &tmp.glide_time) == 1) {

            continue;

        /* 'bend 2' */
        } else if (sscanf(buf, " bend %d", &tmp.bend_range) == 1) {

            continue;

        /* 'lfoY g 1 0 0 0 0' */
        /* 'lfoY v 1 0 0 0 0' */
        /* 'lfoY m 1 0 0 0 0' */
        } else if (sscanf(buf, " lfoY %c", &c) == 1) {

            struct plfo *lfo;

            switch (c) {
              case 'g': lfo = &tmp.glfo; break;
              case 'v': lfo = &tmp.vlfo; break;
              case 'm': lfo = &tmp.mlfo; break;
              default:
                return 0;
            }
            if (y_sscanf(buf, " lfoY %c %f %d %f %d %f",
                         &c, &lfo->frequency, &lfo->waveform, &lfo->delay,
                         &lfo->amp_mod_src, &lfo->amp_mod_amt) != 6)
                return 0;

        /* 'mlfo 90 0' */
        } else if (y_sscanf(buf, " mlfo %f %f", &tmp.mlfo_phase_spread,
                            &tmp.mlfo_random_freq) == 2) {

            continue;

        /* 'egY o 0 0.1 1 0.1 1 0.1 1 0.2 0.1 1 0 0 0 0 0' */
        /* 'egY 1 0 0.1 1 0.1 1 0.1 1 0.2 0.1 1 0 0 0 0 0' */
        /* 'egY 2 0 0.1 1 0.1 1 0.1 1 0.2 0.1 1 0 0 0 0 0' */
        /* 'egY 3 0 0.1 1 0.1 1 0.1 1 0.2 0.1 1 0 0 0 0 0' */
        /* 'egY 4 0 0.1 1 0.1 1 0.1 1 0.2 0.1 1 0 0 0 0 0' */
        } else if (sscanf(buf, " egY %c", &c) == 1) {

            struct peg *eg;

            switch (c) {
              case 'o': eg = &tmp.ego; break;
              case '1': eg = &tmp.eg1; break;
              case '2': eg = &tmp.eg2; break;
              case '3': eg = &tmp.eg3; break;
              case '4': eg = &tmp.eg4; break;
              default:
                return 0;
            }
            if (y_sscanf(buf, " egY %c %d %d %f %f %d %f %f %d %f %f %d %f %f %f %f %d %f",
                         &c, &eg->mode,
                         &eg->shape1, &eg->time1, &eg->level1,
                         &eg->shape2, &eg->time2, &eg->level2,
                         &eg->shape3, &eg->time3, &eg->level3,
                         &eg->shape4, &eg->time4,
                         &eg->vel_level_sens, &eg->vel_time_scale,
                         &eg->kbd_time_scale, &eg->amp_mod_src,
                         &eg->amp_mod_amt) != 18)
                return 0;

        /* 'modmix 1 0 0 0 0' */
        } else if (y_sscanf(buf, " modmix %f %d %f %d %f", &tmp.modmix_bias,
                            &tmp.modmix_mod1_src, &tmp.modmix_mod1_amt,
                            &tmp.modmix_mod2_src, &tmp.modmix_mod2_amt) == 5) {

            continue;

        /* 'WhySynth patch end' */
        } else if (sscanf(buf, " WhySynth patch %3s", buf2) == 1 &&
                 !strcmp(buf2, "end")) {

            break; /* finished */

        } else {

            return 0; /* unrecognized line */
        }
    }

    memcpy(patch, &tmp, sizeof(y_patch_t));

    return 1;  /* -FIX- error handling yet to be implemented */
}

char *
y_data_locate_patch_file(const char *origpath, const char *project_dir)
{
    struct stat statbuf;
    char *path;
    const char *filename;

    if (stat(origpath, &statbuf) == 0)
        return strdup(origpath);
    else if (!project_dir)
        return NULL;
    
    filename = strrchr(origpath, '/');
    
    if (filename) ++filename;
    else filename = origpath;
    if (!*filename) return NULL;
    
    path = (char *)malloc(strlen(project_dir) + strlen(filename) + 2);
    sprintf(path, "%s/%s", project_dir, filename);
    
    if (stat(path, &statbuf) == 0)
        return path;
    
    free(path);
    return NULL;
}

