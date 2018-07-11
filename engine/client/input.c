/*
input.c - win32 input devices
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#ifndef XASH_DEDICATED

#include "common.h"
#include "input.h"
#include "touch.h"
#include "client.h"
#include "vgui_draw.h"
#include "wrect.h"
#include "joyinput.h"

#ifdef XASH_SDL
#include <SDL.h>
#endif

#ifdef _WIN32
#include "windows.h"
#endif

Xash_Cursor*	in_mousecursor;
qboolean	in_mouseactive;				// false when not focus app
qboolean	in_mouseinitialized;
qboolean	in_mouse_suspended;
int	in_mouse_oldbuttonstate;
int	in_mouse_buttons;
static struct inputstate_s
{
	float lastpitch, lastyaw;
} inputstate;

extern convar_t *vid_fullscreen;

static byte scan_to_key[128] = 
{ 
	0,27,'1','2','3','4','5','6','7','8','9','0','-','=',K_BACKSPACE,9,
	'q','w','e','r','t','y','u','i','o','p','[',']', 13 , K_CTRL,
	'a','s','d','f','g','h','j','k','l',';','\'','`',
	K_SHIFT,'\\','z','x','c','v','b','n','m',',','.','/',K_SHIFT,
	'*',K_ALT,' ',K_CAPSLOCK,
	K_F1,K_F2,K_F3,K_F4,K_F5,K_F6,K_F7,K_F8,K_F9,K_F10,
	K_PAUSE,0,K_HOME,K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,
	K_RIGHTARROW,K_KP_PLUS,K_END,K_DOWNARROW,K_PGDN,K_INS,K_DEL,
	0,0,0,K_F11,K_F12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

convar_t *m_enginemouse;
convar_t *m_pitch;
convar_t *m_yaw;

convar_t *m_enginesens;
convar_t *m_ignore;
convar_t *cl_forwardspeed;
convar_t *cl_sidespeed;
convar_t *cl_backspeed;
convar_t *look_filter;
/*
=======
Host_MapKey

Map from windows to engine keynums
=======
*/
static int Host_MapKey( int key )
{
	int	result, modified;
	qboolean	is_extended = false;

	modified = ( key >> 16 ) & 255;
	if( modified > 127 ) return 0;

	if( key & ( 1U << 24 ))
		is_extended = true;

	result = scan_to_key[modified];

	if( !is_extended )
	{
		switch( result )
		{
		case K_HOME: return K_KP_HOME;
		case K_UPARROW: return K_KP_UPARROW;
		case K_PGUP: return K_KP_PGUP;
		case K_LEFTARROW: return K_KP_LEFTARROW;
		case K_RIGHTARROW: return K_KP_RIGHTARROW;
		case K_END: return K_KP_END;
		case K_DOWNARROW: return K_KP_DOWNARROW;
		case K_PGDN: return K_KP_PGDN;
		case K_INS: return K_KP_INS;
		case K_DEL: return K_KP_DEL;
		default: return result;
		}
	}
	else
	{
		switch( result )
		{
		case K_PAUSE: return K_KP_NUMLOCK;
		case 0x0D: return K_KP_ENTER;
		case 0x2F: return K_KP_SLASH;
		case 0xAF: return K_KP_PLUS;
		}
		return result;
	}
}

/*
===========
IN_StartupMouse
===========
*/

#ifdef USE_EVDEV

#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <dirent.h>
#define MAX_EVDEV_DEVICES 5

struct evdev_s
{
int initialized, devices;
int fds[MAX_EVDEV_DEVICES];
string paths[MAX_EVDEV_DEVICES];
qboolean grab;
float grabtime;
float x, y;
} evdev;

int KeycodeFromEvdev(int keycode, int value);

static void Evdev_CheckPermissions()
{
#ifdef __ANDROID__
	system( "su 0 chmod 664 /dev/input/event*" );
#endif
}

void Evdev_Setup( void )
{
	if( evdev.initialized )
		return;
#ifdef __ANDROID__
	system( "su 0 supolicy --live \"allow appdomain input_device dir { ioctl read getattr search open }\" \"allow appdomain input_device chr_file { ioctl read write getattr lock append open }\"" );
	system( "su 0 setenforce permissive" );
#endif
	evdev.initialized = true;
}

#define EV_HASBIT( x, y ) ( x[y / 32] & (1 << y % 32) )

void Evdev_Autodetect_f( void )
{
	int i;
	DIR *dir;
	struct dirent *entry;

	Evdev_Setup();

	Evdev_CheckPermissions();

	if( !( dir = opendir( "/dev/input" ) ) )
	    return;

	while( ( entry = readdir( dir ) ) )
	{
		int fd;
		string path;
		uint types[EV_MAX] = {0};
		uint codes[( KEY_MAX - 1 ) / 32 + 1] = {0};
		qboolean hasbtnmouse;

		if( evdev.devices >= MAX_EVDEV_DEVICES )
			continue;

		Q_snprintf( path, MAX_STRING, "/dev/input/%s", entry->d_name );

		for( i = 0; i < evdev.devices; i++ )
			if( !Q_strncmp( evdev.paths[i], path, MAX_STRING ) )
				goto next;

		if( Q_strncmp( entry->d_name, "event", 5 ) )
			continue;

		fd = open( path, O_RDONLY | O_NONBLOCK );

		if( fd == -1 )
			continue;

		ioctl( fd, EVIOCGBIT( 0, EV_MAX ), types );

		if( !EV_HASBIT( types, EV_KEY ) )
			goto close;

		ioctl( fd, EVIOCGBIT( EV_KEY, KEY_MAX ), codes );

		if( EV_HASBIT( codes, KEY_LEFTCTRL ) && EV_HASBIT( codes, KEY_SPACE ) )
			goto open;

		if( !EV_HASBIT( codes, BTN_MOUSE ) )
			goto close;

		if( EV_HASBIT( types, EV_REL ) )
		{
			Q_memset( codes, 0, sizeof( codes ) );
			ioctl( fd, EVIOCGBIT( EV_REL, KEY_MAX ), codes );

			if( !EV_HASBIT( codes, REL_X ) )
				goto close;

			if( !EV_HASBIT( codes, REL_Y ) )
				goto close;

			if( !EV_HASBIT( codes, REL_WHEEL ) )
				goto close;

			goto open;
		}
		goto close;
open:
		Q_strncpy( evdev.paths[evdev.devices], path, MAX_STRING );
		evdev.fds[evdev.devices++] = fd;
		Msg( "Opened device %s\n", path );
		goto next;
close:
		close( fd );
next:
		continue;
	}
	closedir( dir );

}

/*
===========
Evdev_OpenDevice

For shitty systems that cannot provide relative mouse axes
===========
*/
void Evdev_OpenDevice ( const char *path )
{
	int ret, i;

	if ( evdev.devices >= MAX_EVDEV_DEVICES )
	{
		Msg( "Only %d devices supported!\n", MAX_EVDEV_DEVICES );
		return;
	}

	Evdev_Setup();

	Evdev_CheckPermissions(); // use root to grant access to evdev

	for( i = 0; i < evdev.devices; i++ )
	{
		if( !Q_strncmp( evdev.paths[i], path, MAX_STRING ) )
		{
			Msg( "device %s already open!\n", path );
			return;
		}
	}

	ret = open( path, O_RDONLY | O_NONBLOCK );
	if( ret < 0 )
	{
		MsgDev( D_ERROR, "Could not open input device %s: %s\n", path, strerror( errno ) );
		return;
	}
	Msg( "Input device #%d: %s opened sucessfully\n", evdev.devices, path );
	evdev.fds[evdev.devices] = ret;
	Q_strncpy( evdev.paths[evdev.devices++], path, MAX_STRING );
}

void Evdev_OpenDevice_f( void )
{
	if( Cmd_Argc() < 2 )
		Msg( "Usage: evdev_opendevice <path>\n" );

	Evdev_OpenDevice( Cmd_Argv( 1 ) );
}

/*
===========
Evdev_CloseDevice_f
===========
*/
void Evdev_CloseDevice_f ( void )
{
	uint i;
	char *arg;

	if( Cmd_Argc() < 2 )
		return;

	arg = Cmd_Argv( 1 );

	if( Q_isdigit( arg ) )
		i = Q_atoi( arg );
	else for( i = 0; i < evdev.devices; i++ )
		if( !Q_strncmp( evdev.paths[i], arg, MAX_STRING ) )
			break;

	if( i >= evdev.devices )
	{
		Msg( "Device %s is not open\n", arg );
		return;
	}

	close( evdev.fds[i] );
	evdev.devices--;
	Msg( "Device %s closed successfully\n", evdev.paths[i] );

	for( ; i < evdev.devices; i++ )
	{
		Q_strncpy( evdev.paths[i], evdev.paths[i+1], MAX_STRING );
		evdev.fds[i] = evdev.fds[i+1];
	}
}

void IN_EvdevFrame ()
{
	int dx = 0, dy = 0, i;

	for( i = 0; i < evdev.devices; i++ )
	{
		struct input_event ev;

		while ( read( evdev.fds[i], &ev, 16) == 16 )
		{
			if ( ev.type == EV_REL )
			{
				switch ( ev.code )
				{
					case REL_X: dx += ev.value;
					break;

					case REL_Y: dy += ev.value;
					break;

					case REL_WHEEL:
					if( ev.value > 0)
					{
						Key_Event( K_MWHEELDOWN, 1 );
						Key_Event( K_MWHEELDOWN, 0 );
					}
					else
					{
						Key_Event( K_MWHEELUP, 1 );
						Key_Event( K_MWHEELUP, 0 );
					}
					break;
				}
			}
			else if ( ( ev.type == EV_KEY ) && cls.key_dest == key_game )
			{
				switch( ev.code )
				{
				case BTN_LEFT:
					Key_Event( K_MOUSE1, ev.value );
					break;
				case BTN_MIDDLE:
					Key_Event( K_MOUSE3, ev.value );
					break;
				case BTN_RIGHT:
					Key_Event( K_MOUSE2, ev.value );
					break;
				default:
					Key_Event ( KeycodeFromEvdev( ev.code, ev.value ) , ev.value);
				}
			}
		}

		if( evdev.grab && evdev.grabtime <= host.realtime )
		{
			ioctl( evdev.fds[i], EVIOCGRAB, (void*) 1 );
			Key_ClearStates();
		}

		if( m_ignore->integer )
			continue;
		
		evdev.x += -dx * m_yaw->value;
		evdev.y += dy * m_pitch->value;
	}
	if( evdev.grabtime <= host.realtime )
		evdev.grab = false;
}

void Evdev_SetGrab( qboolean grab )
{
	int i;

	if( grab )
	{
		Key_Event( K_ESCAPE, 0 ); //Do not leave ESC down
		evdev.grabtime = host.realtime + 0.5;
		Key_ClearStates();
	}
	else
	{
		for( i = 0; i < evdev.devices; i++ )
			ioctl( evdev.fds[i], EVIOCGRAB, (void*) 0 );
	}
	evdev.grab = grab;
}

void IN_EvdevMove( float *yaw, float *pitch )
{
	*yaw += evdev.x;
	*pitch += evdev.y;
	evdev.x = evdev.y = 0.0f;
}
#endif

void IN_StartupMouse( void )
{
	if( Host_IsDedicated() ) return;

	m_ignore = Cvar_Get( "m_ignore", DEFAULT_M_IGNORE, CVAR_ARCHIVE , "ignore mouse events" );

	m_enginemouse = Cvar_Get( "m_enginemouse", "0", CVAR_ARCHIVE, "read mouse events in engine instead of client" );
	m_enginesens = Cvar_Get( "m_enginesens", "0.3", CVAR_ARCHIVE, "mouse sensitivity, when m_enginemouse enabled" );
	m_pitch = Cvar_Get( "m_pitch", "0.022", CVAR_ARCHIVE, "mouse pitch value" );
	m_yaw = Cvar_Get( "m_yaw", "0.022", CVAR_ARCHIVE, "mouse yaw value" );
	look_filter = Cvar_Get( "look_filter", "0", CVAR_ARCHIVE, "filter look events making it smoother" );
	
	// You can use -nomouse argument to prevent using mouse from client
	// -noenginemouse will disable all mouse input
	if( Sys_CheckParm( "-noenginemouse" )) return; 

	in_mouse_buttons = 8;
	in_mouseinitialized = true;
}

static void IN_ActivateCursor( void )
{
	if( cls.key_dest == key_menu )
	{
#ifdef XASH_SDL
		SDL_SetCursor( in_mousecursor );
#endif
	}
}

void IN_SetCursor( Xash_Cursor* hCursor )
{
	in_mousecursor = hCursor;

	IN_ActivateCursor();
}

/*
===========
IN_ToggleClientMouse

Called when key_dest is changed
===========
*/
void IN_ToggleClientMouse( int newstate, int oldstate )
{
	if( newstate == oldstate ) return;

	if( oldstate == key_game )
	{
		if( cls.initialized )
			clgame.dllFuncs.IN_DeactivateMouse();
	}
	else if( newstate == key_game )
	{
		// reset mouse pos, so cancel effect in game
#ifdef XASH_SDL
		if(touch_enable->integer)
		{
			SDL_SetRelativeMouseMode( SDL_FALSE );
			SDL_SetWindowGrab( host.hWnd,  SDL_FALSE );
		}
		else
		{
			SDL_WarpMouseInWindow( host.hWnd, host.window_center_x, host.window_center_y );
			SDL_SetWindowGrab( host.hWnd, SDL_TRUE );
			if( clgame.dllFuncs.pfnLookEvent )
				SDL_SetRelativeMouseMode( SDL_TRUE );
		}
#endif
		if( cls.initialized )
			clgame.dllFuncs.IN_ActivateMouse();
	}

	if( ( newstate == key_menu || newstate == key_console || newstate == key_message ) && ( !CL_IsBackgroundMap() || CL_IsBackgroundDemo()))
	{
#ifdef XASH_SDL
		SDL_SetWindowGrab(host.hWnd, SDL_FALSE);
		if( clgame.dllFuncs.pfnLookEvent )
			SDL_SetRelativeMouseMode( SDL_FALSE );
#endif
#ifdef __ANDROID__
		Android_ShowMouse( true );
#endif
#ifdef USE_EVDEV
		Evdev_SetGrab( false );
#endif
	}
	else
	{
#ifdef __ANDROID__
		Android_ShowMouse( false );
#endif
#ifdef USE_EVDEV
		Evdev_SetGrab( true );
#endif
	}
}

/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void IN_ActivateMouse( qboolean force )
{
	static qboolean	oldstate;

	if( !in_mouseinitialized )
		return;

	if( CL_Active() && host.mouse_visible && !force )
		return;	// VGUI controls

	if( cls.key_dest == key_menu && vid_fullscreen && !vid_fullscreen->integer)
	{
		// check for mouse leave-entering
		if( !in_mouse_suspended && !UI_MouseInRect( ))
			in_mouse_suspended = true;

		if( oldstate != in_mouse_suspended )
		{
			if( in_mouse_suspended )
			{
#ifdef XASH_SDL
				SDL_ShowCursor( false );
#endif
				UI_ShowCursor( false );
			}
		}

		oldstate = in_mouse_suspended;

		if( in_mouse_suspended )
		{
			in_mouse_suspended = false;
			in_mouseactive = false; // re-initialize mouse
			UI_ShowCursor( true );
		}
	}

	if( in_mouseactive ) return;
	in_mouseactive = true;

	if( UI_IsVisible( )) return;

	if( cls.key_dest == key_game )
	{
		clgame.dllFuncs.IN_ActivateMouse();
#ifdef XASH_SDL
		SDL_GetRelativeMouseState( 0, 0 ); // Reset mouse position
#endif
	}

}

/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
void IN_DeactivateMouse( void )
{
	if( !in_mouseinitialized || !in_mouseactive )
		return;

	if( cls.key_dest == key_game && cls.initialized )
	{
		clgame.dllFuncs.IN_DeactivateMouse();
	}
	in_mouseactive = false;
#ifdef XASH_SDL
	SDL_SetWindowGrab( host.hWnd, SDL_FALSE );
#endif
}

/*
================
IN_Mouse
================
*/
void IN_MouseMove( void )
{
	POINT	current_pos = {0, 0};
	
	if( !in_mouseinitialized || !in_mouseactive || m_ignore->integer )
		return;

	// find mouse movement
#ifdef XASH_SDL
	SDL_GetMouseState( &current_pos.x, &current_pos.y );
#endif

	VGui_MouseMove( current_pos.x, current_pos.y );

	if( !UI_IsVisible() )
		return;

	// Show cursor in UI
#ifdef XASH_SDL
	if( UI_IsVisible() )
		SDL_ShowCursor( SDL_TRUE );
#endif

	// if the menu is visible, move the menu cursor
	UI_MouseMove( current_pos.x, current_pos.y );

	IN_ActivateCursor();
}

/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent( int mstate )
{
	int	i;

	if( !in_mouseinitialized || !in_mouseactive )
		return;

	if( m_ignore->integer )
		return;

	if( cls.key_dest == key_game )
	{
#if defined( XASH_SDL )
		static qboolean ignore; // igonre mouse warp event
		int x, y;
		SDL_GetMouseState(&x, &y);
		if( host.mouse_visible )
			SDL_ShowCursor( SDL_TRUE );
		else
			SDL_ShowCursor( SDL_FALSE );

		if( x < host.window_center_x / 2 ||
			y < host.window_center_y / 2 ||
			x > host.window_center_x + host.window_center_x / 2 ||
			y > host.window_center_y + host.window_center_y / 2 )
		{
			SDL_WarpMouseInWindow(host.hWnd, host.window_center_x, host.window_center_y);
			ignore = 1; // next mouse event will be mouse warp
			return;
		}

		if ( !ignore )
		{
			if( !m_enginemouse->integer )
				clgame.dllFuncs.IN_MouseEvent( mstate );
		}
		else
		{
			SDL_GetRelativeMouseState( 0, 0 ); // reset relative state
			ignore = 0;
		}
#endif
		return;
	}
	else
	{
#if defined(XASH_SDL) && !defined(_WIN32)
		SDL_SetRelativeMouseMode( SDL_FALSE );
		SDL_ShowCursor( SDL_TRUE );
#endif
		IN_MouseMove();
	}

	// perform button actions
	for( i = 0; i < in_mouse_buttons; i++ )
	{
		if(( mstate & ( 1U << i )) && !( in_mouse_oldbuttonstate & ( 1U << i )))
		{
			Key_Event( K_MOUSE1 + i, true );
		}

		if(!( mstate & ( 1U << i )) && ( in_mouse_oldbuttonstate & ( 1U << i )))
		{
			Key_Event( K_MOUSE1 + i, false );
		}
	}	

	in_mouse_oldbuttonstate = mstate;
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown( void )
{
	IN_DeactivateMouse( );

#ifdef USE_EVDEV
	Cmd_RemoveCommand( "evdev_open" );
	Cmd_RemoveCommand( "evdev_close" );
	Cmd_RemoveCommand( "evdev_autodetect" );
#endif
}


/*
===========
IN_Init
===========
*/
void IN_Init( void )
{
	IN_StartupMouse( );

	cl_forwardspeed	= Cvar_Get( "cl_forwardspeed", "400", CVAR_ARCHIVE | CVAR_CLIENTDLL, "Default forward move speed" );
	cl_backspeed	= Cvar_Get( "cl_backspeed", "400", CVAR_ARCHIVE | CVAR_CLIENTDLL, "Default back move speed"  );
	cl_sidespeed	= Cvar_Get( "cl_sidespeed", "400", CVAR_ARCHIVE | CVAR_CLIENTDLL, "Default side move speed"  );

	if( !Host_IsDedicated() )
		Joy_Init(); // common joystick support init

#ifdef USE_EVDEV
	Cmd_AddCommand ("evdev_open", Evdev_OpenDevice_f, "Open event device");
	Cmd_AddCommand ("evdev_close", Evdev_CloseDevice_f, "Close event device");
	Cmd_AddCommand ("evdev_autodetect", Evdev_Autodetect_f, "Automaticly open mouses and keyboards");
#endif
}

/*
================
IN_JoyMove

Common function for engine joystick movement

	-1 < forwardmove < 1,	-1 < sidemove < 1

================
*/

#define F (1U << 0)	// Forward
#define B (1U << 1)	// Back
#define L (1U << 2)	// Left
#define R (1U << 3)	// Right
#define T (1U << 4)	// Forward stop
#define S (1U << 5)	// Side stop
void IN_JoyAppendMove( usercmd_t *cmd, float forwardmove, float sidemove )
{
	static uint moveflags = T | S;
		
	if( forwardmove ) cmd->forwardmove  = forwardmove * cl_forwardspeed->value;
	if( sidemove ) cmd->sidemove  = sidemove * cl_sidespeed->value;
	
	if( forwardmove )
		moveflags &= ~T;
	else if( !( moveflags & T ) )
	{
		Cmd_ExecuteString("-back", src_command );
		Cmd_ExecuteString("-forward", src_command );
		moveflags |= T;
	}
	if( sidemove )
		moveflags &= ~S;
	else if( !( moveflags & S ) )
	{
		Cmd_ExecuteString("-moveleft", src_command );
		Cmd_ExecuteString("-moveright", src_command );
		moveflags |= S;
	}

	if ( forwardmove > 0.7 && !( moveflags & F ))
	{
		moveflags |= F;
		Cmd_ExecuteString( "+forward", src_command );
	}
	else if ( forwardmove < 0.7 && ( moveflags & F ))
	{
		moveflags &= ~F;
		Cmd_ExecuteString( "-forward", src_command );
	}
	if ( forwardmove < -0.7 && !( moveflags & B ))
	{
		moveflags |= B;
		Cmd_ExecuteString( "+back", src_command );
	}
	else if ( forwardmove > -0.7 && ( moveflags & B ))
	{
		moveflags &= ~B;
		Cmd_ExecuteString( "-back", src_command );
	}
	if ( sidemove > 0.9 && !( moveflags & R ))
	{
		moveflags |= R;
		Cmd_ExecuteString( "+moveright", src_command );
	}
	else if ( sidemove < 0.9 && ( moveflags & R ))
	{
		moveflags &= ~R;
		Cmd_ExecuteString( "-moveright", src_command );
	}
	if ( sidemove < -0.9 && !( moveflags & L ))
	{
		moveflags |= L;
		Cmd_ExecuteString( "+moveleft", src_command );
	}
	else if ( sidemove > -0.9 && ( moveflags & L ))
	{
		moveflags &= ~L;
		Cmd_ExecuteString( "-moveleft", src_command );
	}
}

/*
================
IN_EngineAppendMove

Called from cl_main.c after generating command in client
================
*/
void IN_EngineAppendMove( float frametime, usercmd_t *cmd, qboolean active )
{
	float forward, side, dpitch, dyaw;

	if( clgame.dllFuncs.pfnLookEvent )
		return;

	if( cls.key_dest != key_game || cl.refdef.paused || cl.refdef.intermission )
		return;

	forward = side = dpitch = dyaw = 0;

	if(active)
	{
		float sensitivity = ( (float)cl.refdef.fov_x / (float)90.0f );
#if XASH_INPUT == INPUT_SDL
		if( m_enginemouse->integer && !m_ignore->integer )
		{
			int mouse_x, mouse_y;
			SDL_GetRelativeMouseState( &mouse_x, &mouse_y );
			cl.refdef.cl_viewangles[PITCH] += mouse_y * m_pitch->value * sensitivity;
			cl.refdef.cl_viewangles[YAW] -= mouse_x * m_yaw->value * sensitivity;
		}
#endif
#ifdef __ANDROID__
		if( !m_ignore->integer )
		{
			float mouse_x, mouse_y;
			Android_MouseMove( &mouse_x, &mouse_y );
			cl.refdef.cl_viewangles[PITCH] += mouse_y * m_pitch->value * sensitivity;
			cl.refdef.cl_viewangles[YAW] -= mouse_x * m_yaw->value * sensitivity;
		}
#endif
		Joy_FinalizeMove( &forward, &side, &dyaw, &dpitch );
		Touch_GetMove( &forward, &side, &dyaw, &dpitch );
		IN_JoyAppendMove( cmd, forward, side );
#ifdef USE_EVDEV
		IN_EvdevMove( &dyaw, &dpitch );
#endif
		if( look_filter->integer )
		{
			dpitch = ( inputstate.lastpitch + dpitch ) / 2;
			dyaw = ( inputstate.lastyaw + dyaw ) / 2;
			inputstate.lastpitch = dpitch;
			inputstate.lastyaw = dyaw;
		}

		cl.refdef.cl_viewangles[YAW] += dyaw * sensitivity;
		cl.refdef.cl_viewangles[PITCH] += dpitch * sensitivity;
		cl.refdef.cl_viewangles[PITCH] = bound( -90, cl.refdef.cl_viewangles[PITCH], 90 );
	}
}
/*
==================
Host_InputFrame

Called every frame, even if not generating commands
==================
*/
void Host_InputFrame( void )
{
	qboolean	shutdownMouse = false;
	float forward = 0, side = 0, pitch = 0, yaw = 0;

#ifdef USE_EVDEV
	IN_EvdevFrame();
#endif
	if( clgame.dllFuncs.pfnLookEvent )
	{
		int dx, dy;

#if XASH_INPUT == INPUT_SDL
		if( in_mouseinitialized && !m_ignore->integer )
		{
			SDL_GetRelativeMouseState( &dx, &dy );
			pitch += dy * m_pitch->value, yaw -= dx * m_yaw->value; //mouse speed
		}
#endif

#ifdef __ANDROID__
		if( !m_ignore->integer )
		{
			float  mouse_x, mouse_y;
			Android_MouseMove( &mouse_x, &mouse_y );
			pitch += mouse_y * m_pitch->value, yaw -= mouse_x * m_yaw->value; //mouse speed
		}
#endif

		Joy_FinalizeMove( &forward, &side, &yaw, &pitch );
		Touch_GetMove( &forward, &side, &yaw, &pitch );
#ifdef USE_EVDEV
		IN_EvdevMove( &yaw, &pitch );
#endif
		if( look_filter->integer )
		{
			pitch = ( inputstate.lastpitch + pitch ) / 2;
			yaw = ( inputstate.lastyaw + yaw ) / 2;
			inputstate.lastpitch = pitch;
			inputstate.lastyaw = yaw;
		}

		if( cls.key_dest == key_game )
		{
			clgame.dllFuncs.pfnLookEvent( yaw, pitch );
			clgame.dllFuncs.pfnMoveEvent( forward, side );
		}
	}
	Cbuf_Execute ();

	if( host.state == HOST_RESTART )
		host.state = HOST_FRAME; // restart is finished

	if( !in_mouseinitialized )
		return;

	if( host.state != HOST_FRAME )
	{
		IN_DeactivateMouse();
		return;
	}

	if( cl.refdef.paused && cls.key_dest == key_game )
		shutdownMouse = true; // release mouse during pause or console typeing
	
	if( shutdownMouse && !vid_fullscreen->integer )
	{
		IN_DeactivateMouse();
		return;
	}

	IN_ActivateMouse( false );

	IN_MouseMove();
}
#endif
