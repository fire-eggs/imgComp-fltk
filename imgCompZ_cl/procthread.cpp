#ifndef _WIN32
#define HAVE_PTHREAD
#define HAVE_PTHREAD_H
#endif

// TODO not an official part of FLTK?
#include "threads.h"

#ifdef _WIN32
#include "stdlib.h"
#define sleep _sleep
#else
#include <unistd.h>
#endif

#include "filedata.h"

#include "logging.h"
#include "archiveData.h"

extern int sourceId;    // hacky
extern char* loadfile;  // hacky
extern bool filterSame; // hacky
extern bool done; // very hacky

// TODO printf statements should be controlled by option
void* proc_func(void* p)
{
    log("load");
    fprintf(stderr, "Load\n");
    // load phash
    readPhash(loadfile, sourceId);
    sourceId++;

    log("compare");

    // compare FileData pairs [ThreadComparePFiles]
    // creates a pairlist
    fprintf(stderr, "Compare\n");
    CompareFiles();

    log("sort");

    // viewing pairlist may be filtered [no matching sources]
    fprintf(stderr, "Sort\n");
    FilterAndSort(filterSame);

    log("archive");
    compareArchives(); // Depends on _viewlist!!!

    fprintf(stderr, "Done\n");
    log("done");
    done = true;
    return NULL;
}

Fl_Thread proc_thread;

void fire_proc_thread()
{
    fl_create_thread(proc_thread, proc_func, NULL);
    // TODO need a windows version of this!
#ifndef _WIN32
    pthread_detach(*(pthread_t*)proc_thread);
#endif
}

