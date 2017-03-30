#pragma once
#ifndef ANDROID_MAIN_H
#define ANDROID_MAIN_H

//
// mobility interface implementation
//
void Android_Vibrate( float life, char flags );
void *Android_GetNativeObject( const char *obj );

#ifndef XASH_SDL
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
#endif

#endif // ANDROID_MAIN_H
