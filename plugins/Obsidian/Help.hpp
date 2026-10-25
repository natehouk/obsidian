// Obsidian — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "ParamDefs.hpp"

namespace obsidian {

static const char* const kManualURL = "https://github.com/natehouk/obsidian/blob/main/docs/MANUAL.md";

inline const char* paramTip(uint32_t p) {
    if (p >= kSlotFirst && p <= kSlotLast) {
        switch ((p - kSlotFirst) % 4) {
        case kSlotSrc: return "What drives this modulation slot. Click to pick a source; an empty slot does nothing.";
        case kSlotDst: return "What this slot modulates. Click to pick a destination: oscillator, filter, envelope, LFO, voice settings, or another slot's amount.";
        case kSlotAmt: return "How much the source moves the destination. Turn left to invert. Pitch reaches 24 semitones, filter frequency 5 octaves.";
        default: return "Bypasses this slot without losing its settings.";
        }
    }
    switch (p) {
    case kO1Wave: case kO2Wave: return "The oscillator's shape: TRI (soft, hollow), SAW (bright, buzzy) or SQR (square; set its width with PW). Band-limited, so high notes don't alias.";
    case kO1Oct: case kO2Oct: return "Shifts the oscillator by whole octaves, three either way.";
    case kO1Semi: case kO2Semi: return "Shifts the oscillator by semitones, up to an octave either way. Try +7 for a fifth.";
    case kO1Fine: case kO2Fine: return "Fine tuning, up to a semitone either way (in cents). Detune the two oscillators slightly for a thicker sound.";
    case kO1PW: case kO2PW: return "The pulse width of the square wave. 50% is a hollow square; narrower is thinner and nasal. Modulate it for movement.";
    case kO1Level: case kO2Level: return "The oscillator's level in the mix.";
    case kO1Pan: case kO2Pan: return "Places the oscillator in the stereo field.";
    case kO1Phase: case kO2Phase: return "On: every note starts the oscillator at the same point, for consistent, punchy attacks. Off: each note starts at a random point, which sounds looser and wider.";
    case kOscSync: return "Hard sync: oscillator 2 restarts every time oscillator 1 starts a cycle. Tune oscillator 2 up (or modulate its pitch) for tearing, vocal sync sounds.";
    case kF1On: case kF2On: return "Switches this filter on or off.";
    case kF1Style: case kF2Style: return "The filter's character: SMOOTH (moderate overdrive, broad), HOT (saturating, screaming peak), RAW (buzzy, self-oscillating) or MONOLITH (the Monolith synth's filter). Click the arrows or scroll.";
    case kF1Type: case kF2Type: return "LP keeps the lows, BP keeps a band around the cutoff, HP keeps the highs.";
    case kF1Slope: case kF2Slope: return "How steeply the filter cuts: 12 or 24 dB per octave.";
    case kF1Freq: case kF2Freq: return "The cutoff frequency, 20 Hz to 20 kHz. In the Init sound, Key Track and Env 1 already move filter 1.";
    case kF1Peak: case kF2Peak: return "Resonance: emphasises the frequencies around the cutoff. RAW and MONOLITH self-oscillate near the top.";
    case kF1Pan: case kF2Pan: return "Offsets the cutoff between the left and right channels, for stereo movement.";
    case kRouting: return "SERIAL: both oscillators go through filter 1, then filter 2. PARALLEL: both oscillators feed both filters, which are summed. SPLIT: oscillator 1 feeds filter 1, oscillator 2 feeds filter 2.";
    case kE1Delay: case kE2Delay: return "Waits this long after the note starts before the envelope begins. Great for delayed vibrato.";
    case kE1Attack: case kE2Attack: return "How long the envelope takes to rise.";
    case kE1Decay: case kE2Decay: return "How long it takes to fall to the sustain level.";
    case kE1Sustain: case kE2Sustain: return "The level it holds while the key is down.";
    case kE1Release: case kE2Release: return "How long it takes to fall to zero after the key is released.";
    case kE1Invert: case kE2Invert: return "Turns the envelope upside down, so it sweeps down instead of up.";
    case kE3Attack: return "Amp envelope attack: how long each note takes to fade in.";
    case kE3Hold: return "Keeps the note at full level for this long after the attack, before the decay starts.";
    case kE3Decay: return "Amp envelope decay: how long it takes to fall to the sustain level.";
    case kE3Sustain: return "Amp envelope sustain: the level while the key is held.";
    case kE3Release: return "Amp envelope release: how long the note takes to fade out after the key is released.";
    case kAmpVel: return "How much velocity changes the volume. 0%: every note is equally loud.";
    case kL1Wave: case kL2Wave: case kL3Wave: return "The LFO's shape: TRI, SIN, SQR, RND (glides smoothly between random values) or S&H (jumps between random values).";
    case kL1Balance: case kL2Balance: case kL3Balance: return "For triangle and sine, skews the shape from a falling ramp through symmetric (50%) to a rising ramp. For square, sets the duty cycle.";
    case kL1Sync: case kL2Sync: case kL3Sync: return "FREE runs at RATE in Hz. STR, DOT and TRIP lock the LFO to the host tempo and song position with straight, dotted or triplet note lengths; RATE then picks the length.";
    case kL1Rate: case kL2Rate: case kL3Rate: return "LFO speed, 0.01 to 50 Hz. When synced, this knob picks a note length (16 bars to 1/64) instead.";
    case kL1Div: case kL2Div: case kL3Div: return "The LFO cycle length when synced, from 16 bars to a 64th note.";
    case kL1NoteSync: case kL2NoteSync: case kL3NoteSync: return "Restarts the LFO for every note, so each voice has its own LFO starting at the same point.";
    case kVoices: return "How many notes can sound at once, up to 24. At 1, Obsidian plays monophonically with last-note priority.";
    case kUnison: return "Stacks up to 4 voices on every note, detuned and spread across the stereo field. Uses up voices: the total is capped at 24.";
    case kUniDetune: return "How far the unison voices are detuned from each other.";
    case kUniSpread: return "How wide the unison voices are spread across the stereo field.";
    case kVolume: return "Output level.";
    case kGlide: return "Slides the pitch from the previous note to the new one. Works in mono and poly.";
    case kGlideMode: return "ALWAYS glides on every note. LEGATO glides only when notes overlap, and in mono doesn't restart the envelopes.";
    case kBendRange: return "How far the pitch-bend wheel bends, up to 24 semitones.";
    case kMidiOn: return "Turns all MIDI input on or off: notes, controllers and MIDI Learn. Leave it on to play Obsidian.";
    default: return "";
    }
}

enum TipTarget { kTipUndo, kTipRedo, kTipLearn, kTipMidi, kTipHelp, kTipA, kTipB, kTipCopy, kTipPrev, kTipPresets, kTipNext,
                 kTipChip, kTipSlotDel, kTipFilter, kTipEnv, kTipLfo, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "MIDI", "Help", "Setting A", "Setting B", "Copy A/B",
                                                   "Previous preset", "Presets", "Next preset", "Modulation source", "Remove slot",
                                                   "Filter response", "Envelope", "LFO" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including preset loads, new modulation and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Right-click a control to unassign.";
    case kTipMidi: return paramTip(kMidiOn);
    case kTipHelp: return "Opens the help pages: oscillators, filters, envelopes and LFOs, modulation, voices, MIDI and shortcuts.";
    case kTipA: case kTipB: return "Obsidian keeps two complete sounds, A and B. Click the other letter to switch and compare.";
    case kTipCopy: return "Copies the current sound to the other slot.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser: factory sounds in six categories, your own sounds, and SAVE CURRENT AS. A * after the name means you have changed it.";
    case kTipNext: return "Load the next preset.";
    case kTipChip: return "Drag this source onto any knob to modulate it. A slot is created with a starting amount, and the knob gets a coloured ring.";
    case kTipSlotDel: return "Removes this modulation slot.";
    case kTipFilter: return "The filter's response at the current type, slope, cutoff and peak.";
    case kTipEnv: return "The envelope's shape. Drag its chip (top right) onto a knob to use it.";
    case kTipLfo: return "The LFO's shape. Drag its chip (top right) onto a knob to use it.";
    default: return "";
    }
}

struct HelpPage { const char* title; const char* body; };
static const HelpPage kHelpPages[] = {
    { "Overview",
      "Obsidian is a polyphonic synthesizer: two oscillators feed two filters, shaped by three envelopes and three LFOs, all tied together by drag-and-drop modulation.\n\n"
      "1. Pick a preset, or start from Init.\n"
      "2. Shape the tone with the oscillators and filters.\n"
      "3. Drag a source chip (ENV 1, LFO 1, VEL, ...) onto any knob to modulate it.\n"
      "4. Set the voices, unison and glide.\n\n"
      "Hover over anything for a moment to see what it does." },
    { "Oscillators & filters",
      "Each oscillator is a triangle, saw or square (with PW), tuned by OCT, SEMI and FINE, with its own LEVEL and PAN. PHASE SYNC starts every note at the same point; HARD SYNC restarts oscillator 2 with oscillator 1.\n\n"
      "Each filter has four styles (SMOOTH, HOT, RAW, MONOLITH), LP/BP/HP at 12 or 24 dB, FREQ, PEAK and stereo PAN.\n\n"
      "Routing: SERIAL (filter 1 into filter 2), PARALLEL (both filters summed) or SPLIT (oscillator 1 to filter 1, oscillator 2 to filter 2)." },
    { "Envelopes & LFOs",
      "ENV 1 and ENV 2 are for modulation: DELAY, ATTACK, DECAY, SUSTAIN, RELEASE, and INVERT.\n\n"
      "The AMP ENV shapes each voice's volume, with a HOLD stage after the attack and a VELOCITY amount.\n\n"
      "Three LFOs: TRI, SIN, SQR, RND and S&H, with BALANCE. Free in Hz or synced to the host (straight, dotted, triplet). NOTE SYNC restarts an LFO for each note." },
    { "Modulation",
      "Drag a source chip onto any knob. The knob gets a coloured ring and a slot appears in the MODULATION panel.\n\n"
      "Each slot has a source, a destination, an amount (left of centre inverts), a bypass switch and a delete button. Click the source or destination to change it.\n\n"
      "Every slot's amount is itself a destination: the mod wheel can bring in vibrato by controlling another slot's depth.\n\n"
      "24 slots in all." },
    { "Voices",
      "VOICES sets the polyphony, up to 24; at 1 Obsidian is monophonic. UNISON stacks up to 4 voices per note, with DETUNE and SPREAD.\n\n"
      "GLIDE slides between notes; ALWAYS glides every note, LEGATO only overlapping ones. BEND sets the pitch-bend range. The sustain pedal holds notes." },
    { "MIDI & shortcuts",
      "MIDI LEARN: click LEARN, click a control, move a controller. Right-click unassigns. The MIDI button switches all MIDI input off.\n\n"
      "A/B holds two complete sounds; COPY copies the current one to the other slot. Every edit is an undo step (Cmd/Ctrl-Z, Cmd/Ctrl-Shift-Z).\n\n"
      "Double-click a knob to type a value; Cmd/Ctrl-click resets it; Shift-drag is fine adjustment; grab the rim of a large knob to turn it in a circle. Esc closes this window." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace obsidian
