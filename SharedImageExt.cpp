#include "SharedImageExt.h"

// TODO should animated GIF be separate instead of in Fl_Shared_image?

SharedImageExt* SharedImageExt::LoadImage(const char* path)
{
	WebPDecBuffer* extra = NULL;
	Fl_Shared_Image *img = LoadWebp(path, &extra);
	if (!img)
		img = Fl_Shared_Image::get(path);
	if (!img)
		return NULL;

	return new SharedImageExt(img, extra);
}