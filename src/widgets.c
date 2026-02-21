/* widgets.c — Slider, Button, TextInput UI bileşenleri */
#include "widgets.h"
#include <stdio.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════
 *  SLIDER
 * ═══════════════════════════════════════════════════════════ */
float slider_ratio(const Slider *s) {
    return (s->val - s->min) / (s->max - s->min);
}

int slider_hit(const Slider *s, int mx, int my) {
    return mx >= s->x - 4  && mx <= s->x + s->w + 4 &&
           my >= s->y - 6  && my <= s->y + s->h + 6;
}

void slider_set_from_x(Slider *s, int mx) {
    float r = (float)(mx - s->x) / (float)s->w;
    if (r < 0.0f) r = 0.0f;
    if (r > 1.0f) r = 1.0f;
    s->val = s->min + r * (s->max - s->min);
}

void slider_draw(RenderCtx *ctx, const Slider *s, int enabled) {
    SDL_Color track  = enabled ? (SDL_Color){55, 55, 80,  255}
                               : (SDL_Color){35, 35, 45,  255};
    SDL_Color handle = enabled ? (SDL_Color){100,145,225, 255}
                               : (SDL_Color){70,  70, 80, 255};

    int cy = s->y + s->h / 2;

    /* İz */
    render_fill_rect(ctx, s->x, cy - 3, s->w, 6, track);

    /* Tutamaç */
    int hx = s->x + (int)(slider_ratio(s) * s->w);
    render_fill_rect   (ctx, hx - 8, cy - 8, 16, 16, handle);
    render_outline_rect(ctx, hx - 8, cy - 8, 16, 16,
                        (SDL_Color){200, 200, 210, 200});

    /* Etiket + değer */
    if (ctx->font_sm) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s: %.1f", s->label, s->val);
        render_text(ctx, ctx->font_sm, buf, s->x, s->y - 18,
                    (SDL_Color){185, 185, 200, 255});
    }
}

/* ═══════════════════════════════════════════════════════════
 *  BUTTON
 * ═══════════════════════════════════════════════════════════ */
int button_hit(const Button *b, int mx, int my) {
    return mx >= b->x && mx <= b->x + b->w &&
           my >= b->y && my <= b->y + b->h;
}

void button_draw(RenderCtx *ctx, const Button *b) {
    SDL_Color bg = b->hover
        ? (SDL_Color){
            (Uint8)SDL_min(b->bg.r + 35, 255),
            (Uint8)SDL_min(b->bg.g + 35, 255),
            (Uint8)SDL_min(b->bg.b + 35, 255), 255 }
        : b->bg;

    render_fill_rect   (ctx, b->x, b->y, b->w, b->h, bg);
    render_outline_rect(ctx, b->x, b->y, b->w, b->h,
                        (SDL_Color){150, 150, 165, 255});

    if (ctx->font_sm) {
        int tw, th;
        TTF_SizeUTF8(ctx->font_sm, b->text, &tw, &th);
        render_text(ctx, ctx->font_sm, b->text,
            b->x + (b->w - tw) / 2,
            b->y + (b->h - th) / 2,
            (SDL_Color){230, 230, 240, 255});
    }
}

/* ═══════════════════════════════════════════════════════════
 *  TEXT INPUT
 * ═══════════════════════════════════════════════════════════ */
int textinput_hit(const TextInput *t, int mx, int my) {
    return mx >= t->x && mx <= t->x + t->w &&
           my >= t->y && my <= t->y + t->h;
}

void textinput_append_char(TextInput *t, char c) {
    if (t->len < (int)sizeof(t->buf) - 2) {
        t->buf[t->len++] = c;
        t->buf[t->len]   = '\0';
    }
}

void textinput_backspace(TextInput *t) {
    if (t->len > 0)
        t->buf[--t->len] = '\0';
}

void textinput_draw(RenderCtx *ctx, const TextInput *t) {
    SDL_Color bg  = t->active ? (SDL_Color){35, 40, 70, 255}
                              : (SDL_Color){22, 22, 40, 255};
    SDL_Color brd = t->active ? (SDL_Color){100,180,255,255}
                              : (SDL_Color){90,  90,115,255};

    render_fill_rect   (ctx, t->x, t->y, t->w, t->h, bg);
    render_outline_rect(ctx, t->x, t->y, t->w, t->h, brd);

    if (ctx->font_sm) {
        char disp[40];
        if (t->active) snprintf(disp, sizeof(disp), "%s|", t->buf);
        else           strncpy (disp, t->buf, sizeof(disp) - 1);
        int th;
        TTF_SizeUTF8(ctx->font_sm, "|", NULL, &th);
        render_text(ctx, ctx->font_sm, disp,
                    t->x + 6, t->y + (t->h - th) / 2,
                    (SDL_Color){240, 240, 245, 255});
    }
}