/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "extdll.h"
#include "basemenu.h"
#include "utils.h"
#include "menu_btnsbmp_table.h"

#define ART_BANNER			"gfx/shell/head_multi"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_INTERNETGAMES		2
#define ID_SPECTATEGAMES		3
#define ID_LANGAME			4
#define ID_CUSTOMIZE		5
#define ID_CONTROLS			6
#define ID_DONE			7

typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuPicButton_s	internetGames;
	menuPicButton_s	spectateGames;
	menuPicButton_s	LANGame;
	menuPicButton_s	Customize;	// playersetup
	menuPicButton_s	Controls;
	menuPicButton_s	done;
} uiMultiPlayer_t;

static uiMultiPlayer_t	uiMultiPlayer;

/*
=================
UI_MultiPlayer_Callback
=================
*/
static void UI_MultiPlayer_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_INTERNETGAMES:
		UI_InternetGames_Menu();
		break;
	case ID_SPECTATEGAMES:
		// UNDONE: not implemented
		break;
	case ID_LANGAME:
		UI_LanGame_Menu();
		break;
	case ID_CUSTOMIZE:
		UI_PlayerSetup_Menu();
		break;
	case ID_CONTROLS:
		UI_Controls_Menu();
		break;
	case ID_DONE:
		UI_PopMenu();
		break;
	}
}

/*
=================
UI_MultiPlayer_Init
=================
*/
static void UI_MultiPlayer_Init( void )
{
	memset( &uiMultiPlayer, 0, sizeof( uiMultiPlayer_t ));

	uiMultiPlayer.menu.vidInitFunc = UI_MultiPlayer_Init;

	uiMultiPlayer.background.generic.id = ID_BACKGROUND;
	uiMultiPlayer.background.generic.type = QMTYPE_BITMAP;
	uiMultiPlayer.background.generic.flags = QMF_INACTIVE;
	uiMultiPlayer.background.generic.x = 0;
	uiMultiPlayer.background.generic.y = 0;
	uiMultiPlayer.background.generic.width = 1024;
	uiMultiPlayer.background.generic.height = 768;
	uiMultiPlayer.background.pic = ART_BACKGROUND;

	uiMultiPlayer.banner.generic.id = ID_BANNER;
	uiMultiPlayer.banner.generic.type = QMTYPE_BITMAP;
	uiMultiPlayer.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiMultiPlayer.banner.generic.x = UI_BANNER_POSX;
	uiMultiPlayer.banner.generic.y = UI_BANNER_POSY;
	uiMultiPlayer.banner.generic.width = UI_BANNER_WIDTH;
	uiMultiPlayer.banner.generic.height = UI_BANNER_HEIGHT;
	uiMultiPlayer.banner.pic = ART_BANNER;

	uiMultiPlayer.internetGames.generic.id = ID_INTERNETGAMES;
	uiMultiPlayer.internetGames.generic.type = QMTYPE_BM_BUTTON;
	uiMultiPlayer.internetGames.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMultiPlayer.internetGames.generic.x = 72;
	uiMultiPlayer.internetGames.generic.y = 230;
	uiMultiPlayer.internetGames.generic.name = "Internet games";
	uiMultiPlayer.internetGames.generic.statusText = "View list of a game internet servers and join the one of your choise";
	uiMultiPlayer.internetGames.generic.callback = UI_MultiPlayer_Callback;

	UI_UtilSetupPicButton( &uiMultiPlayer.internetGames, PC_INET_GAME );

	uiMultiPlayer.spectateGames.generic.id = ID_SPECTATEGAMES;
	uiMultiPlayer.spectateGames.generic.type = QMTYPE_BM_BUTTON;
	uiMultiPlayer.spectateGames.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY|QMF_GRAYED;
	uiMultiPlayer.spectateGames.generic.x = 72;
	uiMultiPlayer.spectateGames.generic.y = 280;
	uiMultiPlayer.spectateGames.generic.name = "Spectate games";
	uiMultiPlayer.spectateGames.generic.statusText = "Spectate internet games";
	uiMultiPlayer.spectateGames.generic.callback = UI_MultiPlayer_Callback;

	UI_UtilSetupPicButton( &uiMultiPlayer.spectateGames, PC_SPECTATE_GAMES );

	uiMultiPlayer.LANGame.generic.id = ID_LANGAME;
	uiMultiPlayer.LANGame.generic.type = QMTYPE_BM_BUTTON;
	uiMultiPlayer.LANGame.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMultiPlayer.LANGame.generic.x = 72;
	uiMultiPlayer.LANGame.generic.y = 330;
	uiMultiPlayer.LANGame.generic.name = "LAN game";
	uiMultiPlayer.LANGame.generic.statusText = "Set up the game on the local area network";
	uiMultiPlayer.LANGame.generic.callback = UI_MultiPlayer_Callback;

	UI_UtilSetupPicButton( &uiMultiPlayer.LANGame, PC_LAN_GAME );

	uiMultiPlayer.Customize.generic.id = ID_CUSTOMIZE;
	uiMultiPlayer.Customize.generic.type = QMTYPE_BM_BUTTON;
	uiMultiPlayer.Customize.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMultiPlayer.Customize.generic.x = 72;
	uiMultiPlayer.Customize.generic.y = 380;
	uiMultiPlayer.Customize.generic.name = "Customize";
	uiMultiPlayer.Customize.generic.statusText = "Choose your player name, and select visual options for your character";
	uiMultiPlayer.Customize.generic.callback = UI_MultiPlayer_Callback;

	UI_UtilSetupPicButton( &uiMultiPlayer.Customize, PC_CUSTOMIZE );

	uiMultiPlayer.Controls.generic.id = ID_CONTROLS;
	uiMultiPlayer.Controls.generic.type = QMTYPE_BM_BUTTON;
	uiMultiPlayer.Controls.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMultiPlayer.Controls.generic.x = 72;
	uiMultiPlayer.Controls.generic.y = 430;
	uiMultiPlayer.Controls.generic.name = "Controls";
	uiMultiPlayer.Controls.generic.statusText = "Change keyboard and mouse settings";
	uiMultiPlayer.Controls.generic.callback = UI_MultiPlayer_Callback;
	
	UI_UtilSetupPicButton( &uiMultiPlayer.Controls, PC_CONTROLS );

	uiMultiPlayer.done.generic.id = ID_DONE;
	uiMultiPlayer.done.generic.type = QMTYPE_BM_BUTTON;
	uiMultiPlayer.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMultiPlayer.done.generic.x = 72;
	uiMultiPlayer.done.generic.y = 480;
	uiMultiPlayer.done.generic.name = "Done";
	uiMultiPlayer.done.generic.statusText = "Go back to the Main Menu";
	uiMultiPlayer.done.generic.callback = UI_MultiPlayer_Callback;

	UI_UtilSetupPicButton( &uiMultiPlayer.done, PC_DONE );

	UI_AddItem( &uiMultiPlayer.menu, (void *)&uiMultiPlayer.background );
	UI_AddItem( &uiMultiPlayer.menu, (void *)&uiMultiPlayer.banner );
	UI_AddItem( &uiMultiPlayer.menu, (void *)&uiMultiPlayer.internetGames );
	UI_AddItem( &uiMultiPlayer.menu, (void *)&uiMultiPlayer.spectateGames );
	UI_AddItem( &uiMultiPlayer.menu, (void *)&uiMultiPlayer.LANGame );
	UI_AddItem( &uiMultiPlayer.menu, (void *)&uiMultiPlayer.Customize );
	UI_AddItem( &uiMultiPlayer.menu, (void *)&uiMultiPlayer.Controls );
	UI_AddItem( &uiMultiPlayer.menu, (void *)&uiMultiPlayer.done );
}

/*
=================
UI_MultiPlayer_Precache
=================
*/
void UI_MultiPlayer_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_MultiPlayer_Menu
=================
*/
void UI_MultiPlayer_Menu( void )
{
	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		return;

	UI_MultiPlayer_Precache();
	UI_MultiPlayer_Init();

	UI_PushMenu( &uiMultiPlayer.menu );
}