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

#define ART_BANNER		"gfx/shell/head_touch"

#define ID_BACKGROUND	0
#define ID_BANNER		1

#define ID_TOUCHOPTIONS  	2
#define ID_TOUCHBUTTONS	  	3
#define ID_DONE		4

typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuPicButton_s	touchOptions;
	menuPicButton_s	touchButtons;
	menuPicButton_s	done;
} uiTouch_t;

static uiTouch_t	uiTouch;

/*
=================
UI_Touch_Callback
=================
*/
static void UI_Touch_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_TOUCHOPTIONS:
		UI_TouchOptions_Menu();
		break;
	case ID_TOUCHBUTTONS:
		UI_TouchButtons_Menu();
		break;
	case ID_DONE:
		UI_PopMenu();
		break;
	}
}

/*
=================
UI_Touch_Init
=================
*/
static void UI_Touch_Init( void )
{
	memset( &uiTouch, 0, sizeof( uiTouch_t ));

	uiTouch.menu.vidInitFunc = UI_Touch_Init;

	uiTouch.background.generic.id = ID_BACKGROUND;
	uiTouch.background.generic.type = QMTYPE_BITMAP;
	uiTouch.background.generic.flags = QMF_INACTIVE;
	uiTouch.background.generic.x = 0;
	uiTouch.background.generic.y = 0;
	uiTouch.background.generic.width = uiStatic.width;
	uiTouch.background.generic.height = 768;
	uiTouch.background.pic = ART_BACKGROUND;

	uiTouch.banner.generic.id = ID_BANNER;
	uiTouch.banner.generic.type = QMTYPE_BITMAP;
	uiTouch.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiTouch.banner.generic.x = UI_BANNER_POSX;
	uiTouch.banner.generic.y = UI_BANNER_POSY;
	uiTouch.banner.generic.width = UI_BANNER_WIDTH;
	uiTouch.banner.generic.height = UI_BANNER_HEIGHT;
	uiTouch.banner.pic = ART_BANNER;

	uiTouch.touchOptions.generic.id = ID_TOUCHOPTIONS;
	uiTouch.touchOptions.generic.type = QMTYPE_BM_BUTTON;
	uiTouch.touchOptions.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY | QMF_ACT_ONRELEASE;
	uiTouch.touchOptions.generic.name = "Touch options";
	uiTouch.touchOptions.generic.statusText = "Touch sensitivity and profile options";
	uiTouch.touchOptions.generic.x = 72;
	uiTouch.touchOptions.generic.y = 230;
	uiTouch.touchOptions.generic.callback = UI_Touch_Callback;
	uiTouch.touchOptions.pic = PIC_Load("gfx/shell/btn_touch_options");

	//UI_UtilSetupPicButton( &uiTouch.touchOptions, PC_TOUCH_OPT );

	uiTouch.touchButtons.generic.id = ID_TOUCHBUTTONS;
	uiTouch.touchButtons.generic.type = QMTYPE_BM_BUTTON;
	uiTouch.touchButtons.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY | QMF_ACT_ONRELEASE;
	uiTouch.touchButtons.generic.name = "Touch Buttons";
	uiTouch.touchButtons.generic.statusText = "Add, remove, edit touch buttons";
	uiTouch.touchButtons.generic.x = 72;
	uiTouch.touchButtons.generic.y = 280;
	uiTouch.touchButtons.generic.callback = UI_Touch_Callback;
	uiTouch.touchButtons.pic = PIC_Load("gfx/shell/btn_touch_buttons");

	//UI_UtilSetupPicButton( &uiTouch.touchButtons, PC_TOUCH_BUTTONS );

	uiTouch.done.generic.id = ID_DONE;
	uiTouch.done.generic.type = QMTYPE_BM_BUTTON;
	uiTouch.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiTouch.done.generic.name = "Done";
	uiTouch.done.generic.statusText = "Go back to the previous menu";
	uiTouch.done.generic.x = 72;
	uiTouch.done.generic.y = 330;
	uiTouch.done.generic.callback = UI_Touch_Callback;

	UI_UtilSetupPicButton( &uiTouch.done, PC_DONE );

	UI_AddItem( &uiTouch.menu, (void *)&uiTouch.background );
	UI_AddItem( &uiTouch.menu, (void *)&uiTouch.banner );
	UI_AddItem( &uiTouch.menu, (void *)&uiTouch.touchOptions );
	UI_AddItem( &uiTouch.menu, (void *)&uiTouch.touchButtons );
	UI_AddItem( &uiTouch.menu, (void *)&uiTouch.done );
}

/*
=================
UI_Touch_Precache
=================
*/
void UI_Touch_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_Touch_Menu
=================
*/
void UI_Touch_Menu( void )
{
	UI_Touch_Precache();
	UI_Touch_Init();

	UI_PushMenu( &uiTouch.menu );
}
