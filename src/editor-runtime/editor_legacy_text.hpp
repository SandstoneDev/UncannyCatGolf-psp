#pragma once
#include "editor_legacy_fonts_data.hpp"
#include "editor_legacy_lookup066.hpp"
#include "editor_font_masks.hpp"
#include "editor_text_flow068.hpp"
#include <vector>
#include <cstring>
namespace EditorLegacyText {
using Glyph=EditorRender::LegacyFonts::Glyph;
inline bool&outline=EditorTextFlow068::outline;
using Kern=EditorRender::LegacyFonts::Kern;
inline const Glyph*glyph(unsigned font,unsigned code){return EditorLegacyLookup066::glyph(font,code);}
inline const Kern*pair(unsigned font,unsigned left,unsigned right){const auto&pairs=EditorRender::LegacyFonts::Kerning;size_t l=0,r=std::size(pairs);while(l<r){size_t m=(l+r)/2;const auto&k=pairs[m];if(std::make_tuple(k.font,k.left,k.right)<std::make_tuple(font,left,right))l=m+1;else r=m;}if(l<std::size(pairs)){const auto&k=pairs[l];if(k.font==font&&k.left==left&&k.right==right)return &k;}return nullptr;}
inline std::vector<unsigned>characters(unsigned font,const char*text){std::vector<unsigned>out;if(!text)return out;while(*text){unsigned code=0;size_t bytes=0;for(const auto&liga:EditorRender::LegacyFonts::Ligatures){size_t n=std::strlen(liga.text);if(liga.font==font&&n>bytes&&std::strncmp(text,liga.text,n)==0){code=liga.code;bytes=n;}}if(bytes){out.push_back(code);text+=bytes;}else out.push_back(EditorArt::codepoint(text));}return out;}
template<class Visitor>inline float shape(unsigned font,const std::vector<unsigned>&text,unsigned first,unsigned last,const Visitor&visit){float pen=0,carry=0;unsigned face=UINT_MAX;for(unsigned i=first;i<last;++i){unsigned cp=text[i];const auto*g=glyph(font,cp);if(!g)g=glyph(font,'?');if(!g)continue;if(g->face!=face||cp==' '||cp=='\t')carry=0;const Kern*before=nullptr,*after=nullptr;if(i>first){const auto*p=glyph(font,text[i-1]);if(p&&p->face==g->face)before=pair(font,text[i-1],cp);}if(i+1<last){const auto*n=glyph(font,text[i+1]);if(n&&n->face==g->face)after=pair(font,cp,text[i+1]);}float advance=g->advance+(before?before->db:0)+(after?after->da:0),offset=(before?before->ob:0)+(after?after->oa:0);if(g->snap){offset=std::round(carry+offset);const float full=carry+advance;advance=std::round(full);carry=full-advance;}else carry=0;visit(*g,pen+offset);pen+=advance;face=g->face;}return pen;}
inline float measure(unsigned font,const char*value){if(font>=std::size(EditorRender::LegacyFonts::Fonts))return 0;auto text=characters(font,value);float result=0;unsigned first=0;for(unsigned at=0;at<=text.size();++at)if(at==text.size()||text[at]=='\n'){result=std::max(result,shape(font,text,first,at,[](const Glyph&,float){}));first=at+1;}return std::ceil(result);}
inline void drawScaled(unsigned font,const char*value,float x,float baseline,uint32_t tint,float maxWidth,float scale){
 if(font>=std::size(EditorRender::LegacyFonts::Fonts)||!value||!*value)return;
 const auto text=characters(font,value);const auto&f=EditorRender::LegacyFonts::Fonts[font];const auto saved=EditorRender::clip;const auto ui=EditorArt::uiScissor;
 EditorRender::clip={float(std::max(ViewX,ui[0])),float(std::max(0,ui[1])),float(std::min(ViewX+408,ui[2])),float(std::min(272,ui[3]))};if(std::isfinite(maxWidth)&&maxWidth<1e20f)EditorRender::clip.right=std::min(EditorRender::clip.right,ViewX+(x+std::max(0.f,maxWidth))*EditorArt::Scale);
 // A finite single-line window exposes the complete value over time.
 // Keep phase metrics in source font units; only the rendered origin moves.
 if(std::isfinite(maxWidth)&&maxWidth<1e20f&&!std::strchr(value,'\n')){
  EditorRender::clip.x=std::max(EditorRender::clip.x,ViewX+x*EditorArt::Scale);
  x-=EditorTextFlow068::offset(measure(font,value)*scale,maxWidth);
 }
 unsigned first=0;float y=baseline,phaseBaseline=baseline;
 for(unsigned at=0;at<=text.size();++at)if(at==text.size()||text[at]=='\n'){
  shape(font,text,first,at,[&](const Glyph&g,float pen){if(!g.w||!g.h)return;const float glyphX=scale==1.f?x+pen+g.ox:x+(pen+g.ox)*scale;const float left=ViewX+glyphX*EditorArt::Scale,top=(y+g.oy*scale)*EditorArt::Scale,w=g.w*EditorArt::Scale*scale,h=g.h*EditorArt::Scale*scale;const std::array<EditorRender::Vertex,4>q={EditorRender::Vertex{left,top,float(g.u),float(g.v)},EditorRender::Vertex{left+w,top,float(g.u+g.w),float(g.v)},EditorRender::Vertex{left+w,top+h,float(g.u+g.w),float(g.v+g.h)},EditorRender::Vertex{left,top+h,float(g.u),float(g.v+g.h)}};// Stroke the original fill mask in screen pixels; no substitute face or plate.
   if(outline&&(tint&0x00ffffffu)!=0){for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(std::abs(dx)+std::abs(dy)==1){auto edge=q;for(auto&v:edge){v.x+=dx;v.y+=dy;}EditorRender::FontMasks::draw(EditorRender::FontMasks::Bank(f.bank),g.page,EditorRender::FontMasks::Fill,edge,tint&0xff000000u,nullptr,nullptr,pen,phaseBaseline);}}
   EditorRender::FontMasks::draw(EditorRender::FontMasks::Bank(f.bank),g.page,EditorRender::FontMasks::Fill,q,tint,nullptr,nullptr,pen,phaseBaseline);});first=at+1;y+=(f.height+3)*scale;phaseBaseline+=f.height+3;
 }
 EditorRender::clip=saved;EditorArt::scissor(ui[0],ui[1],ui[2],ui[3]);
}
inline void draw(unsigned font,const char*value,float x,float baseline,uint32_t tint=White,float maxWidth=1e30f){drawScaled(font,value,x,baseline,tint,maxWidth,1.f);}
inline void install(){EditorArt::textHook=draw;EditorArt::textWidthHook=measure;EditorArt::scaledTextHook=drawScaled;}
inline void uninstall(){if(EditorArt::textHook==draw)EditorArt::textHook=nullptr;if(EditorArt::textWidthHook==measure)EditorArt::textWidthHook=nullptr;if(EditorArt::scaledTextHook==drawScaled)EditorArt::scaledTextHook=nullptr;}
} // namespace EditorLegacyText
