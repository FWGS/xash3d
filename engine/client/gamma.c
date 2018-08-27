/*
gamma.c - gamma routines
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
#ifndef XASH_DEDICATED
#include "common.h"
#include <mathlib.h>
#include "gl_local.h"

//-----------------------------------------------------------------------------
// Gamma conversion support
//-----------------------------------------------------------------------------
static byte	gammatable[256];
static byte	texgammatable[256];		// palette is sent through this to convert to screen gamma

void BuildGammaTable( float gamma, float texGamma )
{
	int	i, inf;
	float	g1, g = gamma;
	double	f;

	// In settings only 1.8-7.0, but give user more values with cvar
	g = bound( MIN_GAMMA, g, MAX_GAMMA );
	texGamma = bound( 1.0f, texGamma, 15.0f );

	g = 1.0f / g;
	g1 = texGamma * g; 

	for( i = 0; i < 256; i++ )
	{
		inf = 255 * pow( i / 255.f, g1 ); 
		texgammatable[i] = bound( 0, inf, 255 );
	}

	for( i = 0; i < 256; i++ )
	{
		f = 255.0 * pow(( float )i / 255.0f, 2.2f / texGamma );
		inf = (int)(f + 0.5f);
		gammatable[i] = bound( 0, inf, 255 );
	}
}

byte TextureToTexGamma( byte b )
{
	//b = bound( 0, b, 255 );
	return texgammatable[b];
}

byte TextureToGamma( byte b )
{
	//b = bound( 0, b, 255 );
	return gammatable[b];
}
#endif
