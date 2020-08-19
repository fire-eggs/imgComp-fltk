#pragma once
#include <FL/Fl_Preferences.H>
#include <string>

#define INITIAL_TOOLBAR_HIGH 40
#define INITIAL_THUMBBAR_WIDE 170 // thumb size + sbwidth + ?
#define SBWIDTH 17                // TODO to preferences

#define TB_COLOR 0x80B3BF00 // TODO settings / theme
#define TB_SELEC 0x5B96A300
#define DEF_BACK_COLOR 0xBFF5D900

#define APPLICATION "imgcomp"
#define BACK_COLOR "displayBackColor"
#define CENTER_TOOLBAR "centerToolbar"
#define DRAW_CHECK "drawCheckerBoard"
#define LOOP_IMAGES "loopImages"
#define MAIN_PREFIX "main"
#define MRU_GROUP "MRU"
#define ORGANIZATION "iglite.com"
#define SLIDE_COUNTDOWN "showCountdown"
#define SLIDE_FULLSCREEN "fullScreen"
#define SLIDE_GROUP "Slideshow"
#define SLIDE_INTERVAL "interval"
#define SLIDE_PAUSED "startPaused"
#define SLIDE_LOOP "loop"
#define SHOW_SCROLL "showScrollbars"
#define THUMB_ON "thumbsInitiallyVisible"
#define THUMB_WIDE "thumb_wide"
#define TOOLBAR_BACK "toolbarBackColor"
#define TOOLBAR_CLICK "toolbarClickColor"
#define TOOLBAR_HIGH "toolbar_high"


class Prefs
{
public:
    Fl_Preferences* _prefs;

    Prefs()
    {
        _prefs = new Fl_Preferences(Fl_Preferences::USER, ORGANIZATION, APPLICATION);
    }

    ~Prefs()
    {
        delete _prefs;
        _prefs = NULL;
    }

    Prefs(Fl_Preferences* group)
    {
        _prefs = group;
    }

    Prefs* getSubPrefs(const char* group)
    {
        if (!_prefs)
            return NULL;
        return new Prefs(new Fl_Preferences(_prefs, group));
    }

    void getWinRect(const char* prefix, int& x, int& y, int& w, int& h)
    {
        std::string n = prefix;
        _prefs->get((n + "_x").c_str(), x,  50);
        _prefs->get((n + "_y").c_str(), y,  50);
        _prefs->get((n + "_w").c_str(), w, 700);
        _prefs->get((n + "_h").c_str(), h, 700);
    }

    void setWinRect(const char* prefix, int x, int y, int w, int h)
    {
        std::string n = prefix;
        _prefs->set((n + "_x").c_str(), x);
        _prefs->set((n + "_y").c_str(), y);
        _prefs->set((n + "_w").c_str(), w);
        _prefs->set((n + "_h").c_str(), h);
        _prefs->flush();
    }

    void set(const char* n, int val)
    {
        _prefs->set(n, val);
        _prefs->flush();
    }
};
