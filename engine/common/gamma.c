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

	g = bound( 1.8f, g, 3.0f );
	texGamma = bound( 1.8f, texGamma, 3.0f );

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
	if( glConfig.deviceSupportsGamma )
		return b;	// passthrough

	b = bound( 0, b, 255 );
	return texgammatable[b];
}

byte TextureToGamma( byte b )
{
	if( glConfig.deviceSupportsGamma )
		return b; // passthrough

	b = bound( 0, b, 255 );
	return gammatable[b];
}