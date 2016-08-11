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
#include "keydefs.h"
#include "menu_btnsbmp_table.h"

#define ART_BANNER		"gfx/shell/head_lan"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_JOINGAME		2
#define ID_CREATEGAME	3
#define ID_GAMEINFO		4
#define ID_REFRESH		5
#define ID_DONE		6
#define ID_SERVERSLIST	7
#define ID_TABLEHINT	8

#define ID_MSGBOX	 	9
#define ID_MSGTEXT	 	10
#define ID_YES	 	130
#define ID_NO	 	131

typedef struct
{
	char		gameDescription[UI_MAX_SERVERS][256];
	char		*gameDescriptionPtr[UI_MAX_SERVERS];

	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;
	menuPicButton_s	joinGame;
	menuPicButton_s	createGame;
	menuPicButton_s	gameInfo;
	menuPicButton_s	refresh;
	menuPicButton_s	done;

	// joingame prompt dialog
	menuAction_s	msgBox;
	menuAction_s	dlgMessage1;
	menuAction_s	dlgMessage2;
	menuPicButton_s	yes;
	menuPicButton_s	no;

	menuScrollList_s	gameList;
	menuAction_s	hintMessage;
	char		hintText[MAX_HINT_TEXT];
	int		refreshTime;
} uiLanGame_t;

static uiLanGame_t	uiLanGame;

static void UI_PromptDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide quit dialog
	uiLanGame.joinGame.generic.flags ^= QMF_INACTIVE; 
	uiLanGame.createGame.generic.flags ^= QMF_INACTIVE;
	uiLanGame.gameInfo.generic.flags ^= QMF_INACTIVE;
	uiLanGame.refresh.generic.flags ^= QMF_INACTIVE;
	uiLanGame.done.generic.flags ^= QMF_INACTIVE;
	uiLanGame.gameList.generic.flags ^= QMF_INACTIVE;

	uiLanGame.msgBox.generic.flags ^= QMF_HIDDEN;
	uiLanGame.dlgMessage1.generic.flags ^= QMF_HIDDEN;
	uiLanGame.dlgMessage2.generic.flags ^= QMF_HIDDEN;
	uiLanGame.no.generic.flags ^= QMF_HIDDEN;
	uiLanGame.yes.generic.flags ^= QMF_HIDDEN;

}

/*
=================
UI_LanGame_KeyFunc
=================
*/
static const char *UI_LanGame_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE && !( uiLanGame.dlgMessage1.generic.flags & QMF_HIDDEN ))
	{
		UI_PromptDialog();
		return uiSoundNull;
	}
	return UI_DefaultKey( &uiLanGame.menu, key, down );
}

/*
=================
UI_LanGame_GetGamesList
=================
*/
static void UI_LanGame_GetGamesList( void )
{
	int		i;
	const char	*info;

	for( i = 0; i < uiStatic.numServers; i++ )
	{
		if( i >= UI_MAX_SERVERS ) break;
		info = uiStatic.serverNames[i];

		uiLanGame.gameDescription[i][0] = 0; // mark this string as empty

		StringConcat( uiLanGame.gameDescription[i], Info_ValueForKey( info, "host" ), QMSB_GAME_LENGTH );
		AddSpaces( uiLanGame.gameDescription[i], QMSB_GAME_LENGTH );

		StringConcat( uiLanGame.gameDescription[i], Info_ValueForKey( info, "map" ), QMSB_MAPNAME_LENGTH );
		AddSpaces( uiLanGame.gameDescription[i], QMSB_MAPNAME_LENGTH );

		StringConcat( uiLanGame.gameDescription[i], Info_ValueForKey( info, "numcl" ), QMSB_MAXCL_LENGTH );
		StringConcat( uiLanGame.gameDescription[i], "\\", QMSB_MAXCL_LENGTH );
		StringConcat( uiLanGame.gameDescription[i], Info_ValueForKey( info, "maxcl" ), QMSB_MAXCL_LENGTH );
		AddSpaces( uiLanGame.gameDescription[i], QMSB_MAXCL_LENGTH );

		char ping[10];
		snprintf( ping, 10, "%.f ms", uiStatic.serverPings[i] * 1000 );
		StringConcat( uiLanGame.gameDescription[i], ping, QMSB_PING_LENGTH );
		AddSpaces( uiLanGame.gameDescription[i], QMSB_PING_LENGTH );

		uiLanGame.gameDescriptionPtr[i] = uiLanGame.gameDescription[i];
	}

	for( ; i < UI_MAX_SERVERS; i++ )
		uiLanGame.gameDescriptionPtr[i] = NULL;

	uiLanGame.gameList.itemNames = (const char **)uiLanGame.gameDescriptionPtr;
	uiLanGame.gameList.numItems = 0; // reset it

	if( !uiLanGame.gameList.generic.charHeight )
		return; // to avoid divide integer by zero

	// count number of items
	while( uiLanGame.gameList.itemNames[uiLanGame.gameList.numItems] )
		uiLanGame.gameList.numItems++;

	// calculate number of visible rows
	uiLanGame.gameList.numRows = (uiLanGame.gameList.generic.height2 / uiLanGame.gameList.generic.charHeight) - 2;
	if( uiLanGame.gameList.numRows > uiLanGame.gameList.numItems ) uiLanGame.gameList.numRows = uiLanGame.gameList.numItems;

	if( uiStatic.numServers )
		uiLanGame.joinGame.generic.flags &= ~QMF_GRAYED;
}

/*
=================
UI_LanGame_JoinGame
=================
*/
static void UI_LanGame_JoinGame( void )
{
	if( !strlen( uiLanGame.gameDescription[uiLanGame.gameList.curItem] ))
		return;

	CLIENT_JOIN( uiStatic.serverAddresses[uiLanGame.gameList.curItem] );
}

/*
=================
UI_Background_Ownerdraw
=================
*/
static void UI_Background_Ownerdraw( void *self )
{
	if( !CVAR_GET_FLOAT( "cl_background" ))
		UI_DrawBackground_Callback( self );

	if( uiStatic.realTime > uiLanGame.refreshTime )
	{
		uiLanGame.refreshTime = uiStatic.realTime + 10000; // refresh every 10 secs
		UI_RefreshServerList();
	}

	// serverinfo has been changed update display
	if( uiStatic.updateServers )
	{
		UI_LanGame_GetGamesList ();
		uiStatic.updateServers = false;
	}
}

/*
=================
UI_MsgBox_Ownerdraw
=================
*/
static void UI_MsgBox_Ownerdraw( void *self )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	UI_FillRect( item->x, item->y, item->width, item->height, uiPromptBgColor );
}

/*
=================
UI_LanGame_Callback
=================
*/
static void UI_LanGame_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_JOINGAME:
		if( CL_IsActive( ))
			UI_PromptDialog();
		else UI_LanGame_JoinGame();
		break;
	case ID_CREATEGAME:
		CVAR_SET_FLOAT( "public", 0.0f );
		UI_CreateGame_Menu();
		break;
	case ID_GAMEINFO:
		// UNDONE: not implemented
		break;
	case ID_REFRESH:
		UI_RefreshServerList();
		break;
	case ID_DONE:
		UI_PopMenu();
		break;
	case ID_YES:
		UI_LanGame_JoinGame();
		break;
	case ID_NO:
		UI_PromptDialog();
		break;
	}
}

/*
=================
UI_LanGame_Init
=================
*/
static void UI_LanGame_Init( void )
{
	memset( &uiLanGame, 0, sizeof( uiLanGame_t ));

	uiLanGame.menu.vidInitFunc = UI_LanGame_Init;
	uiLanGame.menu.keyFunc = UI_LanGame_KeyFunc;

	StringConcat( uiLanGame.hintText, "Name", QMSB_GAME_LENGTH );
	AddSpaces( uiLanGame.hintText, QMSB_GAME_LENGTH );
	StringConcat( uiLanGame.hintText, "Map", QMSB_MAPNAME_LENGTH );
	AddSpaces( uiLanGame.hintText, QMSB_MAPNAME_LENGTH );
	StringConcat( uiLanGame.hintText, "Players", QMSB_MAXCL_LENGTH );
	AddSpaces( uiLanGame.hintText, QMSB_MAXCL_LENGTH );
	StringConcat( uiLanGame.hintText, "Ping", QMSB_PING_LENGTH );
	AddSpaces( uiLanGame.hintText, QMSB_PING_LENGTH );

	uiLanGame.background.generic.id = ID_BACKGROUND;
	uiLanGame.background.generic.type = QMTYPE_BITMAP;
	uiLanGame.background.generic.flags = QMF_INACTIVE;
	uiLanGame.background.generic.x = 0;
	uiLanGame.background.generic.y = 0;
	uiLanGame.background.generic.width = uiStatic.width;
	uiLanGame.background.generic.height = 768;
	uiLanGame.background.pic = ART_BACKGROUND;
	uiLanGame.background.generic.ownerdraw = UI_Background_Ownerdraw;

	uiLanGame.banner.generic.id = ID_BANNER;
	uiLanGame.banner.generic.type = QMTYPE_BITMAP;
	uiLanGame.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiLanGame.banner.generic.x = UI_BANNER_POSX;
	uiLanGame.banner.generic.y = UI_BANNER_POSY;
	uiLanGame.banner.generic.width = UI_BANNER_WIDTH;
	uiLanGame.banner.generic.height = UI_BANNER_HEIGHT;
	uiLanGame.banner.pic = ART_BANNER;

	uiLanGame.joinGame.generic.id = ID_JOINGAME;
	uiLanGame.joinGame.generic.type = QMTYPE_BM_BUTTON;
	uiLanGame.joinGame.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_GRAYED;
	uiLanGame.joinGame.generic.x = 72;
	uiLanGame.joinGame.generic.y = 230;
	uiLanGame.joinGame.generic.name = "Join game";
	uiLanGame.joinGame.generic.statusText = "Join to selected game";
	uiLanGame.joinGame.generic.callback = UI_LanGame_Callback;

	UI_UtilSetupPicButton( &uiLanGame.joinGame, PC_JOIN_GAME );

	uiLanGame.createGame.generic.id = ID_CREATEGAME;
	uiLanGame.createGame.generic.type = QMTYPE_BM_BUTTON;
	uiLanGame.createGame.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiLanGame.createGame.generic.x = 72;
	uiLanGame.createGame.generic.y = 280;
	uiLanGame.createGame.generic.name = "Create game";
	uiLanGame.createGame.generic.statusText = "Create new LAN game";
	uiLanGame.createGame.generic.callback = UI_LanGame_Callback;

	UI_UtilSetupPicButton( &uiLanGame.createGame, PC_CREATE_GAME );

	uiLanGame.gameInfo.generic.id = ID_GAMEINFO;
	uiLanGame.gameInfo.generic.type = QMTYPE_BM_BUTTON;
	uiLanGame.gameInfo.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_GRAYED;
	uiLanGame.gameInfo.generic.x = 72;
	uiLanGame.gameInfo.generic.y = 330;
	uiLanGame.gameInfo.generic.name = "View game info";
	uiLanGame.gameInfo.generic.statusText = "Get detail game info";
	uiLanGame.gameInfo.generic.callback = UI_LanGame_Callback;

	UI_UtilSetupPicButton( &uiLanGame.gameInfo, PC_VIEW_GAME_INFO );

	uiLanGame.refresh.generic.id = ID_REFRESH;
	uiLanGame.refresh.generic.type = QMTYPE_BM_BUTTON;
	uiLanGame.refresh.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiLanGame.refresh.generic.x = 72;
	uiLanGame.refresh.generic.y = 380;
	uiLanGame.refresh.generic.name = "Refresh";
	uiLanGame.refresh.generic.statusText = "Refresh servers list";
	uiLanGame.refresh.generic.callback = UI_LanGame_Callback;

	UI_UtilSetupPicButton( &uiLanGame.refresh, PC_REFRESH );

	uiLanGame.done.generic.id = ID_DONE;
	uiLanGame.done.generic.type = QMTYPE_BM_BUTTON;
	uiLanGame.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiLanGame.done.generic.x = 72;
	uiLanGame.done.generic.y = 430;
	uiLanGame.done.generic.name = "Done";
	uiLanGame.done.generic.statusText = "Return to main menu";
	uiLanGame.done.generic.callback = UI_LanGame_Callback;

	UI_UtilSetupPicButton( &uiLanGame.done, PC_DONE );

	uiLanGame.msgBox.generic.id = ID_MSGBOX;
	uiLanGame.msgBox.generic.type = QMTYPE_ACTION;
	uiLanGame.msgBox.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiLanGame.msgBox.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiLanGame.msgBox.generic.x = DLG_X + 192;
	uiLanGame.msgBox.generic.y = 256;
	uiLanGame.msgBox.generic.width = 640;
	uiLanGame.msgBox.generic.height = 256;

	uiLanGame.dlgMessage1.generic.id = ID_MSGTEXT;
	uiLanGame.dlgMessage1.generic.type = QMTYPE_ACTION;
	uiLanGame.dlgMessage1.generic.flags = QMF_INACTIVE|QMF_HIDDEN|QMF_DROPSHADOW;
	uiLanGame.dlgMessage1.generic.name = "Join a network game will exit";
	uiLanGame.dlgMessage1.generic.x = DLG_X + 248;
	uiLanGame.dlgMessage1.generic.y = 280;

	uiLanGame.dlgMessage2.generic.id = ID_MSGTEXT;
	uiLanGame.dlgMessage2.generic.type = QMTYPE_ACTION;
	uiLanGame.dlgMessage2.generic.flags = QMF_INACTIVE|QMF_HIDDEN|QMF_DROPSHADOW;
	uiLanGame.dlgMessage2.generic.name = "any current game, OK to exit?";
	uiLanGame.dlgMessage2.generic.x = DLG_X + 248;
	uiLanGame.dlgMessage2.generic.y = 310;

	uiLanGame.yes.generic.id = ID_YES;
	uiLanGame.yes.generic.type = QMTYPE_BM_BUTTON;
	uiLanGame.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_HIDDEN|QMF_DROPSHADOW;
	uiLanGame.yes.generic.name = "Ok";
	uiLanGame.yes.generic.x = DLG_X + 380;
	uiLanGame.yes.generic.y = 460;
	uiLanGame.yes.generic.callback = UI_LanGame_Callback;

	UI_UtilSetupPicButton( &uiLanGame.yes, PC_OK );

	uiLanGame.no.generic.id = ID_NO;
	uiLanGame.no.generic.type = QMTYPE_BM_BUTTON;
	uiLanGame.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_HIDDEN|QMF_DROPSHADOW;
	uiLanGame.no.generic.name = "Cancel";
	uiLanGame.no.generic.x = DLG_X + 530;
	uiLanGame.no.generic.y = 460;
	uiLanGame.no.generic.callback = UI_LanGame_Callback;

	UI_UtilSetupPicButton( &uiLanGame.no, PC_CANCEL );

	uiLanGame.hintMessage.generic.id = ID_TABLEHINT;
	uiLanGame.hintMessage.generic.type = QMTYPE_ACTION;
	uiLanGame.hintMessage.generic.flags = QMF_INACTIVE|QMF_SMALLFONT;
	uiLanGame.hintMessage.generic.color = uiColorHelp;
	uiLanGame.hintMessage.generic.name = uiLanGame.hintText;
	uiLanGame.hintMessage.generic.x = 360;
	uiLanGame.hintMessage.generic.y = 225;

	uiLanGame.gameList.generic.id = ID_SERVERSLIST;
	uiLanGame.gameList.generic.type = QMTYPE_SCROLLLIST;
	uiLanGame.gameList.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_SMALLFONT;
	uiLanGame.gameList.generic.x = 340;
	uiLanGame.gameList.generic.y = 255;
	uiLanGame.gameList.generic.width = 660;
	uiLanGame.gameList.generic.height = 440;
	uiLanGame.gameList.generic.callback = UI_LanGame_Callback;
	uiLanGame.gameList.itemNames = (const char **)uiLanGame.gameDescriptionPtr;

	// server.dll needs for reading savefiles or startup newgame
	if( !CheckGameDll( ))
		uiLanGame.createGame.generic.flags |= QMF_GRAYED;	// server.dll is missed - remote servers only

	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.background );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.banner );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.joinGame );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.createGame );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.gameInfo );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.refresh );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.done );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.hintMessage );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.gameList );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.msgBox );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.dlgMessage1 );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.dlgMessage2 );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.no );
 	UI_AddItem( &uiLanGame.menu, (void *)&uiLanGame.yes );

	uiLanGame.refreshTime = uiStatic.realTime + 500; // delay before update 0.5 sec
}

/*
=================
UI_LanGame_Precache
=================
*/
void UI_LanGame_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_LanGame_Menu
=================
*/
void UI_LanGame_Menu( void )
{
	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		return;

	// stop demos to allow network sockets to open
	if ( gpGlobals->demoplayback && CVAR_GET_FLOAT( "cl_background" ))
	{
		uiStatic.m_iOldMenuDepth = uiStatic.menuDepth;
		CLIENT_COMMAND( FALSE, "stop\n" );
		uiStatic.m_fDemosPlayed = true;
	}

	UI_LanGame_Precache();
	UI_LanGame_Init();

	uiLanGame.refreshTime = uiStatic.realTime + 10000; // refresh every 10 secs
	UI_RefreshServerList();

	UI_PushMenu( &uiLanGame.menu );
}
