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
#include "menu_strings.h"

#define ART_BANNER		"gfx/shell/head_newgame"

#define ID_BACKGROUND	0
#define ID_BANNER		1

#define ID_EASY	  	2
#define ID_MEDIUM	  	3
#define ID_DIFFICULT  	4
#define ID_CANCEL		5

#define ID_MSGBOX	 	6
#define ID_MSGTEXT	 	7
#define ID_YES		130
#define ID_NO	 	131

typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuPicButton_s	easy;
	menuPicButton_s	medium;
	menuPicButton_s	hard;
	menuPicButton_s	cancel;

	// newgame prompt dialog
	menuAction_s	msgBox;
	menuAction_s	dlgMessage1;
	menuPicButton_s	yes;
	menuPicButton_s	no;

	float		skill;

} uiNewGame_t;

static uiNewGame_t	uiNewGame;

/*
=================
UI_NewGame_StartGame
=================
*/
static void UI_NewGame_StartGame( float skill )
{
	if( CVAR_GET_FLOAT( "host_serverstate" ) && CVAR_GET_FLOAT( "maxplayers" ) > 1 )
		HOST_ENDGAME( "end of the game" );

	CVAR_SET_FLOAT( "skill", skill );
	CVAR_SET_FLOAT( "deathmatch", 0.0f );
	CVAR_SET_FLOAT( "teamplay", 0.0f );
	CVAR_SET_FLOAT( "pausable", 1.0f ); // singleplayer is always allowing pause
	CVAR_SET_FLOAT( "maxplayers", 1.0f );
	CVAR_SET_FLOAT( "coop", 0.0f );

	BACKGROUND_TRACK( NULL, NULL );

	CLIENT_COMMAND( FALSE, "newgame\n" );
}

static void UI_PromptDialog( float skill )
{
	if ( CL_IsActive( ) == FALSE )
	{
		UI_NewGame_StartGame( skill );
		return;
	}
	
	uiNewGame.skill = skill;

	// toggle main menu between active\inactive
	// show\hide quit dialog
	uiNewGame.easy.generic.flags ^= QMF_INACTIVE; 
	uiNewGame.medium.generic.flags ^= QMF_INACTIVE;
	uiNewGame.hard.generic.flags ^= QMF_INACTIVE;
	uiNewGame.cancel.generic.flags ^= QMF_INACTIVE;

	uiNewGame.msgBox.generic.flags ^= QMF_HIDDEN;
	uiNewGame.dlgMessage1.generic.flags ^= QMF_HIDDEN;
	uiNewGame.no.generic.flags ^= QMF_HIDDEN;
	uiNewGame.yes.generic.flags ^= QMF_HIDDEN;

}

/*
=================
UI_NewGame_KeyFunc
=================
*/
static const char *UI_NewGame_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE && !( uiNewGame.dlgMessage1.generic.flags & QMF_HIDDEN ))
	{
		UI_PromptDialog( 0.0f );	// clear skill
		return uiSoundNull;
	}
	return UI_DefaultKey( &uiNewGame.menu, key, down );
}

/*
=================
UI_NewGame_Callback
=================
*/
static void UI_NewGame_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_EASY:
		UI_PromptDialog( 1.0f );
		break;
	case ID_MEDIUM:
		UI_PromptDialog( 2.0f );
		break;
	case ID_DIFFICULT:
		UI_PromptDialog( 3.0f );
		break;
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_YES:
		UI_NewGame_StartGame( uiNewGame.skill );
		break;
	case ID_NO:
		UI_PromptDialog( 1.0f ); // clear skill
		break;
	}
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
UI_NewGame_Init
=================
*/
static void UI_NewGame_Init( void )
{
	memset( &uiNewGame, 0, sizeof( uiNewGame_t ));

	uiNewGame.menu.vidInitFunc = UI_NewGame_Init;
	uiNewGame.menu.keyFunc = UI_NewGame_KeyFunc;
	
	uiNewGame.background.generic.id = ID_BACKGROUND;
	uiNewGame.background.generic.type = QMTYPE_BITMAP;
	uiNewGame.background.generic.flags = QMF_INACTIVE;
	uiNewGame.background.generic.x = 0;
	uiNewGame.background.generic.y = 0;
	uiNewGame.background.generic.width = 1024;
	uiNewGame.background.generic.height = 768;
	uiNewGame.background.pic = ART_BACKGROUND;

	uiNewGame.banner.generic.id = ID_BANNER;
	uiNewGame.banner.generic.type = QMTYPE_BITMAP;
	uiNewGame.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiNewGame.banner.generic.x = UI_BANNER_POSX;
	uiNewGame.banner.generic.y = UI_BANNER_POSY;
	uiNewGame.banner.generic.width = UI_BANNER_WIDTH;
	uiNewGame.banner.generic.height = UI_BANNER_HEIGHT;
	uiNewGame.banner.pic = ART_BANNER;

	uiNewGame.easy.generic.id = ID_EASY;
	uiNewGame.easy.generic.type = QMTYPE_BM_BUTTON;
	uiNewGame.easy.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiNewGame.easy.generic.name = "Easy";
	uiNewGame.easy.generic.statusText = MenuStrings[HINT_SKILL_EASY];
	uiNewGame.easy.generic.x = 72;
	uiNewGame.easy.generic.y = 230;
	uiNewGame.easy.generic.callback = UI_NewGame_Callback;

	UI_UtilSetupPicButton( &uiNewGame.easy, PC_EASY );

	uiNewGame.medium.generic.id = ID_MEDIUM;
	uiNewGame.medium.generic.type = QMTYPE_BM_BUTTON;
	uiNewGame.medium.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiNewGame.medium.generic.name = "Medium";
	uiNewGame.medium.generic.statusText = MenuStrings[HINT_SKILL_NORMAL];
	uiNewGame.medium.generic.x = 72;
	uiNewGame.medium.generic.y = 280;
	uiNewGame.medium.generic.callback = UI_NewGame_Callback;
	
	UI_UtilSetupPicButton( &uiNewGame.medium, PC_MEDIUM );

	uiNewGame.hard.generic.id = ID_DIFFICULT;
	uiNewGame.hard.generic.type = QMTYPE_BM_BUTTON;
	uiNewGame.hard.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiNewGame.hard.generic.name = "Difficult";
	uiNewGame.hard.generic.statusText = MenuStrings[HINT_SKILL_HARD];
	uiNewGame.hard.generic.x = 72;
	uiNewGame.hard.generic.y = 330;
	uiNewGame.hard.generic.callback = UI_NewGame_Callback;

	UI_UtilSetupPicButton( &uiNewGame.hard, PC_DIFFICULT );

	uiNewGame.cancel.generic.id = ID_CANCEL;
	uiNewGame.cancel.generic.type = QMTYPE_BM_BUTTON;
	uiNewGame.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiNewGame.cancel.generic.name = "Cancel";
	uiNewGame.cancel.generic.statusText = "Go back to the main Menu";
	uiNewGame.cancel.generic.x = 72;
	uiNewGame.cancel.generic.y = 380;
	uiNewGame.cancel.generic.callback = UI_NewGame_Callback;

	UI_UtilSetupPicButton( &uiNewGame.cancel, PC_CANCEL );

	uiNewGame.msgBox.generic.id = ID_MSGBOX;
	uiNewGame.msgBox.generic.type = QMTYPE_ACTION;
	uiNewGame.msgBox.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiNewGame.msgBox.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiNewGame.msgBox.generic.x = 192;
	uiNewGame.msgBox.generic.y = 256;
	uiNewGame.msgBox.generic.width = 640;
	uiNewGame.msgBox.generic.height = 256;

	uiNewGame.dlgMessage1.generic.id = ID_MSGTEXT;
	uiNewGame.dlgMessage1.generic.type = QMTYPE_ACTION;
	uiNewGame.dlgMessage1.generic.flags = QMF_INACTIVE|QMF_HIDDEN|QMF_DROPSHADOW|QMF_CENTER_JUSTIFY;
	uiNewGame.dlgMessage1.generic.name = MenuStrings[HINT_RESTART_GAME];
	uiNewGame.dlgMessage1.generic.x = 192;
	uiNewGame.dlgMessage1.generic.y = 280;
	uiNewGame.dlgMessage1.generic.width = 640;
	uiNewGame.dlgMessage1.generic.height = 256;

	uiNewGame.yes.generic.id = ID_YES;
	uiNewGame.yes.generic.type = QMTYPE_BM_BUTTON;
	uiNewGame.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_HIDDEN|QMF_DROPSHADOW;
	uiNewGame.yes.generic.name = "Ok";
	uiNewGame.yes.generic.x = 380;
	uiNewGame.yes.generic.y = 460;
	uiNewGame.yes.generic.callback = UI_NewGame_Callback;

	UI_UtilSetupPicButton( &uiNewGame.yes, PC_OK );

	uiNewGame.no.generic.id = ID_NO;
	uiNewGame.no.generic.type = QMTYPE_BM_BUTTON;
	uiNewGame.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_HIDDEN|QMF_DROPSHADOW;
	uiNewGame.no.generic.name = "Cancel";
	uiNewGame.no.generic.x = 530;
	uiNewGame.no.generic.y = 460;
	uiNewGame.no.generic.callback = UI_NewGame_Callback;

	UI_UtilSetupPicButton( &uiNewGame.no, PC_CANCEL );

	UI_AddItem( &uiNewGame.menu, (void *)&uiNewGame.background );
	UI_AddItem( &uiNewGame.menu, (void *)&uiNewGame.banner );
	UI_AddItem( &uiNewGame.menu, (void *)&uiNewGame.easy );
	UI_AddItem( &uiNewGame.menu, (void *)&uiNewGame.medium );
	UI_AddItem( &uiNewGame.menu, (void *)&uiNewGame.hard );
	UI_AddItem( &uiNewGame.menu, (void *)&uiNewGame.cancel );
	UI_AddItem( &uiNewGame.menu, (void *)&uiNewGame.msgBox );
	UI_AddItem( &uiNewGame.menu, (void *)&uiNewGame.dlgMessage1 );
	UI_AddItem( &uiNewGame.menu, (void *)&uiNewGame.no );
	UI_AddItem( &uiNewGame.menu, (void *)&uiNewGame.yes );
}

/*
=================
UI_NewGame_Precache
=================
*/
void UI_NewGame_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_NewGame_Menu
=================
*/
void UI_NewGame_Menu( void )
{
	if( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	{
		// completely ignore save\load menus for multiplayer_only
		return;
	}

	if( !CheckGameDll( )) return;

	UI_NewGame_Precache();
	UI_NewGame_Init();

	UI_PushMenu( &uiNewGame.menu );
}