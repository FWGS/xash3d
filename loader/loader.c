/*
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wine/module.h"
#include "wine/pe_image.h"
#include "wine/winbase.h"
#include "debug.h"

// Wrapper functions (to convert __cdecl to __stdcall)
void * Loader_LoadLibrary (const char *name)
{
	printf("Loader_LoadLibrary( \"%s\" )\n", name);
    return (void *)LoadLibraryA(name);
}
void * Loader_GetProcAddress (void *hndl, const char *name)
{ 
	dbg_printf("Loader_GetProcAdderss( %p, \"%s\" )\n", hndl, name);
	return (void *)GetProcAddress((HMODULE)hndl, name);
}
void Loader_FreeLibrary(void *hndl)
{
	printf("Loader_FreeLibrary( %p )\n", hndl);
	FreeLibrary((HMODULE)hndl);
}

void *Loader_GetDllHandle( void *hndl )
{
	dbg_printf("Loader_GetDllHandle( %p )\n", hndl);
	return (void*)MODULE32_LookupHMODULE( (HMODULE)hndl );
}

const char * Loader_GetFuncName( void *hndl, void *func)
{
	WINE_MODREF *wm = MODULE32_LookupHMODULE( (HMODULE)hndl );
	if(wm) return PE_FindFunctionName(wm, func);
	return 0;
}

const char * Loader_GetFuncName_int( void *wm , void *func)
{
	const char *s = PE_FindFunctionName(wm, func);
	printf("Loader_GetFuncName_int( %p ) => \"%s\"\n", func, s);
	return s;
}
