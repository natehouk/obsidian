// Obsidian — custom vector UI (DPF/NanoVG). Original design. License: MIT.
#include "DistrhoUI.hpp"
#include "ParamDefs.hpp"
#include "Presets.hpp"
#include "UserPresets.hpp"
#include "ResizeHandle.hpp"
#include <cctype>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

START_NAMESPACE_DISTRHO

using namespace obsidian;
using DGL_NAMESPACE::ResizeHandle;
typedef std::complex<float> cplx;

static constexpr float W = 1240.f, H = 780.f;

#define C_BG        Color(0x0f, 0x10, 0x12)
#define C_PANEL     Color(0x18, 0x1a, 0x1d)
#define C_PANEL_HI  Color(0x22, 0x25, 0x29)
#define C_LINE      Color(0x2c, 0x30, 0x35)
#define C_TEXT      Color(0xe8, 0xe4, 0xdc)
#define C_DIM       Color(0x84, 0x88, 0x8e)
#define C_FAINT     Color(0x4a, 0x4e, 0x54)
#define C_ACC       Color(0xb4, 0x9c, 0xff)       // obsidian violet
#define C_DISPLAY   Color(0x0b, 0x0c, 0x0d)

static Color withAlpha(const Color& c, float a) { Color r(c); r.alpha = a; return r; }
static Color srcColor(int s) {
    switch (s) {
    case kSrcLfo1: return Color(0x5f, 0xd0, 0xc0);
    case kSrcLfo2: return Color(0x4f, 0xb0, 0xe8);
    case kSrcLfo3: return Color(0x7f, 0xe0, 0x8a);
    case kSrcEg1:  return Color(0xff, 0xa8, 0x3e);
    case kSrcEg2:  return Color(0xff, 0x7a, 0x5c);
    case kSrcEg3:  return Color(0xf0, 0xd0, 0x60);
    case kSrcVelocity: return Color(0xb4, 0x9c, 0xff);
    case kSrcAftertouch: return Color(0xe0, 0x8c, 0xd8);
    case kSrcKey: return Color(0xc8, 0xc0, 0xb0);
    case kSrcBend: return Color(0x9a, 0xa4, 0xb4);
    case kSrcWheel: return Color(0xd8, 0xb4, 0xff);
    default: return C_DIM;
    }
}
static const char* srcShort(int s) {
    static const char* n[kNumSources] = { "-", "LFO 1", "LFO 2", "LFO 3", "ENV 1", "ENV 2", "AMP", "VEL", "AT", "KEY", "BEND", "WHEEL" };
    return n[s < 0 || s >= kNumSources ? 0 : s];
}

// ---------------------------------------------------------------- layout
enum WType { Knob, Mini, Toggle, Segment, Stepper, SlotSrc, SlotDst, SlotByp, SlotDel };
struct Ctl { WType type; uint32_t param; float x, y, r; const char* label; };
struct Panel { float x, y, w, h; const char* title; };
struct Chip { int src; float x, y; };

static constexpr float R1 = 64, R1H = 250, R2 = 322, R2H = 200, R3 = 530, R3H = 234;
static constexpr float MODX = 16, MODW = 868, SLOT_COLW = 281, SLOT_ROWH = 23, SLOT_Y0 = R3 + 42;

static const Panel kPanels[] = {
    { 16, R1, 296, R1H, "OSC 1" }, { 320, R1, 296, R1H, "OSC 2" }, { 624, R1, 296, R1H, "" }, { 928, R1, 296, R1H, "" },
    { 16, R2, 194, R2H, "ENV 1" }, { 218, R2, 194, R2H, "ENV 2" }, { 420, R2, 194, R2H, "AMP ENV" },
    { 622, R2, 194, R2H, "LFO 1" }, { 824, R2, 194, R2H, "LFO 2" }, { 1026, R2, 198, R2H, "LFO 3" },
    { MODX, R3, MODW, R3H, "MODULATION" }, { 892, R3, 332, R3H, "VOICE" },
};

static std::vector<Ctl> buildControls() {
    std::vector<Ctl> c;
    for (int o = 0; o < 2; ++o) {
        const float x = o == 0 ? 16.f : 320.f;
        const uint32_t b = o == 0 ? kO1Wave : kO2Wave;
        c.push_back({ Segment, b, x + 148, R1 + 52, 220, "" });
        c.push_back({ Knob, b + 1, x + 40,  R1 + 122, 15, "OCT" });
        c.push_back({ Knob, b + 2, x + 100, R1 + 122, 15, "SEMI" });
        c.push_back({ Knob, b + 3, x + 160, R1 + 122, 15, "FINE" });
        c.push_back({ Knob, b + 4, x + 236, R1 + 122, 17, "PW" });
        c.push_back({ Knob, b + 5, x + 48,  R1 + 200, 20, "LEVEL" });
        c.push_back({ Knob, b + 6, x + 130, R1 + 200, 17, "PAN" });
        c.push_back({ Toggle, b + 7, x + 226, R1 + 200, 0, "PHASE SYNC" });
        if (o == 1) c.push_back({ Toggle, kOscSync, x + 254, R1 + 20, 0, "HARD SYNC" });
    }
    for (int f = 0; f < 2; ++f) {
        const float x = f == 0 ? 624.f : 928.f;
        const uint32_t b = f == 0 ? kF1On : kF2On;
        c.push_back({ Toggle, b, x + 30, R1 + 22, 0, f == 0 ? "FILTER 1" : "FILTER 2" });
        c.push_back({ Stepper, b + 1, x + 216, R1 + 22, 128, "" });
        c.push_back({ Segment, b + 2, x + 86, R1 + 56, 140, "" });
        c.push_back({ Segment, b + 3, x + 232, R1 + 56, 96, "" });
        c.push_back({ Knob, b + 4, x + 60,  R1 + 196, 26, "FREQ" });
        c.push_back({ Knob, b + 5, x + 150, R1 + 196, 20, "PEAK" });
        c.push_back({ Knob, b + 6, x + 234, R1 + 196, 18, "PAN" });
    }
    for (int e = 0; e < 2; ++e) {
        const float x = e == 0 ? 16.f : 218.f;
        const uint32_t b = e == 0 ? kE1Delay : kE2Delay;
        c.push_back({ Knob, b,     x + 34,  R2 + 104, 13, "DELAY" });
        c.push_back({ Knob, b + 1, x + 97,  R2 + 104, 13, "ATTACK" });
        c.push_back({ Knob, b + 2, x + 160, R2 + 104, 13, "DECAY" });
        c.push_back({ Knob, b + 3, x + 34,  R2 + 160, 13, "SUSTAIN" });
        c.push_back({ Knob, b + 4, x + 97,  R2 + 160, 13, "RELEASE" });
        c.push_back({ Toggle, b + 5, x + 160, R2 + 158, 0, "INVERT" });
    }
    c.push_back({ Knob, kE3Attack,  420 + 34,  R2 + 104, 13, "ATTACK" });
    c.push_back({ Knob, kE3Hold,    420 + 97,  R2 + 104, 13, "HOLD" });
    c.push_back({ Knob, kE3Decay,   420 + 160, R2 + 104, 13, "DECAY" });
    c.push_back({ Knob, kE3Sustain, 420 + 34,  R2 + 160, 13, "SUSTAIN" });
    c.push_back({ Knob, kE3Release, 420 + 97,  R2 + 160, 13, "RELEASE" });
    c.push_back({ Knob, kAmpVel,    420 + 160, R2 + 160, 13, "VELOCITY" });
    for (int l = 0; l < 3; ++l) {
        const float x = 622.f + 202.f * l;
        const uint32_t b = kL1Wave + 6 * l;
        c.push_back({ Segment, b,     x + 97, R2 + 82, 172, "" });
        c.push_back({ Segment, b + 2, x + 97, R2 + 108, 172, "" });
        c.push_back({ Knob, b + 3, x + 38, R2 + 162, 15, "RATE" });
        c.push_back({ Knob, b + 1, x + 97, R2 + 162, 13, "BALANCE" });
        c.push_back({ Toggle, b + 5, x + 158, R2 + 160, 0, "NOTE SYNC" });
    }
    // voice
    c.push_back({ Segment, kRouting, 892 + 166, R3 + 50, 300, "" });
    c.push_back({ Knob, kVoices,    892 + 40,  R3 + 116, 15, "VOICES" });
    c.push_back({ Knob, kUnison,    892 + 104, R3 + 116, 15, "UNISON" });
    c.push_back({ Knob, kUniDetune, 892 + 168, R3 + 116, 15, "DETUNE" });
    c.push_back({ Knob, kUniSpread, 892 + 232, R3 + 116, 15, "SPREAD" });
    c.push_back({ Knob, kVolume,    892 + 296, R3 + 116, 18, "VOLUME" });
    c.push_back({ Knob, kGlide,     892 + 40,  R3 + 188, 15, "GLIDE" });
    c.push_back({ Segment, kGlideMode, 892 + 132, R3 + 188, 116, "" });
    c.push_back({ Knob, kBendRange, 892 + 232, R3 + 188, 15, "BEND" });
    // modulation slots: 3 columns x 8 rows
    for (int s = 0; s < kNumSlots; ++s) {
        const float x = MODX + 12 + (s / 8) * SLOT_COLW, y = SLOT_Y0 + (s % 8) * SLOT_ROWH + SLOT_ROWH / 2;
        c.push_back({ SlotSrc, slotParam(s, kSlotSrc), x + 20, y, 64, "" });
        c.push_back({ SlotDst, slotParam(s, kSlotDst), x + 96, y, 108, "" });
        c.push_back({ Mini,    slotParam(s, kSlotAmt), x + 220, y, 8, "" });
        c.push_back({ SlotByp, slotParam(s, kSlotBypass), x + 242, y, 6, "" });
        c.push_back({ SlotDel, slotParam(s, kSlotSrc), x + 259, y, 6, "" });
    }
    return c;
}
static const std::vector<Ctl>& controls() { static std::vector<Ctl> c = buildControls(); return c; }

static std::vector<Chip> buildChips() {
    std::vector<Chip> c;
    for (int e = 0; e < 3; ++e) c.push_back({ kSrcEg1 + e, (e == 0 ? 16.f : (e == 1 ? 218.f : 420.f)) + 194 - 62, R2 + 8 });
    for (int l = 0; l < 3; ++l) c.push_back({ kSrcLfo1 + l, 622.f + 202.f * l + (l == 2 ? 198 : 194) - 62, R2 + 8 });
    const int g[5] = { kSrcVelocity, kSrcAftertouch, kSrcKey, kSrcBend, kSrcWheel };
    for (int i = 0; i < 5; ++i) c.push_back({ g[i], MODX + 150 + i * 62.f, R3 + 18 });
    return c;
}
static const std::vector<Chip>& chips() { static std::vector<Chip> c = buildChips(); return c; }
static constexpr float CHIP_W = 52, CHIP_H = 18;
static constexpr float CLR_X = MODX + 470, CLR_W = 96;

static const char* const* segLabels(uint32_t p, int& n) {
    static const char* const ow[] = { "TRI", "SAW", "SQR" };
    static const char* const type[] = { "LP", "BP", "HP" };
    static const char* const slope[] = { "12", "24" };
    static const char* const lw[] = { "TRI", "SIN", "SQR", "RND", "S&H" };
    static const char* const sync[] = { "FREE", "STR", "DOT", "TRIP" };
    static const char* const route[] = { "SERIAL", "PARALLEL", "SPLIT" };
    static const char* const gm[] = { "ALWAYS", "LEGATO" };
    switch (p) {
    case kO1Wave: case kO2Wave: n = 3; return ow;
    case kF1Type: case kF2Type: n = 3; return type;
    case kF1Slope: case kF2Slope: n = 2; return slope;
    case kL1Wave: case kL2Wave: case kL3Wave: n = 5; return lw;
    case kL1Sync: case kL2Sync: case kL3Sync: n = 4; return sync;
    case kRouting: n = 3; return route;
    case kGlideMode: n = 2; return gm;
    default: n = 0; return nullptr;
    }
}

static constexpr float PS_X = 484.f, PS_Y = 20.f, PS_W = 272.f, PS_H = 28.f, PS_ARROW = 30.f;
static constexpr float LEARN_X = 782.f, LEARN_W = 72.f, MIDI_X = 862.f, MIDI_W = 56.f;
static constexpr float PM_COLW = 146.f, PM_ROW = 23.f, PM_HEAD = 22.f, PM_PAD = 8.f;

static cplx filterH(const float* v, int f, float hz) {
    const uint32_t b = f == 0 ? kF1On : kF2On;
    const int type = (int)(v[b + 2] + .5f), stages = v[b + 3] > 0.5f ? 2 : 1;
    const float fc = v[b + 4], r = std::pow(clampf(v[b + 5], 0.f, 1.f), 0.6f);
    const StyleDef& sd = styleDef((int)(v[b + 1] + .5f));
    const float* kb = bwK(stages);
    const float w = hz / fc;
    cplx h(1, 0);
    for (int i = 0; i < stages; ++i) {
        const float k = std::max(i == stages - 1 ? kb[i] * (1 - r) + sd.kMin * r : kb[i], 0.035f);
        const cplx den(1 - w * w, k * w);
        h *= type == 0 ? cplx(1, 0) / den : (type == 2 ? cplx(-w * w, 0) / den : cplx(0, w * std::max(k, 0.12f)) / den);
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
    }

protected:
    float uiScale() const { return (float)getWidth() / W; }

    // ---- values -------------------------------------------------------------
    void setValue(uint32_t p, float v) {
        const ParamDef& d = paramDef(p);
        v = clampf(v, d.min, d.max);
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); repaint();
    }
    void commit(uint32_t p, float v) { editParameter(p, true); setValue(p, v); editParameter(p, false); }

    void parameterChanged(uint32_t i, float v) override {
        if (i >= kParamCount) return;
        if (i == kOutLearn) {
            if (v != lastLearnEcho && v > 0) {
                const int code = (int)v % 10000 - 1, prm = code / 128, cc = code % 128;
                for (int& m : paramCC) if (m == cc) m = -1;
                if (prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
                learnWaiting = -1;
            }
            lastLearnEcho = v; repaint(); return;
        }
        if (val[i] != v) { val[i] = v; repaint(); }
    }
    void stateChanged(const char* key, const char* value) override {
        if (std::strcmp(key, "midimap") != 0) return;
        for (int& m : paramCC) m = -1;
        const char* q = value;
        while (q && *q) {
            int cc = -1, prm = -1;
            if (std::sscanf(q, "%d:%d", &cc, &prm) == 2 && prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
            const char* s = std::strchr(q, ';'); if (!s) break; q = s + 1;
        }
        repaint();
    }

    uint32_t effParam(const Ctl& c) const {
        for (int l = 0; l < 3; ++l) { const uint32_t b = kL1Wave + 6 * l; if (c.param == b + 3 && val[b + 2] > 0.5f) return b + 4; }
        return c.param;
    }
    bool dimmed(uint32_t p) const {
        if (p >= kF1Style && p <= kF1Pan && val[kF1On] < 0.5f) return true;
        if (p >= kF2Style && p <= kF2Pan && val[kF2On] < 0.5f) return true;
        if ((p == kO1PW && val[kO1Wave] < 1.5f) || (p == kO2PW && val[kO2Wave] < 1.5f)) return true;
        if (p == kGlideMode && val[kGlide] < 0.0005f) return true;
        if ((p == kUniDetune || p == kUniSpread) && val[kUnison] < 1.5f) return true;
        if (p >= kSlotFirst && p <= kSlotLast) { const int s = (p - kSlotFirst) / 4; if (val[slotParam(s, kSlotBypass)] > 0.5f) return true; }
        return false;
    }
    bool slotUsed(int s) const { return val[slotParam(s, kSlotSrc)] > 0.5f || val[slotParam(s, kSlotDst)] > 0.5f; }

    // ---- formatting ---------------------------------------------------------
    void fmt(uint32_t p, char* b, size_t n) const {
        const float v = val[p];
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) { snprintf(b, n, "%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))]); return; }
        const ParamDef& d = paramDef(p);
        if (d.hints & kHintBoolean) { snprintf(b, n, "%s", v > 0.5f ? "On" : "Off"); return; }
        if (p == kF1Freq || p == kF2Freq) { if (v >= 1000) snprintf(b, n, "%.2f kHz", v / 1000); else snprintf(b, n, "%.0f Hz", v); return; }
        if (p == kO1Pan || p == kO2Pan || p == kF1Pan || p == kF2Pan) { if (std::fabs(v) < .005f) snprintf(b, n, "Center"); else snprintf(b, n, "%s %.0f%%", v < 0 ? "L" : "R", std::fabs(v) * 100); return; }
        if (p == kO1Oct || p == kO2Oct) { snprintf(b, n, "%+d oct", (int)v); return; }
        if (p == kO1Semi || p == kO2Semi || p == kBendRange) { snprintf(b, n, "%+d st", (int)v); return; }
        if (p == kO1Fine || p == kO2Fine) { snprintf(b, n, "%+.0f ct", v); return; }
        if (p == kVolume) { snprintf(b, n, "%+.1f dB", v); return; }
        if (p == kVoices || p == kUnison) { snprintf(b, n, "%d", (int)v); return; }
        if (p == kL1Rate || p == kL2Rate || p == kL3Rate) { snprintf(b, n, v < 1 ? "%.2f Hz" : "%.1f Hz", v); return; }
        if (isTimeParam(p)) { if (v < 1) snprintf(b, n, "%.0f ms", v * 1000); else snprintf(b, n, "%.2f s", v); return; }
        if (p >= kSlotFirst && p <= kSlotLast) { snprintf(b, n, "%+.1f%%", v * 100); return; }
        snprintf(b, n, "%.0f%%", v * 100);
    }
    bool parse(uint32_t p, const std::string& in, float& out) const {
        std::string s; for (char ch : in) if (!std::isspace((unsigned char)ch)) s += (char)std::tolower((unsigned char)ch);
        if (s.empty()) return false;
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) {
            for (int i = 0; i < ne; ++i) {
                std::string lab; for (const char* c = l[i]; *c; ++c) if (*c != ' ') lab += (char)std::tolower((unsigned char)*c);
                if (lab.compare(0, s.size(), s) == 0) { out = (float)i; return true; }
            }
            return false;
        }
        const bool panP = p == kO1Pan || p == kO2Pan || p == kF1Pan || p == kF2Pan;
        if (panP) {
            if (s[0] == 'c') { out = 0; return true; }
            const float sign = s[0] == 'l' ? -1.f : 1.f;
            out = sign * (float)std::atof((s[0] == 'l' || s[0] == 'r') ? s.c_str() + 1 : s.c_str()) / 100.f; return true;
        }
        char* end = nullptr; double x = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) return false;
        std::string suf(end);
        if (suf == "k" || suf == "khz") x *= 1000;
        const bool raw = p == kF1Freq || p == kF2Freq || p == kL1Rate || p == kL2Rate || p == kL3Rate || p == kVolume || p == kVoices || p == kUnison
                      || p == kO1Oct || p == kO2Oct || p == kO1Semi || p == kO2Semi || p == kO1Fine || p == kO2Fine || p == kBendRange;
        if (raw) { out = (float)x; return true; }
        if (isTimeParam(p)) { out = (float)(suf == "ms" ? x / 1000 : x); return true; }
        out = (float)(x / 100.0); return true;
    }

    // ---- drawing --------------------------------------------------------------
    void onNanoDisplay() override {
        const float s = uiScale();
        save(); scale(s, s);
        beginPath(); rect(0, 0, W, H); fillColor(C_BG); fill();
        fontFace(NANOVG_DEJAVU_SANS_TTF);

        // header: faceted shard
        beginPath(); moveTo(26, 19); lineTo(36, 27); lineTo(33, 48); lineTo(21, 44); lineTo(18, 28); closePath(); fillColor(C_ACC); fill();
        beginPath(); moveTo(26, 19); lineTo(33, 48); lineTo(21, 44); closePath(); fillColor(withAlpha(C_BG, 0.35f)); fill();
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(50, 34, "OBSIDIAN", nullptr);
        fontSize(10); textLetterSpacing(2.5f); fillColor(C_DIM); text(256, 35, "POLYPHONIC SYNTHESIZER", nullptr);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); text(W - 22, 35, "EPSILON AUDIO  ·  1.0", nullptr);
        drawPresetBar();
        drawHeaderButton(LEARN_X, LEARN_W, learnMode ? "LEARNING" : "LEARN", learnMode, hoverHeader == 1);
        drawHeaderButton(MIDI_X, MIDI_W, "MIDI", val[kMidiOn] > 0.5f, hoverHeader == 2);

        for (const Panel& p : kPanels) drawPanel(p);
        for (int f = 0; f < 2; ++f) drawFilterCurve(f, (f == 0 ? 624.f : 928.f) + 14, R1 + 76, 268, 82);
        for (int e = 0; e < 3; ++e) drawEnvShape(e, (e == 0 ? 16.f : (e == 1 ? 218.f : 420.f)) + 12, R2 + 34, 170, 38);
        for (int l = 0; l < 3; ++l) drawLfo(l, 622.f + 202.f * l + 12, R2 + 34, 170, 30);
        drawSlotBackgrounds();

        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) drawCtl(cs[i], i == active || i == hover);
        for (const Chip& c : chips()) drawChip(c.src, c.x, c.y, hoverChip == c.src, false);
        drawStatus();
        if (editing >= 0) drawEditBox();
        if (dragSrc > 0) drawChip(dragSrc, dragPX - CHIP_W / 2, dragPY - CHIP_H / 2, true, true);
        if (popup.open) drawPopup();
        if (menuOpen) drawPresetMenu();
        if (saving) drawSaveBox();
        restore();
    }

    void drawHeaderButton(float x, float w, const char* label, bool on, bool hot) {
        beginPath(); roundedRect(x, PS_Y, w, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.22f) : C_PANEL); fill();
        strokeColor(on ? C_ACC : (hot ? C_DIM : C_LINE)); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM);
        text(x + w / 2, PS_Y + PS_H / 2 + 0.5f, label, nullptr);
    }
    void drawPanel(const Panel& p) {
        beginPath(); roundedRect(p.x, p.y, p.w, p.h, 6); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        if (p.title[0]) { fontSize(9.5f); textLetterSpacing(2.2f); fillColor(C_DIM); textAlign(ALIGN_LEFT | ALIGN_TOP); text(p.x + 14, p.y + 10, p.title, nullptr); }
    }
    void displayBox(float x, float y, float w, float h) {
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
    }
    void drawFilterCurve(int f, float x, float y, float w, float h) {
        displayBox(x, y, w, h);
        const bool on = val[f == 0 ? kF1On : kF2On] > 0.5f;
        const int N = 120;
        beginPath();
        for (int i = 0; i <= N; ++i) {
            const float hz = 20.f * std::pow(1000.f, (float)i / N);
            const float db = clampf(20 * std::log10(std::max(std::abs(filterH(val, f, hz)), 1e-4f)), -30, 22);
            const float px = x + 3 + (w - 6) * i / N, py = y + h - 3 - (h - 6) * (db + 30) / 52;
            if (!i) moveTo(px, py); else lineTo(px, py);
        }
        strokeColor(withAlpha(C_ACC, on ? 0.95f : 0.25f)); strokeWidth(1.5f); stroke();
    }
    void drawEnvShape(int e, float x, float y, float w, float h) {
        displayBox(x, y, w, h);
        float dl, a, hold = 0, d, s, r;
        if (e < 2) { const uint32_t b = e == 0 ? kE1Delay : kE2Delay; dl = val[b]; a = val[b + 1]; d = val[b + 2]; s = val[b + 3]; r = val[b + 4]; }
        else { dl = 0; a = val[kE3Attack]; hold = val[kE3Hold]; d = val[kE3Decay]; s = val[kE3Sustain]; r = val[kE3Release]; }
        const float sus = 0.4f, tot = std::max(dl + a + hold + d + sus + r, 0.3f), sx = (w - 10) / tot, x0 = x + 5, yb = y + h - 4, hh = h - 8;
        const bool inv = e < 2 && val[e == 0 ? kE1Invert : kE2Invert] > 0.5f;
        auto Y = [&](float lvl) { return inv ? yb - hh + hh * lvl : yb - hh * lvl; };
        beginPath(); moveTo(x0, Y(0)); lineTo(x0 + dl * sx, Y(0)); lineTo(x0 + (dl + a) * sx, Y(1)); lineTo(x0 + (dl + a + hold) * sx, Y(1));
        for (int i = 1; i <= 16; ++i) { const float t = i / 16.f; lineTo(x0 + (dl + a + hold + d * t) * sx, Y(s + (1 - s) * std::exp(-4 * t))); }
        lineTo(x0 + (dl + a + hold + d + sus) * sx, Y(s));
        for (int i = 1; i <= 16; ++i) { const float t = i / 16.f; lineTo(x0 + (dl + a + hold + d + sus + r * t) * sx, Y(s * std::exp(-4 * t))); }
        strokeColor(srcColor(kSrcEg1 + e)); strokeWidth(1.4f); stroke();
    }
    void drawLfo(int l, float x, float y, float w, float h) {
        displayBox(x, y, w, h);
        const uint32_t b = kL1Wave + 6 * l;
        const int wave = (int)(val[b] + .5f);
        beginPath();
        for (int i = 0; i <= 160; ++i) {
            const float t = (float)i / 160, ph = std::fmod(t * 2.f, 1.f);
            const float py = y + h / 2 - Engine::lfoShape(ph, wave, val[b + 1]) * h * 0.34f;
            if (!i) moveTo(x + 4 + (w - 8) * t, py); else lineTo(x + 4 + (w - 8) * t, py);
        }
        strokeColor(srcColor(kSrcLfo1 + l)); strokeWidth(1.4f); stroke();
        char buf[24]; fmt(val[b + 2] > 0.5f ? b + 4 : b + 3, buf, sizeof buf);
        fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_DIM); text(x + w - 5, y + 3, buf, nullptr);
    }
    void drawChip(int src, float x, float y, bool hot, bool dragging) {
        const Color col = srcColor(src);
        beginPath(); roundedRect(x, y, CHIP_W, CHIP_H, CHIP_H / 2); fillColor(withAlpha(col, dragging ? 0.9f : (hot ? 0.35f : 0.16f))); fill();
        strokeColor(col); strokeWidth(1); stroke();
        beginPath(); circle(x + 9, y + CHIP_H / 2, 3); fillColor(dragging ? C_BG : col); fill();
        fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(dragging ? C_BG : C_TEXT);
        text(x + CHIP_W / 2 + 5, y + CHIP_H / 2 + 0.5f, srcShort(src), nullptr);
    }
    void drawSlotBackgrounds() {
        for (int s = 0; s < kNumSlots; ++s) {
            const float x = MODX + 12 + (s / 8) * SLOT_COLW, y = SLOT_Y0 + (s % 8) * SLOT_ROWH;
            beginPath(); roundedRect(x, y + 1, SLOT_COLW - 8, SLOT_ROWH - 2, 4);
            fillColor(selSlot == s ? withAlpha(C_ACC, 0.12f) : (slotUsed(s) ? C_PANEL_HI : Color(0x1c, 0x1e, 0x21))); fill();
            fontSize(8); textLetterSpacing(0.5f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(C_FAINT);
            char b[4]; snprintf(b, sizeof b, "%d", s + 1); text(x + 14, y + SLOT_ROWH / 2 + 0.5f, b, nullptr);
        }
    }

    // modulation summary for a knob: total |amount| of active slots and the colour of the strongest
    bool modInfo(uint32_t p, float& depth, Color& col) const {
        const int d = destForParam(p);
        if (d < 0) return false;
        depth = 0; float best = 0; int bs = 0;
        for (int s = 0; s < kNumSlots; ++s) {
            if (val[slotParam(s, kSlotBypass)] > 0.5f) continue;
            if ((int)(val[slotParam(s, kSlotDst)] + .5f) != d) continue;
            const int src = (int)(val[slotParam(s, kSlotSrc)] + .5f); if (src <= 0) continue;
            const float a = std::fabs(val[slotParam(s, kSlotAmt)]); depth += a; if (a >= best) { best = a; bs = src; }
        }
        if (depth <= 0) return false;
        col = srcColor(bs); return true;
    }

    void drawKnobBody(float x, float y, float r, float n, bool bipolar, bool hot, bool dim, uint32_t p) {
        const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f, ang = a0 + (a1 - a0) * n;
        beginPath(); arc(x, y, r, a0, a1, CW); strokeColor(C_LINE); strokeWidth(r > 20 ? 4.f : (r > 12 ? 3.f : 2.5f)); lineCap(ROUND); stroke();
        const float from = bipolar ? (a0 + a1) / 2 : a0;
        if (std::fabs(ang - from) > 0.001f) { beginPath(); arc(x, y, r, std::min(from, ang), std::max(from, ang), CW); strokeColor(dim ? C_FAINT : C_ACC); stroke(); }
        float depth; Color mc;
        if (modInfo(p, depth, mc)) {   // outer ring: this knob is modulated
            const float span = std::min(1.f, depth * 2.f) * (a1 - a0) * 0.5f;
            beginPath(); arc(x, y, r + 4, std::max(a0, ang - span), std::min(a1, ang + span), CW);
            strokeColor(withAlpha(mc, 0.85f)); strokeWidth(2.f); lineCap(ROUND); stroke();
        }
        const float cr = r - (r > 20 ? 8 : (r > 12 ? 6 : 4));
        beginPath(); circle(x, y, cr);
        fillPaint(radialGradient(x - cr * 0.3f, y - cr * 0.4f, 0, cr * 1.4f, hot ? Color(0x3a, 0x3e, 0x44) : C_PANEL_HI, Color(0x14, 0x15, 0x17)));
        fill(); strokeColor(Color(255, 255, 255, 18 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(x + std::cos(ang) * cr * 0.3f, y + std::sin(ang) * cr * 0.3f); lineTo(x + std::cos(ang) * cr * 0.9f, y + std::sin(ang) * cr * 0.9f);
        strokeColor(dim ? C_DIM : C_TEXT); strokeWidth(r > 12 ? 2.f : 1.5f); lineCap(ROUND); stroke();
    }

    void drawCtl(const Ctl& c, bool hot) {
        const uint32_t p = effParam(c);
        const bool dim = dimmed(c.param);
        char buf[48];
        const bool dropTarget = dragSrc > 0 && hot && destForParam(p) >= 0;
        switch (c.type) {
        case Knob: case Mini: {
            const bool bip = paramDef(p).min < 0 && p != kVolume;
            if (c.type == Mini && !slotUsed((p - kSlotFirst) / 4)) break;
            drawKnobBody(c.x, c.y, c.r, toNorm(p, val[p]), bip, hot, dim, p);
            if (dropTarget) { beginPath(); circle(c.x, c.y, c.r + 8); strokeColor(srcColor(dragSrc)); strokeWidth(2); stroke(); }
            if (c.type == Knob) {
                fontSize(8.5f); textLetterSpacing(1.5f); textAlign(ALIGN_CENTER | ALIGN_TOP);
                if (hot && editing < 0 && !learnMode && dragSrc <= 0) { fmt(p, buf, sizeof buf); fillColor(C_ACC); text(c.x, c.y + c.r + 6, buf, nullptr); }
                else { fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + c.r + 6, c.label, nullptr); }
            }
            break; }
        case Toggle: {
            const bool on = val[p] > 0.5f, isFilterOn = p == kF1On || p == kF2On;
            const float w = 32, h = 17, x = c.x - w / 2, y = c.y - h / 2;
            beginPath(); roundedRect(x, y, w, h, h / 2); fillColor(on && !dim ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill();
            strokeColor(on && !dim ? C_ACC : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(on ? x + w - h / 2 : x + h / 2, c.y, 5.5f); fillColor(dim ? C_FAINT : (on ? C_ACC : C_DIM)); fill();
            if (isFilterOn) { fontSize(9.5f); textLetterSpacing(2.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(c.x + w / 2 + 8, c.y + 0.5f, c.label, nullptr); }
            else if (p == kOscSync) { fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(c.x - w / 2 - 6, c.y + 0.5f, c.label, nullptr); }
            else { fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_CENTER | ALIGN_TOP); fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + 13, c.label, nullptr); }
            break; }
        case Segment: {
            int n = 0; const char* const* names = segLabels(p, n);
            const int cur = (int)(val[p] + .5f); const float sw = c.r / n, sh = 19, x0 = c.x - c.r / 2;
            for (int i = 0; i < n; ++i) {
                const float x = x0 + sw * i;
                beginPath(); roundedRect(x + 1, c.y - sh / 2, sw - 2, sh, 4); fillColor(i == cur ? (dim ? C_FAINT : C_ACC) : C_PANEL_HI); fill();
                fontSize(8.5f); textLetterSpacing(1.3f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                fillColor(i == cur ? C_BG : (dim ? C_FAINT : C_DIM)); text(x + sw / 2, c.y + 0.5f, names[i], nullptr);
            }
            break; }
        case Stepper: {
            const float x = c.x - c.r / 2, y = c.y - 10, w = c.r, h = 20;
            beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_PANEL_HI); fill(); strokeColor(hot ? C_DIM : C_LINE); strokeWidth(1); stroke();
            for (int side = 0; side < 2; ++side) {
                const float ax = side == 0 ? x + 10 : x + w - 10, d = side == 0 ? -1.f : 1.f;
                beginPath(); moveTo(ax - 2.5f * d, c.y - 4); lineTo(ax + 2.5f * d, c.y); lineTo(ax - 2.5f * d, c.y + 4); closePath(); fillColor(dim ? C_FAINT : C_DIM); fill();
            }
            fmt(p, buf, sizeof buf); std::string up(buf); for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
            fontSize(9.5f); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(dim ? C_FAINT : C_ACC); text(c.x, c.y + 0.5f, up.c_str(), nullptr);
            break; }
        case SlotSrc: case SlotDst: {
            const int s = (p - kSlotFirst) / 4;
            if (!slotUsed(s)) { if (c.type == SlotSrc) { fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(c.x, c.y + 0.5f, "empty", nullptr); } break; }
            const int v = (int)(val[p] + .5f);
            if (c.type == SlotSrc) {
                beginPath(); circle(c.x + 4, c.y, 3); fillColor(srcColor(v)); fill();
                fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot ? C_TEXT : (dim ? C_FAINT : srcColor(v)));
                text(c.x + 11, c.y + 0.5f, srcShort(v), nullptr);
                fillColor(C_FAINT); text(c.x + 58, c.y + 0.5f, ">", nullptr);
            } else {
                std::string nm = destName(v); if (nm.size() > 17) nm = nm.substr(0, 16) + ".";
                fontSize(9); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot ? C_TEXT : (dim ? C_FAINT : C_DIM));
                text(c.x - 22, c.y + 0.5f, nm.c_str(), nullptr);
            }
            break; }
        case SlotByp: {
            const int s = (p - kSlotFirst) / 4; if (!slotUsed(s)) break;
            const bool byp = val[p] > 0.5f;
            beginPath(); circle(c.x, c.y, 5); fillColor(byp ? C_PANEL : withAlpha(C_ACC, 0.8f)); fill(); strokeColor(byp ? C_FAINT : C_ACC); strokeWidth(1); stroke();
            break; }
        case SlotDel: {
            const int s = (p - kSlotFirst) / 4; if (!slotUsed(s)) break;
            beginPath(); moveTo(c.x - 3.5f, c.y - 3.5f); lineTo(c.x + 3.5f, c.y + 3.5f); moveTo(c.x + 3.5f, c.y - 3.5f); lineTo(c.x - 3.5f, c.y + 3.5f);
            strokeColor(hot ? C_TEXT : C_FAINT); strokeWidth(1.3f); stroke();
            break; }
        }
        // MIDI learn badges
        if (learnMode && (c.type == Knob || c.type == Mini || c.type == Toggle || c.type == Segment || c.type == Stepper)) {
            const int cc = paramCC[c.param]; const bool waiting = learnWaiting == (int)c.param;
            if (cc >= 0 || waiting) {
                const float by = c.type == Knob ? c.y - c.r - 10 : c.y - 15;
                snprintf(buf, sizeof buf, waiting ? "?" : "CC%d", cc);
                fontSize(8); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                const float bw = waiting ? 14 : 30;
                beginPath(); roundedRect(c.x - bw / 2, by - 6, bw, 12, 3); fillColor(waiting ? C_ACC : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fillColor(waiting ? C_BG : C_ACC); text(c.x, by + 0.5f, buf, nullptr);
            }
        }
    }

    void drawStatus() {
        std::string msg; char vb[48];
        if (statusFlash > 0) msg = statusText;
        else if (learnMode) msg = learnWaiting >= 0 ? std::string("Move a MIDI controller: ") + paramDef(learnWaiting).name : "MIDI Learn: click a control, move a controller. Right-click unassigns.";
        else if (dragSrc > 0) { const int h = hover; msg = std::string("Drop ") + sourceName(dragSrc) + (h >= 0 && destForParam(effParam(controls()[h])) >= 0 ? std::string(" onto ") + destName(destForParam(effParam(controls()[h]))) : " onto a knob"); }
        else if (hover >= 0) {
            const Ctl& c = controls()[hover]; const uint32_t p = effParam(c);
            if (c.type == SlotDel) msg = "Remove this modulation";
            else if (c.type == SlotByp) msg = "Bypass this modulation";
            else { fmt(p, vb, sizeof vb); msg = std::string(paramDef(p).name) + "   " + vb; }
        } else msg = "Drag a source onto any knob to modulate it";
        fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(learnMode || statusFlash > 0 ? C_ACC : C_DIM);
        text(MODX + MODW - 14, R3 + 27, msg.c_str(), nullptr);
        if (learnMode && learnWaiting < 0) {
            beginPath(); roundedRect(CLR_X, R3 + 9, CLR_W, CHIP_H, 4); fillColor(hoverClear ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
            fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(CLR_X + CLR_W / 2, R3 + 9 + CHIP_H / 2 + 0.5f, "CLEAR ALL CCs", nullptr);
        }
        if (statusFlash > 0) --statusFlash;
    }
    void flash(const std::string& s) { statusText = s; statusFlash = 120; repaint(); }

    void drawEditBox() {
        const Ctl& c = controls()[editing];
        const float w = 84, h = 20, x = c.x - w / 2, y = c.y + c.r + 3;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(11); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(c.x, y + h / 2 + 0.5f, (editBuf + "|").c_str(), nullptr);
    }

    // ---- slot pickers (source / destination lists) ---------------------------
    struct Popup { bool open = false; uint32_t param = 0; float x = 0, y = 0; int hover = -1; } popup;
    int popupCount() const { return (popup.param - kSlotFirst) % 4 == kSlotSrc ? kNumSources : kNumDests; }
    const char* popupLabel(int i) const { return (popup.param - kSlotFirst) % 4 == kSlotSrc ? sourceName(i) : destName(i); }
    void popupLayout(float& x, float& y, float& w, float& h, int& rows, float& colw) const {
        const int n = popupCount(); rows = std::min(n, 16); const int cols = (n + rows - 1) / rows; colw = 118;
        w = cols * colw + 12; h = rows * 19 + 12;
        x = std::min(W - w - 8, std::max(8.f, popup.x)); y = std::min(H - h - 8, std::max(8.f, popup.y));
    }
    void drawPopup() {
        float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw);
        beginPath(); rect(x - 10, y - 4, w + 20, h + 18); fillPaint(boxGradient(x, y + 2, w, h, 6, 12, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        const int cur = (int)(val[popup.param] + .5f);
        const bool isSrc = (popup.param - kSlotFirst) % 4 == kSlotSrc;
        for (int i = 0; i < popupCount(); ++i) {
            const float ix = x + 6 + (i / rows) * colw, iy = y + 6 + (i % rows) * 19;
            if (i == popup.hover) { beginPath(); roundedRect(ix, iy, colw - 4, 19, 3); fillColor(withAlpha(C_ACC, 0.25f)); fill(); }
            if (isSrc && i > 0) { beginPath(); circle(ix + 7, iy + 9.5f, 3); fillColor(srcColor(i)); fill(); }
            fontSize(9.5f); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(i == cur ? C_ACC : (i == popup.hover ? C_TEXT : C_DIM));
            text(ix + (isSrc ? 15 : 6), iy + 10, popupLabel(i), nullptr);
        }
    }
    int popupHit(float px, float py) const {
        float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw);
        if (px < x + 6 || py < y + 6 || px > x + w - 6 || py > y + h - 6) return -1;
        const int i = (int)((px - x - 6) / colw) * rows + (int)((py - y - 6) / 19);
        return i < popupCount() ? i : -1;
    }

    // ---- presets ------------------------------------------------------------------
    bool matchesValues(const float* v) const {
        for (uint32_t i = 0; i < kParamCount; ++i) {
            if (paramDef(i).hints & kHintOutput) continue;
            if (std::fabs(val[i] - v[i]) > 1e-4f * std::max(1.f, std::fabs(v[i]))) return false;
        }
        return true;
    }
    void applyValues(const float* v) { for (uint32_t i = 0; i < kParamCount; ++i) if (!(paramDef(i).hints & kHintOutput) && val[i] != v[i]) commit(i, v[i]); }
    void applyFactory(int idx) { idx = (idx % (int)kNumPresets + (int)kNumPresets) % (int)kNumPresets; applyValues(presets()[idx].v); curFactory = idx; curUser.clear(); repaint(); }
    void applyUser(const std::string& name) { float v[kParamCount]; if (!loadUserPreset(name, v)) return; applyValues(v); std::copy(v, v + kParamCount, userVals); curUser = name; curFactory = -1; repaint(); }
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
        fontSize(12); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(presetHover == 1 || menuOpen ? C_TEXT : Color(0xd0, 0xcc, 0xc4));
        text(x + w / 2 - 6, y + h / 2 + 0.5f, name.c_str(), nullptr);
        const float cx = x + w - a - 14, cy = y + h / 2;
        beginPath(); moveTo(cx - 4, cy - 2); lineTo(cx + 4, cy - 2); lineTo(cx, cy + 3); closePath(); fillColor(C_DIM); fill();
    }
    struct MRect { float x, y, w, h; int kind; int idx; };
    void menuLayout(float& mx, float& my, float& mw, float& mh, std::vector<MRect>& items) const {
        const int cols = kNumCategories + 1;
        mw = PM_COLW * cols + 2 * PM_PAD; mx = std::min(W - mw - 8, std::max(8.f, PS_X + PS_W / 2 - mw / 2)); my = PS_Y + PS_H + 4;
        items.clear();
        const float top = my + PM_PAD + PM_ROW + 6 + PM_HEAD;
        int rows[kNumCategories + 1] = {}; int maxRows = 1;
        for (uint32_t i = 0; i < kNumPresets; ++i) {
            const char* cat = presets()[i].category;
            if (std::strcmp(cat, "Init") == 0) { items.push_back({ mx + PM_PAD, my + PM_PAD, PM_COLW - 4, PM_ROW, 0, (int)i }); continue; }
            for (int c = 0; c < kNumCategories; ++c)
                if (std::strcmp(cat, kCategories[c]) == 0) { items.push_back({ mx + PM_PAD + c * PM_COLW, top + rows[c] * PM_ROW, PM_COLW - 4, PM_ROW, 0, (int)i }); maxRows = std::max(maxRows, ++rows[c]); }
        }
        for (int u = 0; u < (int)userPresets.size() && u < 16; ++u) items.push_back({ mx + PM_PAD + kNumCategories * PM_COLW, top + u * PM_ROW, PM_COLW - 4, PM_ROW, 1, u });
        maxRows = std::max(maxRows, std::min(16, (int)userPresets.size()));
        items.push_back({ mx + mw - PM_PAD - 150, my + PM_PAD, 150, PM_ROW, 2, 0 });
        mh = PM_PAD + PM_ROW + 6 + PM_HEAD + maxRows * PM_ROW + PM_PAD;
    }
    void drawPresetMenu() {
        float mx, my, mw, mh; std::vector<MRect> items; menuLayout(mx, my, mw, mh, items);
        beginPath(); rect(mx - 12, my - 4, mw + 24, mh + 20); fillPaint(boxGradient(mx, my + 2, mw, mh, 6, 14, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(mx, my, mw, mh, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(mx + PM_PAD, my + PM_PAD + PM_ROW + 3); lineTo(mx + mw - PM_PAD, my + PM_PAD + PM_ROW + 3); strokeColor(C_LINE); stroke();
        fontSize(8.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        for (int c = 0; c <= kNumCategories; ++c) {
            std::string up = c < kNumCategories ? kCategories[c] : "User"; for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
            fillColor(c < kNumCategories ? C_ACC : srcColor(kSrcLfo1)); text(mx + PM_PAD + c * PM_COLW + 12, my + PM_PAD + PM_ROW + 6 + PM_HEAD / 2, up.c_str(), nullptr);
        }
        if (userPresets.empty()) { fontSize(10); textLetterSpacing(0.4f); fillColor(C_FAINT); text(mx + PM_PAD + kNumCategories * PM_COLW + 12, my + PM_PAD + PM_ROW + 6 + PM_HEAD + PM_ROW / 2, "(none saved yet)", nullptr); }
        for (int i = 0; i < (int)items.size(); ++i) {
            const MRect& r = items[i]; const bool hot = i == menuHover;
            if (r.kind == 2) {
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(hot ? withAlpha(C_ACC, 0.25f) : C_PANEL); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fontSize(9.5f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(r.x + r.w / 2, r.y + r.h / 2 + 0.5f, "SAVE CURRENT AS...", nullptr);
                continue;
            }
            const bool isCur = r.kind == 0 ? (curUser.empty() && r.idx == curFactory) : (curUser == userPresets[r.idx]);
            const bool del = r.kind == 1 && pendingDelete == r.idx;
            if (hot || del) { beginPath(); roundedRect(r.x, r.y, r.w, r.h, 4); fillColor(del ? Color(0xd0, 0x50, 0x40, 0.35f) : withAlpha(C_ACC, 0.2f)); fill(); }
            if (isCur) { beginPath(); circle(r.x + 6, r.y + r.h / 2, 2.5f); fillColor(C_ACC); fill(); }
            fontSize(11); textLetterSpacing(0.5f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot || isCur || del ? C_TEXT : C_DIM);
            const std::string label = del ? "Delete? click again" : (r.kind == 0 ? std::string(presets()[r.idx].name) : userPresets[r.idx]);
            text(r.x + 12, r.y + r.h / 2 + 0.5f, label.c_str(), nullptr);
        }
    }
    int menuHit(float x, float y) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it);
        for (int i = 0; i < (int)it.size(); ++i) if (x >= it[i].x && x < it[i].x + it[i].w && y >= it[i].y && y < it[i].y + it[i].h) return i; return -1; }
    MRect menuItem(int i) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it); return it[i]; }
    void drawSaveBox() {
        const float w = 360, h = 96, x = W / 2 - w / 2, y = 140;
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.45f)); fill();
        beginPath(); roundedRect(x, y, w, h, 8); fillColor(C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(x + 16, y + 14, "SAVE PRESET AS", nullptr);
        beginPath(); roundedRect(x + 16, y + 36, w - 32, 26, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); stroke();
        fontSize(13); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(x + 26, y + 49.5f, (saveName + "|").c_str(), nullptr);
        fontSize(8.5f); textLetterSpacing(1.2f); fillColor(C_FAINT); textAlign(ALIGN_RIGHT | ALIGN_TOP);
        text(x + w - 16, y + 72, saveError ? "COULD NOT SAVE - CHECK THE NAME" : "ENTER TO SAVE   ·   ESC TO CANCEL", nullptr);
    }

    // ---- interaction --------------------------------------------------------------
    int hit(float x, float y, int* seg = nullptr) const {
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) {
            const Ctl& c = cs[i];
            switch (c.type) {
            case Knob: case Mini: {
                if (c.type == Mini && !slotUsed((c.param - kSlotFirst) / 4)) break;
                const float rr = c.r + (c.type == Mini ? 4 : 6); if ((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y) <= rr * rr) return i; break; }
            case Toggle: if (std::fabs(x - c.x) <= 19 && std::fabs(y - c.y) <= 11) return i; break;
            case Segment: { int n = 0; segLabels(c.param, n); const float x0 = c.x - c.r / 2;
                if (x >= x0 && x < x0 + c.r && std::fabs(y - c.y) <= 10) { if (seg) *seg = (int)((x - x0) / (c.r / n)); return i; } break; }
            case Stepper: if (std::fabs(x - c.x) <= c.r / 2 && std::fabs(y - c.y) <= 11) { if (seg) *seg = x < c.x ? -1 : 1; return i; } break;
            case SlotSrc: if (x >= c.x - 4 && x < c.x + c.r && std::fabs(y - c.y) <= 10) return i; break;
            case SlotDst: { const int s = (c.param - kSlotFirst) / 4; if (!slotUsed(s)) break;
                if (x >= c.x - 24 && x < c.x + c.r - 22 && std::fabs(y - c.y) <= 10) return i; break; }
            case SlotByp: case SlotDel: { const int s = (c.param - kSlotFirst) / 4; if (!slotUsed(s)) break;
                if (std::fabs(x - c.x) <= 8 && std::fabs(y - c.y) <= 9) return i; break; }
            }
        }
        return -1;
    }
    bool clearHit(float x, float y) const { return learnMode && learnWaiting < 0 && x >= CLR_X && x < CLR_X + CLR_W && y >= R3 + 9 && y < R3 + 9 + CHIP_H; }
    int chipHit(float x, float y) const { for (const Chip& c : chips()) if (x >= c.x && x < c.x + CHIP_W && y >= c.y && y < c.y + CHIP_H) return c.src; return -1; }
    int headerHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H) return 0;
        if (x >= LEARN_X && x <= LEARN_X + LEARN_W) return 1;
        if (x >= MIDI_X && x <= MIDI_X + MIDI_W) return 2;
        return 0;
    }
    int presetBarHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H || x < PS_X || x > PS_X + PS_W) return -1;
        return x < PS_X + PS_ARROW ? 0 : (x > PS_X + PS_W - PS_ARROW ? 2 : 1);
    }

    void assign(int src, int dst) {
        int slot = -1;
        for (int s = 0; s < kNumSlots; ++s) if (!slotUsed(s)) { slot = s; break; }
        if (slot < 0) { flash("All 24 modulation slots are in use - remove one first"); return; }
        const float amt = (src == kSrcKey && (dst == kDstF1Freq || dst == kDstF2Freq)) ? 0.2f
                        : ((dst == kDstO1Pitch || dst == kDstO2Pitch) ? 0.02f : 0.3f);
        commit(slotParam(slot, kSlotSrc), (float)src); commit(slotParam(slot, kSlotDst), (float)dst);
        commit(slotParam(slot, kSlotAmt), amt); commit(slotParam(slot, kSlotBypass), 0);
        selSlot = slot;
        flash(std::string("Slot ") + std::to_string(slot + 1) + ": " + sourceName(src) + " > " + destName(dst));
    }
    void clearSlot(int s) {
        commit(slotParam(s, kSlotSrc), 0); commit(slotParam(s, kSlotDst), 0); commit(slotParam(s, kSlotAmt), 0); commit(slotParam(s, kSlotBypass), 0);
    }
    void setLearn(bool on) { learnMode = on; learnWaiting = -1; if (!on) setState("cmd", "cancel"); repaint(); }
    void endEdit(bool apply) {
        if (editing < 0) return;
        const uint32_t p = effParam(controls()[editing]); float v;
        if (apply && parse(p, editBuf, v)) commit(p, v);
        editing = -1; editBuf.clear(); repaint();
    }

    bool onKeyboard(const KeyboardEvent& ev) override {
        if (!ev.press) return false;
        std::string* buf = saving ? &saveName : (editing >= 0 ? &editBuf : nullptr);
        if (!buf) {
            if (ev.key == kKeyEscape && (learnMode || popup.open)) { popup.open = false; setLearn(false); return true; }
            return false;
        }
        if (ev.key == kKeyEnter) {
            if (saving) {
                if (saveUserPreset(saveName, val)) { userPresets = listUserPresets(); curUser = sanitizeName(saveName); curFactory = -1; std::copy(val, val + kParamCount, userVals); saving = false; saveError = false; }
                else saveError = true;
            } else endEdit(true);
            repaint(); return true;
        }
        if (ev.key == kKeyEscape) { if (saving) saving = false; else endEdit(false); repaint(); return true; }
        if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!buf->empty()) buf->pop_back(); repaint(); return true; }
        if (ev.key >= 32 && ev.key < 127 && buf->size() < (saving ? 40u : 12u)) { *buf += (char)ev.key; repaint(); return true; }
        return true;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (ev.press) statusFlash = 0;
        if (ev.button == 2 && ev.press) {
            if (menuOpen) { const int m = menuHit(x, y); if (m >= 0 && menuItem(m).kind == 1) { pendingDelete = menuItem(m).idx; repaint(); } return true; }
            const int i = hit(x, y);
            if (learnMode && i >= 0) { const uint32_t p = controls()[i].param; char b[24]; snprintf(b, sizeof b, "unmap:%u", p); setState("cmd", b); paramCC[p] = -1; repaint(); return true; }
            return false;
        }
        if (ev.button != 1) return false;
        if (!ev.press) {
            if (dragSrc > 0) {                     // drop a modulation source
                const int i = hit(x, y);
                if (i >= 0) { const int d = destForParam(effParam(controls()[i])); if (d >= 0) assign(dragSrc, d); }
                dragSrc = -1; repaint(); return true;
            }
            if (active >= 0) { editParameter(effParam(controls()[active]), false); active = -1; repaint(); return true; }
            return false;
        }
        if (editing >= 0) endEdit(true);
        if (popup.open) {
            const int it = popupHit(x, y);
            if (it >= 0) commit(popup.param, (float)it);
            popup.open = false; repaint(); return true;
        }
        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m >= 0) {
                const MRect r = menuItem(m);
                if (r.kind == 2) { menuOpen = false; saving = true; saveError = false; saveName = curUser; }
                else if (r.kind == 1 && pendingDelete == r.idx) { deleteUserPreset(userPresets[r.idx]); if (curUser == userPresets[r.idx]) curUser.clear(); userPresets = listUserPresets(); pendingDelete = -1; repaint(); return true; }
                else { menuOpen = false; if (r.kind == 0) applyFactory(r.idx); else applyUser(userPresets[r.idx]); }
            } else menuOpen = false;
            pendingDelete = -1; menuHover = -1; repaint(); return true;
        }
        const int hh = headerHit(x, y);
        if (hh == 1) { setLearn(!learnMode); return true; }
        if (hh == 2) { commit(kMidiOn, val[kMidiOn] > 0.5f ? 0.f : 1.f); return true; }
        const int pb = presetBarHit(x, y);
        if (pb == 0) { step(-1); return true; }
        if (pb == 2) { step(1); return true; }
        if (pb == 1) { userPresets = listUserPresets(); menuOpen = true; menuHover = -1; pendingDelete = -1; repaint(); return true; }
        if (!learnMode) { const int ch = chipHit(x, y); if (ch > 0) { dragSrc = ch; dragPX = x; dragPY = y; repaint(); return true; } }
        if (learnMode && clearHit(x, y)) { setState("cmd", "clear"); for (int& m : paramCC) m = -1; repaint(); return true; }

        int seg = 0; const int i = hit(x, y, &seg);
        if (i < 0) { selSlot = -1; return false; }
        const Ctl& c = controls()[i];
        const uint32_t p = effParam(c);
        if (learnMode) { learnWaiting = (int)c.param; char b[24]; snprintf(b, sizeof b, "learn:%u", c.param); setState("cmd", b); repaint(); return true; }
        switch (c.type) {
        case Toggle: commit(p, val[p] > 0.5f ? 0.f : 1.f); return true;
        case Segment: commit(p, (float)seg); return true;
        case Stepper: commit(p, (float)(((int)(val[p] + .5f) + seg + kNumStyles) % kNumStyles)); return true;
        case SlotSrc: case SlotDst: popup.open = true; popup.param = c.type == SlotSrc ? p : p; popup.x = c.x - 20; popup.y = c.y + 12; popup.hover = -1;
            if (c.type == SlotSrc && !slotUsed((p - kSlotFirst) / 4)) popup.param = p;
            selSlot = (int)(p - kSlotFirst) / 4; repaint(); return true;
        case SlotByp: commit(p, val[p] > 0.5f ? 0.f : 1.f); return true;
        case SlotDel: clearSlot((int)(p - kSlotFirst) / 4); return true;
        default: break;
        }
        if (ev.mod & (kModifierControl | kModifierSuper)) { commit(p, defv[p]); return true; }
        if (i == lastClick && ev.time - lastClickTime < 350) {
            lastClick = -1; editing = i;
            char b[48]; fmt(p, b, sizeof b); editBuf = b;
            const bool panP = p == kO1Pan || p == kO2Pan || p == kF1Pan || p == kF2Pan;
            const size_t sp = editBuf.find(' '); if (sp != std::string::npos && !panP) editBuf.resize(sp);
            if (!editBuf.empty() && editBuf.back() == '%') editBuf.pop_back();
            if ((p == kF1Freq || p == kF2Freq) && val[p] >= 1000) { char kb[16]; snprintf(kb, sizeof kb, "%.2fk", val[p] / 1000); editBuf = kb; }
            if (panP) { char pb2[16]; snprintf(pb2, sizeof pb2, "%s%.0f", val[p] < 0 ? "L" : "R", std::fabs(val[p]) * 100); editBuf = std::fabs(val[p]) < .005f ? "C" : pb2; }
            if (isTimeParam(p) && val[p] < 1) { char tb[16]; snprintf(tb, sizeof tb, "%.0fms", val[p] * 1000); editBuf = tb; }
            repaint(); return true;
        }
        lastClick = i; lastClickTime = ev.time;
        if (c.type == Mini) selSlot = (int)(p - kSlotFirst) / 4;
        active = i; dragY = y; dragNorm = toNorm(p, val[p]);
        editParameter(p, true); repaint();
        return true;
    }

    bool onMotion(const MotionEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (dragSrc > 0) { dragPX = x; dragPY = y; const int h = hit(x, y); hover = h; repaint(); return true; }
        if (active >= 0) {
            const uint32_t p = effParam(controls()[active]);
            float sens = (ev.mod & kModifierShift) ? 1500.f : 220.f;
            if (paramDef(p).hints & kHintInteger) sens = std::max(60.f, 14.f * (paramDef(p).max - paramDef(p).min));
            if (controls()[active].type == Mini) sens *= (ev.mod & kModifierShift) ? 1.f : 2.f;     // slot amounts: finer by default
            dragNorm = clampf(dragNorm + (dragY - y) / sens, 0.f, 1.f); dragY = y;
            setValue(p, fromNorm(p, dragNorm));
            return true;
        }
        if (popup.open) { const int h = popupHit(x, y); if (h != popup.hover) { popup.hover = h; repaint(); } return true; }
        if (menuOpen) { const int m = menuHit(x, y); if (m != menuHover) { menuHover = m; repaint(); } return true; }
        const int hh = headerHit(x, y); if (hh != hoverHeader) { hoverHeader = hh; repaint(); }
        const int pb = presetBarHit(x, y); if (pb != presetHover) { presetHover = pb; repaint(); }
        const int ch = chipHit(x, y); if (ch != hoverChip) { hoverChip = ch; repaint(); }
        const bool hc = clearHit(x, y); if (hc != hoverClear) { hoverClear = hc; repaint(); }
        const int h = hit(x, y); if (h != hover) { hover = h; repaint(); }
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const float s = uiScale(), sx = ev.pos.getX() / s, sy = ev.pos.getY() / s;
        if (menuOpen || saving || popup.open) return true;
        if (presetBarHit(sx, sy) >= 0) { step(ev.delta.getY() > 0 ? -1 : 1); return true; }
        const int i = hit(sx, sy); if (i < 0) return false;
        const Ctl& c = controls()[i]; const uint32_t p = effParam(c);
        if (c.type == Stepper) { commit(p, (float)(((int)(val[p] + .5f) + (ev.delta.getY() > 0 ? 1 : -1) + kNumStyles) % kNumStyles)); return true; }
        if (c.type != Knob && c.type != Mini) return false;
        float st = (ev.mod & kModifierShift) ? 0.004f : (c.type == Mini ? 0.01f : 0.025f);
        if (paramDef(p).hints & kHintInteger) st = 1.f / (paramDef(p).max - paramDef(p).min);
        commit(p, fromNorm(p, toNorm(p, val[p]) + (ev.delta.getY() > 0 ? st : -st)));
        return true;
    }

private:
    float val[kParamCount], defv[kParamCount], userVals[kParamCount] = {};
    int paramCC[kParamCount];
    int active = -1, hover = -1, lastClick = -1, editing = -1, selSlot = -1;
    uint lastClickTime = 0;
    float dragY = 0, dragNorm = 0;
    int dragSrc = -1, hoverChip = -1; float dragPX = 0, dragPY = 0;
    std::string editBuf, statusText; int statusFlash = 0;
    int curFactory = 0, presetHover = -1, menuHover = -1, pendingDelete = -1, hoverHeader = 0;
    std::string curUser; std::vector<std::string> userPresets;
    bool menuOpen = false, saving = false, saveError = false; std::string saveName;
    bool learnMode = false, hoverClear = false; int learnWaiting = -1; float lastLearnEcho = 0;
    ResizeHandle resizeHandle;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObsidianUI)
};

UI* createUI() { return new ObsidianUI(); }

END_NAMESPACE_DISTRHO
