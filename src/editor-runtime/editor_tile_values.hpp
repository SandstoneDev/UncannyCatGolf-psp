#pragma once
#include <cctype>
#include <cstdint>
#include <string>
namespace EditorTileValues {
// Common source TileMap coordinates are JSON-integer pairs. Reject any other
// grammar so the existing general tuple reader retains its full semantics.
inline bool pair(const std::string&s,int&x,int&y){
 const char*p=s.c_str();auto ws=[&](){while(*p&&std::isspace(static_cast<unsigned char>(*p)))++p;};
 auto integer=[&](int&out){bool negative=false;if(*p=='-'){negative=true;++p;}if(*p<'0'||*p>'9')return false;const bool zero=*p=='0';uint64_t value=0;unsigned digits=0;
  while(*p>='0'&&*p<='9'){if(value>2147483648ull/10)return false;value=value*10+unsigned(*p++-'0');++digits;if(value>(negative?2147483648ull:2147483647ull))return false;}
  if(zero&&digits>1)return false;
  out=negative?(value==2147483648ull?(-2147483647-1):-int(value)):int(value);return true;
 };
 int a=0,b=0;ws();if(*p++!='(')return false;ws();if(!integer(a))return false;ws();if(*p++!=',')return false;ws();if(!integer(b))return false;ws();if(*p++!=')')return false;ws();if(*p)return false;x=a;y=b;return true;
}
}
