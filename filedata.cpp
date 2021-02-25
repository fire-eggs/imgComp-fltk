#include "filedata.h"
#include <FL/Fl.H> // fl_fopen
#include <FL/fl_ask.H> // fl_alert
//#include <stdlib.h>
#include <algorithm>

#include <omp.h>
#include <mutex>
#include "logging.h"

FileSet _data;
std::vector<Pair*>* _pairlist;
std::vector<Pair*>* _viewlist;

#define LIMIT 50000000

std::string* replaceStrChar(std::string *str, const std::string& replace, char ch) 
{
  // set our locator equal to the first appearance of any character in replace
  size_t found = str->find_first_of(replace);

  while (found != std::string::npos) { // While our position in the sting is in range.
      str->at(found) = ch;
    //str[found] = ch; // Change the character at position.
    found = str->find_first_of(replace, found+1); // Relocate again.
  }

  return str; // return our new string.
}

const char * gMount = "/run/user/1000/gvfs/smb-share:domain=troll,server=troll,share=g,user=kevin/";
const char * yMount = "/run/user/1000/gvfs/smb-share:domain=192.168.111.157,server=192.168.111.157,share=sambashare,user=guest/";

void readPhash(char* filename, int sourceId)
{
    // TODO need general mechanism to xlate Windows drive letters!

    FILE* fptr = fl_fopen(filename, "r");
    if (!fptr)
        return; // TODO remove from MRU list (?)
    char buffer[2048];
    int limit = LIMIT;
    bool firstLine = true;
    int is_animated = false;
    while (fgets(buffer, 2048, fptr))
    {
        if (firstLine)
        {
            firstLine = false;
            continue;
        }

        if (!buffer[0])
            continue;

        // Lines of the form <hash>|<crc>|<animated>|<filepath>

        char* parts = strtok(buffer, "|");
        if (!parts) // unexpected data
            continue;

        unsigned long long phash = strtoull(parts, NULL, 10);
        if (phash == 0ULL)
            continue; // unexpected data: assuming hash cannot be zero

        parts = strtok(NULL, "|");
        if (!parts) // unexpected data
            continue;

        unsigned long crc = strtoul(parts, NULL, 10);
        if (crc == 0ULL)
            continue; // unexpected data: assuming CRC cannot be zero

        parts = strtok(NULL, "|");
        if (!parts) // unexpected data
            continue;

        if (strlen(parts) == 1)  // phash-fltk
        {
            is_animated = parts[0] == '1';

        parts = strtok(NULL, "|");
        if (!parts) // unexpected data
            continue;
        }

        parts[strlen(parts) - 1] = '\0'; //kill trailing newline
        FileData* fd = new FileData();
        fd->Name = new std::string(parts);
        
        // TODO assuming the .phashc files are in Windows format
#ifndef _WIN32
        // None of this magic is necessary if a symbolic link is created in the
        // program's working folder:
        //
        // ln -s '/run/user/1000/gvfs/smb-share:domain=troll,server=troll,share=g,user=kevin' g:
        //
        // NOTE the case must match that used in the .phashc file
        //
#ifdef TEST        
        size_t dex = fd->Name->find("g:\\");
        if (dex != std::string::npos)
        {
            fd->Name->replace(dex,3, gMount);
        }
        else
        {
            size_t dex = fd->Name->find("y:\\");
            if (dex != std::string::npos)
            {
                fd->Name->replace(dex,3, yMount);
            }
        }
#endif
        // translate any windows paths to unix [slashes and trailing carriage-return
        replaceStrChar(fd->Name, "\\", '/');
        fd->Name->erase(remove(fd->Name->begin(), fd->Name->end(), '\r'), fd->Name->end());
#endif

        fd->CRC = crc;
        fd->PHash = phash;
        fd->Source = sourceId;
        fd->Vals = NULL;
        fd->FVals = NULL;
        fd->Animated = is_animated;
        fd->Archive = -1;
        fd->ActualPath = NULL;

        _data.Add(fd);

        limit--;
        if (limit < 0)
            break;
    }
    fclose(fptr);
}

void ClearPairList()
{
    if (_viewlist && _viewlist != _pairlist)
        delete _viewlist;
    _viewlist = NULL;
    
    if (_pairlist)
    {
        // TODO delete each Pair?
        delete _pairlist;
    }
    _pairlist = new std::vector<Pair*>();
}

void ClearData()
{
    _data.Clear();
}

int phashHamDist(unsigned long long val1, unsigned long long val2)
{
    unsigned long long x = val1 ^ val2;

    const unsigned long long m1 = 0x5555555555555555ULL;
    const unsigned long long m2 = 0x3333333333333333ULL;
    const unsigned long long h01 = 0x0101010101010101ULL;
    const unsigned long long m4 = 0x0f0f0f0f0f0f0f0fULL;

    x -= (x >> 1) & m1;
    x = (x & m2) + ((x >> 2) & m2);
    x = (x + (x >> 4)) & m4;
    return (int)((x * h01) >> 56);
}

// Throw out any pair where delta exceeds 6
static int THRESHOLD = 15; // Hamming distance always a multiple of two

std::mutex _pair_lock;

void CompareOneFile(int me)
{
    FileData* my = _data.Get(me);
    size_t count = _data.Count();
#pragma omp parallel for
    for (int j = me + 1; j < count; j++)
    {
        FileData* they = _data.Get(j);

        // animated and non-animated images can't match
        if (my->Animated != they->Animated)
            continue;

        int val = phashHamDist(my->PHash, they->PHash);
        if (val < THRESHOLD)
        {
            Pair* p = new Pair();
            p->Val = val / 2;
            p->FileLeftDex = me;
            p->FileRightDex = j;
            p->CRCMatch = (my->CRC == they->CRC) &&
                           my->CRC != 0 &&
                           they->CRC != 0;
            p->valid = true; // assume valid until otherwise

            std::lock_guard<std::mutex> guard(_pair_lock);
            _pairlist->push_back(p);
        }
    }
}

void CompareFiles()
{
    ClearPairList();
    for (int i = 0; i < _data.Count(); i++)
        CompareOneFile(i);
}

size_t GetPairCount() // the number of visible pairs
{
    if (!_viewlist)
        return 0;
    return _viewlist->size(); 
}

char* GetPairText(int who) // the text to display for a specific pair
{
    char buff[2048];
    buff[0] = '\0';

    Pair* p = _viewlist->at(who);
    if (p->Val == 0 && p->CRCMatch)
        strcat(buff, "DUP | ");
    else
        sprintf(buff, "%03d | ", p->Val);
    FileData* dataL = _data.Get(p->FileLeftDex);
    FileData* dataR = _data.Get(p->FileRightDex);

    strcat(buff, dataL->Name->c_str());
    strcat(buff, "|");
    strcat(buff, dataR->Name->c_str());

    char* clone = new char[strlen(buff)+1];
    strcpy(clone, buff);
    return clone;
}

void* GetPairData(int who) // the data to store for a specific pair
{
    return (void*)(intptr_t)who;
}

Pair* GetPair(int who)
{
    if (!_viewlist || who >= GetPairCount())
        return NULL;
    return _viewlist->at(who);
}

FileData* GetFD(int who)
{
    return _data.Get(who);
}

// C++ compare function : return true if me < other
int Compare(Pair *me, Pair* other)
{
    if (me->Val < other->Val)
        return true;
    if (me->Val > other->Val)
        return false;
    if (me->CRCMatch && other->CRCMatch)
        return _data.Get(me->FileLeftDex)->Name <
               _data.Get(other->FileLeftDex)->Name;
    if (me->CRCMatch)
        return true;
    return false;
}

std::vector<Pair*> *FilterMatchingSources()
{
    std::vector<Pair*> *viewlist = new std::vector<Pair*>();
    for (int i = 0; i < _pairlist->size(); i++)
    {
        Pair *p = _pairlist->at(i);
        FileData* dataL = _data.Get(p->FileLeftDex);
        FileData* dataR = _data.Get(p->FileRightDex);
        
        if (dataL->Source != dataR->Source)
            viewlist->push_back(p);
    }
    return viewlist;
}

void FilterAndSort(bool filter)
{
    if (!_pairlist)
        return; 

    std::sort(_pairlist->begin(), _pairlist->end(), Compare);

    // filter
    if (filter)
        _viewlist = FilterMatchingSources();
    else
        _viewlist = new std::vector<Pair*>(*_pairlist);
}

#include <FL/filename.H>

bool MoveFile(const char *nameForm, const char *destpath, const char *srcpath)
{
    //printf("MoveFile: srcpath: %s\n", srcpath);
    //printf("MoveFile: destpath: %s\n", destpath);

    // rename the source file to be a duplicate of the destination
    // i.e. <sourcebase>/<srcfile> to <sourcebase>/dup0_<destfile>
    
    // 1. determine the destination file name [not path!]
    const char* destfname = fl_filename_name(destpath);
        
    // 2. get the source "base" path [path up to the filename]        
    const char *srcbase = strrchr(srcpath, '/');
    if (!srcbase)
        srcbase = strrchr(srcpath, '\\');
        //return false; // TODO source not a legal path+filename ?
        
    char spath[MAXNAMLEN];
    memset(spath, 0, MAXNAMLEN);
    strncpy(spath, srcpath, (srcbase - srcpath)); // NOTE: do not want the trailing slash
    
    // 3. test up to 10 numbered names, using provided format
    char buff[MAXNAMLEN];
    int i;
    for (i = 0; i < 10; i++)
    {
        sprintf(buff, nameForm, spath, i, destfname);
        
        //printf("MoveFile: outpath: %s\n", buff);       
        
        log("MoveFile: attempt to rename %s to %s", srcpath, buff);
        if (fl_access(buff, F_OK) != -1)
            log("MoveFile: target file already exists");
        else
        {
            int res = rename(srcpath, buff);
            if (res)
                log("MoveFile: failed to rename to target file");
            else
            {
                log("MoveFile: success");
                break; // success
            }
        }
    }
    if (i == 10)
        fl_alert("All attempts to rename the file failed. See the log.");
    return true;
}

void RemoveMissingFile(int filedex)
{
    // Remove all instances of the given file from the list
    int len = _viewlist->size() - 1;
    for (int i= len; i >= 0; i--)
    {
        Pair *p = _viewlist->at(i);
        if (p->FileLeftDex == filedex ||
            p->FileRightDex == filedex)
            _viewlist->erase(_viewlist->begin() + i);
    }
}

const char* GetActualPath(Pair* p, bool left)
{
    FileData* fd = GetFD(left ? p->FileLeftDex : p->FileRightDex);
    //if (fd->Archive == -1)
    return fd->Name->c_str(); // TODO memory leak?
}

void compareArchives() {}
void pixVsArchives() {}

bool checkAnyStandalone() { return true; }