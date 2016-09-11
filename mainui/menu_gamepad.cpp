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

#define ART_BANNER			"gfx/shell/head_gamepad"

#define ID_BACKGROUND 0
#define ID_BANNER 1

enum
{
	ID_DONE = 2,
	ID_RT_THRESHOLD,
	ID_LT_THRESHOLD,
	ID_SIDE,
	ID_FORWARD,
	ID_PITCH,
	ID_YAW,
	ID_INVERT_SIDE,
	ID_INVERT_FORWARD,
	ID_INVERT_PITCH,
	ID_INVERT_YAW,
	ID_AXIS_BIND1,
	ID_AXIS_BIND2,
	ID_AXIS_BIND3,
	ID_AXIS_BIND4,
	ID_AXIS_BIND5,
	ID_AXIS_BIND6
};

enum engineAxis_t
{
	JOY_AXIS_SIDE = 0,
	JOY_AXIS_FWD,
	JOY_AXIS_PITCH,
	JOY_AXIS_YAW,
	JOY_AXIS_RT,
	JOY_AXIS_LT,
	JOY_AXIS_NULL
};

static const char *axisNames[7] =
{
	"Side Movement",
	"Forward Movement",
	"Camera Vertical Turn",
	"Camera Horizontal Turn",
	"Right Trigger",
	"Left Trigger",
	"NOT BOUND"
};

typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuPicButton_s	done;
	menuSlider_s side, forward, pitch, yaw;
	menuCheckBox_s invSide, invFwd, invPitch, invYaw;

	menuSpinControl_s axisBind[6];

	menuAction_s axisBind_label;
} uiGamePad_t;

static uiGamePad_t		uiGamePad;

/*
=================
UI_GamePad_GetConfig
=================
*/
static void UI_GamePad_GetConfig( void )
{
	float side, forward, pitch, yaw;
	char binding[7] = { 0 };
	static char lt_threshold_text[8], rt_threshold_text[8];

	strncpy( binding, CVAR_GET_STRING( "joy_axis_binding"), sizeof( binding ));

	side = CVAR_GET_FLOAT( "joy_side" );
	forward = CVAR_GET_FLOAT( "joy_forward" );
	pitch = CVAR_GET_FLOAT( "joy_pitch" );
	yaw = CVAR_GET_FLOAT( "joy_yaw" );

	uiGamePad.side.curValue = fabs( side );
	uiGamePad.forward.curValue = fabs( forward );
	uiGamePad.pitch.curValue = fabs( pitch );
	uiGamePad.yaw.curValue = fabs( yaw );

	uiGamePad.invSide.enabled = side < 0.0f ? true: false;
	uiGamePad.invFwd.enabled = forward < 0.0f ? true: false;
	uiGamePad.invPitch.enabled = pitch < 0.0f ? true: false;
	uiGamePad.invYaw.enabled = yaw < 0.0f ? true: false;

	// I made a monster...
	for( int i = 0; i < sizeof( binding ) - 1; i++ )
	{
		switch( binding[i] )
		{
		case 's':
			uiGamePad.axisBind[i].generic.name = axisNames[JOY_AXIS_SIDE];
			uiGamePad.axisBind[i].curValue = JOY_AXIS_SIDE;
			break;
		case 'f':
			uiGamePad.axisBind[i].generic.name = axisNames[JOY_AXIS_FWD];
			uiGamePad.axisBind[i].curValue = JOY_AXIS_FWD;
			break;
		case 'p':
			uiGamePad.axisBind[i].generic.name = axisNames[JOY_AXIS_PITCH];
			uiGamePad.axisBind[i].curValue = JOY_AXIS_PITCH;
			break;
		case 'y':
			uiGamePad.axisBind[i].generic.name = axisNames[JOY_AXIS_YAW];
			uiGamePad.axisBind[i].curValue = JOY_AXIS_YAW;
			break;
		case 'r':
			uiGamePad.axisBind[i].generic.name = axisNames[JOY_AXIS_RT];
			uiGamePad.axisBind[i].curValue = JOY_AXIS_RT;
			break;
		case 'l':
			uiGamePad.axisBind[i].generic.name = axisNames[JOY_AXIS_LT];
			uiGamePad.axisBind[i].curValue = JOY_AXIS_LT;
			break;
		default:
			uiGamePad.axisBind[i].generic.name = axisNames[JOY_AXIS_NULL];
			uiGamePad.axisBind[i].curValue = JOY_AXIS_NULL;
		}
	}
}

/*
=================
UI_GamePad_SetConfig
=================
*/
static void UI_GamePad_SetConfig( void )
{
	float side, forward, pitch, yaw;
	char binding[7] = { 0 };

	side = uiGamePad.side.curValue;
	if( uiGamePad.invSide.enabled )
		side *= -1;

	forward = uiGamePad.forward.curValue;
	if( uiGamePad.invFwd.enabled )
		forward *= -1;

	pitch = uiGamePad.pitch.curValue;
	if( uiGamePad.invPitch.enabled )
		pitch *= -1;

	yaw = uiGamePad.yaw.curValue;
	if( uiGamePad.invYaw.enabled )
		yaw *= -1;

	for( int i = 0; i < 6; i++ )
	{
		switch( (int)uiGamePad.axisBind[i].curValue )
		{
		case JOY_AXIS_SIDE: binding[i]  = 's'; break;
		case JOY_AXIS_FWD: binding[i]   = 'f'; break;
		case JOY_AXIS_PITCH: binding[i] = 'p'; break;
		case JOY_AXIS_YAW: binding[i]   = 'y'; break;
		case JOY_AXIS_RT: binding[i]    = 'r'; break;
		case JOY_AXIS_LT: binding[i]    = 'l'; break;
		default: binding[i] = '0'; break;
		}
	}

	CVAR_SET_FLOAT( "joy_side", side );
	CVAR_SET_FLOAT( "joy_forward", forward );
	CVAR_SET_FLOAT( "joy_pitch", pitch );
	CVAR_SET_FLOAT( "joy_yaw", yaw );
	CVAR_SET_STRING( "joy_axis_binding", binding );
}

/*
=================
UI_GamePad_UpdateConfig
=================
*/
static void UI_GamePad_UpdateConfig( void )
{
	UI_GamePad_SetConfig();
	UI_GamePad_GetConfig();
}

/*
=================
UI_GamePad_Callback
=================
*/
static void UI_GamePad_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	switch( item->id )
	{
	case ID_INVERT_SIDE:
	case ID_INVERT_FORWARD:
	case ID_INVERT_PITCH:
	case ID_INVERT_YAW:
		if( event == QM_PRESSED )
			((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_PRESSED;
		else ((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_FOCUS;
		break;
	}

	if( event == QM_CHANGED )
	{
		UI_GamePad_UpdateConfig();
		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		UI_PopMenu();
		break;
	}
}

/*
=================
UI_GamePad_Init
=================
*/
static void UI_GamePad_Init( void )
{
	memset( &uiGamePad, 0, sizeof( uiGamePad_t ));

	uiGamePad.menu.vidInitFunc = UI_GamePad_Init;

	uiGamePad.background.generic.id	= ID_BACKGROUND;
	uiGamePad.background.generic.type = QMTYPE_BITMAP;
	uiGamePad.background.generic.flags = QMF_INACTIVE;
	uiGamePad.background.generic.x = 0;
	uiGamePad.background.generic.y = 0;
	uiGamePad.background.generic.width = uiStatic.width;
	uiGamePad.background.generic.height = 768;
	uiGamePad.background.pic = ART_BACKGROUND;

	uiGamePad.banner.generic.id = ID_BANNER;
	uiGamePad.banner.generic.type = QMTYPE_BITMAP;
	uiGamePad.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiGamePad.banner.generic.x = UI_BANNER_POSX;
	uiGamePad.banner.generic.y = UI_BANNER_POSY;
	uiGamePad.banner.generic.width = UI_BANNER_WIDTH;
	uiGamePad.banner.generic.height	= UI_BANNER_HEIGHT;
	uiGamePad.banner.pic = ART_BANNER;

	uiGamePad.done.generic.id = ID_DONE;
	uiGamePad.done.generic.type = QMTYPE_BM_BUTTON;
	uiGamePad.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiGamePad.done.generic.x = 72;
	uiGamePad.done.generic.y = 630;
	uiGamePad.done.generic.name = "Done";
	uiGamePad.done.generic.statusText = "Go back to the Configuration Menu";
	uiGamePad.done.generic.callback = UI_GamePad_Callback;

	UI_UtilSetupPicButton( &uiGamePad.done, PC_DONE );

	uiGamePad.axisBind_label.generic.type = QMTYPE_ACTION;
	uiGamePad.axisBind_label.generic.flags = QMF_CENTER_JUSTIFY|QMF_INACTIVE|QMF_DROPSHADOW;
	uiGamePad.axisBind_label.generic.x = 52;
	uiGamePad.axisBind_label.generic.y = 180;
	uiGamePad.axisBind_label.generic.color = uiColorHelp;
	uiGamePad.axisBind_label.generic.height = 26;
	uiGamePad.axisBind_label.generic.width = 200;
	uiGamePad.axisBind_label.generic.charHeight = 30;
	uiGamePad.axisBind_label.generic.charWidth = 17;
	uiGamePad.axisBind_label.generic.callback = UI_GamePad_Callback;
	uiGamePad.axisBind_label.generic.name = "Axis binding map";


	for( int i = 0, y = 230; i < 6; i++, y += 50 )
	{
		uiGamePad.axisBind[i].generic.id = ID_AXIS_BIND1 + i;
		uiGamePad.axisBind[i].generic.type = QMTYPE_SPINCONTROL;
		uiGamePad.axisBind[i].generic.flags = QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
		uiGamePad.axisBind[i].generic.x = 72;
		uiGamePad.axisBind[i].generic.y = y;
		uiGamePad.axisBind[i].generic.height = 26;
		uiGamePad.axisBind[i].generic.width = 256;
		uiGamePad.axisBind[i].generic.charWidth = 11;
		uiGamePad.axisBind[i].generic.charHeight = 22;
		uiGamePad.axisBind[i].generic.callback = UI_GamePad_Callback;
		uiGamePad.axisBind[i].generic.statusText = "Set axis binding";
		uiGamePad.axisBind[i].minValue = JOY_AXIS_SIDE;
		uiGamePad.axisBind[i].maxValue = JOY_AXIS_NULL;
		uiGamePad.axisBind[i].range = 1;
	}

	uiGamePad.side.generic.id = ID_SIDE;
	uiGamePad.side.generic.type = QMTYPE_SLIDER;
	uiGamePad.side.generic.flags = QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiGamePad.side.generic.x = 400;
	uiGamePad.side.generic.y = 250;
	uiGamePad.side.generic.callback = UI_GamePad_Callback;
	uiGamePad.side.generic.name = "Side";
	uiGamePad.side.generic.statusText = "Side movement sensitivity";
	uiGamePad.side.minValue = 0.0f;
	uiGamePad.side.maxValue = 2.0f;
	uiGamePad.side.range = 0.1f;

	uiGamePad.invSide.generic.id = ID_INVERT_SIDE;
	uiGamePad.invSide.generic.type = QMTYPE_CHECKBOX;
	uiGamePad.invSide.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_DROPSHADOW;
	uiGamePad.invSide.generic.name = "Invert";
	uiGamePad.invSide.generic.x = 620;
	uiGamePad.invSide.generic.y = 230;
	uiGamePad.invSide.generic.callback = UI_GamePad_Callback;
	uiGamePad.invSide.generic.statusText = "Invert side movement axis";

	uiGamePad.forward.generic.id = ID_FORWARD;
	uiGamePad.forward.generic.type = QMTYPE_SLIDER;
	uiGamePad.forward.generic.flags = QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiGamePad.forward.generic.x = 400;
	uiGamePad.forward.generic.y = 300;
	uiGamePad.forward.generic.callback = UI_GamePad_Callback;
	uiGamePad.forward.generic.name = "Forward";
	uiGamePad.forward.generic.statusText = "Forward movement sensitivity";
	uiGamePad.forward.minValue = 0.0f;
	uiGamePad.forward.maxValue = 2.0f;
	uiGamePad.forward.range = 0.1f;

	uiGamePad.invFwd.generic.id = ID_INVERT_FORWARD;
	uiGamePad.invFwd.generic.type = QMTYPE_CHECKBOX;
	uiGamePad.invFwd.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_DROPSHADOW;
	uiGamePad.invFwd.generic.name = "Invert";
	uiGamePad.invFwd.generic.x = 620;
	uiGamePad.invFwd.generic.y = 280;
	uiGamePad.invFwd.generic.callback = UI_GamePad_Callback;
	uiGamePad.invFwd.generic.statusText = "Invert forward movement axis";

	uiGamePad.pitch.generic.id = ID_PITCH;
	uiGamePad.pitch.generic.type = QMTYPE_SLIDER;
	uiGamePad.pitch.generic.flags = QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiGamePad.pitch.generic.x = 400;
	uiGamePad.pitch.generic.y = 350;
	uiGamePad.pitch.generic.callback = UI_GamePad_Callback;
	uiGamePad.pitch.generic.name = "Pitch";
	uiGamePad.pitch.generic.statusText = "Pitch rotating sensitivity";
	uiGamePad.pitch.minValue = 0.0f;
	uiGamePad.pitch.maxValue = 200.0f;
	uiGamePad.pitch.range = 1.0f;

	uiGamePad.invPitch.generic.id = ID_INVERT_PITCH;
	uiGamePad.invPitch.generic.type = QMTYPE_CHECKBOX;
	uiGamePad.invPitch.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_DROPSHADOW;
	uiGamePad.invPitch.generic.name = "Invert";
	uiGamePad.invPitch.generic.x = 620;
	uiGamePad.invPitch.generic.y = 330;
	uiGamePad.invPitch.generic.callback = UI_GamePad_Callback;
	uiGamePad.invPitch.generic.statusText = "Invert pitch axis";

	uiGamePad.yaw.generic.id = ID_YAW;
	uiGamePad.yaw.generic.type = QMTYPE_SLIDER;
	uiGamePad.yaw.generic.flags = QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiGamePad.yaw.generic.x = 400;
	uiGamePad.yaw.generic.y = 400;
	uiGamePad.yaw.generic.callback = UI_GamePad_Callback;
	uiGamePad.yaw.generic.name = "Yaw";
	uiGamePad.yaw.generic.statusText = "Yaw rotating sensitivity";
	uiGamePad.yaw.minValue = 0.0f;
	uiGamePad.yaw.maxValue = 200.0f;
	uiGamePad.yaw.range = 1.0f;

	uiGamePad.invYaw.generic.id = ID_INVERT_YAW;
	uiGamePad.invYaw.generic.type = QMTYPE_CHECKBOX;
	uiGamePad.invYaw.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_DROPSHADOW;
	uiGamePad.invYaw.generic.name = "Invert";
	uiGamePad.invYaw.generic.x = 620;
	uiGamePad.invYaw.generic.y = 380;
	uiGamePad.invYaw.generic.callback = UI_GamePad_Callback;
	uiGamePad.invYaw.generic.statusText = "Invert yaw axis";

	UI_GamePad_GetConfig();

	UI_AddItem( &uiGamePad.menu, &uiGamePad.background );
	UI_AddItem( &uiGamePad.menu, &uiGamePad.banner );
	UI_AddItem( &uiGamePad.menu, &uiGamePad.done );
	for( int i = 0; i < 6; i++ )
	{
		UI_AddItem( &uiGamePad.menu, &uiGamePad.axisBind[i] );
	}
	UI_AddItem( &uiGamePad.menu, &uiGamePad.side );
	UI_AddItem( &uiGamePad.menu, &uiGamePad.invSide );
	UI_AddItem( &uiGamePad.menu, &uiGamePad.forward );
	UI_AddItem( &uiGamePad.menu, &uiGamePad.invFwd );
	UI_AddItem( &uiGamePad.menu, &uiGamePad.pitch );
	UI_AddItem( &uiGamePad.menu, &uiGamePad.invPitch );
	UI_AddItem( &uiGamePad.menu, &uiGamePad.yaw );
	UI_AddItem( &uiGamePad.menu, &uiGamePad.invYaw );
	UI_AddItem( &uiGamePad.menu, &uiGamePad.axisBind_label );
}

/*
=================
UI_GamePad_Precache
=================
*/
void UI_GamePad_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_GamePad_Menu
=================
*/
void UI_GamePad_Menu( void )
{
	UI_GamePad_Precache();
	UI_GamePad_Init();

	UI_GamePad_UpdateConfig();
	UI_PushMenu( &uiGamePad.menu );
}
