/*
android.cpp -- Run Xash Engine on Android
nicknekit
*/

#ifdef __ANDROID__
#define GAME_PATH	"valve"	// default dir to start from

#include "common.h"
#include <android/log.h>

int main( int argc, char **argv )
{
    __android_log_print(ANDROID_LOG_DEBUG,"Xash","Starting xash engine...");
	return Host_Main( GAME_PATH, false, NULL );
}
#endif

