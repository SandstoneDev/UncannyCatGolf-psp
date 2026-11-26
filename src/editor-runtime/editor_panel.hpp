#pragma once
#include "editor_toolbar_plate066.hpp"
#include "editor_list_layout067.hpp"
#include "property_layout067.hpp"
#include "editor_readable_labels067.hpp"
#include "editor_color_picker_layout.hpp"
#include "editor_colors.hpp"
#include "editor_controller.hpp"
#include "editor_files.hpp"
#include "editor_panel_color.hpp"
#include "editor_picker_cells.hpp"
#include "editor_modifier_help067.hpp"
#include "editor_hotkey_help068.hpp"
#include "editor_tooltips068.hpp"
#include <map>
namespace EditorPanel {
using Controller = EditorController::Controller;
using Value = ucg::editor::Value;
using Point = ucg::editor::Vec2;
using Rect = EditorArt::Rect;
using Color = ucg::editor::Color;
inline void box(float x, float y, float w, float h, uint32_t color) {
  rectangle(ViewX + x * EditorArt::Scale, y * EditorArt::Scale,
            w * EditorArt::Scale, h * EditorArt::Scale, color);
}
inline bool inside(Point p, float x, float y, float w, float h) {
  return Rect{x, y, w, h}.contains(float(p.x), float(p.y));
}
inline int control(const char *path) {
  for (unsigned i = 0; i < std::size(EditorArt::Packed::Controls); ++i)
    if (!std::strcmp(path, EditorArt::Packed::Controls[i].path))
      return int(i);
  return -1;
}
inline std::string decimal(double value) {
  char b[64];
  std::snprintf(b, sizeof b, "%.6g", value);
  for (char *p = b; *p; ++p)
    if (*p == ',')
      *p = '.';
  return b;
}
inline std::string tail(const std::string &p) {
  auto at = p.find_last_of('/');
  return p.substr(at == std::string::npos ? 0 : at + 1);
}
struct Field {
  const char *path;
  const char *key;
};
inline constexpr Field Fields[] = {{"levelName", "levelName"},
                                   {"levelAuthor", "levelAuthor"},
                                   {"world", "world"},
                                   {"id", "id"},
                                   {"rankPeak", "rank1"},
                                   {"rankSwag", "rank2"},
                                   {"rankOk", "rank3"},
                                   {"rankErm", "rank4"},
                                   {"levelBounds", "levelBounds"},
                                   {"par", "par"},
                                   {"time", "time"},
                                   {"song", "song"},
                                   {"bgSelect", "bg"},
                                   {"modulate", "light"},
                                   {"bright", "tBright"},
                                   {"normal", "tNormal"},
                                   {"dark", "tDark"},
                                   {"extra", "tExtra"},
                                   {"songName", "customSongName"},
                                   {"tiles", "customtiles"},
                                   {"tiles2", "switch1customtiles"},
                                   {"tiles3", "switch2customtiles"},
                                   {"tiles4", "switch3customtiles"},
                                   {"tiles5", "switch4customtiles"},
                                   {"tiles6", "dogcustomtiles"},
                                   {"modifiers", "mods"},
                                   {"customFloor", "customfloor"},
                                   {"bpm", "bpm"},
                                   {"loopoffset", "loopPoint"}};
inline const char *fieldKey(const std::string &name) {
  for (const auto &f : Fields)
    if (name == f.path)
      return f.key;
  return nullptr;
}
class Panel {
  friend struct PanelTestAccess;
  Controller &c;
  EditorKeyboard::Keyboard keyboard;
  enum class Input {
    None,
    Metadata,
    Variable,
    Deco,
    Filename,
    Folder,
    ColorHex,
    ColorChannel
  };
  Input target = Input::None;
  std::string targetKey;
  bool numeric = false;
  double inputMin = 0, inputMax = 0;
  bool bounded = false;
  int popup = -1;
  std::vector<std::string> options;
  float popupX = 0, popupY = 0, popupW = 224;
  float popupScroll = 0;
  unsigned popupSelected = 0;
  std::vector<EditorMenuArt::Item> popupItems, colorMenuItems;
  EditorMenuArt::Layout popupGeometry, colorMenuGeometry;
  float colorMenuX = 0, colorMenuY = 0;
  bool colorOpen = false, colorSampling = false;
  int colorMenu = -1;
  std::map<std::string, EditorPanelColor::Context> colorContexts;
  EditorPanelColor::Context *colorUi = nullptr;
  std::vector<Color> presetCache;
  std::string colorKey;
  bool colorSelection = false;
  Color color{}, oldColor{};
  int presetDrag = -1;
  bool presetDragging = false;
  Point presetDragOrigin{};
  float colorX = 256, colorY = 120;
  int spinChannel = -1, spinDirection = 0;
  uint32_t spinNext = 0;
  enum class FilePurpose { Level, Palette };
  FilePurpose filePurpose = FilePurpose::Level;
  bool files = false, fileSave = false, overwrite = false;
  std::string fileRoot, fileRelative, fileName;
  std::vector<EditorFiles::Entry> fileEntries;
  unsigned fileSelected = 0, fileScroll = 0, fileFocusGeneration = 0;
  bool fileFocusPrepared = false;
  unsigned objectScroll = 0, decoScroll = 0, floorScroll = 0, modifierHelpScroll = 0;
  unsigned hotkeyHelpScroll=0;
  unsigned propertyScroll = 0;int propertyObject = -1;size_t propertySelection = ~size_t(0);
  unsigned clearPresses = 0, clearTimer = 0, toastTimer = 0;
  uint32_t toastUntil068=0,tooltipSince068=0;int tooltipControl068=-1;
  bool welcome = true, exitQuestion = false;
  std::string message;
  Point cursor{};
  bool heldPrimary = false;
  int activeControl = -1;
  bool inputWasActive = false;
  std::function<void()> afterKeyboard;

public:
#ifdef UCG_EDITOR_CAPTURE
  std::array<uint32_t,6> captureFileState()const{
    auto hash=[](const std::string&text){uint32_t value=2166136261u;for(unsigned char c:text)value=(value^c)*16777619u;return value;};
    return {uint32_t(files),fileSelected,fileScroll,uint32_t(fileEntries.size()),hash(fileRelative),fileSelected<fileEntries.size()?hash(fileEntries[fileSelected].name):0};
  }
#endif
  std::function<bool(std::string &)> onPlay;
  std::function<void()> onExit, onReload, onMusic, onKeyboardClosed,
      onBeforeLoad;
  std::function<Color(Point)> onSampleColor;
  std::function<void(unsigned, float, float, float, float)> thumbnail;
  explicit Panel(Controller &controller) : c(controller) {}
  bool keyboardActive() const { return keyboard.active(); }
  bool modal() const {
    return files || popup >= 0 || colorOpen || welcome || exitQuestion ||
           keyboard.active() || overwrite;
  }
  bool isWelcome() const { return welcome; }
  void showWelcome(bool show = true) {
    welcome = show;
    if (show) {
      // A new PC Editor scene has new ColorPickerButton instances.
      colorUi = nullptr;
      colorContexts.clear();
      popup = -1;
      options.clear();
      colorOpen = colorSampling = false;
      colorMenu = -1;
      files = overwrite = exitQuestion = false;
      objectScroll = decoScroll = floorScroll = 0;
      clearPresses = clearTimer = 0;
      activeControl = -1;
    }
  }
  void say(const std::string &s) {
    message = s;
    toastTimer = 1;
    const float duration=4000.f+std::max(0.f,EditorArt::textWidth(3,s.c_str())*2.25f-488)/.06f;
    toastUntil068=sceKernelGetSystemTimeLow()+uint32_t(std::min(duration,600000.f)*1000);
    event("message", unsigned(s.size()));
  }
  void drawKeyboard() {
    const bool before = keyboard.active();
    keyboard.draw();
    if (before && !keyboard.active()) {
      finishInput();
      if (onKeyboardClosed)
        onKeyboardClosed();
    }
  }
  void refresh() {
    if (onReload)
      onReload();
  }
  void openFiles(bool saving) {
    filePurpose = FilePurpose::Level;
    files = true;
    fileSave = saving;
    fileRoot = companion(executable.c_str(), "custom_levels");
    fileRelative.clear();
    fileName = EditorFiles::name(c.path);
    if (!c.path.empty()) {
      auto folder = EditorFiles::base(c.path);
      if (folder.compare(0, fileRoot.size(), fileRoot) == 0 &&
          folder.size() > fileRoot.size())
        fileRelative = folder.substr(fileRoot.size() + 1);
    }
    fileSelected = fileScroll = 0;
    reloadFiles();
  }
  bool blocksWorld(Point p) const { return modal() || c.tab==4 || (c.tab==5&&EditorModifierHelp067::contains(float(p.x),float(p.y))) || (c.tab==3&&p.x>=256&&p.y>=376&&p.y<476) || p.x < 256 || EditorToolbarLayout066::blocksWorld(float(p.x),float(p.y)); }
  void update(const EditorInput::Actions &a, Point p, unsigned pressed,
              unsigned held) {
    cursor = p;
    heldPrimary = a.primary;
    if(toastTimer&&int32_t(sceKernelGetSystemTimeLow()-toastUntil068)>=0)toastTimer=0;
    if (clearTimer && !--clearTimer)
      clearPresses = 0;
    if (keyboard.active())
      return;
    if (files && !overwrite) {
      updateFiles(a, pressed);
      return;
    }
    if (overwrite) {
      if (a.back) {
        overwrite = false;
        return;
      }
      if (a.primaryDown) {
        if (inside(p, 320, 256, 104, 32)) {
          overwrite = false;
          saveChosen();
        } else if (inside(p, 448, 256, 104, 32))
          overwrite = false;
      }
      return;
    }
    if (exitQuestion) {
      if (a.back)
        exitQuestion = false;
      if (a.primaryDown) {
        if (inside(p, 320, 256, 104, 32)) {
          exitQuestion = false;
          if (onExit)
            onExit();
        } else if (inside(p, 448, 256, 104, 32))
          exitQuestion = false;
      }
      return;
    }
    if (colorOpen) {
      updateColor(a);
      return;
    }
    if (popup >= 0) {
      updatePopup(a);
      return;
    }
    if (a.back && c.model.selected()) {
      std::string e;
      if (!c.finishDrag(e))
        say(e);
      c.model.selection_valid = false;
      c.point = -1;
      return;
    }
    if (a.back || (a.pause && !c.model.selected())) {
      exitQuestion = true;
      return;
    }
    if (welcome) {
      if (a.primaryDown) {
        if (EditorArt::controlRect(unsigned(control("pickOption/create"))).contains(float(p.x),float(p.y)))
          openFiles(true);
        else if (EditorArt::controlRect(unsigned(control("pickOption/edit"))).contains(float(p.x),float(p.y)))
          openFiles(false);
      }
      return;
    }
    if ((a.toggleMode || a.undo || a.redo || a.remove) && c.dragging) {
      std::string e;
      if (!c.finishDrag(e))
        say(e);
    }
    if (a.toggleMode)
      c.selectMode = !c.selectMode;
    if (!c.selectMode)
      c.model.selection_valid = false;
    if (a.copy) {
      std::string e;
      if (!c.model.copy_selected(e))
        say(e.empty() ? lang("Nothing selected", "Ничего не выбрано") : e);
    }
    if (a.paste) {
      std::string e;
      if (!c.model.paste(c.world(p), e))
        say(e);
    }
    if (a.undo) {
      std::string e;
      if (!c.model.undo(e))
        say(e);
      else
        refresh();
    }
    if (a.redo) {
      std::string e;
      if (!c.model.redo(e))
        say(e);
      else
        refresh();
    }
    if (a.remove && c.model.selected()) {
      std::string e;
      if (!c.model.erase_selected(e))
        say(e);
      c.point = -1;
    }
    if(a.wheel&&c.tab==4&&EditorHotkeyHelp068::contains(float(p.x),float(p.y))){hotkeyHelpScroll=EditorHotkeyHelp068::wheel(hotkeyHelpScroll,a.wheel);return;}
    if(a.wheel&&c.tab==5&&EditorModifierHelp067::contains(float(p.x),float(p.y))){modifierHelpScroll=EditorModifierHelp067::wheel(modifierHelpScroll,a.wheel,float(p.x),float(p.y));return;}
    if (a.wheel && p.x < 256) {
      if((c.tab==0&&!EditorArt::controlRect(13).contains(float(p.x),float(p.y)))||(c.tab==3&&!EditorArt::controlRect(25).contains(float(p.x),float(p.y)))||c.tab==5){EditorTextLayout068::wheel(c.tab,a.wheel);return;}
      const auto*properties=c.tab==2?syncPropertyLayout():nullptr;
      if(properties&&EditorPropertyLayout067::Area.contains(float(p.x),float(p.y)))
        propertyScroll=EditorPropertyLayout067::shift(propertyScroll,unsigned(properties->variable_count),a.wheel);
      else scrollList(a.wheel);
      return;
    }
    if (c.tab == 1 && inside(p, 16, 144, 224, 208) && a.primary) {
      for (const auto &cell : EditorArt::Packed::PickerCells)
        if (inside(p, 19 + cell.x * 16, 40 + cell.y * 16, 16, 16)) {
          c.atlasX = cell.ax;
          c.atlasY = cell.ay;
          break;
        }
      return;
    }
    if (updateProperties(a))
      return;
    for (unsigned i = 0; i < std::size(EditorArt::Packed::Controls); ++i) {
      if (!visible(i))
        continue;
      if(EditorTextLayout068::sidebar(i)&&EditorTextLayout068::scrolling(c.tab)&&(p.y<EditorTextLayout068::Top||p.y>=EditorTextLayout068::Bottom))continue;
      const auto &w = EditorArt::Packed::Controls[i];
      if (!EditorArt::controlRect(i).contains(float(p.x), float(p.y)))
        continue;
      activeControl = int(i);
      if (a.primaryDown) {
        audio.playTuned(static_cast<Sound>(332), -2, .8);
        activate(i, a);
      } else if ((pressed & (EditorInput::Left | EditorInput::Right)) &&
                 !std::strcmp(w.type, "SpinBox"))
        changeNumber(i, pressed & EditorInput::Right ? 1 : -1);
      return;
    }
    activeControl = -1;
    (void)held;
  }
  void draw() {
    EditorTextFlow068::tick=sceKernelGetSystemTimeLow()/1000;
    const unsigned background=c.tab<6?EditorArt::uiBackground(c.tab,russian()):~0u;
    const bool capacitySplit=drawOpen&&sceGuCheckList()>int(sizeof(displayList)/2);
    if(capacitySplit)::editorPerf067.forcedSplit();
    if(capacitySplit||!EditorArt::frameResident(background))flushBatch();
    if (!EditorArt::prepareUi(c.tab, russian())) {
      if(!drawOpen)beginBatch();
      say("Editor UI asset error");
      return;
    }
    if(!drawOpen)beginBatch();
    EditorArt::readableLabelHook=EditorReadableLabels067::draw;EditorReadableLabels067::selection=c.selectMode;
    EditorArt::drawUi(c.tab, russian());
    if(!EditorToolbarPlate066::draw())say("Editor toolbar asset error");
    for (unsigned i = 0; i < std::size(EditorArt::Packed::Controls); ++i) {
      if (!visible(i))
        continue;
      const auto &w = EditorArt::Packed::Controls[i];
      const auto name = tail(w.path);
      std::string value = valueFor(i);
      bool checked = checkedFor(i);
      const char *key = fieldKey(name);
      const auto *field = key ? c.model.document.root.find(key) : nullptr;
      const bool placeholder = !std::strcmp(w.type, "LineEdit") && key &&
                               (!field || (field->kind == Value::Kind::String &&
                                           field->text.empty()));
      EditorArt::drawControl(
          i, value.c_str(), checked,
          EditorArt::controlRect(i).contains(float(cursor.x), float(cursor.y)),
          heldPrimary && activeControl == int(i), russian(), placeholder);
      if (!std::strcmp(w.type, "ColorPickerButton")) {
        Color col = readColor(name);
        const auto r=EditorArt::controlRect(i);const auto clip=EditorArt::uiScissor;
        if(EditorTextLayout068::scrolling(c.tab))EditorArt::scissor(clip[0],std::max(clip[1],int(EditorTextLayout068::Top*EditorArt::Scale)),clip[2],std::min(clip[3],int(EditorTextLayout068::Bottom*EditorArt::Scale)));
        box(r.x + 2, r.y + 2, r.w - 4, r.h - 4, EditorColors::rgba(col));
        EditorArt::scissor(clip[0],clip[1],clip[2],clip[3]);
      }
    }
    drawLists();
    drawProperties();
    if (c.tab == 4) {
      namespace Help=EditorHotkeyHelp068;
      const bool outline=EditorTextFlow068::outline;EditorTextFlow068::outline=true;
      popupPanel(8,96,752,368);
      for(unsigned row=0;row<Help::Rows&&hotkeyHelpScroll+row<Help::Count;++row)
        EditorArt::textScaled(3,Help::Lines[hotkeyHelpScroll+row],Help::X,Help::baseline(row),White,Help::Width,Help::TextScale);
      EditorTextFlow068::outline=outline;
    }
    if(c.tab==5){
      namespace Help=EditorModifierHelp067;
      const bool outline=EditorTextFlow068::outline;EditorTextFlow068::outline=true;
      const auto clip=EditorArt::uiScissor;
      EditorArt::scissor(std::max(clip[0],ViewX+int(std::ceil(Help::X*EditorArt::Scale))),std::max(clip[1],int(std::ceil(Help::Y*EditorArt::Scale))),std::min(clip[2],ViewX+int(std::floor((Help::X+Help::Width)*EditorArt::Scale))),std::min(clip[3],int(std::floor((Help::Y+Help::Height)*EditorArt::Scale))));
      for(unsigned row=0;row<Help::Rows&&modifierHelpScroll+row<Help::Count;++row){
        const auto line=Help::line(modifierHelpScroll+row);char text[256];
        if(line.length<sizeof text){std::memcpy(text,line.first,line.length);text[line.length]=0;EditorArt::textScaled(3,text,Help::X,Help::baseline(row),White,Help::Width,Help::TextScale);}
      }
      EditorArt::scissor(clip[0],clip[1],clip[2],clip[3]);EditorTextFlow068::outline=outline;
    }
    if(!modal())drawTooltip();
    if (welcome) {
      box(256, 40, 512, 472, 0x90000000);
      for (const auto &l : EditorArt::Packed::Labels)
        if (l.welcome)
          EditorArt::frame(l.frames[unsigned(russian())], l.x, l.y);
      drawNamed("pickOption/create");
      drawNamed("pickOption/edit");
    }
    if (popup >= 0)
      drawPopup();
    if (colorOpen)
      drawColor();
    if (files)
      drawFiles();
    if (exitQuestion || overwrite) {
      const bool outline=EditorTextFlow068::outline;EditorTextFlow068::outline=true;
      popupPanel(288, 184, 304, 120);
      const auto question=EditorTooltips068::lines(overwrite
          ?lang("Replace existing file?","Заменить существующий файл?")
          :lang("Exit level editor?","Выйти из редактора?"),272,2.f);
      for(unsigned row=0;row<question.size();++row)EditorArt::textScaled(3,question[row].c_str(),304,216+row*32,White,272,2.f);
      button(320, 256, 104, 32, lang("Yes", "Да"));
      button(448, 256, 104, 32, lang("Cancel", "Отмена"));
      EditorTextFlow068::outline=outline;
    }
    if (toastTimer && !message.empty()) {
      box(260, 430, 504, 48, 0xff000000);
      EditorArt::textScaled(3,message.c_str(),268,466,White,488,2.25f);
    }
  }

private:
  bool visible(unsigned i) const {
    const auto &w = EditorArt::Packed::Controls[i];
    std::string path = w.path;
    if (path.rfind("pickOption/", 0) == 0)
      return false;
    if (path.rfind("HUD/Tools/", 0) == 0)
      return c.tools;
    if (w.tab >= 0 && w.tab != int(c.tab))
      return false;
    if (path.find("example") != std::string::npos)
      return false;
    return true;
  }
  std::string valueFor(unsigned i) const {
    const auto &w = EditorArt::Packed::Controls[i];
    auto n = tail(w.path);
    if (n == "tabselector") {
      const char *en[] = {"Level Info", "Tiles",   "Objects",
                          "Decoration", "Hotkeys", "Custom"};
      const char *ru[] = {"Информация", "Тайлы",   "Объекты",
                          "Декорации",  "Клавиши", "Свои ресурсы"};
      return russian() ? ru[c.tab] : en[c.tab];
    }
    if (n == "layer") {
      const char *en[] = {"Normal",  "Switch1", "Switch2",
                          "Switch3", "Switch4", "Dog Tiles"};
      return en[c.layer];
    }
    if (n == "clear")
      return clearPresses ? "CONFIRM " + std::to_string(clearPresses) + "/3"
                          : lang("CLEAR", "ОЧИСТИТЬ");
    if (n == "save")
      return lang("SAVE", "СОХР.");
    if (n == "load")
      return lang("LOAD", "ОТКР.");
    if (n == "playtest")
      return lang("PLAY", "ТЕСТ");
    if (n == "tools")
      return lang("TOOLS", "ИНСТР.");
    if (n == "reload")
      return lang("Reload Custom", "Обновить ресурсы");
    if (n == "collision")
      return lang("Collisions", "Коллизии");
    if (n == "gridx")
      return "X " + decimal(c.gridX);
    if (n == "gridy")
      return "Y " + decimal(c.gridY);
    if (n == "vFrames")
      return decimal(c.decoV) + " Vframes";
    if (n == "hFrames")
      return decimal(c.decoH) + " Hframes";
    if (n == "Z Index")
      return decimal(c.decoZ) + " Z Index";
    if (n == "speed")
      return decimal(c.decoSpeed) + " Speed";
    if (n == "frame")
      return decimal(c.decoFrame) + " Frame";
    if (const char *k = fieldKey(n)) {
      const auto &v = c.get(k);
      if (n == "song") {
        const char *names[] = {"Tutorial", "Golf Central", "Chuckle Park",
                               "Final Frontier", "Elysian Fields"};
        int song = int(v.numeric());
        return song >= 0 && song < 5 ? names[song] : "Custom";
      }
      if (v.kind == Value::Kind::String)
        return v.text.empty() && !std::strcmp(w.type, "LineEdit") ? w.text
                                                                  : v.text;
      if (v.kind == Value::Kind::Number)
        return decimal(v.numeric());
    }
    return w.text;
  }
  bool checkedFor(unsigned i) const {
    auto n = tail(EditorArt::Packed::Controls[i].path);
    if (n == "collision")
      return c.decoCollision;
    if (n == "PlacementMode")
      return !c.selectMode;
    if (n == "SelectMode")
      return c.selectMode;
    if (n == "MusicToggle")
      return c.music;
    if (n == "grid")
      return c.grid;
    return false;
  }
  Color readColor(const std::string &name) const {
    Color result;
    if (name == "exampleColor") {
      if (auto *n = c.model.selected())
        if (!c.model.selection_deco)
          ucg::editor::read_color(n->array[2], result);
    } else if (const char *k = fieldKey(name))
      ucg::editor::read_color(c.get(k), result);
    return result;
  }
  void mutateNumber(const std::string &name, double n) {
    std::string e;
    if (name == "gridx")
      c.gridX = n;
    else if (name == "gridy")
      c.gridY = n;
    else if (name == "vFrames")
      c.decoV = int(n);
    else if (name == "hFrames")
      c.decoH = int(n);
    else if (name == "Z Index")
      c.decoZ = int(n);
    else if (name == "speed")
      c.decoSpeed = n;
    else if (name == "frame")
      c.decoFrame = int(n);
    else if (const char *k = fieldKey(name)) {
      if (!c.set(k, Value::number(n), e))
        say(e);
      refresh();
    }
  }
  double currentNumber(const std::string &n) const {
    if (n == "gridx")
      return c.gridX;
    if (n == "gridy")
      return c.gridY;
    if (n == "vFrames")
      return c.decoV;
    if (n == "hFrames")
      return c.decoH;
    if (n == "Z Index")
      return c.decoZ;
    if (n == "speed")
      return c.decoSpeed;
    if (n == "frame")
      return c.decoFrame;
    if (const char *k = fieldKey(n))
      return c.number(k);
    return 0;
  }
  void changeNumber(unsigned i, int direction) {
    const auto &w = EditorArt::Packed::Controls[i];
    auto name = tail(w.path);
    mutateNumber(name, std::clamp(currentNumber(name) + direction * w.step,
                                  double(w.minimum), double(w.maximum)));
  }
  void beginInput(Input kind, const std::string &key, const std::string &value,
                  const char *prompt, bool isNumber = false, double minimum = 0,
                  double maximum = 0, bool limit = false) {
    target = kind;
    targetKey = key;
    numeric = isNumber;
    inputMin = minimum;
    inputMax = maximum;
    bounded = limit;
    inputWasActive = keyboard.begin(value, prompt, russian());
    if (!inputWasActive)
      say("Cannot open PSP keyboard");
  }
  void finishInput() {
    if (!inputWasActive)
      return;
    inputWasActive = false;
    if (!keyboard.accepted()) {
      target = Input::None;
      return;
    }
    const std::string text = keyboard.result();
    std::string e;
    Value v(text);
    if (numeric) {
      Value parsed;
      if (!ucg::editor::parse_json(text, parsed, e) ||
          parsed.kind != Value::Kind::Number) {
        say(lang("Enter a number", "Введите число"));
        target = Input::None;
        return;
      }
      double n = parsed.numeric();
      if (bounded)
        n = std::clamp(n, inputMin, inputMax);
      v = Value::number(n);
    }
    switch (target) {
    case Input::Metadata:
      if (!c.set(targetKey.c_str(), v, e))
        say(e);
      refresh();
      break;
    case Input::Variable:
      if (!c.setVariable(targetKey, v, e))
        say(e);
      break;
    case Input::Deco:
      mutateNumber(targetKey, v.numeric());
      break;
    case Input::Filename:
      fileName = text;
      break;
    case Input::Folder: {
      std::string path;
      if (!EditorFiles::join(fileRoot, fileRelative + "/" + text, path) ||
          !EditorFiles::make(path))
        say(lang("Cannot create folder", "Не удалось создать папку"));
      else
        reloadFiles();
      break;
    }
    case Input::ColorHex:
      if (colorUi && colorUi->editHex(text)) {
        color = colorUi->picker.value;
        applyColor();
      }
      break;
    case Input::ColorChannel:
      if (colorUi && colorUi->editChannel(
                         unsigned(std::atoi(targetKey.c_str())), v.numeric())) {
        color = colorUi->picker.value;
        applyColor();
      }
      break;
    default:
      break;
    }
    target = Input::None;
  }
  void activate(unsigned i, const EditorInput::Actions &a) {
    const auto &w = EditorArt::Packed::Controls[i];
    auto n = tail(w.path);
    std::string e;
    if (n == "tabselector") {
      options = {
          lang("Level Info", "Информация"), lang("Tiles", "Тайлы"),
          lang("Objects", "Объекты"),       lang("Decoration", "Декорации"),
          lang("Hotkeys", "Клавиши"),       lang("Custom", "Свои ресурсы")};
      openPopup(i);
    } else if (n == "song") {
      options = {"Tutorial",       "Golf Central",
                 "Chuckle Park",   "Final Frontier",
                 "Elysian Fields", lang("Custom", "Своя музыка")};
      openPopup(i);
    } else if (n == "layer") {
      options = {"Normal",  "Switch1", "Switch2",
                 "Switch3", "Switch4", "Dog Tiles"};
      openPopup(i);
    } else if (!std::strcmp(w.type, "LineEdit")) {
      if (const char *k = fieldKey(n))
        beginInput(Input::Metadata, k, c.get(k).text,
                   lang("Edit text", "Изменить текст"));
    } else if (!std::strcmp(w.type, "SpinBox")) {
      const auto r=EditorArt::controlRect(i);
      if (cursor.x > r.x + r.w - 18) {
        changeNumber(i, cursor.y < r.y + r.h / 2 ? 1 : -1);
        return;
      }
      const char *k = fieldKey(n);
      beginInput(k ? Input::Metadata : Input::Deco, k ? k : n,
                 decimal(currentNumber(n)), lang("Value", "Значение"), true,
                 w.minimum, w.maximum, true);
    } else if (!std::strcmp(w.type, "ColorPickerButton")) {
      colorKey = fieldKey(n) ? fieldKey(n) : "";
      colorSelection = false;
      color = readColor(n);
      const auto r=EditorArt::controlRect(i);openColor(w.path, color, r.x, r.y + r.h);
    } else if (n == "collision")
      c.decoCollision = !c.decoCollision;
    else if (n == "ItemList") {
      if (c.tab == 2) {
        int row = EditorListLayout067::hitRow(float(cursor.y-w.y-4),EditorListLayout067::ObjectRow,EditorListLayout067::ObjectRows);
        unsigned index = objectScroll + unsigned(std::max(0, row));
        if (row>=0 && index < 26)
          c.object = index;
      } else if (c.tab == 3) {
        int row = EditorListLayout067::hitRow(float(cursor.y-EditorTextLayout068::y(3,108)),EditorListLayout067::DecoRow,EditorListLayout067::DecoRows);
        unsigned index = decoScroll + unsigned(std::max(0,row));
        if (row>=0 && index < 66 + c.customDeco.size()) {
          c.deco = index;
          c.decoScaleX = c.decoScaleY = 1;
        }
      }
    } else if (n == "bgSelect") {
      int row = int((cursor.y - EditorTextLayout068::y(0,EditorTextLayout068::FloorTop)) / (EditorTextLayout068::FloorPitch*EditorTextLayout068::Stretch));
      unsigned index = floorScroll + unsigned(std::max(0, row));
      if (index < std::size(EditorArt::Packed::Floors)) {
        if (!c.set("bg", Value::number(index), e))
          say(e);
        refresh();
      }
    } else if (n == "save") {
      if (c.path.empty())
        openFiles(true);
      else if (!c.save(e))
        say(e);
      else
        say(lang("Saved!", "Сохранено!"));
    } else if (n == "load")
      openFiles(false);
    else if (n == "playtest") {
      if (c.path.empty()) {
        say(lang("Save the level before playtest",
                 "Сначала сохраните уровень"));
        openFiles(true);
      } else if (!c.save(e))
        say(e);
      else if (onPlay && !onPlay(e))
        say(e);
    } else if (n == "clear") {
      ++clearPresses;
      clearTimer = 120;
      if (clearPresses == 4) {
        if (!c.clearLikePc(e))
          say(e);
        else {
          welcome = true;
          refresh();
        }
        clearPresses = 0;
      }
    } else if (n == "tools")
      c.tools = !c.tools;
    else if (n == "PlacementMode")
      c.selectMode = false;
    else if (n == "SelectMode")
      c.selectMode = true;
    else if (n == "grid")
      c.grid = !c.grid;
    else if (n == "switchToggle")
      c.switchPreview = (c.switchPreview + 1) % 5;
    else if (n == "MusicToggle") {
      c.music = !c.music;
      if (onMusic)
        onMusic();
    } else if (n == "reload")
      refresh();
    else if (n == "shiftRight" || n == "shiftLeft" || n == "shiftUp" ||
             n == "ShiftDown") {
      int x = n == "shiftRight"  ? 1
              : n == "shiftLeft" ? -1
                                 : 0,
          y = n == "ShiftDown" ? 1
              : n == "shiftUp" ? -1
                               : 0;
      if (!c.model.mutate(
              [&](ucg::editor::Document &d) { d.shift_tiles(x, y, e); }, e))
        say(e);
    }
    (void)a;
  }
  void rebuildPopup() {
    popupItems.clear();
    std::string name =
        popup >= 0 && size_t(popup) < std::size(EditorArt::Packed::Controls)
            ? tail(EditorArt::Packed::Controls[popup].path)
            : "";
    for (unsigned i = 0; i < options.size(); ++i) {
      EditorMenuArt::Item item;
      item.text = options[i];
      item.id = int(i);
      item.radio = true;
      item.checked = i == popupSelected;
      if (name == "song")
        item.icon = EditorMenuArt::Icon::Song;
      popupItems.push_back(std::move(item));
    }
    popupGeometry =
        EditorMenuArt::layout(popupItems, popupW, 760, 504, popupScroll);
    popupScroll = popupGeometry.scroll;
    popupX = std::clamp(popupX, 4.f, std::max(4.f, 764 - popupGeometry.w));
    popupY = std::clamp(popupY, 4.f, std::max(4.f, 508 - popupGeometry.h));
  }
  void openPopup(unsigned i) {
    popup = int(i);
    const auto &w = EditorArt::Packed::Controls[i];
    const auto name = tail(w.path);
    const auto r=EditorArt::controlRect(i);popupX = r.x;
    popupY = r.y + r.h;
    popupW = r.w;
    popupScroll = 0;
    popupSelected = name == "tabselector" ? c.tab
                    : name == "layer"     ? c.layer
                    : name == "song" ? unsigned(std::max(0., c.number("song")))
                                     : 0;
    rebuildPopup();
  }
  void updatePopup(const EditorInput::Actions &a) {
    if (a.back) {
      popup = -1;
      enumVariable.clear();
      return;
    }
    if (a.wheel) {
      popupScroll = std::clamp(popupScroll - a.wheel * 20.f, 0.f,
                               popupGeometry.scrollMax);
      rebuildPopup();
    }
    if (!a.primaryDown)
      return;
    const int row = EditorMenuArt::hit(popupGeometry, popupItems,
                                       cursor.x - popupX, cursor.y - popupY);
    if (row >= 0) {
      const unsigned index = unsigned(popupItems[row].id);
      const auto name =
          popup >= 0 && size_t(popup) < std::size(EditorArt::Packed::Controls)
              ? tail(EditorArt::Packed::Controls[popup].path)
              : "";
      std::string e;
      if (name == "tabselector")
        c.tab = index;
      else if (name == "layer")
        c.layer = index;
      else if (name == "song")
        c.set("song", Value::number(index), e);
      else if (!enumVariable.empty())
        c.setVariable(enumVariable, Value::number(index), e);
      if (!e.empty())
        say(e);
    }
    popup = -1;
    enumVariable.clear();
  }
  std::string enumVariable;
  void popupPanel(float x, float y, float w, float h) {
    flushBatch();
    bool ok = EditorArt::preparePopupPanel();
    beginBatch();
    if (ok) {
      EditorArt::frame(EditorArt::Packed::PopupPanel, x, y, w / 480, h / 360);
    } else
      box(x, y, w, h, 0xff000000);
  }
  void button(float x, float y, float w, float h, const char *label) {
    if(inside(cursor,x,y,w,h)){box(x,y,1,h,White);box(x+w-1,y,1,h,White);}
    EditorArt::textScaled(3,label,x+4,y+h/2+8,White,w-8,20.f/12);
  }
  void drawTooltip(){
    int hovered=-1;for(unsigned i=0;i<std::size(EditorArt::Packed::Controls);++i)if(visible(i)&&EditorArt::controlRect(i).contains(float(cursor.x),float(cursor.y))&&EditorTooltips068::text(EditorArt::Packed::Controls[i].path)){hovered=int(i);break;}
    const uint32_t now=sceKernelGetSystemTimeLow();
    if(hovered!=tooltipControl068){tooltipControl068=hovered;tooltipSince068=now;}
    if(hovered<0||uint32_t(now-tooltipSince068)<600000)return;
    const auto lines=EditorTooltips068::lines(EditorTooltips068::text(EditorArt::Packed::Controls[unsigned(hovered)].path));
    popupPanel(272,80,480,float(lines.size()*32+16));
    const bool old=EditorTextFlow068::outline;EditorTextFlow068::outline=true;
    for(unsigned row=0;row<lines.size();++row)EditorArt::textScaled(3,lines[row].c_str(),280,112+row*32,White,464,1.75f);
    EditorTextFlow068::outline=old;
  }
  void drawPopup() {
    EditorMenuArt::drawMenu(popupGeometry, popupItems, popupX, popupY,
                            EditorMenuArt::hit(popupGeometry, popupItems,
                                               cursor.x - popupX,
                                               cursor.y - popupY));
  }
  void drawNamed(const char *name) {
    int id = control(name);
    if (id >= 0) {
      EditorArt::drawControl(
          unsigned(id), nullptr, false,
          EditorArt::controlRect(unsigned(id)).contains(float(cursor.x), float(cursor.y)),
          false, russian());
    }
  }
  void scrollList(int amount) {
    auto shift = [&](unsigned &v, int max) {
      v = unsigned(std::clamp(int(v) - amount, 0, std::max(0, max)));
    };
    if (c.tab == 2)
      shift(objectScroll, int(EditorListLayout067::lastScroll(26,EditorListLayout067::ObjectRows)));
    else if (c.tab == 3)
      shift(decoScroll, int(EditorListLayout067::lastScroll(66+unsigned(c.customDeco.size()),EditorListLayout067::DecoRows)));
    else if (c.tab == 0)
      shift(floorScroll, int(std::size(EditorArt::Packed::Floors)-EditorTextLayout068::FloorRows));
    else if(c.tab==5)
      modifierHelpScroll=EditorModifierHelp067::wheel(modifierHelpScroll,amount,float(cursor.x),float(cursor.y));
  }
  void drawLists() {
    if (c.tab == 0) {
      const auto clip=EditorArt::uiScissor;
      EditorArt::scissor(clip[0],std::max(clip[1],int(EditorTextLayout068::Top*EditorArt::Scale)),clip[2],std::min(clip[3],int(EditorTextLayout068::Bottom*EditorArt::Scale)));
      const char*rankIcons[]={"assetsNEW/graphics/editor/peak.png","assetsNEW/graphics/editor/swag.png","assetsNEW/graphics/editor/ok.png","assetsNEW/graphics/editor/erm.png"};
      if(thumbnail)for(unsigned row=0;row<4;++row)for(unsigned image=0;image<std::size(EditorArt::Packed::Images);++image)if(!std::strcmp(EditorArt::Packed::Images[image].path,rankIcons[row])){const auto r=EditorArt::controlRect(5+row);thumbnail(image,r.x+6,r.y+10,16,16);break;}
      EditorArt::scissor(clip[0],clip[1],clip[2],clip[3]);
      if(EditorTextLayout068::y(0,EditorTextLayout068::FloorTop)>=EditorTextLayout068::Bottom||EditorTextLayout068::y(0,EditorTextLayout068::FloorTop+EditorTextLayout068::FloorRows*EditorTextLayout068::FloorPitch)<=EditorTextLayout068::Top)return;
      EditorArt::scissor(
          ViewX + int(16 * EditorArt::Scale), int(std::max(EditorTextLayout068::Top,EditorTextLayout068::y(0,EditorTextLayout068::FloorTop)) * EditorArt::Scale),
          ViewX + int(240 * EditorArt::Scale), int(std::min(EditorTextLayout068::Bottom,EditorTextLayout068::y(0,EditorTextLayout068::FloorTop+EditorTextLayout068::FloorRows*EditorTextLayout068::FloorPitch)) * EditorArt::Scale));
      for (unsigned n = 0;
           n < EditorTextLayout068::FloorRows && n + floorScroll < std::size(EditorArt::Packed::Floors);
           ++n) {
        unsigned idx = n + floorScroll;
        float x = 20, y = EditorTextLayout068::y(0,EditorTextLayout068::FloorTop + n * EditorTextLayout068::FloorPitch);
        if (unsigned(c.number("bg")) == idx)
          box(x,y,216,EditorTextLayout068::FloorPitch,0x4cffffff);
        if (thumbnail)
          thumbnail(EditorArt::Packed::Floors[idx].image,x,y+8,32,32);
        const auto name=EditorTooltips068::lines(EditorArt::Packed::Floors[idx].name,184,2.25f);
        for(unsigned line=0;line<name.size();++line)EditorArt::textScaled(3,name[line].c_str(),x+36,y+(name.size()>1?24:36)+26*line,White,184,2.25f);
      }
      EditorArt::fullView();
    } else if (c.tab == 2 || c.tab == 3) {
      bool dec = c.tab == 3;
      unsigned first = dec ? decoScroll : objectScroll,
               count = dec ? EditorListLayout067::DecoRows : EditorListLayout067::ObjectRows;
      float x = 28, y = dec ? EditorTextLayout068::y(3,108) : 100;
      EditorArt::scissor(ViewX + int(28 * EditorArt::Scale),
                         int((dec ? std::max(EditorTextLayout068::Top,EditorTextLayout068::y(3,108)) : 100) * EditorArt::Scale),
                         ViewX + int(224 * EditorArt::Scale),
                         int((dec ? std::min(EditorTextLayout068::Bottom,EditorTextLayout068::y(3,356)) : 224) * EditorArt::Scale));
      for (unsigned n = 0; n < count; ++n) {
        unsigned idx = first + n;
        if (idx >= (dec ? 66 + c.customDeco.size() : 26))
          break;
        float itemX = x,
              rowY = y + n*(dec?EditorListLayout067::DecoRow:EditorListLayout067::ObjectRow);
        if ((dec ? c.deco : c.object) == idx)
          box(itemX, rowY, 200, dec?EditorListLayout067::DecoRow:EditorListLayout067::ObjectRow, 0x4cffffff);
        const char *name = nullptr;
        unsigned image = 0;
        if (!dec) {
          name = EditorArt::Packed::Objects[idx].name;
          image = EditorArt::Packed::Objects[idx].image;
        } else if (idx < 66) {
          name = EditorArt::Packed::Deco[idx].name;
          image = EditorArt::Packed::Deco[idx].image;
        } else
          name = c.customDeco[idx - 66].c_str();
        if (thumbnail && (!dec || idx < 66))
          thumbnail(image, itemX, rowY + 2, dec ? 24 : 16, dec ? 24 : 16);
        const float nameWidth=dec?164.f:176.f;
        if(dec&&idx>=66)EditorArt::textScaled(4,name,itemX+32,rowY+38,White,nameWidth,3.25f);
        else{const auto lines=EditorTooltips068::lines(name,nameWidth,3.25f,4);for(unsigned line=0;line<lines.size();++line)EditorArt::textScaled(4,lines[line].c_str(),itemX+(dec?32:20),rowY+(lines.size()>1?24:38)+26*line,White,nameWidth,3.25f);}
      }
      EditorArt::fullView();
    }
  }
  const ucg::editor::ObjectDef *propertyDefinition() const {
    if (!c.selectMode || !c.model.selected() || c.model.selection_deco)
      return nullptr;
    int id = c.selectedObject();
    return id >= 0 && id < 26 ? &ucg::editor::object_definitions[id] : nullptr;
  }
  const ucg::editor::ObjectDef*syncPropertyLayout(){
    const auto*def=propertyDefinition();
    if(!def){propertyScroll=0;propertyObject=-1;propertySelection=~size_t(0);return nullptr;}
    if(propertyObject!=def->id||propertySelection!=c.model.selection_index){propertyScroll=0;propertyObject=def->id;propertySelection=c.model.selection_index;}
    propertyScroll=EditorPropertyLayout067::clamp(propertyScroll,unsigned(def->variable_count));return def;
  }
  bool updateProperties(const EditorInput::Actions &a) {
    if (c.tab != 2 || !a.primaryDown)
      return false;
    const auto *def = syncPropertyLayout();
    if (!def)
      return false;
    using Layout=EditorPropertyLayout067::Rect;
    const auto hit=[&](Layout r){return r.contains(float(cursor.x),float(cursor.y));};
    if(def->variable_count>EditorPropertyLayout067::Rows){
      if(hit(EditorPropertyLayout067::Previous)){propertyScroll=EditorPropertyLayout067::shift(propertyScroll,unsigned(def->variable_count),1);return true;}
      if(hit(EditorPropertyLayout067::Next)){propertyScroll=EditorPropertyLayout067::shift(propertyScroll,unsigned(def->variable_count),-1);return true;}
    }
    for (size_t i = propertyScroll; i < def->variable_count&&i<propertyScroll+EditorPropertyLayout067::Rows; ++i) {
      const auto &v = def->variables[i];
      const auto row=EditorPropertyLayout067::hit(unsigned(i),propertyScroll);
      const float y=row.y;
      if (!hit(row))
        continue;
      if (!c.model.selected() || c.model.selection_deco) {
        say(lang("Select an object to change variables",
                 "Выберите объект для изменения"));
        return true;
      }
      std::string e;
      Value initial;
      if (!c.variable(v.varname, initial, e))
        ucg::editor::parse_json(v.default_json, initial, e);
      if (!std::strcmp(v.type, "bool")) {
        c.setVariable(v.varname, Value(!initial.boolean), e);
      } else if (!std::strcmp(v.type, "enum")) {
        Value names;
        if (ucg::editor::parse_json(v.enum_names_json, names, e)) {
          options.clear();
          for (const auto &name : names.array)
            options.push_back(name.text);
          popup = control("HUD/tabs/objectsTab/exampleEnum");
          popupX = 24;
          popupY = y + 24;
          popupW = 208;
          popupScroll = 0;
          enumVariable = v.varname;
          popupSelected = unsigned(std::max(0., initial.numeric()));
          rebuildPopup();
        }
      } else
        beginInput(Input::Variable, v.varname, decimal(initial.numeric()),
                   v.name, true, v.min_value, v.max_value, v.bounded);
      if (!e.empty())
        say(e);
      return true;
    }
    if (hit(EditorPropertyLayout067::Color) && c.model.selected() &&
        !c.model.selection_deco) {
      colorSelection = true;
      colorKey.clear();
      color = readColor("exampleColor");
      openColor("HUD/tabs/objectsTab/exampleColor", color, 256, 120);
      return true;
    }
    return false;
  }
  void drawProperties() {
    if (c.tab != 2)
      return;
    const auto *def = syncPropertyLayout();
    if (!def)
      return;
    for (size_t i = propertyScroll; i < def->variable_count&&i<propertyScroll+EditorPropertyLayout067::Rows; ++i) {
      const auto &v = def->variables[i];
      const auto field=EditorPropertyLayout067::field(unsigned(i),propertyScroll,!std::strcmp(v.type,"bool"));
      const float y=field.y;
      const std::string label=v.name;size_t split=label.size();
      if(EditorArt::textWidth(3,label.c_str())*EditorPropertyLayout067::LabelScale>EditorPropertyLayout067::LabelWidth){
        split=label.find_last_of(' ');
        while(split!=std::string::npos&&EditorArt::textWidth(3,label.substr(0,split).c_str())*EditorPropertyLayout067::LabelScale>EditorPropertyLayout067::LabelWidth){
          if(!split){split=std::string::npos;break;}split=label.find_last_of(' ',split-1);
        }
      }
      const float baseline=EditorPropertyLayout067::labelBaseline(unsigned(i),propertyScroll);
      const bool outline=EditorTextFlow068::outline;EditorTextFlow068::outline=true;
      EditorArt::textScaled(3,label.substr(0,split).c_str(),24,baseline,White,208,EditorPropertyLayout067::LabelScale);
      if(split<label.size())EditorArt::textScaled(3,label.substr(split+1).c_str(),24,baseline+22,White,208,EditorPropertyLayout067::LabelScale);
      EditorTextFlow068::outline=outline;
      Value current;
      std::string e;
      if (!c.variable(v.varname, current, e))
        ucg::editor::parse_json(v.default_json, current, e);
      const char *path = !std::strcmp(v.type, "bool")
                             ? "HUD/tabs/objectsTab/exampleBool"
                         : !std::strcmp(v.type, "enum")
                             ? "HUD/tabs/objectsTab/exampleEnum"
                             : "HUD/tabs/objectsTab/exampleVariable";
      int id = control(path);
      std::string text =
          current.kind == Value::Kind::Number ? decimal(current.numeric()) : "";
      if (!std::strcmp(v.type, "enum")) {
        Value names;
        if (ucg::editor::parse_json(v.enum_names_json, names, e)) {
          int selected = int(current.numeric());
          if (selected >= 0 && size_t(selected) < names.array.size())
            text = names.array[selected].text;
        }
      }
      if (id >= 0)
        EditorArt::drawControlAt(
            unsigned(id), field.x, y,
            text.c_str(), current.boolean, false, false, russian());
    }
    if(def->variable_count>EditorPropertyLayout067::Rows){
      const auto arrow=[&](EditorPropertyLayout067::Rect r,const char*text){
        const int id=control("HUD/TopTab/save");if(id>=0){const auto&skin=EditorArt::Packed::Controls[id];EditorArt::frame(skin.skins[inside(cursor,r.x,r.y,r.w,r.h)?1:0],r.x,r.y,r.w/skin.w,r.h/skin.h);}
        EditorArt::textScaled(3,text,r.x+12,r.y+14,White,r.w-8,1.5f);
      };
      arrow(EditorPropertyLayout067::Previous,"<");arrow(EditorPropertyLayout067::Next,">");
      const auto range=std::to_string(propertyScroll+1)+"-"+std::to_string(std::min(unsigned(def->variable_count),propertyScroll+EditorPropertyLayout067::Rows))+" / "+std::to_string(def->variable_count);
      EditorArt::textScaled(4,range.c_str(),72,436,White,120,3.25f);
    }
    const auto colorRect=EditorPropertyLayout067::Color;
    const bool outline=EditorTextFlow068::outline;EditorTextFlow068::outline=true;
    EditorArt::textScaled(3,lang("Color", "Цвет"),24,460,White,160,2.25f);
    box(colorRect.x,colorRect.y,colorRect.w,colorRect.h,EditorColors::rgba(readColor("exampleColor")));
    EditorArt::textScaled(3,def->credit,24,490,White,208,1.75f);EditorTextFlow068::outline=outline;
  }
  void invalidateFileFocus(){if(!++fileFocusGeneration)++fileFocusGeneration;}
  void reloadFiles() {
    invalidateFileFocus();
    std::string e;
    if (!EditorFiles::list(fileRoot, fileRelative, fileEntries, e))
      say(e);
    fileEntries.erase(std::remove_if(fileEntries.begin(), fileEntries.end(),
                                     [&](const EditorFiles::Entry &entry) {
                                       return !entry.directory &&
                                              !fileExtension(entry.name);
                                     }),
                      fileEntries.end());
    fileSelected = std::min<unsigned>(
        fileSelected,
        fileEntries.empty() ? 0 : unsigned(fileEntries.size() - 1));
    fileScroll = std::min(fileScroll, fileSelected);
  }
  void saveChosen() {
    std::string relative = fileRelative.empty() ? fileName
                                                : fileRelative + "/" + fileName,
                path;
    if (fileName.empty() ||
        fileName.find_first_of("/\\:") != std::string::npos) {
      say("Invalid filename");
      return;
    }
    if (!fileExtension(relative))
      relative += filePurpose == FilePurpose::Palette ? ".tres" : ".json";
    if (!EditorFiles::join(fileRoot, relative, path)) {
      say("Invalid filename");
      return;
    }
    std::string e;
    if (filePurpose == FilePurpose::Palette) {
      if (!colorUi) {
        say("No active color picker");
        return;
      }
      if (fileSave) {
        if (!colorUi->save(path, e, &presetCache)) {
          say(e);
          return;
        }
      } else {
        if (!colorUi->load(path, e)) {
          say(e);
          return;
        }
        if (!EditorPanelColor::copy(colorUi->logical, presetCache, e))
          say(e);
      }
      files = false;
      fitColor();
      event(fileSave ? "palette-saved" : "palette-loaded",
            unsigned(colorUi->logical.size()));
      return;
    }
    if (fileSave) {
      auto old = c.path;
      c.path = path;
      if (!c.save(e)) {
        c.path = old;
        say(e);
        return;
      }
    } else {
      if (onBeforeLoad)
        onBeforeLoad();
      if (!c.load(path, e)) {
        say(e);
        return;
      }
    }
    files = false;
    welcome = false;
    refresh();
    event(fileSave ? "saved" : "loaded",
          unsigned(c.model.document.nodes().size()),
          unsigned(c.model.document.decorations().size()));
  }
  void chooseEntry() {
    if (fileEntries.empty() || fileSelected >= fileEntries.size())
      return;
    const auto &entry = fileEntries[fileSelected];
    if (entry.directory) {
      if (entry.name == "..")
        fileRelative = EditorFiles::base(fileRelative);
      else
        fileRelative =
            fileRelative.empty() ? entry.name : fileRelative + "/" + entry.name;
      fileSelected = fileScroll = 0;
      reloadFiles();
    } else if (fileExtension(entry.name)) {
      fileName = entry.name;
      if (!fileSave)
        saveChosen();
    }
  }
  void updateFiles(const EditorInput::Actions &a, unsigned pressed) {
    if(!fileFocusPrepared)invalidateFileFocus();
    fileFocusPrepared=false;
    if (a.back) {
      files = false;
      return;
    }
    if (a.wheel) {
      fileScroll =
          unsigned(std::clamp(int(fileScroll) - a.wheel, 0,
                              std::max(0, int(fileEntries.size()) - int(EditorTextLayout068::FileRows))));
      if(!fileEntries.empty())fileSelected=std::clamp(fileSelected,fileScroll,std::min(unsigned(fileEntries.size()-1),fileScroll+EditorTextLayout068::FileRows-1));
    }
    if (pressed & EditorInput::Up) {
      if (fileSelected)
        --fileSelected;
    }
    if (pressed & EditorInput::Down) {
      if (fileSelected + 1 < fileEntries.size())
        ++fileSelected;
    }
    if (fileSelected < fileScroll)
      fileScroll = fileSelected;
    if (fileSelected >= fileScroll + EditorTextLayout068::FileRows)
      fileScroll = fileSelected - EditorTextLayout068::FileRows + 1;
    if (a.pause) {
      chooseEntry();
      return;
    }
    if (!a.primaryDown)
      return;
    if (inside(cursor,284,EditorTextLayout068::FileTop,452,EditorTextLayout068::FileRows*EditorTextLayout068::FilePitch)) {
      unsigned row = unsigned((cursor.y - EditorTextLayout068::FileTop) / EditorTextLayout068::FilePitch);
      fileSelected = fileScroll + row;
      if (fileSelected < fileEntries.size())
        chooseEntry();
    } else if (inside(cursor, 284, 370, 452, 44))
      beginInput(Input::Filename, "", fileName,
                 filePurpose == FilePurpose::Palette
                     ? lang("Palette filename", "Имя файла палитры")
                     : lang("Filename", "Имя файла"));
    else if (inside(cursor, 284, 82, 452, 44))
      beginInput(Input::Folder, "", "", lang("New folder", "Новая папка"));
    else if (inside(cursor, 284, 426, 128, 40)) {
      if (fileName.empty()) {
        beginInput(Input::Filename, "",
                   filePurpose == FilePurpose::Palette ? "new_palette.tres"
                                                       : "level.json",
                   filePurpose == FilePurpose::Palette
                       ? lang("Palette filename", "Имя файла палитры")
                       : lang("Filename", "Имя файла"));
        return;
      }
      std::string relative = fileRelative.empty()
                                 ? fileName
                                 : fileRelative + "/" + fileName,
                  path;
      if (!fileExtension(relative))
        relative += filePurpose == FilePurpose::Palette ? ".tres" : ".json";
      if (fileSave && EditorFiles::join(fileRoot, relative, path) &&
          EditorFiles::exists(path))
        overwrite = true;
      else
        saveChosen();
    } else if (inside(cursor, 428, 426, 128, 40)) {
      files = false;
    } else if (inside(cursor, 572, 426, 164, 40))
      beginInput(Input::Folder, "", "", lang("New folder", "Новая папка"));
  }
  void drawFiles() {
    const bool outline=EditorTextFlow068::outline;EditorTextFlow068::outline=true;
    popupPanel(272, 32, 476, 436);
    EditorArt::textScaled(
        3,
        filePurpose == FilePurpose::Palette
            ? (fileSave ? lang("Save Color Palette", "Сохранить палитру")
                        : lang("Load Color Palette", "Загрузить палитру"))
        : fileSave ? lang("Save level", "Сохранить уровень")
                   : lang("Open level", "Открыть уровень"),
        284,72,White,452,1.75f);
    button(284, 82, 452, 44,
           ((filePurpose == FilePurpose::Palette ? "palettes/"
                                                 : "custom_levels/") +
            fileRelative)
               .c_str());
    box(284,138,452,220, 0xff000000);
    for (unsigned i = 0; i < EditorTextLayout068::FileRows && fileScroll + i < fileEntries.size(); ++i) {
      auto index = fileScroll + i;
      const auto &entry = fileEntries[index];
      float y = EditorTextLayout068::FileTop + i * EditorTextLayout068::FilePitch;
      if (fileSelected == index)
        box(284,y,452,EditorTextLayout068::FilePitch, 0x4cffffff);
      std::string name = (entry.directory ? "[DIR] " : "") + entry.name;
      EditorArt::textScaled(3,name.c_str(),288,y+28,White,440,1.75f);
    }
    button(284, 370, 452, 44,
           fileName.empty() ? lang("Filename...", "Имя файла...")
                            : fileName.c_str());
    button(284, 426, 128, 40,
           fileSave ? lang("Save", "Сохранить") : lang("Open", "Открыть"));
    button(428, 426, 128, 40, lang("Cancel", "Отмена"));
    button(572, 426, 164, 40, lang("New folder", "Новая папка"));
    EditorTextFlow068::outline=outline;
  }
  void applyColor() {
    std::string e;
    if (colorSelection) {
      if (!c.model.edit_selected(
              [&](Value &v) { v.array[2] = fullColorValue(color); }, e))
        say(e);
    } else if (!colorKey.empty()) {
      if (!c.set(colorKey.c_str(), fullColorValue(color), e))
        say(e);
    }
    // Color/palette/tint are derived live; keep immutable texture caches warm.
  }
  static Value fullColorValue(Color c) {
    return Value("(" + ucg::editor::decimal(float(c.r)) + ", " +
                 ucg::editor::decimal(float(c.g)) + ", " +
                 ucg::editor::decimal(float(c.b)) + ", " +
                 ucg::editor::decimal(float(c.a)) + ")");
  }
  bool fileExtension(const std::string &name) const {
    return filePurpose == FilePurpose::Palette
               ? EditorPaletteFiles::detail::extension(name) != 0
               : EditorFiles::extension(name, ".json");
  }
  void openPaletteFiles(bool saving) {
    filePurpose = FilePurpose::Palette;
    files = true;
    fileSave = saving;
    fileRoot = companion(executable.c_str(), "palettes");
    fileRelative.clear();
    fileName = saving ? "new_palette.tres" : "";
    std::string e;
    if (!EditorFiles::make(fileRoot))
      say(lang("Cannot open palette folder",
               "Не удалось открыть папку палитр"));
    fileSelected = fileScroll = 0;
    reloadFiles();
  }
  void openColor(const std::string &key, Color value, float x, float y) {
    auto found = colorContexts.find(key);
    if (found == colorContexts.end()) {
      EditorPanelColor::Context fresh;
      std::string e;
      if (!EditorPanelColor::copy(presetCache, fresh.logical, e) ||
          !EditorPanelColor::copy(presetCache, fresh.visual, e)) {
        say(e);
        return;
      }
      fresh.selectedPreset = int(fresh.visual.size()) - 1;
      found = colorContexts.emplace(key, std::move(fresh)).first;
    }
    colorUi = &found->second;
    colorUi->setColor(value);
    colorUi->focusChannel = -1;
    oldColor = color = value;
    colorX = x;
    colorY = y;
    colorOpen = true;
    colorSampling = false;
    colorMenu = -1;
    presetDrag = -1;
    presetDragging = false;
    spinChannel = -1;
    fitColor();
  }
  void fitColor() {
    if(!colorUi)return;
    const auto m=colorUi->layout();colorUi->presetScroll068=m.presetFirstRow068;
    constexpr float scale=EditorColorPickerArt::PresentationScale068;
    colorX=std::clamp(colorX,4*scale,768-(m.w+4)*scale);
    colorY=std::clamp(colorY,4*scale,512-(m.h+4)*scale);
  }
  std::string colorHex() const {
    auto out = EditorColors::html(color);
    if (color.a >= 1)
      out.resize(6);
    return out;
  }
  void closeColor() {
    colorOpen = false;
    colorSampling = false;
    colorMenu = -1;
    presetDrag = -1;
    presetDragging = false;
    spinChannel = -1;
    if (colorUi)
      colorUi->dragKind = -1;
  }
  void openColorMenu(int kind, const EditorColorPickerArt::Rect &anchor) {
    if (!colorUi)
      return;
    colorMenu = kind;
    colorMenuItems = kind == 0   ? colorUi->modeItems()
                     : kind == 1 ? colorUi->shapeItems()
                                 : colorUi->paletteItems();
    colorMenuGeometry = EditorMenuArt::layout(colorMenuItems);
    const auto projected=EditorColorPickerArt::logicalRect068(anchor,colorX,colorY);
    auto at = EditorMenuArt::anchor(projected, 0, 0);
    colorMenuX =
        std::clamp(at.x, 4.f, std::max(4.f, 764 - colorMenuGeometry.w));
    colorMenuY =
        std::clamp(at.y, 4.f, std::max(4.f, 508 - colorMenuGeometry.h));
  }
  void colorMenuAction(int id) {
    if (!colorUi)
      return;
    auto &ui = *colorUi;
    std::string e;
    if (colorMenu == 0) {
      if (id >= 0 && id < 4)
        ui.mode(static_cast<EditorColors::State::Mode>(id));
      else if (id == 5)
        ui.colorized = !ui.colorized;
    } else if (colorMenu == 1) {
      if (id >= 0 && id < 4)
        ui.chooseShape(unsigned(id));
    } else if (id == 0) {
      if (ui.path.empty())
        openPaletteFiles(true);
      else if (!ui.save(ui.path, e, &presetCache))
        say(e);
    } else if (id == 1)
      openPaletteFiles(true);
    else if (id == 2)
      openPaletteFiles(false);
    else if (id == 4) {
      for (auto c : ui.logical)
        EditorColorPickerArt::erasePreset(presetCache, c);
      ui.clear();
    }
    colorMenu = -1;
    fitColor();
  }
  const char *channelName(unsigned i) const {
    static const char *en[][4] = {{"Red", "Green", "Blue", "Alpha"},
                                  {"Hue", "Saturation", "Value", "Alpha"},
                                  {"Red", "Green", "Blue", "Alpha"},
                                  {"Hue", "Saturation", "Lightness", "Alpha"}};
    static const char *ru[][4] = {{"Красный", "Зелёный", "Синий", "Альфа"},
                                  {"Тон", "Насыщенность", "Яркость", "Альфа"},
                                  {"Красный", "Зелёный", "Синий", "Альфа"},
                                  {"Тон", "Насыщенность", "Светлота", "Альфа"}};
    return russian() ? ru[unsigned(colorUi->picker.mode)][i]
                     : en[unsigned(colorUi->picker.mode)][i];
  }
  void updateColor(const EditorInput::Actions &a) {
    if (!colorUi)
      return;
    auto &ui = *colorUi;
    Point p{EditorColorPickerArt::source068(cursor.x,colorX),EditorColorPickerArt::source068(cursor.y,colorY)};
    auto m = ui.layout();
    if (colorSampling) {
      if (a.back) {
        colorSampling = false;
        return;
      }
      if (a.primaryDown && onSampleColor) {
        color = onSampleColor(cursor);
        ui.setColor(color);
        colorSampling = false;
        applyColor();
      }
      return;
    }
    if (colorMenu >= 0) {
      if (a.back) {
        colorMenu = -1;
        return;
      }
      if (a.wheel) {
        colorMenuGeometry = EditorMenuArt::layout(
            colorMenuItems, 0, 760, 504,
            std::clamp(colorMenuGeometry.scroll - a.wheel * 20.f, 0.f,
                       colorMenuGeometry.scrollMax));
      }
      if (a.primaryDown) {
        int row =
            EditorMenuArt::hit(colorMenuGeometry, colorMenuItems,
                               cursor.x - colorMenuX, cursor.y - colorMenuY);
        if (row >= 0)
          colorMenuAction(colorMenuItems[row].id);
        else
          colorMenu = -1;
      }
      return;
    }
    if (a.back) {
      color = oldColor;
      ui.setColor(color);
      applyColor();
      closeColor();
      return;
    }
    if (a.primaryDown && !inside(p, -4, -4, m.w + 8, m.h + 8)) {
      closeColor();
      return;
    }
    if (presetDrag >= 0) {
      if (a.primary && std::hypot(cursor.x - presetDragOrigin.x,
                                  cursor.y - presetDragOrigin.y) >= 10)
        presetDragging = true;
      if (a.primaryUp || !a.primary) {
        if (presetDragging)
          for (unsigned i = 0; i < ui.visual.size(); ++i)
            if (m.preset(i).contains(p.x, p.y)) {
              ui.move(unsigned(presetDrag), i);
              break;
            }
        presetDrag = -1;
        presetDragging = false;
      }
      return;
    }
    if (ui.dragKind >= 0) {
      if (a.primary) {
        if (ui.drag(m, p.x, p.y)) {
          color = ui.picker.value;
          applyColor();
        }
      }
      if (a.primaryUp || !a.primary)
        ui.endDrag();
      return;
    }
    if (spinChannel >= 0) {
      if (a.primaryUp || !a.primary) {
        ui.addRecent(color);
        spinChannel = -1;
      } else if (int32_t(EditorPanelColor::micros() - spinNext) >= 0) {
        const double step = ui.picker.mode == EditorColors::State::Mode::Linear
                                ? double(float(.01))
                                : 1.;
        if (ui.editChannel(unsigned(spinChannel),
                           ui.picker.channel(unsigned(spinChannel)) +
                               spinDirection * step)) {
          color = ui.picker.value;
          applyColor();
        }
        spinNext = EditorPanelColor::micros() + 75000;
      }
      return;
    }
    if(a.wheel&&ui.savedOpen&&m.presetGrid.contains(p.x,p.y)){
      ui.presetScroll068=unsigned(std::clamp(int(m.presetFirstRow068)-a.wheel,0,int(EditorColorPickerArt::lastPresetRow068(unsigned(ui.visual.size())))));return;
    }
    if(a.primaryDown&&(m.presetPrevious068.contains(p.x,p.y)||m.presetNext068.contains(p.x,p.y))){
      ui.presetScroll068=m.presetFirstRow068+(m.presetNext068.contains(p.x,p.y)?1u:unsigned(-1));return;
    }
    if (a.secondaryDown && ui.savedOpen) {
      for (const auto &c : ui.visual)
        if (m.preset(unsigned(&c - ui.visual.data())).contains(p.x, p.y)) {
          Color removed = c;
          ui.erase(removed);
          EditorColorPickerArt::erasePreset(presetCache, removed);
          fitColor();
          return;
        }
    }
    if (!a.primaryDown)
      return;
    if (m.sampler.contains(p.x, p.y)) {
      colorSampling = true;
      return;
    }
    if (m.shape.contains(p.x, p.y)) {
      openColorMenu(1, m.shape);
      return;
    }
    if (m.modeMenu.contains(p.x, p.y)) {
      openColorMenu(0, m.modeMenu);
      return;
    }
    if (m.paletteMenu.contains(p.x, p.y)) {
      openColorMenu(2, m.paletteMenu);
      return;
    }
    for (unsigned i = 0; i < 3; ++i)
      if (m.mode[i].contains(p.x, p.y)) {
        ui.mode(static_cast<EditorColors::State::Mode>(i));
        return;
      }
    if (m.sample.contains(p.x, p.y) && p.x < m.sample.x + m.sample.w / 2) {
      color = oldColor;
      ui.setColor(color);
      applyColor();
      return;
    }
    if (ui.beginPlane(m, p.x, p.y) || ui.beginBar(m, p.x, p.y)) {
      color = ui.picker.value;
      applyColor();
      return;
    }
    for (unsigned i = 0; i < 4; ++i) {
      if (ui.beginChannel(m, i, p.x, p.y)) {
        ui.focusChannel = int(i);
        color = ui.picker.value;
        applyColor();
        return;
      }
      if (m.spin[i].contains(p.x, p.y)) {
        ui.focusChannel = -1;
        if (p.x >= m.spin[i].x + 40) {
          spinChannel = int(i);
          spinDirection = p.y < m.spin[i].y + 11.5 ? 1 : -1;
          spinNext = EditorPanelColor::micros() + 600000;
          const double step =
              ui.picker.mode == EditorColors::State::Mode::Linear
                  ? double(float(.01))
                  : 1.;
          if (ui.editChannel(i, ui.picker.channel(i) + spinDirection * step)) {
            color = ui.picker.value;
            applyColor();
          }
        } else
          beginInput(Input::ColorChannel, std::to_string(i),
                     ui.picker.channelText(i), channelName(i), true);
        return;
      }
    }
    if (ui.hexVisible() && m.hex.contains(p.x, p.y)) {
      ui.focusChannel = -1;
      beginInput(Input::ColorHex, "", colorHex(),
                 lang("Hex or named color", "Код или название цвета"));
      return;
    }
    if (m.swatchesHeader.contains(p.x, p.y)) {
      ui.savedOpen = !ui.savedOpen;
      fitColor();
      return;
    }
    if (m.recentHeader.contains(p.x, p.y)) {
      ui.recentOpen = !ui.recentOpen;
      fitColor();
      return;
    }
    if (ui.savedOpen && m.add.contains(p.x, p.y)) {
      std::string e;
      if (ui.add(color, &e)) {
        const bool exists =
            std::any_of(presetCache.begin(), presetCache.end(), [&](Color v) {
              return EditorPanelColor::same(v, color);
            });
        if (!EditorPanelColor::ensure(
                presetCache, unsigned(presetCache.size()) + !exists, e))
          say(e);
        else
          EditorColorPickerArt::addPreset(presetCache, color);
      } else
        say(e);
      ui.presetScroll068=EditorColorPickerArt::ensurePreset068(unsigned(std::max(0,ui.selectedPreset)),ui.presetScroll068);
      fitColor();
      return;
    }
    for (unsigned i = 0; i < ui.visual.size(); ++i)
      if (m.preset(i).contains(p.x, p.y)) {
        ui.focusChannel = -1;
        color = ui.visual[i];
        ui.setColor(color);
        ui.addRecent(color);
        applyColor();
        presetDrag = int(i);
        presetDragOrigin = cursor;
        presetDragging = false;
        return;
      }
    for (unsigned i = 0; i < ui.recent.size(); ++i)
      if (m.recent(i).contains(p.x, p.y)) {
        ui.focusChannel = -1;
        color = ui.recent[i];
        ui.setColor(color);
        ui.selectRecent(i);
        applyColor();
        return;
      }
  }
  void drawColor() {
    if (colorSampling || !colorUi)
      return;
    auto &ui = *colorUi;
    auto m = ui.layout();
    Point p{EditorColorPickerArt::source068(cursor.x,colorX),EditorColorPickerArt::source068(cursor.y,colorY)};
    auto label = ui.label();
    {EditorColorPickerArt::DrawScope068 transform(colorX,colorY);
    EditorColorPickerArt::drawBase(
        m, 0, 0, unsigned(ui.picker.mode), ui.hexVisible(),
        m.swatchesHeader.contains(p.x, p.y), m.recentHeader.contains(p.x, p.y),
        m.add.contains(p.x, p.y) ? (heldPrimary ? 2 : 1) : 0, label.c_str());
    EditorColorPickerArt::drawPlane(
        m, 0, 0, ui.actualShape(),
        ui.actualShape() == 3 ? ui.okPlane.h * 360. : ui.hsv.h,
        ui.actualShape() == 3 ? ui.okPlane.s : ui.hsv.s, ui.hsv.v,
        ui.okPlane.l);
    EditorColorPickerArt::drawShape(m, 0, 0, ui.shape,
                                    m.shape.contains(p.x, p.y));
    EditorPanelColor::drawSample(m.sample, 0, 0, oldColor, color);
    int hover = -1;
    for (unsigned i = 0; i < 4; ++i)
      if (m.slider[i].contains(p.x, p.y))
        hover = int(i);
    EditorPanelColor::drawChannels(m, 0, 0, ui, hover);
    for (unsigned i = 0; i < 4; ++i)
      EditorColorPickerArt::drawText(m.spinText[i], 0, 0,
                                     ui.picker.channelText(i).c_str(), true);
    if (ui.hexVisible())
      EditorColorPickerArt::drawText(m.hex, 0, 0, colorHex().c_str(),
                                     false);
    for (unsigned i = m.presetBegin068(); ui.savedOpen && i < m.presetEnd068(); ++i) {
      if(m.preset(i).w<=0)continue;
      auto v = ui.visual[i];
      EditorColorPickerArt::drawSwatch(m.preset(i), 0, 0, v.r, v.g,
                                       v.b, v.a, int(i) == ui.selectedPreset);
    }
    for (unsigned i = 0; ui.recentOpen && i < ui.recent.size(); ++i) {
      auto v = ui.recent[i];
      EditorColorPickerArt::drawSwatch(m.recent(i), 0, 0, v.r, v.g,
                                       v.b, v.a, int(i) == ui.selectedRecent);
    }
    if (presetDragging && presetDrag >= 0 &&
        unsigned(presetDrag) < ui.visual.size()) {
      auto v = ui.visual[unsigned(presetDrag)];
      EditorColorPickerArt::drawSwatch({float(p.x), float(p.y), 16, 16}, 0,
                                       0, v.r, v.g, v.b, v.a);
    }
    if(ui.savedOpen&&EditorColorPickerArt::lastPresetRow068(m.presetCount)){
      char range[64];const unsigned first=m.presetFirstRow068?m.presetFirstRow068*EditorColorPickerArt::Columns:1,last=std::min(m.presetCount,(m.presetFirstRow068+m.presetRows068)*EditorColorPickerArt::Columns-1);
      std::snprintf(range,sizeof range,"%u-%u/%u",first,last,m.presetCount);EditorColorPickerArt::drawText(m.presetRange068,0,0,range,false);
    }
    }
    if (colorMenu >= 0)
      EditorMenuArt::drawMenu(
          colorMenuGeometry, colorMenuItems, colorMenuX, colorMenuY,
          EditorMenuArt::hit(colorMenuGeometry, colorMenuItems,
                             cursor.x - colorMenuX, cursor.y - colorMenuY));
  }
};
} // namespace EditorPanel
