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

//#include "platform/android/android-gameif.h"
#include "SDL_system.h"
#endif

// we don't have our controls at this time
mobile_engfuncs_t *gMobileEngfuncs;

convar_t *vibration_length;
convar_t *vibration_enable;

void Android_Vibrate( float life, char flags );

static void pfnVibrate( float life, char flags )
{
	if( !vibration_enable->value )
		return;

	if( life < 0.0f )
	{
		MsgDev( D_WARN, "Negative vibrate time: %f\n", life );
		return;
	}

	MsgDev( D_NOTE, "Vibrate: %f %d\n", life, flags );

	// here goes platform-specific backends
#ifdef __ANDROID__
	Android_Vibrate( life * vibration_length->value, flags );
#endif
}

static void Vibrate_f()
{
	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: vibrate <time>\n" );
		return;
	}

	pfnVibrate( Q_atof( Cmd_Argv(1) ), VIBRATE_NORMAL );
}

static void pfnEnableTextInput( int enable )
{
#if defined(XASH_SDL)
	SDLash_EnableTextInput(enable);
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
	IN_TouchAddClientButton,
	IN_TouchAddDefaultButton,
	IN_TouchHideButtons,
	IN_TouchRemoveButton,
	IN_TouchSetClientOnly,
	IN_TouchResetDefaultButtons
};

void Mobile_Init( void )
{
	// find a mobility interface
	pfnMobilityInterface ExportToClient = Com_GetProcAddress(clgame.hInstance, MOBILITY_CLIENT_EXPORT);

	gMobileEngfuncs = &gpMobileEngfuncs;

	if( ExportToClient )
	{
		ExportToClient( gMobileEngfuncs );
	}
	else
	{
		MsgDev( D_INFO, "Mobility interface not found\n");
	}

	Cmd_AddCommand( "vibrate", Vibrate_f, "Vibrate for specified time");
	vibration_length = Cvar_Get( "vibration_length", "1.0", CVAR_ARCHIVE, "Vibration length");
	vibration_enable = Cvar_Get( "vibration_enable", "1", CVAR_ARCHIVE, "Enable vibration");
}

void Mobile_Destroy( void )
{
	Cmd_RemoveCommand( "vibrate" );
}
