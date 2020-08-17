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
void FilterAndSort(bool);

