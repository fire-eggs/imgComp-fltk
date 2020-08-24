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
#include "events.h"
#include <FL/Fl.H>

extern int sourceId;    // hacky
extern char* loadfile;  // hacky
extern bool filterSame; // hacky

void* proc_func(void *p)
{
    Fl::handle_(KBR_START_LOAD, 0);
    
    log("load");
    // load phash
    readPhash(loadfile, sourceId);
    sourceId++;

    log("compare");
    Fl::handle_(KBR_START_COMPARE, 0);
    
    // compare FileData pairs [ThreadComparePFiles]
    // creates a pairlist
    CompareFiles();

    log("sort");
    Fl::handle_(KBR_START_SORT, 0);
   
    // viewing pairlist may be filtered [no matching sources]
    FilterAndSort(filterSame);

    log ("done");
    Fl::handle_(KBR_DONE_LOAD, 0);
    
    return NULL;
}

Fl_Thread proc_thread;

void fire_proc_thread()
{
    fl_create_thread(proc_thread, proc_func, NULL);
    pthread_detach(*(pthread_t*)proc_thread);
}

