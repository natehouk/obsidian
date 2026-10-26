// Obsidian 3.0 — arpeggiator: note order, octave range, free or tempo-synced rate with a rate offset,
// groove (every second note pushed or pulled), note length up to legato, latch, retrigger.
// The engine feeds it the keys and calls tick() every sample; tick() returns note on / off events.
// License: MIT.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace obsidian {

enum ArpOrder { kArpUp, kArpDown, kArpUpDown, kArpPlayed, kArpRandom, kNumArpOrders };
enum ArpLatch { kLatchOff, kLatchHeld, kLatchFull };
static constexpr int kNumArpDivs = 9;
// synced step lengths, in beats: 1/4, 1/8, 1/8 dotted, 1/8 triplet, 1/16, 1/16 dotted, 1/16 triplet, 1/32, 1/2
inline float arpDivBeats(int i) {
    static const float t[kNumArpDivs] = { 1.f, 0.5f, 0.75f, 1.f / 3.f, 0.25f, 0.375f, 1.f / 6.f, 0.125f, 2.f };
    return t[i < 0 ? 0 : (i >= kNumArpDivs ? kNumArpDivs - 1 : i)];
}
inline const char* arpDivName(int i) {
    static const char* const n[kNumArpDivs] = { "1/4", "1/8", "1/8 dot", "1/8 trip", "1/16", "1/16 dot", "1/16 trip", "1/32", "1/2" };
    return n[i < 0 ? 0 : (i >= kNumArpDivs ? kNumArpDivs - 1 : i)];
}

struct Arp {
    struct Key { int note, vel; uint32_t order; };
    Key keys[32]; int nKeys = 0;              // the pattern's notes
    int down[32]; int nDown = 0;              // keys physically held
    uint32_t orderCounter = 0, rng = 99991;
    double phase = 0;                         // in steps
    long step = -1;                           // the step last triggered
    int playing = -1, playingVel = 0; double offAt = 0; bool tie = false;
    int seqPos = 0; int dir = 1;

    void reset() { nKeys = nDown = 0; phase = 0; step = -1; playing = -1; seqPos = 0; dir = 1; }

    void keyOn(int note, int vel, int) {
        if (nDown == 0) nKeys = 0;            // a new chord replaces the (latched) pattern
        if (nDown < 32) down[nDown++] = note;
        for (int i = 0; i < nKeys; ++i) if (keys[i].note == note) { keys[i].vel = vel; return; }
        if (nKeys < 32) keys[nKeys++] = { note, vel, ++orderCounter };
    }
    // returns true when the pattern became empty
    bool keyOff(int note, int latch) {
        int j = 0; for (int i = 0; i < nDown; ++i) if (down[i] != note) down[j++] = down[i];
        nDown = j;
        if (latch == kLatchOff) {
            j = 0; for (int i = 0; i < nKeys; ++i) if (keys[i].note != note) keys[j++] = keys[i];
            nKeys = j;
        } else if (nDown == 0 && latch == kLatchHeld && nKeys < 2) nKeys = 0;     // HELD only keeps chords
        return nKeys == 0;
    }
    void clearLatch() { nKeys = 0; }

    // the note for a sequence position
    int noteAt(int pos, int order, int octaves, int& vel) {
        const int n = nKeys;
        Key tmp[32]; std::copy(keys, keys + n, tmp);
        if (order == kArpPlayed) std::sort(tmp, tmp + n, [](const Key& a, const Key& b) { return a.order < b.order; });
        else std::sort(tmp, tmp + n, [](const Key& a, const Key& b) { return a.note < b.note; });
        const int span = n * (octaves + 1);
        if (span <= 0) { vel = 0; return -1; }
        int idx;
        switch (order) {
        case kArpDown: idx = span - 1 - (pos % span); break;
        case kArpUpDown: { const int cyc = span > 1 ? 2 * span - 2 : 1; const int q = pos % cyc; idx = q < span ? q : cyc - q; break; }
        case kArpRandom: { rng = rng * 1664525u + 1013904223u; idx = (int)((rng >> 8) % (uint32_t)span); break; }
        default: idx = pos % span; break;
        }
        const Key& k = tmp[idx % n];
        vel = k.vel;
        return std::min(127, k.note + 12 * (idx / n));
    }
};

} // namespace obsidian
