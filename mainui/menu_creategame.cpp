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

#define ART_BANNER		"gfx/shell/head_creategame"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_ADVOPTIONS	2
#define ID_DONE		3
#define ID_CANCEL		4
#define ID_MAPLIST		5
#define ID_TABLEHINT	6
#define ID_MAXCLIENTS	7
#define ID_HOSTNAME		8
#define ID_PASSWORD		9
#define ID_HLTV		10
#define ID_DEDICATED	11

#define ID_MSGBOX	 	12
#define ID_MSGTEXT	 	13
#define ID_YES	 	130
#define ID_NO	 	131

#define MAPNAME_LENGTH	20
#define TITLE_LENGTH	20+MAPNAME_LENGTH

typedef struct
{
	char		mapName[UI_MAXGAMES][64];
	char		mapsDescription[UI_MAXGAMES][256];
	char		*mapsDescriptionPtr[UI_MAXGAMES];

	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;
	menuPicButton_s	advOptions;
	menuPicButton_s	done;
	menuPicButton_s	cancel;

	menuField_s	maxClients;
	menuField_s	hostName;
	menuField_s	password;
	menuCheckBox_s	hltv;
	menuCheckBox_s	dedicatedServer;

	// newgame prompt dialog
	menuAction_s	msgBox;
	menuAction_s	dlgMessage1;
	menuAction_s	dlgMessage2;
	menuPicButton_s	yes;
	menuPicButton_s	no;

	menuScrollList_s	mapsList;
	menuAction_s	hintMessage;
	char		hintText[MAX_HINT_TEXT];
} uiCreateGame_t;

static uiCreateGame_t	uiCreateGame;

/*
=================
UI_CreateGame_Begin
=================
*/
static void UI_CreateGame_Begin( void )
{
	if( !MAP_IS_VALID( uiCreateGame.mapName[uiCreateGame.mapsList.curItem] ))
		return;	// bad map

	if( CVAR_GET_FLOAT( "host_serverstate" ) && CVAR_GET_FLOAT( "maxplayers" ) == 1 )
		HOST_ENDGAME( "end of the game" );

	CVAR_SET_FLOAT( "deathmatch", 1.0f );	// start deathmatch as default
	CVAR_SET_FLOAT( "maxplayers", atoi( uiCreateGame.maxClients.buffer ));
	CVAR_SET_STRING( "hostname", uiCreateGame.hostName.buffer );
	CVAR_SET_STRING( "defaultmap", uiCreateGame.mapName[uiCreateGame.mapsList.curItem] );
	CVAR_SET_FLOAT( "hltv", uiCreateGame.hltv.enabled );

	BACKGROUND_TRACK( NULL, NULL );

	// all done, start server
	if( uiCreateGame.dedicatedServer.enabled )
	{
		HOST_WRITECONFIG ( CVAR_GET_STRING( "servercfgfile" ));

		char cmd[128];
		sprintf( cmd, "#%s", gMenu.m_gameinfo.gamefolder );

		// NOTE: dedicated server will be executed "defaultmap"
		// from engine after restarting
		HOST_CHANGEGAME( cmd, "Starting dedicated server...\n" );
	}
	else
	{
		HOST_WRITECONFIG ( CVAR_GET_STRING( "lservercfgfile" ));

		char cmd[128];
		sprintf( cmd, "exec %s\nmap %s\n", CVAR_GET_STRING( "lservercfgfile" ), CVAR_GET_STRING( "defaultmap" ));
	
		CLIENT_COMMAND( FALSE, cmd );
	}
}

static void UI_PromptDialog( void )
{
	if( !CVAR_GET_FLOAT( "host_serverstate" ) || CVAR_GET_FLOAT( "cl_background" ))
	{
		UI_CreateGame_Begin();
		return;
	}
	
	// toggle main menu between active\inactive
	// show\hide quit dialog
	uiCreateGame.advOptions.generic.flags ^= QMF_INACTIVE; 
	uiCreateGame.done.generic.flags ^= QMF_INACTIVE;
	uiCreateGame.cancel.generic.flags ^= QMF_INACTIVE;
	uiCreateGame.maxClients.generic.flags ^= QMF_INACTIVE;
	uiCreateGame.hostName.generic.flags ^= QMF_INACTIVE;
	uiCreateGame.password.generic.flags ^= QMF_INACTIVE;
	uiCreateGame.dedicatedServer.generic.flags ^= QMF_INACTIVE;
	uiCreateGame.hltv.generic.flags ^= QMF_INACTIVE;
	uiCreateGame.mapsList.generic.flags ^= QMF_INACTIVE;

	uiCreateGame.msgBox.generic.flags ^= QMF_HIDDEN;
	uiCreateGame.dlgMessage1.generic.flags ^= QMF_HIDDEN;
	uiCreateGame.dlgMessage2.generic.flags ^= QMF_HIDDEN;
	uiCreateGame.no.generic.flags ^= QMF_HIDDEN;
	uiCreateGame.yes.generic.flags ^= QMF_HIDDEN;

}

/*
=================
UI_CreateGame_KeyFunc
=================
*/
static const char *UI_CreateGame_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE && !( uiCreateGame.dlgMessage1.generic.flags & QMF_HIDDEN ))
	{
		UI_PromptDialog();
		return uiSoundNull;
	}
	return UI_DefaultKey( &uiCreateGame.menu, key, down );
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
UI_CreateGame_GetMapsList
=================
*/
static void UI_CreateGame_GetMapsList( void )
{
	char *afile;

	if( !CHECK_MAP_LIST( FALSE ) || (afile = (char *)LOAD_FILE( "maps.lst", NULL )) == NULL )
	{
		uiCreateGame.done.generic.flags |= QMF_GRAYED;
		uiCreateGame.mapsList.itemNames = (const char **)uiCreateGame.mapsDescriptionPtr;
		Con_Printf( "Cmd_GetMapsList: can't open maps.lst\n" );
		return;
	}

	char *pfile = afile;
	char token[1024];
	int numMaps = 0;
	
	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		if( numMaps >= UI_MAXGAMES ) break;
		StringConcat( uiCreateGame.mapName[numMaps], token, sizeof( uiCreateGame.mapName[0] ));
		StringConcat( uiCreateGame.mapsDescription[numMaps], token, MAPNAME_LENGTH );
		StringConcat( uiCreateGame.mapsDescription[numMaps], uiEmptyString, MAPNAME_LENGTH );
		if(( pfile = COM_ParseFile( pfile, token )) == NULL ) break; // unexpected end of file
		StringConcat( uiCreateGame.mapsDescription[numMaps], token, TITLE_LENGTH );
		StringConcat( uiCreateGame.mapsDescription[numMaps], uiEmptyString, TITLE_LENGTH );
		uiCreateGame.mapsDescriptionPtr[numMaps] = uiCreateGame.mapsDescription[numMaps];
		numMaps++;
	}

	if( !numMaps ) uiCreateGame.done.generic.flags |= QMF_GRAYED;

	for( ; numMaps < UI_MAXGAMES; numMaps++ ) uiCreateGame.mapsDescriptionPtr[numMaps] = NULL;
	uiCreateGame.mapsList.itemNames = (const char **)uiCreateGame.mapsDescriptionPtr;
	FREE_FILE( afile );
}

/*
=================
UI_CreateGame_Callback
=================
*/
static void UI_CreateGame_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	switch( item->id )
	{
	case ID_HLTV:
	case ID_DEDICATED:
		if( event == QM_PRESSED )
			((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_PRESSED;
		else ((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_FOCUS;
		break;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_ADVOPTIONS:
		// UNDONE: not implemented
		break;
	case ID_DONE:
		UI_PromptDialog();
		break;
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_YES:
		UI_CreateGame_Begin();
		break;
	case ID_NO:
		UI_PromptDialog();
		break;
	}
}

/*
=================
UI_CreateGame_Init
=================
*/
static void UI_CreateGame_Init( void )
{
	memset( &uiCreateGame, 0, sizeof( uiCreateGame_t ));

	uiCreateGame.menu.vidInitFunc = UI_CreateGame_Init;
	uiCreateGame.menu.keyFunc = UI_CreateGame_KeyFunc;

	StringConcat( uiCreateGame.hintText, "Map", MAPNAME_LENGTH );
	StringConcat( uiCreateGame.hintText, uiEmptyString, MAPNAME_LENGTH );
	StringConcat( uiCreateGame.hintText, "Title", TITLE_LENGTH );
	StringConcat( uiCreateGame.hintText, uiEmptyString, TITLE_LENGTH );

	uiCreateGame.background.generic.id = ID_BACKGROUND;
	uiCreateGame.background.generic.type = QMTYPE_BITMAP;
	uiCreateGame.background.generic.flags = QMF_INACTIVE;
	uiCreateGame.background.generic.x = 0;
	uiCreateGame.background.generic.y = 0;
	uiCreateGame.background.generic.width = 1024;
	uiCreateGame.background.generic.height = 768;
	uiCreateGame.background.pic = ART_BACKGROUND;

	uiCreateGame.banner.generic.id = ID_BANNER;
	uiCreateGame.banner.generic.type = QMTYPE_BITMAP;
	uiCreateGame.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiCreateGame.banner.generic.x = UI_BANNER_POSX;
	uiCreateGame.banner.generic.y = UI_BANNER_POSY;
	uiCreateGame.banner.generic.width = UI_BANNER_WIDTH;
	uiCreateGame.banner.generic.height = UI_BANNER_HEIGHT;
	uiCreateGame.banner.pic = ART_BANNER;

	uiCreateGame.advOptions.generic.id = ID_ADVOPTIONS;
	uiCreateGame.advOptions.generic.type = QMTYPE_BM_BUTTON;
	uiCreateGame.advOptions.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_GRAYED;
	uiCreateGame.advOptions.generic.x = 72;
	uiCreateGame.advOptions.generic.y = 230;
	uiCreateGame.advOptions.generic.name = "Adv. Options";
	uiCreateGame.advOptions.generic.statusText = "Open the LAN game advanced options menu";
	uiCreateGame.advOptions.generic.callback = UI_CreateGame_Callback;

	UI_UtilSetupPicButton( &uiCreateGame.advOptions, PC_ADV_OPT );

	uiCreateGame.done.generic.id = ID_DONE;
	uiCreateGame.done.generic.type = QMTYPE_BM_BUTTON;
	uiCreateGame.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiCreateGame.done.generic.x = 72;
	uiCreateGame.done.generic.y = 280;
	uiCreateGame.done.generic.name = "Ok";
	uiCreateGame.done.generic.statusText = "Start the multiplayer game";
	uiCreateGame.done.generic.callback = UI_CreateGame_Callback;

	UI_UtilSetupPicButton( &uiCreateGame.done, PC_OK );

	uiCreateGame.cancel.generic.id = ID_CANCEL;
	uiCreateGame.cancel.generic.type = QMTYPE_BM_BUTTON;
	uiCreateGame.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiCreateGame.cancel.generic.x = 72;
	uiCreateGame.cancel.generic.y = 330;
	uiCreateGame.cancel.generic.name = "Cancel";
	uiCreateGame.cancel.generic.statusText = "Return to LAN game menu";
	uiCreateGame.cancel.generic.callback = UI_CreateGame_Callback;

	UI_UtilSetupPicButton( &uiCreateGame.cancel, PC_CANCEL );

	uiCreateGame.dedicatedServer.generic.id = ID_DEDICATED;
	uiCreateGame.dedicatedServer.generic.type = QMTYPE_CHECKBOX;
	uiCreateGame.dedicatedServer.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_MOUSEONLY|QMF_DROPSHADOW;
	uiCreateGame.dedicatedServer.generic.name = "Dedicated server";
	uiCreateGame.dedicatedServer.generic.x = 72;
	uiCreateGame.dedicatedServer.generic.y = 685;
	uiCreateGame.dedicatedServer.generic.callback = UI_CreateGame_Callback;
	uiCreateGame.dedicatedServer.generic.statusText = "faster, but you can't join the server from this machine";

	uiCreateGame.hltv.generic.id = ID_HLTV;
	uiCreateGame.hltv.generic.type = QMTYPE_CHECKBOX;
	uiCreateGame.hltv.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_MOUSEONLY|QMF_DROPSHADOW;
	uiCreateGame.hltv.generic.name = "HLTV";
	uiCreateGame.hltv.generic.x = 72;
	uiCreateGame.hltv.generic.y = 635;
	uiCreateGame.hltv.generic.callback = UI_CreateGame_Callback;
	uiCreateGame.hltv.generic.statusText = "enable hltv mode in multiplayer";

	uiCreateGame.hintMessage.generic.id = ID_TABLEHINT;
	uiCreateGame.hintMessage.generic.type = QMTYPE_ACTION;
	uiCreateGame.hintMessage.generic.flags = QMF_INACTIVE|QMF_SMALLFONT;
	uiCreateGame.hintMessage.generic.color = uiColorHelp;
	uiCreateGame.hintMessage.generic.name = uiCreateGame.hintText;
	uiCreateGame.hintMessage.generic.x = 590;
	uiCreateGame.hintMessage.generic.y = 215;

	uiCreateGame.mapsList.generic.id = ID_MAPLIST;
	uiCreateGame.mapsList.generic.type = QMTYPE_SCROLLLIST;
	uiCreateGame.mapsList.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_SMALLFONT;
	uiCreateGame.mapsList.generic.x = 590;
	uiCreateGame.mapsList.generic.y = 245;
	uiCreateGame.mapsList.generic.width = 410;
	uiCreateGame.mapsList.generic.height = 440;
	uiCreateGame.mapsList.generic.callback = UI_CreateGame_Callback;

	uiCreateGame.hostName.generic.id = ID_HOSTNAME;
	uiCreateGame.hostName.generic.type = QMTYPE_FIELD;
	uiCreateGame.hostName.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiCreateGame.hostName.generic.name = "Server Name:";
	uiCreateGame.hostName.generic.x = 350;
	uiCreateGame.hostName.generic.y = 260;
	uiCreateGame.hostName.generic.width = 205;
	uiCreateGame.hostName.generic.height = 32;
	uiCreateGame.hostName.generic.callback = UI_CreateGame_Callback;
	uiCreateGame.hostName.maxLength = 16;
	strcpy( uiCreateGame.hostName.buffer, CVAR_GET_STRING( "hostname" ));

	uiCreateGame.maxClients.generic.id = ID_MAXCLIENTS;
	uiCreateGame.maxClients.generic.type = QMTYPE_FIELD;
	uiCreateGame.maxClients.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NUMBERSONLY;
	uiCreateGame.maxClients.generic.name = "Max Players:";
	uiCreateGame.maxClients.generic.x = 350;
	uiCreateGame.maxClients.generic.y = 360;
	uiCreateGame.maxClients.generic.width = 205;
	uiCreateGame.maxClients.generic.height = 32;
	uiCreateGame.maxClients.maxLength = 3;

	if( CVAR_GET_FLOAT( "maxplayers" ) <= 1 )
		strcpy( uiCreateGame.maxClients.buffer, "8" );
	else sprintf( uiCreateGame.maxClients.buffer, "%i", (int)CVAR_GET_FLOAT( "maxplayers" ));

	uiCreateGame.password.generic.id = ID_PASSWORD;
	uiCreateGame.password.generic.type = QMTYPE_FIELD;
	uiCreateGame.password.generic.flags = QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDEINPUT;
	uiCreateGame.password.generic.name = "Password:";
	uiCreateGame.password.generic.x = 350;
	uiCreateGame.password.generic.y = 460;
	uiCreateGame.password.generic.width = 205;
	uiCreateGame.password.generic.height = 32;
	uiCreateGame.password.generic.callback = UI_CreateGame_Callback;
	uiCreateGame.password.maxLength = 16;

	uiCreateGame.msgBox.generic.id = ID_MSGBOX;
	uiCreateGame.msgBox.generic.type = QMTYPE_ACTION;
	uiCreateGame.msgBox.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiCreateGame.msgBox.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiCreateGame.msgBox.generic.x = 192;
	uiCreateGame.msgBox.generic.y = 256;
	uiCreateGame.msgBox.generic.width = 640;
	uiCreateGame.msgBox.generic.height = 256;

	uiCreateGame.dlgMessage1.generic.id = ID_MSGTEXT;
	uiCreateGame.dlgMessage1.generic.type = QMTYPE_ACTION;
	uiCreateGame.dlgMessage1.generic.flags = QMF_INACTIVE|QMF_HIDDEN|QMF_DROPSHADOW;
	uiCreateGame.dlgMessage1.generic.name = "Starting a new game will exit";
	uiCreateGame.dlgMessage1.generic.x = 248;
	uiCreateGame.dlgMessage1.generic.y = 280;

	uiCreateGame.dlgMessage2.generic.id = ID_MSGTEXT;
	uiCreateGame.dlgMessage2.generic.type = QMTYPE_ACTION;
	uiCreateGame.dlgMessage2.generic.flags = QMF_INACTIVE|QMF_HIDDEN|QMF_DROPSHADOW;
	uiCreateGame.dlgMessage2.generic.name = "any current game, OK to exit?";
	uiCreateGame.dlgMessage2.generic.x = 248;
	uiCreateGame.dlgMessage2.generic.y = 310;

	uiCreateGame.yes.generic.id = ID_YES;
	uiCreateGame.yes.generic.type = QMTYPE_BM_BUTTON;
	uiCreateGame.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_HIDDEN|QMF_DROPSHADOW;
	uiCreateGame.yes.generic.name = "Ok";
	uiCreateGame.yes.generic.x = 380;
	uiCreateGame.yes.generic.y = 460;
	uiCreateGame.yes.generic.callback = UI_CreateGame_Callback;

	UI_UtilSetupPicButton( &uiCreateGame.yes, PC_OK );

	uiCreateGame.no.generic.id = ID_NO;
	uiCreateGame.no.generic.type = QMTYPE_BM_BUTTON;
	uiCreateGame.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_HIDDEN|QMF_DROPSHADOW;
	uiCreateGame.no.generic.name = "Cancel";
	uiCreateGame.no.generic.x = 530;
	uiCreateGame.no.generic.y = 460;
	uiCreateGame.no.generic.callback = UI_CreateGame_Callback;

	UI_UtilSetupPicButton( &uiCreateGame.no, PC_CANCEL );

	UI_CreateGame_GetMapsList();

	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.background );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.banner );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.advOptions );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.done );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.cancel );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.maxClients );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.hostName );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.password );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.dedicatedServer );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.hltv );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.hintMessage );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.mapsList );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.msgBox );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.dlgMessage1 );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.dlgMessage2 );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.no );
	UI_AddItem( &uiCreateGame.menu, (void *)&uiCreateGame.yes );
}

/*
=================
UI_CreateGame_Precache
=================
*/
void UI_CreateGame_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_CreateGame_Menu
=================
*/
void UI_CreateGame_Menu( void )
{
	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		return;

	UI_CreateGame_Precache();
	UI_CreateGame_Init();

	UI_PushMenu( &uiCreateGame.menu );
}