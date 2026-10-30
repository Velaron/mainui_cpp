/*
ClientWindow.cpp - base client menu window
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

#include "BaseMenu.h"
#include "ClientWindow.h"
#include "SteamButton.h"

#include <stdio.h>

// The frame of the Steam version (see SteamFrame.h)
void CClientWindow::VidInit()
{
	m_frame = SteamFrame::Compute( ScreenWidth, ScreenHeight );

	// the items of the menus are placed in the units of the menu, the frame itself in pixels
	pos.x = m_frame.panelX / uiStatic.scaleX;
	pos.y = m_frame.panelY / uiStatic.scaleY;
	size.w = m_frame.panelW / uiStatic.scaleX;
	size.h = m_frame.panelH / uiStatic.scaleY;

	BaseClass::VidInit();

	m_scPos = Point( m_frame.panelX, m_frame.panelY );
	m_scSize = Size( m_frame.panelW, m_frame.panelH );

	// the title: Trebuchet MS; the size is fitted to the screenshot of the Steam version (capitals 93..114 and
	// 183 pixels for "SELECT TEAM" at 1080)
	m_hTitleFont = CreateSteamFont( "Trebuchet MS", m_frame.titleEm, 1.29f, 400 );

	// "Default": Verdana 600 (its cell is 1.215 of its letters, its capitals 0.727)
	m_hTextFont = CreateSteamFont( "Verdana", m_frame.textEm, 1.215f, 600 );
	if( m_hTextFont <= 0 )
		m_hTextFont = uiStatic.hSmallerFont;
	m_iCapHeight = (int)( m_frame.textEm * 0.727f + 0.5f );

	for( int i = 0; i < m_iNumSteamButtons; ++i )
	{
		m_steamButtons[i]->textFont = m_hTextFont;
		m_steamButtons[i]->capHeight = m_iCapHeight;
	}
}

void CClientWindow::SetButtonShown( CMenuAction *button, int key, bool shown )
{
	if( !button || key <= 0 || key >= 256 )
		return;

	if( shown )
	{
		keys[key] = button->onPressed;
		keyButtons[key] = button;
		button->Show();
	}
	else
	{
		keys[key].Reset();
		keyButtons[key] = NULL;
		button->Hide();
	}
}

CSteamMenuButton *CClientWindow::AddSteamButton( int key, const char *name, int slot, CEventCallback callback )
{
	CSteamMenuButton *act = new CSteamMenuButton();

	// "&" marks the hotkey in VGUI2 and is not drawn ("K&M" is shown as "KM" by the Steam version), "&&" is "&"
	size_t n = 0;
	for( const char *c = name; *c && n + 1 < sizeof( act->label ); ++c )
	{
		if( *c == '&' )
		{
			if( c[1] != '&' )
				continue;
			++c;
		}
		act->label[n++] = *c;
	}
	act->label[n] = 0;
	act->slot = slot;
	act->frame = &m_frame;
	act->onPressed = callback;
	act->SetText( "" );       // the button draws its text itself
	act->bDrawStroke = false;
	act->pos = Point( 0, 0 );
	act->size = Size( 1, 1 );  // the real place is set in pixels by its VidInit

	if( key > 0 && key < 256 )
	{
		keys[key] = callback;
		keyButtons[key] = act;
	}

	if( m_iNumBtns >= 256 )
		Host_Error( "Too many client window buttons!" );

	buttons[m_iNumBtns++] = act;

	if( m_iNumSteamButtons < (int)( sizeof( m_steamButtons ) / sizeof( m_steamButtons[0] )))
		m_steamButtons[m_iNumSteamButtons++] = act;

	AddItem( act );
	return act;
}

HFont CClientWindow::CreateSteamFont( const char *name, int em, float cellPerEm, int weight )
{
	// the fonts of Windows get "tall + 6" as the height of their cell
	int cell = (int)( em * cellPerEm + 0.5f );
	if( cell < 8 )
		cell = 8;

	// FONT_GDI_TEXT: the letters are drawn by Windows as in a whole string and placed by their ABC widths, as the
	// VGUI2 fonts of the Steam version (even spaces between the letters, "1 PISTOLS", "PREVIOUS")
#if defined( _WIN32 )
	HFont font = CFontBuilder( name, cell - 6, weight ).SetFlags( FONT_GDI_TEXT ).Create();
#else
	// FreeType (and stb) take the height of the letters (em), the bold file comes from fontconfig by the weight
	HFont font = CFontBuilder( name, em, weight ).Create();
#endif
	if( font <= 0 || g_FontMgr->GetFontTall( font ) <= 0 )
		return 0;
	return font;
}

void CClientWindow::DrawSteamText( HFont font, int x, int capTop, int capHeight, int maxWidth, const char *text, unsigned int color )
{
	if( !text || !text[0] )
		return;

	if( font <= 0 )
		return;

	const int tall = g_FontMgr->GetFontTall( font );
	const int ascent = g_FontMgr->GetFontAscent( font );
	const int y = capTop - ( ascent - capHeight );

	UI_DrawString( font, x, y, maxWidth, tall, text, color, tall, QM_LEFT | QM_TOP, ETF_NO_WRAP | ETF_FORCECOL );
}

void CClientWindow::FillRoundedRect( int x, int y, int w, int h, int radius, bool roundTop, bool roundBottom, unsigned int color )
{
	if( w <= 0 || h <= 0 )
		return;

	if( radius > w / 2 ) radius = w / 2;
	if( radius > h / 2 ) radius = h / 2;
	if( radius < 0 ) radius = 0;

	const int rt = roundTop ? radius : 0;
	const int rb = roundBottom ? radius : 0;

	UI_FillRect( x, y + rt, w, h - rt - rb, color );

	for( int i = 0; i < rt; ++i )
	{
		const int inset = SteamFrame::RoundedInset( rt, i );
		UI_FillRect( x + inset, y + i, w - inset * 2, 1, color );
	}

	for( int i = 0; i < rb; ++i )
	{
		const int inset = SteamFrame::RoundedInset( rb, i );
		UI_FillRect( x + inset, y + h - 1 - i, w - inset * 2, 1, color );
	}
}

void CClientWindow::DrawFrame( int x, int y, int w, int h, unsigned int color )
{
	if( w <= 0 || h <= 0 )
		return;

	UI_FillRect( x, y, w, 1, color );
	if( h > 1 )
		UI_FillRect( x, y + h - 1, w, 1, color );
	if( h > 2 )
	{
		UI_FillRect( x, y + 1, 1, h - 2, color );
		if( w > 1 )
			UI_FillRect( x + w - 1, y + 1, 1, h - 2, color );
	}
}

void CClientWindow::Draw()
{
	if( m_bShowScores )
		return; // the table of the players is shown instead

	const SteamFrame::Layout &l = m_frame;

	// the panel ("ViewportBG": black, 200) in two parts with a cut between them
	const unsigned int bg = PackRGBA( 0, 0, 0, 200 );
	const int lowerY = l.cutY + l.cutH;
	FillRoundedRect( l.panelX, l.panelY, l.panelW, l.cutY - l.panelY, l.radius, true, false, bg );
	FillRoundedRect( l.panelX, lowerY, l.panelW, l.panelY + l.panelH - lowerY, l.radius, false, true, bg );

	// the icon ("TitleIcon": 255 174 0)
	UI_DrawPic( l.iconX, l.iconY, l.iconSize, l.iconSize, PackRGBA( 255, 174, 0, 255 ), "gfx/vgui/CS_logo.tga", QM_DRAWTRANS );

	// the title ("BaseText": 255 176 0)
	if( m_hTitleFont > 0 )
	{
		const int ascent = g_FontMgr->GetFontAscent( m_hTitleFont );
		const int tall = g_FontMgr->GetFontTall( m_hTitleFont );
		UI_DrawString( m_hTitleFont, l.titleX, l.titleBaseline + 1 - ascent, l.panelX + l.panelW - l.titleX, tall,
		               szName, PackRGBA( 255, 176, 0, 255 ), tall, QM_LEFT | QM_TOP, ETF_NO_WRAP | ETF_FORCECOL );
	}
	else
	{
		UI_DrawString( font, l.titleX, l.panelY + l.U( 14.0f ), l.panelW, l.U( 24.0f ),
		               szName, PackRGBA( 255, 176, 0, 255 ), m_scChSize, QM_LEFT, ETF_NOSIZELIMIT );
	}

	BaseClass::Draw();
}

// The key of the table of the players ("+showscores", Tab by default) works in the menus as in the game
// (as in the Steam version): while it is held the table is shown and the menu is not drawn.
static bool IsShowScoresKey( int key )
{
	const char *binding = EngFuncs::KEY_GetBinding( key );
	return binding && !stricmp( binding, "+showscores" );
}

bool CClientWindow::KeyDown( int key )
{
	if ( eTransitionType == ANIM_CLOSING )
		return false;

	if ( IsShowScoresKey( key ))
	{
		if ( !m_bShowScores )
		{
			m_bShowScores = true;
			EngFuncs::ClientCmd( false, "+showscores" );
		}
		return true;
	}

	if ( key == K_ENTER || key == K_KP_ENTER )
	{
		CMenuBaseItem *item = ItemAtCursor();
		if ( item && !( item->iFlags & ( QMF_GRAYED | QMF_INACTIVE ) ) )
		{
			item->_Event( QM_PRESSED );
			item->_Event( QM_RELEASED );
			return true;
		}
	}

	if ( key >= 0 && key < 256 && ( keys[key] || keyButtons[key] ) )
	{
		CMenuBaseItem *item = keyButtons[key] ? keyButtons[key] : ( m_iNumBtns ? buttons[0] : NULL );
		if ( item )
		{
			if ( keyButtons[key] )
			{
				static_cast<CMenuAction *>( keyButtons[key] )->onPressed( item );
			}
			else
			{
				( keys[key] )( item );
			}
			return true;
		}
	}

	return BaseClass::KeyDown( key );
}

bool CClientWindow::KeyUp( int key )
{
	if ( IsShowScoresKey( key ))
	{
		StopShowScores();
		return true;
	}

	return BaseClass::KeyUp( key );
}

void CClientWindow::StopShowScores()
{
	if ( m_bShowScores )
	{
		m_bShowScores = false;
		EngFuncs::ClientCmd( false, "-showscores" );
	}
}

CMenuAction *CClientWindow::AddButton( int key, const char *name, Point pos, CEventCallback callback )
{
	CMenuAction *act = new CMenuAction();

	act->pos = pos;
	act->onPressed = callback;
	act->SetBackground( 0U, PackRGBA( 255, 0, 0, 64 ) );
	if ( *name == '&' ) // fast hack
		name++;
	act->SetText( name );
	act->SetCharSize( QM_SMALLERFONT );
	act->size = Size( 250, 32 );
	act->bDrawStroke = true;
	act->m_bLimitBySize = true;
	act->colorStroke = uiInputTextColor;
	act->iStrokeWidth = 1;
	act->eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	act->eTextAlignment = QM_LEFT;
	act->SetTextInsetX( 8 );

	if ( key > 0 && key < 256 )
	{
		keys[key] = callback;
		keyButtons[key] = act;

		if ( key >= 'a' && key <= 'z' )
		{
			int upper = key - 'a' + 'A';
			keys[upper] = callback;
			keyButtons[upper] = act;
		}
		else if ( key >= 'A' && key <= 'Z' )
		{
			int lower = key - 'A' + 'a';
			keys[lower] = callback;
			keyButtons[lower] = act;
		}
	}

	if ( m_iNumBtns >= 256 )
	{
		Host_Error( "Too many client window buttons!" );
	}

	buttons[m_iNumBtns++] = act;

	AddItem( act );

	return act;
}
