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

#ifndef _WIN32
    #include <limits.h>

    #ifdef __APPLE__
	#define OS_LIB_EXT "dylib"
    #else
	#define OS_LIB_EXT "so"
    #endif

    #define TRUE	    1
    #define FALSE	    0

    // Windows-specific
    #define _stdcall
    #define _inline	    inline
    #define O_BINARY    0		//In Linux O_BINARY didn't exist

    // Windows functions to Linux equivalent
    #define _mkdir( x ) mkdir( x, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH )
    #define LoadLibrary( x ) dlopen( x, RTLD_LAZY )
    #define GetProcAddress( x, y ) dlsym( x, y )
    #define FreeLibrary( x ) dlclose( x )

    typedef unsigned char	    BYTE;
    typedef unsigned char	    byte;
    typedef short int	    WORD;
    typedef unsigned int	    DWORD;
    typedef long int	    LONG;
    typedef unsigned long int   ULONG;

    typedef void* HANDLE;
    typedef void* HMODULE;
    typedef void* HINSTANCE;

    typedef char* LPSTR;

    typedef struct POINT_s
    {
	int x, y;
    } POINT;
#else
    #define OS_LIB_EXT "dll"
#endif

#endif
