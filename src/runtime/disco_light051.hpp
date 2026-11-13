#pragma once

struct DiscoLight051 {
    Vec2 lamps[4],center;
    float cosine,sine,diameter;

    DiscoLight051(const GameplayView& view,unsigned tick)
        : center(view.position({})),diameter(256*view.worldScale()) {
        const unsigned cycle=tick%240;
        const float half=std::min(cycle,240-cycle)/120.0f;
        const float angle=6.283185307f*half*half*(3-2*half);
        cosine=std::cos(angle);
        sine=std::sin(angle);
        constexpr Vec2 positions[]={{0,-256},{-320,0},{0,256},{320,0}};
        for (unsigned i=0;i<std::size(positions);++i) {
            const auto p=positions[i];
            lamps[i]=view.position({p.x*cosine-p.y*sine,p.x*sine+p.y*cosine});
        }
    }

    uint32_t tint(Vec2 p,bool ambient) const {
        const float purple=rotating(p,lamps[0])+rotating(p,lamps[2]);
        const float red=rotating(p,lamps[1])+rotating(p,lamps[3]);
        const auto byte=[](float value) { return static_cast<unsigned>(std::clamp(value*255,0.0f,255.0f)); };
        const auto delta=p-center;
        const float white=ambient?kernel(delta.x/(2*diameter)+0.5f,delta.y/(2*diameter)+0.5f):0;
        const float shade=ambient?55/255.0f:0, blue=ambient?105/255.0f:0;
        return GU_RGBA(byte(shade+white+red+0.5485f*purple),
                       byte(shade+white+0.40094f*purple),byte(blue+white+purple),255);
    }

private:
    static float kernel(float u,float v) {
        if (u<0 || u>=1 || v<0 || v>=1) return 0;
        constexpr unsigned Last=CosmicLight612KernelSide-1;
        const float x=u*Last,y=v*Last;
        const unsigned i=std::min(Last-1,static_cast<unsigned>(x)),j=std::min(Last-1,static_cast<unsigned>(y));
        const auto* top=CosmicLight612Kernel+j*CosmicLight612KernelSide+i;
        const auto* bottom=top+CosmicLight612KernelSide;
        const float dx=x-i,dy=y-j;
        return ((top[0]*(1-dx)+top[1]*dx)*(1-dy)+(bottom[0]*(1-dx)+bottom[1]*dx)*dy)/255.0f;
    }

    float rotating(Vec2 p,Vec2 at) const {
        const auto delta=p-at;
        return kernel((delta.x*cosine+delta.y*sine)/diameter+0.5f,
                      (-delta.x*sine+delta.y*cosine)/diameter+0.5f);
    }
};
