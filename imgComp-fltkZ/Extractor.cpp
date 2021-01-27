#include "logging.h"

#include <filesystem>
namespace fs = std::filesystem;

// archive library
#include "bitextractor.hpp"
#include "bitexception.hpp"
using namespace bit7z;

#define TEMPDIR "E:\\tmp"
#define TEMPDIR2 "E:\\tmp\\"
#define TEMPDIRW L"E:\\tmp"

Bit7zLibrary* lib7z = NULL;
BitExtractor* extr7z = NULL;
BitExtractor* extrRar;
BitExtractor* extrZip;

#pragma warning(disable : 4996)

bool hasEnding(std::string const& fullString, std::string const& ending)
{
	if (fullString.length() >= ending.length())
	{
		// case-insensitive
		std::string end = fullString.substr(fullString.length() - ending.length(), ending.length());
		return 0 == _stricmp(end.c_str(), ending.c_str());
		//return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else
	{
		return false;
	}
}

void init()
{
	if (lib7z)
		return;

	lib7z = new Bit7zLibrary(L"7z.dll");
	extr7z = new BitExtractor(*lib7z, BitFormat::SevenZip);
	extrZip = new BitExtractor(*lib7z, BitFormat::Zip);
	extrRar = new BitExtractor(*lib7z, BitFormat::Rar);
}

#include <FL/fl_utf8.h> // for fl_utf8toUtf16()

static wchar_t* utf8_to_wchar(const char* utf8, wchar_t*& wbuf, int lg = -1) 
{
	unsigned len = (lg >= 0) ? (unsigned)lg : (unsigned)strlen(utf8);
	unsigned wn = fl_utf8toUtf16(utf8, len, NULL, 0) + 1; // Query length
	wbuf = (wchar_t*)realloc(wbuf, sizeof(wchar_t) * wn);
	wn = fl_utf8toUtf16(utf8, len, (unsigned short*)wbuf, wn); // Convert string
	wbuf[wn] = 0;
	return wbuf;
}


std::string _outpath;

const char* ExtractFile(std::string arcpath, std::string filepath)
{
	init();

	wchar_t* arcpathW = NULL;
	wchar_t* filepathW = NULL;
	utf8_to_wchar(arcpath.c_str(), arcpathW, -1);
	utf8_to_wchar(filepath.c_str(), filepathW, -1);

	std::wstring arcpath2(arcpath.begin(), arcpath.end());
	std::wstring fpath2(filepath.begin(), filepath.end());

	fs::create_directory(TEMPDIR);

	fs::path p(arcpath);
	std::string outdir0(TEMPDIR2);
	std::string outdir = outdir0 + p.stem().string() + std::string("\\");

	wchar_t* outdirW = NULL;
	utf8_to_wchar(outdir.c_str(), outdirW, -1);
	//std::wstring outdirW(outdir.begin(), outdir.end());

	fs::create_directory(outdir);
	try
	{
		if (hasEnding(arcpath, ".RAR") || hasEnding(arcpath, ".CBR"))
			extrRar->extractMatching(arcpathW, filepathW, outdirW);
		if (hasEnding(arcpath, ".ZIP") || hasEnding(arcpath, ".CBZ"))
			extrZip->extractMatching(arcpathW, filepathW, outdirW);
		if (hasEnding(arcpath, ".7Z") || hasEnding(arcpath, ".CB7"))
			extr7z->extractMatching(arcpathW, filepathW, outdirW);

		_outpath = (outdir + filepath);
		return (_outpath).c_str();
	}
	catch (const BitException& be)
	{
		log("Failed to extract from %s", (char*)(arcpath.c_str()));
	}
	return NULL;
}