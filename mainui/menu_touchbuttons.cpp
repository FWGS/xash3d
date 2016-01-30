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

#define ART_BANNER	  	"gfx/shell/head_touch_buttons"

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
#define ID_COLOR	 	15
#define ID_PREVIEW	 	16
#define ID_TEXTURE	 	17
#define ID_COMMAND	 	18
#define ID_SELECT	 	19
#define ID_LOCK			21
#define ID_MP			22
#define ID_SP			23
#define ID_ADDITIVE		24
#define ID_EDITOR		25
#define ID_CANCEL	 	26
#define ID_YES	 	130
#define ID_NO	 	131
typedef struct
{
	menuFramework_s	menu;
    char		bNames[UI_MAXGAMES][95];
    char		bTextures[UI_MAXGAMES][95];
    char		bCommands[UI_MAXGAMES][95];
    unsigned char		bColors[UI_MAXGAMES][4];
	int        bFlags[UI_MAXGAMES];
    bool        gettingList;
	HIMAGE textureid;
	char selectedName[256];
	int curflags;

    char		*bNamesPtr[UI_MAXGAMES];
	menuBitmap_s	background;
	menuBitmap_s	banner;

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
	menuCheckBox_s	additive;
	menuPicButton_s	reset;
	menuPicButton_s	remove;
	menuPicButton_s	save;
    menuPicButton_s	select;
	menuPicButton_s	editor;
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
	bool initialized;
} uiTouchButtons_t;

static uiTouchButtons_t	uiTouchButtons;

#define BIT(i) ( 1<<i )

#define TOUCH_FL_HIDE BIT( 0 )
#define TOUCH_FL_NOEDIT BIT( 1 )
#define TOUCH_FL_CLIENT BIT( 2 )
#define TOUCH_FL_MP BIT( 3 )
#define TOUCH_FL_SP BIT( 4 )
#define TOUCH_FL_DEF_SHOW BIT( 5 )
#define TOUCH_FL_DEF_HIDE BIT( 6 )
#define TOUCH_FL_DRAW_ADDITIVE BIT( 7 )
#define TOUCH_FL_STROKE BIT( 8 )

static void UI_TouchButtons_UpdateFields( void );

void UI_TouchButtons_AddButtonToList( const char *name, const char *texture, const char *command, unsigned char *color, int flags )
{
	if( !uiTouchButtons.gettingList )
		return;
	int i = uiTouchButtons.buttonList.numItems++;
	strcpy(uiTouchButtons.bNames[i], name);
	uiTouchButtons.bNamesPtr[i] = uiTouchButtons.bNames[i];
	strcpy(uiTouchButtons.bTextures[i], texture);
	strcpy(uiTouchButtons.bCommands[i], command);
	uiTouchButtons.bColors[i][0] = color[0];
	uiTouchButtons.bColors[i][1] = color[1];
	uiTouchButtons.bColors[i][2] = color[2];
	uiTouchButtons.bColors[i][3] = color[3];
	uiTouchButtons.bFlags[i] = flags;
}

void UI_TouchButtons_GetButtonList()
{
	if( !uiTouchButtons.initialized )
		return;
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
	UI_TouchButtons_UpdateFields();
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
#define CURCOLOR1(x) ( (unsigned int) ( uiTouchButtons.x.curValue * 255.0f ) )
static void UI_Preview_Ownerdraw( void *self )
{
    menuCommon_s	*item = (menuCommon_s *)self;
	UI_FillRect( item->x - 2, item->y - 2, item->width + 4, item->height + 4, 0xFFC0C0C0 );
	UI_FillRect( item->x, item->y, item->width, item->height, 0xFF808080 );
	PIC_Set( uiTouchButtons.textureid, CURCOLOR1(red), CURCOLOR1(green), CURCOLOR1(blue), CURCOLOR1(alpha));
	if( uiTouchButtons.additive.enabled )
		PIC_DrawAdditive( item->x, item->y, item->width, item->height );
	else
		PIC_DrawTrans( item->x, item->y, item->width, item->height );
}
#define CURCOLOR ( (unsigned int)(uiTouchButtons.blue.curValue * 255) | \
				 (unsigned int)(uiTouchButtons.green.curValue * 255) << 8 | \
				 (unsigned int)(uiTouchButtons.red.curValue * 255) << 16 | \
				 (unsigned int)(uiTouchButtons.alpha.curValue * 255) << 24 )
static void UI_Color_Ownerdraw( void *self )
{
    menuCommon_s	*item = (menuCommon_s *)self;

	UI_FillRect( item->x, item->y, item->width, item->height, CURCOLOR );
}

static void UI_DeleteButton()
{
	char command[256];
	snprintf(command, 256, "touch_removebutton %s\n", uiTouchButtons.selectedName );
	CLIENT_COMMAND(1, command);
	UI_TouchButtons_GetButtonList();
}

static void UI_ResetButtons()
{
	CLIENT_COMMAND( 0, "touch_removeall\n" );
	CLIENT_COMMAND( 0, "touch_loaddefaults\n" );
	UI_TouchButtons_GetButtonList();
}

static void UI_TouchButtons_UpdateFields()
{
	strcpy( uiTouchButtons.selectedName, uiTouchButtons.bNames[uiTouchButtons.buttonList.curItem]);
	strcpy( uiTouchButtons.texture.buffer, uiTouchButtons.bTextures[uiTouchButtons.buttonList.curItem]);
	strcpy( uiTouchButtons.command.buffer, uiTouchButtons.bCommands[uiTouchButtons.buttonList.curItem]);
	uiTouchButtons.red.curValue = (float) uiTouchButtons.bColors[uiTouchButtons.buttonList.curItem][0]/255;
	uiTouchButtons.green.curValue = (float) uiTouchButtons.bColors[uiTouchButtons.buttonList.curItem][1]/255;
	uiTouchButtons.blue.curValue = (float) uiTouchButtons.bColors[uiTouchButtons.buttonList.curItem][2]/255;
	uiTouchButtons.alpha.curValue = (float) uiTouchButtons.bColors[uiTouchButtons.buttonList.curItem][3]/255;
	uiTouchButtons.curflags = uiTouchButtons.bFlags[ uiTouchButtons.buttonList.curItem ];
	uiTouchButtons.mp.enabled = !!( uiTouchButtons.curflags & TOUCH_FL_MP );
	uiTouchButtons.sp.enabled = !!( uiTouchButtons.curflags & TOUCH_FL_SP );
	uiTouchButtons.lock.enabled = !!( uiTouchButtons.curflags & TOUCH_FL_NOEDIT );
	uiTouchButtons.hide.enabled = !!( uiTouchButtons.curflags & TOUCH_FL_HIDE );
	uiTouchButtons.additive.enabled = !!( uiTouchButtons.curflags & TOUCH_FL_DRAW_ADDITIVE );
	if( uiTouchButtons.texture.buffer[0] && uiTouchButtons.texture.buffer[0] != '#' )
		uiTouchButtons.textureid = PIC_Load(uiTouchButtons.texture.buffer);
	else
		uiTouchButtons.textureid = 0;
	uiTouchButtons.name.buffer[0] = 0;
	uiTouchButtons.name.cursor = 0;
	uiTouchButtons.texture.cursor = strlen( uiTouchButtons.texture.buffer );
	if( uiTouchButtons.texture.cursor > uiTouchButtons.texture.widthInChars )
		uiTouchButtons.texture.scroll = uiTouchButtons.texture.cursor;
	else
		uiTouchButtons.texture.scroll = 0;
	uiTouchButtons.command.cursor = strlen( uiTouchButtons.command.buffer );
	if( uiTouchButtons.command.cursor > uiTouchButtons.command.widthInChars )
		uiTouchButtons.command.scroll = uiTouchButtons.command.cursor;
	else
		uiTouchButtons.command.scroll = 0;

}
static void UI_TouchButtons_DisableButtons()
{
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
	uiTouchButtons.cancel.generic.flags |= QMF_INACTIVE;
	uiTouchButtons.command.generic.flags |= QMF_INACTIVE;
	uiTouchButtons.texture.generic.flags |= QMF_INACTIVE;
	uiTouchButtons.sp.generic.flags |= QMF_INACTIVE;
	uiTouchButtons.mp.generic.flags |= QMF_INACTIVE;
	uiTouchButtons.lock.generic.flags |= QMF_INACTIVE;
	uiTouchButtons.additive.generic.flags |= QMF_INACTIVE;
	uiTouchButtons.editor.generic.flags |= QMF_INACTIVE;
}
static void UI_TouchButtons_EnableButtons()
{
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
	uiTouchButtons.cancel.generic.flags &= ~QMF_INACTIVE;
	uiTouchButtons.command.generic.flags &= ~QMF_INACTIVE;
	uiTouchButtons.texture.generic.flags &= ~QMF_INACTIVE;
	uiTouchButtons.sp.generic.flags &= ~QMF_INACTIVE;
	uiTouchButtons.mp.generic.flags &= ~QMF_INACTIVE;
	uiTouchButtons.lock.generic.flags &= ~QMF_INACTIVE;
	uiTouchButtons.additive.generic.flags &= ~QMF_INACTIVE;
	uiTouchButtons.editor.generic.flags &= ~QMF_INACTIVE;
}
static void UI_TouchButtons_FileDialogCallback( bool success )
{
	if( success )
	{
		strcpy( uiTouchButtons.texture.buffer, uiFileDialogGlobal.result );
		uiTouchButtons.textureid = PIC_Load(uiTouchButtons.texture.buffer);
	}
	UI_TouchButtons_EnableButtons();
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
	case ID_SP:
		if( uiTouchButtons.sp.enabled )
			uiTouchButtons.mp.enabled = false;
	case ID_MP:
		if( uiTouchButtons.mp.enabled )
			uiTouchButtons.sp.enabled = false;
    case ID_HIDE:
	case ID_ADDITIVE:
	case ID_LOCK:
		if( event == QM_PRESSED )
			((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_PRESSED;
		else ((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_FOCUS;
		// clean all flags that we may change
		uiTouchButtons.curflags &= ~ ( TOUCH_FL_HIDE | TOUCH_FL_NOEDIT | TOUCH_FL_MP | TOUCH_FL_SP | TOUCH_FL_DRAW_ADDITIVE );
		if( uiTouchButtons.mp.enabled )
			uiTouchButtons.curflags |= TOUCH_FL_MP;
		if( uiTouchButtons.sp.enabled )
			uiTouchButtons.curflags |= TOUCH_FL_SP;
		if( uiTouchButtons.hide.enabled )
			uiTouchButtons.curflags |= TOUCH_FL_HIDE;
		if( uiTouchButtons.lock.enabled )
			uiTouchButtons.curflags |= TOUCH_FL_NOEDIT;
		if( uiTouchButtons.additive.enabled )
			uiTouchButtons.curflags |= TOUCH_FL_DRAW_ADDITIVE;
		break;
	}

	if( event == QM_CHANGED )
	{
		switch( item->id )
		{
		case ID_TEXTURE:
			// update current texture
			if( uiTouchButtons.texture.buffer[0] && uiTouchButtons.texture.buffer[0] != '#' )
				uiTouchButtons.textureid = PIC_Load(uiTouchButtons.texture.buffer);
			else
				uiTouchButtons.textureid = 0;
			break;
		case ID_BUTTONLIST:
			UI_TouchButtons_UpdateFields();
			break;
		}
		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		CLIENT_COMMAND(0, "touch_writeconfig\n");
		UI_PopMenu();
		break;
	case ID_CANCEL:
		CLIENT_COMMAND(0, "touch_loadconfig\n");
		UI_PopMenu();
		break;
	case ID_RESET:
		UI_TouchButtons_DisableButtons();

        uiTouchButtons.msgBox.generic.flags &= ~QMF_HIDDEN;
        uiTouchButtons.promptMessage.generic.flags &= ~QMF_HIDDEN;
        uiTouchButtons.no.generic.flags &= ~QMF_HIDDEN;
        uiTouchButtons.yes.generic.flags &= ~QMF_HIDDEN;
        strcpy( uiTouchButtons.dialogText, "Reset all buttons?" );
        uiTouchButtons.dialogAction = UI_ResetButtons;
		break;
	case ID_DELETE:
		UI_TouchButtons_DisableButtons();


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
		UI_TouchButtons_EnableButtons();

        uiTouchButtons.msgBox.generic.flags |= QMF_HIDDEN;
        uiTouchButtons.promptMessage.generic.flags |= QMF_HIDDEN;
        uiTouchButtons.no.generic.flags |= QMF_HIDDEN;
        uiTouchButtons.yes.generic.flags |= QMF_HIDDEN;
		break;
	case ID_SAVE:
		if( strlen(uiTouchButtons.name.buffer) > 0)
		{
			char command[256];
			snprintf( command, 256, "touch_addbutton \"%s\" \"%s\" \"%s\"\n", uiTouchButtons.name.buffer,
					  uiTouchButtons.texture.buffer, uiTouchButtons.command.buffer );
			CLIENT_COMMAND(0, command);
			snprintf( command, 256, "touch_setflags \"%s\" %i\n", uiTouchButtons.name.buffer, uiTouchButtons.curflags );
			CLIENT_COMMAND(0, command);
			snprintf( command, 256, "touch_setcolor \"%s\" %d %d %d %d\n", uiTouchButtons.name.buffer, CURCOLOR1(red), CURCOLOR1(green), CURCOLOR1(blue),CURCOLOR1(alpha) );
			CLIENT_COMMAND(1, command);
			uiTouchButtons.name.buffer[0] = 0;
			uiTouchButtons.name.cursor = 0;
		}
		else
		{
			char command[256];
			snprintf( command, 256, "touch_settexture \"%s\" \"%s\"\n", uiTouchButtons.selectedName, uiTouchButtons.texture.buffer );
			CLIENT_COMMAND(0, command);
			snprintf( command, 256, "touch_setcommand \"%s\" \"%s\"\n", uiTouchButtons.selectedName, uiTouchButtons.command.buffer );
			CLIENT_COMMAND(0, command);
			snprintf( command, 256, "touch_setflags \"%s\" %i\n", uiTouchButtons.selectedName, uiTouchButtons.curflags );
			CLIENT_COMMAND(0, command);
			snprintf( command, 256, "touch_setcolor \"%s\" %d %d %d %d\n", uiTouchButtons.selectedName, CURCOLOR1(red), CURCOLOR1(green), CURCOLOR1(blue),CURCOLOR1(alpha) );
			CLIENT_COMMAND(1, command);
		}
		UI_TouchButtons_GetButtonList();
		break;
	case ID_EDITOR:
		UI_TouchEdit_Menu();
		break;
	case ID_SELECT:
		UI_TouchButtons_DisableButtons();
		uiFileDialogGlobal.npatterns = 7;
		strcpy( uiFileDialogGlobal.patterns[0], "touch/*.tga");
		strcpy( uiFileDialogGlobal.patterns[1], "touch_default/*.tga");
		strcpy( uiFileDialogGlobal.patterns[2], "gfx/touch/*");
		strcpy( uiFileDialogGlobal.patterns[3], "gfx/vgui/*");
		strcpy( uiFileDialogGlobal.patterns[4], "gfx/shell/*");
		strcpy( uiFileDialogGlobal.patterns[5], "*.tga");
		uiFileDialogGlobal.preview = true;
		uiFileDialogGlobal.valid = true;
		uiFileDialogGlobal.callback = UI_TouchButtons_FileDialogCallback;
		UI_FileDialog_Menu();
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
	uiTouchButtons.initialized = true;

	uiTouchButtons.background.generic.id = ID_BACKGROUND;
	uiTouchButtons.background.generic.type = QMTYPE_BITMAP;
	uiTouchButtons.background.generic.flags = QMF_INACTIVE;
	uiTouchButtons.background.generic.x = 0;
	uiTouchButtons.background.generic.y = 0;
	uiTouchButtons.background.generic.width = uiStatic.width;
	uiTouchButtons.background.generic.height = 768;
	uiTouchButtons.background.pic = ART_BACKGROUND;

	uiTouchButtons.banner.generic.id = ID_BANNER;
	uiTouchButtons.banner.generic.type = QMTYPE_BITMAP;
	uiTouchButtons.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiTouchButtons.banner.generic.x = UI_BANNER_POSX;
	uiTouchButtons.banner.generic.y = UI_BANNER_POSY - 70;
	uiTouchButtons.banner.generic.width = UI_BANNER_WIDTH;
	uiTouchButtons.banner.generic.height = UI_BANNER_HEIGHT;
	uiTouchButtons.banner.pic = ART_BANNER;
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

	uiTouchButtons.additive.generic.id = ID_ADDITIVE;
	uiTouchButtons.additive.generic.type = QMTYPE_CHECKBOX;
	uiTouchButtons.additive.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_MOUSEONLY|QMF_DROPSHADOW;
	uiTouchButtons.additive.generic.name = "Additive";
	uiTouchButtons.additive.generic.x = 384 - 72 + 400;
	uiTouchButtons.additive.generic.y = 470;
	uiTouchButtons.additive.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.additive.generic.statusText = "Set button additive draw mode";

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

	uiTouchButtons.save.generic.id = ID_SAVE;
	uiTouchButtons.save.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.save.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW | QMF_ACT_ONRELEASE;
	uiTouchButtons.save.generic.x = 384 - 72 + 320;
	uiTouchButtons.save.generic.y = 520;
	uiTouchButtons.save.generic.name = "Save";
	uiTouchButtons.save.generic.statusText = "Save as new button";
	uiTouchButtons.save.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.save.pic = PIC_Load("gfx/shell/btn_touch_save");

	uiTouchButtons.editor.generic.id = ID_EDITOR;
	uiTouchButtons.editor.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.editor.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW | QMF_ACT_ONRELEASE;
	uiTouchButtons.editor.generic.x = 384 - 72 + 320;
	uiTouchButtons.editor.generic.y = 580;
	uiTouchButtons.editor.generic.name = "Editor";
	uiTouchButtons.editor.generic.statusText = "Open interactive editor";
	uiTouchButtons.editor.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.editor.pic = PIC_Load("gfx/shell/btn_touch_editor");

	uiTouchButtons.select.generic.id = ID_SELECT;
	uiTouchButtons.select.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.select.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW | QMF_ACT_ONRELEASE;
	uiTouchButtons.select.generic.x = 500;
	uiTouchButtons.select.generic.y = 300;
	uiTouchButtons.select.generic.name = "Select";
	uiTouchButtons.select.generic.statusText = "Select texture from list";
	uiTouchButtons.select.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.select.pic = PIC_Load("gfx/shell/btn_touch_select");

	uiTouchButtons.name.generic.id = ID_NAME;
	uiTouchButtons.name.generic.type = QMTYPE_FIELD;
	uiTouchButtons.name.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.name.generic.name = "New Button:";
	uiTouchButtons.name.generic.x = 400;
	uiTouchButtons.name.generic.y = 520;
	uiTouchButtons.name.generic.width = 205;
	uiTouchButtons.name.generic.height = 32;
	uiTouchButtons.name.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.name.maxLength = 255;

	uiTouchButtons.command.generic.id = ID_COMMAND;
	uiTouchButtons.command.generic.type = QMTYPE_FIELD;
	uiTouchButtons.command.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.command.generic.name = "Command:";
	uiTouchButtons.command.generic.x = 400;
	uiTouchButtons.command.generic.y = 150;
	uiTouchButtons.command.generic.width = 205;
	uiTouchButtons.command.generic.height = 32;
	uiTouchButtons.command.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.command.maxLength = 255;

	uiTouchButtons.texture.generic.id = ID_TEXTURE;
	uiTouchButtons.texture.generic.type = QMTYPE_FIELD;
	uiTouchButtons.texture.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.texture.generic.name = "Texture:";
	uiTouchButtons.texture.generic.x = 400;
	uiTouchButtons.texture.generic.y = 250;
	uiTouchButtons.texture.generic.width = 205;
	uiTouchButtons.texture.generic.height = 32;
	uiTouchButtons.texture.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.texture.maxLength = 255;

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

	uiTouchButtons.reset.generic.id = ID_RESET;
	uiTouchButtons.reset.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.reset.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_DROPSHADOW | QMF_ACT_ONRELEASE;
	uiTouchButtons.reset.generic.name = "Reset";
	uiTouchButtons.reset.generic.x = 384 - 72 + 480;
	uiTouchButtons.reset.generic.y = 580;
	uiTouchButtons.reset.generic.callback = UI_TouchButtons_Callback;
	uiTouchButtons.reset.generic.statusText = "Reset touch to default state";
	uiTouchButtons.reset.pic = PIC_Load("gfx/shell/btn_touch_reset");

	uiTouchButtons.remove.generic.id = ID_DELETE;
	uiTouchButtons.remove.generic.type = QMTYPE_BM_BUTTON;
	uiTouchButtons.remove.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiTouchButtons.remove.generic.x = 384 - 72 + 480;
	uiTouchButtons.remove.generic.y = 520;
	uiTouchButtons.remove.generic.name = "Delete";
	uiTouchButtons.remove.generic.statusText = "Delete saved game";
	uiTouchButtons.remove.generic.callback = UI_TouchButtons_Callback;
	UI_UtilSetupPicButton( &uiTouchButtons.remove, PC_DELETE );

	uiTouchButtons.buttonList.itemNames = (const char **)uiTouchButtons.bNamesPtr;

	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.background );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.remove );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.reset );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.done );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.cancel );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.red );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.green );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.blue );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.alpha );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.hide );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.additive );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.sp );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.mp );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.lock );

	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.buttonList );

	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.save );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.select );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.editor );

	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.name );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.banner );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.command );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.texture );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.color );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.preview );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.msgBox );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.promptMessage );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.no );
	UI_AddItem( &uiTouchButtons.menu, (void *)&uiTouchButtons.yes );

	UI_TouchButtons_GetButtonList();
}

/*
=================
UI_TouchButtons_Precache
=================
*/
void UI_TouchButtons_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
	uiTouchButtons.gettingList = false; // prevent filling list before init
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
