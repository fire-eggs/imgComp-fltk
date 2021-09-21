//
// Image display widget header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 2002-2005 by Michael Sweet.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#ifndef Fl_Image_Display_H
#  define Fl_Image_Display_H
#  include <FL/Fl_Group.H>
#  include <FL/Fl_Scrollbar.H>
#  include <FL/Fl_Shared_Image.H>
#ifdef ANIMGIF
# include "Fl_Anim_GIF_Image.h"
#endif

enum
{
  FL_IMAGE_PAN,
  FL_IMAGE_ZOOM_IN,
  FL_IMAGE_ZOOM_OUT,
  FL_IMAGE_CLICK,
  FL_SLIDE
};

enum class ZoomMode
{
    Auto,
    ScaleToHigh,
    ScaleToWide,
    ScaleToFit,
    ScaleToFill,
    FullSize
};

class FL_EXPORT Fl_Image_Display : public Fl_Group
{
  static uchar		gamma_lut_[256];

  int			mode_;

  Fl_Image	*value_;
#ifdef ANIMGIF  
  Fl_Anim_GIF_Image *animgif_; // KBR
#endif

  Fl_Scrollbar		xscrollbar_;
  Fl_Scrollbar		yscrollbar_;
  double			factor_;
  int			xsize_;
  int			xstep_;
  int			xmod_;
  int			ysize_;
  int			start_ex_, start_ey_;
  int			start_x_, start_y_;
  int			mouse_x_, mouse_y_;
  int			last_x_, last_y_;
  ZoomMode      zoomMode_;

  static void	image_cb(void *p, int X, int Y, int W, uchar *D);
  static void	scrollbar_cb(Fl_Widget *w, void *d);
  void		update_mouse_xy();
  void		update_scrollbars();
  void drawcheckerboard(int, int, int, int);

  public:
      bool   _scrollbarsOn;
      bool   _drawChecker;

  protected:

    void draw() override;

  public:

  Fl_Image_Display(int X, int Y, int W, int H);
  ~Fl_Image_Display();

  int			handle(int event) override;
  void			mode(int m) { mode_ = m; }
  int			mode() const { return mode_; }
  int			mouse_x() const { return mouse_x_; }
  int			mouse_y() const { return mouse_y_; }
  void			position(int X, int Y);
  void			resize(int X, int Y, int W, int H) override;
  void			scale(double factor);
  double		scale() const { return factor_; }
  int			start_x() const { return start_x_; }
  int			start_y() const { return start_y_; }
  void			value(Fl_Image *v);
  Fl_Image	    *value() { return value_; }
  int			xposition() { return xscrollbar_.value(); }
  int			yposition() { return yscrollbar_.value(); }

  static void		set_gamma(float val = 2.2f);

  void zoomDelta(int);
  void set_zoommode(ZoomMode);
  void zoomIntoMouse();
  void zoomIn();

  typedef void (Zoom_Callback)(int);
  Zoom_Callback* _zoomCallback;
  void setZoomCallback(Zoom_Callback* v) { _zoomCallback = v; }
};

#endif // !Fl_Image_Display_H
