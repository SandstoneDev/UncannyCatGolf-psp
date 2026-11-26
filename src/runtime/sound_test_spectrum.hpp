#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
// Independent radix2 analysis with the original Godot4.4 analyzer's configured
// 256 positive-frequency bins (512 input samples) and its two-period Hann.
class SoundTestSpectrum {
public:
 SoundTestSpectrum() {
  for(unsigned i=0;i<N;++i)window[i]=.5f-.5f*std::cos(6.28318530718f*i/256);
 }
 void reset(){position=0;for(auto&v:bands)v.store(0,std::memory_order_relaxed);}
 float band(unsigned i)const{return i<8?bands[i].load(std::memory_order_relaxed)/16777216.f:0;}
 void feed(const int16_t*pcm,unsigned frames) {
  for(unsigned i=0;i<frames;++i) {
   real[0][position]=pcm[i*2]/32768.f*window[position];
   real[1][position]=pcm[i*2+1]/32768.f*window[position];
   if(++position==N){analyze();position=0;}
  }
 }
private:
 static constexpr unsigned N=512;
 unsigned position=0;
 float real[2][N]{},imag[N]{},window[N]{};
 std::atomic<unsigned> bands[8]{};
 void transform(float*r) {
  std::fill_n(imag,N,0.f);
  for(unsigned i=1,j=0;i<N;++i){
   unsigned bit=N>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;
   if(i<j)std::swap(r[i],r[j]);
  }
  for(unsigned length=2;length<=N;length<<=1){
   const float angle=-6.28318530718f/length;
   const float wr=std::cos(angle),wi=std::sin(angle);
   for(unsigned begin=0;begin<N;begin+=length){
    float cr=1,ci=0;
    for(unsigned j=0;j<length/2;++j){
     const unsigned a=begin+j,b=a+length/2;
     const float tr=r[b]*cr-imag[b]*ci,ti=r[b]*ci+imag[b]*cr;
     const float ar=r[a],ai=imag[a];r[a]=ar+tr;imag[a]=ai+ti;r[b]=ar-tr;imag[b]=ai-ti;
     const float next=cr*wr-ci*wi;ci=cr*wi+ci*wr;cr=next;
    }
   }
  }
 }
 void analyze() {
  constexpr float edges[]={0,40,100,300,500,750,1500,3000,6000};
  float sum[8]{};
  for(unsigned channel=0;channel<2;++channel){
   transform(real[channel]);
   for(unsigned band=0;band<8;++band){
    const unsigned first=static_cast<unsigned>(edges[band]*256/22050);
    const unsigned last=static_cast<unsigned>(edges[band+1]*256/22050);
    float peak=0;
    for(unsigned i=first;i<=last;++i)peak=std::max(peak,std::hypot(real[channel][i],imag[i])/256);
    sum[band]+=peak*.5f;
   }
  }
  for(unsigned i=0;i<8;++i)bands[i].store(static_cast<unsigned>(std::max(0.f,sum[i])*16777216.f),std::memory_order_relaxed);
 }
};
