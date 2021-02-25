#include <locale.h>
#include "logging.h"
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Anim_GIF_Image.H>

#include "filedata.h"

bool filterSame;
bool done;
char* loadfile1; // hacky
char* loadfile2; // hacky
int sourceId;
extern void fire_proc_thread();

bool exists(const std::string& name)
{
    struct stat buffer;
    if (stat(name.c_str(), &buffer) != 0)
    {
        return false;
    }
    if (!(buffer.st_mode & S_IFREG)) // not a file
    {
        return false;
    }
    return true;
}

void showResults()
{
    size_t count = GetPairCount();
    for (int i = 0; i < count; i++)
    {
        char* label = GetPairText(i);
        printf("%s\n", label);
        delete label;
    }
}

// TODO consider other options: filter-same, threshold
// TODO printf statements should be controlled by option

int main(int argc, char** argv)
{
    // check command line args
    // .phashc file
    if (argc < 2)
    {
        printf("Usage: %s <path to .phashc file> [<path to .phashc file>]\n", argv[0]);
        return 1;
    }

    filterSame = false; // force OFF for single file

    if (!exists(argv[1]))
    {
        printf("Path doesn't exist - >%s<\n", argv[1]);
        return 1;
    }

    loadfile2 = NULL;
    loadfile1 = argv[1];

    if (argc > 2)
    {
        if (!exists(argv[2]))
        {
            printf("Path doesn't exist - >%s<\n", argv[2]);
            return 1;
        }

        filterSame = true; // force ON for multiple files
        loadfile2 = argv[2];
    }

    setlocale(LC_ALL, "");

    initlog("imgCompcl.log");

    fl_register_images();
    Fl_Anim_GIF_Image::animate = false;

    // TODO how does this fail? (empty phashcz, other error)
    sourceId = 0;
    done = false;
    fire_proc_thread();

    // waiting on thread
    while (!done)
        ;

    // OK, what have we got?
    //showResults();
}
