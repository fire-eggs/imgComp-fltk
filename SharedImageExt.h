#pragma once
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Anim_GIF_Image.H>
#include <webp\decode.h>

extern Fl_Shared_Image* LoadWebp(const char*, WebPDecBuffer**);

// TODO animated
// TODO get frame(s)
// TODO animated webp

class SharedImageExt
{
private:
	Fl_Shared_Image* _img;
	WebPDecBuffer* _extra;

public:
	SharedImageExt(Fl_Shared_Image* img, WebPDecBuffer* webpExtra) 
	{
		_img = img;
		_extra = webpExtra;
	}

	~SharedImageExt()
	{
		if (!_extra)
			WebPFreeDecBuffer(_extra);
		while (_img->refcount() > 0)
			_img->release();
	}

	void release()
	{
		if (_img->refcount() <= 1 && _extra)
			WebPFreeDecBuffer(_extra);
		_img->release();
	}

	Fl_Image* image()
	{
		Fl_Anim_GIF_Image *animgif_ = dynamic_cast<Fl_Anim_GIF_Image*>(_img->KBR());
		if (animgif_)
			return animgif_->image(0); // always return frame 0 for static image
		return _img;
	}

	Fl_Image* baseImage()
	{
		return _img;
	}

	static SharedImageExt* LoadImage(const char* path);
};