// Play a chord (or a short phrase for mono presets) through every factory preset; report levels.
#include "../plugins/Obsidian/ParamDefs.hpp"
#include "../plugins/Obsidian/Presets.hpp"
#include <cstdio>
#include <vector>
using namespace obsidian;
int main(){ const int SR=48000; int bad=0;
 for(uint32_t i=0;i<kNumPresets;++i){ Engine e; e.setSampleRate(SR);
  for(uint32_t k=0;k<kParamCount;++k) e.p[k]=presets()[i].v[k];
  e.setTransport(true,true,0,120,4);
  const bool mono = e.p[kVoices]<1.5f; std::vector<float> L(SR*3),R(SR*3); size_t pos=0;
  auto rend=[&](int n){ e.render(L.data()+pos,R.data()+pos,n); pos+=n; };
  if(mono){ int seq[]={36,43,48,46}; for(int n:seq){ e.noteOn(n,100); rend(SR/2); e.noteOff(n); rend(SR/10);} }
  else { for(int n:{48,55,60,64}) e.noteOn(n,100); rend(SR*2); for(int n:{48,55,60,64}) e.noteOff(n); }
  rend((int)(L.size()-pos));
  double s=0; float pk=0; bool nf=false; for(size_t k=0;k<L.size();++k){ s+=L[k]*L[k]+R[k]*R[k]; pk=std::max(pk,std::max(std::fabs(L[k]),std::fabs(R[k]))); nf|=!std::isfinite(L[k]); }
  float r=std::sqrt(s/(2*L.size())); bool ok=!nf && r>0.01f && pk<=1.f; bad+=!ok;
  printf("%-16s %-16s rms %.3f peak %.2f %s\n",presets()[i].name,presets()[i].category,r,pk,ok?"":"<-- CHECK"); }
 printf("%u presets, %d flagged\n",kNumPresets,bad); return bad; }
