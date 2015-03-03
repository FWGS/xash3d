#include "events.h"
#include "keydefs.h"
#include "input.h"

int SDLash_EventFilter( SDL_Event* event)
{
	switch ( event->type )
	{
		case SDL_MOUSEMOTION:
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

		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			SDLash_MouseEvent(event->button);
			break;

		case SDL_WINDOWEVENT:
			if( host.state == HOST_SHUTDOWN )
				break; // no need to activate
			if( host.state != HOST_RESTART )
			{
				switch( event->window.type )
				{
				case SDL_WINDOWEVENT_MINIMIZED:
					host.state = HOST_SLEEP;
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					host.state = HOST_NOFOCUS;
					IN_DeactivateMouse();
					break;
				default:
					host.state = HOST_FRAME;
					IN_ActivateMouse(true);
				}
			}
	}
	VGUI_SurfaceWndProc(event);
	return 0;
}
void SDLash_KeyEvent(SDL_KeyboardEvent key)
{
	// TODO: improve that.
	int keynum = key.keysym.sym;
	int down = key.type == SDL_KEYDOWN ? 1 : 0;
	if(key.repeat) return;
	switch(key.keysym.sym)
	{
	case SDLK_BACKSPACE:
		keynum =  K_BACKSPACE;
		break;
	case SDL_SCANCODE_UP:
		keynum = K_UPARROW;
		break;
	case SDL_SCANCODE_DOWN:
		keynum = K_DOWNARROW;
		break;
	case SDL_SCANCODE_LEFT:
		keynum = K_LEFTARROW;
		break;
	case SDL_SCANCODE_RIGHT:
		keynum = K_RIGHTARROW;
		break;
	case SDLK_LALT:
	case SDLK_RALT:
		keynum = K_ALT;
		break;
	case SDLK_RSHIFT:
	case SDLK_LSHIFT:
		keynum = K_SHIFT;
		break;
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		keynum = K_CTRL;
		break;
	case SDLK_INSERT:
		keynum = K_INS;
		break;
	case SDLK_DELETE:
		keynum = K_DEL;
		break;
	case SDLK_PAGEUP:
		keynum = K_PGUP;
		break;
	case SDLK_PAGEDOWN:
		keynum = K_PGDN;
		break;
	case SDLK_HOME:
		keynum = K_HOME;
		break;
	case SDLK_END:
		keynum = K_END;
		break;
	}

	if((key.keysym.sym >= SDLK_F1) && (key.keysym.sym <= SDLK_F12))
	{
		keynum = key.keysym.scancode + 77;
	}
	if((key.keysym.sym >= SDLK_KP_1) && (key.keysym.sym <= SDLK_KP_0))
	{
		keynum = key.keysym.scancode - 41;
	}
	//printf("Pressed key. Code: %i\n", keynum);
	Key_Event(keynum, down);
}

void SDLash_MouseEvent(SDL_MouseButtonEvent button)
{
	int down = button.type == SDL_MOUSEBUTTONDOWN ? 1 : 0;
	Key_Event(240 + button.button, down);
}

void SDLash_WheelEvent(SDL_MouseWheelEvent wheel)
{
	int updown = wheel.x < 0 ? K_MWHEELDOWN : K_MWHEELDOWN;
	Key_Event(updown, true);
	Key_Event(updown, false);
}
