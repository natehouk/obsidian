// CPU: the 2.x case (24 voices, both oscillators, both 24 dB filters, all LFOs and 8 classic slots), and 3.0
// cases: 16 voices with four oscillators, four filters, 20 slots of per-voice modulation and every effect;
// the same at 4x (HQ); and 64 voices.
#include "../plugins/Obsidian/ParamDefs.hpp"
#include <chrono>
#include <cstdio>
#include <memory>
using namespace obsidian;
static void time(Engine& e, const char* what) {
    const int N = 48000 * 5; float L[256], R[256];
    auto t0 = std::chrono::steady_clock::now(); for (int i = 0; i < N; i += 256) e.render(L, R, 256); auto t1 = std::chrono::steady_clock::now();
    const double sec = std::chrono::duration<double>(t1 - t0).count();
    printf("%-58s %2d voices: %5.1f%% of one core\n", what, e.activeVoices(), sec / 5 * 100);
}
static void v3(Engine& e, int voices) {
    e.p[kVoices] = voices; e.p[kO2Level] = 0.7f; for (int o = 0; o < 4; ++o) e.p[kOscOnFirst + o] = 1;
    for (int f = 0; f < 4; ++f) { e.p[fpar(f, kFOn)] = 1; e.p[fpar(f, kFSlope)] = kSlope24; e.p[fpar(f, kFStyle)] = 4 + f; }
    e.p[kRevOn] = e.p[kDlyOn] = e.p[kChoOn] = e.p[kPhsOn] = e.p[kDrvOn] = e.p[kCmpOn] = 1;
    for (int s = 0; s < 20; ++s) e.setSlot2(s, 1 + s % 12, 1 + (s * 7) % (kV2SlotFirst - 1), 0.05f);
}
int main() {
    { std::unique_ptr<Engine> e(new Engine()); e->setSampleRate(48000); e->p[kO2Level] = 0.7f; e->p[kF2On] = 1; e->p[kF2Slope] = 1; e->p[kVoices] = 24;
      e->setSlot(6, kSrcLfo2, kDstF1Freq, 0.2f); e->setSlot(7, kSrcLfo3, kDstO1PW, 0.3f); for (int n = 36; n < 60; ++n) e->noteOn(n, 100); time(*e, "2.x: 2 osc, 2 filters, 8 slots"); }
    { std::unique_ptr<Engine> e(new Engine()); e->setSampleRate(48000); v3(*e, 16); for (int n = 40; n < 56; ++n) e->noteOn(n, 100); time(*e, "3.0: 4 osc, 4 filters, 20 slots, all effects"); }
    { std::unique_ptr<Engine> e(new Engine()); e->setSampleRate(48000); v3(*e, 16); e->p[kHq] = 1; for (int n = 40; n < 56; ++n) e->noteOn(n, 100); time(*e, "3.0 as above, HQ (4x)"); }
    { std::unique_ptr<Engine> e(new Engine()); e->setSampleRate(48000); v3(*e, 64); for (int n = 30; n < 94; ++n) e->noteOn(n, 100); time(*e, "3.0: 64 voices, 4 osc, 4 filters, 20 slots, all effects"); }
    return 0;
}
