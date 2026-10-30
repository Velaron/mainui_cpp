/*
SteamFrame.h - the frame of the client menus as in the Steam version of CS 1.6
(the VGUI2 windows of the Steam version, measured on its screenshots at 1920x1080)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#pragma once

#include <math.h>

// Like the VGUI2 panels of the Steam version the window is sized in units of 1/480 of the screen height
// (2.25 pixels at 1080). At 1920x1080 (pixels):
//
//   the panel 285..1629 x 45..1029 (600 x 440 units in the middle of the screen), corners of 20, black (204),
//   a cut of 2 pixels at 162 (it lets the game be seen through it)
//   the CS icon (gfx/vgui/CS_logo.tga) as a square of 88 at (297, 57), coloured "TitleIcon" (255 174 0)
//   the title ("SELECT TEAM"): capitals 93..114 from x 412, Trebuchet MS
//
// The places agree with resource/UI/TeamMenu.res, ClassMenu_*.res and MOTD.res of the Steam version: a control
// at (xpos, ypos) of them is at (56 + xpos, ypos - 20) units from the corner of the panel.
//
// The team menu:
//   buttons 333 x 45 from (411, 261), one under another every 72 pixels (1, 2, 3, then 5, 6 and 0 a place lower),
//   a frame of 1 pixel ("BorderBright" 188 112 0 128), the text in Verdana 600 ("Default" of ClientScheme.res)
//   from x 417, its capitals in the middle of the button
//   the description of the map: a box 789..1499 x 261..903 (black, 200) with a frame of 1 pixel, the text from
//   x 798 every 21 pixels, its first capitals at 268; a scroll bar of 54 pixels on the right (1445..1498),
//   its buttons 53 pixels high with grey arrows

namespace SteamFrame {

struct Layout
{
	float u = 1.0f;                       // pixels per unit
	int panelX = 0, panelY = 0, panelW = 0, panelH = 0;
	int radius = 0;
	int cutY = 0, cutH = 1;               // the cut: cutY .. cutY + cutH
	int iconX = 0, iconY = 0, iconSize = 0;
	int titleX = 0, titleBaseline = 0, titleEm = 0;
	int textEm = 0, textTall = 0;         // "Default": the height of the letters and the "tall" of the scheme
	bool textAntialias = true;

	int U( float v ) const { return int( v * u + 0.5f ); }
};

// The font "Default" of resource/ClientScheme.res: Verdana 600, its "tall" depends on the height of the screen;
// the Steam version draws letters of tall * 3 / 4 pixels
inline void DefaultFont( int screenH, int &tall, int &em, bool &antialias )
{
	if( screenH < 600 )       { tall = 12; antialias = false; }
	else if( screenH < 768 )  { tall = 13; antialias = false; }
	else if( screenH < 1024 ) { tall = 14; antialias = true; }
	else if( screenH < 1200 ) { tall = 20; antialias = true; }
	else                      { tall = 24; antialias = true; }

	em = tall * 3 / 4;
	if( em < 4 )
		em = 4;
}

inline Layout Compute( int screenW, int screenH )
{
	Layout l;

	float u = screenH / 480.0f;
	if( 600.0f * u > screenW )
		u = screenW / 600.0f;
	l.u = u;

	l.panelX = int( screenW * 0.5f - 300.0f * u );
	l.panelY = int( screenH * 0.5f - 220.0f * u );
	if( l.panelX < 0 ) l.panelX = 0;
	if( l.panelY < 0 ) l.panelY = 0;
	l.panelW = l.U( 597.8f );
	l.panelH = l.U( 437.8f );
	l.radius = l.U( 9.0f );

	l.cutY = l.panelY + l.U( 52.0f );
	l.cutH = int( u ) > 0 ? int( u ) : 1;

	l.iconX = l.panelX + l.U( 5.3f );
	l.iconY = l.panelY + l.U( 5.3f );
	l.iconSize = l.U( 39.0f );

	l.titleX = l.panelX + l.U( 56.0f );
	l.titleBaseline = l.panelY + l.U( 31.1f );
	l.titleEm = l.U( 13.8f );

	DefaultFont( screenH, l.textTall, l.textEm, l.textAntialias );
	return l;
}

// The team menu
struct TeamMenu
{
	// slot: 0 for "1", 1 for "2", 2 for "3", 3 for "5", 4 for "6", 5 for "0" (cancel)
	static void Button( const Layout &l, int slot, int &x, int &y, int &w, int &h )
	{
		x = l.panelX + l.U( 56.0f );
		y = l.panelY + l.U( 96.0f + 32.0f * slot );
		w = l.U( 148.0f );
		h = l.U( 20.0f );
	}

	static int ButtonTextInset( const Layout &l ) { return l.U( 2.7f ); }

	// the box of the description of the map
	static void InfoBox( const Layout &l, int &x, int &y, int &w, int &h )
	{
		x = l.panelX + l.U( 224.0f );
		y = l.panelY + l.U( 96.0f );
		w = l.U( 316.0f );
		h = l.U( 285.8f );
	}

	static int ScrollBarWidth( const Layout &l )   { return l.U( 24.0f ); }  // 54 at 1080
	static int ScrollButtonHeight( const Layout &l ) { return l.U( 23.5f ); } // 53 at 1080
	static int TextInset( const Layout &l )        { return l.U( 4.4f ); }   // 10 at 1080 ("D" at 799)
	static int FirstCapTop( const Layout &l )      { return l.U( 3.6f ); }   // 8 at 1080 (capitals at 268)
	static int LineHeight( const Layout &l )       { return l.textTall + 1; } // 21 at 1080
};

// The class menu: the same buttons (1..4, then 5 "AUTO-SELECT" a place lower: at 621 at 1080), a box with the
// picture of the class 789..1463 x 261..701 (black, a frame of 1 pixel), the picture as high as the box with its
// own proportions (its middle at 1078 at 1080, cut by the box), the description under it: "DefaultSmall"
// (Verdana, thin), "LabelDimText" (255 176 0 164), its capitals 778..788 from x 790
struct ClassMenu
{
	static void PictureBox( const Layout &l, int &x, int &y, int &w, int &h )
	{
		x = l.panelX + l.U( 224.0f );
		y = l.panelY + l.U( 96.0f );
		w = l.U( 300.0f );
		h = l.U( 196.0f );
	}

	static int PictureCenterX( const Layout &l ) { return l.panelX + l.U( 352.7f ); }

	static int DescriptionX( const Layout &l )      { return l.panelX + l.U( 224.0f ); } // 789 at 1080
	static int DescriptionCapTop( const Layout &l ) { return l.panelY + l.U( 326.2f ); } // 779 at 1080
};

// The buy menus (resource/UI/MainBuyMenu.res, Buy*.res and classes/<item>.res of the Steam version) are placed
// in the coordinates of a screen of 640 x 480 units in the middle of the real screen: a control at (xpos, ypos)
// is at (xpos - 20, ypos - 20) units from the corner of the panel (the "1" button at 76, 116 is at 411, 261 at 1080)
namespace Res
{
	inline int X( const Layout &l, float xpos ) { return l.panelX + l.U( xpos - 20.0f ); }
	inline int Y( const Layout &l, float ypos ) { return l.panelY + l.U( ypos - 20.0f ); }
}

// How far the rounded corner is from the side of the rectangle in the row i (0 is the outermost row);
// (the same rounding as the windows of the client)
inline int RoundedInset( int radius, int i )
{
	const float dy = radius - i - 0.5f;
	return radius - (int)( sqrtf( (float)( radius * radius ) - dy * dy ) + 0.5f );
}

} // namespace SteamFrame
