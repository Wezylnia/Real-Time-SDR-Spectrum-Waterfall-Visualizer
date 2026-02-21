#pragma once
/* fft.h — Hann penceresi + Cooley-Tukey FFT + PSD hesabı */

#include <stdint.h>

#define FFT_SIZE 1024

/* Hann penceresini önceden hesapla (program başında bir kez çağır) */
void fft_init(void);

/*
 * raw: RTL-SDR'den gelen ham uint8_t IQ tamponu, uzunluk = FFT_SIZE*2
 * psd_out: FFT shift uygulanmış güç değerleri (dB), uzunluk = FFT_SIZE
 */
void fft_compute_psd(const uint8_t *raw, float *psd_out);