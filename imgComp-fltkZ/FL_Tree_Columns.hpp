#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tree.H>

const char* const tree_open_xpm[] = {
  "11 11 3 1",
  ".    c #fefefe",
  "#    c #444444",
  "@    c #000000",
  "###########",
  "#.........#",
  "#.........#",
  "#....@....#",
  "#....@....#",
  "#..@@@@@..#",
  "#....@....#",
  "#....@....#",
  "#.........#",
  "#.........#",
  "###########"
};

const char* const tree_close_xpm[] = {
  "11 11 3 1",
  ".    c #fefefe",
  "#    c #444444",
  "@    c #000000",
  "###########",
  "#.........#",
  "#.........#",
  "#.........#",
  "#.........#",
  "#..@@@@@..#",
  "#.........#",
  "#.........#",
  "#.........#",
  "#.........#",
  "###########"
};


class TreeRowItem : public Fl_Tree_Item {
public:
    TreeRowItem(Fl_Tree* tree, const char* text) : Fl_Tree_Item(tree) {
        this->label(text);
    }
    int draw_item_content(int render);
};

// Small convenience class to handle adding columns.
//    TreeRowItem does most of the work.
//
class TreeWithColumns : public Fl_Tree {
    bool colseps_flag;     // enable/disable column separator lines
    bool resizing_flag;    // enable/disable interactive resizing
    char col_char;         // column delimiter character
    int* col_widths;       // array of column widths (supplied by application)
    int first_col_minw;    // minimum width of first column
    int drag_col;          // internal: column being FL_DRAG'ed
    Fl_Cursor last_cursor; // internal: last mouse cursor value

protected:
    int column_near_mouse() {
        // Event not inside browser area? (eg. scrollbar) Early exit
        if (!Fl::event_inside(_tix, _tiy, _tiw, _tih)) return(-1);
        int mousex = Fl::event_x() + hposition();
        int colx = x() + first_col_minw;
        for (int t = 0; col_widths[t]; t++) {
            colx += col_widths[t];
            int diff = mousex - colx;
            // Mouse near column? Return column #
            if (diff >= -4 && diff <= 4) return(t);
        }
        return(-1);
    }
    // Change the mouse cursor
    //     Does nothing if cursor already set to same value.
    //
    void change_cursor(Fl_Cursor newcursor) {
        if (newcursor == last_cursor) return;
        window()->cursor(newcursor);
        last_cursor = newcursor;
    }

public:
    TreeWithColumns(int X, int Y, int W, int H, const char* L = 0) : Fl_Tree(X, Y, W, H, L) {
        colseps_flag = true;
        resizing_flag = true;
        col_char = '\t';
        col_widths = 0;
        first_col_minw = 80;
        drag_col = -1;
        last_cursor = FL_CURSOR_DEFAULT;
        // We need the default tree icons on all platforms.
        //    For some reason someone made Fl_Tree have different icons on the Mac,
        //    which doesn't look good for this application, so we force the icons
        //    to be consistent with the '+' and '-' icons and dotted connection lines.
        //
        connectorstyle(FL_TREE_CONNECTOR_DOTTED);
        openicon(new Fl_Pixmap(tree_open_xpm));
        closeicon(new Fl_Pixmap(tree_close_xpm));
    }
    // The minimum width of column #1 in pixels.
    //     During interactive resizing, don't allow first column to be smaller than this.
    //
    int  first_column_minw() { return first_col_minw; }
    void first_column_minw(int val) { first_col_minw = val; }
    // Enable/disable the vertical column lines
    void column_separators(bool val) { colseps_flag = val; }
    bool column_separators() const { return colseps_flag; }
    // Enable/disable the vertical column lines
    void resizing(bool val) { resizing_flag = val; }
    bool resizing() const { return resizing_flag; }
    // Change the column delimiter character
    void column_char(char val) { this->col_char = val; }
    char column_char() const { return col_char; }
    // Set the column array. 
    //     Make sure the last entry is zero.
    //     User allocated array must remain allocated for lifetime of class instance.
    //     Must be large enough for all columns in data!
    //
    void column_widths(int* val) { this->col_widths = val; }
    int* column_widths() const { return col_widths; }
    TreeRowItem* AddRow(const char* s, TreeRowItem* parent_item = 0) {
        TreeRowItem* item = new TreeRowItem(this, s);  // create new item
        if (parent_item == 0) {                      // wants root item as parent?
            if (strcmp(root()->label(), "ROOT") == 0) {  // default root item?
                this->root(item);                          // make this item the new root
                // Special colors for root item -- this is the "header"
                item->labelfgcolor(0xffffff00);
                item->labelbgcolor(0x8888ff00);
                return item;
            }
            else {
                parent_item = (TreeRowItem*)root();        // use root as parent
            }
        }
        parent_item->add(prefs(), "", item);           // add item to hierarchy
        return item;                                   // return the new item
    }
    // Manage column resizing
    int handle(int e) {
        if (!resizing_flag) return Fl_Tree::handle(e);    // resizing off? early exit
        // Handle column resizing
        int ret = 0;
        switch (e) {
        case FL_ENTER:
            ret = 1;
            break;
        case FL_MOVE:
            change_cursor((column_near_mouse() >= 0) ? FL_CURSOR_WE : FL_CURSOR_DEFAULT);
            ret = 1;
            break;
        case FL_PUSH: {
            int whichcol = column_near_mouse();
            if (whichcol >= 0) {
                // Clicked on resizer? Start dragging that column
                drag_col = whichcol;
                change_cursor(FL_CURSOR_DEFAULT);
                return 1;   // eclipse event from Fl_Tree's handle()
            }
            break;
        }
        case FL_DRAG:
            if (drag_col != -1) {
                // Sum up column widths to determine position
                int mousex = Fl::event_x() + hposition();
                int newwidth = mousex - (x() + first_column_minw());
                for (int t = 0; col_widths[t] && t < drag_col; t++)
                {
                    newwidth -= col_widths[t];
                }
                // Apply new width, redraw interface
                col_widths[drag_col] = newwidth;
                if (col_widths[drag_col] < 2) col_widths[drag_col] = 2;  // XXX: 2 should be a class member
                recalc_tree();
                redraw();
                return 1;        // eclipse event from Fl_Tree's handle()
            }
            break;
        case FL_LEAVE:
        case FL_RELEASE:
            change_cursor(FL_CURSOR_DEFAULT);          // ensure normal cursor
            if (drag_col != -1 && e == FL_RELEASE) { // release during drag mode?
                drag_col = -1;   // disable drag mode
                return 1;        // eclipse event from base class; we handled it
            }
            drag_col = -1;
            ret = 1;
            break;
        }
        return(Fl_Tree::handle(e) ? 1 : ret);
    }

    // Hide these base class methods from the API; we don't want app using them,
    // as we expect all items in the tree to be TreeRowItems, not Fl_Tree_Items.
private:
    using Fl_Tree::add;
};
