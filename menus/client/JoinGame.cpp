/*
JoinGame.cpp - Team selection menu
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

static class CClientJoinGame : public CClientWindow
{
public:
	typedef CClientWindow BaseClass;
	CClientJoinGame() : BaseClass( "CClientJoinGame" ) {}

	void _Init() override;
	void Reload() override;
	void VidInit() override;
	void Draw() override;
	bool KeyDown( int key ) override;

	bool hasSpectator;
	bool hasVIP;
	bool hasCancel;
	CMenuAction *spectate;
	CMenuAction *vipbutton;
	CMenuAction *cancel;

	CEventCallback MakeCb( const char *cmd )
	{
		return CEventCallback( MenuCb( &CClientJoinGame::teamPressedCb ), (void *)cmd );
	}

private:
	const char *command;

	// the description of the map
	char m_szInfo[8192];
	int m_iInfoLines = 0;
	int m_iScroll = 0;          // the first line shown

	int VisibleLines() const;
	void InfoBoxRects( int &bx, int &by, int &bw, int &bh, int &sx, int &sbh ) const;

	void ConfirmSelection()
	{
		Hide();
		EngFuncs::ClientCmd( false, command );
	}

	void teamPressedCb( void *pExtra )
	{
		command = (const char *)pExtra;
		ConfirmSelection();
	}

} uiJoinGame;

void CClientJoinGame::_Init()
{
	m_szInfo[0] = 0;

	// the places of the Steam version: 1, 2, 3 (VIP), then 5, 6 and 0 (cancel) a place lower
	AddSteamButton( '1', L( "Cstrike_Terrorist_Forces" ), 0, MakeCb( "jointeam 1" ) );
	AddSteamButton( '2', L( "Cstrike_CT_Forces" ), 1, MakeCb( "jointeam 2" ) );
	vipbutton = AddSteamButton( '3', L( "Cstrike_VIP_Team" ), 2, MakeCb( "jointeam 3" ) );
	AddSteamButton( '5', L( "Cstrike_Team_AutoAssign" ), 3, MakeCb( "jointeam 5" ) );
	spectate = AddSteamButton( '6', L( "Cstrike_Menu_Spectate" ), 4, MakeCb( "jointeam 6" ) );
	cancel = AddSteamButton( '0', L( "Cstrike_Cancel" ), 5, CEventCallback( VoidCb( &CMenuBaseWindow::Hide ) ) );

	szName = L("Cstrike_Join_Team");
}

void CClientJoinGame::VidInit()
{
	BaseClass::VidInit(); // the frame, the buttons and their font
}

void CClientJoinGame::InfoBoxRects( int &bx, int &by, int &bw, int &bh, int &sx, int &sbh ) const
{
	SteamFrame::TeamMenu::InfoBox( m_frame, bx, by, bw, bh );
	sx = bx + bw - 1 - SteamFrame::TeamMenu::ScrollBarWidth( m_frame ); // the left edge of the scroll bar
	sbh = SteamFrame::TeamMenu::ScrollButtonHeight( m_frame );
}

int CClientJoinGame::VisibleLines() const
{
	int bx, by, bw, bh, sx, sbh;
	InfoBoxRects( bx, by, bw, bh, sx, sbh );

	const int lineH = SteamFrame::TeamMenu::LineHeight( m_frame );
	const int lines = ( bh - 2 - SteamFrame::TeamMenu::FirstCapTop( m_frame )) / ( lineH > 0 ? lineH : 1 );
	return lines > 1 ? lines : 1;
}

void CClientJoinGame::Draw()
{
	if( m_bShowScores )
		return; // the table of the players is shown instead of the menu

	BaseClass::Draw(); // the frame and the buttons

	int bx, by, bw, bh, sx, sbh;
	InfoBoxRects( bx, by, bw, bh, sx, sbh );

	const unsigned int border = PackRGBA( 188, 112, 0, 128 );

	// the box: "WindowBG" (black, 200) with a frame of 1 pixel
	UI_FillRect( bx + 1, by + 1, bw - 2, bh - 2, PackRGBA( 0, 0, 0, 200 ));
	DrawFrame( bx, by, bw, bh, border );

	// the scroll bar: a darker track and two buttons in frames with grey arrows (1445..1497 x 262..314 and
	// 850..902 at 1080); the buttons are as wide as the track, their frames touch the frame of the box (as the
	// scroll bar of the list of the players of the spectator mode)
	const int trackY = by + 1, trackH = bh - 2, trackW = bx + bw - 1 - sx;
	UI_FillRect( sx, trackY, trackW, trackH, PackRGBA( 0, 0, 0, 128 ));
	DrawFrame( sx, trackY, trackW, sbh, border );
	DrawFrame( sx, trackY + trackH - sbh, trackW, sbh, border );

	const unsigned int arrow = PackRGBA( 176, 175, 173, 255 );
	const int arrowW = m_frame.U( 3.1f ) | 1;        // 7 at 1080
	const int arrowH = ( arrowW + 1 ) / 2;           // 4
	// in the middle of the buttons (the Steam version has them 9 pixels from the right edge)
	const int arrowX = sx + trackW / 2;
	const int upY = trackY + ( sbh - arrowH ) / 2;
	const int downY = trackY + trackH - sbh + ( sbh - arrowH ) / 2;

	for( int i = 0; i < arrowH; ++i )
	{
		const int half = i;                      // the point at the top of the up arrow
		UI_FillRect( arrowX - half, upY + i, half * 2 + 1, 1, arrow );
		UI_FillRect( arrowX - half, downY + arrowH - 1 - i, half * 2 + 1, 1, arrow );
	}

	// the text, line by line from the first line shown
	const int lineH = SteamFrame::TeamMenu::LineHeight( m_frame );
	const int textX = bx + SteamFrame::TeamMenu::TextInset( m_frame );
	const int textW = sx - textX - m_frame.U( 2.0f );
	int capTop = by + SteamFrame::TeamMenu::FirstCapTop( m_frame );
	const int visible = VisibleLines();

	const char *p = m_szInfo;
	int line = 0, shown = 0;
	char buf[512];

	while( *p && shown < visible )
	{
		const char *end = p;
		while( *end && *end != '\n' )
			end++;

		if( line >= m_iScroll )
		{
			int len = (int)( end - p );
			if( len > (int)sizeof( buf ) - 1 )
				len = sizeof( buf ) - 1;
			memcpy( buf, p, len );
			buf[len] = 0;
			if( len > 0 && buf[len - 1] == '\r' )
				buf[len - 1] = 0;

			DrawSteamText( m_hTextFont, textX, capTop, m_iCapHeight, textW, buf, PackRGBA( 255, 176, 0, 255 ));
			capTop += lineH;
			shown++;
		}

		line++;
		p = *end ? end + 1 : end;
	}
}

bool CClientJoinGame::KeyDown( int key )
{
	int bx, by, bw, bh, sx, sbh;
	InfoBoxRects( bx, by, bw, bh, sx, sbh );

	const int cx = uiStatic.cursorX, cy = uiStatic.cursorY;
	const bool inBox = cx >= bx && cx < bx + bw && cy >= by && cy < by + bh;
	const int maxScroll = m_iInfoLines - VisibleLines() > 0 ? m_iInfoLines - VisibleLines() : 0;

	if( inBox && ( key == K_MWHEELUP || key == K_MWHEELDOWN ))
	{
		m_iScroll += key == K_MWHEELUP ? -3 : 3;
		m_iScroll = m_iScroll < 0 ? 0 : ( m_iScroll > maxScroll ? maxScroll : m_iScroll );
		return true;
	}

	if( key == K_MOUSE1 && cx >= sx && cx < bx + bw )
	{
		if( cy >= by && cy < by + sbh )
		{
			m_iScroll = m_iScroll > 0 ? m_iScroll - 1 : 0;
			return true;
		}

		if( cy >= by + bh - sbh && cy < by + bh )
		{
			m_iScroll = m_iScroll < maxScroll ? m_iScroll + 1 : maxScroll;
			return true;
		}
	}

	return BaseClass::KeyDown( key );
}

void CClientJoinGame::Reload()
{
	// the hidden buttons don't answer their keys either
	SetButtonShown( spectate, '6', hasSpectator );
	SetButtonShown( vipbutton, '3', hasVIP );
	SetButtonShown( cancel, '0', hasCancel );

	const char *mapname = g_pClient->GetLevelName();
	const char *text = L( "Cstrike_TitlesTXT_Map_Description_not_available" );
	char *txt = nullptr;

	if( mapname && mapname[0] )
	{
		char buf[256];
		snprintf( buf, 256, "maps/%s.txt", mapname );
		txt = (char*)EngFuncs::COM_LoadFile( buf );
		if( txt )
			text = txt;
	}

	Q_strncpy( m_szInfo, text ? text : "", sizeof( m_szInfo ));

	if( txt )
		EngFuncs::COM_FreeFile( txt );

	m_iInfoLines = 0;
	for( const char *p = m_szInfo; *p; ++p )
		if( *p == '\n' )
			m_iInfoLines++;
	if( m_szInfo[0] )
		m_iInfoLines++;
	m_iScroll = 0;
}

void UI_JoinGame_Show( int param1, int param2 )
{
	uiJoinGame.hasSpectator = param1 & 1;
	uiJoinGame.hasVIP		= param1 & 2;
	uiJoinGame.hasCancel    = param1 & 4;

	uiJoinGame.Show();
}
