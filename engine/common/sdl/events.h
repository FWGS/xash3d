#ifndef KEYWRAPPER_H
#define KEYWRAPPER_H
#ifdef XASH_SDL
#include <SDL_events.h>
#include "common.h"

int SDLash_EventFilter(SDL_Event* event);
void SDLash_KeyEvent(SDL_KeyboardEvent key);
void SDLash_MouseEvent(SDL_MouseButtonEvent button);
void SDLash_WheelEvent(SDL_MouseWheelEvent wheel);
void SDLash_InputEvent(SDL_TextInputEvent input);

// Prototype
void SDLash_TouchEvent(SDL_TouchFingerEvent finger);
#endif // XASH_SDL
#endif // KEYWRAPPER_H
