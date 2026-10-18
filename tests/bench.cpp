// CPU check: 24 voices, both oscillators, both 24 dB filters, all LFOs and 6 modulation slots active.
#include "../plugins/Obsidian/ParamDefs.hpp"
#include <chrono>
#include <cstdio>
using namespace obsidian;
int main(){ Engine e; e.setSampleRate(48000); e.p[kO2Level]=0.7f; e.p[kF2On]=1; e.p[kF2Slope]=1; e.p[kVoices]=24; e.p[kUnison]=1;
 e.setSlot(6,kSrcLfo2,kDstF1Freq,0.2f); e.setSlot(7,kSrcLfo3,kDstO1PW,0.3f);
 for(int n=36;n<60;++n) e.noteOn(n,100);
 const int N=48000*10; float L[256],R[256];
 auto t0=std::chrono::steady_clock::now(); for(int i=0;i<N;i+=256) e.render(L,R,256); auto t1=std::chrono::steady_clock::now();
 double sec=std::chrono::duration<double>(t1-t0).count();
 printf("10 s of audio with %d voices rendered in %.2f s (%.1f%% of one core)\n", e.activeVoices(), sec, sec/10*100); }
