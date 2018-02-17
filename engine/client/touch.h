/*
touch.h - natoive touch controls
Copyright (C) 2015 mittorn

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
#ifndef TOUCH_H
#define TOUCH_H

typedef enum
{
	event_down = 0,
	event_up,
	event_motion
} touchEventType;

extern convar_t *touch_enable;

// touch.c
void IN_TouchDraw( void );
void IN_TouchEditClear( void );
void IN_TouchSetClientOnly( qboolean state );
void IN_TouchRemoveButton( const char *name );
void IN_TouchHideButtons( const char *name, qboolean hide );
//void IN_TouchSetCommand( const char *name, const char *command );
//void IN_TouchSetTexture( const char *name, const char *texture );
//void IN_TouchSetColor( const char *name, byte *color );
void IN_TouchAddClientButton( const char *name, const char *texture, const char *command, float x1, float y1, float x2, float y2, byte *color, int round, float aspect, int flags );
void IN_TouchAddDefaultButton( const char *name, const char *texturefile, const char *command, float x1, float y1, float x2, float y2, byte *color, int round, float aspect, int flags );
void IN_TouchInitConfig( void );
void IN_TouchWriteConfig( void );
void IN_TouchInit( void );
void IN_TouchShutdown( void );
void IN_TouchMove( float * forward, float *side, float *yaw, float *pitch );
void IN_TouchResetDefaultButtons( void );
int IN_TouchEvent( touchEventType type, int fingerID, float x, float y, float dx, float dy );
void IN_TouchKeyEvent( int key, int down );
#endif // TOUCH_H
