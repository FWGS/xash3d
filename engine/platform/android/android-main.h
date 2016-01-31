#pragma once
#ifndef ANDROID_MAIN_H
#define ANDROID_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

void GetTrackData( float values[3]);
void Android_SwapBuffers();
void Android_GetScreenRes( int *width, int *height );

#ifdef __cplusplus
}
#endif

#endif // ANDROID_MAIN_H
