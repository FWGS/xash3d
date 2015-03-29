/*
android.cpp -- Run Xash Engine on Android
nicknekit
*/

#ifdef __ANDROID__
#define GAME_PATH	"valve"	// default dir to start from

#include "common.h"
#include <android/log.h>

/* Include the SDL main definition header */
#include "SDL_main.h"

int SDL_main( int argc, char **argv )
{
	__android_log_print(ANDROID_LOG_DEBUG,"Xash","Starting xash engine...");
	return Host_Main( GAME_PATH, false, NULL );
}

/*******************************************************************************
				 Functions called by JNI
*******************************************************************************/
#include <jni.h>

/* Called before SDL_main() to initialize JNI bindings in SDL library */
extern void SDL_Android_Init(JNIEnv* env, jclass cls);

/* Start up the SDL app */
void Java_org_libsdl_app_SDLActivity_nativeInit(JNIEnv* env, jclass cls, jobject obj)
{
	/* This interface could expand with ABI negotiation, calbacks, etc. */
	SDL_Android_Init(env, cls);

	SDL_SetMainReady();

	/* Run the application code! */
	int status;
	char *argv[2];
	argv[0] = SDL_strdup("SDL_app");
	argv[1] = NULL;
	status = SDL_main(1, argv);

	/* Do not issue an exit or the whole application will terminate instead of just the SDL thread */
	/* exit(status); */
}
#endif

