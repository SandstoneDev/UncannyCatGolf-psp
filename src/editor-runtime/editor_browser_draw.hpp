#pragma once
namespace EditorBrowser {
inline constexpr float BrowserPi = 3.14159265358979323846f;
inline uint32_t hue(float value, uint32_t alpha = 0xff000000u) {
    float h = value - std::floor(value), p = h * 6;
    int i = int(p);
    float f = p - i;
    float r = 0, g = 0, b = 0;
    switch (i % 6) {
    case 0:
        r = 1;
        g = f;
        break;
    case 1:
        r = 1 - f;
        g = 1;
        break;
    case 2:
        g = 1;
        b = f;
        break;
    case 3:
        g = 1 - f;
        b = 1;
        break;
    case 4:
        r = f;
        b = 1;
        break;
    default:
        r = 1;
        b = 1 - f;
        break;
    }
    return alpha | uint32_t(r * 255) | (uint32_t(g * 255) << 8) | (uint32_t(b * 255) << 16);
}
inline float Browser::rich(Canvas &p, const std::string &input, float x, float y, float width,
                           float height, unsigned font, uint32_t tint, float offset) {
    if(p.richText){
      if(p.pushClip)p.pushClip(x,y,width,height);else if(p.clip)p.clip(x,y,width,height);
      const float result=p.richText(input,x,y,width,height,offset,float(animation)/60);
      if(p.popClip)p.popClip();else if(p.resetClip)p.resetClip();
      return result;
    }
    if (!p.text)
        return 0;
    struct Style {
        unsigned font;
        uint32_t tint;
        bool wave = false, shake = false, rainbow = false;
    };
    Style current{font, tint, false, false, false};
    std::vector<std::pair<std::string, Style>> stack;
    float lineHeight = font == p.smallFont ? 15 : font == p.titleFont ? 29 : 19;
    float ascent = font == p.smallFont ? 12 : font == p.titleFont ? 24 : 16;
    if(p.fontMetrics){const auto m=p.fontMetrics(font);ascent=m.first;lineHeight=m.first+m.second+3;}
    float at = x, baseline = y + ascent - offset, last = y + height;
    if(p.pushClip)p.pushClip(x,y,width,height);
    else if (p.clip)p.clip(x,y,width,height);
    unsigned index = 0;
    const float clock = float(animation) / 60;
    for (size_t i = 0; i < input.size();) {
        if (input[i] == '[') {
            size_t close = input.find(']', i + 1);
            if (close != std::string::npos) {
                std::string tag = input.substr(i + 1, close - i - 1),
                            name = tag.substr(0, tag.find_first_of(" ="));
                bool closing = !name.empty() && name[0] == '/';
                if (closing)
                    name.erase(name.begin());
                bool known = name == "wave" || name == "shake" || name == "rainbow" ||
                             name == "font" || name == "color" || name == "b" || name == "i" ||
                             name == "u" || name == "s";
                if (known) {
                    if (closing) {
                        for (size_t j = stack.size(); j > 0; --j)
                            if (stack[j - 1].first == name) {
                                current = stack[j - 1].second;
                                stack.resize(j - 1);
                                break;
                            }
                    } else {
                        stack.emplace_back(name, current);
                        if (name == "wave")
                            current.wave = true;
                        else if (name == "shake")
                            current.shake = true;
                        else if (name == "rainbow")
                            current.rainbow = true;
                        else if (name == "font") {
                            size_t equal = tag.find('=');
                            if (equal != std::string::npos) {
                                std::string path = tag.substr(equal + 1);
                                if (path.size() > 1 &&
                                    (path.front() == '\'' || path.front() == '"'))
                                    path = path.substr(1, path.size() - 2);
                                if (p.fontByPath) {
                                    unsigned selected = p.fontByPath(path, current.font);
                                    if (selected != ~0u)
                                        current.font = selected;
                                    else
                                        notice = tr("Description font unavailable: ",
                                                    "Нет шрифта описания: ") +
                                                 path;
                                } else
                                    notice = tr("Description font unavailable: ",
                                                "Нет шрифта описания: ") +
                                             path;
                            }
                        } else if (name == "color") {
                            auto hash = tag.find('#');
                            if (hash != std::string::npos) {
                                unsigned rgb = 0;
                                if (std::sscanf(tag.c_str() + hash + 1, "%6x", &rgb) == 1)
                                    current.tint = 0xff000000u | ((rgb >> 16) & 255) |
                                                   (rgb & 0xff00) | ((rgb & 255) << 16);
                            }
                        }
                    }
                    i = close + 1;
                    continue;
                }
            }
        }
        unsigned char lead = (unsigned char)input[i];
        size_t count = lead < 128 ? 1 : lead < 224 ? 2 : lead < 240 ? 3 : 4;
        if (i + count > input.size())
            break;
        std::string character = input.substr(i, count);
        i += count;
        if (character == "\n") {
            at = x;
            baseline += lineHeight;
            continue;
        }
        float advance = p.measure ? p.measure(current.font, character.c_str()) : 8.f;
        if (character == " ") {
            size_t end = i;
            while (end < input.size() && input[end] != ' ' && input[end] != '\n' &&
                   input[end] != '[')
                ++end;
            std::string word = input.substr(i, end - i);
            float next = p.measure ? p.measure(current.font, word.c_str()) : float(word.size()) * 8;
            if (at > x && at - x + advance + next > width) {
                at = x;
                baseline += lineHeight;
                continue;
            }
        }
        if (at - x + advance > width) {
            at = x;
            baseline += lineHeight;
        }
        float dy = current.wave ? std::sin(clock * 5 - float(index) * .4f) * 3 : 0, dx = 0;
        if (current.shake) {
            dy += std::sin(float(index) * 37 + std::floor(clock * 20) * 13) * 1.5f;
            dx = std::cos(float(index) * 13 + std::floor(clock * 20) * 41) * 1.5f;
        }
        uint32_t color = current.rainbow ? hue(clock * .3f + float(index) * .06f) : current.tint;
        if (baseline >= y - 4 && baseline <= last)
            p.text(current.font, character.c_str(), at + dx, baseline + dy, color,
                   width - (at - x));
        at += advance;
        ++index;
    }
    if(p.popClip)p.popClip();
    else if(p.resetClip)p.resetClip();
    return baseline + offset - y + lineHeight - ascent;
}
inline void Browser::draw(Canvas &p) {
    const uint32_t white = 0xffffffffu, yellow = 0xff00ffffu;
    auto rect = [&](float x, float y, float w, float h, uint32_t c) {
        if (p.rect)
            p.rect(x, y, w, h, c);
    };
    auto line = [&](unsigned f, const std::string &s, float x, float baseline,
                    uint32_t c = 0xffffffffu, float width = 500) {
        if (p.text)
            p.text(f, s.c_str(), x, baseline, c, width);
    };
    auto button = [&](Rect r, const std::string &label, bool focus) {
        // Original Editor Button theme: transparent center, white side borders.
        if(focus){rect(r.x,r.y,1,r.h,white);rect(r.x+r.w-1,r.y,1,r.h,white);}
        float textWidth =
            p.measure ? p.measure(p.bodyFont, label.c_str()) : float(label.size()) * 8;
        line(p.bodyFont, label, r.x + std::max(3.f, (r.w - textWidth) * .5f), r.y + r.h * .5f + 6,
             white, r.w - 6);
    };
    if (mode == Mode::Playing)
        return;
    if (mode == Mode::Menu) {
        if (p.menu)
            p.menu(russian, menuIndex, pressedPrimary ? 2 : 1);
        else {
            rect(256, 0, 512, 512, 0xff31453du);
            line(p.titleFont, tr("Level Editor", "Редактор уровней"), 352, 76);
            button(PlayButton, tr("Level Player", "Играть"), menuIndex == 0);
            button(EditButton, tr("Level Editor", "Редактор"), menuIndex == 1);
        }
        if(p.back)p.back(russian,float(animation)/60,8);
        else button(BackButton, tr("Back", "Назад"), BackButton.contains(cursor));
        return;
    }
    if(p.background)p.background(mode==Mode::Results,float(animation)/60);
    else rect(256,0,512,512,0xff252c2fu);
    auto label=[&](unsigned font,const std::string&text,float x,float y,float w,float h,unsigned align=0,bool middle=false,bool wrap=false,uint32_t color=0xffffffffu){
      if(p.label)p.label(font,text.c_str(),x,y,w,h,align,middle,wrap,color);
      else line(font,text,x,y+h*.5f+6,color,w);
    };
    auto back=[&](float y){if(p.back)p.back(russian,float(animation)/60,y);else button({264,y,64,32},tr("Back","Назад"),Rect{264,y,64,32}.contains(cursor));};
    if (mode == Mode::Worlds) {
        if(p.clip)p.clip(272,48,480,456);
        for(size_t wi=0;wi<worlds.size();++wi){
            const float y=48+float(wi)*220-scroll;
            if(y+212<48||y>504)continue;
            auto&world=worlds[wi];
            bool hover=Rect{272,y+4,472,208}.contains(cursor)||(keyFocus&&wi==worldIndex);
            if(p.skin)p.skin(hover?(pressedPrimary?2:1):0,272,y+4,472,208,white);
            else rect(272,y+4,472,208,hover?0xff7b8580u:0xff48534eu);
            label(p.bodyFont,world.name,384,y+4,352,32,0,true);
            label(p.authorFont,std::string("by ")+world.author,336,y+36,400,28,0,true,false);
            if(p.skin)p.skin(3,336,y+68,384,64,white);else rect(336,y+68,384,64,0x84101010u);
            world.descriptionHeight=rich(p,world.description,339,y+68,378,64,p.descriptionFont,white,world.descriptionScroll);
            // rich restores clip; every nested description remains within its world scroll viewport.
            if(p.clip)p.clip(272,48,480,456);
            if(p.worldIcon)p.worldIcon(world.icon.empty()?"":rootPath+"/"+world.directory+"/"+world.icon,false,286,y+30,36,36,white);
            else if(!world.icon.empty()&&p.image)p.image(rootPath+"/"+world.directory+"/"+world.icon,286,y+30,36,36);
            world.levelWidth=0;
            for(auto&level:world.metadata){level.buttonWidth=std::max(160.f,24+(p.measure?std::ceil(p.measure(p.sublevelFont,level.label.c_str())):25));world.levelWidth+=level.buttonWidth+4;}
            if(!world.metadata.empty())world.levelWidth-=4;
            world.levelScroll=std::clamp(world.levelScroll,0.f,std::max(0.f,world.levelWidth-456));
            if(p.clip)p.clip(280,std::max(48.f,y+144),456,std::max(0.f,std::min(504.f,y+204)-std::max(48.f,y+144)));
            float x=280-world.levelScroll;
            for(size_t li=0;li<world.metadata.size();++li){
                const float width=world.metadata[li].buttonWidth;const Rect r{x,y+144,width,32};
                if(x+width<280){x+=width+4;continue;}if(x>=736)break;
                bool selected=(keyFocus&&wi==worldIndex&&li==levelIndex)||r.contains(cursor);
                const int rank=rankFor(wi,li);const uint32_t tint=rank==5?yellow:rank<0?0xff696969u:white;
                if(p.skin){p.skin(selected?(pressedPrimary?6:5):4,x,y+144,width,32,tint);p.skin(rank>0?unsigned(15-rank):15,x+2,y+152,16,16,tint);}
                else button(r,world.metadata[li].label,selected);
                if(keyFocus&&wi==worldIndex&&li==levelIndex&&p.focus)p.focus(x,y+144,width,32,tint);
                const unsigned fontLevel=selected?(pressedPrimary?255:242):223;
                const uint32_t textTint=0xff000000u|(((tint&255)*fontLevel/255))|((((tint>>8)&255)*fontLevel/255)<<8)|((((tint>>16)&255)*fontLevel/255)<<16);
                if(p.skin)label(p.sublevelFont,world.metadata[li].label,x+22,y+144,width-24,32,0,true,false,textTint);
                const auto*score=stats.find(key(wi,li)+"_score");
                const std::string text=score?std::string("Score: ")+std::to_string(integer(score)):"No score...";
                label(p.scoreFont,text,x-4,y+176,width,28,1,true);
                x+=width+4;
            }
            if(p.clip)p.clip(272,48,480,456);
            if(world.levelWidth>456&&p.scrollbar){
              const float thumb=std::max(8.f,456*456/world.levelWidth),tx=280+(456-thumb)*world.levelScroll/(world.levelWidth-456);
              p.scrollbar(true,0,280,y+204,456,8);p.scrollbar(true,Rect{tx,y+204,thumb,8}.contains(cursor)?2:1,tx,y+204,thumb,8);
            }
        }
        if(p.resetClip)p.resetClip();
        if(worlds.size()>2&&p.scrollbar){const float total=float(worlds.size())*220-4,thumb=std::max(8.f,456*456/total),ty=48+(456-thumb)*scroll/(total-456);p.scrollbar(false,0,744,48,8,456);p.scrollbar(false,Rect{744,ty,8,thumb}.contains(cursor)?2:1,744,ty,8,thumb);}
        label(p.titleFont,"Custom Levels",400,8,352,33,2,true);
        back(8);
        if(!notice.empty())line(p.smallFont,notice,272,507,0xff40c0ffu,476);
        return;
    }
    if (mode == Mode::LevelResults) {
        line(p.titleFont, tr("LEVEL COMPLETE!", "УРОВЕНЬ ПРОЙДЕН!"), 312, 144, yellow, 432);
        line(p.bodyFont, std::string(tr("Score: ", "Счёт: ")) + std::to_string(latest.score), 312,
             212);
        line(p.bodyFont, std::string(tr("Strokes: ", "Удары: ")) + std::to_string(latest.strokes),
             312, 244);
        line(p.bodyFont, std::string(tr("Time: ", "Время: ")) + timeText(int64_t(latest.seconds)),
             312, 276);
        line(p.smallFont, std::string(tr("Rank: ", "Ранг: ")) + std::to_string(latest.rank), 312,
             308);
        button({264, 400, 144, 40}, tr("Next", "Далее"), true);
        button({424, 400, 144, 40}, tr("Retry", "Повторить"), false);
        button({584, 400, 144, 40}, tr("Skip", "Пропустить"), false);
        button(BackButton, tr("Back", "Назад"), false);
        return;
    }
    if (mode == Mode::Results) {
        const auto&world=worlds[worldIndex];
        const std::string png=rootPath+"/"+world.directory+"/icon.png";
        if(p.worldIcon)p.worldIcon(EditorFiles::exists(png)?png:"",true,256,0,512,512,0xc7ffffffu);
        const float titleY=resultPose(animation,0),statsY=resultPose(animation,1);
        label(p.resultTitleFont,world.named?world.name+" COMPLETED!":"You messed something up but you completed the world so good for you",272,titleY-40,480,80,1,true,true);
        const std::string rows=
          std::string("Levels Skipped: ")+std::to_string(summary.skips)+"/"+std::to_string(summary.levels)+"\n"+
          "Levels Peaked: "+std::to_string(summary.peaks)+"/"+std::to_string(summary.levels)+"\n"+
          "Total Strokes: "+std::to_string(summary.strokes)+"\n"+
          "Total Score: "+std::to_string(summary.score)+"\n"+
          "Total Time: "+timeText(summary.seconds)+"\n";
        label(p.resultFont,rows,264,statsY,496,248,1,false,true);
        back(resultPose(animation,2)-24);
        return;
    }
    if (mode == Mode::Error) {
        rect(272,76,472,344,0xff000000u); // Original Editor Panel theme
        line(p.titleFont, tr("Unable to continue", "Не удалось продолжить"), 288, 116, yellow, 432);
        rich(p, error, 288, 140, 432, 192, p.bodyFont, white);
        button({288, 344, 192, 40}, tr("Retry", "Повторить"), true);
        button({520, 344, 192, 40}, tr("Back", "Назад"), false);
    }
}
} // namespace EditorBrowser
