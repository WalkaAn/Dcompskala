#pragma once
// SKALA vokal (ala companding dbx) untuk strip A mic.
//
// Seluruh dinamika dirapatkan rata dari satu titik poros, TANPA threshold:
//     gainDb = (poros - level) * (1 - 1/skala)
// Bisikan naik, teriakan turun, bentuk frasa tetap sama (hanya diskalakan).
//
//  - Lantai: di bawah level ini angkatan berhenti bertambah (desis tidak ikut naik).
//  - Batas angkatan: pengaman feedback untuk mic di sound system.
//  - Pembaca level RMS halus (puluhan ms) supaya awal bunyi/konsonan (bright) lewat utuh.
//  - Lookahead ~6 ms: suara ditunda sedikit supaya pembaca sempat "mengintip" ke depan.
//  - ON/OFF dihaluskan lewat gain (tidak ada klik); jalur tunda tetap jalan saat OFF supaya
//    latensi tidak berubah-ubah.

#include <algorithm>
#include <cmath>

#include "dsp_base.h"

struct ScaleParams {
    bool on = false;
    float ratio = 2.7f;       // 1 = tidak diubah
    float pivotDb = -6.f;     // titik poros (dBFS)
    float floorDb = -75.f;    // lantai (dBFS)
    float smoothMs = 60.f;    // kehalusan pembaca
    float maxBoostDb = 18.f;  // batas angkatan
};

class VocalScale {
public:
    float gainDb = 0.f;  // gain saat ini (dB), positif = diangkat; untuk meter

    void reset() {
        for (int i = 0; i < kMax; ++i) dl[i] = dr[i] = 0.f;
        idx = 0;
        ms = 0.f;
        gainDb = 0.f;
    }

    void prepare(float sampleRate, const ScaleParams& p) {
        const float sm = std::min(std::max(p.smoothMs, 5.f), 500.f);
        if (sm != lastSmooth || sampleRate != lastSr) {
            lastSmooth = sm;
            lastSr = sampleRate;
            msCoef = std::exp(-1.f / (sm * 0.001f * sampleRate));
            gCoef = std::exp(-1.f / (0.005f * sampleRate));  // penghalus gain 5 ms
            la = std::max(1, std::min(kMax - 1, static_cast<int>(0.006f * sampleRate)));
        }
    }

    // Diolah di tempat. stereo=false: hanya l yang dipakai.
    void process(float& l, float& r, bool stereo, const ScaleParams& p) {
        // 1. baca level (stereo terkunci: rata-rata daya L/R)
        const float pw = stereo ? 0.5f * (l * l + r * r) : l * l;
        ms = msCoef * ms + (1.f - msCoef) * pw;
        if (ms < 1e-20f) ms = 0.f;  // hindari denormal
        const float levelDb = 10.f * std::log10(ms + 1e-12f);

        // 2. gain skala
        float target = 0.f;
        const float ratio = std::max(1.f, p.ratio);
        if (p.on && ratio > 1.001f) {
            const float k = 1.f - 1.f / ratio;
            const float lv = std::max(levelDb, p.floorDb);
            target = std::min((p.pivotDb - lv) * k, p.maxBoostDb);
        }
        gainDb = gCoef * gainDb + (1.f - gCoef) * target;
        const float g = dbToLin(gainDb);

        // 3. tunda sedikit lalu terapkan gain
        dl[idx] = l;
        dr[idx] = stereo ? r : l;
        int rd = idx - la;
        if (rd < 0) rd += kMax;
        l = dl[rd] * g;
        r = dr[rd] * g;
        if (++idx >= kMax) idx = 0;
    }

private:
    static constexpr int kMax = 1024;  // cukup untuk 6 ms sampai 96 kHz
    float dl[kMax] = {}, dr[kMax] = {};
    int idx = 0, la = 288;
    float ms = 0.f, msCoef = 0.999f, gCoef = 0.995f;
    float lastSmooth = -1.f, lastSr = -1.f;
};
