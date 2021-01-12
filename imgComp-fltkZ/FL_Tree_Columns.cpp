#include <stdio.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tree.H>

//
// Demonstrate Fl_Tree with character delimited interactively resizable columns
// erco 08/23/2019
//

#define LEFT_MARGIN 5         // TBD: Make this a class attribute

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

// DERIVE CUSTOM CLASS FROM Fl_Tree_Item TO SHOW DATA IN COLUMNS
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

// Handle custom drawing of the item
//
//    All we're responsible for is drawing the 'label' area of the item
//    and it's background. Fl_Tree gives us a hint as to what the
//    foreground and background colors should be via the fg/bg parameters,
//    and whether we're supposed to render anything or not.
//
//    The only other thing we must do is return the maximum X position
//    of scrollable content, i.e. the right most X position of content
//    that we want the user to be able to use the horizontal scrollbar
//    to reach.
//
int TreeRowItem::draw_item_content(int render) {
    TreeWithColumns* treewc = (TreeWithColumns*)tree();
    Fl_Color fg = drawfgcolor();
    Fl_Color bg = drawbgcolor();
    const int* col_widths = treewc->column_widths();
    //    Show the date and time as two small strings
    //    one on top of the other in a single item.
    //
    // Our item's label dimensions
    int X = label_x(), Y = label_y(),
        W = label_w(), H = label_h(),
        RX = treewc->x() - treewc->hposition() + treewc->first_column_minw(),  // start first column at a fixed position
        RY = Y + H - fl_descent();  // text draws here
    // Render background
    if (render) {
        if (is_selected()) { fl_draw_box(prefs().selectbox(), X, Y, W, H, bg); }
        else { fl_color(bg); fl_rectf(X, Y, W, H); }
        fl_font(labelfont(), labelsize());
    }
    // Render columns
    if (render) fl_push_clip(X, Y, W, H);
    {
        // Draw each column
        //   ..or if not rendering, at least calculate width of row so we can return it.
        //
        int t = 0;
        const char* s = label();
        char delim_str[2] = { treewc->column_char(), 0 };  // strcspn() wants a char[]
        while (*s) {
            int n = strcspn(s, delim_str);         // find index to next delimiter char in 's' (or eos if none)
            if (n > 0 && render) {                 // renderable string with at least 1 or more chars?
                int XX = (t == 0) ? X : RX;          // TBD: Rename XX to something more meaningful
                // Don't clip last column.
                //    See if there's more columns after this one; if so, clip the column.
                //    If not, let column run to edge of widget
                //
                int CW = col_widths[t];              // clip width based on column width
                // If first column, clip to 2nd column's left edge
                if (t == 0) { CW = (RX + col_widths[0]) - XX; }
                // If last column, clip to right edge of widget
                if (*(s + n) == 0) { CW = (x() + w() - XX); }
                // Draw the text
                //    We want first field (PID) indented, rest of fields fixed column.
                //
                fl_color(fg);
                fl_push_clip(XX, Y, CW, H);          // prevent text from running into next column
                fl_draw(s, n, XX + LEFT_MARGIN, RY);
                fl_pop_clip();                       // clip off
                // Draw vertical lines for all columns except first
                if (t > 0 && treewc->column_separators()) {
                    fl_color(FL_BLACK);
                    fl_line(RX, Y, RX, Y + H);
                }
            }
            if (*(s + n) == treewc->column_char()) {
                s += n + 1;               // skip field + delim
                RX += col_widths[t++];  // keep track of fixed column widths for all except right column
                continue;
            }
            else {
                // Last field? Return entire length of unclipped field
                RX += fl_width(s) + LEFT_MARGIN;
                s += n;
            }
        }
    }
    if (render) fl_pop_clip();
    return RX;                    // return right most edge of what we've rendered
}

#if false
int main(int argc, char* argv[]) {
    Fl::scheme("gtk+");
    Fl_Double_Window* win = new Fl_Double_Window(550, 400, "Tree With Columns");
    win->begin();
    {
        static int col_widths[] = { 80, 50, 50, 50, 0 };

        // Create the tree
        TreeWithColumns* tree = new TreeWithColumns(0, 0, win->w(), win->h());
        tree->selectmode(FL_TREE_SELECT_MULTI);     // multiselect
        tree->column_widths(col_widths);            // set column widths array
        tree->resizing(true);                       // enable interactive resizing
        tree->column_char('\t');                    // use tab char as column delimiter
        tree->first_column_minw(100);               // minimum width of first column

        // Add some items in a hierarchy:
        //
        //  5197 ?        Ss     0:00 /usr/sbin/sshd -D
        // 12807 ?        Ss     0:00  \_ sshd: root@pts/6
        // 12811 pts/6    Ss+    0:00  |   \_ -tcsh
        //  3993 ?        Ss     0:00  \_ sshd: erco [priv]
        //  3997 ?        S      0:00      \_ sshd: erco@pts/14
        //  3998 pts/14   Ss+    0:00          \_ -tcsh
        //  5199 ?        Ssl    8:02 /usr/sbin/rsyslogd -n
        //  5375 ?        Ss     0:00 /usr/sbin/atd -f
        // 13778 ?        Ss     0:00  \_ /usr/sbin/atd -f
        // 13781 ?        SN     0:00  |   \_ sh
        // 13785 ?        SN   684:59  |       \_ xterm -title Reminder: now -geometry 40x6-1+1 -fg white -bg #cc0000 -hold -font
        //
        TreeRowItem* top;       // top of hierarchy
        // First row is header
        top = tree->AddRow("PID\tTTY\tSTAT\tTIME\tCOMMAND");        // first row is always the "header"
        // Add the hierarchical rows of data..
        top = tree->AddRow("5197\t?\tSs\t0:00\t/usr/sbin/sshd -D"); // top level
        // Add 200 copies of data to tree to test performance
        for (int t = 0; t < 200; t++) {
            {
                TreeRowItem* child1 = tree->AddRow("12807\t?\tSs\t0:00\tsshd: root@pts/6", top);
                {
                    tree->AddRow("12811\tpts/6\tSs+\t0:00\t-tcsh", child1);
                }
                child1 = tree->AddRow("3993\t?\tSs\t0:00\tsshd: erco [priv]", top);
                {
                    TreeRowItem* child2 = tree->AddRow("3997\t?\tS\t0:00\tsshd: erco@pts/14", child1);
                    {
                        tree->AddRow("3998\tpts/14\tSs+\t0:00\t-tcsh", child2);
                    }
                }
            }
            top = tree->AddRow("5199\t?\tSsl\t8:02\t/usr/sbin/rsyslogd -n");
            top = tree->AddRow("5375\t?\tSs\t0:00\t/usr/sbin/atd -f");
            {
                TreeRowItem* child1 = tree->AddRow("13778\t?\tSs\t0:00\t/usr/sbin/atd -f", top);
                {
                    TreeRowItem* child2 = tree->AddRow("13781\t?\tSN\t0:00\tsh", child1);
                    {
                        tree->AddRow("13785\t?\tSN\t684:59\txterm -title Reminder: now -geometry 40x6-1+1 -fg white -bg #cc0000 -hold -font", child2);
                    }
                }
            }
        }
        // tree->show_self();
    }
    win->end();
    win->resizable(win);
    win->show(argc, argv);
    return(Fl::run());
}
#endif
