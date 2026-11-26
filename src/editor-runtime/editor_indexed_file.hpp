#pragma once
#include "editor_indexed_source.hpp"
#include <cerrno>
#include <cstdio>
namespace EditorIndexedSource {
// One renderer-owned stream. No decoded slab or libc read-ahead allocation.
template<unsigned Count> class File {
  std::FILE*file=nullptr;
  Result state=Result::Unavailable;
  bool attempted=false;
  static bool readAt(void*context,uint32_t at,void*out,size_t bytes){
    auto*f=static_cast<File*>(context)->file;
    return f&&std::fseek(f,long(at),SEEK_SET)==0&&std::fread(out,1,bytes,f)==bytes;
  }
public:
  Reader<Count> reader;
  File()=default;
  File(const File&)=delete;
  File&operator=(const File&)=delete;
  ~File(){close();}
  void close(){reader.close();if(file)std::fclose(file);file=nullptr;attempted=false;state=Result::Unavailable;}
  Result open(const char*path){
    if(attempted)return state;
    attempted=true;errno=0;file=std::fopen(path,"rb");
    if(!file){state=errno==ENOENT?Result::Unavailable:Result::Fault;return state;}
    state=Result::Fault;
    if(std::setvbuf(file,nullptr,_IONBF,0)==0&&std::fseek(file,0,SEEK_END)==0){
      const long size=std::ftell(file);
      if(size>=0&&uint64_t(size)<=UINT32_MAX&&reader.open(readAt,this,uint32_t(size)))state=Result::Ready;
    }
    if(state!=Result::Ready){std::fclose(file);file=nullptr;}
    return state;
  }
};
}
