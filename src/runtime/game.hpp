#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>

struct Vec2 {
    float x, y;
    Vec2 operator+(Vec2 v) const { return {x + v.x, y + v.y}; }
    Vec2 operator-(Vec2 v) const { return {x - v.x, y - v.y}; }
    Vec2 operator*(float k) const { return {x * k, y * k}; }
};

inline float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline float length(Vec2 v) { return std::sqrt(dot(v, v)); }

#include "tile_collision.hpp"
#include "switch_blocks.hpp"

struct Course {
    char magic[8];
    uint32_t cells, par;
    float left, top, right, bottom;
    Vec2 spawn, goal;
    float playerRadius, goalRadius;
    float maxDrag, minDrag, dragMultiplier, frictionCoeff, frictionPow, stopSpeed;
    float fieldLeft, fieldTop, fieldSize, catWidth, catHeight;
};
static_assert(sizeof(Course) == 100);

constexpr long CourseBytes = sizeof(Course);

inline bool readCourse(std::FILE* file, Course& course) {
    if (std::fseek(file, 0, SEEK_END) != 0 || std::ftell(file) < CourseBytes ||
        std::fseek(file, 0, SEEK_SET) != 0 || std::fread(&course, sizeof(course), 1, file) != 1) {
        return false;
    }
    const float fieldTiles = course.fieldSize / 16.0f;
    // Legacy geometry uses a 34-tile field even when UCGX has 38 collision tiles per side.
    const float cellSideLimit = fieldTiles <= TileField::Size ? TileField::WideSize : fieldTiles;
    const float fields[] = {course.left, course.top, course.right, course.bottom,
                            course.spawn.x, course.spawn.y, course.goal.x, course.goal.y,
                            course.playerRadius, course.goalRadius, course.maxDrag, course.minDrag,
                            course.dragMultiplier, course.frictionCoeff, course.frictionPow, course.stopSpeed,
                            course.fieldLeft, course.fieldTop, course.fieldSize, course.catWidth, course.catHeight};
    return std::all_of(std::begin(fields), std::end(fields), [](float f) { return std::isfinite(f); }) &&
           std::memcmp(course.magic, "UCG001\0\1", 8) == 0 && course.cells > 0 &&
           course.cells <= cellSideLimit * cellSideLimit && course.par <= 99 &&
           course.left < course.right && course.top < course.bottom &&
           course.playerRadius > 0 && course.goalRadius > 0 &&
           course.minDrag > 0 && course.maxDrag >= course.minDrag && course.dragMultiplier > 0 &&
           course.frictionCoeff > 0 && course.frictionPow > 0 && course.stopSpeed > 0 &&
           fieldTiles >= 32 && fieldTiles <= TileField::MaxSize &&
           fieldTiles == std::floor(fieldTiles) && course.catWidth > 0 && course.catHeight > 0;
}

inline bool loadCourse(const char* path, Course& course) {
    std::FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    const bool ok = readCourse(file, course);
    std::fclose(file);
    return ok;
}

struct BlockMask {
    uint64_t low=0,high=0;

    constexpr BlockMask(uint64_t first=0) : low(first) {}
    explicit constexpr operator bool() const { return low || high; }

    constexpr bool has(unsigned index) const {
        if (index>=128) return false;
        return ((index<64?low:high) & (uint64_t{1}<<(index%64)))!=0;
    }

    void set(unsigned index) {
        if (index<128) (index<64?low:high)|=uint64_t{1}<<(index%64);
    }

    friend constexpr bool operator==(BlockMask a,BlockMask b) { return a.low==b.low && a.high==b.high; }
    friend constexpr bool operator!=(BlockMask a,BlockMask b) { return !(a==b); }
};

class Game {
public:
    explicit Game(const Course& scene, const TileField* tiles = nullptr) : course(scene), tiles(tiles) { restart(); }

    Vec2 position{}, velocity{};
    unsigned strokes = 0;
    unsigned bounces = 0;
    bool won = false, pushed = false, passiveBounce = false;
    bool goalEnabled = true;

    void setGoal(Vec2 next) { goal=next; movingGoal=true; }
    int passiveBounceIndex=-1;
    bool fixedTileBounce=false;

    bool moving() const { return dot(velocity, velocity) > 0; }

    bool hit(Vec2 drag,bool allowDrift=false) {
        if (won || (moving() && !allowDrift)) return false;
        const float power = length(drag);
        if (power < course.minDrag) return false;
        velocity = drag * (course.dragMultiplier * std::min(course.maxDrag / power, 1.0f));
        ++strokes;
        return true;
    }

    void restart() {
        position = course.spawn;
        movingGoal=false;
        velocity = {};
        strokes = 0;
        bounces = 0;
        won = pushed = passiveBounce = false;
        passiveBounceIndex=-1;
    }

    int step(const SolidBlock* blocks=nullptr,unsigned count=0,BlockMask destroyed={},
             const CircleCollider* enemies=nullptr,unsigned enemyCount=0,
             const SolidBlock* doors=nullptr,unsigned doorCount=0,uint32_t opened=0,
             const SolidBlock* movingBlocks=nullptr,unsigned movingCount=0,
             const CircleCollider* extraEnemies=nullptr,unsigned extraEnemyCount=0,
             SwitchRun* switches=nullptr,const TileField* auxiliaryWalls=nullptr,
             const CircleCollider* passiveBody=nullptr,unsigned passiveBodyCount=1,
             Vec2 additionalVelocity={},bool drifting=false,
             WallContact (*solidObstacle)(Vec2,Vec2,float)=nullptr) {
        if (switches) switches->hitBlock=-1;
        pushed=passiveBounce=fixedTileBounce=false;
        passiveBounceIndex=-1;
        if (won) return -1;
        const Vec2 beforeRecovery=position;
        const Vec2 overlapNormal=switches?switches->recover(position,course.playerRadius,tiles,auxiliaryWalls):Vec2{};
        const float windSquared=dot(additionalVelocity,additionalVelocity);
        const bool windy=windSquared>0.0f;
        if (windy) velocity=velocity+additionalVelocity;
        const float speed = moving() ? length(velocity) : 0.0f;
        if (!windy && speed<=course.stopSpeed && (!drifting || speed==0.0f)) {
            velocity = {};
            const bool movedByEnemy=recoverFromEnemies(enemies,enemyCount);
            const bool movedByExtraEnemy=recoverFromEnemies(extraEnemies,extraEnemyCount);
            const bool movedByPassiveBody=passiveBody &&
                recoverFromEnemies(passiveBody,passiveBodyCount,auxiliaryWalls,switches);
            pushed=recoverFromMovingBlocks(movingBlocks,movingCount) ||
                movedByEnemy || movedByExtraEnemy || movedByPassiveBody;
            if (movingGoal || pushed || position.x!=beforeRecovery.x || position.y!=beforeRecovery.y) checkGoal();
            return -1;
        }

        // Wind drives idle motion without friction. A moving player skips drag
        // only when their total speed equals the speed added by this tick's wind.
        if (speed>0.0f && !(windy && (drifting || speed==std::sqrt(windSquared)))) {
            const float loss = course.maxDrag * std::pow(course.frictionCoeff / speed, course.frictionPow);
            velocity = velocity * (1.0f - loss);
        }
        const Vec2 motion = velocity * (1.0f / 60.0f);
        const unsigned previousBounces=bounces;
        int hit=-1;
        if (tiles && tiles->active()) {
            WallContact contact = dot(overlapNormal,overlapNormal)>0 ?
                WallContact{0,overlapNormal}:tiles->sweep(position,motion,course.playerRadius);
            const auto fixedContact=contact;
            if (auxiliaryWalls) {
                const auto wall=auxiliaryWalls->sweep(position,motion,course.playerRadius);
                if (wall.time<contact.time) contact=wall;
            }
            if (solidObstacle) {
                const auto wall=solidObstacle(position,motion,course.playerRadius);
                if (wall.time<contact.time) contact=wall;
            }
            for (unsigned i=0;i<count;++i) {
                if (destroyed.has(i)) continue;
                const float previous=contact.time;
                TileField::sweepBlock(blocks[i],position,motion,course.playerRadius,contact);
                if (contact.time<previous) hit=static_cast<int>(i);
            }
            for (unsigned i=0;i<doorCount;++i) {
                if (opened&(1u<<i)) continue;
                const float previous=contact.time;
                TileField::sweepBlock(doors[i],position,motion,course.playerRadius,contact);
                if (contact.time<previous) hit=static_cast<int>(count+i);
            }
            for (unsigned i=0;i<enemyCount;++i) {
                const float previous=contact.time;
                sweepCircle(enemies[i],position,motion,course.playerRadius,contact);
                if (contact.time<previous) hit=static_cast<int>(count+doorCount+i);
            }
            for (unsigned i=0;i<extraEnemyCount;++i) {
                const float previous=contact.time;
                sweepCircle(extraEnemies[i],position,motion,course.playerRadius,contact);
                if (contact.time<previous) hit=static_cast<int>(count+doorCount+enemyCount+i);
            }
            for (unsigned i=0;i<movingCount;++i) {
                if (movingBlocks[i].halfSize.x<=0) continue;
                const float previous=contact.time;
                TileField::sweepBlock(movingBlocks[i],position,motion,course.playerRadius,contact);
                if (contact.time<previous) hit=static_cast<int>(count+doorCount+enemyCount+extraEnemyCount+i);
            }
            if (passiveBody) {
                for (unsigned i=0;i<passiveBodyCount;++i) {
                    const float previous=contact.time;
                    sweepCircle(passiveBody[i],position,motion,course.playerRadius,contact);
                    if (contact.time<previous) passiveBounceIndex=static_cast<int>(i);
                }
                passiveBounce=passiveBounceIndex>=0;
                if (passiveBounce) hit=-1;
            }
            int switchHit=-1;
            if (switches) {
                const float previous=contact.time;
                switchHit=switches->sweep(position,motion,course.playerRadius,contact);
                if (contact.time<previous) {
                    hit=-1;
                    passiveBounce=false;
                    passiveBounceIndex=-1;
                }
            }
            position = position + motion * std::min(1.0f, contact.time);
            if (contact.time <= 1.0f) {
                velocity = velocity - contact.normal * (2.0f * dot(velocity, contact.normal));
                ++bounces;
                fixedTileBounce=contact.time==fixedContact.time && dot(overlapNormal,overlapNormal)==0;
                if (switchHit>=0) switches->strike(static_cast<unsigned>(switchHit));
                if (!drifting && hit>=static_cast<int>(count+doorCount))
                    position=position+velocity*(1.0f/60.0f);
            }
        } else {
            moveInRectangle(motion);
        }

        if (bounces!=previousBounces) {
            if (drifting) pushed=true;
            const float speedSquared=dot(velocity,velocity);
            const float minimumSquared=100.0f*windSquared;
            if (speedSquared>0.0f && speedSquared<minimumSquared)
                velocity=velocity*std::sqrt(minimumSquared/speedSquared);
        }
        checkGoal();
        return drifting?-1:hit;
    }

private:
    const Course& course;
    const TileField* tiles;
    Vec2 goal{};
    bool movingGoal=false;

    bool recoverFromEnemies(const CircleCollider* enemies,unsigned count,
                            const TileField* auxiliaryWalls=nullptr,const SwitchRun* switches=nullptr) {
        // Godot recovers overlapping bodies even when move_and_collide receives zero motion.
        bool moved=false;
        for (unsigned i=0;i<count;++i) {
            const auto& enemy=enemies[i];
            if (enemy.radius<=0) continue;
            const float clearance=course.playerRadius+enemy.radius;
            const Vec2 offset=position-enemy.center;
            const float distanceSquared=dot(offset,offset);
            if (distanceSquared>=clearance*clearance) continue;
            const float distance=std::sqrt(distanceSquared);
            const Vec2 normal=distance>0.001f?offset*(1.0f/distance):Vec2{1,0};
            const Vec2 correction=normal*(clearance-distance);
            WallContact contact=tiles && tiles->active() ?
                tiles->sweep(position,correction,course.playerRadius) : WallContact{};
            if (auxiliaryWalls) {
                const auto wall=auxiliaryWalls->sweep(position,correction,course.playerRadius);
                if (wall.time<contact.time) contact=wall;
            }
            if (switches) switches->sweep(position,correction,course.playerRadius,contact);
            const float limit=std::min(1.0f,contact.time);
            position=position+correction*limit;
            moved|=limit>0;
        }
        return moved;
    }

    bool recoverFromMovingBlocks(const SolidBlock* blocks,unsigned count) {
        bool moved=false;
        for (unsigned i=0;i<count;++i) {
            const auto& block=blocks[i];
            if (block.halfSize.x<=0) continue;
            const Vec2 local=position-block.center, half=block.halfSize;
            const Vec2 closest{std::clamp(local.x,-half.x,half.x),std::clamp(local.y,-half.y,half.y)};
            const Vec2 offset=local-closest;
            const float distance=length(offset);
            if (distance>=course.playerRadius) continue;
            Vec2 correction;
            if (distance>0.001f) {
                correction=offset*((course.playerRadius-distance)/distance);
            } else {
                const float x=half.x+course.playerRadius-std::abs(local.x);
                const float y=half.y+course.playerRadius-std::abs(local.y);
                correction=x<y?Vec2{std::copysign(x,local.x),0}:Vec2{0,std::copysign(y,local.y)};
            }
            const float limit=tiles && tiles->active() ?
                std::min(1.0f,tiles->sweep(position,correction,course.playerRadius).time) : 1.0f;
            position=position+correction*limit;
            moved|=limit>0;
        }
        return moved;
    }

    void checkGoal() {
        if (!goalEnabled) return;
        const Vec2 target=movingGoal?goal:course.goal;
        const Vec2 offset = target - position;
        const float radius = course.playerRadius + course.goalRadius;
        if (dot(offset, offset) <= radius * radius) {
            won = true;
            position = target;
            velocity = {};
        }
    }

    void moveInRectangle(Vec2 motion) {
        const float tx = wallTime(position.x, motion.x, course.left + course.playerRadius,
                                  course.right - course.playerRadius);
        const float ty = wallTime(position.y, motion.y, course.top + course.playerRadius,
                                  course.bottom - course.playerRadius);
        // move_and_collide stops at the contact. The reflected speed takes effect on the next tick.
        const float t = std::min({1.0f, tx, ty});
        const Vec2 advance = motion * t;
        position = position + advance;

        constexpr float contactEpsilon = 0.0001f;
        if (tx <= 1.0f && std::abs(tx - t) < contactEpsilon) {
            position.x = motion.x < 0 ? course.left + course.playerRadius : course.right - course.playerRadius;
            velocity.x = -velocity.x;
            ++bounces;
        }
        if (ty <= 1.0f && std::abs(ty - t) < contactEpsilon) {
            position.y = motion.y < 0 ? course.top + course.playerRadius : course.bottom - course.playerRadius;
            velocity.y = -velocity.y;
            ++bounces;
        }

    }

    static float wallTime(float position, float movement, float lo, float hi) {
        if (movement < 0 && position + movement <= lo) return std::max(0.0f, (lo - position) / movement);
        if (movement > 0 && position + movement >= hi) return std::max(0.0f, (hi - position) / movement);
        return 2.0f;
    }
};
