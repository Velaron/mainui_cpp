/*
JoinClass.cpp - Character class selection menu
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
#include "Scissor.h"
#include "PlayerModelView.h"
#include <math.h>

// The 3D model of the class (ui_showclassmodels 1) in the box of the picture of the Steam version
class CSteamClassModelView : public CMenuPlayerModelView
{
public:
	typedef CMenuPlayerModelView BaseClass;
	const SteamFrame::Layout *frame = nullptr;

	void VidInit( void ) override
	{
		BaseClass::VidInit();

		if( !frame )
			return;

		int bx, by, bw, bh;
		SteamFrame::ClassMenu::PictureBox( *frame, bx, by, bw, bh );
		m_scPos = Point( bx + 1, by + 1 );
		m_scSize = Size( bw - 2, bh - 2 );

		// the camera for the new size of the view
		refdef.viewport[0] = m_scPos.x;
		refdef.viewport[1] = m_scPos.y;
		refdef.viewport[2] = m_scSize.w;
		refdef.viewport[3] = m_scSize.h;
		CalcFov();

		if( ent )
			ent->origin[0] = ent->curstate.origin[0] = 45.0f / tan( DEG2RAD( refdef.fov_y / 2.0f ));
	}
};

// The class menu as in the Steam version (see SteamFrame::ClassMenu): the buttons on the left, the picture of
// the class (gfx/vgui/<class>.tga) in a box on the right and its description under it. The class under the
// cursor (at first "AUTO-SELECT") is lit and shown.
class CClientJoinClass : public CClientWindow
{
public:
	typedef CClientWindow BaseClass;
	CClientJoinClass() : BaseClass( "CClientJoinClass" ) {}

	CEventCallback MakeCb( const char *cmd )
	{
		return CEventCallback( MenuCb( &CClientJoinClass::classPressedCb ), (void*)cmd );
	}

	CEventCallback MakeFocusCb( const char *cmd )
	{
		return CEventCallback( MenuCb( &CClientJoinClass::classFocusCb ), (void*)cmd );
	}

	void _Init() override;
	void Reload() override;
	void VidInit() override;
	void Draw() override;

	CMenuAction *AddClassButton( int key, const char *name, const char *modelname, int slot );

	bool hasCzero;
	bool hasCancel;
	CSteamClassModelView player;
	CMenuAction *cancel = nullptr;

	void ConfirmSelection()
	{
		UI_CloseClientMenu();
		EngFuncs::ClientCmd( false, command );
	}

protected:
	const char *m_szDefaultClass;

private:
	CUtlString command;

	char m_szPicture[64] = "";     // gfx/vgui/<class>.tga (when the 3D model is not shown)
	char m_szDescription[1024] = "";
	HFont m_hDescFont = 0;         // "DefaultSmall": Verdana, thin

	void classFocusCb( void *pExtra )
	{
		const char *sz = (const char *)pExtra;
		const char *loctext = NULL;

		struct ClassEntry {
			const char *modelT;
			const char *modelCT;
			const char *labelT;
			const char *labelCT;
			int joinClass;
		};

		static constexpr int JOINCLASS_AUTO_CS = 5;
		static constexpr int JOINCLASS_AUTO_CZ = 6;

		static const ClassEntry classTable[] =
		{
			{ "terror",   "urban",    "Cstrike_Terror_Label", "Cstrike_Urban_Label", 1 },
			{ "leet",     "gsg9",     "Cstrike_Leet_Label", "Cstrike_GSG9_Label", 2 },
			{ "arctic",   "sas",      "Cstrike_Arctic_Label", "Cstrike_SAS_Label", 3 },
			{ "guerilla", "gign",     "Cstrike_Guerilla_Label", "Cstrike_GIGN_Label", 4 },
		};

		static const ClassEntry czeroClass = { "militia",  "spetsnaz", "Cstrike_Militia_Label", "Cstrike_Spetsnaz_Label", 5 };

		static constexpr int TOTAL_CLASSES = (int)(sizeof(classTable) / sizeof(classTable[0]));

		bool showModel = true;

		if( !strcmp( sz, "t_random" ) || !strcmp( sz, "ct_random" ) )
		{
			int joinClass = hasCzero ? JOINCLASS_AUTO_CZ : JOINCLASS_AUTO_CS;
			command.Format( "joinclass %d", joinClass );
			loctext = L( "Cstrike_AutoSelect_Label" );
			showModel = false;
		}
		else if( hasCzero && !strcmp( sz, czeroClass.modelT ) )
		{
			command.Format( "joinclass %d", czeroClass.joinClass );
			loctext = L( czeroClass.labelT );
		}
		else if( hasCzero && !strcmp( sz, czeroClass.modelCT ) )
		{
			command.Format( "joinclass %d", czeroClass.joinClass );
			loctext = L( czeroClass.labelCT );
		}
		else
		{
			int idx;
			for( idx = 0; idx < TOTAL_CLASSES; idx++ )
			{
				const ClassEntry &entry = classTable[idx];
				if( !strcmp( sz, entry.modelT ) )
				{
					command.Format( "joinclass %d", entry.joinClass );
					loctext = L( entry.labelT );
					break;
				}

				if( !strcmp( sz, entry.modelCT ) )
				{
					command.Format( "joinclass %d", entry.joinClass );
					loctext = L( entry.labelCT );
					break;
				}
			}

			if( idx >= TOTAL_CLASSES )
				showModel = false;
		}

		Q_strncpy( m_szDescription, loctext ? loctext : "", sizeof( m_szDescription ));

		// the 3D model if it is wanted (ui_showclassmodels 1) and can be loaded, else the picture
		if( showModel && ui_showclassmodels && ui_showclassmodels->value )
		{
			player.ent = EngFuncs::GetPlayerModel();
			if( player.ent )
			{
				CUtlString model;
				model.Format( "models/player/%s/%s.mdl", sz, sz );
				player.hPlayerImage = 0;
				player.eOverrideMode = CMenuPlayerModelView::PMV_SHOWMODEL;
				EngFuncs::SetModel( player.ent, model );
				if( !player.ent->model )
					showModel = false;
			}
			else
			{
				showModel = false;
			}
		}
		else
		{
			showModel = false;
		}

		if( showModel )
		{
			m_szPicture[0] = 0;
			player.Show();
		}
		else
		{
			snprintf( m_szPicture, sizeof( m_szPicture ), "gfx/vgui/%s.tga", sz );
			player.Hide();
		}

		// the button of this class is lit
		for( int i = 0; i < m_iNumSteamButtons; ++i )
			m_steamButtons[i]->chosen = m_szButtonModel[i] && !strcmp( m_szButtonModel[i], sz );
	}

	void classPressedCb( void *pExtra )
	{
		classFocusCb( pExtra );
		ConfirmSelection();
	}

	const char *m_szButtonModel[16] = {};
};

static class CClientJoinClassT: public CClientJoinClass
{
	typedef CClientJoinClass BaseClass;
public:
	void _Init() override;
} uiJoinClassT;

static class CClientJoinClassCT: public CClientJoinClass
{
	typedef CClientJoinClass BaseClass;
public:
	void _Init() override;
} uiJoinClassCT;

void CClientJoinClass::_Init()
{
	// the 3D model in the box of the picture: its own background, no frame (the box has it)
	player.frame = &m_frame;
	player.SetRect( 0, 0, 1, 1 ); // the real place is set in pixels by its VidInit
	player.backgroundColor = PackRGBA( 0, 0, 0, 200 );
	player.colorStroke = PackRGBA( 0, 0, 0, 0 );
	player.eFocusAnimation = QM_NOFOCUSANIMATION;
	player.bDrawAsPlayer = false;
	player.iFlags |= QMF_MOUSEONLY;
	player.m_bCanCaptureFocus = false;
	AddItem( player );

	szName = L( "Cstrike_Join_Class" );
}

void CClientJoinClass::VidInit()
{
	BaseClass::VidInit(); // the frame, the buttons and their font

	// "DefaultSmall": Verdana of the normal weight, the same size
	m_hDescFont = CreateSteamFont( "Verdana", m_frame.textEm, 1.215f, 400 );
	if( m_hDescFont <= 0 )
		m_hDescFont = m_hTextFont;
}

void CClientJoinClass::Reload()
{
	// "0 CANCEL" (ClassMenu_*.res: under "AUTO-SELECT") only when the class is chosen again
	SetButtonShown( cancel, '0', hasCancel );

	player.eOverrideMode = CMenuPlayerModelView::PMV_SHOWIMAGE;
	player.hPlayerImage = 0;
	player.ent = EngFuncs::GetPlayerModel();
	if( player.ent )
		player.ent->angles[1] += 15;

	// as in the Steam version the menu opens with "AUTO-SELECT" chosen
	if( m_szDefaultClass )
		classFocusCb( (void*)m_szDefaultClass );
}

void CClientJoinClass::Draw()
{
	if( m_bShowScores )
		return; // the table of the players is shown instead of the menu

	BaseClass::Draw(); // the frame and the buttons

	int bx, by, bw, bh;
	SteamFrame::ClassMenu::PictureBox( m_frame, bx, by, bw, bh );

	// the picture: as high as the box, its own proportions, cut by the box (the 3D model, when it is shown,
	// is drawn with the buttons and has its own background)
	if( m_szPicture[0] )
	{
		UI_FillRect( bx + 1, by + 1, bw - 2, bh - 2, PackRGBA( 0, 0, 0, 200 ));

		HIMAGE pic = EngFuncs::PIC_Load( m_szPicture );
		const int pw = pic ? EngFuncs::PIC_Width( pic ) : 0;
		const int ph = pic ? EngFuncs::PIC_Height( pic ) : 0;

		if( pw > 0 && ph > 0 )
		{
			const int h = bh - 2;
			const int w = (int)( (float)h * pw / ph + 0.5f );
			const int x = SteamFrame::ClassMenu::PictureCenterX( m_frame ) - w / 2;

			UI::Scissor::PushScissor( bx + 1, by + 1, bw - 2, bh - 2 );
			UI_DrawPic( x, by + 1, w, h, uiColorWhite, m_szPicture, QM_DRAWTRANS );
			UI::Scissor::PopScissor();
		}
	}

	DrawFrame( bx, by, bw, bh, PackRGBA( 188, 112, 0, 128 ));

	// the description, line by line ("LabelDimText": 255 176 0 164)
	const int lineH = SteamFrame::TeamMenu::LineHeight( m_frame );
	const int x = SteamFrame::ClassMenu::DescriptionX( m_frame );
	const int maxW = m_frame.panelX + m_frame.panelW - m_frame.U( 20.0f ) - x;
	int capTop = SteamFrame::ClassMenu::DescriptionCapTop( m_frame );

	const char *p = m_szDescription;
	char buf[512];

	while( *p && capTop + lineH < m_frame.panelY + m_frame.panelH )
	{
		const char *end = p;
		while( *end && *end != '\n' )
			end++;

		int len = (int)( end - p );
		if( len > (int)sizeof( buf ) - 1 )
			len = sizeof( buf ) - 1;
		memcpy( buf, p, len );
		buf[len] = 0;
		if( len > 0 && buf[len - 1] == '\r' )
			buf[len - 1] = 0;

		DrawSteamText( m_hDescFont, x, capTop, m_iCapHeight, maxW, buf, PackRGBA( 255, 176, 0, 164 ));
		capTop += lineH;
		p = *end ? end + 1 : end;
	}
}

CMenuAction *CClientJoinClass::AddClassButton( int key, const char *name, const char *modelname, int slot )
{
	const int index = m_iNumSteamButtons;
	CSteamMenuButton *act = AddSteamButton( key, name, slot, MakeCb( modelname ));
	act->onGotFocus = MakeFocusCb( modelname );

	if( index < (int)( sizeof( m_szButtonModel ) / sizeof( m_szButtonModel[0] )))
		m_szButtonModel[index] = modelname;

	return act;
}

// the places of the Steam version: 1..4, then the CZ class and "AUTO-SELECT" a place lower
void CClientJoinClassT::_Init()
{
	m_szDefaultClass = "t_random";
	AddClassButton( '1', L( "Cstrike_Terror" ), "terror", 0 );
	AddClassButton( '2', L( "Cstrike_L337_Krew" ), "leet", 1 );
	AddClassButton( '3', L( "Cstrike_Arctic" ), "arctic", 2 );
	AddClassButton( '4', L( "Cstrike_Guerilla" ), "guerilla", 3 );
	if( hasCzero )
	{
		AddClassButton( '5', L( "Cstrike_Militia" ), "militia", 4 );
		AddClassButton( '6', L( "Cstrike_Auto_Select" ), "t_random", 5 );
	}
	else
	{
		AddClassButton( '5', L( "Cstrike_Auto_Select" ), "t_random", 5 );
	}

	cancel = AddSteamButton( '0', L( "Cstrike_Cancel" ), 6, CEventCallback( VoidCb( &CMenuBaseWindow::Hide ) ) );

	BaseClass::_Init();
}

void CClientJoinClassCT::_Init()
{
	m_szDefaultClass = "ct_random";
	AddClassButton( '1', L( "Cstrike_Urban" ), "urban", 0 );
	AddClassButton( '2', L( "Cstrike_GSG9" ), "gsg9", 1 );
	AddClassButton( '3', L( "Cstrike_SAS" ), "sas", 2 );
	AddClassButton( '4', L( "Cstrike_GIGN" ), "gign", 3 );
	if( hasCzero )
	{
		AddClassButton( '5', L( "Cstrike_Spetsnaz" ), "spetsnaz", 4 );
		AddClassButton( '6', L( "Cstrike_Auto_Select" ), "ct_random", 5 );
	}
	else
	{
		AddClassButton( '5', L( "Cstrike_Auto_Select" ), "ct_random", 5 );
	}

	cancel = AddSteamButton( '0', L( "Cstrike_Cancel" ), 6, CEventCallback( VoidCb( &CMenuBaseWindow::Hide ) ) );

	BaseClass::_Init();
}

void UI_JoinClassT_Show( int param1, int param2 )
{
	uiJoinClassT.hasCzero = param1;
	uiJoinClassT.hasCancel = param2 != 0;
	EngFuncs::KEY_SetDest( KEY_MENU );
	uiJoinClassT.Show();
}

void UI_JoinClassCT_Show( int param1, int param2 )
{
	uiJoinClassCT.hasCzero = param1;
	uiJoinClassCT.hasCancel = param2 != 0;
	EngFuncs::KEY_SetDest( KEY_MENU );
	uiJoinClassCT.Show();
}
