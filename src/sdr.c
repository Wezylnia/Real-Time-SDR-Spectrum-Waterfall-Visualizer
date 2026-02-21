/* sdr.c — RTL-SDR cihaz soyutlama katmanı */
#include "sdr.h"
#include <stdio.h>
#include <string.h>

int sdr_open(SdrDevice *s) {
    s->center_freq = SDR_DEFAULT_FREQ;
    s->sample_rate = SDR_DEFAULT_SR;
    s->agc_on      = 1;
    s->gain_db     = 0.0f;
    s->dev         = NULL;

    int count = rtlsdr_get_device_count();
    if (count == 0) {
        fprintf(stderr, "[SDR] Hata: RTL-SDR cihazi bulunamadi!\n");
        return -1;
    }
    printf("[SDR] Cihaz: %s\n", rtlsdr_get_device_name(0));

    if (rtlsdr_open(&s->dev, 0) < 0) {
        fprintf(stderr, "[SDR] Hata: Cihaz acilamadi.\n");
        return -1;
    }

    rtlsdr_set_sample_rate  (s->dev, s->sample_rate);
    rtlsdr_set_center_freq  (s->dev, s->center_freq);
    rtlsdr_set_tuner_gain_mode(s->dev, 0);   /* AGC açık */
    rtlsdr_reset_buffer     (s->dev);

    printf("[SDR] Baslangic frekans : %.3f MHz\n", s->center_freq / 1e6);
    printf("[SDR] Ornekleme hizi    : %.3f MHz\n", s->sample_rate / 1e6);
    return 0;
}

void sdr_close(SdrDevice *s) {
    if (s->dev) {
        rtlsdr_close(s->dev);
        s->dev = NULL;
    }
}

int sdr_read_block(SdrDevice *s, uint8_t *raw_out) {
    int n_read = 0;
    int ret = rtlsdr_read_sync(s->dev, raw_out, FFT_SIZE * 2, &n_read);
    if (ret < 0 || n_read < FFT_SIZE * 2) return -1;
    return 0;
}

void sdr_set_freq(SdrDevice *s, uint32_t hz) {
    if (hz < 24000000u)    hz = 24000000u;
    if (hz > 1766000000u)  hz = 1766000000u;
    s->center_freq = hz;
    rtlsdr_set_center_freq(s->dev, hz);
}

void sdr_set_sr(SdrDevice *s, uint32_t sr) {
    s->sample_rate = sr;
    rtlsdr_set_sample_rate(s->dev, sr);
    rtlsdr_reset_buffer(s->dev);
}

void sdr_set_agc(SdrDevice *s, int on) {
    s->agc_on = on;
    rtlsdr_set_tuner_gain_mode(s->dev, on ? 0 : 1);
    if (!on)
        rtlsdr_set_tuner_gain(s->dev, (int)(s->gain_db * 10.0f));
}

void sdr_set_gain(SdrDevice *s, float db) {
    s->gain_db = db;
    if (!s->agc_on)
        rtlsdr_set_tuner_gain(s->dev, (int)(db * 10.0f));
}

void sdr_shift_freq(SdrDevice *s, int32_t delta_hz) {
    int64_t f = (int64_t)s->center_freq + delta_hz;
    if (f < 24000000LL)    f = 24000000LL;
    if (f > 1766000000LL)  f = 1766000000LL;
    sdr_set_freq(s, (uint32_t)f);
}