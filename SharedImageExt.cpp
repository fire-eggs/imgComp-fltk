#include "SharedImageExt.h"

#include "Webp.h"

#include <FL/Fl_SVG_Image.H>
#include <FL/Fl_GIF_Image.H>
#include <FL/Fl_BMP_Image.H>
#include <FL/Fl_PNM_Image.H>

#define HAVE_LIBPNG
#include <FL/Fl_PNG_Image.H>
#define HAVE_LIBJPEG
#include <FL/Fl_JPEG_Image.H>
#include "Fl_Anim_GIF_Image.h"

// TODO should animated GIF be separate instead of in Fl_Shared_image?

#if false
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
#endif

Fl_Image *                                      // O - Image, if found
fl_check_images(const char *name,               // I - Filename
                uchar      *header,             // I - Header data from file
                int headerlen) {                // I - Amount of data

    if (headerlen < 6) // not a valid image
        return nullptr;

    if (memcmp(header, "GIF87a", 6) == 0 ||
        memcmp(header, "GIF89a", 6) == 0) // GIF file
        return new Fl_GIF_Image(name);

    if (memcmp(header, "BM", 2) == 0)     // BMP file
        return new Fl_BMP_Image(name);

    if (header[0] == 'P' && header[1] >= '1' && header[1] <= '7')
        // Portable anymap
        return new Fl_PNM_Image(name);

#ifdef HAVE_LIBPNG
    if (memcmp(header, "\211PNG", 4) == 0)// PNG file
    return new Fl_PNG_Image(name);
#endif // HAVE_LIBPNG

#ifdef HAVE_LIBJPEG
    if (memcmp(header, "\377\330\377", 3) == 0 && // Start-of-Image
      header[3] >= 0xc0 && header[3] <= 0xfe)   // APPn .. comment for JPEG file
    return new Fl_JPEG_Image(name);
#endif // HAVE_LIBJPEG

#ifdef FLTK_USE_SVG
#  if defined(HAVE_LIBZ)
    if (header[0] == 0x1f && header[1] == 0x8b) { // denotes gzip'ed data
    int fd = fl_open_ext(name, 1, 0);
    if (fd < 0) return NULL;
    gzFile gzf =  gzdopen(fd, "r");
    if (gzf) {
      gzread(gzf, header, headerlen);
      gzclose(gzf);
    }
  }
#  endif // HAVE_LIBZ

// KBR handle BOM
    if (memcmp(header, "<svg", 4) == 0 ||
        memcmp(&(header[3]), "<svg", 4) == 0 ||
        memcmp(header, "<?xml", 5) == 0 ||
        memcmp(&(header[3]), "<?xml", 5) == 0 )
    {
        return new Fl_SVG_Image(name);
    }
#endif // FLTK_USE_SVG

    return 0;
}

Fl_Image *loadFile(char *filename, Fl_Widget *owner)
{
    // 1. Try to open as webp [animated or not]
    Fl_Image *img = LoadWebp(filename, owner);
    if (img)
        return img;

#ifdef APNG
    img = LoadAPNG(filename, owner);
    if (img)
        return img;
#endif

    // 2. Try to open as (animated) gif
    // TODO KBR 20210921 : hack, let Fl_Anim_GIF_Image also hold non-animated. Allows some sort of view.
    Fl_Anim_GIF_Image *animgif = new Fl_Anim_GIF_Image(filename, nullptr, Fl_Anim_GIF_Image::Start);
    //if (animgif && animgif->valid() && animgif->is_animated()) // TODO can use fail() ?
    if (animgif && animgif->valid())
    {
        animgif->canvas(owner, Fl_Anim_GIF_Image::Flags::DontResizeCanvas |
                               Fl_Anim_GIF_Image::Flags::DontSetAsImage);
        return animgif;
    }

    delete animgif;

    FILE *fp;
    uchar header[64];
    int count=0;

    if ((fp = fl_fopen(filename, "rb")) != NULL) {
        count = fread(header, 1, sizeof(header), fp);
        fclose(fp);
        if (count == 0)
            return nullptr;
    } else {
        return nullptr;
    }

    return fl_check_images(filename, header, count);
}
