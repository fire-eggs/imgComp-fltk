#pragma once

#include <string>
#include <vector>

struct FileData
{
    std::string* Name;
    int* Vals; // may be unused
    double* FVals; // may be unused
    int Source;
    unsigned long long PHash; // 64 bit
    unsigned long CRC;        // 32 bit
    int Animated;             // image is animated or not
    int Archive;              // from archive (as archive list index)
    std::string* ActualPath;    // where extracted from archive onto disk
    unsigned long long hash090; // 64 bit : calculated phash as rotated 90degrees clockwise
    unsigned long long hash180; // 64 bit : calculated phash as rotated 90degrees clockwise
    unsigned long long hash270; // 64 bit : calculated phash as rotated 90degrees clockwise
};

class Pair
{
public:
    int FileLeftDex;
    int FileRightDex;
    std::string op;  // TODO unused?
    int Val;
    double FVal;     // TODO unused?
    bool CRCMatch;
    bool valid;      // been removed/renamed at some point?
    bool rotate;     // match via rotation

    int Compare(Pair other)
    {
        int delta = Val - other.Val;
        if (delta == 0)
        {
            if (CRCMatch != other.CRCMatch)
            {
                if (CRCMatch)
                    return -1;
                if (other.CRCMatch)
                    return 1;
            }
            //return Name.compare(other.Name);
        }
        return delta;
    }
};

class FileSet
{
public:
    std::vector<FileData*> Infos;

    void Add(FileData* entry)
    {
        Infos.push_back(entry);
    }

    size_t Count()
    {
        return Infos.size();
    }

    FileData* Get(int index)
    {
        return Infos[index];
    }
    
    void Clear()
    {
        // TODO any memory leak from failing to delete contents of FileData instances?
        for (int i=0; i < Infos.size(); i++) {
            delete Infos[i]->Name;
            delete Infos[i]->ActualPath;
            delete Infos[i];
        }
        Infos.clear();
    }
};

void readPhash(char*, int);
void CompareFiles();
size_t GetPairCount(); // the number of visible pairs
char *GetPairText(int who); // the text to display for a specific pair
void* GetPairData(int who); // the data to store for a specific pair

Pair* GetPair(int dex);
FileData* GetFD(int dex);

void ClearPairList();
void ClearData();
void FilterAndSort(bool, bool);

bool MoveFile(const char *nameForm, const char *destpath, const char *srcpath);
void RemoveMissingFile(int filedex);

const char* GetActualPath(Pair* p, bool left);

bool checkAnyStandalone();

//int OSCopyFile(const char* source, const char* destination);
int cp(const char *to, const char *from);
