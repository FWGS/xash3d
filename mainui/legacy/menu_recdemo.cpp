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

#define ART_BANNER	     	"gfx/shell/head_save"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_RECORD		2
#define ID_DELETE		3
#define ID_CANCEL		4
#define ID_DEMOLIST		5
#define ID_TABLEHINT	6
#define ID_LEVELSHOT	7
#define ID_MSGBOX	 	8
#define ID_MSGTEXT	 	9
#define ID_YES	 	10
#define ID_NO	 	11

#define LEVELSHOT_X		72
#define LEVELSHOT_Y		400
#define LEVELSHOT_W		192
#define LEVELSHOT_H		160

#define TITLE_LENGTH	32
#define MAPNAME_LENGTH	24+TITLE_LENGTH
#define MAXCLIENTS_LENGTH	16+MAPNAME_LENGTH

typedef struct
{
	char		demoName[UI_MAXGAMES][CS_SIZE];
	char		delName[UI_MAXGAMES][CS_SIZE];
	char		demoDescription[UI_MAXGAMES][256];
	char		*demoDescriptionPtr[UI_MAXGAMES];

	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;
	menuAction_s	record;
	menuAction_s	remove;
	menuAction_s	cancel;

	menuScrollList_s	demosList;

	menuBitmap_s	levelShot;
	menuAction_s	hintMessage;
	char		hintText[MAX_HINT_TEXT];

	// prompt dialog
	menuAction_s	msgBox;
	menuAction_s	promptMessage;
	menuAction_s	yes;
	menuAction_s	no;
} uiRecDemo_t;

static uiRecDemo_t		uiRecDemo;

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
	uiRecDemo.record.generic.flags ^= QMF_INACTIVE; 
	uiRecDemo.remove.generic.flags ^= QMF_INACTIVE;
	uiRecDemo.cancel.generic.flags ^= QMF_INACTIVE;
	uiRecDemo.demosList.generic.flags ^= QMF_INACTIVE;

	uiRecDemo.msgBox.generic.flags ^= QMF_HIDDEN;
	uiRecDemo.promptMessage.generic.flags ^= QMF_HIDDEN;
	uiRecDemo.no.generic.flags ^= QMF_HIDDEN;
	uiRecDemo.yes.generic.flags ^= QMF_HIDDEN;

}

/*
=================
UI_RecDemo_KeyFunc
=================
*/
static const char *UI_RecDemo_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE && uiRecDemo.record.generic.flags & QMF_INACTIVE )
	{
		UI_DeleteDialog();
		return uiSoundNull;
	}
	return UI_DefaultKey( &uiRecDemo.menu, key, down );
}

/*
=================
UI_RecDemo_GetDemoList
=================
*/
static void UI_RecDemo_GetDemoList( void )
{
	char	comment[256];
	char	**filenames;
	int	i = 0, j, numFiles;

	filenames = FS_SEARCH( "demos/*.dem", &numFiles, TRUE );

	if ( CL_IsActive () && !gpGlobals->demorecording && !gpGlobals->demoplayback )
	{
		char maxClients[32];
		sprintf( maxClients, "%i", gpGlobals->maxClients );

		// create new entry for current save game
		strncpy( uiRecDemo.demoName[i], "new", CS_SIZE );
		StringConcat( uiRecDemo.demoDescription[i], gpGlobals->maptitle, TITLE_LENGTH );
		StringConcat( uiRecDemo.demoDescription[i], uiEmptyString, TITLE_LENGTH ); // fill remaining entries
		StringConcat( uiRecDemo.demoDescription[i], "New Demo", MAPNAME_LENGTH );
		StringConcat( uiRecDemo.demoDescription[i], uiEmptyString, MAPNAME_LENGTH );
		StringConcat( uiRecDemo.demoDescription[i], maxClients, MAXCLIENTS_LENGTH );
		StringConcat( uiRecDemo.demoDescription[i], uiEmptyString, MAXCLIENTS_LENGTH );
		uiRecDemo.demoDescriptionPtr[i] = uiRecDemo.demoDescription[i];
		i++;
	}

	for( j = 0; j < numFiles; i++, j++ )
	{
		if( i >= UI_MAXGAMES ) break;
		
		if( !GET_DEMO_COMMENT( filenames[j], comment ))
		{
			if( strlen( comment ))
			{
				// get name string even if not found - C:_GetComment can be mark demos
				// as <CORRUPTED> <OLD VERSION> etc
				// get name string even if not found - SV_GetComment can be mark saves
				// as <CORRUPTED> <OLD VERSION> etc
				StringConcat( uiRecDemo.demoDescription[i], uiEmptyString, TITLE_LENGTH );
				StringConcat( uiRecDemo.demoDescription[i], comment, MAPNAME_LENGTH );
				StringConcat( uiRecDemo.demoDescription[i], uiEmptyString, MAXCLIENTS_LENGTH );
				uiRecDemo.demoDescriptionPtr[i] = uiRecDemo.demoDescription[i];
				COM_FileBase( filenames[j], uiRecDemo.demoName[i] );
				COM_FileBase( filenames[j], uiRecDemo.delName[i] );
			}
			else uiRecDemo.demoDescriptionPtr[i] = NULL;
			continue;
		}

		// strip path, leave only filename (empty slots doesn't have demoname)
		COM_FileBase( filenames[j], uiRecDemo.demoName[i] );
		COM_FileBase( filenames[j], uiRecDemo.delName[i] );

		// fill demo desc
		StringConcat( uiRecDemo.demoDescription[i], comment + CS_SIZE, TITLE_LENGTH );
		StringConcat( uiRecDemo.demoDescription[i], uiEmptyString, TITLE_LENGTH );
		StringConcat( uiRecDemo.demoDescription[i], comment, MAPNAME_LENGTH );
		StringConcat( uiRecDemo.demoDescription[i], uiEmptyString, MAPNAME_LENGTH ); // fill remaining entries
		StringConcat( uiRecDemo.demoDescription[i], comment + CS_SIZE * 2, MAXCLIENTS_LENGTH );
		StringConcat( uiRecDemo.demoDescription[i], uiEmptyString, MAXCLIENTS_LENGTH );
		uiRecDemo.demoDescriptionPtr[i] = uiRecDemo.demoDescription[i];
	}

	for ( ; i < UI_MAXGAMES; i++ )
		uiRecDemo.demoDescriptionPtr[i] = NULL;
	uiRecDemo.demosList.itemNames = (const char **)uiRecDemo.demoDescriptionPtr;

	if( strlen( uiRecDemo.demoName[0] ) == 0 || !CL_IsActive () || gpGlobals->demoplayback )
		uiRecDemo.record.generic.flags |= QMF_GRAYED;
	else uiRecDemo.record.generic.flags &= ~QMF_GRAYED;

	if( strlen( uiRecDemo.delName[0] ) == 0 || !stricmp( gpGlobals->demoname, uiRecDemo.delName[uiRecDemo.demosList.curItem] ))
		uiRecDemo.remove.generic.flags |= QMF_GRAYED;
	else uiRecDemo.remove.generic.flags &= ~QMF_GRAYED;
}

/*
=================
UI_RecDemo_Callback
=================
*/
static void UI_RecDemo_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event == QM_CHANGED )
	{
		// never overwrite existing saves, because their names was never get collision
		if( strlen( uiRecDemo.demoName[uiRecDemo.demosList.curItem] ) == 0 || !CL_IsActive() || gpGlobals->demoplayback )
			uiRecDemo.record.generic.flags |= QMF_GRAYED;
		else uiRecDemo.record.generic.flags &= ~QMF_GRAYED;

		if( strlen( uiRecDemo.delName[uiRecDemo.demosList.curItem] ) == 0 || !stricmp( gpGlobals->demoname, uiRecDemo.delName[uiRecDemo.demosList.curItem] ))
			uiRecDemo.remove.generic.flags |= QMF_GRAYED;
		else uiRecDemo.remove.generic.flags &= ~QMF_GRAYED;
		return;
	}

	if( event != QM_ACTIVATED )
		return;
	
	switch( item->id )
	{
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_RECORD:
		if( gpGlobals->demorecording )
		{
			CLIENT_COMMAND( FALSE, "stop" );
			uiRecDemo.record.generic.name = "Record";
			uiRecDemo.record.generic.statusText = "Record a new demo";
			uiRecDemo.remove.generic.flags &= ~QMF_GRAYED;
		}
		else if( strlen( uiRecDemo.demoName[uiRecDemo.demosList.curItem] ))
		{
			char	cmd[128];

			sprintf( cmd, "demos/%s.bmp", uiRecDemo.demoName[uiRecDemo.demosList.curItem] );
			PIC_Free( cmd );

			sprintf( cmd, "record \"%s\"\n", uiRecDemo.demoName[uiRecDemo.demosList.curItem] );
			CLIENT_COMMAND( FALSE, cmd );
			UI_CloseMenu();
		}
		break;
	case ID_NO:
	case ID_DELETE:
		UI_DeleteDialog();
		break;
	case ID_YES:
		if( strlen( uiRecDemo.delName[uiRecDemo.demosList.curItem] ))
		{
			char	cmd[128];
			sprintf( cmd, "killdemo \"%s\"\n", uiRecDemo.delName[uiRecDemo.demosList.curItem] );

			CLIENT_COMMAND( TRUE, cmd );

			sprintf( cmd, "demos/%s.bmp", uiRecDemo.delName[uiRecDemo.demosList.curItem] );
			PIC_Free( cmd );

			// restarts the menu
			UI_PopMenu();
			UI_RecDemo_Menu();
			return;
		}
		UI_DeleteDialog();
		break;
	}
}

/*
=================
UI_RecDemo_Ownerdraw
=================
*/
static void UI_RecDemo_Ownerdraw( void *self )
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

		if( strlen( uiRecDemo.demoName[uiRecDemo.demosList.curItem] ))
		{
			char	demoshot[128];

			sprintf( demoshot, "demos/%s.bmp", uiRecDemo.demoName[uiRecDemo.demosList.curItem] );

			if( !FILE_EXISTS( demoshot ))
				UI_DrawPicAdditive( x, y, w, h, uiColorWhite, "{GRAF001" );
			else UI_DrawPic( x, y, w, h, uiColorWhite, demoshot );
		}
		else UI_DrawPicAdditive( x, y, w, h, uiColorWhite, "{GRAF001" );

		// draw the rectangle
		UI_DrawRectangle( item->x, item->y, item->width, item->height, uiInputFgColor );
	}
}

/*
=================
UI_RecDemo_Init
=================
*/
static void UI_RecDemo_Init( void )
{
	memset( &uiRecDemo, 0, sizeof( uiRecDemo_t ));

	uiRecDemo.menu.vidInitFunc = UI_RecDemo_Init;
	uiRecDemo.menu.keyFunc = UI_RecDemo_KeyFunc;

	StringConcat( uiRecDemo.hintText, "Title", TITLE_LENGTH );
	StringConcat( uiRecDemo.hintText, uiEmptyString, TITLE_LENGTH );
	StringConcat( uiRecDemo.hintText, "Map", MAPNAME_LENGTH );
	StringConcat( uiRecDemo.hintText, uiEmptyString, MAPNAME_LENGTH );
	StringConcat( uiRecDemo.hintText, "Max Clients", MAXCLIENTS_LENGTH );
	StringConcat( uiRecDemo.hintText, uiEmptyString, MAXCLIENTS_LENGTH );

	uiRecDemo.background.generic.id = ID_BACKGROUND;
	uiRecDemo.background.generic.type = QMTYPE_BITMAP;
	uiRecDemo.background.generic.flags = QMF_INACTIVE;
	uiRecDemo.background.generic.x = 0;
	uiRecDemo.background.generic.y = 0;
	uiRecDemo.background.generic.width = 1024;
	uiRecDemo.background.generic.height = 768;
	uiRecDemo.background.pic = ART_BACKGROUND;

	uiRecDemo.banner.generic.id = ID_BANNER;
	uiRecDemo.banner.generic.type = QMTYPE_BITMAP;
	uiRecDemo.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiRecDemo.banner.generic.x = UI_BANNER_POSX;
	uiRecDemo.banner.generic.y = UI_BANNER_POSY;
	uiRecDemo.banner.generic.width = UI_BANNER_WIDTH;
	uiRecDemo.banner.generic.height = UI_BANNER_HEIGHT;
	uiRecDemo.banner.pic = ART_BANNER;

	uiRecDemo.record.generic.id = ID_RECORD;
	uiRecDemo.record.generic.type = QMTYPE_ACTION;
	uiRecDemo.record.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiRecDemo.record.generic.x = 72;
	uiRecDemo.record.generic.y = 230;

	if( gpGlobals->demorecording )
	{
		uiRecDemo.record.generic.name = "Stop";
		uiRecDemo.record.generic.statusText = "Stop a demo recording";
	}
	else
	{
		uiRecDemo.record.generic.name = "Record";
		uiRecDemo.record.generic.statusText = "Record a new demo";
	}
	uiRecDemo.record.generic.callback = UI_RecDemo_Callback;

	uiRecDemo.remove.generic.id = ID_DELETE;
	uiRecDemo.remove.generic.type = QMTYPE_ACTION;
	uiRecDemo.remove.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiRecDemo.remove.generic.x = 72;
	uiRecDemo.remove.generic.y = 280;
	uiRecDemo.remove.generic.name = "Delete";
	uiRecDemo.remove.generic.statusText = "Delete a demo";
	uiRecDemo.remove.generic.callback = UI_RecDemo_Callback;

	uiRecDemo.cancel.generic.id = ID_CANCEL;
	uiRecDemo.cancel.generic.type = QMTYPE_ACTION;
	uiRecDemo.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiRecDemo.cancel.generic.x = 72;
	uiRecDemo.cancel.generic.y = 330;
	uiRecDemo.cancel.generic.name = "Cancel";
	uiRecDemo.cancel.generic.statusText = "Return back to main menu";
	uiRecDemo.cancel.generic.callback = UI_RecDemo_Callback;

	uiRecDemo.hintMessage.generic.id = ID_TABLEHINT;
	uiRecDemo.hintMessage.generic.type = QMTYPE_ACTION;
	uiRecDemo.hintMessage.generic.flags = QMF_INACTIVE|QMF_SMALLFONT;
	uiRecDemo.hintMessage.generic.color = uiColorHelp;
	uiRecDemo.hintMessage.generic.name = uiRecDemo.hintText;
	uiRecDemo.hintMessage.generic.x = 360;
	uiRecDemo.hintMessage.generic.y = 225;

	uiRecDemo.levelShot.generic.id = ID_LEVELSHOT;
	uiRecDemo.levelShot.generic.type = QMTYPE_BITMAP;
	uiRecDemo.levelShot.generic.flags = QMF_INACTIVE;
	uiRecDemo.levelShot.generic.x = LEVELSHOT_X;
	uiRecDemo.levelShot.generic.y = LEVELSHOT_Y;
	uiRecDemo.levelShot.generic.width = LEVELSHOT_W;
	uiRecDemo.levelShot.generic.height = LEVELSHOT_H;
	uiRecDemo.levelShot.generic.ownerdraw = UI_RecDemo_Ownerdraw;

	uiRecDemo.demosList.generic.id = ID_DEMOLIST;
	uiRecDemo.demosList.generic.type = QMTYPE_SCROLLLIST;
	uiRecDemo.demosList.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_SMALLFONT;
	uiRecDemo.demosList.generic.x = 360;
	uiRecDemo.demosList.generic.y = 255;
	uiRecDemo.demosList.generic.width = 640;
	uiRecDemo.demosList.generic.height = 440;
	uiRecDemo.demosList.generic.callback = UI_RecDemo_Callback;

	uiRecDemo.msgBox.generic.id = ID_MSGBOX;
	uiRecDemo.msgBox.generic.type = QMTYPE_ACTION;
	uiRecDemo.msgBox.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiRecDemo.msgBox.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiRecDemo.msgBox.generic.x = 192;
	uiRecDemo.msgBox.generic.y = 256;
	uiRecDemo.msgBox.generic.width = 640;
	uiRecDemo.msgBox.generic.height = 256;

	uiRecDemo.promptMessage.generic.id = ID_MSGBOX;
	uiRecDemo.promptMessage.generic.type = QMTYPE_ACTION;
	uiRecDemo.promptMessage.generic.flags = QMF_INACTIVE|QMF_DROPSHADOW|QMF_HIDDEN;
	uiRecDemo.promptMessage.generic.name = "Delete selected demo?";
	uiRecDemo.promptMessage.generic.x = 315;
	uiRecDemo.promptMessage.generic.y = 280;

	uiRecDemo.yes.generic.id = ID_YES;
	uiRecDemo.yes.generic.type = QMTYPE_ACTION;
	uiRecDemo.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiRecDemo.yes.generic.name = "Ok";
	uiRecDemo.yes.generic.x = 380;
	uiRecDemo.yes.generic.y = 460;
	uiRecDemo.yes.generic.callback = UI_RecDemo_Callback;

	uiRecDemo.no.generic.id = ID_NO;
	uiRecDemo.no.generic.type = QMTYPE_ACTION;
	uiRecDemo.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiRecDemo.no.generic.name = "Cancel";
	uiRecDemo.no.generic.x = 530;
	uiRecDemo.no.generic.y = 460;
	uiRecDemo.no.generic.callback = UI_RecDemo_Callback;

	UI_RecDemo_GetDemoList();

	UI_AddItem( &uiRecDemo.menu, (void *)&uiRecDemo.background );
	UI_AddItem( &uiRecDemo.menu, (void *)&uiRecDemo.banner );
	UI_AddItem( &uiRecDemo.menu, (void *)&uiRecDemo.record );
	UI_AddItem( &uiRecDemo.menu, (void *)&uiRecDemo.remove );
	UI_AddItem( &uiRecDemo.menu, (void *)&uiRecDemo.cancel );
	UI_AddItem( &uiRecDemo.menu, (void *)&uiRecDemo.hintMessage );
	UI_AddItem( &uiRecDemo.menu, (void *)&uiRecDemo.levelShot );
	UI_AddItem( &uiRecDemo.menu, (void *)&uiRecDemo.demosList );
	UI_AddItem( &uiRecDemo.menu, (void *)&uiRecDemo.msgBox );
	UI_AddItem( &uiRecDemo.menu, (void *)&uiRecDemo.promptMessage );
	UI_AddItem( &uiRecDemo.menu, (void *)&uiRecDemo.no );
	UI_AddItem( &uiRecDemo.menu, (void *)&uiRecDemo.yes );
}

/*
=================
UI_RecDemo_Precache
=================
*/
void UI_RecDemo_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_RecDemo_Menu
=================
*/
void UI_RecDemo_Menu( void )
{
	if( !CheckGameDll( )) return;

	UI_RecDemo_Precache();
	UI_RecDemo_Init();

	UI_PushMenu( &uiRecDemo.menu );
}