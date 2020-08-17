#include "ViewWin.h"
#include <FL/Fl_Double_Window.H>
#include "Fl_Image_Display.H"

Fl_Double_Window* _win;
Fl_Image_Display* _disp;
Pair* _viewing;
bool _startLeft;

void ViewImage()
{
    const char* path;
    
    if (_startLeft)
        path = GetFD(_viewing->FileLeftDex)->Name->c_str();
    else
        path = GetFD(_viewing->FileRightDex)->Name->c_str();

    Fl_Shared_Image* img = Fl_Shared_Image::get(path);

    _disp->value(img);

    char buff[1024];
    if (_startLeft)
        sprintf(buff, "Left image: %s", path);
    else
        sprintf(buff, "Right image: %s", path);

    _win->label(buff);
}

void initShow()
{
    _win = 0;
}

void makeWin()
{
    int x = 100;
    int y = 100;
    int w = 1024;
    int h = 768;

    _win = new Fl_Double_Window(x, y, w, h);

    _win->begin();
    _disp = new Fl_Image_Display(0, 0, w, h);

    // btnbar
    // swap, close, 100% / fit, zoom in, zoom out

    _win->end();
    _win->resizable(_disp);
}

void showView(Pair* toview, bool startLeft)
{
    if (!_win)
        makeWin();

    _viewing = toview;
    _startLeft = startLeft;

    _win->show();
    ViewImage(); // NOTE: title bar doesn't update until shown

    while (_win->shown())
        Fl::wait();
}
