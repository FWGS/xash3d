/*
touchscreen.c - touchscreen support prototype
Copyright (C) 2015 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "touchscreen.h"
#include "gl_local.h"

touchbutton_t *buttons; // dynamic array of buttons
touchbutton_t *lastButton;

int x, y;



void SDLash_TouchEvent(SDL_TouchFingerEvent finger)
{
	x = finger.x * glState.width;
	y = finger.y * glState.height;

	// Find suitable button
	for( lastButton = buttons; lastButton != NULL; lastButton++ )
	{
		if( ( x > lastButton->touchField.left &&
			  x < lastButton->touchField.right ) &&
			( y > lastButton->touchField.bottom &&
			  y < lastButton->touchField.top ) )
		{
			lastButton->touchEvent(finger);
			break;
		}
	}

	MsgDev(D_NOTE, "Caught a Touch Event: %i %f %f, button pointer %p", finger.fingerId, finger.x, finger.y, lastButton);
}

bool touchInit()
{
	buttons = (touchbutton_t*)malloc( sizeof(touchbutton_t) * 10 );
	// Why ten? Because we can!


}
