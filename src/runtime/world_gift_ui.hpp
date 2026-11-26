#pragma once
// Include after gift_ui.hpp. The world presenter is the sole selector gift renderer.
#include "world_gift_art.hpp"
#include "world_gift_sequence.hpp"
#include "world_gift_arrival.hpp"

namespace WorldGift {
inline Sequence sequence;
inline ArrivalPulse arrivalPulse;
inline Read read=nullptr;
inline Open open=nullptr;
inline std::string path;
inline int textRecord=-1,iconRecord=-1;
inline void bind(const char* asset,Read snapshot,Open commit) {
    path=asset;read=snapshot;open=commit;sequence={};arrivalPulse={};textRecord=iconRecord=-1;
}
inline bool update(uint64_t now,bool dismissPressed,bool dismissHeld) {
    if(!read)return false;
    if(!sequence.active() && WorldState::nextPresent(read())>=0)audio.prime(Sound::GiftFly);
    return sequence.update(read(),open,now,dismissPressed,dismissHeld,[](bool blast) {
        if(blast)audio.play(Sound::GiftBlast);
        else {audio.play(Sound::GiftFly);audio.prime(Sound::GiftBlast);}
    });
}
inline void submitted(bool success){sequence.submitted(success);}
inline void presented(){sequence.presented();}

inline constexpr unsigned SpinRows=224,TextBytes=256*256*4,IconBytes=128*128*4;
static_assert([](){for(const auto& p:SelectorGifts::Spin)if(p.v+p.height>int(SpinRows))return false;return true;}());
static_assert(sizeof giftPixels>=SpinRows*512*4+TextBytes+IconBytes);
static_assert(sizeof burstGiftPixels>=EditorLegacyGiftBlast::WorkspaceBytes);
inline uint8_t* textPixels(){return reinterpret_cast<uint8_t*>(giftPixels+SpinRows*512);}
inline uint8_t* iconPixels(){return textPixels()+TextBytes;}

struct Arena {size_t used=0;};
inline voidpf alloc(voidpf opaque,uInt count,uInt size) {
    auto& arena=*static_cast<Arena*>(opaque);
    constexpr size_t capacity=EditorLegacyGiftBlast::WorkspaceBytes;
    if(!size || !count || count>capacity/size)return nullptr;
    const size_t n=size_t(count)*size,at=(arena.used+63)&~size_t(63);
    if(at>capacity || n>capacity-at)return nullptr;
    auto* memory=reinterpret_cast<uint8_t*>(burstGiftPixels)+at;
    std::memset(memory,0,n);arena.used=at+n;return memory;
}
inline void release(voidpf,voidpf){}
inline bool unpack(std::FILE* f,unsigned index,uint8_t* output,size_t capacity) {
    if(index>=std::size(WorldGiftArt::records))return false;
    const auto& r=WorldGiftArt::records[index];
    if(r.raw!=capacity || r.offset>WorldGiftArt::BankBytes || r.packed>WorldGiftArt::BankBytes-r.offset ||
       std::fseek(f,static_cast<long>(r.offset),SEEK_SET)!=0)return false;
    Arena arena;z_stream z{};z.zalloc=alloc;z.zfree=release;z.opaque=&arena;
    if(inflateInit(&z)!=Z_OK)return false;
    uint8_t input[4096];unsigned remaining=r.packed;
    z.next_out=output;z.avail_out=r.raw;
    int result=Z_OK;
    while(result==Z_OK) {
        if(!z.avail_in && remaining) {
            const auto n=std::min<unsigned>(remaining,sizeof input);
            if(std::fread(input,1,n,f)!=n)break;
            z.next_in=input;z.avail_in=n;remaining-=n;
        }
        result=inflate(&z,Z_NO_FLUSH);
    }
    const bool good=result==Z_STREAM_END && !remaining && !z.avail_in && !z.avail_out &&
                    crc32(0,output,capacity)==r.crc;
    inflateEnd(&z);
    if(good)sceKernelDcacheWritebackRange(output,capacity);
    return good;
}
inline bool prepare(unsigned text,unsigned icon) {
    if(textRecord==int(text) && iconRecord==int(icon))return true;
    const auto file=openAssetFile(path.c_str());
    if(!file || std::fseek(file.get(),0,SEEK_END) || std::ftell(file.get())!=long(WorldGiftArt::BankBytes))return false;
    // The previous texture or the old Burst scratch can be referenced even in an open list.
    if(!EditorLegacyGiftBlast::fence()){giftGeFailure=EditorLegacyGiftBlast::error();return false;}
    if(textRecord!=int(text)){
        textRecord=-1;if(!unpack(file.get(),text,textPixels(),TextBytes))return false;textRecord=int(text);
    }
    if(iconRecord!=int(icon)){
        iconRecord=-1;if(!unpack(file.get(),icon,iconPixels(),IconBytes))return false;iconRecord=int(icon);
    }
    sceGuTexFlush();return true;
}
inline float slotX(unsigned slot) {
    // master.tscn places slots nine and above at x=-256 relative to x=227.
    return (227.f-(slot>=9?256.f:25.6f*slot))*(17.f/32);
}
inline void spin(float x,float y,float scale,unsigned ms) {
    const auto& p=SelectorGifts::Spin[(ms/125)%std::size(SelectorGifts::Spin)];
    setTexture(GU_PSM_8888,SelectorGifts::Width,SelectorGifts::Height,giftPixels);
    patch(p,ViewX+x+p.x*scale,y+p.y*scale,scale);
}
inline void indicators(unsigned ms) {
    if(!read)return;
    const auto& data=read();
    const uint64_t now=sceKernelGetSystemTimeWide();
    arrivalPulse.observe(data,now);
    const auto n=data.pendingCount;
    for(unsigned i=0;i<std::min<unsigned>(9,n);++i)
        if(!sequence.active() || i!=sequence.slot)spin(slotX(i),68,arrivalPulse.scale(data.pending[i],now),ms);
}
inline void iconDraw(unsigned w,unsigned h,float x,float y,float scale,bool rainbow,unsigned ms) {
    setTexture(GU_PSM_8888,128,128,iconPixels());
    if(!rainbow) {patch({0,0,int16_t(w),int16_t(h),0,0},ViewX+x,y,scale);return;}
    // The original samples its 64-entry nearest-repeat gradient using SCREEN_UV.x-TIME*0.5.
    // The containing game viewport is 408px wide, independent of the PSP sidebars.
    const float phase=(ms%2000u)*.032f;
    for(unsigned u=0;u<w;) {
        const unsigned col=static_cast<unsigned>(int(std::floor((x+(u+.5f)*scale)*64.f/408.f-phase)))&63u;
        unsigned end=u+1;
        while(end<w && (static_cast<unsigned>(int(std::floor((x+(end+.5f)*scale)*64.f/408.f-phase)))&63u)==col)++end;
        patch({int16_t(u),0,int16_t(end-u),int16_t(h),0,0},ViewX+x+u*scale,y,scale,WorldGiftArt::gradient[col]);
        u=end;
    }
}
inline void draw(unsigned clockMs) {
    sequence.drawn(false);
    indicators(clockMs);
    if(!sequence.active()) {
        if(sequence.fadeStart){
            const auto frame=std::min(30u,sequence.elapsed*60/1000);
            rectangle(ViewX+136,0,272,272,GU_RGBA(0u,0u,0u,unsigned(230*(1-WorldGiftArt::motion[frame].dark))));
        }
        return;
    }
    const unsigned ms=sequence.elapsed,id=unsigned(sequence.id);
    const auto& motion=WorldGiftArt::motion[std::min(ms,2000u)*60/1000];
    const float travel=motion.travel;
    const float ox=slotX(sequence.slot);
    rectangle(ViewX+136,0,272,272,GU_RGBA(0u,0u,0u,unsigned(230*(sequence.darkAlready?1.f:motion.dark))));
    if(ms<1750) {
        spin(ox+(272-ox)*travel+motion.shake*(17.f/32),68+68*travel,(.2f+.8f*travel)*motion.zoom,clockMs);
    }
    if(ms<1500)return;
    const unsigned index=(russian()?43:0)+id;
    const auto& panel=WorldGiftArt::panels[index];
    const float scale=motion.details;
    unsigned icon=panel.icon,w=panel.w,h=panel.h;
    float ix=panel.x,iy=panel.y;
    const bool custom=id==7 && (ArtistCopycat::base!=0 || ArtistCopycat::drawn);
    const auto& base=WorldGiftArt::bases[ArtistCopycat::base<std::size(WorldGiftArt::bases)?ArtistCopycat::base:0];
    // In the accepted EN/RU Copycat page the 128px source control is centred at (272,128).
    if(custom){icon=base.icon;w=base.w;h=base.h;ix=272+base.x;iy=128+base.y;}
    if(!prepare(panel.text,icon)){
        if(!giftGeFailure)localeText(lang("Could not read gift","Не удалось загрузить подарок"),272,236,.72f,Gold,true);
        return;
    }
    setTexture(GU_PSM_8888,256,256,textPixels());
    for(unsigned part=0;part<3;++part){
        const auto& r=WorldGiftArt::textParts[index*3+part];
        patch({r.x,r.y,r.w,r.h,0,0},ViewX+272+(r.x-128)*scale,136+(r.y-128)*scale,scale);
    }
    iconDraw(w,h,272+(ix-272)*scale,136+(iy-136)*scale,scale,id==13,clockMs);
    if(custom && ArtistCopycat::drawn){
        const float ink=68.f*512.f/std::max(base.sourceW,base.sourceH)*scale;
        ArtistInk::bind(ArtistCopycat::pixels.data());
        patch(ArtistInk::Full,ViewX+272-ink*.5f,136-8*scale-ink*.5f,ink/128);
    }
    bool effect=true;
    if(motion.blast>=0){
        effect=EditorLegacyGiftBlast::draw(unsigned(motion.blast),ViewX+272,136);
        if(!effect){
            const char* message=EditorLegacyGiftBlast::error();
            if(std::strstr(message," GE list") || std::strstr(message,"fence legacy gift pixels")){
                giftGeFailure=message;return;
            }
            localeText(lang("Could not display gift effect","Не удалось показать эффект подарка"),272,251,.58f,Gold,true);
        }
    }
    if(sequence.phase==Phase::NeedsRetry)
        localeText(lang("Could not save. X: retry","Не удалось сохранить. ×: повторить"),272,250,.65f,Gold,true);
    sequence.drawn(effect);
}
}

