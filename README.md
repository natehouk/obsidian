# Obsidian

**A polyphonic four-oscillator, four-filter synthesizer plugin with effects and an arpeggiator, for macOS, Windows and Linux (VST3 · AU · CLAP · VST2).**

Part of **[Epsilon Audio](https://github.com/natehouk/epsilon-plugins)**, a family of free, open-source plugins: Monolith, Caldera, Obsidian, Strata, Basalt, Gneiss, Mica, Flint, Granite, Magma, Schist, Slate, Talc, Geode and Malachite.

![Obsidian](screenshot.png)

Obsidian is a clean-room design with the feature set of FabFilter Twin 3 (2023). Versions 1 and 2 followed the original Twin (2005); version 3 follows Twin 3. It was written from scratch, working only from FabFilter's public product pages, release announcements and contemporary reviews. No FabFilter code, binaries, presets, artwork or trademarks were used.

It is the third instrument in a family:
- [Monolith](https://github.com/natehouk/monolith), a synth
- [Caldera](https://github.com/natehouk/caldera), a filter effect
- **Obsidian**, which shares Caldera's filter styles (Monolith's filter is one of them) and the Epsilon modulation system (XLFOs, envelopes, followers, XY pads) with Caldera, Strata and Magma.

## Download

Get the latest build from [**Releases**](../../releases):

| Platform | File | Contains |
|---|---|---|
| macOS 10.13+ (Apple Silicon + Intel) | `Obsidian-macos-universal.zip` | VST3, AU, CLAP, manual |
| Windows x64 | `Obsidian-windows-x64.zip` | VST3, CLAP, VST2, manual |
| Linux x64 | `Obsidian-linux-x64.zip` | VST3, CLAP, VST2, manual |

### Install

| Platform | Copy | To |
|---|---|---|
| macOS | `Obsidian.vst3` / `Obsidian.component` / `Obsidian.clap` | `~/Library/Audio/Plug-Ins/VST3` / `Components` / `CLAP` |
| Windows | `Obsidian.vst3` (folder) / `Obsidian.clap` | `C:\Program Files\Common Files\VST3\` / `CLAP\` |
| Linux | `Obsidian.vst3` / `Obsidian.clap` | `~/.vst3/` / `~/.clap/` |

**macOS:** builds are notarized once the project's Apple signing credentials are set up. Until then they are ad-hoc signed; after copying, run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Obsidian*
```

## Features

**Oscillators (4)**
- **Triangle, saw, square, sine, white noise or pink noise**; the tones are band-limited.
- **Octave** (±3), **semitone** (±12), **fine** (±100 cents), **level**, stereo **pan**, **pulse width**.
- **Sync** knob: a hard-sync ratio from ×1 to ×16 against a hidden master at the note's pitch, so any oscillator can do sync sweeps on its own. Obsidian 2's osc 1 → osc 2 hard sync is still there.
- **Phase sync** per oscillator, **ring modulation** between 1–2 and 3–4, analog **drift**, master **tune** and **transpose**.

**Filters (4)**
- **Eleven styles:** Smooth, Hot, Raw and Monolith from Obsidian 2, plus Clean, Warm, Acid, Tube, Gentle, Metal and Hollow.
- **Low / band / high pass, bell, low and high shelf, notch**, at **6, 12, 24 or 48 dB/oct**, with **freq**, **peak**, **gain** and stereo **pan**.
- **Routing:** serial, parallel or **per oscillator** (oscillator *n* into filter *n*).
- **Freq and peak offsets** that move all four filters at once.
- An interactive display: drag handles for frequency and peak/gain, double-click to add a filter, right-click to remove one.
- **HQ mode:** 4× oversampled voices for alias-free screaming filters and sync (about 0.5 ms latency, reported to the host).

**Effects:** reverb (0.2–20 s, brightness, predelay), delay (free or synced, stereo offset, feedback, ping-pong, low and high cut), chorus (single or double), phaser / flanger, drive (pre or post) and a compressor (threshold, ratio, attack, release, auto gain, parallel amount).

**Arpeggiator:** up, down, up/down, as played or random; up to 3 extra octaves; free rate (0.25–20 Hz) or synced with a ×0.5–×2 offset; groove; legato (note length); latch off / held / full; retrigger; **lock**, which keeps the arpeggiator when you change presets; and its own section presets.

**Voices**
- Up to **64 voices**, **unison** up to 8 with detune and stereo spread.
- **Glide** in mono and poly, always or legato.
- **Per oscillator** voice mode: each new note goes to the next oscillator, so chords are spread across them.
- Pitch-bend range up to ±24 st, sustain pedal (CC64), all-notes-off and all-sound-off.

**Modulation: drag and drop, up to 100 slots**
- **Sources:** 4 **XLFOs** (seven waves or a 16-step sequencer with glide; free or synced; per note or shared), 2 **envelope generators** with draggable curve handles and legato triggering, 2 **envelope followers** (per voice or whole synth, with a transient mode), 2 **XY pads**, and **velocity, key, mod wheel, pitch bend and aftertouch**. Obsidian 2's three LFOs, two envelopes and the amp envelope remain as classic sources with their 24 slots.
- Drag a **source chip** onto any knob: the knob gets a coloured ring showing the range and a live dot showing the modulation. Right-click a knob to list and edit what modulates it.
- Every slot has source, destination, a bipolar amount, bypass and delete; every slot amount is itself a destination.
- **Destinations** include every oscillator and filter parameter, tune, glide, unison spread, the effects, the arpeggiator rate, the XLFOs, the EGs and every slot amount.
- **Source presets:** load or save a single XLFO, EG or EF.

![Modulation](screenshot-modulation.png)

**Workflow**
- **A/B** with copy, and **undo/redo** for every edit, including new modulation, preset loads and A/B switches.
- **Hover tips** everywhere, a **HELP window**, and a **[full manual](docs/MANUAL.md)** (also in every release zip as `Obsidian-Manual.html`).
- **MIDI Learn:**
  1. Click **LEARN**.
  2. Click a control, then move a knob or fader on your controller.
  3. Right-click a control to unassign it, or use **Clear all CCs**.

  Assignments are saved with your project.
- **MIDI** switch: turns all MIDI handling off.
- **60 factory presets** in eight categories: Bass, Lead, Pad, Keys & Plucks, Brass & Strings, FX & Motion, Arp & Sequence and Texture. The browser **searches** names and categories as you type, and a star keeps a preset in **Favourites**.
- **Save your own presets** as small text files, which load the same on every OS:
  - macOS: `~/Library/Audio/Presets/Epsilon Audio/Obsidian/`
  - Windows: `%APPDATA%\Epsilon Audio\Obsidian\Presets\`
  - Linux: `~/.config/epsilon-audio/obsidian/presets/`
- To delete a user preset, right-click it in the menu, then click it again to confirm.
- **Resizable** window, double-click to **type a value**, **Cmd/Ctrl-click** to reset a control, grab the rim of a large knob to turn it in a circle.
- **Compatible:** sessions and presets from Obsidian 1.x and 2.x load unchanged and sound identical.

![Presets](screenshot-presets.png)

![Help](screenshot-help.png)

### Compared with FabFilter Twin 3

| Twin 3 | Obsidian 3.0 |
|---|---|
| 4 oscillators: saw, square, triangle, sine, noise; pure variants | ✓ triangle, saw, square, sine, white and pink noise, band-limited |
| Detune, scale, sync knob, phase sync, level/pan per oscillator | ✓ octave/semitone/fine, sync ratio ×1–×16, phase sync, level, pan, PW |
| Ring modulation, analog drift | ✓ ring 1×2 and 3×4, drift |
| Master tune, portamento | ✓ tune, transpose, glide (always or legato) |
| 4 filters, 11 styles; LP/BP/HP/bell/shelves/notch; 6–48 dB; frequency pan | ✓ eleven original styles, all shapes and slopes, gain, pan |
| Serial, parallel and per-oscillator filter routing; freq/peak offsets | ✓ |
| HQ (oversampling) mode | ✓ 4× |
| Reverb, delay, chorus, phaser/flanger, drive, compressor | ✓ |
| Arpeggiator: order, octaves, rate/sync, groove, legato, latch, retrigger, lock, section presets | ✓ |
| Up to 64 voices, unison, per-oscillator voice mode | ✓ |
| XLFOs (step sequencer), EGs with curves, EFs, XY controllers, MIDI sources | ✓ 4 XLFOs, 2 EGs, 2 EFs (with transient mode), 2 XY pads, MIDI |
| Drag-and-drop modulation with visual feedback, up to 100 slots | ✓ 100 slots plus Obsidian 2's 24, live rings, per-knob slot list |
| MIDI Learn, undo/redo, A/B, help and interactive hints | ✓ |
| Preset browser | ✓ categories, search, favourites, user presets |
| Factory presets; loads earlier presets | 60, plus your own; loads Obsidian 1.x and 2.x projects and presets, sounding identical |
| Resizable interface, full-screen mode | Freely resizable interface |
| AAX, AudioSuite | Not supported (needs Avid's SDK and signing) |

## Build from source

The plugin is built on [DPF](https://github.com/DISTRHO/DPF):

```
git clone --recursive https://github.com/DISTRHO/DPF.git dpf
make -C plugins/Obsidian                    # Linux / macOS (macOS also builds the AU)
make -C plugins/Obsidian WINDOWS=true CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++
```

GitHub Actions builds all three platforms on every push. The macOS build is universal. It is signed and notarized when the Developer ID secrets are set (`MACOS_CERT_P12`, `MACOS_CERT_PASSWORD`, `NOTARY_KEY`, `NOTARY_KEY_ID`, `NOTARY_ISSUER`), and ad-hoc signed otherwise. A `v*` tag, or a release published on GitHub, attaches the zips to that release.

## Tests

```
cd tests
for t in features features3 compat fuzz presets modpresets; do g++ -O2 -std=c++17 -I../plugins/Obsidian $t.cpp -o $t && ./$t; done
g++ -O3 -ffast-math -std=c++17 -I../plugins/Obsidian bench.cpp -o bench && ./bench
```

- `features`: voices, unison and stealing, mono, hard and phase sync, routing, slots, slot-on-slot, delay, hold, S&H, filter styles, glide (the 2.x behaviour)
- `features3`: the 3.0 features: four oscillators, noise, ring, sync ratio, tune, drift, 64 voices, unison 8, per-oscillator mode, four filters, slopes, shapes, offsets, per-oscillator routing, styles, HQ aliasing, every effect, the arpeggiator and latch, XLFOs, EG curves, XY, followers, global destinations, 100 slots
- `compat`: plays random 2.x sounds and MIDI through the 3.0 engine and a frozen copy of the 2.0 engine (`tests/legacy/`); the outputs must match to better than -120 dB (they are identical)
- `fuzz`: random parameters, MIDI and transport at 44.1-192 kHz: finite output, at most 0 dBFS
- `presets`: plays every factory preset and checks its level
- `modpresets`: the source presets and their files
- `bench`: CPU

CPU at 48 kHz on one modern x86 core, all voices sounding:

| Load | CPU |
|---|---|
| 2.x: 24 voices, 2 oscillators, both 24 dB filters, 8 slots | 20% |
| 3.0: 16 voices, 4 oscillators, 4 filters, 20 slots, every effect | 29% |
| The same with HQ (4× oversampling) | 79% |
| 3.0: 64 voices, 4 oscillators, 4 filters, 20 slots, every effect | 104% |

Normal playing uses much less: only sounding voices, and only the oscillators and filters that are on, cost CPU.

Release checks:
- Steinberg's VST3 validator passes 47/47.
- The interface is exercised with scripted input and MIDI, including drag-and-drop modulation, the filter handles, the step and curve editors, the slot pickers, the source tabs, the effects, the arpeggiator, MIDI Learn, presets, A/B, undo and redo, hover tips and help.

## Source layout

| File | Contents |
|---|---|
| `plugins/Obsidian/ObsidianEngine.hpp` | All DSP: voices, oscillators, filters, envelopes, LFOs, modulation, the 2.x render path and the 3.0 render path. Header-only |
| `plugins/Obsidian/SynthFx.hpp` | Reverb, delay, chorus, phaser/flanger, drive and compressor |
| `plugins/Obsidian/Arp.hpp` | Arpeggiator |
| `plugins/Obsidian/ParamDefs.hpp` | Parameter table, labels, modulation source and destination names, value mapping |
| `plugins/Obsidian/Presets.hpp` | Factory presets |
| `plugins/Obsidian/UserPresets.hpp` | User preset files |
| `plugins/Obsidian/ComponentPresets.hpp` | Factory and user presets for single modulation sources |
| `plugins/Obsidian/common/` | Shared Epsilon Audio code (modulation system v2, oversampling), vendored from epsilon-common |
| `plugins/Obsidian/Help.hpp` | Hover tip text and in-plugin help pages |
| `plugins/Obsidian/ObsidianPlugin.hpp/.cpp` | Plugin wrapper: parameters, programs, MIDI, MIDI Learn, transport, latency |
| `plugins/Obsidian/ObsidianUI.cpp` | Custom vector interface (NanoVG) |
| `docs/MANUAL.md` | User manual (`Obsidian-Manual.html` is generated from it with pandoc) |
| `tests/legacy/` | Frozen copy of the 2.0 engine, for the compatibility test |

New parameters are only ever appended, so saved sessions and automation keep working across versions.

## Changelog

- **3.1:** arpeggiator presets (as in Twin 3); the preset browser searches names and categories as you type, and keeps Favourites.
- **3.0:** Four oscillators with sine, white and pink noise, a sync-ratio knob, ring modulation, drift, master tune and transpose; four filters with seven new styles, bell, shelf and notch shapes, 6 and 48 dB slopes, gain, per-oscillator routing, freq/peak offsets and an interactive display; 4× HQ mode; reverb, delay, chorus, phaser/flanger, drive and compressor; an arpeggiator; up to 64 voices, unison 8 and a per-oscillator voice mode; the Epsilon modulation system (4 XLFOs with step sequencers, 2 curved envelopes, 2 followers, 2 XY pads, MIDI sources, 100 slots, per-source presets); a new interface; 18 new factory presets. Sessions and presets from 1.x and 2.x load unchanged and sound identical.
- **2.0:** A/B with copy, undo/redo, hover tips, a help window and a full manual (in every release zip), rim-grab turning on the large knobs. Sessions and automation from 1.x load unchanged.
- **1.0:** First release.

## License

[MIT](LICENSE). Built with [DPF](https://github.com/DISTRHO/DPF) and its bundled NanoVG, which are under the ISC license. `plugins/Obsidian/ResizeHandle.hpp` comes from DPF's examples and keeps its ISC notice.

"FabFilter" and "FabFilter Twin" are trademarks of FabFilter Software Instruments. Obsidian is not affiliated with or endorsed by FabFilter.
