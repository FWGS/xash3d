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

#define ART_BANNER		"gfx/shell/head_saveload"

#define ID_BACKGROUND	0
#define ID_BANNER		1

#define ID_PLAY	  	2
#define ID_RECORD	  	3
#define ID_DONE		4

#define ID_MSGHINT		5

typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuAction_s	record;
	menuAction_s	play;
	menuAction_s	done;

	menuAction_s	hintMessage;
	char		hintText[MAX_HINT_TEXT];
} uiPlayRec_t;

static uiPlayRec_t	uiPlayRec;

/*
=================
UI_PlayRec_Callback
=================
*/
static void UI_PlayRec_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_PLAY:
		UI_PlayDemo_Menu();
		break;
	case ID_RECORD:
		UI_RecDemo_Menu();
		break;
	case ID_DONE:
		UI_PopMenu();
		break;
	}
}

/*
=================
UI_PlayRec_Init
=================
*/
static void UI_PlayRec_Init( void )
{
	memset( &uiPlayRec, 0, sizeof( uiPlayRec_t ));

	uiPlayRec.menu.vidInitFunc = UI_PlayRec_Init;

	strcat( uiPlayRec.hintText, "During play or record demo, you can quickly stop\n" );
	strcat( uiPlayRec.hintText, "playing/recording demo by pressing " );
	strcat( uiPlayRec.hintText, KEY_KeynumToString( KEY_GetKey( "stop" )));
	strcat( uiPlayRec.hintText, ".\n" );

	uiPlayRec.background.generic.id = ID_BACKGROUND;
	uiPlayRec.background.generic.type = QMTYPE_BITMAP;
	uiPlayRec.background.generic.flags = QMF_INACTIVE;
	uiPlayRec.background.generic.x = 0;
	uiPlayRec.background.generic.y = 0;
	uiPlayRec.background.generic.width = 1024;
	uiPlayRec.background.generic.height = 768;
	uiPlayRec.background.pic = ART_BACKGROUND;

	uiPlayRec.banner.generic.id = ID_BANNER;
	uiPlayRec.banner.generic.type = QMTYPE_BITMAP;
	uiPlayRec.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiPlayRec.banner.generic.x = UI_BANNER_POSX;
	uiPlayRec.banner.generic.y = UI_BANNER_POSY;
	uiPlayRec.banner.generic.width = UI_BANNER_WIDTH;
	uiPlayRec.banner.generic.height = UI_BANNER_HEIGHT;
	uiPlayRec.banner.pic = ART_BANNER;

	uiPlayRec.play.generic.id = ID_PLAY;
	uiPlayRec.play.generic.type = QMTYPE_ACTION;
	uiPlayRec.play.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiPlayRec.play.generic.name = "Play demo";
	uiPlayRec.play.generic.statusText = "Play a specified demo";
	uiPlayRec.play.generic.x = 72;
	uiPlayRec.play.generic.y = 230;
	uiPlayRec.play.generic.callback = UI_PlayRec_Callback;

	uiPlayRec.record.generic.id = ID_RECORD;
	uiPlayRec.record.generic.type = QMTYPE_ACTION;
	uiPlayRec.record.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiPlayRec.record.generic.name = "Record demo";
	uiPlayRec.record.generic.statusText = "Record demo at this time";
	uiPlayRec.record.generic.x = 72;
	uiPlayRec.record.generic.y = 280;
	uiPlayRec.record.generic.callback = UI_PlayRec_Callback;

	uiPlayRec.done.generic.id = ID_DONE;
	uiPlayRec.done.generic.type = QMTYPE_ACTION;
	uiPlayRec.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiPlayRec.done.generic.name = "Done";
	uiPlayRec.done.generic.statusText = "Go back to the Main Menu";
	uiPlayRec.done.generic.x = 72;
	uiPlayRec.done.generic.y = 330;
	uiPlayRec.done.generic.callback = UI_PlayRec_Callback;

	uiPlayRec.hintMessage.generic.id = ID_MSGHINT;
	uiPlayRec.hintMessage.generic.type = QMTYPE_ACTION;
	uiPlayRec.hintMessage.generic.flags = QMF_INACTIVE|QMF_SMALLFONT;
	uiPlayRec.hintMessage.generic.color = uiColorHelp;
	uiPlayRec.hintMessage.generic.name = uiPlayRec.hintText;
	uiPlayRec.hintMessage.generic.x = 360;
	uiPlayRec.hintMessage.generic.y = 480;

	UI_AddItem( &uiPlayRec.menu, (void *)&uiPlayRec.background );
	UI_AddItem( &uiPlayRec.menu, (void *)&uiPlayRec.banner );
	UI_AddItem( &uiPlayRec.menu, (void *)&uiPlayRec.play );
	UI_AddItem( &uiPlayRec.menu, (void *)&uiPlayRec.record );
	UI_AddItem( &uiPlayRec.menu, (void *)&uiPlayRec.done );
	UI_AddItem( &uiPlayRec.menu, (void *)&uiPlayRec.hintMessage );
}

/*
=================
UI_PlayRec_Precache
=================
*/
void UI_PlayRec_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_PlayRec_Menu
=================
*/
void UI_PlayRec_Menu( void )
{
	UI_PlayRec_Precache();
	UI_PlayRec_Init();

	UI_PushMenu( &uiPlayRec.menu );
}