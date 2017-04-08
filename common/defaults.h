/*
defaults.h - set up default configuration
Copyright (C) 2016 Mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef DEFAULTS_H
#define DEFAULTS_H

#include "backends.h"

/*
===================================================================

SETUP BACKENDS DEFINATIONS

===================================================================
*/
#ifndef XASH_DEDICATED

	#ifdef XASH_SDL

		#ifndef XASH_VIDEO

			// special case for nanogl builds
			#ifdef XASH_NANOGL
				#define XASH_VIDEO VIDEO_SDL_NANOGL // VIDEO_SDL_NANOGL
			#else
				#define XASH_VIDEO VIDEO_SDL
			#endif // XASH_NANOGL

		#endif // XASH_VIDEO

		// by default, use SDL subsystems
		#ifndef XASH_TIMER
			#define XASH_TIMER TIMER_SDL
		#endif

		#ifndef XASH_INPUT
			#define XASH_INPUT INPUT_SDL
		#endif

		#ifndef XASH_SOUND
			#define XASH_SOUND SOUND_SDL
		#endif

	#endif //XASH_SDL

	#if defined __ANDROID__ && !defined XASH_SDL

		#ifndef XASH_VIDEO
			#define XASH_VIDEO VIDEO_ANDROID
		#endif

		#ifndef XASH_TIMER
			#define XASH_TIMER TIMER_LINUX
		#endif

		#ifndef XASH_INPUT
			#define XASH_INPUT INPUT_ANDROID
		#endif

		#ifndef XASH_SOUND
			#define XASH_SOUND SOUND_OPENSLES
		#endif
	#endif // android case

#endif // XASH_DEDICATED

// select crashhandler based on defines
#ifndef XASH_CRASHHANDLER
	#ifdef _WIN32
		#ifdef DBGHELP
			#define XASH_CRASHHANDLER CRASHHANDLER_DBGHELP
		#endif
	#elif defined CRASHHANDLER
		#define XASH_CRASHHANDLER CRASHHANDLER_UCONTEXT
	#else
		#define XASH_CRASHHANDLER CRASHHANDLER_NULL
	#endif
#endif

// no timer - no xash
#ifndef XASH_TIMER
	#ifdef _WIN32
		#define XASH_TIMER TIMER_WIN32
	#else
		#define XASH_TIMER TIMER_LINUX
	#endif
#endif

//
// fallback to NULL
//
#ifndef XASH_VIDEO
	#define XASH_VIDEO VIDEO_NULL
#endif

#ifndef XASH_SOUND
	#define XASH_SOUND SOUND_NULL
#endif

#ifndef XASH_INPUT
	#define XASH_INPUT INPUT_NULL
#endif

/*
=========================================================================

Default build-depended cvar and constant values

=========================================================================
*/

#if defined __ANDROID__ || TARGET_OS_IPHONE
	#define DEFAULT_TOUCH_ENABLE "1"
	#define DEFAULT_M_IGNORE "1"
#else
	#define DEFAULT_TOUCH_ENABLE "0"
	#define DEFAULT_M_IGNORE "0"
#endif

#define DEFAULT_SV_MASTER "ms.xash.su:27010"

// allow override for developer/debug builds
#ifndef DEFAULT_DEV
	#define DEFAULT_DEV 0
#endif

#ifndef DEFAULT_FULLSCREEN
#define DEFAULT_FULLSCREEN 1
#endif

#if TARGET_OS_IPHONE
    #define DEFAULT_CON_MAXFRAC "0.5"
#else
	#define DEFAULT_CON_MAXFRAC "1"
#endif

#endif // DEFAULTS_H
