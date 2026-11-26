#pragma once
#include <png.h>
extern "C" {
#include <jpeglib.h>
}
#include <setjmp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <algorithm>
namespace EditorImages {
struct Info {unsigned width=0,height=0;bool jpeg=false;};
struct Window {unsigned x,y,width,height;};
constexpr unsigned MaxDimension=16384,MaxWindow=1024;
constexpr uint64_t MaxAlphaPixels=16u*1024u*1024u;
struct Samples {
 unsigned width=0,height=0,next=0,xs[MaxWindow]{},ys[MaxWindow]{}; bool fixedCoordinates=false;
 void configure(unsigned w,unsigned h){if(fixedCoordinates){next=0;return;}for(unsigned x=0;x<width;++x)xs[x]=unsigned(uint64_t(2*x+1)*w/(2*width));for(unsigned y=0;y<height;++y)ys[y]=unsigned(uint64_t(2*y+1)*h/(2*height));next=0;}
 void row(const unsigned char* pixels,unsigned y,uint32_t*out,unsigned stride,unsigned channels,int pass=-1){
  constexpr unsigned starts[]={0,4,0,2,0,1,0},steps[]={8,8,4,4,2,2,1};
  while(next<height&&ys[next]<y)++next;
  while(next<height&&ys[next]==y){for(unsigned x=0;x<width;++x){const unsigned sx=xs[x];if(pass>=0&&(sx<starts[pass]||(sx-starts[pass])%steps[pass]))continue;const auto*c=pixels+sx*channels;out[next*stride+x]=uint32_t(c[0])|(uint32_t(c[1])<<8)|(uint32_t(c[2])<<16)|(channels==4?uint32_t(c[3])<<24:0xff000000u);}++next;}
 }
};
struct JpegError {jpeg_error_mgr base;jmp_buf jump;};
inline void jpegFailure(j_common_ptr c){longjmp(reinterpret_cast<JpegError*>(c->err)->jump,1);}
inline void jpegStrictMessage(j_common_ptr c,int level){if(level<0)jpegFailure(c);}
inline bool jpegRead(FILE*f,Info&info,const Window*window,uint32_t*out,unsigned stride,bool strict=false,Samples*samples=nullptr){
 jpeg_decompress_struct decoder{};JpegError error{};
 decoder.err=jpeg_std_error(&error.base);error.base.error_exit=jpegFailure;
 if(strict)error.base.emit_message=jpegStrictMessage;
 unsigned char* volatile row=nullptr;
 if(setjmp(error.jump)){std::free(const_cast<unsigned char*>(row));jpeg_destroy_decompress(&decoder);return false;}
 jpeg_create_decompress(&decoder);jpeg_stdio_src(&decoder,f);
 if(jpeg_read_header(&decoder,TRUE)!=JPEG_HEADER_OK){jpeg_destroy_decompress(&decoder);return false;}
 if(!decoder.image_width||!decoder.image_height||decoder.image_width>MaxDimension||decoder.image_height>MaxDimension){jpeg_destroy_decompress(&decoder);return false;}
 info={decoder.image_width,decoder.image_height,true};
 if(samples)samples->configure(info.width,info.height);
 if(strict&&uint64_t(info.width)*info.height>MaxAlphaPixels){jpeg_destroy_decompress(&decoder);return false;}
 if(!window){jpeg_destroy_decompress(&decoder);return true;}
 decoder.out_color_space=JCS_RGB;jpeg_start_decompress(&decoder);
 row=static_cast<unsigned char*>(std::malloc(decoder.output_width*decoder.output_components));
 if(!row){jpeg_destroy_decompress(&decoder);return false;}
 while(decoder.output_scanline<decoder.output_height){
  const unsigned y=decoder.output_scanline;auto*p=const_cast<unsigned char*>(row);jpeg_read_scanlines(&decoder,&p,1);
  if(samples){samples->row(p,y,out,stride,3);continue;}
  if(y<window->y||y-window->y>=window->height)continue;
  const unsigned count=window->x<info.width?std::min(window->width,info.width-window->x):0;
  for(unsigned x=0;x<count;++x){
   const auto*color=p+(x+window->x)*3;
   out[(y-window->y)*stride+x]=uint32_t(color[0])|(uint32_t(color[1])<<8)|(uint32_t(color[2])<<16)|0xff000000u;
  }
 }
 std::free(const_cast<unsigned char*>(row));row=nullptr;jpeg_finish_decompress(&decoder);jpeg_destroy_decompress(&decoder);return true;
}
inline bool pngRead(FILE*f,Info&info,const Window*window,uint32_t*out,unsigned stride,Samples*samples=nullptr){
 auto*png=png_create_read_struct(PNG_LIBPNG_VER_STRING,nullptr,nullptr,nullptr);if(!png)return false;
 auto*meta=png_create_info_struct(png);if(!meta){png_destroy_read_struct(&png,nullptr,nullptr);return false;}
 unsigned char* volatile row=nullptr;
 if(setjmp(png_jmpbuf(png))){std::free(const_cast<unsigned char*>(row));png_destroy_read_struct(&png,&meta,nullptr);return false;}
 png_set_user_limits(png,MaxDimension,MaxDimension);png_init_io(png,f);png_read_info(png,meta);
 const unsigned width=png_get_image_width(png,meta),height=png_get_image_height(png,meta);
 if(!width||!height||width>MaxDimension||height>MaxDimension){png_destroy_read_struct(&png,&meta,nullptr);return false;}
 info={width,height,false};
 if(samples)samples->configure(width,height);
 if(!window){png_destroy_read_struct(&png,&meta,nullptr);return true;}
 const unsigned type=png_get_color_type(png,meta),depth=png_get_bit_depth(png,meta);
 if(depth==16)png_set_strip_16(png);
 if(type==PNG_COLOR_TYPE_PALETTE)png_set_palette_to_rgb(png);
 if(type==PNG_COLOR_TYPE_GRAY&&depth<8)png_set_expand_gray_1_2_4_to_8(png);
 const bool transparent=png_get_valid(png,meta,PNG_INFO_tRNS);
 if(transparent)png_set_tRNS_to_alpha(png);
 if(type==PNG_COLOR_TYPE_GRAY||type==PNG_COLOR_TYPE_GRAY_ALPHA)png_set_gray_to_rgb(png);
 if(!(type&PNG_COLOR_MASK_ALPHA)&&!transparent)png_set_add_alpha(png,255,PNG_FILLER_AFTER);
 const int passes=png_set_interlace_handling(png);png_read_update_info(png,meta);
 const unsigned rowBytes=png_get_rowbytes(png,meta);
 if(rowBytes!=width*4){png_destroy_read_struct(&png,&meta,nullptr);return false;}
 row=static_cast<unsigned char*>(std::calloc(1,rowBytes));
 if(!row){png_destroy_read_struct(&png,&meta,nullptr);return false;}
 // Noninterlaced files use one row. Adam7 must merge persistent selected rows.
 if(passes>1){
  for(int pass=0;pass<passes;++pass){if(samples)samples->next=0;for(unsigned y=0;y<height;++y){
   std::memset(const_cast<unsigned char*>(row),0,rowBytes);
   png_read_row(png,const_cast<unsigned char*>(row),nullptr);
   if(samples){constexpr unsigned yStart[]={0,0,4,0,2,0,1},yStep[]={8,8,8,4,4,2,2};if(y>=yStart[pass]&&!((y-yStart[pass])%yStep[pass]))samples->row(const_cast<const unsigned char*>(row),y,out,stride,4,pass);continue;}
   if(y<window->y||y-window->y>=window->height)continue;
   // png combines only the pixels belonging to this Adam7 pass.
   constexpr unsigned xs[]={0,4,0,2,0,1,0},ys[]={0,0,4,0,2,0,1},dx[]={8,8,4,4,2,2,1},dy[]={8,8,8,4,4,2,2};
   if(y<ys[pass]||(y-ys[pass])%dy[pass])continue;
   for(unsigned x=xs[pass];x<width;x+=dx[pass])if(x>=window->x&&x-window->x<window->width)
    std::memcpy(out+(y-window->y)*stride+x-window->x,const_cast<const unsigned char*>(row)+4*x,4);
  }}
 }else for(unsigned y=0;y<height;++y){
  png_read_row(png,const_cast<unsigned char*>(row),nullptr);
  if(samples){samples->row(const_cast<const unsigned char*>(row),y,out,stride,4);continue;}
  if(y<window->y||y-window->y>=window->height)continue;
  const unsigned count=window->x<width?std::min(window->width,width-window->x):0;
  std::memcpy(out+(y-window->y)*stride,const_cast<const unsigned char*>(row)+std::min(window->x,width)*4,count*4);
 }
 png_read_end(png,meta);std::free(const_cast<unsigned char*>(row));png_destroy_read_struct(&png,&meta,nullptr);return true;
}
inline bool load(const char*path,Info&info,const Window*window,uint32_t*out,unsigned stride,Samples*samples=nullptr){
 auto*f=std::fopen(path,"rb");if(!f)return false;unsigned char signature[8]{};
 bool ok=std::fread(signature,1,8,f)==8&&std::fseek(f,0,SEEK_SET)==0;
 if(ok)ok=png_sig_cmp(signature,0,8)==0?pngRead(f,info,window,out,stride,samples):
  signature[0]==0xff&&signature[1]==0xd8?jpegRead(f,info,window,out,stride,samples!=nullptr,samples):false;
 std::fclose(f);return ok;
}
inline bool inspect(const char*path,Info&info){return load(path,info,nullptr,nullptr,0);}
// One full source decode with bounded row storage, sampling original RGBA texels
// at destination pixel centers. Original files and world-image data stay intact.
inline bool thumbnail(const char*path,uint32_t*out,unsigned width,unsigned height,unsigned stride){
 if(!path||!out||!width||!height||width>MaxWindow||height>MaxWindow||stride<width)return false;
 for(unsigned y=0;y<height;++y)std::fill_n(out+y*stride,width,0u);
 Samples samples;samples.width=width;samples.height=height;Info info;const Window window{0,0,width,height};return load(path,info,&window,out,stride,&samples);
}
inline bool read(const char*path,Window window,uint32_t*out,unsigned stride){
 if(!out||!window.width||!window.height||window.width>MaxWindow||window.height>MaxWindow||stride<window.width||
    window.y>UINT32_MAX-window.height||window.x>UINT32_MAX-window.width)return false;
 for(unsigned y=0;y<window.height;++y)std::fill_n(out+y*stride,window.width,0u);
 Info info;return load(path,info,&window,out,stride);
}

// One source decode, one packed alpha bit per pixel; no full RGBA image allocation.
struct AlphaMask {
 Info info{};unsigned char*bits=nullptr;size_t bytes=0;
 AlphaMask()=default;AlphaMask(const AlphaMask&)=delete;AlphaMask&operator=(const AlphaMask&)=delete;
 ~AlphaMask(){clear();}
 void clear(){std::free(bits);bits=nullptr;bytes=0;info={};}
 bool at(unsigned x,unsigned y)const{const size_t i=size_t(y)*info.width+x;return bits&&(bits[i>>3]&(1u<<(i&7)));}
};
inline bool alphaAllocate(AlphaMask&mask){
 const uint64_t pixels=uint64_t(mask.info.width)*mask.info.height;
 if(!pixels||pixels>MaxAlphaPixels)return false;
 mask.bytes=size_t((pixels+7)/8);mask.bits=static_cast<unsigned char*>(std::calloc(mask.bytes,1));
 return mask.bits!=nullptr;
}
inline bool pngAlpha(FILE*f,AlphaMask&mask){
 auto*png=png_create_read_struct(PNG_LIBPNG_VER_STRING,nullptr,nullptr,nullptr);if(!png)return false;
 auto*meta=png_create_info_struct(png);if(!meta){png_destroy_read_struct(&png,nullptr,nullptr);return false;}
 unsigned char* volatile row=nullptr;
 if(setjmp(png_jmpbuf(png))){std::free(const_cast<unsigned char*>(row));png_destroy_read_struct(&png,&meta,nullptr);return false;}
 png_set_user_limits(png,MaxDimension,MaxDimension);png_set_crc_action(png,PNG_CRC_ERROR_QUIT,PNG_CRC_ERROR_QUIT);
 png_init_io(png,f);png_read_info(png,meta);
 const unsigned width=png_get_image_width(png,meta),height=png_get_image_height(png,meta);
 mask.info={width,height,false};
 if(!width||!height||width>MaxDimension||height>MaxDimension||!alphaAllocate(mask)){png_destroy_read_struct(&png,&meta,nullptr);return false;}
 const unsigned type=png_get_color_type(png,meta),depth=png_get_bit_depth(png,meta);
 if(depth==16)png_set_strip_16(png);
 if(type==PNG_COLOR_TYPE_PALETTE)png_set_palette_to_rgb(png);
 if(type==PNG_COLOR_TYPE_GRAY&&depth<8)png_set_expand_gray_1_2_4_to_8(png);
 const bool transparent=png_get_valid(png,meta,PNG_INFO_tRNS);
 if(transparent)png_set_tRNS_to_alpha(png);
 if(type==PNG_COLOR_TYPE_GRAY||type==PNG_COLOR_TYPE_GRAY_ALPHA)png_set_gray_to_rgb(png);
 if(!(type&PNG_COLOR_MASK_ALPHA)&&!transparent)png_set_add_alpha(png,255,PNG_FILLER_AFTER);
 const int passes=png_set_interlace_handling(png);png_read_update_info(png,meta);
 const unsigned rowBytes=png_get_rowbytes(png,meta);
 if(rowBytes!=width*4){png_destroy_read_struct(&png,&meta,nullptr);return false;}
 row=static_cast<unsigned char*>(std::calloc(1,rowBytes));
 if(!row){png_destroy_read_struct(&png,&meta,nullptr);return false;}
 constexpr unsigned xs[]={0,4,0,2,0,1,0},ys[]={0,0,4,0,2,0,1},dx[]={8,8,4,4,2,2,1},dy[]={8,8,8,4,4,2,2};
 for(int pass=0;pass<passes;++pass)for(unsigned y=0;y<height;++y){
  if(passes>1)std::memset(const_cast<unsigned char*>(row),0,rowBytes);
  png_read_row(png,const_cast<unsigned char*>(row),nullptr);
  if(passes>1&&(y<ys[pass]||(y-ys[pass])%dy[pass]))continue;
  for(unsigned x=passes>1?xs[pass]:0;x<width;x+=passes>1?dx[pass]:1){
   // Godot's alpha/255.0 > 0.1 is exactly alpha >= 26 for eight-bit alpha.
   if(const_cast<const unsigned char*>(row)[x*4+3]>=26){const size_t i=size_t(y)*width+x;mask.bits[i>>3]|=1u<<(i&7);}
  }
 }
 png_read_end(png,meta);std::free(const_cast<unsigned char*>(row));png_destroy_read_struct(&png,&meta,nullptr);return true;
}
inline bool readAlpha(const char*path,AlphaMask&mask,const char*&error){
 mask.clear();error=nullptr;
 if(!path||!*path){error="Image path is empty";return false;}
 auto*f=std::fopen(path,"rb");if(!f){error="Cannot open image";return false;}
 unsigned char signature[8]{};bool ok=std::fread(signature,1,8,f)==8&&std::fseek(f,0,SEEK_SET)==0;
 if(ok&&png_sig_cmp(signature,0,8)==0)ok=pngAlpha(f,mask);
 else if(ok&&signature[0]==0xff&&signature[1]==0xd8){
  const Window empty{0,0,0,0};Info info{};ok=jpegRead(f,info,&empty,nullptr,0,true);mask.info=info;
  if(ok)ok=alphaAllocate(mask);
  if(ok){std::memset(mask.bits,255,mask.bytes);const unsigned tail=(uint64_t(info.width)*info.height)&7;if(tail)mask.bits[mask.bytes-1]&=(1u<<tail)-1;}
 }else ok=false;
 if(std::ferror(f))ok=false;
 std::fclose(f);
 if(!ok){
  error=uint64_t(mask.info.width)*mask.info.height>MaxAlphaPixels?"Image exceeds 16M-pixel alpha limit":"Invalid or truncated PNG/JPEG, unsupported dimensions, or insufficient memory";
  mask.clear();
 }
 return ok;
}
}


#include "editor_image_resize.hpp"
