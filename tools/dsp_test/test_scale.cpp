#include <cstdio>
#include "../../app/src/main/cpp/vocal_scale.h"
int main(){ const float SR=48000; VocalScale v; v.reset(); ScaleParams p; p.on=true; v.prepare(SR,p);
 float lv[]={-40,-30,-20,-12,-5}; bool bad=false;
 for(float L:lv){ float a=std::pow(10.f,L/20)*1.414f; double si=0,so=0; int n=SR;
  for(int i=0;i<n;i++){float x=a*std::sin(6.2831853f*300*i/SR),r=x; v.process(x,r,false,p); if(!std::isfinite(x))bad=true; if(i>n/2){so+=x*x;}}
  printf("in %.0f dB -> out %.1f dB\n",L,10*std::log10(so/(n/2)));}
 p.on=false; float x=0.1f,r; for(int i=0;i<SR;i++){x=0.1f;r=x;v.process(x,r,false,p);} printf("OFF gain %.2f dB, NaN %s\n",v.gainDb,bad?"ADA":"tidak");}
