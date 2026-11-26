#pragma once

// Uses the existing UTF-8 decoder and the loaded locale glyph advances.
namespace ReadableText {
struct Line {
    const char* first;
    const char* last;
    unsigned offset,characters,width;
};

template<class Emit>
void lines(const char* text,unsigned maxWidth,Emit emit) {
    const char* cursor=text;
    unsigned offset=0;
    while (*cursor) {
        while (*cursor==' ') { ++cursor; ++offset; }
        if (!*cursor) break;
        const char* first=cursor;
        const char* space=nullptr;
        unsigned width=0,count=0,spaceWidth=0,spaceCount=0;
        while (*cursor && *cursor!='\n') {
            const char* glyph=cursor;
            const unsigned advance=glyphAdvance[nextGlyph(cursor)];
            if (width+advance>maxWidth && count) {
                cursor=glyph;
                if (space) { cursor=space; width=spaceWidth; count=spaceCount; }
                break;
            }
            if (*glyph==' ') { space=glyph; spaceWidth=width; spaceCount=count; }
            width+=advance;
            ++count;
        }
        emit(Line{first,cursor,offset,count,width});
        offset+=count;
        if (*cursor==' ' || *cursor=='\n') { ++cursor; ++offset; }
    }
}
}
