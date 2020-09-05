#include "MostRecentPaths.h"
#include <cstdlib> // free
#include <cstring> // strcmp

MostRecentPaths::MostRecentPaths(Fl_Preferences* root)
{
	_prefs = new Fl_Preferences(root, "MRU");
	char name[2] = "0";
	for (int i = 0; i < 10; i++)
	{
		_pathList.push_back(NULL);
		name[0] = '0' + i;
		_prefs->get(name, _pathList[i], "");
	}
}

MostRecentPaths::~MostRecentPaths()
{
	for (int i = 0; i < 10; i++)
		free((void*)(_pathList[i]));
	delete _prefs;
}

void MostRecentPaths::ShuffleDown(int stopAt)
{
	// assumes _pathList[stopAt] has been set to NULL
	for (long i = stopAt; i > 0; i--)
		_pathList[i] = _pathList[i - 1];
}

void MostRecentPaths::Add(const char* newPath)
{
	//// For FLTK paths must be in Unix format
	//while (char * res = strchr((char *)newPath, '/'))
	//	*res = '\\';

	// if path already exists in list, move to "most recent"
	// otherwise, set as "most recent" and lose "oldest"
	int reorder_to = -1;
	for (int i=0; i < 10; i++)
		if (strcmp(_pathList[i], newPath) == 0)
		{
			if (i == 0)
				return; // "new" already at MRU, do nothing
			reorder_to = i;
			break;
		}
	if (reorder_to == -1)
	{
		free(_pathList[9]);
		ShuffleDown(9);
		_pathList[0] = (char *)malloc(strlen(newPath) + 1);
		strcpy(_pathList[0], newPath);
	}
	else
	{
		char* temp = _pathList[reorder_to];
		ShuffleDown(reorder_to);
		_pathList[0] = temp;
	}
}

void MostRecentPaths::Save()
{
	char name[2] = "0";
	for (int i = 0; i < 10; i++)
	{
		name[0] = '0' + i;
		_prefs->set(name, _pathList[i]);
	}
	_prefs->flush();
}

char** MostRecentPaths::getAll()
{
	return &_pathList[0];
}

int MostRecentPaths::getCount()
{
	for (int i = 0; i < 10; i++)
		if (!_pathList[i] || _pathList[i][0] == '\0')
			return i;
	return 10;
}
