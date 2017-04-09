#ifdef XASH_SDL
#include <SDL.h>

#include "common.h"
#include "keydefs.h"
#include "input.h"
#include "client.h"
#include "vgui_draw.h"
#include "events.h"
#include "touch.h"
#include "joyinput.h"
#include "sound.h"
#include "gl_vidnt.h"

extern convar_t *vid_fullscreen;
extern convar_t *snd_mute_losefocus;
static int wheelbutton;
static SDL_Joystick *joy;

void R_ChangeDisplaySettingsFast( int w, int h );

#define DECLARE_KEY_RANGE( min, max, repl ) if( keynum >= (min) && keynum <= (max) ) { keynum = keynum - (min) + (repl); }

void SDLash_KeyEvent( SDL_KeyboardEvent key, int down )
{
	int keynum = key.keysym.scancode;

	DECLARE_KEY_RANGE( SDL_SCANCODE_A, SDL_SCANCODE_Z, 'a' )
	else DECLARE_KEY_RANGE( SDL_SCANCODE_1, SDL_SCANCODE_9, '1' )
	else DECLARE_KEY_RANGE( SDL_SCANCODE_F1, SDL_SCANCODE_F12, K_F1 )
	else
	{
		switch( keynum )
		{
		case SDL_SCANCODE_GRAVE: keynum = '`'; break;
		case SDL_SCANCODE_0: keynum = '0'; break;
		case SDL_SCANCODE_BACKSLASH: keynum = '\\'; break;
		case SDL_SCANCODE_LEFTBRACKET: keynum = '['; break;
		case SDL_SCANCODE_RIGHTBRACKET: keynum = ']'; break;
		case SDL_SCANCODE_EQUALS: keynum = '='; break;
		case SDL_SCANCODE_MINUS: keynum = '-'; break;
		case SDL_SCANCODE_TAB: keynum = K_TAB; break;
		case SDL_SCANCODE_RETURN: keynum = K_ENTER; break;
		case SDL_SCANCODE_ESCAPE: keynum = K_ESCAPE; break;
		case SDL_SCANCODE_SPACE: keynum = K_SPACE; break;
		case SDL_SCANCODE_BACKSPACE: keynum = K_BACKSPACE; break;
		case SDL_SCANCODE_UP: keynum = K_UPARROW; break;
		case SDL_SCANCODE_LEFT: keynum = K_LEFTARROW; break;
		case SDL_SCANCODE_DOWN: keynum = K_DOWNARROW; break;
		case SDL_SCANCODE_RIGHT: keynum = K_RIGHTARROW; break;
		case SDL_SCANCODE_LALT:
		case SDL_SCANCODE_RALT: keynum = K_ALT; break;
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL: keynum = K_CTRL; break;
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT: keynum = K_SHIFT; break;
		case SDL_SCANCODE_LGUI:
		case SDL_SCANCODE_RGUI: keynum = K_WIN; break;
		case SDL_SCANCODE_INSERT: keynum = K_INS; break;
		case SDL_SCANCODE_DELETE: keynum = K_DEL; break;
		case SDL_SCANCODE_PAGEDOWN: keynum = K_PGDN; break;
		case SDL_SCANCODE_PAGEUP: keynum = K_PGUP; break;
		case SDL_SCANCODE_HOME: keynum = K_HOME; break;
		case SDL_SCANCODE_END: keynum = K_END; break;
		case SDL_SCANCODE_KP_7: keynum = K_KP_HOME; break;
		case SDL_SCANCODE_KP_8: keynum = K_KP_UPARROW; break;
		case SDL_SCANCODE_KP_9: keynum = K_KP_PGUP; break;
		case SDL_SCANCODE_KP_4: keynum = K_KP_LEFTARROW; break;
		case SDL_SCANCODE_KP_5: keynum = K_KP_5; break;
		case SDL_SCANCODE_KP_6: keynum = K_KP_RIGHTARROW; break;
		case SDL_SCANCODE_KP_1: keynum = K_KP_END; break;
		case SDL_SCANCODE_KP_2: keynum = K_KP_DOWNARROW; break;
		case SDL_SCANCODE_KP_3: keynum = K_KP_PGDN; break;
		case SDL_SCANCODE_KP_0: keynum = K_KP_INS; break;
		case SDL_SCANCODE_KP_PERIOD: keynum = K_KP_DEL; break;
		case SDL_SCANCODE_KP_ENTER: keynum = K_KP_ENTER; break;
		case SDL_SCANCODE_KP_PLUS: keynum = K_KP_PLUS; break;
		case SDL_SCANCODE_KP_MINUS: keynum = K_KP_MINUS; break;
		case SDL_SCANCODE_KP_DIVIDE: keynum = K_KP_SLASH; break;
		case SDL_SCANCODE_KP_MULTIPLY: keynum = '*'; break;
		case SDL_SCANCODE_NUMLOCKCLEAR: keynum = K_KP_NUMLOCK; break;
		case SDL_SCANCODE_CAPSLOCK: keynum = K_CAPSLOCK; break;
		case SDL_SCANCODE_APPLICATION: keynum = K_WIN; break; // (compose key) ???
		case SDL_SCANCODE_SLASH: keynum = '/'; break;
		case SDL_SCANCODE_PERIOD: keynum = '.'; break;
		case SDL_SCANCODE_SEMICOLON: keynum = ';'; break;
		case SDL_SCANCODE_APOSTROPHE: keynum = '\''; break;
		case SDL_SCANCODE_COMMA: keynum = ','; break;
		case SDL_SCANCODE_PRINTSCREEN:
		{
			host.force_draw_version = true;
			host.force_draw_version_time = host.realtime + FORCE_DRAW_VERSION_TIME;
			break;
		}
		case SDL_SCANCODE_UNKNOWN:
		{
			if( down ) MsgDev( D_INFO, "SDLash_KeyEvent: Unknown scancode\n" );
			return;
		}
		default:
			if( down ) MsgDev( D_INFO, "SDLash_KeyEvent: Unknown key: %s = %i\n", SDL_GetScancodeName( keynum ), keynum );
			return;
		}
	}

	Key_Event( keynum, down );
}

void SDLash_MouseEvent(SDL_MouseButtonEvent button)
{
	int down = button.type == SDL_MOUSEBUTTONDOWN ? 1 : 0;
	if( in_mouseinitialized && !m_ignore->integer && button.which != SDL_TOUCH_MOUSEID )
	{
		Key_Event( 240 + button.button, down );
	}
}

void SDLash_WheelEvent(SDL_MouseWheelEvent wheel)
{
	wheelbutton = wheel.y < 0 ? K_MWHEELDOWN : K_MWHEELUP;
	Key_Event( wheelbutton, true );
}

void SDLash_InputEvent(SDL_TextInputEvent input)
{
	int i;

	// Pass characters one by one to Con_CharEvent
	for(i = 0; input.text[i]; ++i)
	{
		int ch;

		if( !Q_stricmp( cl_charset->string, "utf-8" ) )
			ch = (unsigned char)input.text[i];
		else
			ch = Con_UtfProcessCharForce( (unsigned char)input.text[i] );

		if( !ch )
			continue;

		Con_CharEvent( ch );
		if( cls.key_dest == key_menu )
			UI_CharEvent ( ch );
		if( cls.key_dest == key_game )
			VGui_KeyEvent( ch, 2 );
	}
}

void SDLash_EnableTextInput( int enable, qboolean force )
{
	if( force )
	{
		if( enable )
			SDL_StartTextInput();
		else
			SDL_StopTextInput();
	}
	else if( enable )
	{
		if( !host.textmode )
		{
			SDL_StartTextInput();
		}
		host.textmode = true;
	}
	else
	{
		SDL_StopTextInput();
		host.textmode = false;
	}
}

void SDLash_EventFilter( void *ev )
{
	SDL_Event *event = (SDL_Event*)ev;
	static int mdown;
	if( wheelbutton )
	{
		Key_Event( wheelbutton, false );
		wheelbutton = 0;
	}

	switch ( event->type )
	{
	/* Mouse events */
	case SDL_MOUSEMOTION:
		if( !host.mouse_visible && event->motion.which != SDL_TOUCH_MOUSEID )
			IN_MouseEvent(0);
#ifdef TOUCHEMU
		if( mdown )
			IN_TouchEvent( event_motion, 0,
						   event->motion.x/scr_width->value,
						   event->motion.y/scr_height->value,
						   event->motion.xrel/scr_width->value,
						   event->motion.yrel/scr_height->value );
		SDL_ShowCursor( true );
#endif
		break;

	case SDL_MOUSEBUTTONUP:
#ifdef TOUCHEMU
		mdown = 0;
		IN_TouchEvent( event_up, 0,
					   event->button.x/scr_width->value,
					   event->button.y/scr_height->value, 0, 0);
#else
		SDLash_MouseEvent( event->button );
#endif
		break;
	case SDL_MOUSEBUTTONDOWN:
#ifdef TOUCHEMU
		mdown = 1;
		IN_TouchEvent( event_down, 0,
					   event->button.x/scr_width->value,
					   event->button.y/scr_height->value, 0, 0);
#else
		SDLash_MouseEvent( event->button );
#endif
		break;

	case SDL_MOUSEWHEEL:
		SDLash_WheelEvent( event->wheel );
		break;

	/* Keyboard events */
	case SDL_KEYDOWN:
		SDLash_KeyEvent( event->key, 1 );
		break;

	case SDL_KEYUP:
		SDLash_KeyEvent( event->key, 0 );
		break;


	/* Touch events */
	case SDL_FINGERDOWN:
	case SDL_FINGERUP:
	case SDL_FINGERMOTION:
	{
		touchEventType type;
		static int scale = 0;
		float x, y, dx, dy;

		if( event->type == SDL_FINGERDOWN )
			type = event_down;
		else if( event->type == SDL_FINGERUP )
			type = event_up ;
		else if(event->type == SDL_FINGERMOTION )
			type = event_motion;
		else break;

		/*
		SDL sends coordinates in [0..width],[0..height] values
		on some devices
		*/
		if( !scale )
		{
			if( ( event->tfinger.x > 0 ) && ( event->tfinger.y > 0 ) )
			{
				if( ( event->tfinger.x > 2 ) && ( event->tfinger.y > 2 ) )
				{
					scale = 2;
					MsgDev( D_INFO, "SDL reports screen coordinates, workaround enabled!\n");
				}
				else
					scale = 1;
			}
		}
		if( scale == 2 )
		{
			x = event->tfinger.x / scr_width->value;
			y = event->tfinger.y / scr_height->value;
			dx = event->tfinger.dx / scr_width->value;
			dy = event->tfinger.dy / scr_height->value;
		}
		else
		{
			x = event->tfinger.x;
			y = event->tfinger.y;
			dx = event->tfinger.dx;
			dy = event->tfinger.dy;
		}

		IN_TouchEvent( type, event->tfinger.fingerId, x, y, dx, dy );
		break;
	}


	/* IME */
	case SDL_TEXTINPUT:
		SDLash_InputEvent( event->text );
		break;

	/* Joystick events */
	case SDL_JOYAXISMOTION:
		Joy_AxisMotionEvent( event->jaxis.which, event->jaxis.axis, event->jaxis.value );
		break;

	case SDL_JOYBALLMOTION:
		Joy_BallMotionEvent( event->jball.which, event->jball.ball, event->jball.xrel, event->jball.yrel );
		break;

	case SDL_JOYHATMOTION:
		Joy_HatMotionEvent( event->jhat.which, event->jhat.hat, event->jhat.value );
		break;

	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		Joy_ButtonEvent( event->jbutton.which, event->jbutton.button, event->jbutton.state );
		break;

	case SDL_JOYDEVICEADDED:
		Joy_AddEvent( event->jdevice.which );
		break;
	case SDL_JOYDEVICEREMOVED:
		Joy_RemoveEvent( event->jdevice.which );
		break;

	case SDL_QUIT:
		Sys_Quit();
		break;

	case SDL_WINDOWEVENT:
		if( event->window.windowID != SDL_GetWindowID( host.hWnd ) )
			return;
		if( ( host.state == HOST_SHUTDOWN ) ||
			( host.state == HOST_RESTART )  ||
			( host.type  == HOST_DEDICATED ) )
			break; // no need to activate
		switch( event->window.event )
		{
		case SDL_WINDOWEVENT_MOVED:
			if( !vid_fullscreen->integer )
			{
				Cvar_SetFloat("r_xpos", (float)event->window.data1);
				Cvar_SetFloat("r_ypos", (float)event->window.data2);
			}
			break;
		case SDL_WINDOWEVENT_RESTORED:
			host.state = HOST_FRAME;
			host.force_draw_version = true;
			host.force_draw_version_time = host.realtime + 2;
			if( vid_fullscreen->integer )
				VID_SetMode();
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			host.state = HOST_FRAME;
			IN_ActivateMouse(true);
			if( snd_mute_losefocus->integer )
			{
				S_Activate( true );
			}
			host.force_draw_version = true;
			host.force_draw_version_time = host.realtime + 2;
			if( vid_fullscreen->integer )
				VID_SetMode();
			break;
		case SDL_WINDOWEVENT_MINIMIZED:
			host.state = HOST_SLEEP;
			VID_RestoreScreenResolution();
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:

#if TARGET_OS_IPHONE
			{
				// Keep running if ftp server enabled
				void IOS_StartBackgroundTask( void );
				IOS_StartBackgroundTask();
			}
#endif
			host.state = HOST_NOFOCUS;
			IN_DeactivateMouse();
			if( snd_mute_losefocus->integer )
			{
				S_Activate( false );
			}
			host.force_draw_version = true;
			host.force_draw_version_time = host.realtime + 1;
			VID_RestoreScreenResolution();
			break;
		case SDL_WINDOWEVENT_CLOSE:
			Sys_Quit();
			break;
		case SDL_WINDOWEVENT_RESIZED:
			if( vid_fullscreen->integer != 0 ) break;
			Cvar_SetFloat( "vid_mode", -2.0f ); // no mode
			R_ChangeDisplaySettingsFast( event->window.data1,
										 event->window.data2 );
			break;
		default:
			break;
		}
	}
}

int SDLash_JoyInit( int numjoy )
{
	int num;
	int i;

	MsgDev( D_INFO, "Joystick: SDL\n" );

	if( SDL_WasInit( SDL_INIT_JOYSTICK ) != SDL_INIT_JOYSTICK &&
		SDL_InitSubSystem( SDL_INIT_JOYSTICK ) )
	{
		MsgDev( D_INFO, "Failed to initialize SDL Joysitck: %s\n", SDL_GetError() );
		return 0;
	}

	if( joy )
	{
		SDL_JoystickClose( joy );
	}

	num = SDL_NumJoysticks();

	if( num > 0 )
		MsgDev( D_INFO, "%i joysticks found:\n", num );
	else
	{
		MsgDev( D_INFO, "No joystick found.\n" );
		return 0;
	}

	for( i = 0; i < num; i++ )
		MsgDev( D_INFO, "%i\t: %s\n", i, SDL_JoystickNameForIndex( i ) );

	MsgDev( D_INFO, "Pass +set joy_index N to command line, where N is number, to select active joystick\n" );

	joy = SDL_JoystickOpen( numjoy );

	if( !joy )
	{
		MsgDev( D_INFO, "Failed to select joystick: %s\n", SDL_GetError( ) );
		return 0;
	}

	MsgDev( D_INFO, "Selected joystick: %s\n"
		"\tAxes: %i\n"
		"\tHats: %i\n"
		"\tButtons: %i\n"
		"\tBalls: %i\n",
		SDL_JoystickName( joy ), SDL_JoystickNumAxes( joy ), SDL_JoystickNumHats( joy ),
		SDL_JoystickNumButtons( joy ), SDL_JoystickNumBalls( joy ) );

	SDL_JoystickEventState( SDL_ENABLE );

	return num;
}

#endif // XASH_SDL
