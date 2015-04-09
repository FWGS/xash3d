/*
touchscreen.h - touchscreen support prototype
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

#ifndef TOUCHSCREEN_H
#define TOUCHSCREEN_H

#include <SDL_touch.h>

#include "events.h"
#include "wrect.h"

typedef enum
{
	touch_tap,		// Just tap a button
	touch_slider,	// Slider. Maybe isn't need because here is touch_stick
	touch_stick		// Like a joystick stick.
} touchButtonType;

typedef struct touchbutton_s
{
	// Field of button in pixels
	wrect_t touchField;
	// Touch button type: tap, stick or slider
	touchButtonType buttonType;
	// Callback for a button
	void (*touchEvent)(SDL_TouchFingerEvent event);
	// Drawing callback
	void (*drawButton)(void);
} touchbutton_t;

#endif
