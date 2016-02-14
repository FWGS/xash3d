/*
mobility_int.h - interface between engine and client for mobile platforms
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

#pragma once
#ifndef MOBILITY_INT_H
#define MOBILITY_INT_H
#ifdef __cplusplus
extern "C" {
#endif

#define MOBILITY_API_VERSION 2
#define MOBILITY_CLIENT_EXPORT "HUD_MobilityInterface"

#define VIBRATE_NORMAL (1 << 0) // just vibrate for given "life"

#define TOUCH_FL_HIDE			(1<<0)
#define TOUCH_FL_NOEDIT			(1<<1)
#define TOUCH_FL_CLIENT			(1<<2)
#define TOUCH_FL_MP				(1<<3)
#define TOUCH_FL_SP				(1<<4)
#define TOUCH_FL_DEF_SHOW		(1<<5)
#define TOUCH_FL_DEF_HIDE		(1<<6)
#define TOUCH_FL_DRAW_ADDITIVE	(1<<7)
#define TOUCH_FL_STROKE			(1<<8)
#define TOUCH_FL_PRECISION			(1<<9)

typedef struct mobile_engfuncs_s
{
	// indicates version of API. Should be equal to MOBILITY_API_VERSION
	// version changes when existing functions are changes
	int version;

	// vibration control
	// life -- time to vibrate in ms
	void (*pfnVibrate)( float life, char flags );

	// enable text input
	void (*pfnEnableTextInput)( int enable );

	// add temporaty button, edit will be disabled
	void (*pfnTouchAddClientButton)( const char *name, const char *texture, const char *command, float x1, float y1, float x2, float y2, unsigned char *color, int round, float aspect, int flags );

	// add button to defaults list. Will be loaded on config generation
	void (*pfnTouchAddDefaultButton)( const char *name, const char *texturefile, const char *command, float x1, float y1, float x2, float y2, unsigned char *color, int round, float aspect, int flags );

	// hide/show buttons by pattern
	void (*pfnTouchHideButtons)( const char *name, unsigned char hide );

	// remove button with given name
	void (*pfnTouchRemoveButton)( const char *name );

	// when enabled, only client buttons shown
	void (*pfnTouchSetClientOnly)( unsigned char state );

	// Clean defaults list
	void (*pfnTouchResetDefaultButtons)();

	// To be continued...
} mobile_engfuncs_t;

extern mobile_engfuncs_t *gMobileEngfuncs;

// function exported from client
// returns 0 on no error otherwise error
typedef int (*pfnMobilityInterface)( mobile_engfuncs_t *gMobileEngfuncs );

#ifdef __cplusplus
}
#endif
#endif
