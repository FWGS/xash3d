#ifdef XASH_SDL
#include "events.h"
#include "keydefs.h"
#include "input.h"
#include "client.h"
#include "vgui_draw.h"

int SDLash_EventFilter( SDL_Event* event)
{
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
			IN_MouseEvent(0);
			break;
		case SDL_QUIT:
			Host_Shutdown();
			break;

		case SDL_KEYDOWN:
		case SDL_KEYUP:
			SDLash_KeyEvent(event->key);
			break;

		case SDL_MOUSEWHEEL:
			SDLash_WheelEvent(event->wheel);
			break;

		case SDL_FINGERMOTION:
		case SDL_FINGERUP:
		case SDL_FINGERDOWN:
			// Pass all touch events to client library
			if(clgame.dllFuncs.pfnTouchEvent)
				clgame.dllFuncs.pfnTouchEvent(event->tfinger.fingerId, event->tfinger.x, event->tfinger.y, event->tfinger.dx, event->tfinger.dy );
			break;

		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		//if(!host.mouse_visible)
			SDLash_MouseEvent(event->button);
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
				case SDL_WINDOWEVENT_MINIMIZED:
					host.state = HOST_SLEEP;
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
				case SDL_WINDOWEVENT_LEAVE:
					host.state = HOST_NOFOCUS;
					IN_DeactivateMouse();
					break;
				default:
					host.state = HOST_FRAME;
					IN_ActivateMouse(true);
				}
			}
	}
	return 0;
}

#ifdef PANDORA
extern int noshouldermb;
#endif

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
#ifdef PANDORA
		keynum = (noshouldermb)?K_SHIFT:K_MOUSE2;
		break;
#endif
	case SDLK_LSHIFT:
		keynum = K_SHIFT; break;
	case SDLK_RCTRL:
#ifdef PANDORA
		keynum = (noshouldermb)?K_SHIFT:K_MOUSE1;
		break;
#endif
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
	/*case SDLK_VOLUMEDOWN:
		keynum = 'e';
		break;
	case SDLK_VOLUMEUP:
		keynum = K_SPACE;
		break;*/
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
	// Pass characters one by one to Con_CharEvent
	for(i = 0; input.text[i]; ++i)
	{
		Con_CharEvent( (int)input.text[i] );
		if( cls.key_dest == key_menu )
			UI_CharEvent ( (int)input.text[i] );
	}
}

void SDLash_EnableTextInput( int enable )
{
	static qboolean isAlreadyEnabled = false;

	if( enable )
	{
		if( !isAlreadyEnabled )
		{
			SDL_StartTextInput();
		}
		isAlreadyEnabled = true;
	}
	else
	{
		SDL_StopTextInput();
		isAlreadyEnabled = false;
	}
}
#endif // XASH_SDL
