#include <stdio.h>
#include "FL_Tree_Columns.hpp"

//
// Demonstrate Fl_Tree with character delimited interactively resizable columns
// erco 08/23/2019
//

#define LEFT_MARGIN 5.0         // TBD: Make this a class attribute

// DERIVE CUSTOM CLASS FROM Fl_Tree_Item TO SHOW DATA IN COLUMNS

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
            size_t n = strcspn(s, delim_str);         // find index to next delimiter char in 's' (or eos if none)
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
