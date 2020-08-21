#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>

#include "ViewWin.h"
#include "Fl_Image_Display.H"

Fl_Double_Window* _win;
Fl_Image_Display* _disp;
Pair* _viewing;
bool _showLeft;
Fl_Button* _bFull; // the 'Fit'/'100%' button

void ViewImage()
{
    const char* path;
    
    if (_showLeft)
        path = GetFD(_viewing->FileLeftDex)->Name->c_str();
    else
        path = GetFD(_viewing->FileRightDex)->Name->c_str();

    Fl_Shared_Image* img = Fl_Shared_Image::get(path);

    _disp->value(img);

    char buff[1024];
    if (_showLeft)
        sprintf(buff, "Left image: %s", path);
    else
        sprintf(buff, "Right image: %s", path);

    _win->label(buff);
}

void bSwapClick(Fl_Widget *, void *d)
{
    _showLeft = !_showLeft;
    ViewImage();
}

void bFullClick(Fl_Widget *, void *d)
{
    // alternate between "100%" and "Fit"
    bool full = _bFull->label()[0] == '1';
    _bFull->label( full ? "Fit" : "100%" );
    if (full)
        _disp->scale(1.0);
    else
        _disp->scale(0);
}

void bZoomInClick(Fl_Widget *, void *)
{
    _disp->zoomDelta(-1);
}

void bZoomOutClick(Fl_Widget *, void *)
{
    _disp->zoomDelta(+1);
}

void bCloseClick(Fl_Widget *, void *)
{
    _win->hide();
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
    _disp = new Fl_Image_Display(0, 0, w, h-27);

    // btnbar
    // swap, close, 100% / fit, zoom in, zoom out
    {
        Fl_Group* o = new Fl_Group(0, h-27, w, 27);
        o->begin();
        
        int bx = 2;
        int bw = 55;  // other buttons same width
        int bwm = bw + 3; // margin
        int bw2 = 80; // wider for zoom buttons
        int bwm2 = bw2 + 3;
        
        Fl_Button* bSwap = new Fl_Button(bx, h-25, bw, 25);
        bSwap->label("Swap");
        bSwap->callback(bSwapClick);

        bx += bwm;
        _bFull = new Fl_Button(bx, h-25, bw, 25);
        _bFull->label("100%");
        _bFull->callback(bFullClick);

        bx += bwm;
        Fl_Button* bZoomIn = new Fl_Button(bx, h-25, bw2, 25);
        bZoomIn->label("Zoom In");
        bZoomIn->callback(bZoomInClick);

        bx += bwm2;
        Fl_Button* bZoomOut = new Fl_Button(bx, h-25, bw2, 25);
        bZoomOut->label("Zoom Out");
        bZoomOut->callback(bZoomOutClick);

        bx += bwm2;
        Fl_Button* bClose = new Fl_Button(bx, h-25, bw, 25);
        bClose->label("Close");
        bClose->callback(bCloseClick);
        
        o->end();
    }

    _win->end();
    _win->resizable(_disp);
}

void showView(Pair* toview, bool startLeft)
{
    if (!_win)
        makeWin();

    _viewing = toview;
    _showLeft = startLeft;

    _win->show();
    ViewImage(); // NOTE: title bar doesn't update until shown

    while (_win->shown())
        Fl::wait();
}
