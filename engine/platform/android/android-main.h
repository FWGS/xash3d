#pragma once
#ifndef ANDROID_MAIN_H
#define ANDROID_MAIN_H

#endif
void Android_Vibrate( float life, char flags );
#ifndef XASH_SDL
void Android_SwapBuffers();
void Android_GetScreenRes( int *width, int *height );
void Android_Init( void );
void Android_EnableTextInput( qboolean enable, qboolean force );
void Android_RunEvents( void );
#endif

#endif // ANDROID_MAIN_H
