#pragma once

#include <string>

std::string getArchivePath(int);
std::string getArchiveName(int);
int getArchiveFileCount(int);

int addArchive(const char* archivePath);

void compareArchives();
void sortArchives();

void clearArchiveData();

// A pair of archives.
struct ArchPair
{
    int score; // percentage match : may exceed 100% due to multiple matching files
    int archId1; // the id of the "left" archive in the archive list
    int archId2; // the id of the "right" archive in the archive list
    std::vector<Pair*>* files;  // file pairs matching between the archives

    // NOTE: does not include those files that don't match
};

size_t getArchPairCount();
char* getArchPairText(int index);
void* getArchPairData(int index);
ArchPair* getArchPair(int index);