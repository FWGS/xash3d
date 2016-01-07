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

#define ART_BANNER	  	"gfx/shell/head_touchbuttons"
#define ART_GAMMA		"gfx/shell/gamma"

#define ID_BACKGROUND 	0
#define ID_BANNER	  	1
#define ID_DONE	  	2
#define ID_RED	3
#define ID_GREEN		4
#define ID_BLUE	5
#define ID_ALPHA	6
#define ID_HIDE	7
#define ID_RESET	8
#define ID_BUTTONLIST	9
#define ID_SAVE 10
#define ID_DELETE 11
#define ID_MSGBOX	 	12
#define ID_MSGTEXT	 	13
#define ID_NAME	 	14
#define ID_CANCEL	 	14
#define ID_COLOR	 	15
#define ID_PREVIEW	 	16
#define ID_TEXTURE	 	17
#define ID_COMMAND	 	18
#define ID_SELECT	 	19
#define ID_LOCK			21
#define ID_MP			22
#define ID_SP			23
#define ID_YES	 	130
#define ID_NO	 	131
typedef struct
{
	menuFramework_s	menu;
    char		bNames[UI_MAXGAMES][95];
    char		bTextures[UI_MAXGAMES][95];
    char		bCommands[UI_MAXGAMES][95];
    unsigned char		bColors[UI_MAXGAMES][4];
    bool        bHide[UI_MAXGAMES];
    bool        gettingList;

    char		*bNamesPtr[UI_MAXGAMES];
	menuBitmap_s	background;
	//menuBitmap_s	banner;
	//menuBitmap_s	testImage;

	menuPicButton_s	done;
    menuPicButton_s	cancel;

    menuSlider_s	red;
    menuSlider_s	green;
    menuSlider_s	blue;
    menuSlider_s	alpha;
    menuCheckBox_s	hide;
	menuCheckBox_s	sp;
	menuCheckBox_s	mp;
	menuCheckBox_s	lock;
	menuPicButton_s	reset;
    menuPicButton_s	set;
	menuPicButton_s	remove;
	menuPicButton_s	save;
    menuPicButton_s	select;
    menuField_s	command;
    menuField_s	texture;
    menuField_s	name;
    menuAction_s color;
    menuAction_s preview;
    menuScrollList_s buttonList;

	// prompt dialog
	menuAction_s	msgBox;
	menuAction_s	promptMessage;
	menuPicButton_s	yes;
	menuPicButton_s	no;
	void ( *dialogAction ) ( void );
	char dialogText[256];
} uiTouchButtons_t;

static uiTouchButtons_t	uiTouchButtons;

void UI_TouchButtons_AddButtonToList( const char *name, const char *texture, const char *command, unsigned char *color, int flags )
{
	if( !uiTouchButtons.gettingList )
		return;
}

void UI_TouchButtons_GetButtonList()
{
	uiTouchButtons.buttonList.numItems = 0;
	CLIENT_COMMAND( 1, "" ); //perform Cbuf_Execute()
	uiTouchButtons.gettingList = true;
	CLIENT_COMMAND( 1, "touch_list\n" );
	uiTouchButtons.gettingList = false;
	int i = uiTouchButtons.buttonList.numItems;

	if( uiTouchButtons.buttonList.generic.charHeight )
	{
		uiTouchButtons.buttonList.numRows = (uiTouchButtons.buttonList.generic.height2 / uiTouchButtons.buttonList.generic.charHeight) - 2;
		if( uiTouchButtons.buttonList.numRows > uiTouchButtons.buttonList.numItems )
			uiTouchButtons.buttonList.numRows = i;
	}

	for ( ; i < UI_MAXGAMES; i++ )
		uiTouchButtons.bNamesPtr[i] = NULL;


	uiTouchButtons.buttonList.itemNames = (const char **)uiTouchButtons.bNamesPtr;
}
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

static void UI_Preview_Ownerdraw( void *self )
{
    menuCommon_s	*item = (menuCommon_s *)self;

    UI_FillRect( item->x, item->y, item->width, item->height, 0xFFFFFFFF );
}

static void UI_Color_Ownerdraw( void *self )
{
    menuCommon_s	*item = (menuCommon_s *)self;

    UI_FillRect( item->x, item->y, item->width, item->height, 0xFFFFFFFF );
}

static void UI_DeleteButton()
{
	char command[256];

    if( uiTouchButtons.buttonList.curItem < 1 )
		return;

    snprintf(command, 256, "touch_removebutton %s\n", uiTouchButtons.bNames[ uiTouchButtons.buttonList.curItem ] );
	CLIENT_COMMAND(1, command);
    //UI_TouchButtons_GetButtonsList();
}

static void UI_ResetButtons()
{
	CLIENT_COMMAND( 0, "touch_removeall\n" );
	CLIENT_COMMAND( 0, "touch_loaddefaults\n" );
}

/*
=================
UI_TouchButtons_Callback
=================
*/
static void UI_TouchButtons_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	switch( item->id )
	{
    case ID_HIDE:
		if( event == QM_PRESSED )
			((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_PRESSED;
		else ((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_FOCUS;
		break;
	}

	if( event == QM_CHANGED )
    {
		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		UI_PopMenu();
		break;
	case ID_RESET:
        uiTouchButtons.set.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.remove.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.hide.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.buttonList.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.blue.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.alpha.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.red.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.green.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.reset.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.name.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.done.generic.flags |= QMF_INACTIVE;


        uiTouchButtons.msgBox.generic.flags &= ~QMF_HIDDEN;
        uiTouchButtons.promptMessage.generic.flags &= ~QMF_HIDDEN;
        uiTouchButtons.no.generic.flags &= ~QMF_HIDDEN;
        uiTouchButtons.yes.generic.flags &= ~QMF_HIDDEN;
        strcpy( uiTouchButtons.dialogText, "Reset all buttons?" );
        uiTouchButtons.dialogAction = UI_ResetButtons;
		break;
	case ID_DELETE:
        uiTouchButtons.set.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.remove.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.hide.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.buttonList.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.blue.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.alpha.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.red.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.green.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.reset.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.name.generic.flags |= QMF_INACTIVE;
        uiTouchButtons.done.generic.flags |= QMF_INACTIVE;


        uiTouchButtons.msgBox.generic.flags &= ~QMF_HIDDEN;
        uiTouchButtons.promptMessage.generic.flags &= ~QMF_HIDDEN;
        uiTouchButtons.no.generic.flags &= ~QMF_HIDDEN;
        uiTouchButtons.yes.generic.flags &= ~QMF_HIDDEN;
        strcpy( uiTouchButtons.dialogText, "Delete selected button?" );
        uiTouchButtons.dialogAction = UI_DeleteButton;
		break;
	case ID_YES:
    if( uiTouchButtons.dialogAction )
        uiTouchButtons.dialogAction();
	case ID_NO:
        uiTouchButtons.set.generic.flags &= ~QMF_INACTIVE;
        uiTouchButtons.remove.generic.flags &= ~QMF_INACTIVE;
        uiTouchButtons.hide.generic.flags &= ~QMF_INACTIVE;
        uiTouchButtons.buttonList.generic.flags &= ~QMF_INACTIVE;
        uiTouchButtons.blue.generic.flags &= ~QMF_INACTIVE;
        uiTouchButtons.alpha.generic.flags &= ~QMF_INACTIVE;
        uiTouchButtons.red.generic.flags &= ~QMF_INACTIVE;
        uiTouchButtons.green.generic.flags &= ~QMF_INACTIVE;
        uiTouchButtons.reset.generic.flags &= ~QMF_INACTIVE;
        uiTouchButtons.name.generic.flags &= ~QMF_INACTIVE;
        uiTouchButtons.done.generic.flags &= ~QMF_INACTIVE;

        uiTouchButtons.msgBox.generic.flags |= QMF_HIDDEN;
        uiTouchButtons.promptMessage.generic.flags |= QMF_HIDDEN;
        uiTouchButtons.no.generic.flags |= QMF_HIDDEN;
        uiTouchButtons.yes.generic.flags |= QMF_HIDDEN;
		break;
	case ID_SAVE:
            //UI_TouchButtons_GetButtonList();
		break;
	}
}

/*
=================
UI_TouchButtons_Init
=================
*/
static void UI_TouchButtons_Init( void )
{
	memset( &uiTouchButtons, 0, sizeof( uiTouchButtons_t ));

	//uiTouchOptions.hTestImage = PIC_Load( ART_GAMMA, PIC_KEEP_RGBDATA );

	uiTouchButtons.menu.vidInitFunc = UI_TouchButtons_Init;

	uiTouchButtons.background.generic.id = ID_BACKGROUND;
	uiTouchButtons.background.generic.type = QMTYPE_BITMAP;
	uiTouchButtons.background.generic.flags = QMF_INACTIVE;
	uiTouchButtons.background.generic.x = 0;
	uiTouchButtons.background.generic.y = 0;
	uiTouchButtons.background.generic.width = uiStatic.width;
	uiTouchButtons.background.generic.height = 768;
	uiTouchButtons.background.pic = ART_BACKGROUND;

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
	uiTouchOptions.testImage.generic.ownerdraw = UI_TouchButtons_Ownerdraw;
*/
	uiTouchButtons.done.generic.id = ID_DONE;
	uiTouchButtons.done.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.done.generic.x = 72;
	uiTouchButtons.done.generic.y = 550;
	uiTouchButtons.done.generic.name = "Done";
	uiTouchButtons.done.generic.statusText = "Save changes and go back to the Touch Menu";
	uiTouchButtons.done.generic.callback = UI_TouchButtons_Callback;

	UI_UtilSetupPicButton( &uiTouchButtons.done, PC_DONE );

	uiTouchButtons.cancel.generic.id = ID_CANCEL;
	uiTouchButtons.cancel.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.cancel.generic.x = 72;
	uiTouchButtons.cancel.generic.y = 600;
	uiTouchButtons.cancel.generic.name = "Cancel";
	uiTouchButtons.cancel.generic.statusText = "Discard changes and go back to the Video Menu";
	uiTouchButtons.cancel.generic.callback = UI_TouchButtons_Callback;

	UI_UtilSetupPicButton( &uiTouchButtons.cancel, PC_CANCEL );

	uiTouchButtons.red.generic.id = ID_RED;
	uiTouchButtons.red.generic.type = QMTYPE_SLIDER;
	uiTouchButtons.red.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.red.generic.name = "Red:";
	uiTouchButtons.red.generic.x = 680;
	uiTouchButtons.red.generic.y = 150;
	uiTouchButtons.red.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.red.generic.statusText = "Horizontal look sensitivity";
	uiTouchButtons.red.minValue = 0.0;
	uiTouchButtons.red.maxValue = 1.0;
	uiTouchButtons.red.range = 0.05f;

	uiTouchButtons.green.generic.id = ID_GREEN;
	uiTouchButtons.green.generic.type = QMTYPE_SLIDER;
	uiTouchButtons.green.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.green.generic.name = "Green:";
	uiTouchButtons.green.generic.x = 680;
	uiTouchButtons.green.generic.y = 210;
	uiTouchButtons.green.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.green.generic.statusText = "Vertical look sensitivity";
	uiTouchButtons.green.minValue = 0.0;
	uiTouchButtons.green.maxValue = 1.0;
	uiTouchButtons.green.range = 0.05f;

	uiTouchButtons.blue.generic.id = ID_BLUE;
	uiTouchButtons.blue.generic.type = QMTYPE_SLIDER;
	uiTouchButtons.blue.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.blue.generic.name = "Blue:";
	uiTouchButtons.blue.generic.x = 680;
	uiTouchButtons.blue.generic.y = 270;
	uiTouchButtons.blue.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.blue.generic.statusText = "Side move sensitivity";
	uiTouchButtons.blue.minValue = 0.0;
	uiTouchButtons.blue.maxValue = 1.0;
	uiTouchButtons.blue.range = 0.05f;

	uiTouchButtons.alpha.generic.id = ID_ALPHA;
	uiTouchButtons.alpha.generic.type = QMTYPE_SLIDER;
	uiTouchButtons.alpha.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.alpha.generic.name = "Alpha:";
	uiTouchButtons.alpha.generic.x = 680;
	uiTouchButtons.alpha.generic.y = 330;
	uiTouchButtons.alpha.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.alpha.generic.statusText = "Forward move sensitivity";
	uiTouchButtons.alpha.minValue = 0.0;
	uiTouchButtons.alpha.maxValue = 1.0;
	uiTouchButtons.alpha.range = 0.05f;

	uiTouchButtons.hide.generic.id = ID_HIDE;
	uiTouchButtons.hide.generic.type = QMTYPE_CHECKBOX;
	uiTouchButtons.hide.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_MOUSEONLY|QMF_DROPSHADOW;
	uiTouchButtons.hide.generic.name = "Hide";
	uiTouchButtons.hide.generic.x = 384 - 72 + 400;
	uiTouchButtons.hide.generic.y = 420;
	uiTouchButtons.hide.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.hide.generic.statusText = "show/hide button";

	uiTouchButtons.mp.generic.id = ID_MP;
	uiTouchButtons.mp.generic.type = QMTYPE_CHECKBOX;
	uiTouchButtons.mp.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_MOUSEONLY|QMF_DROPSHADOW;
	uiTouchButtons.mp.generic.name = "MP";
	uiTouchButtons.mp.generic.x = 400;
	uiTouchButtons.mp.generic.y = 420;
	uiTouchButtons.mp.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.mp.generic.statusText = "Show button only in multiplayer";

	uiTouchButtons.sp.generic.id = ID_SP;
	uiTouchButtons.sp.generic.type = QMTYPE_CHECKBOX;
	uiTouchButtons.sp.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_MOUSEONLY|QMF_DROPSHADOW;
	uiTouchButtons.sp.generic.name = "SP";
	uiTouchButtons.sp.generic.x = 160 - 72 + 400;
	uiTouchButtons.sp.generic.y = 420;
	uiTouchButtons.sp.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.sp.generic.statusText = "Show button only in singleplayer";

	uiTouchButtons.lock.generic.id = ID_LOCK;
	uiTouchButtons.lock.generic.type = QMTYPE_CHECKBOX;
	uiTouchButtons.lock.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_MOUSEONLY|QMF_DROPSHADOW;
	uiTouchButtons.lock.generic.name = "Lock";
	uiTouchButtons.lock.generic.x = 256 - 72 + 400;
	uiTouchButtons.lock.generic.y = 420;
	uiTouchButtons.lock.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.lock.generic.statusText = "Lock button editing";


	uiTouchButtons.buttonList.generic.id = ID_BUTTONLIST;
	uiTouchButtons.buttonList.generic.type = QMTYPE_SCROLLLIST;
	uiTouchButtons.buttonList.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_SMALLFONT;
	uiTouchButtons.buttonList.generic.x = 72;
	uiTouchButtons.buttonList.generic.y = 150;
	uiTouchButtons.buttonList.generic.width = 300;
	uiTouchButtons.buttonList.generic.height = 370;
	uiTouchButtons.buttonList.generic.callback = UI_TouchButtons_Callback;

	uiTouchButtons.reset.generic.id = ID_RESET;
	uiTouchButtons.reset.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.reset.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.reset.generic.name = "Reset";
	uiTouchButtons.reset.generic.x = 270;
	uiTouchButtons.reset.generic.y = 600;
	uiTouchButtons.reset.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.reset.generic.statusText = "Reset touch to default state";

	uiTouchButtons.remove.generic.id = ID_DELETE;
	uiTouchButtons.remove.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.remove.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.remove.generic.x = 270;
	uiTouchButtons.remove.generic.y = 550;
	uiTouchButtons.remove.generic.name = "Delete";
	uiTouchButtons.remove.generic.statusText = "Delete saved game";
	uiTouchButtons.remove.generic.callback = UI_TouchButtons_Callback;
	UI_UtilSetupPicButton( &uiTouchButtons.remove, PC_DELETE );

	uiTouchButtons.save.generic.id = ID_SAVE;
	uiTouchButtons.save.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.save.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.save.generic.x = 384 - 72 + 400;
	uiTouchButtons.save.generic.y = 520;
	uiTouchButtons.save.generic.name = "Save";
	uiTouchButtons.save.generic.statusText = "Save as new button";
	uiTouchButtons.save.generic.callback = UI_TouchButtons_Callback;

	uiTouchButtons.select.generic.id = ID_SELECT;
	uiTouchButtons.select.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.select.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.select.generic.x = 500;
	uiTouchButtons.select.generic.y = 300;
	uiTouchButtons.select.generic.name = "Select";
	uiTouchButtons.select.generic.statusText = "Select texture from list";
	uiTouchButtons.select.generic.callback = UI_TouchButtons_Callback;

	uiTouchButtons.name.generic.id = ID_NAME;
	uiTouchButtons.name.generic.type = QMTYPE_FIELD;
	uiTouchButtons.name.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.name.generic.name = "New Button:";
	uiTouchButtons.name.generic.x = 400;
	uiTouchButtons.name.generic.y = 520;
	uiTouchButtons.name.generic.width = 205;
	uiTouchButtons.name.generic.height = 32;
	uiTouchButtons.name.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.name.maxLength = 16;

	uiTouchButtons.command.generic.id = ID_COMMAND;
	uiTouchButtons.command.generic.type = QMTYPE_FIELD;
	uiTouchButtons.command.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.command.generic.name = "Command:";
	uiTouchButtons.command.generic.x = 400;
	uiTouchButtons.command.generic.y = 150;
	uiTouchButtons.command.generic.width = 205;
	uiTouchButtons.command.generic.height = 32;
	uiTouchButtons.command.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.command.maxLength = 16;

	uiTouchButtons.texture.generic.id = ID_TEXTURE;
	uiTouchButtons.texture.generic.type = QMTYPE_FIELD;
	uiTouchButtons.texture.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.texture.generic.name = "Texture:";
	uiTouchButtons.texture.generic.x = 400;
	uiTouchButtons.texture.generic.y = 250;
	uiTouchButtons.texture.generic.width = 205;
	uiTouchButtons.texture.generic.height = 32;
	uiTouchButtons.texture.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.texture.maxLength = 16;

	uiTouchButtons.msgBox.generic.id = ID_MSGBOX;
	uiTouchButtons.msgBox.generic.type = QMTYPE_ACTION;
	uiTouchButtons.msgBox.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiTouchButtons.msgBox.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiTouchButtons.msgBox.generic.x = DLG_X + 192;
	uiTouchButtons.msgBox.generic.y = 256;
	uiTouchButtons.msgBox.generic.width = 640;
	uiTouchButtons.msgBox.generic.height = 256;

	uiTouchButtons.color.generic.id = ID_COLOR;
	uiTouchButtons.color.generic.type = QMTYPE_ACTION;
	uiTouchButtons.color.generic.flags = QMF_INACTIVE;
	uiTouchButtons.color.generic.ownerdraw = UI_Color_Ownerdraw; // just a fill rectangle
	uiTouchButtons.color.generic.x = 800;
	uiTouchButtons.color.generic.y = 360;
	uiTouchButtons.color.generic.width = 70;
	uiTouchButtons.color.generic.height = 50;

	uiTouchButtons.preview.generic.id = ID_PREVIEW;
	uiTouchButtons.preview.generic.type = QMTYPE_ACTION;
	uiTouchButtons.preview.generic.flags = QMF_INACTIVE;
	uiTouchButtons.preview.generic.ownerdraw = UI_Preview_Ownerdraw; // just a fill rectangle
	uiTouchButtons.preview.generic.x = 400;
	uiTouchButtons.preview.generic.y = 300;
	uiTouchButtons.preview.generic.width = 70;
	uiTouchButtons.preview.generic.height = 70;

	uiTouchButtons.promptMessage.generic.id = ID_MSGBOX;
	uiTouchButtons.promptMessage.generic.type = QMTYPE_ACTION;
	uiTouchButtons.promptMessage.generic.flags = QMF_INACTIVE|QMF_DROPSHADOW|QMF_HIDDEN;
	uiTouchButtons.promptMessage.generic.name = uiTouchButtons.dialogText;
	uiTouchButtons.promptMessage.generic.x = DLG_X + 315;
	uiTouchButtons.promptMessage.generic.y = 280;

	uiTouchButtons.yes.generic.id = ID_YES;
	uiTouchButtons.yes.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiTouchButtons.yes.generic.name = "Ok";
	uiTouchButtons.yes.generic.x = DLG_X + 380;
	uiTouchButtons.yes.generic.y = 460;
	uiTouchButtons.yes.generic.callback = UI_TouchButtons_Callback;

	UI_UtilSetupPicButton( &uiTouchButtons.yes, PC_OK );

	uiTouchButtons.no.generic.id = ID_NO;
	uiTouchButtons.no.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiTouchButtons.no.generic.name = "Cancel";
	uiTouchButtons.no.generic.x = DLG_X + 530;
	uiTouchButtons.no.generic.y = 460;
	uiTouchButtons.no.generic.callback = UI_TouchButtons_Callback;

	UI_UtilSetupPicButton( &uiTouchButtons.no, PC_CANCEL );
	UI_TouchButtons_GetButtonList();

	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.background );
	//UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.banner );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.done );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.cancel );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.red );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.green );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.blue );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.alpha );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.hide );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.sp );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.mp );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.lock );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.reset );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.buttonList );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.remove );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.save );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.select );

	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.name );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.command );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.texture );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.color );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.preview );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.msgBox );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.promptMessage );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.no );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.yes );


}

/*
=================
UI_TouchButtons_Precache
=================
*/
void UI_TouchButtons_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	//PIC_Load( ART_BANNER );
}

/*
=================
UI_TouchButtons_Menu
=================
*/
void UI_TouchButtons_Menu( void )
{
    UI_TouchButtons_Precache();
    UI_TouchButtons_Init();

    UI_PushMenu( &uiTouchButtons.menu );
}
