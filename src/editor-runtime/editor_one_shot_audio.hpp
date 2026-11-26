#pragma once
#include <array>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <new>

// Three owners: UI publishes complete command batches; the existing archive
// reader owns source cursors/keys; the output thread owns consumed/offset.
// Pages are immutable between produced.release and consumed.release. There is
// no allocation, file access, or lock on the output thread.
template<unsigned Sources> class EditorOneShotAudio {
public:
    static constexpr unsigned Voices=32, Pages=3, Block=1024;
    static constexpr unsigned DirectVoices=1024,HeapLimit=1024*1024;
    static constexpr unsigned ResidentIds[]={306,309,310};
    static constexpr unsigned OriginalResidentBytes=Sources>310?(37981+17201+32265)*4:0;
    static constexpr unsigned Keys=4096, Commands=4096, Input=4098;
    static constexpr unsigned MemberBits=13,MemberMask=(1u<<MemberBits)-1,GenerationMask=(1u<<(32-MemberBits))-1;
    using Clock=uint32_t(*)();
    static constexpr unsigned StartupBudgetUsec=23220; // one1024-frame44100Hz output block
    struct Diagnostics {unsigned requests=0,starts=0,initialWaitBlocks=0,underrunBlocks=0,maxStartUsec=0,lateStarts=0;};
    struct Source {const int16_t* prefix=nullptr;unsigned resident=0,frames=0;};
    template<class Read> bool prepare(const Source* input,Read read) {
        clear();
        std::unique_ptr<Data> next(new(std::nothrow) Data);
        if(!next)return false;
        for(unsigned i=0;i<Sources;++i){
            if(input[i].resident>input[i].frames||(input[i].resident&&!input[i].prefix))return false;
            next->sources[i]=input[i];
        }
        if(sizeof(Data)>HeapLimit)return false;
        size_t total=0;
        for(unsigned id:ResidentIds)if(id<Sources&&input[id].frames&&input[id].resident<input[id].frames){
            if(input[id].frames>(HeapLimit-sizeof(Data)-total)/4)return false;
            total+=size_t(input[id].frames)*4;
        }
        std::unique_ptr<int16_t[]> samples(total?new(std::nothrow) int16_t[total/2]:nullptr);
        if(total&&!samples)return false;
        size_t offset=0;
        for(unsigned id:ResidentIds)if(id<Sources&&input[id].frames&&input[id].resident<input[id].frames){
            auto* out=samples.get()+offset/2;
            for(unsigned at=0;at<input[id].frames;){
                const unsigned n=std::min(4096u,input[id].frames-at);
                if(read(id,at,out+size_t(at)*2,n)!=n)return false;
                at+=n;
            }
            next->sources[id]={out,input[id].frames,input[id].frames};offset+=size_t(input[id].frames)*4;
        }
        data=std::move(next);residentPcm=std::move(samples);residentBytes=unsigned(total);
        return true;
    }
    bool prepare(const Source* input){return prepare(input,[](unsigned,unsigned,int16_t*,unsigned)->unsigned{return 0;});}
    // Only before reader/output threads start or after both have joined.
    void clear() {data.reset();residentPcm.reset();residentBytes=0;inBatch=false;staged=batch=0;clock=nullptr;clockBatchStarted=false;}
    static constexpr size_t allocationBytes();
    unsigned allocatedBytes()const{return data?unsigned(sizeof(Data))+residentBytes:0;}
    // Atomic-only producer query, used to wake the lower-priority reader after
    // publishing a whole batch. Never inspect reader-owned fields on the UI.
    bool commandPending()const{return data&&(data->published.load(std::memory_order_acquire)!=data->taken.load(std::memory_order_acquire)||
        data->resetRequest.load(std::memory_order_acquire)!=data->readerReset.load(std::memory_order_acquire));}
    // READER THREAD ONLY. Keep priority above the rendering thread for the
    // entire undecoded tail, not only while music's much deeper queue is low.
    bool readerNeedsPriority()const{
        if(!data)return false;
        if(commandPending())return true;
        for(const auto&v:data->voices)if(v.used&&!v.ended)return true;
        return false;
    }
    // Set once after prepare and before either audio thread starts.
    void setClock(Clock fn){clock=fn;clockBatchStarted=false;}
    Diagnostics diagnostics(unsigned id=Sources)const{
        Diagnostics out;if(!data)return out;
        for(unsigned i=0;i<Sources;++i){if(id<Sources&&i!=id)continue;const auto&s=data->stats[i];
            out.requests+=s.requests.load(std::memory_order_relaxed);out.starts+=s.starts.load(std::memory_order_relaxed);
            out.initialWaitBlocks+=s.initialWaitBlocks.load(std::memory_order_relaxed);out.underrunBlocks+=s.underrunBlocks.load(std::memory_order_relaxed);
            out.maxStartUsec=std::max(out.maxStartUsec,s.maxStartUsec.load(std::memory_order_relaxed));out.lateStarts+=s.lateStarts.load(std::memory_order_relaxed);
        }return out;
    }
    void beginBatch() {
        if(!data||inBatch)return;
        inBatch=true;
        if(clock){const unsigned now=clock();if(!clockBatchStarted||int32_t(now-batch)>0)batch=now;else ++batch;clockBatchStarted=true;}
        else ++batch;
        if(!batch)++batch;
    }
    void endBatch() {
        if(!data||!inBatch)return;
        data->published.store(staged,std::memory_order_release);inBatch=false;
    }
    bool play(unsigned id,uint32_t owner,uint32_t node,float gainDb=0,float pitch=1) {
        if(!data||id>=Sources||!data->sources[id].frames||!std::isfinite(gainDb)||!std::isfinite(pitch))return false;
        return enqueue(id,owner,node,unsigned(std::lround(std::pow(10.f,std::clamp(gainDb,-96.f,12.f)/20.f)*256.f)),
                       unsigned(std::clamp(pitch,.01f,31.99f)*32768.f+.5f)*2u);
    }
    // Source Nuke.queue_free destroys these Baddie nodes, including dead nodes
    // whose logical entity slots were already recycled. It never calls death().
    bool stopBaddies(){return data&&enqueue(Sources,0,0,0,0);}
    bool stopOwner(unsigned owner){return data&&enqueue(Sources+1,owner,0,0,0);}
private:
    bool enqueue(unsigned id,unsigned owner,unsigned node,unsigned gain,unsigned step){
        const bool single=!inBatch;if(single)beginBatch();
        const unsigned read=data->taken.load(std::memory_order_acquire);
        if(staged-read==Commands) {
            data->queueRejected.fetch_add(1,std::memory_order_relaxed);
            if(single)endBatch();
            return false;
        }
        auto& c=data->commands[staged%Commands];
        c={id,owner,node,gain,step,batch,data->resetRequest.load(std::memory_order_relaxed)};
        if(id<Sources){data->queued[id].fetch_add(1,std::memory_order_relaxed);data->stats[id].requests.fetch_add(1,std::memory_order_relaxed);}
        ++staged;
        if(single)endBatch();
        return true;
    }
public:
    void requestReset() {if(data)data->resetRequest.fetch_add(1,std::memory_order_release);}
    bool resetReady() const {
        return !data||(data->readerReset.load(std::memory_order_acquire)==data->resetRequest.load(std::memory_order_acquire)&&
                      data->mixerReset.load(std::memory_order_acquire)==data->resetRequest.load(std::memory_order_acquire));
    }
    unsigned rejected() const {
        return data?data->queueRejected.load()+data->voiceRejected.load()+data->keyRejected.load()+data->decodeErrors.load():0;
    }
    unsigned voiceRejections() const {return data?data->voiceRejected.load():0;}
    unsigned keyRejections() const {return data?data->keyRejected.load():0;}
    unsigned queueRejections() const {return data?data->queueRejected.load():0;}
    unsigned decodeFailures() const {return data?data->decodeErrors.load():0;}
    unsigned underruns() const {return data?data->underruns.load():0;}
    bool playing(unsigned id) const {
        if(!data||id>=Sources)return false;
        if(data->queued[id].load(std::memory_order_acquire))return true;
        for(const auto& v:data->voices)
            if(v.activeId.load(std::memory_order_acquire)==id+1)return true;
        for(const auto& v:data->direct)
            if(v.activeId.load(std::memory_order_acquire)==id+1)return true;
        return false;
    }
    // read(id,frame,destination,frames) is called only by the archive reader.
    // Its source cursor is independent of every legacy per-ID effect cursor.
    template<class Read> bool service(Read read) {
        if(!data)return false;
        auto& d=*data;bool activity=false;
        const unsigned reset=d.resetRequest.load(std::memory_order_acquire);
        if(reset!=d.readerSeen) {
            for(auto& v:d.voices)cancel(v);
            for(auto& v:d.direct)cancel(v);
            for(auto& k:d.keys)k.state=0;
            d.readerSeen=reset;activity=true;
        }
        reap();
        const unsigned published=d.published.load(std::memory_order_acquire);
        unsigned at=d.taken.load(std::memory_order_relaxed);
        while(at!=published) {
            const auto c=d.commands[at%Commands];
            if(c.reset==d.readerSeen){if(c.id<Sources)accept(c);else stopMatching(c.id==Sources,c.owner);}
            if(c.id<Sources)d.queued[c.id].fetch_sub(1,std::memory_order_release);
            d.taken.store(++at,std::memory_order_release);activity=true;
        }
        // The reset ACK also covers removal of every already-published old
        // command, leaving queue capacity available to the next scene.
        d.readerReset.store(d.readerSeen,std::memory_order_release);
        for(unsigned n=0;n<Voices;++n) {
            const unsigned index=(d.next+n)%Voices;auto& v=d.voices[index];
            if(!v.used||v.ended)continue;
            const unsigned write=v.produced.load(std::memory_order_relaxed);
            if(write-v.consumed.load(std::memory_order_acquire)>=Pages)continue;
            auto& page=v.pages[write%Pages];const auto& source=d.sources[v.id];
            const uint64_t end=uint64_t(source.frames)<<16;
            const unsigned size=unsigned(std::min<uint64_t>(Block,(end-v.phase+v.step-1)/v.step));
            uint64_t phase=v.phase;
            // High authored pitch must not expand the shared source scratch or
            // shorten the three output pages. Decode bounded source sub-slices.
            const unsigned sliceLimit=unsigned((uint64_t(Input-2)<<16)/v.step)+1;
            for(unsigned offset=0;offset<size;) {
                const unsigned chunk=std::min(size-offset,sliceLimit),first=unsigned(phase>>16);
                const unsigned last=unsigned(std::min<uint64_t>(source.frames-1,((phase+uint64_t(chunk-1)*v.step)>>16)+1));
                const unsigned needed=last-first+1;
                if(needed>Input||!sourceRead(read,v.id,first,d.input,needed)) {
                    d.decodeErrors.fetch_add(1,std::memory_order_relaxed);cancel(v);return true;
                }
                for(unsigned i=0;i<chunk;++i,phase+=v.step) {
                    const unsigned frame=unsigned(phase>>16)-first,next=std::min(frame+1,needed-1),fraction=unsigned(phase&65535);
                    for(unsigned ch=0;ch<2;++ch) {
                        const int32_t a=d.input[frame*2+ch],b=d.input[next*2+ch];
                        const int64_t sample=int64_t(a)+int64_t(b-a)*fraction/65536;
                        // Interpolation stays inside the int16 endpoints. Gain
                        // is applied later in int64, so this storage is exact.
                        page.samples[(offset+i)*2+ch]=int16_t(sample);
                    }
                }
                offset+=chunk;
            }
            v.phase=phase;v.ended=phase>=end;
            page.generation=v.generation;page.frames=size;page.gain=v.gain;page.last=v.ended;
            v.produced.store(write+1,std::memory_order_release);
            if(!v.ready){v.ready=true;recordStart(v);publish(v);v.pendingGeneration.store(0,std::memory_order_release);}
            d.next=(index+1)%Voices;return true;
        }
        return activity;
    }
    void mix(int32_t* output,unsigned frames,bool paused,unsigned gain=256) {
        if(!data||!output)return;
        for(unsigned offset=0;offset<frames;) {
            const unsigned size=std::min(Block,frames-offset);
            mixBlock(output+offset*2,size,paused,std::min(gain,256u));offset+=size;
        }
    }
private:
    struct Command {unsigned id,owner,node,gain,step,batch,reset;};
    struct Key {unsigned owner=0,node=0,generation=0;uint16_t voice=0;uint8_t state=0;};
    struct Page {int16_t samples[Block*2];unsigned generation=0,frames=0,gain=256;bool last=false;};
    struct Cohort {
        // Generation+membership are one lock-free publication. A reader restart
        // cannot make the output apply a new cohort's count to an old page.
        std::atomic<unsigned> state{0},finished{0},activeId{0};
        // Reader only.
        bool used=false,ended=false;unsigned id=0,gain=0,step=0,batch=0,generation=0;
    };
    struct Voice:Cohort {
        Page pages[Pages];std::atomic<unsigned> produced{0},consumed{0};
        uint64_t phase=0;bool ready=false;unsigned members=0;std::atomic<unsigned> pendingGeneration{0};
        // Mixer only.
        unsigned offset=0,seen=0;
    };
    struct Direct:Cohort {
        // Coherent snapshots use these atomics plus a double state check;
        // reader slot reuse never races non-atomic playback parameters.
        std::atomic<unsigned> publishedId{0},publishedStep{0},publishedGain{0};
        uint64_t phase=0;unsigned seen=0;
    };
    struct Stats {std::atomic<unsigned> requests{0},starts{0},initialWaitBlocks{0},underrunBlocks{0},maxStartUsec{0},lateStarts{0};};
    struct Data {
        Source sources[Sources];Voice voices[Voices];Direct direct[DirectVoices];Key keys[Keys];Command commands[Commands];
        int16_t input[Input*2];int64_t accumulation[Block*2];
        std::atomic<unsigned> queued[Sources]{};Stats stats[Sources];
        std::atomic<unsigned> published{0},taken{0},resetRequest{0},readerReset{0},mixerReset{0};
        std::atomic<unsigned> queueRejected{0},voiceRejected{0},keyRejected{0},decodeErrors{0},underruns{0};
        unsigned readerSeen=0,next=0;
    };
    template<class Read> bool sourceRead(Read read,unsigned id,unsigned first,int16_t* out,unsigned count) {
        const auto& s=data->sources[id];unsigned resident=0;
        if(s.prefix&&first<s.resident) {
            resident=std::min(count,s.resident-first);
            std::copy_n(s.prefix+size_t(first)*2,resident*2,out);
        }
        return resident==count||read(id,first+resident,out+resident*2,count-resident)==count-resident;
    }
    static void cancel(Cohort& v) {
        v.state.store(0,std::memory_order_seq_cst);
        v.activeId.store(0,std::memory_order_release);v.used=false;
    }
    Cohort& cohort(unsigned index){return index<Voices?static_cast<Cohort&>(data->voices[index]):static_cast<Cohort&>(data->direct[index-Voices]);}
    void remove(Key& k){
        if(k.state!=1)return;
        auto& old=cohort(k.voice);
        if(old.used&&old.generation==k.generation){
            if(k.voice<Voices)removeMember(data->voices[k.voice]);else removeMember(data->direct[k.voice-Voices]);
        }
        k.state=2;
    }
    void stopMatching(bool baddies,unsigned owner){
        for(auto& k:data->keys)if(k.state==1){
            const auto& v=cohort(k.voice);
            if(baddies?(v.id>=306&&v.id<=310):k.owner==owner)remove(k);
        }
    }
    unsigned keySlot(unsigned owner,unsigned node) const {
        const unsigned hash=(owner*0x9e3779b9u)^(node*0x85ebca6bu);
        unsigned empty=Keys;
        for(unsigned n=0;n<Keys;++n) {
            const unsigned i=(hash+n)&(Keys-1);const auto& k=data->keys[i];
            if(k.state==1&&k.owner==owner&&k.node==node)return i;
            if(k.state!=1&&empty==Keys)empty=i;
            if(!k.state)break;
        }
        return empty;
    }
    void accept(const Command& c) {
        auto& d=*data;const unsigned key=keySlot(c.owner,c.node);
        if(key==Keys){d.keyRejected.fetch_add(1,std::memory_order_relaxed);return;}
        auto& k=d.keys[key];
        remove(k);
        const auto& source=d.sources[c.id];
        if(source.prefix&&source.resident==source.frames)acceptInto(c,k,d.direct,Voices);
        else acceptInto(c,k,d.voices,0);
    }
    static void initialize(Voice& v,const Command&){v.phase=0;v.ready=false;v.members=0;v.pendingGeneration.store(v.generation,std::memory_order_release);}
    static void initialize(Direct& v,const Command& c){
        v.publishedId.store(c.id,std::memory_order_seq_cst);
        v.publishedStep.store(c.step,std::memory_order_seq_cst);
        v.publishedGain.store(c.gain,std::memory_order_seq_cst);
    }
    static void publish(Voice&v){if(v.ready)v.state.store((v.generation<<MemberBits)|v.members,std::memory_order_seq_cst);}
    static unsigned memberCount(const Voice&v){return v.members;}
    static unsigned memberCount(const Direct&v){return v.state.load(std::memory_order_relaxed)&MemberMask;}
    static void setMembers(Voice&v,unsigned n){v.members=n;publish(v);}
    static void setMembers(Direct&v,unsigned n){v.state.store((v.generation<<MemberBits)|n,std::memory_order_seq_cst);}
    template<class V>static void removeMember(V&v){const unsigned n=memberCount(v);if(n<=1)cancel(v);else setMembers(v,n-1);}
    void recordStart(const Cohort&v){
        auto&s=data->stats[v.id];s.starts.fetch_add(1,std::memory_order_relaxed);
        if(clock){unsigned elapsed=clock()-v.batch;if(elapsed>INT32_MAX)elapsed=0;
            if(elapsed>s.maxStartUsec.load(std::memory_order_relaxed))s.maxStartUsec.store(elapsed,std::memory_order_relaxed);
            if(elapsed>StartupBudgetUsec)s.lateStarts.fetch_add(1,std::memory_order_relaxed);
        }
    }
    void recordDirectStart(const Voice&){}
    void recordDirectStart(const Direct&v){recordStart(v);}
    static unsigned queuedPages(const Voice&v){return v.produced.load(std::memory_order_relaxed)-v.consumed.load(std::memory_order_acquire);}
    static unsigned queuedPages(const Direct&){return 0;}
    template<class V,unsigned N>void acceptInto(const Command& c,Key& k,V(&voices)[N],unsigned base){
        unsigned chosen=N,free=N,depth=UINT32_MAX;
        for(unsigned i=0;i<N;++i) {
            const auto& v=voices[i];
            if(v.used&&v.id==c.id&&v.step==c.step&&v.gain==c.gain&&v.batch==c.batch){chosen=i;break;}
            if(!v.used&&queuedPages(v)<depth){free=i;depth=queuedPages(v);}
        }
        if(chosen==N) {
            chosen=free;
            if(chosen==N){data->voiceRejected.fetch_add(1,std::memory_order_relaxed);return;}
            auto& v=voices[chosen];
            v.used=true;v.ended=false;v.id=c.id;v.step=c.step;v.gain=c.gain;v.batch=c.batch;
            v.generation=(v.generation+1)&GenerationMask;if(!v.generation)++v.generation;
            v.state.exchange(0,std::memory_order_seq_cst);v.activeId.store(c.id+1,std::memory_order_release);
            initialize(v,c);recordDirectStart(v);
        }
        auto& v=voices[chosen];
        k.owner=c.owner;k.node=c.node;k.voice=uint16_t(base+chosen);k.generation=v.generation;k.state=1;
        setMembers(v,memberCount(v)+1);
    }
    void reap() {
        auto& d=*data;
        for(unsigned i=0;i<Voices+DirectVoices;++i) {
            auto& v=cohort(i);
            if(!v.used||v.finished.load(std::memory_order_acquire)!=v.generation)continue;
            for(auto& k:d.keys)if(k.state==1&&k.voice==i&&k.generation==v.generation)k.state=2;
            cancel(v);
        }
    }
    void mixBlock(int32_t* out,unsigned size,bool paused,unsigned gain) {
        auto& d=*data;std::fill_n(d.accumulation,size*2,int64_t(0));
        std::array<bool,Sources> initialWaiting{};
        const unsigned reset=d.resetRequest.load(std::memory_order_acquire);
        const unsigned resetAtStart=d.readerReset.load(std::memory_order_acquire);
        for(auto& v:d.voices) {
            unsigned read=v.consumed.load(std::memory_order_relaxed),at=0;
            const unsigned state=v.state.load(std::memory_order_acquire),wanted=state>>MemberBits,members=state&MemberMask;
            if(wanted!=v.seen){v.offset=0;v.seen=wanted;}
            const unsigned pendingId=v.activeId.load(std::memory_order_acquire);
            if(!wanted&&pendingId&&pendingId<=Sources)initialWaiting[pendingId-1]=true;
            while(at<size) {
                const unsigned write=v.produced.load(std::memory_order_acquire);
                if(read==write)break;
                const auto& page=v.pages[read%Pages];
                // A first page can precede activation by one preemption. Keep
                // that page intact; discard only older canceled generations.
                if(!wanted&&v.activeId.load(std::memory_order_acquire)&&page.generation==v.pendingGeneration.load(std::memory_order_acquire))break;
                if(v.state.load(std::memory_order_acquire)!=state)break;
                if(!wanted||page.generation!=wanted||!members) {
                    v.offset=0;v.consumed.store(++read,std::memory_order_release);continue;
                }
                if(paused)break;
                const unsigned n=std::min(size-at,page.frames-v.offset);
                if(gain)for(unsigned i=0;i<n*2;++i)
                    d.accumulation[at*2+i]+=((int64_t(page.samples[v.offset*2+i])*page.gain/256)*gain/256)*members;
                v.offset+=n;at+=n;
                if(v.offset==page.frames) {
                    if(page.last)v.finished.store(wanted,std::memory_order_release);
                    v.offset=0;v.consumed.store(++read,std::memory_order_release);
                }
            }
            if(!paused&&wanted&&members&&at<size&&v.finished.load(std::memory_order_relaxed)!=wanted&&v.state.load(std::memory_order_acquire)==state){
                d.underruns.fetch_add(1,std::memory_order_relaxed);
                const unsigned id=v.activeId.load(std::memory_order_acquire);
                if(id&&id<=Sources&&v.state.load(std::memory_order_acquire)==state)d.stats[id-1].underrunBlocks.fetch_add(1,std::memory_order_relaxed);
            }
        }
        if(!paused)for(unsigned id=0;id<Sources;++id)
            if(initialWaiting[id]||d.queued[id].load(std::memory_order_acquire))d.stats[id].initialWaitBlocks.fetch_add(1,std::memory_order_relaxed);
        for(auto& v:d.direct){
            unsigned state=v.state.load(std::memory_order_seq_cst),id=0,step=0,voiceGain=0;
            bool stable=false;
            for(unsigned attempt=0;attempt<4&&state;++attempt){
                id=v.publishedId.load(std::memory_order_seq_cst);step=v.publishedStep.load(std::memory_order_seq_cst);voiceGain=v.publishedGain.load(std::memory_order_seq_cst);
                const unsigned after=v.state.load(std::memory_order_seq_cst);
                if(after==state){stable=true;break;}state=after;
            }
            if(!state)continue;
            if(!stable){d.underruns.fetch_add(1,std::memory_order_relaxed);continue;}
            const unsigned wanted=state>>MemberBits,members=state&MemberMask;
            if(wanted!=v.seen){v.phase=0;v.seen=wanted;}
            if(paused||!members)continue;
            const auto& source=d.sources[id];const uint64_t end=uint64_t(source.frames)<<16;
            for(unsigned i=0;i<size&&v.phase<end;++i,v.phase+=step){
                const unsigned at=unsigned(v.phase>>16),next=std::min(at+1,source.frames-1),fraction=unsigned(v.phase&65535);
                if(gain)for(unsigned ch=0;ch<2;++ch){
                    const int32_t a=source.prefix[at*2+ch],b=source.prefix[next*2+ch];
                    const int64_t sample=int64_t(a)+int64_t(b-a)*fraction/65536;
                    d.accumulation[i*2+ch]+=((sample*voiceGain/256)*gain/256)*members;
                }
            }
            if(v.phase>=end)v.finished.store(wanted,std::memory_order_release);
        }
        for(unsigned i=0;i<size*2;++i)
            out[i]=int32_t(std::clamp(int64_t(out[i])+d.accumulation[i],int64_t(INT32_MIN),int64_t(INT32_MAX)));
        // A reader reset ACK guarantees no old page can be published afterward.
        // This block has dropped every queued old-generation page before ACK.
        if(resetAtStart==reset)d.mixerReset.store(reset,std::memory_order_release);
    }
    std::unique_ptr<Data> data;
    std::unique_ptr<int16_t[]> residentPcm;
    unsigned residentBytes=0;
    bool inBatch=false;unsigned staged=0,batch=0;Clock clock=nullptr;bool clockBatchStarted=false;
};
template<unsigned N> constexpr size_t EditorOneShotAudio<N>::allocationBytes(){return sizeof(Data)+OriginalResidentBytes;}
