#pragma once
#ifndef KEYWRAPPER_H
#define KEYWRAPPER_H

#ifdef XASH_SDL

void SDLash_EventFilter( void *event );
void SDLash_EnableTextInput( int enable, qboolean force );
int SDLash_JoyInit( int numjoy ); // pass -1 to init every joystick

#endif // XASH_SDL
#endif // KEYWRAPPER_H
