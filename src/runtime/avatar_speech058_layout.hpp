#pragma once
#include <cstdint>
#include "avatar_speech058_data.hpp"
namespace AvatarSpeech058 {
constexpr unsigned MaxText=96, MaxGlyphs=80, MaxLines=8;
struct Glyph { int16_t x,y; uint8_t letter,phase,image; };
struct Line { uint8_t first,end; int16_t width,x,baseline; };
struct Layout {
    char text[MaxText]{};
    Glyph glyphs[MaxGlyphs]{};
    Line lines[MaxLines]{};
    uint8_t glyphCount=0,lineCount=0;
};
inline unsigned index(unsigned char letter) {
    for (unsigned i=0;Data::Alphabet[i];++i) if (static_cast<unsigned char>(Data::Alphabet[i])==letter) return i;
    return 0;
}
inline unsigned advance(unsigned char letter) { return Data::Advance64[index(letter)]; }

inline Layout prepare(unsigned code, uint64_t count) {
    Layout result{};
    if (code<1 || code>4) return result;
    const char* messages[]={"Remember, you can drag from ANYWHERE on the screen!",
        "Please. Stop dragging from me. You can click anywhere. please",
        "Congratulations! You've optionally clicked on me %d times!", "Cowabunga"};
    unsigned length=0;
    for (const char* source=messages[code-1];*source;++source) {
        if (source[0]=='%' && source[1]=='d') {
            char digits[20]; unsigned n=0;
            do {digits[n++]=static_cast<char>('0'+count%10);count/=10;} while (count);
            while (n) result.text[length++]=digits[--n];
            ++source;
        } else result.text[length++]=*source;
    }
    unsigned first=0;
    while (first<length && result.lineCount<MaxLines) {
        unsigned end=first,width64=0,lastSpace=first;
        while (end<length) {
            const unsigned next=width64+advance(result.text[end]);
            if (next>128*64 && lastSpace>first) {end=lastSpace;break;}
            if (result.text[end]==' ') lastSpace=end;
            width64=next;
            ++end;
        }
        while (end>first && result.text[end-1]==' ') --end;
        width64=0;
        for (unsigned i=first;i<end;++i) width64+=advance(result.text[i]);
        auto& line=result.lines[result.lineCount++];
        line.first=static_cast<uint8_t>(first);line.end=static_cast<uint8_t>(end);
        line.width=static_cast<int16_t>((width64+63)/64);
        // RichTextLabel floors center alignment; TextParagraph ceil-measures lines.
        line.x=static_cast<int16_t>(-((line.width+1)/2));
        first=end;
        while (first<length && result.text[first]==' ') ++first;
    }
    const int totalHeight=18*result.lineCount-6*(result.lineCount-1);
    for (unsigned row=0;row<result.lineCount;++row) {
        auto& line=result.lines[row];
        line.baseline=static_cast<int16_t>(-118+(90-totalHeight)/2+14+12*row);
        int x64=line.x*64;
        for (unsigned i=line.first;i<line.end;++i) {
            const auto letter=static_cast<unsigned char>(result.text[i]);
            if (letter!=' ' && result.glyphCount<MaxGlyphs) {
                // Godot AUTO at size12 uses quarter phases and floor(x+1/8).
                const int rounded=x64+8;
                const int base=rounded>=0?rounded/64:-((-rounded+63)/64);
                const unsigned phase=static_cast<unsigned>((rounded-base*64)/16);
                result.glyphs[result.glyphCount++]={static_cast<int16_t>(base),line.baseline,letter,static_cast<uint8_t>(phase),static_cast<uint8_t>(index(letter)*4+phase)};
            }
            x64+=static_cast<int>(advance(letter));
        }
    }
    return result;
}
}
