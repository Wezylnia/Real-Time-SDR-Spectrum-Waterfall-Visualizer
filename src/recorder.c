    /* recorder.c — Arka plan IQ kayıt sistemi */
#include "recorder.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define REC_RING_SIZE 64
#define REC_BLOCK     (FFT_SIZE * 2)

/* ── Ring buffer ──────────────────────────────────────────── */
static uint8_t  s_ring[REC_RING_SIZE][REC_BLOCK];
static volatile int s_wi    = 0;   /* yazma indeksi */
static volatile int s_ri    = 0;   /* okuma indeksi */
static volatile int s_alive = 0;   /* thread çalışıyor mu */
static FILE    *s_fp        = NULL;
static HANDLE   s_thread    = NULL;
static CRITICAL_SECTION s_cs;

/* ── Kayıt iş parçacığı ───────────────────────────────────── */
static DWORD WINAPI rec_thread(LPVOID _arg) {
    (void)_arg;
    while (s_alive || s_ri != s_wi) {
        EnterCriticalSection(&s_cs);
        int has_data = (s_ri != s_wi);
        LeaveCriticalSection(&s_cs);

        if (has_data && s_fp) {
            fwrite(s_ring[s_ri % REC_RING_SIZE], 1, REC_BLOCK, s_fp);
            EnterCriticalSection(&s_cs);
            s_ri++;
            LeaveCriticalSection(&s_cs);
        } else {
            Sleep(1);
        }
    }
    if (s_fp) { fclose(s_fp); s_fp = NULL; }
    return 0;
}

/* ── Genel API ────────────────────────────────────────────── */
void recorder_init(RecorderState *r) {
    memset(r, 0, sizeof(*r));
    InitializeCriticalSection(&s_cs);
}

void recorder_start(RecorderState *r) {
    if (r->active) return;

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    snprintf(r->filepath, sizeof(r->filepath),
        "C:\\RtlSdr\\iq_%04d%02d%02d_%02d%02d%02d.bin",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);

    s_fp = fopen(r->filepath, "wb");
    if (!s_fp) {
        fprintf(stderr, "[REC] Dosya acilamadi: %s\n", r->filepath);
        return;
    }

    s_ri = s_wi = 0;
    s_alive = 1;
    r->active = 1;
    s_thread = CreateThread(NULL, 0, rec_thread, NULL, 0, NULL);
    printf("[REC] Kayit basladi: %s\n", r->filepath);
}

void recorder_stop(RecorderState *r) {
    if (!r->active) return;
    s_alive   = 0;
    r->active = 0;
    if (s_thread) {
        WaitForSingleObject(s_thread, 8000);
        CloseHandle(s_thread);
        s_thread = NULL;
    }
    printf("[REC] Kayit durduruldu: %s\n", r->filepath);
}

void recorder_push(RecorderState *r, const uint8_t *raw) {
    if (!r->active) return;
    EnterCriticalSection(&s_cs);
    if (s_wi - s_ri < REC_RING_SIZE) {
        memcpy(s_ring[s_wi % REC_RING_SIZE], raw, REC_BLOCK);
        s_wi++;
    }
    LeaveCriticalSection(&s_cs);
}