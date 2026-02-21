/* panel.c — Sağ taraf kontrol paneli */
#include "panel.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>   /* atof */

const SrOption SR_OPTS[SR_COUNT] = {
    { 1024000, "1.024 MHz" },
    { 2048000, "2.048 MHz" },
    { 2400000, "2.4  MHz"  },
};

/* ── Konum sabitleri ────────────────────────────────────────── */
#define PX  (PANEL_X + 12)
#define PW  (PANEL_W - 24)

/* ── Yardımcı: SR butonlarını güncelle ───────────────────────── */
static void update_sr_buttons(Panel *p) {
    for (int i = 0; i < SR_COUNT; i++) {
        p->btn_sr[i].bg = (i == p->sr_sel)
            ? (SDL_Color){55, 120, 55, 255}
            : (SDL_Color){40, 40,  70, 255};
    }
}

/* ── Panel ilklendirme ──────────────────────────────────────── */
void panel_init(Panel *p) {
    memset(p, 0, sizeof(*p));
    p->sr_sel = 1;   /* 2.048 MHz varsayılan */
    p->drag   = NULL;

    int y = 32;

    /* Frekans girişi + "Ayarla" butonu */
    p->ti_freq    = (TextInput){ PX, y+14, PW-60, 22, "100.000", 7, 0 };
    p->btn_setfreq= (Button){ PX+PW-56, y+14, 52, 22, "Ayarla",
                               {40,80,160,255}, 0 };
    y += 52;

    /* Sample rate butonları */
    int bw = (PW - SR_COUNT + 1) / SR_COUNT;
    for (int i = 0; i < SR_COUNT; i++) {
        p->btn_sr[i] = (Button){ PX + i*(bw+2), y+14, bw, 20, "",
                                  {40,40,70,255}, 0 };
        strncpy(p->btn_sr[i].text, SR_OPTS[i].label,
                sizeof(p->btn_sr[i].text) - 1);
    }
    update_sr_buttons(p);
    y += 48;

    /* AGC butonu */
    p->btn_agc = (Button){ PX, y, PW, 22, "AGC: Açık",
                            {55,115,55,255}, 0 };
    y += 36;

    /* Kazanç slider'ı */
    p->sl_gain  = (Slider){ PX, y+14, PW, 10, 0.0f, 49.6f, 0.0f, 0,
                             "RF Kazancı (dB)" };
    y += 44;

    /* Min dB slider'ı */
    p->sl_dbmin = (Slider){ PX, y+14, PW, 10, -100.0f, 0.0f, -60.0f, 0,
                             "Min Güç (dB)" };
    y += 44;

    /* Max dB slider'ı */
    p->sl_dbmax = (Slider){ PX, y+14, PW, 10, -60.0f, 60.0f, 0.0f, 0,
                             "Max Güç (dB)" };
    y += 52;

    /* Kayıt butonları */
    p->btn_rec  = (Button){ PX,          y, PW/2-3, 26, "Kayıt Başla",
                             {145,28,28,255}, 0 };
    p->btn_stop = (Button){ PX+PW/2+3,   y, PW/2-3, 26, "Durdur",
                             {55,55,70,255},  0 };
}

/* ── Panel çizimi ───────────────────────────────────────────── */
void panel_draw(RenderCtx *ctx, const Panel *p,
                const SdrDevice *sdr, const RecorderState *rec) {
    /* Arka plan */
    render_fill_rect(ctx, PANEL_X, 0, PANEL_W, WIN_H,
                     (SDL_Color){14, 14, 28, 255});
    render_line(ctx, PANEL_X, 0, PANEL_X, WIN_H,
                (SDL_Color){50, 50, 80, 255});

    /* Başlık */
    render_text_center(ctx, ctx->font_md, "Kontrol Paneli",
        PANEL_X + PANEL_W / 2, 8, (SDL_Color){175, 205, 255, 255});
    render_line(ctx, PANEL_X+8, 30, PANEL_X+PANEL_W-8, 30,
                (SDL_Color){40, 50, 80, 255});

    SDL_Color lbl = {150, 180, 215, 255};
    render_text(ctx, ctx->font_sm, "Merkez Frekans (MHz)", PX,  32, lbl);
    render_text(ctx, ctx->font_sm, "Bant Genisliği",       PX,  84, lbl);

    textinput_draw(ctx, &p->ti_freq);
    button_draw(ctx, &p->btn_setfreq);
    for (int i = 0; i < SR_COUNT; i++) button_draw(ctx, &p->btn_sr[i]);
    button_draw(ctx, &p->btn_agc);

    slider_draw(ctx, &p->sl_gain,  !sdr->agc_on);
    slider_draw(ctx, &p->sl_dbmin, 1);
    slider_draw(ctx, &p->sl_dbmax, 1);

    button_draw(ctx, &p->btn_rec);
    button_draw(ctx, &p->btn_stop);

    /* Kayıt durumu */
    int sy = p->btn_rec.y + 38;
    if (rec->active) {
        render_text(ctx, ctx->font_sm, "Kaydediliyor...", PX, sy,
                    (SDL_Color){255, 70, 70, 255});
        const char *slash = strrchr(rec->filepath, '\\');
        render_text(ctx, ctx->font_sm, slash ? slash+1 : rec->filepath,
                    PX, sy + 16, (SDL_Color){155, 155, 165, 255});
    } else if (rec->filepath[0]) {
        render_text(ctx, ctx->font_sm, "Kayıt tamam ✓", PX, sy,
                    (SDL_Color){80, 210, 95, 255});
    }

    /* Alt durum */
    char buf[64];
    snprintf(buf, sizeof(buf), "FC : %.3f MHz", sdr->center_freq / 1e6);
    render_text(ctx, ctx->font_sm, buf, PX, WIN_H-44,
                (SDL_Color){105,115,130,255});
    snprintf(buf, sizeof(buf), "SR : %.3f MHz", sdr->sample_rate / 1e6);
    render_text(ctx, ctx->font_sm, buf, PX, WIN_H-28,
                (SDL_Color){105,115,130,255});
}

/* ── Yardımcı: frekans giriş alanını SDR'a uygula ──────────── */
static void apply_freq(Panel *p, SdrDevice *sdr) {
    double mhz = atof(p->ti_freq.buf);
    sdr_set_freq(sdr, (uint32_t)(mhz * 1e6));
    snprintf(p->ti_freq.buf, sizeof(p->ti_freq.buf),
             "%.3f", sdr->center_freq / 1e6);
    p->ti_freq.len = (int)strlen(p->ti_freq.buf);
}

/* ── Yardımcı: AGC text güncelle ───────────────────────────── */
static void refresh_agc_btn(Panel *p, const SdrDevice *sdr) {
    if (sdr->agc_on) {
        strncpy(p->btn_agc.text, "AGC: Açık", sizeof(p->btn_agc.text)-1);
        p->btn_agc.bg = (SDL_Color){55,115,55,255};
    } else {
        snprintf(p->btn_agc.text, sizeof(p->btn_agc.text),
                 "AGC Off  %.1f dB", sdr->gain_db);
        p->btn_agc.bg = (SDL_Color){118,72,20,255};
    }
}

/* ── Olay işleyicisi ────────────────────────────────────────── */
void panel_handle_event(Panel *p, SDL_Event *ev,
                        SdrDevice *sdr, RecorderState *rec,
                        RenderCtx *ctx) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    switch (ev->type) {

    /* ─── Fare bas ─── */
    case SDL_MOUSEBUTTONDOWN:
        if (ev->button.button != SDL_BUTTON_LEFT) break;

        /* TextInput odak */
        if (textinput_hit(&p->ti_freq, mx, my)) {
            p->ti_freq.active = 1;
            SDL_StartTextInput();
        } else {
            p->ti_freq.active = 0;
            SDL_StopTextInput();
        }

        /* Butonlar */
        if (button_hit(&p->btn_setfreq, mx, my)) apply_freq(p, sdr);

        for (int i = 0; i < SR_COUNT; i++) {
            if (button_hit(&p->btn_sr[i], mx, my)) {
                p->sr_sel = i;
                sdr_set_sr(sdr, SR_OPTS[i].sr);
                update_sr_buttons(p);
                break;
            }
        }

        if (button_hit(&p->btn_agc, mx, my)) {
            sdr_set_agc(sdr, !sdr->agc_on);
            refresh_agc_btn(p, sdr);
        }
        if (button_hit(&p->btn_rec,  mx, my) && !rec->active) recorder_start(rec);
        if (button_hit(&p->btn_stop, mx, my) &&  rec->active) recorder_stop(rec);

        /* Slider sürükleme başlat */
        if (!sdr->agc_on && slider_hit(&p->sl_gain, mx, my)) {
            p->drag = &p->sl_gain;
            slider_set_from_x(&p->sl_gain, mx);
            sdr_set_gain(sdr, p->sl_gain.val);
            refresh_agc_btn(p, sdr);
        }
        if (slider_hit(&p->sl_dbmin, mx, my)) {
            p->drag = &p->sl_dbmin;
            slider_set_from_x(&p->sl_dbmin, mx);
            ctx->db_min = p->sl_dbmin.val;
        }
        if (slider_hit(&p->sl_dbmax, mx, my)) {
            p->drag = &p->sl_dbmax;
            slider_set_from_x(&p->sl_dbmax, mx);
            ctx->db_max = p->sl_dbmax.val;
        }
        break;

    /* ─── Fare bırak ─── */
    case SDL_MOUSEBUTTONUP:
        p->drag = NULL;
        break;

    /* ─── Fare hareketi ─── */
    case SDL_MOUSEMOTION:
        if (p->drag) {
            slider_set_from_x(p->drag, mx);
            if (p->drag == &p->sl_gain && !sdr->agc_on) {
                sdr_set_gain(sdr, p->sl_gain.val);
                refresh_agc_btn(p, sdr);
            }
            if (p->drag == &p->sl_dbmin) ctx->db_min = p->sl_dbmin.val;
            if (p->drag == &p->sl_dbmax) ctx->db_max = p->sl_dbmax.val;
        }
        /* Hover güncelle */
        p->btn_setfreq.hover = button_hit(&p->btn_setfreq, mx, my);
        p->btn_agc.hover     = button_hit(&p->btn_agc,     mx, my);
        p->btn_rec.hover     = button_hit(&p->btn_rec,     mx, my);
        p->btn_stop.hover    = button_hit(&p->btn_stop,    mx, my);
        for (int i = 0; i < SR_COUNT; i++)
            p->btn_sr[i].hover = button_hit(&p->btn_sr[i], mx, my);
        break;

    /* ─── Metin girişi ─── */
    case SDL_TEXTINPUT:
        if (p->ti_freq.active) {
            for (const char *c = ev->text.text; *c; c++)
                if ((*c >= '0' && *c <= '9') || *c == '.')
                    textinput_append_char(&p->ti_freq, *c);
        }
        break;

    /* ─── Tuş ─── */
    case SDL_KEYDOWN:
        if (p->ti_freq.active) {
            if (ev->key.keysym.sym == SDLK_BACKSPACE)
                textinput_backspace(&p->ti_freq);
            if (ev->key.keysym.sym == SDLK_RETURN ||
                ev->key.keysym.sym == SDLK_KP_ENTER)
                apply_freq(p, sdr);
            if (ev->key.keysym.sym == SDLK_ESCAPE) {
                p->ti_freq.active = 0;
                SDL_StopTextInput();
            }
        } else {
            /* Frekans kaydırma klavye kısayolları */
            if (ev->key.keysym.sym == SDLK_RIGHT)  sdr_shift_freq(sdr, +1000000);
            if (ev->key.keysym.sym == SDLK_LEFT)   sdr_shift_freq(sdr, -1000000);
            if (ev->key.keysym.sym == SDLK_UP)     sdr_shift_freq(sdr, +100000);
            if (ev->key.keysym.sym == SDLK_DOWN)   sdr_shift_freq(sdr, -100000);
            /* Frekans değiştiyse text box'ı güncelle */
            snprintf(p->ti_freq.buf, sizeof(p->ti_freq.buf),
                     "%.3f", sdr->center_freq / 1e6);
            p->ti_freq.len = (int)strlen(p->ti_freq.buf);
        }
        break;
    }
}