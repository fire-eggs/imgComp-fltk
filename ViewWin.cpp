#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Image_Surface.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#include "ViewWin.h"
#include "Fl_Image_Display.H"
#include "prefs.h"

Fl_Double_Window* _win;
Fl_Image_Display* _disp;
Pair* _viewing;
Fl_RGB_Image* _view;
Fl_Image* _viewL;
Fl_Image* _viewR;

bool _showLeft;
Fl_Button* _bFull; // the 'Fit'/'100%' button
bool zoomFit;
bool diffMode;

Fl_RGB_Image* _diffImageL;
Fl_RGB_Image* _diffImageR;

extern Prefs* _PREFS;

#ifdef DEBUG
bool savetocolorpng( Fl_RGB_Image* src, const char* fpath );
#endif

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
    if (!diffMode)
    {
        const char* path = NULL;

        Fl_Image* img;
        if (_viewing)
        {
            if (_showLeft)
                path = GetFD(_viewing->FileLeftDex)->Name->c_str();
            else
                path = GetFD(_viewing->FileRightDex)->Name->c_str();

            img = Fl_Shared_Image::get(path); // TODO should be loadFile() call?
        }
        else
        {
            img = _showLeft ? _viewL : _viewR;
        }

        if (!img)
            return;

        auto scale = _disp->scale();
        _disp->value(img);
        _disp->scale(scale);

        char buff[1024];
        if (path)
        {
            if (_showLeft)
                sprintf(buff, "Left image: %s", path);
            else
                sprintf(buff, "Right image: %s", path);
        }
        else
        {
            if (_showLeft)
                sprintf(buff, "Left image: ?"); // TODO display of path
            else
                sprintf(buff, "Right image: ?"); // TODO display of path
        }
        _win->label(buff);
    }
    else
    {
        auto scale = _disp->scale();
        if (_showLeft)
        {
            _win->label("Diff: Left vs Right");
            _disp->value((Fl_Shared_Image*)_diffImageL);
        }
        else
        {
            _win->label("Diff: Right vs Left");
            _disp->value((Fl_Shared_Image*)_diffImageR);
        }
        _disp->scale(scale);
        _disp->redraw(); // TODO why is this necessary? Is NOT if label changed
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
    zoomFit = !zoomFit;
    _bFull->label( zoomFit ? "100%" : "Fit" );
    if (!zoomFit)
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

    zoomFit = true; // start in 'fit' mode
    _disp->_scrollbarsOn = false;
    _disp->_drawChecker = false;

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

bool isActive = false;

void reallyShowView()
{
    _win->show();
    ViewImage(); // NOTE: title bar doesn't update until shown
    isActive = true;
    while (_win->shown())
        Fl::wait();
    isActive = false;
}

void showView(Fl_Image* leftI, Fl_Image* rightI, bool startLeft)
{
    if (isActive) return;
    if (!_win)
        makeWin();

    diffMode = false;
    _viewing = NULL;
    _view = NULL;
    _viewL = leftI;
    _viewR = rightI;
    _showLeft = startLeft;
    _disp->scale(zoomFit ? 0.0 : 1.0);

    reallyShowView();
}

void showView(Pair* toview, bool startLeft)
{
    if (isActive) return;
    if (!_win)
        makeWin();

    diffMode = false;
    _viewing = toview;
    _view = NULL;
    _viewL = _viewR = NULL;
    _showLeft = startLeft;
    _disp->scale(zoomFit ? 0.0 : 1.0);
    _disp->label("");
    reallyShowView();
}

void showView()
{
    if (isActive) return;
    if (!_win)
        makeWin();
    _disp->scale(zoomFit ? 0.0 : 1.0);
    _view = _diffImageL;
    _viewing = NULL;
    reallyShowView();
}

void setOutPixel(unsigned char* outbuf, unsigned long offset, int diff, 
    const unsigned char p1, const unsigned char p2, const unsigned char p3)
{
    if (diff < 10)
    {
        // set black
        *(outbuf + offset + 0) = 0;
        *(outbuf + offset + 1) = 0;
        *(outbuf + offset + 2) = 0;
    }
    else
    {
        *(outbuf + offset + 0) = p1;
        *(outbuf + offset + 1) = p2;
        *(outbuf + offset + 2) = p3;
    }
}

unsigned char* outbufL;
unsigned char* outbufR;

bool doDiff(Fl_Image* imgL, Fl_Image* imgR)
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
    size_t size = (size_t)h * (size_t)w * d;
    outbufL = (unsigned char*)malloc(size); // NOTE: *cannot* use 'new' here
    outbufR = (unsigned char*)malloc(size); // NOTE: *cannot* use 'new' here

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

            setOutPixel(outbufL, offset, diff, p1L, p2L, p3L);
            setOutPixel(outbufR, offset, diff, p1R, p2R, p3R);

            offset += d; // move to next pixel
        }
    }

    _diffImageL = new Fl_RGB_Image(outbufL, w, h, d);
    _diffImageR = new Fl_RGB_Image(outbufR, w, h, d);

    return true;
}

Fl_Image *to24bit(Fl_Image* img)
{
    Fl_Image_Surface *imgSurf = new Fl_Image_Surface(img->w(), img->h());
    Fl_Surface_Device::push_current(imgSurf);

    fl_color(FL_WHITE);
    fl_rectf(0,0,img->w(),img->h());    // draw filled rectangle as background

    Fl_Anim_GIF_Image *animgif = dynamic_cast<Fl_Anim_GIF_Image *>(img);
    if (animgif) {
        animgif->frame(0);
        animgif->Fl_Pixmap::draw(0, 0);
    }
    else
        img->draw(0, 0);
    Fl_RGB_Image *ret = imgSurf->image();
    Fl_Surface_Device::pop_current();
    delete imgSurf;
    return ret;
}

bool doDiff1(Fl_Image* imgL, Fl_Image* imgR, bool stretch, bool release)
{
    Fl_Pixmap *pimg = dynamic_cast<Fl_Pixmap *>(imgL);
    if (pimg)
    {
        imgL = to24bit(imgL); // TODO memory leak?
        /*
        Fl_Image_Surface *imgSurf = new Fl_Image_Surface(imgL->w(), imgL->h());
        Fl_Surface_Device::push_current(imgSurf);
        imgL->draw(0, 0);
        imgL = imgSurf->image();
        Fl_Surface_Device::pop_current();
        delete imgSurf;
         */
    }
    pimg = dynamic_cast<Fl_Pixmap *>(imgR);
    if (pimg)
    {
        imgR = to24bit(imgR); // TODO memory leak?
/*
        Fl_Image_Surface *imgSurf = new Fl_Image_Surface(imgR->w(), imgR->h());
        Fl_Surface_Device::push_current(imgSurf);
        imgR->draw(0, 0);
        imgR = imgSurf->image();
        Fl_Surface_Device::pop_current();
        delete imgSurf;
*/
    }

    // Deal with stretch
    if (!stretch)
    {
        return doDiff(imgL, imgR);
    }

    int newH = imgL->h();
    if (imgR->h() > newH)
        newH = imgR->h();
    int newW = imgL->w();
    if (imgR->w() > newW)
        newW = imgR->w();
    Fl_Image* newImageL = imgL->copy(newW, newH);
    Fl_Image* newImageR = imgR->copy(newW, newH);

    auto res = doDiff(newImageL, newImageR);
    newImageL->release();
    newImageR->release();
    return res;
}

bool diff(bool stretch, Fl_Image* imgL, Fl_Image* imgR)
{
    if (imgL->d() != imgR->d() || imgL->d() == 4)
    {
        // Do this for 32-bit images as well, because black line drawings w/transparency goto solid black otherwise
//        fl_alert("Mixed image depths");

        Fl_Image *newL = to24bit(imgL);
        Fl_Image *newR = to24bit(imgR);
#ifdef DEBUG
        savetocolorpng((Fl_RGB_Image *)newL,"/home/kevin/temp/mixL.png");
        savetocolorpng((Fl_RGB_Image *)newR,"/home/kevin/temp/mixR.png");
#endif

        bool res = doDiff1(newL, newR, stretch, false);

        delete newL;
        delete newR;

        return res;
    }

    return doDiff1(imgL, imgR, stretch, false);
}

bool diff(Pair* toview, bool stretch)
{
    // 1. get the two images
    const char* pathL = GetFD(toview->FileLeftDex)->Name->c_str();
    const char* pathR = GetFD(toview->FileRightDex)->Name->c_str();

    Fl_Shared_Image* imgL = Fl_Shared_Image::get(pathL);
    if (!imgL)
        return false;
    Fl_Shared_Image* imgR = Fl_Shared_Image::get(pathR);
    if (!imgR)
    {
        imgL->release();
        return false;
    }

    if (imgL->d() != imgR->d())
    {
        imgL->release();
        imgR->release();
        return false; // punt on other depths for now
    }

    bool res = doDiff1(imgL, imgR, stretch, true);

    imgL->release();
    imgR->release();
    return res;
}

void showDiff(Pair* toview, bool stretch)
{
    if (isActive) return;
    
    if (!diff(toview, stretch))
        return;
    diffMode = true;
    showView();
    _diffImageL->release();
    _diffImageR->release();
    free(outbufL);
    free(outbufR);
    outbufL = outbufR = NULL;
}

void showDiff(Fl_Image* imgL, Fl_Image* imgR, bool stretch)
{
    if (isActive) return;

    if (!diff(stretch, imgL, imgR))
        return;
    diffMode = true;
    showView();
    _diffImageL->release();
    _diffImageR->release();
    free(outbufL);
    free(outbufR);
    outbufL = outbufR = NULL;
    _disp->value(0);
}

#ifdef DEBUG
#include "png/png.h"
#include <stdio.h>

bool savetocolorpng( Fl_RGB_Image* src, const char* fpath )
{
    if ( src == NULL )
        return false;

    if ( src->d() < 3 )
        return false;

    FILE* fp = fopen( fpath, "wb" );

    if ( fp == NULL )
        return false;

    png_structp png_ptr     = NULL;
    png_infop   info_ptr    = NULL;
    png_bytep   row         = NULL;

    png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
    if ( png_ptr != NULL )
    {
        info_ptr = png_create_info_struct( png_ptr );
        if ( info_ptr != NULL )
        {
            if ( setjmp( png_jmpbuf( (png_ptr) ) ) == 0 )
            {
                int mx = src->w();
                int my = src->h();
                int pd = 3;

                png_init_io( png_ptr, fp );
                png_set_IHDR( png_ptr,
                              info_ptr,
                              mx,
                              my,
                              8,
                              PNG_COLOR_TYPE_RGB,
                              PNG_INTERLACE_NONE,
                              PNG_COMPRESSION_TYPE_BASE,
                              PNG_FILTER_TYPE_BASE);

                png_write_info( png_ptr, info_ptr );

                row = (png_bytep)malloc( src->w() * sizeof( png_byte ) * 3 );
                if ( row != NULL )
                {
                    const char* buf = src->data()[0];
                    int bque = 0;

                    for( int y=0; y<my; y++ )
                    {
                        for( int x=0; x<mx; x++ )
                        {
                            row[ (x*3) + 0 ] = buf[ bque + 0 ];
                            row[ (x*3) + 1 ] = buf[ bque + 1 ];
                            row[ (x*3) + 2 ] = buf[ bque + 2 ];
                            bque += pd;
                        }

                        png_write_row( png_ptr, row );
                    }

                    png_write_end( png_ptr, NULL );

                    fclose( fp );

                    free(row);
                }

                png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
                png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

                return true;
            }
        }
    }

    fclose(fp);
    return false;
}
#endif
