#pragma once
#include "editor_files.hpp"
#include "editor_input.hpp"
#include "editor_browser_tweens.hpp"
#include <cstdint>
#include <functional>
namespace EditorBrowser {
using Value = ucg::editor::Value;
struct Canvas {
    std::function<void(float, float, float, float, uint32_t)> rect;
    std::function<void(unsigned, const char *, float, float, uint32_t, float)> text;
    std::function<float(unsigned, const char *)> measure;
    std::function<std::pair<float,float>(unsigned)> fontMetrics;
    std::function<bool(const std::string &, float, float, float, float)> image;
    std::function<void(bool, unsigned, unsigned)> menu;
    std::function<unsigned(const std::string &, unsigned)> fontByPath;
    std::function<void(float, float, float, float)> clip;
    std::function<void()> resetClip;
    std::function<void(float,float,float,float)> pushClip;
    std::function<void()> popClip;
    std::function<float(const std::string&,float,float,float,float,float,float)> richText;
    std::function<void(bool,float)> background;
    std::function<void(bool,float,float)> back;
    std::function<void(unsigned,float,float,float,float,uint32_t)> skin;
    std::function<void(bool,unsigned,float,float,float,float)> scrollbar;
    std::function<void(float,float,float,float,uint32_t)> focus;
    std::function<void(unsigned,const char*,float,float,float,float,unsigned,bool,bool,uint32_t)> label;
    std::function<void(const std::string&,bool,float,float,float,float,uint32_t)> worldIcon;
    unsigned authorFont=3,sublevelFont=0,descriptionFont=3,resultFont=0,resultTitleFont=5;
    unsigned titleFont = 5, bodyFont = 0, smallFont = 3, scoreFont = 4;
};
struct LevelView {
    std::string filename, label, title;
    float buttonWidth=49;
    int64_t ranks[4] = {7500, 6500, 5500, 5000};
    bool rankPresent[4]{};
};
struct WorldView : EditorFiles::World {
    std::vector<LevelView> metadata;
    float descriptionScroll = 0, descriptionHeight = 0,levelScroll=0,levelWidth=0;
};
struct Summary {
    int skips = 0, peaks = 0, levels = 0;
    int64_t strokes = 0, score = 0, seconds = 0;
    bool builder = false;
};
struct Rect {
    float x, y, w, h;
    bool contains(ucg::editor::Vec2 p) const {
        return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h;
    }
};
inline int64_t integer(const Value *v, int64_t fallback = 0) {
    if (!v || v->kind != Value::Kind::Number)
        return fallback;
    const auto &t = v->text;
    if (t.find_first_of(".eE") == std::string::npos) {
        errno = 0;
        char *end = nullptr;
        long long n = std::strtoll(t.c_str(), &end, 10);
        if (!errno && end && *end == 0)
            return int64_t(n);
    }
    double d = v->numeric();
    if (d >= 9223372036854775808.0 || d < -9223372036854775808.0 || !std::isfinite(d))
        return INT64_MIN;
    return int64_t(d);
}
inline int64_t add(int64_t a, int64_t b) {
    uint64_t bits = uint64_t(a) + uint64_t(b);
    int64_t n;
    std::memcpy(&n, &bits, sizeof(n));
    return n;
}
inline std::string text(const Value &v, const char *key, const std::string &fallback = {}) {
    const auto *p = v.find(key);
    return p && p->kind == Value::Kind::String ? p->text : fallback;
}
class Browser {
    friend struct FocusAccess067;
  public:
    enum class Mode { Menu, Worlds, Playing, LevelResults, Results, Error };
    Mode mode = Mode::Menu;
    std::string error, notice;
    std::vector<WorldView> worlds;
    Value stats = Value::dict();
    Summary summary;
    std::function<bool(const std::string &, const std::string &, std::string &)> onLaunchWorldLevel;
    std::function<void()> onOpenEditor, onReturnMain, onBeforeRefresh, onBeforeResults;
    std::function<double()> clock;
    std::function<bool(std::string &)> onBuilderReward;
    ucg::editor::FileOps fileOps;
    bool open(const std::string &root, const std::string &statsFile, bool ru,
              std::string &outError) {
        rootPath = root;
        statsPath = statsFile;
        russian = ru;
        statsReady = false;
        resultsPrepared = false;
        worldIndex = levelIndex = 0;
        menuIndex = 0;
        scroll = 0;
        error.clear();
        notice.clear();
        animation = 0;
        summary = {};
        worlds.clear();
        catalogueReady069 = false;
        if (!EditorFiles::make(rootPath)) {
            outError = "Cannot create custom_levels: " + rootPath;
            fail(outError, Recovery::Open);
            return false;
        }
        if (!loadStats(outError)) {
            fail(outError, Recovery::Open);
            return false;
        }
        mode = Mode::Menu;
        outError.clear();
        return true;
    }
    void update(const EditorInput::Actions &, ucg::editor::Vec2, unsigned pressed = 0,
                unsigned held = 0);
    void draw(Canvas &);
    bool start(size_t world, size_t level) {
        if (!catalogueReady069) {
            std::string e;
            if (!refresh(e)) { fail(e, Recovery::Browse); return false; }
        }
        if (world >= worlds.size() || level >= worlds[world].levels.size()) {
            fail("Selected world/level no longer exists", Recovery::Browse);
            return false;
        }
        worldIndex = world;
        levelIndex = level;
        return launch();
    }
    void notifyFinished(int64_t score, unsigned strokes, double seconds, int rank) {
        if (mode != Mode::Playing)
            return;
        if (!std::isfinite(seconds) || seconds < 0) {
            fail("Invalid level completion time", Recovery::Launch);
            return;
        }
        latest = {score, strokes, seconds, rank};
        pendingResult = true;
        commitResult();
    }
    void notifySkipped() {
        if (mode != Mode::Playing && mode != Mode::LevelResults)
            return;
        advance();
    }
    void retry() {
        if (mode == Mode::Error) {
            recover();
            return;
        }
        if (worldIndex < worlds.size() && levelIndex < worlds[worldIndex].levels.size())
            launch();
    }
    void next() {
        if (mode == Mode::LevelResults)
            advance();
    }
    void returnToWorlds() {
        std::string e;
        if (!statsReady && !loadStats(e)) {
            fail(e, Recovery::Browse);
            return;
        }
        if (!refresh(e)) {
            fail(e, Recovery::Browse);
            return;
        }
        mode = Mode::Worlds;
        error.clear();
        pendingResult = false;
        worldIndex = worlds.empty() ? 0 : std::min(worldIndex, worlds.size() - 1);
        levelIndex = worlds.empty() || worlds[worldIndex].levels.empty()
                         ? 0
                         : std::min(levelIndex, worlds[worldIndex].levels.size() - 1);
    }
    size_t selected_world() const { return worldIndex; }
    size_t selected_level() const { return levelIndex; }
    std::string playing_world() const {
        return worldIndex < worlds.size() ? worlds[worldIndex].directory : "";
    }
    std::string playing_file() const {
        return worldIndex < worlds.size() && levelIndex < worlds[worldIndex].levels.size()
                   ? worlds[worldIndex].levels[levelIndex]
                   : "";
    }
    void set_language(bool ru) { russian = ru; }
    static constexpr Rect PlayButton{320, 234, 384, 80}, EditButton{320, 326, 384, 80},
        BackButton{264, 8, 120, 64};

  private:
    enum class Recovery { Open, Browse, Launch, Save, Reward };
    Recovery recovery = Recovery::Open;
    struct Result {
        int64_t score = 0;
        unsigned strokes = 0;
        double seconds = 0;
        int rank = 0;
    };
    Result latest;
    std::string rootPath, statsPath;
    bool russian = false, statsReady = false, pendingResult = false, keyFocus = false;
    bool resultsPrepared = false, catalogueReady069 = false;
    size_t worldIndex = 0, levelIndex = 0;
    unsigned menuIndex = 0, animation = 0;
    Mode animationMode=Mode::Menu;double animationStart=0,lastClock=-1;
    float scroll = 0;
    ucg::editor::Vec2 cursor{};
    bool pressedPrimary = false;
    int draggingScroll=-2; // -2 none, -1 world list, otherwise horizontal world row
    void revealLevel(){if(worldIndex>=worlds.size())return;auto&w=worlds[worldIndex];float x=0;for(size_t i=0;i<w.metadata.size();++i){const float right=x+w.metadata[i].buttonWidth;if(i==levelIndex){if(x<w.levelScroll)w.levelScroll=x;else if(right>w.levelScroll+456)w.levelScroll=right-456;break;}x=right+4;}}
    const char *tr(const char *en, const char *ru) const { return russian ? ru : en; }
    void fail(const std::string &message, Recovery next) {
        error = message;
        recovery = next;
        mode = Mode::Error;
    }
    bool loadStats(std::string &e) {
        ucg::editor::Document d;
        if (EditorFiles::exists(statsPath)) {
            if (!ucg::editor::load_file(statsPath, d, e))
                return false;
        } else {
            if (!EditorFiles::make(EditorFiles::base(statsPath))) {
                e = "Cannot create custom statistics folder";
                return false;
            }
            if (!ucg::editor::save_atomic(statsPath, d, e, fileOps))
                return false;
        }
        stats = std::move(d.root);
        statsReady = true;
        return true;
    }
    bool refresh(std::string &e) {
        catalogueReady069 = false;
        // Release immutable view caches before parsing a new catalogue.
        if (onBeforeRefresh) onBeforeRefresh();
        std::vector<EditorFiles::World> catalog;
        if (!EditorFiles::worlds(rootPath, catalog, e))
            return false;
        if (catalog.size() > 128) {
            e = "Custom world limit 128 exceeded";
            return false;
        }
        std::vector<WorldView> next;
        size_t total = 0;
        for (auto &item : catalog) {
            WorldView w;
            static_cast<EditorFiles::World &>(w) = std::move(item);
            const std::string folder = rootPath + "/" + w.directory;
            if (EditorFiles::exists(folder + "/icon.jpg"))
                w.icon = "icon.jpg";
            for (const auto &filename : w.levels) {
                if (++total > 4096) {
                    e = "Custom level catalogue limit 4096 exceeded";
                    return false;
                }
                ucg::editor::Document d;
                if (!ucg::editor::load_file(folder + "/" + filename, d, e)) {
                    e = filename + ": " + e;
                    return false;
                }
                LevelView l;
                l.filename = filename;
                l.label = text(d.root, "world") + "-" + text(d.root, "id");
                l.title = text(d.root, "levelName");
                for (int rank = 0; rank < 4; ++rank){
                    const auto*value=d.root.find("rank"+std::to_string(rank+1));l.rankPresent[rank]=value!=nullptr;
                    l.ranks[rank]=integer(value,l.ranks[rank]);
                }
                w.metadata.push_back(std::move(l));
            }
            next.push_back(std::move(w));
        }
        worlds = std::move(next);
        catalogueReady069 = true;
        e.clear();
        return true;
    }
    bool launch() {
        if (!statsReady) {
            fail("Custom statistics were not loaded", Recovery::Open);
            return false;
        }
        if (!onLaunchWorldLevel) {
            fail("No custom level launch handler", Recovery::Launch);
            return false;
        }
        std::string e;
        if (!onLaunchWorldLevel(rootPath + "/" + worlds[worldIndex].directory,
                                worlds[worldIndex].levels[levelIndex], e)) {
            fail(e.empty() ? "Custom level could not start" : e, Recovery::Launch);
            return false;
        }
        resultsPrepared = false;
        mode = Mode::Playing;
        error.clear();
        pendingResult = false;
        return true;
    }
    std::string key(size_t world, size_t level) const {
        return worlds[world].directory + worlds[world].levels[level];
    }
    bool commitResult() {
        const std::string k = key(worldIndex, levelIndex);
        const auto *old = stats.find(k + "_score");
        if (old && integer(old) > latest.score) {
            pendingResult = false;
            mode = Mode::LevelResults;
            return true;
        }
        Value candidate = stats;
        candidate[k + "_time"] = Value::number(latest.seconds);
        candidate[k + "_strokes"] = Value::number(latest.strokes);
        candidate[k + "_score"] = Value::number_text(std::to_string(latest.score));
        candidate[k + "_rank1"] =
            Value::number_text(std::to_string(worlds[worldIndex].metadata[levelIndex].ranks[0]));
        ucg::editor::Document d;
        d.root = candidate;
        std::string e;
        if (!ucg::editor::save_atomic(statsPath, d, e, fileOps)) {
            fail("Custom statistics: " + e, Recovery::Save);
            return false;
        }
        stats = std::move(candidate);
        pendingResult = false;
        mode = Mode::LevelResults;
        error.clear();
        return true;
    }
    void advance() {
        if (++levelIndex < worlds[worldIndex].levels.size()) {
            launch();
            return;
        }
        levelIndex = worlds[worldIndex].levels.empty() ? 0 : worlds[worldIndex].levels.size() - 1;
        finishWorld();
    }
    void finishWorld() {
        // Dispose obsolete world pixels before the first Results allocation or
        // reward cue. Reward recovery preserves this preparation and Pending.
        if (!resultsPrepared) {
            resultsPrepared = true;
            if (onBeforeResults) onBeforeResults();
        }
        summary = {};
        summary.levels = int(worlds[worldIndex].levels.size());
        for (size_t i = 0; i < worlds[worldIndex].levels.size(); ++i) {
            const std::string k = key(worldIndex, i);
            summary.seconds = add(summary.seconds, integer(stats.find(k + "_time")));
            summary.strokes = add(summary.strokes, integer(stats.find(k + "_strokes")));
            const auto *score = stats.find(k + "_score");
            if (!score) {
                ++summary.skips;
                continue;
            }
            int64_t n = integer(score);
            if (n == 0)
                ++summary.skips;
            summary.score = add(summary.score, n);
            const auto *peak = stats.find(k + "_rank1");
            if (peak && n >= integer(peak))
                ++summary.peaks;
        }
        summary.builder = worlds[worldIndex].directory == "ExampleWorld" && summary.skips == 0 &&
                          summary.levels > 0;
        if (summary.builder) {
            std::string e;
            if (!onBuilderReward || !onBuilderReward(e)) {
                fail(e.empty() ? "Builder costume reward handler unavailable" : e,
                     Recovery::Reward);
                return;
            }
        }
        mode = Mode::Results;
        animation = 0;
        error.clear();
    }
    void recover() {
        std::string e;
        switch (recovery) {
        case Recovery::Open:
            if (loadStats(e) && refresh(e)) {
                mode = Mode::Menu;
                error.clear();
            } else
                fail(e, Recovery::Open);
            break;
        case Recovery::Browse:
            returnToWorlds();
            break;
        case Recovery::Launch:
            launch();
            break;
        case Recovery::Save:
            if (pendingResult)
                commitResult();
            else
                returnToWorlds();
            break;
        case Recovery::Reward:
            finishWorld();
            break;
        }
    }
    int rankFor(size_t w, size_t l) const {
        const auto *s = stats.find(key(w, l) + "_score");
        if (!s)
            return -1;
        int64_t score = integer(s);
        for (int i = 0; i < 4; ++i)
            if (worlds[w].metadata[l].rankPresent[i] && score >= worlds[w].metadata[l].ranks[i])
                return 5 - i;
        return score > 0 ? 1 : 0;
    }
    float rich(Canvas &, const std::string &, float, float, float, float, unsigned, uint32_t,
               float offset = 0);
    static std::string timeText(int64_t value) {
        bool negative = value < 0;
        uint64_t n = negative ? uint64_t(-(value + 1)) + 1 : uint64_t(value);
        std::string seconds = std::to_string(n % 60);
        if (seconds.size() < 2)
            seconds = "0" + seconds;
        return (negative ? "-" : "") + std::to_string(n / 60) + ":" + seconds;
    }
};
inline void Browser::update(const EditorInput::Actions &a, ucg::editor::Vec2 at, unsigned pressed,
                            unsigned held) {
    auto previousCursor = cursor;
    cursor = at;
    pressedPrimary = a.primary;
    if(clock){const double now=clock();if(std::isfinite(now)){if(lastClock<0||animationMode!=mode||now<animationStart){animationStart=now;animationMode=mode;}animation=unsigned(std::max(0.,std::min((now-animationStart)*60.,double(UINT32_MAX))));lastClock=now;}}else animation++;
    bool chord = held & EditorInput::Select;
    bool up = !chord && (pressed & EditorInput::Up), down = !chord && (pressed & EditorInput::Down),
         left = !chord && (pressed & EditorInput::Left),
         right = !chord && (pressed & EditorInput::Right);
    bool activate = a.primaryDown;
    if (previousCursor.x != at.x || previousCursor.y != at.y)
        keyFocus = false;
    if (up || down || left || right)
        keyFocus = true;
    if (mode == Mode::Playing)
        return;
    if (mode == Mode::Error) {
        if (a.back || (activate && Rect{520, 344, 192, 40}.contains(at))) {
            mode = Mode::Menu;
            return;
        }
        if ((activate && Rect{288, 344, 192, 40}.contains(at)) || a.pause)
            recover();
        return;
    }
    if (mode == Mode::Menu) {
        if (up || down)
            menuIndex = 1 - menuIndex;
        if (!keyFocus) {
            if (PlayButton.contains(cursor))
                menuIndex = 0;
            else if (EditButton.contains(cursor))
                menuIndex = 1;
        }
        if (a.back || (activate && !keyFocus && BackButton.contains(cursor))) {
            if (onReturnMain)
                onReturnMain();
            return;
        }
        if (activate) {
            if (keyFocus ? menuIndex == 1 : EditButton.contains(cursor)) {
                if (onOpenEditor)
                    onOpenEditor();
            } else if (PlayButton.contains(cursor) || keyFocus)
                returnToWorlds();
        }
        return;
    }
    if (mode == Mode::Worlds) {
        if(!a.primary)draggingScroll=-2;
        auto dragScroll=[&](){
          if(draggingScroll==-1){const float total=std::max(456.f,float(worlds.size())*220-4),thumb=std::max(8.f,456*456/total);scroll=std::clamp((float(at.y)-48-thumb*.5f)/(456-thumb),0.f,1.f)*(total-456);}
          else if(draggingScroll>=0&&size_t(draggingScroll)<worlds.size()){auto&w=worlds[size_t(draggingScroll)];const float total=std::max(456.f,w.levelWidth),thumb=std::max(8.f,456*456/total);w.levelScroll=std::clamp((float(at.x)-280-thumb*.5f)/(456-thumb),0.f,1.f)*(total-456);}
        };
        if(draggingScroll!=-2){dragScroll();return;}
        if(activate&&!keyFocus&&worlds.size()>2&&Rect{744,48,8,456}.contains(at)){draggingScroll=-1;dragScroll();return;}
        if (a.back || (activate && !keyFocus && BackButton.contains(cursor))) {
            mode = Mode::Menu;
            return;
        }
        if (worlds.empty())
            return;
        if (up && worldIndex > 0)
            --worldIndex;
        if (down && worldIndex + 1 < worlds.size())
            ++worldIndex;
        if (up || down) {
            levelIndex = 0;
            scroll = std::max(0.f, float(worldIndex) * 220 - 220);
        }
        if (left && levelIndex > 0)
            --levelIndex;
        if (right && levelIndex + 1 < worlds[worldIndex].levels.size())
            ++levelIndex;
        if(left||right)revealLevel();
        bool localScroll = false;
        if (a.wheel) {
            for (size_t i = 0; i < worlds.size(); ++i) {
                float y = 48 + float(i) * 220 - scroll;
                if (Rect{339, y + 68, 378, 64}.contains(at)) {
                    auto &w = worlds[i];
                    w.descriptionScroll = std::clamp(w.descriptionScroll - a.wheel * 15, 0.f,
                                                     std::max(0.f, w.descriptionHeight - 64));
                    localScroll = true;
                    break;
                }
                if (Rect{280, y + 144, 456, 40}.contains(at)) {
                    worldIndex = i;
                    int64_t nextLevel = int64_t(levelIndex) - a.wheel;
                    levelIndex = worlds[i].levels.empty()
                                     ? 0
                                     : size_t(std::clamp<int64_t>(
                                           nextLevel, 0, int64_t(worlds[i].levels.size()) - 1));
                    revealLevel();
                    localScroll = true;
                    break;
                }
            }
        }
        if (!localScroll)
            scroll = std::clamp(scroll - float(a.wheel) * 32, 0.f,
                                std::max(0.f, float(worlds.size()) * 220 - 460));
        if (activate) {
            if (keyFocus) {
                if (!worlds[worldIndex].levels.empty()) start(worldIndex, levelIndex);
                return;
            }
            for (size_t i = 0; i < worlds.size(); ++i) {
                float y = 48 + float(i) * 220 - scroll;
                if (at.x >= 272 && at.x < 744 && at.y >= y + 4 && at.y < y + 212) {
                    worldIndex = i;
                    auto&w=worlds[i];
                    if(w.levelWidth>456&&Rect{280,y+204,456,8}.contains(at)){draggingScroll=int(i);dragScroll();return;}
                    if(Rect{280,y+144,456,32}.contains(at)){
                      float x=280-w.levelScroll;
                      for(size_t l=0;l<w.metadata.size();++l){const auto width=w.metadata[l].buttonWidth;if(Rect{x,y+144,width,32}.contains(at)){start(i,l);return;}x+=width+4;}
                    } else levelIndex=0;
                    return;
                }
            }

        }
        return;
    }
    if (mode == Mode::LevelResults) {
        if (a.back) {
            returnToWorlds();
            return;
        }
        if (a.secondaryDown || a.undo) {
            retry();
            return;
        }
        if (activate) {
            if (Rect{424, 400, 144, 40}.contains(at))
                retry();
            else if (Rect{584, 400, 144, 40}.contains(at))
                notifySkipped();
            else
                next();
        }
        return;
    }
    if (mode == Mode::Results) {
        float y = resultPose(animation,2)-24;
        if (y < 512 && y + 64 > 0 && (a.back || (activate && Rect{264, y, 120, 64}.contains(at))))
            returnToWorlds();
    }
    (void)held;
}
} // namespace EditorBrowser
#include "editor_browser_draw.hpp"
