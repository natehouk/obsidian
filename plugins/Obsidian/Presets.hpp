// Factory presets (each lists only what it changes from the engine defaults). License: MIT.
// Engine defaults include six slots: Key -> F1 Freq, Env 1 -> F1 Freq, LFO 1 -> both osc pitches
// (amount 0) and Mod Wheel -> those two amounts, i.e. the mod wheel adds vibrato.
#pragma once
#include "ObsidianEngine.hpp"
#include <algorithm>

namespace obsidian {

struct PV { uint32_t id; float v; };
struct PresetDef { const char* name; const char* category; const PV* pv; };
struct Preset { const char* name; const char* category; float v[kParamCount]; };

static const char* const kCategories[] = { "Bass", "Lead", "Pad", "Keys & Plucks", "Brass & Strings", "FX & Motion", "Arp & Sequence", "Texture" };
static constexpr int kNumCategories = 8;

#define SL(i, s, d, a) { kSlotFirst + (i) * 4 + 0, (float)(s) }, { kSlotFirst + (i) * 4 + 1, (float)(d) }, { kSlotFirst + (i) * 4 + 2, (a) }, { kSlotFirst + (i) * 4 + 3, 0 }
#define OFF(i) { kSlotFirst + (i) * 4 + 0, 0 }, { kSlotFirst + (i) * 4 + 1, 0 }, { kSlotFirst + (i) * 4 + 2, 0 }
#define END { kParamCount, 0 }
#define S2(i, s, d, a) { sp(i, kSlotSrc), (float)(s) }, { sp(i, kSlotDst), (float)(d) }, { sp(i, kSlotAmt), (a) }
#define ON(o) { kOscOnFirst + (o), 1 }
#define STEPS(l, a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) { lp_(l, em::kLStep1), a0 }, { lp_(l, em::kLStep1 + 1), a1 }, { lp_(l, em::kLStep1 + 2), a2 }, { lp_(l, em::kLStep1 + 3), a3 }, \
    { lp_(l, em::kLStep1 + 4), a4 }, { lp_(l, em::kLStep1 + 5), a5 }, { lp_(l, em::kLStep1 + 6), a6 }, { lp_(l, em::kLStep1 + 7), a7 }, { lp_(l, em::kLStep1 + 8), a8 }, { lp_(l, em::kLStep1 + 9), a9 }, \
    { lp_(l, em::kLStep1 + 10), a10 }, { lp_(l, em::kLStep1 + 11), a11 }, { lp_(l, em::kLStep1 + 12), a12 }, { lp_(l, em::kLStep1 + 13), a13 }, { lp_(l, em::kLStep1 + 14), a14 }, { lp_(l, em::kLStep1 + 15), a15 }
namespace pv {
static const PV init[] = { END };

// ---------------------------------------------------------------- Bass (mono-ish)
static const PV synthBass[] = { {kVoices,1},{kO1Wave,1},{kO2Wave,2},{kO2Level,.6f},{kO2Oct,-1},{kO2Fine,0},{kF1Style,3},{kF1Freq,350},{kF1Peak,.35f},
    {kE1Decay,.25f},{kE1Sustain,.1f},{kE3Release,.08f},SL(1,kSrcEg1,kDstF1Freq,.45f), END };
static const PV acidBass[] = { {kVoices,1},{kGlideMode,1},{kGlide,.07f},{kO1Wave,1},{kO2Level,0},{kO1Oct,-1},{kF1Style,3},{kF1Freq,300},{kF1Peak,.82f},
    {kE1Attack,0},{kE1Decay,.2f},{kE1Sustain,0},{kE3Sustain,1},{kE3Release,.05f},SL(1,kSrcEg1,kDstF1Freq,.55f),SL(6,kSrcVelocity,kDstF1Peak,.15f), END };
static const PV syncBass[] = { {kVoices,1},{kO1Wave,1},{kO2Wave,1},{kO2Level,1},{kO1Level,.3f},{kOscSync,1},{kO2Semi,7},{kO1Oct,-1},{kO2Oct,-1},
    {kF1Freq,900},{kF1Peak,.3f},{kE1Decay,.3f},{kE1Sustain,0},SL(6,kSrcEg1,kDstO2Pitch,.35f),{kE3Release,.08f}, END };
static const PV subBass[] = { {kVoices,1},{kO1Wave,0},{kO1Oct,-1},{kO2Wave,2},{kO2Oct,-2},{kO2Level,.5f},{kO2Fine,0},{kF1Freq,500},{kF1Peak,.1f},
    {kE3Attack,.002f},{kE3Sustain,1},{kE3Release,.1f},OFF(1), END };
static const PV reeseBass[] = { {kVoices,1},{kUnison,3},{kUniDetune,.35f},{kUniSpread,.6f},{kO1Wave,1},{kO2Wave,1},{kO2Level,.8f},{kO2Fine,-12},{kO1Oct,-1},{kO2Oct,-1},
    {kF1Style,1},{kF1Freq,600},{kF1Peak,.3f},{kL2Rate,.2f},SL(6,kSrcLfo2,kDstF1Freq,.12f),OFF(1),{kE3Release,.15f}, END };
static const PV pluckBass[] = { {kVoices,1},{kO1Wave,2},{kO1PW,.3f},{kO2Level,0},{kO1Oct,-1},{kF1Style,2},{kF1Freq,260},{kF1Peak,.5f},{kVolume,-2},
    {kE1Attack,0},{kE1Decay,.12f},{kE1Sustain,0},{kE3Decay,.35f},{kE3Sustain,.2f},{kE3Release,.08f},SL(1,kSrcEg1,kDstF1Freq,.6f), END };
static const PV wobbleBass[] = { {kVoices,1},{kO1Wave,1},{kO2Wave,2},{kO2Level,.7f},{kO2Oct,-1},{kO1Oct,-1},{kF1Style,1},{kF1Freq,250},{kF1Peak,.6f},
    {kL2Wave,0},{kL2Sync,1},{kL2Div,7},SL(6,kSrcLfo2,kDstF1Freq,.45f),OFF(1),{kE3Sustain,1},{kE3Release,.08f}, END };

// ---------------------------------------------------------------- Lead
static const PV syncLead[] = { {kVoices,1},{kGlide,.06f},{kO1Wave,1},{kO2Wave,1},{kO1Level,.2f},{kO2Level,1},{kOscSync,1},{kO2Semi,12},
    {kF1Freq,4000},{kF1Peak,.25f},{kE2Attack,.5f},{kE2Decay,1.5f},{kE2Sustain,.3f},SL(6,kSrcEg2,kDstO2Pitch,.25f),SL(7,kSrcWheel,kDstO2Pitch,.3f), END };
static const PV squareLead[] = { {kVoices,1},{kGlide,.05f},{kO1Wave,2},{kO2Wave,2},{kO2Level,.7f},{kO2Fine,8},{kO1PW,.4f},{kF1Freq,3000},{kF1Peak,.3f},
    {kL2Rate,.7f},SL(6,kSrcLfo2,kDstO1PW,.4f),SL(7,kSrcLfo2,kDstO2PW,-.4f),{kE3Attack,.01f}, END };
static const PV screamLead[] = { {kVoices,1},{kO1Wave,1},{kO2Wave,1},{kO2Level,.9f},{kO2Fine,12},{kF1Style,1},{kF1Freq,1800},{kF1Peak,.75f},{kF1Slope,0},
    {kL1Rate,5.5f},SL(2,kSrcLfo1,kDstO1Pitch,.006f),SL(3,kSrcLfo1,kDstO2Pitch,.006f),SL(6,kSrcAftertouch,kDstF1Freq,.3f), END };
static const PV glideLead[] = { {kVoices,1},{kGlideMode,1},{kGlide,.25f},{kO1Wave,0},{kO2Wave,1},{kO2Level,.4f},{kO2Oct,1},{kF1Freq,2500},{kF1Peak,.35f},{kE3Release,.35f}, END };
static const PV whistleLead[] = { {kVoices,1},{kGlide,.12f},{kO1Wave,0},{kO2Level,0},{kO1Oct,1},{kF1Style,3},{kF1Freq,3000},{kF1Peak,.3f},
    {kE3Attack,.06f},{kE3Release,.4f},{kL1Rate,5},SL(2,kSrcLfo1,kDstO1Pitch,.008f),OFF(1),{kVolume,-3}, END };
static const PV unisonLead[] = { {kVoices,2},{kUnison,4},{kUniDetune,.25f},{kUniSpread,.8f},{kO1Wave,1},{kO2Wave,1},{kO2Level,.6f},{kO2Oct,1},
    {kF1Freq,3500},{kF1Peak,.2f},{kE3Release,.3f}, END };
static const PV chipLead[] = { {kVoices,1},{kO1Wave,2},{kO1PW,.25f},{kO2Level,0},{kF1On,0},{kE3Attack,0},{kE3Decay,.15f},{kE3Sustain,.7f},{kE3Release,.05f},
    {kL3Wave,2},{kL3Sync,1},{kL3Div,9},SL(6,kSrcLfo3,kDstO1PW,.2f),OFF(0),OFF(1), END };

// ---------------------------------------------------------------- Pad
static const PV warmPad[] = { {kVoices,12},{kO1Wave,1},{kO2Wave,1},{kO2Level,.8f},{kO2Fine,9},{kF1Freq,900},{kF1Peak,.2f},
    {kE1Attack,1.5f},{kE1Decay,2},{kE1Sustain,.6f},{kE1Release,2},{kE3Attack,1},{kE3Release,2.2f},{kL2Rate,.15f},SL(6,kSrcLfo2,kDstF1Freq,.08f), END };
static const PV glassPad[] = { {kVoices,12},{kO1Wave,0},{kO2Wave,2},{kO2Oct,1},{kO2Level,.4f},{kO2PW,.2f},{kF1Style,3},{kF1Freq,3500},{kF1Peak,.45f},
    {kE3Attack,1.2f},{kE3Release,3},{kL3Rate,.25f},{kL3Wave,1},SL(6,kSrcLfo3,kDstO2PW,.3f),OFF(1), END };
static const PV stereoPad[] = { {kVoices,8},{kUnison,2},{kUniDetune,.2f},{kUniSpread,1},{kO1Wave,1},{kO2Wave,0},{kO2Level,.6f},{kRouting,2},{kF2On,1},
    {kF2Type,0},{kF2Freq,1500},{kF1Freq,700},{kF1Pan,-.3f},{kF2Pan,.3f},{kE3Attack,.8f},{kE3Release,2.5f},{kL2Rate,.1f},SL(6,kSrcLfo2,kDstF1Pan,.4f),SL(7,kSrcLfo2,kDstF2Pan,-.4f), END };
static const PV darkPad[] = { {kVoices,12},{kO1Wave,1},{kO2Wave,2},{kO2Oct,-1},{kO2Level,.6f},{kF1Style,0},{kF1Freq,400},{kF1Peak,.4f},
    {kE1Attack,3},{kE1Decay,4},{kE1Sustain,.3f},{kE3Attack,1.5f},{kE3Release,3},SL(1,kSrcEg1,kDstF1Freq,.3f), END };
static const PV sweepPad[] = { {kVoices,12},{kO1Wave,1},{kO2Wave,1},{kO2Fine,-10},{kO2Level,.8f},{kF1Style,3},{kF1Freq,250},{kF1Peak,.55f},
    {kL2Sync,1},{kL2Div,2},{kL2Wave,1},SL(6,kSrcLfo2,kDstF1Freq,.35f),{kE3Attack,.6f},{kE3Release,2}, END };
static const PV choirPad[] = { {kVoices,12},{kO1Wave,0},{kO2Wave,1},{kO2Level,.35f},{kRouting,1},{kF2On,1},{kF1Type,1},{kF2Type,1},{kF1Freq,700},{kF2Freq,1150},
    {kF1Peak,.6f},{kF2Peak,.6f},{kF1Slope,0},{kF2Slope,0},{kVolume,0},{kE3Attack,.9f},{kE3Release,1.8f},{kL1Rate,5},SL(2,kSrcLfo1,kDstO1Pitch,.004f),SL(3,kSrcLfo1,kDstO2Pitch,.004f),OFF(0),OFF(1), END };
static const PV evolvePad[] = { {kVoices,12},{kO1Wave,1},{kO2Wave,2},{kO2Level,.7f},{kF1Freq,800},{kF1Peak,.4f},{kL3Wave,3},{kL3Rate,.3f},{kL2Rate,.07f},
    SL(6,kSrcLfo3,kDstF1Freq,.2f),SL(7,kSrcLfo2,kDstO2PW,.35f),SL(8,kSrcLfo2,kDstSlotFirst + 6,.2f),{kE3Attack,1.2f},{kE3Release,3}, END };

// ---------------------------------------------------------------- Keys & Plucks
static const PV pluck[] = { {kVoices,12},{kO1Wave,2},{kO1PW,.25f},{kO2Wave,1},{kO2Level,.4f},{kF1Style,3},{kF1Freq,250},{kF1Peak,.5f},
    {kE1Attack,0},{kE1Decay,.18f},{kE1Sustain,0},{kE3Decay,.5f},{kE3Sustain,0},{kE3Release,.35f},SL(1,kSrcEg1,kDstF1Freq,.7f),SL(6,kSrcVelocity,kDstF1Freq,.15f), END };
static const PV electricPiano[] = { {kVoices,12},{kO1Wave,0},{kO2Wave,0},{kO2Oct,2},{kO2Level,.25f},{kF1Freq,2500},{kF1Peak,.1f},{kE3Attack,.002f},{kE3Decay,1.8f},
    {kE3Sustain,.15f},{kE3Release,.5f},{kAmpVel,.7f},{kE2Attack,0},{kE2Decay,.2f},{kE2Sustain,0},SL(6,kSrcEg2,kDstO2Level,.5f),{kL1Rate,4},SL(7,kSrcLfo1,kDstVolume,.08f), END };
static const PV clav[] = { {kVoices,12},{kO1Wave,2},{kO1PW,.15f},{kO2Level,0},{kF1Type,1},{kF1Slope,0},{kF1Style,2},{kF1Freq,900},{kF1Peak,.35f},{kVolume,5},{kE1Attack,0},{kE1Decay,.12f},
    {kE1Sustain,0},{kE3Decay,.4f},{kE3Sustain,.25f},{kE3Release,.06f},SL(1,kSrcEg1,kDstF1Freq,.3f), END };
static const PV bell[] = { {kVoices,16},{kO1Wave,0},{kO2Wave,0},{kO2Semi,7},{kO2Oct,1},{kO2Fine,13},{kO2Level,.6f},{kF1Freq,6000},{kF1Peak,.2f},
    {kE3Attack,.001f},{kE3Decay,2.5f},{kE3Sustain,0},{kE3Release,2},OFF(1), END };
static const PV organ[] = { {kVoices,16},{kO1Wave,0},{kO2Wave,0},{kO2Oct,1},{kO2Level,.6f},{kO2Fine,0},{kF1Freq,5000},{kF1Peak,0},{kE3Attack,.004f},
    {kE3Sustain,1},{kE3Release,.05f},{kAmpVel,0},OFF(1),{kL1Rate,6.5f},SL(2,kSrcLfo1,kDstO1Pitch,.002f),SL(3,kSrcLfo1,kDstO2Pitch,.002f), END };
static const PV harp[] = { {kVoices,16},{kO1Wave,1},{kO2Wave,0},{kO2Oct,1},{kO2Level,.4f},{kF1Freq,1500},{kF1Peak,.25f},{kE1Attack,0},{kE1Decay,.4f},{kE1Sustain,0},
    {kE3Attack,.001f},{kE3Decay,1.4f},{kE3Sustain,0},{kE3Release,1.2f},{kE3Hold,.05f}, END };
static const PV stab[] = { {kVoices,12},{kUnison,2},{kUniDetune,.3f},{kO1Wave,1},{kO2Wave,1},{kO2Level,.8f},{kF1Style,1},{kF1Freq,700},{kF1Peak,.45f},
    {kE1Attack,0},{kE1Decay,.25f},{kE1Sustain,.1f},{kE3Decay,.3f},{kE3Sustain,.2f},{kE3Release,.2f},SL(1,kSrcEg1,kDstF1Freq,.5f), END };

// ---------------------------------------------------------------- Brass & Strings
static const PV brass[] = { {kVoices,8},{kO1Wave,1},{kO2Wave,1},{kO2Fine,6},{kO2Level,.9f},{kF1Freq,500},{kF1Peak,.2f},{kE1Attack,.08f},{kE1Decay,.5f},
    {kE1Sustain,.5f},{kE3Attack,.03f},{kE3Release,.25f},SL(1,kSrcEg1,kDstF1Freq,.4f),SL(6,kSrcVelocity,kDstF1Freq,.15f), END };
static const PV softBrass[] = { {kVoices,8},{kO1Wave,1},{kO2Wave,2},{kO2Level,.5f},{kF1Freq,700},{kF1Peak,.15f},{kE1Attack,.25f},{kE1Decay,1},{kE1Sustain,.4f},
    {kE3Attack,.15f},{kE3Release,.5f},SL(1,kSrcEg1,kDstF1Freq,.25f),SL(6,kSrcAftertouch,kDstF1Freq,.25f), END };
static const PV strings[] = { {kVoices,16},{kUnison,2},{kUniDetune,.18f},{kUniSpread,.7f},{kO1Wave,1},{kO2Wave,1},{kO2Fine,-7},{kO2Level,.8f},{kF1Freq,3000},
    {kF1Peak,.1f},{kE3Attack,.4f},{kE3Release,1},{kL1Rate,5.2f},SL(2,kSrcLfo1,kDstO1Pitch,.003f),SL(3,kSrcLfo1,kDstO2Pitch,.003f),OFF(1), END };
static const PV pwmStrings[] = { {kVoices,16},{kO1Wave,2},{kO2Wave,2},{kO2Level,.8f},{kO2Fine,5},{kL2Rate,.5f},SL(6,kSrcLfo2,kDstO1PW,.35f),SL(7,kSrcLfo2,kDstO2PW,-.35f),
    {kF1Freq,2200},{kF1Peak,.1f},{kE3Attack,.35f},{kE3Release,.9f},OFF(1), END };
static const PV syncBrass[] = { {kVoices,8},{kO1Wave,1},{kO2Wave,1},{kOscSync,1},{kO2Semi,5},{kO2Level,.8f},{kO1Level,.6f},{kF1Freq,1200},{kF1Peak,.2f},
    {kE2Attack,.05f},{kE2Decay,.4f},{kE2Sustain,0},SL(6,kSrcEg2,kDstO2Pitch,.2f),{kE3Attack,.03f},{kE3Release,.3f}, END };
static const PV cello[] = { {kVoices,1},{kGlide,.08f},{kGlideMode,1},{kO1Wave,1},{kO2Wave,2},{kO2Level,.4f},{kO1Oct,-1},{kO2Oct,-1},{kF1Freq,1400},{kF1Peak,.25f},
    {kE3Attack,.15f},{kE3Release,.4f},{kL1Rate,5},{kL1NoteSync,1},{kE2Delay,.4f},{kE2Attack,.5f},{kE2Sustain,1},SL(6,kSrcEg2,kDstSlotFirst + 2,.006f),SL(7,kSrcEg2,kDstSlotFirst + 3,.006f), END };

// ---------------------------------------------------------------- FX & Motion
static const PV sAndH[] = { {kVoices,8},{kO1Wave,1},{kO2Level,0},{kF1Style,3},{kF1Freq,800},{kF1Peak,.7f},{kL3Wave,4},{kL3Sync,1},{kL3Div,8},
    SL(6,kSrcLfo3,kDstF1Freq,.4f),OFF(1),{kE3Sustain,1}, END };
static const PV gatedPad[] = { {kVoices,12},{kO1Wave,1},{kO2Wave,1},{kO2Fine,10},{kO2Level,.8f},{kF1Freq,1500},{kL2Wave,2},{kL2Sync,1},{kL2Div,8},{kL2Balance,.4f},
    SL(6,kSrcLfo2,kDstVolume,-.9f),OFF(1),{kE3Attack,.3f},{kE3Release,1}, END };
static const PV laser[] = { {kVoices,4},{kO1Wave,1},{kO2Level,0},{kO1Oct,2},{kE2Attack,0},{kE2Decay,.3f},{kE2Sustain,0},SL(6,kSrcEg2,kDstO1Pitch,-1),
    {kE3Decay,.35f},{kE3Sustain,0},{kE3Release,.1f},OFF(0),OFF(1),{kF1Freq,8000}, END };
static const PV riser[] = { {kVoices,4},{kO1Wave,1},{kO2Wave,1},{kO2Level,.8f},{kO2Fine,-15},{kF1Style,1},{kF1Freq,200},{kF1Peak,.5f},{kE2Attack,8},{kE2Sustain,1},
    SL(6,kSrcEg2,kDstF1Freq,.9f),SL(7,kSrcEg2,kDstO1Pitch,.5f),SL(8,kSrcEg2,kDstO2Pitch,.5f),OFF(0),OFF(1),{kE3Release,1}, END };
static const PV arpMotion[] = { {kVoices,8},{kO1Wave,2},{kO2Level,0},{kF1Style,3},{kF1Freq,500},{kF1Peak,.6f},{kL2Wave,0},{kL2Sync,1},{kL2Div,3},{kL3Wave,2},{kL3Sync,1},{kL3Div,8},
    SL(6,kSrcLfo2,kDstF1Freq,.3f),SL(7,kSrcLfo3,kDstVolume,-.8f),SL(8,kSrcLfo3,kDstO1PW,.25f),OFF(1),{kE3Sustain,1}, END };
static const PV alarm[] = { {kVoices,2},{kO1Wave,2},{kO2Level,0},{kF1On,0},{kL1Wave,2},{kL1Rate,3},SL(2,kSrcLfo1,kDstO1Pitch,.2f),SL(4,kSrcNone,kDstNone,0),OFF(0),OFF(1), END };
static const PV wind[] = { {kVoices,4},{kO1Wave,1},{kO2Wave,2},{kO2Oct,3},{kO2Level,.6f},{kOscSync,0},{kF1Type,1},{kF1Slope,0},{kF1Style,3},{kF1Freq,600},{kF1Peak,.7f},{kVolume,0},
    {kL3Wave,3},{kL3Rate,.2f},SL(6,kSrcLfo3,kDstF1Freq,.4f),{kE3Attack,1.5f},{kE3Release,2.5f},OFF(0),OFF(1), END };
// ================================================================ 3.0
// ---------------------------------------------------------------- Bass
static const PV fourOscBass[] = { {kVoices,1},{kO1Wave,1},{kO1Oct,-1},{kO2Wave,2},{kO2Oct,-2},{kO2Level,.6f},{kO2Fine,0},ON(2),{kO3Wave,3},{kO3Oct,-2},{kO3Level,.7f},
    ON(3),{kO4Wave,5},{kO4Oct,0},{kO4Level,.08f},{kF1Style,7},{kF1Freq,420},{kF1Peak,.35f},{kE1Decay,.25f},{kE1Sustain,.1f},{kE3Release,.08f},
    {kDrvOn,1},{kDrvAmount,.35f},{kDrvPost,1},{kCmpOn,1},{kCmpThreshold,-20},{kCmpRatio,4},{kVolume,-8}, END };
static const PV reese3[] = { {kVoices,1},{kUnison,6},{kUniDetune,.4f},{kUniSpread,.7f},{kDrift,.4f},{kO1Wave,1},{kO2Wave,1},{kO2Level,.8f},{kO2Fine,-14},{kO1Oct,-1},{kO2Oct,-1},
    {kF1Style,6},{kF1Freq,700},{kF1Peak,.35f},OFF(1),{kE3Release,.15f},{kChoOn,1},{kChoMix,.35f},{kChoMode,1},
    {lp_(0, em::kLRate),.15f},S2(0,em::kSrcXlfo1,kDstF1Freq,.12f),{kVolume,-2}, END };
// ---------------------------------------------------------------- Lead
static const PV hollowSync[] = { {kVoices,1},{kGlide,.05f},{kO1Wave,1},{kO2Level,0},{kOscRatioFirst,2.5f},{kF1Style,10},{kF1Freq,2500},{kF1Peak,.55f},OFF(1),
    {lp_(0, em::kLRate),.3f},{lp_(0, em::kLWave),1},S2(0,em::kSrcXlfo1,kV2RatioFirst,.12f),S2(1,em::kSrcWheel,kV2RatioFirst,.4f),
    {kDlyOn,1},{kDlyDiv,2},{kDlyMix,.25f},{kDlyFeedback,.35f},{kRevOn,1},{kRevMix,.15f},{kVolume,-4}, END };
static const PV ringLead[] = { {kVoices,1},{kO1Wave,3},{kO2Wave,3},{kO2Level,1},{kO1Level,.4f},{kO2Semi,7},{kO2Fine,0},{kRing12,1},{kF1Freq,5000},OFF(1),
    {kE3Sustain,1},{kDlyOn,1},{kDlyDiv,1},{kDlyMix,.3f},{kDlyPingPong,1},{kRevOn,1},{kRevMix,.2f},{kVolume,-3}, END };
static const PV hqScreamer[] = { {kVoices,1},{kHq,1},{kO1Wave,1},{kO2Wave,2},{kO2Level,.7f},{kO2Fine,8},{kF1Style,9},{kF1Freq,1400},{kF1Peak,.75f},{kF2On,1},{kF2Style,2},{kF2Type,1},{kF2Freq,2800},{kF2Peak,.5f},
    {kRouting,1},{kE1Decay,.6f},{kE1Sustain,.4f},{kDrvOn,1},{kDrvAmount,.45f},{kRevOn,1},{kRevMix,.12f},{kVolume,-6}, END };
// ---------------------------------------------------------------- Pad
static const PV shimmerPad[] = { {kVoices,12},{kDrift,.5f},{kO1Wave,1},{kO2Wave,1},{kO2Level,.8f},{kO2Fine,9},ON(2),{kO3Wave,1},{kO3Fine,-9},{kO3Oct,1},{kO3Level,.5f},ON(3),{kO4Wave,3},{kO4Oct,1},{kO4Level,.4f},
    {kF1Style,8},{kF1Freq,2200},{kF1Peak,.2f},OFF(1),{kE3Attack,1.2f},{kE3Release,2.5f},{kChoOn,1},{kChoMode,1},{kChoMix,.5f},{kRevOn,1},{kRevMix,.45f},{kRevTime,6},
    {kPhsOn,1},{kPhsMix,.3f},{kPhsRate,.08f},{lp_(1, em::kLRate),.07f},{kXlfoRetrigFirst + 1,0},S2(0,em::kSrcXlfo2,kV2FreqOffset,.12f),{kVolume,-11}, END };
static const PV breathingPad[] = { {kVoices,10},{kO1Wave,2},{kO1PW,.3f},{kO2Wave,1},{kO2Level,.6f},{kO2Fine,6},ON(3),{kO4Wave,5},{kO4Level,.15f},
    {kF1Style,5},{kF1Freq,900},{kF1Peak,.3f},OFF(1),{kF3On,1},{kF3Freq,1800},{fpar(2, kFGain),6},{kE3Attack,.8f},{kE3Release,2},
    {ep(1, em::kEAttack),2.5f},{ep(1, em::kEDecay),3},{ep(1, em::kESustain),.3f},{kEgCurveFirst + 3,-.7f},S2(0,em::kSrcEg2,kDstF1Freq,.45f),
    S2(1,em::kSrcXy1X,kV2FreqOffset,.6f),S2(2,em::kSrcXy1Y,kV2PeakOffset,.5f),{xyp(0,0),.3f},{xyp(0,1),.2f},{kRevOn,1},{kRevMix,.4f},{kRevTime,4},{kVolume,-9}, END };
// ---------------------------------------------------------------- Keys & Plucks
static const PV tineKeys[] = { {kVoices,12},{kO1Wave,3},{kO2Wave,3},{kO2Oct,2},{kO2Level,.35f},{kO2Fine,0},{kOscRatioFirst + 1,1},{kF1On,0},
    {kE3Attack,.002f},{kE3Decay,1.8f},{kE3Sustain,0},{kE3Release,.5f},{ep(0, em::kEDecay),.2f},{ep(0, em::kESustain),0},S2(0,em::kSrcEg1,kDstO2Level,.5f),
    S2(1,em::kSrcVelocity,kDstO2Level,.3f),{kChoOn,1},{kChoMix,.35f},{kRevOn,1},{kRevMix,.2f},{kRevTime,1.6f},OFF(0),OFF(1),{kVolume,-6}, END };
static const PV vowelKeys[] = { {kVoices,10},{kO1Wave,1},{kO2Wave,2},{kO2Level,.5f},{kO2Fine,5},{kF1Type,kBell},{kF1Style,4},{kF1Freq,700},{fpar(0, kFGain),12},{kF1Peak,.6f},
    {kF2On,1},{kF2Type,kBell},{kF2Style,4},{kF2Freq,1200},{fpar(1, kFGain),10},{kF2Peak,.6f},{kF3On,1},{kF3Type,kHighShelf},{kF3Freq,3000},{fpar(2, kFGain),-12},
    {kF4On,1},{kF4Type,0},{kF4Style,8},{kF4Freq,4000},{kF4Slope,0},OFF(0),OFF(1),S2(0,em::kSrcWheel,kV2FreqOffset,.5f),{kE3Decay,1},{kE3Sustain,.5f},{kVolume,-12}, END };
static const PV transientPluck[] = { {kVoices,8},{kO1Wave,1},{kO2Wave,2},{kO2Level,.5f},{kF1Style,3},{kF1Freq,500},{kF1Peak,.45f},OFF(1),
    {kEfModeFirst,1},{fp(0, em::kFAttack),.001f},{fp(0, em::kFRelease),.2f},S2(0,em::kSrcEf1,kDstF1Freq,.6f),{kE3Decay,.6f},{kE3Sustain,.2f},{kDlyOn,1},{kDlyMix,.2f},{kVolume,-3}, END };
// ---------------------------------------------------------------- FX & Motion
static const PV noiseSweep[] = { {kVoices,4},{kO1Wave,5},{kO2Level,0},{kF1Type,1},{kF1Style,4},{kF1Freq,300},{kF1Peak,.6f},OFF(0),OFF(1),
    {ep(0, em::kEAttack),4},{ep(0, em::kESustain),1},{kEgCurveFirst,-.6f},S2(0,em::kSrcEg1,kDstF1Freq,.7f),{kE3Attack,.5f},{kE3Release,3},
    {kRevOn,1},{kRevMix,.5f},{kRevTime,8},{kPhsOn,1},{kPhsMode,1},{kPhsMix,.4f},{kPhsRate,.1f},{kVolume,0}, END };
// ---------------------------------------------------------------- Arp & Sequence
static const PV arpUp[] = { {kVoices,8},{kArpOn,1},{kArpSync,1},{kArpDiv,4},{kArpOctaves,1},{kArpGate,.5f},{kO1Wave,1},{kO2Wave,2},{kO2Level,.5f},{kF1Style,3},{kF1Freq,900},{kF1Peak,.4f},
    {kE1Decay,.15f},{kE1Sustain,0},{kE3Decay,.3f},{kE3Sustain,.3f},{kE3Release,.12f},{kDlyOn,1},{kDlyDiv,2},{kDlyMix,.3f},{kDlyPingPong,1},{kDlyFeedback,.45f},{kRevOn,1},{kRevMix,.15f},{kVolume,-4}, END };
static const PV arpRandom[] = { {kVoices,8},{kArpOn,1},{kArpSync,1},{kArpDiv,6},{kArpOrder,kArpRandom},{kArpOctaves,2},{kArpGate,.4f},{kO1Wave,2},{kO1PW,.25f},{kO2Level,0},
    {kF1Style,6},{kF1Freq,700},{kF1Peak,.55f},{lp_(0, em::kLWave),6},{lp_(0, em::kLSync),1},{lp_(0, em::kLDiv),8},{kXlfoRetrigFirst,0},S2(0,em::kSrcXlfo1,kDstF1Freq,.3f),
    {kE3Decay,.2f},{kE3Sustain,.2f},{kDlyOn,1},{kDlyDiv,3},{kDlyMix,.25f},{kVolume,-3}, END };
static const PV grooveArp[] = { {kVoices,8},{kArpOn,1},{kArpSync,1},{kArpDiv,4},{kArpOrder,kArpUpDown},{kArpGroove,.45f},{kArpLatch,kLatchHeld},{kArpGate,.7f},{kO1Wave,1},{kO2Wave,1},{kO2Level,.6f},{kO2Oct,1},{kO2Fine,4},
    {kF1Style,1},{kF1Freq,1200},{kF1Peak,.35f},{kE3Decay,.25f},{kE3Sustain,.4f},{kChoOn,1},{kChoMix,.3f},{kRevOn,1},{kRevMix,.15f},{kVolume,-3}, END };
static const PV stepFilterBass[] = { {kVoices,1},{kO1Wave,1},{kO1Oct,-1},{kO2Wave,2},{kO2Oct,-1},{kO2Level,.5f},{kF1Style,3},{kF1Freq,300},{kF1Peak,.65f},OFF(1),{kE3Sustain,1},{kE3Release,.08f},
    {lp_(0, em::kLMode),1},{lp_(0, em::kLSync),1},{lp_(0, em::kLDiv),4},{lp_(0, em::kLGlide),.15f},{kXlfoRetrigFirst,0},
    STEPS(0, .9f,-.4f,.3f,-.8f, .6f,-.2f,1,-.6f, .2f,-.9f,.7f,-.3f, .5f,-.7f,.8f,-1),S2(0,em::kSrcXlfo1,kDstF1Freq,.35f),{kDrvOn,1},{kDrvAmount,.3f},{kVolume,-9}, END };
static const PV tranceGate[] = { {kVoices,12},{kO1Wave,1},{kO2Wave,1},{kO2Level,.8f},{kO2Fine,10},{kUnison,2},{kUniDetune,.2f},{kF1Freq,2500},{kE3Attack,.05f},{kE3Release,.6f},
    {lp_(0, em::kLMode),1},{lp_(0, em::kLSync),1},{lp_(0, em::kLDiv),4},{lp_(0, em::kLGlide),.05f},{kXlfoRetrigFirst,0},
    STEPS(0, 1,1,-1,1, 1,-1,1,1, 1,-1,1,1, -1,1,-1,-1),S2(0,em::kSrcXlfo1,kDstVolume,.5f),{kDlyOn,1},{kDlyDiv,2},{kDlyMix,.2f},{kRevOn,1},{kRevMix,.3f},{kVolume,-12}, END };
// ---------------------------------------------------------------- Texture
static const PV perOscChords[] = { {kVoices,8},{kPolyMode,1},{kO1Wave,1},{kO2Wave,2},{kO2Level,.9f},{kO2Fine,0},ON(2),{kO3Wave,0},{kO3Oct,1},{kO3Level,.9f},ON(3),{kO4Wave,3},{kO4Oct,1},{kO4Level,1},
    {kRouting,2},{kF1Freq,1800},{kF2On,1},{kF2Type,0},{kF2Freq,1200},{kF2Peak,.5f},{kF2Style,3},{kF3On,1},{kF3Type,1},{kF3Style,6},{kF3Freq,1500},{kF3Peak,.4f},{kF4On,0},
    {kE3Attack,.02f},{kE3Release,.8f},{kChoOn,1},{kChoMix,.4f},{kRevOn,1},{kRevMix,.3f},{kVolume,-8}, END };
static const PV driftStrings[] = { {kVoices,12},{kDrift,.8f},{kO1Wave,1},{kO2Wave,1},{kO2Level,.9f},{kO2Fine,7},ON(2),{kO3Wave,2},{kO3PW,.35f},{kO3Level,.5f},{kO3Fine,-7},
    {kF1Style,5},{kF1Freq,2600},{kF1Peak,.15f},{kF2On,1},{kF2Type,2},{kF2Freq,150},{kE3Attack,.35f},{kE3Release,1.2f},
    {kPhsOn,1},{kPhsMix,.35f},{kPhsRate,.15f},{kRevOn,1},{kRevMix,.3f},{kRevTime,3},{kCmpOn,1},{kCmpThreshold,-24},{kCmpRatio,2},{kVolume,-10}, END };
} // namespace pv
#undef SL
#undef S2
#undef ON
#undef STEPS
#undef OFF
#undef END

static const PresetDef kPresetDefs[] = {
    {"Init", "Init", pv::init},
    {"Synth Bass", "Bass", pv::synthBass}, {"Acid Bass", "Bass", pv::acidBass}, {"Sync Bass", "Bass", pv::syncBass}, {"Sub Bass", "Bass", pv::subBass},
    {"Reese", "Bass", pv::reeseBass}, {"Pluck Bass", "Bass", pv::pluckBass}, {"Wobble 1/8", "Bass", pv::wobbleBass},
    {"Sync Lead", "Lead", pv::syncLead}, {"PWM Lead", "Lead", pv::squareLead}, {"Screamer", "Lead", pv::screamLead}, {"Glide Lead", "Lead", pv::glideLead},
    {"Whistle", "Lead", pv::whistleLead}, {"Unison Lead", "Lead", pv::unisonLead}, {"Chip Lead", "Lead", pv::chipLead},
    {"Warm Pad", "Pad", pv::warmPad}, {"Glass Pad", "Pad", pv::glassPad}, {"Stereo Pad", "Pad", pv::stereoPad}, {"Dark Pad", "Pad", pv::darkPad},
    {"Sweep Pad", "Pad", pv::sweepPad}, {"Choir", "Pad", pv::choirPad}, {"Evolving", "Pad", pv::evolvePad},
    {"Pluck", "Keys & Plucks", pv::pluck}, {"Electric Piano", "Keys & Plucks", pv::electricPiano}, {"Clav", "Keys & Plucks", pv::clav},
    {"Bell", "Keys & Plucks", pv::bell}, {"Organ", "Keys & Plucks", pv::organ}, {"Harp", "Keys & Plucks", pv::harp}, {"Stab", "Keys & Plucks", pv::stab},
    {"Brass", "Brass & Strings", pv::brass}, {"Soft Brass", "Brass & Strings", pv::softBrass}, {"Strings", "Brass & Strings", pv::strings},
    {"PWM Strings", "Brass & Strings", pv::pwmStrings}, {"Sync Brass", "Brass & Strings", pv::syncBrass}, {"Solo Cello", "Brass & Strings", pv::cello},
    {"S&H Filter", "FX & Motion", pv::sAndH}, {"Gated Pad 1/16", "FX & Motion", pv::gatedPad}, {"Laser", "FX & Motion", pv::laser},
    {"Riser", "FX & Motion", pv::riser}, {"Arp Motion", "FX & Motion", pv::arpMotion}, {"Alarm", "FX & Motion", pv::alarm}, {"Wind", "FX & Motion", pv::wind},
    // 3.0
    {"Four-Osc Bass", "Bass", pv::fourOscBass}, {"Reese Drift", "Bass", pv::reese3},
    {"Hollow Sync Lead", "Lead", pv::hollowSync}, {"Ring Lead", "Lead", pv::ringLead}, {"HQ Screamer", "Lead", pv::hqScreamer},
    {"Shimmer Pad", "Pad", pv::shimmerPad}, {"Breathing Pad (XY)", "Pad", pv::breathingPad},
    {"Tine Keys", "Keys & Plucks", pv::tineKeys}, {"Vowel Keys (Wheel)", "Keys & Plucks", pv::vowelKeys}, {"Transient Pluck", "Keys & Plucks", pv::transientPluck},
    {"Noise Sweep", "FX & Motion", pv::noiseSweep},
    {"Arp Up 1/16", "Arp & Sequence", pv::arpUp}, {"Random Arp", "Arp & Sequence", pv::arpRandom}, {"Groove Arp (Latch)", "Arp & Sequence", pv::grooveArp},
    {"Step Filter Bass", "Arp & Sequence", pv::stepFilterBass}, {"Trance Gate", "Arp & Sequence", pv::tranceGate},
    {"Per-Osc Chords", "Texture", pv::perOscChords}, {"Drift Strings", "Texture", pv::driftStrings},
};
static constexpr uint32_t kNumPresets = sizeof(kPresetDefs) / sizeof(kPresetDefs[0]);

inline const Preset* presets() {
    static Preset table[kNumPresets];
    static bool done = false;
    if (done) return table;
    done = true;
    Engine e;
    for (uint32_t i = 0; i < kNumPresets; ++i) {
        table[i].name = kPresetDefs[i].name; table[i].category = kPresetDefs[i].category;
        std::copy(e.p, e.p + kParamCount, table[i].v);
        for (const PV* x = kPresetDefs[i].pv; x->id != kParamCount; ++x) table[i].v[x->id] = x->v;
    }
    return table;
}

} // namespace obsidian
