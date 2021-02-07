#include <locale.h>
#include "logging.h"
#include "SharedImageExt.h"

#include "filedata.h"
#include "archiveData.h"

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

bool done;
int sourceId;
bool filterSame;
char *loadfile; // hacky
extern void fire_proc_thread();

static wchar_t* utf8_to_wchar(const char* utf8, wchar_t*& wbuf, int lg = -1) 
{
    unsigned len = (lg >= 0) ? (unsigned)lg : (unsigned)strlen(utf8);
    unsigned wn = fl_utf8toUtf16(utf8, len, NULL, 0) + 1; // Query length
    wbuf = (wchar_t*)realloc(wbuf, sizeof(wchar_t) * wn);
    wn = fl_utf8toUtf16(utf8, len, (unsigned short*)wbuf, wn); // Convert string
    wbuf[wn] = 0;
    return wbuf;
}

// TODO how to convert to show unicode filenames?
// TODO command line option to specify threshold
void showResults()
{
    size_t count = getArchPairCount();
    for (int i = 0; i < count; i++)
    {
        //wchar_t *wbuf = NULL;
        char* label = getArchPairText(i);
        //wchar_t* out = utf8_to_wchar(label, wbuf);
        printf("%s\n", label);
        //printf("%ls\n", out); // TODO how to convert to show Unicode???
    }
}

int main(int argc, char** argv)
{
    // check command line args
    // .phashcz file
    if (argc < 2)
    {
        printf("Usage: %s <path to .phashcz file>\n", argv[0]);
        return 1;
    }

    filterSame = false;

    // TODO Consider other choices, e.g. filter-same; multiple input files
    if (!exists(argv[1]))
    {
        printf("Path doesn't exist - >%s<\n", argv[1]);
        return 1;
    }

    setlocale(LC_ALL, "");

	initlog("imgcompZcl.log");

    fl_register_images();
    Fl_Anim_GIF_Image::animate = false;

    loadfile = argv[1];
    // TODO how does this fail? (empty phashcz, other error)
    done = false;
    fire_proc_thread();

    // waiting on thread
    while (!done)
        ;

    // OK, what have we got?
    showResults();
}
