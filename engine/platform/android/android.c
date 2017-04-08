/*
android.cpp -- Run Xash Engine on Android
nicknekit
*/

#if defined(__ANDROID__) && defined(XASH_SDL)

#define GAME_PATH	"valve"	// default dir to start from
#include "common.h"
#include <android/log.h>
#include <jni.h>
/* Include the SDL main definition header */
#include "SDL_main.h"

int SDL_main( int argc, char **argv )
{
	__android_log_print(ANDROID_LOG_DEBUG,"Xash","Starting xash engine...");
	return Host_Main(argc, argv, getenv("XASH3D_GAMEDIR"), false, NULL );
}

/*******************************************************************************
				 Functions called by JNI
*******************************************************************************/


/* Called before SDL_main() to initialize JNI bindings in SDL library */
extern void SDL_Android_Init(JNIEnv* env, jclass cls);

/* Start up the SDL app */
int Java_org_libsdl_app_SDLActivity_nativeInit(JNIEnv* env, jclass cls, jobject array)
{
    int i;
    int argc;
    int status;

    /* This interface could expand with ABI negotiation, callbacks, etc. */
    SDL_Android_Init(env, cls);

    SDL_SetMainReady();

    /* Prepare the arguments. */

    int len = (*env)->GetArrayLength(env, array);
    char* argv[1 + len + 1];
    argc = 0;
    /* Use the name "app_process" so PHYSFS_platformCalcBaseDir() works.
       https://bitbucket.org/MartinFelis/love-android-sdl2/issue/23/release-build-crash-on-start
     */
    argv[argc++] = SDL_strdup("app_process");
    for (i = 0; i < len; ++i) {
        const char* utf;
        char* arg = NULL;
        jstring string = (*env)->GetObjectArrayElement(env, array, i);
        if (string) {
            utf = (*env)->GetStringUTFChars(env, string, 0);
            if (utf) {
                arg = SDL_strdup(utf);
                (*env)->ReleaseStringUTFChars(env, string, utf);
            }
            (*env)->DeleteLocalRef(env, string);
        }
        if (!arg) {
            arg = SDL_strdup("");
        }
        argv[argc++] = arg;
    }
    argv[argc] = NULL;


    /* Run the application. */

    status = SDL_main(argc, argv);

    /* Release the arguments. */

    for (i = 0; i < argc; ++i) {
        SDL_free(argv[i]);
    }

    /* Do not issue an exit or the whole application will terminate instead of just the SDL thread */
    /* exit(status); */

    return status;
}

int Java_org_libsdl_app_SDLActivity_setenv
  (JNIEnv* env, jclass clazz, jstring key, jstring value, jboolean overwrite)
{
    char* k = (char *) (*env)->GetStringUTFChars(env, key, NULL);
    char* v = (char *) (*env)->GetStringUTFChars(env, value, NULL);
    int err = setenv(k, v, overwrite);
    (*env)->ReleaseStringUTFChars(env, key, k);
    (*env)->ReleaseStringUTFChars(env, value, v);
    return err;
}

void *SDL_AndroidGetJNIEnv();
void *SDL_AndroidGetActivity();

JNIEnv *env = NULL;
jmethodID vibrmid;
jclass actcls;
jobject activity;
void Android_GetMethods()
{
	env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	activity = (jobject)SDL_AndroidGetActivity();
	actcls = (*env)->GetObjectClass(env, activity);
	vibrmid = (*env)->GetMethodID(env, actcls, "vibrate", "(S)V");
}

void Android_Vibrate( float life, char flags )
{
	int time = (int)life;

	if( !env )
		 Android_GetMethods();

	if (vibrmid == 0)
		return;

	(*env)->CallVoidMethod(env, activity, vibrmid, time);
}

#endif // __ANDROID__ && XASH_SDL
