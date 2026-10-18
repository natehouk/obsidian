#include "../plugins/Obsidian/ParamDefs.hpp"
#include <cstdio>
#include <random>
using namespace obsidian;
int main(){ std::mt19937 g(11); std::uniform_real_distribution<float> u(0,1); int bad=0; float pk=0;
 for(int sr: {44100,96000,192000}){ Engine e; e.setSampleRate(sr);
  for(int it=0; it<1500; ++it){
   for(uint32_t i=0;i<kParamCount;++i){ const ParamDef& d=paramDef(i); if(d.hints&kHintOutput) continue;
     float v=(u(g)<.3f)?(u(g)<.5f?d.min:d.max):d.min+(d.max-d.min)*u(g); if(d.hints&kHintInteger) v=std::round(v); e.p[i]=v; }
   e.p[kMidiOn]=1; e.setTransport(u(g)<.5f,u(g)<.5f,u(g)*64,40+u(g)*200,2+int(u(g)*6));
   for(int k=0;k<3;++k){ if(u(g)<.6f) e.noteOn(24+int(u(g)*90),1+int(u(g)*126)); if(u(g)<.4f) e.noteOff(24+int(u(g)*90)); }
   if(u(g)<.1f) e.pitchBend(int(u(g)*16383)); if(u(g)<.1f) e.modWheel(int(u(g)*127)); if(u(g)<.1f) e.channelPressure(int(u(g)*127)); if(u(g)<.05f) e.sustain(u(g)<.5f);
   float L[300],R[300]; e.render(L,R,300);
   for(int k=0;k<300;++k){ if(!std::isfinite(L[k])||!std::isfinite(R[k])) bad++; pk=std::max(pk,std::max(std::fabs(L[k]),std::fabs(R[k]))); } } }
 printf("fuzz nonfinite=%d peak=%.3f\n",bad,pk); return bad||pk>1.f; }
