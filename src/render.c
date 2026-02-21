/* render.c — SDL2 çizim yardımcıları: ızgara, spektrum, şelale */
#include "render.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

/* ── Başlatma ────────────────────────────────────────────────── */
int render_init(RenderCtx *ctx, SDL_Renderer *renderer) {
    ctx->renderer = renderer;
    ctx->db_min   = -60.0f;
    ctx->db_max   =   0.0f;
    ctx->font_sm  = NULL;
    ctx->font_md  = NULL;

    if (TTF_Init() != 0) {
        fprintf(stderr, "[RENDER] TTF_Init hatasi: %s\n", TTF_GetError());
        return -1;
    }

    const char *fonts[] = {
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/tahoma.ttf",
        NULL
    };
    for (int i = 0; fonts[i] && !ctx->font_sm; i++) {
        ctx->font_sm = TTF_OpenFont(fonts[i], 13);
        ctx->font_md = TTF_OpenFont(fonts[i], 15);
    }
    if (!ctx->font_sm)
        fprintf(stderr, "[RENDER] Font yuklenemedi — metin gosterilmeyecek.\n");
    return 0;
}

void render_free(RenderCtx *ctx) {
    if (ctx->font_sm) { TTF_CloseFont(ctx->font_sm); ctx->font_sm = NULL; }
    if (ctx->font_md) { TTF_CloseFont(ctx->font_md); ctx->font_md = NULL; }
    TTF_Quit();
}

/* ── Temel çizim ─────────────────────────────────────────────── */
void render_fill_rect(RenderCtx *ctx, int x, int y, int w, int h, SDL_Color c) {
    SDL_SetRenderDrawColor(ctx->renderer, c.r, c.g, c.b, c.a);
    SDL_Rect r = {x, y, w, h};
    SDL_RenderFillRect(ctx->renderer, &r);
}

void render_outline_rect(RenderCtx *ctx, int x, int y, int w, int h, SDL_Color c) {
    SDL_SetRenderDrawColor(ctx->renderer, c.r, c.g, c.b, c.a);
    SDL_Rect r = {x, y, w, h};
    SDL_RenderDrawRect(ctx->renderer, &r);
}

void render_line(RenderCtx *ctx, int x1, int y1, int x2, int y2, SDL_Color c) {
    SDL_SetRenderDrawColor(ctx->renderer, c.r, c.g, c.b, c.a);
    SDL_RenderDrawLine(ctx->renderer, x1, y1, x2, y2);
}

void render_text(RenderCtx *ctx, TTF_Font *f, const char *s, int x, int y, SDL_Color c) {
    if (!f || !s || !*s) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(f, s, c);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ctx->renderer, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(ctx->renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void render_text_center(RenderCtx *ctx, TTF_Font *f, const char *s, int cx, int y, SDL_Color c) {
    if (!f || !s || !*s) return;
    int tw, th;
    TTF_SizeUTF8(f, s, &tw, &th);
    render_text(ctx, f, s, cx - tw / 2, y, c);
}

/* ── Renk haritası (viridis benzeri) ─────────────────────────── */
SDL_Color render_colormap(float v, float vmin, float vmax) {
    float t = (v - vmin) / (vmax - vmin);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    static const float r5[] = { 0.267f, 0.282f, 0.129f, 0.993f, 0.993f };
    static const float g5[] = { 0.005f, 0.341f, 0.571f, 0.906f, 1.000f };
    static const float b5[] = { 0.329f, 0.930f, 0.557f, 0.144f, 0.900f };

    float seg = t * 4.0f;
    int   idx = (int)seg;
    if (idx >= 4) idx = 3;
    float f = seg - idx;

    return (SDL_Color){
        (Uint8)((r5[idx] + f * (r5[idx+1] - r5[idx])) * 255),
        (Uint8)((g5[idx] + f * (g5[idx+1] - g5[idx])) * 255),
        (Uint8)((b5[idx] + f * (b5[idx+1] - b5[idx])) * 255),
        255
    };
}

/* ── Grafik alanında dB → Y dönüşümü — [SPEC_TOP, SPEC_TOP+SPEC_H] arasına sıkıştırılır ─ */
static int db_to_y(const RenderCtx *ctx, float db) {
    /* dışarı taşmayı önlemek için önce dB değerini sıkıştır */
    if (db > ctx->db_max) db = ctx->db_max;
    if (db < ctx->db_min) db = ctx->db_min;
    float r = (ctx->db_max - db) / (ctx->db_max - ctx->db_min);
    int y = SPEC_TOP + (int)(r * SPEC_H);
    /* ek güvenlik sınırı */
    if (y < SPEC_TOP)          y = SPEC_TOP;
    if (y > SPEC_TOP + SPEC_H) y = SPEC_TOP + SPEC_H;
    return y;
}

/* ── Ekranı temizle ──────────────────────────────────────────── */
void render_clear(RenderCtx *ctx) {
    SDL_SetRenderDrawColor(ctx->renderer, 10, 10, 16, 255);
    SDL_RenderClear(ctx->renderer);
}

/* ── Izgara + eksen etiketleri ───────────────────────────────── */
void render_grid(RenderCtx *ctx, float fc_mhz, float bw_mhz) {
    SDL_Color grid = {42, 42, 55,  255};
    SDL_Color axis = {95, 95, 110, 255};
    SDL_Color lc   = {140,140,150, 255};

    /* Yatay dB ızgarası */
    for (float db = ctx->db_min; db <= ctx->db_max + 0.5f; db += 10.0f) {
        int y = db_to_y(ctx, db);
        if (y < SPEC_TOP || y > SPEC_TOP + SPEC_H) continue;
        SDL_Color gc = (fabsf(db) < 0.5f)
            ? (SDL_Color){190, 50, 50, 255} : grid;
        render_line(ctx, GRAPH_L, y, GRAPH_L + GRAPH_W, y, gc);
        char buf[12]; snprintf(buf, sizeof(buf), "%.0f", db);
        render_text(ctx, ctx->font_sm, buf, 2, y - 6, lc);
    }

    /* Dikey frekans ızgarası */
    for (int i = 0; i <= 8; i++) {
        int x = GRAPH_L + (int)((float)i / 8.0f * GRAPH_W);
        render_line(ctx, x, SPEC_TOP,  x, SPEC_TOP  + SPEC_H,  grid);
        render_line(ctx, x, WFALL_TOP, x, WFALL_TOP + WFALL_H, grid);

        float f = fc_mhz - bw_mhz / 2.0f + ((float)i / 8.0f) * bw_mhz;
        char buf[16]; snprintf(buf, sizeof(buf), "%.2f", f);
        render_text(ctx, ctx->font_sm, buf, x - 16, SPEC_TOP  + SPEC_H  + 4, lc);
        render_text(ctx, ctx->font_sm, buf, x - 16, WFALL_TOP + WFALL_H + 4, lc);
    }

    /* Eksen çerçeveleri */
    render_line(ctx, GRAPH_L,         SPEC_TOP,          GRAPH_L+GRAPH_W, SPEC_TOP,          axis);
    render_line(ctx, GRAPH_L,         SPEC_TOP+SPEC_H,   GRAPH_L+GRAPH_W, SPEC_TOP+SPEC_H,   axis);
    render_line(ctx, GRAPH_L,         SPEC_TOP,          GRAPH_L,         SPEC_TOP+SPEC_H,   axis);
    render_line(ctx, GRAPH_L+GRAPH_W, SPEC_TOP,          GRAPH_L+GRAPH_W, SPEC_TOP+SPEC_H,   axis);

    render_line(ctx, GRAPH_L,         WFALL_TOP,         GRAPH_L+GRAPH_W, WFALL_TOP,         axis);
    render_line(ctx, GRAPH_L,         WFALL_TOP+WFALL_H, GRAPH_L+GRAPH_W, WFALL_TOP+WFALL_H, axis);
    render_line(ctx, GRAPH_L,         WFALL_TOP,         GRAPH_L,         WFALL_TOP+WFALL_H, axis);
    render_line(ctx, GRAPH_L+GRAPH_W, WFALL_TOP,         GRAPH_L+GRAPH_W, WFALL_TOP+WFALL_H, axis);

    /* Başlıklar */
    render_text_center(ctx, ctx->font_md, "Anlık Frekans Spektrumu",
        GRAPH_L + GRAPH_W / 2, SPEC_TOP  - 22, (SDL_Color){170, 200, 255, 255});
    render_text_center(ctx, ctx->font_md, "Canlı Şelale Grafiği",
        GRAPH_L + GRAPH_W / 2, WFALL_TOP - 22, (SDL_Color){170, 200, 255, 255});
}

/* ── Spektrum çizgisi ────────────────────────────────────────── */
void render_spectrum(RenderCtx *ctx, const float *psd) {
    /* db_to_y artık clamp yapıyor; baseline da grafik altı */
    int baseline = SPEC_TOP + SPEC_H;
    int top_clip = SPEC_TOP;

    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);

    /* SDL_RenderSetClipRect ile grafik alanını kilitleyerek taşmayı engelle */
    SDL_Rect clip = { GRAPH_L, SPEC_TOP, GRAPH_W, SPEC_H };
    SDL_RenderSetClipRect(ctx->renderer, &clip);

    for (int k = 0; k < FFT_SIZE - 1; k++) {
        int x1 = GRAPH_L + (int)((float)k       / FFT_SIZE * GRAPH_W);
        int x2 = GRAPH_L + (int)((float)(k + 1) / FFT_SIZE * GRAPH_W);
        int y1 = db_to_y(ctx, psd[k]);
        int y2 = db_to_y(ctx, psd[k + 1]);

        /* Doldurma (yarı saydam) — baseline'dan yukarıya */
        SDL_SetRenderDrawColor(ctx->renderer, 30, 80, 160, 55);
        SDL_RenderDrawLine(ctx->renderer, x1, y1, x1, baseline);

        /* Ana çizgi */
        SDL_SetRenderDrawColor(ctx->renderer, 80, 160, 255, 255);
        SDL_RenderDrawLine(ctx->renderer, x1, y1, x2, y2);
    }

    SDL_RenderSetClipRect(ctx->renderer, NULL);   /* clip'i kaldır */
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_NONE);

    (void)top_clip; /* kullanılmıyor, clamp db_to_y'de yapılıyor */
}

/* ── Şelale (waterfall) ──────────────────────────────────────── */
void render_waterfall(RenderCtx *ctx,
                      const float waterfall[WATERFALL_ROWS][FFT_SIZE]) {
    int cell_h = WFALL_H / WATERFALL_ROWS;
    if (cell_h < 1) cell_h = 1;

    for (int row = 0; row < WATERFALL_ROWS; row++) {
        int y = WFALL_TOP + row * cell_h;
        for (int k = 0; k < FFT_SIZE; k++) {
            int x = GRAPH_L + (int)((float)k       / FFT_SIZE * GRAPH_W);
            int w = (int)   ((float)(k + 1) / FFT_SIZE * GRAPH_W) - x;
            if (w < 1) w = 1;
            SDL_Color c = render_colormap(waterfall[row][k],
                                          ctx->db_min, ctx->db_max);
            render_fill_rect(ctx, x, y, w, cell_h, c);
        }
    }
}

void render_present(RenderCtx *ctx) {
    SDL_RenderPresent(ctx->renderer);
}