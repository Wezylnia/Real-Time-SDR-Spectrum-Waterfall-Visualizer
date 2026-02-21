/*
 * main.c — RTL-SDR Radar Kontrol Paneli
 *
 * Tüm modülleri bir araya getirir:
 *   fft       → Hann penceresi + Cooley-Tukey FFT + PSD
 *   sdr       → RTL-SDR cihaz soyutlama
 *   recorder  → Arka plan IQ kayıt (Windows thread)
 *   render    → SDL2 çizim katmanı + SDL_ttf
 *   widgets   → Slider / Button / TextInput
 *   panel     → Kontrol paneli düzeni + olay işleme
 *
 * Derleme (MSYS2 MinGW64 terminali):
 *   cd /c/RtlSdr/radar
 *   make
 *
 * Klavye kısayolları:
 *   ← →   ±1 MHz     ↑ ↓   ±100 kHz     ESC  Çıkış
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fft.h"
#include "sdr.h"
#include "recorder.h"
#include "render.h"
#include "widgets.h"
#include "panel.h"

/* Şelale tamponu: [satır][fft_bin] */
static float s_waterfall[WATERFALL_ROWS][FFT_SIZE];
static float s_psd[FFT_SIZE];

static void waterfall_push(const float *psd) {
    memmove(s_waterfall[0], s_waterfall[1],
            sizeof(float) * FFT_SIZE * (WATERFALL_ROWS - 1));
    memcpy(s_waterfall[WATERFALL_ROWS - 1], psd, sizeof(float) * FFT_SIZE);
}

/* ── main ─────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    /* ── 1. FFT başlat ─────────────────────────────────────── */
    fft_init();
    memset(s_waterfall, 0, sizeof(s_waterfall));

    /* ── 2. SDR aç ─────────────────────────────────────────── */
    SdrDevice sdr;
    if (sdr_open(&sdr) != 0) return 1;

    /* ── 3. Kaydedici başlat ───────────────────────────────── */
    RecorderState rec;
    recorder_init(&rec);

    /* ── 4. SDL2 başlat ────────────────────────────────────── */
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *win = SDL_CreateWindow(
        "RTL-SDR Radar Kontrol Paneli  |  ← → ±1MHz  ↑ ↓ ±100kHz  ESC Çıkış",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    if (!win) {
        fprintf(stderr, "Pencere olusturulamadi: %s\n", SDL_GetError());
        sdr_close(&sdr);
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *sdl_ren = SDL_CreateRenderer(
        win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!sdl_ren) {
        fprintf(stderr, "Renderer olusturulamadi: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        sdr_close(&sdr);
        SDL_Quit();
        return 1;
    }

    /* DPI’dan bağımsız sanal canvas — pencere büyüklüğü ne olursa olsun
       tampon her zaman WIN_W x WIN_H piksel gibi çalgışır. */
    SDL_RenderSetLogicalSize(sdl_ren, WIN_W, WIN_H);

    /* ── 5. Render bağlamı ─────────────────────────────────── */
    RenderCtx ctx;
    render_init(&ctx, sdl_ren);

    /* ── 6. Kontrol paneli ─────────────────────────────────── */
    Panel panel;
    panel_init(&panel);

    /* db_min / db_max başlangıç değerlerini slider'larla senkronize et */
    ctx.db_min = panel.sl_dbmin.val;
    ctx.db_max = panel.sl_dbmax.val;

    /* ── 7. Ham veri tamponu ───────────────────────────────── */
    uint8_t *raw = (uint8_t *)malloc(FFT_SIZE * 2);
    if (!raw) {
        fprintf(stderr, "Bellek hatasi\n");
        return 1;
    }

    /* ── 8. Ana döngü ──────────────────────────────────────── */
    int running = 1;
    while (running) {

        /* Olayları işle */
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { running = 0; break; }
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.sym == SDLK_ESCAPE) { running = 0; break; }

            panel_handle_event(&panel, &ev, &sdr, &rec, &ctx);
        }
        if (!running) break;

        /* RTL-SDR'dan bir blok oku */
        if (sdr_read_block(&sdr, raw) == 0) {
            recorder_push(&rec, raw);
            fft_compute_psd(raw, s_psd);
            waterfall_push(s_psd);
        }

        /* Çiz */
        render_clear(&ctx);

        float fc_mhz = sdr.center_freq / 1e6f;
        float bw_mhz = sdr.sample_rate  / 1e6f;
        render_grid     (&ctx, fc_mhz, bw_mhz);
        render_spectrum (&ctx, s_psd);
        render_waterfall(&ctx, (const float (*)[FFT_SIZE])s_waterfall);
        panel_draw      (&ctx, &panel, &sdr, &rec);

        render_present(&ctx);
    }

    /* ── 9. Temizlik ───────────────────────────────────────── */
    if (rec.active) recorder_stop(&rec);
    free(raw);
    render_free(&ctx);
    SDL_DestroyRenderer(sdl_ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    sdr_close(&sdr);

    printf("Program kapatildi.\n");
    return 0;
}