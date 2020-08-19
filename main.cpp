#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Shared_Image.H>

#include "filedata.h"
#include "ViewWin.h"

int sourceId;
bool filterSame;
Fl_Browser* _listbox;
Fl_Box* _leftImgView;
Fl_Box* _rightImgView;
Fl_Group* _btnBox;
Fl_Button* _btnLDup;
Fl_Button* _btnLView;
Fl_Button* _btnRDup;
Fl_Button* _btnRView;
Fl_Button* _btnDiff;
Fl_Button* _btnDiffS;

int widths[] = {40, 340, 340, 0};

// NOTE: relying on toggle menus auto initialized to OFF

class MainWin : public Fl_Double_Window
{
public:
    MainWin(int w, int h) : Fl_Double_Window(w,h)
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
}

int MainWin::handle(int event)
{
    // TODO double-click on img boxes?
    return Fl_Double_Window::handle(event);
}

void btnView(bool left)
{
    int line = _listbox->value();
    if (line == 0)
        return;

    int data = (intptr_t)_listbox->data(line);

    Pair* p = GetPair(data);
    showView(p, left);
}

void btnL_cb(Fl_Widget* w, void* d)
{
    btnView(true);
}

void btnR_cb(Fl_Widget* w, void* d)
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

void load_cb(Fl_Widget* w, void* d)
{
    char filename[1024] = "";
    char* loadfile = fl_file_chooser("Open phash file", "*.phashc", filename);
    if (!loadfile)
        return;

    clear_controls();
    Fl::flush();
    
    // load phash
    readPhash(loadfile, sourceId);
    sourceId++;

    // compare FileData pairs [ThreadComparePFiles]
    // creates a pairlist
    CompareFiles();

    // viewing pairlist may be filtered [no matching sources]
    FilterAndSort(filterSame);

    // load pairlist into browser
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
    sprintf(buff, "(%d,%dx%d)[%2gK] : (%d,%dx%d)[%2gK]", ilh, ilw, ild, fsl, irh, irw, ird, fsr);
    _window->label(buff);
}

void onListClick(Fl_Widget* w, void* d)
{
    int line = _listbox->value();
    int data = (intptr_t)_listbox->data(line);
    printf("LB: val: %d data:%d\n", line, data);

    Pair* p = GetPair(data);
    const char* pathL = GetFD(p->FileLeftDex)->Name->c_str();
    const char* pathR = GetFD(p->FileRightDex)->Name->c_str();

    printf("PathL: %s\n", pathL);
    printf("PathR: %s\n", pathR);

    Fl_Shared_Image* imgL = Fl_Shared_Image::get(pathL);
    Fl_Shared_Image* imgR = Fl_Shared_Image::get(pathR);

    // imgL or imgR may be null [file missing]
    // Force selection of next entry
    // TODO test multiple missing files
    if (!imgL || !imgR)
    {
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
}

void quit_cb(Fl_Widget* w, void* d)
{
    exit(0);
}

Fl_Menu_Item mainmenuItems[] =
{
    {"&File", 0, 0, 0, FL_SUBMENU},
    {"&Load...", 0, load_cb},
    {"&Clear", 0, clear_cb},
    {"E&xit", 0, quit_cb},
    {0},

    {"&Options", 0, 0, 0, FL_SUBMENU},
    {"Fi&lter Same Phash", 0, filter_cb, 0, FL_MENU_TOGGLE}, // TODO toggle button
    {"Lock Left Dup Button", 0, lockL_cb, 0, FL_MENU_TOGGLE}, // TODO toggle
    {"Lock Right Dup Button", 0, lockR_cb, 0, FL_MENU_TOGGLE}, // TODO toggle
    {0},

    {0}
};

int main(int argc, char** argv)
{
    filterSame = false;

    fl_register_images();

    MainWin window(720, 520);

    Fl_Menu_Bar* menu = new Fl_Menu_Bar(0, 0, window.w(), 25);
    menu->copy(mainmenuItems);

    _listbox = new Fl_Hold_Browser(0, 25, window.w(), 250);
    _listbox->callback(onListClick);
    _listbox->column_char('|');
    _listbox->column_widths(widths);

    window.resizable(_listbox);

    // TODO need callbacks
#define BTNBOXY 235
    _btnBox = new Fl_Group(0, BTNBOXY, window.w(), BTN_BOX_HIGH);
    _btnBox->begin();

    _btnLDup = new Fl_Button(5, BTNBOXY+3, 50, BTN_HIGH);
    _btnLDup->label("Dup");
    _btnLView = new Fl_Button(60, BTNBOXY + 3, 50, BTN_HIGH);
    _btnLView->label("View");
    _btnLView->callback(btnL_cb);
    _btnDiff = new Fl_Button(115, BTNBOXY + 3, 50, BTN_HIGH);
    _btnDiff->label("Diff");
    _btnDiffS = new Fl_Button(170, BTNBOXY + 3, 100, BTN_HIGH);
    _btnDiffS->label("Diff - Stretch");
    _btnRDup = new Fl_Button(275, BTNBOXY + 3, 50, BTN_HIGH);
    _btnRDup->label("Dup");
    _btnRView = new Fl_Button(330, BTNBOXY + 3, 50, BTN_HIGH);
    _btnRView->label("View");
    _btnRView->callback(btnR_cb);

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
    
    window.show(argc, argv);
    return Fl::run();

}
