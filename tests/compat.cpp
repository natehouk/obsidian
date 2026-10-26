// 2.x compatibility: random 2.x settings (3.0 parameters at their defaults), modulation slots, MIDI (notes,
// sustain, bend, wheel, aftertouch), tempo and parameter changes between blocks, through the frozen 2.0
// engine and the 3.0 engine: the outputs must match.
#include "../plugins/Obsidian/ParamDefs.hpp"
#include "legacy/ObsidianEngine2.hpp"
#include <cstdio>
#include <memory>
#include <random>
#include <vector>
using namespace obsidian;
int main() {
    std::mt19937 g(9); std::uniform_real_distribution<float> u(0, 1);
    int bad = 0; float worst = -1000;
    for (int trial = 0; trial < 40; ++trial) {
        const int sr = trial % 3 == 0 ? 44100 : (trial % 3 == 1 ? 48000 : 96000);
        std::unique_ptr<Engine> e(new Engine()); std::unique_ptr<obsidian2::Engine> o(new obsidian2::Engine());
        auto rnd = [&](uint32_t i) {
            const ParamDef& d = paramDef(i); float lo = d.min, hi = d.max;
            if (i == kO1Wave || i == kO2Wave) hi = 2;                               // 2.x ranges
            if (i == kF1Style || i == kF2Style) hi = kNumStyles2 - 1;
            if (i == kF1Type || i == kF2Type) hi = 2;
            if (i == kF1Slope || i == kF2Slope) hi = 1;
            if (i == kVoices) hi = 24;
            if (i == kUnison) hi = 4;
            if (i >= kSlotFirst && i <= kSlotLast && (i - kSlotFirst) % 4 == kSlotDst) hi = obsidian2::kNumDests - 1;
            float v = (d.hints & kHintLog) ? lo * std::pow(hi / lo, u(g)) : lo + (hi - lo) * u(g);
            if (d.hints & kHintInteger) v = std::round(v);
            if (i >= kSlotFirst && i <= kSlotLast && (i - kSlotFirst) % 4 == kSlotSrc && u(g) < 0.6f) v = 0;
            if (i == kMidiOn) v = 1;
            if (i == kVolume) v = clampf(v, -18.f, -6.f);
            if (i == kO1Level || i == kO2Level) v = std::max(v, 0.4f);
            if (i == kE3Attack || i == kE3Hold) v = std::min(v, 0.3f);
            if (i == kE3Sustain) v = std::max(v, 0.3f);
            if (i == kF1Freq || i == kF2Freq) v = std::max(v, 300.f);
            return v;
        };
        for (uint32_t i = 0; i < kOutLearn; ++i) { const float v = rnd(i); e->p[i] = v; o->p[i] = v; }
        e->setSampleRate((float)sr); o->setSampleRate((float)sr);
        const int N = sr * 3 / 2; std::vector<float> L1(N), R1(N), L2(N), R2(N);
        for (int i = 0; i < N; i += 256) {
            const int n = std::min(256, N - i);
            const float ev = u(g);
            if (ev < 0.12f) { const int nt = 30 + (int)(u(g) * 60), vv = 1 + (int)(u(g) * 126); e->noteOn(nt, vv); o->noteOn(nt, vv); }
            else if (ev < 0.22f) { const int nt = 30 + (int)(u(g) * 60); e->noteOff(nt); o->noteOff(nt); }
            else if (ev < 0.24f) { e->allNotesOff(); o->allNotesOff(); }
            else if (ev < 0.26f) { const bool dn = u(g) < 0.5f; e->sustain(dn); o->sustain(dn); }
            else if (ev < 0.29f) { const int w = (int)(u(g) * 127); e->modWheel(w); o->modWheel(w); }
            else if (ev < 0.32f) { const int b = (int)(u(g) * 16383); e->pitchBend(b); o->pitchBend(b); }
            else if (ev < 0.34f) { const int a = (int)(u(g) * 127); e->channelPressure(a); o->channelPressure(a); }
            else if (ev < 0.35f) { e->allSoundOff(); o->allSoundOff(); }
            if (u(g) < 0.04f) { const uint32_t k = (uint32_t)(u(g) * kSlotFirst); const float v = rnd(k); e->p[k] = v; o->p[k] = v; }
            const bool play = (i / 9000) % 2 == 0;
            e->setTransport(true, play, i / (double)sr * 0.5, 120, 4); o->setTransport(true, play, i / (double)sr * 0.5, 120, 4);
            e->render(L1.data() + i, R1.data() + i, n);
            o->render(L2.data() + i, R2.data() + i, n);
        }
        double dd = 0, s = 1e-30; for (int i = 0; i < N; ++i) { dd += (L1[i] - L2[i]) * (L1[i] - L2[i]) + (R1[i] - R2[i]) * (R1[i] - R2[i]); s += L2[i] * L2[i] + R2[i] * R2[i]; }
        const float db = dd == 0 ? -240.f : (float)(10 * std::log10(dd / s));
        worst = std::max(worst, db); if (db > -120) { ++bad; printf("trial %d (%d Hz): difference %.1f dB\n", trial, sr, db); }
        if (s < 1e-6) printf("trial %d: silent\n", trial);
    }
    printf("2.x compatibility: largest difference %.1f dB below the output level, %d failures\n", -worst, bad);
    return bad;
}
