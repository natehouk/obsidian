// Obsidian — polyphonic two-oscillator, two-filter synthesizer engine with a 24-slot modulation matrix.
// Clean-room implementation from public, generic techniques: PolyBLEP oscillators, cascaded TPT
// state-variable filters (Zavalishin) with per-style nonlinearities, one-pole envelope segments.
// No third-party product code, assets or disassembly were used. License: MIT (see LICENSE).

#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace obsidian {

static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }

static constexpr int kNumSlots = 24;
static constexpr int kMaxVoices = 24;

// ------------------------------------------------------------------ params (append-only)
#define OSC(n)  kO##n##Wave, kO##n##Oct, kO##n##Semi, kO##n##Fine, kO##n##PW, kO##n##Level, kO##n##Pan, kO##n##Phase
#define FLT(n)  kF##n##On, kF##n##Style, kF##n##Type, kF##n##Slope, kF##n##Freq, kF##n##Peak, kF##n##Pan
#define ENV(n)  kE##n##Delay, kE##n##Attack, kE##n##Decay, kE##n##Sustain, kE##n##Release, kE##n##Invert
#define LFOP(n) kL##n##Wave, kL##n##Balance, kL##n##Sync, kL##n##Rate, kL##n##Div, kL##n##NoteSync
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
    kParamCount
};
#undef OSC
#undef FLT
#undef ENV
#undef LFOP
enum SlotField { kSlotSrc = 0, kSlotDst = 1, kSlotAmt = 2, kSlotBypass = 3 };
inline uint32_t slotParam(int slot, int field) { return kSlotFirst + slot * 4 + field; }

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

static constexpr int kNumStyles = 4;
static constexpr int kNumDivs = 11;
inline float divBars(int i) {
    static const float t[kNumDivs] = { 16, 8, 4, 2, 1, 1.f/2, 1.f/4, 1.f/8, 1.f/16, 1.f/32, 1.f/64 };
    return t[i < 0 ? 0 : (i >= kNumDivs ? kNumDivs - 1 : i)];
}

// ------------------------------------------------------------------ filter
struct StyleDef { float drive; int nl; float kMin; };
inline const StyleDef& styleDef(int s) {
    static const StyleDef t[kNumStyles] = {
        { 1.2f, 0,  0.30f },   // Smooth   : moderate overdrive, broad resonance
        { 2.0f, 2, -0.05f },   // Hot      : saturating integrators, screaming
        { 3.0f, 3, -0.08f },   // Raw      : hard-clipped feedback, buzzy self-oscillation
        { 1.2f, 1, -0.04f },   // Monolith : Monolith's soft-limited self-oscillating filter
    };
    return t[s < 0 ? 0 : (s >= kNumStyles ? kNumStyles - 1 : s)];
}
inline const float* bwK(int stages) {
    static const float k1[1] = { 1.41421356f };
    static const float k2[2] = { 1.84775907f, 0.76536686f };
    return stages == 1 ? k1 : k2;
}
struct FilterChan {
    float ic1[2] = {0, 0}, ic2[2] = {0, 0};
    void reset() { ic1[0] = ic1[1] = ic2[0] = ic2[1] = 0; }
    // r = resonance already shaped as reso^0.6 (computed at modulation rate by the caller)
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

// ------------------------------------------------------------------ LFO
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
    float ph[2] = {0, 0}, tri[2] = {0, 0};
    uint32_t age = 0;
    FilterChan flt[2][2];
    Env eg[3];
    Lfo lfo[3];
    float d[kNumDests] = {};          // latest modulation sums (used at control rate)
    struct Cache { float dt[2] = {0.01f, 0.01f}, pw[2] = {0.5f, 0.5f}, gl[2] = {0, 0}, gr[2] = {0, 0}, fg[2][2] = {{0.1f, 0.1f}, {0.1f, 0.1f}}, pk[2] = {0, 0}, vol = 1; } c;
    int ctl = 0, sub = 0;
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
    }
    void setSlot(int s, int src, int dst, float amt) {
        p[slotParam(s, kSlotSrc)] = (float)src; p[slotParam(s, kSlotDst)] = (float)dst;
        p[slotParam(s, kSlotAmt)] = amt; p[slotParam(s, kSlotBypass)] = 0;
    }

    void setSampleRate(float sr) {
        fs = sr; smooth = 1.f - std::exp(-1.f / (0.01f * sr));
        reset();
    }
    void reset() {
        uint32_t seed = 12345;
        for (Voice& v : voices) {
            v = Voice();
            for (Lfo& l : v.lfo) { seed = seed * 1664525u + 1013904223u; l.rng = seed | 1; l.rndB = l.rnd(); }
        }
        held = 0; groupCounter = 0; lastNote = -1; sustainDown = false; dcL = dcR = dcXl = dcXr = 0;
        for (double& g : gLfo) g = 0;
    }
    void setTransport(bool valid, bool playing, double barPos, double bpm, double bpb) {
        tValid = valid; tPlaying = playing; tBarPos = barPos; tBpm = bpm > 1 ? bpm : 120; tBpb = bpb > 0 ? bpb : 4;
    }

    // ---------------------------------------------------------------- MIDI
    void noteOn(int note, int vel) {
        if (p[kMidiOn] < 0.5f) return;
        const int uni = std::max(1, std::min(8, (int)(p[kUnison] + 0.5f)));
        const int maxNotes = std::max(1, std::min((int)(p[kVoices] + 0.5f), kMaxVoices / uni));
        const float v = vel / 127.f;
        if (maxNotes == 1) { monoNoteOn(note, v, uni); return; }
        const bool glideFrom = p[kGlide] > 0.0005f && lastNote >= 0 && (p[kGlideMode] < 0.5f || anyKeyDown());
        // the same key again: retrigger its voices in place
        int group = -1;
        for (Voice& x : voices) if (x.active() && x.note == note && x.group >= 0) { group = x.group; break; }
        if (group >= 0) {
            int u = 0;
            for (Voice& x : voices) if (x.group == group) startVoice(x, note, v, group, u++, uni, x.curNote, true);
            lastNote = note;
            return;
        }
        // respect the polyphony limit: retire the oldest note (released ones first)
        if (countGroups() >= maxNotes) killOldestGroup();
        group = ++groupCounter;
        for (int u = 0; u < uni; ++u) startVoice(*freeVoice(), note, v, group, u, uni, glideFrom ? (float)lastNote : (float)note, true);
        lastNote = note;
    }
    void noteOff(int note) {
        if (p[kMidiOn] < 0.5f) return;
        const int uni = std::max(1, (int)(p[kUnison] + 0.5f));
        const int maxNotes = std::max(1, std::min((int)(p[kVoices] + 0.5f), kMaxVoices / uni));
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
    void sustain(bool down) {
        sustainDown = down;
        if (!down) for (Voice& x : voices) if (x.sustained) { x.sustained = false; release(x); }
    }
    void allNotesOff() { held = 0; sustainDown = false; for (Voice& x : voices) { x.keyDown = x.sustained = false; release(x); } }
    void allSoundOff() { for (Voice& x : voices) { x.eg[2].stage = Env::Idle; x.eg[2].value = 0; x.note = -1; x.group = -1; } held = 0; }
    void pitchBend(int v14) { bend = (v14 - 8192) / 8192.f; }
    void modWheel(int v) { wheel = v / 127.f; }
    void channelPressure(int v) { chanAT = v / 127.f; }
    void polyPressure(int note, int v) { for (Voice& x : voices) if (x.note == note) x.at = v / 127.f; }

    int activeVoices() const { int n = 0; for (const Voice& v : voices) n += v.active(); return n; }
    int activeNotes() const { return countGroups(); }

    // ---------------------------------------------------------------- render
    void render(float* outL, float* outR, uint32_t n) {
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

    // display helper
    static float lfoShape(float t, int wave, float bal) {
        if (wave >= 3) { const float steps[8] = { .2f, -.7f, .9f, -.1f, .5f, -.9f, .3f, -.4f }; const int k = (int)(t * 8) % 8;
                         return wave == 4 ? steps[k] : steps[k] + (steps[(k + 1) % 8] - steps[k]) * (t * 8 - (int)(t * 8)); }
        Lfo l; l.phase = t; return l.value(wave, bal);
    }

private:
    struct ActiveSlot { int slot, src, dst; };

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
    Voice* freeVoice() {
        for (Voice& x : voices) if (!x.active()) return &x;
        Voice* best = &voices[0];
        for (Voice& x : voices) {
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
    }
    float uniPhase(int, int) { seedPh = seedPh * 1664525u + 1013904223u; return (float)((seedPh >> 8) & 0xFFFF) / 65536.f; }   // free-running osc: random start

    void monoNoteOn(int note, float vel, int uni) {
        const bool wasHeld = held > 0;
        if (held < 16) stack[held++] = note;
        const bool legato = p[kGlideMode] > 0.5f;
        bool groupAlive = false;
        for (Voice& x : voices) if (x.group == monoGroup && x.active()) groupAlive = true;
        if (!groupAlive) {
            for (Voice& x : voices) if (x.group == monoGroup) x.group = -1;
            monoGroup = ++groupCounter;
            for (int u = 0; u < uni; ++u) startVoice(*freeVoice(), note, vel, monoGroup, u, uni, (float)note, true);
        } else {
            const bool glide = p[kGlide] > 0.0005f && (!legato || wasHeld);
            const bool retrig = !(legato && wasHeld);
            int u = 0;
            for (Voice& x : voices) if (x.group == monoGroup && x.active()) {
                const float from = glide ? x.curNote : (float)note;
                if (retrig) startVoice(x, note, vel, monoGroup, u, uni, from, true);
                else { x.note = note; x.targetNote = (float)note; x.keyDown = true; x.sustained = false; if (!glide) x.curNote = (float)note; }
                ++u;
            }
        }
        lastNote = note;
    }
    void keyUp(Voice& x) { if (sustainDown) x.sustained = true; else release(x); }
    void release(Voice& x) { for (Env& e : x.eg) e.gateOff(); x.keyDown = false; }

    Voice voices[kMaxVoices];
    ActiveSlot act[kNumSlots]; int nActive = 0;
    double gLfo[3] = {0, 0, 0};
    float fs = 48000.f, smooth = 0.01f, sVol = 0.5f;
    float bend = 0, wheel = 0, chanAT = 0;
    float dcL = 0, dcR = 0, dcXl = 0, dcXr = 0;
    int stack[16] = {}; int held = 0, monoGroup = -1, groupCounter = 0, lastNote = -1;
    uint32_t ageCounter = 0, seedPh = 777;
    bool sustainDown = false;
    bool tValid = false, tPlaying = false; double tBarPos = 0, tBpm = 120, tBpb = 4;
};

} // namespace obsidian
