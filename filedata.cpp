#include <cstdarg> // va_start
#include <algorithm>
#include <omp.h>
#include <mutex>

#include <FL/Fl.H> // fl_fopen
#include <FL/fl_ask.H> // fl_alert
#include "filedata.h"
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

        // Lines of the form:
        // phash-fltk:  <hash>|<crc>|<animated>|<hash>|<hash>|<hash>|<filepath>
        // testapp: <hash>|<crc>|<hash>|<hash>|<hash>|<filepath>

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
        /* if CRC is zero, can't be used
        if (crc == 0ULL)
            continue; // unexpected data: assuming CRC cannot be zero
        */

        parts = strtok(NULL, "|");
        if (!parts) // unexpected data
            continue;

        unsigned long long hash090 = 0;
        unsigned long long hash180 = 0;
        unsigned long long hash270 = 0;
        if (strlen(parts) == 1)  // phash-fltk
        {
            is_animated = parts[0] == '1';

            parts = strtok(NULL, "|");
            if (!parts) // unexpected data
                continue;
        }

        hash090 = strtoull(parts, NULL, 10);
        parts = strtok(NULL, "|");
        if (!parts) // unexpected data
            continue;

        hash180 = strtoull(parts, NULL, 10);
        parts = strtok(NULL, "|");
        if (!parts) // unexpected data
            continue;

        hash270 = strtoull(parts, NULL, 10);
        parts = strtok(NULL, "|");
        if (!parts) // unexpected data
            continue;

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

        fd->hash090 = hash090;
        fd->hash180 = hash180;
        fd->hash270 = hash270;
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
        for (int i=0; i<_pairlist->size(); i++)
            delete _pairlist->at(i);
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
static const int MINTHRESHOLD =  0; // Hamming distance always a multiple of two
static const int MAXTHRESHOLD = 13; // Hamming distance always a multiple of two

std::mutex _pair_lock;

char msg_buf[1024]; // must be global to last beyond end of message() function

void message_cb(void *);

void message(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsnprintf(msg_buf, 1024, format, ap);
    Fl::awake(reinterpret_cast<Fl_Awake_Handler>(message_cb), msg_buf);
    va_end(ap);
}

void CompareOneFile(int me)
{
    FileData* my = _data.Get(me);
    size_t count = _data.Count();
#pragma omp parallel for default(none) shared(me,count,_data,_pairlist,_pair_lock, my)
    for (int j = me + 1; j < count; j++)
    {
        FileData* they = _data.Get(j);

        // animated and non-animated images can't match
        if (my->Animated != they->Animated)
            continue;

        int val0 = phashHamDist(my->PHash, they->PHash);
        int val1 = phashHamDist(my->PHash, they->hash090);
        int val2 = phashHamDist(my->PHash, they->hash180);
        int val3 = phashHamDist(my->PHash, they->hash270);
        int val = std::min(val0, val1);
        val = std::min(val, val2);
        val = std::min(val, val3);
        if (val >= MINTHRESHOLD && val < MAXTHRESHOLD)
        {
            Pair* p = new Pair();
            p->Val = val / 2;
            p->FileLeftDex = me;
            p->FileRightDex = j;
            p->CRCMatch = (my->CRC == they->CRC) &&
                           my->CRC != 0 &&
                           they->CRC != 0;
            p->rotate = val != val0; // true if match via rotation
            p->valid = true; // assume valid until otherwise

            // TODO mark pair based on match type (r90, r180, etc)
            std::lock_guard<std::mutex> guard(_pair_lock);
            _pairlist->push_back(p);
        }
    }
}

void CompareFiles()
{
    ClearPairList();
    int count = _data.Count();
    for (int i = 0; i < count; i++)
    {
        if ((i % 100) == 0)
            message("Comparing %d of %d", i, count);
        CompareOneFile(i);
    }
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
        sprintf(buff, "DUP%c | ", p->rotate ? 'R' : ' ');
    else
        sprintf(buff, "%03d%c | ", p->Val, p->rotate ? 'R' : ' ');
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

std::vector<Pair*> *FilterMatchingSources(bool same, bool rotation)
{
    std::vector<Pair*> *viewlist = new std::vector<Pair*>();
    for (int i = 0; i < _pairlist->size(); i++)
    {
        Pair *p = _pairlist->at(i);
        FileData* dataL = _data.Get(p->FileLeftDex);
        FileData* dataR = _data.Get(p->FileRightDex);

        if (same && dataL->Source == dataR->Source)
            continue;
        if (rotation && p->rotate)
            continue;

        viewlist->push_back(p);
    }
    return viewlist;
}

void FilterAndSort(bool filterSame, bool filterRotation)
{
    if (!_pairlist)
        return; 

    std::sort(_pairlist->begin(), _pairlist->end(), Compare);

    if (_viewlist && _viewlist != _pairlist)
        delete _viewlist;
    _viewlist = NULL;

    // filter
    if (filterSame || filterRotation)
        _viewlist = FilterMatchingSources(filterSame, filterRotation);
    else
        _viewlist = new std::vector<Pair*>(*_pairlist);
}

#include <FL/filename.H>

bool MoveFile(const char *nameForm, const char *destpath, const char *srcpath)
{
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
                
        log("MoveFile: attempt to rename \n%s to \n%s", srcpath, buff);
        if (fl_access(buff, F_OK) != -1)
            log("MoveFile: target file already exists");
        else
        {
            int res = rename(srcpath, buff);
            if (res)
                log("MoveFile: failed to rename to target file (%d)", errno);
            else
            {
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

/*
#include <fcntl.h>
#include <unistd.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <copyfile.h>
#else
#include <sys/sendfile.h>
#endif

int OSCopyFile(const char* source, const char* destination)
{
    int input, output;
    if ((input = open(source, O_RDONLY)) == -1)
    {
        return -1;
    }
    if ((output = creat(destination, 0660)) == -1)
    {
        close(input);
        return -1;
    }

    //Here we use kernel-space copying for performance reasons
#if defined(__APPLE__) || defined(__FreeBSD__)
    //fcopyfile works on FreeBSD and OS X 10.5+
    int result = fcopyfile(input, output, 0, COPYFILE_ALL);
#else
    //sendfile will work with non-socket output (i.e. regular file) on Linux 2.6.33+
    off_t bytesCopied = 0;
    struct stat fileinfo = {0};
    fstat(input, &fileinfo);
    int result = sendfile(output, input, &bytesCopied, fileinfo.st_size);
#endif

    close(input);
    close(output);

    return result;
}
*/

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

// This is necessary rather than runtime version to handle copies across the network (?)
int cp(const char *to, const char *from)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return -1;

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

    out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}
