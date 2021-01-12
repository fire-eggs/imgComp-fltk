#pragma once

#include "filedata.h"

void initShow();
void showView(Pair* toview, bool startLeft);
void showDiff(Pair* toview, bool stretch);
void showDiff(Fl_Image* imgL, Fl_Image* imgR, bool stretch);
void showView(Fl_Image* leftI, Fl_Image* rightI, bool startLeft);

