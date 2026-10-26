// Obsidian 3.0 — the effects section: reverb (8-line feedback delay network), stereo delay with
// feedback filters, chorus (single or double), phaser / flanger, drive and compressor.
// Clean-room implementations of generic, textbook techniques. License: MIT.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace obsidian {
namespace fx {

static constexpr float kPiF = 3.14159265358979f;
inline float fclamp(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float onePoleCoef(float hz, float fs) { return 1.f - std::exp(-2.f * kPiF * hz / fs); }

// a power-of-two delay line with linear-interpolated reads
struct Line {
    std::vector<float> buf; uint32_t mask = 0, w = 0;
    void alloc(uint32_t n) { uint32_t s = 16; while (s < n + 4) s <<= 1; buf.assign(s, 0.f); mask = s - 1; w = 0; }
    void clear() { std::fill(buf.begin(), buf.end(), 0.f); }
    inline void write(float x) { buf[w] = x; w = (w + 1) & mask; }
    inline float read(float d) const {          // d >= 1 samples behind the last write
        const float rp = (float)w - d; const float fl = std::floor(rp); const float fr = rp - fl;
        const uint32_t i0 = (uint32_t)(int32_t)fl & mask, i1 = (i0 + 1) & mask;
        return buf[i0] + (buf[i1] - buf[i0]) * fr;
    }
    inline float readInt(uint32_t d) const { return buf[(w - d) & mask]; }
    float maxDelay() const { return (float)(buf.size() - 4); }
};

// ------------------------------------------------------------------ reverb
struct Reverb {
    static constexpr int N = 8;
    Line line[N], pre[2]; float len[N] = {}, lp[N] = {}, ap1[2][2] = {};
    Line apL[2][2];
    float fs = 48000;
    void init(float sr) {
        fs = sr;
        static const float ms[N] = { 29.7f, 37.1f, 41.1f, 43.7f, 53.1f, 59.3f, 67.9f, 73.3f };
        for (int i = 0; i < N; ++i) { len[i] = std::floor(ms[i] * 0.001f * sr); line[i].alloc((uint32_t)(len[i] * 1.1f) + 8); }
        lastT = lastB = -1;
        for (int c = 0; c < 2; ++c) { pre[c].alloc((uint32_t)(0.26f * sr)); apL[c][0].alloc((uint32_t)(0.006f * sr)); apL[c][1].alloc((uint32_t)(0.009f * sr)); }
        clear();
    }
    void clear() { for (Line& l : line) l.clear(); for (Line& l : pre) l.clear(); for (auto& a : apL) for (Line& l : a) l.clear(); std::fill(lp, lp + N, 0.f); }
    float g[N] = {}, a = 0.5f, pd = 1, lastT = -1, lastB = -1;
    // control rate: time = decay (RT60) in seconds, bright 0..1, predelay in seconds
    void prepare(float time, float bright, float predelay) {
        if (time != lastT) { lastT = time; for (int i = 0; i < N; ++i) g[i] = std::pow(10.f, -3.f * len[i] / (std::max(time, 0.05f) * fs)); }
        if (bright != lastB) { lastB = bright; a = onePoleCoef(1500.f * std::pow(12.f, fclamp(bright, 0.f, 1.f)), fs); }
        pd = std::max(1.f, predelay * fs);
    }
    inline void process(float inL, float inR, float& outL, float& outR) {
        pre[0].write(inL); pre[1].write(inR);
        float x[2] = { pre[0].read(pd), pre[1].read(pd) };
        // input diffusion: two all-passes per channel
        static const float apMs[2] = { 0.0047f, 0.0073f };
        for (int c = 0; c < 2; ++c) for (int k = 0; k < 2; ++k) {
            const float d = apMs[k] * fs * (c ? 1.07f : 1.f), gg = 0.6f;
            const float del = apL[c][k].read(d), v = x[c] + gg * del;
            apL[c][k].write(v); x[c] = del - gg * v;
        }
        float y[N], sum = 0;
        for (int i = 0; i < N; ++i) { const float v = line[i].readInt((uint32_t)len[i]) * g[i]; lp[i] += (v - lp[i]) * a; y[i] = lp[i]; sum += y[i]; }
        const float h = 2.f / N * sum;
        for (int i = 0; i < N; ++i) line[i].write(y[i] - h + (i & 1 ? x[1] : x[0]) * 0.5f);
        outL = (y[0] + y[2] + y[4] + y[6]) * 0.45f; outR = (y[1] + y[3] + y[5] + y[7]) * 0.45f;
    }
};

// ------------------------------------------------------------------ stereo delay
struct Delay {
    Line l[2]; float fs = 48000, sT[2] = { 0, 0 }, hp[2] = {}, lpS[2] = {}, fbL = 0, fbR = 0;
    void init(float sr) { fs = sr; for (Line& x : l) x.alloc((uint32_t)(4.1f * sr)); clear(); }
    void clear() { for (Line& x : l) x.clear(); hp[0] = hp[1] = lpS[0] = lpS[1] = 0; fbL = fbR = 0; }
    float tT[2] = { 0.3f, 0.3f }, g = 0, ah = 0.01f, al = 0.5f, sm = 0.001f; bool pp = false;
    // control rate: times in seconds; fb 0..1; lowCut/highCut in Hz
    void prepare(float tL, float tR, float fb, bool pingPong, float lowCut, float highCut) {
        tT[0] = tL; tT[1] = tR; g = fclamp(fb, 0.f, 0.98f); pp = pingPong; ah = onePoleCoef(lowCut, fs); al = onePoleCoef(highCut, fs);
        sm = 1.f - std::exp(-1.f / (0.08f * fs));
        if (sT[0] <= 0) { sT[0] = tL; sT[1] = tR; }
    }
    inline void process(float inL, float inR, float& outL, float& outR) {
        sT[0] += (tT[0] - sT[0]) * sm; sT[1] += (tT[1] - sT[1]) * sm;
        float y[2];
        for (int c = 0; c < 2; ++c) y[c] = l[c].read(fclamp(sT[c] * fs, 2.f, l[c].maxDelay()));
        for (int c = 0; c < 2; ++c) { hp[c] += (y[c] - hp[c]) * ah; const float v = y[c] - hp[c]; lpS[c] += (v - lpS[c]) * al; y[c] = lpS[c]; }
        if (pp) { l[0].write(0.5f * (inL + inR) + g * y[1]); l[1].write(g * y[0]); }
        else { l[0].write(inL + g * y[0]); l[1].write(inR + g * y[1]); }
        outL = y[0]; outR = y[1];
    }
};

// ------------------------------------------------------------------ chorus (one or two modulated taps per channel)
struct Chorus {
    Line l[2]; double ph = 0; float fs = 48000;
    void init(float sr) { fs = sr; for (Line& x : l) x.alloc((uint32_t)(0.08f * sr)); clear(); }
    void clear() { for (Line& x : l) x.clear(); }
    // rate Hz, depth 0..1, delay seconds (base), dbl: two taps
    inline void process(float inL, float inR, float rate, float depth, float delay, bool dbl, float& outL, float& outR) {
        l[0].write(inL); l[1].write(inR);
        ph += rate / fs; if (ph >= 1) ph -= 1;
        const float base = delay * fs, exc = fclamp(depth, 0.f, 1.f) * std::min(delay, 0.015f) * 0.9f * fs;
        const float p = 2.f * kPiF * (float)ph;
        outL = l[0].read(std::max(2.f, base + exc * std::sin(p)));
        outR = l[1].read(std::max(2.f, base + exc * std::sin(p + 0.5f * kPiF)));
        if (dbl) {
            outL = 0.7071f * (outL + l[0].read(std::max(2.f, 1.5f * base + exc * std::sin(p + kPiF))));
            outR = 0.7071f * (outR + l[1].read(std::max(2.f, 1.5f * base + exc * std::sin(p + 1.5f * kPiF))));
        }
    }
};

// ------------------------------------------------------------------ phaser (6 all-pass stages) / flanger
struct Phaser {
    float z[2][6] = {}, fbS[2] = {}; double ph = 0; float fs = 48000; Line fl[2];
    void init(float sr) { fs = sr; for (Line& x : fl) x.alloc((uint32_t)(0.02f * sr)); clear(); }
    void clear() { std::memset(z, 0, sizeof z); fbS[0] = fbS[1] = 0; for (Line& x : fl) x.clear(); }
    float a[2] = { 0, 0 }; int tick = 0;
    // flanger: a short swept delay instead of the all-passes. shift 0..1: left/right LFO phase offset (up to half a cycle)
    inline void process(float inL, float inR, bool flanger, float rate, float depth, float fb, float shift, float& outL, float& outR) {
        ph += rate / fs; if (ph >= 1) ph -= 1;
        const float in[2] = { inL, inR }; float out[2];
        const float g = fclamp(fb, 0.f, 0.92f), d = fclamp(depth, 0.f, 1.f);
        const bool upd = tick-- <= 0; if (upd) tick = 3;
        for (int c = 0; c < 2; ++c) {
            if (flanger) {
                const float lfo = 0.5f + 0.5f * std::sin(2.f * kPiF * (float)ph + (c ? kPiF * fclamp(shift, 0.f, 1.f) : 0.f));
                const float dl = (0.0003f + 0.0065f * d * lfo) * fs;
                const float y = fl[c].read(std::max(1.5f, dl));
                fl[c].write(in[c] + g * y);
                out[c] = y;
            } else {
                if (upd) {
                    const float lfo = 0.5f + 0.5f * std::sin(2.f * kPiF * (float)ph + (c ? kPiF * fclamp(shift, 0.f, 1.f) : 0.f));
                    const float hz = 200.f * std::exp2(4.3f * d * lfo + 0.5f * (1.f - d));
                    const float t = std::tan(kPiF * fclamp(hz, 20.f, 0.4f * fs) / fs); a[c] = (t - 1.f) / (t + 1.f);
                }
                float x = in[c] + g * fbS[c];
                for (int k = 0; k < 6; ++k) { const float y = a[c] * x + z[c][k]; z[c][k] = x - a[c] * y; x = y; }
                fbS[c] = x; out[c] = x;
            }
        }
        outL = out[0]; outR = out[1];
    }
};

// ------------------------------------------------------------------ drive: subtle saturation to amp-style distortion
inline float drive(float x, float amt) {
    if (amt <= 0.f) return x;
    const float g = 1.f + 30.f * amt * amt;
    const float a = std::tanh(g * x + 0.15f * amt) - std::tanh(0.15f * amt);      // a touch of asymmetry
    return a / std::sqrt(g);
}

// ------------------------------------------------------------------ compressor (stereo-linked, soft knee)
struct Compressor {
    float env = 0, gr = 0, fs = 48000; float meterIn = 0, meterGr = 0;
    float cA = 0.1f, cR = 0.001f, thr = -18, sl = 0.5f, makeup = 1, gain = 1, gainT = 1; int tick = 0;
    void init(float sr) { fs = sr; env = gr = 0; gain = gainT = 1; }
    void clear() { env = gr = 0; gain = gainT = 1; }
    // control rate: threshold dB, ratio, attack/release seconds
    void prepare(float threshold, float ratio, float att, float rel, bool autoGain) {
        cA = 1.f - std::exp(-1.f / (std::max(att, 0.0001f) * fs)); cR = 1.f - std::exp(-1.f / (std::max(rel, 0.001f) * fs));
        thr = threshold; sl = 1.f - 1.f / std::max(ratio, 1.f); makeup = autoGain ? -thr * sl * 0.5f : 0.f;
    }
    // the gain to apply now (linear)
    inline float process(float l, float r) {
        const float a = std::max(std::fabs(l), std::fabs(r));
        env += (a > env ? cA : cR) * (a - env);
        if (tick-- <= 0) {
            tick = 7;
            const float lv = 20.f * std::log10(std::max(env, 1e-6f)), knee = 6.f, over = lv - thr;
            float target = 0;
            if (over > knee / 2) target = over * sl;
            else if (over > -knee / 2) { const float o = over + knee / 2; target = sl * o * o / (2 * knee); }
            gr = target; meterIn = lv; meterGr = gr;
            gainT = std::pow(10.f, (makeup - gr) / 20.f);
        }
        gain += (gainT - gain) * 0.25f;
        return gain;
    }
};

} // namespace fx
} // namespace obsidian
