/*
BuyMenu.cpp - Counter-Strike buy menu UI implementation
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
#include "utlstring.h"

// The buy menus as in the Steam version: the places come from its resource/UI/MainBuyMenu.res, Buy*.res and
// classes/<item>.res (in the units of a 640 x 480 screen, see SteamFrame::Res), the colours from
// resource/ClientScheme.res, all the texts in "Default" (Verdana 600) like the other client menus.
//
// At 1920x1080 (pixels): the buttons 411..743, one under another every 72 pixels from 261, "0 CANCEL" at 855;
// "SHOP BY CATEGORY" from x 429, its capitals at 215; the divider of the main menu 771..774 x 261..900;
// "A AUTO-BUY" and "R RE-BUY PREVIOUS" 800..1159. The picture of an item: a box 789..1463 from 261, 288 pixels
// high (180 for the rifles and the shotguns); its text from x 789, the values from 1104, the rows every 27
// pixels (36 for the rifles and the shotguns). The items the player can't afford are drawn in "DisabledText1".

namespace {

const unsigned int COLOR_BRIGHT = PackRGBA( 255, 176, 0, 255 );  // "BrightControlText"
const unsigned int COLOR_LABEL  = PackRGBA( 255, 176, 0, 164 );  // "LabelDimText": the info of an item
const unsigned int COLOR_BORDER = PackRGBA( 188, 112, 0, 128 );  // "BorderBright" / "BorderDark"
const unsigned int COLOR_BOXBG  = PackRGBA( 0, 0, 0, 200 );      // "WindowBG"

// the money of the player (sent by the client in cscl_currentmoney), -1 if it is not known
int PlayerMoney( void )
{
	const char *value = EngFuncs::GetCvarString( "cscl_currentmoney" );
	if( !value || !value[0] )
		return -1;
	return atoi( value );
}

// the price from its text (": $1000"), -1 if there is none
int ParsePrice( const char *text )
{
	if( !text )
		return -1;

	const char *p = strchr( text, '$' );
	if( !p )
		p = text;

	while( *p && ( *p < '0' || *p > '9' ))
		p++;

	if( !*p )
		return -1;

	return atoi( p );
}

void NormalizeTranslationString( const char *source, CUtlString &result )
{
	result.SetValue( "" );
	for ( const char *t = source; *t; ++t )
	{
		if ( t[0] == '\\' && t[1] == 'n' )
		{
			result.Append( "\n" );
			++t;
			continue;
		}

		char temp[2] = { *t, '\0' };
		result.Append( temp );
	}
}

const char *TranslateStringIfFound( const char *key )
{
	const char *translated = L( key );
	return translated == key ? NULL : translated;
}

} // namespace

struct BuyMenuWeaponInfo
{
	const char *name, *image, *command, *name2;
};

class CClientBaseBuyMenu : public CClientWindow
{
public:
	typedef CClientWindow BaseClass;

	// the look of classes/<item>.res: a picture box 300 x 128 units with the rows of the info every 12 units from
	// 126 (pistols, sub-machine guns, the machine gun), 300 x 80 with the rows every 16 from 86 (rifles,
	// shotguns), or the price and the description (equipment)
	enum infoStyle_e
	{
		INFO_TALL,
		INFO_WIDE,
		INFO_EQUIPMENT
	};

	enum ammoClass_e
	{
		BUY_NONE,
		BUY_PRIMAMMO,
		BUY_SECAMMO,
		BUY_EQUIPMENT
	} eAmmoClass = BUY_NONE;

	infoStyle_e eInfoStyle = INFO_TALL;

	void Init() override
	{
		if( !WasInit() )
		{
			CClientWindow::Init();

			CSteamMenuButton *cancel = AddSteamButton( '0', L( "Cstrike_Cancel" ), 0,
				CEventCallback( MenuCb( &CClientBaseBuyMenu::cancelPressedCb ), nullptr ));
			cancel->resX = 76.0f;
			cancel->resY = 380.0f;
			cancel->resW = 148.0f;
			cancel->resH = 20.0f;
			cancel->onGotFocus = MenuCb( &CClientBaseBuyMenu::cancelFocusCb );
		}
	}

	void Reload() override
	{
		m_pShown = numWeapons > 0 ? &weapons[0] : nullptr;
		UpdateInfo();
	}

	void Draw() override
	{
		if( m_bShowScores )
			return; // the table of the players is shown instead of the menu

		UpdateAffordable();
		BaseClass::Draw(); // the frame and the buttons

		if( m_pShown )
			DrawInfo();
	}

	void AddWeapon( BuyMenuWeaponInfo info )
	{
		const int i = numWeapons;
		weapons[i] = info;

		CUtlString labelKey;
		labelKey.Format( "Cstrike_%s", info.name );

		CEventCallback pressCb = MenuCb( &CClientBaseBuyMenu::weaponPressedCb );
		pressCb.pExtra = &weapons[i];

		CSteamMenuButton *btn = AddSteamButton( '1' + i, L( labelKey.String() ), i, pressCb );
		btn->resX = 76.0f;
		btn->resY = 116.0f + 32.0f * i;
		btn->resW = 148.0f;
		btn->resH = 20.0f;
		btn->onPressed.pExtra = &weapons[i];
		btn->onGotFocus = MenuCb( &CClientBaseBuyMenu::weaponFocusCb );
		btn->onGotFocus.pExtra = &weapons[i];

		weaponButtons[i] = btn;
		numWeapons++;
	}

	BuyMenuWeaponInfo weapons[10];
	CSteamMenuButton *weaponButtons[10] = {};
	int prices[10] = {};
	int numWeapons = 0;

private:
	BuyMenuWeaponInfo *m_pShown = nullptr; // the item whose info is shown

	// the texts of the info, made when the shown item changes
	struct InfoRow
	{
		CUtlString label, value;
	};
	InfoRow m_rows[9];
	int m_iNumRows = 0;
	CUtlString m_image;

	int IndexOf( const BuyMenuWeaponInfo *weapon ) const
	{
		for( int i = 0; i < numWeapons; ++i )
			if( &weapons[i] == weapon )
				return i;
		return -1;
	}

	const char *PriceKey( const BuyMenuWeaponInfo *weapon, CUtlString &key ) const
	{
		key.Format( "%s_%sPrice", eAmmoClass == BUY_EQUIPMENT ? "Cstrike" : "CStrike", weapon->name2 );
		return key.String();
	}

	// the items the player can't afford are disabled (as in the Steam version); if the money isn't known all of
	// them can be bought
	void UpdateAffordable( void )
	{
		const int money = PlayerMoney();

		for( int i = 0; i < numWeapons; ++i )
		{
			if( !prices[i] )
			{
				CUtlString key;
				prices[i] = ParsePrice( L( PriceKey( &weapons[i], key )));
				if( !prices[i] )
					prices[i] = -1;
			}

			weaponButtons[i]->disabled = money >= 0 && prices[i] > 0 && prices[i] > money;
		}
	}

	void UpdateInfo( void )
	{
		m_iNumRows = 0;
		m_image = "";

		const BuyMenuWeaponInfo *weapon = m_pShown;
		if( !weapon )
			return;

		CUtlString key;
		auto addRow = [this]( const char *label, const char *value )
		{
			if( m_iNumRows >= (int)( sizeof( m_rows ) / sizeof( m_rows[0] )))
				return;
			m_rows[m_iNumRows].label = label ? label : "";
			m_rows[m_iNumRows].value = value ? value : "";
			m_iNumRows++;
		};

		if( eAmmoClass == BUY_EQUIPMENT )
		{
			addRow( L( "Cstrike_PriceLabel" ), L( PriceKey( weapon, key )));

			CUtlString description;
			key.Format( "Cstrike_%sDescription", weapon->name2 );
			NormalizeTranslationString( L( key.String() ), description );
			addRow( L( "Cstrike_DescriptionLabel" ), description.String() );
		}
		else
		{
			addRow( L( "CStrike_PriceLabel" ), L( PriceKey( weapon, key )));

			key.Format( "CStrike_%sOrigin", weapon->name2 );
			addRow( L( "CStrike_OriginLabel" ), L( key.String() ));

			key.Format( "CStrike_%sCalibre", weapon->name2 );
			addRow( L( "CStrike_CalibreLabel" ), L( key.String() ));

			key.Format( "CStrike_%sClipCapacity", weapon->name2 );
			addRow( L( "CStrike_ClipCapacityLabel" ), L( key.String() ));

			key.Format( "CStrike_%sRateOfFire", weapon->name2 );
			addRow( L( "CStrike_RateOfFireLabel" ), L( key.String() ));

			// "WEIGHT (EMPTY)" or "WEIGHT (LOADED)": whichever the item has
			const char *weightLabel = L( "CStrike_WeightEmptyLabel" );
			key.Format( "CStrike_%sWeightEmpty", weapon->name2 );
			const char *weight = TranslateStringIfFound( key.String() );
			if( !weight )
			{
				weightLabel = L( "CStrike_WeightLoadedLabel" );
				key.Format( "CStrike_%sWeightLoaded", weapon->name2 );
				weight = TranslateStringIfFound( key.String() );
			}
			addRow( weightLabel, weight ? weight : L( key.String() ));

			key.Format( "CStrike_%sProjectileWeight", weapon->name2 );
			addRow( L( "CStrike_ProjectileWeightLabel" ), L( key.String() ));

			key.Format( "CStrike_%sMuzzleVelocity", weapon->name2 );
			addRow( L( "CStrike_MuzzleVelocityLabel" ), L( key.String() ));

			key.Format( "CStrike_%sMuzzleEnergy", weapon->name2 );
			addRow( L( "CStrike_MuzzleEnergyLabel" ), L( key.String() ));
		}

		m_image.Format( "gfx/vgui/%s.tga", weapon->image );
	}

	// the info of the item: ItemInfo of Buy*.res at (244, 116), its contents from classes/<item>.res
	void DrawInfo()
	{
		const SteamFrame::Layout &l = m_frame;
		const float infoX = 244.0f, infoY = 116.0f;

		float boxH = 128.0f, imageY = 8.0f, imageH = 128.0f, rowY = 126.0f, rowStep = 12.0f;
		if( eInfoStyle == INFO_WIDE )
		{
			boxH = 80.0f;
			imageH = 64.0f;
			rowY = 86.0f;
			rowStep = 16.0f;
		}
		else if( eInfoStyle == INFO_EQUIPMENT )
		{
			imageY = 4.0f;
			rowY = 138.0f;
			rowStep = 18.0f;
		}

		// the picture in its box ("WindowBG" with a frame of 1 pixel); the picture may stick out of the box
		const int bx = SteamFrame::Res::X( l, infoX );
		const int by = SteamFrame::Res::Y( l, infoY );
		const int bw = l.U( 300.0f );
		const int bh = l.U( boxH );
		{
			UI_FillRect( bx, by, bw, bh, COLOR_BOXBG );

			if( m_image.Length() )
			{
				UI_DrawPic( bx, SteamFrame::Res::Y( l, infoY + imageY ), l.U( 256.0f ), l.U( imageH ),
				            uiColorWhite, m_image.String(), QM_DRAWTRANS );
			}

			DrawFrame( bx, by, bw, bh, COLOR_BORDER );
		}

		// the rows: the labels at 0 and the values (": $400") at 140 units; the labels are 24 units high with the
		// text in their middle (equipment: at their top, "north-west")
		const int labelX = bx;
		const int valueX = SteamFrame::Res::X( l, infoX + 140.0f );
		const int labelW = valueX - labelX;
		const int valueW = l.U( 300.0f ) - ( valueX - labelX );
		const int lineH = l.U( 8.45f ); // the lines of a description: 19 pixels at 1080

		for( int i = 0; i < m_iNumRows; ++i )
		{
			const int top = SteamFrame::Res::Y( l, infoY + rowY + rowStep * i );
			const int capTop = eInfoStyle == INFO_EQUIPMENT ? top + l.U( 1.6f ) : top + ( l.U( 24.0f ) - m_iCapHeight + 1 ) / 2;

			DrawSteamText( m_hTextFont, labelX, capTop, m_iCapHeight, labelW, m_rows[i].label.String(), COLOR_LABEL );

			// the value may have several lines (the descriptions of the equipment)
			const char *p = m_rows[i].value.String();
			int lineCap = capTop;
			char buf[256];
			while( p && *p )
			{
				const char *end = strchr( p, '\n' );
				int len = end ? (int)( end - p ) : (int)strlen( p );
				if( len > (int)sizeof( buf ) - 1 )
					len = sizeof( buf ) - 1;
				memcpy( buf, p, len );
				buf[len] = 0;

				DrawSteamText( m_hTextFont, valueX, lineCap, m_iCapHeight, valueW + l.U( 100.0f ), buf, COLOR_LABEL );
				lineCap += lineH;
				p = end ? end + 1 : nullptr;
			}
		}
	}

	void cancelPressedCb( void *pExtra )
	{
		UI_CloseClientMenu();
	}

	void cancelFocusCb( void *pExtra )
	{
		m_pShown = nullptr;
		UpdateInfo();
	}

	// the info of an item is shown under the cursor, also if the player can't afford it (as in the Steam version)
	void weaponFocusCb( void *pExtra )
	{
		m_pShown = (BuyMenuWeaponInfo *)pExtra;
		UpdateInfo();
	}

	void weaponPressedCb( void *pExtra )
	{
		BuyMenuWeaponInfo *weapon = (BuyMenuWeaponInfo*)pExtra;
		const int i = IndexOf( weapon );
		if( i >= 0 && weaponButtons[i] && weaponButtons[i]->disabled )
			return; // the player can't afford it: the button doesn't work, as in the Steam version

		confirmCb( (void*)weapon->command );
	}

	void confirmCb( void *pExtra )
	{
		const char *command = (const char*)pExtra;
		EngFuncs::ClientCmd( true, command );

		if( EngFuncs::GetCvarFloat( "ui_cs_autofill" ))
		{
			switch( eAmmoClass )
			{
			case BUY_NONE:
				break;
			case BUY_PRIMAMMO: EngFuncs::ClientCmd( false, "primammo" ); break;
			case BUY_SECAMMO:  EngFuncs::ClientCmd( false, "secammo" ); break;
			case BUY_EQUIPMENT: break;
			}
		}
		UI_CloseClientMenu();
	}
};

class CClientMainBuyMenu : public CClientWindow {
public:
	typedef CClientWindow BaseClass;

	virtual void _Init() override;
	void Draw() override;

	void SetTeam( int team ) { m_iTeam = team; }
	int GetTeam() const { return m_iTeam; }

private:
	CSteamMenuButton *AddResButton( int key, const char *name, float x, float y, float w, CEventCallback callback )
	{
		CSteamMenuButton *btn = AddSteamButton( key, name, 0, callback );
		btn->resX = x;
		btn->resY = y;
		btn->resW = w;
		btn->resH = 20.0f;
		return btn;
	}

	int m_iTeam = TEAM_TERRORIST;
	CSteamCheckBox m_autoFill; // "Auto buy ammo" (not in the Steam version): at the bottom of the menu
};
class CClientPistolsTMenu         : public CClientBaseBuyMenu { virtual void _Init() override; };
class CClientPistolsCTMenu        : public CClientBaseBuyMenu { virtual void _Init() override; };
class CClientShotgunsMenu         : public CClientBaseBuyMenu { virtual void _Init() override; };
class CClientSubMachineGunsTMenu  : public CClientBaseBuyMenu { virtual void _Init() override; };
class CClientSubMachineGunsCTMenu : public CClientBaseBuyMenu { virtual void _Init() override; };
class CClientRiflesTMenu          : public CClientBaseBuyMenu { virtual void _Init() override; };
class CClientRiflesCTMenu         : public CClientBaseBuyMenu { virtual void _Init() override; };
class CClientMachineGunsMenu      : public CClientBaseBuyMenu { virtual void _Init() override; };
class CClientItemTMenu            : public CClientBaseBuyMenu { virtual void _Init() override; };
class CClientItemCTMenu           : public CClientBaseBuyMenu { virtual void _Init() override; };

template <typename T>
void Menu_Show( void )
{
	EngFuncs::KEY_SetDest( KEY_MENU );
	static T menu;
	menu.Show();
}

template <typename T>
void Menu_Show( int team )
{
	EngFuncs::KEY_SetDest( KEY_MENU );
	static T menu;
	menu.Show();
}

template <>
void Menu_Show<CClientMainBuyMenu>( int team )
{
	EngFuncs::KEY_SetDest( KEY_MENU );
	static CClientMainBuyMenu menu;
	menu.SetTeam( team );
	menu.Show();
}

template <typename T, typename CT>
void Menu_Show_Team( int num ) { num == TEAM_TERRORIST ? Menu_Show<T>() : Menu_Show<CT>(); }

template <typename T, typename CT>
void Menu_Show_Team( CMenuBaseItem *item, void * ) {
	item->Parent()->Hide();
	CClientMainBuyMenu *menu = static_cast<CClientMainBuyMenu *>( item->Parent() );
	if( menu && menu->GetTeam() == TEAM_TERRORIST )
		Menu_Show<T>();
	else
		Menu_Show<CT>();
}

void UI_BuyMenu_Show( int param1, int param2 ) { Menu_Show<CClientMainBuyMenu>( param2 ); }
void UI_BuyMenu_Pistol_Show( int param1, int param2 ) { Menu_Show_Team<CClientPistolsTMenu, CClientPistolsCTMenu>( param2 ); }
void UI_BuyMenu_Shotgun_Show( int param1, int param2 ) { Menu_Show<CClientShotgunsMenu>( param2 ); }
void UI_BuyMenu_Submachine_Show( int param1, int param2 ) { Menu_Show_Team<CClientSubMachineGunsTMenu, CClientSubMachineGunsCTMenu>( param2 ); }
void UI_BuyMenu_Rifle_Show( int param1, int param2 ) { Menu_Show_Team<CClientRiflesTMenu, CClientRiflesCTMenu>( param2 ); }
void UI_BuyMenu_Machinegun_Show( int param1, int param2 ) { Menu_Show_Team<CClientMachineGunsMenu, CClientMachineGunsMenu>( param2 ); }
void UI_BuyMenu_Item_Show( int param1, int param2 ) { Menu_Show_Team<CClientItemTMenu, CClientItemCTMenu>( param2 ); }

// resource/UI/MainBuyMenu.res: the categories at 76 from 116 every 32 units, "0 CANCEL" at 380, "A" and "R" at
// 249, a divider at 236 between them, "SHOP BY CATEGORY" at (84, 87)
void CClientMainBuyMenu::_Init()
{
	szName = L("Cstrike_Buy_Menu");

	float y = 116.0f;
	AddResButton( '1', L( "Cstrike_Pistols" ), 76.0f, y, 148.0f, Menu_Show_Team<CClientPistolsTMenu, CClientPistolsCTMenu> ); y += 32.0f;
	AddResButton( '2', L( "Cstrike_Shotguns" ), 76.0f, y, 148.0f, Menu_Show_Team<CClientShotgunsMenu, CClientShotgunsMenu> ); y += 32.0f;
	AddResButton( '3', L( "Cstrike_SubMachineGuns" ), 76.0f, y, 148.0f, Menu_Show_Team<CClientSubMachineGunsTMenu, CClientSubMachineGunsCTMenu> ); y += 32.0f;
	AddResButton( '4', L( "Cstrike_Rifles" ), 76.0f, y, 148.0f, Menu_Show_Team<CClientRiflesTMenu, CClientRiflesCTMenu> ); y += 32.0f;
	AddResButton( '5', L( "Cstrike_MachineGuns" ), 76.0f, y, 148.0f, Menu_Show_Team<CClientMachineGunsMenu, CClientMachineGunsMenu> ); y += 32.0f;
	AddResButton( '6', L( "Cstrike_Prim_Ammo" ), 76.0f, y, 148.0f, ExecAndHide( "primammo" )); y += 32.0f;
	AddResButton( '7', L( "Cstrike_Sec_Ammo" ), 76.0f, y, 148.0f, ExecAndHide( "secammo" )); y += 32.0f;
	AddResButton( '8', L( "Cstrike_Equipment" ), 76.0f, y, 148.0f, Menu_Show_Team<CClientItemTMenu, CClientItemCTMenu> );
	AddResButton( '0', L( "Cstrike_Cancel" ), 76.0f, 380.0f, 148.0f, CEventCallback( []( CMenuBaseItem *, void * ) { UI_CloseClientMenu(); } ));

	// the letters work in both cases
	CSteamMenuButton *autobuy = AddResButton( 'a', L( "Cstrike_BuyMenuAutobuy" ), 249.0f, 116.0f, 160.0f, ExecAndHide( "autobuy" ));
	CSteamMenuButton *rebuy = AddResButton( 'r', L( "Cstrike_BuyMenuRebuy" ), 249.0f, 148.0f, 160.0f, ExecAndHide( "rebuy" ));
	keys['A'] = autobuy->onPressed;
	keyButtons['A'] = autobuy;
	keys['R'] = rebuy->onPressed;
	keyButtons['R'] = rebuy;

	// the ammunition is bought with the weapon (ui_cs_autofill); this is not in the Steam version, so the check box
	// is at the bottom, under "0 CANCEL"
	Q_strncpy( m_autoFill.label, L( "Auto buy ammo" ), sizeof( m_autoFill.label ));
	m_autoFill.cvarName = "ui_cs_autofill";
	m_autoFill.frame = &m_frame;
	m_autoFill.resX = 76.0f;
	m_autoFill.resY = 414.0f;
	AddItem( m_autoFill );
}

void CClientMainBuyMenu::Draw()
{
	if( m_bShowScores )
		return; // the table of the players is shown instead of the menu

	m_autoFill.textFont = m_hTextFont;
	m_autoFill.capHeight = m_iCapHeight;

	BaseClass::Draw(); // the frame, the buttons and the check box

	const SteamFrame::Layout &l = m_frame;

	// "SHOP BY CATEGORY" ("BrightControlText"), its capitals 19 pixels below the top of its label at 1080
	DrawSteamText( m_hTextFont, SteamFrame::Res::X( l, 84.0f ), SteamFrame::Res::Y( l, 87.0f ) + l.U( 8.6f ), m_iCapHeight,
	               l.U( 160.0f ), L( "Cstrike_Select_Category" ), COLOR_BRIGHT );

	// the divider between the columns: 2 pixels at 1080 (771..772), dimmer than the frames of the buttons
	const int dividerW = (int)l.u > 0 ? (int)l.u : 1;
	UI_FillRect( SteamFrame::Res::X( l, 236.0f ), SteamFrame::Res::Y( l, 116.0f ), dividerW, l.U( 284.0f ), PackRGBA( 155, 121, 58, 128 ));
}

void CClientPistolsTMenu::_Init()
{
	szName = L( "Cstrike_PistolsLabel" );
	eAmmoClass = BUY_SECAMMO;
	eInfoStyle = INFO_TALL;

	AddWeapon( BuyMenuWeaponInfo{ "Glock18", "glock18", "glock", "Glock" });
	AddWeapon( BuyMenuWeaponInfo{ "USP45", "usp45", "usp", "USP45" });
	AddWeapon( BuyMenuWeaponInfo{ "P228", "p228", "p228", "P228" });
	AddWeapon( BuyMenuWeaponInfo{ "DesertEagle", "deserteagle", "deagle", "DesertEagle" });
	AddWeapon( BuyMenuWeaponInfo{ "Elites", "elites", "elites", "Elites" });

}
void CClientPistolsCTMenu::_Init()
{
	szName = L( "Cstrike_PistolsLabel" );
	eAmmoClass = BUY_SECAMMO;
	eInfoStyle = INFO_TALL;

	AddWeapon( BuyMenuWeaponInfo{ "Glock18", "glock18", "glock", "Glock" });
	AddWeapon( BuyMenuWeaponInfo{ "USP45", "usp45", "usp", "USP45" });
	AddWeapon( BuyMenuWeaponInfo{ "P228", "p228", "p228", "P228" });
	AddWeapon( BuyMenuWeaponInfo{ "DesertEagle", "deserteagle", "deagle", "DesertEagle" });
	AddWeapon( BuyMenuWeaponInfo{ "FiveSeven", "fiveseven", "fiveseven", "FiveSeven" });
}
void CClientShotgunsMenu::_Init()
{
	szName = L( "Cstrike_ShotgunsLabel" );
	eAmmoClass = BUY_PRIMAMMO;
	eInfoStyle = INFO_WIDE;

	AddWeapon( BuyMenuWeaponInfo{ "m3", "m3", "m3", "M3" });
	AddWeapon( BuyMenuWeaponInfo{ "xm1014", "xm1014", "xm1014",	"XM1014" });
}

void CClientSubMachineGunsTMenu::_Init()
{
	szName = L( "Cstrike_SubmachinegunsLabel" );
	eAmmoClass = BUY_PRIMAMMO;
	eInfoStyle = INFO_TALL;

	AddWeapon( BuyMenuWeaponInfo{ "MAC10", "mac10", "mac10", "Mac10" });
	AddWeapon( BuyMenuWeaponInfo{ "MP5", "mp5", "mp5", "MP5" });
	AddWeapon( BuyMenuWeaponInfo{ "UMP45", "ump45", "ump45", "UMP45" });
	AddWeapon( BuyMenuWeaponInfo{ "P90", "p90", "p90", "P90" });
}
void CClientSubMachineGunsCTMenu::_Init()
{
	szName = L( "Cstrike_SubmachinegunsLabel" );
	eAmmoClass = BUY_PRIMAMMO;
	eInfoStyle = INFO_TALL;

	AddWeapon( BuyMenuWeaponInfo{ "Tmp", "tmp", "tmp", "TMP" });
	AddWeapon( BuyMenuWeaponInfo{ "MP5", "mp5", "mp5", "MP5" });
	AddWeapon( BuyMenuWeaponInfo{ "UMP45", "ump45", "ump45", "UMP45" });
	AddWeapon( BuyMenuWeaponInfo{ "P90", "p90", "p90", "P90" });
}
void CClientRiflesTMenu::_Init()
{
	szName = L( "Cstrike_RiflesLabel" );
	eAmmoClass = BUY_PRIMAMMO;
	eInfoStyle = INFO_WIDE;

	AddWeapon( BuyMenuWeaponInfo{ "Galil", "galil", "galil", "Galil" });
	AddWeapon( BuyMenuWeaponInfo{ "AK47", "ak47", "ak47", "AK47" });
	AddWeapon( BuyMenuWeaponInfo{ "Scout_TER", "scout", "scout", "Scout" });
	AddWeapon( BuyMenuWeaponInfo{ "SG552", "sg552", "sg552", "SG552" });
	AddWeapon( BuyMenuWeaponInfo{ "AWP_TER", "awp", "awp", "AWP" });
	AddWeapon( BuyMenuWeaponInfo{ "G3SG1", "g3sg1", "g3sg1", "G3SG1" });

}
void CClientRiflesCTMenu::_Init()
{
	szName = L( "Cstrike_RiflesLabel" );
	eAmmoClass = BUY_PRIMAMMO;
	eInfoStyle = INFO_WIDE;

	AddWeapon( BuyMenuWeaponInfo{ "Famas", "famas", "famas", "Famas" });
	AddWeapon( BuyMenuWeaponInfo{ "Scout_CT", "scout", "scout", "Scout" });
	AddWeapon( BuyMenuWeaponInfo{ "M4A1", "m4a1", "m4a1", "M4A1" });
	AddWeapon( BuyMenuWeaponInfo{ "Aug", "aug", "aug", "Aug" });
	AddWeapon( BuyMenuWeaponInfo{ "SG550", "sg550", "sg550", "SG550" });
	AddWeapon( BuyMenuWeaponInfo{ "AWP_CT", "awp", "awp", "AWP" });

}
void CClientMachineGunsMenu::_Init()
{
	szName = L( "Cstrike_MachinegunsLabel" );
	eAmmoClass = BUY_PRIMAMMO;
	eInfoStyle = INFO_TALL;

	AddWeapon( BuyMenuWeaponInfo{ "M249", "m249", "m249", "M249" });
}
void CClientItemTMenu::_Init()
{
	szName = L( "Cstrike_EquipmentLabel" );
	eAmmoClass = BUY_EQUIPMENT;
	eInfoStyle = INFO_EQUIPMENT;

	AddWeapon( BuyMenuWeaponInfo{ "Kevlar", "kevlar", "vest", "Kevlar" });
	AddWeapon( BuyMenuWeaponInfo{ "Kevlar_Helmet", "kevlar_helmet", "vesthelm", "KevlarHelmet" });
	AddWeapon( BuyMenuWeaponInfo{ "Flashbang", "flashbang", "flash", "Flashbang" });
	AddWeapon( BuyMenuWeaponInfo{ "HE_Grenade", "hegrenade", "hegren", "HEGrenade" });
	AddWeapon( BuyMenuWeaponInfo{ "Smoke_Grenade", "smokegrenade", "sgren", "SmokeGrenade" });
	AddWeapon( BuyMenuWeaponInfo{ "NightVision_Button_TER", "nightvision", "nvgs", "Nightvision" });
}
void CClientItemCTMenu::_Init()
{
	szName = L( "Cstrike_EquipmentLabel" );
	eAmmoClass = BUY_EQUIPMENT;
	eInfoStyle = INFO_EQUIPMENT;

	AddWeapon( BuyMenuWeaponInfo{ "Kevlar", "kevlar", "vest", "Kevlar" });
	AddWeapon( BuyMenuWeaponInfo{ "Kevlar_Helmet", "kevlar_helmet", "vesthelm", "KevlarHelmet" });
	AddWeapon( BuyMenuWeaponInfo{ "Flashbang", "flashbang", "flash", "Flashbang" });
	AddWeapon( BuyMenuWeaponInfo{ "HE_Grenade", "hegrenade", "hegren", "HEGrenade" });
	AddWeapon( BuyMenuWeaponInfo{ "Smoke_Grenade", "smokegrenade", "sgren", "SmokeGrenade" });
	AddWeapon( BuyMenuWeaponInfo{ "Defuser", "defuser", "defuser", "Defuser" });
	AddWeapon( BuyMenuWeaponInfo{ "NightVision_Button_CT", "nightvision", "nvgs", "Nightvision" });
	AddWeapon( BuyMenuWeaponInfo{ "Shield", "shield", "shield", "Shield" });
}
