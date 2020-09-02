#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>

#include "ViewWin.h"
#include "Fl_Image_Display.H"
#include "prefs.h"

Fl_Double_Window* _win;
Fl_Image_Display* _disp;
Pair* _viewing;
Fl_RGB_Image* _view;
bool _showLeft;
Fl_Button* _bFull; // the 'Fit'/'100%' button

extern Prefs* _PREFS;

class ViewWin : public Fl_Double_Window
{
public:
    ViewWin(int x, int y, int w, int h) : Fl_Double_Window(x, y, w, h)
    {}

    void resize(int, int, int, int) override;
};

void ViewWin::resize(int x, int y, int w, int h)
{
    Fl_Double_Window::resize(x, y, w, h);
    _PREFS->setWinRect("View", x, y, w, h);
}

void ViewImage()
{
    if (_viewing)
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
    else
    {
        _disp->value((Fl_Shared_Image *)_view);
    }
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
    int x, y, w, h;
    _PREFS->getWinRect("View", x, y, w, h);

    _win = new ViewWin(x, y, w, h);

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
    _view = NULL;
    _showLeft = startLeft;

    _win->show();
    ViewImage(); // NOTE: title bar doesn't update until shown

    while (_win->shown())
        Fl::wait();
}

void showView(Fl_RGB_Image *img)
{
    if (!_win)
        makeWin();
    _view = img;
    _viewing = NULL;
    _win->show();
    ViewImage(); // NOTE: title bar doesn't update until shown
    while (_win->shown())
        Fl::wait();
}

Fl_RGB_Image* _diffImage;

bool doDiff(Fl_Shared_Image* imgL, Fl_Shared_Image* imgR)
{
    // 2. get the image pixel data [assuming Fl_RGB_Image for now]
    const char* const* dataL0 = imgL->data();
    const char* const* dataR0 = imgR->data();

    const unsigned char* dataL = (const unsigned char*)imgL->data()[0];
    const unsigned char* dataR = (const unsigned char*)imgR->data()[0];

    int w = imgL->w();
    int h = imgL->h();
    int d = imgL->d();

    // 3. create the output pixel buffer
    auto size = h * w * 3;
    unsigned char* outbuf = (unsigned char*)malloc(size); // NOTE: *cannot* use 'new' here

    // 4. loop x,y thru two images, write diff to output
    for (int y = 0; y < h; y++)
    {
        unsigned long offset = y * w * d; // TODO actual image W
        for (int x = 0; x < w; x++)
        {
            const unsigned char p1L = dataL[offset + 0];
            const unsigned char p2L = dataL[offset + 1];
            const unsigned char p3L = dataL[offset + 2];

            const unsigned char p1R = dataR[offset + 0];
            const unsigned char p2R = dataR[offset + 1];
            const unsigned char p3R = dataR[offset + 2];

            int diff = abs(p1L - p1R + p2L - p2R + p3L - p3R);

            if (diff < 10)
            {
                // set black
                *(outbuf + offset + 0) = 0;
                *(outbuf + offset + 1) = 0;
                *(outbuf + offset + 2) = 0;
            }
            else
            {
                *(outbuf + offset + 0) = p1L;
                *(outbuf + offset + 1) = p2L;
                *(outbuf + offset + 2) = p3L;
            }

            offset += d; // move to next pixel
        }
    }

    _diffImage = new Fl_RGB_Image(outbuf, w, h, 3);
    return true;
}

bool diff(Pair* toview, bool stretch)
{
    // 1. get the two images
    const char* pathL = GetFD(toview->FileLeftDex)->Name->c_str();
    const char* pathR = GetFD(toview->FileRightDex)->Name->c_str();

    Fl_Shared_Image* imgL = Fl_Shared_Image::get(pathL);
    Fl_Shared_Image* imgR = Fl_Shared_Image::get(pathR);

    if (imgL->d() != 3 || imgR->d() != 3)
        return false; // punt on other depths for now

    // Deal with stretch
    if (!stretch)
        return doDiff(imgL, imgR);

    int newH = imgL->h();
    if (imgR->h() > newH)
        newH = imgR->h();
    int newW = imgL->w();
    if (imgR->w() > newW)
        newW = imgR->w();
    Fl_Shared_Image* newImageL = (Fl_Shared_Image *)imgL->copy(newW, newH);
    Fl_Shared_Image* newImageR = (Fl_Shared_Image *)imgR->copy(newW, newH);
    return doDiff(newImageL, newImageR);
}

void showDiff(Pair* toview, bool stretch)
{
    if (!diff(toview, stretch))
        return;
    showView(_diffImage);
    _diffImage->release();
}
