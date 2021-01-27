#pragma once

#include <string>

std::string getArchivePath(int);
std::string getArchiveName(int);

int addArchive(const char* archivePath);

void compareArchives();