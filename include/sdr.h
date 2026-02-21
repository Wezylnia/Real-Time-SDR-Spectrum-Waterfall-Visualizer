#pragma once
/* sdr.h — RTL-SDR cihaz soyutlama katmanı */

#include <stdint.h>
#include <rtl-sdr.h>
#include "fft.h"   /* FFT_SIZE için */

#define SDR_DEFAULT_FREQ   100000000u   /* 100 MHz */
#define SDR_DEFAULT_SR     2048000u     /* 2.048 MS/s */

typedef struct {
    rtlsdr_dev_t *dev;
    uint32_t      center_freq;
    uint32_t      sample_rate;
    int           agc_on;       /* 1 = AGC, 0 = manuel */
    float         gain_db;      /* Manuel kazanç (0–49.6 dB) */
} SdrDevice;

/* Cihazı aç ve varsayılan ayarları uygula.
   Başarılıysa 0, hata varsa -1 döner. */
int  sdr_open  (SdrDevice *s);
void sdr_close (SdrDevice *s);

/* FFT_SIZE*2 bayt okur, raw_out'a yazar.
   Başarılıysa 0, hata varsa -1. */
int  sdr_read_block(SdrDevice *s, uint8_t *raw_out);

void sdr_set_freq   (SdrDevice *s, uint32_t hz);
void sdr_set_sr     (SdrDevice *s, uint32_t sr);
void sdr_set_agc    (SdrDevice *s, int on);
void sdr_set_gain   (SdrDevice *s, float db);
void sdr_shift_freq (SdrDevice *s, int32_t delta_hz);