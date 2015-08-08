#ifdef __cplusplus
extern "C" {
#endif
void Android_SwapBuffers();
void Android_GetScreenRes( int *width, int *height );
void Android_Events();
void Android_Move( usercmd_t *cmd );
void Android_DrawControls(); // android-touchif.cpp
#ifdef __cplusplus
}
#endif
