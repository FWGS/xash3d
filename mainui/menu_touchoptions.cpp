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

#define ART_BANNER	  	"gfx/shell/head_touchoptions"
#define ART_GAMMA		"gfx/shell/gamma"

#define ID_BACKGROUND 	0
#define ID_BANNER	  	1
#define ID_DONE	  	2
#define ID_LOOKX	3
#define ID_LOOKY		4
#define ID_MOVEX	5 
#define ID_MOVEY	6
#define ID_ENABLE	7
#define ID_RESET	8
#define ID_PROFILELIST	9
#define ID_SAVE 10
#define ID_DELETE 11
#define ID_MSGBOX	 	12
#define ID_MSGTEXT	 	13
#define ID_PROFILENAME	 	14
#define ID_YES	 	130
#define ID_NO	 	131
typedef struct
{
	menuFramework_s	menu;
	char		profileDesc[UI_MAXGAMES][95];
	char		*profileDescPtr[UI_MAXGAMES];
	menuBitmap_s	background;
	//menuBitmap_s	banner;
	//menuBitmap_s	testImage;

	menuPicButton_s	done;

	menuSlider_s	lookX;
	menuSlider_s	lookY;
	menuSlider_s	moveX;
	menuSlider_s	moveY;
	menuCheckBox_s	enable;
	menuPicButton_s	reset;
	menuPicButton_s	save;
	menuPicButton_s	remove;
	menuField_s	profilename;
	menuScrollList_s profiles;

	// prompt dialog
	menuAction_s	msgBox;
	menuAction_s	promptMessage;
	menuPicButton_s	yes;
	menuPicButton_s	no;
	void ( *dialogAction ) ( void );
	char dialogText[256];
} uiTouchOptions_t;

static uiTouchOptions_t	uiTouchOptions;



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


static void UI_TouchOptions_GetProfileList( void )
{
	char	**filenames;
	int	i = 1, numFiles, j;
	char *curprofile;

	filenames = FS_SEARCH( "touch_profiles/*.cfg", &numFiles, TRUE );
	curprofile = CVAR_GET_STRING("touch_config_file");

	strncpy( uiTouchOptions.profileDesc[0], "default", CS_SIZE );
	uiTouchOptions.profileDescPtr[0] = uiTouchOptions.profileDesc[0];
	if( !strcmp( "touch.cfg", curprofile ) )
		uiTouchOptions.profiles.curItem = 0;

	for ( j = 0; j < numFiles; i++, j++ )
	{
		if( i >= UI_MAXGAMES ) break;
		
		// strip path, leave only filename (empty slots doesn't have savename)
		COM_FileBase( filenames[j], uiTouchOptions.profileDesc[i] );
		uiTouchOptions.profileDescPtr[i] = uiTouchOptions.profileDesc[i];
		if( !strcmp( filenames[j], curprofile ) )
			uiTouchOptions.profiles.curItem = i;
	}
	uiTouchOptions.profiles.numItems = i;

	if( uiTouchOptions.profiles.generic.charHeight )
	{
		uiTouchOptions.profiles.numRows = (uiTouchOptions.profiles.generic.height2 / uiTouchOptions.profiles.generic.charHeight) - 2;
		if( uiTouchOptions.profiles.numRows > uiTouchOptions.profiles.numItems )
			uiTouchOptions.profiles.numRows = i;
	}

	for ( ; i < UI_MAXGAMES; i++ )
		uiTouchOptions.profileDescPtr[i] = NULL;


	uiTouchOptions.profiles.itemNames = (const char **)uiTouchOptions.profileDescPtr;
}

/*
=================
UI_TouchOptions_GetConfig
=================
*/
static void UI_TouchOptions_GetConfig( void )
{
	uiTouchOptions.lookX.curValue = RemapVal( CVAR_GET_FLOAT( "touch_yaw" ), 50.0f, 500.0f, 0.0f, 1.0f );
	uiTouchOptions.lookY.curValue = RemapVal( CVAR_GET_FLOAT( "touch_pitch" ), 50.0f, 500.0f, 0.0f, 1.0f );
	uiTouchOptions.moveX.curValue = ( 2.0f / CVAR_GET_FLOAT( "touch_sidezone" ) ) / 100;
	uiTouchOptions.moveY.curValue = ( 2.0f / CVAR_GET_FLOAT( "touch_forwardzone" ) ) / 100;

	if( CVAR_GET_FLOAT( "touch_enable" ))
		uiTouchOptions.enable.enabled = 1;
}

/*
=================
UI_TouchOptions_SetConfig
=================
*/

static void UI_TouchOptions_SetConfig( void )
{
	CVAR_SET_FLOAT( "touch_yaw", RemapVal( uiTouchOptions.lookX.curValue, 0.0f, 1.0f, 50.0f, 500.0f ));
	CVAR_SET_FLOAT( "touch_pitch", RemapVal( uiTouchOptions.lookY.curValue, 0.0f, 1.0f, 50.0f, 500.0f ));
	CVAR_SET_FLOAT( "touch_sidezone", ( 2.0 / uiTouchOptions.moveX.curValue ) / 100 );
	CVAR_SET_FLOAT( "touch_forwardzone", ( 2.0 / uiTouchOptions.moveX.curValue ) / 100 );
	CVAR_SET_FLOAT( "touch_enable", uiTouchOptions.enable.enabled );
}

static void UI_DeleteProfile()
{
	char command[256];

	if( uiTouchOptions.profiles.curItem < 1 )
		return;

	snprintf(command, 256, "touch_deleteprofile %s\n", uiTouchOptions.profileDesc[ uiTouchOptions.profiles.curItem ] );
	CLIENT_COMMAND(1, command);
	UI_TouchOptions_GetProfileList();
}

static void UI_ResetButtons()
{
	CLIENT_COMMAND( 0, "touch_removeall\n" );
	CLIENT_COMMAND( 0, "touch_loaddefaults\n" );
	CLIENT_COMMAND( 0, "touch_pitch 90\n" );
	CLIENT_COMMAND( 0, "touch_yaw 120\n" );
	CLIENT_COMMAND( 0, "touch_forwardzone 0.06\n" );
	CLIENT_COMMAND( 0, "touch_sidezone 0.06\n" );
	UI_TouchOptions_GetConfig();
}

/*
=================
UI_TouchOptions_Callback
=================
*/
static void UI_TouchOptions_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	switch( item->id )
	{
	case ID_ENABLE:
		if( event == QM_PRESSED )
			((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_PRESSED;
		else ((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_FOCUS;
		break;
	}

	if( event == QM_CHANGED )
	{
		UI_TouchOptions_SetConfig();
		if( uiTouchOptions.profiles.curItem == 0 )
			uiTouchOptions.remove.generic.flags |= QMF_GRAYED;
		else uiTouchOptions.remove.generic.flags &= ~QMF_GRAYED;
		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		UI_TouchOptions_SetConfig();
		UI_PopMenu();
		break;
	case ID_RESET:
		uiTouchOptions.save.generic.flags |= QMF_INACTIVE; 
		uiTouchOptions.remove.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.enable.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.profiles.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.moveX.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.moveY.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.lookX.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.lookY.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.reset.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.profilename.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.done.generic.flags |= QMF_INACTIVE;


		uiTouchOptions.msgBox.generic.flags &= ~QMF_HIDDEN;
		uiTouchOptions.promptMessage.generic.flags &= ~QMF_HIDDEN;
		uiTouchOptions.no.generic.flags &= ~QMF_HIDDEN;
		uiTouchOptions.yes.generic.flags &= ~QMF_HIDDEN;
		strcpy( uiTouchOptions.dialogText, "Reset all buttons?" );
		uiTouchOptions.dialogAction = UI_ResetButtons;
		break;
	case ID_DELETE:
		uiTouchOptions.save.generic.flags |= QMF_INACTIVE; 
		uiTouchOptions.remove.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.enable.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.profiles.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.moveX.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.moveY.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.lookX.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.lookY.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.reset.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.profilename.generic.flags |= QMF_INACTIVE;
		uiTouchOptions.done.generic.flags |= QMF_INACTIVE;


		uiTouchOptions.msgBox.generic.flags &= ~QMF_HIDDEN;
		uiTouchOptions.promptMessage.generic.flags &= ~QMF_HIDDEN;
		uiTouchOptions.no.generic.flags &= ~QMF_HIDDEN;
		uiTouchOptions.yes.generic.flags &= ~QMF_HIDDEN;
		strcpy( uiTouchOptions.dialogText, "Delete selected profile?" );
		uiTouchOptions.dialogAction = UI_DeleteProfile;
		break;
	case ID_YES:
	if( uiTouchOptions.dialogAction )
		uiTouchOptions.dialogAction();
	case ID_NO:
		uiTouchOptions.save.generic.flags &= ~QMF_INACTIVE; 
		uiTouchOptions.remove.generic.flags &= ~QMF_INACTIVE;
		uiTouchOptions.enable.generic.flags &= ~QMF_INACTIVE;
		uiTouchOptions.profiles.generic.flags &= ~QMF_INACTIVE;
		uiTouchOptions.moveX.generic.flags &= ~QMF_INACTIVE;
		uiTouchOptions.moveY.generic.flags &= ~QMF_INACTIVE;
		uiTouchOptions.lookX.generic.flags &= ~QMF_INACTIVE;
		uiTouchOptions.lookY.generic.flags &= ~QMF_INACTIVE;
		uiTouchOptions.reset.generic.flags &= ~QMF_INACTIVE;
		uiTouchOptions.profilename.generic.flags &= ~QMF_INACTIVE;
		uiTouchOptions.done.generic.flags &= ~QMF_INACTIVE;

		uiTouchOptions.msgBox.generic.flags |= QMF_HIDDEN;
		uiTouchOptions.promptMessage.generic.flags |= QMF_HIDDEN;
		uiTouchOptions.no.generic.flags |= QMF_HIDDEN;
		uiTouchOptions.yes.generic.flags |= QMF_HIDDEN;
		break;
	case ID_SAVE:
		{
			char name[256];
			snprintf( name, 256, "touch_profiles/%s.cfg", uiTouchOptions.profilename.buffer );
			CVAR_SET_STRING("touch_config_file", name );
			CLIENT_COMMAND( 1, "touch_writeconfig\n" );
		}
		UI_TouchOptions_GetProfileList();
		break;
	}
}

/*
=================
UI_TouchOptions_Init
=================
*/
static void UI_TouchOptions_Init( void )
{
	memset( &uiTouchOptions, 0, sizeof( uiTouchOptions_t ));

	//uiTouchOptions.hTestImage = PIC_Load( ART_GAMMA, PIC_KEEP_RGBDATA );

	uiTouchOptions.menu.vidInitFunc = UI_TouchOptions_Init;

	uiTouchOptions.background.generic.id = ID_BACKGROUND;
	uiTouchOptions.background.generic.type = QMTYPE_BITMAP;
	uiTouchOptions.background.generic.flags = QMF_INACTIVE;
	uiTouchOptions.background.generic.x = 0;
	uiTouchOptions.background.generic.y = 0;
	uiTouchOptions.background.generic.width = uiStatic.width;
	uiTouchOptions.background.generic.height = 768;
	uiTouchOptions.background.pic = ART_BACKGROUND;

	/*uiTouchOptions.banner.generic.id = ID_BANNER;
	uiTouchOptions.banner.generic.type = QMTYPE_BITMAP;
	uiTouchOptions.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiTouchOptions.banner.generic.x = UI_BANNER_POSX;
	uiTouchOptions.banner.generic.y = UI_BANNER_POSY;
	uiTouchOptions.banner.generic.width = UI_BANNER_WIDTH;
	uiTouchOptions.banner.generic.height = UI_BANNER_HEIGHT;
	uiTouchOptions.banner.pic = ART_BANNER;*/
/*
	uiTouchOptions.testImage.generic.id = ID_BANNER;
	uiTouchOptions.testImage.generic.type = QMTYPE_BITMAP;
	uiTouchOptions.testImage.generic.flags = QMF_INACTIVE;
	uiTouchOptions.testImage.generic.x = 390;
	uiTouchOptions.testImage.generic.y = 225;
	uiTouchOptions.testImage.generic.width = 480;
	uiTouchOptions.testImage.generic.height = 450;
	uiTouchOptions.testImage.pic = ART_GAMMA;
	uiTouchOptions.testImage.generic.ownerdraw = UI_TouchOptions_Ownerdraw;
*/
	uiTouchOptions.done.generic.id = ID_DONE;
	uiTouchOptions.done.generic.type = QMTYPE_BM_BUTTON;
	uiTouchOptions.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchOptions.done.generic.x = 72;
	uiTouchOptions.done.generic.y = 535;
	uiTouchOptions.done.generic.name = "Done";
	uiTouchOptions.done.generic.statusText = "Go back to the Video Menu";
	uiTouchOptions.done.generic.callback = UI_TouchOptions_Callback;

	UI_UtilSetupPicButton( &uiTouchOptions.done, PC_DONE );

	uiTouchOptions.lookX.generic.id = ID_LOOKX;
	uiTouchOptions.lookX.generic.type = QMTYPE_SLIDER;
	uiTouchOptions.lookX.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiTouchOptions.lookX.generic.name = "Look X";
	uiTouchOptions.lookX.generic.x = 72;
	uiTouchOptions.lookX.generic.y = 280;
	uiTouchOptions.lookX.generic.callback = UI_TouchOptions_Callback;
	uiTouchOptions.lookX.generic.statusText = "Horizontal look sensitivity";
	uiTouchOptions.lookX.minValue = 0.0;
	uiTouchOptions.lookX.maxValue = 1.0;
	uiTouchOptions.lookX.range = 0.05f;

	uiTouchOptions.lookY.generic.id = ID_LOOKY;
	uiTouchOptions.lookY.generic.type = QMTYPE_SLIDER;
	uiTouchOptions.lookY.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiTouchOptions.lookY.generic.name = "Look Y";
	uiTouchOptions.lookY.generic.x = 72;
	uiTouchOptions.lookY.generic.y = 340;
	uiTouchOptions.lookY.generic.callback = UI_TouchOptions_Callback;
	uiTouchOptions.lookY.generic.statusText = "Vertical look sensitivity";
	uiTouchOptions.lookY.minValue = 0.0;
	uiTouchOptions.lookY.maxValue = 1.0;
	uiTouchOptions.lookY.range = 0.05f;

	uiTouchOptions.moveX.generic.id = ID_MOVEX;
	uiTouchOptions.moveX.generic.type = QMTYPE_SLIDER;
	uiTouchOptions.moveX.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiTouchOptions.moveX.generic.name = "Side";
	uiTouchOptions.moveX.generic.x = 72;
	uiTouchOptions.moveX.generic.y = 400;
	uiTouchOptions.moveX.generic.callback = UI_TouchOptions_Callback;
	uiTouchOptions.moveX.generic.statusText = "Side move sensitivity";
	uiTouchOptions.moveX.minValue = 0.0;
	uiTouchOptions.moveX.maxValue = 1.0;
	uiTouchOptions.moveX.range = 0.05f;

	uiTouchOptions.moveY.generic.id = ID_MOVEY;
	uiTouchOptions.moveY.generic.type = QMTYPE_SLIDER;
	uiTouchOptions.moveY.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiTouchOptions.moveY.generic.name = "Forward";
	uiTouchOptions.moveY.generic.x = 72;
	uiTouchOptions.moveY.generic.y = 460;
	uiTouchOptions.moveY.generic.callback = UI_TouchOptions_Callback;
	uiTouchOptions.moveY.generic.statusText = "Forward move sensitivity";
	uiTouchOptions.moveY.minValue = 0.0;
	uiTouchOptions.moveY.maxValue = 1.0;
	uiTouchOptions.moveY.range = 0.05f;

	uiTouchOptions.enable.generic.id = ID_ENABLE;
	uiTouchOptions.enable.generic.type = QMTYPE_CHECKBOX;
	uiTouchOptions.enable.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_MOUSEONLY|QMF_DROPSHADOW;
	uiTouchOptions.enable.generic.name = "Enable";
	uiTouchOptions.enable.generic.x = 72;
	uiTouchOptions.enable.generic.y = 665;
	uiTouchOptions.enable.generic.callback = UI_TouchOptions_Callback;
	uiTouchOptions.enable.generic.statusText = "enable/disable touch controls";
	
	uiTouchOptions.profiles.generic.id = ID_PROFILELIST;
	uiTouchOptions.profiles.generic.type = QMTYPE_SCROLLLIST;
	uiTouchOptions.profiles.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_SMALLFONT;
	uiTouchOptions.profiles.generic.x = 360;
	uiTouchOptions.profiles.generic.y = 255;
	uiTouchOptions.profiles.generic.width = 300;
	uiTouchOptions.profiles.generic.height = 340;
	uiTouchOptions.profiles.generic.callback = UI_TouchOptions_Callback;

	uiTouchOptions.reset.generic.id = ID_RESET;
	uiTouchOptions.reset.generic.type = QMTYPE_BM_BUTTON;
	uiTouchOptions.reset.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchOptions.reset.generic.name = "Reset";
	uiTouchOptions.reset.generic.x = 72;
	uiTouchOptions.reset.generic.y = 615;
	uiTouchOptions.reset.generic.callback = UI_TouchOptions_Callback;
	uiTouchOptions.reset.generic.statusText = "Reset touch to default state";
	
	uiTouchOptions.remove.generic.id = ID_DELETE;
	uiTouchOptions.remove.generic.type = QMTYPE_BM_BUTTON;
	uiTouchOptions.remove.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchOptions.remove.generic.x = 360;
	uiTouchOptions.remove.generic.y = 255 + 355;
	uiTouchOptions.remove.generic.name = "Delete";
	uiTouchOptions.remove.generic.statusText = "Delete saved game";
	uiTouchOptions.remove.generic.callback = UI_TouchOptions_Callback;
	UI_UtilSetupPicButton( &uiTouchOptions.remove, PC_DELETE );

	uiTouchOptions.profilename.generic.id = ID_PROFILENAME;
	uiTouchOptions.profilename.generic.type = QMTYPE_FIELD;
	uiTouchOptions.profilename.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchOptions.profilename.generic.name = "New Profile:";
	uiTouchOptions.profilename.generic.x = 680;
	uiTouchOptions.profilename.generic.y = 260;
	uiTouchOptions.profilename.generic.width = 205;
	uiTouchOptions.profilename.generic.height = 32;
	uiTouchOptions.profilename.generic.callback = UI_TouchOptions_Callback;
	uiTouchOptions.profilename.maxLength = 16;

	uiTouchOptions.save.generic.id = ID_SAVE;
	uiTouchOptions.save.generic.type = QMTYPE_BM_BUTTON;
	uiTouchOptions.save.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchOptions.save.generic.x = 680;
	uiTouchOptions.save.generic.y = 330;
	uiTouchOptions.save.generic.name = "Save";
	uiTouchOptions.save.generic.statusText = "Save new profile";
	uiTouchOptions.save.generic.callback = UI_TouchOptions_Callback;
	
	uiTouchOptions.msgBox.generic.id = ID_MSGBOX;
	uiTouchOptions.msgBox.generic.type = QMTYPE_ACTION;
	uiTouchOptions.msgBox.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiTouchOptions.msgBox.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiTouchOptions.msgBox.generic.x = DLG_X + 192;
	uiTouchOptions.msgBox.generic.y = 256;
	uiTouchOptions.msgBox.generic.width = 640;
	uiTouchOptions.msgBox.generic.height = 256;

	uiTouchOptions.promptMessage.generic.id = ID_MSGBOX;
	uiTouchOptions.promptMessage.generic.type = QMTYPE_ACTION;
	uiTouchOptions.promptMessage.generic.flags = QMF_INACTIVE|QMF_DROPSHADOW|QMF_HIDDEN;
	uiTouchOptions.promptMessage.generic.name = uiTouchOptions.dialogText;
	uiTouchOptions.promptMessage.generic.x = DLG_X + 315;
	uiTouchOptions.promptMessage.generic.y = 280;

	uiTouchOptions.yes.generic.id = ID_YES;
	uiTouchOptions.yes.generic.type = QMTYPE_BM_BUTTON;
	uiTouchOptions.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiTouchOptions.yes.generic.name = "Ok";
	uiTouchOptions.yes.generic.x = DLG_X + 380;
	uiTouchOptions.yes.generic.y = 460;
	uiTouchOptions.yes.generic.callback = UI_TouchOptions_Callback;

	UI_UtilSetupPicButton( &uiTouchOptions.yes, PC_OK );

	uiTouchOptions.no.generic.id = ID_NO;
	uiTouchOptions.no.generic.type = QMTYPE_BM_BUTTON;
	uiTouchOptions.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiTouchOptions.no.generic.name = "Cancel";
	uiTouchOptions.no.generic.x = DLG_X + 530;
	uiTouchOptions.no.generic.y = 460;
	uiTouchOptions.no.generic.callback = UI_TouchOptions_Callback;

	UI_UtilSetupPicButton( &uiTouchOptions.no, PC_CANCEL );
	UI_TouchOptions_GetConfig();
	UI_TouchOptions_GetProfileList();

	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.background );
	//UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.banner );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.done );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.lookX );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.lookY );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.moveX );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.moveY );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.enable );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.reset );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.profiles );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.save );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.profilename );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.msgBox );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.promptMessage );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.no );
	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.yes );

	UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.remove );
}

/*
=================
UI_TouchOptions_Precache
=================
*/
void UI_TouchOptions_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	//PIC_Load( ART_BANNER );
}

/*
=================
UI_TouchOptions_Menu
=================
*/
void UI_TouchOptions_Menu( void )
{
	UI_TouchOptions_Precache();
	UI_TouchOptions_Init();

	UI_PushMenu( &uiTouchOptions.menu );
}
