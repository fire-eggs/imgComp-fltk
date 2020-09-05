#pragma once

#include <vector>
#include <FL/Fl_Preferences.H>

class MostRecentPaths
{
private:
	std::vector<char*> _pathList;
	Fl_Preferences* _prefs;
	void ShuffleDown(int);
	MostRecentPaths(const MostRecentPaths&) = delete; // no copying!
	MostRecentPaths& operator=(const MostRecentPaths&) = delete;

public:
	MostRecentPaths(Fl_Preferences* prefs);
	~MostRecentPaths();
	void Add(const char* newPath);
	char** getAll();
	void Save();
	int getCount();
};

