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

#include "port.h"

#include "common.h"
#include "input.h"
#include "client.h"
#include "vgui_draw.h"

#ifdef _WIN32
#include "windows.h"
#endif

#define PRINTSCREEN_ID	1
#define WND_HEADSIZE	wnd_caption		// some offset
#define WND_BORDER		3			// sentinel border in pixels

Xash_Cursor*	in_mousecursor;
qboolean	in_mouseactive;				// false when not focus app
qboolean	in_restore_spi;
qboolean	in_mouseinitialized;
int	in_mouse_oldbuttonstate;
qboolean	in_mouse_suspended;
int	in_mouse_buttons;
#ifdef _WIN32
RECT	window_rect, real_rect;
#endif
uint	in_mouse_wheel;
int	wnd_caption;
convar_t *fullscreen = 0;
#ifdef PANDORA
int noshouldermb = 0;
#endif

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

#ifdef XASH_SDL
convar_t *m_valvehack;
convar_t *m_enginemouse;
#endif

convar_t *m_enginesens;
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

	if( key & ( 1 << 24 ))
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
#include <linux/input.h>

int evdev_open, mouse_fd, evdev_dx, evdev_dy;

convar_t	*evdev_mousepath;
convar_t	*evdev_grab;


int KeycodeFromEvdev(int keycode);

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
	char chmodstr[ 255 ] = "su -c chmod 666 ";
	strcat( chmodstr, evdev_mousepath->string );
	system(chmodstr);
#endif
	mouse_fd = open ( evdev_mousepath->string, O_RDONLY | O_NONBLOCK );
	if ( mouse_fd < 0 )
	{
		MsgDev( D_ERROR, "Could not open input device %s\n", evdev_mousepath->string );
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
	if ( evdev_open )
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
				}
			}
			else if ( ( ev.type == EV_KEY ) && (evdev_grab->value == 1.0 ) )
			{
				Key_Event ( KeycodeFromEvdev( ev.code ) , ev.value);
			}
		}
		if( ( evdev_grab->value == 1 ) && ( cls.key_dest != key_game ) )
		{
			ioctl( mouse_fd, EVIOCGRAB, (void*) 0);
			Key_Event( K_ESCAPE, 0 ); //Do not leave ESC down
		}
		if(clgame.dllFuncs.pfnLookEvent)
			clgame.dllFuncs.pfnLookEvent( evdev_dx, evdev_dy );
		else
		{
			cl.refdef.cl_viewangles[PITCH] += evdev_dy * m_enginesens->value;
			cl.refdef.cl_viewangles[PITCH] = bound( -90, cl.refdef.cl_viewangles[PITCH], 90 );
			cl.refdef.cl_viewangles[YAW] -= evdev_dx * m_enginesens->value;
		}
	}
	
}
#endif

void IN_StartupMouse( void )
{
	if( host.type == HOST_DEDICATED ) return;
	
	// You can use -nomouse argument to prevent using mouse from client
	// -noenginemouse will disable all mouse input
	if( Sys_CheckParm( "-noenginemouse" )) return; 

#ifdef XASH_SDL
	m_valvehack = Cvar_Get("m_valvehack", "0", CVAR_ARCHIVE, "Enable mouse hack for client.so with different SDL binary");
	m_enginemouse = Cvar_Get("m_enginemouse", "0", CVAR_ARCHIVE, "Read mouse events in engine instead of client");
	m_enginesens = Cvar_Get("m_enginesens", "0.3", CVAR_ARCHIVE, "Mouse sensitivity, when m_enginemouse enabled");
#endif

	in_mouse_buttons = 8;
	in_mouseinitialized = true;
	fullscreen = Cvar_FindVar( "fullscreen" );

#ifdef _WIN32
	in_mouse_wheel = RegisterWindowMessage( "MSWHEEL_ROLLMSG" );
#endif
}

static qboolean IN_CursorInRect( void )
{
#ifdef _WIN32
	POINT	curpos;
	
	if( !in_mouseinitialized || !in_mouseactive )
		return false;

	// find mouse movement
	//GetMouseState( &curpos );

	SDL_GetMouseState(&curpos.x, &curpos.y);

	if( curpos.x < real_rect.left + WND_BORDER )
		return false;
	if( curpos.x > real_rect.right - WND_BORDER * 3 )
		return false;
	if( curpos.y < real_rect.top + WND_HEADSIZE + WND_BORDER )
		return false;
	if( curpos.y > real_rect.bottom - WND_BORDER * 3 )
		return false;
	return true;
#endif
	return true;
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

	if( newstate == key_menu && ( !CL_IsBackgroundMap() || CL_IsBackgroundDemo()))
	{
#ifdef XASH_SDL
		SDL_SetWindowGrab(host.hWnd, false);
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
	int		width, height;
	static int	oldstate;

	if( !in_mouseinitialized )
		return;

	if( CL_Active() && host.mouse_visible && !force )
		return;	// VGUI controls
#ifdef USE_EVDEV
		if( evdev_open && ( evdev_grab->value == 1 ) && cls.key_dest == key_game )
		{
			ioctl( mouse_fd, EVIOCGRAB, (void*) 1);
			Key_Event( K_ESCAPE, 0 ); //Do not leave ESC down
		}
		else
#endif

	if( cls.key_dest == key_menu && fullscreen && !fullscreen->integer)
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

		if( in_mouse_suspended && IN_CursorInRect( ))
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
	}
#ifdef XASH_SDL
	SDL_SetWindowGrab( host.hWnd, true );
	SDL_GetRelativeMouseState( 0, 0 ); // Reset mouse position
#endif
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
	SDL_SetWindowGrab( host.hWnd, false );
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

	// Show cursor in UI
#ifdef XASH_SDL
	if( UI_IsVisible() ) SDL_ShowCursor( true );
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
	if( cls.key_dest == key_game )
	{
#if defined(XASH_SDL) && !defined(_WIN32)
		static qboolean ignore; // igonre mouse warp event
		if( m_valvehack->integer == 0 )
		{
			if( host.mouse_visible )
				SDL_SetRelativeMouseMode( SDL_FALSE );
			else
				SDL_SetRelativeMouseMode( SDL_TRUE );
		}
		else
		{
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
				if( m_enginemouse->integer )
				{
					int mouse_x, mouse_y;
					SDL_GetRelativeMouseState( &mouse_x, &mouse_y );
					cl.refdef.cl_viewangles[PITCH] += mouse_y * m_enginesens->value;
					cl.refdef.cl_viewangles[PITCH] = bound( -90, cl.refdef.cl_viewangles[PITCH], 90 );
					cl.refdef.cl_viewangles[YAW] -= mouse_x * m_enginesens->value;
				}
				else clgame.dllFuncs.IN_MouseEvent( mstate );
			}
			else
			{
				SDL_GetRelativeMouseState( 0, 0 ); // reset relative state
				ignore = 0;
			}
		}
#endif
		return;
	}
	else
	{
#if defined(XASH_SDL) && !defined(_WIN32)
		SDL_SetRelativeMouseMode( false );
#endif
		IN_MouseMove();
	}

	// perform button actions
	for( i = 0; i < in_mouse_buttons; i++ )
	{
		if(( mstate & ( 1<<i )) && !( in_mouse_oldbuttonstate & ( 1<<i )))
		{
			Key_Event( K_MOUSE1 + i, true );
		}

		if(!( mstate & ( 1<<i )) && ( in_mouse_oldbuttonstate & ( 1<<i )))
		{
			Key_Event( K_MOUSE1 + i, false );
		}
	}	

	in_mouse_oldbuttonstate = mstate;
}

#ifdef XASH_SDL
/*
==========================
SDL Joystick Code
==========================
*/

struct sdl_joydada_s
{
	qboolean open;
	int num_axes;
	char binding[10];
	SDL_Joystick *joy;
} joydata;


convar_t *joy_index;
convar_t *joy_binding;
convar_t *joy_pitch;
convar_t *joy_yaw;
convar_t *joy_forward;
convar_t *joy_side;
convar_t *joy_enable;
void IN_SDL_JoyMove( float *forward, float *side, float *pitch, float *yaw )
{
	int i;
	if(!joydata.joy) return;
	// Extend cvar with zeroes
	if(joy_binding->modified)
	{
		Q_strncpy(joydata.binding, joy_binding->string, 10);
		Q_strncat(joydata.binding,"0000000000", joydata.num_axes);
		joy_binding->modified = false;
	}
	for(i = 0; i < joydata.num_axes; i++)
	{
		signed short value = SDL_JoystickGetAxis( joydata.joy, i );
		if( value <= 3200 && value >= -3200 ) continue;
		switch(joy_binding->string[i])
		{
			case 'f': *forward -= joy_forward->value/32768.0 * value;break;
			case 's': *side += joy_side->value/32768.0 * value;break;
			case 'p': *pitch += joy_pitch->value/32768.0 *value;break;
			case 'y': *yaw -= joy_yaw->value/32768.0 * value;break;
			default:break;
		}
	}
}


void IN_SDL_JoyInit( void )
{
	int num;
	joydata.joy = 0;
	joy_binding = Cvar_Get( "joy_binding", "sfyp", CVAR_ARCHIVE, "Joystick binding (f/s/p/y)" );
	joy_binding->modified = true;
	joy_index = Cvar_Get( "joy_index" ,"0" , CVAR_ARCHIVE, "Joystick number to open" );
	joy_enable = Cvar_Get( "joy_enable" ,"1" , CVAR_ARCHIVE, "Enable joystick" );
	joy_pitch = Cvar_Get( "joy_pitch" ,"1.0" , CVAR_ARCHIVE, "Joystick pitch sensitivity" );
	joy_yaw = Cvar_Get( "joy_yaw" ,"1.0" , CVAR_ARCHIVE, "Joystick yaw sensitivity" );
	joy_side = Cvar_Get( "joy_side" ,"1.0" , CVAR_ARCHIVE, "Joystick side sensitivity" );
	joy_forward = Cvar_Get( "joy_forward" ,"1.0" , CVAR_ARCHIVE, "Joystick forward sensitivity" );
	if (!joy_enable->integer) return;
	
	if( ( num = SDL_NumJoysticks() ) )
	{
		MsgDev ( D_INFO, "%d joysticks found\n", num );
		joydata.joy = SDL_JoystickOpen(joy_index->integer);
		if(!joydata.joy)
		{
			MsgDev ( D_ERROR, "Failed to open joystick!\n");
		}
		joydata.num_axes = SDL_JoystickNumAxes(joydata.joy);
		MsgDev ( D_INFO, "Joystick %s has %d axes\n", SDL_JoystickName(joydata.joy), joydata.num_axes );
	}
}

#endif

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown( void )
{
	IN_DeactivateMouse( );
}


/*
===========
IN_Init
===========
*/
void IN_Init( void )
{
#ifdef PANDORA
	if( Sys_CheckParm( "-noshouldermb" )) noshouldermb = 1;
#endif

	IN_StartupMouse( );

	cl_forwardspeed	= Cvar_Get( "cl_forwardspeed", "400", CVAR_ARCHIVE | CVAR_CLIENTDLL, "Default forward move speed" );
	cl_backspeed	= Cvar_Get( "cl_backspeed", "400", CVAR_ARCHIVE | CVAR_CLIENTDLL, "Default back move speed"  );
	cl_sidespeed	= Cvar_Get( "cl_sidespeed", "400", CVAR_ARCHIVE | CVAR_CLIENTDLL, "Default side move speed"  );
#ifdef XASH_SDL
	IN_SDL_JoyInit();
#endif
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

#define F 1<<0	// Forward
#define B 1<<1	// Back
#define L 1<<2	// Left
#define R 1<<3	// Right
#define T 1<<4	// Forward stop
#define S 1<<5	// Side stop
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
	float forward = 0, side = 0;
	if(clgame.dllFuncs.pfnLookEvent)
		return;
	if(active)
	{
#ifdef __ANDROID__
		Android_Move( &forward, &side, &cl.refdef.cl_viewangles[PITCH], &cl.refdef.cl_viewangles[YAW] );
#endif
#ifdef XASH_SDL
		IN_SDL_JoyMove( &forward, &side, &cl.refdef.cl_viewangles[PITCH], &cl.refdef.cl_viewangles[YAW] );
#endif
		IN_JoyAppendMove( cmd, forward, side );

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

	rand (); // keep the random time dependent

	Sys_SendKeyEvents ();

#ifdef __ANDROID__
	Android_Events();
#endif

#ifdef USE_EVDEV
	IN_EvdevFrame();
#endif
	if(clgame.dllFuncs.pfnLookEvent)
	{
		int dx, dy;
#ifdef __ANDROID__
		Android_Move( &forward, &side, &pitch, &yaw );
#endif
#ifdef XASH_SDL
		IN_SDL_JoyMove( &forward, &side, &pitch, &yaw );
		if( in_mouseinitialized )
		{
			SDL_GetRelativeMouseState( &dx, &dy );
			pitch +=dy, yaw +=dx;
		}
#endif
		clgame.dllFuncs.pfnLookEvent(yaw * 50, pitch * 50);
		clgame.dllFuncs.pfnMoveEvent(forward, side);
	}
	Cbuf_Execute ();

	if( host.state == HOST_RESTART )
		host.state = HOST_FRAME; // restart is finished

	if( host.type == HOST_DEDICATED )
	{
		// let the dedicated server some sleep
		Sys_Sleep( 1 );
	}
	else
	{
		if( host.state == HOST_NOFOCUS )
		{
			if( Host_ServerState() && CL_IsInGame( ))
				Sys_Sleep( 1 ); // listenserver
			else Sys_Sleep( 20 ); // sleep 20 ms otherwise
		}
		else if( host.state == HOST_SLEEP )
		{
			// completely sleep in minimized state
			Sys_Sleep( 20 );
		}
	}

	if( !in_mouseinitialized )
		return;

	if( host.state != HOST_FRAME )
	{
		IN_DeactivateMouse();
		return;
	}

	if( cl.refdef.paused && cls.key_dest == key_game )
		shutdownMouse = true; // release mouse during pause or console typeing
	
	if( shutdownMouse && !fullscreen->integer )
	{
		IN_DeactivateMouse();
		return;
	}

	IN_ActivateMouse( false );
	IN_MouseMove();
}

/*
====================
IN_WndProc

main window procedure
====================
*/
long IN_WndProc( void *hWnd, uint uMsg, uint wParam, long lParam )
{
/*
#ifdef _WIN32
	int	i, temp = 0;
	qboolean	fActivate;

	if( uMsg == in_mouse_wheel )
		uMsg = WM_MOUSEWHEEL;

	VGUI_SurfaceWndProc( hWnd, uMsg, wParam, lParam );

	switch( uMsg )
	{
	case WM_KILLFOCUS:
		if( Cvar_VariableInteger( "fullscreen" ))
			ShowWindow( host.hWnd, SW_SHOWMINNOACTIVE );
		break;
	case WM_SETCURSOR:
		IN_ActivateCursor();
		break;
	case WM_MOUSEWHEEL:
		if( !in_mouseactive ) break;
		if(( short )HIWORD( wParam ) > 0 )
		{
			Key_Event( K_MWHEELUP, true );
			Key_Event( K_MWHEELUP, false );
		}
		else
		{
			Key_Event( K_MWHEELDOWN, true );
			Key_Event( K_MWHEELDOWN, false );
		}
		break;
	case WM_CREATE:
		host.hWnd = hWnd;
		GetWindowRect( host.hWnd, &real_rect );
		RegisterHotKey( host.hWnd, PRINTSCREEN_ID, 0, VK_SNAPSHOT );
		break;
	case WM_CLOSE:
		Sys_Quit();
		break;
	case WM_ACTIVATE:
		if( host.state == HOST_SHUTDOWN )
			break; // no need to activate
		if( host.state != HOST_RESTART )
		{
			if( HIWORD( wParam ))
				host.state = HOST_SLEEP;
			else if( LOWORD( wParam ) == WA_INACTIVE )
				host.state = HOST_NOFOCUS;
			else host.state = HOST_FRAME;
			fActivate = (host.state == HOST_FRAME) ? true : false;
		}
		else fActivate = true; // video sucessfully restarted

		wnd_caption = GetSystemMetrics( SM_CYCAPTION ) + WND_BORDER;

		S_Activate( fActivate, host.hWnd );
		IN_ActivateMouse( fActivate );
		Key_ClearStates();

		if( host.state == HOST_FRAME )
		{
			SetForegroundWindow( hWnd );
			ShowWindow( hWnd, SW_RESTORE );
		}
		else if( Cvar_VariableInteger( "fullscreen" ) && host.state != HOST_RESTART )
		{
			ShowWindow( hWnd, SW_MINIMIZE );
		}
		break;
	case WM_MOVE:
		if( !Cvar_VariableInteger( "fullscreen" ))
		{
			RECT	rect;
			int	xPos, yPos, style;

			xPos = (short)LOWORD( lParam );    // horizontal position 
			yPos = (short)HIWORD( lParam );    // vertical position 

			rect.left = rect.top = 0;
			rect.right = rect.bottom = 1;
			style = GetWindowLong( hWnd, GWL_STYLE );
			AdjustWindowRect( &rect, style, FALSE );

			Cvar_SetFloat( "r_xpos", xPos + rect.left );
			Cvar_SetFloat( "r_ypos", yPos + rect.top );
			GetWindowRect( host.hWnd, &real_rect );
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEMOVE:
		for( i = 0; i < in_mouse_buttons; i++ )
		{
			if( wParam & mouse_buttons[i] )
				temp |= (1<<i);
		}
		IN_MouseEvent( temp );
		break;
	case WM_SYSCOMMAND:
		// never turn screensaver while Xash is active
		if( wParam == SC_SCREENSAVE && host.state != HOST_SLEEP )
			return 0;
		break;
	case WM_SYSKEYDOWN:
		if( wParam == VK_RETURN )
		{
			// alt+enter fullscreen switch
			Cvar_SetFloat( "fullscreen", !Cvar_VariableValue( "fullscreen" ));
			return 0;
		}
		// intentional fallthrough
	case WM_KEYDOWN:
		Key_Event( Host_MapKey( lParam ), true );
		if( Host_MapKey( lParam ) == K_ALT )
			return 0;	// prevent WC_SYSMENU call
		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		Key_Event( Host_MapKey( lParam ), false );
		break;
	case WM_CHAR:
		CL_CharEvent( wParam );
		break;
	case WM_HOTKEY:
		switch( LOWORD( wParam ))
		{
		case PRINTSCREEN_ID:
			// anti FiEctro system: prevent to write snapshot without Xash version
			Q_strncpy( cls.shotname, "clipboard.bmp", sizeof( cls.shotname ));
			cls.scrshot_action = scrshot_snapshot; // build new frame for screenshot
			host.write_to_clipboard = true;
			cls.envshot_vieworg = NULL;
			break;
		}
		break;
	}
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
#else*/
	return 0;
//#endif
}
