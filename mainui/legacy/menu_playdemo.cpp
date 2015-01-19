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

#define ART_BANNER	     	"gfx/shell/head_load"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_PLAY		2
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
	menuAction_s	play;
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
} uiPlayDemo_t;

static uiPlayDemo_t		uiPlayDemo;

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
	uiPlayDemo.play.generic.flags ^= QMF_INACTIVE; 
	uiPlayDemo.remove.generic.flags ^= QMF_INACTIVE;
	uiPlayDemo.cancel.generic.flags ^= QMF_INACTIVE;
	uiPlayDemo.demosList.generic.flags ^= QMF_INACTIVE;

	uiPlayDemo.msgBox.generic.flags ^= QMF_HIDDEN;
	uiPlayDemo.promptMessage.generic.flags ^= QMF_HIDDEN;
	uiPlayDemo.no.generic.flags ^= QMF_HIDDEN;
	uiPlayDemo.yes.generic.flags ^= QMF_HIDDEN;

}

/*
=================
UI_PlayDemo_KeyFunc
=================
*/
static const char *UI_PlayDemo_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE && uiPlayDemo.play.generic.flags & QMF_INACTIVE )
	{
		UI_DeleteDialog();
		return uiSoundNull;
	}
	return UI_DefaultKey( &uiPlayDemo.menu, key, down );
}

/*
=================
UI_PlayDemo_GetDemoList
=================
*/
static void UI_PlayDemo_GetDemoList( void )
{
	char	comment[256];
	char	**filenames;
	int	i, numFiles;

	filenames = FS_SEARCH( "demos/*.dem", &numFiles, TRUE );

	for( i = 0; i < numFiles; i++ )
	{
		if( i >= UI_MAXGAMES ) break;
		
		if( !GET_DEMO_COMMENT( filenames[i], comment ))
		{
			if( strlen( comment ))
			{
				// get name string even if not found - CL_GetComment can be mark demos
				// as <CORRUPTED> <OLD VERSION> etc
				StringConcat( uiPlayDemo.demoDescription[i], uiEmptyString, TITLE_LENGTH );
				StringConcat( uiPlayDemo.demoDescription[i], comment, MAPNAME_LENGTH );
				StringConcat( uiPlayDemo.demoDescription[i], uiEmptyString, MAXCLIENTS_LENGTH );
				uiPlayDemo.demoDescriptionPtr[i] = uiPlayDemo.demoDescription[i];
				COM_FileBase( filenames[i], uiPlayDemo.delName[i] );
			}
			else uiPlayDemo.demoDescriptionPtr[i] = NULL;
			continue;
		}

		// strip path, leave only filename (empty slots doesn't have savename)
		COM_FileBase( filenames[i], uiPlayDemo.demoName[i] );
		COM_FileBase( filenames[i], uiPlayDemo.delName[i] );

		// fill demo desc
		StringConcat( uiPlayDemo.demoDescription[i], comment + CS_SIZE, TITLE_LENGTH );
		StringConcat( uiPlayDemo.demoDescription[i], uiEmptyString, TITLE_LENGTH );
		StringConcat( uiPlayDemo.demoDescription[i], comment, MAPNAME_LENGTH );
		StringConcat( uiPlayDemo.demoDescription[i], uiEmptyString, MAPNAME_LENGTH ); // fill remaining entries
		StringConcat( uiPlayDemo.demoDescription[i], comment + CS_SIZE * 2, MAXCLIENTS_LENGTH );
		StringConcat( uiPlayDemo.demoDescription[i], uiEmptyString, MAXCLIENTS_LENGTH );
		uiPlayDemo.demoDescriptionPtr[i] = uiPlayDemo.demoDescription[i];
	}

	for ( ; i < UI_MAXGAMES; i++ )
		uiPlayDemo.demoDescriptionPtr[i] = NULL;
	uiPlayDemo.demosList.itemNames = (const char **)uiPlayDemo.demoDescriptionPtr;

	if( strlen( uiPlayDemo.demoName[0] ) == 0 )
		uiPlayDemo.play.generic.flags |= QMF_GRAYED;
	else uiPlayDemo.play.generic.flags &= ~QMF_GRAYED;

	if( strlen( uiPlayDemo.delName[0] ) == 0 || !stricmp( gpGlobals->demoname, uiPlayDemo.delName[uiPlayDemo.demosList.curItem] ))
		uiPlayDemo.remove.generic.flags |= QMF_GRAYED;
	else uiPlayDemo.remove.generic.flags &= ~QMF_GRAYED;
}

/*
=================
UI_PlayDemo_Callback
=================
*/
static void UI_PlayDemo_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event == QM_CHANGED )
	{
		if( strlen( uiPlayDemo.demoName[uiPlayDemo.demosList.curItem] ) == 0 )
			uiPlayDemo.play.generic.flags |= QMF_GRAYED;
		else uiPlayDemo.play.generic.flags &= ~QMF_GRAYED;

		if( strlen( uiPlayDemo.delName[uiPlayDemo.demosList.curItem] ) == 0 || !stricmp( gpGlobals->demoname, uiPlayDemo.delName[uiPlayDemo.demosList.curItem] ))
			uiPlayDemo.remove.generic.flags |= QMF_GRAYED;
		else uiPlayDemo.remove.generic.flags &= ~QMF_GRAYED;
		return;
	}

	if( event != QM_ACTIVATED )
		return;
	
	switch( item->id )
	{
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_PLAY:
		if( gpGlobals->demoplayback || gpGlobals->demorecording )
		{
			CLIENT_COMMAND( FALSE, "stop" );
			uiPlayDemo.play.generic.name = "Play";
			uiPlayDemo.play.generic.statusText = "Play a demo";
			uiPlayDemo.remove.generic.flags &= ~QMF_GRAYED;
		}
		else if( strlen( uiPlayDemo.demoName[uiPlayDemo.demosList.curItem] ))
		{
			char	cmd[128];
			sprintf( cmd, "playdemo \"%s\"\n", uiPlayDemo.demoName[uiPlayDemo.demosList.curItem] );
			CLIENT_COMMAND( FALSE, cmd );
		}
		break;
	case ID_NO:
	case ID_DELETE:
		UI_DeleteDialog();
		break;
	case ID_YES:
		if( strlen( uiPlayDemo.delName[uiPlayDemo.demosList.curItem] ))
		{
			char	cmd[128];
			sprintf( cmd, "killdemo \"%s\"\n", uiPlayDemo.delName[uiPlayDemo.demosList.curItem] );

			CLIENT_COMMAND( TRUE, cmd );

			sprintf( cmd, "demos/%s.bmp", uiPlayDemo.delName[uiPlayDemo.demosList.curItem] );
			PIC_Free( cmd );

			// restarts the menu
			UI_PopMenu();
			UI_PlayDemo_Menu();
			return;
		}
		UI_DeleteDialog();
		break;
	}
}

/*
=================
UI_PlayDemo_Ownerdraw
=================
*/
static void UI_PlayDemo_Ownerdraw( void *self )
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

		if( strlen( uiPlayDemo.demoName[uiPlayDemo.demosList.curItem] ))
		{
			char	demoshot[128];

			sprintf( demoshot, "demos/%s.bmp", uiPlayDemo.demoName[uiPlayDemo.demosList.curItem] );

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
UI_PlayDemo_Init
=================
*/
static void UI_PlayDemo_Init( void )
{
	memset( &uiPlayDemo, 0, sizeof( uiPlayDemo_t ));

	uiPlayDemo.menu.vidInitFunc = UI_PlayDemo_Init;
	uiPlayDemo.menu.keyFunc = UI_PlayDemo_KeyFunc;

	StringConcat( uiPlayDemo.hintText, "Title", TITLE_LENGTH );
	StringConcat( uiPlayDemo.hintText, uiEmptyString, TITLE_LENGTH );
	StringConcat( uiPlayDemo.hintText, "Map", MAPNAME_LENGTH );
	StringConcat( uiPlayDemo.hintText, uiEmptyString, MAPNAME_LENGTH );
	StringConcat( uiPlayDemo.hintText, "Max Clients", MAXCLIENTS_LENGTH );
	StringConcat( uiPlayDemo.hintText, uiEmptyString, MAXCLIENTS_LENGTH );

	uiPlayDemo.background.generic.id = ID_BACKGROUND;
	uiPlayDemo.background.generic.type = QMTYPE_BITMAP;
	uiPlayDemo.background.generic.flags = QMF_INACTIVE;
	uiPlayDemo.background.generic.x = 0;
	uiPlayDemo.background.generic.y = 0;
	uiPlayDemo.background.generic.width = 1024;
	uiPlayDemo.background.generic.height = 768;
	uiPlayDemo.background.pic = ART_BACKGROUND;

	uiPlayDemo.banner.generic.id = ID_BANNER;
	uiPlayDemo.banner.generic.type = QMTYPE_BITMAP;
	uiPlayDemo.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiPlayDemo.banner.generic.x = UI_BANNER_POSX;
	uiPlayDemo.banner.generic.y = UI_BANNER_POSY;
	uiPlayDemo.banner.generic.width = UI_BANNER_WIDTH;
	uiPlayDemo.banner.generic.height = UI_BANNER_HEIGHT;
	uiPlayDemo.banner.pic = ART_BANNER;

	uiPlayDemo.play.generic.id = ID_PLAY;
	uiPlayDemo.play.generic.type = QMTYPE_ACTION;
	uiPlayDemo.play.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiPlayDemo.play.generic.x = 72;
	uiPlayDemo.play.generic.y = 230;

	if ( gpGlobals->demoplayback )
	{
		uiPlayDemo.play.generic.name = "Stop";
		uiPlayDemo.play.generic.statusText = "Stop a demo playing";
	}
	else if ( gpGlobals->demorecording )
	{
		uiPlayDemo.play.generic.name = "Stop";
		uiPlayDemo.play.generic.statusText = "Stop a demo recording";
	}
	else
	{
		uiPlayDemo.play.generic.name = "Play";
		uiPlayDemo.play.generic.statusText = "Play a demo";
	}
	uiPlayDemo.play.generic.callback = UI_PlayDemo_Callback;

	uiPlayDemo.remove.generic.id = ID_DELETE;
	uiPlayDemo.remove.generic.type = QMTYPE_ACTION;
	uiPlayDemo.remove.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiPlayDemo.remove.generic.x = 72;
	uiPlayDemo.remove.generic.y = 280;
	uiPlayDemo.remove.generic.name = "Delete";
	uiPlayDemo.remove.generic.statusText = "Delete a demo";
	uiPlayDemo.remove.generic.callback = UI_PlayDemo_Callback;

	uiPlayDemo.cancel.generic.id = ID_CANCEL;
	uiPlayDemo.cancel.generic.type = QMTYPE_ACTION;
	uiPlayDemo.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiPlayDemo.cancel.generic.x = 72;
	uiPlayDemo.cancel.generic.y = 330;
	uiPlayDemo.cancel.generic.name = "Cancel";
	uiPlayDemo.cancel.generic.statusText = "Return back to main menu";
	uiPlayDemo.cancel.generic.callback = UI_PlayDemo_Callback;

	uiPlayDemo.hintMessage.generic.id = ID_TABLEHINT;
	uiPlayDemo.hintMessage.generic.type = QMTYPE_ACTION;
	uiPlayDemo.hintMessage.generic.flags = QMF_INACTIVE|QMF_SMALLFONT;
	uiPlayDemo.hintMessage.generic.color = uiColorHelp;
	uiPlayDemo.hintMessage.generic.name = uiPlayDemo.hintText;
	uiPlayDemo.hintMessage.generic.x = 360;
	uiPlayDemo.hintMessage.generic.y = 225;

	uiPlayDemo.levelShot.generic.id = ID_LEVELSHOT;
	uiPlayDemo.levelShot.generic.type = QMTYPE_BITMAP;
	uiPlayDemo.levelShot.generic.flags = QMF_INACTIVE;
	uiPlayDemo.levelShot.generic.x = LEVELSHOT_X;
	uiPlayDemo.levelShot.generic.y = LEVELSHOT_Y;
	uiPlayDemo.levelShot.generic.width = LEVELSHOT_W;
	uiPlayDemo.levelShot.generic.height = LEVELSHOT_H;
	uiPlayDemo.levelShot.generic.ownerdraw = UI_PlayDemo_Ownerdraw;

	uiPlayDemo.demosList.generic.id = ID_DEMOLIST;
	uiPlayDemo.demosList.generic.type = QMTYPE_SCROLLLIST;
	uiPlayDemo.demosList.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_SMALLFONT;
	uiPlayDemo.demosList.generic.x = 360;
	uiPlayDemo.demosList.generic.y = 255;
	uiPlayDemo.demosList.generic.width = 640;
	uiPlayDemo.demosList.generic.height = 440;
	uiPlayDemo.demosList.generic.callback = UI_PlayDemo_Callback;

	uiPlayDemo.msgBox.generic.id = ID_MSGBOX;
	uiPlayDemo.msgBox.generic.type = QMTYPE_ACTION;
	uiPlayDemo.msgBox.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiPlayDemo.msgBox.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiPlayDemo.msgBox.generic.x = 192;
	uiPlayDemo.msgBox.generic.y = 256;
	uiPlayDemo.msgBox.generic.width = 640;
	uiPlayDemo.msgBox.generic.height = 256;

	uiPlayDemo.promptMessage.generic.id = ID_MSGBOX;
	uiPlayDemo.promptMessage.generic.type = QMTYPE_ACTION;
	uiPlayDemo.promptMessage.generic.flags = QMF_INACTIVE|QMF_DROPSHADOW|QMF_HIDDEN;
	uiPlayDemo.promptMessage.generic.name = "Delete selected demo?";
	uiPlayDemo.promptMessage.generic.x = 315;
	uiPlayDemo.promptMessage.generic.y = 280;

	uiPlayDemo.yes.generic.id = ID_YES;
	uiPlayDemo.yes.generic.type = QMTYPE_ACTION;
	uiPlayDemo.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiPlayDemo.yes.generic.name = "Ok";
	uiPlayDemo.yes.generic.x = 380;
	uiPlayDemo.yes.generic.y = 460;
	uiPlayDemo.yes.generic.callback = UI_PlayDemo_Callback;

	uiPlayDemo.no.generic.id = ID_NO;
	uiPlayDemo.no.generic.type = QMTYPE_ACTION;
	uiPlayDemo.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiPlayDemo.no.generic.name = "Cancel";
	uiPlayDemo.no.generic.x = 530;
	uiPlayDemo.no.generic.y = 460;
	uiPlayDemo.no.generic.callback = UI_PlayDemo_Callback;

	UI_PlayDemo_GetDemoList();

	UI_AddItem( &uiPlayDemo.menu, (void *)&uiPlayDemo.background );
	UI_AddItem( &uiPlayDemo.menu, (void *)&uiPlayDemo.banner );
	UI_AddItem( &uiPlayDemo.menu, (void *)&uiPlayDemo.play );
	UI_AddItem( &uiPlayDemo.menu, (void *)&uiPlayDemo.remove );
	UI_AddItem( &uiPlayDemo.menu, (void *)&uiPlayDemo.cancel );
	UI_AddItem( &uiPlayDemo.menu, (void *)&uiPlayDemo.hintMessage );
	UI_AddItem( &uiPlayDemo.menu, (void *)&uiPlayDemo.levelShot );
	UI_AddItem( &uiPlayDemo.menu, (void *)&uiPlayDemo.demosList );
	UI_AddItem( &uiPlayDemo.menu, (void *)&uiPlayDemo.msgBox );
	UI_AddItem( &uiPlayDemo.menu, (void *)&uiPlayDemo.promptMessage );
	UI_AddItem( &uiPlayDemo.menu, (void *)&uiPlayDemo.no );
	UI_AddItem( &uiPlayDemo.menu, (void *)&uiPlayDemo.yes );
}

/*
=================
UI_PlayDemo_Precache
=================
*/
void UI_PlayDemo_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_PlayDemo_Menu
=================
*/
void UI_PlayDemo_Menu( void )
{
	UI_PlayDemo_Precache();
	UI_PlayDemo_Init();

	UI_PushMenu( &uiPlayDemo.menu );
}