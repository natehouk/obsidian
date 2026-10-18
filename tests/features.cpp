// Behaviour checks for Obsidian.
#include "../plugins/Obsidian/ParamDefs.hpp"
#include <cstdio>
#include <vector>
using namespace obsidian;
static int fails=0;
#define CHECK(c,msg) do{ bool ok_=(c); printf("%s  %s\n", ok_?"PASS":"FAIL", msg); if(!ok_) fails++; }while(0)
static const int SR=48000;
struct Out{ std::vector<float> l,r; };
static Out run(Engine& e,int n){ Out o; o.l.resize(n); o.r.resize(n); e.render(o.l.data(),o.r.data(),n); return o; }
static float rms(const std::vector<float>& v,size_t a=0,size_t z=0){ if(!z) z=v.size(); double s=0; for(size_t i=a;i<z;++i) s+=v[i]*v[i]; return std::sqrt(s/(z-a)); }
static int zc(const std::vector<float>& v){ int z=0; for(size_t i=1;i<v.size();++i) z+=(v[i-1]<0)!=(v[i]<0); return z; }
static Engine mk(){ Engine e; e.setSampleRate(SR); e.p[kF1On]=0; for(int s=0;s<kNumSlots;++s) e.p[slotParam(s,kSlotBypass)]=1; return e; }
int main(){
  { Engine e=mk(); e.p[kVoices]=8; e.p[kUnison]=1; for(int n:{60,64,67,71}) e.noteOn(n,100); run(e,256);
    CHECK(e.activeVoices()==4 && e.activeNotes()==4,"poly: 4 notes use 4 voices");
    e.p[kUnison]=3; e.noteOn(74,100); run(e,256); CHECK(e.activeVoices()==7,"unison 3: next note adds 3 voices");
    Engine f=mk(); f.p[kVoices]=4; for(int n=60;n<70;++n) f.noteOn(n,100); run(f,SR/20);
    printf("      notes after 10 keys with Voices=4: %d\n",f.activeNotes()); CHECK(f.activeNotes()==4,"polyphony limit respected (Voices = 4)");
    Engine g=mk(); g.p[kVoices]=24; g.p[kUnison]=4; for(int n=40;n<60;++n) g.noteOn(n,100); run(g,SR/20);
    CHECK(g.activeVoices()<=24,"never more than 24 voices"); }
  { Engine e=mk(); e.p[kVoices]=1; e.p[kUnison]=2; for(int n:{60,64,67}) e.noteOn(n,100); run(e,256);
    CHECK(e.activeVoices()==2,"mono + unison 2: one note, 2 voices"); }
  { // hard sync: raises upper harmonics; compare zero crossings of osc2 alone with and without sync
    auto zcs=[&](bool sync){ Engine e=mk(); e.p[kO1Level]=0; e.p[kO2Level]=1; e.p[kO2Semi]=7; e.p[kO2Fine]=0; e.p[kO2Wave]=1; e.p[kOscSync]=sync; e.p[kO1Wave]=1; e.p[kO2Oct]=1;
      e.noteOn(45,127); Out o=run(e,SR/2); return zc(o.l); };
    int a=zcs(false), b=zcs(true); printf("      zero crossings free %d sync %d\n",a,b); CHECK(a!=b,"osc 2 hard sync changes the waveform"); }
  { // phase sync: two renders of the same note are identical
    auto first=[&](){ Engine e=mk(); e.p[kO1Phase]=1; e.p[kO2Phase]=1; e.noteOn(60,100); Out o=run(e,512); return o.l; };
    auto a=first(), b=first(); float d=0; for(size_t i=0;i<a.size();++i) d=std::max(d,std::fabs(a[i]-b[i])); CHECK(d<1e-6f,"phase sync: notes start identically"); }
  { // split routing: osc 2 goes only through filter 2
    Engine e=mk(); e.p[kRouting]=2; e.p[kF1On]=1; e.p[kF2On]=1; e.p[kF1Type]=0; e.p[kF1Freq]=20; e.p[kF2Type]=0; e.p[kF2Freq]=20000;
    e.p[kO1Level]=1; e.p[kO2Level]=0; e.noteOn(72,127); float o1=rms(run(e,SR/4).l,2400);
    Engine f=e; f.reset(); f.p[kO1Level]=0; f.p[kO2Level]=1; f.noteOn(72,127); float o2=rms(run(f,SR/4).l,2400);
    printf("      split: osc1 via LP 20 Hz %.4f, osc2 via LP 20 kHz %.4f\n",o1,o2); CHECK(o2>o1*20,"split routing: osc 1 -> filter 1, osc 2 -> filter 2"); }
  { // modulation slot: LFO 1 -> osc 1 pitch makes vibrato; bypass turns it off
    auto spread=[&](bool byp){ Engine e=mk(); e.p[kO1Wave]=0; e.p[kL1Rate]=1; e.setSlot(0,kSrcLfo1,kDstO1Pitch,0.1f); e.p[slotParam(0,kSlotBypass)]=byp;
      e.noteOn(60,127); int lo=1<<30,hi=0; for(int k=0;k<20;++k){ Out o=run(e,SR/10); int z=zc(o.l); lo=std::min(lo,z); hi=std::max(hi,z);} return hi-lo; };
    int a=spread(false), b=spread(true); printf("      zc spread with slot %d, bypassed %d\n",a,b); CHECK(a>b+2,"slot LFO1 -> pitch works, bypass disables it"); }
  { // slot amount modulated by another slot (mod wheel scales vibrato)
    auto spread=[&](int wheel){ Engine e=mk(); e.p[kO1Wave]=0; e.p[kL1Rate]=1; e.setSlot(0,kSrcLfo1,kDstO1Pitch,0.f); e.setSlot(1,kSrcWheel,kDstSlotFirst+0,0.1f);
      e.modWheel(wheel); e.noteOn(60,127); int lo=1<<30,hi=0; for(int k=0;k<20;++k){ Out o=run(e,SR/10); int z=zc(o.l); lo=std::min(lo,z); hi=std::max(hi,z);} return hi-lo; };
    int a=spread(0), b=spread(127); printf("      vibrato spread wheel 0: %d, wheel 127: %d\n",a,b); CHECK(b>a+2,"mod wheel modulates another slot's amount"); }
  { // env delay: EG1 -> F1 freq with 0.25 s delay: filter opens late
    Engine e=mk(); e.p[kF1On]=1; e.p[kF1Freq]=100; e.p[kF1Peak]=0; e.p[kE1Delay]=0.25f; e.p[kE1Attack]=0.001f; e.p[kE1Sustain]=1; e.setSlot(0,kSrcEg1,kDstF1Freq,1);
    e.noteOn(60,127); Out o=run(e,SR/2); float early=rms(o.l,2400,SR/5), late=rms(o.l,SR*3/10,SR/2);
    printf("      before delay %.4f after %.4f\n",early,late); CHECK(late>early*3,"envelope delay postpones the attack"); }
  { // amp hold keeps full level before decaying
    Engine e=mk(); e.p[kE3Attack]=0.001f; e.p[kE3Hold]=0.2f; e.p[kE3Decay]=0.02f; e.p[kE3Sustain]=0; e.noteOn(60,127); Out o=run(e,SR/2);
    float inHold=rms(o.l,SR/20,SR/7), after=rms(o.l,SR/3,SR/2); printf("      in hold %.4f after %.4f\n",inHold,after); CHECK(inHold>0.05f && after<inHold*0.05f,"amp hold stage"); }
  { // sample & hold LFO produces steps; random triangle is continuous
    int changes=0; Lfo l; l.rng=5; float prev=l.value(4,.5f); for(int i=0;i<48000;++i){ l.advance(4.0/48000); float v=l.value(4,.5f); if(v!=prev) changes++; prev=v; }
    CHECK(changes>=3 && changes<=5,"S&H LFO changes once per cycle"); }
  { // self-oscillating styles ring after the oscillators stop
    for(int st=0; st<kNumStyles; ++st){ Engine e=mk(); e.p[kF1On]=1; e.p[kF1Style]=st; e.p[kF1Peak]=1; e.p[kF1Freq]=800; e.p[kE3Sustain]=1; e.p[kE3Release]=5;
      e.p[kO1Level]=1; e.noteOn(48,127); run(e,2400); e.p[kO1Level]=0; Out o=run(e,SR/2); float t=rms(o.l,SR/4,SR/2);
      int n; char m[80]; snprintf(m,sizeof m,"%-8s %s",enumLabels(kF1Style,n)[st], styleDef(st).kMin<0?"self-oscillates":"stays quiet"); CHECK(styleDef(st).kMin<0 ? t>0.01f : t<0.003f, m); } }
  { // glide: poly glides from the previous note
    Engine e=mk(); e.p[kGlide]=0.3f; e.p[kO1Wave]=0; e.noteOn(48,127); run(e,SR/4); e.noteOff(48); e.noteOn(72,127);
    Out a=run(e,SR/20); Out b=run(e,SR/2); int z1=zc(a.l)*10, z2=zc(std::vector<float>(b.l.end()-SR/20,b.l.end()))*10;
    printf("      early %d Hz-ish, settled %d Hz-ish\n",z1/2,z2/2); CHECK(z1<z2*0.8f,"poly glide slides up to the new note"); }
  printf("%s (%d failures)\n", fails?"FAILED":"ALL PASSED", fails); return fails?1:0; }
