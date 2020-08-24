#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Shared_Image.H>

#include "filedata.h"
#include "ViewWin.h"
#include "prefs.h"

#include <unistd.h> // getcwd

#include "events.h"

Prefs* _PREFS;

// TODO push into MainWin class?
int sourceId;
bool filterSame;
Fl_Browser* _listbox;
Fl_Box* _leftImgView;
Fl_Box* _rightImgView;
Fl_Group* _btnBox;
Fl_Button* _btnLDup;
Fl_Button* _btnRDup;
Fl_Button* _btnDiff;
char *loadfile; // hacky

extern char *_logpath; // hacky

int widths[] = {40, 340, 340, 0};

// NOTE: relying on toggle menus auto initialized to OFF

class MainWin : public Fl_Double_Window
{
public:
    MainWin(int x, int y, int w, int h) : Fl_Double_Window(x, y, w,h)
    {}

    void resize(int, int, int, int) override;
    int handle(int event) override;

};

MainWin* _window;

#define BTN_BOX_HALFHIGH 16
#define BTN_BOX_HIGH BTN_BOX_HALFHIGH * 2
#define BTN_HIGH BTN_BOX_HIGH - 6

void MainWin::resize(int x, int y, int w, int h)
{
    Fl_Double_Window::resize(x, y, w, h);

    // size children as desired
    int newhigh = (int)(h * 0.4 - BTN_BOX_HALFHIGH);
    _listbox->resize(_listbox->x(), _listbox->y(), w, newhigh);
    int newy = _listbox->y() + newhigh + BTN_BOX_HIGH;
    _btnBox->resize(0, newy - BTN_BOX_HIGH, w, BTN_BOX_HIGH);
    newhigh = (int)(h * 0.6 - BTN_BOX_HALFHIGH);
    _leftImgView->resize(0, newy, w / 2, newhigh);
    _rightImgView->resize(w/2, newy, w / 2, newhigh);

    widths[1] = (w - 40) / 2;
    widths[2] = widths[1];
    _listbox->column_widths(widths);
    
    _PREFS->setWinRect(MAIN_PREFIX, x, y, w, h);
}

int MainWin::handle(int event)
{
    // TODO double-click on img boxes?
    return Fl_Double_Window::handle(event);
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

void load_listbox()
{
    size_t count = GetPairCount();
    if (count < 1)
    {
        _listbox->add("No candidates found", NULL);
        printf("Empty list\n");
    }

    for (int i = 0; i < count; i++)
    {
        _listbox->add(GetPairText(i), GetPairData(i));
    }
    _listbox->make_visible(1);
    _listbox->redraw();
}

void ReloadListbox()
{
    _listbox->clear();
    load_listbox();
}

double getNiceFileSize(const char *path)
{
    struct stat st;
    stat(path, &st);
    auto sizeK = st.st_size / 1024.0;
    return sizeK;
}

void updateTitle(const char *pathL, Fl_Shared_Image *imgL, const char *pathR, Fl_Shared_Image *imgR)
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
}

void onListClick(Fl_Widget* w, void* d)
{
    if (_listbox->size() < 1) // list is now empty, done
        return;
    
    int line = _listbox->value();
    if (!line) 
        line = 1; // when advancing thru the list below reaches bottom, selection is set to none.
        
    int data = (intptr_t)_listbox->data(line);
//    printf("LB: val: %d data:%d\n", line, data);

    Pair* p = GetPair(data);
    const char* pathL = GetFD(p->FileLeftDex)->Name->c_str();
    const char* pathR = GetFD(p->FileRightDex)->Name->c_str();

//     printf("PathL: %s\n", pathL);
//     printf("PathR: %s\n", pathR);

    Fl_Shared_Image* imgL = Fl_Shared_Image::get(pathL);
    Fl_Shared_Image* imgR = Fl_Shared_Image::get(pathR);

    // imgL or imgR may be null [file missing]
    // Force selection of next entry
    if (!imgL || !imgR)
    {
        // TODO do this w/o recursion!
        if (imgL) imgL->release();
        if (imgR) imgR->release();
        _listbox->select(line + 1, 1); // NOTE: does NOT force 'onclick' event
        _listbox->remove(line);
        _listbox->redraw();
        onListClick(0, 0); // force onclick
        return;
    }

    // TODO size images proportionally to view size
    // both images exist
    int iw = _leftImgView->w();
    int ih = _leftImgView->h();

    // TODO memory leak?
//     if (_leftImgView->image())
//         _leftImgView->image()->release();
//     if (_rightImgView->image())
//         _rightImgView->image()->release();
    _leftImgView->image(imgL->copy(iw, ih));
    _rightImgView->image(imgR->copy(iw,ih));

    updateTitle(pathL, imgL, pathR, imgR);
    
    imgL->release();
    imgR->release();

    _leftImgView->redraw();
    _rightImgView->redraw();
    
    // TODO ensure the current line is up a little bit - can't click to get to next line sometimes
}

void btnDup(bool left)
{
    // rename one of the images as a duplicate of the other
    // bool left : rename the 'left' image

    Pair* p = GetCurrentPair();
    if (!p)
        return;
    
    auto pathL = GetFD(p->FileLeftDex)->Name->c_str();
    auto pathR = GetFD(p->FileRightDex)->Name->c_str();
    auto target = left ? pathR : pathL;
    auto source = left ? pathL : pathR;
    
    // Trying for <srcpath>/<srcfile> to <srcpath>/dup0_<destfile>
    if (MoveFile("%s/dup%d_%s", target, source))
    {
        int oldsel = _listbox->value();
        RemoveMissingFile(left ? p->FileLeftDex : p->FileRightDex);
        ReloadListbox();
        _listbox->value(oldsel);
        onListClick(0, 0); // force onclick       
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
    showView(p, left);
    
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

void clear_controls()
{
    // used on load, Clear menu
    _listbox->clear();

// TODO memory leak?
//    if (_leftImgView->image())
//        _leftImgView->image()->release();
//    if (_rightImgView->image())
//        _rightImgView->image()->release();
    
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
}

void filter_cb(Fl_Widget* w, void* d)
{
    filterSame = !filterSame;

    // clear_controls();
    // get a filtered view list
    // update the browser
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

void load_cb(Fl_Widget* , void* )
{
    char filename[1024] = "";
    loadfile = fl_file_chooser("Open phash file", "*.phashc", filename);
    if (!loadfile)
        return;

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
    // TODO popup the log file
}

Fl_Menu_Item mainmenuItems[] =
{
    {"&File", 0, 0, 0, FL_SUBMENU},
    {"&Load...", 0, load_cb},
    {"&Clear", 0, clear_cb},
    {"E&xit", 0, quit_cb},
    {0},

    {"&Options", 0, 0, 0, FL_SUBMENU},
    {"Fi&lter Same Phash", 0, filter_cb, 0, FL_MENU_TOGGLE},
    {"Lock Left Dup Button", 0, lockL_cb, 0, FL_MENU_TOGGLE},
    {"Lock Right Dup Button", 0, lockR_cb, 0, FL_MENU_TOGGLE},
    {"View log file ...", 0, viewLog_cb, 0},
    {0},

    {0}
};

int handleSpecial(int event)
{
    switch (event)
    {       
        case KBR_START_LOAD:
            _window->label("Loading...");
            Fl::flush();
            break;
        case KBR_START_COMPARE:
            _window->label("Diffing...");
            Fl::flush();
            break;
        case KBR_START_SORT:
            _window->label("Sorting...");
            Fl::flush();
            break;
        case KBR_DONE_LOAD:
            _window->label("Ready!");
            load_listbox();
            // do NOT flush here!
            onListClick(0,0);
            break;
            
        default:
            return 0;
    }
    return 1;
}

int main(int argc, char** argv)
{
    initlog();
    
    // use remembered main window size
    _PREFS = new Prefs();  
    int x, y, w, h;
    _PREFS->getWinRect(MAIN_PREFIX, x, y, w, h);
    
    filterSame = false;

    fl_register_images();

    MainWin window(x, y, w, h);

    Fl_Menu_Bar* menu = new Fl_Menu_Bar(0, 0, window.w(), 25);
    menu->copy(mainmenuItems);

    _listbox = new Fl_Hold_Browser(0, 25, window.w(), 250);
    _listbox->color(FL_BACKGROUND_COLOR, FL_GREEN);
    _listbox->callback(onListClick);
    _listbox->column_char('|');
    _listbox->column_widths(widths);

    window.resizable(_listbox);

#define BTNBOXY 235
    _btnBox = new Fl_Group(0, BTNBOXY, window.w(), BTN_BOX_HIGH);
    _btnBox->begin();

    _btnLDup = new Fl_Button(5, BTNBOXY+3, 50, BTN_HIGH);
    _btnLDup->label("Dup");
    _btnLDup->callback(btnDupL_cb);
    
    Fl_Button *btnLView = new Fl_Button(60, BTNBOXY + 3, 50, BTN_HIGH);
    btnLView->label("View");
    btnLView->callback(btnViewL_cb);
    
    _btnDiff = new Fl_Button(115, BTNBOXY + 3, 50, BTN_HIGH);
    _btnDiff->label("Diff");
    
    Fl_Button *btnDiffS = new Fl_Button(170, BTNBOXY + 3, 100, BTN_HIGH);
    btnDiffS->label("Diff - Stretch");
    
    _btnRDup = new Fl_Button(275, BTNBOXY + 3, 50, BTN_HIGH);
    _btnRDup->label("Dup");
    _btnRDup->callback(btnDupR_cb);
    
    Fl_Button* btnRView = new Fl_Button(330, BTNBOXY + 3, 50, BTN_HIGH);
    btnRView->label("View");
    btnRView->callback(btnViewR_cb);

    _btnBox->end();

    _leftImgView = new Fl_Box(0, BTNBOXY + BTN_BOX_HIGH, window.w() / 2, 250);
    _leftImgView->box(FL_UP_BOX);
    _leftImgView->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    _rightImgView = new Fl_Box(window.w()/2, 275, window.w() / 2, 250);
    _rightImgView->box(FL_UP_BOX);
    _rightImgView->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

#ifdef _DEBUG
    _leftImgView->color(FL_RED);
    _rightImgView->color(FL_BLUE);
#endif

    initShow();

    _window = &window;
    Fl::add_handler(handleSpecial); // handle internal events
    
    window.show(argc, argv);
    return Fl::run();

}
