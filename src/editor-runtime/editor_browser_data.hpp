#pragma once
namespace EditorBrowserArt {namespace Packed {
struct Frame {const char*path;float x,y,w,h;unsigned width,height;bool raw;};
inline constexpr Frame Frames[]={
{"data/world/editor-aux/browser/BrowserCard0.png",-8.0f,-8.0f,472.0f,140.0f,259,83,false},
{"data/world/editor-aux/browser/BrowserCard1.png",-8.0f,-8.0f,472.0f,140.0f,259,83,false},
{"data/world/editor-aux/browser/BrowserCard2.png",-8.0f,-8.0f,472.0f,140.0f,259,83,false},
{"data/world/editor-aux/browser/BrowserDescription.png",-8.0f,-8.0f,384.0f,64.0f,212,42,false},
{"data/world/editor-aux/browser/BrowserSublevel0.png",-8.0f,-8.0f,60.0f,24.0f,40,21,false},
{"data/world/editor-aux/browser/BrowserSublevel1.png",-8.0f,-8.0f,60.0f,24.0f,40,21,false},
{"data/world/editor-aux/browser/BrowserSublevel2.png",-8.0f,-8.0f,60.0f,24.0f,40,21,false},
{"data/world/editor-aux/browser/BrowserGo0.png",-8.0f,-8.0f,64.0f,64.0f,42,42,false},
{"data/world/editor-aux/browser/BrowserGo1.png",-8.0f,-8.0f,64.0f,64.0f,42,42,false},
{"data/world/editor-aux/browser/BrowserGo2.png",-8.0f,-8.0f,64.0f,64.0f,42,42,false},
{"data/world/editor-aux/browser/BrowserRank_peak.png",0.0f,0.0f,16.0f,16.0f,8,8,false},
{"data/world/editor-aux/browser/BrowserRank_swag.png",0.0f,0.0f,16.0f,16.0f,8,8,false},
{"data/world/editor-aux/browser/BrowserRank_ok.png",0.0f,0.0f,16.0f,16.0f,8,8,false},
{"data/world/editor-aux/browser/BrowserRank_erm.png",0.0f,0.0f,16.0f,16.0f,8,8,false},
{"data/world/editor-aux/browser/BrowserRank_atrocious.png",0.0f,0.0f,16.0f,16.0f,8,8,false},
{"data/world/editor-aux/browser/BrowserRank_norank.png",0.0f,0.0f,16.0f,16.0f,8,8,false},
{"data/world/editor-aux/browser/BrowserHScrollBar_scroll.png",0.0f,0.0f,64.0f,8.0f,76,20,true},
{"data/world/editor-aux/browser/BrowserHScrollBar_scroll_focus.png",0.0f,0.0f,64.0f,8.0f,76,20,true},
{"data/world/editor-aux/browser/BrowserHScrollBar_grabber.png",0.0f,0.0f,64.0f,8.0f,76,20,true},
{"data/world/editor-aux/browser/BrowserHScrollBar_grabber_highlight.png",0.0f,0.0f,64.0f,8.0f,76,20,true},
{"data/world/editor-aux/browser/BrowserHScrollBar_grabber_pressed.png",0.0f,0.0f,64.0f,8.0f,76,20,true},
{"data/world/editor-aux/browser/BrowserVScrollBar_scroll.png",0.0f,0.0f,8.0f,64.0f,20,76,true},
{"data/world/editor-aux/browser/BrowserVScrollBar_scroll_focus.png",0.0f,0.0f,8.0f,64.0f,20,76,true},
{"data/world/editor-aux/browser/BrowserVScrollBar_grabber.png",0.0f,0.0f,8.0f,64.0f,20,76,true},
{"data/world/editor-aux/browser/BrowserVScrollBar_grabber_highlight.png",0.0f,0.0f,8.0f,64.0f,20,76,true},
{"data/world/editor-aux/browser/BrowserVScrollBar_grabber_pressed.png",0.0f,0.0f,8.0f,64.0f,20,76,true},
{"data/world/editor-aux/browser/BrowserSubFocus.png",-8.0f,-8.0f,60.0f,24.0f,40,21,false},
};
struct ScrollStyle {int frame;float left,top,right,bottom,expandLeft,expandTop,expandRight,expandBottom;};
inline constexpr ScrollStyle Scroll[2][5]={
{{16,10.0f,10.0f,10.0f,10.0f,6.0f,6.0f,6.0f,6.0f},{17,9.0f,9.0f,9.0f,9.0f,6.0f,6.0f,6.0f,6.0f},{18,10.0f,10.0f,10.0f,10.0f,6.0f,6.0f,6.0f,6.0f},{19,10.0f,10.0f,10.0f,10.0f,6.0f,6.0f,6.0f,6.0f},{20,10.0f,10.0f,10.0f,10.0f,6.0f,6.0f,6.0f,6.0f}},
{{21,6.0f,6.0f,6.0f,6.0f,6.0f,6.0f,6.0f,6.0f},{22,9.0f,9.0f,9.0f,9.0f,6.0f,6.0f,6.0f,6.0f},{23,6.0f,6.0f,6.0f,6.0f,6.0f,6.0f,6.0f,6.0f},{24,6.0f,6.0f,6.0f,6.0f,6.0f,6.0f,6.0f,6.0f},{25,6.0f,6.0f,6.0f,6.0f,6.0f,6.0f,6.0f,6.0f}},
};
inline constexpr int FocusFrame=26;
}
struct Image {const char*path;unsigned width,height;};
inline constexpr Image DefaultIcon={"data/world/editor-aux/browser/default-world.png",128,128};
}
