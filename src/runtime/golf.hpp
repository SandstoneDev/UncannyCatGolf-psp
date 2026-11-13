#pragma once
#include "campaign.hpp"
#include "game.hpp"
#include "campaign.hpp"

constexpr unsigned GolfStages=18, GolfHazardStages=CourseCount-TutorialCount, MaxExtraGhosts=7;
constexpr unsigned MaxSand=8, MaxVertices=16, MaxBubbles=12, BubbleTicks=240;
constexpr unsigned MaxBlocks=32, MaxWater=8, MaxSkulls=8;
constexpr unsigned BlockAtlasWidth=256, BlockAtlasHeight=256;
constexpr unsigned BlockSprites=8*(BlockAtlasHeight/24);
constexpr unsigned RushHourGolfStage=GolfStages+5, DimlitGolfStage=GolfStages+7, BankGolfStage=GolfStages+10;
constexpr unsigned RectangularBlockCount=3;
constexpr unsigned RushHourBlockAtlasY=216, RushHourBlockAtlasStride=80;
constexpr unsigned DimlitBlockAtlasY=192, DimlitBlockAtlasStride=64;
constexpr unsigned BankBlockAtlasX=228, BankBlockAtlasY=184;
constexpr unsigned LightboxBlockAtlasY=240, LightboxBlockAtlasStride=16;

struct CourseArea {
    uint32_t count;
    Vec2 points[MaxVertices];

    bool touches(Vec2 position,float radius) const {
        bool inside=false;
        for (unsigned i=0;i<count;++i) {
            const Vec2 start=points[i], end=points[(i+1)%count];
            const Vec2 offset=position-start, edge=end-start;
            const float projection=dot(offset,edge);
            const float cross=edge.x*offset.y-edge.y*offset.x;
            if (dot(offset,offset)<=radius*radius ||
                (projection>0 && projection<dot(edge,edge) && cross*cross<=radius*radius*dot(edge,edge))) {
                return true;
            }
            if ((start.y>position.y)!=(end.y>position.y) && ((cross>0)==(edge.y>0))) inside=!inside;
        }
        return inside;
    }
};

struct Bubble {
    Vec2 position;
    float scale;
    uint32_t sprite,phase;
};

struct SkullSpawn { Vec2 position,direction; };

struct GolfStage {
    uint32_t sandCount,bubbleCount;
    CourseArea sand[MaxSand];
    Bubble bubbles[MaxBubbles];
    uint32_t blockCount,waterCount,firstBlock;
    SolidBlock blocks[MaxBlocks];
    CourseArea water[MaxWater];
    uint32_t skullCount,hasKey;
    SkullSpawn skulls[MaxSkulls];
    Vec2 keyHint,keyTrigger,keyHalfSize;
    uint32_t extraGhostCount;
    Vec2 extraGhostSpawns[MaxExtraGhosts];

    bool onSand(Vec2 position,float radius) const {
        return touchesArea(sand,sandCount,position,radius);
    }

    bool inWater(Vec2 position,float radius) const {
        return touchesArea(water,waterCount,position,radius);
    }

private:
    static bool touchesArea(const CourseArea* areas,unsigned count,Vec2 position,float radius) {
        for (unsigned i=0;i<count;++i) {
            if (areas[i].touches(position,radius)) return true;
        }
        return false;
    }
};

struct GolfData {
    uint32_t magic;
    GolfStage stages[GolfHazardStages];
    float damping;
    float bubblePoses[2][BubbleTicks];
};
static_assert(sizeof(GolfStage)==3104 && sizeof(GolfData)==8+GolfHazardStages*sizeof(GolfStage)+2*BubbleTicks*sizeof(float));

inline bool readGolfData(std::FILE* file,GolfData& data) {
    if (std::fread(&data,sizeof data,1,file)!=1 || data.magic!=0x49474355 ||
        !std::isfinite(data.damping) || data.damping<=0 || data.damping>=1) return false;
    unsigned nextBlock=0;
    for (unsigned index=0;index<GolfHazardStages;++index) {
        const auto& stage=data.stages[index];
        const bool rushHour=index==RushHourGolfStage, dimlit=index==DimlitGolfStage;
        const bool bank=index==BankGolfStage;
        const bool park=index+TutorialCount>=ChuckleFirstCourse;
        if (((rushHour || dimlit) && stage.blockCount!=RectangularBlockCount) ||
            (bank && stage.blockCount!=1) || stage.sandCount>MaxSand || stage.bubbleCount>MaxBubbles ||
            stage.blockCount>MaxBlocks || stage.waterCount>MaxWater ||
            stage.skullCount>MaxSkulls || stage.hasKey>1 || stage.extraGhostCount>MaxExtraGhosts ||
            stage.firstBlock!=nextBlock || (!park && stage.blockCount>BlockSprites-nextBlock)) return false;
        for (unsigned i=0;i<stage.extraGhostCount;++i) {
            const Vec2 spawn=stage.extraGhostSpawns[i];
            if (!std::isfinite(spawn.x) || !std::isfinite(spawn.y)) return false;
        }
        nextBlock+=stage.blockCount;
        if (stage.hasKey) {
            for (Vec2 v:{stage.keyHint,stage.keyTrigger,stage.keyHalfSize}) {
                if (!std::isfinite(v.x) || !std::isfinite(v.y)) return false;
            }
            if (stage.keyHalfSize.x<=0 || stage.keyHalfSize.y<=0) return false;
        }
        for (unsigned i=0;i<stage.skullCount;++i) {
            const auto& skull=stage.skulls[i];
            if (!std::isfinite(skull.position.x) || !std::isfinite(skull.position.y) ||
                !std::isfinite(skull.direction.x) || !std::isfinite(skull.direction.y) ||
                length(skull.direction)<0.99f || length(skull.direction)>1.01f) return false;
        }
        for (unsigned i=0;i<stage.sandCount+stage.waterCount;++i) {
            const auto& pit=i<stage.sandCount ? stage.sand[i] : stage.water[i-stage.sandCount];
            if (pit.count<3 || pit.count>MaxVertices) return false;
            for (unsigned j=0;j<pit.count;++j) {
                if (!std::isfinite(pit.points[j].x) || !std::isfinite(pit.points[j].y)) return false;
            }
        }
        for (unsigned i=0;i<stage.blockCount;++i) {
            const auto& block=stage.blocks[i];
            const Vec2 half=block.halfSize;
            const bool validSize=rushHour ? half.x>0 && half.x<=80 && half.y>0 && half.y<=24 :
                                 dimlit   ? half.x==(i==2?48:32) && half.y==16 :
                                 bank     ? half.x==32 && half.y==32 :
                                 index+TutorialCount==CosmicSecondCourse ? half.x==160 && half.y==16 :
                                 index+TutorialCount==CosmicFifteenthCourse ? (half.x==80 || half.x==176) && half.y==16 :
                                 park     ? half.x>0 && half.x<=128 && half.y>0 && half.y<=64 :
                                            half.x==16 && half.y==16;
            if (!std::isfinite(block.center.x) || !std::isfinite(block.center.y) || !validSize) return false;
        }
        for (unsigned i=0;i<stage.bubbleCount;++i) {
            const auto& bubble=stage.bubbles[i];
            if (!std::isfinite(bubble.position.x) || !std::isfinite(bubble.position.y) ||
                !std::isfinite(bubble.scale) || bubble.scale<=0 || bubble.scale>1 ||
                bubble.sprite>=3 || bubble.phase>1) return false;
        }
    }
    for (const auto& track:data.bubblePoses) {
        for (float offset:track) if (!std::isfinite(offset) || std::abs(offset)>128) return false;
    }
    return true;
}

struct SkullRun {
    static constexpr float Radius=16.0f, Speed=100.0f;
    static constexpr unsigned Bonus=25;
    static constexpr unsigned Capacity=32;
    CircleCollider bodies[Capacity]{};
    Vec2 velocity[Capacity]{};
    unsigned defeatedAt[Capacity]{};
    unsigned bonus=0;

    void restart(const GolfStage& stage) { restart(stage.skulls,stage.skullCount); }

    void restart(const SkullSpawn* spawns,unsigned count) {
        *this={};
        for (unsigned i=0;i<count;++i) {
            bodies[i]={spawns[i].position,Radius};
            velocity[i]=spawns[i].direction*Speed;
        }
    }

    void step(const TileField& walls,unsigned count,
              const CircleCollider* otherSkulls=nullptr,unsigned otherCount=0,float despawnDistance=384.0f,
              const SolidBlock* slotDoor=nullptr,const TileField* gate=nullptr,
              const SolidBlock* blocks=nullptr,unsigned blockCount=0,BlockMask broken={},
              WallContact (*otherWalls)(Vec2,Vec2,float)=nullptr) {
        for (unsigned i=0;i<count;++i) {
            auto& skull=bodies[i];
            if (skull.radius<=0) continue;
            if (std::max(std::abs(skull.center.x),std::abs(skull.center.y))>despawnDistance) {
                skull.radius=0;
                continue;
            }
            const Vec2 motion=velocity[i]*(1.0f/60.0f);
            auto contact=walls.sweep(skull.center,motion,skull.radius);
            if (otherWalls) {
                const auto other=otherWalls(skull.center,motion,skull.radius);
                if (other.time<contact.time) contact=other;
            }
            if (gate) {
                const auto wall=gate->sweep(skull.center,motion,skull.radius);
                if (wall.time<contact.time) contact=wall;
            }
            for (unsigned j=0;j<count;++j) {
                if (j!=i) sweepCircle(bodies[j],skull.center,motion,skull.radius,contact);
            }
            for (unsigned j=0;j<blockCount;++j)
                if (!broken.has(j)) TileField::sweepBlock(blocks[j],skull.center,motion,skull.radius,contact);
            if (slotDoor) TileField::sweepBlock(*slotDoor,skull.center,motion,skull.radius,contact);
            for (unsigned j=0;j<otherCount;++j)
                sweepCircle(otherSkulls[j],skull.center,motion,skull.radius,contact);
            skull.center=skull.center+motion*std::min(contact.time,1.0f);
            if (contact.time<=1.0f) {
                velocity[i]=velocity[i]-contact.normal*(2.0f*dot(velocity[i],contact.normal));
            }
        }
    }

    void defeat(unsigned index,unsigned tick) {
        bodies[index].radius=0;
        defeatedAt[index]=tick;
        bonus+=Bonus;
    }
};

struct GhostMotion {
    Vec2 position{},velocity{};

    bool chase(Vec2 target) {
        const Vec2 offset=target-position;
        const float distance=length(offset);
        if (distance>0.001f) velocity=velocity+offset*(0.35f/distance);
        const float speed=length(velocity);
        if (speed>40.0f) velocity=velocity*(40.0f/speed);
        position=position+velocity*(1.0f/60.0f);
        const Vec2 remaining=target-position;
        return dot(remaining,remaining)<=27.9655f*27.9655f;
    }
};

inline bool touchesKey(const GolfStage& stage,Vec2 point,float radius) {
    const Vec2 offset=point-stage.keyTrigger;
    const float x=std::max(std::abs(offset.x)-stage.keyHalfSize.x,0.0f);
    const float y=std::max(std::abs(offset.y)-stage.keyHalfSize.y,0.0f);
    return x*x+y*y<=radius*radius;
}
