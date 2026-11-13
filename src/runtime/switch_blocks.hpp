#pragma once

#include <array>
#include <memory>
#include <new>
#include <utility>

class SwitchStage {
public:
    static constexpr unsigned MaxLayers=8, MaxBlocks=32;

    SolidBlock blocks[MaxBlocks]{};
    unsigned blockCount=0,layerCount=0;

    bool load(const char* const* layerPaths,unsigned layers,
              const SolidBlock* newBlocks,unsigned count,uint32_t fixedFaces=0,TileField* borrowed=nullptr) {
        if (layers>MaxLayers || count>MaxBlocks || (layers && !layerPaths) ||
            (count && !newBlocks)) return false;
        for (unsigned i=0;i<count;++i) {
            const auto& block=newBlocks[i];
            if (!std::isfinite(block.center.x) || !std::isfinite(block.center.y) ||
                !std::isfinite(block.halfSize.x) || !std::isfinite(block.halfSize.y) ||
                block.halfSize.x<=0 || block.halfSize.y<=0) return false;
        }

        std::array<std::unique_ptr<TileField>,MaxLayers> fields;
        if (layers) {
            for (unsigned i=0;i<layers;++i) {
                if (!layerPaths[i]) return false;
                if (!borrowed) fields[i].reset(new(std::nothrow) TileField);
                auto* field=borrowed?new (borrowed+i) TileField:fields[i].get();
                if (!field) return false;
                std::FILE* file=std::fopen(layerPaths[i],"rb");
                if (!file) return false;
                const bool loaded=field->read(file);
                std::fclose(file);
                if (!loaded) return false;
            }
        }

        if (count) std::copy_n(newBlocks,count,blocks);
        blockCount=count;
        layerCount=layers;
        keepFace=fixedFaces;
        collision=std::move(fields);
        this->borrowed=borrowed;
        return true;
    }

    void setGridOffset(Vec2 offset) {
        for (unsigned i=0;i<layerCount;++i) (borrowed?borrowed+i:collision[i].get())->setOffset(offset);
    }

    const TileField* tiles(unsigned state) const {
        return state<layerCount?(borrowed?borrowed+state:collision[state].get()):nullptr;
    }

    unsigned face(unsigned index,unsigned state) const {
        return index<blockCount && !(keepFace&(1u<<index))?state:0;
    }

    explicit operator bool() const { return blockCount || layerCount; }

private:
    uint32_t keepFace=0;
    TileField* borrowed=nullptr;
    std::array<std::unique_ptr<TileField>,MaxLayers> collision;
};

struct SwitchRun {
    unsigned state=0;
    int hitBlock=-1;

    void restart(const SwitchStage* next) {
        stage=next;
        state=0;
        hitBlock=-1;
        recovering=false;
    }

    unsigned face(unsigned index) const { return stage?stage->face(index,state):0; }

    void strike(unsigned index) {
        if (!stage || index>=stage->blockCount) return;
        hitBlock=static_cast<int>(index);
        if (stage->layerCount>1) setState((state+1)%stage->layerCount);
    }

    void setState(unsigned next,bool recoverNew=true) {
        if (!stage || next>stage->layerCount || next==state) return;
        state=next;
        recovering=recoverNew && next<stage->layerCount;
    }

    const TileField* activeTiles() const { return stage?stage->tiles(state):nullptr; }

    Vec2 recover(Vec2& position,float radius,const TileField* fixed,const TileField* auxiliary=nullptr) {
        if (!recovering) return {};
        const TileField* active=stage->tiles(state);
        if (!active) {
            recovering=false;
            return {};
        }
        // Godot resolves overlap with newly enabled colliders before moving.
        Vec2 overlap=active->recoverCircle(position,radius);
        if (dot(overlap,overlap)>0) {
            constexpr float safeRadiusPadding=0.08f;
            const float safeRadius=radius+safeRadiusPadding;
            for (unsigned attempt=0;attempt<4;++attempt) {
                Vec2 shift=active->recoverCircle(position,safeRadius);
                if (fixed && fixed->active()) shift=fixed->recoverCircle(position,safeRadius,shift);
                if (auxiliary) shift=auxiliary->recoverCircle(position,safeRadius,shift);
                for (unsigned i=0;i<stage->blockCount;++i)
                    shift=TileField::recoverBlock(stage->blocks[i],position,safeRadius,shift);
                position=position+shift;
                if (dot(shift,shift)<1.0e-8f) break;
            }
            overlap=active->recoverCircle(position,radius);
        }
        const float distance=length(overlap);
        recovering=distance>0.0001f;
        return recovering?overlap*(1.0f/distance):Vec2{};
    }

    int sweep(Vec2 position,Vec2 motion,float radius,WallContact& contact) const {
        if (!stage) return -1;
        if (const TileField* field=stage->tiles(state)) {
            const WallContact layer=field->sweep(position,motion,radius);
            if (layer.time<contact.time) contact=layer;
        }
        int block=-1;
        for (unsigned i=0;i<stage->blockCount;++i) {
            const float previous=contact.time;
            TileField::sweepBlock(stage->blocks[i],position,motion,radius,contact);
            if (contact.time<previous) block=static_cast<int>(i);
        }
        return block;
    }

private:
    const SwitchStage* stage=nullptr;
    bool recovering=false;
};
