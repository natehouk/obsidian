// Obsidian — polyphonic synthesizer engine.
// 2.x: two oscillators, two filters, three envelopes, three LFOs and a 24-slot modulation matrix.
// 3.0: four oscillators (sine and noise, sync ratio, ring modulation, drift), four filters in eleven
// styles with bell / shelf / notch shapes and 6 to 48 dB slopes, per-oscillator polyphony, 64 voices,
// 4x oversampling, an effects section, an arpeggiator, and the Epsilon modulation system (XLFOs,
// envelopes with curves, followers, XY pads, MIDI sources, 100 slots) evaluated per voice.
// When none of the 3.0 features is in use, the 2.x code runs unchanged, so old sessions render
// sample for sample as before.
// Clean-room implementation from public, generic techniques: PolyBLEP oscillators, cascaded TPT
// state-variable filters (Zavalishin) with per-style nonlinearities, one-pole envelope segments.
// No third-party product code, assets or disassembly were used. License: MIT (see LICENSE).

#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "common/ModSystem.hpp"
#include "common/Halfband.hpp"
#include "SynthFx.hpp"
#include "Arp.hpp"

namespace obsidian {

static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }

namespace em = epsilon::mod;

static constexpr int kNumSlots = 24;                 // classic (2.x) slots
static constexpr int kNumSlots2 = 100;               // modulation system v2 slots
static constexpr int kMaxVoicesLegacy = 24;
static constexpr int kMaxVoices = 64;
static constexpr int kNumOsc = 4, kNumFilters = 4;

// ------------------------------------------------------------------ params (append-only)
#define OSC(n)  kO##n##Wave, kO##n##Oct, kO##n##Semi, kO##n##Fine, kO##n##PW, kO##n##Level, kO##n##Pan, kO##n##Phase
#define FLT(n)  kF##n##On, kF##n##Style, kF##n##Type, kF##n##Slope, kF##n##Freq, kF##n##Peak, kF##n##Pan
#define ENV(n)  kE##n##Delay, kE##n##Attack, kE##n##Decay, kE##n##Sustain, kE##n##Release, kE##n##Invert
#define LFOP(n) kL##n##Wave, kL##n##Balance, kL##n##Sync, kL##n##Rate, kL##n##Div, kL##n##NoteSync
enum OField { kOWave, kOOct, kOSemi, kOFine, kOPW, kOLevel, kOPan, kOPhase, kOFields };
enum FField { kFOn, kFStyle, kFType, kFSlope, kFFreq, kFPeak, kFPan, kFBaseFields, kFGain = kFBaseFields };
enum Param : uint32_t {
    OSC(1), OSC(2), kOscSync,
    FLT(1), FLT(2), kRouting,
    ENV(1), ENV(2),
    kE3Attack, kE3Hold, kE3Decay, kE3Sustain, kE3Release, kAmpVel,
    LFOP(1), LFOP(2), LFOP(3),
    kVoices, kUnison, kUniDetune, kUniSpread, kGlide, kGlideMode, kBendRange, kVolume,
    kSlotFirst,
    kSlotLast = kSlotFirst + kNumSlots * 4 - 1,
    kMidiOn,
    kOutLearn,
    // ---- 3.0
    OSC(3), OSC(4),
    kOscOnFirst,                                     // 4: oscillator on
    kOscRatioFirst = kOscOnFirst + kNumOsc,          // 4: hard sync ratio (1 = off)
    kRing12 = kOscRatioFirst + kNumOsc, kRing34,
    kDrift, kTune, kTranspose, kPolyMode, kHq,
    FLT(3), FLT(4),
    kFGainFirst,                                     // 4: bell / shelf gain
    kFreqOffset = kFGainFirst + kNumFilters, kPeakOffset,
    kRevOn, kRevMix, kRevTime, kRevBright, kRevPredelay,
    kDlyOn, kDlyMix, kDlySync, kDlyTime, kDlyDiv, kDlyOffset, kDlyFeedback, kDlyPingPong, kDlyLowCut, kDlyHighCut,
    kChoOn, kChoMix, kChoMode, kChoRate, kChoDepth, kChoDelay,
    kPhsOn, kPhsMix, kPhsMode, kPhsRate, kPhsDepth, kPhsFeedback, kPhsShift,
    kDrvOn, kDrvAmount, kDrvPost,
    kCmpOn, kCmpThreshold, kCmpRatio, kCmpAttack, kCmpRelease, kCmpAutoGain, kCmpMix,
    kArpOn, kArpRate, kArpSync, kArpDiv, kArpOffset, kArpOrder, kArpOctaves, kArpGroove, kArpGate, kArpLatch, kArpRetrig, kArpLock,
    kXlfoFirst,
    kEgFirst = kXlfoFirst + em::kNumXlfo * em::kXlfoFields,
    kEfFirst = kEgFirst + em::kNumEg * em::kEgFields,
    kXyFirst = kEfFirst + em::kNumEf * em::kEfFields,
    kSlot2First = kXyFirst + em::kNumXy * 2,
    kXlfoRetrigFirst = kSlot2First + kNumSlots2 * em::kSlotFields,   // 4: XLFO restarts per note (per voice)
    kEgCurveFirst = kXlfoRetrigFirst + em::kNumXlfo,                   // 2 x 3: attack, decay, release curve
    kEfModeFirst = kEgCurveFirst + em::kNumEg * 3,                     // 2: level / transient
    kParamCount = kEfModeFirst + em::kNumEf
};
#undef OSC
#undef FLT
#undef ENV
#undef LFOP
enum SlotField { kSlotSrc = 0, kSlotDst = 1, kSlotAmt = 2, kSlotBypass = 3 };
inline uint32_t slotParam(int slot, int field) { return kSlotFirst + slot * 4 + field; }
inline uint32_t opar(int o, int field) {
    static const uint32_t b[kNumOsc] = { kO1Wave, kO2Wave, kO3Wave, kO4Wave };
    return b[o & 3] + (uint32_t)field;
}
inline uint32_t fpar(int f, int field) {
    static const uint32_t b[kNumFilters] = { kF1On, kF2On, kF3On, kF4On };
    return field == kFGain ? kFGainFirst + (uint32_t)f : b[f & 3] + (uint32_t)field;
}
inline uint32_t lp_(int l, int f) { return kXlfoFirst + l * em::kXlfoFields + f; }
inline uint32_t ep(int e, int f) { return kEgFirst + e * em::kEgFields + f; }
inline uint32_t fp(int e, int f) { return kEfFirst + e * em::kEfFields + f; }
inline uint32_t xyp(int x, int axis) { return kXyFirst + x * 2 + axis; }
inline uint32_t sp(int s, int f) { return kSlot2First + s * em::kSlotFields + f; }

// classic (2.x) sources and destinations
enum Source { kSrcNone, kSrcLfo1, kSrcLfo2, kSrcLfo3, kSrcEg1, kSrcEg2, kSrcEg3, kSrcVelocity, kSrcAftertouch,
              kSrcKey, kSrcBend, kSrcWheel, kNumSources };

enum Dest {
    kDstNone,
    kDstO1Pitch, kDstO1PW, kDstO1Level, kDstO1Pan,
    kDstO2Pitch, kDstO2PW, kDstO2Level, kDstO2Pan,
    kDstF1Freq, kDstF1Peak, kDstF1Pan, kDstF2Freq, kDstF2Peak, kDstF2Pan,
    kDstE1Delay, kDstE1Attack, kDstE1Decay, kDstE1Sustain, kDstE1Release,
    kDstE2Delay, kDstE2Attack, kDstE2Decay, kDstE2Sustain, kDstE2Release,
    kDstE3Attack, kDstE3Hold, kDstE3Decay, kDstE3Sustain, kDstE3Release,
    kDstL1Rate, kDstL1Balance, kDstL2Rate, kDstL2Balance, kDstL3Rate, kDstL3Balance,
    kDstVolume, kDstUniDetune,
    kDstSlotFirst,
    kNumDests = kDstSlotFirst + kNumSlots
};
// v2 destinations: 1..37 are the classic destinations (same units), then the 3.0 ones
enum Dest2 {
    kV2O3First = kDstSlotFirst,                          // osc 3 and 4: pitch, PW, level, pan
    kV2RatioFirst = kV2O3First + 8,                      // osc 1-4 sync ratio
    kV2Tune = kV2RatioFirst + kNumOsc, kV2Glide, kV2Spread,
    kV2F3First,                                          // filters 3 and 4: freq, peak, pan
    kV2GainFirst = kV2F3First + 6,                       // filters 1-4 gain
    kV2FreqOffset = kV2GainFirst + kNumFilters, kV2PeakOffset,
    kV2RevMix, kV2RevTime, kV2RevBright, kV2RevPredelay,
    kV2DlyMix, kV2DlyTime, kV2DlyFeedback,
    kV2ChoMix, kV2ChoRate, kV2ChoDepth,
    kV2PhsMix, kV2PhsRate, kV2PhsDepth, kV2PhsFeedback,
    kV2DrvAmount, kV2CmpThreshold, kV2CmpMix,
    kV2ArpRate,
    kV2XlfoRateFirst, kV2XlfoBalFirst = kV2XlfoRateFirst + em::kNumXlfo,
    kV2EgFirst = kV2XlfoBalFirst + em::kNumXlfo,         // A, D, S, R per EG
    kV2SlotFirst = kV2EgFirst + em::kNumEg * 4,
    kNumDests2 = kV2SlotFirst + kNumSlots2
};
static constexpr int kV2GlobalFirst = kV2RevMix, kV2GlobalLast = kV2ArpRate;   // destinations of the whole synth, not per voice

static constexpr int kNumStyles = 11, kNumStyles2 = 4;
static constexpr int kNumWaves = 6;                     // triangle, saw, square, sine, white noise, pink noise
enum Shape { kLowPass, kBandPass, kHighPass, kBell, kLowShelf, kHighShelf, kNotch, kNumShapes };
enum Slope { kSlope12, kSlope24, kSlope48, kSlope6, kNumSlopes };
enum Routing { kSerial, kParallel, kSplit, kNumRoutings };
static constexpr int kNumDivs = 11;
inline float divBars(int i) {
    static const float t[kNumDivs] = { 16, 8, 4, 2, 1, 1.f/2, 1.f/4, 1.f/8, 1.f/16, 1.f/32, 1.f/64 };
    return t[i < 0 ? 0 : (i >= kNumDivs ? kNumDivs - 1 : i)];
}

// ------------------------------------------------------------------ filter
//  nl: 0 linear, 1 soft-limited state, 2 tanh integrator, 3 hard-clipped state; in: 0 as 2.x, 1 asymmetric tube, 2 hard clip
struct StyleDef { float drive; int nl; float kMin; float bassLoss; int in; float scoop; };
inline const StyleDef& styleDef(int s) {
    static const StyleDef t[kNumStyles] = {
        { 1.2f, 0,  0.30f, 0, 0, 0 },     // Smooth   : moderate overdrive, broad resonance
        { 2.0f, 2, -0.05f, 0, 0, 0 },     // Hot      : saturating integrators, screaming
        { 3.0f, 3, -0.08f, 0, 0, 0 },     // Raw      : hard-clipped feedback, buzzy self-oscillation
        { 1.2f, 1, -0.04f, 0, 0, 0 },     // Monolith : Monolith's soft-limited self-oscillating filter
        // 3.0
        { 1.0f, 0,  0.08f, 0, 0, 0 },     // Clean    : linear, never self-oscillates
        { 1.6f, 0,  0.04f, 0, 0, 0 },     // Warm     : saturated input, linear filter
        { 1.8f, 1, -0.06f, 0.5f, 0, 0 },  // Acid     : resonance thins the bass
        { 1.5f, 2, -0.02f, 0, 1, 0 },     // Tube     : asymmetric warm overdrive
        { 1.15f, 0, 0.18f, 0, 0, 0 },     // Gentle   : clean, broad, polite
        { 2.6f, 3, -0.10f, 0, 2, 0 },     // Metal    : hard-clipped input and feedback
        { 1.3f, 1, -0.06f, 0, 0, 0.6f },  // Hollow   : the band at the cutoff is carved out
    };
    return t[s < 0 ? 0 : (s >= kNumStyles ? kNumStyles - 1 : s)];
}
inline const char* styleName(int s) {
    static const char* const n[kNumStyles] = { "Smooth", "Hot", "Raw", "Monolith", "Clean", "Warm", "Acid", "Tube", "Gentle", "Metal", "Hollow" };
    return n[s < 0 ? 0 : (s >= kNumStyles ? kNumStyles - 1 : s)];
}
inline const float* bwK(int stages) {
    static const float k1[1] = { 1.41421356f };
    static const float k2[2] = { 1.84775907f, 0.76536686f };
    static const float k4[4] = { 1.96157056f, 1.66293922f, 1.11114047f, 0.39018064f };
    return stages == 1 ? k1 : (stages == 2 ? k2 : k4);
}
inline int slopeStages(int slope) { return slope == kSlope12 ? 1 : (slope == kSlope24 ? 2 : (slope == kSlope48 ? 4 : 0)); }
inline float eqDamping(float reso) { return 1.41421356f * (1.f - 0.85f * std::pow(clampf(reso, 0.f, 1.f), 0.8f)); }
inline bool isEqShape(int t) { return t == kBell || t == kLowShelf || t == kHighShelf; }
// a fast tanh (rational, exact at 0 and saturating at +-3) for the 3.0 filter paths
inline float ftanh(float x) { x = clampf(x, -3.f, 3.f); const float x2 = x * x; return x * (27.f + x2) / (27.f + 9.f * x2); }

struct FilterChan {
    float ic1[4] = {0, 0, 0, 0}, ic2[4] = {0, 0, 0, 0}, z1 = 0, dcx = 0, dcy = 0;
    void reset() { for (int i = 0; i < 4; ++i) ic1[i] = ic2[i] = 0; z1 = dcx = dcy = 0; }
    // 2.x: r = resonance already shaped as reso^0.6 (computed at modulation rate by the caller)
    inline float process(float x, float g, float r, int type, int stages, int style) {
        const StyleDef& sd = styleDef(style);
        if (sd.nl == 3) x = (x * sd.drive) / (1.f + std::fabs(x * sd.drive)) / sd.drive * 1.1f;
        else            x = std::tanh(x * sd.drive) / sd.drive * 1.1f;
        const float* kb = bwK(stages);
        for (int i = 0; i < stages; ++i) {
            const float k = i == stages - 1 ? kb[i] * (1.f - r) + sd.kMin * r : kb[i];
            const float a1 = 1.f / (1.f + g * (g + k));
            const float v3 = x - ic2[i];
            const float v1 = a1 * ic1[i] + g * a1 * v3;
            const float v2 = ic2[i] + g * v1;
            switch (sd.nl) {
            case 1: ic1[i] = 2.f * v1 - ic1[i]; ic1[i] = ic1[i] / std::sqrt(1.f + 0.08f * ic1[i] * ic1[i]); break;
            case 2: ic1[i] = 2.f * 2.5f * std::tanh(v1 / 2.5f) - ic1[i]; break;
            case 3: ic1[i] = clampf(2.f * v1 - ic1[i], -1.f, 1.f); break;
            default: ic1[i] = 2.f * v1 - ic1[i]; break;
            }
            ic2[i] = 2.f * v2 - ic2[i];
            x = type == 0 ? v2 : (type == 2 ? x - k * v1 - v2 : v1 * std::max(std::fabs(k), 0.12f));
        }
        return x;
    }
    inline void svf(int i, float x, float g, float k, int nl, float& v1, float& v2) {
        const float a1 = 1.f / (1.f + g * (g + k));
        const float v3 = x - ic2[i];
        v1 = a1 * ic1[i] + g * a1 * v3;
        v2 = ic2[i] + g * v1;
        switch (nl) {
        case 1: ic1[i] = 2.f * v1 - ic1[i]; ic1[i] = ic1[i] / std::sqrt(1.f + 0.08f * ic1[i] * ic1[i]); break;
        case 2: ic1[i] = 2.f * 2.5f * ftanh(v1 / 2.5f) - ic1[i]; break;
        case 3: ic1[i] = clampf(2.f * v1 - ic1[i], -1.f, 1.f); break;
        default: ic1[i] = 2.f * v1 - ic1[i]; break;
        }
        ic2[i] = 2.f * v2 - ic2[i];
    }
    // 3.0: every style, shape and slope. stages 0 = 6 dB/oct; reso = raw peak (0..1); eqGain linear
    inline float process3(float x, float g, float r, float reso, int type, int stages, int style, float eqGain) {
        if (style < kNumStyles2 && type <= kHighPass && (stages == 1 || stages == 2)) return process(x, g, r, type, stages, style);
        const StyleDef& sd = styleDef(style);
        if (sd.drive != 1.f) {
            if (sd.in == 1) { const float b = 0.25f, y = (ftanh(sd.drive * x + b) - 0.2454649f) / sd.drive * 1.1f; const float o = y - dcx + 0.9995f * dcy; dcx = y; dcy = o; x = o; }
            else if (sd.in == 2) x = clampf(x * sd.drive, -1.f, 1.f) / sd.drive * 1.1f;
            else if (sd.nl == 3) x = (x * sd.drive) / (1.f + std::fabs(x * sd.drive)) / sd.drive * 1.1f;
            else x = ftanh(x * sd.drive) / sd.drive * 1.1f;
        }
        if (sd.bassLoss > 0.f && type <= kHighPass) x *= 1.f - sd.bassLoss * r;
        bool wide = false;
        if (stages == 0) {
            if (type == kLowPass || type == kHighPass || type == kLowShelf || type == kHighShelf) {
                const float G = g / (1.f + g), v = (x - z1) * G, lo = v + z1; z1 = lo + v;
                const float hi = x - lo;
                switch (type) { case kLowPass: return lo; case kHighPass: return hi; case kLowShelf: return x + (eqGain - 1.f) * lo; default: return x + (eqGain - 1.f) * hi; }
            }
            wide = type == kBell || type == kNotch;
            stages = 1;
        }
        if (type <= kHighPass) {
            const float* kb = bwK(stages);
            for (int i = 0; i < stages; ++i) {
                const bool last = i == stages - 1;
                const float k = last ? kb[i] * (1.f - r) + sd.kMin * r : kb[i];
                float v1, v2; svf(i, x, g, k, sd.nl, v1, v2);
                float y = type == kLowPass ? v2 : (type == kHighPass ? x - k * v1 - v2 : v1 * std::max(std::fabs(k), 0.12f));
                if (last && sd.scoop > 0.f) y -= sd.scoop * r * v1 * std::max(std::fabs(k), 0.12f);
                x = y;
            }
            return x;
        }
        const float k = std::max(eqDamping(reso) * (wide ? 2.f : 1.f) + std::min(0.f, sd.kMin) * r, 0.02f);
        const float gs = stages == 1 ? eqGain : std::pow(eqGain, 1.f / stages);
        for (int i = 0; i < stages; ++i) {
            float v1, v2; svf(i, x, g, k, sd.nl, v1, v2);
            switch (type) {
            case kBell: x = x + (gs - 1.f) * k * v1; break;
            case kLowShelf: x = x + (gs - 1.f) * v2; break;
            case kHighShelf: x = x + (gs - 1.f) * (x - k * v1 - v2); break;
            default: x = x - k * v1; break;                  // notch
            }
        }
        return x;
    }
};

inline float polyBlep(float t, float dt) {
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}

// ------------------------------------------------------------------ envelope: delay / attack / hold / decay / sustain / release
struct Env {
    enum Stage { Idle, Delay, Attack, Hold, Decay, Sustain, Release } stage = Idle;
    float value = 0, s = 0.5f, cA = 0, cD = 0, cR = 0; int delayN = 0, holdN = 0, count = 0;
    static float coef(float t, float sr) { return 1.f - std::exp(-1.f / std::max(t * sr, 1.f)); }
    void set(float sr, float delay, float a, float hold, float d, float sus, float r) {
        delayN = (int)(delay * sr); holdN = (int)(hold * sr); s = clampf(sus, 0.f, 1.f);
        cA = coef(a / 1.5f, sr); cD = coef(d / 4.f, sr); cR = coef(r / 4.f, sr);
    }
    void gateOn() { count = 0; stage = delayN > 0 ? Delay : Attack; }
    void gateOff() { if (stage != Idle) stage = Release; }
    bool active() const { return stage != Idle; }
    float process() {
        switch (stage) {
        case Delay:   if (++count >= delayN) { stage = Attack; count = 0; } break;
        case Attack:  value += (1.2f - value) * cA; if (value >= 1.f) { value = 1.f; stage = holdN > 0 ? Hold : Decay; count = 0; } break;
        case Hold:    if (++count >= holdN) stage = Decay; break;
        case Decay:   value += (s - value) * cD; if (std::fabs(value - s) < 1e-4f) { value = s; stage = Sustain; } break;
        case Sustain: value = s; break;
        case Release: value -= value * cR; if (value < 1e-5f) { value = 0; stage = Idle; } break;
        case Idle:    value = 0; break;
        }
        return value;
    }
};

// 3.0 envelope generator: attack, decay and release with adjustable curves (-1 slow start .. 0 linear .. 1 fast start)
struct CurveEnv {
    enum Stage { Idle, Attack, Decay, Sustain, Release } stage = Idle;
    float value = 0, from = 0, t = 0;
    static float shape(float t, float c) {
        if (std::fabs(c) < 1e-3f) return t;
        const float k = 6.f * c;
        return k > 0 ? (1.f - std::exp(-k * t)) / (1.f - std::exp(-k)) : (std::exp(-k * t) - 1.f) / (std::exp(-k) - 1.f);
    }
    void gate(bool on) { if (on) { stage = Attack; from = value; t = 0; } else if (stage != Idle) { stage = Release; from = value; t = 0; } }
    bool gated() const { return stage == Attack || stage == Decay || stage == Sustain; }
    // dt: seconds per call
    float process(float dt, float A, float D, float S, float R, float cA, float cD, float cR) {
        S = clampf(S, 0.f, 1.f);
        switch (stage) {
        case Attack:  t += dt / std::max(A, 0.0005f); if (t >= 1.f) { value = 1.f; stage = Decay; from = 1.f; t = 0; } else value = from + (1.f - from) * shape(t, cA); break;
        case Decay:   t += dt / std::max(D, 0.0005f); if (t >= 1.f) { value = S; stage = Sustain; } else value = from + (S - from) * shape(t, cD); break;
        case Sustain: value = S; break;
        case Release: t += dt / std::max(R, 0.0005f); if (t >= 1.f) { value = 0; stage = Idle; } else value = from * (1.f - shape(t, cR)); break;
        case Idle:    value = 0; break;
        }
        return value;
    }
};

// ------------------------------------------------------------------ LFO (classic)
struct Lfo {
    double phase = 0; float rndA = 0, rndB = 0, held = 0, out = 0; uint32_t rng = 1;
    float rnd() { rng = rng * 1664525u + 1013904223u; return (int32_t)rng * (1.f / 2147483648.f); }
    static float warp(float t, float bal) {
        bal = clampf(bal, 0.02f, 0.98f);
        return t < bal ? 0.5f * t / bal : 0.5f + 0.5f * (t - bal) / (1.f - bal);
    }
    // wave: 0 triangle, 1 sine, 2 square, 3 random triangle, 4 sample & hold
    float value(int wave, float bal) const {
        const float t = (float)phase;
        switch (wave) {
        case 1: return -std::cos(2.f * kPi * warp(t, bal));
        case 2: return t < clampf(bal, 0.02f, 0.98f) ? 1.f : -1.f;
        case 3: return rndA + (rndB - rndA) * t;
        case 4: return held;
        default: { const float u = warp(t, bal); return u < 0.5f ? -1.f + 4.f * u : 3.f - 4.f * u; }
        }
    }
    void advance(double inc) {
        phase += inc;
        if (phase >= 1.0) { phase -= std::floor(phase); rndA = rndB; rndB = rnd(); held = rnd(); }
    }
};

struct Voice {
    int note = -1, group = -1;
    bool keyDown = false, sustained = false;
    float vel = 1, at = 0, curNote = 60, targetNote = 60, detune = 0, panOff = 0;
    float ph[kNumOsc] = {0, 0, 0, 0}, tri[kNumOsc] = {0, 0, 0, 0};
    uint32_t age = 0;
    FilterChan flt[kNumFilters][2];
    Env eg[3];
    Lfo lfo[3];
    float d[kNumDests] = {};          // latest modulation sums (used at control rate)
    struct Cache { float dt[kNumOsc] = {0.01f, 0.01f, 0.01f, 0.01f}, pw[kNumOsc] = {0.5f, 0.5f, 0.5f, 0.5f}, gl[kNumOsc] = {0, 0, 0, 0}, gr[kNumOsc] = {0, 0, 0, 0},
                   fg[kNumFilters][2] = {{0.1f, 0.1f}, {0.1f, 0.1f}, {0.1f, 0.1f}, {0.1f, 0.1f}}, pk[kNumFilters] = {0, 0, 0, 0}, vol = 1,
                   pkRaw[kNumFilters] = {0, 0, 0, 0}, gain[kNumFilters] = {1, 1, 1, 1}, ratio[kNumOsc] = {1, 1, 1, 1}, glide = 1; } c;
    int ctl = 0, sub = 0;
    // 3.0
    float mph[kNumOsc] = {0, 0, 0, 0}, drift[kNumOsc] = {0, 0, 0, 0}, driftTarget[kNumOsc] = {0, 0, 0, 0};
    float pink[7] = {0, 0, 0, 0, 0, 0, 0}; uint32_t rng = 1;
    int oscMask = 0xF;
    em::Xlfo xl[em::kNumXlfo]; CurveEnv eg2[em::kNumEg];
    float efFast[em::kNumEf] = {0, 0}, efSlow[em::kNumEf] = {0, 0}, outLevel = 0;
    float src2[em::kNumSources] = {};
    float d2[kNumDests2] = {};
    bool mono = false;
    bool active() const { return eg[2].active(); }
};

class Engine {
public:
    float p[kParamCount];

    Engine() {
        std::fill(p, p + kParamCount, 0.f);
        p[kO1Wave] = 1; p[kO1PW] = 0.5f; p[kO1Level] = 1; p[kO1Phase] = 0;
        p[kO2Wave] = 1; p[kO2PW] = 0.5f; p[kO2Level] = 0; p[kO2Fine] = 7;
        p[kF1On] = 1; p[kF1Style] = 0; p[kF1Type] = 0; p[kF1Slope] = 1; p[kF1Freq] = 2500; p[kF1Peak] = 0.2f;
        p[kF2On] = 0; p[kF2Style] = 0; p[kF2Type] = 2; p[kF2Slope] = 0; p[kF2Freq] = 60; p[kF2Peak] = 0.1f;
        p[kE1Attack] = 0.005f; p[kE1Decay] = 0.4f; p[kE1Sustain] = 0.3f; p[kE1Release] = 0.3f;
        p[kE2Attack] = 0.2f;   p[kE2Decay] = 0.8f; p[kE2Sustain] = 0.5f; p[kE2Release] = 0.6f;
        p[kE3Attack] = 0.004f; p[kE3Decay] = 0.5f; p[kE3Sustain] = 0.85f; p[kE3Release] = 0.25f; p[kAmpVel] = 0.4f;
        p[kL1Wave] = 0; p[kL1Balance] = 0.5f; p[kL1Rate] = 5;   p[kL1Div] = 7;
        p[kL2Wave] = 1; p[kL2Balance] = 0.5f; p[kL2Rate] = 0.5f; p[kL2Div] = 4;
        p[kL3Wave] = 4; p[kL3Balance] = 0.5f; p[kL3Rate] = 8;   p[kL3Div] = 8;
        p[kVoices] = 8; p[kUnison] = 1; p[kUniDetune] = 0.15f; p[kUniSpread] = 0.5f;
        p[kGlide] = 0; p[kGlideMode] = 0; p[kBendRange] = 2; p[kVolume] = -6;
        p[kMidiOn] = 1;
        // default slots: half key tracking on filter 1, EG 1 opens the filter,
        // mod wheel brings in LFO 1 vibrato (by modulating the amounts of slots 3 and 4)
        setSlot(0, kSrcKey,   kDstF1Freq, 0.5f);
        setSlot(1, kSrcEg1,   kDstF1Freq, 0.3f);
        setSlot(2, kSrcLfo1,  kDstO1Pitch, 0.f);
        setSlot(3, kSrcLfo1,  kDstO2Pitch, 0.f);
        setSlot(4, kSrcWheel, kDstSlotFirst + 2, 0.015f);
        setSlot(5, kSrcWheel, kDstSlotFirst + 3, 0.015f);
        // ---- 3.0 defaults: everything new is off or neutral
        p[kO3Wave] = 1; p[kO3PW] = 0.5f; p[kO3Level] = 0.8f; p[kO4Wave] = 3; p[kO4PW] = 0.5f; p[kO4Level] = 0.8f; p[kO4Oct] = -1;
        p[kOscOnFirst] = 1; p[kOscOnFirst + 1] = 1;
        for (int o = 0; o < kNumOsc; ++o) p[kOscRatioFirst + o] = 1;
        p[kF3On] = 0; p[kF3Style] = 4; p[kF3Type] = kBell; p[kF3Slope] = 0; p[kF3Freq] = 1000; p[kF3Peak] = 0.3f;
        p[kF4On] = 0; p[kF4Style] = 4; p[kF4Type] = kHighShelf; p[kF4Slope] = 0; p[kF4Freq] = 6000; p[kF4Peak] = 0.2f;
        p[kRevMix] = 0.25f; p[kRevTime] = 2.f; p[kRevBright] = 0.5f; p[kRevPredelay] = 0.02f;
        p[kDlyMix] = 0.25f; p[kDlySync] = 1; p[kDlyTime] = 0.375f; p[kDlyDiv] = 2; p[kDlyOffset] = 0; p[kDlyFeedback] = 0.4f; p[kDlyLowCut] = 150; p[kDlyHighCut] = 6000;
        p[kChoMix] = 0.5f; p[kChoRate] = 0.6f; p[kChoDepth] = 0.5f; p[kChoDelay] = 0.012f;
        p[kPhsMix] = 0.5f; p[kPhsRate] = 0.3f; p[kPhsDepth] = 0.7f; p[kPhsFeedback] = 0.5f; p[kPhsShift] = 0.25f;
        p[kDrvAmount] = 0.3f;
        p[kCmpThreshold] = -18; p[kCmpRatio] = 3; p[kCmpAttack] = 0.01f; p[kCmpRelease] = 0.15f; p[kCmpAutoGain] = 1; p[kCmpMix] = 1;
        p[kArpRate] = 8; p[kArpSync] = 1; p[kArpDiv] = 4; p[kArpOffset] = 1; p[kArpGate] = 0.6f;
        for (int l = 0; l < em::kNumXlfo; ++l) for (int f = 0; f < em::kXlfoFields; ++f) p[lp_(l, f)] = f < em::kLStep1 ? em::xlfoRange(f).def : ((f - em::kLStep1) % 4) / 1.5f - 1.f;
        for (int e = 0; e < em::kNumEg; ++e) for (int f = 0; f < em::kEgFields; ++f) p[ep(e, f)] = em::egRange(f).def;
        for (int e = 0; e < em::kNumEg; ++e) p[ep(e, em::kETrigger)] = 0;     // note-triggered
        for (int e = 0; e < em::kNumEf; ++e) for (int f = 0; f < em::kEfFields; ++f) p[fp(e, f)] = em::efRange(f).def;
        for (int x = 0; x < em::kNumXy * 2; ++x) p[kXyFirst + x] = 0.5f;
        for (int e = 0; e < em::kNumEg; ++e) { p[kEgCurveFirst + e * 3] = 0; p[kEgCurveFirst + e * 3 + 1] = 0.5f; p[kEgCurveFirst + e * 3 + 2] = 0.5f; }
        for (int l = 0; l < em::kNumXlfo; ++l) p[kXlfoRetrigFirst + l] = 1;
    }
    void setSlot(int s, int src, int dst, float amt) {
        p[slotParam(s, kSlotSrc)] = (float)src; p[slotParam(s, kSlotDst)] = (float)dst;
        p[slotParam(s, kSlotAmt)] = amt; p[slotParam(s, kSlotBypass)] = 0;
    }
    void setSlot2(int s, int src, int dst, float amt) { p[sp(s, kSlotSrc)] = (float)src; p[sp(s, kSlotDst)] = (float)dst; p[sp(s, kSlotAmt)] = amt; p[sp(s, kSlotBypass)] = 0; }

    void setSampleRate(float sr) {
        fs = sr; smooth = 1.f - std::exp(-1.f / (0.01f * sr));
        rev.init(sr); dly.init(sr); cho.init(sr); phs.init(sr); cmp.init(sr);
        reset();
    }
    void reset() {
        uint32_t seed = 12345;
        for (Voice& v : voices) {
            v = Voice();
            for (Lfo& l : v.lfo) { seed = seed * 1664525u + 1013904223u; l.rng = seed | 1; l.rndB = l.rnd(); }
        }
        uint32_t s2 = 4242;
        for (Voice& v : voices) { s2 = s2 * 1664525u + 1013904223u; v.rng = s2 | 1; for (em::Xlfo& x : v.xl) { s2 = s2 * 1664525u + 1013904223u; x.rng = s2 | 1; x.rndB = x.rnd(); } }
        held = 0; groupCounter = 0; lastNote = -1; sustainDown = false; dcL = dcR = dcXl = dcXr = 0;
        for (double& g : gLfo) g = 0;
        for (em::Xlfo& x : gXl) { x = em::Xlfo(); }
        rev.clear(); dly.clear(); cho.clear(); phs.clear(); cmp.clear(); ovsL.reset(); ovsR.reset();
        arp.reset(); arpRunning = false; synthLevel = 0; nextOsc = 0; gMod = 0;
        std::fill(offG, offG + kNumDests2, 0.f);
    }
    void setTransport(bool valid, bool playing, double barPos, double bpm, double bpb) {
        tValid = valid; tPlaying = playing; tBarPos = barPos; tBpm = bpm > 1 ? bpm : 120; tBpb = bpb > 0 ? bpb : 4; tSamples = 0;
    }

    // does this setting use anything new in 3.0? (if not, the 2.x code runs)
    bool usesV3() const {
        if (p[kOscOnFirst] < 0.5f || p[kOscOnFirst + 1] < 0.5f || p[kOscOnFirst + 2] > 0.5f || p[kOscOnFirst + 3] > 0.5f) return true;
        for (int o = 0; o < kNumOsc; ++o) if (p[kOscRatioFirst + o] != 1.f) return true;
        if (p[kO1Wave] > 2.5f || p[kO2Wave] > 2.5f) return true;
        if (p[kRing12] > 0.5f || p[kRing34] > 0.5f || p[kDrift] != 0.f || p[kTune] != 0.f || p[kTranspose] != 0.f || p[kPolyMode] > 0.5f || p[kHq] > 0.5f) return true;
        if (p[kF3On] > 0.5f || p[kF4On] > 0.5f || p[kFreqOffset] != 0.f || p[kPeakOffset] != 0.f) return true;
        for (int f = 0; f < 2; ++f) if (p[fpar(f, kFStyle)] > 3.5f || p[fpar(f, kFType)] > 2.5f || p[fpar(f, kFSlope)] > 1.5f) return true;
        if (p[kRevOn] > 0.5f || p[kDlyOn] > 0.5f || p[kChoOn] > 0.5f || p[kPhsOn] > 0.5f || p[kDrvOn] > 0.5f || p[kCmpOn] > 0.5f || p[kArpOn] > 0.5f) return true;
        if (p[kVoices] > kMaxVoicesLegacy + 0.5f || p[kUnison] > 4.5f) return true;
        for (int s = 0; s < kNumSlots2; ++s) if (p[sp(s, kSlotSrc)] > 0.5f && p[sp(s, kSlotDst)] > 0.5f && p[sp(s, kSlotBypass)] < 0.5f) return true;
        return false;
    }

    // ---------------------------------------------------------------- MIDI
    void noteOn(int note, int vel) {
        if (p[kMidiOn] < 0.5f) return;
        lastVel = vel / 127.f; lastKey = note;
        if (p[kArpOn] > 0.5f) { arp.keyOn(note, vel, (int)std::round(p[kArpLatch])); ++keysDown; return; }
        ++keysDown;
        voiceOn(note, vel);
    }
    void noteOff(int note) {
        if (p[kMidiOn] < 0.5f) return;
        keysDown = std::max(0, keysDown - 1);
        if (p[kArpOn] > 0.5f) {
            arp.keyOff(note, (int)std::round(p[kArpLatch]));
            if (note != arp.playing) for (const Voice& x : voices) if (x.note == note && x.keyDown) { voiceOff(note); break; }   // a key held from before the arpeggiator was on
            return;
        }
        voiceOff(note);
    }
    void sustain(bool down) {
        sustainDown = down;
        if (!down) for (Voice& x : voices) if (x.sustained) { x.sustained = false; release(x); }
    }
    void allNotesOff() { held = 0; sustainDown = false; keysDown = 0; arp.reset(); arpRunning = false; for (Voice& x : voices) { x.keyDown = x.sustained = false; release(x); } }
    void allSoundOff() {
        for (Voice& x : voices) { x.eg[2].stage = Env::Idle; x.eg[2].value = 0; x.note = -1; x.group = -1; } held = 0; keysDown = 0;
        arp.reset(); arpRunning = false;
        rev.clear(); dly.clear(); cho.clear(); phs.clear();
    }
    void pitchBend(int v14) { bend = (v14 - 8192) / 8192.f; }
    void modWheel(int v) { wheel = v / 127.f; }
    void channelPressure(int v) { chanAT = v / 127.f; }
    void polyPressure(int note, int v) { for (Voice& x : voices) if (x.note == note) x.at = v / 127.f; }

    int activeVoices() const { int n = 0; for (const Voice& v : voices) n += v.active(); return n; }
    int activeNotes() const { return countGroups(); }
    uint32_t latency() const { return p[kHq] > 0.5f ? (uint32_t)(epsilon::Oversampler::latency(4) / 2) : 0u; }

    // ---------------------------------------------------------------- render
    void render(float* outL, float* outR, uint32_t n) {
        if (usesV3()) { renderV3(outL, outR, n); tSamples += n; return; }
        if (arpRunning || arp.playing >= 0) { if (arp.playing >= 0) voiceOff(arp.playing); arp.playing = -1; arpRunning = false; }
        renderLegacy(outL, outR, n);
        tSamples += n;
    }

    // display helper
    static float lfoShape(float t, int wave, float bal) {
        if (wave >= 3) { const float steps[8] = { .2f, -.7f, .9f, -.1f, .5f, -.9f, .3f, -.4f }; const int k = (int)(t * 8) % 8;
                         return wave == 4 ? steps[k] : steps[k] + (steps[(k + 1) % 8] - steps[k]) * (t * 8 - (int)(t * 8)); }
        Lfo l; l.phase = t; return l.value(wave, bal);
    }
    static const em::Layout& layout() { static const em::Layout L = { kXlfoFirst, kEgFirst, kEfFirst, kXyFirst, kSlot2First, kNumSlots2, kV2SlotFirst, kNumDests2 }; return L; }

    // read-back for the UI (the newest voice's sources and offsets; the global XLFO phases; the compressor)
    float uiSrc2[em::kNumSources] = {}; float uiOff2[kNumDests2] = {}; double uiXlfoPhase[em::kNumXlfo] = {}; float uiClassic[kNumSources] = {};
    float uiGr = 0; int uiArpNote = -1;
    // the arpeggiator's current pattern (sorted by pitch), for the display
    int arpPattern(int* notes, int max) const {
        int n = std::min(arp.nKeys, max); for (int i = 0; i < n; ++i) notes[i] = arp.keys[i].note;
        std::sort(notes, notes + n); return p[kArpOn] > 0.5f ? n : 0;
    }

private:
    // ================================================================ 2.x
    void renderLegacy(float* outL, float* outR, uint32_t n) {
        // collect the active slots once per block
        nActive = 0;
        for (int s = 0; s < kNumSlots; ++s) {
            const int src = (int)(p[slotParam(s, kSlotSrc)] + 0.5f), dst = (int)(p[slotParam(s, kSlotDst)] + 0.5f);
            if (src <= kSrcNone || src >= kNumSources || dst <= kDstNone || dst >= kNumDests) continue;
            if (p[slotParam(s, kSlotBypass)] > 0.5f) continue;
            act[nActive++] = { s, src, dst };
        }
        int wave[2], fType[2], fStages[2], fStyle[2]; bool fOn[2];
        for (int o = 0; o < 2; ++o) wave[o] = (int)(p[o == 0 ? kO1Wave : kO2Wave] + 0.5f);
        for (int f = 0; f < 2; ++f) {
            const uint32_t b = f == 0 ? kF1On : kF2On;
            fOn[f] = p[b] > 0.5f; fStyle[f] = (int)(p[b + 1] + 0.5f); fType[f] = (int)(p[b + 2] + 0.5f); fStages[f] = p[b + 3] > 0.5f ? 2 : 1;
        }
        const int routing = (int)(p[kRouting] + 0.5f);
        const bool sync = p[kOscSync] > 0.5f;
        const float glideCoef = p[kGlide] > 0.0005f ? 1.f - std::exp(-1.f / (p[kGlide] * 0.25f * fs)) : 1.f;
        const float bendSt = bend * p[kBendRange];
        const int uni = std::max(1, (int)(p[kUnison] + 0.5f));
        const float voiceGain = 0.3f / std::sqrt((float)uni);

        // global LFO phases (for LFOs without note sync), locked to song position when synced
        double inc[3];
        for (int l = 0; l < 3; ++l) {
            const uint32_t b = kL1Wave + l * 6;
            const int sy = (int)(p[b + 2] + 0.5f);
            if (sy == 0) inc[l] = p[b + 3] / fs;
            else {
                double bars = divBars((int)(p[b + 4] + 0.5f)); if (sy == 2) bars *= 1.5; if (sy == 3) bars *= 2.0 / 3.0;
                inc[l] = tBpm / (60.0 * fs * tBpb) / bars;
                if (tValid && tPlaying && p[b + 5] < 0.5f) { double ph = tBarPos / bars; gLfo[l] = ph - std::floor(ph); }
            }
        }

        for (uint32_t i = 0; i < n; ++i) {
            sVol += (std::pow(10.f, p[kVolume] / 20.f) - sVol) * smooth;
            for (int l = 0; l < 3; ++l) { gLfo[l] += inc[l]; if (gLfo[l] >= 1.0) gLfo[l] -= std::floor(gLfo[l]); }
            float L = 0, R = 0;
            for (Voice& v : voices) {
                if (!v.active()) continue;
                // control rate (every 16 samples): envelope times and rates, which can be modulated
                if (v.ctl-- <= 0) { v.ctl = 15; updateControl(v); }
                for (int l = 0; l < 3; ++l) {
                    const uint32_t b = kL1Wave + l * 6;
                    if (p[b + 5] > 0.5f) v.lfo[l].advance(inc[l] * modRate(v, l));                    // note-synced: per voice
                    else { const double before = v.lfo[l].phase; v.lfo[l].phase = gLfo[l]; if (gLfo[l] < before) { v.lfo[l].rndA = v.lfo[l].rndB; v.lfo[l].rndB = v.lfo[l].rnd(); v.lfo[l].held = v.lfo[l].rnd(); } }
                }
                const float e1 = v.eg[0].process(), e2 = v.eg[1].process(), e3 = v.eg[2].process();
                v.curNote += (v.targetNote - v.curNote) * glideCoef;

                // modulation rate (every 4 samples): matrix, pitch, levels, pans and filter coefficients
                if (v.sub-- <= 0) {
                    v.sub = 3;
                    float src[kNumSources];
                    src[kSrcNone] = 0;
                    for (int l = 0; l < 3; ++l) {
                        const uint32_t b = kL1Wave + l * 6;
                        src[kSrcLfo1 + l] = v.lfo[l].value((int)(p[b] + 0.5f), clampf(p[b + 1] + v.d[kDstL1Balance + 2 * l], 0.f, 1.f));
                    }
                    src[kSrcEg1] = p[kE1Invert] > 0.5f ? -e1 : e1;
                    src[kSrcEg2] = p[kE2Invert] > 0.5f ? -e2 : e2;
                    src[kSrcEg3] = e3;
                    src[kSrcVelocity] = v.vel; src[kSrcAftertouch] = std::max(chanAT, v.at);
                    src[kSrcKey] = (v.curNote - 60.f) / 60.f; src[kSrcBend] = bend; src[kSrcWheel] = wheel;

                    // slot amounts first (they can be modulated by other slots), then destinations
                    float amt[kNumSlots];
                    for (int k = 0; k < nActive; ++k) amt[act[k].slot] = p[slotParam(act[k].slot, kSlotAmt)];
                    for (int k = 0; k < nActive; ++k)
                        if (act[k].dst >= kDstSlotFirst) {
                            const int t = act[k].dst - kDstSlotFirst;
                            amt[t] = clampf(amt[t] + p[slotParam(act[k].slot, kSlotAmt)] * src[act[k].src], -1.f, 1.f);
                        }
                    float* d = v.d;
                    std::fill(d, d + kDstSlotFirst, 0.f);
                    for (int k = 0; k < nActive; ++k)
                        if (act[k].dst < kDstSlotFirst) d[act[k].dst] += amt[act[k].slot] * src[act[k].src];

                    const float base = v.curNote + bendSt + v.detune * (1.f + d[kDstUniDetune]);
                    for (int o = 0; o < 2; ++o) {
                        const uint32_t b = o == 0 ? kO1Wave : kO2Wave;
                        const float note = base + 12.f * p[b + 1] + p[b + 2] + p[b + 3] * 0.01f + 24.f * d[o == 0 ? kDstO1Pitch : kDstO2Pitch];
                        v.c.dt[o] = clampf(440.f * std::pow(2.f, (note - 69.f) / 12.f) / fs, 1e-6f, 0.45f);
                        v.c.pw[o] = clampf(p[b + 4] + 0.45f * d[o == 0 ? kDstO1PW : kDstO2PW], 0.05f, 0.95f);
                        const float lvl = clampf(p[b + 5] + d[o == 0 ? kDstO1Level : kDstO2Level], 0.f, 1.f);
                        const float pan = clampf(p[b + 6] + d[o == 0 ? kDstO1Pan : kDstO2Pan] + v.panOff, -1.f, 1.f);
                        const float a = (pan + 1.f) * 0.25f * kPi;
                        v.c.gl[o] = lvl * std::cos(a) * 1.4142f; v.c.gr[o] = lvl * std::sin(a) * 1.4142f;
                    }
                    for (int f = 0; f < 2; ++f) {
                        if (!fOn[f]) continue;
                        const uint32_t b = f == 0 ? kF1On : kF2On;
                        const float oct = std::log2(p[b + 4]) + 5.f * d[f == 0 ? kDstF1Freq : kDstF2Freq];
                        v.c.pk[f] = std::pow(clampf(p[b + 5] + d[f == 0 ? kDstF1Peak : kDstF2Peak], 0.f, 1.f), 0.6f);
                        const float pn = clampf(p[b + 6] + d[f == 0 ? kDstF1Pan : kDstF2Pan], -1.5f, 1.5f);
                        v.c.fg[f][0] = std::tan(kPi * clampf(std::pow(2.f, oct - 2.f * pn), 10.f, 0.45f * fs) / fs);
                        v.c.fg[f][1] = std::tan(kPi * clampf(std::pow(2.f, oct + 2.f * pn), 10.f, 0.45f * fs) / fs);
                    }
                    v.c.vol = std::max(0.f, 1.f + d[kDstVolume]);
                }

                // oscillators (osc 2 can be hard-synced to osc 1)
                float os[2];
                bool wrapped = false;
                for (int o = 0; o < 2; ++o) {
                    v.ph[o] += v.c.dt[o];
                    if (v.ph[o] >= 1.f) { v.ph[o] -= 1.f; if (o == 0) wrapped = true; }
                    if (o == 1 && sync && wrapped) v.ph[1] = v.ph[0] * v.c.dt[1] / v.c.dt[0];
                    os[o] = osc(v, o, wave[o], v.c.dt[o], v.c.pw[o]);
                }
                float sl[2], sr[2];
                for (int o = 0; o < 2; ++o) { sl[o] = os[o] * v.c.gl[o]; sr[o] = os[o] * v.c.gr[o]; }
                // filters
                float fl[2], fr[2];
                auto runF = [&](int f, float inl, float inr, float& ol, float& orr) {
                    if (!fOn[f]) { ol = inl; orr = inr; return; }
                    ol = v.flt[f][0].process(inl, v.c.fg[f][0], v.c.pk[f], fType[f], fStages[f], fStyle[f]);
                    orr = v.flt[f][1].process(inr, v.c.fg[f][1], v.c.pk[f], fType[f], fStages[f], fStyle[f]);
                };
                float yl, yr;
                const float ml = sl[0] + sl[1], mr = sr[0] + sr[1];
                if (routing == 0) { runF(0, ml, mr, fl[0], fr[0]); runF(1, fl[0], fr[0], yl, yr); }
                else if (routing == 1) {
                    runF(0, ml, mr, fl[0], fr[0]); runF(1, ml, mr, fl[1], fr[1]);
                    if (fOn[0] && fOn[1]) { yl = 0.7071f * (fl[0] + fl[1]); yr = 0.7071f * (fr[0] + fr[1]); }
                    else if (fOn[0]) { yl = fl[0]; yr = fr[0]; }
                    else if (fOn[1]) { yl = fl[1]; yr = fr[1]; }
                    else { yl = ml; yr = mr; }
                } else { runF(0, sl[0], sr[0], fl[0], fr[0]); runF(1, sl[1], sr[1], fl[1], fr[1]); yl = fl[0] + fl[1]; yr = fr[0] + fr[1]; }

                const float amp = e3 * (1.f - p[kAmpVel] * (1.f - v.vel)) * v.c.vol * voiceGain;
                L += yl * amp; R += yr * amp;
                if (!v.active()) { for (auto& fc : v.flt) for (auto& c : fc) c.reset(); v.note = -1; v.group = -1; }
            }
            L *= sVol; R *= sVol;
            float oL = L - dcXl + 0.9995f * dcL; dcXl = L; dcL = oL;
            float oR = R - dcXr + 0.9995f * dcR; dcXr = R; dcR = oR;
            outL[i] = softCeil(oL); outR[i] = softCeil(oR);
        }
    }

    // ================================================================ 3.0
    struct ActiveSlot { int slot, src, dst; };

    // v2 matrix over the active slots: off[] gets every destination's offset
    void matrix2(const float* src, float* off, bool globalsOnly) const {
        std::fill(off, off + kNumDests2, 0.f);
        float amt[kNumSlots2];
        for (int k = 0; k < nActive2; ++k) amt[act2[k].slot] = p[sp(act2[k].slot, kSlotAmt)];
        for (int k = 0; k < nActive2; ++k) if (act2[k].dst >= kV2SlotFirst) {
            const int t = act2[k].dst - kV2SlotFirst; const float v = p[sp(act2[k].slot, kSlotAmt)] * src[act2[k].src];
            amt[t] = clampf(amt[t] + v, -1.f, 1.f); off[act2[k].dst] += v; }
        for (int k = 0; k < nActive2; ++k) {
            const int d = act2[k].dst; if (d >= kV2SlotFirst) continue;
            if (globalsOnly && (d < kV2GlobalFirst || d > kV2GlobalLast)) continue;
            off[d] += amt[act2[k].slot] * src[act2[k].src];
        }
    }
    float xlfoRate(int l, const float* off) const {
        const int sync = (int)std::round(p[lp_(l, em::kLSync)]);
        if (sync > 0) return (float)(tBpm / 60.0 / 4.0 / (em::divBars((int)std::round(p[lp_(l, em::kLDiv)])) * em::syncFactor(sync)));
        return clampf(p[lp_(l, em::kLRate)] * std::pow(2.f, 4.f * clampf(off[kV2XlfoRateFirst + l], -4.f, 4.f)), 0.001f, 200.f);
    }
    float xlfoValue(em::Xlfo& x, int l, double rate, float dt, float bal) const {
        if (!(x.phase >= 0.0 && x.phase < 1.0)) x.phase = 0;
        if (p[lp_(l, em::kLMode)] > 0.5f) {
            const int steps = (int)clampf(std::round(p[lp_(l, em::kLSteps)]), 1, em::kNumSteps);
            const int st = (int)clampf((float)(x.phase * steps), 0.f, (float)(steps - 1));
            const float target = clampf(p[lp_(l, em::kLStep1 + st)], -1.f, 1.f), gl = clampf(p[lp_(l, em::kLGlide)], 0.f, 1.f);
            const float stepSecs = (float)(1.0 / std::max(rate, 1e-4) / steps);
            const float c = gl < 0.01f ? 1.f : 1.f - std::exp(-dt / std::max(gl * stepSecs * 0.5f, dt));
            x.stepOut += (target - x.stepOut) * c;
            return x.stepOut;
        }
        return x.wave((int)std::round(p[lp_(l, em::kLWave)]), clampf(bal, 0.f, 1.f));
    }
    float efValue(int e, float fast, float slow) const {
        const float g = p[fp(e, em::kFGain)];
        if (p[kEfModeFirst + e] > 0.5f) {         // transient: how far the fast follower is above the slow one
            const float d = 20.f * std::log10(std::max(fast, 1e-6f)) - 20.f * std::log10(std::max(slow, 1e-6f));
            return clampf((d + g * 0.5f) / 18.f, 0.f, 1.f);
        }
        return clampf((20.f * std::log10(std::max(fast, 1e-9f)) + g + 54.f) / 54.f, 0.f, 1.f);
    }

    void renderV3(float* outL, float* outR, uint32_t n) {
        nActive = 0;
        for (int s = 0; s < kNumSlots; ++s) {
            const int src = (int)(p[slotParam(s, kSlotSrc)] + 0.5f), dst = (int)(p[slotParam(s, kSlotDst)] + 0.5f);
            if (src <= kSrcNone || src >= kNumSources || dst <= kDstNone || dst >= kNumDests) continue;
            if (p[slotParam(s, kSlotBypass)] > 0.5f) continue;
            act[nActive++] = { s, src, dst };
        }
        nActive2 = 0;
        for (int s = 0; s < kNumSlots2; ++s) {
            const int src = (int)std::round(p[sp(s, kSlotSrc)]), dst = (int)std::round(p[sp(s, kSlotDst)]);
            if (src <= em::kSrcNone || src >= em::kNumSources || dst <= 0 || dst >= kNumDests2 || p[sp(s, kSlotBypass)] > 0.5f) continue;
            act2[nActive2++] = { s, src, dst };
        }
        int wave[kNumOsc]; bool oOn[kNumOsc];
        for (int o = 0; o < kNumOsc; ++o) { wave[o] = (int)std::round(p[opar(o, kOWave)]); oOn[o] = p[kOscOnFirst + o] > 0.5f; }
        int fType[kNumFilters], fStages[kNumFilters], fStyle[kNumFilters]; bool fOn[kNumFilters];
        for (int f = 0; f < kNumFilters; ++f) {
            fOn[f] = p[fpar(f, kFOn)] > 0.5f; fStyle[f] = (int)std::round(p[fpar(f, kFStyle)]); fType[f] = (int)std::round(p[fpar(f, kFType)]);
            fStages[f] = slopeStages((int)std::round(p[fpar(f, kFSlope)]));
            if (fStages[f] == 0 && fType[f] == kBandPass) fStages[f] = 1;
        }
        const int routing = (int)std::round(p[kRouting]);
        const bool sync = p[kOscSync] > 0.5f, ring12 = p[kRing12] > 0.5f, ring34 = p[kRing34] > 0.5f;
        const float bendSt = bend * p[kBendRange];
        const int uni = std::max(1, (int)(p[kUnison] + 0.5f));
        const float voiceGain = 0.3f / std::sqrt((float)uni);
        const int K = p[kHq] > 0.5f ? 4 : 1; const float fsK = fs * (float)K;
        const float transpose = 12.f * std::round(p[kTranspose]);
        const float drift = clampf(p[kDrift], 0.f, 1.f);
        const float dtc = 4.f / fs;                         // the modulation rate (seconds per tick)
        float efA[em::kNumEf], efR[em::kNumEf], efAs[em::kNumEf], efRs[em::kNumEf];
        for (int e = 0; e < em::kNumEf; ++e) {
            efA[e] = 1.f - std::exp(-1.f / (std::max(p[fp(e, em::kFAttack)], 0.0001f) * fs));
            efR[e] = 1.f - std::exp(-1.f / (std::max(p[fp(e, em::kFRelease)], 0.001f) * fs));
            efAs[e] = 1.f - std::exp(-1.f / (std::max(p[fp(e, em::kFAttack)] * 10.f, 0.01f) * fs));
            efRs[e] = 1.f - std::exp(-1.f / (std::max(p[fp(e, em::kFRelease)] * 4.f, 0.05f) * fs));
        }

        double inc[3];
        for (int l = 0; l < 3; ++l) {
            const uint32_t b = kL1Wave + l * 6;
            const int sy = (int)(p[b + 2] + 0.5f);
            if (sy == 0) inc[l] = p[b + 3] / fs;
            else {
                double bars = divBars((int)(p[b + 4] + 0.5f)); if (sy == 2) bars *= 1.5; if (sy == 3) bars *= 2.0 / 3.0;
                inc[l] = tBpm / (60.0 * fs * tBpb) / bars;
                if (tValid && tPlaying && p[b + 5] < 0.5f) { double ph = tBarPos / bars; gLfo[l] = ph - std::floor(ph); }
            }
        }
        const bool revOn = p[kRevOn] > 0.5f, dlyOn = p[kDlyOn] > 0.5f, choOn = p[kChoOn] > 0.5f, phsOn = p[kPhsOn] > 0.5f, drvOn = p[kDrvOn] > 0.5f, cmpOn = p[kCmpOn] > 0.5f;
        const float fxK = 1.f - std::exp(-1.f / (0.02f * fs));
        volT = std::pow(10.f, p[kVolume] / 20.f);

        for (uint32_t i = 0; i < n; ++i) {
            arpTick(i);
            sVol += (volT - sVol) * smooth;
            for (int l = 0; l < 3; ++l) { gLfo[l] += inc[l]; if (gLfo[l] >= 1.0) gLfo[l] -= std::floor(gLfo[l]); }

            // the whole synth's modulation (global XLFOs, the FX and arpeggiator destinations), every 4 samples
            if (gMod-- <= 0) {
                gMod = 3;
                for (int l = 0; l < em::kNumXlfo; ++l) {
                    const float r = xlfoRate(l, offG);
                    const int sy = (int)std::round(p[lp_(l, em::kLSync)]);
                    if (sy > 0 && tValid && tPlaying) {
                        const double bars = em::divBars((int)std::round(p[lp_(l, em::kLDiv)])) * em::syncFactor(sy);
                        const double before = gXl[l].phase; gXl[l].phase = std::fmod(barPosNow(i) / bars, 1.0); if (gXl[l].phase < 0) gXl[l].phase += 1;
                        if (gXl[l].phase < before) { gXl[l].rndA = gXl[l].rndB; gXl[l].rndB = gXl[l].rnd(); gXl[l].held = gXl[l].rnd(); }
                    } else gXl[l].advance(r * dtc);
                    gSrc2[em::kSrcXlfo1 + l] = xlfoValue(gXl[l], l, r, dtc, p[lp_(l, em::kLBalance)] + offG[kV2XlfoBalFirst + l]);
                    uiXlfoPhase[l] = gXl[l].phase;
                }
                for (int e = 0; e < em::kNumEf; ++e) gSrc2[em::kSrcEf1 + e] = efValue(e, synthLevel, synthSlow);
                for (int x = 0; x < em::kNumXy; ++x) { gSrc2[em::kSrcXy1X + 2 * x] = p[xyp(x, 0)]; gSrc2[em::kSrcXy1Y + 2 * x] = p[xyp(x, 1)]; }
                gSrc2[em::kSrcVelocity] = lastVel; gSrc2[em::kSrcKey] = lastKey < 0 ? 0.f : clampf((lastKey - 60) / 60.f, -1.f, 1.f);
                gSrc2[em::kSrcWheel] = wheel; gSrc2[em::kSrcBend] = bend; gSrc2[em::kSrcAftertouch] = chanAT;
                const Voice* nv = newestVoice();
                const float* gs = nv ? nv->src2 : gSrc2;
                if (nv) std::copy(nv->src2, nv->src2 + em::kNumSources, uiSrc2); else std::copy(gSrc2, gSrc2 + em::kNumSources, uiSrc2);
                matrix2(gs, offG, false);
                std::copy(offG, offG + kNumDests2, uiOff2);
                // effect coefficients
                const float* o = offG;
                if (revOn) rev.prepare(clampf(p[kRevTime] * std::pow(2.f, 3.f * o[kV2RevTime]), 0.2f, 20.f), clampf(p[kRevBright] + o[kV2RevBright], 0.f, 1.f), clampf(p[kRevPredelay] + 0.25f * o[kV2RevPredelay], 0.f, 0.25f));
                if (dlyOn) {
                    float t = p[kDlySync] > 0.5f ? (float)(arpDivBeats((int)std::round(p[kDlyDiv])) * 60.0 / tBpm) : p[kDlyTime];
                    t = clampf(t * std::pow(2.f, 2.f * o[kV2DlyTime]), 0.001f, 2.f);
                    const float off = clampf(p[kDlyOffset], -0.5f, 0.5f);           // positive: the right side longer; negative: the left
                    dly.prepare(t * (1.f + std::max(0.f, -off)), t * (1.f + std::max(0.f, off)), clampf(p[kDlyFeedback] + o[kV2DlyFeedback], 0.f, 1.f), p[kDlyPingPong] > 0.5f, p[kDlyLowCut], p[kDlyHighCut]);
                }
                if (cmpOn) cmp.prepare(clampf(p[kCmpThreshold] + 30.f * o[kV2CmpThreshold], -60.f, 0.f), p[kCmpRatio], p[kCmpAttack], p[kCmpRelease], p[kCmpAutoGain] > 0.5f);
                fxDrive = clampf(p[kDrvAmount] + o[kV2DrvAmount], 0.f, 1.f);
                fxChoRate = clampf(p[kChoRate] * std::pow(2.f, 3.f * o[kV2ChoRate]), 0.01f, 10.f); fxChoDepth = clampf(p[kChoDepth] + o[kV2ChoDepth], 0.f, 1.f);
                fxPhsRate = clampf(p[kPhsRate] * std::pow(2.f, 3.f * o[kV2PhsRate]), 0.01f, 10.f); fxPhsDepth = clampf(p[kPhsDepth] + o[kV2PhsDepth], 0.f, 1.f); fxPhsFb = clampf(p[kPhsFeedback] + o[kV2PhsFeedback], 0.f, 1.f);
                fxMix[0] = clampf(p[kRevMix] + o[kV2RevMix], 0.f, 1.f); fxMix[1] = clampf(p[kDlyMix] + o[kV2DlyMix], 0.f, 1.f);
                fxMix[2] = clampf(p[kChoMix] + o[kV2ChoMix], 0.f, 1.f); fxMix[3] = clampf(p[kPhsMix] + o[kV2PhsMix], 0.f, 1.f); fxMix[4] = clampf(p[kCmpMix] + o[kV2CmpMix], 0.f, 1.f);
                volT = std::pow(10.f, p[kVolume] / 20.f);
            }

            float accL[4] = {0, 0, 0, 0}, accR[4] = {0, 0, 0, 0};
            for (Voice& v : voices) {
                if (!v.active()) continue;
                if (v.ctl-- <= 0) {
                    v.ctl = 15; updateControl(v);
                    // drift: each oscillator wanders towards a new random target a few times a second
                    if (drift > 0.f) for (int o = 0; o < kNumOsc; ++o) {
                        if (((v.rng = v.rng * 1664525u + 1013904223u) >> 24) < 6) v.driftTarget[o] = ((int32_t)(v.rng = v.rng * 1664525u + 1013904223u)) * (1.f / 2147483648.f);
                        v.drift[o] += (v.driftTarget[o] - v.drift[o]) * 0.02f;
                    }
                }
                for (int l = 0; l < 3; ++l) {
                    const uint32_t b = kL1Wave + l * 6;
                    if (p[b + 5] > 0.5f) v.lfo[l].advance(inc[l] * modRate(v, l));
                    else { const double before = v.lfo[l].phase; v.lfo[l].phase = gLfo[l]; if (gLfo[l] < before) { v.lfo[l].rndA = v.lfo[l].rndB; v.lfo[l].rndB = v.lfo[l].rnd(); v.lfo[l].held = v.lfo[l].rnd(); } }
                }
                const float e1 = v.eg[0].process(), e2 = v.eg[1].process(), e3 = v.eg[2].process();
                v.curNote += (v.targetNote - v.curNote) * v.c.glide;

                if (v.sub-- <= 0) {
                    v.sub = 3;
                    float src[kNumSources];
                    src[kSrcNone] = 0;
                    for (int l = 0; l < 3; ++l) {
                        const uint32_t b = kL1Wave + l * 6;
                        src[kSrcLfo1 + l] = v.lfo[l].value((int)(p[b] + 0.5f), clampf(p[b + 1] + v.d[kDstL1Balance + 2 * l], 0.f, 1.f));
                    }
                    src[kSrcEg1] = p[kE1Invert] > 0.5f ? -e1 : e1;
                    src[kSrcEg2] = p[kE2Invert] > 0.5f ? -e2 : e2;
                    src[kSrcEg3] = e3;
                    src[kSrcVelocity] = v.vel; src[kSrcAftertouch] = std::max(chanAT, v.at);
                    src[kSrcKey] = (v.curNote - 60.f) / 60.f; src[kSrcBend] = bend; src[kSrcWheel] = wheel;
                    // v2 sources for this voice
                    float* s2 = v.src2; const float* o2 = v.d2;
                    for (int l = 0; l < em::kNumXlfo; ++l) {
                        if (p[kXlfoRetrigFirst + l] > 0.5f) {
                            const float r = xlfoRate(l, o2);
                            v.xl[l].advance(r * dtc);
                            s2[em::kSrcXlfo1 + l] = xlfoValue(v.xl[l], l, r, dtc, p[lp_(l, em::kLBalance)] + o2[kV2XlfoBalFirst + l]);
                        } else s2[em::kSrcXlfo1 + l] = gSrc2[em::kSrcXlfo1 + l];
                    }
                    for (int e = 0; e < em::kNumEg; ++e) {
                        auto t = [&](int f, int k) { return p[ep(e, f)] * std::pow(2.f, 4.f * o2[kV2EgFirst + e * 4 + k]); };
                        s2[em::kSrcEg1 + e] = v.eg2[e].process(dtc, t(em::kEAttack, 0), t(em::kEDecay, 1), p[ep(e, em::kESustain)] + o2[kV2EgFirst + e * 4 + 2], t(em::kERelease, 3),
                                                               p[kEgCurveFirst + e * 3], p[kEgCurveFirst + e * 3 + 1], p[kEgCurveFirst + e * 3 + 2]);
                    }
                    for (int e = 0; e < em::kNumEf; ++e) s2[em::kSrcEf1 + e] = p[fp(e, em::kFSource)] > 0.5f ? gSrc2[em::kSrcEf1 + e] : efValue(e, v.efFast[e], v.efSlow[e]);
                    for (int x = 0; x < em::kNumXy * 2; ++x) s2[em::kSrcXy1X + x] = p[kXyFirst + x];
                    s2[em::kSrcVelocity] = v.vel; s2[em::kSrcKey] = clampf((v.curNote - 60.f) / 60.f, -1.f, 1.f);
                    s2[em::kSrcWheel] = wheel; s2[em::kSrcBend] = bend; s2[em::kSrcAftertouch] = std::max(chanAT, v.at); s2[0] = 0;

                    float amt[kNumSlots];
                    for (int k = 0; k < nActive; ++k) amt[act[k].slot] = p[slotParam(act[k].slot, kSlotAmt)];
                    for (int k = 0; k < nActive; ++k)
                        if (act[k].dst >= kDstSlotFirst) {
                            const int t = act[k].dst - kDstSlotFirst;
                            amt[t] = clampf(amt[t] + p[slotParam(act[k].slot, kSlotAmt)] * src[act[k].src], -1.f, 1.f);
                        }
                    float* d = v.d;
                    std::fill(d, d + kDstSlotFirst, 0.f);
                    for (int k = 0; k < nActive; ++k)
                        if (act[k].dst < kDstSlotFirst) d[act[k].dst] += amt[act[k].slot] * src[act[k].src];
                    float* d2 = v.d2;
                    matrix2(s2, d2, false);
                    for (int k = 1; k < kDstSlotFirst; ++k) d[k] += d2[k];

                    const float base = v.curNote + bendSt + transpose + p[kTune] * 0.01f + d2[kV2Tune] + v.detune * (1.f + d[kDstUniDetune]);
                    for (int o = 0; o < kNumOsc; ++o) {
                        float dp, dw, dl, dn;
                        if (o < 2) { const int b = o == 0 ? kDstO1Pitch : kDstO2Pitch; dp = d[b]; dw = d[b + 1]; dl = d[b + 2]; dn = d[b + 3]; }
                        else { const int b = kV2O3First + (o - 2) * 4; dp = d2[b]; dw = d2[b + 1]; dl = d2[b + 2]; dn = d2[b + 3]; }
                        const float note = base + 12.f * p[opar(o, kOOct)] + p[opar(o, kOSemi)] + p[opar(o, kOFine)] * 0.01f + 24.f * dp + 0.15f * drift * v.drift[o];
                        v.c.dt[o] = clampf(440.f * std::pow(2.f, (note - 69.f) / 12.f) / fsK, 1e-7f, 0.45f);
                        v.c.ratio[o] = clampf(p[kOscRatioFirst + o] + 15.f * d2[kV2RatioFirst + o], 1.f, 16.f);
                        v.c.pw[o] = clampf(p[opar(o, kOPW)] + 0.45f * dw, 0.05f, 0.95f);
                        const float lvl = clampf(p[opar(o, kOLevel)] + dl, 0.f, 1.f);
                        const float pan = clampf(p[opar(o, kOPan)] + dn + v.panOff * (1.f + d2[kV2Spread]), -1.f, 1.f);
                        const float a = (pan + 1.f) * 0.25f * kPi;
                        v.c.gl[o] = lvl * std::cos(a) * 1.4142f; v.c.gr[o] = lvl * std::sin(a) * 1.4142f;
                    }
                    const float fOff = p[kFreqOffset] + 5.f * d2[kV2FreqOffset], pOff = p[kPeakOffset] + d2[kV2PeakOffset];
                    for (int f = 0; f < kNumFilters; ++f) {
                        if (!fOn[f]) continue;
                        float df, dpk, dpn;
                        if (f < 2) { const int b = f == 0 ? kDstF1Freq : kDstF2Freq; df = d[b]; dpk = d[b + 1]; dpn = d[b + 2]; }
                        else { const int b = kV2F3First + (f - 2) * 3; df = d2[b]; dpk = d2[b + 1]; dpn = d2[b + 2]; }
                        const float oct = std::log2(p[fpar(f, kFFreq)]) + 5.f * df + fOff;
                        const float reso = clampf(p[fpar(f, kFPeak)] + dpk + pOff, 0.f, 1.f);
                        v.c.pk[f] = std::pow(reso, 0.6f); v.c.pkRaw[f] = reso;
                        const float pn = clampf(p[fpar(f, kFPan)] + dpn, -1.5f, 1.5f);
                        v.c.fg[f][0] = std::tan(kPi * clampf(std::pow(2.f, oct - 2.f * pn), 10.f, 0.45f * fsK) / fsK);
                        v.c.fg[f][1] = std::tan(kPi * clampf(std::pow(2.f, oct + 2.f * pn), 10.f, 0.45f * fsK) / fsK);
                        v.c.gain[f] = std::pow(10.f, clampf(p[fpar(f, kFGain)] + 24.f * d2[kV2GainFirst + f], -24.f, 24.f) / 20.f);
                    }
                    v.c.vol = std::max(0.f, 1.f + d[kDstVolume]);
                    {   // identical channels (no pan anywhere): run the filters once
                        bool mono = true;
                        for (int o = 0; o < kNumOsc && mono; ++o) mono = v.c.gl[o] == v.c.gr[o];
                        for (int f = 0; f < kNumFilters && mono; ++f) mono = !fOn[f] || v.c.fg[f][0] == v.c.fg[f][1];
                        if (v.mono && !mono) for (auto& fc : v.flt) fc[1] = fc[0];
                        v.mono = mono;
                    }
                    const float gt = p[kGlide] * std::pow(2.f, 4.f * d2[kV2Glide]);
                    v.c.glide = gt > 0.0005f ? 1.f - std::exp(-1.f / (gt * 0.25f * fs)) : 1.f;
                }

                const float amp = e3 * (1.f - p[kAmpVel] * (1.f - v.vel)) * v.c.vol * voiceGain;
                float lvl = 0;
                for (int k = 0; k < K; ++k) {
                    float os[kNumOsc] = {0, 0, 0, 0};
                    bool wrapped = false;
                    for (int o = 0; o < kNumOsc; ++o) {
                        if (!oOn[o] || !(v.oscMask & (1 << o))) continue;
                        const float dt = v.c.dt[o], r = v.c.ratio[o];
                        if (r > 1.0001f) {                 // sync ratio: a hidden master at the note's pitch restarts the oscillator
                            v.mph[o] += dt;
                            if (v.mph[o] >= 1.f) { v.mph[o] -= 1.f; v.ph[o] = v.mph[o] * r; v.ph[o] -= std::floor(v.ph[o]); }
                            else { v.ph[o] += dt * r; if (v.ph[o] >= 1.f) v.ph[o] -= std::floor(v.ph[o]); }
                        } else {
                            v.ph[o] += dt;
                            if (v.ph[o] >= 1.f) { v.ph[o] -= 1.f; if (o == 0) wrapped = true; }
                            if (o == 1 && sync && wrapped && oOn[0]) v.ph[1] = v.ph[0] * dt / v.c.dt[0];
                        }
                        os[o] = osc3(v, o, wave[o], dt * r, v.c.pw[o]);
                    }
                    if (ring12) os[1] *= os[0];
                    if (ring34) os[3] *= os[2];
                    float sl[kNumOsc], sr[kNumOsc];
                    for (int o = 0; o < kNumOsc; ++o) { sl[o] = os[o] * v.c.gl[o]; sr[o] = os[o] * v.c.gr[o]; }
                    const bool mono = v.mono;
                    auto runF = [&](int f, float inl, float inr, float& ol, float& orr) {
                        if (!fOn[f]) { ol = inl; orr = inr; return; }
                        ol = v.flt[f][0].process3(inl, v.c.fg[f][0], v.c.pk[f], v.c.pkRaw[f], fType[f], fStages[f], fStyle[f], v.c.gain[f]);
                        orr = mono ? ol : v.flt[f][1].process3(inr, v.c.fg[f][1], v.c.pk[f], v.c.pkRaw[f], fType[f], fStages[f], fStyle[f], v.c.gain[f]);
                    };
                    float yl = 0, yr = 0;
                    if (routing == kSerial) {
                        yl = sl[0] + sl[1] + sl[2] + sl[3]; yr = sr[0] + sr[1] + sr[2] + sr[3];
                        for (int f = 0; f < kNumFilters; ++f) if (fOn[f]) runF(f, yl, yr, yl, yr);
                    } else if (routing == kParallel) {
                        const float ml = sl[0] + sl[1] + sl[2] + sl[3], mr = sr[0] + sr[1] + sr[2] + sr[3];
                        int cnt = 0;
                        for (int f = 0; f < kNumFilters; ++f) if (fOn[f]) { float a, b; runF(f, ml, mr, a, b); yl += a; yr += b; ++cnt; }
                        if (cnt == 0) { yl = ml; yr = mr; } else { const float g = cnt == 2 ? 0.7071f : 1.f / std::sqrt((float)cnt); yl *= g; yr *= g; }
                    } else {
                        for (int f = 0; f < kNumFilters; ++f) { float a, b; runF(f, sl[f], sr[f], a, b); yl += a; yr += b; }
                    }
                    accL[k] += yl * amp; accR[k] += yr * amp;
                    lvl = std::max(lvl, std::max(std::fabs(yl * amp), std::fabs(yr * amp)));
                }
                for (int e = 0; e < em::kNumEf; ++e) {
                    v.efFast[e] += (lvl > v.efFast[e] ? efA[e] : efR[e]) * (lvl - v.efFast[e]);
                    v.efSlow[e] += (lvl > v.efSlow[e] ? efAs[e] : efRs[e]) * (lvl - v.efSlow[e]);
                }
                if (!v.active()) { for (auto& fc : v.flt) for (auto& c : fc) c.reset(); v.note = -1; v.group = -1; }
                else if (!(lvl < 1e4f)) for (auto& fc : v.flt) for (auto& c : fc) c.reset();       // a filter ran away: start it again
            }
            float L, R;
            if (K == 1) { L = accL[0]; R = accR[0]; }
            else { L = ovsL.down(accL, 4); R = ovsR.down(accR, 4); }
            {   // the whole synth's level, for the followers on SYNTH
                const float a = std::max(std::fabs(L), std::fabs(R));
                synthLevel += (a > synthLevel ? efA[0] : efR[0]) * (a - synthLevel);
                synthSlow += (a > synthSlow ? efAs[0] : efRs[0]) * (a - synthSlow);
            }
            L *= sVol; R *= sVol;
            if (!(std::fabs(L) < 1e4f)) L = 0;                      // never let a runaway voice into the effects' buffers
            if (!(std::fabs(R) < 1e4f)) R = 0;

            // ---- effects
            if (drvOn && p[kDrvPost] < 0.5f) { L = fx::drive(L, fxDrive); R = fx::drive(R, fxDrive); }
            if (revOn) { sRev += (fxMix[0] - sRev) * fxK; float wl, wr; rev.process(L, R, wl, wr); L += sRev * wl; R += sRev * wr; }
            if (dlyOn) { sDly += (fxMix[1] - sDly) * fxK; float wl, wr; dly.process(L, R, wl, wr); L += sDly * wl; R += sDly * wr; }
            if (choOn) {
                sCho += (fxMix[2] - sCho) * fxK;
                float wl, wr; cho.process(L, R, fxChoRate, fxChoDepth, p[kChoDelay], p[kChoMode] > 0.5f, wl, wr);
                const float m = sCho; L = L * (1.f - 0.5f * m) + wl * m * 0.7f; R = R * (1.f - 0.5f * m) + wr * m * 0.7f;
            }
            if (phsOn) {
                sPhs += (fxMix[3] - sPhs) * fxK;
                float wl, wr; phs.process(L, R, p[kPhsMode] > 0.5f, fxPhsRate, fxPhsDepth, fxPhsFb, p[kPhsShift], wl, wr);
                const float m = sPhs; L = L * (1.f - 0.5f * m) + wl * m * 0.5f; R = R * (1.f - 0.5f * m) + wr * m * 0.5f;
            }
            if (drvOn && p[kDrvPost] > 0.5f) { L = fx::drive(L, fxDrive); R = fx::drive(R, fxDrive); }
            if (cmpOn) { const float g = cmp.process(L, R), m = fxMix[4]; L = L * (1.f - m) + L * g * m; R = R * (1.f - m) + R * g * m; uiGr = cmp.meterGr; }
            float oL = L - dcXl + 0.9995f * dcL; dcXl = L; dcL = oL;
            float oR = R - dcXr + 0.9995f * dcR; dcXr = R; dcR = oR;
            outL[i] = softCeil(oL); outR[i] = softCeil(oR);
        }
        if (const Voice* nv = newestVoice()) {
            float src[kNumSources] = {};
            for (int l = 0; l < 3; ++l) src[kSrcLfo1 + l] = nv->lfo[l].value((int)(p[kL1Wave + 6 * l] + 0.5f), p[kL1Balance + 6 * l]);
            src[kSrcEg1] = nv->eg[0].value; src[kSrcEg2] = nv->eg[1].value; src[kSrcEg3] = nv->eg[2].value; src[kSrcVelocity] = nv->vel;
            src[kSrcKey] = (nv->curNote - 60.f) / 60.f; src[kSrcBend] = bend; src[kSrcWheel] = wheel; src[kSrcAftertouch] = std::max(chanAT, nv->at);
            std::copy(src, src + kNumSources, uiClassic);
        }
    }

    double barPosNow(uint32_t i) const { return tBarPos + (double)(tSamples + i) * tBpm / (60.0 * fs * tBpb); }
    const Voice* newestVoice() const {
        const Voice* best = nullptr;
        for (const Voice& v : voices) if (v.active() && v.keyDown && (!best || v.age > best->age)) best = &v;
        if (!best) for (const Voice& v : voices) if (v.active() && (!best || v.age > best->age)) best = &v;
        return best;
    }

    // ---- arpeggiator: called every sample by renderV3
    void arpTick(uint32_t i) {
        if (p[kArpOn] < 0.5f) {
            if (arp.playing >= 0) { voiceOff(arp.playing); arp.playing = -1; }
            arpRunning = false; uiArpNote = -1; return;
        }
        if (arp.nKeys == 0) {
            if (arp.playing >= 0) { voiceOff(arp.playing); arp.playing = -1; }
            arpRunning = false; uiArpNote = -1; return;
        }
        const bool synced = p[kArpSync] > 0.5f, retrig = p[kArpRetrig] > 0.5f;
        const double beats = arpDivBeats((int)std::round(p[kArpDiv]));
        const double mul = clampf(p[kArpOffset], 0.5f, 2.f);
        const double stepsPerSec = synced ? tBpm / 60.0 / beats * mul : clampf(p[kArpRate], 0.25f, 20.f) * std::pow(2.f, 2.f * offG[kV2ArpRate]);
        if (!arpRunning) { arpRunning = true; arp.phase = 0; arp.step = -1; arp.seqPos = 0; }
        if (synced && tValid && tPlaying && !retrig) {
            const double ph = barPosNow(i) * tBpb / beats * mul;
            if (ph < arp.phase - 1) { arp.step = (long)std::floor(ph) - 1; }      // the transport jumped back
            arp.phase = ph;
        } else arp.phase += stepsPerSec / fs;
        if (arp.playing >= 0 && arp.phase >= arp.offAt) { voiceOff(arp.playing); arp.playing = -1; }
        const float groove = clampf(p[kArpGroove], -0.9f, 0.9f);
        const long k = (long)std::floor(arp.phase);
        for (long kk = k - 1; kk <= k; ++kk) {
            if (kk <= arp.step) continue;
            const double tk = (double)kk + ((kk & 1) ? 0.5 * groove : 0.0);
            if (arp.phase < tk) continue;
            arp.step = kk;
            const int order = (int)std::round(p[kArpOrder]), octs = (int)std::round(p[kArpOctaves]);
            int vel = 100; const int note = arp.noteAt(arp.seqPos++, order, octs, vel);
            const int prev = arp.playing;
            if (prev >= 0 && prev != note) voiceOff(prev);
            if (note >= 0) { voiceOn(note, vel); arp.playing = note; uiArpNote = note; }
            const float gate = clampf(p[kArpGate], 0.05f, 1.f);
            const double len = 1.0 + ((kk & 1) ? -0.5 * groove : 0.5 * groove);
            arp.offAt = gate >= 0.99f ? 1e18 : tk + gate * len * 0.98;
        }
    }

    static float softCeil(float x) {
        const float a = std::fabs(x);
        if (!(a < 1e6f)) return 0.f;
        return a > 0.89f ? std::copysign(0.89f + 0.11f * std::tanh((a - 0.89f) / 0.11f), x) : x;
    }
    float modRate(const Voice& v, int l) const { return std::pow(2.f, 4.f * v.d[kDstL1Rate + 2 * l]); }

    void updateControl(Voice& v) {
        const float* d = v.d;
        auto t = [](float base, float m) { return base * std::pow(2.f, 4.f * m); };
        v.eg[0].set(fs, t(p[kE1Delay], d[kDstE1Delay]), t(p[kE1Attack], d[kDstE1Attack]), 0, t(p[kE1Decay], d[kDstE1Decay]),
                    p[kE1Sustain] + d[kDstE1Sustain], t(p[kE1Release], d[kDstE1Release]));
        v.eg[1].set(fs, t(p[kE2Delay], d[kDstE2Delay]), t(p[kE2Attack], d[kDstE2Attack]), 0, t(p[kE2Decay], d[kDstE2Decay]),
                    p[kE2Sustain] + d[kDstE2Sustain], t(p[kE2Release], d[kDstE2Release]));
        v.eg[2].set(fs, 0, t(p[kE3Attack], d[kDstE3Attack]), t(p[kE3Hold], d[kDstE3Hold]), t(p[kE3Decay], d[kDstE3Decay]),
                    p[kE3Sustain] + d[kDstE3Sustain], t(p[kE3Release], d[kDstE3Release]));
    }

    float osc(Voice& v, int o, int wave, float dt, float pw) {
        const float t = v.ph[o];
        if (wave == 1) return 2.f * t - 1.f - polyBlep(t, dt);
        if (wave == 2) {
            float sq = t < pw ? 1.f : -1.f; sq += polyBlep(t, dt);
            float t2 = t - pw; if (t2 < 0.f) t2 += 1.f; sq -= polyBlep(t2, dt); return sq;
        }
        float sq = t < 0.5f ? 1.f : -1.f; sq += polyBlep(t, dt);
        float t3 = t + 0.5f; if (t3 >= 1.f) t3 -= 1.f; sq -= polyBlep(t3, dt);
        v.tri[o] = dt * 4.f * sq + (1.f - 0.5f * dt) * v.tri[o];
        return v.tri[o];
    }
    // 3.0: the 2.x waves plus sine, white noise and pink noise
    float osc3(Voice& v, int o, int wave, float dt, float pw) {
        if (wave <= 2) return osc(v, o, wave, dt, pw);
        if (wave == 3) return std::sin(2.f * kPi * v.ph[o]);
        v.rng = v.rng * 1664525u + 1013904223u;
        const float w = (int32_t)v.rng * (1.f / 2147483648.f);
        if (wave == 4) return w;
        float* b = v.pink;                                   // pink: a sum of first-order filtered white noise (Kellet's method)
        b[0] = 0.99886f * b[0] + w * 0.0555179f; b[1] = 0.99332f * b[1] + w * 0.0750759f; b[2] = 0.96900f * b[2] + w * 0.1538520f;
        b[3] = 0.86650f * b[3] + w * 0.3104856f; b[4] = 0.55000f * b[4] + w * 0.5329522f; b[5] = -0.7616f * b[5] - w * 0.0168980f;
        const float pk = b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362f; b[6] = w * 0.115926f;
        return pk * 0.2f;
    }

    // ---- voices: the 2.x allocation, with the 3.0 pool size, per-oscillator mode and envelope legato
    int pool() const { return usesV3() ? kMaxVoices : kMaxVoicesLegacy; }
    void voiceOn(int note, int vel) {
        const int P = pool();
        const int uni = std::max(1, std::min(8, (int)(p[kUnison] + 0.5f)));
        const int maxNotes = std::max(1, std::min((int)(p[kVoices] + 0.5f), P / uni));
        const float v = vel / 127.f;
        if (maxNotes == 1) { monoNoteOn(note, v, uni); return; }
        const bool glideFrom = p[kGlide] > 0.0005f && lastNote >= 0 && (p[kGlideMode] < 0.5f || anyKeyDown());
        const bool legatoEg = anyKeyDown();
        // the same key again: retrigger its voices in place
        int group = -1;
        for (int k = 0; k < P; ++k) { Voice& x = voices[k]; if (x.active() && x.note == note && x.group >= 0) { group = x.group; break; } }
        if (group >= 0) {
            int u = 0;
            for (int k = 0; k < P; ++k) { Voice& x = voices[k]; if (x.group == group) startVoice(x, note, v, group, u++, uni, x.curNote, true); }
            lastNote = note;
            return;
        }
        // respect the polyphony limit: retire the oldest note (released ones first)
        if (countGroups() >= maxNotes) killOldestGroup();
        group = ++groupCounter;
        const int mask = oscMaskForNewNote();
        for (int u = 0; u < uni; ++u) { Voice& x = *freeVoice(P); startVoice(x, note, v, group, u, uni, glideFrom ? (float)lastNote : (float)note, true); x.oscMask = mask; egLegato(x, legatoEg); }
        lastNote = note;
    }
    void voiceOff(int note) {
        const int P = pool();
        const int uni = std::max(1, (int)(p[kUnison] + 0.5f));
        const int maxNotes = std::max(1, std::min((int)(p[kVoices] + 0.5f), P / uni));
        if (maxNotes == 1) {
            int j = 0; for (int i = 0; i < held; ++i) if (stack[i] != note) stack[j++] = stack[i];
            const bool removed = j != held; held = j;
            if (!removed) return;
            if (held > 0) {
                const int n = stack[held - 1];
                for (Voice& x : voices) if (x.group == monoGroup && x.active()) { x.note = n; x.targetNote = (float)n; if (p[kGlide] <= 0.0005f) x.curNote = (float)n; }
                lastNote = n;
            } else for (Voice& x : voices) if (x.group == monoGroup) keyUp(x);
            return;
        }
        for (Voice& x : voices) if (x.note == note && x.keyDown) { x.keyDown = false; keyUp(x); }
    }
    int oscMaskForNewNote() {
        if (p[kPolyMode] < 0.5f) return 0xF;
        int on[kNumOsc], n = 0;
        for (int o = 0; o < kNumOsc; ++o) if (p[kOscOnFirst + o] > 0.5f) on[n++] = o;
        if (n == 0) return 0xF;
        return 1 << on[(nextOsc++) % n];
    }
    // EGs set to LEGATO keep running from the voice already playing
    void egLegato(Voice& x, bool otherKeys) {
        for (int e = 0; e < em::kNumEg; ++e) {
            if (p[ep(e, em::kETrigger)] < 0.5f || !otherKeys) continue;
            const Voice* src = nullptr;
            for (const Voice& y : voices) if (&y != &x && y.active() && y.keyDown && y.eg2[e].gated() && (!src || y.age > src->age)) src = &y;
            if (src) x.eg2[e] = src->eg2[e];
        }
    }

    bool anyKeyDown() const { for (const Voice& x : voices) if (x.keyDown) return true; return held > 0; }
    int countGroups() const {
        int g[kMaxVoices]; int n = 0;
        for (const Voice& x : voices) if (x.active() && x.group >= 0) {
            bool seen = false; for (int i = 0; i < n; ++i) if (g[i] == x.group) seen = true;
            if (!seen) g[n++] = x.group;
        }
        return n;
    }
    void killOldestGroup() {
        int best = -1; uint32_t bestAge = UINT32_MAX; bool bestReleased = false;
        for (const Voice& x : voices) if (x.active() && x.group >= 0) {
            const bool rel = !x.keyDown && !x.sustained;
            if ((rel && !bestReleased) || (rel == bestReleased && x.age < bestAge)) { best = x.group; bestAge = x.age; bestReleased = rel; }
        }
        for (Voice& x : voices) if (x.group == best) fastKill(x);
    }
    void fastKill(Voice& x) { x.eg[2].gateOff(); x.eg[2].cR = 1.f - std::exp(-1.f / (0.003f * fs)); x.keyDown = false; x.sustained = false; x.group = -2; }
    Voice* freeVoice(int P) {
        for (int k = 0; k < P; ++k) if (!voices[k].active()) return &voices[k];
        Voice* best = &voices[0];
        for (int k = 0; k < P; ++k) {
            Voice& x = voices[k];
            const bool rel = !x.keyDown && !x.sustained, brel = !best->keyDown && !best->sustained;
            if ((rel && !brel) || (rel == brel && x.age < best->age)) best = &x;
        }
        return best;
    }
    void startVoice(Voice& v, int note, float vel, int group, int u, int uni, float from, bool retrigger) {
        const bool wasActive = v.active();
        v.note = note; v.group = group; v.vel = vel; v.keyDown = true; v.sustained = false; v.at = 0; v.age = ++ageCounter;
        v.targetNote = (float)note; v.curNote = from;
        const float spreadPos = uni > 1 ? (u - (uni - 1) * 0.5f) / ((uni - 1) * 0.5f) : 0.f;   // -1..1
        v.detune = spreadPos * p[kUniDetune] * 0.5f;                                            // up to +/- 0.5 st
        v.panOff = spreadPos * p[kUniSpread];
        for (int o = 0; o < 2; ++o)
            if (p[o == 0 ? kO1Phase : kO2Phase] > 0.5f || !wasActive) v.ph[o] = p[o == 0 ? kO1Phase : kO2Phase] > 0.5f ? 0.f : uniPhase(u, o);
        for (int l = 0; l < 3; ++l) if (p[kL1NoteSync + 6 * l] > 0.5f) v.lfo[l].phase = 0;
        if (retrigger) {
            std::fill(v.d, v.d + kNumDests, 0.f);
            updateControl(v);                       // envelope times must be set before the gate (delay/hold)
            for (Env& e : v.eg) e.gateOn();
        }
        v.ctl = 15; v.sub = 0;
        if (!wasActive) for (auto& fc : v.flt) for (auto& c : fc) c.reset();
        // 3.0 state (unused by the 2.x path)
        for (int o = 2; o < kNumOsc; ++o) if (p[opar(o, kOPhase)] > 0.5f || !wasActive) v.ph[o] = p[opar(o, kOPhase)] > 0.5f ? 0.f : v.ph[o - 2] * 0.618f + 0.31f * o;
        for (int o = 0; o < kNumOsc; ++o) { if (v.ph[o] >= 1.f) v.ph[o] -= std::floor(v.ph[o]); v.mph[o] = v.ph[o]; }
        v.oscMask = 0xF;
        if (retrigger) {
            for (int l = 0; l < em::kNumXlfo; ++l) if (p[kXlfoRetrigFirst + l] > 0.5f) { v.xl[l].phase = 0; v.xl[l].lastStep = -1; }
            for (CurveEnv& e : v.eg2) e.gate(true);
            std::fill(v.d2, v.d2 + kNumDests2, 0.f);
        }
        v.c.glide = 1.f; v.mono = false;
    }
    float uniPhase(int, int) { seedPh = seedPh * 1664525u + 1013904223u; return (float)((seedPh >> 8) & 0xFFFF) / 65536.f; }   // free-running osc: random start

    void monoNoteOn(int note, float vel, int uni) {
        const int P = pool();
        const bool wasHeld = held > 0;
        if (held < 16) stack[held++] = note;
        const bool legato = p[kGlideMode] > 0.5f;
        bool groupAlive = false;
        for (Voice& x : voices) if (x.group == monoGroup && x.active()) groupAlive = true;
        if (!groupAlive) {
            for (Voice& x : voices) if (x.group == monoGroup) x.group = -1;
            monoGroup = ++groupCounter;
            const int mask = oscMaskForNewNote();
            for (int u = 0; u < uni; ++u) { Voice& x = *freeVoice(P); startVoice(x, note, vel, monoGroup, u, uni, (float)note, true); x.oscMask = mask; }
        } else {
            const bool glide = p[kGlide] > 0.0005f && (!legato || wasHeld);
            const bool retrig = !(legato && wasHeld);
            int u = 0;
            for (Voice& x : voices) if (x.group == monoGroup && x.active()) {
                const float from = glide ? x.curNote : (float)note;
                const int mask = x.oscMask;
                if (retrig) { startVoice(x, note, vel, monoGroup, u, uni, from, true); x.oscMask = mask; }
                else { x.note = note; x.targetNote = (float)note; x.keyDown = true; x.sustained = false; if (!glide) x.curNote = (float)note; }
                ++u;
            }
        }
        lastNote = note;
    }
    void keyUp(Voice& x) { if (sustainDown) x.sustained = true; else release(x); }
    void release(Voice& x) { for (Env& e : x.eg) e.gateOff(); for (CurveEnv& e : x.eg2) e.gate(false); x.keyDown = false; }

    Voice voices[kMaxVoices];
    ActiveSlot act[kNumSlots]; int nActive = 0;
    ActiveSlot act2[kNumSlots2]; int nActive2 = 0;
    double gLfo[3] = {0, 0, 0};
    float fs = 48000.f, smooth = 0.01f, sVol = 0.5f;
    float bend = 0, wheel = 0, chanAT = 0;
    float dcL = 0, dcR = 0, dcXl = 0, dcXr = 0;
    int stack[16] = {}; int held = 0, monoGroup = -1, groupCounter = 0, lastNote = -1;
    uint32_t ageCounter = 0, seedPh = 777;
    bool sustainDown = false;
    bool tValid = false, tPlaying = false; double tBarPos = 0, tBpm = 120, tBpb = 4; uint64_t tSamples = 0;
    // 3.0
    em::Xlfo gXl[em::kNumXlfo]; float gSrc2[em::kNumSources] = {}; float offG[kNumDests2] = {}; int gMod = 0;
    float lastVel = 0; int lastKey = -1, keysDown = 0, nextOsc = 0;
    float synthLevel = 0, synthSlow = 0;
    fx::Reverb rev; fx::Delay dly; fx::Chorus cho; fx::Phaser phs; fx::Compressor cmp;
    float sRev = 0, sDly = 0, sCho = 0, sPhs = 0, volT = 0.5f;
    float fxDrive = 0, fxChoRate = 1, fxChoDepth = 0.5f, fxPhsRate = 1, fxPhsDepth = 0.5f, fxPhsFb = 0.5f, fxMix[5] = {0, 0, 0, 0, 0};
    epsilon::Oversampler ovsL, ovsR;
    Arp arp; bool arpRunning = false;
};

} // namespace obsidian
