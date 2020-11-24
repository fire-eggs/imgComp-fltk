#pragma once

#include "filedata.h"

void initShow();
void showView(Pair* toview, bool startLeft);
void showDiff(Pair* toview, bool stretch);
void showDiff(Fl_Image* imgL, Fl_Image* imgR, bool stretch);
void showView(Fl_Shared_Image* leftI, Fl_Shared_Image* rightI, bool startLeft);

