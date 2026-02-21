/* sdr.c — RTL-SDR cihaz soyutlama katmanı (asenkron okuma mimarisi) */
#include "sdr.h"
#include <stdio.h>
#include <string.h>

int sdr_open(SdrDevice *s) {
    s->center_freq   = SDR_DEFAULT_FREQ;
    s->sample_rate   = SDR_DEFAULT_SR;
    s->agc_on        = 1;
    s->gain_db       = 0.0f;
    s->dev           = NULL;
    /* Async alanlarını sıfırla */
    s->async_thread  = NULL;
    s->async_running = 0;
    s->disp_fresh    = 0;
    s->data_cb       = NULL;
    s->data_cb_ud    = NULL;

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

/* ── Asenkron okuma ─────────────────────────────────────────── */

/*
 * sdr_async_cb — librtlsdr'ın async thread'inden her blok geldiğinde
 * çağrılır. İki iş yapar:
 *   1. Kaydediciyi besler: data_cb her blok için çağrılır → veri KAYBI YOK.
 *   2. GUI görüntüleme tamponunu günceller: GUI kendi VSYNC hızında bu
 *      tamponu okur; aradaki blokları doğal olarak atlar (istenen davranış).
 */
static void sdr_async_cb(unsigned char *buf, uint32_t len, void *ctx) {
    SdrDevice *s = (SdrDevice *)ctx;

    if (!s->async_running) {
        rtlsdr_cancel_async(s->dev);
        return;
    }

    uint32_t block = FFT_SIZE * 2;
    if (len < block) return;

    /* 1. Kaydedici callback'i — her blok, veri kaybı olmadan */
    if (s->data_cb)
        s->data_cb(buf, block, s->data_cb_ud);

    /* 2. GUI görüntüleme tamponu — en son bloğu sakla */
    EnterCriticalSection(&s->disp_cs);
    memcpy(s->disp_buf, buf, block);
    s->disp_fresh = 1;
    LeaveCriticalSection(&s->disp_cs);
}

/* rtlsdr_read_async bloklayıcı bir çağrıdır; kendi thread'inde çalışır. */
static DWORD WINAPI sdr_async_thread_fn(LPVOID arg) {
    SdrDevice *s = (SdrDevice *)arg;
    /*
     * 3. ve 4. parametre (buf_num=0, buf_len=FFT_SIZE*2):
     *   buf_num=0  → kütüphane varsayılan tampon sayısını kullanır (15).
     *   buf_len    → her callback çağrısında gelecek bayt sayısı.
     * rtlsdr_cancel_async() çağrılana veya async_running=0 olana dek
     * bu fonksiyon geri dönmez.
     */
    rtlsdr_read_async(s->dev, sdr_async_cb, s, 0, FFT_SIZE * 2);
    return 0;
}

void sdr_start_async(SdrDevice *s, SdrDataCb cb, void *userdata) {
    s->data_cb       = cb;
    s->data_cb_ud    = userdata;
    s->async_running = 1;
    s->disp_fresh    = 0;
    InitializeCriticalSection(&s->disp_cs);
    s->async_thread  = CreateThread(NULL, 0, sdr_async_thread_fn, s, 0, NULL);
    printf("[SDR] Asenkron okuma basladi.\n");
}

void sdr_stop_async(SdrDevice *s) {
    if (!s->async_running) return;
    s->async_running = 0;
    rtlsdr_cancel_async(s->dev);
    if (s->async_thread) {
        WaitForSingleObject(s->async_thread, 5000);
        CloseHandle(s->async_thread);
        s->async_thread = NULL;
    }
    DeleteCriticalSection(&s->disp_cs);
    printf("[SDR] Asenkron okuma durduruldu.\n");
}

int sdr_pop_block(SdrDevice *s, uint8_t *out) {
    int got = 0;
    EnterCriticalSection(&s->disp_cs);
    if (s->disp_fresh) {
        memcpy(out, s->disp_buf, FFT_SIZE * 2);
        s->disp_fresh = 0;
        got = 1;
    }
    LeaveCriticalSection(&s->disp_cs);
    return got;
}