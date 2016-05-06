/*
android.cpp -- Run Xash Engine on Android
nicknekit
*/

#ifdef __ANDROID__
#define GAME_PATH	"valve"	// default dir to start from

#include "common.h"
#include <android/log.h>
#include <jni.h>




#ifdef XASH_SDL
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

#else

#include "nanogl.h" //use NanoGL
#include <pthread.h>
#include "common.h"
#include "input.h"
#include "client.h"
static const int s_android_scantokey[] =
{
	0,				K_LEFTARROW,	K_RIGHTARROW,	0,				K_ESCAPE,		// 0
	0,				0,				'0',			'1',			'2',			// 5
	'3',			'4',			'5',			'6',			'7',			// 10
	'8',			'9',			'*',			'#',			K_UPARROW,		// 15
	K_DOWNARROW,	K_LEFTARROW,	K_RIGHTARROW,	K_ENTER,		0,				// 20
	0,				0,				0,				0,				'a',			// 25
	'b',			'c',			'd',			'e',			'f',			// 30
	'g',			'h',			'i',			'j',			'k',			// 35
	'l',			'm',			'n',			'o',			'p',			// 40
	'q',			'r',			's',			't',			'u',			// 45
	'v',			'w',			'x',			'y',			'z',			// 50
	',',			'.',			K_ALT,			K_ALT,			K_SHIFT,		// 55
	K_SHIFT,		K_TAB,			K_SPACE,		0,				0,				// 60
	0,				K_ENTER,		K_BACKSPACE,	'`',			'-',			// 65
	'=',			'[',			']',			'\\',			';',			// 70
	'\'',			'/',			'@',			K_KP_NUMLOCK,		0,				// 75
	0,				'+',			0,				0,				0,				// 80
	0,				0,				0,				0,				0,				// 85
	0,				0,				K_PGUP,			K_PGDN,			0,				// 90
	0,				K_A_BUTTON,		K_B_BUTTON,		K_C_BUTTON,		K_X_BUTTON,		// 95
	K_Y_BUTTON,		K_Z_BUTTON,		K_LSHOULDER,	K_RSHOULDER,	K_LTRIGGER,		// 100
	K_RTRIGGER,		K_LSTICK,		K_RSTICK,		K_ESCAPE,		K_ESCAPE,		// 105
	0,				K_ESCAPE,		K_DEL,			K_CTRL,			K_CTRL,		// 110
	K_CAPSLOCK,		0,	0,				0,				0,				// 115
	0,				K_PAUSE,		K_HOME,			K_END,			K_INS,			// 120
	0,				0,				0,				0,				0,				// 125
	0,				K_F1,			K_F2,			K_F3,			K_F4,			// 130
	K_F5,			K_F6,			K_F7,			K_F8,			K_F9,			// 135
	K_F10,			K_F11,			K_F12,			K_KP_NUMLOCK,		K_KP_INS,			// 140
	K_KP_END,		K_KP_DOWNARROW,	K_KP_PGDN,		K_KP_LEFTARROW,	K_KP_5,			// 145
	K_KP_RIGHTARROW,K_KP_HOME,		K_KP_UPARROW,		K_KP_PGUP,		K_KP_SLASH,		// 150
	0,		K_KP_MINUS,		K_KP_PLUS,		K_KP_DEL,			',',			// 155
	K_KP_ENTER,		'=',		'(',			')'
};

jclass gClass;
JNIEnv *gEnv;
jmethodID gSwapBuffers;
jmethodID gRestoreEGL;
JavaVM *gVM;
int gWidth, gHeight;
/*JNIEXPORT jint JNICALL JNIOnLoad(JavaVM * vm, void*)
{
	gVM = vm;
	
	return JNI_VERSION_1_6;
}*/

#define ANDROID_MAX_EVENTS 64
#define MAX_FINGERS 10
typedef enum event_type
{
	event_touch_down,
	event_touch_up,
	event_touch_move,
	event_key_down,
	event_key_up,
	event_motion,
	event_set_pause
} eventtype_t;

typedef struct event_s
{
	eventtype_t type;
	int arg; // finger or key id
	float x, y, dx, dy;
} event_t;

typedef struct finger_s
{
	float x, y;
	qboolean down;
} finger_t;

static struct {
	pthread_mutex_t mutex;
	pthread_mutex_t framemutex;
	event_t queue[ANDROID_MAX_EVENTS];
	volatile int count;
	finger_t fingers[MAX_FINGERS];
	char inputtext[256];
} events = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER };

#define Android_Lock() pthread_mutex_lock(&events.mutex);
#define Android_Unlock() pthread_mutex_unlock(&events.mutex);
#define Android_PushEvent() Android_Unlock()
int IN_TouchEvent( int type, int fingerID, float x, float y, float dx, float dy );

event_t *Android_AllocEvent()
{
	Android_Lock();
	if( events.count == ANDROID_MAX_EVENTS )
		events.count--; //override last event
	return &events.queue[ events.count++ ];
}

void Android_RunEvents()
{
	int i;
	Android_Lock();
	pthread_mutex_unlock( &events.framemutex );
	for( i = 0; i < events.count; i++ )
	{
		switch( events.queue[i].type )
		{
		case event_touch_down:
		case event_touch_up:
		case event_touch_move:
			IN_TouchEvent( events.queue[i].type, events.queue[i].arg, events.queue[i].x, events.queue[i].y, events.queue[i].dx, events.queue[i].dy );
			Msg("T: %i %d %d %f %f %f %f\n", i,  events.queue[i].type, events.queue[i].arg, events.queue[i].x, events.queue[i].y, events.queue[i].dx, events.queue[i].dy );
		break;
		case event_key_down:
			Key_Event( events.queue[i].arg, true );
			break;
		case event_key_up:
			Key_Event( events.queue[i].arg, false );
			break;
		case event_set_pause:
			if( !events.queue[i].arg )
			{
				host.state = HOST_FRAME;
				(*gEnv)->CallStaticVoidMethod(gEnv, gClass, gRestoreEGL);

			}
			if( events.queue[i].arg )
			{
				host.state = HOST_NOFOCUS;
			}
			//sleep(1);
			//pthread_cond_signal( &events.framecond );
			break;
		}
	}
	events.count = 0;
	for( i = 0; events.inputtext[i]; i++ )
	{
		int ch;

		if( !Q_stricmp( cl_charset->string, "utf-8" ) )
			ch = (unsigned char)events.inputtext[i];
		else
			ch = Con_UtfProcessCharForce( (unsigned char)events.inputtext[i] );

		if( !ch )
			continue;

		Con_CharEvent( ch );
		if( cls.key_dest == key_menu )
			UI_CharEvent ( ch );
	}
	events.inputtext[0] = 0;
	Android_Unlock();
	pthread_mutex_unlock( &events.framemutex );
}

int Java_in_celest_xash3d_XashActivity_nativeInit(JNIEnv* env, jclass cls, jobject array)
{
    int i;
    int argc;
    int status;
    /* Prepare the arguments. */

    int len = (*env)->GetArrayLength(env, array);
    char* argv[1 + len + 1];
    argc = 0;
    argv[argc++] = strdup("app_process");
    for (i = 0; i < len; ++i) {
        const char* utf;
        char* arg = NULL;
        jstring string = (*env)->GetObjectArrayElement(env, array, i);
        if (string) {
            utf = (*env)->GetStringUTFChars(env, string, 0);
            if (utf) {
                arg = strdup(utf);
                (*env)->ReleaseStringUTFChars(env, string, utf);
            }
            (*env)->DeleteLocalRef(env, string);
        }
        if (!arg) {
            arg = strdup("");
        }
        argv[argc++] = arg;
    }
    argv[argc] = NULL;

    /* Init callbacks. */

    gEnv = env;
    gClass = (*env)->FindClass(env, "in/celest/xash3d/XashActivity");
    gSwapBuffers = (*env)->GetStaticMethodID(env, gClass, "swapBuffers", "()V");
	gRestoreEGL = (*env)->GetStaticMethodID(env, gClass, "restoreEGL", "()V");

    nanoGL_Init();
    /* Run the application. */

    status = Host_Main(argc, argv, GAME_PATH, false, NULL);

    /* Release the arguments. */

    for (i = 0; i < argc; ++i) {
        free(argv[i]);
    }

    return status;
}

void Java_in_celest_xash3d_XashActivity_onNativeResize(JNIEnv* env, jclass cls, jint width, jint height)
{
	gWidth=width, gHeight=height;
}

void Java_in_celest_xash3d_XashActivity_nativeQuit(JNIEnv* env, jclass cls)
{
}
void Java_in_celest_xash3d_XashActivity_nativeSetPause(JNIEnv* env, jclass cls, jint pause )
{
	event_t *event = Android_AllocEvent();
	event->type = event_set_pause;
	event->arg = pause;
	Android_PushEvent();
	pthread_mutex_lock( &events.framemutex );
	pthread_mutex_unlock( &events.framemutex );
	//pthread_cond_wait( &events.framecond, &events.mutex );
}

void Java_in_celest_xash3d_XashActivity_nativeKey(JNIEnv* env, jclass cls, jint down, jint code)
{
	event_t *event;

	if( code >= ( sizeof( s_android_scantokey ) / sizeof( s_android_scantokey[0] ) ) )
		return;

	event = Android_AllocEvent();

	if( down )
		event->type = event_key_down;
	else
		event->type = event_key_up;

	event->arg = s_android_scantokey[code];

	Android_PushEvent();
}

void Java_in_celest_xash3d_XashActivity_nativeString(JNIEnv* env, jclass cls, jobject string)
{
	char* str = (char *) (*env)->GetStringUTFChars(env, string, NULL);
	Android_Lock();
	strncat( events.inputtext, str, 256 );
	Android_Unlock();
	(*env)->ReleaseStringUTFChars(env, string, str);
}

#ifdef SOFTFP_LINK
void Java_in_celest_xash3d_XashActivity_nativeTouch(JNIEnv* env, jclass cls, jint finger, jint action, jfloat x, jfloat y ) __attribute__((pcs("aapcs")));
#endif
void Java_in_celest_xash3d_XashActivity_nativeTouch(JNIEnv* env, jclass cls, jint finger, jint action, jfloat x, jfloat y )
{
	float dx, dy;
	event_t *event;

	// if something wrong with android event
	if( finger > MAX_FINGERS )
		return;

	if( !( action >=0 && action <= 2 ) )
		return;

	x /= gWidth;
	y /= gHeight;

	if( action )
		dx = x - events.fingers[finger].x, dy = y - events.fingers[finger].y;
	else
		dx = dy = 0.0f;
	events.fingers[finger].x = x, events.fingers[finger].y = y;

	// check if we should skip some events
	if( ( action == 2 ) && ( !dx || !dy ) )
		return;

	if( ( action == 0 ) && events.fingers[finger].down )
		return;

	if( ( action == 1 ) && !events.fingers[finger].down )
		return;

	if( action == 2 && !events.fingers[finger].down )
			action = 0;

	if( action == 0 )
		events.fingers[finger].down = true;
	else if( action == 1 )
		events.fingers[finger].down = false;

	event = Android_AllocEvent();
	event->arg = finger;
	event->type = action;
	event->x = x;
	event->y = y;
	event->dx = dx;
	event->dy = dy;
	Android_PushEvent();
}

int Java_in_celest_xash3d_XashActivity_setenv
  (JNIEnv* env, jclass clazz, jstring key, jstring value, jboolean overwrite)
{
	char* k = (char *) (*env)->GetStringUTFChars(env, key, NULL);
	char* v = (char *) (*env)->GetStringUTFChars(env, value, NULL);
	int err = setenv(k, v, overwrite);
	(*env)->ReleaseStringUTFChars(env, key, k);
	(*env)->ReleaseStringUTFChars(env, value, v);
	return err;
}
void Android_SwapBuffers()
{
	nanoGL_Flush();
	(*gEnv)->CallStaticVoidMethod(gEnv, gClass, gSwapBuffers);
}

void Android_GetScreenRes(int *width, int *height)
{
	*width=gWidth, *height=gHeight;
}
void Android_Vibrate( float life, char flags )
{
	//stub
}
#endif
#endif

