#pragma once
/* widgets.h — Slider, Button, TextInput UI bileşenleri */

#include "render.h"

/* ── Slider ─────────────────────────────────────────────────── */
typedef struct {
    int         x, y, w, h;
    float       min, max, val;
    int         dragging;
    const char *label;
} Slider;

void  slider_draw       (RenderCtx *ctx, const Slider *s, int enabled);
int   slider_hit        (const Slider *s, int mx, int my);
void  slider_set_from_x (Slider *s, int mx);
float slider_ratio      (const Slider *s);

/* ── Button ─────────────────────────────────────────────────── */
typedef struct {
    int      x, y, w, h;
    char     text[48];
    SDL_Color bg;
    int      hover;
} Button;

void button_draw(RenderCtx *ctx, const Button *b);
int  button_hit (const Button *b, int mx, int my);

/* ── TextInput ──────────────────────────────────────────────── */
typedef struct {
    int  x, y, w, h;
    char buf[32];
    int  len;
    int  active;
} TextInput;

void textinput_draw       (RenderCtx *ctx, const TextInput *t);
int  textinput_hit        (const TextInput *t, int mx, int my);
void textinput_append_char(TextInput *t, char c);
void textinput_backspace  (TextInput *t);
