# Obsidian 2.0 — User Manual

Obsidian is a polyphonic synthesizer. It has:

- two oscillators feeding two filters
- three envelopes and three LFOs
- 24 drag-and-drop modulation slots

It plays up to 24 voices, with unison and glide. Typical uses:

- warm and evolving pads, strings and brass
- sync leads, unison leads and basses
- keys, plucks and bells
- rhythmic, tempo-synced motion and effects

Obsidian runs as VST3, AU, CLAP and VST2 on macOS, Windows and Linux.

**Contents**

1. [Installing](#1-installing)
2. [A quick tour](#2-a-quick-tour)
3. [The oscillators](#3-the-oscillators)
4. [The filters](#4-the-filters)
5. [The envelopes](#5-the-envelopes)
6. [The LFOs](#6-the-lfos)
7. [Modulation](#7-modulation)
8. [Voices](#8-voices)
9. [Knobs](#9-knobs)
10. [A/B, undo and redo](#10-ab-undo-and-redo)
11. [MIDI](#11-midi)
12. [Presets](#12-presets)
13. [Help and hover tips](#13-help-and-hover-tips)
14. [Recipes](#14-recipes)
15. [Parameter reference](#15-parameter-reference)
16. [Troubleshooting](#16-troubleshooting)

---

## 1. Installing

Download the zip for your system from the project's **Releases** page and copy the plugin files:

| System | Copy | To |
|---|---|---|
| macOS | `Obsidian.vst3`, `Obsidian.component` (AU), `Obsidian.clap` | `~/Library/Audio/Plug-Ins/VST3`, `.../Components`, `.../CLAP` |
| Windows | the `Obsidian.vst3` folder, `Obsidian.clap` | `C:\Program Files\Common Files\VST3\`, `...\CLAP\` |
| Linux | `Obsidian.vst3`, `Obsidian.clap` | `~/.vst3/`, `~/.clap/` |

**macOS:** if the build isn't notarized and macOS blocks it, open Terminal and run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Obsidian*
```

Rescan plugins in your DAW. Obsidian appears under **Epsilon Audio** as an instrument.

Projects saved with Obsidian 1.x open unchanged in 2.0.

---

## 2. A quick tour

![Obsidian](../screenshot.png)

- **Header:** the preset browser, **A / B** and **COPY**, **UNDO / REDO**, **LEARN** (MIDI Learn), **MIDI** on/off and **HELP**.
- **Top row:** **OSC 1**, **OSC 2**, **FILTER 1** and **FILTER 2**.
- **Middle row:** **ENV 1**, **ENV 2**, **AMP ENV**, **LFO 1**, **LFO 2** and **LFO 3**. Each has a coloured source chip at its top right.
- **Bottom row:**
  - **MODULATION:** chips for velocity, aftertouch, key, pitch bend and mod wheel, and the 24 slots
  - **VOICE:** routing, voices, unison, volume, glide and bend

---

## 3. The oscillators

Each oscillator has:

- **Wave:** **TRI** (soft, hollow), **SAW** (bright, buzzy) or **SQR** (square). All are band-limited, so high notes don't alias.
- **OCT** (±3), **SEMI** (±12) and **FINE** (±100 cents).
- **PW:** the pulse width of the square wave. It is greyed out for TRI and SAW.
- **LEVEL** and **PAN**.
- **PHASE SYNC:**
  - on: every note starts the oscillator at the same point, for consistent, punchy attacks
  - off: each note starts at a random point, which sounds looser and wider

**HARD SYNC** (on OSC 2) restarts oscillator 2 each time oscillator 1 starts a cycle. Tune oscillator 2 higher, or modulate its pitch, for the classic tearing sync sound.

---

## 4. The filters

Each filter can be switched on or off on its own.

**Styles.** Click the arrows on either side of the style name, or scroll over it:

| Style | Character |
|---|---|
| **Smooth** | Moderate overdrive, broad resonance |
| **Hot** | Saturating, with a screaming peak |
| **Raw** | Buzzy, self-oscillating |
| **Monolith** | The Monolith synth's filter, self-oscillating |

**Controls**
- **Type:** **LP**, **BP** or **HP**.
- **Slope:** 12 or 24 dB per octave.
- **FREQ:** 20 Hz to 20 kHz.
- **PEAK:** resonance.
- **PAN:** offsets the cutoff between the left and right channels.

The curve above the knobs shows the filter's response.

**Routing** (in the VOICE panel):
- **SERIAL:** both oscillators go through filter 1, then filter 2.
- **PARALLEL:** both oscillators feed both filters, and their outputs are summed.
- **SPLIT:** oscillator 1 feeds filter 1, and oscillator 2 feeds filter 2.

---

## 5. The envelopes

- **ENV 1** and **ENV 2** are for modulation. Each has:
  - **DELAY:** waits before the envelope starts, for example for delayed vibrato
  - **ATTACK**, **DECAY**, **SUSTAIN**, **RELEASE**
  - **INVERT:** turns the envelope upside down
- **AMP ENV** shapes each voice's volume. It has:
  - **ATTACK**, **HOLD**, **DECAY**, **SUSTAIN**, **RELEASE**; HOLD keeps the note at full level after the attack
  - **VELOCITY:** how much velocity changes the volume

Every envelope stage can itself be a modulation destination.

---

## 6. The LFOs

Each LFO has:

- **Wave:**
  - **TRI**, **SIN**, **SQR**
  - **RND:** glides smoothly between random values
  - **S&H:** jumps between random values
- **BALANCE:** for triangle and sine, it skews the shape from a falling ramp through symmetric (50%) to a rising ramp. For square, it sets the duty cycle.
- **Sync mode:**
  - **FREE:** RATE is in Hz, 0.01 to 50.
  - **STR**, **DOT**, **TRIP:** locked to the host tempo and song position, with straight, dotted or triplet note lengths from 16 bars to 1/64. RATE then picks the length.
- **NOTE SYNC:** restarts the LFO for every note, so each voice has its own LFO.

RATE and BALANCE can both be modulation destinations.

---

## 7. Modulation

**Drag and drop**
- Drag a **source chip** onto any knob. A slot is created with a sensible starting amount, and the knob gets a coloured ring.
- The chips for ENV 1, ENV 2, AMP, LFO 1, LFO 2 and LFO 3 sit on their own panels.
- **VEL** (velocity), **AT** (aftertouch, channel or polyphonic), **KEY** (key tracking), **BEND** (pitch bend) and **WHEEL** (mod wheel) sit on the MODULATION panel.

**Slots.** There are 24. Each one has:
- a **source**; click to pick one
- a **destination**; click to pick one
- an **amount**: turning it left inverts
- a **bypass** switch
- a **delete** button

**Destinations**
- oscillator pitch, PW, level and pan
- filter freq, peak and pan
- every envelope stage
- LFO rate and balance
- volume and unison detune
- **every slot's amount**

Because slot amounts are destinations too, one slot can control how much another slot modulates. In the Init sound, the mod wheel brings in vibrato this way: slots 5 and 6 send the wheel to the amounts of slots 3 and 4, which send LFO 1 to the pitch of each oscillator.

**Ranges:** filter frequency modulation reaches ±5 octaves, and pitch modulation ±24 semitones. With **KEY → Filter Freq** at 20%, the filter tracks the keyboard 1:1.

![Destination picker](../screenshot-modulation.png)

---

## 8. Voices

- **VOICES** (1-24): how many notes sound at once. At 1, Obsidian is monophonic, with last-note priority.
- **UNISON** (1-4): stacks voices on every note.
  - **DETUNE** spreads their pitch and **SPREAD** their stereo position.
  - The total is capped at 24 voices; the oldest note is retired first, released notes before held ones.
- **GLIDE:** slides between notes, in mono and poly.
  - **ALWAYS** glides on every note.
  - **LEGATO** glides only when notes overlap, and in mono doesn't restart the envelopes.
- **BEND:** the pitch-bend range, up to 24 semitones.
- **VOLUME:** the output level.
- The **sustain pedal** (CC64) holds notes. All Notes Off and All Sound Off are honoured.

---

## 9. Knobs

- **Drag** up or down; hold **Shift** for fine adjustment. Slot amounts are finer by default.
- **Grab the rim** of a large knob (LEVEL, FREQ, PEAK) to turn it in a circle.
- **Mouse wheel** turns a knob.
- **Double-click** to type a value: `1.2k`, `L40`, `-6`, `250ms`, `+7`. **Enter** applies it, **Esc** cancels.
- **Cmd/Ctrl-click** resets to the default.
- The line at the top right of the MODULATION panel shows the name and value of whatever is under the pointer.

---

## 10. A/B, undo and redo

- **A / B** hold two complete sounds, modulation included. Click the other letter to switch; the first time, B starts as a copy of A.
- **COPY** copies the current sound into the other slot.
- Every edit is an undo step, including new modulation, preset loads and A/B switches. Obsidian remembers the last 200.
  - Undo: **UNDO**, or Cmd/Ctrl-Z
  - Redo: **REDO**, or Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y

---

## 11. MIDI

**MIDI Learn**
1. Click **LEARN**.
2. Click a knob or switch.
3. Move a knob or fader on your controller.
4. Click **LEARN** again (or press Esc) to finish.

- Right-click a control to unassign it.
- **CLEAR ALL CCs** (in the MODULATION panel) removes them all.
- Assignments are saved with your project.
- A learned controller replaces that controller's usual job (for example, CC1 as the mod wheel).

**The MIDI button** switches all MIDI input off, notes included. Leave it on to play Obsidian.

---

## 12. Presets

There are 42 factory sounds in six groups, plus your own:

| Group | Presets |
|---|---|
| Bass | Synth Bass, Acid Bass, Sync Bass, Sub Bass, Reese, Pluck Bass, Wobble 1/8 |
| Lead | Sync Lead, PWM Lead, Screamer, Glide Lead, Whistle, Unison Lead, Chip Lead |
| Pad | Warm Pad, Glass Pad, Stereo Pad, Dark Pad, Sweep Pad, Choir, Evolving |
| Keys & Plucks | Pluck, Electric Piano, Clav, Bell, Organ, Harp, Stab |
| Brass & Strings | Brass, Soft Brass, Strings, PWM Strings, Sync Brass, Solo Cello |
| FX & Motion | S&H Filter, Gated Pad 1/16, Laser, Riser, Arp Motion, Alarm, Wind |

**Init** is a plain starting point, with key tracking, a filter envelope and mod-wheel vibrato already set up.

**Browsing**
- Click the preset name to open the browser.
- **◀ ▶**, or the mouse wheel over the name, step through factory and user presets.
- A `*` after the name means you have changed the sound since loading it.

**Your own presets**
- **SAVE CURRENT AS…** stores your own sounds, in one of these folders:
  - macOS: `~/Library/Audio/Presets/Epsilon Audio/Obsidian`
  - Windows: `%APPDATA%\Epsilon Audio\Obsidian\Presets`
  - Linux: `~/.config/epsilon-audio/obsidian/presets`
- Preset files are plain text and work on every system.
- To delete one, right-click it in the browser and click again to confirm.

---

## 13. Help and hover tips

- Rest the pointer on any control, chip, display or header button to see what it does and its current value.
- **HELP** opens the in-plugin pages. The switch at the bottom of the help window turns hover tips on or off.
- **FULL MANUAL (WEB)** opens this manual.

---

## 14. Recipes

- **Sync lead:**
  - both oscillators SAW, HARD SYNC on, OSC 2 SEMI +7
  - drag ENV 1 onto OSC 2's SEMI knob, then raise its slot amount for the sweep
- **Supersaw pad:**
  - UNISON 4, DETUNE 30%, SPREAD 80%
  - slow AMP ENV attack and release
  - LFO 1 (SIN, slow) dragged onto FILTER 1 FREQ
- **Delayed vibrato:** ENV 2 with DELAY 0.5 s and a slow attack. Drag it onto a slot's amount whose source is LFO 1 → Osc pitch.
- **Gated pad:** LFO 2 SQR, STR 1/16, dragged onto VOLUME with a negative amount.
- **Split stack:** routing SPLIT, OSC 1 into a low-pass, OSC 2 into a high-pass, panned apart.
- **Velocity brightness:** drag VEL onto FILTER 1 FREQ.

---

## 15. Parameter reference

Every control is automatable. New parameters are only ever added at the end, so sessions and automation keep working across versions.

- **Each oscillator:**
  - Wave (Triangle, Saw, Square)
  - Octave (±3), Semitone (±12), Fine (±100 ct), Pulse Width
  - Level, Pan, Phase Sync
- **Osc 2 Hard Sync.**
- **Each filter:**
  - On, Style (Smooth, Hot, Raw, Monolith)
  - Type (Low Pass, Band Pass, High Pass), Slope (12, 24 dB/oct)
  - Freq (20 Hz-20 kHz), Peak, Pan
- **Filter Routing:** Serial, Parallel, Split.
- **Env 1 and Env 2:**
  - Delay (0-5 s), Attack (0-10 s), Decay, Sustain, Release (up to 10 s)
  - Invert
- **Amp envelope:**
  - Attack, Hold (0-5 s), Decay, Sustain, Release
  - Velocity
- **Each LFO:**
  - Wave (Triangle, Sine, Square, Random, Sample & Hold), Balance
  - Sync (Free, Straight, Dotted, Triplet), Rate (0.01-50 Hz), Division (16 bars-1/64)
  - Note Sync
- **Voice:**
  - Voices (1-24), Unison (1-4), Unison Detune, Unison Spread
  - Glide (0-2 s), Glide Mode (Always, Legato)
  - Bend Range (0-24 st), Volume (-36 to +6 dB)
- **24 modulation slots,** each with Source, Destination, Amount (±100%) and Bypass.
- **MIDI:** on/off.

Obsidian also reports one output value that hosts may list; it only passes MIDI Learn messages to the interface.

---

## 16. Troubleshooting

- **Obsidian doesn't appear in my DAW.** Check the install folders above and rescan plugins. On macOS, run the `xattr` command from section 1.
- **No sound.** Check that the MIDI button is on and that the track sends MIDI to Obsidian. Check VOLUME, the oscillator LEVELs, and that the filters aren't closed all the way.
- **A knob doesn't move when the sound changes.** It may be modulated: the coloured ring shows by how much. Bypass or delete its slot in the MODULATION panel.
- **Can't add more modulation.** All 24 slots are in use; delete one first.
- **The LFO doesn't follow the tempo.** Choose STR, DOT or TRIP instead of FREE.
- **Hover tips get in the way.** Turn them off with the switch at the bottom of the HELP window.
- **Latency:** Obsidian has none.
