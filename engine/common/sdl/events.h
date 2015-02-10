#ifndef KEYWRAPPER_H
#define KEYWRAPPER_H

#include "common.h"
#include "SDL2/SDL.h"

int SDLash_EventFilter(SDL_Event* event);
void SDLash_KeyEvent(SDL_KeyboardEvent key);
void SDLash_MouseEvent(SDL_MouseButtonEvent button);
void SDLash_WheelEvent(SDL_MouseWheelEvent wheel);

#endif // KEYWRAPPER_H
