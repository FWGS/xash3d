
#ifdef __cplusplus
extern "C" {
#endif
void Android_SwapBuffers();
void Android_GetScreenRes( int *width, int *height );
#ifdef BELOKOCONTROLS
void Android_Events();
void Android_Move( float *forward, float *side, float *pitch, float *yaw );
void Android_DrawControls(); // android-touchif.cpp
#endif
#ifdef __cplusplus
}
#endif
