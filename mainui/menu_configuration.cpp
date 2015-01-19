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

#define ART_BANNER	     	"gfx/shell/head_config"

#define ID_BACKGROUND    	0
#define ID_BANNER	     	1

#define ID_CONTROLS		2
#define ID_AUDIO	     	3
#define ID_VIDEO	     	4
#define ID_UPDATE   	5
#define ID_DONE	     	6
#define ID_MSGBOX	 	7
#define ID_MSGTEXT	 	8
#define ID_YES	 	130
#define ID_NO	 	131

typedef struct
{
	menuFramework_s	menu;
	
	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuPicButton_s	controls;
	menuPicButton_s	audio;
	menuPicButton_s	video;
	menuPicButton_s	update;
	menuPicButton_s	done;

	// update dialog
	menuAction_s	msgBox;
	menuAction_s	updatePrompt;
	menuPicButton_s	yes;
	menuPicButton_s	no;
} uiOptions_t;

static uiOptions_t		uiOptions;

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

static void UI_CheckUpdatesDialog( void )
{
	// toggle configuration menu between active\inactive
	// show\hide CheckUpdates dialog
	uiOptions.controls.generic.flags ^= QMF_INACTIVE; 
	uiOptions.audio.generic.flags ^= QMF_INACTIVE;
	uiOptions.video.generic.flags ^= QMF_INACTIVE;
	uiOptions.update.generic.flags ^= QMF_INACTIVE;
	uiOptions.done.generic.flags ^= QMF_INACTIVE;

	uiOptions.msgBox.generic.flags ^= QMF_HIDDEN;
	uiOptions.updatePrompt.generic.flags ^= QMF_HIDDEN;
	uiOptions.no.generic.flags ^= QMF_HIDDEN;
	uiOptions.yes.generic.flags ^= QMF_HIDDEN;

}

/*
=================
UI_Options_KeyFunc
=================
*/
static const char *UI_Options_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE && uiOptions.done.generic.flags & QMF_INACTIVE )
	{
		UI_CheckUpdatesDialog ();	// cancel 'check updates' dialog
		return uiSoundNull;
	}
	return UI_DefaultKey( &uiOptions.menu, key, down );
}

/*
=================
UI_Options_Callback
=================
*/
static void UI_Options_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		UI_PopMenu();
		break;
	case ID_CONTROLS:
		UI_Controls_Menu();
		break;
	case ID_AUDIO:
		UI_Audio_Menu();
		break;
	case ID_VIDEO:
		UI_Video_Menu();
		break;
	case ID_UPDATE:
		UI_CheckUpdatesDialog();
		break;
	case ID_YES:
		SHELL_EXECUTE( gMenu.m_gameinfo.update_url, NULL, TRUE );
		break;
	case ID_NO:
		UI_CheckUpdatesDialog();
		break;
	}
}

/*
=================
UI_Options_Init
=================
*/
static void UI_Options_Init( void )
{
	memset( &uiOptions, 0, sizeof( uiOptions_t ));

	uiOptions.menu.vidInitFunc = UI_Options_Init;
	uiOptions.menu.keyFunc = UI_Options_KeyFunc;

	uiOptions.background.generic.id = ID_BACKGROUND;
	uiOptions.background.generic.type = QMTYPE_BITMAP;
	uiOptions.background.generic.flags = QMF_INACTIVE;
	uiOptions.background.generic.x = 0;
	uiOptions.background.generic.y = 0;
	uiOptions.background.generic.width = 1024;
	uiOptions.background.generic.height = 768;
	uiOptions.background.pic = ART_BACKGROUND;

	uiOptions.banner.generic.id = ID_BANNER;
	uiOptions.banner.generic.type = QMTYPE_BITMAP;
	uiOptions.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiOptions.banner.generic.x = UI_BANNER_POSX;
	uiOptions.banner.generic.y = UI_BANNER_POSY;
	uiOptions.banner.generic.width = UI_BANNER_WIDTH;
	uiOptions.banner.generic.height = UI_BANNER_HEIGHT;
	uiOptions.banner.pic = ART_BANNER;

	uiOptions.controls.generic.id	= ID_CONTROLS;
	uiOptions.controls.generic.type = QMTYPE_BM_BUTTON;
	uiOptions.controls.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiOptions.controls.generic.x = 72;
	uiOptions.controls.generic.y = 230;
	uiOptions.controls.generic.name = "Controls";
	uiOptions.controls.generic.statusText = "Change keyboard and mouse settings";
	uiOptions.controls.generic.callback = UI_Options_Callback;

	UI_UtilSetupPicButton( &uiOptions.controls, PC_CONTROLS );

	uiOptions.audio.generic.id = ID_AUDIO;
	uiOptions.audio.generic.type = QMTYPE_BM_BUTTON;
	uiOptions.audio.generic.flags	= QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiOptions.audio.generic.x = 72;
	uiOptions.audio.generic.y = 280;
	uiOptions.audio.generic.name = "Audio";
	uiOptions.audio.generic.statusText = "Change sound volume and quality";
	uiOptions.audio.generic.callback = UI_Options_Callback;

	UI_UtilSetupPicButton( &uiOptions.audio, PC_AUDIO );

	uiOptions.video.generic.id = ID_VIDEO;
	uiOptions.video.generic.type = QMTYPE_BM_BUTTON;
	uiOptions.video.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiOptions.video.generic.x = 72;
	uiOptions.video.generic.y = 330;
	uiOptions.video.generic.name = "Video";
	uiOptions.video.generic.statusText = "Change screen size, video mode and gamma";
	uiOptions.video.generic.callback = UI_Options_Callback;

	UI_UtilSetupPicButton( &uiOptions.video, PC_VIDEO );

	uiOptions.update.generic.id = ID_UPDATE;
	uiOptions.update.generic.type = QMTYPE_BM_BUTTON;
	uiOptions.update.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiOptions.update.generic.x = 72;
	uiOptions.update.generic.y = 380;
	uiOptions.update.generic.name = "Update";
	uiOptions.update.generic.statusText = "Donwload the latest version of the Xash3D engine";
	uiOptions.update.generic.callback = UI_Options_Callback;
	UI_UtilSetupPicButton(&uiOptions.update,PC_UPDATE);

	if( !strlen( gMenu.m_gameinfo.update_url ))
		uiOptions.update.generic.flags |= QMF_GRAYED;

	uiOptions.done.generic.id = ID_DONE;
	uiOptions.done.generic.type = QMTYPE_BM_BUTTON;
	uiOptions.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiOptions.done.generic.x = 72;
	uiOptions.done.generic.y = 430;
	uiOptions.done.generic.name = "Done";
	uiOptions.done.generic.statusText = "Go back to the Main Menu";
	uiOptions.done.generic.callback = UI_Options_Callback;

	UI_UtilSetupPicButton( &uiOptions.done, PC_DONE );

	uiOptions.msgBox.generic.id = ID_MSGBOX;
	uiOptions.msgBox.generic.type = QMTYPE_ACTION;
	uiOptions.msgBox.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiOptions.msgBox.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiOptions.msgBox.generic.x = 192;
	uiOptions.msgBox.generic.y = 256;
	uiOptions.msgBox.generic.width = 640;
	uiOptions.msgBox.generic.height = 256;

	uiOptions.updatePrompt.generic.id = ID_MSGBOX;
	uiOptions.updatePrompt.generic.type = QMTYPE_ACTION;
	uiOptions.updatePrompt.generic.flags = QMF_INACTIVE|QMF_DROPSHADOW|QMF_HIDDEN;
	uiOptions.updatePrompt.generic.name = "Check the Internet for updates?";
	uiOptions.updatePrompt.generic.x = 248;
	uiOptions.updatePrompt.generic.y = 280;

	uiOptions.yes.generic.id = ID_YES;
	uiOptions.yes.generic.type = QMTYPE_BM_BUTTON;
	uiOptions.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiOptions.yes.generic.name = "Ok";
	uiOptions.yes.generic.x = 380;
	uiOptions.yes.generic.y = 460;
	uiOptions.yes.generic.callback = UI_Options_Callback;

	UI_UtilSetupPicButton( &uiOptions.yes, PC_OK );

	uiOptions.no.generic.id = ID_NO;
	uiOptions.no.generic.type = QMTYPE_BM_BUTTON;
	uiOptions.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiOptions.no.generic.name = "Cancel";
	uiOptions.no.generic.x = 530;
	uiOptions.no.generic.y = 460;
	uiOptions.no.generic.callback = UI_Options_Callback;

	UI_UtilSetupPicButton( &uiOptions.no, PC_CANCEL );

	UI_AddItem( &uiOptions.menu, (void *)&uiOptions.background );
	UI_AddItem( &uiOptions.menu, (void *)&uiOptions.banner );
	UI_AddItem( &uiOptions.menu, (void *)&uiOptions.done );
	UI_AddItem( &uiOptions.menu, (void *)&uiOptions.controls );
	UI_AddItem( &uiOptions.menu, (void *)&uiOptions.audio );
	UI_AddItem( &uiOptions.menu, (void *)&uiOptions.video );
	UI_AddItem( &uiOptions.menu, (void *)&uiOptions.update );
	UI_AddItem( &uiOptions.menu, (void *)&uiOptions.msgBox );
	UI_AddItem( &uiOptions.menu, (void *)&uiOptions.updatePrompt );
	UI_AddItem( &uiOptions.menu, (void *)&uiOptions.no );
	UI_AddItem( &uiOptions.menu, (void *)&uiOptions.yes );
}

/*
=================
UI_Options_Precache
=================
*/
void UI_Options_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_Options_Menu
=================
*/
void UI_Options_Menu( void )
{
	UI_Options_Precache();
	UI_Options_Init();
	
	UI_PushMenu( &uiOptions.menu );
}