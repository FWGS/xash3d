/*
game.cpp -- executable to run Xash Engine
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


#ifdef XASH_SDL
#include <SDL_main.h>
#include <SDL_messagebox.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if _MSC_VER <= 1600
#define true 1
#define false 0
#else
#include <stdbool.h>
#endif
#ifdef __APPLE__
 #include <dlfcn.h>
 #include <errno.h>
 #define XASHLIB                "libxash.dylib"
 #define dlmount(x)          dlopen(x, RTLD_LAZY)
 #define FreeLibrary(x)          dlclose(x)
 #define GetProcAddress(x, y)    dlsym(x, y)
 #define HINSTANCE               void*
#elif __unix__
 #include <dlfcn.h>
 #include <errno.h>
 #define XASHLIB                "libxash.so"
 #define dlmount(x)         dlopen(x, RTLD_NOW)
 #define HINSTANCE               void*
#elif _WIN32
 #define dlmount(x) LoadLibraryA(x)
 #define dlclose(x) FreeLibrary(x)
 #define dlsym(x,y) GetProcAddress(x,y)
 #ifndef XASH_DEDICATED
  #define XASHLIB                 "xash_sdl.dll"
 #else
  #define XASHLIB                 "xash_dedicated.dll"
 #endif
 #include "windows.h" 
#endif

#define GAME_PATH	"valve"	// default dir to start from

typedef void (*pfnChangeGame)( const char *progname );
typedef int (*pfnInit)( int argc, char **argv, const char *progname, int bChangeGame, pfnChangeGame func );
typedef void (*pfnShutdown)( void );

pfnInit Xash_Main;
pfnShutdown Xash_Shutdown = NULL;
char szGameDir[128]; // safe place to keep gamedir
int szArgc;
char **szArgv;
HINSTANCE	hEngine;

void Xash_Error( const char *errorstring )
{
#ifdef XASH_SDL
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Xash Error", errorstring, NULL);
#else
	fprintf(stderr, "Xash Error: %s\n", errorstring);
#endif
	exit( 1 );
}

void Sys_LoadEngine( void )
{
	if(( hEngine = dlmount( XASHLIB )) == NULL )
	{
#ifndef _WIN32
		printf("%s\n", dlerror());
#endif
		Xash_Error( "Unable to load the " XASHLIB );
	}

	if(( Xash_Main = (pfnInit)dlsym( hEngine, "Host_Main" )) == NULL )
	{
#ifndef _WIN32
		printf("%s\n", dlerror());
#endif
		Xash_Error( XASHLIB " missed 'Host_Main' export" );
	}

	// this is non-fatal for us but change game will not working
	Xash_Shutdown = (pfnShutdown)dlsym( hEngine, "Host_Shutdown" );
}

void Sys_UnloadEngine( void )
{
	if( Xash_Shutdown ) Xash_Shutdown( );
	if( hEngine ) dlclose( hEngine );

	Xash_Main = NULL;
	Xash_Shutdown = NULL;
}

void Sys_ChangeGame( const char *progname )
{
	if( !progname || !progname[0] ) Xash_Error( "Sys_ChangeGame: NULL gamedir" );
	if( Xash_Shutdown == NULL ) Xash_Error( "Sys_ChangeGame: missed 'Host_Shutdown' export\n" );
	strncpy( szGameDir, progname, sizeof( szGameDir ) - 1 );

	Sys_UnloadEngine ();
	Sys_LoadEngine ();

	Xash_Main( szArgc, szArgv, szGameDir, true, ( Xash_Shutdown != NULL ) ? Sys_ChangeGame : NULL );
}


#if _WIN32 && !__MINGW32__ && _MSC_VER >= 1200 
//#pragma comment(lib, "shell32.lib")
int __stdcall WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdLine, int nShow)
#else // _WIN32
int main( int argc, char **argv )
#endif
{

#if _WIN32 && !__MINGW32__ && _MSC_VER >= 1200
	LPWSTR* lpArgv = CommandLineToArgvW(GetCommandLineW(), &szArgc);
	int size, i = 0;
	szArgv = (char**)malloc(szArgc*sizeof(char*));
	for (; i < szArgc; ++i)
	{
		size = wcslen(lpArgv[i]) + 1;
		szArgv[i] = (char*)malloc(size);
		wcstombs(szArgv[i], lpArgv[i], size);
	}
	LocalFree(lpArgv);
#else
	szArgc = argc;
	szArgv = argv;
#endif

	Sys_LoadEngine();

	return Xash_Main( szArgc, szArgv, GAME_PATH, false, ( Xash_Shutdown != NULL ) ? Sys_ChangeGame : NULL );
}
