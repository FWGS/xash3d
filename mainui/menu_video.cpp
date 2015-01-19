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

#define ART_BANNER		"gfx/shell/head_video"

#define ID_BACKGROUND	0
#define ID_BANNER		1

#define ID_VIDOPTIONS  	2
#define ID_VIDMODES	  	3
#define ID_DONE		4

typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuPicButton_s	vidOptions;
	menuPicButton_s	vidModes;
	menuPicButton_s	done;
} uiVideo_t;

static uiVideo_t	uiVideo;

/*
=================
UI_Video_Callback
=================
*/
static void UI_Video_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_VIDOPTIONS:
		UI_VidOptions_Menu();
		break;
	case ID_VIDMODES:
		UI_VidModes_Menu();
		break;
	case ID_DONE:
		UI_PopMenu();
		break;
	}
}

/*
=================
UI_Video_Init
=================
*/
static void UI_Video_Init( void )
{
	memset( &uiVideo, 0, sizeof( uiVideo_t ));

	uiVideo.menu.vidInitFunc = UI_Video_Init;

	uiVideo.background.generic.id = ID_BACKGROUND;
	uiVideo.background.generic.type = QMTYPE_BITMAP;
	uiVideo.background.generic.flags = QMF_INACTIVE;
	uiVideo.background.generic.x = 0;
	uiVideo.background.generic.y = 0;
	uiVideo.background.generic.width = 1024;
	uiVideo.background.generic.height = 768;
	uiVideo.background.pic = ART_BACKGROUND;

	uiVideo.banner.generic.id = ID_BANNER;
	uiVideo.banner.generic.type = QMTYPE_BITMAP;
	uiVideo.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiVideo.banner.generic.x = UI_BANNER_POSX;
	uiVideo.banner.generic.y = UI_BANNER_POSY;
	uiVideo.banner.generic.width = UI_BANNER_WIDTH;
	uiVideo.banner.generic.height = UI_BANNER_HEIGHT;
	uiVideo.banner.pic = ART_BANNER;

	uiVideo.vidOptions.generic.id = ID_VIDOPTIONS;
	uiVideo.vidOptions.generic.type = QMTYPE_BM_BUTTON;
	uiVideo.vidOptions.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiVideo.vidOptions.generic.name = "Video options";
	uiVideo.vidOptions.generic.statusText = "Set video options such as screen size, gamma and image quality.";
	uiVideo.vidOptions.generic.x = 72;
	uiVideo.vidOptions.generic.y = 230;
	uiVideo.vidOptions.generic.callback = UI_Video_Callback;

	UI_UtilSetupPicButton( &uiVideo.vidOptions, PC_VID_OPT );

	uiVideo.vidModes.generic.id = ID_VIDMODES;
	uiVideo.vidModes.generic.type = QMTYPE_BM_BUTTON;
	uiVideo.vidModes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiVideo.vidModes.generic.name = "Video modes";
	uiVideo.vidModes.generic.statusText = "Set video modes and configure 3D accelerators.";
	uiVideo.vidModes.generic.x = 72;
	uiVideo.vidModes.generic.y = 280;
	uiVideo.vidModes.generic.callback = UI_Video_Callback;

	UI_UtilSetupPicButton( &uiVideo.vidModes, PC_VID_MODES );

	uiVideo.done.generic.id = ID_DONE;
	uiVideo.done.generic.type = QMTYPE_BM_BUTTON;
	uiVideo.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiVideo.done.generic.name = "Done";
	uiVideo.done.generic.statusText = "Go back to the previous menu";
	uiVideo.done.generic.x = 72;
	uiVideo.done.generic.y = 330;
	uiVideo.done.generic.callback = UI_Video_Callback;

	UI_UtilSetupPicButton( &uiVideo.done, PC_DONE );

	UI_AddItem( &uiVideo.menu, (void *)&uiVideo.background );
	UI_AddItem( &uiVideo.menu, (void *)&uiVideo.banner );
	UI_AddItem( &uiVideo.menu, (void *)&uiVideo.vidOptions );
	UI_AddItem( &uiVideo.menu, (void *)&uiVideo.vidModes );
	UI_AddItem( &uiVideo.menu, (void *)&uiVideo.done );
}

/*
=================
UI_Video_Precache
=================
*/
void UI_Video_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_Video_Menu
=================
*/
void UI_Video_Menu( void )
{
	UI_Video_Precache();
	UI_Video_Init();

	UI_PushMenu( &uiVideo.menu );
}