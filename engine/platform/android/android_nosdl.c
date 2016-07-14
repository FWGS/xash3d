/*
android_nosdl.c - android backend
Copyright (C) 2016 mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#if defined(__ANDROID__) && !defined( XASH_SDL )

#include "nanogl.h" //use NanoGL
#include <pthread.h>
#include "common.h"
#include "input.h"
#include "client.h"
#include <android/log.h>
#include <jni.h>
#include <EGL/egl.h> // nanogl
convar_t *android_sleep;

static const int s_android_scantokey[] =
{
	0,				K_LEFTARROW,	K_RIGHTARROW,	K_AUX26,		K_ESCAPE,		// 0
	K_AUX26,		K_AUX25,		'0',			'1',			'2',			// 5
	'3',			'4',			'5',			'6',			'7',			// 10
	'8',			'9',			'*',			'#',			K_UPARROW,		// 15
	K_DOWNARROW,	K_LEFTARROW,	K_RIGHTARROW,	K_ENTER,		K_AUX32,		// 20
	K_AUX31,		K_AUX29,		K_AUX28,		K_AUX27,		'a',			// 25
	'b',			'c',			'd',			'e',			'f',			// 30
	'g',			'h',			'i',			'j',			'k',			// 35
	'l',			'm',			'n',			'o',			'p',			// 40
	'q',			'r',			's',			't',			'u',			// 45
	'v',			'w',			'x',			'y',			'z',			// 50
	',',			'.',			K_ALT,			K_ALT,			K_SHIFT,		// 55
	K_SHIFT,		K_TAB,			K_SPACE,		0,				0,				// 60
	0,				K_ENTER,		K_BACKSPACE,	'`',			'-',			// 65
	'=',			'[',			']',			'\\',			';',			// 70
	'\'',			'/',			'@',			K_KP_NUMLOCK,	0,				// 75
	0,				'+',			'`',			0,				0,				// 80
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
	event_set_pause,
	event_resize
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
	pthread_mutex_t mutex; // this mutex is locked while not running frame, used for events synchronization
	pthread_mutex_t framemutex; // this mutex is locked while engine is running and unlocked while it reading events, used for pause in background.
	event_t queue[ANDROID_MAX_EVENTS];
	volatile int count;
	finger_t fingers[MAX_FINGERS];
	char inputtext[256];
} events = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER };

static struct jnimethods_s
{
	jclass actcls;
	JNIEnv *env;
	jmethodID swapBuffers;
	jmethodID toggleEGL;
	jmethodID enableTextInput;
	jmethodID vibrate;
	jmethodID messageBox;
	jmethodID createGLContext;
	int width, height;
} jni;

static struct nativeegl_s
{
	qboolean valid;
	EGLDisplay dpy;
	EGLSurface surface;
} negl;

#define Android_Lock() pthread_mutex_lock(&events.mutex);
#define Android_Unlock() pthread_mutex_unlock(&events.mutex);
#define Android_PushEvent() Android_Unlock()

void Android_UpdateSurface( void );
int IN_TouchEvent( int type, int fingerID, float x, float y, float dx, float dy );
int EXPORT Host_Main( int argc, const char **argv, const char *progname, int bChangeGame, void *func );
void VID_SetMode( void );
void S_Activate( qboolean active );

/*
========================
Android_AllocEvent

Lock event queue and return pointer to next event.
Caller must do Android_PushEvent() to unlock queue after setting parameters.
========================
*/
event_t *Android_AllocEvent()
{
	Android_Lock();
	if( events.count == ANDROID_MAX_EVENTS )
	{
		events.count--; //override last event
		__android_log_print( ANDROID_LOG_ERROR, "Xash", "Too many events!!!" );
	}
	return &events.queue[ events.count++ ];
}

/*
========================
Android_RunEvents

Execute all events from queue
========================
*/
void Android_RunEvents()
{
	int i;

	// enter events read
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
			break;

		case event_key_down:
			Key_Event( events.queue[i].arg, true );
			break;
		case event_key_up:
			Key_Event( events.queue[i].arg, false );
			break;

		case event_set_pause:
			// destroy EGL surface when hiding application
			if( !events.queue[i].arg )
			{
				host.state = HOST_FRAME;
				S_Activate( true );
				(*jni.env)->CallStaticVoidMethod( jni.env, jni.actcls, jni.toggleEGL, 1 );
				Android_UpdateSurface();
				Android_SwapInterval( Cvar_VariableInteger( "gl_swapinterval" ) );
			}
			if( events.queue[i].arg )
			{
				host.state = HOST_NOFOCUS;
				S_Activate( false );
				(*jni.env)->CallStaticVoidMethod( jni.env, jni.actcls, jni.toggleEGL, 0 );
				negl.valid = false;
			}
			break;

		case event_resize:
			// reinitialize EGL and change engine screen size
			if( host.state == HOST_NORMAL && ( scr_width->integer != jni.width || scr_height->integer != jni.height ) )
			{
				(*jni.env)->CallStaticVoidMethod( jni.env, jni.actcls, jni.toggleEGL, 0 );
				(*jni.env)->CallStaticVoidMethod( jni.env, jni.actcls, jni.toggleEGL, 1 );
				Android_UpdateSurface();
				Android_SwapInterval( Cvar_VariableInteger( "gl_swapinterval" ) );
				VID_SetMode();
			}
			break;
		}
	}

	events.count = 0; // no more events

	// text input handled separately to allow unicode symbols
	for( i = 0; events.inputtext[i]; i++ )
	{
		int ch;

		// if engine does not use utf-8, we need to convert it to preferred encoding
		if( !Q_stricmp( cl_charset->string, "utf-8" ) )
			ch = (unsigned char)events.inputtext[i];
		else
			ch = Con_UtfProcessCharForce( (unsigned char)events.inputtext[i] );

		if( !ch )
			continue;

		// otherwise just push it by char, text render will decode unicode strings
		Con_CharEvent( ch );
		if( cls.key_dest == key_menu )
			UI_CharEvent ( ch );
	}
	events.inputtext[0] = 0; // no more text

	//end events read
	Android_Unlock();
	pthread_mutex_lock( &events.framemutex );
}

/*
=====================================================
JNI callbacks

On application start, setenv and onNativeResize called from
ui thread to set up engine configuration
nativeInit called directly from engine thread and will not return until exit.
These functions may be called from other threads at any time:
nativeKey
nativeTouch
onNativeResize
nativeString
nativeSetPause
=====================================================
*/
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

	jni.env = env;
	jni.actcls = (*env)->FindClass(env, "in/celest/xash3d/XashActivity");
	jni.swapBuffers = (*env)->GetStaticMethodID(env, jni.actcls, "swapBuffers", "()V");
	jni.toggleEGL = (*env)->GetStaticMethodID(env, jni.actcls, "toggleEGL", "(I)V");
	jni.enableTextInput = (*env)->GetStaticMethodID(env, jni.actcls, "showKeyboard", "(I)V");
	jni.vibrate = (*env)->GetStaticMethodID(env, jni.actcls, "vibrate", "(I)V" );
	jni.messageBox = (*env)->GetStaticMethodID(env, jni.actcls, "messageBox", "(Ljava/lang/String;Ljava/lang/String;)V");
	jni.createGLContext = (*env)->GetStaticMethodID(env, jni.actcls, "createGLContext", "()Z");

	nanoGL_Init();
	/* Run the application. */

	status = Host_Main( argc, (const char**)argv, getenv("XASH3D_GAMEDIR"), false, NULL );

	/* Release the arguments. */

	for (i = 0; i < argc; ++i) {
		free(argv[i]);
	}

	return status;
}

void Java_in_celest_xash3d_XashActivity_onNativeResize( JNIEnv* env, jclass cls, jint width, jint height )
{
	event_t *event;

	if( !width || !height )
		return;

	jni.width=width, jni.height=height;

	// alloc update event to change screen size
	event = Android_AllocEvent();
	event->type = event_resize;
	Android_PushEvent();
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

	// if pause enabled, hold engine by locking frame mutex.
	// Engine will stop after event reading and will not continue untill unlock
	if( android_sleep && android_sleep->value )
	{
		if( pause )
			pthread_mutex_lock( &events.framemutex );
		else
			pthread_mutex_unlock( &events.framemutex );
	}
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

	// not touch action?
	if( !( action >=0 && action <= 2 ) )
		return;

	// 0.0f .. 1.0f
	x /= jni.width;
	y /= jni.height;

	if( action )
		dx = x - events.fingers[finger].x, dy = y - events.fingers[finger].y;
	else
		dx = dy = 0.0f;
	events.fingers[finger].x = x, events.fingers[finger].y = y;

	// check if we should skip some events
	if( ( action == 2 ) && ( !dx && !dy ) )
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

int Java_in_celest_xash3d_XashActivity_setenv( JNIEnv* env, jclass clazz, jstring key, jstring value, jboolean overwrite )
{
	char* k = (char *) (*env)->GetStringUTFChars(env, key, NULL);
	char* v = (char *) (*env)->GetStringUTFChars(env, value, NULL);
	int err = setenv(k, v, overwrite);
	(*env)->ReleaseStringUTFChars(env, key, k);
	(*env)->ReleaseStringUTFChars(env, value, v);
	return err;
}


/*
========================
Android_SwapBuffers

Update screen. Use native EGL if possible
========================
*/
void Android_SwapBuffers()
{
	if( negl.valid )
		eglSwapBuffers( negl.dpy, negl.surface );
	else
	{
		nanoGL_Flush();
		(*jni.env)->CallStaticVoidMethod( jni.env, jni.actcls, jni.swapBuffers );
	}
}

/*
========================
Android_GetScreenRes

Resolution got from last resize event
========================
*/
void Android_GetScreenRes(int *width, int *height)
{
	*width=jni.width, *height=jni.height;
}

/*
========================
Android_Init

Initialize android-related cvars
========================
*/
void Android_Init()
{
	android_sleep = Cvar_Get( "android_sleep", "1", CVAR_ARCHIVE, "Enable sleep in background" );
}

/*
========================
Android_UpdateSurface

Check if we may use native EGL without jni calls
========================
*/
void Android_UpdateSurface( void )
{
	negl.valid = false;

	if( Sys_CheckParm("-nonativeegl") )
		return; //disabled by user

	negl.dpy = eglGetCurrentDisplay();

	if( negl.dpy == EGL_NO_DISPLAY )
		return;

	negl.surface = eglGetCurrentSurface(EGL_DRAW);

	if( negl.surface == EGL_NO_SURFACE )
		return;

	// now check if swapBuffers does not give error
	if( eglSwapBuffers( negl.dpy, negl.surface ) == EGL_FALSE )
		return;

	// double check
	if( eglGetError() != EGL_SUCCESS )
		return;

	__android_log_print( ANDROID_LOG_VERBOSE, "Xash", "native EGL enabled" );

	negl.valid = true;
}

/*
========================
Android_InitGL
========================
*/
qboolean Android_InitGL()
{
	qboolean result;
	result = (*jni.env)->CallStaticBooleanMethod( jni.env, jni.actcls, jni.createGLContext );
	Android_UpdateSurface();
	return result;
}


/*
========================
Android_EnableTextInput

Show virtual keyboard
========================
*/
void Android_EnableTextInput( qboolean enable, qboolean force )
{
	if( force )
		(*jni.env)->CallStaticVoidMethod( jni.env, jni.actcls, jni.enableTextInput, enable );
	else if( enable )
	{
		if( !host.textmode )
		{
			(*jni.env)->CallStaticVoidMethod( jni.env, jni.actcls, jni.enableTextInput, 1 );
		}
		host.textmode = true;
	}
	else
	{
		(*jni.env)->CallStaticVoidMethod( jni.env, jni.actcls, jni.enableTextInput, 0 );
		host.textmode = false;
	}
}

/*
========================
Android_Vibrate
========================
*/
void Android_Vibrate( float life, char flags )
{
	if( life )
		(*jni.env)->CallStaticVoidMethod( jni.env, jni.actcls, jni.vibrate, (int)life );
}


/*
========================
Android_MessageBox

Show messagebox and wait for OK button press
========================
*/
void Android_MessageBox(const char *title, const char *text)
{
	(*jni.env)->CallStaticVoidMethod( jni.env, jni.actcls, jni.messageBox, (*jni.env)->NewStringUTF( jni.env, title ), (*jni.env)->NewStringUTF( jni.env ,text ) );
}



/*
========================
Android_SwapInterval
========================
*/
void Android_SwapInterval( int interval )
{
	// there is no eglSwapInterval in EGL10/EGL11 classes,
	// so only native backend supported
	if( negl.valid )
		eglSwapInterval( negl.dpy, interval );
}

#endif
