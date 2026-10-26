// Parameter metadata shared by the DSP wrapper and the UI. License: MIT.
#pragma once
#include "ObsidianEngine.hpp"
#include <cstdio>
#include <cstring>
#include <string>

namespace obsidian {

static constexpr uint32_t kHintInteger = 1, kHintBoolean = 2, kHintLog = 4, kHintOutput = 8, kHintBipolar = 16;
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

// v2 destinations (the classic ones first, in the same order)
inline const char* destName2(int d) {
    static std::string names[kNumDests2]; static bool init = false;
    if (!init) {
        init = true; char b[48];
        for (int k = 0; k < kDstSlotFirst; ++k) names[k] = destName(k);
        for (int k = kDstE1Delay; k <= kDstL3Balance; ++k) names[k] = std::string("Classic ") + destName(k);
        for (int o = 0; o < 2; ++o) { static const char* const f[4] = { "Pitch", "PW", "Level", "Pan" };
            for (int k = 0; k < 4; ++k) { std::snprintf(b, sizeof b, "Osc %d %s", o + 3, f[k]); names[kV2O3First + o * 4 + k] = b; } }
        for (int o = 0; o < kNumOsc; ++o) { std::snprintf(b, sizeof b, "Osc %d Sync", o + 1); names[kV2RatioFirst + o] = b; }
        names[kV2Tune] = "Master Tune"; names[kV2Glide] = "Portamento"; names[kV2Spread] = "Unison Spread";
        for (int f = 0; f < 2; ++f) { static const char* const n[3] = { "Freq", "Peak", "Pan" };
            for (int k = 0; k < 3; ++k) { std::snprintf(b, sizeof b, "Filter %d %s", f + 3, n[k]); names[kV2F3First + f * 3 + k] = b; } }
        for (int f = 0; f < kNumFilters; ++f) { std::snprintf(b, sizeof b, "Filter %d Gain", f + 1); names[kV2GainFirst + f] = b; }
        names[kV2FreqOffset] = "Filter Freq Offset"; names[kV2PeakOffset] = "Filter Peak Offset";
        names[kV2RevMix] = "Reverb Amount"; names[kV2RevTime] = "Reverb Time"; names[kV2RevBright] = "Reverb Brightness"; names[kV2RevPredelay] = "Reverb Predelay";
        names[kV2DlyMix] = "Delay Amount"; names[kV2DlyTime] = "Delay Time"; names[kV2DlyFeedback] = "Delay Feedback";
        names[kV2ChoMix] = "Chorus Amount"; names[kV2ChoRate] = "Chorus Rate"; names[kV2ChoDepth] = "Chorus Depth";
        names[kV2PhsMix] = "Phaser Amount"; names[kV2PhsRate] = "Phaser Rate"; names[kV2PhsDepth] = "Phaser Depth"; names[kV2PhsFeedback] = "Phaser Feedback";
        names[kV2DrvAmount] = "Drive"; names[kV2CmpThreshold] = "Comp Threshold"; names[kV2CmpMix] = "Comp Amount"; names[kV2ArpRate] = "Arp Rate";
        for (int l = 0; l < em::kNumXlfo; ++l) { std::snprintf(b, sizeof b, "XLFO %d Rate", l + 1); names[kV2XlfoRateFirst + l] = b; std::snprintf(b, sizeof b, "XLFO %d Balance", l + 1); names[kV2XlfoBalFirst + l] = b; }
        for (int e = 0; e < em::kNumEg; ++e) { static const char* const n[4] = { "Attack", "Decay", "Sustain", "Release" };
            for (int k = 0; k < 4; ++k) { std::snprintf(b, sizeof b, "EG %d %s", e + 1, n[k]); names[kV2EgFirst + e * 4 + k] = b; } }
        for (int s = 0; s < kNumSlots2; ++s) { std::snprintf(b, sizeof b, "Mod %d Amount", s + 1); names[kV2SlotFirst + s] = b; }
    }
    return names[d < 0 || d >= kNumDests2 ? 0 : d].c_str();
}

inline const ParamDef& paramDef(uint32_t i) {
    static ParamDef t[kParamCount];
    static bool done = false;
    if (!done) {
        done = true;
        auto set = [](uint32_t i, const char* n, const char* s, const char* u, float lo, float hi, uint32_t h) { t[i] = { n, s, u, lo, hi, h }; };
        const uint32_t I = kHintInteger, B = kHintBoolean | kHintInteger;
        set(kO1Wave, "Osc 1 Wave", "o1wave", "", 0, kNumWaves - 1, I);   set(kO2Wave, "Osc 2 Wave", "o2wave", "", 0, kNumWaves - 1, I);
        set(kO1Oct, "Osc 1 Octave", "o1oct", "", -3, 3, I);   set(kO2Oct, "Osc 2 Octave", "o2oct", "", -3, 3, I);
        set(kO1Semi, "Osc 1 Semitone", "o1semi", "st", -12, 12, I); set(kO2Semi, "Osc 2 Semitone", "o2semi", "st", -12, 12, I);
        set(kO1Fine, "Osc 1 Fine", "o1fine", "ct", -100, 100, 0);  set(kO2Fine, "Osc 2 Fine", "o2fine", "ct", -100, 100, 0);
        set(kO1PW, "Osc 1 Pulse Width", "o1pw", "", 0.05f, 0.95f, 0); set(kO2PW, "Osc 2 Pulse Width", "o2pw", "", 0.05f, 0.95f, 0);
        set(kO1Level, "Osc 1 Level", "o1lvl", "", 0, 1, 0);   set(kO2Level, "Osc 2 Level", "o2lvl", "", 0, 1, 0);
        set(kO1Pan, "Osc 1 Pan", "o1pan", "", -1, 1, kHintBipolar);       set(kO2Pan, "Osc 2 Pan", "o2pan", "", -1, 1, kHintBipolar);
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
            set(b + 2, nm[f][2], sy[f][2], "", 0, kNumShapes - 1, I);
            set(b + 3, nm[f][3], sy[f][3], "", 0, kNumSlopes - 1, I);
            set(b + 4, nm[f][4], sy[f][4], "Hz", 20, 20000, kHintLog);
            set(b + 5, nm[f][5], sy[f][5], "", 0, 1, 0);
            set(b + 6, nm[f][6], sy[f][6], "", -1, 1, kHintBipolar);
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
        set(kVoices, "Voices", "voices", "", 1, kMaxVoices, I);     set(kUnison, "Unison", "unison", "", 1, 8, I);
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
        set(kOutLearn, "MIDI Learn", "o_learn", "", 0, 10000000, kHintOutput);
        // ---- 3.0 (names and symbols are kept in static storage)
        static std::string ns[kParamCount], ys[kParamCount];
        auto set3 = [&](uint32_t i, const std::string& n, const std::string& y, const char* u, float lo, float hi, uint32_t h) { ns[i] = n; ys[i] = y; set(i, ns[i].c_str(), ys[i].c_str(), u, lo, hi, h); };
        for (int o = 2; o < kNumOsc; ++o) {
            const std::string n = "Osc " + std::to_string(o + 1) + " ", y = "o" + std::to_string(o + 1);
            set3(opar(o, kOWave), n + "Wave", y + "wave", "", 0, kNumWaves - 1, I); set3(opar(o, kOOct), n + "Octave", y + "oct", "", -3, 3, I);
            set3(opar(o, kOSemi), n + "Semitone", y + "semi", "st", -12, 12, I); set3(opar(o, kOFine), n + "Fine", y + "fine", "ct", -100, 100, 0);
            set3(opar(o, kOPW), n + "Pulse Width", y + "pw", "", 0.05f, 0.95f, 0); set3(opar(o, kOLevel), n + "Level", y + "lvl", "", 0, 1, 0);
            set3(opar(o, kOPan), n + "Pan", y + "pan", "", -1, 1, kHintBipolar); set3(opar(o, kOPhase), n + "Phase Sync", y + "phase", "", 0, 1, B);
        }
        for (int o = 0; o < kNumOsc; ++o) {
            set3(kOscOnFirst + o, "Osc " + std::to_string(o + 1) + " On", "o" + std::to_string(o + 1) + "on", "", 0, 1, B);
            set3(kOscRatioFirst + o, "Osc " + std::to_string(o + 1) + " Sync", "o" + std::to_string(o + 1) + "ratio", "", 1, 16, 0);
        }
        set3(kRing12, "Ring Mod 1x2", "ring12", "", 0, 1, B); set3(kRing34, "Ring Mod 3x4", "ring34", "", 0, 1, B);
        set3(kDrift, "Drift", "drift", "", 0, 1, 0); set3(kTune, "Master Tune", "tune", "ct", -100, 100, kHintBipolar);
        set3(kTranspose, "Transpose", "transpose", "oct", -3, 3, I); set3(kPolyMode, "Per Oscillator", "polymode", "", 0, 1, B);
        set3(kHq, "High Quality", "hq", "", 0, 1, B);
        { static const char* fn[kFBaseFields] = { "On", "Style", "Type", "Slope", "Freq", "Peak", "Pan" }; static const char* fs[kFBaseFields] = { "on", "style", "type", "slope", "freq", "peak", "pan" };
          for (int f = 2; f < kNumFilters; ++f) for (int k = 0; k < kFBaseFields; ++k) {
            const std::string n = "Filter " + std::to_string(f + 1) + " " + fn[k], y = "f" + std::to_string(f + 1) + fs[k];
            switch (k) { case kFOn: set3(fpar(f, k), n, y, "", 0, 1, B); break; case kFStyle: set3(fpar(f, k), n, y, "", 0, kNumStyles - 1, I); break;
                         case kFType: set3(fpar(f, k), n, y, "", 0, kNumShapes - 1, I); break; case kFSlope: set3(fpar(f, k), n, y, "", 0, kNumSlopes - 1, I); break;
                         case kFFreq: set3(fpar(f, k), n, y, "Hz", 20, 20000, kHintLog); break; case kFPeak: set3(fpar(f, k), n, y, "", 0, 1, 0); break;
                         default: set3(fpar(f, k), n, y, "", -1, 1, kHintBipolar); break; } } }
        for (int f = 0; f < kNumFilters; ++f) set3(fpar(f, kFGain), "Filter " + std::to_string(f + 1) + " Gain", "f" + std::to_string(f + 1) + "gain", "dB", -24, 24, kHintBipolar);
        set3(kFreqOffset, "Filter Freq Offset", "freqoff", "oct", -5, 5, kHintBipolar); set3(kPeakOffset, "Filter Peak Offset", "peakoff", "", -1, 1, kHintBipolar);
        set3(kRevOn, "Reverb On", "revon", "", 0, 1, B); set3(kRevMix, "Reverb Amount", "revmix", "", 0, 1, 0); set3(kRevTime, "Reverb Time", "revtime", "s", 0.2f, 20, kHintLog);
        set3(kRevBright, "Reverb Brightness", "revbright", "", 0, 1, 0); set3(kRevPredelay, "Reverb Predelay", "revpre", "s", 0, 0.25f, 0);
        set3(kDlyOn, "Delay On", "dlyon", "", 0, 1, B); set3(kDlyMix, "Delay Amount", "dlymix", "", 0, 1, 0); set3(kDlySync, "Delay Sync", "dlysync", "", 0, 1, B);
        set3(kDlyTime, "Delay Time", "dlytime", "s", 0.001f, 2, kHintLog); set3(kDlyDiv, "Delay Note", "dlydiv", "", 0, kNumArpDivs - 1, I);
        set3(kDlyOffset, "Delay Stereo Offset", "dlyoff", "", -0.5f, 0.5f, kHintBipolar); set3(kDlyFeedback, "Delay Feedback", "dlyfb", "", 0, 1, 0);
        set3(kDlyPingPong, "Delay Ping-Pong", "dlypp", "", 0, 1, B); set3(kDlyLowCut, "Delay Low Cut", "dlylow", "Hz", 20, 2000, kHintLog); set3(kDlyHighCut, "Delay High Cut", "dlyhigh", "Hz", 500, 20000, kHintLog);
        set3(kChoOn, "Chorus On", "choon", "", 0, 1, B); set3(kChoMix, "Chorus Amount", "chomix", "", 0, 1, 0); set3(kChoMode, "Chorus Mode", "chomode", "", 0, 1, I);
        set3(kChoRate, "Chorus Rate", "chorate", "Hz", 0.02f, 5, kHintLog); set3(kChoDepth, "Chorus Depth", "chodepth", "", 0, 1, 0); set3(kChoDelay, "Chorus Delay", "chodelay", "s", 0.002f, 0.03f, 0);
        set3(kPhsOn, "Phaser On", "phson", "", 0, 1, B); set3(kPhsMix, "Phaser Amount", "phsmix", "", 0, 1, 0); set3(kPhsMode, "Phaser Mode", "phsmode", "", 0, 1, I);
        set3(kPhsRate, "Phaser Rate", "phsrate", "Hz", 0.02f, 5, kHintLog); set3(kPhsDepth, "Phaser Depth", "phsdepth", "", 0, 1, 0);
        set3(kPhsFeedback, "Phaser Feedback", "phsfb", "", 0, 1, 0); set3(kPhsShift, "Phaser Stereo Shift", "phsshift", "", 0, 1, 0);
        set3(kDrvOn, "Drive On", "drvon", "", 0, 1, B); set3(kDrvAmount, "Drive", "drive", "", 0, 1, 0); set3(kDrvPost, "Drive Position", "drvpost", "", 0, 1, I);
        set3(kCmpOn, "Compressor On", "cmpon", "", 0, 1, B); set3(kCmpThreshold, "Comp Threshold", "cmpthr", "dB", -60, 0, 0); set3(kCmpRatio, "Comp Ratio", "cmpratio", "", 1, 20, kHintLog);
        set3(kCmpAttack, "Comp Attack", "cmpatk", "s", 0.0005f, 0.2f, kHintLog); set3(kCmpRelease, "Comp Release", "cmprel", "s", 0.01f, 2, kHintLog);
        set3(kCmpAutoGain, "Comp Auto Gain", "cmpauto", "", 0, 1, B); set3(kCmpMix, "Comp Amount", "cmpmix", "", 0, 1, 0);
        set3(kArpOn, "Arp On", "arpon", "", 0, 1, B); set3(kArpRate, "Arp Rate", "arprate", "Hz", 0.25f, 20, kHintLog); set3(kArpSync, "Arp Sync", "arpsync", "", 0, 1, B);
        set3(kArpDiv, "Arp Note", "arpdiv", "", 0, kNumArpDivs - 1, I); set3(kArpOffset, "Arp Rate Offset", "arpoff", "x", 0.5f, 2, kHintLog);
        set3(kArpOrder, "Arp Order", "arporder", "", 0, kNumArpOrders - 1, I); set3(kArpOctaves, "Arp Octaves", "arpoct", "", 0, 3, I);
        set3(kArpGroove, "Arp Groove", "arpgroove", "", -0.9f, 0.9f, kHintBipolar); set3(kArpGate, "Arp Legato", "arpgate", "", 0.05f, 1, 0);
        set3(kArpLatch, "Arp Latch", "arplatch", "", 0, 2, I); set3(kArpRetrig, "Arp Retrigger", "arpretrig", "", 0, 1, B); set3(kArpLock, "Arp Lock", "arplock", "", 0, 1, B);
        static const char* const xn[em::kLStep1] = { "Mode", "Wave", "Rate", "Sync", "Division", "Balance", "Glide", "Steps" };
        static const char* const xs[em::kLStep1] = { "mode", "wave", "rate", "sync", "div", "bal", "glide", "steps" };
        for (int l = 0; l < em::kNumXlfo; ++l) for (int f = 0; f < em::kXlfoFields; ++f) {
            const em::FieldRange r = em::xlfoRange(f); const std::string pre = "XLFO " + std::to_string(l + 1) + " ", sy = "x" + std::to_string(l + 1);
            if (f < em::kLStep1) set3(lp_(l, f), pre + xn[f], sy + xs[f], f == em::kLRate ? "Hz" : "", r.min, r.max, (r.integer ? I : 0) | (r.log ? kHintLog : 0));
            else set3(lp_(l, f), pre + "Step " + std::to_string(f - em::kLStep1 + 1), sy + "s" + std::to_string(f - em::kLStep1 + 1), "", -1, 1, kHintBipolar);
        }
        static const char* const en[em::kEgFields] = { "Attack", "Decay", "Sustain", "Release", "Trigger", "Threshold" };
        static const char* const es[em::kEgFields] = { "a", "d", "s", "r", "trig", "thr" };
        static const char* const eu[em::kEgFields] = { "s", "s", "", "s", "", "dB" };
        for (int e = 0; e < em::kNumEg; ++e) for (int f = 0; f < em::kEgFields; ++f) { const em::FieldRange r = em::egRange(f);
            set3(ep(e, f), "EG " + std::to_string(e + 1) + " " + en[f], "e" + std::to_string(e + 1) + es[f], eu[f], r.min, r.max, (r.integer ? I : 0) | (r.log ? kHintLog : 0)); }
        static const char* const fn2[em::kEfFields] = { "Attack", "Release", "Gain", "Source" };
        static const char* const fs2[em::kEfFields] = { "a", "r", "gain", "src" };
        static const char* const fu2[em::kEfFields] = { "s", "s", "dB", "" };
        for (int e = 0; e < em::kNumEf; ++e) for (int f = 0; f < em::kEfFields; ++f) { const em::FieldRange r = em::efRange(f);
            set3(fp(e, f), "EF " + std::to_string(e + 1) + " " + fn2[f], "ef" + std::to_string(e + 1) + fs2[f], fu2[f], r.min, r.max, (r.integer ? I : 0) | (r.log ? kHintLog : 0)); }
        for (int x = 0; x < em::kNumXy; ++x) { set3(xyp(x, 0), "XY " + std::to_string(x + 1) + " X", "xy" + std::to_string(x + 1) + "x", "", 0, 1, 0); set3(xyp(x, 1), "XY " + std::to_string(x + 1) + " Y", "xy" + std::to_string(x + 1) + "y", "", 0, 1, 0); }
        static const char* const sn2[em::kSlotFields] = { "Source", "Destination", "Amount", "Bypass" };
        static const char* const ss2[em::kSlotFields] = { "src", "dst", "amt", "byp" };
        for (int s = 0; s < kNumSlots2; ++s) for (int f = 0; f < em::kSlotFields; ++f) {
            const std::string n = "Mod " + std::to_string(s + 1) + " " + sn2[f], y = "m" + std::to_string(s + 1) + ss2[f];
            if (f == kSlotSrc) set3(sp(s, f), n, y, "", 0, em::kNumSources - 1, I);
            else if (f == kSlotDst) set3(sp(s, f), n, y, "", 0, kNumDests2 - 1, I);
            else if (f == kSlotAmt) set3(sp(s, f), n, y, "", -1, 1, kHintBipolar);
            else set3(sp(s, f), n, y, "", 0, 1, B);
        }
        for (int l = 0; l < em::kNumXlfo; ++l) set3(kXlfoRetrigFirst + l, "XLFO " + std::to_string(l + 1) + " Per Note", "x" + std::to_string(l + 1) + "note", "", 0, 1, B);
        for (int e = 0; e < em::kNumEg; ++e) { static const char* const c[3] = { "Attack", "Decay", "Release" }; static const char* const cs[3] = { "ca", "cd", "cr" };
            for (int k = 0; k < 3; ++k) set3(kEgCurveFirst + e * 3 + k, "EG " + std::to_string(e + 1) + " " + c[k] + " Curve", "e" + std::to_string(e + 1) + cs[k], "", -1, 1, kHintBipolar); }
        for (int e = 0; e < em::kNumEf; ++e) set3(kEfModeFirst + e, "EF " + std::to_string(e + 1) + " Transient", "ef" + std::to_string(e + 1) + "trans", "", 0, 1, B);
    }
    return t[i];
}

inline const char* const* enumLabels(uint32_t p, int& count) {
    static const char* const wave[] = { "Triangle", "Saw", "Square", "Sine", "White Noise", "Pink Noise" };
    static const char* const style[] = { "Smooth", "Hot", "Raw", "Monolith", "Clean", "Warm", "Acid", "Tube", "Gentle", "Metal", "Hollow" };
    static const char* const type[] = { "Low Pass", "Band Pass", "High Pass", "Bell", "Low Shelf", "High Shelf", "Notch" };
    static const char* const slope[] = { "12 dB/oct", "24 dB/oct", "48 dB/oct", "6 dB/oct" };
    static const char* const route[] = { "Serial", "Parallel", "Per Oscillator" };
    static const char* const lw[] = { "Triangle", "Sine", "Square", "Random", "Sample & Hold" };
    static const char* const sync[] = { "Free", "Straight", "Dotted", "Triplet" };
    static const char* const div[] = { "16 bars", "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
    static const char* const gm[] = { "Always", "Legato" };
    switch (p) {
    case kO1Wave: case kO2Wave: case kO3Wave: case kO4Wave: count = kNumWaves; return wave;
    case kF1Style: case kF2Style: case kF3Style: case kF4Style: count = kNumStyles; return style;
    case kF1Type: case kF2Type: case kF3Type: case kF4Type: count = kNumShapes; return type;
    case kF1Slope: case kF2Slope: case kF3Slope: case kF4Slope: count = kNumSlopes; return slope;
    case kRouting: count = 3; return route;
    case kL1Wave: case kL2Wave: case kL3Wave: count = 5; return lw;
    case kL1Sync: case kL2Sync: case kL3Sync: count = 4; return sync;
    case kL1Div: case kL2Div: case kL3Div: count = 11; return div;
    case kGlideMode: count = 2; return gm;
    default: break;
    }
    {
        static const char* const chm[] = { "Single", "Double" };
        static const char* const phm[] = { "Phaser", "Flanger" };
        static const char* const dpos[] = { "Pre", "Post" };
        static const char* const ord[] = { "Up", "Down", "Up/Down", "As Played", "Random" };
        static const char* const lat[] = { "Off", "Held", "Full" };
        static const char* const oc[] = { "0", "+1", "+2", "+3" };
        static const char* adiv[kNumArpDivs]; static bool ai = false; if (!ai) { ai = true; for (int k = 0; k < kNumArpDivs; ++k) adiv[k] = arpDivName(k); }
        static const char* const xmode[] = { "Wave", "Steps" };
        static const char* const xwave[] = { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Random", "Sample & Hold" };
        static const char* const etrig[] = { "Note", "Legato" };
        static const char* const efs[] = { "Voice", "Synth" };
        static const char* const sync4[] = { "Free", "Straight", "Dotted", "Triplet" };
        static const char* const div11[] = { "16 bars", "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
        switch (p) {
        case kChoMode: count = 2; return chm;
        case kPhsMode: count = 2; return phm;
        case kDrvPost: count = 2; return dpos;
        case kArpOrder: count = kNumArpOrders; return ord;
        case kArpLatch: count = 3; return lat;
        case kArpOctaves: count = 4; return oc;
        case kArpDiv: case kDlyDiv: count = kNumArpDivs; return adiv;
        default: break;
        }
        if (p >= kXlfoFirst && p < kEgFirst) {
            const int f = (int)((p - kXlfoFirst) % em::kXlfoFields);
            if (f == em::kLMode) { count = 2; return xmode; }
            if (f == em::kLWave) { count = em::kNumWaves; return xwave; }
            if (f == em::kLSync) { count = 4; return sync4; }
            if (f == em::kLDiv) { count = 11; return div11; }
        }
        if (p >= kEgFirst && p < kEfFirst && (p - kEgFirst) % em::kEgFields == em::kETrigger) { count = 2; return etrig; }
        if (p >= kEfFirst && p < kXyFirst && (p - kEfFirst) % em::kEfFields == em::kFSource) { count = 2; return efs; }
        if (p >= kSlot2First && p < kXlfoRetrigFirst) {
            static const char* srcs2[em::kNumSources]; static const char* dsts2[kNumDests2]; static bool init2 = false;
            if (!init2) { init2 = true; for (int i = 0; i < em::kNumSources; ++i) srcs2[i] = em::sourceName(i); for (int i = 0; i < kNumDests2; ++i) dsts2[i] = destName2(i); }
            const int f = (int)((p - kSlot2First) % em::kSlotFields);
            if (f == kSlotSrc) { count = em::kNumSources; return srcs2; }
            if (f == kSlotDst) { count = kNumDests2; return dsts2; }
        }
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
// v2 slot amounts use a square-law curve, so the small depths pitch needs are easy to set
inline bool isSlotAmount2(uint32_t p) { return p >= kSlot2First && p < kXlfoRetrigFirst && (p - kSlot2First) % em::kSlotFields == kSlotAmt; }
inline bool isSetting(uint32_t p) { return p < kParamCount && !(paramDef(p).hints & kHintOutput); }
inline float toNorm(uint32_t p, float v) {
    const ParamDef& d = paramDef(p);
    if (isSlotAmount2(p)) return 0.5f + 0.5f * std::copysign(std::sqrt(std::fabs(clampf(v, -1.f, 1.f))), v);
    if (d.hints & kHintLog) return std::log(v / d.min) / std::log(d.max / d.min);
    float n = (v - d.min) / (d.max - d.min);
    if (isTimeParam(p)) n = std::sqrt(std::max(n, 0.f));
    return n;
}
inline float fromNorm(uint32_t p, float n) {
    const ParamDef& d = paramDef(p);
    n = clampf(n, 0.f, 1.f);
    if (isSlotAmount2(p)) { const float u = 2.f * n - 1.f; return u * std::fabs(u); }
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
// which v2 destination a knob maps to, or -1
inline int destForParam2(uint32_t p) {
    const int c = destForParam(p);
    if (c > 0 && c < kDstSlotFirst) return c;
    for (int o = 2; o < kNumOsc; ++o) {
        const int b = kV2O3First + (o - 2) * 4;
        if (p == opar(o, kOOct) || p == opar(o, kOSemi) || p == opar(o, kOFine)) return b;
        if (p == opar(o, kOPW)) return b + 1;
        if (p == opar(o, kOLevel)) return b + 2;
        if (p == opar(o, kOPan)) return b + 3;
    }
    for (int o = 0; o < kNumOsc; ++o) if (p == kOscRatioFirst + (uint32_t)o) return kV2RatioFirst + o;
    for (int f = 0; f < kNumFilters; ++f) {
        if (p == fpar(f, kFGain)) return kV2GainFirst + f;
        if (f >= 2) { const int b = kV2F3First + (f - 2) * 3; if (p == fpar(f, kFFreq)) return b; if (p == fpar(f, kFPeak)) return b + 1; if (p == fpar(f, kFPan)) return b + 2; }
    }
    switch (p) {
    case kTune: return kV2Tune; case kGlide: return kV2Glide; case kUniSpread: return kV2Spread;
    case kFreqOffset: return kV2FreqOffset; case kPeakOffset: return kV2PeakOffset;
    case kRevMix: return kV2RevMix; case kRevTime: return kV2RevTime; case kRevBright: return kV2RevBright; case kRevPredelay: return kV2RevPredelay;
    case kDlyMix: return kV2DlyMix; case kDlyTime: case kDlyDiv: return kV2DlyTime; case kDlyFeedback: return kV2DlyFeedback;
    case kChoMix: return kV2ChoMix; case kChoRate: return kV2ChoRate; case kChoDepth: return kV2ChoDepth;
    case kPhsMix: return kV2PhsMix; case kPhsRate: return kV2PhsRate; case kPhsDepth: return kV2PhsDepth; case kPhsFeedback: return kV2PhsFeedback;
    case kDrvAmount: return kV2DrvAmount; case kCmpThreshold: return kV2CmpThreshold; case kCmpMix: return kV2CmpMix;
    case kArpRate: case kArpOffset: return kV2ArpRate;
    default: break;
    }
    for (int l = 0; l < em::kNumXlfo; ++l) { if (p == lp_(l, em::kLRate) || p == lp_(l, em::kLDiv)) return kV2XlfoRateFirst + l; if (p == lp_(l, em::kLBalance)) return kV2XlfoBalFirst + l; }
    for (int e = 0; e < em::kNumEg; ++e) { static const int f[4] = { em::kEAttack, em::kEDecay, em::kESustain, em::kERelease }; for (int k = 0; k < 4; ++k) if (p == ep(e, f[k])) return kV2EgFirst + e * 4 + k; }
    if (isSlotAmount2(p)) return kV2SlotFirst + (int)((p - kSlot2First) / em::kSlotFields);
    return -1;
}

} // namespace obsidian
