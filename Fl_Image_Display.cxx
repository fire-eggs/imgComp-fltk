//
// "$Id: Fl_Image_Display.cxx 321 2005-01-23 03:52:44Z easysw $"
//
// Image display widget methods for the Fast Light Tool Kit (FLTK).
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
// Contents:
//
//   Fl_Image_Display::Fl_Image_Display()  - Create a new image display widget.
//   Fl_Image_Display::~Fl_Image_Display() - Destroy an image display widget.
//   Fl_Image_Display::draw()              - Draw the image display widget.
//   Fl_Image_Display::handle()            - Handle events in the widget.
//   Fl_Image_Display::image_cb()          - Provide a single line of an image.
//   Fl_Image_Display::position()          - Reposition the image on the screen.
//   Fl_Image_Display::resize()            - Resize the image display widget.
//   Fl_Image_Display::scale()             - Scale the image.
//   Fl_Image_Display::scrollbar_cb()      - Update the display based on the
//                                           scrollbar position.
//   Fl_Image_Display::set_gamma()         - Set the display gamma...
//   Fl_Image_Display::update_scrollbars() - Update the scrollbars.
//   Fl_Image_Display::update_mouse_xy()   - Update the mouse X and Y values.
//   Fl_Image_Display::value()             - Set the image to be displayed.
//

#include "Fl_Image_Display.H"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <stdio.h>
#include <math.h>
#include <FL/Fl_Tiled_Image.H>

//#include <FL/names.h>  // for debugging event names

//
// Absolute value macro...
//

#define abs(a) ((a) < 0 ? -(a) : (a))
#define max(a,b) ((a) > (b) ? (a) : (b))

//
// Scrollbar width...
//
#define SBWIDTH	17


//
// Gamma lookup table...
//
uchar	Fl_Image_Display::gamma_lut_[256];
// TODO gamma as an option


/* XPM */
static const char* const checker_xpm[] = {
"20 20 2 1",
" 	c #CCCCCCCCCCCC",
".	c #FFFFFFFFFFFF",
"          ..........",
"          ..........",
"          ..........",
"          ..........",
"          ..........",
"          ..........",
"          ..........",
"          ..........",
"          ..........",
"          ..........",
"..........          ",
"..........          ",
"..........          ",
"..........          ",
"..........          ",
"..........          ",
"..........          ",
"..........          ",
"..........          ",
"..........          " };

Fl_Tiled_Image* check;

//
// 'Fl_Image_Display::Fl_Image_Display()' - Create a new image display widget.
//

Fl_Image_Display::Fl_Image_Display(int X, int Y, int W, int H)
	: Fl_Group(X, Y, W, H),
	xscrollbar_(X, Y + H - SBWIDTH, W - SBWIDTH, SBWIDTH),
	yscrollbar_(X + W - SBWIDTH, Y, SBWIDTH, H - SBWIDTH)
{

	box(FL_DOWN_BOX);

	check = new Fl_Tiled_Image(new Fl_Pixmap((const char* const*)checker_xpm));

	//color(FL_GREEN);

	selection_color(FL_BACKGROUND_COLOR);
	labeltype(FL_NORMAL_LABEL);
	labelfont(0);
	labelsize(14);
	labelcolor(FL_FOREGROUND_COLOR);
	align(FL_ALIGN_TOP);
	when(FL_WHEN_RELEASE);

	end();

	value_ = 0;
#ifdef ANIMGIF    
	animgif_ = 0;
#endif    
	factor_ = 0.0;
	mode_ = FL_IMAGE_PAN;
	mouse_x_ = 0;
	mouse_y_ = 0;

	xscrollbar_.type(FL_HORIZONTAL);
	xscrollbar_.callback(scrollbar_cb, this);

	yscrollbar_.type(FL_VERTICAL);
	yscrollbar_.callback(scrollbar_cb, this);

	resize(X, Y, W, H);

	_scrollbarsOn = true;
	_drawChecker = false;

	zoomMode_ = ZoomMode::Auto; // TODO prefs?
	_zoomCallback = NULL;
}

//
// 'Fl_Image_Display::~Fl_Image_Display()' - Destroy an image display widget.
//
Fl_Image_Display::~Fl_Image_Display()
{
}


//
// 'Fl_Image_Display::draw()' - Draw the image display widget.
//
void Fl_Image_Display::draw()
{
	int	xoff, yoff;			// Offset of image
	int	X, Y, W, H;			// Interior of widget

#ifdef DEBUG
	puts("Fl_Image_Display::draw()");
#endif // DEBUG

	X = x() + Fl::box_dx(box());
	Y = y() + Fl::box_dy(box());
	W = w() - Fl::box_dw(box());
	H = h() - Fl::box_dh(box());

	if (factor_ && _scrollbarsOn)
	{
		xscrollbar_.show();
		yscrollbar_.show();

		W -= SBWIDTH;
		H -= SBWIDTH;
	}
	else
	{
		xscrollbar_.hide();
		yscrollbar_.hide();
	}

	if (damage() & FL_DAMAGE_SCROLL)
		fl_push_clip(X, Y, W, H);

	if (factor_ && _scrollbarsOn)
		draw_box(box(), x(), y(), w() - SBWIDTH, h() - SBWIDTH, color());
	else
		draw_box();

	if (damage() & FL_DAMAGE_SCROLL)
	{
		fl_pop_clip();
	}
	else if (factor_ && _scrollbarsOn)
	{
		fl_color(FL_GRAY);
		fl_rectf(x() + w() - SBWIDTH, y() + h() - SBWIDTH, SBWIDTH, SBWIDTH);
	}

	if (value_ && value_->w() && value_->h())
	{
#ifdef DEBUG
		printf("    value_=%p, w()=%d, h()=%d\n", value_, value_->w(), value_->h());
#endif // DEBUG

		fl_push_clip(X, Y, W, H);

		xoff = 0;
		if (xsize_ <= W)
			xoff = (W - xsize_) / 2;
		yoff = 0;
		if (ysize_ <= H)
			yoff = (H - ysize_) / 2;

		xoff += X;
		yoff += Y;

		xstep_ = value_->w() / xsize_;
		xmod_ = value_->w() % xsize_;

		int drawW = xsize_ > W ? W : xsize_;
		int drawH = ysize_ > H ? H : ysize_;

#ifdef DEBUG
		printf("============================\n");
		printf("    xoff=%d, yoff=%d, xsize_=%d, ysize_=%d, xstep_=%d, xmod_=%d\n",
			xoff, yoff, xsize_, ysize_, xstep_, xmod_);
#endif // DEBUG

		int D = value_->d();

		// TODO prefs : full window or image-only?
		//drawcheckerboard(X, Y, W, H); // full window
		drawcheckerboard(xoff, yoff, drawW, drawH); // image only

		// Animated image containers have two cases
		// 1. Actually animated: need to draw the current frame, using the frame depth
		// 2. Not animated: need to draw straight [may not be in a format as required
		//    by fl_draw_image TODO probably a failure of the image container?
		bool drawn = false;
#ifdef ANIMGIF           
		if (animgif_)
		{
			if (animgif_->is_animated() && animgif_->image())
			{
				D = animgif_->image(0)->d(); // TODO always drawing frame 0, not "current"
			}
			else
			{
				// TODO how to draw Fl_GIF_Image ?
				animgif_->Fl_Pixmap::draw(xoff, yoff, drawW, drawH);
				drawn = true;
			}
		}
#endif
		if (!drawn)
			fl_draw_image(image_cb, this, xoff, yoff, drawW, drawH, D);

		fl_pop_clip();
	}

	bool showFilename = false; // TODO preferences
	if (showFilename)
		draw_label(X, Y, W, H - 2 * labelsize());

	if (factor_ && _scrollbarsOn)
	{
		if (damage() & FL_DAMAGE_SCROLL)
		{
			update_child(xscrollbar_);
			update_child(yscrollbar_);
		}
		else
		{
			draw_child(xscrollbar_);
			draw_child(yscrollbar_);
		}
	}
}

//
// 'Fl_Image_Display::handle()' - Handle events in the widget.
//

int					// O - 1 if handled, 0 otherwise
Fl_Image_Display::handle(int event)	// I - Event to handle
{
	int scrollbarDelta = _scrollbarsOn ? SBWIDTH : 0;

	//printf("Event: %s\n", fl_eventnames[event]);

		switch (event)
		{
		case FL_KEYDOWN:
		case FL_SHORTCUT:
			//printf("KD: %d %s\n", Fl::event_key(), Fl::event_text());
			switch (Fl::event_key())
			{
			case FL_Left:
				// TODO value used from prefs (scroll speed)
				position(xscrollbar_.value()-15, yscrollbar_.value());
				return 1;

			case FL_Right:
				// TODO value used from prefs (scroll speed)
				position(xscrollbar_.value() + 15, yscrollbar_.value());
				return 1;

			case FL_Down:
				// TODO value used from prefs (scroll speed)
				position(xscrollbar_.value(), yscrollbar_.value() + 15);
				return 1;

			case FL_Up:
				// TODO value used from prefs (scroll speed)
				position(xscrollbar_.value(), yscrollbar_.value() - 15);
				return 1;

			case ' ': // Advance (slideshow)
			case FL_BackSpace: // Previous (slideshow)
				if (!(Fl::event_state() & (FL_SHIFT | FL_CTRL | FL_ALT | FL_META)))
				{
					do_callback();
					return (1);
				}
				break;

			case '-': // Zoom out
				if (factor_)
					scale(factor_ * 0.8f);
				else
					scale((double)xsize_ / (double)value_->w() * 0.8f);

				return (1);

			case '=': // Zoom in
				zoomIn();
				return 1;
			}

			break;

		case FL_PUSH:

			if (!value_ || mode_ == FL_SLIDE)
				return 0;

			if (Fl::event_x() < (x() + w() - scrollbarDelta) &&
				Fl::event_y() < (y() + h() - scrollbarDelta))
			{
				update_mouse_xy();

				if (Fl::event_clicks())
				{
					// double-click, not click-and-drag
					zoomIntoMouse();
					return 1;
				}

				last_x_ = Fl::event_x_root();
				last_y_ = Fl::event_y_root();

				start_x_ = mouse_x_;
				start_y_ = mouse_y_;

				start_ex_ = Fl::event_x();
				start_ey_ = Fl::event_y();

				return (1);
			}
			break;

		case FL_DRAG:
			if (!value_ || mode_ == FL_SLIDE)
				return 0;

			if (mode_ == FL_IMAGE_PAN)
				position(xscrollbar_.value() + last_x_ - Fl::event_x_root(),
					yscrollbar_.value() + last_y_ - Fl::event_y_root());
			else if (mode_ != FL_IMAGE_ZOOM_OUT)
			{
				window()->make_current();

				fl_overlay_rect(start_ex_, start_ey_,
					Fl::event_x() - start_ex_,
					Fl::event_y() - start_ey_);
			}

			last_x_ = Fl::event_x_root();
			last_y_ = Fl::event_y_root();
			update_mouse_xy();
			return (1);

		case FL_RELEASE:
			if (!value_ || mode_ == FL_SLIDE)
				return 0;

			update_mouse_xy();

			switch (mode_)
			{
			case FL_IMAGE_ZOOM_IN:
				window()->make_current();
				fl_overlay_clear();

				if (Fl::event_button() == FL_LEFT_MOUSE)
				{
					int W, H;

					W = w() - scrollbarDelta - Fl::box_dw(box());
					H = h() - scrollbarDelta - Fl::box_dh(box());

					if (abs(start_ex_ - Fl::event_x()) > 2 ||
						abs(start_ey_ - Fl::event_y()) > 2)
					{
						// Zoom to box...
						float xfactor, yfactor;

						xfactor = (float)W / (float)abs(mouse_x_ - start_x_);
						yfactor = (float)H / (float)abs(mouse_y_ - start_y_);

						//                    printf("start_x_=%d, start_y_=%d, mouse_x_=%d, mouse_y_=%d\n",
						//		           start_x_, start_y_, mouse_x_, mouse_y_);
						//		    printf("W=%d, H=%d, dx=%d, dy=%d\n", W, H,
						//		           abs(mouse_x_ - start_x_), abs(mouse_x_ - start_x_));
						//                    printf("xfactor=%g, yfactor=%g\n", xfactor, yfactor);

						scale(xfactor < yfactor ? xfactor : yfactor);
						position((int)((mouse_x_ < start_x_ ? mouse_x_ : start_x_) * scale()),
							(int)((mouse_y_ < start_y_ ? mouse_y_ : start_y_) * scale()));
					}
					else
					{
						zoomDelta(-1);
						position((int)((mouse_x_ < start_x_ ? mouse_x_ : start_x_) * scale()) - W / 2,
							(int)((mouse_y_ < start_y_ ? mouse_y_ : start_y_) * scale()) - H / 2);
					}
					break;
				}

			case FL_IMAGE_ZOOM_OUT:
				zoomDelta(1);
				break;

			case FL_IMAGE_CLICK:
				window()->make_current();
				fl_overlay_clear();

				do_callback();
				break;

			//case FL_FOCUS:
			//case FL_UNFOCUS:
			//	// required to accept keyboard events
			//	return 1;
			}
		}

	return (Fl_Group::handle(event));
}


//
// 'Fl_Image_Display::image_cb()' - Provide a single line of an image.
//
void
Fl_Image_Display::image_cb(void* p,	// I - Image display widget
	int   X,	// I - X offset
	int   Y,	// I - Y offset
	int   W,	// I - Width of image row
	uchar* D)	// O - Image data
{
	const uchar* inptr;		// Pointer into image

	Fl_Image_Display* display = (Fl_Image_Display*)p; // display widget
	Fl_Image* img = display->value_;                  // image to display

#ifdef ANIMGIF    
	if (display->animgif_ && display->animgif_->is_animated())
	{
		img = display->animgif_->image();
	}
#endif

	int depth = img->d();

	// Bresenham values
	int xstep = display->xstep_ * depth;
	int xmod = display->xmod_;
	int xsize = display->xsize_;
	int xerr = (X * xmod) % xsize;

	int scrollDelta = display->_scrollbarsOn ? SBWIDTH : 0;

	if (xsize > (display->w() - Fl::box_dw(display->box()) - scrollDelta))
		X = (X + display->xscrollbar_.value()) * (img->w() - 1) / (xsize - 1);
	else
		X = X * (img->w() - 1) / (xsize - 1);

	if (display->ysize_ > (display->h() - Fl::box_dh(display->box()) - scrollDelta))
		Y = (Y + display->yscrollbar_.value()) * (img->h() - 1) /
		(display->ysize_ - 1);
	else
		Y = Y * (img->h() - 1) / (display->ysize_ - 1);

	// KBR 20200421 fix integer overflow problem
	long yl = max(Y,0);
	long xl = max(X,0); // KBR 20200508 how did X go negative?
	long wl = img->w();
	long dl = depth;
	ulong offset = (yl * wl + xl) * dl;
	inptr = (const uchar*)img->data()[0] + offset;

	switch (depth)
	{
	case 1:
		for (; W > 0; W--)
		{
			*D++ = *inptr; // gamma_lut_[*inptr];

			inptr += xstep;
			xerr += xmod;

			if (xerr >= xsize)
			{
				xerr -= xsize;
				inptr += depth;
			}
		}
		break;
	case 2:
		for (; W > 0; W--)
		{
			*D++ = inptr[0];
			*D++ = inptr[1];

			inptr += xstep;
			xerr += xmod;

			if (xerr >= xsize)
			{
				xerr -= xsize;
				inptr += depth;
			}
		}
		break;
	case 3:
		for (; W > 0; W--)
		{
			/* KBR TEST
			*D++ = gamma_lut_[inptr[0]];
			*D++ = gamma_lut_[inptr[1]];
			*D++ = gamma_lut_[inptr[2]];
			*/
			*D++ = inptr[0];
			*D++ = inptr[1];
			*D++ = inptr[2];

			inptr += xstep;
			xerr += xmod;

			if (xerr >= xsize)
			{
				xerr -= xsize;
				inptr += depth;
			}
		}
		break;
	case 4: // KBR
		for (; W > 0; W--)
		{
			/* KBR test
			*D++ = gamma_lut_[inptr[0]];
			*D++ = gamma_lut_[inptr[1]];
			*D++ = gamma_lut_[inptr[2]];
			*D++ = gamma_lut_[inptr[3]];
			*/

			*D++ = inptr[0];
			*D++ = inptr[1];
			*D++ = inptr[2];
			*D++ = inptr[3];

			inptr += xstep;
			xerr += xmod;

			if (xerr >= xsize)
			{
				xerr -= xsize;
				inptr += depth;
			}
		}
		break;
	}
}


//
// 'Fl_Image_Display::position()' - Reposition the image on the screen.
//

void
Fl_Image_Display::position(int X,	// I - New X offset
	int Y)	// I - New Y offset
{
	// interior size
	int W = w() - (_scrollbarsOn ? SBWIDTH : 0);
	int H = h() - (_scrollbarsOn ? SBWIDTH : 0);

	if (X < 0)
		X = 0;
	else if (X > (xsize_ - W))
		X = xsize_ - W;

	if (Y < 0)
		Y = 0;
	else if (Y > (ysize_ - H))
		Y = ysize_ - H;

	// TODO are these pointless if (e.g.) X, W, xsize_ haven't changed?
	xscrollbar_.value(X, W, 0, xsize_);
	yscrollbar_.value(Y, H, 0, ysize_);

	damage(FL_DAMAGE_SCROLL);
}


//
// 'Fl_Image_Display::resize()' - Resize the image display widget.
//

void
Fl_Image_Display::resize(int X,		// I - New X position
	int Y,		// I - New Y position
	int W,		// I - New width
	int H)		// I - New height
{
	Fl_Widget::resize(X, Y, W, H);

	xscrollbar_.resize(X, Y + H - SBWIDTH, W - SBWIDTH, SBWIDTH);
	yscrollbar_.resize(X + W - SBWIDTH, Y, SBWIDTH, H - SBWIDTH);

	int scrollbarDelta = _scrollbarsOn ? SBWIDTH : 0;
	W -= Fl::box_dw(box()) + scrollbarDelta;
	H -= Fl::box_dh(box()) + scrollbarDelta;

	if (factor_ == 0.0f && value_)
	{
		xsize_ = W;

		if (xsize_ > (value_->w() * 4))
			xsize_ = value_->w() * 4;

		ysize_ = xsize_ * value_->h() / value_->w();

		if (ysize_ > H)
		{
			ysize_ = H;

			if (ysize_ > (value_->h() * 4))
				ysize_ = value_->h() * 4;

			xsize_ = ysize_ * value_->w() / value_->h();
		}
	}

	update_scrollbars();

	redraw();
}


//
// 'Fl_Image_Display::scale()' - Scale the image.
//
void
Fl_Image_Display::scale(double factor)	// I - Scaling factor (0 = auto)
{
	int	X, Y, W, H;			// Interior of widget
	double	ratio;			// Scaling ratio

	if (factor > 10.0f)
		factor = 10.0f;

	// Make sure that the image doesn't get scaled to nothin'...
	if (value_)
	{
		if (factor > 0.0f && (value_->w() * factor) < 32.0f && value_->w() > 32)
			factor = 32.0f / value_->w();

		if (factor > 0.0f && (value_->h() * factor) < 32.0f && value_->h() > 32)
			factor = 32.0f / value_->h();
	}

	bool noScroll = true;
	if (factor == 0.0f)
		ratio = 0.0f;
	else
	{
		ratio = factor / factor_;
		noScroll = !_scrollbarsOn;
	}

	factor_ = factor;

	redraw();

	if (!value_ || !value_->w() || !value_->h())
		return;

	W = w() - (noScroll ? 0 : SBWIDTH) - Fl::box_dw(box());
	H = h() - (noScroll ? 0 : SBWIDTH) - Fl::box_dh(box());

	if (factor_ == 0.0f)
	{
		xsize_ = W;

		if (xsize_ > (value_->w() * 4))
			xsize_ = value_->w() * 4;

		ysize_ = xsize_ * value_->h() / value_->w();

		if (ysize_ > H)
		{
			ysize_ = H;

			if (ysize_ > (value_->h() * 4))
				ysize_ = value_->h() * 4;

			xsize_ = ysize_ * value_->w() / value_->h();
		}

		X = 0;
		Y = 0;
	}
	else
	{
		xsize_ = (int)((float)value_->w() * factor_ + 0.5f);
		ysize_ = (int)((float)value_->h() * factor_ + 0.5f);

		if (xsize_ <= W)
		{
			// The image will be centered...
			X = 0;
		}
		else if (ratio == 0.0)
		{
			// Previous zoom was auto-fit, center it...
			X = (xsize_ - W) / 2;
		}
		else
		{
			// Try to center on the previous location...
			X = (int)((xscrollbar_.value() + W / 2) * ratio) - W / 2;
		}

		if (ysize_ <= H)
		{
			// The image will be centered...
			Y = 0;
		}
		else if (ratio == 0.0)
		{
			// Previous zoom was auto-fit, center it...
			Y = (ysize_ - H) / 2;
		}
		else
		{
			// Try to center on the previous location...
			Y = (int)((yscrollbar_.value() + H / 2) * ratio) - H / 2;
		}
	}

	// Update the scrollbars...
	if (X < 0)
		X = 0;
	else if (X > (xsize_ - W))
		X = xsize_ - W;

	xscrollbar_.value(X, W, 0, xsize_);

	if (xsize_ <= W)
		xscrollbar_.deactivate();
	else
		xscrollbar_.activate();

	if (Y < 0)
		Y = 0;
	else if (Y > (ysize_ - H))
		Y = ysize_ - H;

	yscrollbar_.value(Y, H, 0, ysize_);

	if (ysize_ <= H)
		yscrollbar_.deactivate();
	else
		yscrollbar_.activate();

	// let consumers know the final zoom value
	if (_zoomCallback)
	{
		int newZoom = (int)(100.0 * ((double)xsize_ / value_->w()) + 0.5);
		_zoomCallback(newZoom);
	}
}


//
// 'Fl_Image_Display::scrollbar_cb()' - Update the display based on the scrollbar position.
//

void
Fl_Image_Display::scrollbar_cb(Fl_Widget* w,
	// I - Widget
	void* d)
	// I - Image display widget
{
	((Fl_Image_Display*)d)->damage(FL_DAMAGE_SCROLL);
}


//
// 'Fl_Image_Display::set_gamma()' - Set the display gamma...
//

void
Fl_Image_Display::set_gamma(float val)	// I - Gamma value
{
	val /= 2.2f;

	for (int i = 0; i < 256; i++)
		gamma_lut_[i] = (int)(255.0 * pow(i / 255.0, val) + 0.5);
}


//
// 'Fl_Image_Display::update_mouse_xy()' - Update the mouse X and Y values.
//

void
Fl_Image_Display::update_mouse_xy()
{
	int	X, Y;				// X,Y position
	int	W, H;				// Width and height

	int scrollbarDelta = _scrollbarsOn ? SBWIDTH : 0;
	X = Fl::event_x() - x() - Fl::box_dx(box());
	Y = Fl::event_y() - y() - Fl::box_dy(box());
	W = w() - scrollbarDelta - Fl::box_dw(box());
	H = h() - scrollbarDelta - Fl::box_dh(box());

	if (!value_ || xsize_ <= 0 || ysize_ <= 0)
	{
		mouse_x_ = -1;
		mouse_y_ = -1;
	}

	if (xsize_ < W)
	{
		X -= (W - xsize_) / 2;

		if (X < 0)
			mouse_x_ = 0;
		else if (X >= xsize_)
			mouse_x_ = value_->w();
		else
			mouse_x_ = X * value_->w() / xsize_;
	}
	else
		mouse_x_ = (xscrollbar_.value() + X) * value_->w() / xsize_;

	if (ysize_ < H)
	{
		Y -= (H - ysize_) / 2;

		if (Y < 0)
			mouse_y_ = 0;
		else if (Y >= ysize_)
			mouse_y_ = value_->h();
		else
			mouse_y_ = Y * value_->h() / ysize_;
	}
	else
		mouse_y_ = (yscrollbar_.value() + Y) * value_->h() / ysize_;

	if (mouse_x_ < 0)
		mouse_x_ = 0;
	else if (mouse_x_ > value_->w())
		mouse_x_ = value_->w();

	if (mouse_y_ < 0)
		mouse_y_ = 0;
	else if (mouse_y_ > value_->h())
		mouse_y_ = value_->h();

	//  printf("xscrollbar_=%d, yscrollbar_=%d\n", xscrollbar_.value(),
	//         yscrollbar_.value());
	//  printf("mouse_x_=%d, mouse_y_=%d\n", mouse_x_, mouse_y_);
}


//
// 'Fl_Image_Display::update_scrollbars()' - Update the scrollbars.
//

void
Fl_Image_Display::update_scrollbars()
{
	if (!value_)
	{
		xscrollbar_.value(0, 1, 0, 1);
		yscrollbar_.value(0, 1, 0, 1);
		return;
	}

	int scrollbarDelta = _scrollbarsOn ? SBWIDTH : 0;

	// Interior Size
	int W = w() - scrollbarDelta - Fl::box_dw(box());
	int H = h() - scrollbarDelta - Fl::box_dh(box());

	int X = xscrollbar_.value();
	if (X > (xsize_ - W))
		X = xsize_ - W;
	else if (X < 0)
		X = 0;

	xscrollbar_.value(X, W, 0, xsize_);

	if (xsize_ <= W)
		xscrollbar_.deactivate();
	else
		xscrollbar_.activate();

	int Y = yscrollbar_.value();
	if (Y > (ysize_ - H))
		Y = ysize_ - H;
	else if (Y < 0)
		Y = 0;

	yscrollbar_.value(Y, H, 0, ysize_);

	if (ysize_ <= H)
		yscrollbar_.deactivate();
	else
		yscrollbar_.activate();
}


//
// 'Fl_Image_Display::value()' - Set the image to be displayed.
//

void
Fl_Image_Display::value(Fl_Image* v)
{
#ifdef ANIMGIF     

#if false // TODO punting on animated GIF playback
	// stop 'previous' gif from playing
	if (animgif_)
	{
		animgif_->canvas(NULL); // Using stop() seems to kill playback altogether
		animgif_ = NULL;
	}

	animgif_ = dynamic_cast<Fl_Anim_GIF_Image*>(v->KBR());

	if (animgif_ && animgif_->is_animated())
	{
		animgif_->canvas(this, Fl_Anim_GIF_Image::Start |
			Fl_Anim_GIF_Image::DontSetAsImage |
			Fl_Anim_GIF_Image::DontResizeCanvas);
	}
#endif

	animgif_ = dynamic_cast<Fl_Anim_GIF_Image*>(v);

#endif

	value_ = v;
	set_zoommode(zoomMode_);

	// Make sure new image position is reset
	// TODO: settings (top, center, ?)
	xscrollbar_.value(0);
	yscrollbar_.value(0);
}

void Fl_Image_Display::zoomDelta(int direction)
{
	if (!value_)
		return;

	if (direction > 0)
	{
		if (factor_)
			scale(factor_ * 0.8f);
		else
			scale((double)xsize_ / (double)value_->w() * 0.8f);
	}
	else
	{
		if (factor_)
			scale(factor_ * 1.25f);
		else
			scale((double)xsize_ / (double)value_->w() * 1.25f);
	}
}

void Fl_Image_Display::zoomIn()
{
	if (factor_)
		scale(factor_ * 1.25f);
	else
		scale((double)xsize_ / (double)value_->w() * 1.25f);
}

void Fl_Image_Display::zoomIntoMouse()
{
	// assumes update_mouse_xy already called so mouse position adjusted to image position

	printf("Double click mouse_x_=%d, mouse_y_=%d\n", mouse_x_, mouse_y_);

	//printf("Before pos: %d, %d max: %f, %f\n", xscrollbar_.value(), yscrollbar_.value(),
	//	xscrollbar_.maximum(), yscrollbar_.maximum());

	zoomIn();

	//printf("After pos: %d, %d max: %f, %f\n", xscrollbar_.value(), yscrollbar_.value(),
	//	xscrollbar_.maximum(), yscrollbar_.maximum());

	int iW = value_->w();
	int iH = value_->h();

	// TODO try to move the image so that the mouse position as close to center as possible
}


void Fl_Image_Display::set_zoommode(ZoomMode newmode)
{
	zoomMode_ = newmode;

	int imgH = value_->h();
	int imgW = value_->w();

	int shoW = w() - (_scrollbarsOn ? SBWIDTH : 0) - Fl::box_dw(box());
	int shoH = h() - (_scrollbarsOn ? SBWIDTH : 0) - Fl::box_dh(box());

	double wideRatio = (double)shoW / (double)imgW;
	double highRatio = (double)shoH / (double)imgH;

	switch (zoomMode_)
	{
	case ZoomMode::Auto:
		if (imgH < shoH && imgW < shoW)
			scale(1.0f); // smaller than window, set full size
		else
			scale(0.0f); // "original" auto mode
		break;
	case ZoomMode::ScaleToFit:
		scale(0.0f);
		break;
	case ZoomMode::ScaleToFill:
		scale(max(wideRatio, highRatio));
		break;
	case ZoomMode::ScaleToHigh:
		scale(highRatio);
		break;
	case ZoomMode::ScaleToWide:
		scale(wideRatio);
		break;
	case ZoomMode::FullSize:
		scale(1.0f);
		break;
	}
}

void Fl_Image_Display::drawcheckerboard(int X, int Y, int W, int H)
{
	if (!_drawChecker)
		return;
	check->draw(X, Y, W, H);
}
