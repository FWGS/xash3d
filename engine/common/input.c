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
#endif

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
void IN_StartupMouse( void )
{
	if( host.type == HOST_DEDICATED ) return;
	if( Sys_CheckParm( "-nomouse" )) return; 

	in_mouse_buttons = 8;
	in_mouseinitialized = true;

#ifdef XASH_SDL
	m_valvehack = Cvar_Get("m_valvehack", "0", CVAR_ARCHIVE, "Enable mouse hack for valve client.so");
#endif

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

	if( cls.key_dest == key_menu && !Cvar_VariableInteger( "fullscreen" ))
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
		if (m_valvehack->integer == 0)
		{
			SDL_SetRelativeMouseMode(SDL_TRUE);
		}
		else
		{
			int x, y;
			SDL_GetMouseState(&x, &y);
			if( x < host.window_center_x / 2 || y < host.window_center_y / 2 ||  x > host.window_center_x + host.window_center_x/2 || y > host.window_center_y + host.window_center_y / 2 )
			{
				SDL_WarpMouseInWindow(host.hWnd, host.window_center_x, host.window_center_y);
				ignore = 1; // next mouse event will be mouse warp
				clgame.dllFuncs.IN_MouseEvent( mstate );
				return;
			}
			if (!ignore)
			{
				clgame.dllFuncs.IN_MouseEvent( mstate );
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

	rand (); // keep the random time dependent

	Sys_SendKeyEvents ();

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
	
	if( shutdownMouse && !Cvar_VariableInteger( "fullscreen" ))
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