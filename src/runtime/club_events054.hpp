#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "club_geometry054.hpp"
#include "club_tracks054.hpp"

namespace ClubEvents {
using ClubTracks::Actor;
using ClubTracks::Pose;
enum class Cue { None, Talk, Hurt, AppleDeath, Bibizu, Bob };
enum class Greenhead { Alive, Explosion, Gone };
struct LocalCivilDateTime { int year=0,month=0,day=0,weekday=0,hour=0,minute=0,second=0; bool dst=false; };
struct Clock { int64_t unixSeconds=0; LocalCivilDateTime local{}; int utcWeekday=0,utcHour=0; };
struct ClubPersistent { bool pennyHi=false,appleHasDied=false; int64_t appleDeathUnix=0; LocalCivilDateTime appleDeathLocal{}; };
inline constexpr char ObituaryText[]="A sad day indeed. The dearly loved Apple John of Golfing fame has left this earth. His wife and son will surely miss him. But he would not have wanted us to cry, Apple John would want us to keep on going, keep on striving to be the best we can be. Rest in Piece, Apple John.";
struct Writer {
    void* context=nullptr;
    bool (*write)(void*,const ClubPersistent&)=nullptr;
    bool commit(const ClubPersistent& p) const {return write && write(context,p);}
};
struct Person {
    float x=0,y=0;
    unsigned ms=0;
    Pose pose=Pose::Idle;
    bool visible=true,solid=true,talk=true,ouch=true,shocked=false,peace=false;
    void play(Pose next) {pose=next;ms=0;}
};
struct Reaction { const char* key=nullptr; Cue cue=Cue::None; bool stop=false,retry=false; };
struct Visit {
    std::array<Person,static_cast<unsigned>(Actor::Count)> people{};
    ClubPersistent persistent{};
    bool appleArmed=false,buried=false,pennyTalked=false,sockbug=false,bobScare=false;
    float blueSpeed=0;
    unsigned computer=0,boomMs=0,bobMs=0;
    Greenhead greenhead=Greenhead::Alive;
    Actor speaker=Actor::Count;
    unsigned talkRemaining=0;
    char key[100]{};
    Person& operator[](Actor a) {return people[static_cast<unsigned>(a)];}
    const Person& operator[](Actor a) const {return people[static_cast<unsigned>(a)];}
    static_assert(std::size(ucg_club054::npcs)==static_cast<unsigned>(Actor::Count));
    static_assert(std::size(ucg_club054::shapes)==std::size(ClubTracks::shapeOwners));
    static constexpr Actor Roberts[]={Actor::RobSunflower,Actor::RobBlue,Actor::RobAlcoholic,Actor::RobSlime,Actor::RobEvil,Actor::CreditsPacman};
    void shock() {
        for(auto a:Roberts) {
            auto& p=(*this)[a];p.shocked=true;p.play(Pose::Shock);
            if(a!=Actor::CreditsPacman)p.ouch=false;
        }
    }
    static bool atWork(const Clock& t) {
        const int h=t.utcHour+(t.local.dst?1:0);
        if(t.utcWeekday==3)return h>=2 && h<=8;
        return (t.utcWeekday==1 || (t.utcWeekday>=4 && t.utcWeekday<=6)) && h>=14 && h<=21;
    }
    void enter(const ClubPersistent& snapshot,const Clock& now,unsigned meditationRoll,unsigned computerRoll) {
        *this={};persistent=snapshot;computer=computerRoll&1;
        for(unsigned i=0;i<people.size();++i) {
            const auto& n=ucg_club054::npcs[i];people[i].x=n.anchor.x;people[i].y=n.anchor.y;
        }
        (*this)[Actor::Bibizu].solid=false;
        sockbug=atWork(now);
        auto& penny=(*this)[Actor::Penny];
        if(snapshot.pennyHi && !sockbug && meditationRoll==2) {
            penny.x=808;penny.y=432;penny.peace=true;penny.ouch=false;penny.play(Pose::Zazen);
        }
        if(!snapshot.appleHasDied)return;
        auto& apple=(*this)[Actor::AppleJohn];apple.talk=apple.ouch=false;shock();
        const uint64_t age=now.unixSeconds>snapshot.appleDeathUnix?
            uint64_t(now.unixSeconds)-uint64_t(snapshot.appleDeathUnix):0;
        if(age<=1555200) {
            apple.play(age>1036800?Pose::Skeleton:age>518400?Pose::Decomposition:Pose::Dead);return;
        }
        buried=true;apple.solid=false;apple.visible=now.local.hour>22 || now.local.hour<6;
        apple.x=1384;apple.y=136;
        for(auto a:Roberts)(*this)[a].visible=false;
        auto& slime=(*this)[Actor::RobSlime];
        slime.visible=true;slime.shocked=false;slime.peace=now.local.hour>=6 && now.local.hour<12;
        slime.x=slime.peace?888:896;slime.y=slime.peace?432:360;slime.play(Pose::Idle);
    }
    void armApple() {
        if(persistent.appleHasDied)return;
        appleArmed=true;(*this)[Actor::AppleJohn].talk=false;
    }
    const char* base(Actor a) const {
        const auto& p=(*this)[a];
        if(a==Actor::CreditsPacman && p.shocked)return "MENU_CreditsExtraGoopieWasted";
        if(a==Actor::Penny && sockbug)return "MENU_CreditsPlaytestSockbug";
        if((a==Actor::Penny || a==Actor::RobSlime) && p.peace)return "MENU_CreditsRobertSlime_Meditation";
        if(a==Actor::RobSlime && buried)return "MENU_CreditsRobertSlime_Therapy";
        return ucg_club054::npcs[unsigned(a)].dialogue;
    }
    void silence() {
        if(speaker!=Actor::Count && (*this)[speaker].pose==Pose::Talk)(*this)[speaker].play(Pose::Idle);
        speaker=Actor::Count;talkRemaining=0;
    }
    void typeFor(Actor a,unsigned milliseconds) {
        speaker=a;talkRemaining=milliseconds;
    }
    Reaction contact(Actor a,bool ouch,const Clock& now,unsigned repeatRoll,Writer writer) {
        auto& p=(*this)[a];
        if(!p.visible || !(ouch?p.ouch:p.talk))return {};
        if(ouch && a==Actor::AppleJohn && appleArmed) {
            if(now.unixSeconds<=0)return {nullptr,Cue::None,true,true};
            auto next=persistent;next.appleHasDied=true;next.appleDeathUnix=now.unixSeconds;next.appleDeathLocal=now.local;
            if(!writer.commit(next))return {nullptr,Cue::None,true,true};
            persistent=next;p.ouch=p.talk=false;p.play(Pose::Dead);shock();
            return {nullptr,Cue::AppleDeath,true};
        }
        if(ouch && a==Actor::Bobmunculua) {
            if(bobScare)return {};
            bobScare=true;bobMs=0;return {nullptr,Cue::Bob};
        }
        if(!ouch && a==Actor::Penny && !persistent.pennyHi) {
            auto next=persistent;next.pennyHi=true;
            if(!writer.commit(next))return {nullptr,Cue::None,true,true};
            persistent=next;
        }
        const char* suffix="";
        if(ouch)suffix="_O";
        else if(a==Actor::Penny && pennyTalked)suffix=repeatRoll==2?"_2":"_1";
        else if(p.shocked && a!=Actor::CreditsPacman)suffix="_T";
        std::snprintf(key,sizeof key,"%s%s",base(a),suffix);
        silence();
        if(!ouch) {
            if(a==Actor::Penny && !sockbug)pennyTalked=true;
            if(!p.shocked && !p.peace)p.play(Pose::Talk);
            return {key,p.shocked || p.peace?Cue::None:Cue::Talk,true};
        }
        if(p.shocked)return {key,Cue::None};
        p.play(Pose::Hurt);
        if(a==Actor::Bibizu) {p.talk=p.ouch=false;return {key,Cue::Bibizu};}
        if(a==Actor::RobBlue)blueSpeed=200;
        return {key,a==Actor::RobBlue || a==Actor::AppleJohn?Cue::None:Cue::Hurt};
    }
    bool explode() {
        if(greenhead!=Greenhead::Alive)return false;
        greenhead=Greenhead::Explosion;computer=2;boomMs=0;return true;
    }
    void finishBoom() {if(greenhead==Greenhead::Explosion)greenhead=Greenhead::Gone;}
    void advance(unsigned ms) {
        for(unsigned i=0;i<people.size();++i) {
            auto& p=people[i];p.ms+=ms;
            if(p.pose!=Pose::Hurt)continue;
            auto a=static_cast<Actor>(i);
            const auto& t=ClubTracks::track(a,p.pose,sockbug);
            if(!t.loop && p.ms>=t.lengthMs)p.play(a==Actor::Bibizu?Pose::Dead:Pose::Idle);
        }
        if(talkRemaining && ms>=talkRemaining)silence();
        else if(talkRemaining)talkRemaining-=ms;
        if(greenhead==Greenhead::Explosion)boomMs+=ms;
        if(bobScare)bobMs=std::min(2000u,bobMs+ms);
    }
    void physicsStep() {
        if(blueSpeed>0 && (*this)[Actor::RobBlue].visible) {
            auto& x=(*this)[Actor::RobBlue].x;
            x-=blueSpeed/60.0f;blueSpeed+=400.0f/60;
            if(x < -2048)(*this)[Actor::RobBlue].visible=false;
        }
    }
};
} // namespace ClubEvents
