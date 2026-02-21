#pragma once
/* panel.h — Sağ taraf kontrol paneli: widget'lar + olay işleme */

#include "render.h"
#include "widgets.h"
#include "sdr.h"
#include "recorder.h"

/* Desteklenen sample rate seçenekleri */
#define SR_COUNT 3
typedef struct { uint32_t sr; const char *label; } SrOption;
extern const SrOption SR_OPTS[SR_COUNT];

/* Tüm panel durumu */
typedef struct {
    /* Kontroller */
    TextInput ti_freq;
    Slider    sl_gain;
    Slider    sl_dbmin;
    Slider    sl_dbmax;
    Button    btn_setfreq;
    Button    btn_sr[SR_COUNT];
    Button    btn_agc;
    Button    btn_rec;
    Button    btn_stop;

    /* Sürükleme takibi */
    Slider   *drag;

    /* Geçerli SR seçim indeksi */
    int sr_sel;
} Panel;

/* Panel widget'larını ilklendir (ekran boyutlarına göre konum hesapla) */
void panel_init(Panel *p);

/* Tüm panel widget'larını çiz */
void panel_draw(RenderCtx *ctx, const Panel *p,
                const SdrDevice *sdr, const RecorderState *rec);

/* SDL2 olaylarını işle: tıklama, sürükle, tuş, metin girişi */
void panel_handle_event(Panel *p, SDL_Event *ev,
                        SdrDevice *sdr, RecorderState *rec,
                        RenderCtx *ctx);