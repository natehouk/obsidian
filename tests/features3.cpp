// Obsidian 3.0 feature checks: four oscillators and the new waves, sync ratio, ring modulation, drift,
// tuning, per-oscillator polyphony, 64 voices, filters 3-4 and the new shapes, slopes, styles and offsets,
// per-oscillator routing, HQ, the effects, the arpeggiator, and the per-voice modulation system.
#include "../plugins/Obsidian/ParamDefs.hpp"
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>
using namespace obsidian;
static int fails = 0;
#define CHECK(c, ...) do { bool ok_ = (c); printf("%s  ", ok_ ? "PASS" : "FAIL"); printf(__VA_ARGS__); printf("\n"); if (!ok_) ++fails; } while (0)
static const int SR = 48000;
typedef std::unique_ptr<Engine> EP;
struct Buf { std::vector<float> l, r; };
static Buf run(Engine& e, int n) { Buf b; b.l.resize(n); b.r.resize(n); for (int i = 0; i < n; i += 256) e.render(b.l.data() + i, b.r.data() + i, std::min(256, n - i)); return b; }
static float rms(const std::vector<float>& v, int a, int z) { double s = 0; for (int i = a; i < z; ++i) s += v[i] * v[i]; return (float)std::sqrt(s / std::max(1, z - a)); }
static float db(float a, float b) { return 20.f * std::log10(std::max(a, 1e-12f) / std::max(b, 1e-12f)); }
static float band(const std::vector<float>& v, int a, int z, float f) {   // Goertzel magnitude (dB)
    double c = 0, s = 0; for (int i = a; i < z; ++i) { const double w = 2 * M_PI * f * i / SR; c += v[i] * std::cos(w); s += v[i] * std::sin(w); }
    return (float)(10 * std::log10((c * c + s * s) / ((double)(z - a) * (z - a)) + 1e-30)); }
static float bandH(const std::vector<float>& v, int a, int z, float f) {  // Hann-windowed: no leakage from strong low partials
    double c = 0, s = 0; const int n = z - a; for (int i = a; i < z; ++i) { const double w = 2 * M_PI * f * i / SR, h = 0.5 - 0.5 * std::cos(2 * M_PI * (i - a) / n); c += h * v[i] * std::cos(w); s += h * v[i] * std::sin(w); }
    return (float)(10 * std::log10((c * c + s * s) / ((double)n * n) + 1e-30)); }
static float pitch(const std::vector<float>& v, int a, int z) {          // from upward zero crossings
    int first = -1, last = -1, n = 0; float fa = 0, fb = 0;
    for (int i = a + 1; i < z; ++i) if (v[i - 1] < 0 && v[i] >= 0) { const float fr = (i - 1) + v[i - 1] / (v[i - 1] - v[i]); if (first < 0) { first = i; fa = fr; } last = i; fb = fr; ++n; }
    return n > 1 ? (n - 1) * SR / (fb - fa) : 0.f;
}
// a plain patch: osc 1 only, no filters, no modulation, fast amp envelope
static EP plain() {
    EP e(new Engine()); e->setSampleRate(SR);
    e->p[kF1On] = 0; e->p[kF2On] = 0; for (int s = 0; s < kNumSlots; ++s) e->p[slotParam(s, kSlotBypass)] = 1;
    e->p[kE3Attack] = 0.001f; e->p[kE3Sustain] = 1; e->p[kE3Release] = 0.05f; e->p[kAmpVel] = 0; e->p[kVolume] = -6; e->p[kO1Phase] = 1;
    return e;
}
int main() {
    // ---- oscillators
    { EP e = plain(); e->p[kO1Wave] = 3; e->noteOn(69, 100); Buf b = run(*e, SR / 2);
      const float h1 = band(b.l, SR / 4, SR / 2, 440), h2 = band(b.l, SR / 4, SR / 2, 880), h3 = band(b.l, SR / 4, SR / 2, 1320);
      CHECK(h1 - std::max(h2, h3) > 50 && std::fabs(pitch(b.l, SR / 4, SR / 2) - 440) < 1, "sine: 440 Hz, harmonics %.0f dB down", h1 - std::max(h2, h3)); }
    { auto tilt = [](int wave) { EP e = plain(); e->p[kO1Wave] = wave; e->noteOn(60, 100); Buf b = run(*e, SR);
          float lo = 0, hi = 0; for (float f = 200; f < 400; f += 37) lo += band(b.l, SR / 4, SR, f); for (float f = 6000; f < 6200; f += 37) hi += band(b.l, SR / 4, SR, f); return (lo - hi) / 6; };
      const float w = tilt(4), pk = tilt(5);
      CHECK(std::fabs(w) < 4 && pk > 10, "noise: white is flat (%.1f dB), pink falls (%.1f dB from 300 Hz to 6 kHz)", w, pk); }
    { EP e = plain(); e->p[kO1Level] = 0; e->p[kOscOnFirst + 2] = 1; e->p[kO3Wave] = 3; e->p[kO3Level] = 1; e->p[kO3Oct] = 1; e->p[kO3Phase] = 1; e->noteOn(57, 100); Buf b = run(*e, SR / 2);
      CHECK(std::fabs(pitch(b.l, SR / 4, SR / 2) - 440) < 1.5f, "oscillator 3 an octave up: %.1f Hz", pitch(b.l, SR / 4, SR / 2)); }
    { EP e = plain(); e->p[kO1Wave] = 3; e->p[kO2Wave] = 3; e->p[kO2Level] = 1; e->p[kO2Fine] = 0; e->p[kO2Semi] = 7; e->p[kO2Phase] = 1; e->p[kO1Level] = 0; e->p[kRing12] = 1;
      e->noteOn(57, 100); Buf b = run(*e, SR / 2); const float f1 = 220, f2 = 220 * std::pow(2.f, 7 / 12.f);
      const float sum = band(b.l, SR / 4, SR / 2, f1 + f2), diff = band(b.l, SR / 4, SR / 2, f2 - f1), orig = band(b.l, SR / 4, SR / 2, f2);
      CHECK(sum - orig > 30 && diff - orig > 30, "ring modulation: sum and difference frequencies (%.0f, %.0f dB over the original)", sum - orig, diff - orig); }
    { auto zc = [](float ratio) { EP e = plain(); e->p[kO1Wave] = 1; e->p[kOscRatioFirst] = ratio; e->noteOn(45, 100); Buf b = run(*e, SR / 2);
          int z = 0; for (int i = SR / 4; i < SR / 2; ++i) z += (b.l[i - 1] < 0) != (b.l[i] < 0); return z; };
      const int a = zc(1), b2 = zc(3.5f);
      CHECK(b2 > a * 2, "sync ratio 3.5 squeezes more cycles into each period (%d -> %d zero crossings), keeping the note's period", a, b2); }
    { EP e = plain(); e->p[kO1Wave] = 3; e->p[kTune] = 100; e->p[kTranspose] = 1; e->noteOn(57, 100); Buf b = run(*e, SR / 2);
      const float f = pitch(b.l, SR / 4, SR / 2), want = 440 * std::pow(2.f, 1 / 12.f);
      CHECK(std::fabs(f - want) < 1.5f, "transpose +1 octave and tune +100 cents: %.1f Hz (want %.1f)", f, want); }
    { EP e = plain(); e->p[kO1Wave] = 3; e->p[kDrift] = 1; e->noteOn(69, 100); Buf b = run(*e, SR * 3);
      float lo = 1e9, hi = 0; for (int s = 1; s < 12; ++s) { const float f = pitch(b.l, s * SR / 4, s * SR / 4 + SR / 4); lo = std::min(lo, f); hi = std::max(hi, f); }
      const float cents = 1200 * std::log2(hi / lo);
      CHECK(cents > 1 && cents < 40, "drift wanders the pitch by %.1f cents", cents); }
    // ---- voices
    { EP e = plain(); e->p[kVoices] = 64; for (int n = 30; n < 70; ++n) e->noteOn(n, 100); run(*e, 512);
      CHECK(e->activeVoices() == 40, "64 voices: 40 notes sound (%d voices)", e->activeVoices()); }
    { EP e = plain(); e->p[kVoices] = 8; e->p[kUnison] = 8; e->noteOn(60, 100); e->noteOn(64, 100); run(*e, 512);
      CHECK(e->activeVoices() == 16, "unison 8: two notes use 16 voices (%d)", e->activeVoices()); }
    { EP e = plain(); e->p[kPolyMode] = 1; e->p[kO1Wave] = 3; e->p[kO2Wave] = 3; e->p[kO2Level] = 1; e->p[kO2Oct] = 1; e->p[kO2Fine] = 0; e->p[kO2Phase] = 1;
      e->noteOn(57, 100); Buf a = run(*e, SR / 4); e->noteOff(57); run(*e, SR / 4); e->noteOn(57, 100); Buf b = run(*e, SR / 4);
      const float fa = pitch(a.l, SR / 8, SR / 4), fb = pitch(b.l, SR / 8, SR / 4);
      CHECK(std::fabs(fa - 220) < 1 && std::fabs(fb - 440) < 1.5f, "per oscillator: the first note plays osc 1 (%.0f Hz), the next osc 2 (%.0f Hz)", fa, fb); }
    // ---- filters
    { auto lvl = [](bool on, float freq) { EP e = plain(); e->p[kO1Wave] = 3; if (on) { e->p[kF3On] = 1; e->p[kF3Type] = kBell; e->p[kF3Freq] = 1000; e->p[fpar(2, kFGain)] = 12; e->p[kF3Style] = 4; }
          const int note = (int)std::round(69 + 12 * std::log2(freq / 440)); e->noteOn(note, 100); Buf b = run(*e, SR / 2); return rms(b.l, SR / 4, SR / 2); };
      const float at1k = db(lvl(true, 1046.5f), lvl(false, 1046.5f)), at100 = db(lvl(true, 110), lvl(false, 110));
      CHECK(at1k > 9 && std::fabs(at100) < 1, "filter 3 bell +12 dB: %.1f dB at 1 kHz, %.1f dB at 110 Hz", at1k, at100); }
    { auto lp = [](int slope, int style) { EP e = plain(); e->p[kO1Wave] = 3; e->p[kF1On] = 1; e->p[kF1Type] = 0; e->p[kF1Freq] = 500; e->p[kF1Peak] = 0; e->p[kF1Slope] = slope; e->p[kF1Style] = style;
          e->noteOn(93, 100); Buf b = run(*e, SR / 2); return rms(b.l, SR / 4, SR / 2); };
      const float s6 = lp(kSlope6, 4), s12 = lp(kSlope12, 4), s24 = lp(kSlope24, 4), s48 = lp(kSlope48, 4);
      CHECK(s6 > s12 && s12 > s24 && s24 > s48 && db(s24, s48) > 25, "slopes at 1.76 kHz through a 500 Hz low pass: 6 %.0f, 12 %.0f, 24 %.0f, 48 %.0f dB", 20 * std::log10(s6), 20 * std::log10(s12), 20 * std::log10(s24), 20 * std::log10(s48)); }
    { auto notch = [](bool on) { EP e = plain(); e->p[kO1Wave] = 3; if (on) { e->p[kF4On] = 1; e->p[kF4Type] = kNotch; e->p[kF4Freq] = 440; e->p[kF4Style] = 4; e->p[kF4Peak] = 0.5f; } e->noteOn(69, 100); Buf b = run(*e, SR / 2); return rms(b.l, SR / 4, SR / 2); };
      CHECK(db(notch(true), notch(false)) < -25, "filter 4 notch at 440 Hz removes a 440 Hz sine (%.0f dB)", db(notch(true), notch(false))); }
    { auto lp = [](float off) { EP e = plain(); e->p[kO1Wave] = 1; e->p[kF1On] = 1; e->p[kF1Freq] = 1000; e->p[kF1Peak] = 0; e->p[kFreqOffset] = off; e->noteOn(45, 100); Buf b = run(*e, SR / 2); return band(b.l, SR / 4, SR / 2, 1980); };
      CHECK(lp(1) - lp(0) > 6, "filter freq offset +1 octave opens all filters (%.1f dB more at 2 kHz)", lp(1) - lp(0)); }
    { EP e = plain(); e->p[kO1Wave] = 3; e->p[kO2Wave] = 3; e->p[kO2Level] = 1; e->p[kO2Oct] = 2; e->p[kO2Fine] = 0; e->p[kRouting] = kSplit;
      e->p[kF1On] = 1; e->p[kF1Type] = 2; e->p[kF1Freq] = 20000; e->p[kF2On] = 1; e->p[kF2Type] = 0; e->p[kF2Freq] = 100; e->p[kF2Slope] = 2;
      e->p[kOscOnFirst + 2] = 1; e->p[kO3Wave] = 3; e->p[kO3Level] = 1; e->p[kO3Oct] = 1; e->p[kF3On] = 0;
      e->noteOn(57, 100); Buf b = run(*e, SR / 2);
      const float a = band(b.l, SR / 4, SR / 2, 220), c = band(b.l, SR / 4, SR / 2, 880), m = band(b.l, SR / 4, SR / 2, 440);
      CHECK(m - std::max(a, c) > 40, "per-oscillator routing: osc 1 into a 20 kHz high pass, osc 2 into a 100 Hz low pass, osc 3 straight through (%.0f dB above the others)", m - std::max(a, c)); }
    { auto styled = [](int style) { EP e = plain(); e->p[kO1Wave] = 1; e->p[kF1On] = 1; e->p[kF1Freq] = 800; e->p[kF1Peak] = 0.7f; e->p[kF1Style] = style; e->noteOn(45, 127); Buf b = run(*e, SR / 2); return b.l; };
      std::vector<float> a = styled(4), b = styled(9); double d = 0; for (int i = SR / 4; i < SR / 2; ++i) d += (a[i] - b[i]) * (a[i] - b[i]);
      CHECK(d > 1, "the new styles sound different (Clean vs Metal)"); }
    { auto alias = [](bool hq) { EP e = plain(); e->p[kO1Wave] = 1; e->p[kHq] = hq; e->p[kF1On] = 1; e->p[kF1Style] = 1; e->p[kF1Freq] = 9000; e->p[kF1Peak] = 0.6f;
          e->noteOn(103, 127); Buf b = run(*e, SR / 2);
          const float f0 = 440 * std::pow(2.f, (103 - 69) / 12.f); float a = 0;      // nothing legitimate below the fundamental: it is all aliasing
          for (float f = 60; f < f0 * 0.9f; f += 41) a += std::pow(10.f, band(b.l, SR / 4, SR / 2, f) / 10);
          return 10 * std::log10(a / std::pow(10.f, band(b.l, SR / 4, SR / 2, f0) / 10)); };
      const float lo = alias(false), hi = alias(true);
      CHECK(hi < lo - 6, "HQ: less aliasing from a hot filter on a high saw (%.0f -> %.0f dB)", lo, hi); }
    // ---- effects
    { auto tail = [](bool on) { EP e = plain(); e->p[kO1Wave] = 1; e->p[kRevOn] = on; e->p[kRevMix] = 0.5f; e->p[kRevTime] = 3; e->noteOn(60, 100); run(*e, SR / 4); e->noteOff(60); Buf b = run(*e, SR); return rms(b.l, SR / 2, SR); };
      CHECK(tail(true) > 100 * tail(false) + 1e-4f, "reverb: a tail after the note (%.4f vs %.6f)", tail(true), tail(false)); }
    { EP e = plain(); e->p[kO1Wave] = 1; e->p[kDlyOn] = 1; e->p[kDlySync] = 0; e->p[kDlyTime] = 0.3f; e->p[kDlyMix] = 1; e->p[kDlyFeedback] = 0; e->p[kDlyLowCut] = 20; e->p[kDlyHighCut] = 20000; e->p[kE3Release] = 0.005f;
      e->noteOn(60, 100); run(*e, SR / 20); e->noteOff(60); Buf b = run(*e, SR / 2);
      const float echo = rms(b.l, (int)(0.3f * SR) - SR / 20 + 400, (int)(0.3f * SR) - 400), gap = rms(b.l, SR / 10, SR / 5);
      CHECK(echo > 10 * gap, "delay: an echo 300 ms later (%.3f vs %.5f in the gap)", echo, gap); }
    { auto lfo = [](int which) { EP e = plain(); e->p[kO1Wave] = 1;
          if (which == 1) { e->p[kChoOn] = 1; e->p[kChoMix] = 1; } else if (which == 2) { e->p[kPhsOn] = 1; e->p[kPhsMix] = 1; e->p[kPhsRate] = 2; } else if (which == 3) { e->p[kPhsOn] = 1; e->p[kPhsMode] = 1; e->p[kPhsMix] = 1; e->p[kPhsRate] = 2; }
          e->noteOn(48, 100); Buf b = run(*e, SR); float lo = 1e9, hi = 0; for (int s = 4; s < 20; ++s) { const float r = band(b.l, s * SR / 20, s * SR / 20 + SR / 20, 1046.5f); lo = std::min(lo, r); hi = std::max(hi, r); } return hi - lo; };
      const float d0 = lfo(0), dc = lfo(1), dp = lfo(2), df = lfo(3);
      CHECK(dc > d0 + 3 && dp > d0 + 6 && df > d0 + 6, "chorus, phaser and flanger move the spectrum (8th harmonic swings %.1f / %.1f / %.1f dB, dry %.1f)", dc, dp, df, d0); }
    { auto thd = [](bool on) { EP e = plain(); e->p[kO1Wave] = 3; e->p[kDrvOn] = on; e->p[kDrvAmount] = 0.7f; e->noteOn(57, 127); Buf b = run(*e, SR / 2);
          return band(b.l, SR / 4, SR / 2, 660) - band(b.l, SR / 4, SR / 2, 220); };
      CHECK(thd(true) > thd(false) + 40, "drive adds harmonics (3rd harmonic %.0f -> %.0f dB)", thd(false), thd(true)); }
    { auto range = [](bool on) { EP e = plain(); e->p[kO1Wave] = 3; e->p[kCmpOn] = on; e->p[kCmpThreshold] = -30; e->p[kCmpRatio] = 8; e->p[kCmpAutoGain] = 0; e->p[kAmpVel] = 1;
          e->noteOn(60, 127); Buf a = run(*e, SR / 2); e->noteOff(60); run(*e, SR / 4); e->noteOn(60, 13); Buf b = run(*e, SR / 2); return db(rms(a.l, SR / 4, SR / 2), rms(b.l, SR / 4, SR / 2)); };
      CHECK(range(true) < range(false) - 8, "compressor: velocity 127 vs 13 differ by %.1f dB instead of %.1f dB", range(true), range(false)); }
    // ---- arpeggiator
    { EP e = plain(); e->p[kArpOn] = 1; e->p[kArpSync] = 1; e->p[kArpDiv] = 4; e->p[kArpOrder] = kArpUp; e->p[kArpOctaves] = 1; e->setTransport(true, false, 0, 120, 4);
      e->noteOn(60, 100); e->noteOn(64, 100); e->noteOn(67, 100);
      std::vector<int> seq; int last = -2; for (int b = 0; b < SR * 2 / 256; ++b) { float L[256], R[256]; e->render(L, R, 256); if (e->uiArpNote != last && e->uiArpNote >= 0) seq.push_back(e->uiArpNote); last = e->uiArpNote; }
      const int want[7] = { 60, 64, 67, 72, 76, 79, 60 }; bool ok = seq.size() >= 7; for (int k = 0; ok && k < 7; ++k) ok = seq[k] == want[k];
      CHECK(ok && seq.size() >= 15 && seq.size() <= 17, "arpeggiator: 1/16 up over two octaves at 120 BPM: %d notes in 2 s, starting %d %d %d %d %d %d %d", (int)seq.size(),
            seq.size() > 0 ? seq[0] : 0, seq.size() > 1 ? seq[1] : 0, seq.size() > 2 ? seq[2] : 0, seq.size() > 3 ? seq[3] : 0, seq.size() > 4 ? seq[4] : 0, seq.size() > 5 ? seq[5] : 0, seq.size() > 6 ? seq[6] : 0); }
    { EP e = plain(); e->p[kArpOn] = 1; e->p[kArpLatch] = kLatchFull; e->p[kArpSync] = 0; e->p[kArpRate] = 10; e->noteOn(60, 100); e->noteOn(67, 100); run(*e, SR / 4); e->noteOff(60); e->noteOff(67);
      Buf b = run(*e, SR / 2); CHECK(rms(b.l, SR / 4, SR / 2) > 0.01f, "arpeggiator latch: keeps playing after the keys are released"); }
    // ---- modulation system v2, per voice
    { EP e = plain(); e->p[kO1Wave] = 3; e->setSlot2(0, em::kSrcXlfo1, kDstO1Pitch, 0.02f); e->p[lp_(0, em::kLRate)] = 5; e->noteOn(69, 100); Buf b = run(*e, SR);
      float lo = 1e9, hi = 0; for (int s = 2; s < 18; ++s) { const float f = pitch(b.l, s * SR / 20, s * SR / 20 + SR / 20); lo = std::min(lo, f); hi = std::max(hi, f); }
      CHECK(hi / lo > 1.03f && hi / lo < 1.1f, "XLFO 1 on osc 1 pitch: vibrato between %.0f and %.0f Hz", lo, hi); }
    { auto open = [](float a) { EP e = plain(); e->p[kO1Wave] = 1; e->p[kF1On] = 1; e->p[kF1Freq] = 300; e->p[kF1Peak] = 0; e->setSlot2(0, em::kSrcEg1, kDstF1Freq, a);
          e->p[ep(0, em::kEAttack)] = 0.001f; e->p[ep(0, em::kEDecay)] = 0.3f; e->p[ep(0, em::kESustain)] = 0; e->noteOn(45, 100); Buf b = run(*e, SR / 2);
          return bandH(b.l, 400, 2800, 2640); };
      CHECK(open(0.6f) > open(0) + 20, "EG 1 on filter freq opens the filter at the start of the note (%.0f dB more at 2.6 kHz)", open(0.6f) - open(0)); }
    { auto curve = [](float c) { EP e = plain(); e->p[kO1Wave] = 3; e->setSlot2(0, em::kSrcEg1, kDstO1Level, -1); e->p[kO1Level] = 1;
          e->p[ep(0, em::kEAttack)] = 0.4f; e->p[ep(0, em::kESustain)] = 1; e->p[kEgCurveFirst] = c; e->noteOn(69, 100); Buf b = run(*e, SR / 2); return rms(b.l, (int)(0.1f * SR), (int)(0.15f * SR)); };
      CHECK(curve(-1) > 2 * curve(1), "EG attack curve: slow start keeps the level up early (%.3f), fast start drops it (%.3f)", curve(-1), curve(1)); }
    { EP e = plain(); e->p[kO1Wave] = 3; e->setSlot2(0, em::kSrcXy1X, kDstO1Level, -1); e->p[xyp(0, 0)] = 1; e->noteOn(69, 100); Buf a = run(*e, SR / 4);
      e->p[xyp(0, 0)] = 0; Buf b = run(*e, SR / 4);
      CHECK(rms(a.l, SR / 8, SR / 4) < 0.01f * rms(b.l, SR / 8, SR / 4), "XY pad X on osc 1 level"); }
    { EP e = plain(); e->p[kO1Wave] = 3; e->p[kVoices] = 2; e->setSlot2(0, em::kSrcVelocity, kDstO1Level, -1); e->p[kO1Level] = 1;
      e->noteOn(60, 127); e->noteOn(67, 10); Buf b = run(*e, SR / 2);
      const float hi = band(b.l, SR / 4, SR / 2, 261.6f), lo = band(b.l, SR / 4, SR / 2, 392);
      CHECK(lo - hi > 20, "velocity modulates each voice on its own (soft note %.0f dB louder)", lo - hi); }
    { EP e = plain(); e->p[kO1Wave] = 1; e->p[kRevOn] = 1; e->p[kRevMix] = 0; e->setSlot2(0, em::kSrcWheel, kV2RevMix, 1); e->modWheel(127); e->noteOn(60, 100); run(*e, SR / 4); e->noteOff(60); Buf b = run(*e, SR);
      CHECK(rms(b.l, SR / 2, SR) > 1e-3f, "mod wheel on the reverb amount (a whole-synth destination)"); }
    { EP e = plain(); e->p[kO1Wave] = 3; for (int s = 0; s < kNumSlots2; ++s) e->setSlot2(s, em::kSrcXlfo2, kDstO1Pitch, 0.0002f); e->noteOn(69, 100); Buf b = run(*e, SR / 2);
      CHECK(rms(b.l, SR / 4, SR / 2) > 0.05f, "all 100 slots at once"); }
    { EP e = plain(); e->p[kO1Wave] = 3; e->p[kPolyMode] = 0; e->setSlot2(0, em::kSrcEf1, kDstO1Pitch, 0.5f); e->p[fp(0, em::kFGain)] = 24;
      e->noteOn(69, 100); Buf b = run(*e, SR / 2); CHECK(pitch(b.l, SR / 4, SR / 2) > 500, "envelope follower on its own voice raises the pitch (%.0f Hz)", pitch(b.l, SR / 4, SR / 2)); }
    printf(fails ? "FAILED (%d failures)\n" : "ALL PASSED (%d failures)\n", fails);
    return fails;
}
