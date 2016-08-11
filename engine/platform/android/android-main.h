#pragma once
#ifndef ANDROID_MAIN_H
#define ANDROID_MAIN_H
void Android_Vibrate( float life, char flags );
#ifndef XASH_SDL
// android_nosdl.c
void Android_SwapBuffers();
void Android_GetScreenRes( int *width, int *height );
void Android_Init( void );
void Android_EnableTextInput( qboolean enable, qboolean force );
void Android_RunEvents( void );
void Android_MessageBox( const char *title, const char *text );
void Android_SwapInterval( int interval );
qboolean Android_InitGL();
void Android_ShutdownGL();
#endif

#endif // ANDROID_MAIN_H
