/*
android-main.h -- android specific main header
Copyright (C) 2015-2017 Flying With Gauss

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
#ifndef ANDROID_MAIN_H
#define ANDROID_MAIN_H

//
// mobility interface implementation
//
void Android_Vibrate( float life, char flags );
void *Android_GetNativeObject( const char *obj );

void Android_ShellExecute( const char *path, const char *parms );

//
// android_nosdl.c
//
void Android_SwapBuffers();
void Android_GetScreenRes( int *width, int *height );
void Android_Init( void );
void Android_EnableTextInput( qboolean enable, qboolean force );
void Android_RunEvents( void );
void Android_MessageBox( const char *title, const char *text );
void Android_SwapInterval( int interval );
qboolean Android_InitGL();
void Android_ShutdownGL();
const char *Android_GetAndroidID( void );
const char *Android_LoadID( void );
void Android_SaveID( const char *id );
void Android_SetTitle( const char *title );
void Android_SetIcon( const char *path );
void Android_MouseMove( float *x, float *y );
void Android_ShowMouse( qboolean show );
void Android_AddMove( float x, float y);

#endif // ANDROID_MAIN_H
