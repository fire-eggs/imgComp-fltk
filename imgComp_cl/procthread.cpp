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
extern char* loadfile1;  // hacky
extern char* loadfile2;  // hacky
extern bool filterSame; // hacky
extern bool done; // very hacky
extern void showResults();

void* proc_func(void* p)
{
    log("load1");
    fprintf(stderr, "Load1\n");
    // load phash
    readPhash(loadfile1, sourceId);
    sourceId++;

    if (loadfile2)
    {
        log("load2");
        fprintf(stderr, "Load2\n");
        readPhash(loadfile2, sourceId);
        sourceId++;
    }

    // compare FileData pairs [ThreadComparePFiles]
    // creates a pairlist
    log("compare");
    fprintf(stderr, "Compare\n");
    CompareFiles();

    // viewing pairlist may be filtered [no matching sources]
    // creates the viewlist
    log("sort");
    fprintf(stderr, "Sort\n");
    FilterAndSort(filterSame);

    fprintf(stderr, "Done\n");
    log("done");

    showResults();

    fprintf(stderr, "fini\n");

    done = true;

    exit(1);
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

