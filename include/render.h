#pragma once
/* render.h — SDL2 çizim yardımcıları: ızgara, spektrum, şelale */

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "fft.h"

/* ── Sanal kanvas ──────────────────────────────── */
/* SDL_RenderSetLogicalSize ile DPI'dan bağımsız            */
/* PANEL_X=1090 → PANEL_W=350, GRAPH_W=1024              */
/* Yükseklik: WFALL_TOP(400)+WFALL_H(340)=740 < 820      */
#define WIN_W        1440
#define WIN_H         820
#define PANEL_X      1090
#define PANEL_W     (WIN_W - PANEL_X)   /* 350 px */
#define GRAPH_L        58
#define GRAPH_W     (PANEL_X - GRAPH_L - 8)  /* 1024 px */

#define SPEC_TOP       40
#define SPEC_H        320
#define WFALL_TOP   (SPEC_TOP + SPEC_H + 40)  /* 400 */
#define WFALL_H       340
#define WATERFALL_ROWS  55

/* ── Render bağlamı ────────────────────────────────────────── */
typedef struct {
    SDL_Renderer *renderer;
    TTF_Font     *font_sm;   /* 13 px */
    TTF_Font     *font_md;   /* 15 px */
    float         db_min;
    float         db_max;
} RenderCtx;

/* ── Başlatma / Kapatma ────────────────────────────────────── */
int  render_init(RenderCtx *ctx, SDL_Renderer *renderer);
void render_free(RenderCtx *ctx);

/* ── Temel çizim yardımcıları (diğer modüller de kullanır) ─── */
void render_fill_rect    (RenderCtx *ctx, int x, int y, int w, int h, SDL_Color c);
void render_outline_rect (RenderCtx *ctx, int x, int y, int w, int h, SDL_Color c);
void render_line         (RenderCtx *ctx, int x1, int y1, int x2, int y2, SDL_Color c);
void render_text         (RenderCtx *ctx, TTF_Font *f, const char *s, int x, int y, SDL_Color c);
void render_text_center  (RenderCtx *ctx, TTF_Font *f, const char *s, int cx, int y, SDL_Color c);

/* ── Grafik çizimleri ──────────────────────────────────────── */
void render_clear    (RenderCtx *ctx);
void render_grid     (RenderCtx *ctx, float fc_mhz, float bw_mhz);
void render_spectrum (RenderCtx *ctx, const float *psd);
void render_waterfall(RenderCtx *ctx, const float waterfall[WATERFALL_ROWS][FFT_SIZE]);
void render_present  (RenderCtx *ctx);

/* ── Renk haritası ─────────────────────────────────────────── */
SDL_Color render_colormap(float v, float vmin, float vmax);