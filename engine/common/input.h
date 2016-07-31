/*
input.h - win32 input devices
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

#ifndef INPUT_H
#define INPUT_H

/*
==============================================================

INPUT

==============================================================
*/

#include "keydefs.h"
#ifdef XASH_SDL
#include <SDL_mouse.h>
typedef SDL_Cursor Xash_Cursor;
#else
typedef void Xash_Cursor;
#endif

typedef struct keyname_s
{
	char       *name;    // key name
	int        keynum;   // key number
	const char *binding; // default bind
} keyname_t;

extern keyname_t keynames[];

//
// input.c
//
void IN_Init( void );
void Host_InputFrame( void );
void IN_Shutdown( void );
void IN_MouseEvent( int mstate );
void IN_ActivateMouse( qboolean force );
void IN_DeactivateMouse( void );
void IN_ToggleClientMouse( int newstate, int oldstate );
long IN_WndProc( void *hWnd, uint uMsg, uint wParam, long lParam );
void IN_SetCursor( Xash_Cursor *hCursor );
extern qboolean	in_mouseinitialized;

#endif//INPUT_H
