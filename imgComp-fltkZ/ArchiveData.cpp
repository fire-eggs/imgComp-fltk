#include <vector>
#include <map>
#include <utility>

#include "filedata.h"

// archive-path to archive index
std::map<std::string, int>* archiveDict;
// list of archive paths
std::vector<std::string>* archiveList;

// <archive-index-pair> to <filePairlist>
typedef std::map < std::pair<int, int>, std::vector<Pair*>*> ArchMapType;
ArchMapType* _archList;

// Add a new archive file to the archive list
int addArchive(const char* archivePath)
{
    // TODO should be std::string arg for unicode filenames?
    // TODO would like to keep track of file count associated to archive

    if (!archiveDict)
    {
        archiveDict = new std::map<std::string, int>();
        archiveList = new std::vector<std::string>();

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
        return dex;
    }
    else
        return it->second;
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
}