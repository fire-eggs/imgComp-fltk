#include "filedata.h"
#include <FL/Fl.H> // fl_fopen
#include <stdlib.h>
#include <algorithm>

FileSet _data;
std::vector<Pair*>* _pairlist;

#define LIMIT 50000

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
    // TODO danbooru .phashc files do NOT have the firstline!
    // TODO need general mechanism to xlate Windows drive letters!

    FILE* fptr = fl_fopen(filename, "r");
    char buffer[2048];
    int limit = LIMIT;
    bool firstLine = true;
    while (fgets(buffer, 2048, fptr))
    {
        if (firstLine)
        {
            firstLine = false;
            continue;
        }

        if (!buffer[0])
            continue;

        char* parts = strtok(buffer, "|");
        char* end;
        unsigned long long phash = strtoull(parts, &end, 10);

        parts = strtok(NULL, "|");
        unsigned long crc = strtoul(parts, &end, 10);

        parts = strtok(NULL, "|");
        parts[strlen(parts) - 1] = '\0'; //kill trailing newline
        FileData* fd = new FileData();
        fd->Name = new std::string(parts);
        
        size_t dex = fd->Name->find("g:\\");
        if (dex != std::string::npos)
        {
            fd->Name->replace(dex,3, gMount);
            replaceStrChar(fd->Name, "\\", '/');
            fd->Name->erase(remove(fd->Name->begin(), fd->Name->end(), '\r'), fd->Name->end());
        }
        else
        {
            size_t dex = fd->Name->find("y:\\");
            if (dex != std::string::npos)
            {
                fd->Name->replace(dex,3, yMount);
                replaceStrChar(fd->Name, "\\", '/');
                fd->Name->erase(remove(fd->Name->begin(), fd->Name->end(), '\r'), fd->Name->end());
            }
        }
        
        fd->CRC = crc;
        fd->PHash = phash;
        fd->Source = sourceId;
        fd->Vals = NULL;
        fd->FVals = NULL;

        _data.Add(fd);

        limit--;
        if (limit < 0)
            break;
    }
    fclose(fptr);
}

void ClearPairList()
{
    if (_pairlist)
    {
        // TODO delete each Pair
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

static int THRESHOLD = 11;

void CompareOneFile(int me)
{
    FileData* my = _data.Get(me);
    size_t count = _data.Count();
    for (int j = me + 1; j < count; j++)
    {
        FileData* they = _data.Get(j);
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
    // TODO this will be the number of filtered pairs
    return _pairlist->size(); 
}

char* GetPairText(int who) // the text to display for a specific pair
{
    char buff[2048];
    buff[0] = '\0';

    Pair* p = _pairlist->at(who);
    if (p->Val == 0 && p->CRCMatch)
        strcat(buff, "DUP|");
    else
        sprintf(buff, "%03d|", p->Val);
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
    return _pairlist->at(who);
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

    //int delta = me->Val - other->Val;
    //if (delta == 0)
    //{
    //    if (me->CRCMatch != other->CRCMatch)
    //    {
    //        if (me->CRCMatch)
    //            return -1;
    //        if (other->CRCMatch)
    //            return 1;
    //    }
    //    //return Name.compare(other.Name);
    //}
    //return delta;
}

void FilterAndSort(bool filter)
{
    std::sort(_pairlist->begin(), _pairlist->end(), Compare);

    // TODO filter
}
