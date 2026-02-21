#pragma once
/* sdr.h — RTL-SDR cihaz soyutlama katmanı (asenkron okuma mimarisi) */

#include <stdint.h>
#include <rtl-sdr.h>
#include <windows.h>
#include "fft.h"   /* FFT_SIZE için */

#define SDR_DEFAULT_FREQ   100000000u   /* 100 MHz */
#define SDR_DEFAULT_SR     2048000u     /* 2.048 MS/s */

/*
 * Asenkron veri callback'i: rtlsdr_read_async thread'inden her I/Q bloğu
 * geldiğinde çağrılır. Kaydedici gibi TÜM bloklara erişmesi gereken
 * bileşenler buraya bağlanır. GUI bu yolu kullanmaz; GUI sdr_pop_block()
 * ile en son bloğu alır.
 */
typedef void (*SdrDataCb)(const uint8_t *buf, uint32_t len, void *userdata);

typedef struct {
    rtlsdr_dev_t *dev;
    uint32_t      center_freq;
    uint32_t      sample_rate;
    int           agc_on;       /* 1 = AGC, 0 = manuel */
    float         gain_db;      /* Manuel kazanç (0–49.6 dB) */

    /* ── Asenkron okuma alanları ──────────────────────────────── */
    HANDLE           async_thread;   /* rtlsdr_read_async'i çalıştıran thread */
    volatile int     async_running;  /* 0 yapılırsa thread durur */

    /*
     * GUI görüntüleme çift tamponu:
     *   - Async thread her blokta disp_buf'u günceller ve disp_fresh=1 yapar.
     *   - GUI thread sdr_pop_block() ile bunu okur ve disp_fresh=0 sıfırlar.
     *   - VSYNC hızındaki GUI kaçırdığı blokları sorunsuz atlar.
     */
    uint8_t          disp_buf[FFT_SIZE * 2];
    volatile int     disp_fresh;
    CRITICAL_SECTION disp_cs;

    /* Tüm blokları görmesi gereken bileşen için callback (kaydedici) */
    SdrDataCb  data_cb;
    void      *data_cb_ud;
} SdrDevice;

/* Cihazı aç ve varsayılan ayarları uygula. Başarılıysa 0, hata varsa -1. */
int  sdr_open  (SdrDevice *s);
void sdr_close (SdrDevice *s);

/*
 * Asenkron okumayı başlat:
 *   cb        — Her blokta çağrılacak işlev (kaydediciye bağla).
 *   userdata  — cb'ye iletilecek bağlam işaretçisi.
 * Bu çağrıdan sonra rtlsdr_read_async arka planda sürekli çalışır;
 * cihaz tam bant genişliğinde veri yollar, hiçbir blok kaçmaz.
 */
void sdr_start_async(SdrDevice *s, SdrDataCb cb, void *userdata);

/* Asenkron okumayı durdur ve thread'i bekle. */
void sdr_stop_async(SdrDevice *s);

/*
 * GUI döngüsü için: async thread yeni bir blok yazdıysa out'a kopyalar
 * ve 1 döner. Henüz yeni blok yoksa (GUI async'ten daha hızlıysa) 0 döner
 * ve GUI bu kareyi çizmeyebilir.
 */
int  sdr_pop_block(SdrDevice *s, uint8_t *out);

void sdr_set_freq   (SdrDevice *s, uint32_t hz);
void sdr_set_sr     (SdrDevice *s, uint32_t sr);
void sdr_set_agc    (SdrDevice *s, int on);
void sdr_set_gain   (SdrDevice *s, float db);
void sdr_shift_freq (SdrDevice *s, int32_t delta_hz);