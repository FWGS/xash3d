#ifdef XASH_SDL
#include "events.h"
#include "keydefs.h"
#include "input.h"
#include "client.h"
#include "vgui_draw.h"

extern convar_t *vid_fullscreen;
extern convar_t *snd_mute_losefocus;
static qboolean lostFocusOnce;
static float oldVolume;
static float oldMusicVolume;

int IN_TouchEvent( qboolean down, int fingerID, float x, float y, float dx, float dy );
void R_ChangeDisplaySettingsFast( int w, int h );

void SDLash_EventFilter( SDL_Event* event)
{
	static int mdown;
	#ifdef XASH_VGUI
	//if( !host.mouse_visible || !VGUI_SurfaceWndProc(event))
	// switch ....
	// CEnginePanel is visible by default, why?
 	VGUI_SurfaceWndProc(event);
	#endif

	switch ( event->type )
	{
		case SDL_MOUSEMOTION:
		if(!host.mouse_visible)
			if( event->motion.which != SDL_TOUCH_MOUSEID )
				IN_MouseEvent(0);
#ifdef TOUCHEMU
			if(mdown)
				IN_TouchEvent(2, 0, (float)event->motion.x/scr_width->value, (float)event->motion.y/scr_height->value, (float)event->motion.xrel/scr_width->value, (float)event->motion.yrel/scr_height->value);
			SDL_ShowCursor( true );
#endif
			break;
		case SDL_QUIT:
			Sys_Quit();
			break;

		case SDL_KEYDOWN:
		case SDL_KEYUP:
			SDLash_KeyEvent(event->key);
			break;

		case SDL_MOUSEWHEEL:
			SDLash_WheelEvent(event->wheel);
			break;

		case SDL_FINGERMOTION:
		IN_TouchEvent( 2, event->tfinger.fingerId, event->tfinger.x, event->tfinger.y, event->tfinger.dx, event->tfinger.dy );
			break;
		case SDL_FINGERUP:
		IN_TouchEvent( 1, event->tfinger.fingerId, event->tfinger.x, event->tfinger.y, event->tfinger.dx, event->tfinger.dy );
			break;
		case SDL_FINGERDOWN:
			// Pass all touch events to client library
			//if(clgame.dllFuncs.pfnTouchEvent)
				//clgame.dllFuncs.pfnTouchEvent(event->tfinger.fingerId, event->tfinger.x, event->tfinger.y, event->tfinger.dx, event->tfinger.dy );
			IN_TouchEvent( 0, event->tfinger.fingerId, event->tfinger.x, event->tfinger.y, event->tfinger.dx, event->tfinger.dy );
			break;

		case SDL_MOUSEBUTTONUP:

#ifdef TOUCHEMU
			mdown = 0;
			IN_TouchEvent(1, 0, (float)event->button.x/scr_width->value, (float)event->button.y/scr_height->value, 0, 0);
#else
			SDLash_MouseEvent(event->button);
#endif
			break;
		case SDL_MOUSEBUTTONDOWN:
		//if(!host.mouse_visible)
#ifdef TOUCHEMU
			mdown = 1;
			IN_TouchEvent(0, 0, (float)event->button.x/scr_width->value, (float)event->button.y/scr_height->value, 0, 0);
#else
			SDLash_MouseEvent(event->button);
#endif
			break;

		case SDL_TEXTEDITING:
			//MsgDev(D_INFO, "Caught a text edit: %s %n %n\n", event->edit.text, event->edit.start, event->edit.length);
			break;

		case SDL_TEXTINPUT:
			SDLash_InputEvent(event->text);
			break;

		case SDL_WINDOWEVENT:
			if( host.state == HOST_SHUTDOWN )
				break; // no need to activate
			if( host.state != HOST_RESTART )
			{
				switch( event->window.event )
				{
				case SDL_WINDOWEVENT_MOVED:
					if(!vid_fullscreen->integer)
					{
						Cvar_SetFloat("r_xpos", (float)event->window.data1);
						Cvar_SetFloat("r_ypos", (float)event->window.data2);
					}
					break;
				case SDL_WINDOWEVENT_MINIMIZED:
					host.state = HOST_SLEEP;
					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					host.state = HOST_FRAME;
					IN_ActivateMouse(true);
					if(lostFocusOnce && snd_mute_losefocus && snd_mute_losefocus->integer)
					{
						Cvar_SetFloat("volume", oldVolume);
						Cvar_SetFloat("musicvolume", oldMusicVolume);
					}
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					host.state = HOST_NOFOCUS;
					IN_DeactivateMouse();
					if(snd_mute_losefocus && snd_mute_losefocus->integer)
					{
						lostFocusOnce = true;
						oldVolume = Cvar_VariableValue("volume");
						oldMusicVolume = Cvar_VariableValue("musicvolume");
						Cvar_SetFloat("volume", 0);
						Cvar_SetFloat("musicvolume", 0);
					}
					break;
				case SDL_WINDOWEVENT_CLOSE:
					Sys_Quit();
					break;
				case SDL_WINDOWEVENT_RESIZED:
					if( vid_fullscreen->integer != 0 ) break;
					Cvar_SetFloat("vid_mode", -2.0f); // no mode
					R_ChangeDisplaySettingsFast( event->window.data1, event->window.data2 );
					break;
				default:
					break;
				}
			}
	}
}

void SDLash_KeyEvent(SDL_KeyboardEvent key)
{
	// TODO: improve that.
	int keynum = key.keysym.sym;
	int down = key.type == SDL_KEYDOWN ? 1 : 0;
	switch(key.keysym.sym)
	{
	case SDLK_BACKSPACE:
		keynum = K_BACKSPACE; break;
	case SDLK_UP:
		keynum = K_UPARROW; break;
	case SDLK_DOWN:
		keynum = K_DOWNARROW; break;
	case SDLK_LEFT:
		keynum = K_LEFTARROW; break;
	case SDLK_RIGHT:
		keynum = K_RIGHTARROW; break;
	case SDLK_LALT:
	case SDLK_RALT:
		keynum = K_ALT;	break;
	case SDLK_RSHIFT:
	case SDLK_LSHIFT:
		keynum = K_SHIFT; break;
	case SDLK_RCTRL:
	case SDLK_LCTRL:
		keynum = K_CTRL; break;
	case SDLK_INSERT:
		keynum = K_INS;	break;
	case SDLK_DELETE:
		keynum = K_DEL; break;
	case SDLK_PAGEUP:
		keynum = K_PGUP; break;
	case SDLK_PAGEDOWN:
		keynum = K_PGDN; break;
	case SDLK_HOME:
		keynum = K_HOME; break;
	case SDLK_END:
		keynum = K_END;	break;
	case ANDROID_K_BACK:
		keynum = K_ESCAPE; break;
	case SDLK_SELECT:
		keynum = K_ENTER; break;
	case SDLK_CAPSLOCK:
		keynum = K_CAPSLOCK; break;
	case SDLK_KP_PLUS:
		keynum = K_KP_PLUS;	break;
	case SDLK_KP_MINUS:
		keynum = K_KP_MINUS; break;
	case SDLK_KP_ENTER:
		keynum = K_KP_ENTER; break;
	case SDLK_KP_DIVIDE:
		keynum = K_KP_SLASH; break;
	case SDLK_NUMLOCKCLEAR:
		keynum = K_KP_NUMLOCK; break;
	case SDLK_VOLUMEDOWN:
		keynum = K_AUX31; break;
	case SDLK_VOLUMEUP:
		keynum = K_AUX32; break;
#ifdef __ANDROID__
	case SDLK_MENU:
		keynum = K_AUX30;
#endif
	}

	if((key.keysym.sym >= SDLK_F1) && (key.keysym.sym <= SDLK_F12))
	{
		keynum = key.keysym.scancode + 77;
	}
	if((key.keysym.sym >= SDLK_KP_1) && (key.keysym.sym <= SDLK_KP_0))
	{
		keynum = key.keysym.scancode - 40;
	}
	if(key.keysym.scancode > 284) // Joystick keys are AUX, if present.
	{
		keynum = key.keysym.scancode - 285 + K_AUX1;
	}
	Key_Event(keynum, down);
}

void SDLash_MouseEvent(SDL_MouseButtonEvent button)
{
	int down = button.type == SDL_MOUSEBUTTONDOWN ? 1 : 0;
	if( in_mouseinitialized && !m_ignore->value && button.which != SDL_TOUCH_MOUSEID )
		Key_Event(240 + button.button, down);
}

void SDLash_WheelEvent(SDL_MouseWheelEvent wheel)
{
	int updown = wheel.y < 0 ? K_MWHEELDOWN : K_MWHEELUP;
	Key_Event(updown, true);
	Key_Event(updown, false);
}

void SDLash_InputEvent(SDL_TextInputEvent input)
{
	int i;
#if 0
	int f, t;
	// Try convert to selected charset
	unsigned char buf[32];

	const char *in = input.text;
	char *out = buf;
	SDL_iconv_t cd;
	Q_memset( &buf, 0, sizeof( buf ) );
	cd = SDL_iconv_open( cl_charset->string, "utf-8" );
	if( cd != (SDL_iconv_t)-1 )
	{
		f = strlen( input.text );
		t = 32;
		t = SDL_iconv( cd, &in, &f, &out, &t );
	}
	if( ( t < 0 ) || ( cd == (SDL_iconv_t)-1 ) )
	Q_strncpy( buf, input.text, 32 );
#endif
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
	}
}

void SDLash_EnableTextInput( int enable )
{
	if( enable )
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
#endif // XASH_SDL
