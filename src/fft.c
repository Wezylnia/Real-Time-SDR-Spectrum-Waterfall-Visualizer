/* fft.c — Hann penceresi + Cooley-Tukey FFT + PSD hesabı */
#include "fft.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct { float r, i; } Cf;  

static float s_hann[FFT_SIZE];
static Cf    s_buf [FFT_SIZE];

void fft_init(void) {
    for (int n = 0; n < FFT_SIZE; n++)
        s_hann[n] = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * n / (FFT_SIZE - 1)));
}

/* Yerinde Cooley-Tukey (radix-2, DIT) */
static void fft_inplace(Cf *x, int N) {
    /* Bit-reversal permutation */
    for (int i = 1, j = 0; i < N; i++) {
        int bit = N >> 1;
        while (j & bit) { j ^= bit; bit >>= 1; }
        j ^= bit;
        if (i < j) { Cf t = x[i]; x[i] = x[j]; x[j] = t; }
    }
    /* Butterfly */
    for (int len = 2; len <= N; len <<= 1) {
        float ang = -2.0f * (float)M_PI / len;
        Cf wl = { cosf(ang), sinf(ang) };
        for (int i = 0; i < N; i += len) {
            Cf w = { 1.0f, 0.0f };
            for (int j = 0; j < len / 2; j++) {
                Cf u = x[i + j];
                Cf v = {
                    x[i+j+len/2].r * w.r - x[i+j+len/2].i * w.i,
                    x[i+j+len/2].r * w.i + x[i+j+len/2].i * w.r
                };
                x[i + j]         = (Cf){ u.r + v.r, u.i + v.i };
                x[i + j + len/2] = (Cf){ u.r - v.r, u.i - v.i };
                float nr = w.r * wl.r - w.i * wl.i;
                float ni = w.r * wl.i + w.i * wl.r;
                w.r = nr; w.i = ni;
            }
        }
    }
}

void fft_compute_psd(const uint8_t *raw, float *psd_out) {
    /* Ham uint8 IQ → pencereli kompleks */
    for (int n = 0; n < FFT_SIZE; n++) {
        s_buf[n].r = ((float)raw[2*n]   - 127.5f) / 128.0f * s_hann[n];
        s_buf[n].i = ((float)raw[2*n+1] - 127.5f) / 128.0f * s_hann[n];
    }

    fft_inplace(s_buf, FFT_SIZE);

    /* fftshift + dB dönüşümü */
    int half = FFT_SIZE / 2;
    for (int k = 0; k < FFT_SIZE; k++) {
        int sk = (k + half) % FFT_SIZE;
        float m2 = s_buf[sk].r * s_buf[sk].r + s_buf[sk].i * s_buf[sk].i;
        psd_out[k] = 10.0f * log10f(m2 + 1e-10f);
    }
}