#pragma once

#include <array>

struct WallContact {
    float time = 2.0f;
    Vec2 normal{};
};

struct SolidBlock { Vec2 center,halfSize; };
struct CircleCollider { Vec2 center; float radius; };

inline void sweepCircle(const CircleCollider& other,Vec2 position,Vec2 motion,float radius,WallContact& contact) {
    if (other.radius<=0) return;
    const Vec2 offset=position-other.center;
    const float a=dot(motion,motion), b=dot(offset,motion);
    if (a==0 || b>=0) return;
    const float sum=radius+other.radius;
    const float discriminant=b*b-a*(dot(offset,offset)-sum*sum);
    if (discriminant<=0) return;
    const float time=std::max(0.0f,(-b-std::sqrt(discriminant))/a);
    if (time>1 || time>=contact.time) return;
    const Vec2 radial=offset+motion*time;
    const float distance=length(radial);
    if (distance>0.001f) contact={time,radial*(1.0f/distance)};
}

class TileField {
public:
    static constexpr int Origin = -17;
    static constexpr int Size = 34;
    static constexpr int WideOrigin = -19, WideSize = 38;
    static constexpr int MaxSize = 88;
    static constexpr int MaxShapes = 32;
    static constexpr unsigned GridCapacity=MaxSize*MaxSize;

    bool read(std::FILE* file) {
        uint32_t magic = 0, count = 0;
        loaded=false;
        worldOffset={};
        if (std::fread(&magic, 4, 1, file) != 1 ||
            (magic != StandardMagic && magic != WideMagic && magic != VariableMagic && magic != RectMagic) ||
            std::fread(&count, 4, 1, file) != 1 || !count || count > MaxShapes ||
            std::fread(&ghostSpawn, sizeof ghostSpawn, 1, file) != 1 ||
            std::fread(&onTime, sizeof onTime, 1, file) != 1 ||
            !std::isfinite(ghostSpawn.x) || !std::isfinite(ghostSpawn.y) ||
            !std::isfinite(onTime) || onTime < 0 || onTime > 600) return false;
        originX = originY = magic == WideMagic ? WideOrigin : Origin;
        width = height = magic == WideMagic ? WideSize : Size;
        if (magic == VariableMagic) {
            int16_t storedOrigin = 0;
            uint16_t storedSize = 0;
            if (std::fread(&storedOrigin, sizeof storedOrigin, 1, file) != 1 ||
                std::fread(&storedSize, sizeof storedSize, 1, file) != 1 ||
                storedSize == 0 || storedSize > MaxSize) return false;
            originX = originY = storedOrigin;
            width = height = storedSize;
        } else if (magic == RectMagic) {
            int16_t origin[2];
            uint16_t size[2];
            if (std::fread(origin,sizeof origin,1,file)!=1 || std::fread(size,sizeof size,1,file)!=1 ||
                !size[0] || !size[1] || size[0]>256 || size[1]>256 ||
                static_cast<unsigned>(size[0])*size[1]>GridCapacity) return false;
            originX=origin[0];
            originY=origin[1];
            width=size[0];
            height=size[1];
        }
        for (unsigned index = 0; index < count; ++index) {
            uint8_t pointCount;
            int8_t coordinates[8];
            if (std::fread(&pointCount, 1, 1, file) != 1 || pointCount < 3 || pointCount > 4 ||
                std::fread(coordinates, 1, sizeof coordinates, file) != sizeof coordinates) return false;
            Shape& shape = shapes[index];
            shape.count = pointCount;
            for (int i = 0; i < pointCount; ++i) {
                const Vec2 point{float(coordinates[2*i]), float(coordinates[2*i+1])};
                if (std::abs(point.x) > 8 || std::abs(point.y) > 8) return false;
                shape.edges[i].point = point;
            }
            for (int i = 0; i < pointCount; ++i) {
                Edge& edge = shape.edges[i];
                const Vec2 direction = shape.edges[(i+1)%pointCount].point - edge.point;
                edge.length = length(direction);
                if (edge.length == 0) return false;
                edge.tangent = direction * (1.0f / edge.length);
                edge.normal = {edge.tangent.y, -edge.tangent.x};
            }
        }
        const unsigned cells=width*height;
        loaded = std::fread(grid.data(), 1, cells, file) == cells &&
               std::all_of(grid.begin(), grid.begin()+cells, [count](uint8_t cell) { return cell <= count; }) &&
                 std::fgetc(file) == EOF && !std::ferror(file);
        return loaded;
    }

    void setOffset(Vec2 offset) { worldOffset=offset; }

    WallContact sweep(Vec2 position, Vec2 motion, float radius) const {
        position=position-worldOffset;
        WallContact result;
        const float lengthSquared = dot(motion, motion);
        if (lengthSquared == 0) return result;
        const Vec2 end = position + motion;
        const int left = std::max(originX, tile(std::min(position.x, end.x) - radius));
        const int right = std::min(originX+width-1, tile(std::max(position.x, end.x) + radius));
        const int top = std::max(originY, tile(std::min(position.y, end.y) - radius));
        const int bottom = std::min(originY+height-1, tile(std::max(position.y, end.y) + radius));
        const float radiusSquared = radius * radius;
        for (int y = top; y <= bottom; ++y) {
            for (int x = left; x <= right; ++x) {
                const unsigned index = grid[(y-originY)*width + x-originX];
                if (!index) continue;
                const Shape& shape = shapes[index-1];
                const Vec2 local = position - Vec2{float(16*x+8), float(16*y+8)};
                sweepShape(shape,local,motion,radius,radiusSquared,lengthSquared,result);
            }
        }
        return result;
    }

    static void sweepBlock(const SolidBlock& block,Vec2 position,Vec2 motion,float radius,WallContact& contact) {
        const auto half=block.halfSize;
        const Vec2 local=position-block.center, end=local+motion;
        // Sweep permits a small negative contact time at a face already touching the ball.
        const float xRange=half.x+radius+0.0001f*std::abs(motion.x);
        const float yRange=half.y+radius+0.0001f*std::abs(motion.y);
        if (std::min(local.x,end.x)>xRange || std::max(local.x,end.x)<-xRange ||
            std::min(local.y,end.y)>yRange || std::max(local.y,end.y)<-yRange) return;
        sweepShape(blockShape(half),local,motion,radius,radius*radius,dot(motion,motion),contact);
    }

    Vec2 recoverCircle(Vec2 position,float radius,Vec2 correction={}) const {
        position=position-worldOffset;
        const int left=std::max(originX,tile(position.x-radius));
        const int right=std::min(originX+width-1,tile(position.x+radius));
        const int top=std::max(originY,tile(position.y-radius));
        const int bottom=std::min(originY+height-1,tile(position.y+radius));
        for (int y=top;y<=bottom;++y) {
            for (int x=left;x<=right;++x) {
                const unsigned index=grid[(y-originY)*width+x-originX];
                if (index) correction=recoverShape(shapes[index-1],
                    position-Vec2{float(16*x+8),float(16*y+8)},radius,correction);
            }
        }
        return correction;
    }

    static Vec2 recoverBlock(const SolidBlock& block,Vec2 position,float radius,Vec2 correction) {
        const Vec2 local=position-block.center, half=block.halfSize;
        if (std::abs(local.x)>=half.x+radius || std::abs(local.y)>=half.y+radius) return correction;
        return recoverShape(blockShape(half),local,radius,correction);
    }

    bool active() const { return loaded; }

    Vec2 ghostSpawn{224,-224};
    float onTime = 10.0f;

private:
    static constexpr uint32_t StandardMagic = 0x54474355; // UCGT
    static constexpr uint32_t WideMagic = 0x58474355;     // UCGX
    static constexpr uint32_t VariableMagic = 0x56474355; // UCGV

    static constexpr uint32_t RectMagic=0x57474355; // UCGW

    struct Edge { Vec2 point, tangent, normal; float length; };
    struct Shape { unsigned count; Edge edges[4]; };
    bool loaded = false;
    Vec2 worldOffset{};
    int originX=Origin,originY=Origin,width=Size,height=Size;
    std::array<Shape, MaxShapes> shapes{};
    std::array<uint8_t, GridCapacity> grid{};

    static int tile(float coordinate) { return static_cast<int>(std::floor(coordinate / 16.0f)); }

    static Shape blockShape(Vec2 half) {
        return {4,{
            {{-half.x,-half.y},{1,0},{0,-1},2*half.x},
            {{ half.x,-half.y},{0,1},{1,0},2*half.y},
            {{ half.x, half.y},{-1,0},{0,1},2*half.x},
            {{-half.x, half.y},{0,-1},{-1,0},2*half.y}
        }};
    }

    static Vec2 recoverShape(const Shape& shape,Vec2 local,float radius,Vec2 correction) {
        float signedDistance=-1.0e20f, nearestSquared=1.0e20f;
        Vec2 face{}, nearest{};
        for (unsigned i=0;i<shape.count;++i) {
            const Edge& edge=shape.edges[i];
            const Vec2 offset=local-edge.point;
            const float plane=dot(offset,edge.normal);
            if (plane>signedDistance) {
                signedDistance=plane;
                face=edge.normal;
            }
            const Vec2 radial=offset-edge.tangent*std::clamp(dot(offset,edge.tangent),0.0f,edge.length);
            const float squared=dot(radial,radial);
            if (squared<nearestSquared) {
                nearestSquared=squared;
                nearest=radial;
            }
        }
        if (signedDistance>0) {
            if (nearestSquared>=radius*radius) return correction;
            signedDistance=std::sqrt(nearestSquared);
            if (signedDistance>0.0001f) face=nearest*(1.0f/signedDistance);
        }
        // Godot uses a 0.08px safe margin with 5% slop and corrects 40% per contact.
        constexpr float slop=0.004f;
        const float depth=radius-signedDistance-dot(face,correction);
        return depth>slop?correction+face*((depth-slop)*0.4f):correction;
    }

    static void sweepShape(const Shape& shape,Vec2 local,Vec2 motion,float radius,
                           float radiusSquared,float lengthSquared,WallContact& result) {
        for (unsigned i = 0; i < shape.count; ++i) {
            const Edge& edge = shape.edges[i];
            const Vec2 offset = local - edge.point;
            const float towards = dot(motion, edge.normal);
            if (towards < 0) {
                const float time = (radius - dot(offset, edge.normal)) / towards;
                if (validTime(time, result)) {
                    const float along = dot(offset + motion*time, edge.tangent);
                    if (along >= 0 && along <= edge.length) result = {std::max(0.0f, time), edge.normal};
                }
            }
            // The straight offset edges meet through circular arcs around the original vertices.
            const float approach = dot(offset, motion);
            if (approach >= 0) continue;
            const float discriminant = approach*approach - lengthSquared*(dot(offset, offset)-radiusSquared);
            if (discriminant <= 0) continue;
            const float time = (-approach - std::sqrt(discriminant)) / lengthSquared;
            if (!validTime(time, result)) continue;
            const Vec2 radial = (offset + motion*time) * (1.0f / radius);
            const Edge& previous = shape.edges[(i+shape.count-1)%shape.count];
            if (dot(radial, edge.tangent) <= 0 && dot(radial, previous.tangent) >= 0) {
                result = {std::max(0.0f, time), radial};
            }
        }
    }

    static bool validTime(float time, const WallContact& result) {
        return time >= -0.0001f && time <= 1.0f && time < result.time;
    }
};
