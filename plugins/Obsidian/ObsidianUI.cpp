// Obsidian — custom vector UI (DPF/NanoVG). Original design. License: MIT.
#include "DistrhoUI.hpp"
#include "ObsidianPlugin.hpp"
#include "UserPresets.hpp"
#include "ComponentPresets.hpp"
#include "Help.hpp"
#include "ResizeHandle.hpp"
#include <cctype>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
extern "C" __declspec(dllimport) void* __stdcall ShellExecuteA(void*, const char*, const char*, const char*, const char*, int);
#endif

START_NAMESPACE_DISTRHO

static void openURL(const char* url) {
#if defined(_WIN32)
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, 1);
#elif defined(__APPLE__)
    const std::string c = std::string("open '") + url + "' &"; if (std::system(c.c_str())) {}
#else
    const std::string c = std::string("xdg-open '") + url + "' >/dev/null 2>&1 &"; if (std::system(c.c_str())) {}
#endif
}
static bool gTipsOn = true;
static double nowSec() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

using namespace obsidian;
using DGL_NAMESPACE::ResizeHandle;
typedef std::complex<float> cplx;

static constexpr float W = 1240.f, H = 866.f;

#define C_BG        Color(0x0f, 0x10, 0x12)
#define C_PANEL     Color(0x18, 0x1a, 0x1d)
#define C_PANEL_HI  Color(0x22, 0x25, 0x29)
#define C_PANEL_LO  Color(0x1c, 0x1e, 0x22)
#define C_LINE      Color(0x2c, 0x30, 0x35)
#define C_TEXT      Color(0xe8, 0xe4, 0xdc)
#define C_DIM       Color(0x84, 0x88, 0x8e)
#define C_FAINT     Color(0x4a, 0x4e, 0x54)
#define C_ACC       Color(0xb4, 0x9c, 0xff)       // obsidian violet
#define C_DISPLAY   Color(0x0b, 0x0c, 0x0d)
#define C_FAV       Color(0xff, 0xc8, 0x5a)

static Color withAlpha(const Color& c, float a) { Color r(c); r.alpha = a; return r; }
static Color oscColor(int o) { static const Color c[kNumOsc] = { Color(0xb4, 0x9c, 0xff), Color(0x5f, 0xd0, 0xc0), Color(0xff, 0xa8, 0x3e), Color(0xe0, 0x8c, 0xd8) }; return c[o & 3]; }
static Color filterColor(int f) { static const Color c[kNumFilters] = { Color(0xff, 0xa8, 0x3e), Color(0x5f, 0xd0, 0xc0), Color(0xb4, 0x9c, 0xff), Color(0xe4, 0xd4, 0x5a) }; return c[f & 3]; }
// chips: v2 sources use their em::Source id (1..17); the classic sources (LFO 1-3, Env 1-2, Amp Env) use 100..105
static constexpr int kClassicChip = 100, kNumClassicChips = 6;
static Color srcColor(int s) {
    if (s >= kClassicChip) return Color(0xc8, 0xc0, 0xb0);
    if (s >= em::kSrcXlfo1 && s <= em::kSrcXlfo4) return Color(0x5f, 0xd0, 0xc0);
    if (s == em::kSrcEg1 || s == em::kSrcEg2) return Color(0xff, 0xa8, 0x3e);
    if (s == em::kSrcEf1 || s == em::kSrcEf2) return Color(0x7f, 0xe0, 0x8a);
    if (s >= em::kSrcXy1X && s <= em::kSrcXy2Y) return Color(0x4f, 0xb0, 0xe8);
    if (s >= em::kSrcVelocity) return Color(0xd8, 0xb4, 0xff);
    return C_DIM;
}
static const char* chipShort(int s) {
    if (s >= kClassicChip) { static const char* const n[kNumClassicChips] = { "LFO 1", "LFO 2", "LFO 3", "ENV 1", "ENV 2", "AMP" }; return n[(s - kClassicChip) % kNumClassicChips]; }
    return em::sourceShort(s);
}
static const char* chipName(int s) {
    if (s >= kClassicChip) { static const char* const n[kNumClassicChips] = { "Classic LFO 1", "Classic LFO 2", "Classic LFO 3", "Classic Env 1", "Classic Env 2", "Amp Env" }; return n[(s - kClassicChip) % kNumClassicChips]; }
    return em::sourceName(s);
}
static const char* classicSrcShort(int s) {
    static const char* n[kNumSources] = { "-", "LFO 1", "LFO 2", "LFO 3", "ENV 1", "ENV 2", "AMP", "VEL", "AT", "KEY", "BEND", "WHEEL" };
    return n[s < 0 || s >= kNumSources ? 0 : s];
}
static const char* waveShort(int w) { static const char* const n[kNumWaves] = { "TRI", "SAW", "SQUARE", "SINE", "WHITE", "PINK" }; return n[w < 0 ? 0 : (w >= kNumWaves ? kNumWaves - 1 : w)]; }

// ---------------------------------------------------------------- layout
static constexpr float RA = 62, RAH = 240, RB = 310, RBH = 210, RC = 528, RCH = 310, STATUS_Y = 852;
static constexpr float OX0 = 16, OCOLW = 151, OW0 = 4 * OCOLW;                             // oscillators
static constexpr float FX0 = 628, FW0 = 596;                                                 // filters
static constexpr float FT_X = 642, FT_Y = RA + 8, FT_W = 52, FT_H = 24;
static constexpr float STYLE_X = 874, STYLE_Y = RA + 8, STYLE_W = 140;
static constexpr float DX = 642, DY = RA + 40, DW = 356, DH = 150;                           // filter display
static constexpr float AX = 16, AW = 290, VX = 314, VW = 262, EX = 584, EW = 640;           // amp, voice, effects
static constexpr float FXU_X = 594, FXU_W = 64, FXD_X = 986, FXD_W = 230;
static constexpr float MX = 16, MY = RC, MW = 580, MH = RCH;                               // modulation sources
static constexpr float SX = 604, SY = RC, SW = 340, SH = RCH;                              // modulation list
static constexpr float PX = 952, PY = RC, PW = 272, PH = RCH;                              // arpeggiator
static constexpr float SEG_H = 20, CHIP_W = 62, CHIP_H = 18;
static constexpr float TAB_Y = MY + 18, TAB_W = 42, TAB_H = 20;
static constexpr int kNumTabs = 13, kTabClassicL = 11, kTabClassicE = 12;
static constexpr float SLOT_ROWH = 22, SLOT_COLW = 320, SLOT_Y0 = SY + 34; static constexpr int SLOT_ROWS = 11;
static constexpr float STEP_X = MX + 16, STEP_Y = MY + 108, STEP_W = 380, STEP_H = 168;
static constexpr float XY_X = MX + 20, XY_Y = MY + 44, XY_W = 300, XY_H = 236;
static constexpr float COMP_W = 110, COMP_H = 20, COMP_X = MX + MW - 14 - COMP_W, COMP_Y = MY + 78;
static constexpr float EGD_X = MX + 16, EGD_Y = MY + 206, EGD_W = 420, EGD_H = 84;          // EG display with curve handles
static constexpr float CL_COLW = 188;
static constexpr float ARPP_X = PX + 112, ARPP_Y = PY + 8, ARPP_W = 72, ARPP_H = 20;
static constexpr float ARPV_X = PX + 14, ARPV_Y = PY + 240, ARPV_W = PW - 28, ARPV_H = 56;

enum WType { Knob, Toggle, Segment, Stepper };
enum Group { GGlobal, GOsc, GFilt, GXlfo, GEg, GEf, GClassicL, GClassicE, GFx };
struct Ctl { WType type; int group, field, idx; float x, y, r; const char* label; };   // Segment/Stepper: r = width

static const char* const kFxNames[6] = { "REVERB", "DELAY", "CHORUS", "PHASER", "DRIVE", "COMP" };
static uint32_t fxOnParam(int u) { static const uint32_t p[6] = { kRevOn, kDlyOn, kChoOn, kPhsOn, kDrvOn, kCmpOn }; return p[u]; }
static uint32_t fxAmtParam(int u) { static const uint32_t p[6] = { kRevMix, kDlyMix, kChoMix, kPhsMix, kDrvAmount, kCmpThreshold }; return p[u]; }
static float fxUnitX(int u) { return FXU_X + 32 + u * FXU_W; }

static std::vector<Ctl> buildControls() {
    std::vector<Ctl> c;
    // oscillators (field: OField, or -1 on, -2 sync ratio, -3 classic 1>2 sync)
    for (int o = 0; o < kNumOsc; ++o) {
        const float x0 = OX0 + o * OCOLW;
        c.push_back({ Toggle, GOsc, -1, o, x0 + 22, RA + 18, 0, "" });
        c.push_back({ Stepper, GOsc, kOWave, o, x0 + 106, RA + 18, 74, "" });
        c.push_back({ Knob, GOsc, kOOct, o, x0 + 28, RA + 98, 13, "OCT" });
        c.push_back({ Knob, GOsc, kOSemi, o, x0 + 75, RA + 98, 13, "SEMI" });
        c.push_back({ Knob, GOsc, kOFine, o, x0 + 122, RA + 98, 13, "FINE" });
        c.push_back({ Knob, GOsc, kOLevel, o, x0 + 28, RA + 152, 13, "LEVEL" });
        c.push_back({ Knob, GOsc, kOPan, o, x0 + 75, RA + 152, 13, "PAN" });
        c.push_back({ Knob, GOsc, kOPW, o, x0 + 122, RA + 152, 13, "PW" });
        c.push_back({ Knob, GOsc, -2, o, x0 + 28, RA + 206, 13, "SYNC" });
        c.push_back({ Toggle, GOsc, kOPhase, o, x0 + 66, RA + 196, 0, "PHASE" });
        if (o == 1) c.push_back({ Toggle, GOsc, -3, o, x0 + 66, RA + 220, 0, "1>2 SYNC" });
    }
    // filters (field: of the selected filter)
    c.push_back({ Segment, GGlobal, kRouting, 0, 1130, RA + 20, 176, "" });
    c.push_back({ Segment, GFilt, kFType, 0, DX + DW / 2, RA + 214, DW, "" });
    c.push_back({ Segment, GFilt, kFSlope, 0, 1110, RA + 214, 196, "" });
    c.push_back({ Knob, GFilt, kFFreq, 0, 1042, RA + 82, 18, "FREQ" });
    c.push_back({ Knob, GFilt, kFPeak, 0, 1110, RA + 82, 15, "PEAK" });
    c.push_back({ Knob, GFilt, kFGain, 0, 1176, RA + 82, 15, "GAIN" });
    c.push_back({ Knob, GFilt, kFPan, 0, 1042, RA + 150, 15, "PAN" });
    c.push_back({ Knob, GGlobal, kFreqOffset, 0, 1110, RA + 150, 15, "FREQ OFS" });
    c.push_back({ Knob, GGlobal, kPeakOffset, 0, 1176, RA + 150, 15, "PEAK OFS" });
    // amp
    static const uint32_t ap[6] = { kE3Attack, kE3Hold, kE3Decay, kE3Sustain, kE3Release, kAmpVel };
    static const char* const al[6] = { "ATTACK", "HOLD", "DECAY", "SUSTAIN", "RELEASE", "VEL" };
    for (int i = 0; i < 6; ++i) c.push_back({ Knob, GGlobal, (int)ap[i], 0, AX + 34 + i * 45.f, RB + 146, 13, al[i] });
    // voice
    c.push_back({ Toggle, GGlobal, kHq, 0, VX + 196, RB + 18, 0, "HQ" });
    c.push_back({ Stepper, GGlobal, kVoices, 0, VX + 36, RB + 56, 56, "VOICES" });
    c.push_back({ Stepper, GGlobal, kUnison, 0, VX + 100, RB + 56, 56, "UNISON" });
    c.push_back({ Knob, GGlobal, kUniDetune, 0, VX + 162, RB + 56, 13, "DETUNE" });
    c.push_back({ Knob, GGlobal, kUniSpread, 0, VX + 222, RB + 56, 13, "SPREAD" });
    c.push_back({ Knob, GGlobal, kGlide, 0, VX + 36, RB + 116, 13, "GLIDE" });
    c.push_back({ Toggle, GGlobal, kGlideMode, 0, VX + 82, RB + 116, 0, "LEGATO" });
    c.push_back({ Stepper, GGlobal, kBendRange, 0, VX + 222, RB + 116, 52, "BEND" });
    c.push_back({ Toggle, GGlobal, kPolyMode, 0, VX + 82, RB + 140, 0, "PER OSC" });
    c.push_back({ Knob, GGlobal, kTune, 0, VX + 36, RB + 176, 13, "TUNE" });
    c.push_back({ Stepper, GGlobal, kTranspose, 0, VX + 100, RB + 176, 56, "OCTAVE" });
    c.push_back({ Knob, GGlobal, kDrift, 0, VX + 162, RB + 176, 13, "DRIFT" });
    c.push_back({ Knob, GGlobal, kVolume, 0, VX + 222, RB + 176, 13, "VOLUME" });
    // effects: the unit strip
    for (int u = 0; u < 6; ++u) {
        c.push_back({ Toggle, GGlobal, (int)fxOnParam(u), 0, fxUnitX(u) - 2, RB + 48, 0, "" });
        c.push_back({ Knob, GGlobal, (int)fxAmtParam(u), 0, fxUnitX(u), RB + 112, 18, u == 4 ? "DRIVE" : (u == 5 ? "THRESH" : "AMOUNT") });
    }
    // effects: the selected unit's settings
    auto fxk = [&](int u, uint32_t p, int col, int row, const char* l) { c.push_back({ Knob, GFx, (int)p, u, FXD_X + 34 + col * 76.f, RB + 70 + row * 64.f, 14, l }); };
    auto fxt = [&](int u, uint32_t p, int col, int row, const char* l) { c.push_back({ Toggle, GFx, (int)p, u, FXD_X + 22 + col * 76.f, RB + 62 + row * 64.f, 0, l }); };
    auto fxs = [&](int u, uint32_t p, int col, int row, float w) { c.push_back({ Segment, GFx, (int)p, u, FXD_X + 34 + col * 76.f + w / 2 - 34, RB + 62 + row * 64.f, w, "" }); };
    fxk(0, kRevTime, 0, 0, "TIME"); fxk(0, kRevBright, 1, 0, "BRIGHT"); fxk(0, kRevPredelay, 2, 0, "PREDELAY");
    fxk(1, kDlyTime, 0, 0, "TIME"); fxk(1, kDlyFeedback, 1, 0, "FEEDBACK"); fxk(1, kDlyOffset, 2, 0, "OFFSET");
    fxk(1, kDlyLowCut, 0, 1, "LOW CUT"); fxk(1, kDlyHighCut, 1, 1, "HIGH CUT");
    fxt(1, kDlySync, 0, 2, "SYNC"); fxt(1, kDlyPingPong, 1, 2, "PING-PONG");
    fxs(2, kChoMode, 0, 0, 140); fxk(2, kChoRate, 0, 1, "RATE"); fxk(2, kChoDepth, 1, 1, "DEPTH"); fxk(2, kChoDelay, 2, 1, "DELAY");
    fxs(3, kPhsMode, 0, 0, 140); fxk(3, kPhsRate, 0, 1, "RATE"); fxk(3, kPhsDepth, 1, 1, "DEPTH"); fxk(3, kPhsFeedback, 2, 1, "FEEDBACK"); fxk(3, kPhsShift, 0, 2, "SHIFT");
    fxs(4, kDrvPost, 0, 0, 140);
    fxk(5, kCmpRatio, 0, 0, "RATIO"); fxk(5, kCmpAttack, 1, 0, "ATTACK"); fxk(5, kCmpRelease, 2, 0, "RELEASE"); fxk(5, kCmpMix, 0, 1, "AMOUNT"); fxt(5, kCmpAutoGain, 1, 1, "AUTO GAIN");
    // XLFO editor
    c.push_back({ Segment, GXlfo, em::kLMode, 0, MX + 82, MY + 62, 130, "" });
    c.push_back({ Segment, GXlfo, em::kLSync, 0, MX + 296, MY + 62, 250, "" });
    c.push_back({ Segment, GXlfo, em::kLWave, 0, MX + 206, MY + 88, 380, "" });
    c.push_back({ Toggle, GXlfo, -1, 0, MX + 472, MY + 108, 0, "PER NOTE" });
    c.push_back({ Knob, GXlfo, em::kLRate, 0, MX + 452, MY + 160, 20, "RATE" });
    c.push_back({ Stepper, GXlfo, em::kLDiv, 0, MX + 452, MY + 160, 74, "" });
    c.push_back({ Knob, GXlfo, em::kLBalance, 0, MX + 530, MY + 160, 20, "BALANCE" });
    c.push_back({ Knob, GXlfo, em::kLGlide, 0, MX + 530, MY + 160, 20, "GLIDE" });
    c.push_back({ Stepper, GXlfo, em::kLSteps, 0, MX + 452, MY + 228, 74, "" });
    // EG editor
    c.push_back({ Segment, GEg, em::kETrigger, 0, MX + 116, MY + 62, 170, "" });
    c.push_back({ Knob, GEg, em::kEAttack, 0, MX + 60, MY + 146, 22, "ATTACK" });
    c.push_back({ Knob, GEg, em::kEDecay, 0, MX + 140, MY + 146, 22, "DECAY" });
    c.push_back({ Knob, GEg, em::kESustain, 0, MX + 220, MY + 146, 22, "SUSTAIN" });
    c.push_back({ Knob, GEg, em::kERelease, 0, MX + 300, MY + 146, 22, "RELEASE" });
    // EF editor
    c.push_back({ Segment, GEf, em::kFSource, 0, MX + 126, MY + 62, 190, "" });
    c.push_back({ Knob, GEf, em::kFAttack, 0, MX + 60, MY + 150, 22, "ATTACK" });
    c.push_back({ Knob, GEf, em::kFRelease, 0, MX + 140, MY + 150, 22, "RELEASE" });
    c.push_back({ Knob, GEf, em::kFGain, 0, MX + 220, MY + 150, 22, "GAIN" });
    c.push_back({ Toggle, GEf, -1, 0, MX + 300, MY + 150, 0, "TRANSIENT" });
    // classic LFOs (field: parameter)
    for (int l = 0; l < 3; ++l) {
        const float x0 = MX + 14 + l * CL_COLW; const uint32_t b = kL1Wave + 6 * l;
        c.push_back({ Segment, GClassicL, (int)b, l, x0 + 88, MY + 80, 172, "" });
        c.push_back({ Segment, GClassicL, (int)(b + 2), l, x0 + 88, MY + 106, 172, "" });
        c.push_back({ Knob, GClassicL, (int)(b + 3), l, x0 + 46, MY + 210, 17, "RATE" });
        c.push_back({ Knob, GClassicL, (int)(b + 1), l, x0 + 128, MY + 210, 15, "BALANCE" });
        c.push_back({ Toggle, GClassicL, (int)(b + 5), l, x0 + 20, MY + 270, 0, "NOTE SYNC" });
    }
    // classic envelopes
    for (int e = 0; e < 2; ++e) {
        const float x0 = MX + 14 + e * CL_COLW; const uint32_t b = e == 0 ? kE1Delay : kE2Delay;
        static const char* const n[5] = { "DLY", "A", "D", "S", "R" };
        for (int k = 0; k < 5; ++k) c.push_back({ Knob, GClassicE, (int)(b + k), e, x0 + 16 + k * 37.f, MY + 210, 12, n[k] });
        c.push_back({ Toggle, GClassicE, (int)(b + 5), e, x0 + 20, MY + 270, 0, "INVERT" });
    }
    // arpeggiator
    c.push_back({ Toggle, GGlobal, kArpOn, 0, PX + 202, PY + 18, 0, "ON" });
    c.push_back({ Segment, GGlobal, kArpOrder, 0, PX + PW / 2, PY + 58, PW - 28, "" });
    c.push_back({ Segment, GGlobal, kArpOctaves, 0, PX + 74, PY + 104, 120, "" });
    c.push_back({ Segment, GGlobal, kArpLatch, 0, PX + PW - 74, PY + 104, 120, "" });
    c.push_back({ Knob, GGlobal, kArpRate, 0, PX + 38, PY + 158, 15, "RATE" });
    c.push_back({ Knob, GGlobal, kArpOffset, 0, PX + 102, PY + 158, 15, "OFFSET" });
    c.push_back({ Knob, GGlobal, kArpGroove, 0, PX + 168, PY + 158, 15, "GROOVE" });
    c.push_back({ Knob, GGlobal, kArpGate, 0, PX + 232, PY + 158, 15, "LEGATO" });
    c.push_back({ Toggle, GGlobal, kArpSync, 0, PX + 30, PY + 214, 0, "SYNC" });
    c.push_back({ Toggle, GGlobal, kArpRetrig, 0, PX + 112, PY + 214, 0, "RETRIG" });
    c.push_back({ Toggle, GGlobal, kArpLock, 0, PX + 200, PY + 214, 0, "LOCK" });
    return c;
}
static const std::vector<Ctl>& controls() { static std::vector<Ctl> c = buildControls(); return c; }

static const char* const* segLabels(int group, int field, int& n) {
    static const char* const route[] = { "SERIAL", "PARALLEL", "PER OSC" };
    static const char* const shape[] = { "LP", "BP", "HP", "BELL", "LO SH", "HI SH", "NOTCH" };
    static const char* const slope[] = { "6 DB", "12 DB", "24 DB", "48 DB" };
    static const char* const lm[] = { "WAVE", "STEPS" };
    static const char* const sy[] = { "FREE", "SYNC", "DOTTED", "TRIPLET" };
    static const char* const wv[] = { "SIN", "TRI", "SAW+", "SAW-", "SQR", "RND", "S&H" };
    static const char* const tr[] = { "NOTE", "LEGATO" };
    static const char* const efs[] = { "VOICE", "SYNTH" };
    static const char* const cw[] = { "TRI", "SIN", "SQR", "RND", "S&H" };
    static const char* const cs[] = { "FREE", "STR", "DOT", "TRIP" };
    static const char* const cho[] = { "SINGLE", "DOUBLE" };
    static const char* const phm[] = { "PHASER", "FLANGER" };
    static const char* const dp[] = { "PRE", "POST" };
    static const char* const ord[] = { "UP", "DOWN", "UP/DN", "PLAYED", "RANDOM" };
    static const char* const oc[] = { "0", "+1", "+2", "+3" };
    static const char* const lat[] = { "OFF", "HELD", "FULL" };
    n = 0;
    if (group == GGlobal) {
        switch (field) {
        case kRouting: n = 3; return route;
        case kArpOrder: n = 5; return ord;
        case kArpOctaves: n = 4; return oc;
        case kArpLatch: n = 3; return lat;
        default: return nullptr;
        }
    }
    if (group == GFilt) { if (field == kFType) { n = 7; return shape; } if (field == kFSlope) { n = 4; return slope; } }
    if (group == GXlfo) { if (field == em::kLMode) { n = 2; return lm; } if (field == em::kLSync) { n = 4; return sy; } if (field == em::kLWave) { n = 7; return wv; } }
    if (group == GEg && field == em::kETrigger) { n = 2; return tr; }
    if (group == GEf && field == em::kFSource) { n = 2; return efs; }
    if (group == GClassicL) {
        if (field == kL1Wave || field == kL2Wave || field == kL3Wave) { n = 5; return cw; }
        if (field == kL1Sync || field == kL2Sync || field == kL3Sync) { n = 4; return cs; }
    }
    if (group == GFx) { if (field == kChoMode) { n = 2; return cho; } if (field == kPhsMode) { n = 2; return phm; } if (field == kDrvPost) { n = 2; return dp; } }
    return nullptr;
}

// header:  logo · preset bar · A B COPY · UNDO REDO · LEARN MIDI · HELP
static constexpr float PS_X = 484.f, PS_Y = 18.f, PS_W = 260.f, PS_H = 28.f, PS_ARROW = 30.f;
static constexpr float A_X = 756.f, AB_W = 30.f, COPY_X = 818.f, COPY_W = 46.f;
static constexpr float UNDO_X = 874.f, REDO_X = 926.f, UR_W = 48.f;
static constexpr float LEARN_X = 984.f, LEARN_W = 70.f, MIDI_X = 1060.f, MIDI_W = 50.f, HELP_X = 1116.f, HELP_W = 50.f;
static constexpr float PM_COLW = 121.f, PM_ROW = 22.f, PM_HEAD = 22.f, PM_PAD = 8.f;

// a filter's response (linear prototype of the engine's filters)
static cplx filterH(const float* v, int f, float hz) {
    const int type = (int)(v[fpar(f, kFType)] + .5f);
    int stages = slopeStages((int)(v[fpar(f, kFSlope)] + .5f));
    const float oct = std::log2(v[fpar(f, kFFreq)]) + v[kFreqOffset];
    const float fc = std::pow(2.f, oct), reso = clampf(v[fpar(f, kFPeak)] + v[kPeakOffset], 0.f, 1.f), r = std::pow(reso, 0.6f);
    const StyleDef& sd = styleDef((int)(v[fpar(f, kFStyle)] + .5f));
    const float w = hz / fc, G = std::pow(10.f, v[fpar(f, kFGain)] / 20.f);
    const cplx s(0, w);
    cplx h(1, 0);
    if (stages == 0 && type != kBandPass && type != kNotch && type != kBell) {
        const cplx lo = 1.f / (s + 1.f), hi = s / (s + 1.f);
        switch (type) { case kLowPass: return lo; case kHighPass: return hi; case kLowShelf: return 1.f + (G - 1.f) * lo; default: return 1.f + (G - 1.f) * hi; }
    }
    const bool wide = stages == 0; if (stages == 0) stages = 1;
    if (type <= kHighPass) {
        const float* kb = bwK(stages);
        for (int i = 0; i < stages; ++i) {
            const float k = std::max(i == stages - 1 ? kb[i] * (1 - r) + sd.kMin * r : kb[i], 0.035f);
            const cplx den = s * s + k * s + 1.f;
            h *= type == kLowPass ? 1.f / den : (type == kHighPass ? s * s / den : s * std::max(k, 0.12f) / den);
        }
        return h;
    }
    const float k = std::max(eqDamping(reso) * (wide ? 2.f : 1.f), 0.02f), gs = std::pow(G, 1.f / stages);
    for (int i = 0; i < stages; ++i) {
        const cplx den = s * s + k * s + 1.f, bp = k * s / den, lp = 1.f / den, hp = s * s / den;
        switch (type) { case kBell: h *= 1.f + (gs - 1.f) * bp; break; case kLowShelf: h *= 1.f + (gs - 1.f) * lp; break;
                        case kHighShelf: h *= 1.f + (gs - 1.f) * hp; break; default: h *= 1.f - bp; break; }
    }
    return h;
}

class ObsidianUI : public UI {
public:
    ObsidianUI() : UI((uint)W, (uint)H), resizeHandle(this) {
        loadSharedResources();
        Engine def;
        for (uint32_t i = 0; i < kParamCount; ++i) { val[i] = def.p[i]; defv[i] = def.p[i]; }
        for (int& m : paramCC) m = -1;
        const double sf = getScaleFactor();
        if (sf != 1.0) setSize((uint)(W * sf), (uint)(H * sf));
        setGeometryConstraints((uint)(W * 0.6 * sf), (uint)(H * 0.6 * sf), true);
        if (isResizable()) resizeHandle.hide();
        userPresets = listUserPresets();
        std::copy(val, val + kParamCount, before);
        std::fill(liveOff, liveOff + kNumDests2, 0.f); std::fill(liveSrc, liveSrc + em::kNumSources, 0.f); std::fill(liveClassic, liveClassic + kNumSources, 0.f);
    }

protected:
    float uiScale() const { return (float)getWidth() / W; }

    // ---- values ---------------------------------------------------------------
    void setValue(uint32_t p, float v) {
        const ParamDef& d = paramDef(p); v = clampf(v, d.min, d.max);
        if (d.hints & kHintInteger) v = std::round(v);
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); repaint();
    }
    void commit(uint32_t p, float v) { editParameter(p, true); setValue(p, v); editParameter(p, false); }
    void parameterChanged(uint32_t i, float v) override {
        if (i >= kParamCount) return;
        if (i == kOutLearn) {
            if (v != lastLearnEcho && v > 0) {
                const int code = (int)v % 1000000 - 1, prm = code / 128, cc = code % 128;
                for (int& m : paramCC) if (m == cc) m = -1;
                if (prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
                learnWaiting = -1;
            }
            lastLearnEcho = v; repaint(); return;
        }
        if (val[i] != v) { val[i] = v; if (!gesture && isSetting(i)) before[i] = v; repaint(); }
    }
    void stateChanged(const char* key, const char* value) override {
        if (std::strcmp(key, "midimap") != 0) return;
        for (int& m : paramCC) m = -1;
        const char* q = value;
        while (q && *q) { int cc = -1, prm = -1; if (std::sscanf(q, "%d:%d", &cc, &prm) == 2 && prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
            const char* s = std::strchr(q, ';'); if (!s) break; q = s + 1; }
        repaint();
    }

    // ---- undo / redo / A-B ----------------------------------------------------------
    typedef std::vector<float> Snap;
    Snap snapshot() const { Snap s(val, val + kParamCount); s.push_back((float)abSlot); return s; }
    static bool differs(const float* a, const float* b) { for (uint32_t i = 0; i < kParamCount; ++i) if (isSetting(i) && a[i] != b[i]) return true; return false; }
    void beginGesture() { gesture = true; std::copy(val, val + kParamCount, before); }
    void endGesture() {
        gesture = false;
        if (differs(before, val)) { Snap u(before, before + kParamCount); u.push_back((float)abSlot); undoStack.push_back(u); if (undoStack.size() > 200) undoStack.erase(undoStack.begin()); redoStack.clear(); }
        std::copy(val, val + kParamCount, before); repaint();
    }
    void applyAll(const float* s) { for (uint32_t i = 0; i < kParamCount; ++i) if (isSetting(i) && val[i] != s[i]) commit(i, s[i]); }
    void restoreSnap(const Snap& s) {
        if (s.size() > kParamCount && (int)s[kParamCount] != abSlot) { abStore[abSlot] = snapshot(); abSlot = (int)s[kParamCount]; }
        applyAll(s.data()); std::copy(val, val + kParamCount, before);
    }
    void undo() { if (undoStack.empty()) return; redoStack.push_back(snapshot()); restoreSnap(undoStack.back()); undoStack.pop_back(); flash("Undo"); }
    void redo() { if (redoStack.empty()) return; undoStack.push_back(snapshot()); restoreSnap(redoStack.back()); redoStack.pop_back(); flash("Redo"); }
    void switchAB(int to) {
        if (to == abSlot) return;
        abStore[abSlot] = snapshot(); if (abStore[to].empty()) abStore[to] = snapshot();
        beginGesture(); applyAll(abStore[to].data()); endGesture();
        abSlot = to; flash(to == 0 ? "Sound A" : "Sound B");
    }
    void copyAB() { abStore[1 - abSlot] = snapshot(); flash(abSlot == 0 ? "Copied A to B" : "Copied B to A"); }

    // ---- parameter of a control -------------------------------------------------------
    static int clampTab(int t, int n) { return t < 0 ? 0 : (t >= n ? n - 1 : t); }
    uint32_t effParam(const Ctl& c) const {
        switch (c.group) {
        case GOsc: return c.field == -1 ? kOscOnFirst + c.idx : (c.field == -2 ? kOscRatioFirst + c.idx : (c.field == -3 ? (uint32_t)kOscSync : opar(c.idx, c.field)));
        case GFilt: return fpar(selF, c.field);
        case GXlfo: { const int l = std::min(tab, em::kNumXlfo - 1); return c.field == -1 ? kXlfoRetrigFirst + l : lp_(l, c.field); }
        case GEg: return ep(clampTab(tab - 4, em::kNumEg), c.field);
        case GEf: { const int e = clampTab(tab - 6, em::kNumEf); return c.field == -1 ? kEfModeFirst + e : fp(e, c.field); }
        case GClassicL: {
            const uint32_t p = (uint32_t)c.field;
            for (int l = 0; l < 3; ++l) { const uint32_t b = kL1Wave + 6 * l; if (p == b + 3 && val[b + 2] > 0.5f) return b + 4; }
            return p; }
        default: {
            const uint32_t p = (uint32_t)c.field;
            if (p == kDlyTime && val[kDlySync] > 0.5f) return kDlyDiv;
            if (p == kArpRate && val[kArpSync] > 0.5f) return kArpDiv;
            return p; }
        }
    }
    bool visible(const Ctl& c) const {
        if (c.group == GXlfo) {
            if (tab > 3) return false;
            const int l = tab; const bool steps = val[lp_(l, em::kLMode)] > 0.5f, sync = val[lp_(l, em::kLSync)] > 0.5f;
            if (c.field == em::kLWave || c.field == em::kLBalance) return !steps;
            if (c.field == em::kLGlide || c.field == em::kLSteps) return steps;
            if (c.field == em::kLRate) return !sync;
            if (c.field == em::kLDiv) return sync;
            return true;
        }
        if (c.group == GEg) return tab == 4 || tab == 5;
        if (c.group == GEf) return tab == 6 || tab == 7;
        if (c.group == GClassicL) return tab == kTabClassicL;
        if (c.group == GClassicE) return tab == kTabClassicE;
        if (c.group == GFx) return c.idx == selFx;
        return true;
    }
    bool fOn(int f) const { return val[fpar(f, kFOn)] > 0.5f; }
    bool oOn(int o) const { return val[kOscOnFirst + o] > 0.5f; }
    int filtersOn() const { int n = 0; for (int f = 0; f < kNumFilters; ++f) n += fOn(f); return n; }
    bool dimmed(uint32_t p) const {
        for (int o = 0; o < kNumOsc; ++o) {
            if (p == opar(o, kOPW)) return !oOn(o) || (int)std::round(val[opar(o, kOWave)]) != 2;
            for (int k = 0; k < kOFields; ++k) if (p == opar(o, k)) return !oOn(o);
            if (p == (uint32_t)(kOscRatioFirst + o)) return !oOn(o);
        }
        for (int f = 0; f < kNumFilters; ++f) {
            if (p == fpar(f, kFGain)) return !fOn(f) || !isEqShape((int)std::round(val[fpar(f, kFType)]));
            for (int k = 1; k < kFBaseFields; ++k) if (p == fpar(f, k)) return !fOn(f);
        }
        if (p == kRouting) return filtersOn() < 2 && (int)std::round(val[kRouting]) != kSplit;
        if (p == kUniDetune || p == kUniSpread) return val[kUnison] < 1.5f;
        if (p == kArpOffset) return val[kArpSync] < 0.5f;
        for (int u = 0; u < 6; ++u) if (p == fxAmtParam(u)) return val[fxOnParam(u)] < 0.5f;
        if (p == kRevTime || p == kRevBright || p == kRevPredelay) return val[kRevOn] < 0.5f;
        if (p >= kDlySync && p <= kDlyHighCut) return val[kDlyOn] < 0.5f;
        if (p >= kChoMode && p <= kChoDelay) return val[kChoOn] < 0.5f;
        if (p >= kPhsMode && p <= kPhsShift) return val[kPhsOn] < 0.5f || (p == kPhsShift && false);
        if (p == kDrvPost) return val[kDrvOn] < 0.5f;
        if (p >= kCmpRatio && p <= kCmpMix) return val[kCmpOn] < 0.5f;
        if (p >= kArpRate && p <= kArpLock && p != kArpLock) return val[kArpOn] < 0.5f;
        return false;
    }

    // ---- formatting ---------------------------------------------------------
    static bool isPan(uint32_t p) {
        for (int o = 0; o < kNumOsc; ++o) if (p == opar(o, kOPan)) return true;
        for (int f = 0; f < kNumFilters; ++f) if (p == fpar(f, kFPan)) return true;
        return false;
    }
    void fmtValue(uint32_t p, float v, char* b, size_t n) const {
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) { snprintf(b, n, "%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))]); return; }
        const ParamDef& d = paramDef(p);
        if (d.hints & kHintBoolean) { snprintf(b, n, "%s", v > 0.5f ? "On" : "Off"); return; }
        if (isPan(p)) { if (std::fabs(v) < .005f) snprintf(b, n, "Center"); else snprintf(b, n, "%s %.0f%%", v < 0 ? "L" : "R", std::fabs(v) * 100); return; }
        for (int o = 0; o < kNumOsc; ++o) {
            if (p == opar(o, kOOct)) { snprintf(b, n, "%+.0f oct", v); return; }
            if (p == opar(o, kOSemi)) { snprintf(b, n, "%+.0f st", v); return; }
            if (p == opar(o, kOFine)) { snprintf(b, n, "%+.0f ct", v); return; }
            if (p == (uint32_t)(kOscRatioFirst + o)) { if (v < 1.005f) snprintf(b, n, "Off"); else snprintf(b, n, "x%.2f", v); return; }
        }
        if (p == kTranspose) { snprintf(b, n, "%+.0f", v); return; }
        if (p == kVoices || p == kUnison) { snprintf(b, n, "%.0f", v); return; }
        if (p == kBendRange) { snprintf(b, n, "%.0f st", v); return; }
        if (p == kTune) { snprintf(b, n, "%+.0f ct", v); return; }
        if (p == kArpOffset) { snprintf(b, n, "x%.2f", v); return; }
        if (p == kDlyOffset) { snprintf(b, n, "%s%.0f%%", v < 0 ? "L " : (v > 0 ? "R " : ""), std::fabs(v) * 200); return; }
        if (p == kCmpRatio) { snprintf(b, n, "%.1f:1", v); return; }
        if (p == kFreqOffset) { snprintf(b, n, "%+.2f oct", v); return; }
        if (isSlotAmount2(p) || (p >= kSlotFirst && p <= kSlotLast && (p - kSlotFirst) % 4 == kSlotAmt)) { const float a = std::fabs(v) * 100; snprintf(b, n, a < 1 && a > 0 ? "%+.2f%%" : "%+.1f%%", v * 100); return; }
        if (std::strcmp(d.unit, "Hz") == 0) { if (v >= 1000) snprintf(b, n, "%.2f kHz", v / 1000); else snprintf(b, n, v < 10 ? "%.2f Hz" : "%.0f Hz", v); return; }
        if (std::strcmp(d.unit, "dB") == 0) { snprintf(b, n, "%+.1f dB", v); return; }
        if (std::strcmp(d.unit, "s") == 0 || isTimeParam(p)) { if (v < 1) snprintf(b, n, "%.0f ms", v * 1000); else snprintf(b, n, "%.2f s", v); return; }
        if (p >= kXlfoFirst && p < kEgFirst && (p - kXlfoFirst) % em::kXlfoFields == em::kLSteps) { snprintf(b, n, "%.0f", v); return; }
        if (d.hints & kHintBipolar) { snprintf(b, n, "%+.0f%%", v * 100); return; }
        snprintf(b, n, "%.0f%%", v * 100);
    }
    void fmt(uint32_t p, char* b, size_t n) const { fmtValue(p, val[p], b, n); }
    bool parse(uint32_t p, const std::string& in, float& out) const {
        std::string s; for (char ch : in) if (!std::isspace((unsigned char)ch)) s += (char)std::tolower((unsigned char)ch);
        if (s.empty()) return false;
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) {
            for (int i = 0; i < ne; ++i) { std::string lab; for (const char* c = l[i]; *c; ++c) if (*c != ' ') lab += (char)std::tolower((unsigned char)*c);
                if (lab.compare(0, s.size(), s) == 0) { out = (float)i; return true; } }
            return false;
        }
        if (isPan(p)) {
            if (s[0] == 'c') { out = 0; return true; }
            const float sign = s[0] == 'l' ? -1.f : 1.f;
            out = sign * (float)std::atof((s[0] == 'l' || s[0] == 'r') ? s.c_str() + 1 : s.c_str()) / 100.f; return true;
        }
        char* end = nullptr; double x = std::strtod(s.c_str()[0] == 'x' ? s.c_str() + 1 : s.c_str(), &end);
        if (end == s.c_str()) return false;
        const std::string suf(end); const ParamDef& d = paramDef(p);
        if (suf == "k" || suf == "khz") x *= 1000;
        if (std::strcmp(d.unit, "s") == 0 || isTimeParam(p)) { out = (float)(suf == "ms" ? x / 1000 : x); return true; }
        if (d.unit[0] != 0 || (d.hints & kHintInteger) || p == kCmpRatio || (p >= kOscRatioFirst && p < kOscRatioFirst + kNumOsc)) { out = (float)x; return true; }
        out = (float)(x / 100.0); return true;
    }

    // ---- drawing --------------------------------------------------------------
    void onNanoDisplay() override {
        const float s = uiScale();
        save(); scale(s, s);
        beginPath(); rect(0, 0, W, H); fillColor(C_BG); fill();
        fontFace(NANOVG_DEJAVU_SANS_TTF);
        drawHeader();
        drawOscPanel();
        drawFilterPanel();
        drawAmpPanel();
        drawVoicePanel();
        drawFxPanel();
        drawSources();
        drawSlots();
        drawArpPanel();
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) if (visible(cs[i])) drawCtl(cs[i], i == active || i == hover);
        drawStatus();
        if (editing >= 0) drawEditBox();
        if (dragSrc > 0) drawChip(dragSrc, dragPX - CHIP_W / 2, dragPY - CHIP_H / 2, true, true);
        if (kpan.open) drawKnobPanel();
        if (popup.open) drawPopup();
        if (menuOpen) drawPresetMenu();
        if (saving) drawSaveBox();
        if (helpOpen) drawHelp();
        else if (tipShown && gTipsOn && dragSrc <= 0) drawTooltip();
        restore();
    }

    void panel(float x, float y, float w, float h, const char* title = "") {
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        if (title[0]) { fontSize(9.5f); textLetterSpacing(2.2f); fillColor(C_DIM); textAlign(ALIGN_LEFT | ALIGN_TOP); text(x + 14, y + 10, title, nullptr); }
    }
    void caption(float x, float y, const char* t, int align = ALIGN_CENTER) { fontSize(7.5f); textLetterSpacing(1.8f); textAlign(align | ALIGN_MIDDLE); fillColor(C_FAINT); text(x, y, t, nullptr); }
    void displayBox(float x, float y, float w, float h) { beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); strokeWidth(1); stroke(); }
    void drawDropButton(float x, float y, float w, float h, const char* label, bool hot) {
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(hot ? C_PANEL_HI : C_PANEL_LO); fill(); strokeColor(hot ? C_DIM : C_LINE); strokeWidth(1); stroke();
        fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot ? C_TEXT : C_DIM); text(x + 10, y + h / 2 + 0.5f, label, nullptr);
        beginPath(); const float ax = x + w - 12, ay = y + h / 2; moveTo(ax - 3.5f, ay - 2); lineTo(ax + 3.5f, ay - 2); lineTo(ax, ay + 2.5f); closePath(); fillColor(C_DIM); fill();
    }
    void drawHeaderButton(float x, float w, const char* label, bool on, bool hot, bool disabled = false) {
        beginPath(); roundedRect(x, PS_Y, w, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.22f) : C_PANEL); fill();
        strokeColor(on ? C_ACC : (hot && !disabled ? C_DIM : C_LINE)); strokeWidth(1); stroke();
        fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(disabled ? C_FAINT : (on ? C_TEXT : C_DIM));
        text(x + w / 2, PS_Y + PS_H / 2 + 0.5f, label, nullptr);
    }
    void drawHeader() {
        beginPath(); moveTo(26, 17); lineTo(36, 25); lineTo(33, 46); lineTo(21, 42); lineTo(18, 26); closePath(); fillColor(C_ACC); fill();
        beginPath(); moveTo(26, 17); lineTo(33, 46); lineTo(21, 42); closePath(); fillColor(withAlpha(C_BG, 0.35f)); fill();
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(50, 32, "OBSIDIAN", nullptr);
        fontSize(9.5f); textLetterSpacing(2.5f); fillColor(C_DIM); text(256, 27, "POLYPHONIC SYNTHESIZER", nullptr);
        char vb[48]; snprintf(vb, sizeof vb, "EPSILON AUDIO 3.0   ·   %d VOICES", liveVoices);
        fontSize(7); textLetterSpacing(1.1f); fillColor(C_FAINT); text(256, 39, vb, nullptr);
        drawPresetBar();
        for (int k = 0; k < 2; ++k) {
            const float x = A_X + k * AB_W; const bool on = abSlot == k, hot = hoverHeader == 6 + k;
            beginPath(); roundedRect(x, PS_Y, AB_W - 2, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.25f) : C_PANEL); fill();
            strokeColor(on ? C_ACC : (hot ? C_DIM : C_LINE)); strokeWidth(1); stroke();
            fontSize(10); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM);
            text(x + (AB_W - 2) / 2, PS_Y + PS_H / 2 + 0.5f, k == 0 ? "A" : "B", nullptr);
        }
        drawHeaderButton(COPY_X, COPY_W, "COPY", false, hoverHeader == 8);
        drawHeaderButton(UNDO_X, UR_W, "UNDO", false, hoverHeader == 3, undoStack.empty());
        drawHeaderButton(REDO_X, UR_W, "REDO", false, hoverHeader == 4, redoStack.empty());
        drawHeaderButton(LEARN_X, LEARN_W, learnMode ? "LEARNING" : "LEARN", learnMode, hoverHeader == 1);
        drawHeaderButton(MIDI_X, MIDI_W, "MIDI", val[kMidiOn] > 0.5f, hoverHeader == 2);
        drawHeaderButton(HELP_X, HELP_W, "HELP", helpOpen, hoverHeader == 5);
        if (latencyMs > 0) { char b[24]; snprintf(b, sizeof b, "%.1f ms", latencyMs); fontSize(7); textLetterSpacing(1.4f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(C_FAINT); text(W - 14, 27, "LATENCY", nullptr); fontSize(9); fillColor(C_DIM); text(W - 14, 39, b, nullptr); }
    }

    // ---- oscillators ------------------------------------------------------------
    static float waveAt(int wave, float t, float pw, int i) {
        switch (wave) {
        case 0: return t < 0.5f ? 4 * t - 1 : 3 - 4 * t;
        case 1: return 2 * t - 1;
        case 2: return t < pw ? 1.f : -1.f;
        case 3: return std::sin(2 * 3.14159265f * t);
        case 4: { uint32_t r = (uint32_t)i * 2654435761u; r ^= r >> 13; return ((r & 0xFFFF) / 32768.f - 1.f) * 0.9f; }
        default: { float a = 0; for (int k = 0; k < 4; ++k) { uint32_t r = (uint32_t)(i >> k) * 2654435761u + k * 977u; r ^= r >> 13; a += ((r & 0xFFFF) / 32768.f - 1.f); } return a * 0.3f; }
        }
    }
    float ringX(int pair) const { return OX0 + (pair * 2 + 1) * OCOLW; }
    bool ringHit(int pair, float x, float y) const { return std::fabs(x - ringX(pair)) <= 9 && std::fabs(y - (RA + 56)) <= 9; }
    void drawOscPanel() {
        panel(OX0, RA, OW0, RAH);
        for (int o = 0; o < kNumOsc; ++o) {
            const float x0 = OX0 + o * OCOLW; const bool on = oOn(o); const Color col = oscColor(o);
            if (o) { beginPath(); moveTo(x0, RA + 10); lineTo(x0, RA + RAH - 10); strokeColor(C_LINE); strokeWidth(1); stroke(); }
            char t[8]; snprintf(t, sizeof t, "%d", o + 1);
            fontSize(9.5f); textLetterSpacing(1.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(on ? col : C_FAINT); text(x0 + 42, RA + 18.5f, t, nullptr);
            const float dx = x0 + 10, dy = RA + 34, dw = OCOLW - 20, dh = 40;
            displayBox(dx, dy, dw, dh);
            const int wave = (int)std::round(val[opar(o, kOWave)]); const float pw = val[opar(o, kOPW)], ratio = val[kOscRatioFirst + o];
            beginPath();
            for (int i = 0; i <= 100; ++i) {
                const float tt = i / 100.f; float ph = std::fmod(tt * 2.f, 1.f);
                if (ratio > 1.005f) ph = std::fmod(ph * ratio, 1.f);
                float v = waveAt(wave, ph, pw, i);
                if (o == 1 && val[kRing12] > 0.5f) v *= waveAt((int)std::round(val[opar(0, kOWave)]), std::fmod(tt * 2.f, 1.f), val[opar(0, kOPW)], i);
                if (o == 3 && val[kRing34] > 0.5f) v *= waveAt((int)std::round(val[opar(2, kOWave)]), std::fmod(tt * 2.f, 1.f), val[opar(2, kOPW)], i);
                const float px = dx + 4 + (dw - 8) * tt, py = dy + dh / 2 - v * (dh / 2 - 5);
                if (!i) moveTo(px, py); else lineTo(px, py);
            }
            strokeColor(withAlpha(col, on ? 0.95f : 0.25f)); strokeWidth(1.3f); stroke();
        }
        for (int pr = 0; pr < 2; ++pr) {          // ring modulation buttons between 1-2 and 3-4
            const bool on = val[pr ? kRing34 : kRing12] > 0.5f, hot = hoverRing == pr;
            const float cx = ringX(pr), cy = RA + 56;
            beginPath(); circle(cx, cy, 8.5f); fillColor(on ? withAlpha(C_ACC, 0.35f) : (hot ? C_PANEL_HI : C_PANEL)); fill(); strokeColor(on || hot ? C_ACC : C_LINE); strokeWidth(1); stroke();
            beginPath(); moveTo(cx - 3.5f, cy - 3.5f); lineTo(cx + 3.5f, cy + 3.5f); moveTo(cx + 3.5f, cy - 3.5f); lineTo(cx - 3.5f, cy + 3.5f); strokeColor(on ? C_TEXT : C_DIM); strokeWidth(1.3f); stroke();
            if (learnMode && (paramCC[pr ? kRing34 : kRing12] >= 0 || learnWaiting == (int)(pr ? kRing34 : kRing12))) learnBadge(cx, cy - 15, pr ? kRing34 : kRing12);
        }
    }

    // ---- filter display (handles: drag frequency and peak / gain) -----------------------
    static float xOfHz(float hz) { return DX + 8 + (DW - 16) * std::log(clampf(hz, 20.f, 20000.f) / 20.f) / std::log(1000.f); }
    static float hzOfX(float x) { return 20.f * std::pow(1000.f, (x - DX - 8) / (DW - 16)); }
    static float yOfDb(float db) { return DY + 6 + (DH - 22) * (1 - (db + 36.f) / 60.f); }
    static float dbOfY(float y) { return (1 - (y - DY - 6) / (DH - 22)) * 60.f - 36.f; }
    static float yOfPeak(float pk) { return DY + DH * 0.82f - pk * DH * 0.66f; }
    static float peakOfY(float y) { return (DY + DH * 0.82f - y) / (DH * 0.66f); }
    void handlePos(int f, float& x, float& y) const {
        x = xOfHz(val[fpar(f, kFFreq)] * std::pow(2.f, val[kFreqOffset]));
        const int t = (int)std::round(val[fpar(f, kFType)]);
        y = isEqShape(t) ? yOfDb(val[fpar(f, kFGain)]) : yOfPeak(val[fpar(f, kFPeak)]);
        y = clampf(y, DY + 10, DY + DH - 22);
    }
    cplx combined(float hz) const {
        const int route = (int)std::round(val[kRouting]);
        cplx acc(route == kParallel ? 0.f : 1.f, 0); int cnt = 0;
        for (int f = 0; f < kNumFilters; ++f) { if (!fOn(f)) continue; const cplx h = filterH(val, f, hz); if (route == kParallel) acc += h; else acc *= h; ++cnt; }
        if (!cnt) return cplx(1, 0);
        return route == kParallel ? acc * (cnt == 2 ? 0.7071f : 1.f / std::sqrt((float)cnt)) : acc;
    }
    void drawFilterPanel() {
        panel(FX0, RA, FW0, RAH);
        for (int f = 0; f < kNumFilters; ++f) {
            const float x = FT_X + f * (FT_W + 4); const bool sel = f == selF, on = fOn(f), hot = hoverFTab == f; const Color col = filterColor(f);
            beginPath(); roundedRect(x, FT_Y, FT_W, FT_H, 5); fillColor(sel ? withAlpha(col, 0.22f) : (hot ? C_PANEL_HI : C_PANEL_LO)); fill();
            strokeColor(sel ? col : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(x + 13, FT_Y + FT_H / 2, 5); fillColor(on ? col : C_PANEL_HI); fill(); strokeColor(withAlpha(col, 0.8f)); stroke();
            char t[8]; snprintf(t, sizeof t, "F%d", f + 1);
            fontSize(10.5f); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(sel ? C_TEXT : C_DIM); text(x + 34, FT_Y + FT_H / 2 + 0.5f, t, nullptr);
            if (learnMode && (paramCC[fpar(f, kFOn)] >= 0 || learnWaiting == (int)fpar(f, kFOn))) learnBadge(x + FT_W / 2, FT_Y - 4, fpar(f, kFOn));
        }
        const Color col = filterColor(selF);
        beginPath(); roundedRect(STYLE_X, STYLE_Y, STYLE_W, 24, 5); fillColor(hoverStyle ? C_PANEL_HI : C_PANEL_LO); fill(); strokeColor(withAlpha(col, 0.8f)); strokeWidth(1); stroke();
        const int st = (int)std::round(val[fpar(selF, kFStyle)]);
        fontSize(7.5f); textLetterSpacing(1.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(STYLE_X + 10, STYLE_Y + 12.5f, "STYLE", nullptr);
        fontSize(11); textLetterSpacing(0.6f); fillColor(C_TEXT); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); text(STYLE_X + STYLE_W - 24, STYLE_Y + 12.5f, styleName(st), nullptr);
        beginPath(); moveTo(STYLE_X + STYLE_W - 15, STYLE_Y + 10); lineTo(STYLE_X + STYLE_W - 8, STYLE_Y + 10); lineTo(STYLE_X + STYLE_W - 11.5f, STYLE_Y + 15); closePath(); fillColor(C_DIM); fill();
        caption(DX + DW / 2, RA + 199, "SHAPE"); caption(1110, RA + 199, "SLOPE");
        // display
        displayBox(DX, DY, DW, DH);
        const float grid[] = { 100, 1000, 10000 }; const char* gl[] = { "100", "1k", "10k" };
        beginPath(); for (float g : grid) { const float x = xOfHz(g); moveTo(x, DY + 4); lineTo(x, DY + DH - 14); }
        for (int db = -24; db <= 12; db += 12) { const float y = yOfDb((float)db); moveTo(DX + 3, y); lineTo(DX + DW - 3, y); }
        strokeColor(Color(255, 255, 255, 8 / 255.f)); strokeWidth(1); stroke();
        fontSize(7.5f); textLetterSpacing(1); textAlign(ALIGN_CENTER | ALIGN_BOTTOM); fillColor(C_FAINT); for (int i = 0; i < 3; ++i) text(xOfHz(grid[i]), DY + DH - 3, gl[i], nullptr);
        const int N = 180; std::vector<float> hz(N + 1); for (int i = 0; i <= N; ++i) hz[i] = hzOfX(DX + 8 + (DW - 16) * i / N);
        scissor(DX + 1, DY + 1, DW - 2, DH - 15);
        for (int f = kNumFilters - 1; f >= 0; --f) {
            if (!fOn(f) && f != selF) continue;
            const Color fc = filterColor(f); const float alpha = fOn(f) ? 1.f : 0.25f;
            std::vector<float> ys(N + 1);
            for (int i = 0; i <= N; ++i) ys[i] = clampf(yOfDb(20 * std::log10(std::max(std::abs(filterH(val, f, hz[i])), 1e-4f))), DY + 2, DY + DH - 14);
            beginPath(); moveTo(DX + 8, DY + DH - 14); for (int i = 0; i <= N; ++i) lineTo(DX + 8 + (DW - 16) * i / N, ys[i]); lineTo(DX + DW - 8, DY + DH - 14); closePath();
            fillPaint(linearGradient(0, DY, 0, DY + DH, withAlpha(fc, (f == selF ? 0.2f : 0.1f) * alpha), withAlpha(fc, 0.f))); fill();
            beginPath(); for (int i = 0; i <= N; ++i) { const float x = DX + 8 + (DW - 16) * i / N; if (!i) moveTo(x, ys[i]); else lineTo(x, ys[i]); }
            strokeColor(withAlpha(fc, 0.9f * alpha)); strokeWidth(f == selF ? 1.8f : 1.2f); stroke();
        }
        if (filtersOn() >= 2 && (int)std::round(val[kRouting]) != kSplit) {
            beginPath(); for (int i = 0; i <= N; ++i) { const float y = clampf(yOfDb(20 * std::log10(std::max(std::abs(combined(hz[i])), 1e-4f))), DY + 2, DY + DH - 14), x = DX + 8 + (DW - 16) * i / N; if (!i) moveTo(x, y); else lineTo(x, y); }
            strokeColor(Color(0xe8, 0xe4, 0xdc, 0.7f)); strokeWidth(1.2f); stroke();
        }
        resetScissor();
        for (int f = 0; f < kNumFilters; ++f) {
            if (!fOn(f)) continue;
            float hx, hy; handlePos(f, hx, hy); const bool hot = dragHandle == f || hoverHandle == f;
            if (f == selF) { beginPath(); circle(hx, hy, 12); strokeColor(withAlpha(filterColor(f), 0.6f)); strokeWidth(1.5f); stroke(); }
            beginPath(); circle(hx, hy, hot ? 9.5f : 8); fillColor(filterColor(f)); fill();
            char t[8]; snprintf(t, sizeof t, "%d", f + 1); fontSize(9.5f); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_BG); text(hx, hy + 0.5f, t, nullptr);
        }
        if (filtersOn() < kNumFilters && hoverDisplay && dragHandle < 0) { fontSize(7.5f); textLetterSpacing(1); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_FAINT); text(DX + DW - 6, DY + 4, "DOUBLE-CLICK TO ADD", nullptr); }
    }

    // ---- amp / voice / effects ----------------------------------------------------
    void drawAdsrH(float x, float y, float w, float h, float a, float hold, float d, float s, float r, const Color& col, float dly = 0) {
        displayBox(x, y, w, h);
        const float sus = 0.4f, tot = std::max(dly + a + hold + d + sus + r, 0.3f), sx = (w - 12) / tot, x0 = x + 5 + dly * sx, yb = y + h - 4, hh = h - 8;
        auto Y = [&](float lvl) { return yb - hh * lvl; };
        beginPath(); moveTo(x + 5, Y(0)); lineTo(x0, Y(0)); lineTo(x0 + a * sx, Y(1)); lineTo(x0 + (a + hold) * sx, Y(1));
        for (int i = 1; i <= 16; ++i) { const float t = i / 16.f; lineTo(x0 + (a + hold + d * t) * sx, Y(s + (1 - s) * std::exp(-4 * t))); }
        lineTo(x0 + (a + hold + d + sus) * sx, Y(s));
        for (int i = 1; i <= 16; ++i) { const float t = i / 16.f; lineTo(x0 + (a + hold + d + sus + r * t) * sx, Y(s * std::exp(-4 * t))); }
        strokeColor(col); strokeWidth(1.4f); stroke();
    }
    void drawAmpPanel() {
        panel(AX, RB, AW, RBH, "AMP ENVELOPE");
        drawAdsrH(AX + 14, RB + 30, AW - 28, 74, val[kE3Attack], val[kE3Hold], val[kE3Decay], val[kE3Sustain], val[kE3Release], C_ACC);
    }
    void drawVoicePanel() { panel(VX, RB, VW, RBH, "VOICE"); }
    void drawFxPanel() {
        panel(EX, RB, EW, RBH, "EFFECTS");
        for (int u = 0; u < 6; ++u) {
            const float cx = fxUnitX(u); const bool sel = u == selFx, on = val[fxOnParam(u)] > 0.5f, hot = hoverFx == u;
            beginPath(); roundedRect(cx - 30, RB + 28, 60, RBH - 38, 5); fillColor(sel ? withAlpha(C_ACC, 0.10f) : (hot ? C_PANEL_HI : C_PANEL_LO)); fill(); if (sel) { strokeColor(withAlpha(C_ACC, 0.6f)); strokeWidth(1); stroke(); }
            fontSize(7.5f); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(sel ? C_TEXT : (on ? C_DIM : C_FAINT)); text(cx, RB + 72, kFxNames[u], nullptr);
            if (u == 5 && on) { const float gr = clampf(liveGr / 24.f, 0.f, 1.f); beginPath(); rect(cx - 22, RB + 172, 44, 4); fillColor(C_DISPLAY); fill(); beginPath(); rect(cx + 22 - 44 * gr, RB + 172, 44 * gr, 4); fillColor(C_ACC); fill(); }
        }
        beginPath(); moveTo(FXD_X - 6, RB + 14); lineTo(FXD_X - 6, RB + RBH - 12); strokeColor(C_LINE); strokeWidth(1); stroke();
        std::string t = std::string(kFxNames[selFx]) + " SETTINGS";
        fontSize(8); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_DIM); text(FXD_X + 6, RB + 18, t.c_str(), nullptr);
        if (selFx == 4) { fontSize(9); textLetterSpacing(0.3f); fillColor(C_FAINT); textAlign(ALIGN_LEFT | ALIGN_TOP);
            textBox(FXD_X + 6, RB + 92, FXD_W - 12, "PRE: before every effect. POST: after the modulation effects, before the compressor. The big knob sets the drive.", nullptr); }
        if (selFx == 0) { fontSize(9); textLetterSpacing(0.3f); fillColor(C_FAINT); textAlign(ALIGN_LEFT | ALIGN_TOP);
            textBox(FXD_X + 6, RB + 128, FXD_W - 12, "Reverb runs first in the chain, then delay, chorus, phaser, drive (post) and the compressor.", nullptr); }
    }
    int fxUnitHit(float x, float y) const { for (int u = 0; u < 6; ++u) { const float cx = fxUnitX(u); if (inRect(x, y, cx - 30, RB + 62, 60, 20)) return u; } return -1; }

    // ---- arpeggiator ---------------------------------------------------------------
    void drawArpPanel() {
        panel(PX, PY, PW, PH, "ARPEGGIATOR");
        drawDropButton(ARPP_X, ARPP_Y, ARPP_W, ARPP_H, "PRESETS", hoverArpComp || (popup.open && popup.kind == 3 && compArp));
        caption(PX + 74, PY + 88, "OCTAVES"); caption(PX + PW - 74, PY + 88, "LATCH"); caption(PX + PW / 2, PY + 42, "ORDER");
        displayBox(ARPV_X, ARPV_Y, ARPV_W, ARPV_H);
        int notes[32]; int n = 0;
        if (ObsidianPlugin* dsp = (ObsidianPlugin*)getPluginInstancePointer()) n = dsp->engine.arpPattern(notes, 32);
        if (n == 0) { fontSize(8.5f); textLetterSpacing(1); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT); text(ARPV_X + ARPV_W / 2, ARPV_Y + ARPV_H / 2, val[kArpOn] > 0.5f ? "HOLD A CHORD" : "OFF", nullptr); return; }
        const int oct = (int)std::round(val[kArpOctaves]); const int span = n * (oct + 1);
        int lo = 127, hi = 0; for (int i = 0; i < n; ++i) { lo = std::min(lo, notes[i]); hi = std::max(hi, notes[i] + 12 * oct); }
        const float bw = (ARPV_W - 12) / std::max(span, 1);
        for (int k = 0; k < span && k < 64; ++k) {
            const int note = notes[k % n] + 12 * (k / n); const float y = ARPV_Y + ARPV_H - 6 - (ARPV_H - 14) * (hi > lo ? (note - lo) / (float)(hi - lo) : 0.5f);
            const bool now = note == liveArpNote;
            beginPath(); roundedRect(ARPV_X + 6 + k * bw + 1, y - 3, std::max(2.f, bw - 2), 6, 2); fillColor(now ? C_ACC : withAlpha(C_ACC, 0.35f)); fill();
        }
    }

    // ---- modulation sources ---------------------------------------------------------
    static const char* tabName(int t) { static const char* const n[kNumTabs] = { "XLFO 1", "XLFO 2", "XLFO 3", "XLFO 4", "EG 1", "EG 2", "EF 1", "EF 2", "XY 1", "XY 2", "MIDI", "LFO 1-3", "ENV 1-2" }; return n[t]; }
    static int tabSource(int t) { static const int s[kNumTabs] = { em::kSrcXlfo1, em::kSrcXlfo2, em::kSrcXlfo3, em::kSrcXlfo4, em::kSrcEg1, em::kSrcEg2, em::kSrcEf1, em::kSrcEf2, em::kSrcXy1X, em::kSrcXy2X, em::kSrcVelocity, kClassicChip, kClassicChip + 3 }; return s[t]; }
    static float tabX(int t) { return MX + 8 + t * (TAB_W + 1.6f); }
    struct Chip { int src; float x, y; };
    std::vector<Chip> chips() const {
        std::vector<Chip> c;
        if (tab <= 7) c.push_back({ tabSource(tab), MX + MW - CHIP_W - 14, MY + 52 });
        else if (tab <= 9) { const int x = tab - 8; c.push_back({ em::kSrcXy1X + 2 * x, MX + MW - CHIP_W - 14, MY + 52 }); c.push_back({ em::kSrcXy1Y + 2 * x, MX + MW - CHIP_W - 14, MY + 78 }); }
        else if (tab == 10) for (int i = 0; i < 5; ++i) c.push_back({ em::kSrcVelocity + i, MX + 24, MY + 52 + i * 40.f });
        else if (tab == kTabClassicL) for (int i = 0; i < 3; ++i) c.push_back({ kClassicChip + i, MX + 14 + i * CL_COLW + 112, MY + 40 });
        else for (int i = 0; i < 3; ++i) c.push_back({ kClassicChip + 3 + i, MX + 14 + i * CL_COLW + 112, MY + 40 });
        return c;
    }
    bool sourceUsed(int s) const {
        if (s >= kClassicChip) { const int cs = 1 + (s - kClassicChip);
            for (int k = 0; k < kNumSlots; ++k) if ((int)std::round(val[slotParam(k, kSlotSrc)]) == cs && (int)std::round(val[slotParam(k, kSlotDst)]) > 0 && val[slotParam(k, kSlotBypass)] < 0.5f) return true;
            return false; }
        for (int k = 0; k < kNumSlots2; ++k) if ((int)std::round(val[sp(k, kSlotSrc)]) == s && (int)std::round(val[sp(k, kSlotDst)]) > 0) return true;
        return false;
    }
    bool anyMidiUsed() const { for (int s = em::kSrcVelocity; s < em::kNumSources; ++s) if (sourceUsed(s)) return true; return false; }
    void drawSources() {
        panel(MX, MY, MW, MH);
        for (int t = 0; t < kNumTabs; ++t) {
            const float x = tabX(t); const bool on = t == tab, hot = t == hoverTab;
            const int s = tabSource(t);
            beginPath(); roundedRect(x, TAB_Y - TAB_H / 2, TAB_W, TAB_H, 4); fillColor(on ? withAlpha(srcColor(s), 0.25f) : (hot ? C_PANEL_HI : C_PANEL_LO)); fill();
            if (on) { strokeColor(srcColor(s)); strokeWidth(1); stroke(); }
            fontSize(7.f); textLetterSpacing(0.4f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(x + TAB_W / 2, TAB_Y + 0.5f, tabName(t), nullptr);
            bool used = false;
            if (t == kTabClassicL) { for (int k = 0; k < 3; ++k) used |= sourceUsed(kClassicChip + k); }
            else if (t == kTabClassicE) { for (int k = 3; k < 6; ++k) used |= sourceUsed(kClassicChip + k); }
            else used = sourceUsed(s) || (t >= 8 && t <= 9 && sourceUsed(s + 1)) || (t == 10 && anyMidiUsed());
            if (used) { beginPath(); circle(x + TAB_W - 5, TAB_Y - TAB_H / 2 + 4, 2); fillColor(srcColor(s)); fill(); }
        }
        if (tab <= 3) drawXlfo(tab);
        else if (tab <= 5) drawEg(tab - 4);
        else if (tab <= 7) drawEf(tab - 6);
        else if (tab <= 9) drawXy(tab - 8);
        else if (tab == 10) drawMidi();
        else if (tab == kTabClassicL) drawClassicL();
        else drawClassicE();
        for (const Chip& c : chips()) drawChip(c.src, c.x, c.y, hoverChip == c.src, false);
        if (tab <= 7) drawDropButton(COMP_X, COMP_Y, COMP_W, COMP_H, tab <= 3 ? "LFO PRESETS" : (tab <= 5 ? "EG PRESETS" : "EF PRESETS"), hoverComp || (popup.open && popup.kind == 3 && !compArp));
    }
    void drawChip(int src, float x, float y, bool hot, bool dragging) {
        const Color col = srcColor(src);
        beginPath(); roundedRect(x, y, CHIP_W, CHIP_H, CHIP_H / 2); fillColor(withAlpha(col, dragging ? 0.9f : (hot ? 0.35f : 0.16f))); fill(); strokeColor(col); strokeWidth(1); stroke();
        const float v = src >= kClassicChip ? liveClassic[1 + src - kClassicChip] : liveSrc[src];
        const bool bip = src >= kClassicChip ? src < kClassicChip + 3 : em::sourceBipolar(src);
        beginPath(); circle(x + 9, y + CHIP_H / 2, 3); fillColor(dragging ? C_BG : col); fill();
        if (!dragging) { const float bx = x + 17, bw = CHIP_W - 24, by = y + CHIP_H - 4;
            beginPath(); rect(bx, by, bw, 1.5f); fillColor(withAlpha(col, 0.25f)); fill();
            const float a = bip ? bx + bw / 2 : bx, b2 = bip ? bx + bw / 2 + clampf(v, -1.f, 1.f) * bw / 2 : bx + clampf(std::fabs(v), 0.f, 1.f) * bw;
            beginPath(); rect(std::min(a, b2), by, std::fabs(b2 - a) + 1, 1.5f); fillColor(col); fill(); }
        fontSize(8); textLetterSpacing(1.1f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(dragging ? C_BG : C_TEXT); text(x + CHIP_W / 2 + 5, y + CHIP_H / 2 - 0.5f, chipShort(src), nullptr);
    }
    void drawXlfo(int l) {
        caption(MX + 82, MY + 44, "MODE"); caption(MX + 296, MY + 44, "TEMPO SYNC");
        const bool steps = val[lp_(l, em::kLMode)] > 0.5f;
        displayBox(STEP_X, STEP_Y, STEP_W, STEP_H);
        const float mid = STEP_Y + STEP_H / 2; const Color col = srcColor(em::kSrcXlfo1);
        beginPath(); moveTo(STEP_X, mid); lineTo(STEP_X + STEP_W, mid); strokeColor(Color(255, 255, 255, 10 / 255.f)); stroke();
        if (steps) {
            const int n = (int)clampf(std::round(val[lp_(l, em::kLSteps)]), 1, em::kNumSteps); const float sw = STEP_W / em::kNumSteps;
            for (int s = 0; s < em::kNumSteps; ++s) {
                const float v = val[lp_(l, em::kLStep1 + s)], x = STEP_X + s * sw, y = mid - v * (STEP_H / 2 - 6);
                beginPath(); rect(x + 2, std::min(y, mid), sw - 4, std::max(1.5f, std::fabs(y - mid))); fillColor(withAlpha(col, s < n ? 0.75f : 0.15f)); fill();
            }
            const float ph = (float)livePhase[l]; beginPath(); const float px = STEP_X + ph * n * sw; moveTo(px, STEP_Y + 2); lineTo(px, STEP_Y + STEP_H - 2); strokeColor(withAlpha(C_TEXT, 0.6f)); stroke();
        } else {
            beginPath();
            const int w = (int)std::round(val[lp_(l, em::kLWave)]); const float bal = val[lp_(l, em::kLBalance)];
            em::Xlfo tmp; tmp.rndA = -0.6f; tmp.rndB = 0.5f; tmp.held = 0.4f;
            for (int i = 0; i <= 120; ++i) { tmp.phase = i / 120.0; const float v = (w == 5 || w == 6) ? std::sin(i * 0.31f) * 0.6f * ((i / 20) % 2 ? 1 : -0.7f) : tmp.wave(w, bal);
                const float x = STEP_X + 4 + (STEP_W - 8) * i / 120.f, y = mid - v * (STEP_H / 2 - 8); if (!i) moveTo(x, y); else lineTo(x, y); }
            strokeColor(col); strokeWidth(1.6f); stroke();
            if (val[kXlfoRetrigFirst + l] < 0.5f) { beginPath(); circle(STEP_X + 4 + (STEP_W - 8) * (float)livePhase[l], mid - liveGlobalXlfo[l] * (STEP_H / 2 - 8), 3.5f); fillColor(C_TEXT); fill(); }
        }
    }
    // the EG display: curve handles at the middle of attack, decay and release
    void egGeom(int e, float& x0, float& sx, float& yb, float& hh) const {
        const float a = val[ep(e, em::kEAttack)], d = val[ep(e, em::kEDecay)], r = val[ep(e, em::kERelease)];
        const float tot = std::max(a + d + 0.4f + r, 0.05f); sx = (EGD_W - 16) / tot; x0 = EGD_X + 8; yb = EGD_Y + EGD_H - 6; hh = EGD_H - 14;
    }
    void egHandle(int e, int k, float& hx, float& hy) const {
        float x0, sx, yb, hh; egGeom(e, x0, sx, yb, hh);
        const float a = val[ep(e, em::kEAttack)], d = val[ep(e, em::kEDecay)], s = val[ep(e, em::kESustain)], r = val[ep(e, em::kERelease)];
        const float c = val[kEgCurveFirst + e * 3 + k];
        const float v = CurveEnv::shape(0.5f, c);
        if (k == 0) { hx = x0 + 0.5f * a * sx; hy = yb - hh * v; }
        else if (k == 1) { hx = x0 + (a + 0.5f * d) * sx; hy = yb - hh * (1.f + (s - 1.f) * v); }
        else { hx = x0 + (a + d + 0.4f + 0.5f * r) * sx; hy = yb - hh * s * (1.f - v); }
    }
    void drawEg(int e) {
        caption(MX + 116, MY + 44, "TRIGGER");
        displayBox(EGD_X, EGD_Y, EGD_W, EGD_H);
        float x0, sx, yb, hh; egGeom(e, x0, sx, yb, hh);
        const float a = val[ep(e, em::kEAttack)], d = val[ep(e, em::kEDecay)], s = val[ep(e, em::kESustain)], r = val[ep(e, em::kERelease)];
        const float ca = val[kEgCurveFirst + e * 3], cd = val[kEgCurveFirst + e * 3 + 1], cr = val[kEgCurveFirst + e * 3 + 2];
        beginPath(); moveTo(x0, yb);
        for (int i = 1; i <= 24; ++i) { const float t = i / 24.f; lineTo(x0 + a * t * sx, yb - hh * CurveEnv::shape(t, ca)); }
        for (int i = 1; i <= 24; ++i) { const float t = i / 24.f; lineTo(x0 + (a + d * t) * sx, yb - hh * (1.f + (s - 1.f) * CurveEnv::shape(t, cd))); }
        lineTo(x0 + (a + d + 0.4f) * sx, yb - hh * s);
        for (int i = 1; i <= 24; ++i) { const float t = i / 24.f; lineTo(x0 + (a + d + 0.4f + r * t) * sx, yb - hh * s * (1.f - CurveEnv::shape(t, cr))); }
        const Color col = srcColor(em::kSrcEg1); strokeColor(col); strokeWidth(1.6f); stroke();
        for (int k = 0; k < 3; ++k) { float hx, hy; egHandle(e, k, hx, hy); beginPath(); circle(hx, hy, hoverEgHandle == k || dragEgHandle == k ? 5.5f : 4.5f); fillColor(C_BG); fill(); strokeColor(col); strokeWidth(1.5f); stroke(); }
        const float lv = clampf(liveSrc[em::kSrcEg1 + e], 0.f, 1.f);
        beginPath(); roundedRect(EGD_X + EGD_W - 7, EGD_Y + 4, 3, EGD_H - 8, 1.5f); fillColor(C_PANEL_HI); fill();
        if (lv > 0.002f) { beginPath(); roundedRect(EGD_X + EGD_W - 7, EGD_Y + EGD_H - 4 - (EGD_H - 8) * lv, 3, (EGD_H - 8) * lv, 1.5f); fillColor(col); fill(); }
    }
    void drawEf(int e) {
        caption(MX + 126, MY + 44, "LISTENS TO");
        const float v = liveSrc[em::kSrcEf1 + e];
        displayBox(MX + 16, MY + 222, 420, 16); beginPath(); rect(MX + 18, MY + 224, 416 * clampf(v, 0.f, 1.f), 12); fillColor(srcColor(em::kSrcEf1)); fill();
        caption(MX + 16, MY + 250, "FOLLOWER (NEWEST VOICE)", ALIGN_LEFT);
    }
    void drawXy(int x) {
        displayBox(XY_X, XY_Y, XY_W, XY_H);
        beginPath(); moveTo(XY_X + XY_W / 2, XY_Y + 6); lineTo(XY_X + XY_W / 2, XY_Y + XY_H - 6); moveTo(XY_X + 6, XY_Y + XY_H / 2); lineTo(XY_X + XY_W - 6, XY_Y + XY_H / 2);
        strokeColor(Color(255, 255, 255, 10 / 255.f)); stroke();
        const float px = XY_X + 8 + (XY_W - 16) * val[xyp(x, 0)], py = XY_Y + XY_H - 8 - (XY_H - 16) * val[xyp(x, 1)];
        const Color col = srcColor(em::kSrcXy1X);
        beginPath(); moveTo(px, XY_Y + 4); lineTo(px, XY_Y + XY_H - 4); moveTo(XY_X + 4, py); lineTo(XY_X + XY_W - 4, py); strokeColor(withAlpha(col, 0.3f)); stroke();
        beginPath(); circle(px, py, dragXy ? 10 : 8); fillColor(col); fill();
        char b[32]; snprintf(b, sizeof b, "X %.0f%%   Y %.0f%%", val[xyp(x, 0)] * 100, val[xyp(x, 1)] * 100);
        fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(XY_X + XY_W + 16, XY_Y + 100, b, nullptr);
    }
    void drawMidi() {
        static const char* const tip[5] = { "each note's velocity", "each note's key, around middle C", "mod wheel (CC1)", "pitch bend wheel", "channel or poly aftertouch" };
        for (int i = 0; i < 5; ++i) { fontSize(9); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(MX + 24 + CHIP_W + 16, MY + 52 + i * 40 + CHIP_H / 2, tip[i], nullptr); }
    }
    void classicCols(const char* const* t) {
        for (int c = 0; c < 3; ++c) {
            const float x0 = MX + 14 + c * CL_COLW;
            if (c) { beginPath(); moveTo(x0 - 6, MY + 38); lineTo(x0 - 6, MY + MH - 12); strokeColor(C_LINE); strokeWidth(1); stroke(); }
            fontSize(8.5f); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_DIM); text(x0 + 2, MY + 49, t[c], nullptr);
        }
    }
    void drawClassicL() {
        static const char* const t[3] = { "LFO 1", "LFO 2", "LFO 3" }; classicCols(t);
        for (int l = 0; l < 3; ++l) {
            const float x0 = MX + 14 + l * CL_COLW, x = x0 + 2, y = MY + 124, w = 172, h = 44;
            displayBox(x, y, w, h);
            const uint32_t b = kL1Wave + 6 * l; const int wave = (int)(val[b] + .5f);
            beginPath();
            for (int i = 0; i <= 120; ++i) { const float tt = (float)i / 120, ph = std::fmod(tt * 2.f, 1.f);
                const float py = y + h / 2 - Engine::lfoShape(ph, wave, val[b + 1]) * h * 0.36f;
                if (!i) moveTo(x + 4 + (w - 8) * tt, py); else lineTo(x + 4 + (w - 8) * tt, py); }
            strokeColor(srcColor(kClassicChip)); strokeWidth(1.4f); stroke();
        }
    }
    void drawClassicE() {
        static const char* const t[3] = { "ENV 1", "ENV 2", "AMP ENV" }; classicCols(t);
        for (int e = 0; e < 2; ++e) {
            const uint32_t b = e == 0 ? kE1Delay : kE2Delay; const float x0 = MX + 14 + e * CL_COLW;
            drawAdsrH(x0 + 2, MY + 80, 172, 84, val[b + 1], 0, val[b + 2], val[b + 3], val[b + 4], srcColor(kClassicChip), val[b]);
        }
        const float x0 = MX + 14 + 2 * CL_COLW;
        fontSize(9); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_FAINT); textBox(x0 + 2, MY + 82, 170, "The amp envelope (edit it in the AMP ENVELOPE panel) is also a modulation source.", nullptr);
    }

    // ---- per-source presets -------------------------------------------------------------
    bool compArp = false;      // the open menu is the arpeggiator's
    int compKind() const { return compArp ? kCompArp : (tab <= 3 ? kCompXlfo : (tab <= 5 ? kCompEg : kCompEf)); }
    int compIndex() const { return compArp ? 0 : (tab <= 3 ? tab : (tab <= 5 ? tab - 4 : tab - 6)); }
    bool compHit(float x, float y) const { return tab <= 7 && inRect(x, y, COMP_X, COMP_Y, COMP_W, COMP_H); }
    bool arpCompHit(float x, float y) const { return inRect(x, y, ARPP_X, ARPP_Y, ARPP_W, ARPP_H); }
    struct CompItem { int type, idx; std::string name; };       // type 0 factory, 1 user file, 2 save
    std::vector<CompItem> compItems; int compPendingDel = -1;
    bool savingComp = false; int savingCompKind = 0, savingCompIdx = 0;
    void openCompMenu(bool arp) {
        compArp = arp; compItems.clear(); const int k = compKind();
        for (int i : compFactoryFor(k)) compItems.push_back({ 0, i, compFactory()[i].name });
        const std::vector<std::string> u = listCompPresets(k); for (int i = 0; i < (int)u.size(); ++i) compItems.push_back({ 1, i, u[i] });
        compItems.push_back({ 2, 0, "Save current..." });
        compPendingDel = -1; popup.open = true; popup.kind = 3; popup.slot = -1; popup.x = arp ? ARPP_X + ARPP_W - 186 : COMP_X + COMP_W - 186; popup.y = arp ? ARPP_Y + ARPP_H + 4 : COMP_Y + COMP_H + 4;
        const float ph = compItems.size() * 20.f + 12; if (popup.y + ph > H - 8) popup.y = (arp ? ARPP_Y : COMP_Y) - ph - 4;
        popup.hover = -1; repaint();
    }
    void applyComp(const float* fields) { const int k = compKind(), ix = compIndex(); beginGesture(); for (int f = 0; f < compFieldCount(k); ++f) commit(compParam(k, ix, f), fields[f]); endGesture(); }
    void pickComp(int i) {
        const CompItem it = compItems[i]; const int k = compKind(); float fv[em::kXlfoFields + kArpCompFields];
        if (it.type == 0) { compFactoryValues(it.idx, fv); applyComp(fv); flash(std::string(compKindName(k)) + " preset: " + it.name); }
        else if (it.type == 1) {
            if (compPendingDel == i) { deleteCompPreset(k, it.name); openCompMenu(compArp); return; }
            if (loadCompPreset(k, it.name, fv)) { applyComp(fv); flash(std::string(compKindName(k)) + " preset: " + it.name); }
        } else { savingComp = true; savingCompKind = k; savingCompIdx = compIndex(); saving = true; saveError = false; saveName.clear(); }
        popup.open = false; repaint();
    }

    // ---- modulation rows: v2 slots (id 0..99) and classic slots (id 1000 + slot) ------------
    static constexpr int kClassicRow = 1000;
    static bool isClassicRow(int id) { return id >= kClassicRow; }
    bool classicUsed(int s) const { return val[slotParam(s, kSlotSrc)] > 0.5f || val[slotParam(s, kSlotDst)] > 0.5f; }
    bool slotUsed(int s) const { return (int)std::round(val[sp(s, kSlotSrc)]) > 0 || (int)std::round(val[sp(s, kSlotDst)]) > 0; }
    uint32_t rowParam(int id, int field) const { return isClassicRow(id) ? slotParam(id - kClassicRow, field) : sp(id, field); }
    int rowSrcRaw(int id) const { return (int)std::round(val[rowParam(id, kSlotSrc)]); }
    Color rowColor(int id) const {
        const int s = rowSrcRaw(id);
        if (isClassicRow(id)) return s <= 0 ? C_DIM : (s <= kSrcEg3 ? srcColor(kClassicChip) : srcColor(em::kSrcVelocity));
        return s <= 0 ? C_DIM : srcColor(s);
    }
    const char* rowSrcShort(int id) const { const int s = rowSrcRaw(id); return isClassicRow(id) ? classicSrcShort(s) : (s > 0 ? em::sourceShort(s) : "-"); }
    const char* rowSrcName(int id) const { const int s = rowSrcRaw(id); return isClassicRow(id) ? sourceName(s) : em::sourceName(s); }
    std::string rowDestName(int id) const { const int d = (int)std::round(val[rowParam(id, kSlotDst)]); return isClassicRow(id) ? destName(d) : destName2(d); }
    bool rowBypassed(int id) const { return val[rowParam(id, kSlotBypass)] > 0.5f; }
    // classic slots with an amount of 0 (the Init sound's vibrato pair) still count: they are part of the sound
    std::vector<int> usedRows() const {
        std::vector<int> v; for (int s = 0; s < kNumSlots2; ++s) if (slotUsed(s)) v.push_back(s);
        for (int s = 0; s < kNumSlots; ++s) if (classicUsed(s)) v.push_back(kClassicRow + s);
        return v;
    }
    struct SlotRow { int slot; float x, y; };
    std::vector<SlotRow> slotRows() const {
        std::vector<SlotRow> r; const std::vector<int> u = usedRows();
        for (int i = slotScroll; i < (int)u.size() && (int)r.size() < SLOT_ROWS; ++i) r.push_back({ u[i], SX + 10, SLOT_Y0 + (i - slotScroll) * SLOT_ROWH });
        return r;
    }
    int maxScroll() const { return std::max(0, (int)usedRows().size() - SLOT_ROWS); }
    void drawRowControls(int id, float x, float cy, float kx, bool showDst, int hoverPart) {
        const bool byp = rowBypassed(id);
        beginPath(); circle(x + 10, cy, 3); fillColor(byp ? C_FAINT : rowColor(id)); fill();
        fontSize(8.5f); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(hoverPart == id * 10 + 1 ? C_TEXT : (byp ? C_FAINT : rowColor(id))); text(x + 18, cy + 0.5f, showDst ? rowSrcShort(id) : rowSrcName(id), nullptr);
        if (showDst) {
            fillColor(C_FAINT); text(x + 62, cy + 0.5f, ">", nullptr);
            std::string nm = rowDestName(id); if (nm.size() > 21) nm = nm.substr(0, 20) + ".";
            fillColor(hoverPart == id * 10 + 2 ? C_TEXT : (byp ? C_FAINT : C_DIM)); text(x + 72, cy + 0.5f, nm.c_str(), nullptr);
        }
        const uint32_t ap = rowParam(id, kSlotAmt);
        drawKnobBody(kx, cy, 7.5f, toNorm(ap, val[ap]), true, hoverPart == id * 10 + 3 || activeRow == id, byp, ap);
        char a[16]; fmt(ap, a, sizeof a); fontSize(8); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(byp ? C_FAINT : C_DIM); text(kx + 11, cy + 0.5f, a, nullptr);
    }
    void drawRowEnd(int id, float bx, float dx, float cy, int hoverPart) {
        const bool byp = rowBypassed(id);
        beginPath(); circle(bx, cy, 4.5f); fillColor(byp ? C_PANEL : withAlpha(C_ACC, 0.8f)); fill(); strokeColor(byp ? C_FAINT : C_ACC); strokeWidth(1); stroke();
        beginPath(); moveTo(dx - 3.5f, cy - 3.5f); lineTo(dx + 3.5f, cy + 3.5f); moveTo(dx + 3.5f, cy - 3.5f); lineTo(dx - 3.5f, cy + 3.5f);
        strokeColor(hoverPart == id * 10 + 5 ? C_TEXT : C_FAINT); strokeWidth(1.3f); stroke();
    }
    void drawSlots() {
        panel(SX, SY, SW, SH);
        const std::vector<int> rows = usedRows(); int n2 = 0; for (int id : rows) n2 += !isClassicRow(id);
        char b[64]; snprintf(b, sizeof b, "MODULATION  %d / %d", n2, kNumSlots2);
        if ((int)rows.size() > n2) snprintf(b, sizeof b, "MODULATION  %d / %d  +  %d CLASSIC", n2, kNumSlots2, (int)rows.size() - n2);
        fontSize(8); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(SX + 14, SY + 18, b, nullptr);
        if (rows.empty()) { fontSize(10.5f); textLetterSpacing(0.3f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT);
            text(SX + SW / 2, SY + SH / 2 - 8, "Drag a source chip onto any knob", nullptr);
            fontSize(9); text(SX + SW / 2, SY + SH / 2 + 10, "Each connection appears here", nullptr); return; }
        for (const SlotRow& r : slotRows()) {
            const int s = r.slot; const bool sel = s == selSlot;
            beginPath(); roundedRect(r.x, r.y + 1, SLOT_COLW, SLOT_ROWH - 2, 4); fillColor(sel ? withAlpha(C_ACC, 0.12f) : (isClassicRow(s) ? C_PANEL_LO : C_PANEL_HI)); fill();
            const float cy = r.y + SLOT_ROWH / 2;
            drawRowControls(s, r.x, cy, r.x + 222, true, hoverSlotPart);
            drawRowEnd(s, r.x + SLOT_COLW - 24, r.x + SLOT_COLW - 10, cy, hoverSlotPart);
        }
        if (maxScroll() > 0) { fontSize(8); textLetterSpacing(1); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(C_FAINT); text(SX + SW - 14, SY + 18, "SCROLL", nullptr); }
    }
    int slotPartHit(float x, float y) const {
        for (const SlotRow& r : slotRows()) {
            if (y < r.y || y >= r.y + SLOT_ROWH || x < r.x || x >= r.x + SLOT_COLW) continue;
            const float rx = x - r.x;
            if (rx < 60) return r.slot * 10 + 1;
            if (rx < 210) return r.slot * 10 + 2;
            if (rx < SLOT_COLW - 32) return r.slot * 10 + 3;
            if (rx < SLOT_COLW - 17) return r.slot * 10 + 4;
            return r.slot * 10 + 5;
        }
        return -1;
    }
    void clearRow(int id) { for (int f = 0; f < 4; ++f) commit(rowParam(id, f), 0); }
    void deleteRow(int id) { beginGesture(); clearRow(id); endGesture(); if (selSlot == id) selSlot = -1; slotScroll = std::min(slotScroll, maxScroll()); }
    bool rowClick(int part, float x, float y, int mods) {
        const int id = part / 10, k = part % 10; selSlot = id;
        if (k == 1 || k == 2) { popup.open = true; popup.kind = isClassicRow(id) ? k + 3 : k; popup.slot = id; popup.x = x - 40; popup.y = y + 12; popup.hover = -1; repaint(); return true; }
        if (k == 3) { const uint32_t ap = rowParam(id, kSlotAmt);
            if (mods & (kModifierControl | kModifierSuper)) { beginGesture(); commit(ap, 0.f); endGesture(); return true; }
            beginGesture(); activeRow = id; dragY = y; dragNorm = toNorm(ap, val[ap]); editParameter(ap, true); repaint(); return true; }
        if (k == 4) { const uint32_t bp = rowParam(id, kSlotBypass); beginGesture(); commit(bp, val[bp] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
        deleteRow(id); return true;
    }

    // ---- floating panel: every row that modulates one knob ----------------------------
    struct KnobPanel { bool open = false; uint32_t param = 0; float x = 0, y = 0; } kpan; int hoverKp = -1;
    static constexpr float KP_W = 270, KP_HEAD = 26, KP_ROWH = 22;
    std::vector<int> rowsFor(uint32_t p) const {
        std::vector<int> v;
        const int d2 = destForParam2(p);
        if (d2 > 0) for (int k = 0; k < kNumSlots2; ++k) if ((int)std::round(val[sp(k, kSlotDst)]) == d2 && (int)std::round(val[sp(k, kSlotSrc)]) > 0) v.push_back(k);
        const int d = destForParam(p);
        if (d > 0) for (int k = 0; k < kNumSlots; ++k) if ((int)std::round(val[slotParam(k, kSlotDst)]) == d && (int)std::round(val[slotParam(k, kSlotSrc)]) > 0) v.push_back(kClassicRow + k);
        return v;
    }
    void kpLayout(float& x, float& y, float& h) const {
        h = KP_HEAD + std::max<size_t>(1, rowsFor(kpan.param).size()) * KP_ROWH + 8;
        x = std::min(W - KP_W - 8, std::max(8.f, kpan.x)); y = std::min(H - h - 8, std::max(8.f, kpan.y));
    }
    bool kpInside(float px, float py) const { float x, y, h; kpLayout(x, y, h); return px >= x && px < x + KP_W && py >= y && py < y + h; }
    int kpHit(float px, float py) const {
        float x, y, h; kpLayout(x, y, h); const std::vector<int> v = rowsFor(kpan.param);
        for (int i = 0; i < (int)v.size(); ++i) { const float ry = y + KP_HEAD + i * KP_ROWH; if (py < ry || py >= ry + KP_ROWH || px < x || px >= x + KP_W) continue;
            const float rx = px - x; return v[i] * 10 + (rx < 124 ? 1 : (rx < KP_W - 44 ? 3 : (rx < KP_W - 24 ? 4 : 5))); }
        return -1;
    }
    void openKnobPanel(uint32_t p, float px, float py) {
        if (destForParam2(p) <= 0 && destForParam(p) <= 0) return;
        if (rowsFor(p).empty()) { flash("Nothing modulates this control yet. Drag a source chip onto it."); return; }
        kpan.open = true; kpan.param = p; kpan.x = px + 14; kpan.y = py - 10; hoverKp = -1; repaint();
    }
    void drawKnobPanel() {
        float x, y, h; kpLayout(x, y, h); const std::vector<int> v = rowsFor(kpan.param);
        beginPath(); rect(x - 10, y - 4, KP_W + 20, h + 18); fillPaint(boxGradient(x, y + 2, KP_W, h, 6, 12, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, KP_W, h, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        std::string t = paramDef(kpan.param).name; for (char& ch : t) ch = (char)std::toupper((unsigned char)ch);
        fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_ACC); text(x + 12, y + 14, t.c_str(), nullptr);
        for (int i = 0; i < (int)v.size(); ++i) {
            const int id = v[i]; const float ry = y + KP_HEAD + i * KP_ROWH, cy = ry + KP_ROWH / 2;
            if (hoverKp / 10 == id) { beginPath(); roundedRect(x + 4, ry + 1, KP_W - 8, KP_ROWH - 2, 3); fillColor(withAlpha(C_ACC, 0.10f)); fill(); }
            drawRowControls(id, x + 4, cy, x + 136, false, hoverKp);
            drawRowEnd(id, x + KP_W - 34, x + KP_W - 14, cy, hoverKp);
        }
    }
    bool ringHitKnob(const Ctl& c, float x, float y) const {
        if (c.type != Knob || c.r <= 9) return false;
        if (rowsFor(effParam(c)).empty()) return false;
        const float tw = c.r > 20 ? 4.f : (c.r > 12 ? 3.f : 2.5f), rr = c.r + tw / 2 + 3;
        const float dist = std::sqrt((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y));
        return std::fabs(dist - rr) <= 3.f;
    }

    // ---- knobs ------------------------------------------------------------------------
    bool modInfo(uint32_t p, float& depth, Color& col, float& live) const {
        depth = 0; float best = 0; Color bc = C_DIM; live = 0;
        const int d2 = destForParam2(p);
        if (d2 > 0) for (int s = 0; s < kNumSlots2; ++s) {
            if (val[sp(s, kSlotBypass)] > 0.5f || (int)std::round(val[sp(s, kSlotDst)]) != d2) continue;
            const int src = (int)std::round(val[sp(s, kSlotSrc)]); if (src <= 0) continue;
            const float a = std::fabs(val[sp(s, kSlotAmt)]); depth += a; if (a >= best) { best = a; bc = srcColor(src); }
        }
        const int d = destForParam(p);
        if (d > 0) for (int s = 0; s < kNumSlots; ++s) {
            if (val[slotParam(s, kSlotBypass)] > 0.5f || (int)std::round(val[slotParam(s, kSlotDst)]) != d) continue;
            if ((int)std::round(val[slotParam(s, kSlotSrc)]) <= 0) continue;
            const float a = std::fabs(val[slotParam(s, kSlotAmt)]); depth += a; if (a > best) { best = a; bc = rowColor(kClassicRow + s); }
        }
        if (depth <= 0) return false;
        col = bc; if (d2 >= kDstSlotFirst) live = liveOff[d2]; return true;
    }
    void drawKnobBody(float x, float y, float r, float n, bool bipolar, bool hot, bool dim, uint32_t p) {
        const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f, ang = a0 + (a1 - a0) * n;
        const float tw = r > 20 ? 4.f : (r > 12 ? 3.f : 2.5f);
        beginPath(); arc(x, y, r, a0, a1, NanoVG::CW); strokeColor(C_LINE); strokeWidth(tw); lineCap(ROUND); stroke();
        const float from = bipolar ? (a0 + a1) / 2 : a0;
        Color arcCol = C_ACC;
        for (int f = 0; f < kNumFilters; ++f) for (int k = 0; k <= kFGain; ++k) if (fpar(f, k) == p) arcCol = filterColor(f);
        for (int o = 0; o < kNumOsc; ++o) { for (int k = 0; k < kOFields; ++k) if (opar(o, k) == p) arcCol = oscColor(o); if (p == (uint32_t)(kOscRatioFirst + o)) arcCol = oscColor(o); }
        if (std::fabs(ang - from) > 0.001f) { beginPath(); arc(x, y, r, std::min(from, ang), std::max(from, ang), NanoVG::CW); strokeColor(dim ? C_FAINT : arcCol); stroke(); }
        float depth, live; Color mc;
        if (r > 9 && modInfo(p, depth, mc, live)) {
            const float span = std::max(0.08f, std::min(1.f, depth * 2.f)) * (a1 - a0) * 0.5f;
            beginPath(); arc(x, y, r + tw / 2 + 3, std::max(a0, ang - span), std::min(a1, ang + span), NanoVG::CW); strokeColor(withAlpha(mc, 0.85f)); strokeWidth(2.f); lineCap(ROUND); stroke();
            if (live != 0.f) { const float la = a0 + (a1 - a0) * clampf(n + live * 0.5f, 0.f, 1.f);
                beginPath(); circle(x + std::cos(la) * (r + tw / 2 + 3), y + std::sin(la) * (r + tw / 2 + 3), 2.4f); fillColor(C_TEXT); fill(); }
        }
        const float cr = r - (r > 20 ? 8 : (r > 12 ? 6 : 4));
        beginPath(); circle(x, y, cr);
        fillPaint(radialGradient(x - cr * 0.3f, y - cr * 0.4f, 0, cr * 1.4f, hot ? Color(0x3a, 0x3e, 0x44) : C_PANEL_HI, Color(0x14, 0x15, 0x17)));
        fill(); strokeColor(Color(255, 255, 255, 18 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(x + std::cos(ang) * cr * 0.3f, y + std::sin(ang) * cr * 0.3f); lineTo(x + std::cos(ang) * cr * 0.9f, y + std::sin(ang) * cr * 0.9f);
        strokeColor(dim ? C_DIM : C_TEXT); strokeWidth(r > 12 ? 2.f : 1.5f); lineCap(ROUND); stroke();
    }
    int segCurrent(const Ctl& c, uint32_t p) const {
        if (c.group == GFilt && c.field == kFSlope) { const int v = (int)std::round(val[p]); return v == kSlope6 ? 0 : v + 1; }
        return (int)std::round(val[p]);
    }
    static bool knobBipolar(uint32_t p) {
        const ParamDef& d = paramDef(p);
        if (p == kVolume) return false;
        return (d.hints & kHintBipolar) || (d.min < 0 && std::strcmp(d.unit, "dB") != 0);
    }
    void learnBadge(float cx, float by, uint32_t p) {
        const int cc = paramCC[p]; const bool waiting = learnWaiting == (int)p; char buf[16];
        snprintf(buf, sizeof buf, waiting ? "?" : "CC%d", cc);
        fontSize(8); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        const float bw = waiting ? 14 : 32;
        beginPath(); roundedRect(cx - bw / 2, by - 6, bw, 12, 3); fillColor(waiting ? C_ACC : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fillColor(waiting ? C_BG : C_ACC); text(cx, by + 0.5f, buf, nullptr);
    }
    void drawCtl(const Ctl& c, bool hot) {
        const uint32_t p = effParam(c); const bool dim = dimmed(p); char buf[48];
        const bool dropTarget = dragSrc > 0 && hot && (dragSrc >= kClassicChip ? destForParam(p) > 0 : destForParam2(p) > 0);
        switch (c.type) {
        case Knob: {
            float n = toNorm(p, val[p]);
            if (paramDef(p).hints & kHintInteger) n = (val[p] - paramDef(p).min) / std::max(1.f, paramDef(p).max - paramDef(p).min);
            drawKnobBody(c.x, c.y, c.r, n, knobBipolar(p), hot, dim, p);
            if (dropTarget) { beginPath(); circle(c.x, c.y, c.r + 9); strokeColor(srcColor(dragSrc)); strokeWidth(2); stroke(); }
            fontSize(7.5f); textLetterSpacing(1.3f); textAlign(ALIGN_CENTER | ALIGN_TOP); fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + c.r + 5, c.label, nullptr);
            fmt(p, buf, sizeof buf); fontSize(9); textLetterSpacing(0.3f); fillColor(dim ? C_FAINT : (hot ? C_ACC : Color(0xd6, 0xd2, 0xca))); text(c.x, c.y + c.r + 15, buf, nullptr);
            break; }
        case Toggle: {
            const bool on = val[p] > 0.5f;
            const float w = 28, h = 15, x = c.x - w / 2, y = c.y - h / 2;
            const Color acc = c.group == GOsc && c.field == -1 ? oscColor(c.idx) : C_ACC;
            beginPath(); roundedRect(x, y, w, h, h / 2); fillColor(on && !dim ? withAlpha(acc, 0.25f) : C_PANEL_HI); fill(); strokeColor(on && !dim ? acc : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(on ? x + w - h / 2 : x + h / 2, c.y, 4.7f); fillColor(dim ? C_FAINT : (on ? acc : C_DIM)); fill();
            if (c.label[0]) { fontSize(7.5f); textLetterSpacing(1.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(on && !dim ? C_TEXT : (dim ? C_FAINT : C_DIM)); text(c.x + w / 2 + 6, c.y + 0.5f, c.label, nullptr); }
            break; }
        case Segment: {
            int n = 0; const char* const* names = segLabels(c.group, c.field, n);
            const int cur = segCurrent(c, p); const float sw = c.r / n, x0 = c.x - c.r / 2;
            const Color on = c.group == GFilt ? filterColor(selF) : C_ACC;
            for (int i = 0; i < n; ++i) {
                const float x = x0 + sw * i;
                beginPath(); roundedRect(x + 1, c.y - SEG_H / 2, sw - 2, SEG_H, 4); fillColor(i == cur ? (dim ? C_FAINT : on) : C_PANEL_HI); fill();
                fontSize(8); textLetterSpacing(1.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(i == cur ? C_BG : (dim ? C_FAINT : C_DIM)); text(x + sw / 2, c.y + 0.5f, names[i], nullptr);
            }
            break; }
        case Stepper: {
            const float x = c.x - c.r / 2, y = c.y - 10, w = c.r, h = 20;
            beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_PANEL_HI); fill(); strokeColor(hot ? C_DIM : C_LINE); strokeWidth(1); stroke();
            for (int side = 0; side < 2; ++side) { const float ax = side == 0 ? x + 8 : x + w - 8, d = side == 0 ? -1.f : 1.f;
                beginPath(); moveTo(ax - 2.5f * d, c.y - 4); lineTo(ax + 2.5f * d, c.y); lineTo(ax - 2.5f * d, c.y + 4); closePath(); fillColor(C_DIM); fill(); }
            std::string up;
            if (c.group == GOsc && c.field == kOWave) up = waveShort((int)std::round(val[p]));
            else { fmt(p, buf, sizeof buf); up = buf; for (char& ch : up) ch = (char)std::toupper((unsigned char)ch); }
            if (c.group == GXlfo && c.field == em::kLSteps) up += " STEPS";
            const Color tc = c.group == GOsc ? oscColor(c.idx) : C_ACC;
            fontSize(8.5f); textLetterSpacing(0.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(dim ? C_FAINT : tc); text(c.x, c.y + 0.5f, up.c_str(), nullptr);
            if (c.label[0]) { fontSize(7.5f); textLetterSpacing(1.3f); textAlign(ALIGN_CENTER | ALIGN_TOP); fillColor(C_DIM); text(c.x, c.y + 14, c.label, nullptr); }
            break; }
        }
        if (learnMode && (paramCC[p] >= 0 || learnWaiting == (int)p))
            learnBadge(c.x, c.type == Knob ? c.y - c.r - 10 : c.y - SEG_H / 2 - 7, p);
    }

    void drawStatus() {
        std::string msg; char vb[48];
        if (flashing()) msg = statusText;
        else if (learnMode) msg = learnWaiting >= 0 ? std::string("Move a MIDI controller: ") + paramDef(learnWaiting).name : "MIDI Learn: click a control, move a controller. Right-click unassigns.";
        else if (dragSrc > 0) { msg = std::string("Drop ") + chipName(dragSrc) + " onto a knob";
            if (hover >= 0) { const uint32_t p = effParam(controls()[hover]); const bool ok = dragSrc >= kClassicChip ? destForParam(p) > 0 : destForParam2(p) > 0; if (ok) msg = std::string("Drop ") + chipName(dragSrc) + " onto " + paramDef(p).name; } }
        else if (hover >= 0) { const uint32_t p = effParam(controls()[hover]); fmt(p, vb, sizeof vb); msg = std::string(paramDef(p).name) + "   " + vb; }
        else msg = "Drag a source chip onto any knob to modulate it.  Hover for tips.";
        fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(learnMode || flashing() ? C_ACC : C_FAINT);
        text(18, STATUS_Y, msg.c_str(), nullptr);
        if (learnMode && learnWaiting < 0) {
            const float cx = W - 16 - 110; beginPath(); roundedRect(cx, STATUS_Y - 8, 110, 16, 4); fillColor(hoverClear ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
            fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(cx + 55, STATUS_Y + 0.5f, "CLEAR ALL CCs", nullptr);
        }
    }
    void flash(const std::string& s) { statusText = s; flashUntil = nowSec() + 2.0; repaint(); }
    bool flashing() const { return nowSec() < flashUntil; }
    void drawEditBox() {
        const Ctl& c = controls()[editing];
        const float w = 90, h = 22, x = c.x - w / 2, y = c.y + c.r + 4;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(12); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(c.x, y + h / 2 + 0.5f, (editBuf + "|").c_str(), nullptr);
    }

    // ---- popups: style 0, v2 source 1, v2 destination 2, source presets 3, classic source 4, classic destination 5 ----
    struct Popup { bool open = false; int kind = 0, slot = -1; float x = 0, y = 0; int hover = -1; } popup;
    int popupCount() const {
        switch (popup.kind) { case 0: return kNumStyles; case 1: return em::kNumSources - 1; case 2: return kNumDests2 - 1; case 4: return kNumSources - 1; case 5: return kNumDests - 1; default: return (int)compItems.size(); }
    }
    std::string popupLabel(int i) const {
        if (popup.kind == 3) { const CompItem& it = compItems[i]; return it.type == 1 ? (compPendingDel == i ? "Delete? click again" : "  " + it.name) : it.name; }
        switch (popup.kind) { case 0: return styleName(i); case 1: return em::sourceName(i + 1); case 2: return destName2(i + 1); case 4: return sourceName(i + 1); default: return destName(i + 1); }
    }
    float popupRowH() const { return popup.kind == 0 ? 22.f : 19.f; }
    void popupLayout(float& x, float& y, float& w, float& h, int& rows, float& colw) const {
        const int n = popupCount();
        if (popup.kind == 0) { rows = n; colw = 160; }
        else if (popup.kind == 3) { rows = n; colw = 180; }
        else if (popup.kind == 1) { rows = 9; colw = 110; }
        else if (popup.kind == 4) { rows = 11; colw = 110; }
        else { rows = popup.kind == 2 ? 30 : 31; colw = 140; }
        const int cols = (n + rows - 1) / rows; w = cols * colw + 12; h = rows * popupRowH() + 12;
        x = std::min(W - w - 8, std::max(8.f, popup.x)); y = std::min(H - h - 8, std::max(8.f, popup.y));
    }
    void popupCell(int i, float x, float y, float colw, int rows, float& ix, float& iy) const { ix = x + 6 + (i / rows) * colw; iy = y + 6 + (i % rows) * popupRowH(); }
    int popupCurrent() const {
        if (popup.kind == 0) return (int)std::round(val[fpar(selF, kFStyle)]);
        if (popup.slot < 0 || popup.kind == 3) return -1;
        return (int)std::round(val[rowParam(popup.slot, (popup.kind == 1 || popup.kind == 4) ? kSlotSrc : kSlotDst)]) - 1;
    }
    void drawPopup() {
        float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw);
        beginPath(); rect(x - 10, y - 4, w + 20, h + 18); fillPaint(boxGradient(x, y + 2, w, h, 6, 12, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        const int cur = popupCurrent(); const float rh = popupRowH();
        for (int i = 0; i < popupCount(); ++i) {
            float ix, iy; popupCell(i, x, y, colw, rows, ix, iy);
            if (i == popup.hover) { beginPath(); roundedRect(ix, iy, colw - 4, rh, 3); fillColor(withAlpha(C_ACC, 0.22f)); fill(); }
            if (popup.kind == 1) { beginPath(); circle(ix + 7, iy + rh / 2, 3); fillColor(srcColor(i + 1)); fill(); }
            if (popup.kind == 4) { beginPath(); circle(ix + 7, iy + rh / 2, 3); fillColor(i < 6 ? srcColor(kClassicChip) : srcColor(em::kSrcVelocity)); fill(); }
            if (popup.kind == 0 && i == kNumStyles2) { beginPath(); rect(ix + 4, iy - 0.5f, colw - 12, 1); fillColor(C_LINE); fill(); }
            if (popup.kind == 3 && i > 0 && compItems[i].type != compItems[i - 1].type) { beginPath(); rect(ix + 4, iy - 0.5f, colw - 12, 1); fillColor(C_LINE); fill(); }
            const bool accent = popup.kind == 3 && (compItems[i].type == 2 || compPendingDel == i);
            fontSize(popup.kind == 0 ? 10.5f : 9.f); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(i == cur || accent ? C_ACC : (i == popup.hover ? C_TEXT : C_DIM));
            text(ix + ((popup.kind == 1 || popup.kind == 4) ? 15 : 6), iy + rh / 2 + 0.5f, popupLabel(i).c_str(), nullptr);
        }
    }
    int popupHit(float px, float py) const {
        float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw);
        for (int i = 0; i < popupCount(); ++i) { float ix, iy; popupCell(i, x, y, colw, rows, ix, iy); if (px >= ix && px < ix + colw - 4 && py >= iy && py < iy + popupRowH()) return i; }
        return -1;
    }
    bool popupInside(float px, float py) const { float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw); return px >= x && px < x + w && py >= y && py < y + h; }
    void popupPick(int i) {
        if (popup.kind == 3) { pickComp(i); return; }
        beginGesture();
        if (popup.kind == 0) commit(fpar(selF, kFStyle), (float)i);
        else if (popup.slot >= 0) commit(rowParam(popup.slot, (popup.kind == 1 || popup.kind == 4) ? kSlotSrc : kSlotDst), (float)(i + 1));
        endGesture(); popup.open = false; repaint();
    }

    // ---- presets ------------------------------------------------------------------
    bool matchesValues(const float* v) const {
        for (uint32_t i = 0; i < kParamCount; ++i) {
            if (!isSetting(i)) continue;
            if (val[kArpLock] > 0.5f && i >= kArpOn && i <= kArpLock) continue;
            if (std::fabs(val[i] - v[i]) > 1e-4f * std::max(1.f, std::fabs(v[i]))) return false;
        }
        return true;
    }
    void applyValues(const float* v) {
        std::vector<float> w(v, v + kParamCount);
        if (val[kArpLock] > 0.5f) for (uint32_t k = kArpOn; k <= kArpLock; ++k) w[k] = val[k];     // ARP LOCK keeps the arpeggiator
        beginGesture(); applyAll(w.data()); endGesture();
    }
    void selectFirstOn() { selF = 0; for (int f = 0; f < kNumFilters; ++f) if (fOn(f)) { selF = f; break; } }
    void applyFactory(int idx) { idx = (idx % (int)kNumPresets + (int)kNumPresets) % (int)kNumPresets; applyValues(presets()[idx].v); curFactory = idx; curUser.clear(); selectFirstOn(); repaint(); }
    void applyUser(const std::string& name) { float v[kParamCount]; std::copy(defv, defv + kParamCount, v); if (!loadUserPreset(name, v)) return; applyValues(v); std::copy(v, v + kParamCount, userVals); curUser = name; curFactory = -1; selectFirstOn(); repaint(); }
    void programLoaded(uint32_t index) override { curFactory = (int)index; curUser.clear(); repaint(); }
    void step(int d) {
        const int nf = (int)kNumPresets, nu = (int)userPresets.size(), total = nf + nu;
        int cur = curFactory; if (!curUser.empty()) for (int i = 0; i < nu; ++i) if (userPresets[i] == curUser) cur = nf + i;
        if (cur < 0) cur = 0;
        const int next = ((cur + d) % total + total) % total;
        if (next < nf) applyFactory(next); else applyUser(userPresets[next - nf]);
    }
    void drawPresetBar() {
        const float x = PS_X, y = PS_Y, w = PS_W, h = PS_H, a = PS_ARROW;
        beginPath(); roundedRect(x, y, w, h, 5); fillColor(C_PANEL); fill(); strokeColor(menuOpen ? C_ACC : C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(x + a, y + 5); lineTo(x + a, y + h - 5); moveTo(x + w - a, y + 5); lineTo(x + w - a, y + h - 5); strokeColor(C_LINE); stroke();
        for (int side = 0; side < 2; ++side) {
            const bool hot = presetHover == (side == 0 ? 0 : 2);
            const float cx = side == 0 ? x + a / 2 : x + w - a / 2, cy = y + h / 2, d = side == 0 ? -1.f : 1.f;
            beginPath(); moveTo(cx - 3 * d, cy - 5); lineTo(cx + 3 * d, cy); lineTo(cx - 3 * d, cy + 5); closePath(); fillColor(hot ? C_ACC : C_DIM); fill();
        }
        std::string name; bool modified = false;
        if (!curUser.empty()) { name = curUser; modified = !matchesValues(userVals); }
        else if (curFactory >= 0) { name = presets()[curFactory].name; modified = !matchesValues(presets()[curFactory].v); }
        else name = "Custom";
        if (modified) name += " *";
        fontSize(12); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(presetHover == 1 || menuOpen ? C_TEXT : Color(0xd8, 0xd4, 0xcc));
        text(x + w / 2 - 6, y + h / 2 + 0.5f, name.c_str(), nullptr);
        const float cx = x + w - a - 14, cy = y + h / 2;
        beginPath(); moveTo(cx - 4, cy - 2); lineTo(cx + 4, cy - 2); lineTo(cx, cy + 3); closePath(); fillColor(C_DIM); fill();
    }
    // preset browser: categories, user presets and favourites in columns; typing searches names and categories.
    // kind: 0 factory, 1 user, 2 save button, 3 search field, 4 favourite (idx into favs)
    struct MRect { float x, y, w, h; int kind; int idx; };
    static std::string lower(const std::string& s) { std::string r(s); for (char& c : r) c = (char)std::tolower((unsigned char)c); return r; }
    std::string favKey(int kind, int idx) const { return kind == 0 ? std::string("f:") + presets()[idx].name : (kind == 1 ? "u:" + userPresets[idx] : (idx < (int)favs.size() ? favs[idx] : std::string())); }
    bool isFav(const std::string& k) const { return std::find(favs.begin(), favs.end(), k) != favs.end(); }
    void toggleFav(const std::string& k) {
        if (k.empty()) return;
        auto it = std::find(favs.begin(), favs.end(), k); if (it != favs.end()) favs.erase(it); else favs.push_back(k);
        saveFavourites(favs); repaint();
    }
    static constexpr int kMenuCols = kNumCategories + 2, kMenuRows = 16;
    void menuLayout(float& mx, float& my, float& mw, float& mh, std::vector<MRect>& items) const {
        mw = PM_COLW * kMenuCols + 2 * PM_PAD; mx = std::min(W - mw - 8, std::max(8.f, PS_X + PS_W / 2 - mw / 2)); my = PS_Y + PS_H + 4;
        items.clear();
        const float top = my + PM_PAD + PM_ROW + 6 + PM_HEAD;
        int maxRows = 1;
        items.push_back({ mx + PM_PAD + PM_COLW, my + PM_PAD, 300, PM_ROW, 3, 0 });
        items.push_back({ mx + mw - PM_PAD - 150, my + PM_PAD, 150, PM_ROW, 2, 0 });
        if (!query.empty()) {
            const std::string q = lower(query); int k = 0;
            auto place = [&](int kind, int idx) { const int col = k / kMenuRows, row = k % kMenuRows; if (col >= kMenuCols) return; items.push_back({ mx + PM_PAD + col * PM_COLW, top + row * PM_ROW, PM_COLW - 4, PM_ROW, kind, idx }); maxRows = std::max(maxRows, row + 1); ++k; };
            for (uint32_t i = 0; i < kNumPresets; ++i) if (lower(presets()[i].name).find(q) != std::string::npos || lower(presets()[i].category).find(q) != std::string::npos) place(0, (int)i);
            for (int u = 0; u < (int)userPresets.size(); ++u) if (lower(userPresets[u]).find(q) != std::string::npos || std::string("user").find(q) != std::string::npos) place(1, u);
        } else {
            int rows[kNumCategories] = {};
            for (uint32_t i = 0; i < kNumPresets; ++i) {
                const char* cat = presets()[i].category;
                if (std::strcmp(cat, "Init") == 0) { items.push_back({ mx + PM_PAD, my + PM_PAD, PM_COLW - 4, PM_ROW, 0, (int)i }); continue; }
                for (int c = 0; c < kNumCategories; ++c)
                    if (std::strcmp(cat, kCategories[c]) == 0) { items.push_back({ mx + PM_PAD + c * PM_COLW, top + rows[c] * PM_ROW, PM_COLW - 4, PM_ROW, 0, (int)i }); maxRows = std::max(maxRows, ++rows[c]); }
            }
            for (int u = 0; u < (int)userPresets.size() && u < kMenuRows; ++u) items.push_back({ mx + PM_PAD + kNumCategories * PM_COLW, top + u * PM_ROW, PM_COLW - 4, PM_ROW, 1, u });
            maxRows = std::max(maxRows, std::min(kMenuRows, (int)userPresets.size()));
            for (int f = 0; f < (int)favs.size() && f < kMenuRows; ++f) items.push_back({ mx + PM_PAD + (kNumCategories + 1) * PM_COLW, top + f * PM_ROW, PM_COLW - 4, PM_ROW, 4, f });
            maxRows = std::max(maxRows, std::min(kMenuRows, (int)favs.size()));
        }
        mh = PM_PAD + PM_ROW + 6 + PM_HEAD + maxRows * PM_ROW + PM_PAD;
    }
    std::string itemLabel(const MRect& r) const {
        if (r.kind == 0) return presets()[r.idx].name;
        if (r.kind == 1) return userPresets[r.idx];
        if (r.kind == 4 && r.idx < (int)favs.size()) return favs[r.idx].substr(2);
        return "";
    }
    void drawPresetMenu() {
        float mx, my, mw, mh; std::vector<MRect> items; menuLayout(mx, my, mw, mh, items);
        beginPath(); rect(mx - 12, my - 4, mw + 24, mh + 20); fillPaint(boxGradient(mx, my + 2, mw, mh, 6, 14, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(mx, my, mw, mh, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(mx + PM_PAD, my + PM_PAD + PM_ROW + 3); lineTo(mx + mw - PM_PAD, my + PM_PAD + PM_ROW + 3); strokeColor(C_LINE); stroke();
        fontSize(8.5f); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        const float hy = my + PM_PAD + PM_ROW + 6 + PM_HEAD / 2;
        if (query.empty()) {
            for (int c = 0; c < kMenuCols; ++c) {
                std::string up = c < kNumCategories ? kCategories[c] : (c == kNumCategories ? "User" : "Favourites"); for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
                fillColor(c < kNumCategories ? C_ACC : C_FAV); text(mx + PM_PAD + c * PM_COLW + 12, hy, up.c_str(), nullptr);
            }
            fontSize(10); textLetterSpacing(0.4f); fillColor(C_FAINT);
            if (userPresets.empty()) text(mx + PM_PAD + kNumCategories * PM_COLW + 12, hy + PM_HEAD / 2 + PM_ROW / 2, "(none saved yet)", nullptr);
            if (favs.empty()) text(mx + PM_PAD + (kNumCategories + 1) * PM_COLW + 12, hy + PM_HEAD / 2 + PM_ROW / 2, "(click a star)", nullptr);
        } else {
            int found = 0; for (const MRect& r : items) if (r.kind == 0 || r.kind == 1) ++found;
            char b[64]; snprintf(b, sizeof b, found ? "%d MATCHING PRESET%s" : "NO PRESET MATCHES", found, found == 1 ? "" : "S"); fillColor(C_ACC); text(mx + PM_PAD + 12, hy, b, nullptr);
        }
        for (int i = 0; i < (int)items.size(); ++i) {
            const MRect& r = items[i]; const bool hot = i == menuHover;
            if (r.kind == 2) {
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(hot ? withAlpha(C_ACC, 0.25f) : C_PANEL); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fontSize(9.5f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(r.x + r.w / 2, r.y + r.h / 2 + 0.5f, "SAVE CURRENT AS...", nullptr);
                continue;
            }
            if (r.kind == 3) {             // search field
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(C_DISPLAY); fill(); strokeColor(query.empty() ? C_LINE : C_ACC); strokeWidth(1); stroke();
                beginPath(); circle(r.x + 12, r.y + r.h / 2, 4); moveTo(r.x + 15, r.y + r.h / 2 + 3); lineTo(r.x + 18, r.y + r.h / 2 + 6); strokeColor(C_DIM); strokeWidth(1.3f); stroke();
                fontSize(11); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(query.empty() ? C_FAINT : C_TEXT);
                text(r.x + 26, r.y + r.h / 2 + 0.5f, query.empty() ? "Type to search names and categories" : (query + "|").c_str(), nullptr);
                continue;
            }
            const std::string key = favKey(r.kind, r.idx);
            const bool isCur = r.kind == 0 ? (curUser.empty() && r.idx == curFactory) : (r.kind == 1 ? curUser == userPresets[r.idx] : false);
            const bool del = r.kind == 1 && pendingDelete == r.idx;
            if (hot || del) { beginPath(); roundedRect(r.x, r.y, r.w, r.h, 4); fillColor(del ? Color(0xd0, 0x50, 0x40, 0.35f) : withAlpha(C_ACC, 0.18f)); fill(); }
            if (isCur) { beginPath(); circle(r.x + 6, r.y + r.h / 2, 2.5f); fillColor(C_ACC); fill(); }
            fontSize(10); textLetterSpacing(0.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot || isCur || del ? C_TEXT : C_DIM);
            const std::string label = del ? "Delete? click again" : itemLabel(r);
            text(r.x + 12, r.y + r.h / 2 + 0.5f, label.c_str(), nullptr);
            // the favourite star
            const bool fav = isFav(key);
            if (fav || hot) drawStar(r.x + r.w - 10, r.y + r.h / 2, 5.f, fav);
        }
    }
    void drawStar(float cx, float cy, float rr, bool filled) {
        beginPath();
        for (int k = 0; k < 10; ++k) { const float a = -3.14159265f / 2 + k * 3.14159265f / 5, q = k % 2 ? rr * 0.45f : rr; if (!k) moveTo(cx + q * std::cos(a), cy + q * std::sin(a)); else lineTo(cx + q * std::cos(a), cy + q * std::sin(a)); }
        closePath();
        if (filled) { fillColor(C_FAV); fill(); } else { strokeColor(C_DIM); strokeWidth(1); stroke(); }
    }
    int menuHit(float x, float y) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it);
        for (int i = 0; i < (int)it.size(); ++i) if (x >= it[i].x && x < it[i].x + it[i].w && y >= it[i].y && y < it[i].y + it[i].h) return i;
        return -1; }
    MRect menuItem(int i) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it); return it[i]; }
    void openMenu() { userPresets = listUserPresets(); favs = loadFavourites(); query.clear(); menuOpen = true; menuHover = -1; pendingDelete = -1; repaint(); }
    void applyFavourite(int idx) {
        if (idx >= (int)favs.size()) return;
        const std::string& k = favs[idx];
        if (k.compare(0, 2, "f:") == 0) { for (uint32_t i = 0; i < kNumPresets; ++i) if (k.substr(2) == presets()[i].name) { applyFactory((int)i); return; } }
        else applyUser(k.substr(2));
    }
    void drawSaveBox() {
        const float w = 360, h = 96, x = W / 2 - w / 2, y = 150;
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.45f)); fill();
        beginPath(); roundedRect(x, y, w, h, 8); fillColor(C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        const char* t = savingComp ? (savingCompKind == kCompXlfo ? "SAVE LFO PRESET AS" : (savingCompKind == kCompEg ? "SAVE ENVELOPE PRESET AS" : (savingCompKind == kCompEf ? "SAVE FOLLOWER PRESET AS" : "SAVE ARPEGGIATOR PRESET AS"))) : "SAVE PRESET AS";
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(x + 16, y + 14, t, nullptr);
        beginPath(); roundedRect(x + 16, y + 36, w - 32, 26, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); stroke();
        fontSize(13); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(x + 26, y + 49.5f, (saveName + "|").c_str(), nullptr);
        fontSize(8.5f); textLetterSpacing(1.2f); fillColor(C_FAINT); textAlign(ALIGN_RIGHT | ALIGN_TOP);
        text(x + w - 16, y + 72, saveError ? "COULD NOT SAVE - CHECK THE NAME" : "ENTER TO SAVE   ·   ESC TO CANCEL", nullptr);
    }

    // ---- modulation assign ---------------------------------------------------------
    void assign(int src, uint32_t p) {
        if (src >= kClassicChip) { const int d = destForParam(p); if (d <= 0) { flash("Classic sources reach what Obsidian 2 could modulate - use an XLFO, EG or EF"); return; } assignClassic(1 + src - kClassicChip, d); return; }
        const int d = destForParam2(p); if (d <= 0) { flash("That control can't be modulated"); return; }
        assignDst(src, d);
    }
    static bool pitchDest(int d) { return d == kDstO1Pitch || d == kDstO2Pitch || d == kV2O3First || d == kV2O3First + 4; }
    void assignDst(int src, int dst) {
        int slot = -1; for (int s = 0; s < kNumSlots2; ++s) if (!slotUsed(s)) { slot = s; break; }
        if (slot < 0) { flash("All 100 modulation slots are in use - remove one first"); return; }
        beginGesture();
        commit(sp(slot, kSlotSrc), (float)src); commit(sp(slot, kSlotDst), (float)dst);
        commit(sp(slot, kSlotAmt), pitchDest(dst) ? (src == em::kSrcKey ? 0.5f : 0.02f) : 0.3f); commit(sp(slot, kSlotBypass), 0);
        endGesture();
        revealRow(slot);
        flash(std::string(em::sourceName(src)) + " > " + destName2(dst));
    }
    void assignClassic(int src, int dst) {
        int slot = -1; for (int s = 0; s < kNumSlots; ++s) if (!classicUsed(s)) { slot = s; break; }
        if (slot < 0) { flash("All 24 classic slots are in use - remove one first"); return; }
        beginGesture();
        commit(slotParam(slot, kSlotSrc), (float)src); commit(slotParam(slot, kSlotDst), (float)dst);
        commit(slotParam(slot, kSlotAmt), pitchDest(dst) ? 0.02f : 0.3f); commit(slotParam(slot, kSlotBypass), 0);
        endGesture();
        revealRow(kClassicRow + slot);
        flash(std::string("Classic ") + sourceName(src) + " > " + destName(dst));
    }
    void revealRow(int id) {
        selSlot = id; const std::vector<int> u = usedRows();
        for (int i = 0; i < (int)u.size(); ++i) if (u[i] == id) { if (i >= slotScroll + SLOT_ROWS) slotScroll = i - SLOT_ROWS + 1; if (i < slotScroll) slotScroll = i; }
    }

    // ---- hit testing ----------------------------------------------------------------
    static bool inRect(float x, float y, float rx, float ry, float rw, float rh) { return x >= rx && x < rx + rw && y >= ry && y < ry + rh; }
    int hit(float x, float y, int* seg = nullptr) const {
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) {
            const Ctl& c = cs[i]; if (!visible(c)) continue;
            switch (c.type) {
            case Knob: { const float rr = c.r + 6; if ((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y) <= rr * rr) return i; break; }
            case Toggle: { const float right = c.label[0] ? 56.f : 16.f; if (x >= c.x - 16 && x <= c.x + right && std::fabs(y - c.y) <= 9) return i; break; }
            case Segment: { int n = 0; segLabels(c.group, c.field, n); const float x0 = c.x - c.r / 2;
                if (x >= x0 && x < x0 + c.r && std::fabs(y - c.y) <= SEG_H / 2 + 1) { if (seg) *seg = (int)((x - x0) / (c.r / n)); return i; } break; }
            case Stepper: if (std::fabs(x - c.x) <= c.r / 2 && std::fabs(y - c.y) <= 10) { if (seg) *seg = x < c.x ? -1 : 1; return i; } break;
            }
        }
        return -1;
    }
    int headerHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H) return 0;
        if (x >= LEARN_X && x <= LEARN_X + LEARN_W) return 1;
        if (x >= MIDI_X && x <= MIDI_X + MIDI_W) return 2;
        if (x >= UNDO_X && x <= UNDO_X + UR_W) return 3;
        if (x >= REDO_X && x <= REDO_X + UR_W) return 4;
        if (x >= HELP_X && x <= HELP_X + HELP_W) return 5;
        if (x >= A_X && x < A_X + AB_W) return 6;
        if (x >= A_X + AB_W && x < A_X + 2 * AB_W) return 7;
        if (x >= COPY_X && x <= COPY_X + COPY_W) return 8;
        return 0;
    }
    int presetBarHit(float x, float y) const { if (y < PS_Y || y > PS_Y + PS_H || x < PS_X || x > PS_X + PS_W) return -1; return x < PS_X + PS_ARROW ? 0 : (x > PS_X + PS_W - PS_ARROW ? 2 : 1); }
    bool inDisplay(float x, float y) const { return inRect(x, y, DX + 2, DY + 2, DW - 4, DH - 14); }
    int handleHit(float x, float y) const {
        int best = -1; float bd = 12 * 12;
        for (int f = kNumFilters - 1; f >= 0; --f) { if (!fOn(f)) continue; float hx, hy; handlePos(f, hx, hy); const float d = (x - hx) * (x - hx) + (y - hy) * (y - hy); if (d <= bd) { bd = d; best = f; } }
        return best;
    }
    int filterTabHit(float x, float y, bool* dot = nullptr) const {
        for (int f = 0; f < kNumFilters; ++f) { const float fx = FT_X + f * (FT_W + 4); if (inRect(x, y, fx, FT_Y, FT_W, FT_H)) { if (dot) *dot = x < fx + 24; return f; } }
        return -1;
    }
    int tabHit(float x, float y) const { if (std::fabs(y - TAB_Y) > TAB_H / 2) return -1; for (int t = 0; t < kNumTabs; ++t) { const float tx = tabX(t); if (x >= tx && x < tx + TAB_W) return t; } return -1; }
    int chipHit(float x, float y) const { for (const Chip& c : chips()) if (inRect(x, y, c.x, c.y, CHIP_W, CHIP_H)) return c.src; return -1; }
    bool styleHit(float x, float y) const { return inRect(x, y, STYLE_X, STYLE_Y, STYLE_W, 24); }
    bool stepHit(float x, float y) const { return tab <= 3 && val[lp_(tab, em::kLMode)] > 0.5f && inRect(x, y, STEP_X, STEP_Y, STEP_W, STEP_H); }
    bool xyHit(float x, float y) const { return tab >= 8 && tab <= 9 && inRect(x, y, XY_X, XY_Y, XY_W, XY_H); }
    int egHandleHit(float x, float y) const {
        if (tab < 4 || tab > 5) return -1;
        for (int k = 0; k < 3; ++k) { float hx, hy; egHandle(tab - 4, k, hx, hy); if ((x - hx) * (x - hx) + (y - hy) * (y - hy) <= 64) return k; }
        return -1;
    }
    bool clearHit(float x, float y) const { return learnMode && learnWaiting < 0 && inRect(x, y, W - 16 - 110, STATUS_Y - 8, 110, 16); }
    int ringBtnHit(float x, float y) const { for (int pr = 0; pr < 2; ++pr) if (ringHit(pr, x, y)) return pr; return -1; }
    int areaHit(float x, float y) const {
        if (compHit(x, y)) return kTipCompPresets;
        if (arpCompHit(x, y)) return kTipArpPresets;
        if (inDisplay(x, y)) return handleHit(x, y) >= 0 ? kTipHandle : kTipDisplay;
        if (filterTabHit(x, y) >= 0) return kTipFilterTab;
        if (stepHit(x, y)) return kTipStepEdit;
        if (xyHit(x, y)) return kTipXyPad;
        if (egHandleHit(x, y) >= 0) return kTipEgCurve;
        if (tabHit(x, y) >= 0) return kTipSourceTab;
        if (fxUnitHit(x, y) >= 0) return kTipFxUnit;
        if (inRect(x, y, ARPV_X, ARPV_Y, ARPV_W, ARPV_H)) return kTipArpView;
        for (int o = 0; o < kNumOsc; ++o) if (inRect(x, y, OX0 + o * OCOLW + 10, RA + 34, OCOLW - 20, 40)) return kTipOscWave;
        if (inRect(x, y, AX + 14, RB + 30, AW - 28, 74)) return kTipEnv;
        return -1;
    }
    void setStepAt(float x, float y, bool reset) {
        const int l = tab, s = (int)clampf((x - STEP_X) / (STEP_W / em::kNumSteps), 0, em::kNumSteps - 1);
        const float v = reset ? 0.f : clampf((STEP_Y + STEP_H / 2 - y) / (STEP_H / 2 - 6), -1.f, 1.f);
        setValue(lp_(l, em::kLStep1 + s), std::round(v * 20.f) / 20.f);
    }
    void setXyAt(float x, float y) { const int k = tab - 8; setValue(xyp(k, 0), clampf((x - XY_X - 8) / (XY_W - 16), 0.f, 1.f)); setValue(xyp(k, 1), clampf((XY_Y + XY_H - 8 - y) / (XY_H - 16), 0.f, 1.f)); }
    void addFilterAt(float hz) {
        int f = -1; for (int k = 0; k < kNumFilters; ++k) if (!fOn(k)) { f = k; break; }
        if (f < 0) { flash("All four filters are on"); return; }
        beginGesture(); commit(fpar(f, kFOn), 1); commit(fpar(f, kFFreq), clampf(hz / std::pow(2.f, val[kFreqOffset]), 20.f, 20000.f)); endGesture();
        selF = f; char b[64]; snprintf(b, sizeof b, "Filter %d on at %.0f Hz", f + 1, hz); flash(b);
    }

    void setLearn(bool on) { learnMode = on; learnWaiting = -1; if (!on) setState("cmd", "cancel"); repaint(); }
    void learn(uint32_t p) { learnWaiting = (int)p; char b[24]; snprintf(b, sizeof b, "learn:%u", p); setState("cmd", b); repaint(); }
    void endEdit(bool apply) {
        if (editing < 0) return;
        const uint32_t p = effParam(controls()[editing]); float v;
        if (apply && parse(p, editBuf, v)) { beginGesture(); commit(p, v); endGesture(); }
        editing = -1; editBuf.clear(); repaint();
    }
    static float angleNorm(const Ctl& c, float x, float y) {
        const float a = std::atan2(y - c.y, x - c.x), a0 = 0.75f * 3.14159265f;
        float rel = a - a0; while (rel < 0) rel += 2 * 3.14159265f;
        if (rel > 1.5f * 3.14159265f) rel = rel > 1.75f * 3.14159265f ? 0.f : 1.5f * 3.14159265f;
        return rel / (1.5f * 3.14159265f);
    }

    bool onKeyboard(const KeyboardEvent& ev) override {
        if (!ev.press) return false;
        std::string* buf = saving ? &saveName : (editing >= 0 ? &editBuf : nullptr);
        if (!buf && menuOpen) {                              // typing in the preset browser searches
            if (ev.key == kKeyEscape) { if (!query.empty()) query.clear(); else menuOpen = false; }
            else if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!query.empty()) query.pop_back(); }
            else if (ev.key >= 32 && ev.key < 127 && query.size() < 32) query += (char)ev.key;
            else return false;
            menuHover = -1; pendingDelete = -1; repaint(); return true;
        }
        if (!buf) {
            const bool cmd = (ev.mod & (kModifierControl | kModifierSuper)) != 0;
            if (cmd && (ev.key == 'z' || ev.key == 'Z')) { if (ev.mod & kModifierShift) redo(); else undo(); return true; }
            if (cmd && (ev.key == 'y' || ev.key == 'Y')) { redo(); return true; }
            if (ev.key == kKeyEscape) { if (helpOpen) helpOpen = false; else if (popup.open) popup.open = false; else if (kpan.open) kpan.open = false; else if (menuOpen) menuOpen = false; else if (learnMode) setLearn(false); repaint(); return true; }
            return false;
        }
        if (ev.key == kKeyEnter) {
            if (saving && savingComp) {
                float fv[em::kXlfoFields + kArpCompFields]; for (int f = 0; f < compFieldCount(savingCompKind); ++f) fv[f] = val[compParam(savingCompKind, savingCompIdx, f)];
                if (saveCompPreset(savingCompKind, saveName, fv)) { saving = savingComp = false; saveError = false; flash("Saved " + sanitizeName(saveName)); } else saveError = true;
            } else if (saving) {
                if (saveUserPreset(saveName, val)) { userPresets = listUserPresets(); curUser = sanitizeName(saveName); curFactory = -1; std::copy(val, val + kParamCount, userVals); saving = false; saveError = false; }
                else saveError = true;
            } else endEdit(true);
            repaint(); return true;
        }
        if (ev.key == kKeyEscape) { if (saving) saving = savingComp = false; else endEdit(false); repaint(); return true; }
        if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!buf->empty()) buf->pop_back(); repaint(); return true; }
        if (ev.key >= 32 && ev.key < 127 && buf->size() < (saving ? 40u : 12u)) {
            char ch = (char)ev.key;
            if ((ev.mod & kModifierShift) && std::isalpha((unsigned char)ch)) ch = (char)std::toupper((unsigned char)ch);
            *buf += ch; repaint(); return true;
        }
        return true;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (ev.press) { flashUntil = 0; tipShown = false; tipSuppressed = tipKey; }
        if (helpOpen) { if (ev.press && ev.button == 1) helpClick(x, y); return true; }
        if (ev.button == 2 && ev.press) {                  // right-click
            if (popup.open && popup.kind == 3) { const int m = popupHit(x, y); compPendingDel = (m >= 0 && compItems[m].type == 1) ? m : -1; repaint(); return true; }
            if (menuOpen) { const int m = menuHit(x, y); if (m >= 0 && menuItem(m).kind == 1) { pendingDelete = menuItem(m).idx; repaint(); } return true; }
            if (learnMode) { const int i = hit(x, y); uint32_t p = kParamCount;
                if (i >= 0) p = effParam(controls()[i]); else { const int ft = filterTabHit(x, y); if (ft >= 0) p = fpar(ft, kFOn); const int rb = ringBtnHit(x, y); if (rb >= 0) p = rb ? kRing34 : kRing12; }
                if (p < kParamCount) { char b[24]; snprintf(b, sizeof b, "unmap:%u", p); setState("cmd", b); paramCC[p] = -1; repaint(); }
                return true; }
            const int hd = inDisplay(x, y) ? handleHit(x, y) : -1;
            if (hd >= 0) { beginGesture(); commit(fpar(hd, kFOn), 0); endGesture(); char b[32]; snprintf(b, sizeof b, "Filter %d off", hd + 1); flash(b); return true; }
            { const int i = hit(x, y);
              if (i >= 0 && controls()[i].type == Stepper) { const uint32_t p = effParam(controls()[i]); const ParamDef& d = paramDef(p); float v = val[p] - 1; if (v < d.min) v = d.max; beginGesture(); commit(p, v); endGesture(); return true; }
              if (i >= 0 && controls()[i].type == Knob) { popup.open = false; openKnobPanel(effParam(controls()[i]), x, y); return true; } }
            return false;
        }
        if (ev.button != 1) return false;
        if (!ev.press) {
            if (dragSrc > 0) { const int i = hit(x, y);
                if (i >= 0) assign(dragSrc, effParam(controls()[i]));
                else { const int sp_ = slotPartHit(x, y);
                    if (sp_ >= 0 && sp_ % 10 == 3) { const int id = sp_ / 10;
                        if (dragSrc >= kClassicChip && isClassicRow(id)) assignClassic(1 + dragSrc - kClassicChip, kDstSlotFirst + id - kClassicRow);
                        else if (dragSrc < kClassicChip && !isClassicRow(id)) assignDst(dragSrc, kV2SlotFirst + id);
                        else flash("Classic sources modulate classic slots; the others modulate the new slots"); } }
                dragSrc = -1; repaint(); return true; }
            if (active >= 0) { editParameter(effParam(controls()[active]), false); active = -1; endGesture(); return true; }
            if (activeRow >= 0) { editParameter(rowParam(activeRow, kSlotAmt), false); activeRow = -1; endGesture(); return true; }
            if (dragHandle >= 0) { const int t = (int)std::round(val[fpar(dragHandle, kFType)]); editParameter(fpar(dragHandle, kFFreq), false); editParameter(fpar(dragHandle, isEqShape(t) ? kFGain : kFPeak), false); dragHandle = -1; endGesture(); return true; }
            if (dragEgHandle >= 0) { editParameter(kEgCurveFirst + (tab - 4) * 3 + dragEgHandle, false); dragEgHandle = -1; endGesture(); return true; }
            if (dragStep) { dragStep = false; endGesture(); return true; }
            if (dragXy) { dragXy = false; endGesture(); return true; }
            return false;
        }
        if (editing >= 0) endEdit(true);
        if (popup.open) { const int i = popupHit(x, y); if (i >= 0) popupPick(i); else if (!popupInside(x, y)) popup.open = false; repaint(); return true; }
        if (kpan.open) {
            const int part = kpHit(x, y);
            if (part < 0) { if (!kpInside(x, y)) kpan.open = false; repaint(); return true; }
            rowClick(part, x, y, ev.mod);
            if (rowsFor(kpan.param).empty()) kpan.open = false;
            repaint(); return true;
        }
        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m >= 0) {
                const MRect r = menuItem(m);
                if (r.kind == 3) return true;                       // the search field: just type
                if ((r.kind == 0 || r.kind == 1 || r.kind == 4) && x >= r.x + r.w - 20) { toggleFav(favKey(r.kind, r.idx)); return true; }
                if (r.kind == 2) { menuOpen = false; saving = true; savingComp = false; saveError = false; saveName = curUser; }
                else if (r.kind == 1 && pendingDelete == r.idx) { deleteUserPreset(userPresets[r.idx]); if (curUser == userPresets[r.idx]) curUser.clear(); userPresets = listUserPresets(); pendingDelete = -1; repaint(); return true; }
                else { menuOpen = false; if (r.kind == 0) applyFactory(r.idx); else if (r.kind == 1) applyUser(userPresets[r.idx]); else applyFavourite(r.idx); }
            } else menuOpen = false;
            pendingDelete = -1; menuHover = -1; repaint(); return true;
        }
        const int hh = headerHit(x, y);
        if (hh == 1) { setLearn(!learnMode); return true; }
        if (hh == 2) { beginGesture(); commit(kMidiOn, val[kMidiOn] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
        if (hh == 3) { undo(); return true; }
        if (hh == 4) { redo(); return true; }
        if (hh == 5) { helpOpen = true; repaint(); return true; }
        if (hh == 6 || hh == 7) { switchAB(hh - 6); return true; }
        if (hh == 8) { copyAB(); return true; }
        const int pb = presetBarHit(x, y);
        if (pb == 0) { step(-1); return true; }
        if (pb == 2) { step(1); return true; }
        if (pb == 1) { openMenu(); return true; }
        if (clearHit(x, y)) { setState("cmd", "clear"); for (int& m : paramCC) m = -1; flash("All MIDI assignments cleared"); return true; }
        { const int rb = ringBtnHit(x, y); if (rb >= 0) { const uint32_t p = rb ? kRing34 : kRing12; if (learnMode) { learn(p); return true; } beginGesture(); commit(p, val[p] > 0.5f ? 0.f : 1.f); endGesture(); return true; } }
        { bool dot = false; const int ft = filterTabHit(x, y, &dot);
          if (ft >= 0) { if (learnMode) { learn(fpar(ft, kFOn)); return true; }
              if (dot) { beginGesture(); commit(fpar(ft, kFOn), fOn(ft) ? 0.f : 1.f); endGesture(); }
              selF = ft; repaint(); return true; } }
        const int tb = tabHit(x, y); if (tb >= 0) { tab = tb; repaint(); return true; }
        { const int u = fxUnitHit(x, y); if (u >= 0) { selFx = u; repaint(); return true; } }
        if (!learnMode) {
            if (compHit(x, y)) { openCompMenu(false); return true; }
            if (arpCompHit(x, y)) { openCompMenu(true); return true; }
            const int ch = chipHit(x, y); if (ch > 0) { dragSrc = ch; dragPX = x; dragPY = y; repaint(); return true; }
            if (styleHit(x, y)) { popup.open = true; popup.kind = 0; popup.slot = -1; popup.x = STYLE_X; popup.y = STYLE_Y + 28; popup.hover = -1; repaint(); return true; }
            const int part = slotPartHit(x, y); if (part >= 0) { rowClick(part, x, y, ev.mod); return true; }
        }
        int seg = 0; const int i = hit(x, y, &seg);
        if (i >= 0 && !learnMode && ringHitKnob(controls()[i], x, y)) { openKnobPanel(effParam(controls()[i]), x, y); return true; }
        if (i >= 0) {
            const Ctl& c = controls()[i]; const uint32_t p = effParam(c);
            if (learnMode) { learn(p); return true; }
            if (c.type == Toggle) { beginGesture(); commit(p, val[p] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
            if (c.type == Segment) {
                beginGesture();
                if (c.group == GFilt && c.field == kFSlope) commit(p, seg == 0 ? (float)kSlope6 : (float)(seg - 1));
                else commit(p, (float)seg);
                endGesture(); return true; }
            if (c.type == Stepper) { const ParamDef& d = paramDef(p); float v = val[p] + (c.group == GOsc && c.field == kOWave ? 1 : seg); if (v > d.max) v = d.min; if (v < d.min) v = d.max; beginGesture(); commit(p, v); endGesture(); return true; }
            if (ev.mod & (kModifierControl | kModifierSuper)) { beginGesture(); commit(p, defv[p]); endGesture(); return true; }
            if (i == lastClick && ev.time - lastClickTime < 350) {
                lastClick = -1; editing = i; char b[48]; fmt(p, b, sizeof b); editBuf = b;
                const size_t spc = editBuf.find(' '); if (spc != std::string::npos && !isPan(p)) editBuf.resize(spc);
                if (!editBuf.empty() && editBuf.back() == '%') editBuf.pop_back();
                if (std::strcmp(paramDef(p).unit, "Hz") == 0 && val[p] >= 1000) { char kb[16]; snprintf(kb, sizeof kb, "%.2fk", val[p] / 1000); editBuf = kb; }
                if (isPan(p)) { char pb2[16]; snprintf(pb2, sizeof pb2, "%s%.0f", val[p] < 0 ? "L" : "R", std::fabs(val[p]) * 100); editBuf = std::fabs(val[p]) < .005f ? "C" : pb2; }
                if (isTimeParam(p) && val[p] < 1) { char tb2[16]; snprintf(tb2, sizeof tb2, "%.0fms", val[p] * 1000); editBuf = tb2; }
                repaint(); return true;
            }
            lastClick = i; lastClickTime = ev.time;
            beginGesture(); active = i; dragY = y; dragNorm = toNorm(p, val[p]);
            const float dist = std::sqrt((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y));
            circular = c.type == Knob && c.r > 17 && dist > c.r * 0.62f && !(paramDef(p).hints & kHintInteger);
            editParameter(p, true);
            if (circular) setValue(p, fromNorm(p, angleNorm(c, x, y)));
            repaint(); return true;
        }
        if (learnMode) return false;
        if (stepHit(x, y)) { beginGesture(); dragStep = true; setStepAt(x, y, (ev.mod & (kModifierControl | kModifierSuper)) != 0); return true; }
        if (xyHit(x, y)) { beginGesture(); dragXy = true; setXyAt(x, y); return true; }
        { const int eh = egHandleHit(x, y); if (eh >= 0) { beginGesture(); dragEgHandle = eh; dragY = y; dragNorm = val[kEgCurveFirst + (tab - 4) * 3 + eh]; editParameter(kEgCurveFirst + (tab - 4) * 3 + eh, true); return true; } }
        if (inDisplay(x, y)) {
            const int hd = handleHit(x, y);
            if (hd >= 0) {
                selF = hd; beginGesture(); dragHandle = hd; dragX = x; dragY = y;
                const int t = (int)std::round(val[fpar(hd, kFType)]);
                editParameter(fpar(hd, kFFreq), true); editParameter(fpar(hd, isEqShape(t) ? kFGain : kFPeak), true); repaint(); return true;
            }
            if (ev.time - lastDispClick < 350 && std::fabs(x - lastDispX) < 6) { lastDispClick = 0; addFilterAt(hzOfX(x)); return true; }
            lastDispClick = ev.time; lastDispX = x; return true;
        }
        selSlot = -1;
        return false;
    }

    bool onMotion(const MotionEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (helpOpen) { const int h = helpHit(x, y); if (h != helpHover) { helpHover = h; repaint(); } return true; }
        if (dragSrc > 0) { dragPX = x; dragPY = y; hover = hit(x, y); repaint(); return true; }
        if (popup.open) { const int h = popupHit(x, y); if (h != popup.hover) { popup.hover = h; repaint(); } return true; }
        if (kpan.open && activeRow < 0) { const int h = kpHit(x, y); if (h != hoverKp) { hoverKp = h; repaint(); } return true; }
        const float fine = (ev.mod & kModifierShift) ? 0.15f : 1.f;
        if (active >= 0) {
            const Ctl& c = controls()[active]; const uint32_t p = effParam(c);
            if (circular) { setValue(p, fromNorm(p, angleNorm(c, x, y))); return true; }
            float sens = 220.f;
            if (paramDef(p).hints & kHintInteger) sens = std::max(60.f, 10.f * (paramDef(p).max - paramDef(p).min));
            dragNorm = clampf(dragNorm + (dragY - y) / sens * fine, 0.f, 1.f); dragY = y;
            if (paramDef(p).hints & kHintInteger) setValue(p, paramDef(p).min + dragNorm * (paramDef(p).max - paramDef(p).min)); else setValue(p, fromNorm(p, dragNorm));
            return true;
        }
        if (activeRow >= 0) { const uint32_t ap = rowParam(activeRow, kSlotAmt); dragNorm = clampf(dragNorm + (dragY - y) / 440.f * fine, 0.f, 1.f); dragY = y; setValue(ap, fromNorm(ap, dragNorm)); return true; }
        if (dragHandle >= 0) {
            const int f = dragHandle, t = (int)std::round(val[fpar(f, kFType)]);
            const float nx = dragX + (x - dragX) * fine, ny = dragY + (y - dragY) * fine; dragX = nx; dragY = ny;
            setValue(fpar(f, kFFreq), clampf(hzOfX(nx) / std::pow(2.f, val[kFreqOffset]), 20.f, 20000.f));
            if (isEqShape(t)) setValue(fpar(f, kFGain), clampf(dbOfY(ny), -24.f, 24.f)); else setValue(fpar(f, kFPeak), clampf(peakOfY(ny), 0.f, 1.f));
            return true;
        }
        if (dragEgHandle >= 0) {
            const int e = tab - 4, k = dragEgHandle; const uint32_t p = kEgCurveFirst + e * 3 + k;
            const float sign = k == 0 ? 1.f : (k == 1 ? -1.f : 1.f) * (k == 2 ? -1.f : 1.f);    // dragging towards the corner bends the curve the way it looks
            dragNorm = clampf(dragNorm + sign * (dragY - y) / 120.f * fine, -1.f, 1.f); dragY = y; setValue(p, std::round(dragNorm * 100.f) / 100.f);
            return true;
        }
        if (dragStep) { setStepAt(x, y, false); return true; }
        if (dragXy) { setXyAt(x, y); return true; }
        if (menuOpen) { const int m = menuHit(x, y); if (m != menuHover) { menuHover = m; repaint(); } return true; }
        const int hh = headerHit(x, y); if (hh != hoverHeader) { hoverHeader = hh; repaint(); }
        const int pb = presetBarHit(x, y); if (pb != presetHover) { presetHover = pb; repaint(); }
        const bool hc = clearHit(x, y); if (hc != hoverClear) { hoverClear = hc; repaint(); }
        const int h = hit(x, y); if (h != hover) { hover = h; repaint(); }
        const int ft = filterTabHit(x, y); if (ft != hoverFTab) { hoverFTab = ft; repaint(); }
        const int tb = tabHit(x, y); if (tb != hoverTab) { hoverTab = tb; repaint(); }
        const int ch = chipHit(x, y); if (ch != hoverChip) { hoverChip = ch; repaint(); }
        const bool hcp = compHit(x, y); if (hcp != hoverComp) { hoverComp = hcp; repaint(); }
        const bool hap = arpCompHit(x, y); if (hap != hoverArpComp) { hoverArpComp = hap; repaint(); }
        const bool hs = styleHit(x, y); if (hs != hoverStyle) { hoverStyle = hs; repaint(); }
        const int hd = inDisplay(x, y) ? handleHit(x, y) : -1; if (hd != hoverHandle) { hoverHandle = hd; repaint(); }
        const bool hdisp = inDisplay(x, y); if (hdisp != hoverDisplay) { hoverDisplay = hdisp; repaint(); }
        const int fu = fxUnitHit(x, y); if (fu != hoverFx) { hoverFx = fu; repaint(); }
        const int rb = ringBtnHit(x, y); if (rb != hoverRing) { hoverRing = rb; repaint(); }
        const int eh = egHandleHit(x, y); if (eh != hoverEgHandle) { hoverEgHandle = eh; repaint(); }
        const int part = slotPartHit(x, y); if (part != hoverSlotPart) { hoverSlotPart = part; repaint(); }
        int key = -1;
        if (h >= 0 && ringHitKnob(controls()[h], x, y)) key = 4000 + kTipModRing;
        else if (h >= 0) key = h;
        else if (hh > 0) key = 1000 + hh;
        else if (pb >= 0) key = 2000 + pb;
        else if (ch > 0) key = 3000 + ch;
        else if (hs) key = 5000;
        else if (rb >= 0) key = 5001 + rb;
        else if (part >= 0) key = 10000 + part;
        else { const int a = areaHit(x, y); if (a >= 0) key = 4000 + a; }
        tipX = x; tipY = y;
        if (key != tipKey) { const bool was = tipShown; tipKey = key; tipSince = nowSec(); tipShown = false; if (key != tipSuppressed) tipSuppressed = -2; if (was) repaint(); }
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const float s = uiScale(), sx = ev.pos.getX() / s, sy = ev.pos.getY() / s;
        if (menuOpen || saving || helpOpen || popup.open) return true;
        const float dir = ev.delta.getY() > 0 ? 1.f : -1.f;
        if (presetBarHit(sx, sy) >= 0) { step(dir > 0 ? -1 : 1); return true; }
        if (inRect(sx, sy, SX, SY, SW, SH)) { slotScroll = (int)clampf((float)(slotScroll - (int)dir), 0.f, (float)maxScroll()); repaint(); return true; }
        if (inDisplay(sx, sy)) { const int hd = handleHit(sx, sy);
            if (hd >= 0) { const int t = (int)std::round(val[fpar(hd, kFType)]); const uint32_t p = fpar(hd, isEqShape(t) ? kFGain : kFPeak);
                beginGesture(); commit(p, fromNorm(p, toNorm(p, val[p]) + dir * 0.02f)); endGesture(); return true; } }
        const int i = hit(sx, sy); if (i < 0) return false;
        const Ctl& c = controls()[i]; const uint32_t p = effParam(c);
        if (c.type == Stepper) { const ParamDef& d = paramDef(p); float v = val[p] + dir; if (v > d.max) v = d.min; if (v < d.min) v = d.max; beginGesture(); commit(p, v); endGesture(); return true; }
        if (c.type != Knob) return false;
        if (paramDef(p).hints & kHintInteger) { beginGesture(); commit(p, val[p] + dir); endGesture(); return true; }
        const float st = (ev.mod & kModifierShift) ? 0.004f : 0.025f;
        beginGesture(); commit(p, fromNorm(p, toNorm(p, val[p]) + dir * st)); endGesture();
        return true;
    }

    void uiIdle() override {
        const double t = nowSec();
        if (t - lastPoll > 0.033) {
            lastPoll = t;
            if (ObsidianPlugin* const dsp = (ObsidianPlugin*)getPluginInstancePointer()) {
                const Engine& e = dsp->engine;
                std::copy(e.uiOff2, e.uiOff2 + kNumDests2, liveOff); std::copy(e.uiSrc2, e.uiSrc2 + em::kNumSources, liveSrc);
                std::copy(e.uiClassic, e.uiClassic + kNumSources, liveClassic);
                for (int l = 0; l < em::kNumXlfo; ++l) livePhase[l] = e.uiXlfoPhase[l];
                for (int l = 0; l < em::kNumXlfo; ++l) liveGlobalXlfo[l] = liveSrc[em::kSrcXlfo1 + l];
                liveGr = e.uiGr; liveArpNote = e.uiArpNote; liveVoices = e.activeVoices();
                latencyMs = e.latency() * 1000.f / std::max(1.0f, (float)getSampleRate());
            }
            repaint();
        }
        if (gTipsOn && !tipShown && tipKey >= 0 && tipKey != tipSuppressed && active < 0 && dragSrc <= 0 && dragHandle < 0 && !menuOpen && !saving && !helpOpen && !popup.open && !kpan.open && editing < 0
            && t - tipSince > 0.6) { tipShown = true; repaint(); }
    }

    // ---- tips --------------------------------------------------------------------
    bool tipContent(std::string& title, std::string& value, std::string& body) const {
        if (tipKey < 0) return false;
        char b[64];
        if (tipKey < 1000) { const Ctl& c = controls()[tipKey]; const uint32_t p = effParam(c); fmt(p, b, sizeof b); title = paramDef(p).name; value = b; body = paramTip(p);
            if (c.type == Knob) body += "\n\nDrag up/down (Shift for fine). Double-click to type, Cmd/Ctrl-click to reset.";
            if (c.type == Knob && (destForParam2(p) > 0 || destForParam(p) > 0)) body += " Drop a source chip here to modulate it; right-click to list what modulates it.";
            if (c.type == Stepper) body += "\n\nClick the arrows (or scroll; right-click steps back).";
            if (dimmed(p)) body += "\n\n(Not active with the current settings.)";
            if (paramCC[p] >= 0) { char cc[24]; snprintf(cc, sizeof cc, "   ·   CC%d", paramCC[p]); value += cc; }
            return true; }
        if (tipKey >= 10000) { const int id = (tipKey - 10000) / 10, k = (tipKey - 10000) % 10;
            if (isClassicRow(id)) { title = "Classic slot " + std::to_string(id - kClassicRow + 1); value.clear(); body = std::string(targetTip(kTipClassicRow)) + (k == 5 ? "\n\nThe cross removes it." : ""); return true; }
            static const int f[6] = { 0, kSlotSrc, kSlotDst, kSlotAmt, kSlotBypass, kSlotSrc };
            title = "Slot " + std::to_string(id + 1); value.clear(); body = k == 5 ? "Removes this modulation." : paramTip(sp(id, f[k])); return true; }
        if (tipKey == 5000) { const int st = (int)std::round(val[fpar(selF, kFStyle)]); title = "Style"; value = styleName(st); body = std::string(styleTip(st)) + "\n\nClick to choose another style."; return true; }
        if (tipKey == 5001 || tipKey == 5002) { const uint32_t p = tipKey == 5001 ? kRing12 : kRing34; title = paramDef(p).name; value = val[p] > 0.5f ? "On" : "Off"; body = paramTip(p); return true; }
        if (tipKey >= 3000 && tipKey < 4000) { const int src = tipKey - 3000; title = chipName(src); value.clear(); body = targetTip(src >= kClassicChip ? kTipClassicChip : kTipChip); return true; }
        int t;
        if (tipKey >= 4000) t = tipKey - 4000;
        else if (tipKey >= 2000) t = kTipPrev + (tipKey - 2000);
        else { const int hh = tipKey - 1000; t = hh == 1 ? kTipLearn : hh == 2 ? kTipMidi : hh == 3 ? kTipUndo : hh == 4 ? kTipRedo : hh == 5 ? kTipHelp : hh == 6 ? kTipA : hh == 7 ? kTipB : kTipCopy; }
        title = targetTitle(t); value.clear(); body = targetTip(t);
        if (t == kTipUndo || t == kTipRedo) { const size_t n = t == kTipUndo ? undoStack.size() : redoStack.size(); snprintf(b, sizeof b, "%d step%s", (int)n, n == 1 ? "" : "s"); value = b; }
        return true;
    }
    void drawTooltip() {
        std::string title, value, body;
        if (!tipContent(title, value, body)) return;
        const float w = 320, pad = 12;
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f);
        float bb[4]; textBoxBounds(0, 0, w - 2 * pad, body.c_str(), nullptr, bb);
        const float h = pad + 16 + 8 + (bb[3] - bb[1]) + pad;
        float x = tipX + 16, y = tipY + 20;
        if (x + w > W - 8) x = tipX - w - 12;
        if (y + h > H - 8) y = tipY - h - 12;
        x = clampf(x, 8, W - w - 8); y = clampf(y, 8, H - h - 8);
        beginPath(); rect(x - 10, y - 4, w + 20, h + 18); fillPaint(boxGradient(x, y + 3, w, h, 6, 14, Color(0, 0, 0, 0.55f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(Color(0x20, 0x23, 0x27)); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_TEXT);
        std::string up = title; for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
        text(x + pad, y + pad + 1, up.c_str(), nullptr);
        if (!value.empty()) { fontSize(10.5f); textLetterSpacing(0.4f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_ACC); text(x + w - pad, y + pad + 1, value.c_str(), nullptr); }
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM);
        textBox(x + pad, y + pad + 24, w - 2 * pad, body.c_str(), nullptr);
        textLineHeight(1.f);
    }

    // ---- help window ---------------------------------------------------------------
    static constexpr float HX = 240, HY = 100, HW = 760, HH = 600, HTAB_W = 180, HTAB_H = 34;
    int helpHit(float x, float y) const {
        if (x < HX || x > HX + HW || y < HY || y > HY + HH) return -2;
        for (int i = 0; i < kNumHelpPages; ++i) if (x >= HX + 12 && x < HX + 12 + HTAB_W && y >= HY + 56 + i * HTAB_H && y < HY + 56 + (i + 1) * HTAB_H) return i;
        const float by = HY + HH - 44;
        if (y >= by && y < by + 28) {
            if (x >= HX + 12 && x < HX + 12 + HTAB_W) return 100;
            if (x >= HX + HW - 330 && x < HX + HW - 140) return 101;
            if (x >= HX + HW - 124 && x < HX + HW - 16) return 102;
        }
        if (x >= HX + HW - 40 && y < HY + 40) return 102;
        return -1;
    }
    void helpClick(float x, float y) {
        const int h = helpHit(x, y);
        if (h >= 0 && h < kNumHelpPages) helpPage = h;
        else if (h == 100) gTipsOn = !gTipsOn;
        else if (h == 101) openURL(kManualURL);
        else if (h == 102 || h == -2) helpOpen = false;
        repaint();
    }
    void drawHelp() {
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.55f)); fill();
        beginPath(); roundedRect(HX, HY, HW, HH, 10); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(13); textLetterSpacing(4.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(HX + 22, HY + 28, "OBSIDIAN HELP", nullptr);
        const float cx = HX + HW - 24, cy = HY + 26;
        beginPath(); moveTo(cx - 6, cy - 6); lineTo(cx + 6, cy + 6); moveTo(cx + 6, cy - 6); lineTo(cx - 6, cy + 6); strokeColor(helpHover == 102 ? C_TEXT : C_DIM); strokeWidth(1.6f); stroke();
        for (int i = 0; i < kNumHelpPages; ++i) {
            const float ty = HY + 56 + i * HTAB_H;
            if (i == helpPage || i == helpHover) { beginPath(); roundedRect(HX + 12, ty + 2, HTAB_W, HTAB_H - 4, 5); fillColor(i == helpPage ? withAlpha(C_ACC, 0.16f) : C_PANEL_HI); fill(); }
            if (i == helpPage) { beginPath(); roundedRect(HX + 12, ty + 8, 3, HTAB_H - 16, 1.5f); fillColor(C_ACC); fill(); }
            fontSize(11.5f); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(i == helpPage ? C_TEXT : C_DIM);
            text(HX + 28, ty + HTAB_H / 2 + 0.5f, kHelpPages[i].title, nullptr);
        }
        beginPath(); moveTo(HX + HTAB_W + 26, HY + 56); lineTo(HX + HTAB_W + 26, HY + HH - 56); strokeColor(C_LINE); strokeWidth(1); stroke();
        const float bx = HX + HTAB_W + 46, bw = HW - HTAB_W - 70;
        fontSize(15); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_ACC); text(bx, HY + 60, kHelpPages[helpPage].title, nullptr);
        fontSize(12); textLetterSpacing(0.2f); textLineHeight(1.38f); fillColor(Color(0xc8, 0xc6, 0xc0));
        textBox(bx, HY + 92, bw, kHelpPages[helpPage].body, nullptr);
        textLineHeight(1.f);
        const float by = HY + HH - 44;
        beginPath(); moveTo(HX + 12, by - 10); lineTo(HX + HW - 12, by - 10); strokeColor(C_LINE); stroke();
        const float tw = 34, th = 18, tx = HX + 16, tyy = by + 14 - th / 2;
        beginPath(); roundedRect(tx, tyy, tw, th, th / 2); fillColor(gTipsOn ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(gTipsOn ? C_ACC : C_LINE); stroke();
        beginPath(); circle(gTipsOn ? tx + tw - th / 2 : tx + th / 2, by + 14, 6); fillColor(gTipsOn ? C_ACC : C_DIM); fill();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(helpHover == 100 ? C_TEXT : C_DIM); text(tx + tw + 10, by + 14.5f, "HOVER TIPS", nullptr);
        auto btn = [&](float x0, float w0, const char* label, bool hot, bool primary) {
            beginPath(); roundedRect(x0, by, w0, 28, 5); fillColor(primary ? (hot ? withAlpha(C_ACC, 0.32f) : withAlpha(C_ACC, 0.16f)) : (hot ? C_PANEL_HI : C_PANEL)); fill();
            strokeColor(primary ? C_ACC : C_LINE); strokeWidth(1); stroke();
            fontSize(9.5f); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(x0 + w0 / 2, by + 14.5f, label, nullptr);
        };
        btn(HX + HW - 330, 190, "FULL MANUAL (WEB)", helpHover == 101, false);
        btn(HX + HW - 124, 108, "CLOSE", helpHover == 102, true);
    }

private:
    float val[kParamCount], defv[kParamCount], userVals[kParamCount] = {}, before[kParamCount];
    int paramCC[kParamCount];
    int active = -1, hover = -1, lastClick = -1, editing = -1;
    uint lastClickTime = 0, lastDispClick = 0; float lastDispX = 0;
    float dragX = 0, dragY = 0, dragNorm = 0; bool circular = false, gesture = false;
    int selF = 0, hoverFTab = -1, dragHandle = -1, hoverHandle = -1, tab = 0, hoverTab = -1, selFx = 0, hoverFx = -1, hoverRing = -1;
    int hoverEgHandle = -1, dragEgHandle = -1;
    bool hoverDisplay = false;
    int dragSrc = -1, hoverChip = -1; float dragPX = 0, dragPY = 0;
    int selSlot = -1, slotScroll = 0, hoverSlotPart = -1, activeRow = -1;
    bool hoverStyle = false, hoverComp = false, hoverArpComp = false, dragStep = false, dragXy = false;
    std::vector<Snap> undoStack, redoStack; Snap abStore[2]; int abSlot = 0;
    std::string editBuf, statusText; double flashUntil = 0;
    int curFactory = 0, presetHover = -1, menuHover = -1, pendingDelete = -1, hoverHeader = 0;
    std::string curUser, query; std::vector<std::string> userPresets, favs;
    bool menuOpen = false, saving = false, saveError = false; std::string saveName;
    bool learnMode = false, hoverClear = false; int learnWaiting = -1; float lastLearnEcho = 0;
    int tipKey = -1, tipSuppressed = -2; double tipSince = 0; bool tipShown = false; float tipX = 0, tipY = 0;
    bool helpOpen = false; int helpPage = 0, helpHover = -1;
    float liveOff[kNumDests2], liveSrc[em::kNumSources], liveClassic[kNumSources]; double livePhase[em::kNumXlfo] = {}; float liveGlobalXlfo[em::kNumXlfo] = {};
    float liveGr = 0, latencyMs = 0; int liveArpNote = -1, liveVoices = 0;
    double lastPoll = 0;
    ResizeHandle resizeHandle;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObsidianUI)
};

UI* createUI() { return new ObsidianUI(); }

END_NAMESPACE_DISTRHO
