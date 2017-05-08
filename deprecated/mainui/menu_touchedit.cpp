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

/*
 * This is empty menu that allows engine to draw touch editor
 */

#include "extdll.h"
#include "basemenu.h"
#include "utils.h"
#include "keydefs.h"

typedef struct
{
	int		active;

	menuFramework_s	menu;
} uiTouchEdit_t;

static uiTouchEdit_t		uiTouchEdit;


/*
=================
UI_TouchEdit_DrawFunc
=================
*/
static void UI_TouchEdit_DrawFunc( void )
{
	if( !CVAR_GET_FLOAT("touch_in_menu") )
	{
		UI_PopMenu();
		UI_TouchButtons_GetButtonList();
	}
}

/*
=================
UI_TouchEdit_KeyFunc
=================
*/
static const char *UI_TouchEdit_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE )
	{
		CVAR_SET_STRING("touch_in_menu", "0");
		CLIENT_COMMAND(0, "touch_disableedit");
		UI_PopMenu();
		return uiSoundOut;
	}
	return uiSoundNull;
}

/*
=================
UI_TouchEdit_Init
=================
*/
static void UI_TouchEdit_Init( void )
{
	uiTouchEdit.menu.drawFunc = UI_TouchEdit_DrawFunc;
	uiTouchEdit.menu.keyFunc = UI_TouchEdit_KeyFunc;
	CVAR_SET_STRING("touch_in_menu", "1");
	CLIENT_COMMAND(0, "touch_enableedit");
}

/*
=================
UI_TouchEdit_Precache
=================
*/
void UI_TouchEdit_Precache( void )
{

}

/*
=================
UI_TouchEdit_Menu
=================
*/
void UI_TouchEdit_Menu( void )
{
	UI_TouchEdit_Precache();
	UI_TouchEdit_Init();

	UI_PushMenu( &uiTouchEdit.menu );
}
