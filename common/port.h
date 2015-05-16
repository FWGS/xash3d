/*
port.h -- Portability Layer for Windows types
Copyright (C) 2015 Alibek Omarov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef PORT_H
#define PORT_H

#ifdef XASH_VGUI
    #if !(defined(__i386__) || defined(_X86_))
	#error "VGUI is exists only for x86. You must disable VGUI flag or build Xash3D for x86 target."
    #endif
#endif

#ifndef _WIN32
    #include <limits.h>
    #include <dlfcn.h>

    #ifdef __APPLE__
	#include <sys/syslimits.h>
	#define OS_LIB_EXT "dylib"
    #else
	#include <linux/limits.h>
	#define OS_LIB_EXT "so"
    #endif

    #ifdef __ANDROID__
	#define MENUDLL "libmenu." OS_LIB_EXT
	#define CLIENTDLL "libclient." OS_LIB_EXT
	#define SERVERDLL "libserver." OS_LIB_EXT
	#define APPPATH "/data/data/in.celest.xash3d.hl/"
	#define LIBPATH APPPATH "lib"
	#define ASSETSPATH APPPATH "assets"
	#define GAMEPATH "/sdcard/xash"
    #else
	#define MENUDLL "libxashmenu." OS_LIB_EXT
    #ifdef PANDORA
	#define CLIENTDLL "client." OS_LIB_EXT
 	#define SERVERDLL "hl." OS_LIB_EXT
	#define LIBPATH "."
	#define GAMEPATH "."
    #else
	#define CLIENTDLL "client." OS_LIB_EXT
    #endif
    #endif

    #define TRUE	    1
    #define FALSE	    0

    // Windows-specific
    #define _stdcall
    #define __stdcall
    #define __cdecl
    #define _inline	    inline
    #define O_BINARY    0		//In Linux O_BINARY didn't exist

    // Windows functions to Linux equivalent
    #define _mkdir( x ) mkdir( x, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH )
    #define LoadLibrary( x ) dlopen( x, RTLD_NOW )
    #define GetProcAddress( x, y ) dlsym( x, y )
    #define FreeLibrary( x ) dlclose( x )
    #define MAKEWORD(a,b)   ((short int)(((unsigned char)(a))|(((short int)((unsigned char)(b)))<<8)))
    #define max(a, b) (a) > (b) ? (a) : (b)
    #define min(a, b) (a) > (b) ? (b) : (a)
    #define tell(a) lseek(a, 0, SEEK_CUR)

    typedef unsigned char   BYTE;
    typedef unsigned char   byte;
    typedef short int	    WORD;
    typedef unsigned int    DWORD;
    typedef long int	    LONG;
    typedef unsigned long int   ULONG;
    typedef long	    WPARAM;
    typedef unsigned int    LPARAM;

    typedef void* HANDLE;
    typedef void* HMODULE;
    typedef void* HINSTANCE;

    typedef char* LPSTR;

    typedef struct tagPOINT
    {
	int x, y;
    } POINT;
#else
    #define OS_LIB_EXT "dll"
    #define MENUDLL "mainui." OS_LIB_EXT
    #define CLIENTDLL "client." OS_LIB_EXT
#endif

#endif
