/*
SteamButton.h - a button of the client menus as in the Steam version of CS 1.6

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

#include "ClientWindow.h"

// A frame of 1 pixel ("BorderBright" 188 112 0 128), the text in "Default" (Verdana 600, "BaseText" 255 176 0)
// with its capitals in the middle of the button. Under the cursor (or chosen, like the class whose picture is
// shown) it is filled with "SelectionBG" (255 176 0, 41 % as measured on the screenshots).
// Its place is set in pixels of the screen: the buttons are one under another (see SteamFrame::TeamMenu).
class CSteamMenuButton : public CMenuAction
{
public:
	typedef CMenuAction BaseClass;

	int slot = 0;
	bool chosen = false;
	bool disabled = false;    // drawn in "DisabledText1" (80 48 0), not lit (an item the player can't afford)
	float resX = -1.0f, resY = 0.0f, resW = 0.0f, resH = 0.0f; // the place in the units of a .res file (if resX >= 0)
	char label[128] = "";
	HFont textFont = 0;
	int capHeight = 0;
	const SteamFrame::Layout *frame = nullptr;

	void VidInit( void ) override
	{
		BaseClass::VidInit();

		if( !frame )
			return;

		int x, y, w, h;
		if( resX >= 0.0f )
		{
			x = SteamFrame::Res::X( *frame, resX );
			y = SteamFrame::Res::Y( *frame, resY );
			w = frame->U( resW );
			h = frame->U( resH );
		}
		else SteamFrame::TeamMenu::Button( *frame, slot, x, y, w, h );
		m_scPos = Point( x, y );
		m_scSize = Size( w, h );
	}

	// a disabled button doesn't work and makes no sound when it is clicked (as in the Steam version)
	bool KeyDown( int key ) override
	{
		if( disabled && ( UI::Key::IsEnter( key ) || UI::Key::IsLeftMouse( key )))
			return true;
		return BaseClass::KeyDown( key );
	}

	bool KeyUp( int key ) override
	{
		if( disabled && ( UI::Key::IsEnter( key ) || UI::Key::IsLeftMouse( key )))
			return true;
		return BaseClass::KeyUp( key );
	}

	void Draw( void ) override
	{
		const bool hover = this == m_pParent->ItemAtCursor() && ( iFlags & ( QMF_HASMOUSEFOCUS | QMF_HASKEYBOARDFOCUS ));
		const bool lit = ( hover || chosen ) && !disabled;

		UI_FillRect( m_scPos, m_scSize, lit ? PackRGBA( 255, 176, 0, 105 ) : PackRGBA( 0, 0, 0, 96 ));
		CClientWindow::DrawFrame( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h, PackRGBA( 188, 112, 0, lit ? 255 : 128 ));

		if( frame )
		{
			const int capTop = m_scPos.y + ( m_scSize.h - capHeight + 1 ) / 2 + 1; // 278 at 1080, as in Steam
			const int x = m_scPos.x + SteamFrame::TeamMenu::ButtonTextInset( *frame );
			CClientWindow::DrawSteamText( textFont, x, capTop, capHeight, m_scPos.x + m_scSize.w - x - 1, label,
			                              disabled ? PackRGBA( 80, 48, 0, 255 ) : PackRGBA( 255, 176, 0, 255 ));
		}
	}
};

// A check box of the Steam version (VGUI2 CheckButton): a box with a frame of 1 pixel ("BorderBright"), a check
// mark in "BrightControlText" and the text in "Default" on its right. It switches a cvar.
class CSteamCheckBox : public CMenuAction
{
public:
	typedef CMenuAction BaseClass;

	char label[128] = "";
	const char *cvarName = nullptr;
	HFont textFont = 0;
	int capHeight = 0;
	const SteamFrame::Layout *frame = nullptr;
	float resX = 0.0f, resY = 0.0f; // the place of the box in the units of a .res file

	CSteamCheckBox()
	{
		onPressed = CEventCallback( []( CMenuBaseItem *self, void * )
		{
			CSteamCheckBox *box = static_cast<CSteamCheckBox *>( self );
			if( box->cvarName )
				EngFuncs::CvarSetValue( box->cvarName, EngFuncs::GetCvarFloat( box->cvarName ) ? 0.0f : 1.0f );
		} );
		SetText( "" );
		bDrawStroke = false;
	}

	int BoxSize( void ) const { return frame ? frame->U( 9.0f ) : 20; } // 20 pixels at 1080

	void VidInit( void ) override
	{
		BaseClass::VidInit();
		if( !frame )
			return;

		// the box and the text: the whole line can be clicked
		const int box = BoxSize();
		m_scPos = Point( SteamFrame::Res::X( *frame, resX ), SteamFrame::Res::Y( *frame, resY ));
		m_scSize = Size( box + frame->U( 120.0f ), box );
	}

	void Draw( void ) override
	{
		const int box = BoxSize();
		const bool hover = this == m_pParent->ItemAtCursor() && ( iFlags & QMF_HASMOUSEFOCUS );
		const bool checked = cvarName && EngFuncs::GetCvarFloat( cvarName ) != 0.0f;

		UI_FillRect( m_scPos.x, m_scPos.y, box, box, PackRGBA( 0, 0, 0, 200 ));
		CClientWindow::DrawFrame( m_scPos.x, m_scPos.y, box, box, PackRGBA( 188, 112, 0, hover ? 255 : 128 ));

		if( checked )
		{
			// a check mark of two strokes
			const unsigned int c = PackRGBA( 255, 176, 0, 255 );
			const int t = box / 7 > 1 ? box / 7 : 1;
			const int x0 = m_scPos.x + box / 5, y0 = m_scPos.y + box / 2;
			const int knee = box / 4;
			for( int i = 0; i <= knee; ++i )
				UI_FillRect( x0 + i, y0 + i - t / 2, t, t, c );
			const int len = box - box / 5 * 2 - knee;
			for( int i = 0; i <= len; ++i )
				UI_FillRect( x0 + knee + i, y0 + knee - i * ( knee + box / 4 ) / ( len > 0 ? len : 1 ) - t / 2, t, t, c );
		}

		if( frame )
		{
			const int capTop = m_scPos.y + ( box - capHeight + 1 ) / 2;
			CClientWindow::DrawSteamText( textFont, m_scPos.x + box + frame->U( 4.0f ), capTop, capHeight,
			                              frame->U( 200.0f ), label, PackRGBA( 255, 176, 0, 255 ));
		}
	}
};
