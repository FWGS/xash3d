/*
touch.h - natoive touch controls
Copyright (C) 2015-2018 mittorn

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
void Touch_Draw( void );
void IN_TouchEditClear( void );
void Touch_SetClientOnly( qboolean state );
void Touch_RemoveButton( const char *name );
void Touch_HideButtons( const char *name, qboolean hide );
//void IN_TouchSetCommand( const char *name, const char *command );
//void IN_TouchSetTexture( const char *name, const char *texture );
//void IN_TouchSetColor( const char *name, byte *color );
void Touch_AddClientButton( const char *name, const char *texture, const char *command, float x1, float y1, float x2, float y2, byte *color, int round, float aspect, int flags );
void Touch_AddDefaultButton( const char *name, const char *texturefile, const char *command, float x1, float y1, float x2, float y2, byte *color, int round, float aspect, int flags );
void Touch_InitConfig( void );
void Touch_WriteConfig( void );
void Touch_Init( void );
void Touch_Shutdown( void );
void Touch_GetMove( float * forward, float *side, float *yaw, float *pitch );
void Touch_ResetDefaultButtons( void );
int IN_TouchEvent( touchEventType type, int fingerID, float x, float y, float dx, float dy );
void Touch_KeyEvent( int key, int down );
#endif // TOUCH_H
