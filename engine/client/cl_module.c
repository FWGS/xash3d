/*
cl_mobile.c - secure module loading
Copyright (C) 2015 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "common.h"
#include "client.h"
#include "library.h"
#include "APIProxy.h"

modfuncs_t g_modfuncs         = { 0 };
cldll_func_dst_t g_cldstAddrs = { 0 };

void CL_LoadSecurityModule( qboolean *critical_exports, dllfunc_t *cdll_exports )
{
	CL_EXPORT_FUNCS F; // export 'F'
	const dllfunc_t		*func;

	// trying to get single export named 'F'
	if ( ( F = (void *)Com_GetProcAddress( clgame.hInstance, "F" ) ) != NULL )
	{
		MsgDev( D_NOTE, "CL_LoadProgs: found single callback export\n" );

		// hacks!
		g_modfuncs.m_nVersion         = k_nModuleVersionCur;
		clgame.dllFuncs.pfnInitialize = &g_modfuncs;
		clgame.dllFuncs.pfnVidInit    = &g_cldstAddrs;

		// trying to fill interface now
		F( &clgame.dllFuncs );

		// check critical functions again
		for ( func = cdll_exports; func && func->name; func++ )
		{
			if ( func->func == NULL )
				break; // BAH critical function was missed
		}

		// because all the exports are loaded through function 'F"
		if ( !func || !func->name )
			*critical_exports = false;
	}
}
