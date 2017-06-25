/*
gl_warp.c - sky and water polygons
Copyright (C) 2010 Uncle Mike

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
#include "client.h"
#include "gl_local.h"
#include "com_model.h"
#include "wadfile.h"

#define MAX_CLIP_VERTS	64 // skybox clip vertices
#define TURBSCALE		( 256.0f / ( M_PI2 ))
static float		speedscale;
static const char*		r_skyBoxSuffix[6] = { "rt", "bk", "lf", "ft", "up", "dn" };
static const int		r_skyTexOrder[6] = { 0, 2, 1, 3, 4, 5 };

static const vec3_t skyclip[6] =
{
{  1,  1,  0 },
{  1, -1,  0 },
{  0, -1,  1 },
{  0,  1,  1 },
{  1,  0,  1 },
{ -1,  0,  1 }
};

// 1 = s, 2 = t, 3 = 2048
static const int st_to_vec[6][3] =
{
{  3, -1,  2 },
{ -3,  1,  2 },
{  1,  3,  2 },
{ -1, -3,  2 },
{ -2, -1,  3 },  // 0 degrees yaw, look straight up
{  2, -1, -3 }   // look straight down
};

// s = [0]/[2], t = [1]/[2]
static const int vec_to_st[6][3] =
{
{ -2,  3,  1 },
{  2,  3, -1 },
{  1,  3,  2 },
{ -1,  3, -2 },
{ -2, -1,  3 },
{ -2,  1, -3 }
};

// speed up sin calculations
float r_turbsin[] =
{
	#include "warpsin.h"
};

static qboolean CheckSkybox( const char *name )
{
	const char	*skybox_ext[] = { "dds", "tga", "bmp" };
	int		i, j, num_checked_sides;
	const char	*sidename;

	// search for skybox images				
	for( i = 0; i < sizeof( skybox_ext ) / sizeof( skybox_ext[0] ); i++ )
	{	
		num_checked_sides = 0;
		for( j = 0; j < 6; j++ )
		{         
			// build side name
			sidename = va( "%s%s.%s", name, r_skyBoxSuffix[j], skybox_ext[i] );
			if( FS_FileExists( sidename, false ))
				num_checked_sides++;

		}

		if( num_checked_sides == 6 )
			return true; // image exists

		for( j = 0; j < 6; j++ )
		{         
			// build side name
			sidename = va( "%s_%s.%s", name, r_skyBoxSuffix[j], skybox_ext[i] );
			if( FS_FileExists( sidename, false ))
				num_checked_sides++;
		}

		if( num_checked_sides == 6 )
			return true; // images exists
	}
	return false;
}

void DrawSkyPolygon( int nump, vec3_t vecs )
{
	int	i, j, axis;
	float	s, t, dv, *vp;
	vec3_t	v, av;

	// decide which face it maps to
	VectorClear( v );

	for( i = 0, vp = vecs; i < nump; i++, vp += 3 )
		VectorAdd( vp, v, v );

	av[0] = fabs( v[0] );
	av[1] = fabs( v[1] );
	av[2] = fabs( v[2] );

	if( av[0] > av[1] && av[0] > av[2] )
		axis = (v[0] < 0) ? 1 : 0;
	else if( av[1] > av[2] && av[1] > av[0] )
		axis = (v[1] < 0) ? 3 : 2;
	else axis = (v[2] < 0) ? 5 : 4;

	// project new texture coords
	for( i = 0; i < nump; i++, vecs += 3 )
	{
		j = vec_to_st[axis][2];
		dv = (j > 0) ? vecs[j-1] : -vecs[-j-1];

		if( dv == 0 ) continue;

		j = vec_to_st[axis][0];
		s = (j < 0) ? -vecs[-j-1] / dv : vecs[j-1] / dv;

		j = vec_to_st[axis][1];
		t = (j < 0) ? -vecs[-j-1] / dv : vecs[j-1] / dv;

		if( s < RI.skyMins[0][axis] ) RI.skyMins[0][axis] = s;
		if( t < RI.skyMins[1][axis] ) RI.skyMins[1][axis] = t;
		if( s > RI.skyMaxs[0][axis] ) RI.skyMaxs[0][axis] = s;
		if( t > RI.skyMaxs[1][axis] ) RI.skyMaxs[1][axis] = t;
	}
}

/*
==============
ClipSkyPolygon
==============
*/
void ClipSkyPolygon( int nump, vec3_t vecs, int stage )
{
	const float	*norm;
	float		*v, d, e;
	qboolean		front, back;
	float		dists[MAX_CLIP_VERTS + 1];
	int		sides[MAX_CLIP_VERTS + 1];
	vec3_t		newv[2][MAX_CLIP_VERTS + 1];
	int		newc[2];
	int		i, j;

	if( nump > MAX_CLIP_VERTS )
		Host_Error( "ClipSkyPolygon: MAX_CLIP_VERTS\n" );
loc1:
	if( stage == 6 )
	{	
		// fully clipped, so draw it
		DrawSkyPolygon( nump, vecs );
		return;
	}

	front = back = false;
	norm = skyclip[stage];
	for( i = 0, v = vecs; i < nump; i++, v += 3 )
	{
		d = DotProduct( v, norm );
		if( d > ON_EPSILON )
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if( d < -ON_EPSILON )
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		dists[i] = d;
	}

	if( !front || !back )
	{	
		// not clipped
		stage++;
		goto loc1;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy( vecs, ( vecs + ( i * 3 )));
	newc[0] = newc[1] = 0;

	for( i = 0, v = vecs; i < nump; i++, v += 3 )
	{
		switch( sides[i] )
		{
		case SIDE_FRONT:
			VectorCopy( v, newv[0][newc[0]] );
			newc[0]++;
			break;
		case SIDE_BACK:
			VectorCopy( v, newv[1][newc[1]] );
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy( v, newv[0][newc[0]] );
			newc[0]++;
			VectorCopy( v, newv[1][newc[1]] );
			newc[1]++;
			break;
		}

		if( sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i] )
			continue;

		d = dists[i] / ( dists[i] - dists[i+1] );
		for( j = 0; j < 3; j++ )
		{
			e = v[j] + d * ( v[j+3] - v[j] );
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon( newc[0], newv[0][0], stage + 1 );
	ClipSkyPolygon( newc[1], newv[1][0], stage + 1 );
}

void MakeSkyVec( float s, float t, int axis )
{
	int	j, k, farclip;
	vec3_t	v, b;

	farclip = RI.farClip;

	b[0] = s * (farclip >> 1);
	b[1] = t * (farclip >> 1);
	b[2] = (farclip >> 1);

	for( j = 0; j < 3; j++ )
	{
		k = st_to_vec[axis][j];
		v[j] = (k < 0) ? -b[-k-1] : b[k-1];
		v[j] += RI.cullorigin[j];
	}

	// avoid bilerp seam
	s = (s + 1.0f) * 0.5f;
	t = (t + 1.0f) * 0.5f;

	if( s < 1.0f / 512.0f )
		s = 1.0f / 512.0f;
	else if( s > 511.0f / 512.0f )
		s = 511.0f / 512.0f;
	if( t < 1.0f / 512.0f )
		t = 1.0f / 512.0f;
	else if( t > 511.0f / 512.0f )
		t = 511.0f / 512.0f;

	t = 1.0f - t;

	pglTexCoord2f( s, t );
	pglVertex3fv( v );
}

/*
==============
R_ClearSkyBox
==============
*/
void R_ClearSkyBox( void )
{
	int	i;

	for( i = 0; i < 6; i++ )
	{
		RI.skyMins[0][i] = RI.skyMins[1][i] = 9999999.0f;
		RI.skyMaxs[0][i] = RI.skyMaxs[1][i] = -9999999.0f;
	}
}

/*
=================
R_AddSkyBoxSurface
=================
*/
void R_AddSkyBoxSurface( msurface_t *fa )
{
	vec3_t	verts[MAX_CLIP_VERTS];
	glpoly_t	*p;
	int	i;
	float *verts_p;

	if( r_fastsky->integer )
		return;

	if( clgame.movevars.skyangle )
	{
		// HACK: force full sky to draw when it has angle
		for( i = 0; i < 6; i++ )
		{
			RI.skyMins[0][i] = RI.skyMins[1][i] = -1.0f;
			RI.skyMaxs[0][i] = RI.skyMaxs[1][i] = 1.0f;
		}
	}

	// calculate vertex values for sky box
	for( p = fa->polys, verts_p = (float *)p + ( ( sizeof( void* ) + sizeof( int ) ) >> 1 ); p; p = p->next )
	{
		for( i = 0; i < p->numverts; i++ )
			VectorSubtract( &verts_p[VERTEXSIZE * i], RI.cullorigin, verts[i] );
		ClipSkyPolygon( p->numverts, verts[0], 0 );
	}
}

/*
==============
R_UnloadSkybox

Unload previous skybox
==============
*/
void R_UnloadSkybox( void )
{
	int	i;

	// release old skybox
	for( i = 0; i < 6; i++ )
	{
		if( !tr.skyboxTextures[i] ) continue;
		GL_FreeTexture( tr.skyboxTextures[i] );
	}

	tr.skyboxbasenum = 5800;	// set skybox base (to let some mods load hi-res skyboxes)

	Q_memset( tr.skyboxTextures, 0, sizeof( tr.skyboxTextures ));
}

/*
==============
R_DrawSkybox
==============
*/
void R_DrawSkyBox( void )
{
	int	i;

	if( clgame.movevars.skyangle )
	{	
		// check for no sky at all
		for( i = 0; i < 6; i++ )
		{
			if( RI.skyMins[0][i] < RI.skyMaxs[0][i] && RI.skyMins[1][i] < RI.skyMaxs[1][i] )
				break;
		}

		if( i == 6 ) return; // nothing visible
	}

	RI.isSkyVisible = true;

	// don't fogging skybox (this fix old Half-Life bug)
	if( !RI.fogCustom )
		pglDisable( GL_FOG );
	pglDisable( GL_BLEND );
	pglDisable( GL_ALPHA_TEST );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

	if( clgame.movevars.skyangle && !VectorIsNull( (float *)&clgame.movevars.skydir_x ))
	{
		matrix4x4	m;
		Matrix4x4_CreateRotate( m, clgame.movevars.skyangle, clgame.movevars.skydir_x, clgame.movevars.skydir_y, clgame.movevars.skydir_z );
		Matrix4x4_ConcatTranslate( m, -RI.vieworg[0], -RI.vieworg[1], -RI.vieworg[2] );
		Matrix4x4_ConcatTransforms( RI.modelviewMatrix, RI.worldviewMatrix, m );
		GL_LoadMatrix( RI.modelviewMatrix );
		tr.modelviewIdentity = false;
	}

	for( i = 0; i < 6; i++ )
	{
		if( RI.skyMins[0][i] >= RI.skyMaxs[0][i] || RI.skyMins[1][i] >= RI.skyMaxs[1][i] )
			continue;

		GL_Bind( XASH_TEXTURE0, tr.skyboxTextures[r_skyTexOrder[i]] );

		pglBegin( GL_QUADS );
		MakeSkyVec( RI.skyMins[0][i], RI.skyMins[1][i], i );
		MakeSkyVec( RI.skyMins[0][i], RI.skyMaxs[1][i], i );
		MakeSkyVec( RI.skyMaxs[0][i], RI.skyMaxs[1][i], i );
		MakeSkyVec( RI.skyMaxs[0][i], RI.skyMins[1][i], i );
		pglEnd();
	}

	R_LoadIdentity();
}

/*
===============
R_SetupSky
===============
*/
void R_SetupSky( const char *skyboxname )
{
	string	loadname;
	string	sidename;
	int	i;

	if( !skyboxname || !*skyboxname )
	{
		R_UnloadSkybox();
		return; // clear old skybox
	}

	Q_snprintf( loadname, sizeof( loadname ), "gfx/env/%s", skyboxname );
	FS_StripExtension( loadname );

	if( loadname[Q_strlen( loadname ) - 1] == '_' )
		loadname[Q_strlen( loadname ) - 1] = '\0';

	// to prevent infinite recursion if default skybox was missed
	if( !CheckSkybox( loadname ) && Q_stricmp( loadname, "gfx/env/desert" ))
	{
		MsgDev( D_ERROR, "R_SetupSky: missed or incomplete skybox '%s'\n", skyboxname );
		R_SetupSky( "desert" ); // force to default
		return; 
	}

	// release old skybox
	R_UnloadSkybox();

	for( i = 0; i < 6; i++ )
	{
		Q_snprintf( sidename, sizeof( sidename ), "%s%s", loadname, r_skyBoxSuffix[i] );
		tr.skyboxTextures[i] = GL_LoadTexture( sidename, NULL, 0, TF_CLAMP|TF_SKY, NULL );
		GL_SetTextureType( tr.skyboxTextures[i], TEX_CUBEMAP );
		if( !tr.skyboxTextures[i] ) break;
	}

	if( i == 6 ) return; // loaded

	// clear previous and try again
	R_UnloadSkybox();

	for( i = 0; i < 6; i++ )
	{
		Q_snprintf( sidename, sizeof( sidename ), "%s_%s", loadname, r_skyBoxSuffix[i] );
		tr.skyboxTextures[i] = GL_LoadTexture( sidename, NULL, 0, TF_CLAMP|TF_SKY, NULL );
		GL_SetTextureType( tr.skyboxTextures[i], TEX_CUBEMAP );
		if( !tr.skyboxTextures[i] ) break;
	}
	if( i == 6 ) return; // loaded

	// completely couldn't load skybox (probably never happens)
	MsgDev( D_ERROR, "R_SetupSky: couldn't load skybox '%s'\n", skyboxname );
	R_UnloadSkybox();
}

/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky( mip_t *mt, byte *buf, texture_t *tx )
{
	rgbdata_t	r_temp, *r_sky;
	uint	*trans, *rgba;
	uint	transpix;
	int	r, g, b;
	int	i, j, p;
	char	texname[32];

	Q_snprintf( texname, sizeof( texname ), "%s%s.mip", ( mt->offsets[0] > 0 ) ? "#" : "", tx->name );

	if( mt->offsets[0] > 0 )
	{
		// NOTE: imagelib detect miptex version by size
		// 770 additional bytes is indicated custom palette
		int size = (int)sizeof( mip_t ) + ((mt->width * mt->height * 85)>>6);
		if( world.version >= HLBSP_VERSION ) size += sizeof( short ) + 768;

		r_sky = FS_LoadImage( texname, buf, size );
	}
	else
	{
		// okay, loading it from wad
		r_sky = FS_LoadImage( texname, NULL, 0 );
	}

	// make sure what sky image is valid
	if( !r_sky || !r_sky->palette || r_sky->type != PF_INDEXED_32 || r_sky->height == 0 )
	{
		MsgDev( D_ERROR, "R_InitSky: unable to load sky texture %s\n", tx->name );
		FS_FreeImage( r_sky );
		return;
	}

	// make an average value for the back to avoid
	// a fringe on the top level
	trans = Mem_Alloc( r_temppool, r_sky->height * r_sky->height * sizeof( *trans ));

	r = g = b = 0;
	for( i = 0; i < r_sky->width >> 1; i++ )
	{
		for( j = 0; j < r_sky->height; j++ )
		{
			p = r_sky->buffer[i * r_sky->width + j + r_sky->height];
			rgba = (uint *)r_sky->palette + p;
			trans[(i * r_sky->height) + j] = *rgba;
			r += ((byte *)rgba)[0];
			g += ((byte *)rgba)[1];
			b += ((byte *)rgba)[2];
		}
	}

	((byte *)&transpix)[0] = r / ( r_sky->height * r_sky->height );
	((byte *)&transpix)[1] = g / ( r_sky->height * r_sky->height );
	((byte *)&transpix)[2] = b / ( r_sky->height * r_sky->height );
	((byte *)&transpix)[3] = 0;

	// build a temporary image
	r_temp = *r_sky;
	r_temp.width = r_sky->width >> 1;
	r_temp.height = r_sky->height;
	r_temp.type = PF_RGBA_32;
	r_temp.flags = IMAGE_HAS_COLOR;
	r_temp.palette = NULL;
	r_temp.buffer = (byte *)trans;
	r_temp.size = r_temp.width * r_temp.height * 4;

	// load it in
	tr.solidskyTexture = GL_LoadTextureInternal( "solid_sky", &r_temp, TF_UNCOMPRESSED|TF_NOMIPMAP, false );

	for( i = 0; i < r_sky->width >> 1; i++ )
	{
		for( j = 0; j < r_sky->height; j++ )
		{
			p = r_sky->buffer[i * r_sky->width + j];
			if( p == 0 )
			{
				trans[(i * r_sky->height) + j] = transpix;
			}
			else
			{         
				rgba = (uint *)r_sky->palette + p;
				trans[(i * r_sky->height) + j] = *rgba;
			}
		}
	}

	r_temp.flags = IMAGE_HAS_COLOR|IMAGE_HAS_ALPHA;

	// load it in
	tr.alphaskyTexture = GL_LoadTextureInternal( "alpha_sky", &r_temp, TF_UNCOMPRESSED|TF_NOMIPMAP, false );

	GL_SetTextureType( tr.solidskyTexture, TEX_BRUSH );
	GL_SetTextureType( tr.alphaskyTexture, TEX_BRUSH );

	// clean up
	FS_FreeImage( r_sky );
	Mem_Free( trans );
}

/*
=============
EmitWaterPolys

Does a water warp on the pre-fragmented glpoly_t chain
=============
*/
void EmitWaterPolys( glpoly_t *polys, qboolean noCull )
{
	glpoly_t	*p;
	float	*v, nv, waveHeight;
	float	s, t, os, ot;
	int	i;

	if( noCull ) pglDisable( GL_CULL_FACE );

	// set the current waveheight
	waveHeight = RI.currentWaveHeight / 32.0f;

	// reset fog color for nonlightmapped water
	GL_ResetFogColor();

	for( p = polys; p; p = p->next )
	{
		pglBegin( GL_POLYGON );

		for( i = 0, v = p->verts[0]; i < p->numverts; i++, v += VERTEXSIZE )
		{
			if( waveHeight )
			{
				nv = r_turbsin[(int)(cl.time * 160.0f + v[1] + v[0]) & 255] + 8.0f;
				nv = (r_turbsin[(int)(v[0] * 5.0f + cl.time * 171.0f - v[1]) & 255] + 8.0f ) * 0.8f + nv;
				nv = nv * waveHeight + v[2];
			}
			else nv = v[2];

			os = v[3];
			ot = v[4];

			s = os + r_turbsin[(int)((ot * 0.125f + cl.time ) * TURBSCALE) & 255];
			s *= ( 1.0f / SUBDIVIDE_SIZE );

			t = ot + r_turbsin[(int)((os * 0.125f + cl.time ) * TURBSCALE) & 255];
			t *= ( 1.0f / SUBDIVIDE_SIZE );

			if( glState.activeTMU != 0 )
				GL_MultiTexCoord2f( glState.activeTMU, s, t );
			else pglTexCoord2f( s, t );
			pglVertex3f( v[0], v[1], nv );
		}
		pglEnd();
	}

	// restore culling
	if( noCull ) pglEnable( GL_CULL_FACE );

	GL_SetupFogColorForSurfaces();
}

/*
=============
EmitSkyPolys
=============
*/
void EmitSkyPolys( msurface_t *fa )
{
	glpoly_t	*p;
	float	*v;
	int	i;
	float	s, t;
	vec3_t	dir;
	float	length;

	for( p = fa->polys; p; p = p->next )
	{
		pglBegin( GL_POLYGON );

		for( i = 0, v = p->verts[0]; i < p->numverts; i++, v += VERTEXSIZE )
		{
			VectorSubtract( v, RI.vieworg, dir );
			dir[2] *= 3.0f; // flatten the sphere

			length = VectorLength( dir );
			length = 6.0f * 63.0f / length;

			dir[0] *= length;
			dir[1] *= length;

			s = ( speedscale + dir[0] ) * (1.0f / 128.0f);
			t = ( speedscale + dir[1] ) * (1.0f / 128.0f);

			pglTexCoord2f( s, t );
			pglVertex3fv( v );
		}
		pglEnd ();
	}
}

/*
=================
R_DrawSkyChain
=================
*/
void R_DrawSkyChain( msurface_t *s )
{
	msurface_t	*fa;

	GL_SetRenderMode( kRenderNormal );
	GL_Bind( XASH_TEXTURE0, tr.solidskyTexture );

	speedscale = cl.time * 8.0f;
	speedscale -= (int)speedscale & ~127;

	for( fa = s; fa; fa = fa->texturechain )
		EmitSkyPolys( fa );

	GL_SetRenderMode( kRenderTransTexture );
	GL_Bind( XASH_TEXTURE0, tr.alphaskyTexture );

	speedscale = cl.time * 16.0f;
	speedscale -= (int)speedscale & ~127;

	for( fa = s; fa; fa = fa->texturechain )
		EmitSkyPolys( fa );

	pglDisable( GL_BLEND );
}

/*
===============
EmitBothSkyLayers

Does a sky warp on the pre-fragmented glpoly_t chain
This will be called for brushmodels, the world
will have them chained together.
===============
*/
void EmitSkyLayers( msurface_t *fa )
{
	GL_SetRenderMode( kRenderNormal );
	GL_Bind( XASH_TEXTURE0, tr.solidskyTexture );

	speedscale = cl.time * 8.0f;
	speedscale -= (int)speedscale & ~127;

	EmitSkyPolys( fa );

	GL_SetRenderMode( kRenderTransTexture );
	GL_Bind( XASH_TEXTURE0, tr.alphaskyTexture );

	speedscale = cl.time * 16.0f;
	speedscale -= (int)speedscale & ~127;

	EmitSkyPolys( fa );

	pglDisable( GL_BLEND );
}
#endif // XASH_DEDICATED
