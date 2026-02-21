#pragma once
/* recorder.h — Arka plan IQ kayıt sistemi (Windows thread + ring buffer)
 *
 * Kayıt formatı: ham uint8_t IQ çiftleri (RTL-SDR natif)
 * Python'da okumak:
 *   data = np.fromfile("iq_YYYYMMDD_HHMMSS.bin", dtype=np.uint8)
 *   iq   = (data[0::2] - 127.5) / 128 + 1j * (data[1::2] - 127.5) / 128
 */

#include <stdint.h>
#include "fft.h"   /* FFT_SIZE için */

typedef struct {
    int  active;             /* 1 = kayıt devam ediyor */
    char filepath[256];      /* Son/aktif dosya yolu */
} RecorderState;

/* recorder_init: yapıyı sıfırla (program başında bir kez) */
void recorder_init(RecorderState *r);

/* recorder_start: yeni dosya aç, arka plan thread'ini başlat */
void recorder_start(RecorderState *r);

/* recorder_stop: thread'i durdur, dosyayı kapat */
void recorder_stop(RecorderState *r);

/* recorder_push: ana döngüden çağrılır; blok ring buffer'a yazılır */
void recorder_push(RecorderState *r, const uint8_t *raw);