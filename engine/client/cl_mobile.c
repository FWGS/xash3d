/*
cl_mobile.c - common mobile interface
Copyright (C) 2015 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "common.h"
#include "client.h"
#include "mobility_int.h"
#include "events.h"
#include "library.h"
#include "gl_local.h"
#include "touch.h"
#if defined(__ANDROID__)
#define BELOKOCONTROLS

#include "platform/android/android-gameif.h"
#include "SDL_system.h"
#endif

// we don't have our controls at this time

touchbutton_t gButtons[TOUCH_ACT_MAX];
mobile_engfuncs_t *gMobileEngfuncs;
static inline void SET_BUTTON_SRECT( int handle, int x1, int y1, int x2, int y2 )
{
#ifdef BELOKOCONTROLS
// translates engine position to beloko touch controls position style
	gButtons[handle].sRect.left = x1 / 26;
	gButtons[handle].sRect.top = y1 / (26*(glState.height)/glState.width);
	gButtons[handle].sRect.right = x2 / 26;
	gButtons[handle].sRect.bottom = y2 / (26*(glState.height)/glState.width);
#else
	gButtons[handle].sRect.left = x1;
	gButtons[handle].sRect.top  = y1;
	gButtons[handle].sRect.right = x2;
	gButtons[handle].sRect.bottom = y2;
#endif
}



static void pfnVibrate( float life, char flags )
{
	// here goes platform-specific backends
#if defined(__ANDROID__)
	Android_Vibrate(life, flags);
#endif
}

static void pfnEnableTextInput( int enable )
{
#if defined(XASH_SDL)
	SDLash_EnableTextInput(enable);
#endif
}

static int Touch_RegisterButton( const char *cmdname )
{
	int i;
	for( i = TOUCH_ACT_USER; i < TOUCH_ACT_MAX; i++ )
	{
		if( gButtons[i].object == 0 )
		{
			gButtons[i].hButton = i;
			gButtons[i].pszCommand = Mem_Alloc( cls.mempool, strlen(cmdname) + 1 );
			Q_strncpy( gButtons[i].pszCommand, cmdname, strlen(cmdname) + 1 );
			return i;
		}
	}

	// no free buttons
	return -1;
}

static touchbutton_t *Touch_AddCustomButton(int x1, int y1, int x2, int y2, int hButton, const char *pszImageName )
{
	SET_BUTTON_SRECT(hButton, x1, y1, x2, y2);

	gButtons[hButton].pszImageName = Mem_Alloc( cls.mempool, strlen(pszImageName) + 1);
	Q_strncpy( gButtons[hButton].pszImageName, pszImageName, strlen(pszImageName) + 1 );

#if defined(__ANDROID__)
	Android_AddButton( &gButtons[hButton] );
#else
	gButtons[hButton].object = NULL;
#endif
}

static touchbutton_t *Touch_GetButtonByID( int hButton )
{
	ASSERT( hButton > 0 && hButton <= TOUCH_ACT_MAX );

	return &gButtons[hButton];
}

static void Touch_RemoveButton( touchbutton_t *pButton )
{
#if defined(__ANDROID__)
	Android_RemoveButton( pButton );
#endif
}

static void Touch_EmitButton( int hButton )
{
#if defined(__ANDROID__)
	Android_EmitButton( hButton );
#endif
}

static void Touch_Disable( qboolean disable )
{

}

static mobile_engfuncs_t gpMobileEngfuncs =
{
	MOBILITY_API_VERSION,
	pfnVibrate,
	pfnEnableTextInput,

//beloko
	Touch_RegisterButton,
	Touch_AddCustomButton,
	Touch_GetButtonByID,
	Touch_RemoveButton,
	Touch_EmitButton,
	Touch_Disable,
	NULL,
// touch.c
	IN_TouchAddClientButton,
	IN_TouchAddDefaultButton,
	IN_TouchHideButtons,
	IN_TouchRemoveButton,
	IN_TouchSetClientOnly
};

void Mobile_Init( void )
{
	gMobileEngfuncs = &gpMobileEngfuncs;


	// find a mobility interface
	pfnMobilityInterface ExportToClient = Com_GetProcAddress(clgame.hInstance, MOBILITY_CLIENT_EXPORT);

	if( ExportToClient )
	{
		ExportToClient( gMobileEngfuncs );
	}
	else
	{
		MsgDev( D_INFO, "Mobility interface not found\n");
	}

	Q_memset( gButtons, 0, sizeof( touchbutton_t ) * TOUCH_ACT_MAX );

	gButtons[TOUCH_ACT_SHOW_NUMBERS].pszCommand = "";
	gButtons[TOUCH_ACT_QUICKSAVE].pszCommand = "savequick";
	gButtons[TOUCH_ACT_QUICKLOAD].pszCommand = "loadquick";
	gButtons[TOUCH_ACT_SHOOT_ALT].pszCommand = "+attack2";
	gButtons[TOUCH_ACT_INVNEXT	].pszCommand = "invnext";
	gButtons[TOUCH_ACT_INVPREV	].pszCommand = "invprev";
	gButtons[TOUCH_ACT_RELOAD	].pszCommand = "+reload";
	gButtons[TOUCH_ACT_CROUCH	].pszCommand = "+duck";
	gButtons[TOUCH_ACT_SHOOT	].pszCommand = "+attack";
	gButtons[TOUCH_ACT_LIGHT	].pszCommand = "impulse 100";
	gButtons[TOUCH_ACT_JUMP		].pszCommand = "+jump";
	gButtons[TOUCH_ACT_CHAT		].pszCommand = "messagemode";
	gButtons[TOUCH_ACT_USE		].pszCommand = "+use";

	gButtons[TOUCH_ACT_USERALIAS1].pszCommand = "_useralias1";
	gButtons[TOUCH_ACT_USERALIAS2].pszCommand = "_useralias2";
	gButtons[TOUCH_ACT_USERALIAS3].pszCommand = "_useralias3";


#if defined(__ANDROID__)
	Android_TouchInit(gButtons);
#endif
}

void Mobile_Destroy( void )
{

}
