// Obsidian — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "ParamDefs.hpp"

namespace obsidian {

static const char* const kManualURL = "https://github.com/natehouk/obsidian/blob/main/docs/MANUAL.md";

inline const char* styleTip(int s) {
    switch (s) {
    case 0: return "Smooth: moderate overdrive, broad resonance.";
    case 1: return "Hot: saturating integrators, a screaming peak.";
    case 2: return "Raw: hard-clipped feedback, buzzy self-oscillation.";
    case 3: return "Monolith: the Monolith synth's filter; soft-limited and self-oscillating.";
    case 4: return "Clean: linear and transparent; never self-oscillates. The style for precise EQ shapes.";
    case 5: return "Warm: a saturated input into a linear filter; rounded, never harsh.";
    case 6: return "Acid: resonance thins out the bass; squelchy and rubbery.";
    case 7: return "Tube: an asymmetric, valve-like input stage; warm overdrive with mostly even harmonics.";
    case 8: return "Gentle: clean, polite and broad.";
    case 9: return "Metal: hard-clipped input and feedback; rough and sharp.";
    case 10: return "Hollow: the band at the cutoff is carved out as the peak rises; nasal and vocal.";
    default: return "";
    }
}
inline const char* filterFieldTip(int k) {
    switch (k) {
    case kFOn: return "Switches this filter on or off. You can also drag its numbered handle in the display; right-click a handle to switch it off, double-click an empty spot to add one.";
    case kFStyle: return "The filter's character, from Clean to Metal. Click to choose.";
    case kFType: return "LP keeps the lows, BP a band around the cutoff, HP the highs. BELL boosts or cuts around the cutoff; LO SH and HI SH boost or cut below or above it (GAIN sets how much); NOTCH removes a narrow band.";
    case kFSlope: return "How steep the filter is: 6, 12, 24 or 48 dB per octave. For bell and shelf shapes, steeper means narrower.";
    case kFFreq: return "The cutoff (or centre) frequency, 20 Hz to 20 kHz. Drag the filter's handle in the display too.";
    case kFPeak: return "Resonance: emphasises the cutoff; most styles self-oscillate near the top. For bell, shelf and notch shapes it narrows the band.";
    case kFPan: return "Offsets the cutoff between the left and right channels, for stereo movement.";
    case kFGain: return "For bell and shelf shapes: how much to boost or cut, up to 24 dB.";
    default: return "";
    }
}

inline const char* paramTip(uint32_t p) {
    for (int f = 0; f < kNumFilters; ++f) for (int k = 0; k <= kFGain; ++k) if (fpar(f, k) == p) return filterFieldTip(k);
    for (int o = 0; o < kNumOsc; ++o) {
        if (p == opar(o, kOWave)) return "The oscillator's shape: triangle, saw, square (set its width with PW), sine, white noise or pink noise. Band-limited, so high notes don't alias.";
        if (p == opar(o, kOOct)) return "Shifts the oscillator by whole octaves, three either way.";
        if (p == opar(o, kOSemi)) return "Shifts the oscillator by semitones, up to an octave either way. Try +7 for a fifth.";
        if (p == opar(o, kOFine)) return "Fine tuning, up to a semitone either way (in cents). Detune oscillators slightly for a thicker sound.";
        if (p == opar(o, kOPW)) return "The pulse width of the square wave. 50% is a hollow square; narrower is thinner and nasal. Modulate it for movement.";
        if (p == opar(o, kOLevel)) return "The oscillator's level in the mix.";
        if (p == opar(o, kOPan)) return "Places the oscillator in the stereo field.";
        if (p == opar(o, kOPhase)) return "On: every note starts the oscillator at the same point, for consistent, punchy attacks. Off: each note starts at a random point, which sounds looser and wider.";
        if (p == (uint32_t)(kOscOnFirst + o)) return "Switches this oscillator on or off.";
        if (p == (uint32_t)(kOscRatioFirst + o)) return "Hard sync ratio. At 1 the oscillator runs normally; higher values squeeze more cycles into each period of the note, for a metallic, tearing sync sound. The pitch stays at the note. Modulate it.";
    }
    switch (p) {
    case kRing12: return "Ring modulation: oscillator 2 is multiplied by oscillator 1, giving sum and difference frequencies: bells, metal and robots.";
    case kRing34: return "Ring modulation: oscillator 4 is multiplied by oscillator 3.";
    case kDrift: return "Analog drift: every oscillator of every voice wanders slowly and randomly in pitch, a few cents at most.";
    case kTune: return "Master tuning, up to a semitone either way (in cents).";
    case kTranspose: return "Transposes the whole synth by octaves.";
    case kPolyMode: return "PER OSC: each new note plays only the next oscillator (1, 2, 3, 4, 1...), so successive notes sound different. Works best with all four oscillators on and set differently.";
    case kHq: return "High quality: the oscillators and filters run at 4x the sample rate, for less aliasing from bright, high and heavily driven sounds. Uses about three times the CPU and adds 24 samples of latency.";
    case kFreqOffset: return "Moves the cutoff of every filter at once, up to 5 octaves either way. Modulate it to sweep all filters together.";
    case kPeakOffset: return "Raises or lowers the peak of every filter at once.";
    case kRevOn: return "Switches the reverb on.";
    case kRevMix: return "How much reverb is added.";
    case kRevTime: return "How long the reverb takes to die away, 0.2 to 20 seconds.";
    case kRevBright: return "Dark (soft, distant) to bright (airy).";
    case kRevPredelay: return "A gap before the reverb starts, up to 250 ms. Modulate it for tape-like pitch movement.";
    case kDlyOn: return "Switches the delay on.";
    case kDlyMix: return "How much of the echoes is added.";
    case kDlySync: return "Locks the delay to the host tempo; NOTE then sets the length.";
    case kDlyTime: return "The delay time, 1 ms to 2 s.";
    case kDlyDiv: return "The delay as a note length.";
    case kDlyOffset: return "Makes one side longer: right of centre lengthens the right side, left of centre the left side, by up to 50%.";
    case kDlyFeedback: return "How much of the echo is fed back: more repeats.";
    case kDlyPingPong: return "The echoes bounce between left and right.";
    case kDlyLowCut: return "Removes lows from the echoes; each repeat gets thinner.";
    case kDlyHighCut: return "Removes highs from the echoes; each repeat gets darker.";
    case kChoOn: return "Switches the chorus on.";
    case kChoMix: return "How much chorus: at 100% the effect is strongest.";
    case kChoMode: return "SINGLE: one modulated copy per side. DOUBLE: two, for a thicker ensemble.";
    case kChoRate: return "How fast the chorus moves.";
    case kChoDepth: return "How far the chorus moves.";
    case kChoDelay: return "The chorus's base delay: short for flanger-like, long for doubling.";
    case kPhsOn: return "Switches the phaser (or flanger) on.";
    case kPhsMix: return "How strong the effect is.";
    case kPhsMode: return "PHASER: six all-pass stages sweep notches through the sound. FLANGER: a short swept delay instead.";
    case kPhsRate: return "How fast the sweep moves.";
    case kPhsDepth: return "How far the sweep moves.";
    case kPhsFeedback: return "Feedback: sharper, more resonant notches.";
    case kPhsShift: return "Offsets the left and right sweeps, up to half a cycle, for a wide stereo effect.";
    case kDrvOn: return "Switches the drive on.";
    case kDrvAmount: return "From subtle saturation to amp-style distortion.";
    case kDrvPost: return "PRE: drive before every other effect. POST: after the modulation effects, just before the compressor.";
    case kCmpOn: return "Switches the compressor on. It is the last effect.";
    case kCmpThreshold: return "The level above which the compressor turns things down.";
    case kCmpRatio: return "How hard it compresses: 2 is gentle, 20 is limiting.";
    case kCmpAttack: return "How fast it reacts to peaks.";
    case kCmpRelease: return "How fast it lets go.";
    case kCmpAutoGain: return "Makes up for the level the compressor takes away.";
    case kCmpMix: return "Blends the compressed signal with the uncompressed one (parallel compression).";
    case kArpOn: return "Switches the arpeggiator on: held notes play one after another.";
    case kArpRate: return "The arpeggiator's speed, 0.25 to 20 notes a second (when not synced).";
    case kArpSync: return "Locks the arpeggiator to the host tempo; NOTE sets the step length and OFFSET multiplies the speed.";
    case kArpDiv: return "The step length when synced.";
    case kArpOffset: return "Multiplies the synced speed, from half to double.";
    case kArpOrder: return "UP, DOWN, UP/DOWN, AS PLAYED or RANDOM.";
    case kArpOctaves: return "Also plays the notes one, two or three octaves higher.";
    case kArpGroove: return "Pushes (right) or pulls (left) every second note, for a shuffle.";
    case kArpGate: return "How long each note lasts, from short and staccato to fully legato.";
    case kArpLatch: return "OFF: only the keys you hold. HELD: a chord keeps playing after you let go. FULL: every note you played keeps playing until you play new ones.";
    case kArpRetrig: return "On: the pattern restarts with the first key. Off: when synced and the host plays, it follows the timeline.";
    case kArpLock: return "Keeps the arpeggiator's settings when you load a preset.";
    default: break;
    }
    if (p >= kXlfoFirst && p < kEgFirst) {
        switch ((p - kXlfoFirst) % em::kXlfoFields) {
        case em::kLMode: return "WAVE: a classic LFO shape. STEPS: a 16-step sequencer you draw in the step display.";
        case em::kLWave: return "The XLFO shape: sine, triangle, saw up, saw down, square, smooth random, sample & hold.";
        case em::kLRate: return "XLFO speed in Hz when not synced.";
        case em::kLSync: return "FREE runs at the Rate. SYNC, DOTTED and TRIPLET lock to the host tempo.";
        case em::kLDiv: return "Length of one cycle when synced, from 16 bars down to a 1/64 note.";
        case em::kLBalance: return "Skews the shape: for triangle and sine from falling to rising ramp; for square it sets the duty cycle.";
        case em::kLGlide: return "In step mode, how smoothly the value slides from one step to the next.";
        case em::kLSteps: return "How many steps the sequence uses before it repeats.";
        default: return "One step of the sequence. Drag up and down in the step display.";
        }
    }
    if (p >= kEgFirst && p < kEfFirst) {
        switch ((p - kEgFirst) % em::kEgFields) {
        case em::kEAttack: return "Envelope attack time."; case em::kEDecay: return "Envelope decay time."; case em::kESustain: return "Level the envelope holds while the key is down.";
        case em::kERelease: return "Envelope release time.";
        case em::kETrigger: return "NOTE: every note starts the envelope. LEGATO: a note played while another is held carries on from where the envelope already is.";
        default: return "Not used by Obsidian.";
        }
    }
    if (p >= kEfFirst && p < kXyFirst) {
        switch ((p - kEfFirst) % em::kEfFields) {
        case em::kFAttack: return "How fast the follower rises with the level."; case em::kFRelease: return "How fast it falls back.";
        case em::kFGain: return "Sensitivity: raises or lowers the level the follower sees.";
        default: return "VOICE: each voice follows its own level. SYNTH: every voice follows the whole synth.";
        }
    }
    if (p >= kXyFirst && p < kSlot2First) return "XY controller position. Drag the dot in the pad; X and Y are two separate sources. Great for MIDI Learn and automation.";
    if (p >= kSlot2First && p < kXlfoRetrigFirst) {
        switch ((p - kSlot2First) % em::kSlotFields) {
        case kSlotSrc: return "The modulation source. Click to change.";
        case kSlotDst: return "What this slot modulates. Click to change: oscillators, filters, envelopes, the effects, the arpeggiator, the XLFOs and EGs, and other slots' amounts.";
        case kSlotAmt: return "How much, and in which direction. Turn left to invert. Pitch reaches 24 semitones, filter frequency 5 octaves.";
        default: return "Switches this slot off without deleting it.";
        }
    }
    if (p >= kXlfoRetrigFirst && p < kEgCurveFirst) return "PER NOTE: every voice has its own XLFO, restarting with each note. Off: one XLFO for the whole synth, so all notes move together.";
    if (p >= kEgCurveFirst && p < kEfModeFirst) return "The curve of this stage: left starts slowly, centre is straight, right starts fast. Drag the dots in the envelope display.";
    if (p >= kEfModeFirst && p < kParamCount) return "TRANSIENT: the follower jumps on every attack and falls back while the sound holds, like a transient detector.";
    switch (p) {

    case kOscSync: return "Classic hard sync: oscillator 2 restarts every time oscillator 1 starts a cycle. Tune oscillator 2 up (or modulate its pitch) for tearing, vocal sync sounds.";
    case kRouting: return "SERIAL: every oscillator goes through the filters that are on, 1 to 4. PARALLEL: every filter gets all the oscillators, and their outputs are summed. PER OSC: oscillator 1 feeds filter 1, oscillator 2 filter 2, and so on.";
    case kE1Delay: case kE2Delay: return "Classic envelope: waits this long after the note starts before the envelope begins. Great for delayed vibrato.";
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
    case kL1Wave: case kL2Wave: case kL3Wave: return "The classic LFO's shape: TRI, SIN, SQR, RND (glides smoothly between random values) or S&H (jumps between random values).";
    case kL1Balance: case kL2Balance: case kL3Balance: return "For triangle and sine, skews the shape from a falling ramp through symmetric (50%) to a rising ramp. For square, sets the duty cycle.";
    case kL1Sync: case kL2Sync: case kL3Sync: return "FREE runs at RATE in Hz. STR, DOT and TRIP lock the LFO to the host tempo and song position with straight, dotted or triplet note lengths; RATE then picks the length.";
    case kL1Rate: case kL2Rate: case kL3Rate: return "LFO speed, 0.01 to 50 Hz. When synced, this knob picks a note length (16 bars to 1/64) instead.";
    case kL1Div: case kL2Div: case kL3Div: return "The LFO cycle length when synced, from 16 bars to a 64th note.";
    case kL1NoteSync: case kL2NoteSync: case kL3NoteSync: return "Restarts the LFO for every note, so each voice has its own LFO starting at the same point.";
    case kVoices: return "How many notes can sound at once, up to 64. At 1, Obsidian plays monophonically with last-note priority.";
    case kUnison: return "Stacks up to 8 voices on every note, detuned and spread across the stereo field. Uses up voices: the total is capped at 64.";
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
                 kTipChip, kTipSlotDel, kTipFilter, kTipEnv, kTipLfo,
                 kTipDisplay, kTipHandle, kTipFilterTab, kTipStepEdit, kTipXyPad, kTipSourceTab, kTipCompPresets, kTipModRing,
                 kTipClassicChip, kTipClassicRow, kTipFxUnit, kTipOscWave, kTipArpView, kTipEgCurve, kTipArpPresets, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "MIDI", "Help", "Setting A", "Setting B", "Copy A/B",
                                                   "Previous preset", "Presets", "Next preset", "Modulation source", "Remove slot",
                                                   "Filter response", "Envelope", "LFO",
                                                   "Filter display", "Filter handle", "Filter", "Step sequence", "XY pad", "Modulation sources", "Source presets", "Modulated",
                                                   "Classic source", "Classic modulation", "Effect", "Waveform", "Arpeggiator", "Envelope curve", "Arpeggiator presets" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including preset loads, new modulation and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Right-click a control to unassign.";
    case kTipMidi: return paramTip(kMidiOn);
    case kTipHelp: return "Opens the help pages: oscillators, filters, effects, arpeggiator, modulation, voices, MIDI and shortcuts.";
    case kTipA: case kTipB: return "Obsidian keeps two complete sounds, A and B. Click the other letter to switch and compare.";
    case kTipCopy: return "Copies the current sound to the other slot.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser: factory sounds in eight categories, your own sounds and your favourites. Type to search names and categories; click a star to keep a preset in Favourites. A * after the name means you have changed it.";
    case kTipNext: return "Load the next preset.";
    case kTipChip: return "Drag this source onto any knob to modulate it. A slot is created with a starting amount, and the knob gets a coloured ring.";
    case kTipSlotDel: return "Removes this modulation slot.";
    case kTipFilter: return "The filter's response at the current type, slope, cutoff and peak.";
    case kTipEnv: return "The envelope's shape. Drag its chip onto a knob to use it.";
    case kTipLfo: return "The LFO's shape. Drag its chip onto a knob to use it.";
    case kTipDisplay: return "The filters' responses and their combined curve. Drag a numbered handle: left/right is frequency, up/down is peak (or gain for bell and shelf). Double-click an empty spot to switch on the next free filter there; right-click a handle to switch it off.";
    case kTipHandle: return "Drag left/right for frequency, up/down for peak (gain for bell and shelf). Click to select; right-click to switch the filter off. Shift-drag is fine adjustment.";
    case kTipFilterTab: return "Click to edit this filter. The dot switches it on or off.";
    case kTipStepEdit: return "Draw the sequence: click or drag up and down across the steps. Cmd/Ctrl-click resets a step to zero.";
    case kTipXyPad: return "Drag the dot. X and Y are separate modulation sources; drag their chips onto knobs.";
    case kTipSourceTab: return "Choose a modulation source to edit: four XLFOs (wave or step sequencer), two envelopes with curves, two envelope followers, two XY pads, the MIDI sources, and Obsidian 2's classic LFOs and envelopes.";
    case kTipArpPresets: return "Presets for just the arpeggiator: load a ready-made pattern (order, octaves, note length, groove, latch) without touching the sound, or save your own. Right-click a saved one to delete it.";
    case kTipCompPresets: return "Presets for just this source: load a ready-made LFO shape, sequence, envelope or follower setting without touching the rest of the sound, or save your own. Right-click a saved one to delete it.";
    case kTipModRing: return "This knob is modulated. Click the ring (or right-click the knob) to list every slot that moves it, with its amount, bypass and delete.";
    case kTipClassicChip: return "A classic source from Obsidian 2. Drag it onto a knob Obsidian 2 could modulate; it makes a classic slot (up to 24). For everything else, use the XLFOs, EGs and followers.";
    case kTipClassicRow: return "A classic slot (Obsidian 2's modulation). Click the source or destination to change it; drag the amount.";
    case kTipFxUnit: return "Click the name to edit this effect's settings on the right; the switch turns it on; the knob sets how much. The chain runs reverb, delay, chorus, phaser, (drive), compressor.";
    case kTipOscWave: return "The oscillator's waveform. Click to step through triangle, saw, square, sine, white and pink noise (right-click steps back).";
    case kTipArpView: return "The pattern the arpeggiator is playing: the held notes, and the one sounding now.";
    case kTipEgCurve: return "Drag a dot up or down to bend the attack, decay or release curve.";
    default: return "";
    }
}

struct HelpPage { const char* title; const char* body; };
static const HelpPage kHelpPages[] = {
    { "Overview",
      "Obsidian is a polyphonic synthesizer: four oscillators feed four filters, then an effects section; an arpeggiator plays the notes, and drag-and-drop modulation ties everything together, per voice.\n\n"
      "1. Pick a preset, or start from Init.\n"
      "2. Shape the tone with the oscillators and filters.\n"
      "3. Drag a source chip (XLFO, EG, VEL, ...) onto any knob to modulate it.\n"
      "4. Add effects, set the voices, unison and glide, or switch on the arpeggiator.\n\n"
      "Hover over anything for a moment to see what it does." },
    { "Oscillators",
      "Four oscillators: triangle, saw, square (with PW), sine, white or pink noise, tuned by OCT, SEMI and FINE, with LEVEL and PAN. The dot switches each one on.\n\n"
      "SYNC is a hard-sync ratio (1 is off); higher squeezes more cycles into each note period. RING (between 1-2 and 3-4) multiplies the pair. PHASE starts every note at the same point.\n\n"
      "DRIFT makes every oscillator wander slightly, like analog. TUNE, TRANSPOSE and BEND are in VOICE." },
    { "Filters",
      "Four filters in eleven styles: low/band/high pass, bell, shelves and notch, 6 to 48 dB. Drag their handles in the display; double-click to add one, right-click a handle to remove it.\n\n"
      "ROUTING: SERIAL (1 into 2 into 3 into 4), PARALLEL (all summed), or PER OSC (oscillator 1 into filter 1, and so on). FREQ and PEAK OFFSET move all filters at once." },
    { "Effects",
      "Reverb, delay, chorus, phaser/flanger, drive and compressor, in that order (drive can also go first). Each has a switch and an AMOUNT knob; click its name to edit the rest on the right. Every effect knob can be modulated." },
    { "Arpeggiator",
      "Switch it on and hold a chord. ORDER: up, down, up/down, as played, random; OCTAVES adds higher octaves. Free RATE, or SYNC to the tempo with a NOTE length and an OFFSET. GROOVE shuffles every second note, LEGATO sets the note length. LATCH keeps the pattern going after you let go. LOCK keeps the arpeggiator when you change presets; PRESETS loads or saves just the arpeggiator." },
    { "Modulation",
      "Sources: four XLFOs (wave or 16-step sequencer, per note or shared), two EGs with curves, two envelope followers, two XY pads, the MIDI sources, and Obsidian 2's classic LFOs and envelopes. Every source runs per voice.\n\n"
      "Drag a chip onto any knob: the knob gets a coloured ring, and the slot appears in the list (100 slots, plus 24 classic). Right-click a knob to list what moves it. A slot's amount is itself a destination." },
    { "Voices",
      "VOICES sets the polyphony, up to 64; at 1 Obsidian is monophonic. UNISON stacks up to 8 voices per note, with DETUNE and SPREAD. PER OSC makes each new note play the next oscillator.\n\n"
      "GLIDE slides between notes; LEGATO glides only overlapping ones. HQ runs the voices at 4x for less aliasing. The AMP envelope shapes each voice's volume." },
    { "MIDI & shortcuts",
      "MIDI LEARN: click LEARN, click a control, move a controller. Right-click unassigns. The MIDI button switches all MIDI input off.\n\n"
      "A/B holds two complete sounds; COPY copies the current one to the other slot. Every edit is an undo step (Cmd/Ctrl-Z, Cmd/Ctrl-Shift-Z).\n\n"
      "PRESETS: in the browser, type to search names and categories; click a star to add a preset to Favourites.\n\n"
      "Double-click a knob to type a value; Cmd/Ctrl-click resets it; Shift-drag is fine adjustment. Esc closes this window." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace obsidian
