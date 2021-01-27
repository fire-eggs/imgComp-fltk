#pragma once

#include <string>

std::string getArchivePath(int);
std::string getArchiveName(int);
int getArchiveFileCount(int);

int addArchive(const char* archivePath);

void compareArchives();
void sortArchives();

void clearArchiveData();