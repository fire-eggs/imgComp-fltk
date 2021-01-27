#include <vector>
#include <map>
#include <utility>
#include <algorithm>

#include "filedata.h"
#include "archiveData.h"

// archive-path to archive index
std::map<std::string, int>* archiveDict = NULL;
// list of archive paths
std::vector<std::string>* archiveList;
// list of archive file counts
std::vector<int>* countList;

// <archive-index-pair> to <filePairlist>
typedef std::map < std::pair<int, int>, std::vector<Pair*>*> ArchMapType;
ArchMapType* _archList = NULL;

void clearArchiveData()
{
    delete archiveDict;
    delete archiveList;
    delete countList;
    delete _archList;
    archiveDict = NULL;
    _archList = NULL;
}

// Add a new archive file to the archive list. Also tracks the number
// of image files associated to that archive [as this function is 
// called once per file].
int addArchive(const char* archivePath)
{
    // TODO should be std::string arg for unicode filenames?
 
    if (!archiveDict)
    {
        archiveDict = new std::map<std::string, int>();
        archiveList = new std::vector<std::string>();
        countList = new std::vector<int>();
    }
    std::string lookup(archivePath);

    // TODO replace with emplace
    std::map<std::string, int>::iterator it;
    it = archiveDict->find(lookup);
    if (it == archiveDict->end())
    {
        // archive name not seen before.
        archiveList->push_back(lookup);
        int dex = archiveList->size() - 1;
        (*archiveDict)[lookup] = dex;
        countList->push_back(1); // first image
        return dex;
    }
    else
    {
        int dex = it->second;
        (*countList)[dex]++;
        return it->second;
    }
}

int getArchiveFileCount(int archiveId)
{
    if (archiveId < 0)
        return 0;
    return (*countList)[archiveId];
}

// The archive path.
std::string getArchivePath(int archiveId)
{
    return (*archiveList)[archiveId];
}

// The archive filename (no path)
// Empty string if not an archive
std::string getArchiveName(int archiveId)
{
    if (archiveId < 0)
        return "";
    std::string apath = getArchivePath(archiveId);
    size_t p = apath.rfind("\\");
    if (p == std::string::npos)
        return "";
    return apath.substr(p+1);
}

// interate across file pairs
// transform into an <archive-pair> to <filepairlist> mapping
//
// in theory then the number of entries in the <filepairlist>
// will suggest the amount of overlap between two archives
void compareArchives()
{
    // TODO limitation: is using _viewlist, not _pairlist! Required to filter sources but also means low-match files not included

    if (!_archList)
    {
        _archList = new std::map<std::pair<int, int>, std::vector<Pair*>*>();
    }

    if (!GetPairCount())
        return; // nothing to do

    // Iterate across file pairs.
    for (int i = 0; i < GetPairCount(); i++)
    {
        Pair* p = GetPair(i);
        FileData* left = GetFD(p->FileLeftDex);
        FileData* rigt = GetFD(p->FileRightDex);

        if (left->Archive < 0 ||
            rigt->Archive < 0)
            continue; // at least one file not in an archive

        // a tuple of archive ids
        std::pair<int,int> archPair;
        archPair.first = left->Archive;
        archPair.second = rigt->Archive;

        // accumulate into a map of <archivepair> -> <list of file pairs>
        std::pair<ArchMapType::iterator, bool> res =
            _archList->emplace(archPair, (std::vector<Pair*>*)NULL);
        if (res.second)
        {
            res.first->second = new std::vector<Pair*>();
        }
        res.first->second->push_back(p);
    }

    sortArchives(); // TODO call via proc_thread instead?
}

struct ArchPair
{
    int score; // percentage match
    int ArchId1;
    int ArchId2;
    std::vector<Pair*>* files;
};
std::vector<ArchPair*>* _archPairList = NULL;

// C++ compare function : return true if me > other FOR REVERSE SORT
int apCompare(ArchPair* me, ArchPair* other)
{
    return me->score > other->score;
}

void sortArchives()
{
    if (_archPairList)
        delete _archPairList;
    _archPairList = new std::vector<ArchPair*>();

    for (ArchMapType::iterator i = _archList->begin(); i != _archList->end(); i++)
    {
        ArchPair* p = new ArchPair();
        p->ArchId1 = i->first.first;
        p->ArchId2 = i->first.second;
        p->files = i->second;

        float fcount = p->files->size();
        int a1 = getArchiveFileCount(p->ArchId1);
        int a2 = getArchiveFileCount(p->ArchId2);
        p->score = (int)((fcount / a1 + fcount / a2) * 50.0f); // percentage

        _archPairList->push_back(p);
    }

    std::sort(_archPairList->begin(), _archPairList->end(), apCompare);
}