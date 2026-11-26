#pragma once
#include <psputility.h>
#include <array>
#include <string>
namespace EditorKeyboard {
inline bool encode(const std::string& in,unsigned short* out,unsigned capacity){
 unsigned n=0;
 for(unsigned i=0;i<in.size();){
  uint32_t c=static_cast<unsigned char>(in[i++]);
  if(c>=0xc2&&c<=0xdf&&i<in.size()){c=((c&31)<<6)|(static_cast<unsigned char>(in[i++])&63);}
  else if(c>=0xe0&&c<=0xef&&i+1<in.size()){const unsigned b=static_cast<unsigned char>(in[i++]),d=static_cast<unsigned char>(in[i++]);c=((c&15)<<12)|((b&63)<<6)|(d&63);}
  else if(c>=0xf0&&c<=0xf4&&i+2<in.size()){const unsigned b=static_cast<unsigned char>(in[i++]),d=static_cast<unsigned char>(in[i++]),e=static_cast<unsigned char>(in[i++]);c=((c&7)<<18)|((b&63)<<12)|((d&63)<<6)|(e&63);}
  if(c>0xffff){if(n+2>=capacity)return false;c-=0x10000;out[n++]=0xd800|(c>>10);out[n++]=0xdc00|(c&1023);}
  else{if(n+1>=capacity)return false;out[n++]=static_cast<unsigned short>(c);}
 }
 out[n]=0;return true;
}
inline std::string decode(const unsigned short* in,unsigned capacity){
 std::string out;
 for(unsigned i=0;i<capacity&&in[i];++i){
  uint32_t c=in[i];
  if(c>=0xd800&&c<=0xdbff&&i+1<capacity&&in[i+1]>=0xdc00&&in[i+1]<=0xdfff){c=0x10000+((c-0xd800)<<10)+(in[++i]-0xdc00);}
  if(c<0x80)out+=char(c);
  else if(c<0x800){out+=char(0xc0|(c>>6));out+=char(0x80|(c&63));}
  else if(c<0x10000){out+=char(0xe0|(c>>12));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}
  else{out+=char(0xf0|(c>>18));out+=char(0x80|((c>>12)&63));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}
 }
 return out;
}
class Keyboard {
 SceUtilityOskData field{};SceUtilityOskParams params{};
 std::array<unsigned short,513> input{},output{};
 std::array<unsigned short,96> prompt{};
 bool opened=false,changed=false;int failure=0;
public:
 bool active()const{return opened;}
 bool accepted()const{return changed;}
 int error()const{return failure;}
 std::string result()const{return decode(output.data(),output.size());}
 bool begin(const std::string& value,const char* description,bool ru){
  if(opened)return false;
  field={};params={};input={};output={};prompt={};changed=false;
  if(!encode(value,input.data(),input.size())||!encode(description,prompt.data(),prompt.size()))return false;
  output=input;
  field.language=ru?PSP_UTILITY_OSK_LANGUAGE_RUSSIAN:PSP_UTILITY_OSK_LANGUAGE_ENGLISH;
  field.inputtype=PSP_UTILITY_OSK_INPUTTYPE_ALL;
  field.lines=1;field.desc=prompt.data();field.intext=input.data();
  field.outtext=output.data();field.outtextlength=output.size();field.outtextlimit=512;
  params.base.size=sizeof params;
  sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&params.base.language);
  sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,&params.base.buttonSwap);
  params.base.graphicsThread=17;params.base.accessThread=19;params.base.fontThread=18;params.base.soundThread=16;
  params.datacount=1;params.data=&field;failure=sceUtilityOskInitStart(&params);
  opened=failure>=0;return opened;
 }
 void draw(){
  if(!opened)return;
  switch(sceUtilityOskGetStatus()){
   case PSP_UTILITY_DIALOG_VISIBLE:sceUtilityOskUpdate(1);break;
   case PSP_UTILITY_DIALOG_QUIT:sceUtilityOskShutdownStart();break;
   case PSP_UTILITY_DIALOG_NONE:changed=field.result!=PSP_UTILITY_OSK_RESULT_CANCELLED;opened=false;break;
   default:break;
  }
 }
};
}
