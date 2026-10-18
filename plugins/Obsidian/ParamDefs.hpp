// Parameter metadata shared by the DSP wrapper and the UI. License: MIT.
#pragma once
#include "ObsidianEngine.hpp"
#include <cstdio>
#include <cstring>

namespace obsidian {

static constexpr uint32_t kHintInteger = 1, kHintBoolean = 2, kHintLog = 4, kHintOutput = 8;
struct ParamDef { const char* name; const char* sym; const char* unit; float min, max; uint32_t hints; };

inline const char* sourceName(int s) {
    static const char* const n[kNumSources] = { "None", "LFO 1", "LFO 2", "LFO 3", "Env 1", "Env 2", "Amp Env",
                                                "Velocity", "Aftertouch", "Key Track", "Pitch Bend", "Mod Wheel" };
    return n[s < 0 || s >= kNumSources ? 0 : s];
}
inline const char* destName(int d) {
    static const char* const n[kDstSlotFirst] = {
        "None", "Osc 1 Pitch", "Osc 1 PW", "Osc 1 Level", "Osc 1 Pan", "Osc 2 Pitch", "Osc 2 PW", "Osc 2 Level", "Osc 2 Pan",
        "Filter 1 Freq", "Filter 1 Peak", "Filter 1 Pan", "Filter 2 Freq", "Filter 2 Peak", "Filter 2 Pan",
        "Env 1 Delay", "Env 1 Attack", "Env 1 Decay", "Env 1 Sustain", "Env 1 Release",
        "Env 2 Delay", "Env 2 Attack", "Env 2 Decay", "Env 2 Sustain", "Env 2 Release",
        "Amp Attack", "Amp Hold", "Amp Decay", "Amp Sustain", "Amp Release",
        "LFO 1 Rate", "LFO 1 Balance", "LFO 2 Rate", "LFO 2 Balance", "LFO 3 Rate", "LFO 3 Balance",
        "Volume", "Unison Detune" };
    static char slotNames[kNumSlots][24];
    static bool init = false;
    if (!init) { init = true; for (int i = 0; i < kNumSlots; ++i) std::snprintf(slotNames[i], sizeof slotNames[i], "Slot %d Amount", i + 1); }
    if (d < 0 || d >= kNumDests) return n[0];
    return d < kDstSlotFirst ? n[d] : slotNames[d - kDstSlotFirst];
}

inline const ParamDef& paramDef(uint32_t i) {
    static ParamDef t[kParamCount];
    static bool done = false;
    if (!done) {
        done = true;
        auto set = [](uint32_t i, const char* n, const char* s, const char* u, float lo, float hi, uint32_t h) { t[i] = { n, s, u, lo, hi, h }; };
        const uint32_t I = kHintInteger, B = kHintBoolean | kHintInteger;
        set(kO1Wave, "Osc 1 Wave", "o1wave", "", 0, 2, I);   set(kO2Wave, "Osc 2 Wave", "o2wave", "", 0, 2, I);
        set(kO1Oct, "Osc 1 Octave", "o1oct", "", -3, 3, I);   set(kO2Oct, "Osc 2 Octave", "o2oct", "", -3, 3, I);
        set(kO1Semi, "Osc 1 Semitone", "o1semi", "st", -12, 12, I); set(kO2Semi, "Osc 2 Semitone", "o2semi", "st", -12, 12, I);
        set(kO1Fine, "Osc 1 Fine", "o1fine", "ct", -100, 100, 0);  set(kO2Fine, "Osc 2 Fine", "o2fine", "ct", -100, 100, 0);
        set(kO1PW, "Osc 1 Pulse Width", "o1pw", "", 0.05f, 0.95f, 0); set(kO2PW, "Osc 2 Pulse Width", "o2pw", "", 0.05f, 0.95f, 0);
        set(kO1Level, "Osc 1 Level", "o1lvl", "", 0, 1, 0);   set(kO2Level, "Osc 2 Level", "o2lvl", "", 0, 1, 0);
        set(kO1Pan, "Osc 1 Pan", "o1pan", "", -1, 1, 0);       set(kO2Pan, "Osc 2 Pan", "o2pan", "", -1, 1, 0);
        set(kO1Phase, "Osc 1 Phase Sync", "o1phase", "", 0, 1, B); set(kO2Phase, "Osc 2 Phase Sync", "o2phase", "", 0, 1, B);
        set(kOscSync, "Osc 2 Hard Sync", "oscsync", "", 0, 1, B);
        for (int f = 0; f < 2; ++f) {
            const uint32_t b = f == 0 ? kF1On : kF2On;
            static const char* nm[2][7] = { { "Filter 1 On", "Filter 1 Style", "Filter 1 Type", "Filter 1 Slope", "Filter 1 Freq", "Filter 1 Peak", "Filter 1 Pan" },
                                            { "Filter 2 On", "Filter 2 Style", "Filter 2 Type", "Filter 2 Slope", "Filter 2 Freq", "Filter 2 Peak", "Filter 2 Pan" } };
            static const char* sy[2][7] = { { "f1on", "f1style", "f1type", "f1slope", "f1freq", "f1peak", "f1pan" },
                                            { "f2on", "f2style", "f2type", "f2slope", "f2freq", "f2peak", "f2pan" } };
            set(b, nm[f][0], sy[f][0], "", 0, 1, B);
            set(b + 1, nm[f][1], sy[f][1], "", 0, kNumStyles - 1, I);
            set(b + 2, nm[f][2], sy[f][2], "", 0, 2, I);
            set(b + 3, nm[f][3], sy[f][3], "", 0, 1, I);
            set(b + 4, nm[f][4], sy[f][4], "Hz", 20, 20000, kHintLog);
            set(b + 5, nm[f][5], sy[f][5], "", 0, 1, 0);
            set(b + 6, nm[f][6], sy[f][6], "", -1, 1, 0);
        }
        set(kRouting, "Filter Routing", "routing", "", 0, 2, I);
        for (int e = 0; e < 2; ++e) {
            const uint32_t b = e == 0 ? kE1Delay : kE2Delay;
            static const char* nm[2][6] = { { "Env 1 Delay", "Env 1 Attack", "Env 1 Decay", "Env 1 Sustain", "Env 1 Release", "Env 1 Invert" },
                                            { "Env 2 Delay", "Env 2 Attack", "Env 2 Decay", "Env 2 Sustain", "Env 2 Release", "Env 2 Invert" } };
            static const char* sy[2][6] = { { "e1dly", "e1atk", "e1dec", "e1sus", "e1rel", "e1inv" }, { "e2dly", "e2atk", "e2dec", "e2sus", "e2rel", "e2inv" } };
            set(b, nm[e][0], sy[e][0], "s", 0, 5, 0); set(b + 1, nm[e][1], sy[e][1], "s", 0, 10, 0); set(b + 2, nm[e][2], sy[e][2], "s", 0.001f, 10, 0);
            set(b + 3, nm[e][3], sy[e][3], "", 0, 1, 0); set(b + 4, nm[e][4], sy[e][4], "s", 0.001f, 10, 0); set(b + 5, nm[e][5], sy[e][5], "", 0, 1, B);
        }
        set(kE3Attack, "Amp Attack", "e3atk", "s", 0, 10, 0);   set(kE3Hold, "Amp Hold", "e3hold", "s", 0, 5, 0);
        set(kE3Decay, "Amp Decay", "e3dec", "s", 0.001f, 10, 0); set(kE3Sustain, "Amp Sustain", "e3sus", "", 0, 1, 0);
        set(kE3Release, "Amp Release", "e3rel", "s", 0.001f, 10, 0); set(kAmpVel, "Amp Velocity", "ampvel", "", 0, 1, 0);
        for (int l = 0; l < 3; ++l) {
            const uint32_t b = kL1Wave + 6 * l;
            static char nm[3][6][28], sy[3][6][12];
            static const char* f[6] = { "Wave", "Balance", "Sync", "Rate", "Division", "Note Sync" };
            static const char* fs[6] = { "wave", "bal", "sync", "rate", "div", "nsync" };
            for (int k = 0; k < 6; ++k) { std::snprintf(nm[l][k], 28, "LFO %d %s", l + 1, f[k]); std::snprintf(sy[l][k], 12, "l%d%s", l + 1, fs[k]); }
            set(b, nm[l][0], sy[l][0], "", 0, 4, I); set(b + 1, nm[l][1], sy[l][1], "", 0, 1, 0); set(b + 2, nm[l][2], sy[l][2], "", 0, 3, I);
            set(b + 3, nm[l][3], sy[l][3], "Hz", 0.01f, 50, kHintLog); set(b + 4, nm[l][4], sy[l][4], "", 0, kNumDivs - 1, I); set(b + 5, nm[l][5], sy[l][5], "", 0, 1, B);
        }
        set(kVoices, "Voices", "voices", "", 1, 24, I);     set(kUnison, "Unison", "unison", "", 1, 4, I);
        set(kUniDetune, "Unison Detune", "unidet", "", 0, 1, 0); set(kUniSpread, "Unison Spread", "unispr", "", 0, 1, 0);
        set(kGlide, "Glide", "glide", "s", 0, 2, 0);          set(kGlideMode, "Glide Mode", "glidemode", "", 0, 1, I);
        set(kBendRange, "Bend Range", "bend", "st", 0, 24, I); set(kVolume, "Volume", "volume", "dB", -36, 6, 0);
        static char sn[kNumSlots][4][24], ss[kNumSlots][4][12];
        static const char* f[4] = { "Source", "Destination", "Amount", "Bypass" };
        static const char* fs[4] = { "src", "dst", "amt", "byp" };
        for (int s = 0; s < kNumSlots; ++s)
            for (int k = 0; k < 4; ++k) {
                std::snprintf(sn[s][k], 24, "Slot %d %s", s + 1, f[k]); std::snprintf(ss[s][k], 12, "s%d%s", s + 1, fs[k]);
                const float hi = k == 0 ? kNumSources - 1 : (k == 1 ? kNumDests - 1 : 1);
                set(slotParam(s, k), sn[s][k], ss[s][k], "", k == 2 ? -1.f : 0.f, hi, k == 2 ? 0 : (k == 3 ? B : I));
            }
        set(kMidiOn, "MIDI", "midion", "", 0, 1, B);
        set(kOutLearn, "MIDI Learn", "o_learn", "", 0, 100000, kHintOutput);
    }
    return t[i];
}

inline const char* const* enumLabels(uint32_t p, int& count) {
    static const char* const wave[] = { "Triangle", "Saw", "Square" };
    static const char* const style[] = { "Smooth", "Hot", "Raw", "Monolith" };
    static const char* const type[] = { "Low Pass", "Band Pass", "High Pass" };
    static const char* const slope[] = { "12 dB/oct", "24 dB/oct" };
    static const char* const route[] = { "Serial", "Parallel", "Split" };
    static const char* const lw[] = { "Triangle", "Sine", "Square", "Random", "Sample & Hold" };
    static const char* const sync[] = { "Free", "Straight", "Dotted", "Triplet" };
    static const char* const div[] = { "16 bars", "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
    static const char* const gm[] = { "Always", "Legato" };
    switch (p) {
    case kO1Wave: case kO2Wave: count = 3; return wave;
    case kF1Style: case kF2Style: count = 4; return style;
    case kF1Type: case kF2Type: count = 3; return type;
    case kF1Slope: case kF2Slope: count = 2; return slope;
    case kRouting: count = 3; return route;
    case kL1Wave: case kL2Wave: case kL3Wave: count = 5; return lw;
    case kL1Sync: case kL2Sync: case kL3Sync: count = 4; return sync;
    case kL1Div: case kL2Div: case kL3Div: count = 11; return div;
    case kGlideMode: count = 2; return gm;
    default: break;
    }
    if (p >= kSlotFirst && p <= kSlotLast) {
        static const char* srcs[kNumSources]; static const char* dsts[kNumDests]; static bool init = false;
        if (!init) { init = true; for (int i = 0; i < kNumSources; ++i) srcs[i] = sourceName(i); for (int i = 0; i < kNumDests; ++i) dsts[i] = destName(i); }
        const int field = (p - kSlotFirst) % 4;
        if (field == kSlotSrc) { count = kNumSources; return srcs; }
        if (field == kSlotDst) { count = kNumDests; return dsts; }
    }
    count = 0; return nullptr;
}

inline bool isTimeParam(uint32_t p) {
    return p == kE1Delay || p == kE1Attack || p == kE1Decay || p == kE1Release || p == kE2Delay || p == kE2Attack || p == kE2Decay
        || p == kE2Release || p == kE3Attack || p == kE3Hold || p == kE3Decay || p == kE3Release || p == kGlide;
}
inline float toNorm(uint32_t p, float v) {
    const ParamDef& d = paramDef(p);
    if (d.hints & kHintLog) return std::log(v / d.min) / std::log(d.max / d.min);
    float n = (v - d.min) / (d.max - d.min);
    if (isTimeParam(p)) n = std::sqrt(std::max(n, 0.f));
    return n;
}
inline float fromNorm(uint32_t p, float n) {
    const ParamDef& d = paramDef(p);
    n = clampf(n, 0.f, 1.f);
    float v;
    if (d.hints & kHintLog) v = d.min * std::pow(d.max / d.min, n);
    else { if (isTimeParam(p)) n = n * n; v = d.min + n * (d.max - d.min); }
    if (d.hints & kHintInteger) v = std::round(v);
    return v;
}

// which modulation destination a knob maps to (for drag-and-drop), or -1
inline int destForParam(uint32_t p) {
    switch (p) {
    case kO1Oct: case kO1Semi: case kO1Fine: return kDstO1Pitch;
    case kO2Oct: case kO2Semi: case kO2Fine: return kDstO2Pitch;
    case kO1PW: return kDstO1PW;  case kO1Level: return kDstO1Level; case kO1Pan: return kDstO1Pan;
    case kO2PW: return kDstO2PW;  case kO2Level: return kDstO2Level; case kO2Pan: return kDstO2Pan;
    case kF1Freq: return kDstF1Freq; case kF1Peak: return kDstF1Peak; case kF1Pan: return kDstF1Pan;
    case kF2Freq: return kDstF2Freq; case kF2Peak: return kDstF2Peak; case kF2Pan: return kDstF2Pan;
    case kE1Delay: return kDstE1Delay; case kE1Attack: return kDstE1Attack; case kE1Decay: return kDstE1Decay; case kE1Sustain: return kDstE1Sustain; case kE1Release: return kDstE1Release;
    case kE2Delay: return kDstE2Delay; case kE2Attack: return kDstE2Attack; case kE2Decay: return kDstE2Decay; case kE2Sustain: return kDstE2Sustain; case kE2Release: return kDstE2Release;
    case kE3Attack: return kDstE3Attack; case kE3Hold: return kDstE3Hold; case kE3Decay: return kDstE3Decay; case kE3Sustain: return kDstE3Sustain; case kE3Release: return kDstE3Release;
    case kL1Rate: case kL1Div: return kDstL1Rate; case kL1Balance: return kDstL1Balance;
    case kL2Rate: case kL2Div: return kDstL2Rate; case kL2Balance: return kDstL2Balance;
    case kL3Rate: case kL3Div: return kDstL3Rate; case kL3Balance: return kDstL3Balance;
    case kVolume: return kDstVolume; case kUniDetune: return kDstUniDetune;
    default: break;
    }
    if (p >= kSlotFirst && p <= kSlotLast && (p - kSlotFirst) % 4 == kSlotAmt) return kDstSlotFirst + (int)((p - kSlotFirst) / 4);
    return -1;
}

} // namespace obsidian
