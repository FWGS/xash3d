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

#include "port.h"

convar_t *touch_enable;

touchbutton_t *buttons; // dynamic array of buttons
touchbutton_t *lastButton;

int x, y;


void SDLash_TouchEvent(SDL_TouchFingerEvent finger)
{
	if(touch_enable->integer == 0)
		return;

	x = finger.x * glState.width;
	y = finger.y * glState.height;

	// Find suitable button
	/*for( lastButton = buttons; lastButton != NULL; lastButton++ )
	{
		if( ( x > lastButton->touchField.left &&
			  x < lastButton->touchField.right ) &&
			( y > lastButton->touchField.bottom &&
			  y < lastButton->touchField.top ) )
		{
			lastButton->touchEvent(finger);
			break;
		}
	}*/

	MsgDev(D_NOTE, "Caught a Touch Event: %i %f %f\n", finger.fingerId, x, y, lastButton);
}

void Touch_Init()
{
	//buttons = (touchbutton_t*)malloc( sizeof(touchbutton_t) );
	//Jump_Button_Init();
	// Why ten? Because we can!

	touch_enable = Cvar_Get( "touch_enable", "0", CVAR_ARCHIVE, "enable on-screen controls and event handling" );
}
