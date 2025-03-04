#pragma GCC diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_File_Chooser.H>
//#include <FL/Fl_Shared_Image.H>
#include "SharedImageExt.h"

#include "Fl_Image_Display.H"
#include "filedata.h"
#include "ViewWin.h"
#include "prefs.h"
#include "MostRecentPaths.h"

#ifdef _WIN32
#else
#include <unistd.h> // getcwd
#endif

#include "events.h"
#include "logging.h"

void load_cb(Fl_Widget*, void*);
void quit_cb(Fl_Widget*, void*);
void clear_cb(Fl_Widget*, void*);
void filter_cb(Fl_Widget*, void*);
void filterR_cb(Fl_Widget*, void*);
void lockL_cb(Fl_Widget*, void*);
void lockR_cb(Fl_Widget*, void*);
void viewLog_cb(Fl_Widget*, void*);
void copyToClip_cb(Fl_Widget*, void*);
void copyBulk_cb(Fl_Widget*, void*);
void clear_controls();
void updateBoxImages();
void onListClick(Fl_Widget* w, void* d);
void cbSupercedeLeft(Fl_Widget *,void*);
void cbDanbooruHideLeft(Fl_Widget *,void*);

Prefs* _PREFS;

// TODO push into MainWin class?
int sourceId;
bool filterSame;
bool filterRotation;

Fl_Browser* _listbox;
Fl_Box* _leftImgView;
Fl_Box* _rightImgView;
Fl_Group* _btnBox;
Fl_Button* _btnLDup;
Fl_Button* _btnRDup;
Fl_Button* _btnDiff;
MostRecentPaths* _mrp;
Fl_Menu_Bar* _menu;

char *_loadfile; // hacky

Fl_Image* _leftImage;
Fl_Image* _rightImage;

int widths[] = {50, 340, 340, 0};

// NOTE: relying on toggle menus auto initialized to OFF

class NPBtn : public Fl_Button
{
    /*
     * This class is a hack to pass the up/down arrows directly to the listbox.
     * When the user presses the Next/Prev buttons, focus is on the button; the
     * next up/down arrow presses need to be done twice to move within the listbox.
     * This class fixes that.
     */
    void (*downCB)(Fl_Widget *, void*);
    void (*upCB)(Fl_Widget *, void*);

public:
    NPBtn(int x, int y, int w, int h, const char *l = nullptr) : Fl_Button(x,y,w,h,l)
    {
        downCB = nullptr;
        upCB   = nullptr;
    }

    void setDown( void (*cb)(Fl_Widget *,void*) ) { downCB = cb; }
    void setUp( void (*cb)(Fl_Widget *,void*) ) { upCB = cb; }

    int handle(int event) override
    {
        int res = Fl_Button::handle(event);
        if (event == FL_KEYDOWN)
        {
            auto key = Fl::event_key();
            if (key == FL_Up)
            {
                upCB(nullptr, nullptr);
                return 0;
            }
            if (key == FL_Down && downCB)
            {
                downCB(nullptr, nullptr);
                return 0;
            }
        }
        return res;
    }
};

class MainWin : public Fl_Double_Window
{
public:
    MainWin(int x, int y, int w, int h) : Fl_Double_Window(x, y, w,h)
    {}

    void resize(int, int, int, int) override;
    int handle(int event) override;
    void do_menu();
};

MainWin* _window;

#define BTN_BOX_HALFHIGH 16
#define BTN_BOX_HIGH (BTN_BOX_HALFHIGH * 2)
#define BTN_HIGH (BTN_BOX_HIGH - 6)

void MainWin::resize(int x, int y, int w, int h)
{
    int oldw = this->w();
    int oldh = this->h();
    Fl_Double_Window::resize(x, y, w, h);
    if (w == oldw && h == oldh)
        return; // merely moving window

    // size children as desired
    int newhigh = (int)(h * 0.4 - BTN_BOX_HALFHIGH);
    _listbox->resize(_listbox->x(), _listbox->y(), w, newhigh);
    int newy = _listbox->y() + newhigh + BTN_BOX_HIGH;
    _btnBox->resize(0, newy - BTN_BOX_HIGH, w, BTN_BOX_HIGH);
    newhigh = h - newy; // (int)(h * 0.6 - BTN_BOX_HALFHIGH);
    _leftImgView->resize(0, newy, w / 2-1, newhigh);
    _rightImgView->resize(w/2+1, newy, w / 2-1, newhigh);
    updateBoxImages();

    widths[1] = (w - widths[0]) / 2;
    widths[2] = widths[1];
    _listbox->column_widths(widths);
    
    _PREFS->setWinRect(MAIN_PREFIX, x, y, w, h);
}

int MainWin::handle(int event)
{
    // TODO double-click on img boxes?
    return Fl_Double_Window::handle(event);
}

enum
{
    MI_FILE,
    MI_LOAD,
    MI_CLEAR,
    MI_EXIT,

    MI_FAVS, // Must be last before MI_FAVx
    MI_FAV0,
    MI_FAV1,
    MI_FAV2,
    MI_FAV3,
    MI_FAV4,
    MI_FAV5,
    MI_FAV6,
    MI_FAV7,
    MI_FAV8,
    MI_FAV9,
};

void updateMRU();

static void MenuCB(Fl_Widget* window_p, void *userdata)
{
    long choice = (long)(size_t)userdata;
    switch (choice)
    {
    case MI_LOAD:
        load_cb(NULL, NULL);
        break;

    case MI_CLEAR:
        clear_cb(NULL, NULL);
        break;

    case MI_EXIT:
        quit_cb(NULL, NULL);
        break;

    case MI_FAV0:
    case MI_FAV1:
    case MI_FAV2:
    case MI_FAV3:
    case MI_FAV4:
    case MI_FAV5:
    case MI_FAV6:
    case MI_FAV7:
    case MI_FAV8:
    case MI_FAV9:
        {
            int path = choice - MI_FAV0;
            char** favs = _mrp->getAll();
            _loadfile = favs[path];

            _mrp->Add(_loadfile); // add "new" path
            _mrp->Save();
            updateMRU(); // update the menu

            clear_controls();
            Fl::flush();

            fire_proc_thread();

        }
        break;
    }
}

Fl_Menu_Item mainmenuItems[] =
{
    {"&File", 0, 0, 0, FL_SUBMENU},
    {"&Load...", 0, load_cb},
    {"&Clear", 0, clear_cb},
    {"Last Used", 0, 0, 0, FL_SUBMENU},
    {0},
    {"E&xit", 0, quit_cb},
    {0},

    {"&Options", 0, 0, 0, FL_SUBMENU},
    {"Fi&lter Same Phash", 0, filter_cb, 0, FL_MENU_TOGGLE},
    {"Filter Rotation Matches", 0, filterR_cb, 0, FL_MENU_TOGGLE},
    {"Lock Left Dup Button", 0, lockL_cb, 0, FL_MENU_TOGGLE},
    {"Lock Right Dup Button", 0, lockR_cb, 0, FL_MENU_TOGGLE},
    {"View log file ...", 0, viewLog_cb, 0},
    {"Copy filenames to clipboard", 0, copyToClip_cb, 0},
    {"Copy all data to clipboard", 0, copyBulk_cb, 0},
    {0},

    {"&Actions", 0, 0, 0, FL_SUBMENU},
    {"Supercede left", 0, cbSupercedeLeft, 0},
    {"Danbooru Hide Left", 0, cbDanbooruHideLeft, 0},
    {0},

    {0}
};

void MainWin::do_menu()
{
    // 1. find the submenu in the "master" menu
    int i = _menu->find_index("&File/Last Used");
    if (i < 0)
        return;
    _menu->clear_submenu(i);

    size_t numfavs = _mrp->getCount();

    Fl_Menu_Item* mi = (Fl_Menu_Item *)_menu->find_item("&File/Last Used");
    if (!numfavs)
    {
        mi->deactivate();
        return;
    }

    mi->activate();
    char** favs = _mrp->getAll();
    char s[1000];
    for (int j = 0; j < numfavs; j++)
    {
        sprintf(s, "&File/Last Used/%d", j);
        int newdex = _menu->add((const char *)s, (const char *)NULL, MenuCB, 0);
        Fl_Menu_Item& item = (Fl_Menu_Item&)_menu->menu()[newdex];
        //mi = (Fl_Menu_Item *)_menu->find_item(s);
        item.label(favs[j]);
        item.argument(MI_FAV0 + j);
    }
}

void updateMRU()
{
    _window->do_menu();
}

Pair *GetCurrentPair()
{
    // Find the image pair currently selected in the listbox (null if none)
    
    int line = _listbox->value();
    if (line == 0)
        return NULL;

    int data = (intptr_t)_listbox->data(line);

    Pair* p = GetPair(data);
    return p;
}

void load_pairview()
{
    size_t count = GetPairCount();
    if (count < 1)
    {
        _listbox->add("No candidates found", NULL);
//        printf("Empty list\n");
    }

    for (int i = 0; i < count; i++)
    {
        if (GetPair(i)->valid)
        {
            char* txt = GetPairText(i);
            _listbox->add(txt, GetPairData(i));
            delete [] txt;
        }
    }
    _listbox->make_visible(1);
    _listbox->redraw();
}

void reloadPairview()
{
    _listbox->clear(); // TODO is a full rebuild necessary?
    load_pairview();
}

double getNiceFileSize(const char *path)
{
    struct stat st;
    stat(path, &st);
    auto sizeK = st.st_size / 1024.0;
    return sizeK;
}

void updateTitle(const char *pathL, Fl_Image *imgL, const char *pathR, Fl_Image *imgR)
{
    int ilw = imgL->w();
    int ilh = imgL->h();
    int ild = imgL->d() * 8;
    int irw = imgR->w();
    int irh = imgR->h();
    int ird = imgR->d() * 8;

    auto fsl = getNiceFileSize(pathL);
    auto fsr = getNiceFileSize(pathR);
    
    char buff[256];
    snprintf(buff, 256, "(%d,%dx%d)[%2gK] : (%d,%dx%d)[%2gK]", ilh, ilw, ild, fsl, irh, irw, ird, fsr);
    _window->label(buff);
    
    // Update the state of Diff button depending on image dimensions
    if (ilw == irw && ilh == irh)
        _btnDiff->activate();
    else
        _btnDiff->deactivate();
    _btnDiff->redraw();
}

void updateBoxImages()
{
    if (!_leftImage || !_rightImage)
        return;

    // TODO size images proportionally to view size
    // both images exist
    int iw = _leftImgView->w();
    int ih = _leftImgView->h();

    if (_leftImgView->image())
        _leftImgView->image()->release();
    if (_rightImgView->image())
        _rightImgView->image()->release();

//    _leftImgView->image(_leftImage->image()->copy(iw, ih));
//    _rightImgView->image(_rightImage->image()->copy(iw, ih));
    _leftImgView->image(_leftImage->copy(iw, ih));
    _rightImgView->image(_rightImage->copy(iw, ih));

    _leftImgView->redraw();
    _rightImgView->redraw();
}

bool isAnimated(Fl_Image *img)
{
    // Determine if an image is animated or not.
    // NOTE: relies on the animated webp having been converted to an animated gif
    if (!img)
        return false;
    Fl_Anim_GIF_Image *gif = dynamic_cast<Fl_Anim_GIF_Image*>(img);
    if (!gif)
        return false;
    return gif->is_animated();
}


void onListClick(Fl_Widget* w, void* d)
{
    if (_listbox->size() < 1) // list is now empty, done
        return;

    int line = _listbox->value();
    if (!line)
        line = 1; // when advancing thru the list below reaches bottom, selection is set to none.

    int data = (intptr_t)_listbox->data(line);
    size_t max = GetPairCount();
    if (data >= max)
        return;

    Pair* p = GetPair(data);
    //const char* pathL = GetFD(p->FileLeftDex)->Name->c_str();
    //const char* pathR = GetFD(p->FileRightDex)->Name->c_str();
    const char* pathL = GetActualPath(p, true);
    const char* pathR = GetActualPath(p, false);

    // release existing image(s)
    if (_leftImage) { _leftImage->release(); _leftImage = NULL; }
    if (_rightImage) { _rightImage->release(); _rightImage = NULL; }

    _leftImage = loadFile((char *)pathL,_leftImgView); //SharedImageExt::LoadImage(pathL);
    _rightImage = loadFile((char *)pathR,_rightImgView); //SharedImageExt::LoadImage(pathR);

    // .phashc file is supposed to have a flag indicating the image is animated or not.
    // right now 20220612 that is not the case. discard pair when mismatch of animation   
    bool leftAnim = isAnimated(_leftImage);
    bool rightAnim = isAnimated(_rightImage);
    
    // _leftImage or _rightImage may be null [file missing]
    // Force selection of next entry
    if (!_leftImage || !_rightImage || (leftAnim != rightAnim))
    {
        p->valid = false;

        // TODO do this w/o recursion!
        // TODO needs to be done in the viewlist ???
        _listbox->select(line + 1, 1); // NOTE: does NOT force 'onclick' event
        _listbox->remove(line);
        _listbox->redraw();
        onListClick(0, 0); // force onclick
        return;
    }

    //updateTitle(pathL, _leftImage->baseImage(), pathR, _rightImage->baseImage());
    updateTitle(pathL, _leftImage, pathR, _rightImage);

    updateBoxImages();
  
    // ensure the current line is up a little bit - can't click to get to next line sometimes
    _listbox->bottomline(line + 5);
}

void btnDup(bool left)
{
    // Common code for the two 'Dup' buttons
    // rename one of the images as a duplicate of the other
    // bool left : rename the 'left' image

    Pair* p = GetCurrentPair();
    if (!p)
        return;
    
    //__debugbreak();

    auto pathL = GetFD(p->FileLeftDex)->Name->c_str();
    auto pathR = GetFD(p->FileRightDex)->Name->c_str();
    auto target = left ? pathR : pathL;
    auto source = left ? pathL : pathR;
    
    // Trying for <srcpath>/<srcfile> to <srcpath>/dup0_<destfile>
    if (MoveFile("%s/dup%d_%s", target, source))
    {
        int oldsel = _listbox->value();
        RemoveMissingFile(left ? p->FileLeftDex : p->FileRightDex);
		p->valid = false;
        reloadPairview();
        _listbox->select(oldsel);
        onListClick(0, 0); // force onclick 
        _listbox->take_focus(); // focus back to listbox
    }
}

void btnDupL_cb(Fl_Widget* w, void* d)
{
    btnDup(true);
}

void btnDupR_cb(Fl_Widget* w, void* d)
{
    btnDup(false);
}

void btnView(bool left)
{
    // activate the view window with the plain images.
    // bool left: start with the 'left' image
    
    Pair* p = GetCurrentPair();
    if (!p)
        return;
    //showView(_leftImage->baseImage(), _rightImage->baseImage(), left);
    showView(_leftImage, _rightImage, left);
    //showView(p, left);
    
    _listbox->take_focus(); // so user doesn't lose their place: focus back to listbox
}

void btnViewL_cb(Fl_Widget* w, void* d)
{
    btnView(true);
}

void btnViewR_cb(Fl_Widget* w, void* d)
{
    btnView(false);
}

void btnHide(bool left)
{
    // current selection, "hide" file by removing from all pairs
    Pair* p = GetCurrentPair();
    if (!p)
        return;

    int oldsel = _listbox->value();
    RemoveMissingFile(left ? p->FileLeftDex : p->FileRightDex);
    p->valid = false;
    reloadPairview();
    _listbox->select(oldsel);
    onListClick(0, 0); // force onclick
    _listbox->take_focus(); // focus back to listbox
}

void btnHideL_cb(Fl_Widget* w, void* d)
{
    btnHide(true);
}

void btnHideR_cb(Fl_Widget* w, void* d)
{
    btnHide(false);
}

void btnDiff(bool left, bool stretch)
{
    Pair* p = GetCurrentPair();
    if (!p)
        return;
    //showDiff(_leftImage->image(), _rightImage->image(), stretch);
    showDiff(_leftImage, _rightImage, stretch);
    //showDiff(p, stretch);

    _listbox->take_focus(); // so user doesn't lose their place: focus back to listbox
}

void btnDiff_cb(Fl_Widget *, void *)
{
    btnDiff(true, false);
}

void btnDiffS_cb(Fl_Widget*, void*)
{
    btnDiff(true, true);
}

void btnNext_cb(Fl_Widget*, void*)
{
    if (_listbox->size() < 1) // list is now empty, done
        return;

    int line = _listbox->value();
    if (!line)
        line = 1; // when advancing thru the list below reaches bottom, selection is set to none.
    _listbox->select(line + 1, 1); // NOTE: does NOT force 'onclick' event
    onListClick(0, 0);
}

void btnPrev_cb(Fl_Widget*, void*)
{
    if (_listbox->size() < 1) // list is now empty, done
        return;

    int line = _listbox->value();
    if (!line)
        line = 1; // when advancing thru the list below reaches bottom, selection is set to none.
    _listbox->select(line - 1, 1); // NOTE: does NOT force 'onclick' event
    onListClick(0, 0);

}

void clear_controls()
{
    // used on load, Clear menu
    _listbox->clear();

    if (_leftImgView->image())
        ((Fl_Shared_Image *)_leftImgView->image())->release();
    if (_rightImgView->image())
        ((Fl_Shared_Image*)_rightImgView->image())->release();
    
    _leftImgView->image(NULL);
    _rightImgView->image(NULL);
    _leftImgView->redraw();
    _rightImgView->redraw();
}

void clear_cb(Fl_Widget* w, void* d)
{
    // wipe all
    sourceId = 0;

    clear_controls();
    ClearPairList();
    ClearData();

    _window->label("");
}

void filter_cb(Fl_Widget* w, void* d)
{
    filterSame = !filterSame;

    clear_controls();
    FilterAndSort(filterSame, filterRotation);
    load_pairview();
    _listbox->select(1);
    onListClick(0, 0);
}

void filterR_cb(Fl_Widget*, void*)
{
    filterRotation = !filterRotation;
    clear_controls();
    FilterAndSort(filterSame, filterRotation);
    load_pairview();
    _listbox->select(1);
    onListClick(0, 0);
}
void lockL_cb(Fl_Widget* w, void* d)
{
    // if left 'Dup' button enabled, disable it
    if (_btnLDup->active())
        _btnLDup->deactivate();
    else
        _btnLDup->activate();
}

void lockR_cb(Fl_Widget* w, void* d)
{
    // if right 'Dup' button enabled, disable it
    if (_btnRDup->active())
        _btnRDup->deactivate();
    else
        _btnRDup->activate();
}

// TODO stolen from FLTK source... genericize?
static void popup(Fl_File_Chooser* filechooser) 
{
    filechooser->show();

    // deactivate Fl::grab(), because it is incompatible with modal windows
    Fl_Window* g = Fl::grab();
    if (g) Fl::grab(0);

    while (filechooser->shown())
        Fl::wait();

    if (g) // regrab the previous popup menu, if there was one
        Fl::grab(g);
}

void load_cb(Fl_Widget* , void* )
{
    char filename[1024] = "";
    Fl_File_Chooser* choose = new Fl_File_Chooser(filename, "*.phashc", 0, "Open phash file");
    choose->preview(false); // force preview off
    popup(choose);
    _loadfile = (char*)choose->value();

    // TODO can't control file_chooser location because underlying window is not exposed by the class

    //loadfile = fl_file_chooser("Open phash file", "*.phashc", filename);
    if (!_loadfile)
        return;

    _mrp->Add(_loadfile); // add new path
    _mrp->Save();
    updateMRU(); // update the menu

    clear_controls();
    Fl::flush();
    
    fire_proc_thread();
}

void quit_cb(Fl_Widget* , void* )
{
    exit(0);
}

void viewLog_cb(Fl_Widget* , void* )
{
    showLog();
}

void cbSupercedeLeft(Fl_Widget *,void*)
{
    // The "left" image is to be superceded by the "right".
    // 1. Mark the left image as dup.
    // 2. Move "right.x" to "left.x".

    Pair* p = GetCurrentPair();
    if (!p)
        return;
    auto pathL = GetFD(p->FileLeftDex)->Name->c_str();
    auto pathR = GetFD(p->FileRightDex)->Name->c_str();

    if (MoveFile("%s/dup%d_%s", pathR, pathL))
    {
        auto rightext = fl_filename_ext(pathR);
        auto newleft = fl_filename_setext((char *) pathL, rightext);

        log("Supercede \n%s with \n%s", newleft, pathR);
        int res = cp(newleft, pathR);
        if (res)
            log("cbSupercedeLeft: failed to cp %s to %s (%d)[%s]", pathR, newleft, errno, strerror(errno));
        else
            btnDup(false); // mark "old" right as dup
    }
}

void cbDanbooruHideLeft(Fl_Widget *, void *)
{
    // The "left" image is to be hidden by Danbooru rules. This currently
    // requires a hard-coded destination tree for the "hidden".
    // TODO consider unix-style hidden?

    std::string hiddenPath = "/mnt/trolle/db2020_aside/hidden/";

    Pair* p = GetCurrentPair();
    if (!p)
        return;
    std::string pathL = GetFD(p->FileLeftDex)->Name->c_str();

    auto extpos = pathL.rfind('.');
    auto digits = pathL.substr(extpos-3, 3);
    auto fnpos = pathL.rfind('/');
    auto filename = pathL.substr(fnpos+1);
    auto destpath = hiddenPath + "0" + digits + "/" + filename;

    log("Hide left: \n%s to \n%s", pathL.c_str(), destpath.c_str());
   
    int res = cp(destpath.c_str(), pathL.c_str());
    if (res)
        log("cbDanbooruHideLeft: failed to cp %s to %s (%d)[%s]", pathL.c_str(), destpath.c_str(), errno, strerror(errno));
    else
        btnDup(true);
}

void copyToClip_cb(Fl_Widget*, void*)
{
    Pair* p = GetCurrentPair();
    if (!p)
        return;

    auto pathL = GetFD(p->FileLeftDex)->Name->c_str();
    auto pathR = GetFD(p->FileRightDex)->Name->c_str();

    size_t size = strlen(pathL) + strlen(pathR) + 2; // +1 for newline, +1 for trailing zero
    char* buff = (char *)malloc(size);
    if (buff)
    {
        sprintf(buff, "%s\n%s", pathL, pathR);
        Fl::copy(buff, (int)size, 2, Fl::clipboard_plain_text);
        free(buff);
    }
}

void copyBulk_cb(Fl_Widget*, void*)
{
    int res = fl_ask("This will take a long time, are you sure?");
    if (res == 0)
        return;

    size_t count = GetPairCount();
    size_t len = 0;
    for (int i = 0; i < count; i++)
    {
        char* txt = GetPairText(i);
        len += strlen(txt);
        delete txt;
    }
    size_t size = len + 3 + count;  // extra newline per line
    char* buff = (char*)malloc(size);
    if (!buff)
        return;
    buff[0] = '\0';
    for (int i = 0; i < count; i++)
    {
        char* txt = GetPairText(i);
        strcat(buff, txt);
        strcat(buff, "\n");
        delete txt;
    }

    Fl::copy(buff, size, 2, Fl::clipboard_plain_text);
    free(buff);

}

int handleSpecial(int event)
{
    switch (event)
    {       
        case KBR_START_LOAD:
            _window->label("Loading...");
            Fl::awake();
            break;
        case KBR_START_COMPARE:
            _window->label("Diffing...");
            Fl::awake();
            break;
        case KBR_START_SORT:
            _window->label("Sorting...");
            Fl::awake();
            break;
        case KBR_DONE_LOAD:
            _window->label("Ready!");
            Fl::awake();
            load_pairview();
            _listbox->select(1);
            //onListClick(0,0);
            Fl::awake();
            //Fl::flush(); // do NOT flush here! the GUI dies
            break;
            
        default:
            return 0;
    }
    return 1;
}

#if false
FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
{
    return _iob;
}
#endif

int main(int argc, char** argv)
{
    initlog("imgcomp.log");
    Fl::lock();
    setlocale(LC_ALL, "C");

    // use remembered main window size
    _PREFS = new Prefs();  
    int x, y, w, h;
    _PREFS->getWinRect(MAIN_PREFIX, x, y, w, h);
    
    _mrp = new MostRecentPaths(_PREFS->_prefs);

    filterSame = false;
    filterRotation = false;

    fl_register_images();
    Fl_Image_Display::set_gamma(2.2f);
#ifdef ANIMGIF
    Fl_Anim_GIF_Image::animate = false;
#endif

    // TODO : use actual size when building controls?
    MainWin window(x, y, w, h);

    _menu = new Fl_Menu_Bar(0, 0, window.w(), 25);
    _menu->copy(mainmenuItems);
    updateMRU();

    _listbox = new Fl_Hold_Browser(0, 25, window.w(), 250);
    _listbox->color(FL_BACKGROUND_COLOR, FL_GREEN);
    _listbox->callback(onListClick);
    _listbox->column_char('|');
    _listbox->column_widths(widths); // TODO how to calculate 0-th width to fit "DUP" at selected font and size?

    // TODO options to set these
    //Fl_Fontsize blah = _listbox->textsize(); // 14
    _listbox->textsize(14);
    //_listbox->textfont(FL_COURIER);

    window.resizable(_listbox);

#define BTNBOXY 235
    _btnBox = new Fl_Group(0, BTNBOXY, window.w(), BTN_BOX_HIGH);
    _btnBox->begin();

    int btnX = 5;
    _btnLDup = new Fl_Button(5, BTNBOXY+3, 50, BTN_HIGH);
    _btnLDup->label("Dup");
    _btnLDup->callback(btnDupL_cb);
    
    btnX += 55;
    Fl_Button *btnLView = new Fl_Button(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    btnLView->label("View");
    btnLView->callback(btnViewL_cb);

    btnX += 55;
    Fl_Button *btnLHide = new Fl_Button(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    btnLHide->label("Hide");
    btnLHide->callback(btnHideL_cb);

    btnX += 55 + 15;
    _btnDiff = new Fl_Button(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    _btnDiff->label("Diff");
    _btnDiff->callback(btnDiff_cb);

    btnX += 55;
    Fl_Button *btnDiffS = new Fl_Button(btnX, BTNBOXY + 3, 100, BTN_HIGH);
    btnDiffS->label("Diff - Stretch");
    btnDiffS->callback(btnDiffS_cb);

    btnX += 105 + 15;
    _btnRDup = new Fl_Button(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    _btnRDup->label("Dup");
    _btnRDup->callback(btnDupR_cb);

    btnX += 55;
    Fl_Button* btnRView = new Fl_Button(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    btnRView->label("View");
    btnRView->callback(btnViewR_cb);

    btnX += 55;
    Fl_Button *btnRHide = new Fl_Button(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    btnRHide->label("Hide");
    btnRHide->callback(btnHideR_cb);

    btnX += 55 + 15;
    auto btnPrev = new NPBtn(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    btnPrev->label("Prev");
    btnPrev->callback(btnPrev_cb);
    btnPrev->setDown(btnNext_cb);  // pass the down arrow directly to the listbox
    btnPrev->setUp(btnPrev_cb);    // pass the up arrow directly to the listbox

    btnX += 55;
    auto btnNext = new NPBtn(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    btnNext->label("Next");
    btnNext->callback(btnNext_cb);
    btnNext->setDown(btnNext_cb);
    btnNext->setUp(btnPrev_cb);

    btnX += 60;
    // An invisible widget to be resizable instead of anything else
    // in the toolbar. NOTE: *must* be the last in the X list
    Fl_Box* invisible = new Fl_Box(btnX, BTNBOXY + 3, 10, BTN_HIGH);
    invisible->hide();
    Fl_Group::current()->resizable(invisible);

    _btnBox->end();

    _leftImgView = new Fl_Box(0, BTNBOXY + BTN_BOX_HIGH, window.w() / 2-1, 250);
    _leftImgView->box(FL_UP_BOX);
    _leftImgView->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    _rightImgView = new Fl_Box(window.w()/2+1, 275, window.w() / 2-1, 250);
    _rightImgView->box(FL_DOWN_BOX);
    _rightImgView->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

#ifdef _DEBUG
    _leftImgView->color(FL_RED);
    _rightImgView->color(FL_BLUE);
#endif

    initShow();

    window.resize(x,y,w+1,h-1); // Hack: force everything to adjust to actual size
    window.resize(x, y, w, h);

    _leftImage = _rightImage = NULL;

    _window = &window;
    Fl::add_handler(handleSpecial); // handle internal events
    
    window.show(argc, argv);
    return Fl::run();

}

void message_cb(void *data) {
    _window->label((const char *)data);
    Fl::flush();
}

#pragma GCC diagnostic pop
