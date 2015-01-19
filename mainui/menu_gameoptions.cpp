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

#define ART_BANNER			"gfx/shell/head_advoptions"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_DONE			2
#define ID_CANCEL			3
#define ID_MAXFPS			4
#define ID_MAXFPSMESSAGE		5
#define ID_HAND			6
#define ID_ALLOWDOWNLOAD		7
#define ID_ALWAYSRUN		8

typedef struct
{
	float		maxFPS;
	int		hand;
	int		allowDownload;
	int		alwaysRun;
} uiGameValues_t;

typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuPicButton_s	done;
	menuPicButton_s	cancel;

	menuSpinControl_s	maxFPS;
	menuAction_s	maxFPSmessage;
	menuCheckBox_s	hand;
	menuCheckBox_s	allowDownload;
	menuCheckBox_s	alwaysRun;
} uiGameOptions_t;

static uiGameOptions_t	uiGameOptions;
static uiGameValues_t	uiGameInitial;

/*
=================
UI_GameOptions_UpdateConfig
=================
*/
static void UI_GameOptions_UpdateConfig( void )
{
	static char	fpsText[8];

	sprintf( fpsText, "%.f", uiGameOptions.maxFPS.curValue );
	uiGameOptions.maxFPS.generic.name = fpsText;

	CVAR_SET_FLOAT( "hand", uiGameOptions.hand.enabled );
	CVAR_SET_FLOAT( "sv_allow_download", uiGameOptions.allowDownload.enabled );
	CVAR_SET_FLOAT( "fps_max", uiGameOptions.maxFPS.curValue );
	CVAR_SET_FLOAT( "cl_run", uiGameOptions.alwaysRun.enabled );
}

/*
=================
UI_GameOptions_DiscardChanges
=================
*/
static void UI_GameOptions_DiscardChanges( void )
{
	CVAR_SET_FLOAT( "hand", uiGameInitial.hand );
	CVAR_SET_FLOAT( "sv_allow_download", uiGameInitial.allowDownload );
	CVAR_SET_FLOAT( "fps_max", uiGameInitial.maxFPS );
	CVAR_SET_FLOAT( "cl_run", uiGameInitial.alwaysRun );
}

/*
=================
UI_GameOptions_KeyFunc
=================
*/
static const char *UI_GameOptions_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE )
		UI_GameOptions_DiscardChanges ();
	return UI_DefaultKey( &uiGameOptions.menu, key, down );
}

/*
=================
UI_GameOptions_GetConfig
=================
*/
static void UI_GameOptions_GetConfig( void )
{
	uiGameInitial.maxFPS = uiGameOptions.maxFPS.curValue = CVAR_GET_FLOAT( "fps_max" );

	if( CVAR_GET_FLOAT( "hand" ))
		uiGameInitial.hand = uiGameOptions.hand.enabled = 1;

	if( CVAR_GET_FLOAT( "cl_run" ))
		uiGameInitial.alwaysRun = uiGameOptions.alwaysRun.enabled = 1;

	if( CVAR_GET_FLOAT( "sv_allow_download" ))
		uiGameInitial.allowDownload = uiGameOptions.allowDownload.enabled = 1;

	UI_GameOptions_UpdateConfig ();
}

/*
=================
UI_GameOptions_Callback
=================
*/
static void UI_GameOptions_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	switch( item->id )
	{
	case ID_HAND:
	case ID_ALLOWDOWNLOAD:
	case ID_ALWAYSRUN:
		if( event == QM_PRESSED )
			((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_PRESSED;
		else ((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_FOCUS;
		break;
	}

	if( event == QM_CHANGED )
	{
		UI_GameOptions_UpdateConfig();
		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		UI_PopMenu();
		break;
	case ID_CANCEL:
		UI_GameOptions_DiscardChanges();
		UI_PopMenu();
		break;
	}
}

/*
=================
UI_GameOptions_Init
=================
*/
static void UI_GameOptions_Init( void )
{
	memset( &uiGameInitial, 0, sizeof( uiGameValues_t ));
	memset( &uiGameOptions, 0, sizeof( uiGameOptions_t ));

	uiGameOptions.menu.vidInitFunc = UI_GameOptions_Init;
	uiGameOptions.menu.keyFunc = UI_GameOptions_KeyFunc;

	uiGameOptions.background.generic.id = ID_BACKGROUND;
	uiGameOptions.background.generic.type = QMTYPE_BITMAP;
	uiGameOptions.background.generic.flags = QMF_INACTIVE;
	uiGameOptions.background.generic.x = 0;
	uiGameOptions.background.generic.y = 0;
	uiGameOptions.background.generic.width = 1024;
	uiGameOptions.background.generic.height = 768;
	uiGameOptions.background.pic = ART_BACKGROUND;

	uiGameOptions.banner.generic.id = ID_BANNER;
	uiGameOptions.banner.generic.type = QMTYPE_BITMAP;
	uiGameOptions.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiGameOptions.banner.generic.x = UI_BANNER_POSX;
	uiGameOptions.banner.generic.y = UI_BANNER_POSY;
	uiGameOptions.banner.generic.width = UI_BANNER_WIDTH;
	uiGameOptions.banner.generic.height = UI_BANNER_HEIGHT;
	uiGameOptions.banner.pic = ART_BANNER;

	uiGameOptions.done.generic.id	= ID_DONE;
	uiGameOptions.done.generic.type = QMTYPE_BM_BUTTON;
	uiGameOptions.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW; 
	uiGameOptions.done.generic.x = 72;
	uiGameOptions.done.generic.y = 230;
	uiGameOptions.done.generic.name = "Done";
	uiGameOptions.done.generic.statusText = "Save changes and go back to the Customize Menu";
	uiGameOptions.done.generic.callback = UI_GameOptions_Callback;

	UI_UtilSetupPicButton( &uiGameOptions.done, PC_DONE );

	uiGameOptions.cancel.generic.id = ID_CANCEL;
	uiGameOptions.cancel.generic.type = QMTYPE_BM_BUTTON;
	uiGameOptions.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiGameOptions.cancel.generic.x = 72;
	uiGameOptions.cancel.generic.y = 280;
	uiGameOptions.cancel.generic.name = "Cancel";
	uiGameOptions.cancel.generic.statusText = "Go back to the Customize Menu";
	uiGameOptions.cancel.generic.callback = UI_GameOptions_Callback;

	UI_UtilSetupPicButton( &uiGameOptions.cancel, PC_CANCEL );

	uiGameOptions.maxFPS.generic.id = ID_MAXFPS;
	uiGameOptions.maxFPS.generic.type = QMTYPE_SPINCONTROL;
	uiGameOptions.maxFPS.generic.flags = QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiGameOptions.maxFPS.generic.x = 315;
	uiGameOptions.maxFPS.generic.y = 270;
	uiGameOptions.maxFPS.generic.width = 168;
	uiGameOptions.maxFPS.generic.height = 26;
	uiGameOptions.maxFPS.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.maxFPS.generic.statusText = "Cap your game frame rate";
	uiGameOptions.maxFPS.minValue = 20;
	uiGameOptions.maxFPS.maxValue = 500;
	uiGameOptions.maxFPS.range = 20;

	uiGameOptions.maxFPSmessage.generic.id = ID_MAXFPSMESSAGE;
	uiGameOptions.maxFPSmessage.generic.type = QMTYPE_ACTION;
	uiGameOptions.maxFPSmessage.generic.flags = QMF_SMALLFONT|QMF_INACTIVE|QMF_DROPSHADOW;
	uiGameOptions.maxFPSmessage.generic.x = 280;
	uiGameOptions.maxFPSmessage.generic.y = 230;
	uiGameOptions.maxFPSmessage.generic.name = "Limit game fps";
	uiGameOptions.maxFPSmessage.generic.color = uiColorHelp;

	uiGameOptions.hand.generic.id = ID_HAND;
	uiGameOptions.hand.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.hand.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_MOUSEONLY|QMF_DROPSHADOW;
	uiGameOptions.hand.generic.x = 280;
	uiGameOptions.hand.generic.y = 330;
	uiGameOptions.hand.generic.name = "Use left hand";
	uiGameOptions.hand.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.hand.generic.statusText = "Draw gun at left side";

	uiGameOptions.allowDownload.generic.id = ID_ALLOWDOWNLOAD;
	uiGameOptions.allowDownload.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.allowDownload.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_MOUSEONLY|QMF_DROPSHADOW;
	uiGameOptions.allowDownload.generic.x = 280;
	uiGameOptions.allowDownload.generic.y = 390;
	uiGameOptions.allowDownload.generic.name = "Allow download";
	uiGameOptions.allowDownload.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.allowDownload.generic.statusText = "Allow download of files from servers";

	uiGameOptions.alwaysRun.generic.id = ID_ALWAYSRUN;
	uiGameOptions.alwaysRun.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.alwaysRun.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_MOUSEONLY|QMF_DROPSHADOW;
	uiGameOptions.alwaysRun.generic.x = 280;
	uiGameOptions.alwaysRun.generic.y = 450;
	uiGameOptions.alwaysRun.generic.name = "Always run";
	uiGameOptions.alwaysRun.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.alwaysRun.generic.statusText = "Switch between run/step models when pressed 'run' button";

	UI_GameOptions_GetConfig();

	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.background );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.banner );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.done );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.cancel );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.maxFPS );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.maxFPSmessage );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.hand );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.alwaysRun );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.allowDownload );
}

/*
=================
UI_GameOptions_Precache
=================
*/
void UI_GameOptions_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_GameOptions_Menu
=================
*/
void UI_GameOptions_Menu( void )
{
	UI_GameOptions_Precache();
	UI_GameOptions_Init();

	UI_GameOptions_UpdateConfig();
	UI_PushMenu( &uiGameOptions.menu );
}