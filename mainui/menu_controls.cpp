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

#define ART_BANNER		"gfx/shell/head_controls"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_DEFAULTS		2
#define ID_ADVANCED		3
#define ID_DONE		4
#define ID_CANCEL		5
#define ID_KEYLIST		6
#define ID_TABLEHINT	7
#define ID_MSGBOX1	 	8
#define ID_MSGBOX2	 	9
#define ID_MSGTEXT	 	10
#define ID_PROMPT	 	11
#define ID_YES	 	130
#define ID_NO	 	131

#define MAX_KEYS		256
#define CMD_LENGTH		38
#define KEY1_LENGTH		20+CMD_LENGTH
#define KEY2_LENGTH		20+KEY1_LENGTH

typedef struct
{
	char		keysBind[MAX_KEYS][CMD_LENGTH];
	char		firstKey[MAX_KEYS][20];
	char		secondKey[MAX_KEYS][20];
	char		keysDescription[MAX_KEYS][256];
	char		*keysDescriptionPtr[MAX_KEYS];

	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;
	menuPicButton_s	defaults;
	menuPicButton_s	advanced;
	menuPicButton_s	done;
	menuPicButton_s	cancel;

	// redefine key wait dialog
	menuAction_s	msgBox1;	// small msgbox
	menuAction_s	msgBox2;	// large msgbox
	menuAction_s	dlgMessage;
	menuAction_s	promptMessage;
	menuPicButton_s	yes;
	menuPicButton_s	no;

	menuScrollList_s	keysList;
	menuAction_s	hintMessage;
	char		hintText[MAX_HINT_TEXT];

	int		bind_grab;	// waiting for key input
} uiControls_t;

static uiControls_t		uiControls;
extern bool		hold_button_stack;

static void UI_ResetToDefaultsDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide reset to defaults dialog
	uiControls.defaults.generic.flags ^= QMF_INACTIVE; 
	uiControls.advanced.generic.flags ^= QMF_INACTIVE;
	uiControls.done.generic.flags ^= QMF_INACTIVE;
	uiControls.cancel.generic.flags ^= QMF_INACTIVE;

	uiControls.keysList.generic.flags ^= QMF_INACTIVE;

	uiControls.msgBox2.generic.flags ^= QMF_HIDDEN;
	uiControls.promptMessage.generic.flags ^= QMF_HIDDEN;
	uiControls.yes.generic.flags ^= QMF_HIDDEN;
	uiControls.no.generic.flags ^= QMF_HIDDEN;
}

/*
=================
UI_Controls_GetKeyBindings
=================
*/
static void UI_Controls_GetKeyBindings( const char *command, int *twoKeys )
{
	int		i, count = 0;
	const char	*b;

	twoKeys[0] = twoKeys[1] = -1;

	for( i = 0; i < 256; i++ )
	{
		b = KEY_GetBinding( i );
		if( !b ) continue;

		if( !stricmp( command, b ))
		{
			twoKeys[count] = i;
			count++;

			if( count == 2 ) break;
		}
	}

	// swap keys if needed
	if( twoKeys[0] != -1 && twoKeys[1] != -1 )
	{
		int tempKey = twoKeys[1];
		twoKeys[1] = twoKeys[0];
		twoKeys[0] = tempKey;
	}
}

void UI_UnbindCommand( const char *command )
{
	int i, l;
	const char *b;

	l = strlen( command );

	for( i = 0; i < 256; i++ )
	{
		b = KEY_GetBinding( i );
		if( !b ) continue;

		if( !strncmp( b, command, l ))
			KEY_SetBinding( i, "" );
	}
}

static void UI_Controls_ParseKeysList( void )
{
	char *afile = (char *)LOAD_FILE( "gfx/shell/kb_act.lst", NULL );
	char *pfile = afile;
	char token[1024];
	int i = 0;

	if( !afile )
	{
		for( ; i < MAX_KEYS; i++ ) uiControls.keysDescriptionPtr[i] = NULL;
		uiControls.keysList.itemNames = (const char **)uiControls.keysDescriptionPtr;
	
		Con_Printf( "UI_Parse_KeysList: kb_act.lst not found\n" );
		return;
	}

	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		char	str[128];

		if( !stricmp( token, "blank" ))
		{
			// seperator
			pfile = COM_ParseFile( pfile, token );
			if( !pfile ) break;	// technically an error

			sprintf( str, "^6%s^7", token );	// enable uiPromptTextColor
			StringConcat( uiControls.keysDescription[i], str, strlen( str ) + 1 );
			StringConcat( uiControls.keysDescription[i], uiEmptyString, 256 );	// empty
			uiControls.keysDescriptionPtr[i] = uiControls.keysDescription[i];
			strcpy( uiControls.keysBind[i], "" );
			strcpy( uiControls.firstKey[i], "" );
			strcpy( uiControls.secondKey[i], "" );
			i++;
		}
		else
		{
			// key definition
			int	keys[2];

			UI_Controls_GetKeyBindings( token, keys );
			strncpy( uiControls.keysBind[i], token, sizeof( uiControls.keysBind[i] ));

			pfile = COM_ParseFile( pfile, token );
			if( !pfile ) break; // technically an error

			sprintf( str, "^6%s^7", token );	// enable uiPromptTextColor

			if( keys[0] == -1 ) strcpy( uiControls.firstKey[i], "" );
			else strncpy( uiControls.firstKey[i], KEY_KeynumToString( keys[0] ), sizeof( uiControls.firstKey[i] ));

			if( keys[1] == -1 ) strcpy( uiControls.secondKey[i], "" ); 
			else strncpy( uiControls.secondKey[i], KEY_KeynumToString( keys[1] ), sizeof( uiControls.secondKey[i] ));

			StringConcat( uiControls.keysDescription[i], str, CMD_LENGTH );
			StringConcat( uiControls.keysDescription[i], uiEmptyString, CMD_LENGTH );

			// HACKHACK this color should be get from kb_keys.lst
			if( !strnicmp( uiControls.firstKey[i], "MOUSE", 5 ))
				sprintf( str, "^5%s^7", uiControls.firstKey[i] );	// cyan
			else sprintf( str, "^3%s^7", uiControls.firstKey[i] );	// yellow
			StringConcat( uiControls.keysDescription[i], str, KEY1_LENGTH );
			StringConcat( uiControls.keysDescription[i], uiEmptyString, KEY1_LENGTH );

			// HACKHACK this color should be get from kb_keys.lst
			if( !strnicmp( uiControls.secondKey[i], "MOUSE", 5 ))
				sprintf( str, "^5%s^7", uiControls.secondKey[i] );// cyan
			else sprintf( str, "^3%s^7", uiControls.secondKey[i] );	// yellow

			StringConcat( uiControls.keysDescription[i], str, KEY2_LENGTH );
			StringConcat( uiControls.keysDescription[i], uiEmptyString, KEY2_LENGTH );
			uiControls.keysDescriptionPtr[i] = uiControls.keysDescription[i];
			i++;
		}
	}

	FREE_FILE( afile );

	for( ; i < MAX_KEYS; i++ ) uiControls.keysDescriptionPtr[i] = NULL;
	uiControls.keysList.itemNames = (const char **)uiControls.keysDescriptionPtr;
}

static void UI_PromptDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide quit dialog
	uiControls.defaults.generic.flags ^= QMF_INACTIVE; 
	uiControls.advanced.generic.flags ^= QMF_INACTIVE;
	uiControls.done.generic.flags ^= QMF_INACTIVE;
	uiControls.cancel.generic.flags ^= QMF_INACTIVE;

	uiControls.keysList.generic.flags ^= QMF_INACTIVE;

	uiControls.msgBox1.generic.flags ^= QMF_HIDDEN;
	uiControls.dlgMessage.generic.flags ^= QMF_HIDDEN;
}

static void UI_Controls_RestartMenu( void )
{
	int lastSelectedKey = uiControls.keysList.curItem;
	int lastTopItem = uiControls.keysList.topItem;

	// HACK to prevent mismatch anim stack
	hold_button_stack = true;

	// restarts the menu
	UI_PopMenu();
	UI_Controls_Menu();

	hold_button_stack = false;

	// restore last key and top item
	uiControls.keysList.curItem = lastSelectedKey;
	uiControls.keysList.topItem = lastTopItem;
}

static void UI_Controls_ResetKeysList( void )
{
	char *afile = (char *)LOAD_FILE( "gfx/shell/kb_def.lst", NULL );
	char *pfile = afile;
	char token[1024];
	int i = 0;

	if( !afile )
	{
		Con_Printf( "UI_Parse_KeysList: kb_act.lst not found\n" );
		return;
	}

	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		char	key[32];

		strncpy( key, token, sizeof( key ));

		pfile = COM_ParseFile( pfile, token );
		if( !pfile ) break;	// technically an error

		char	cmd[128];

		if( key[0] == '\\' && key[1] == '\\' )
		{
			key[0] = '\\';
			key[1] = '\0';
		}

		UI_UnbindCommand( token );

		sprintf( cmd, "bind \"%s\" \"%s\"\n", key, token );
		CLIENT_COMMAND( TRUE, cmd );
	}

	FREE_FILE( afile );
	UI_Controls_RestartMenu ();
}

/*
=================
UI_Controls_KeyFunc
=================
*/
static const char *UI_Controls_KeyFunc( int key, int down )
{
	char	cmd[128];

	if( uiControls.msgBox1.generic.flags & QMF_HIDDEN )
	{
		if( down && key == K_ESCAPE && uiControls.defaults.generic.flags & QMF_INACTIVE )
		{
			UI_ResetToDefaultsDialog();
			return uiSoundNull;
		}
	}
	
	if( down )
	{
		if( uiControls.bind_grab )	// assume we are in grab-mode
		{
			// defining a key
			if( key == '`' || key == '~' )
			{
				return uiSoundBuzz;
			}
			else if( key != K_ESCAPE )
			{
				const char *bindName = uiControls.keysBind[uiControls.keysList.curItem];
				sprintf( cmd, "bind \"%s\" \"%s\"\n", KEY_KeynumToString( key ), bindName );
				CLIENT_COMMAND( TRUE, cmd );
			}

			uiControls.bind_grab = false;
			UI_Controls_RestartMenu();

			return uiSoundLaunch;
		}

		if( key == K_ENTER && uiControls.dlgMessage.generic.flags & QMF_HIDDEN )
		{
			if( !strlen( uiControls.keysBind[uiControls.keysList.curItem] ))
			{
				// probably it's a seperator
				return uiSoundBuzz;
			}

			// entering to grab-mode
			const char *bindName = uiControls.keysBind[uiControls.keysList.curItem];
			int keys[2];
	
			UI_Controls_GetKeyBindings( bindName, keys );
			if( keys[1] != -1 ) UI_UnbindCommand( bindName );
			uiControls.bind_grab = true;

			UI_PromptDialog();	// show prompt
			return uiSoundKey;
		}

		if(( key == K_BACKSPACE || key == K_DEL ) && uiControls.dlgMessage.generic.flags & QMF_HIDDEN )
		{
			// delete bindings

			if( !strlen( uiControls.keysBind[uiControls.keysList.curItem] ))
			{
				// probably it's a seperator
				return uiSoundNull;
			}

			const char *bindName = uiControls.keysBind[uiControls.keysList.curItem];
			UI_UnbindCommand( bindName );
			UI_StartSound( uiSoundRemoveKey );
			UI_Controls_RestartMenu();

			return uiSoundNull;
		}
	}
	return UI_DefaultKey( &uiControls.menu, key, down );
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

/*
=================
UI_Controls_Callback
=================
*/
static void UI_Controls_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_DEFAULTS:
	case ID_NO:
		UI_ResetToDefaultsDialog ();
		break;
	case ID_YES:
		UI_Controls_ResetKeysList ();
		break;
	case ID_ADVANCED:
		UI_AdvControls_Menu();
		break;
	}
}

/*
=================
UI_Controls_Init
=================
*/
static void UI_Controls_Init( void )
{
	memset( &uiControls, 0, sizeof( uiControls_t ));

	uiControls.menu.vidInitFunc = UI_Controls_Init;
	uiControls.menu.keyFunc = UI_Controls_KeyFunc;

	StringConcat( uiControls.hintText, "Action", CMD_LENGTH );
	StringConcat( uiControls.hintText, uiEmptyString, CMD_LENGTH-4 );
	StringConcat( uiControls.hintText, "Key/Button", KEY1_LENGTH );
	StringConcat( uiControls.hintText, uiEmptyString, KEY1_LENGTH-8 );
	StringConcat( uiControls.hintText, "Alternate", KEY2_LENGTH );
	StringConcat( uiControls.hintText, uiEmptyString, KEY2_LENGTH );

	uiControls.background.generic.id = ID_BACKGROUND;
	uiControls.background.generic.type = QMTYPE_BITMAP;
	uiControls.background.generic.flags = QMF_INACTIVE;
	uiControls.background.generic.x = 0;
	uiControls.background.generic.y = 0;
	uiControls.background.generic.width = 1024;
	uiControls.background.generic.height = 768;
	uiControls.background.pic = ART_BACKGROUND;

	uiControls.banner.generic.id = ID_BANNER;
	uiControls.banner.generic.type = QMTYPE_BITMAP;
	uiControls.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiControls.banner.generic.x = UI_BANNER_POSX;
	uiControls.banner.generic.y = UI_BANNER_POSY;
	uiControls.banner.generic.width = UI_BANNER_WIDTH;
	uiControls.banner.generic.height = UI_BANNER_HEIGHT;
	uiControls.banner.pic = ART_BANNER;

	uiControls.defaults.generic.id = ID_DEFAULTS;
	uiControls.defaults.generic.type = QMTYPE_BM_BUTTON;
	uiControls.defaults.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiControls.defaults.generic.x = 72;
	uiControls.defaults.generic.y = 230;
	uiControls.defaults.generic.name = "Use defaults";
	uiControls.defaults.generic.statusText = "Reset all buttons binding to their default values";
	uiControls.defaults.generic.callback = UI_Controls_Callback;

	UI_UtilSetupPicButton( &uiControls.defaults, PC_USE_DEFAULTS );

	uiControls.advanced.generic.id = ID_ADVANCED;
	uiControls.advanced.generic.type = QMTYPE_BM_BUTTON;
	uiControls.advanced.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiControls.advanced.generic.x = 72;
	uiControls.advanced.generic.y = 280;
	uiControls.advanced.generic.name = "Adv controls";
	uiControls.advanced.generic.statusText = "Change mouse sensitivity, enable autoaim, mouselook and crosshair";
	uiControls.advanced.generic.callback = UI_Controls_Callback;

	UI_UtilSetupPicButton( &uiControls.advanced, PC_ADV_CONTROLS );

	uiControls.done.generic.id = ID_DONE;
	uiControls.done.generic.type = QMTYPE_BM_BUTTON;
	uiControls.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiControls.done.generic.x = 72;
	uiControls.done.generic.y = 330;
	uiControls.done.generic.name = "Ok";
	uiControls.done.generic.statusText = "Save changes and return to configuration menu";
	uiControls.done.generic.callback = UI_Controls_Callback;

	UI_UtilSetupPicButton( &uiControls.done, PC_DONE );

	uiControls.cancel.generic.id = ID_CANCEL;
	uiControls.cancel.generic.type = QMTYPE_BM_BUTTON;
	uiControls.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiControls.cancel.generic.x = 72;
	uiControls.cancel.generic.y = 380;
	uiControls.cancel.generic.name = "Cancel";
	uiControls.cancel.generic.statusText = "Discard changes and return to configuration menu";
	uiControls.cancel.generic.callback = UI_Controls_Callback;

	UI_UtilSetupPicButton( &uiControls.cancel, PC_CANCEL );

	uiControls.hintMessage.generic.id = ID_TABLEHINT;
	uiControls.hintMessage.generic.type = QMTYPE_ACTION;
	uiControls.hintMessage.generic.flags = QMF_INACTIVE|QMF_SMALLFONT;
	uiControls.hintMessage.generic.color = uiColorHelp;
	uiControls.hintMessage.generic.name = uiControls.hintText;
	uiControls.hintMessage.generic.x = 360;
	uiControls.hintMessage.generic.y = 225;

	uiControls.keysList.generic.id = ID_KEYLIST;
	uiControls.keysList.generic.type = QMTYPE_SCROLLLIST;
	uiControls.keysList.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_SMALLFONT;
	uiControls.keysList.generic.x = 360;
	uiControls.keysList.generic.y = 255;
	uiControls.keysList.generic.width = 640;
	uiControls.keysList.generic.height = 440;
	uiControls.keysList.generic.callback = UI_Controls_Callback;

	UI_Controls_ParseKeysList();

	uiControls.msgBox1.generic.id = ID_MSGBOX1;
	uiControls.msgBox1.generic.type = QMTYPE_ACTION;
	uiControls.msgBox1.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiControls.msgBox1.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiControls.msgBox1.generic.x = 192;
	uiControls.msgBox1.generic.y = 256;
	uiControls.msgBox1.generic.width = 640;
	uiControls.msgBox1.generic.height = 128;

	uiControls.msgBox2.generic.id = ID_MSGBOX2;
	uiControls.msgBox2.generic.type = QMTYPE_ACTION;
	uiControls.msgBox2.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiControls.msgBox2.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiControls.msgBox2.generic.x = 192;
	uiControls.msgBox2.generic.y = 256;
	uiControls.msgBox2.generic.width = 640;
	uiControls.msgBox2.generic.height = 256;

	uiControls.dlgMessage.generic.id = ID_MSGTEXT;
	uiControls.dlgMessage.generic.type = QMTYPE_ACTION;
	uiControls.dlgMessage.generic.flags = QMF_INACTIVE|QMF_HIDDEN|QMF_DROPSHADOW;
	uiControls.dlgMessage.generic.name = "Press a key or button";
	uiControls.dlgMessage.generic.x = 320;
	uiControls.dlgMessage.generic.y = 280;

	uiControls.promptMessage.generic.id = ID_PROMPT;
	uiControls.promptMessage.generic.type = QMTYPE_ACTION;
	uiControls.promptMessage.generic.flags = QMF_INACTIVE|QMF_DROPSHADOW|QMF_HIDDEN;
	uiControls.promptMessage.generic.name = "Reset buttons to default?";
	uiControls.promptMessage.generic.x = 290;
	uiControls.promptMessage.generic.y = 280;

	uiControls.yes.generic.id = ID_YES;
	uiControls.yes.generic.type = QMTYPE_BM_BUTTON;
	uiControls.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiControls.yes.generic.name = "Ok";
	uiControls.yes.generic.x = 380;
	uiControls.yes.generic.y = 460;
	uiControls.yes.generic.callback = UI_Controls_Callback;

	UI_UtilSetupPicButton( &uiControls.yes, PC_OK );

	uiControls.no.generic.id = ID_NO;
	uiControls.no.generic.type = QMTYPE_BM_BUTTON;
	uiControls.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiControls.no.generic.name = "Cancel";
	uiControls.no.generic.x = 530;
	uiControls.no.generic.y = 460;
	uiControls.no.generic.callback = UI_Controls_Callback;

	UI_UtilSetupPicButton( &uiControls.no, PC_CANCEL );

	UI_AddItem( &uiControls.menu, (void *)&uiControls.background );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.banner );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.defaults );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.advanced );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.done );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.cancel );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.hintMessage );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.keysList );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.msgBox1 );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.msgBox2 );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.dlgMessage );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.promptMessage );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.no );
	UI_AddItem( &uiControls.menu, (void *)&uiControls.yes );
}

/*
=================
UI_Controls_Precache
=================
*/
void UI_Controls_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_Controls_Menu
=================
*/
void UI_Controls_Menu( void )
{
	UI_Controls_Precache();
	UI_Controls_Init();

	UI_PushMenu( &uiControls.menu );
}