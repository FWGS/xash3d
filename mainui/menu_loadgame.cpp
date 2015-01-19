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

#define ART_BANNER	     	"gfx/shell/head_load"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_LOAD		2
#define ID_DELETE		3
#define ID_CANCEL		4
#define ID_SAVELIST		5
#define ID_TABLEHINT	6
#define ID_LEVELSHOT	7
#define ID_MSGBOX	 	8
#define ID_MSGTEXT	 	9
#define ID_YES	 	130
#define ID_NO	 	131

#define LEVELSHOT_X		72
#define LEVELSHOT_Y		400
#define LEVELSHOT_W		192
#define LEVELSHOT_H		160

#define TIME_LENGTH		20
#define NAME_LENGTH		32+TIME_LENGTH
#define GAMETIME_LENGTH	15+NAME_LENGTH

typedef struct
{
	char		saveName[UI_MAXGAMES][CS_SIZE];
	char		delName[UI_MAXGAMES][CS_SIZE];
	char		saveDescription[UI_MAXGAMES][95];
	char		*saveDescriptionPtr[UI_MAXGAMES];

	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;
	menuPicButton_s	load;
	menuPicButton_s	remove;
	menuPicButton_s	cancel;

	menuScrollList_s	savesList;

	menuBitmap_s	levelShot;
	menuAction_s	hintMessage;
	char		hintText[MAX_HINT_TEXT];

	// prompt dialog
	menuAction_s	msgBox;
	menuAction_s	promptMessage;
	menuPicButton_s	yes;
	menuPicButton_s	no;
} uiLoadGame_t;

static uiLoadGame_t		uiLoadGame;

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

static void UI_DeleteDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide remove dialog
	uiLoadGame.load.generic.flags ^= QMF_INACTIVE; 
	uiLoadGame.remove.generic.flags ^= QMF_INACTIVE;
	uiLoadGame.cancel.generic.flags ^= QMF_INACTIVE;
	uiLoadGame.savesList.generic.flags ^= QMF_INACTIVE;

	uiLoadGame.msgBox.generic.flags ^= QMF_HIDDEN;
	uiLoadGame.promptMessage.generic.flags ^= QMF_HIDDEN;
	uiLoadGame.no.generic.flags ^= QMF_HIDDEN;
	uiLoadGame.yes.generic.flags ^= QMF_HIDDEN;
}

/*
=================
UI_LoadGame_KeyFunc
=================
*/
static const char *UI_LoadGame_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE && uiLoadGame.load.generic.flags & QMF_INACTIVE )
	{
		UI_DeleteDialog();
		return uiSoundNull;
	}
	return UI_DefaultKey( &uiLoadGame.menu, key, down );
}

/*
=================
UI_LoadGame_GetGameList
=================
*/
static void UI_LoadGame_GetGameList( void )
{
	char	comment[256];
	char	**filenames;
	int	i, numFiles;

	filenames = FS_SEARCH( "save/*.sav", &numFiles, TRUE );

	// sort the saves in reverse order (oldest past at the end)
	qsort( filenames, numFiles, sizeof( char* ), (cmpfunc)COM_CompareSaves );

	for ( i = 0; i < numFiles; i++ )
	{
		if( i >= UI_MAXGAMES ) break;
		
		if( !GET_SAVE_COMMENT( filenames[i], comment ))
		{
			if( strlen( comment ))
			{
				// get name string even if not found - SV_GetComment can be mark saves
				// as <CORRUPTED> <OLD VERSION> etc
				StringConcat( uiLoadGame.saveDescription[i], uiEmptyString, TIME_LENGTH );
				StringConcat( uiLoadGame.saveDescription[i], comment, NAME_LENGTH );
				StringConcat( uiLoadGame.saveDescription[i], uiEmptyString, NAME_LENGTH );
				uiLoadGame.saveDescriptionPtr[i] = uiLoadGame.saveDescription[i];
				COM_FileBase( filenames[i], uiLoadGame.delName[i] );
			}
			else uiLoadGame.saveDescriptionPtr[i] = NULL;
			continue;
		}

		// strip path, leave only filename (empty slots doesn't have savename)
		COM_FileBase( filenames[i], uiLoadGame.saveName[i] );
		COM_FileBase( filenames[i], uiLoadGame.delName[i] );

		// fill save desc
		StringConcat( uiLoadGame.saveDescription[i], comment + CS_SIZE, TIME_LENGTH );
		StringConcat( uiLoadGame.saveDescription[i], " ", TIME_LENGTH );
		StringConcat( uiLoadGame.saveDescription[i], comment + CS_SIZE + CS_TIME, TIME_LENGTH );
		StringConcat( uiLoadGame.saveDescription[i], uiEmptyString, TIME_LENGTH ); // fill remaining entries
		StringConcat( uiLoadGame.saveDescription[i], comment, NAME_LENGTH );
		StringConcat( uiLoadGame.saveDescription[i], uiEmptyString, NAME_LENGTH );
		StringConcat( uiLoadGame.saveDescription[i], comment + CS_SIZE + (CS_TIME * 2), GAMETIME_LENGTH );
		StringConcat( uiLoadGame.saveDescription[i], uiEmptyString, GAMETIME_LENGTH );
		uiLoadGame.saveDescriptionPtr[i] = uiLoadGame.saveDescription[i];
	}

	for ( ; i < UI_MAXGAMES; i++ )
		uiLoadGame.saveDescriptionPtr[i] = NULL;

	uiLoadGame.savesList.itemNames = (const char **)uiLoadGame.saveDescriptionPtr;

	if ( strlen( uiLoadGame.saveName[0] ) == 0 )
		uiLoadGame.load.generic.flags |= QMF_GRAYED;
	else uiLoadGame.load.generic.flags &= ~QMF_GRAYED;

	if ( strlen( uiLoadGame.delName[0] ) == 0 )
		uiLoadGame.remove.generic.flags |= QMF_GRAYED;
	else uiLoadGame.remove.generic.flags &= ~QMF_GRAYED;
}

/*
=================
UI_LoadGame_Callback
=================
*/
static void UI_LoadGame_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event == QM_CHANGED )
	{
		if( strlen( uiLoadGame.saveName[uiLoadGame.savesList.curItem] ) == 0 )
			uiLoadGame.load.generic.flags |= QMF_GRAYED;
		else uiLoadGame.load.generic.flags &= ~QMF_GRAYED;

		if( strlen( uiLoadGame.delName[uiLoadGame.savesList.curItem] ) == 0 )
			uiLoadGame.remove.generic.flags |= QMF_GRAYED;
		else uiLoadGame.remove.generic.flags &= ~QMF_GRAYED;
		return;
	}

	if( event != QM_ACTIVATED )
		return;
	
	switch( item->id )
	{
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_LOAD:
		if( strlen( uiLoadGame.saveName[uiLoadGame.savesList.curItem] ))
		{
			char	cmd[128];
			sprintf( cmd, "load \"%s\"\n", uiLoadGame.saveName[uiLoadGame.savesList.curItem] );

			BACKGROUND_TRACK( NULL, NULL );

			CLIENT_COMMAND( FALSE, cmd );
		}
		break;
	case ID_NO:
	case ID_DELETE:
		UI_DeleteDialog();
		break;
	case ID_YES:
		if( strlen( uiLoadGame.delName[uiLoadGame.savesList.curItem] ))
		{
			char	cmd[128];
			sprintf( cmd, "killsave \"%s\"\n", uiLoadGame.delName[uiLoadGame.savesList.curItem] );

			CLIENT_COMMAND( TRUE, cmd );

			sprintf( cmd, "save/%s.bmp", uiLoadGame.delName[uiLoadGame.savesList.curItem] );
			PIC_Free( cmd );

			// restarts the menu
			UI_PopMenu();
			UI_LoadGame_Menu();
			return;
		}
		UI_DeleteDialog();
		break;
	}
}

/*
=================
UI_LoadGame_Ownerdraw
=================
*/
static void UI_LoadGame_Ownerdraw( void *self )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( item->type != QMTYPE_ACTION && item->id == ID_LEVELSHOT )
	{
		int	x, y, w, h;

		// draw the levelshot
		x = LEVELSHOT_X;
		y = LEVELSHOT_Y;
		w = LEVELSHOT_W;
		h = LEVELSHOT_H;
		
		UI_ScaleCoords( &x, &y, &w, &h );

		if( strlen( uiLoadGame.saveName[uiLoadGame.savesList.curItem] ))
		{
			char	saveshot[128];

			sprintf( saveshot, "save/%s.bmp", uiLoadGame.saveName[uiLoadGame.savesList.curItem] );

			if( !FILE_EXISTS( saveshot ))
				UI_DrawPicAdditive( x, y, w, h, uiColorWhite, "{GRAF001" );
			else UI_DrawPic( x, y, w, h, uiColorWhite, saveshot );
		}
		else UI_DrawPicAdditive( x, y, w, h, uiColorWhite, "{GRAF001" );

		// draw the rectangle
		UI_DrawRectangle( item->x, item->y, item->width, item->height, uiInputFgColor );
	}
}

/*
=================
UI_LoadGame_Init
=================
*/
static void UI_LoadGame_Init( void )
{
	memset( &uiLoadGame, 0, sizeof( uiLoadGame_t ));

	uiLoadGame.menu.vidInitFunc = UI_LoadGame_Init;
	uiLoadGame.menu.keyFunc = UI_LoadGame_KeyFunc;

	StringConcat( uiLoadGame.hintText, "Time", TIME_LENGTH );
	StringConcat( uiLoadGame.hintText, uiEmptyString, TIME_LENGTH );
	StringConcat( uiLoadGame.hintText, "Game", NAME_LENGTH );
	StringConcat( uiLoadGame.hintText, uiEmptyString, NAME_LENGTH );
	StringConcat( uiLoadGame.hintText, "Elapsed time", GAMETIME_LENGTH );
	StringConcat( uiLoadGame.hintText, uiEmptyString, GAMETIME_LENGTH );

	uiLoadGame.background.generic.id = ID_BACKGROUND;
	uiLoadGame.background.generic.type = QMTYPE_BITMAP;
	uiLoadGame.background.generic.flags = QMF_INACTIVE;
	uiLoadGame.background.generic.x = 0;
	uiLoadGame.background.generic.y = 0;
	uiLoadGame.background.generic.width = 1024;
	uiLoadGame.background.generic.height = 768;
	uiLoadGame.background.pic = ART_BACKGROUND;

	uiLoadGame.banner.generic.id = ID_BANNER;
	uiLoadGame.banner.generic.type = QMTYPE_BITMAP;
	uiLoadGame.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiLoadGame.banner.generic.x = UI_BANNER_POSX;
	uiLoadGame.banner.generic.y = UI_BANNER_POSY;
	uiLoadGame.banner.generic.width = UI_BANNER_WIDTH;
	uiLoadGame.banner.generic.height = UI_BANNER_HEIGHT;
	uiLoadGame.banner.pic = ART_BANNER;

	uiLoadGame.load.generic.id = ID_LOAD;
	uiLoadGame.load.generic.type = QMTYPE_BM_BUTTON;
	uiLoadGame.load.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiLoadGame.load.generic.x = 72;
	uiLoadGame.load.generic.y = 230;
	uiLoadGame.load.generic.name = "Load";
	uiLoadGame.load.generic.statusText = "Load saved game";
	uiLoadGame.load.generic.callback = UI_LoadGame_Callback;

	UI_UtilSetupPicButton( &uiLoadGame.load, PC_LOAD_GAME );

	uiLoadGame.remove.generic.id = ID_DELETE;
	uiLoadGame.remove.generic.type = QMTYPE_BM_BUTTON;
	uiLoadGame.remove.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiLoadGame.remove.generic.x = 72;
	uiLoadGame.remove.generic.y = 280;
	uiLoadGame.remove.generic.name = "Delete";
	uiLoadGame.remove.generic.statusText = "Delete saved game";
	uiLoadGame.remove.generic.callback = UI_LoadGame_Callback;

	UI_UtilSetupPicButton( &uiLoadGame.remove, PC_DELETE );

	uiLoadGame.cancel.generic.id = ID_CANCEL;
	uiLoadGame.cancel.generic.type = QMTYPE_BM_BUTTON;
	uiLoadGame.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiLoadGame.cancel.generic.x = 72;
	uiLoadGame.cancel.generic.y = 330;
	uiLoadGame.cancel.generic.name = "Cancel";
	uiLoadGame.cancel.generic.statusText = "Return back to main menu";
	uiLoadGame.cancel.generic.callback = UI_LoadGame_Callback;

	UI_UtilSetupPicButton( &uiLoadGame.cancel, PC_CANCEL );

	uiLoadGame.hintMessage.generic.id = ID_TABLEHINT;
	uiLoadGame.hintMessage.generic.type = QMTYPE_ACTION;
	uiLoadGame.hintMessage.generic.flags = QMF_INACTIVE|QMF_SMALLFONT;
	uiLoadGame.hintMessage.generic.color = uiColorHelp;
	uiLoadGame.hintMessage.generic.name = uiLoadGame.hintText;
	uiLoadGame.hintMessage.generic.x = 360;
	uiLoadGame.hintMessage.generic.y = 225;

	uiLoadGame.levelShot.generic.id = ID_LEVELSHOT;
	uiLoadGame.levelShot.generic.type = QMTYPE_BITMAP;
	uiLoadGame.levelShot.generic.flags = QMF_INACTIVE;
	uiLoadGame.levelShot.generic.x = LEVELSHOT_X;
	uiLoadGame.levelShot.generic.y = LEVELSHOT_Y;
	uiLoadGame.levelShot.generic.width = LEVELSHOT_W;
	uiLoadGame.levelShot.generic.height = LEVELSHOT_H;
	uiLoadGame.levelShot.generic.ownerdraw = UI_LoadGame_Ownerdraw;

	uiLoadGame.savesList.generic.id = ID_SAVELIST;
	uiLoadGame.savesList.generic.type = QMTYPE_SCROLLLIST;
	uiLoadGame.savesList.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_SMALLFONT;
	uiLoadGame.savesList.generic.x = 360;
	uiLoadGame.savesList.generic.y = 255;
	uiLoadGame.savesList.generic.width = 640;
	uiLoadGame.savesList.generic.height = 440;
	uiLoadGame.savesList.generic.callback = UI_LoadGame_Callback;

	uiLoadGame.msgBox.generic.id = ID_MSGBOX;
	uiLoadGame.msgBox.generic.type = QMTYPE_ACTION;
	uiLoadGame.msgBox.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiLoadGame.msgBox.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiLoadGame.msgBox.generic.x = 192;
	uiLoadGame.msgBox.generic.y = 256;
	uiLoadGame.msgBox.generic.width = 640;
	uiLoadGame.msgBox.generic.height = 256;

	uiLoadGame.promptMessage.generic.id = ID_MSGBOX;
	uiLoadGame.promptMessage.generic.type = QMTYPE_ACTION;
	uiLoadGame.promptMessage.generic.flags = QMF_INACTIVE|QMF_DROPSHADOW|QMF_HIDDEN;
	uiLoadGame.promptMessage.generic.name = "Delete selected game?";
	uiLoadGame.promptMessage.generic.x = 315;
	uiLoadGame.promptMessage.generic.y = 280;

	uiLoadGame.yes.generic.id = ID_YES;
	uiLoadGame.yes.generic.type = QMTYPE_BM_BUTTON;
	uiLoadGame.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiLoadGame.yes.generic.name = "Ok";
	uiLoadGame.yes.generic.x = 380;
	uiLoadGame.yes.generic.y = 460;
	uiLoadGame.yes.generic.callback = UI_LoadGame_Callback;

	UI_UtilSetupPicButton( &uiLoadGame.yes, PC_OK );

	uiLoadGame.no.generic.id = ID_NO;
	uiLoadGame.no.generic.type = QMTYPE_BM_BUTTON;
	uiLoadGame.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiLoadGame.no.generic.name = "Cancel";
	uiLoadGame.no.generic.x = 530;
	uiLoadGame.no.generic.y = 460;
	uiLoadGame.no.generic.callback = UI_LoadGame_Callback;

	UI_UtilSetupPicButton( &uiLoadGame.no, PC_CANCEL );

	UI_LoadGame_GetGameList();

	UI_AddItem( &uiLoadGame.menu, (void *)&uiLoadGame.background );
	UI_AddItem( &uiLoadGame.menu, (void *)&uiLoadGame.banner );
	UI_AddItem( &uiLoadGame.menu, (void *)&uiLoadGame.load );
	UI_AddItem( &uiLoadGame.menu, (void *)&uiLoadGame.remove );
	UI_AddItem( &uiLoadGame.menu, (void *)&uiLoadGame.cancel );
	UI_AddItem( &uiLoadGame.menu, (void *)&uiLoadGame.hintMessage );
	UI_AddItem( &uiLoadGame.menu, (void *)&uiLoadGame.levelShot );
	UI_AddItem( &uiLoadGame.menu, (void *)&uiLoadGame.savesList );
	UI_AddItem( &uiLoadGame.menu, (void *)&uiLoadGame.msgBox );
	UI_AddItem( &uiLoadGame.menu, (void *)&uiLoadGame.promptMessage );
	UI_AddItem( &uiLoadGame.menu, (void *)&uiLoadGame.no );
	UI_AddItem( &uiLoadGame.menu, (void *)&uiLoadGame.yes );
}

/*
=================
UI_LoadGame_Precache
=================
*/
void UI_LoadGame_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_LoadGame_Menu
=================
*/
void UI_LoadGame_Menu( void )
{
	if( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	{
		// completely ignore save\load menus for multiplayer_only
		return;
	}

	if( !CheckGameDll( )) return;

	UI_LoadGame_Precache();
	UI_LoadGame_Init();

	UI_PushMenu( &uiLoadGame.menu );
}