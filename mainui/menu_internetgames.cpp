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

#define ART_BANNER		"gfx/shell/head_inetgames"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_JOINGAME		2
#define ID_CREATEGAME	3
#define ID_GAMEINFO		4
#define ID_REFRESH		5
#define ID_DONE		6
#define ID_SERVERSLIST	7
#define ID_TABLEHINT	8
#define ID_NAT			11
#define ID_DIRECT		12

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
	menuPicButton_s	direct;
	menuPicButton_s	nat;

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
	int		refreshTime2;
} uiInternetGames_t;

static uiInternetGames_t	uiInternetGames;

static void UI_PromptDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide quit dialog
	uiInternetGames.joinGame.generic.flags ^= QMF_INACTIVE;
	uiInternetGames.createGame.generic.flags ^= QMF_INACTIVE;
	uiInternetGames.gameInfo.generic.flags ^= QMF_INACTIVE;
	uiInternetGames.refresh.generic.flags ^= QMF_INACTIVE;
	uiInternetGames.done.generic.flags ^= QMF_INACTIVE;
	uiInternetGames.gameList.generic.flags ^= QMF_INACTIVE;
	uiInternetGames.nat.generic.flags ^= QMF_INACTIVE;
	uiInternetGames.direct.generic.flags ^= QMF_INACTIVE;

	uiInternetGames.msgBox.generic.flags ^= QMF_HIDDEN;
	uiInternetGames.dlgMessage1.generic.flags ^= QMF_HIDDEN;
	uiInternetGames.dlgMessage2.generic.flags ^= QMF_HIDDEN;
	uiInternetGames.no.generic.flags ^= QMF_HIDDEN;
	uiInternetGames.yes.generic.flags ^= QMF_HIDDEN;

}

/*
=================
UI_InternetGames_KeyFunc
=================
*/
static const char *UI_InternetGames_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE && !( uiInternetGames.dlgMessage1.generic.flags & QMF_HIDDEN ))
	{
		UI_PromptDialog();
		return uiSoundNull;
	}
	return UI_DefaultKey( &uiInternetGames.menu, key, down );
}

/*
=================
UI_InternetGames_GetGamesList
=================
*/
static void UI_InternetGames_GetGamesList( void )
{
	int		i;
	const char	*info;

	for( i = 0; i < uiStatic.numServers; i++ )
	{
		if( i >= UI_MAX_SERVERS )
			break;
		info = uiStatic.serverNames[i];

		uiInternetGames.gameDescription[i][0] = 0; // mark this string as empty

		StringConcat( uiInternetGames.gameDescription[i], Info_ValueForKey( info, "host" ), QMSB_GAME_LENGTH );
		AddSpaces( uiInternetGames.gameDescription[i], QMSB_GAME_LENGTH );

		StringConcat( uiInternetGames.gameDescription[i], Info_ValueForKey( info, "map" ), QMSB_MAPNAME_LENGTH );
		AddSpaces( uiInternetGames.gameDescription[i], QMSB_MAPNAME_LENGTH );

		StringConcat( uiInternetGames.gameDescription[i], Info_ValueForKey( info, "numcl" ), QMSB_MAXCL_LENGTH );
		StringConcat( uiInternetGames.gameDescription[i], "\\", QMSB_MAXCL_LENGTH );
		StringConcat( uiInternetGames.gameDescription[i], Info_ValueForKey( info, "maxcl" ), QMSB_MAXCL_LENGTH );
		AddSpaces( uiInternetGames.gameDescription[i], QMSB_MAXCL_LENGTH );

		char ping[10];
		snprintf( ping, 10, "%.f ms", uiStatic.serverPings[i] * 1000 );
		StringConcat( uiInternetGames.gameDescription[i], ping, QMSB_PING_LENGTH );
		AddSpaces( uiInternetGames.gameDescription[i], QMSB_PING_LENGTH );

		uiInternetGames.gameDescriptionPtr[i] = uiInternetGames.gameDescription[i];
	}

	for( ; i < UI_MAX_SERVERS; i++ )
		uiInternetGames.gameDescriptionPtr[i] = NULL;

	uiInternetGames.gameList.itemNames = (const char **)uiInternetGames.gameDescriptionPtr;
	uiInternetGames.gameList.numItems = 0; // reset it
	uiInternetGames.gameList.curItem = 0; // reset it

	if( !uiInternetGames.gameList.generic.charHeight )
		return; // to avoid divide integer by zero

	// count number of items
	while( uiInternetGames.gameList.itemNames[uiInternetGames.gameList.numItems] )
		uiInternetGames.gameList.numItems++;

	// calculate number of visible rows
	uiInternetGames.gameList.numRows = (uiInternetGames.gameList.generic.height2 / uiInternetGames.gameList.generic.charHeight) - 2;
	if( uiInternetGames.gameList.numRows > uiInternetGames.gameList.numItems ) uiInternetGames.gameList.numRows = uiInternetGames.gameList.numItems;

	if( uiStatic.numServers )
		uiInternetGames.joinGame.generic.flags &= ~QMF_GRAYED;
}

/*
=================
UI_InternetGames_JoinGame
=================
*/
static void UI_InternetGames_JoinGame( void )
{
	// close dialog
	if( !(uiInternetGames.yes.generic.flags & QMF_HIDDEN ) )
		UI_PromptDialog();

	if( !strlen( uiInternetGames.gameDescription[uiInternetGames.gameList.curItem] ))
		return;

	CLIENT_JOIN( uiStatic.serverAddresses[uiInternetGames.gameList.curItem] );
	// prevent refresh durning connect
	uiInternetGames.refreshTime = uiStatic.realTime + 999999;
}

#define REFRESH_LIST() \
if( uiStatic.realTime > uiInternetGames.refreshTime2 ) \
{ \
UI_RefreshInternetServerList(); \
uiInternetGames.refreshTime2 = uiStatic.realTime + (CVAR_GET_FLOAT("cl_nat")?4000:1000); \
uiInternetGames.refresh.generic.flags |= QMF_GRAYED; \
if( uiStatic.realTime + 20000 < uiInternetGames.refreshTime ) \
	uiInternetGames.refreshTime = uiStatic.realTime + 20000; \
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

	if( !CVAR_GET_FLOAT( "cl_nat" ) && uiStatic.realTime > uiInternetGames.refreshTime )
	{
		REFRESH_LIST();
		uiInternetGames.refreshTime = uiStatic.realTime + 20000;
	}
	if( uiStatic.realTime > uiInternetGames.refreshTime2 )
	{
		uiInternetGames.refresh.generic.flags &= ~QMF_GRAYED;
	}

	// serverinfo has been changed update display
	if( uiStatic.updateServers )
	{
		UI_InternetGames_GetGamesList ();
		uiStatic.updateServers = false;
	}
	UI_FillRect( 780 * uiStatic.scaleX, 184 * uiStatic.scaleY, (1000-780) * uiStatic.scaleX, 38 * uiStatic.scaleY, 0x80000000 );
	UI_FillRect( 777 * uiStatic.scaleX, 180 * uiStatic.scaleY, (1000-780+6) * uiStatic.scaleX, 4 * uiStatic.scaleY, 0xFF555555 );
	UI_FillRect( 777 * uiStatic.scaleX, 184 * uiStatic.scaleY, 4 * uiStatic.scaleX, 38 * uiStatic.scaleY, 0xFF555555 );
	if( !CVAR_GET_FLOAT( "cl_nat") )
	{
		uiInternetGames.nat.generic.flags &= ~QMF_GRAYED;
		uiInternetGames.direct.generic.flags |= QMF_GRAYED;
		UI_FillRect( 780 * uiStatic.scaleX, 184 * uiStatic.scaleY, 120 * uiStatic.scaleX, 38 * uiStatic.scaleY, 0xFF555555 );
	}
	else
	{
		uiInternetGames.direct.generic.flags &= ~QMF_GRAYED;
		uiInternetGames.nat.generic.flags |= QMF_GRAYED;
		UI_FillRect( 900 * uiStatic.scaleX, 184 * uiStatic.scaleY, 102 * uiStatic.scaleX, 38 * uiStatic.scaleY, 0xFF555555 );
	}

	UI_FillRect( 338 * uiStatic.scaleX, 223 * uiStatic.scaleY, 662 * uiStatic.scaleX, 34 * uiStatic.scaleY, 0x80000000 );
	UI_FillRect( 340 * uiStatic.scaleX, 221 * uiStatic.scaleY, 662 * uiStatic.scaleX, 4 * uiStatic.scaleY, 0xFF555555 );
	UI_FillRect( 336 * uiStatic.scaleX, 221 * uiStatic.scaleY, 4 * uiStatic.scaleX, 36 * uiStatic.scaleY, 0xFF555555);
	UI_FillRect( (339+660) * uiStatic.scaleX, 184 * uiStatic.scaleY, 4 * uiStatic.scaleX, 72 * uiStatic.scaleY, 0xFF555555);



	//UI_FillRect( 340, 225, 660, 30, 0x80404040 );

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
UI_InternetGames_Callback
=================
*/
static void UI_InternetGames_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_JOINGAME:
		if( CL_IsActive( ))
			UI_PromptDialog();
		else UI_InternetGames_JoinGame();
		break;
	case ID_CREATEGAME:
		CVAR_SET_FLOAT( "public", 1.0f );
		UI_CreateGame_Menu();
		break;
	case ID_GAMEINFO:
		// UNDONE: not implemented
		break;
	case ID_REFRESH:
		if( uiStatic.realTime > uiInternetGames.refreshTime2 )
		REFRESH_LIST();
		uiInternetGames.refresh.generic.flags |= QMF_GRAYED;
		break;
	case ID_DONE:
		CVAR_SET_FLOAT( "cl_nat", 0 );
		UI_PopMenu();
		break;
	case ID_YES:
		UI_InternetGames_JoinGame();
		break;
	case ID_NO:
		UI_PromptDialog();
		break;
	case ID_DIRECT:
		CVAR_SET_FLOAT( "cl_nat", 0 );
		uiInternetGames.nat.generic.flags &= ~QMF_GRAYED;
		uiInternetGames.direct.generic.flags |= QMF_GRAYED;
		UI_RefreshInternetServerList();
		uiInternetGames.refreshTime2 = uiStatic.realTime + 1000;
		uiStatic.numServers = 0;
		uiInternetGames.gameList.numItems = 0; // reset it
		uiInternetGames.gameList.curItem = 0; // reset it
		uiInternetGames.gameList.numRows = 0;
		for( int i = 0 ; i < UI_MAX_SERVERS; i++ )
			uiInternetGames.gameDescriptionPtr[i] = NULL;
		break;
	case ID_NAT:
		if( uiInternetGames.refreshTime2 > uiStatic.realTime )
		    break;
		CVAR_SET_FLOAT( "cl_nat", 1 );
		uiInternetGames.direct.generic.flags &= ~QMF_GRAYED;
		uiInternetGames.nat.generic.flags |= QMF_GRAYED;
		UI_RefreshInternetServerList();
		uiInternetGames.refreshTime2 = uiStatic.realTime + 4000;
		uiStatic.numServers = 0;
		uiInternetGames.gameList.numItems = 0; // reset it
		uiInternetGames.gameList.curItem = 0; // reset it
		uiInternetGames.gameList.numRows = 0;
		for( int i = 0 ; i < UI_MAX_SERVERS; i++ )
			uiInternetGames.gameDescriptionPtr[i] = NULL;
		break;
	}
}

/*
=================
UI_InternetGames_Init
=================
*/
static void UI_InternetGames_Init( void )
{
	memset( &uiInternetGames, 0, sizeof( uiInternetGames_t ));

	uiInternetGames.menu.vidInitFunc = UI_InternetGames_Init;
	uiInternetGames.menu.keyFunc = UI_InternetGames_KeyFunc;

	StringConcat( uiInternetGames.hintText, "Name", QMSB_GAME_LENGTH );
	AddSpaces( uiInternetGames.hintText, QMSB_GAME_LENGTH );
	StringConcat( uiInternetGames.hintText, "Map", QMSB_MAPNAME_LENGTH );
	AddSpaces( uiInternetGames.hintText, QMSB_MAPNAME_LENGTH );
	StringConcat( uiInternetGames.hintText, "Players", QMSB_MAXCL_LENGTH );
	AddSpaces( uiInternetGames.hintText, QMSB_MAXCL_LENGTH );
	StringConcat( uiInternetGames.hintText, "Ping", QMSB_PING_LENGTH );
	AddSpaces( uiInternetGames.hintText, QMSB_PING_LENGTH );

	uiInternetGames.background.generic.id = ID_BACKGROUND;
	uiInternetGames.background.generic.type = QMTYPE_BITMAP;
	uiInternetGames.background.generic.flags = QMF_INACTIVE;
	uiInternetGames.background.generic.x = 0;
	uiInternetGames.background.generic.y = 0;
	uiInternetGames.background.generic.width = uiStatic.width;
	uiInternetGames.background.generic.height = 768;
	uiInternetGames.background.pic = ART_BACKGROUND;
	uiInternetGames.background.generic.ownerdraw = UI_Background_Ownerdraw;

	uiInternetGames.banner.generic.id = ID_BANNER;
	uiInternetGames.banner.generic.type = QMTYPE_BITMAP;
	uiInternetGames.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiInternetGames.banner.generic.x = UI_BANNER_POSX;
	uiInternetGames.banner.generic.y = UI_BANNER_POSY;
	uiInternetGames.banner.generic.width = UI_BANNER_WIDTH;
	uiInternetGames.banner.generic.height = UI_BANNER_HEIGHT;
	uiInternetGames.banner.pic = ART_BANNER;

	uiInternetGames.joinGame.generic.id = ID_JOINGAME;
	uiInternetGames.joinGame.generic.type = QMTYPE_BM_BUTTON;
	uiInternetGames.joinGame.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_GRAYED;
	uiInternetGames.joinGame.generic.x = 72;
	uiInternetGames.joinGame.generic.y = 230;
	uiInternetGames.joinGame.generic.name = "Join game";
	uiInternetGames.joinGame.generic.statusText = "Join to selected game";
	uiInternetGames.joinGame.generic.callback = UI_InternetGames_Callback;

	UI_UtilSetupPicButton( &uiInternetGames.joinGame, PC_JOIN_GAME );

	uiInternetGames.createGame.generic.id = ID_CREATEGAME;
	uiInternetGames.createGame.generic.type = QMTYPE_BM_BUTTON;
	uiInternetGames.createGame.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiInternetGames.createGame.generic.x = 72;
	uiInternetGames.createGame.generic.y = 280;
	uiInternetGames.createGame.generic.name = "Create game";
	uiInternetGames.createGame.generic.statusText = "Create new Internet game";
	uiInternetGames.createGame.generic.callback = UI_InternetGames_Callback;

	UI_UtilSetupPicButton( &uiInternetGames.createGame, PC_CREATE_GAME );

	uiInternetGames.gameInfo.generic.id = ID_GAMEINFO;
	uiInternetGames.gameInfo.generic.type = QMTYPE_BM_BUTTON;
	uiInternetGames.gameInfo.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_GRAYED;
	uiInternetGames.gameInfo.generic.x = 72;
	uiInternetGames.gameInfo.generic.y = 330;
	uiInternetGames.gameInfo.generic.name = "View game info";
	uiInternetGames.gameInfo.generic.statusText = "Get detail game info";
	uiInternetGames.gameInfo.generic.callback = UI_InternetGames_Callback;

	UI_UtilSetupPicButton( &uiInternetGames.gameInfo, PC_VIEW_GAME_INFO );

	uiInternetGames.refresh.generic.id = ID_REFRESH;
	uiInternetGames.refresh.generic.type = QMTYPE_BM_BUTTON;
	uiInternetGames.refresh.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiInternetGames.refresh.generic.x = 72;
	uiInternetGames.refresh.generic.y = 380;
	uiInternetGames.refresh.generic.name = "Refresh";
	uiInternetGames.refresh.generic.statusText = "Refresh servers list";
	uiInternetGames.refresh.generic.callback = UI_InternetGames_Callback;

	UI_UtilSetupPicButton( &uiInternetGames.refresh, PC_REFRESH );

	uiInternetGames.nat.generic.id = ID_NAT;
	uiInternetGames.nat.generic.type = QMTYPE_BM_BUTTON;
	uiInternetGames.nat.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiInternetGames.nat.generic.x = 920;
	uiInternetGames.nat.generic.y = 190;
	uiInternetGames.nat.generic.name = "NAT";
	uiInternetGames.nat.generic.statusText = "NAT-bypassed servers";
	uiInternetGames.nat.generic.callback = UI_InternetGames_Callback;

	uiInternetGames.direct.generic.id = ID_DIRECT;
	uiInternetGames.direct.generic.type = QMTYPE_BM_BUTTON;
	uiInternetGames.direct.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_GRAYED;
	uiInternetGames.direct.generic.x = 785;
	uiInternetGames.direct.generic.y = 190;
	uiInternetGames.direct.generic.name = "Direct";
	uiInternetGames.direct.generic.statusText = "Main servers";
	uiInternetGames.direct.generic.callback = UI_InternetGames_Callback;

	uiInternetGames.done.generic.id = ID_DONE;
	uiInternetGames.done.generic.type = QMTYPE_BM_BUTTON;
	uiInternetGames.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiInternetGames.done.generic.x = 72;
	uiInternetGames.done.generic.y = 430;
	uiInternetGames.done.generic.name = "Done";
	uiInternetGames.done.generic.statusText = "Return to main menu";
	uiInternetGames.done.generic.callback = UI_InternetGames_Callback;

	UI_UtilSetupPicButton( &uiInternetGames.done, PC_DONE );

	uiInternetGames.msgBox.generic.id = ID_MSGBOX;
	uiInternetGames.msgBox.generic.type = QMTYPE_ACTION;
	uiInternetGames.msgBox.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiInternetGames.msgBox.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiInternetGames.msgBox.generic.x = DLG_X + 192;
	uiInternetGames.msgBox.generic.y = 256;
	uiInternetGames.msgBox.generic.width = 640;
	uiInternetGames.msgBox.generic.height = 256;

	uiInternetGames.dlgMessage1.generic.id = ID_MSGTEXT;
	uiInternetGames.dlgMessage1.generic.type = QMTYPE_ACTION;
	uiInternetGames.dlgMessage1.generic.flags = QMF_INACTIVE|QMF_HIDDEN|QMF_DROPSHADOW;
	uiInternetGames.dlgMessage1.generic.name = "Join a network game will exit";
	uiInternetGames.dlgMessage1.generic.x = DLG_X + 248;
	uiInternetGames.dlgMessage1.generic.y = 280;

	uiInternetGames.dlgMessage2.generic.id = ID_MSGTEXT;
	uiInternetGames.dlgMessage2.generic.type = QMTYPE_ACTION;
	uiInternetGames.dlgMessage2.generic.flags = QMF_INACTIVE|QMF_HIDDEN|QMF_DROPSHADOW;
	uiInternetGames.dlgMessage2.generic.name = "any current game, OK to exit?";
	uiInternetGames.dlgMessage2.generic.x = DLG_X + 248;
	uiInternetGames.dlgMessage2.generic.y = 310;

	uiInternetGames.yes.generic.id = ID_YES;
	uiInternetGames.yes.generic.type = QMTYPE_BM_BUTTON;
	uiInternetGames.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_HIDDEN|QMF_DROPSHADOW;
	uiInternetGames.yes.generic.name = "Ok";
	uiInternetGames.yes.generic.x = DLG_X + 380;
	uiInternetGames.yes.generic.y = 460;
	uiInternetGames.yes.generic.callback = UI_InternetGames_Callback;

	UI_UtilSetupPicButton( &uiInternetGames.yes, PC_OK );

	uiInternetGames.no.generic.id = ID_NO;
	uiInternetGames.no.generic.type = QMTYPE_BM_BUTTON;
	uiInternetGames.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_HIDDEN|QMF_DROPSHADOW;
	uiInternetGames.no.generic.name = "Cancel";
	uiInternetGames.no.generic.x = DLG_X + 530;
	uiInternetGames.no.generic.y = 460;
	uiInternetGames.no.generic.callback = UI_InternetGames_Callback;

	UI_UtilSetupPicButton( &uiInternetGames.no, PC_CANCEL );

	uiInternetGames.hintMessage.generic.id = ID_TABLEHINT;
	uiInternetGames.hintMessage.generic.type = QMTYPE_ACTION;
	uiInternetGames.hintMessage.generic.flags = QMF_INACTIVE|QMF_SMALLFONT;
	uiInternetGames.hintMessage.generic.color = uiColorHelp;
	uiInternetGames.hintMessage.generic.name = uiInternetGames.hintText;
	uiInternetGames.hintMessage.generic.x = 340;
	uiInternetGames.hintMessage.generic.y = 225;

	uiInternetGames.gameList.generic.id = ID_SERVERSLIST;
	uiInternetGames.gameList.generic.type = QMTYPE_SCROLLLIST;
	uiInternetGames.gameList.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_SMALLFONT;
	uiInternetGames.gameList.generic.x = 340;
	uiInternetGames.gameList.generic.y = 255;
	uiInternetGames.gameList.generic.width = 660;
	uiInternetGames.gameList.generic.height = 440;
	uiInternetGames.gameList.generic.callback = UI_InternetGames_Callback;
	uiInternetGames.gameList.itemNames = (const char **)uiInternetGames.gameDescriptionPtr;

	// server.dll needs for reading savefiles or startup newgame
	if( !CheckGameDll( ))
		uiInternetGames.createGame.generic.flags |= QMF_GRAYED;	// server.dll is missed - remote servers only

	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.background );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.banner );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.joinGame );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.createGame );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.gameInfo );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.refresh );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.done );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.direct );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.nat );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.hintMessage );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.gameList );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.msgBox );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.dlgMessage1 );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.dlgMessage2 );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.no );
	UI_AddItem( &uiInternetGames.menu, (void *)&uiInternetGames.yes );

	uiInternetGames.refreshTime = uiStatic.realTime + 500; // delay before update 0.5 sec
}

/*
=================
UI_InternetGames_Precache
=================
*/
void UI_InternetGames_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_InternetGames_Menu
=================
*/
void UI_InternetGames_Menu( void )
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

	UI_InternetGames_Precache();
	UI_InternetGames_Init();

	UI_PushMenu( &uiInternetGames.menu );
}
