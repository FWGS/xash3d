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

#define ART_BANNER		"gfx/shell/head_custom"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_ACTIVATE		2
#define ID_DONE		3
#define ID_GOTOSITE		4
#define ID_MODLIST		5
#define ID_TABLEHINT	6
#define ID_MSGBOX	 	7
#define ID_MSGTEXT	 	8
#define ID_YES	 	130
#define ID_NO	 	131

#define MAX_MODS		512	// engine limit

#define TYPE_LENGTH		16
#define NAME_SPACE		4
#define NAME_LENGTH		32+TYPE_LENGTH
#define VER_LENGTH		6+NAME_LENGTH
#define SIZE_LENGTH		10+VER_LENGTH

typedef struct
{
	char		modsDir[MAX_MODS][64];
	char		modsWebSites[MAX_MODS][256];
	char		modsDescription[MAX_MODS][256];
	char		*modsDescriptionPtr[MAX_MODS];

	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;
	menuPicButton_s	load;
	menuPicButton_s	go2url;
	menuPicButton_s	done;

	// prompt dialog
	menuAction_s	msgBox;
	menuAction_s	promptMessage;
	menuPicButton_s	yes;
	menuPicButton_s	no;

	menuScrollList_s	modList;
	menuAction_s	hintMessage;
	char		hintText[MAX_HINT_TEXT];
} uiCustomGame_t;

static uiCustomGame_t	uiCustomGame;

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

static void UI_EndGameDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide delete dialog
	uiCustomGame.load.generic.flags ^= QMF_INACTIVE; 
	uiCustomGame.go2url.generic.flags ^= QMF_INACTIVE;
	uiCustomGame.done.generic.flags ^= QMF_INACTIVE;
	uiCustomGame.modList.generic.flags ^= QMF_INACTIVE;

	uiCustomGame.msgBox.generic.flags ^= QMF_HIDDEN;
	uiCustomGame.promptMessage.generic.flags ^= QMF_HIDDEN;
	uiCustomGame.no.generic.flags ^= QMF_HIDDEN;
	uiCustomGame.yes.generic.flags ^= QMF_HIDDEN;

}

/*
=================
UI_LoadGame_KeyFunc
=================
*/
static const char *UI_CustomGame_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE && uiCustomGame.load.generic.flags & QMF_INACTIVE )
	{
		UI_EndGameDialog();
		return uiSoundNull;
	}
	return UI_DefaultKey( &uiCustomGame.menu, key, down );
}

/*
=================
UI_CustomGame_GetModList
=================
*/
static void UI_CustomGame_GetModList( void )
{
	int	numGames;
	GAMEINFO	**games;

	games = GET_GAMES_LIST( &numGames );

	for( int i = 0; i < numGames; i++ )
	{
		strncpy( uiCustomGame.modsDir[i], games[i]->gamefolder, sizeof( uiCustomGame.modsDir[i] ));
		strncpy( uiCustomGame.modsWebSites[i], games[i]->game_url, sizeof( uiCustomGame.modsWebSites[i] ));

		if( strlen( games[i]->type ))
			StringConcat( uiCustomGame.modsDescription[i], games[i]->type, TYPE_LENGTH );
		StringConcat( uiCustomGame.modsDescription[i], uiEmptyString, TYPE_LENGTH );

		if( ColorStrlen( games[i]->title ) > 31 ) // NAME_LENGTH
		{
			StringConcat( uiCustomGame.modsDescription[i], games[i]->title, ( NAME_LENGTH - NAME_SPACE ));
			StringConcat( uiCustomGame.modsDescription[i], "...", NAME_LENGTH );
		}
		else StringConcat( uiCustomGame.modsDescription[i], games[i]->title, NAME_LENGTH );

		StringConcat( uiCustomGame.modsDescription[i], uiEmptyString, NAME_LENGTH );
		StringConcat( uiCustomGame.modsDescription[i], games[i]->version, VER_LENGTH );
		StringConcat( uiCustomGame.modsDescription[i], uiEmptyString, VER_LENGTH );
		if( strlen( games[i]->size ))
			StringConcat( uiCustomGame.modsDescription[i], games[i]->size, SIZE_LENGTH );
		else StringConcat( uiCustomGame.modsDescription[i], "0.0 Mb", SIZE_LENGTH );     
		StringConcat( uiCustomGame.modsDescription[i], uiEmptyString, SIZE_LENGTH );
		uiCustomGame.modsDescriptionPtr[i] = uiCustomGame.modsDescription[i];

		if( !strcmp( gMenu.m_gameinfo.gamefolder, games[i]->gamefolder ))
			uiCustomGame.modList.curItem = i;
	}

	for( ; i < MAX_MODS; i++ )
		uiCustomGame.modsDescriptionPtr[i] = NULL;

	uiCustomGame.modList.itemNames = (const char **)uiCustomGame.modsDescriptionPtr;

	// see if the load button should be grayed
	if( !stricmp( gMenu.m_gameinfo.gamefolder, uiCustomGame.modsDir[uiCustomGame.modList.curItem] ))
		uiCustomGame.load.generic.flags |= QMF_GRAYED;
	if( strlen( uiCustomGame.modsWebSites[uiCustomGame.modList.curItem] ) == 0 )
		uiCustomGame.go2url.generic.flags |= QMF_GRAYED;
}

/*
=================
UI_CustomGame_Callback
=================
*/
static void UI_CustomGame_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event == QM_CHANGED )
	{
		// aee if the load button should be grayed
		if( !stricmp( gMenu.m_gameinfo.gamefolder, uiCustomGame.modsDir[uiCustomGame.modList.curItem] ))
			uiCustomGame.load.generic.flags |= QMF_GRAYED;
		else uiCustomGame.load.generic.flags &= ~QMF_GRAYED;

		if( strlen( uiCustomGame.modsWebSites[uiCustomGame.modList.curItem] ) == 0 )
			uiCustomGame.go2url.generic.flags |= QMF_GRAYED;
		else uiCustomGame.go2url.generic.flags &= ~QMF_GRAYED;
		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		UI_PopMenu();
		break;
	case ID_GOTOSITE:
		if( strlen( uiCustomGame.modsWebSites[uiCustomGame.modList.curItem] ))
			SHELL_EXECUTE( uiCustomGame.modsWebSites[uiCustomGame.modList.curItem], NULL, false );
		break;
	case ID_ACTIVATE:
	case ID_NO:
		if ( CL_IsActive( ))
		{
			UI_EndGameDialog();
			break;	// don't fuck up the game
		}
	case ID_YES:
		// restart all engine systems with new game
		char cmd[128];
		sprintf( cmd, "game %s\n", uiCustomGame.modsDir[uiCustomGame.modList.curItem] );
		CLIENT_COMMAND( FALSE, cmd );
		UI_EndGameDialog();
		break;
	}
}

/*
=================
UI_CustomGame_Init
=================
*/
static void UI_CustomGame_Init( void )
{
	memset( &uiCustomGame, 0, sizeof( uiCustomGame_t ));

	uiCustomGame.menu.vidInitFunc = UI_CustomGame_Init;
	uiCustomGame.menu.keyFunc = UI_CustomGame_KeyFunc;

	StringConcat( uiCustomGame.hintText, "Type", TYPE_LENGTH );
	StringConcat( uiCustomGame.hintText, uiEmptyString, TYPE_LENGTH );
	StringConcat( uiCustomGame.hintText, "Name", NAME_LENGTH );
	StringConcat( uiCustomGame.hintText, uiEmptyString, NAME_LENGTH );
	StringConcat( uiCustomGame.hintText, "Version", VER_LENGTH );
	StringConcat( uiCustomGame.hintText, uiEmptyString, VER_LENGTH );
	StringConcat( uiCustomGame.hintText, "Size", SIZE_LENGTH );
	StringConcat( uiCustomGame.hintText, uiEmptyString, SIZE_LENGTH );

	uiCustomGame.background.generic.id = ID_BACKGROUND;
	uiCustomGame.background.generic.type = QMTYPE_BITMAP;
	uiCustomGame.background.generic.flags = QMF_INACTIVE;
	uiCustomGame.background.generic.x = 0;
	uiCustomGame.background.generic.y = 0;
	uiCustomGame.background.generic.width = 1024;
	uiCustomGame.background.generic.height = 768;
	uiCustomGame.background.pic = ART_BACKGROUND;

	uiCustomGame.banner.generic.id = ID_BANNER;
	uiCustomGame.banner.generic.type = QMTYPE_BITMAP;
	uiCustomGame.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiCustomGame.banner.generic.x = UI_BANNER_POSX;
	uiCustomGame.banner.generic.y = UI_BANNER_POSY;
	uiCustomGame.banner.generic.width = UI_BANNER_WIDTH;
	uiCustomGame.banner.generic.height = UI_BANNER_HEIGHT;
	uiCustomGame.banner.pic = ART_BANNER;

	uiCustomGame.load.generic.id = ID_ACTIVATE;
	uiCustomGame.load.generic.type = QMTYPE_BM_BUTTON;
	uiCustomGame.load.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiCustomGame.load.generic.x = 72;
	uiCustomGame.load.generic.y = 230;
	uiCustomGame.load.generic.name = "Activate";
	uiCustomGame.load.generic.statusText = "Activate selected custom game";
	uiCustomGame.load.generic.callback = UI_CustomGame_Callback;

	UI_UtilSetupPicButton( &uiCustomGame.load, PC_ACTIVATE );

	uiCustomGame.go2url.generic.id = ID_GOTOSITE;
	uiCustomGame.go2url.generic.type = QMTYPE_BM_BUTTON;
	uiCustomGame.go2url.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiCustomGame.go2url.generic.x = 72;
	uiCustomGame.go2url.generic.y = 280;
	uiCustomGame.go2url.generic.name = "Visit web site";
	uiCustomGame.go2url.generic.statusText = "Visit the web site of game developrs";
	uiCustomGame.go2url.generic.callback = UI_CustomGame_Callback;

	UI_UtilSetupPicButton( &uiCustomGame.go2url, PC_VISIT_WEB_SITE );

	uiCustomGame.done.generic.id = ID_DONE;
	uiCustomGame.done.generic.type = QMTYPE_BM_BUTTON;
	uiCustomGame.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiCustomGame.done.generic.x = 72;
	uiCustomGame.done.generic.y = 330;
	uiCustomGame.done.generic.name = "Done";
	uiCustomGame.done.generic.statusText = "Return to main menu";
	uiCustomGame.done.generic.callback = UI_CustomGame_Callback;

	UI_UtilSetupPicButton( &uiCustomGame.done, PC_DONE );

	uiCustomGame.hintMessage.generic.id = ID_TABLEHINT;
	uiCustomGame.hintMessage.generic.type = QMTYPE_ACTION;
	uiCustomGame.hintMessage.generic.flags = QMF_INACTIVE|QMF_SMALLFONT;
	uiCustomGame.hintMessage.generic.color = uiColorHelp;
	uiCustomGame.hintMessage.generic.name = uiCustomGame.hintText;
	uiCustomGame.hintMessage.generic.x = 360;
	uiCustomGame.hintMessage.generic.y = 225;

	uiCustomGame.modList.generic.id = ID_MODLIST;
	uiCustomGame.modList.generic.type = QMTYPE_SCROLLLIST;
	uiCustomGame.modList.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_SMALLFONT;
	uiCustomGame.modList.generic.x = 360;
	uiCustomGame.modList.generic.y = 255;
	uiCustomGame.modList.generic.width = 640;
	uiCustomGame.modList.generic.height = 440;
	uiCustomGame.modList.generic.callback = UI_CustomGame_Callback;

	uiCustomGame.msgBox.generic.id = ID_MSGBOX;
	uiCustomGame.msgBox.generic.type = QMTYPE_ACTION;
	uiCustomGame.msgBox.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiCustomGame.msgBox.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiCustomGame.msgBox.generic.x = 192;
	uiCustomGame.msgBox.generic.y = 256;
	uiCustomGame.msgBox.generic.width = 640;
	uiCustomGame.msgBox.generic.height = 256;

	uiCustomGame.promptMessage.generic.id = ID_MSGBOX;
	uiCustomGame.promptMessage.generic.type = QMTYPE_ACTION;
	uiCustomGame.promptMessage.generic.flags = QMF_INACTIVE|QMF_DROPSHADOW|QMF_HIDDEN;
	uiCustomGame.promptMessage.generic.name = "Leave current game?";
	uiCustomGame.promptMessage.generic.x = 315;
	uiCustomGame.promptMessage.generic.y = 280;

	uiCustomGame.yes.generic.id = ID_YES;
	uiCustomGame.yes.generic.type = QMTYPE_BM_BUTTON;
	uiCustomGame.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiCustomGame.yes.generic.name = "Ok";
	uiCustomGame.yes.generic.x = 380;
	uiCustomGame.yes.generic.y = 460;
	uiCustomGame.yes.generic.callback = UI_CustomGame_Callback;

	UI_UtilSetupPicButton( &uiCustomGame.yes, PC_OK );

	uiCustomGame.no.generic.id = ID_NO;
	uiCustomGame.no.generic.type = QMTYPE_BM_BUTTON;
	uiCustomGame.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiCustomGame.no.generic.name = "Cancel";
	uiCustomGame.no.generic.x = 530;
	uiCustomGame.no.generic.y = 460;
	uiCustomGame.no.generic.callback = UI_CustomGame_Callback;

	UI_UtilSetupPicButton( &uiCustomGame.no, PC_CANCEL );

	UI_CustomGame_GetModList();

	UI_AddItem( &uiCustomGame.menu, (void *)&uiCustomGame.background );
	UI_AddItem( &uiCustomGame.menu, (void *)&uiCustomGame.banner );
	UI_AddItem( &uiCustomGame.menu, (void *)&uiCustomGame.load );
	UI_AddItem( &uiCustomGame.menu, (void *)&uiCustomGame.go2url );
	UI_AddItem( &uiCustomGame.menu, (void *)&uiCustomGame.done );
	UI_AddItem( &uiCustomGame.menu, (void *)&uiCustomGame.hintMessage );
	UI_AddItem( &uiCustomGame.menu, (void *)&uiCustomGame.modList );
	UI_AddItem( &uiCustomGame.menu, (void *)&uiCustomGame.msgBox );
	UI_AddItem( &uiCustomGame.menu, (void *)&uiCustomGame.promptMessage );
	UI_AddItem( &uiCustomGame.menu, (void *)&uiCustomGame.no );
	UI_AddItem( &uiCustomGame.menu, (void *)&uiCustomGame.yes );
}

/*
=================
UI_CustomGame_Precache
=================
*/
void UI_CustomGame_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_CustomGame_Menu
=================
*/
void UI_CustomGame_Menu( void )
{
	// current instance is not support game change
	if( !CVAR_GET_FLOAT( "host_allow_changegame" ))
		return;
	
	UI_CustomGame_Precache();
	UI_CustomGame_Init();

	UI_PushMenu( &uiCustomGame.menu );
}