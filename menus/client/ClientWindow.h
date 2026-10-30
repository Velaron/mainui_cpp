/*
ClientWindow.h - base client menu window
Copyright (C) 2018 a1batross
Copyright (C) 2026 $_Vladislav

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

#include "BaseClientWindow.h"
#include "Action.h"
#include "SteamFrame.h"

// the letters are drawn by Windows one at a time as in a whole string (ExtTextOut, grayscale antialiasing) and
// placed by their ABC widths, as the VGUI2 fonts of the Steam version: even spaces between the letters
// (a flag of CFontBuilder::SetFlags, after FONT_STRIKEOUT; only the fonts of Windows know it)
#ifndef FONT_GDI_TEXT
#define FONT_GDI_TEXT ( 1 << 3 )
#endif

class CSteamMenuButton;

class CClientWindow : public CMenuBaseClientWindow
{
public:
	typedef CMenuBaseClientWindow BaseClass;
	CClientWindow( const char *name = "CClientWindow" ) : BaseClass( name )
	{
		// SetCharSize( QM_DEFAULTFONT );
		m_iNumBtns = 0;
		for ( int i = 0; i < 256; i++ )
		{
			keys[i].Reset();
			keyButtons[i] = NULL;
		}
	}
	~CClientWindow()
	{
		for ( int i = 0; i < m_iNumBtns; i++ )
		{
			delete buttons[i];
		}
	}

	void Show() override
	{
		EngFuncs::KEY_SetDest( KEY_MENU );
		EngFuncs::ClientCmd( true, "touch_setclientonly 1" );
		BaseClass::Show();
	}
	void Hide() override
	{
		StopShowScores();
		BaseClass::Hide();
		if ( m_pStack->Count() <= 1 )
		{
			EngFuncs::KEY_ClearStates();
			EngFuncs::KEY_SetDest( KEY_GAME );
			EngFuncs::ClientCmd( false, "touch_setclientonly 0" );
		}
	}

	CEventCallback ExecAndHide( const char *szCmd )
	{
		return CEventCallback( []( CMenuBaseItem *pSelf, void *pExtra )
		                       {
			UI_CloseClientMenu();
			EngFuncs::ClientCmd( false, (const char*)pExtra ); }, (void *)szCmd );
	}

	CMenuAction *AddButton( int key, const char *name, Point pos, CEventCallback callback );

	bool KeyUp( int key ) override;
	void StopShowScores( void ); // the key of the table of the players was released or the menu is closed
	bool KeyDown( int key ) override;
	void VidInit() override;
	void Draw() override;
	CEventCallback keys[256];

protected:
	CMenuAction *buttons[256];
	CMenuBaseItem *keyButtons[256];
	int m_iNumBtns;

	// the frame of the Steam version (see SteamFrame.h), in pixels of the screen
	SteamFrame::Layout m_frame;
	HFont m_hTitleFont = 0;
	bool m_bShowScores = false;

	// the buttons of the Steam version (SteamButton.h) and their font ("Default": Verdana 600)
	CSteamMenuButton *AddSteamButton( int key, const char *name, int slot, CEventCallback callback );
	void SetButtonShown( CMenuAction *button, int key, bool shown ); // a hidden button doesn't answer its key
	CSteamMenuButton *m_steamButtons[16];
	int m_iNumSteamButtons = 0;
	HFont m_hTextFont = 0;
	int m_iCapHeight = 0;

public:
	// drawing in pixels of the screen like the Steam version
	// a text: x is its left edge, capTop the top of its capital letters
	static void DrawSteamText( HFont font, int x, int capTop, int capHeight, int maxWidth, const char *text, unsigned int color );
	static void FillRoundedRect( int x, int y, int w, int h, int radius, bool roundTop, bool roundBottom, unsigned int color );
	static void DrawFrame( int x, int y, int w, int h, unsigned int color ); // 1 pixel inside the rectangle
	static HFont CreateSteamFont( const char *name, int em, float cellPerEm, int weight );
};
