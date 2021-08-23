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
#include "archiveData.h"
#include "ViewWin.h"
#include "prefs.h"
#include "MostRecentPaths.h"

#ifdef _WIN32
#else
#include <unistd.h> // getcwd
#endif

#include "events.h"
#include "logging.h"

#include "FL_Tree_Columns.hpp"

void load_cb(Fl_Widget*, void*);
void quit_cb(Fl_Widget*, void*);
void clear_cb(Fl_Widget*, void*);
void filter_cb(Fl_Widget*, void*);
void lockL_cb(Fl_Widget*, void*);
void lockR_cb(Fl_Widget*, void*);
void viewLog_cb(Fl_Widget*, void*);
void copyToClip_cb(Fl_Widget*, void*);
void clear_controls();
void updateBoxImages();
void onListClick(Fl_Widget* w, void* d);

Prefs* _PREFS;

// TODO push into MainWin class?
int sourceId;
bool filterSame;
//Fl_Browser* _listbox;
TreeWithColumns* _pairview;
Fl_Box* _leftImgView;
Fl_Box* _rightImgView;
Fl_Group* _btnBox;
Fl_Button* _btnLDup;
Fl_Button* _btnRDup;
Fl_Button* _btnDiff;
MostRecentPaths* _mrp;
Fl_Menu_Bar* _menu;

char* loadfile; // hacky

extern char* _logpath; // hacky

SharedImageExt* _leftImage;
SharedImageExt* _rightImage;

int widths[] = { 60, 340, 340, 0 };

// NOTE: relying on toggle menus auto initialized to OFF

class MainWin : public Fl_Double_Window
{
public:
    MainWin(int x, int y, int w, int h) : Fl_Double_Window(x, y, w, h)
    {}

    void resize(int, int, int, int) override;
    int handle(int event) override;
    void do_menu();
};

MainWin* _window;

#define BTN_BOX_HALFHIGH 16
#define BTN_BOX_HIGH BTN_BOX_HALFHIGH * 2
#define BTN_HIGH BTN_BOX_HIGH - 6

void MainWin::resize(int x, int y, int w, int h)
{
    int oldw = this->w();
    int oldh = this->h();
    Fl_Double_Window::resize(x, y, w, h);
    if (w == oldw && h == oldh)
        return; // merely moving window

    // size children as desired
    int newhigh = (int)(h * 0.4 - BTN_BOX_HALFHIGH);
    //_listbox->resize(_listbox->x(), _listbox->y(), w, newhigh);
    _pairview->resize(_pairview->x(), _pairview->y(), w, newhigh);
    //int newy = _listbox->y() + newhigh + BTN_BOX_HIGH;
    int newy = _pairview->y() + newhigh + BTN_BOX_HIGH;
    _btnBox->resize(0, newy - BTN_BOX_HIGH, w, BTN_BOX_HIGH);
    newhigh = h - newy; // (int)(h * 0.6 - BTN_BOX_HALFHIGH);
    _leftImgView->resize(0, newy, w / 2, newhigh);
    _rightImgView->resize(w / 2, newy, w / 2, newhigh);
    updateBoxImages();

    widths[1] = (w - widths[0]) / 2;
    widths[2] = widths[1];
    _pairview->column_widths(widths);

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

static void MenuCB(Fl_Widget* window_p, void* userdata)
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
        loadfile = favs[path];

        _mrp->Add(loadfile); // add "new" path
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
    {"Lock Left Dup Button", 0, lockL_cb, 0, FL_MENU_TOGGLE},
    {"Lock Right Dup Button", 0, lockR_cb, 0, FL_MENU_TOGGLE},
    {"View log file ...", 0, viewLog_cb, 0},
    {"Copy filenames to clipboard", 0, copyToClip_cb, 0},
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

    Fl_Menu_Item* mi = (Fl_Menu_Item*)_menu->find_item("&File/Last Used");
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
        int newdex = _menu->add((const char*)s, (const char*)NULL, MenuCB, 0);
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

ArchPair* getCurrentArchivePair()
{
    Fl_Tree_Item* sel = _pairview->first_selected_item();
    if (!sel)
        return NULL;
    int data = (int)sel->user_data();

    //// Find the image pair currently selected in the listbox (null if none)

    //int line = _listbox->value();
    //if (line == 0)
    //    return NULL;

    //int data = (intptr_t)_listbox->data(line);

    ArchPair* p2 = getArchPair(data);
    return p2;
}

Pair* getCurrentPair()
{
    ArchPair* p2 = getCurrentArchivePair();
    if (!p2)
        return NULL;
    return p2->files->at(0); // TODO temp hack: return first file pair
}

void load_pairview()
{
    size_t count = getArchPairCount();
    for (int i = 0; i < count; i++)
    {
        char* label = getArchPairText(i);
        TreeRowItem* row = _pairview->AddRow(label);
        delete label; // OK?
        row->user_data(getArchPairData(i));

        ArchPair* arcP = getArchPair(i);
        for (int j = 0; j < arcP->files->size(); j++)
        {
            char buff[2048];
            Pair* p = arcP->files->at(j);

            FileData *f1 = GetFD(p->FileLeftDex);
            FileData* f2 = GetFD(p->FileRightDex);

            // TODO why might the "left" file not match the "left" archive??
            FileData* leftF = f1->Archive == arcP->archId1 ? f1 : f2;
            FileData* rightF = f1->Archive == arcP->archId1 ? f2 : f1;

            sprintf(buff, "%03d | %s | %s", p->Val,
                leftF->Name->c_str(),
                rightF->Name->c_str());
            TreeRowItem *crow= _pairview->AddRow(buff, row);
            crow->user_data((void *)j);
        }
    }

    // TODO these need to be children of tree rows
    //size_t count = GetPairCount();
    //for (int i = 0; i < count; i++)
    //{
    //    char *label = GetPairText(i);
    //    TreeRowItem* row = _pairview->AddRow(label);  // TODO is this a copy or the object? need to delete at the appropriate time
    //    row->user_data((void*)GetPairData(i));
    //}
    _pairview->redraw();

    //if (count < 1)
    //{
    //    _listbox->add("No candidates found", NULL);
    //    //        printf("Empty list\n");
    //}

    //for (int i = 0; i < count; i++)
    //{
    //    if (GetPair(i)->valid)
    //        _listbox->add(GetPairText(i), GetPairData(i));
    //}
    //_listbox->make_visible(1);
    //_listbox->redraw();
}

void reloadPairview()
{
    //_listbox->clear();
    //_pairview->clear();
    _pairview->clear_children(_pairview->first());
    load_pairview();
}

double getNiceFileSize(const char* path)
{
    struct stat st;
    stat(path, &st);
    auto sizeK = st.st_size / 1024.0;
    return sizeK;
}

void updateTitle(const char* pathL, Fl_Image* imgL, const char* pathR, Fl_Image* imgR)
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
    {
        // TODO display an existing image?
        _leftImgView->image(NULL);
        _rightImgView->image(NULL);
        _leftImgView->redraw();
        _rightImgView->redraw();
        return;
    }

    // TODO size images proportionally to view size
    // both images exist
    int iw = _leftImgView->w();
    int ih = _leftImgView->h();

    if (_leftImgView->image())
        _leftImgView->image()->release();
    if (_rightImgView->image())
        _rightImgView->image()->release();

    _leftImgView->image(_leftImage->image()->copy(iw, ih));
    _rightImgView->image(_rightImage->image()->copy(iw, ih));

    _leftImgView->redraw();
    _rightImgView->redraw();

}

Fl_Tree_Item* _last = NULL;

void onListClick(Fl_Widget* w, void* d)
{
    // TODO prevents initial draw from "Done" logic
    //Fl_Tree_Reason why = _pairview->callback_reason();
    //if (why != FL_TREE_REASON_SELECTED)
    //    return;

    Fl_Tree_Item* who = _pairview->callback_item();
    if (who == _last)
        return;
    _last = who;
    int data = (int)who->user_data();

    //if (_listbox->size() < 1) // list is now empty, done
    //    return;

    //int line = _listbox->value();
    //if (!line)
    //    line = 1; // when advancing thru the list below reaches bottom, selection is set to none.

    //int data = (intptr_t)_listbox->data(line);
    //size_t max = GetPairCount();
    //if (data >= max)
    //    return;

    ArchPair* p2;
    Pair* p;
    if (who->has_children())
    {
        // TODO temp hack: show first filepair in archives [may NOT be the first file in each archive!]
        p2 = getArchPair(data);
        p = p2->files->at(0);
        //Pair* p = GetPair(data);
    }
    else
    {
        int dad = (int)who->parent()->user_data();
        p2 = getArchPair(dad);
        p = p2->files->at(data);
    }

    //const char* pathL = GetFD(p->FileLeftDex)->Name->c_str();
    //const char* pathR = GetFD(p->FileRightDex)->Name->c_str();
    const char* pathL = GetActualPath(p, true);
    const char* pathR = GetActualPath(p, false);

    // release existing image(s)
    if (_leftImage) { _leftImage->release(); _leftImage = NULL; }
    if (_rightImage) { _rightImage->release(); _rightImage = NULL; }

    _leftImage = SharedImageExt::LoadImage(pathL);
    _rightImage = SharedImageExt::LoadImage(pathR);

    //// _leftImage or _rightImage may be null [file missing]
    //// Force selection of next entry
    //// Force selection of next entry
    //if (!_leftImage || !_rightImage)
    //{
    //    p->valid = false;

    //    // TODO do this w/o recursion!
    //    // TODO needs to be done in the viewlist ???
    //    _listbox->select(line + 1, 1); // NOTE: does NOT force 'onclick' event
    //    _listbox->remove(line);
    //    _listbox->redraw();
    //    onListClick(0, 0); // force onclick
    //    return;
    //}

    if (_leftImage && _rightImage) // TODO shouldn't happen
        updateTitle(pathL, _leftImage->baseImage(), pathR, _rightImage->baseImage());
    updateBoxImages();

    //// ensure the current line is up a little bit - can't click to get to next line sometimes
    //_listbox->bottomline(line + 5);
}

void btnDup(bool left)
{
    //// Common code for the two 'Dup' buttons
    //// rename one of the images as a duplicate of the other
    //// bool left : rename the 'left' image

    ArchPair* p2 = getCurrentArchivePair();
    if (!p2)
        return;

    // Can't 'dup' a file in an archive
    if (left && p2->archId1 != -1)
        return;
    if (!left && p2->archId2 != -1)
        return;

    Pair *p = p2->files->at(0); // TODO temp hack: return first file pair

    auto pathL = GetFD(p->FileLeftDex)->Name->c_str();
    auto pathR = GetFD(p->FileRightDex)->Name->c_str();
    auto target = left ? pathR : pathL;
    auto source = left ? pathL : pathR;

    // Trying for <srcpath>/<srcfile> to <srcpath>/dup0_<destfile>
    if (MoveFile("%s/dup%d_%s", target, source))
    {
        if (_leftImgView->image())
        {
            _leftImgView->image()->release();
            _leftImgView->image(NULL);
        }
        if (_rightImgView->image())
        {
            _rightImgView->image()->release();
            _rightImgView->image(NULL);
        }

    //    int oldsel = _listbox->value();
    //    RemoveMissingFile(left ? p->FileLeftDex : p->FileRightDex);
    //    p->valid = false;
    //    ReloadListbox();
    //    _listbox->select(oldsel);
    //    onListClick(0, 0); // force onclick       
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

    Pair* p = getCurrentPair();
    if (!p)
        return;
    Fl_Image* leftI = _leftImage ? _leftImage->baseImage() : NULL;
    Fl_Image* rightI = _rightImage ? _rightImage->baseImage() : NULL;
    showView(leftI, rightI, left);

    _pairview->take_focus(); // so user doesn't lose their place: focus back to list
}

void btnViewL_cb(Fl_Widget* w, void* d)
{
    btnView(true);
}

void btnViewR_cb(Fl_Widget* w, void* d)
{
    btnView(false);
}

void btnDiff(bool left, bool stretch)
{
    Pair* p = getCurrentPair();
    if (!p)
        return;
    showDiff(_leftImage->image(), _rightImage->image(), stretch);
    //showDiff(p, stretch);

    //_listbox->take_focus(); // so user doesn't lose their place: focus back to listbox
    _pairview->take_focus();
}

void btnDiff_cb(Fl_Widget*, void*)
{
    btnDiff(true, false);
}

void btnDiffS_cb(Fl_Widget*, void*)
{
    btnDiff(true, true);
}

void btnNext_cb(Fl_Widget*, void*)
{
    Fl_Tree_Item* sel = _pairview->first_selected_item();
    if (sel) _pairview->deselect(sel);
    Fl_Tree_Item* next = _pairview->next(sel);
    if (!next) return;       
    _pairview->select(next);
    // TODO if next is archive-pair, move to first image
}

void btnPrev_cb(Fl_Widget*, void*)
{
    Fl_Tree_Item* sel = _pairview->first_selected_item();
    if (sel) _pairview->deselect(sel);
    Fl_Tree_Item* next = _pairview->prev(sel);
    if (!next) return;
    _pairview->select(next);
    // TODO if next is archive-pair, move to first image
}

void clear_controls()
{
    // used on load, Clear menu
    //_listbox->clear();
    _pairview->clear_children(_pairview->first());
    //_pairview->clear();

    if (_leftImgView->image())
        ((Fl_Shared_Image*)_leftImgView->image())->release();
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
    clearArchiveData();

    _window->label("");
}

void filter_cb(Fl_Widget* w, void* d)
{
    filterSame = !filterSame;

    clear_controls();
    FilterAndSort(filterSame);
    load_pairview();
    //_listbox->select(1);
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

void load_cb(Fl_Widget*, void*)
{
    char filename[1024] = "";
    Fl_File_Chooser* choose = new Fl_File_Chooser(filename, "PHASH (*.{phashc,phashcz})\tAll (*.*)", 0, "Open phash file");
    choose->preview(false); // force preview off
    popup(choose);
    loadfile = (char*)choose->value();

    // TODO can't control file_chooser location because underlying window is not exposed by the class

    //loadfile = fl_file_chooser("Open phash file", "*.phashc", filename);
    if (!loadfile)
        return;

    _mrp->Add(loadfile); // add new path
    _mrp->Save();
    updateMRU(); // update the menu

    clear_controls();
    Fl::flush();

    fire_proc_thread();
}

void quit_cb(Fl_Widget*, void*)
{
    exit(0);
}

void viewLog_cb(Fl_Widget*, void*)
{
    // TODO popup the log file
}

void copyToClip_cb(Fl_Widget*, void*)
{
    ArchPair* zipPair = getCurrentArchivePair();
    if (!zipPair)
        return;
    Pair* p = zipPair->files->at(0);

    std::string a1;
    std::string a2;

    // TODO refactor to common code
    if (zipPair->archId1 != -1 &&
        zipPair->archId2 != -1)
    {
        // both archives, only get the archive paths
        a1 = getArchivePath(zipPair->archId1);
        a2 = getArchivePath(zipPair->archId2);
    }
    else
    {
        // one or both are files, want to get the filepath within the archive
        if (zipPair->archId1 != -1)
            a1 = getArchivePath(zipPair->archId1) + ":" + *GetFD(p->FileLeftDex)->Name;
        else
            a1 = *GetFD(p->FileLeftDex)->Name;

        if (zipPair->archId2 != -1)
            a2 = getArchivePath(zipPair->archId2) + ":" + *GetFD(p->FileRightDex)->Name;
        else
            a2 = *GetFD(p->FileRightDex)->Name;
    }

    int size = a1.length() + a2.length() + 3;
    char* buff = (char*)malloc(size);
    if (buff)
    {
        sprintf(buff, "%s\n%s", a1.c_str(), a2.c_str());
        Fl::copy(buff, size, 2, Fl::clipboard_plain_text);
        free(buff);
    }
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
        load_pairview();
        // do NOT flush here!
        //_listbox->select(1);
        onListClick(0, 0);
        Fl::awake();
        break;

    default:
        return 0;
    }
    return 1;
}

int main(int argc, char** argv)
{
    initlog("imgcompZ.log");

    // use remembered main window size
    _PREFS = new Prefs();
    int x, y, w, h;
    _PREFS->getWinRect(MAIN_PREFIX, x, y, w, h);

    _mrp = new MostRecentPaths(_PREFS->_prefs);

    filterSame = false;

    fl_register_images();
    Fl_Image_Display::set_gamma(2.2f);
    Fl_Anim_GIF_Image::animate = false;

    // TODO : use actual size when building controls?
    MainWin window(x, y, w, h);

    _menu = new Fl_Menu_Bar(0, 0, window.w(), 25);
    _menu->copy(mainmenuItems);
    updateMRU();

    _pairview = new TreeWithColumns(0, 25, window.w(), 250);
    _pairview->selectmode(FL_TREE_SELECT_SINGLE);
    _pairview->resizing(true);
    _pairview->column_char('|');
    _pairview->column_widths(widths);
    _pairview->AddRow("Blah|Blah|Blah");
    _pairview->callback(onListClick);
    _pairview->when(FL_WHEN_RELEASE);

    //_listbox = new Fl_Hold_Browser(0, 25, window.w(), 250);
    //_listbox->color(FL_BACKGROUND_COLOR, FL_GREEN);
    //_listbox->callback(onListClick);
    //_listbox->column_char('|');
    //_listbox->column_widths(widths); // TODO how to calculate 0-th width to fit "DUP" at selected font and size?

    // TODO options to set these
    //Fl_Fontsize blah = _listbox->textsize(); // 14

    //_listbox->textsize(14);

    //_listbox->textfont(FL_COURIER);

    //window.resizable(_listbox);
    window.resizable(_pairview);

#define BTNBOXY 235
    _btnBox = new Fl_Group(0, BTNBOXY, window.w(), BTN_BOX_HIGH);
    _btnBox->begin();

    int btnX = 5;
    _btnLDup = new Fl_Button(5, BTNBOXY + 3, 50, BTN_HIGH);
    _btnLDup->label("Dup");
    _btnLDup->callback(btnDupL_cb);

    btnX += 55;
    Fl_Button* btnLView = new Fl_Button(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    btnLView->label("View");
    btnLView->callback(btnViewL_cb);

    btnX += 55 + 10;
    _btnDiff = new Fl_Button(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    _btnDiff->label("Diff");
    _btnDiff->callback(btnDiff_cb);

    btnX += 55;
    Fl_Button* btnDiffS = new Fl_Button(btnX, BTNBOXY + 3, 100, BTN_HIGH);
    btnDiffS->label("Diff - Stretch");
    btnDiffS->callback(btnDiffS_cb);

    btnX += 105 + 10;
    _btnRDup = new Fl_Button(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    _btnRDup->label("Dup");
    _btnRDup->callback(btnDupR_cb);

    btnX += 55;
    Fl_Button* btnRView = new Fl_Button(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    btnRView->label("View");
    btnRView->callback(btnViewR_cb);

    btnX += 55 + 10;
    Fl_Button* btnPrev = new Fl_Button(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    btnPrev->label("Prev");
    btnPrev->callback(btnPrev_cb);
    btnX += 55;
    Fl_Button* btnNext = new Fl_Button(btnX, BTNBOXY + 3, 50, BTN_HIGH);
    btnNext->label("Next");
    btnNext->callback(btnNext_cb);

    btnX += 60;
    // An invisible widget to be resizable instead of anything else
    // in the toolbar. NOTE: *must* be the last in the X list
    Fl_Box* invisible = new Fl_Box(btnX, BTNBOXY + 3, 10, BTN_HIGH);
    invisible->hide();
    Fl_Group::current()->resizable(invisible);

    _btnBox->end();

    _leftImgView = new Fl_Box(0, BTNBOXY + BTN_BOX_HIGH, window.w() / 2, 250);
    _leftImgView->box(FL_UP_BOX);
    _leftImgView->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    _rightImgView = new Fl_Box(window.w() / 2, 275, window.w() / 2, 250);
    _rightImgView->box(FL_UP_BOX);
    _rightImgView->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

#ifdef _DEBUG
    _leftImgView->color(FL_RED);
    _rightImgView->color(FL_BLUE);
#endif

    initShow();

    window.resize(x, y, w + 1, h - 1); // Hack: force everything to adjust to actual size
    window.resize(x, y, w, h);

    _leftImage = _rightImage = NULL;

    _window = &window;
    Fl::add_handler(handleSpecial); // handle internal events

    window.show(argc, argv);
    return Fl::run();

}
