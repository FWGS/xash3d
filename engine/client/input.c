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

int evdev_open, mouse_fd, evdev_dx, evdev_dy;

convar_t	*evdev_mousepath;
convar_t	*evdev_grab;


int KeycodeFromEvdev(int keycode, int value);

/*
===========
Evdev_OpenMouse_f
===========
For shitty systems that cannot provide relative mouse axes
*/
void Evdev_OpenMouse_f ( void )
{
	/* Android users can try open all devices starting from last
	by listing all of them in script as they cannot write udev rules
	for example:
		evdev_mousepath /dev/input/event10
		evdev_mouseopen
		evdev_mousepath /dev/input/event9
		evdev_mouseopen
		evdev_mousepath /dev/input/event8
		evdev_mouseopen
		etc
	So we will not print annoying messages that it is already open
	*/
	if ( evdev_open ) return;
#ifdef __ANDROID__ // use root to grant access to evdev
	char chmodstr[ 255 ] = "su 0 chmod 777 ";
	strcat( chmodstr, evdev_mousepath->string );
	system( chmodstr );
	// allow write input via selinux, need for some lollipop devices
	system( "su 0 supolicy --live \"allow appdomain input_device dir { ioctl read getattr search open }\" \"allow appdomain input_device chr_file { ioctl read write getattr lock append open }\"" );
	system( chmodstr );
	system( "su 0 setenforce permissive" );
	system( chmodstr );
#endif
	mouse_fd = open ( evdev_mousepath->string, O_RDONLY | O_NONBLOCK );
	if ( mouse_fd < 0 )
	{
		MsgDev( D_ERROR, "Could not open input device %s: %s\n", evdev_mousepath->string, strerror( errno ) );
		return;
	}
	MsgDev( D_INFO, "Input device %s opened sucessfully\n", evdev_mousepath->string );
	evdev_open = 1;
}
/*
===========
Evdev_OpenClose_f
===========
Allow open other mouse
*/
void Evdev_CloseMouse_f ( void )
{
	if ( !evdev_open ) return;
	evdev_open = 0;
	close( mouse_fd );
}

void IN_EvdevFrame ()
{
	if( evdev_open && !m_ignore->integer )
	{
		struct input_event ev;
		evdev_dx = evdev_dy = 0;
		while ( read( mouse_fd, &ev, 16) == 16 )
		{
			if ( ev.type == EV_REL )
			{
				switch ( ev.code )
				{
					case REL_X: evdev_dx += ev.value;
					break;
					case REL_Y: evdev_dy += ev.value;
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
			else if ( ( ev.type == EV_KEY ) && (evdev_grab->value == 1.0 ) )
			{
				switch (ev.code) {
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
		if(clgame.dllFuncs.pfnLookEvent)
			clgame.dllFuncs.pfnLookEvent( -evdev_dx * m_yaw->value, evdev_dy * m_pitch->value );
		else
		{
			cl.refdef.cl_viewangles[PITCH] += evdev_dy * m_enginesens->value;
			cl.refdef.cl_viewangles[PITCH] = bound( -90, cl.refdef.cl_viewangles[PITCH], 90 );
			cl.refdef.cl_viewangles[YAW] -= evdev_dx * m_enginesens->value;
		}
	}
	
}

void Evdev_SetGrab( qboolean grab )
{
	if( !evdev_open || !evdev_grab->integer )
		return;
	ioctl( mouse_fd, EVIOCGRAB, (void*) grab );
	if( grab )
		Key_Event( K_ESCAPE, 0 ); //Do not leave ESC down
}

#endif

void IN_StartupMouse( void )
{
	if( Host_IsDedicated() ) return;

	m_ignore = Cvar_Get( "m_ignore", DEFAULT_M_IGNORE, CVAR_ARCHIVE , "ignore mouse events" );

	m_enginemouse = Cvar_Get("m_enginemouse", "0", CVAR_ARCHIVE, "Read mouse events in engine instead of client");
	m_enginesens = Cvar_Get("m_enginesens", "0.3", CVAR_ARCHIVE, "Mouse sensitivity, when m_enginemouse enabled");
	m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE, "Mouse pitch value");
	m_yaw = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE, "Mouse yaw value");
	
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
		SDL_WarpMouseInWindow( host.hWnd, host.window_center_x, host.window_center_y );
#endif
		if( cls.initialized )
			clgame.dllFuncs.IN_ActivateMouse();
	}

	if( ( newstate == key_menu || newstate == key_console ) && ( !CL_IsBackgroundMap() || CL_IsBackgroundDemo()))
	{
#ifdef XASH_SDL
		SDL_SetWindowGrab(host.hWnd, SDL_FALSE);
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
	static int	oldstate;

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
	SDL_SetWindowGrab( host.hWnd, SDL_TRUE );
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
#ifdef USE_EVDEV
	if( evdev_open && ( evdev_grab->value == 1 ) )
	{
		ioctl( mouse_fd, EVIOCGRAB, (void*) 0);
		Key_Event( K_ESCAPE, 0 ); //Do not leave ESC down
	}
#endif
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
	POINT	current_pos;
	
	if( !in_mouseinitialized || !in_mouseactive || !UI_IsVisible( ))
		return;

	if( m_ignore->integer )
		return;

	// Show cursor in UI
#ifdef XASH_SDL
	if( UI_IsVisible() ) SDL_ShowCursor( SDL_TRUE );
#endif
	// find mouse movement
#ifdef XASH_SDL
	SDL_GetMouseState( &current_pos.x, &current_pos.y );
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
#if defined(XASH_SDL)
		static qboolean ignore; // igonre mouse warp event
		int x, y;
		SDL_GetMouseState(&x, &y);
		if( host.mouse_visible )
			SDL_ShowCursor( SDL_TRUE );
		else
			SDL_ShowCursor( SDL_FALSE );
		if( x < host.window_center_x / 2 || y < host.window_center_y / 2 ||  x > host.window_center_x + host.window_center_x/2 || y > host.window_center_y + host.window_center_y / 2 )
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
	Cmd_RemoveCommand( "evdev_mouseopen" );
	Cmd_RemoveCommand( "evdev_mouseclose" );
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
	evdev_mousepath	= Cvar_Get( "evdev_mousepath", "", 0, "Path for evdev device node");
	evdev_grab = Cvar_Get( "evdev_grab", "0", CVAR_ARCHIVE, "Enable event device grab" );
	Cmd_AddCommand ("evdev_mouseopen", Evdev_OpenMouse_f, "Open device selected by evdev_mousepath");
	Cmd_AddCommand ("evdev_mouseclose", Evdev_CloseMouse_f, "Close current evdev device");
	evdev_open = 0;
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
	float forward = 0, side = 0, dpitch = 0, dyaw = 0;
	if(clgame.dllFuncs.pfnLookEvent)
		return;
	if(active)
	{
		float sensitivity = ((float)cl.refdef.fov_x / (float)90.0f);
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
			float  mouse_x, mouse_y;
			Android_MouseMove( &mouse_x, &mouse_y );
			cl.refdef.cl_viewangles[PITCH] += mouse_y * m_pitch->value * sensitivity;
			cl.refdef.cl_viewangles[YAW] -= mouse_x * m_yaw->value * sensitivity;
		}
#endif
		Joy_FinalizeMove( &forward, &side, &dyaw, &dpitch );
		IN_TouchMove( &forward, &side, &dyaw, &dpitch );
		IN_JoyAppendMove( cmd, forward, side );

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

#ifndef __ANDROID__
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
		IN_TouchMove( &forward, &side, &yaw, &pitch );
		clgame.dllFuncs.pfnLookEvent( yaw, pitch );
		clgame.dllFuncs.pfnMoveEvent( forward, side );
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
