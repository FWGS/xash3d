/*
gl_rsurf.c - surface-related refresh code
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
#include "mod_local.h"
#include "mathlib.h"
			
typedef struct
{
	int		allocated[BLOCK_SIZE_MAX];
	int		current_lightmap_texture;
	msurface_t	*dynamic_surfaces;
	msurface_t	*lightmap_surfaces[MAX_LIGHTMAPS];
	byte		lightmap_buffer[BLOCK_SIZE_MAX*BLOCK_SIZE_MAX*4];
	byte		deluxemap_buffer[BLOCK_SIZE_MAX*BLOCK_SIZE_MAX*4];
} gllightmapstate_t;

static int		nColinElim; // stats
static vec2_t		world_orthocenter;
static vec2_t		world_orthohalf;
static byte		visbytes[MAX_MAP_LEAFS/8];
static uint		r_blocklights[BLOCK_SIZE_MAX*BLOCK_SIZE_MAX*3];
static int		r_blockdeluxe[BLOCK_SIZE_MAX*BLOCK_SIZE_MAX*3];
static glpoly_t		*fullbright_polys[MAX_TEXTURES];
static qboolean		draw_fullbrights = false;
static mextrasurf_t		*detail_surfaces[MAX_TEXTURES];
static qboolean		draw_details = false;
static msurface_t		*skychain = NULL;
static gllightmapstate_t	gl_lms;

static void LM_UploadBlock( qboolean dynamic );

static qboolean R_AddSurfToVBO( msurface_t *surf, qboolean buildlightmaps );
static void R_DrawVBO( qboolean drawlightmaps, qboolean drawtextures );

byte *Mod_GetCurrentVis( void )
{
	return Mod_LeafPVS( r_viewleaf, cl.worldmodel );
}

void Mod_SetOrthoBounds( float *mins, float *maxs )
{
	if( clgame.drawFuncs.GL_OrthoBounds )
	{
		clgame.drawFuncs.GL_OrthoBounds( mins, maxs );
	}

	Vector2Average( maxs, mins, world_orthocenter );
	Vector2Subtract( maxs, world_orthocenter, world_orthohalf );
}

static void BoundPoly( int numverts, float *verts, vec3_t mins, vec3_t maxs )
{
	int	i, j;
	float	*v;

	mins[0] = mins[1] = mins[2] = 99999.0f;
	maxs[0] = maxs[1] = maxs[2] = -99999.0f;

	for( i = 0, v = verts; i < numverts; i++ )
	{
		for( j = 0; j < 3; j++, v++ )
		{
			if( *v < mins[j] ) mins[j] = *v;
			if( *v > maxs[j] ) maxs[j] = *v;
		}
	}
}

static void SubdividePolygon_r( msurface_t *warpface, int numverts, float *verts )
{
	int	i, j, k, f, b;
	vec3_t	mins, maxs;
	float	m, frac, s, t, *v, vertsDiv, *verts_p;
	vec3_t	front[SUBDIVIDE_SIZE], back[SUBDIVIDE_SIZE], total;
	float	dist[SUBDIVIDE_SIZE], total_s, total_t, total_ls, total_lt;
	glpoly_t	*poly;

	Q_memset( dist, 0, SUBDIVIDE_SIZE * sizeof( float ) );

	if( numverts > ( SUBDIVIDE_SIZE - 4 ))
		Host_Error( "Mod_SubdividePolygon: too many vertexes on face ( %i )\n", numverts );

	BoundPoly( numverts, verts, mins, maxs );

	for( i = 0; i < 3; i++ )
	{
		m = ( mins[i] + maxs[i] ) * 0.5f;
		m = SUBDIVIDE_SIZE * floor( m / SUBDIVIDE_SIZE + 0.5f );
		if( maxs[i] - m < 8 ) continue;
		if( m - mins[i] < 8 ) continue;

		// cut it
		v = verts + i;
		for( j = 0; j < numverts; j++, v += 3 )
			dist[j] = *v - m;

		// wrap cases
		dist[j] = dist[0];
		v -= i;
		VectorCopy( verts, v );

		f = b = 0;
		v = verts;
		for( j = 0; j < numverts; j++, v += 3 )
		{
			if( dist[j] >= 0 )
			{
				VectorCopy( v, front[f] );
				f++;
			}

			if( dist[j] <= 0 )
			{
				VectorCopy (v, back[b]);
				b++;
			}

			if( dist[j] == 0 || dist[j+1] == 0 )
				continue;

			if(( dist[j] > 0 ) != ( dist[j+1] > 0 ))
			{
				// clip point
				frac = dist[j] / ( dist[j] - dist[j+1] );
				for( k = 0; k < 3; k++ )
					front[f][k] = back[b][k] = v[k] + frac * (v[3+k] - v[k]);
				f++;
				b++;
			}
		}

		SubdividePolygon_r( warpface, f, front[0] );
		SubdividePolygon_r( warpface, b, back[0] );
		return;
	}

	// add a point in the center to help keep warp valid
	poly = Mem_Alloc( loadmodel->mempool, sizeof( glpoly_t ) + ( ( numverts - 4 ) + 2) * VERTEXSIZE * sizeof( float ));
	poly->next = warpface->polys;
	poly->flags = warpface->flags;
	warpface->polys = poly;
	poly->numverts = numverts + 2;
	verts_p = (float *)poly + ( ( sizeof( void* ) + sizeof( int ) ) >> 1 ); // pointer glpoly_t->verts
	VectorClear( total );
	total_s = total_ls = 0.0f;
	total_t = total_lt = 0.0f;

	for( i = 0; i < numverts; i++, verts += 3 )
	{
		VectorCopy( verts, &verts_p[VERTEXSIZE * ( i + 1 )] );
		VectorAdd( total, verts, total );

		if( warpface->flags & SURF_DRAWTURB )
		{
			s = DotProduct( verts, warpface->texinfo->vecs[0] );
			t = DotProduct( verts, warpface->texinfo->vecs[1] );
		}
		else
		{
			s = DotProduct( verts, warpface->texinfo->vecs[0] ) + warpface->texinfo->vecs[0][3];
			t = DotProduct( verts, warpface->texinfo->vecs[1] ) + warpface->texinfo->vecs[1][3];
			s /= warpface->texinfo->texture->width; 
			t /= warpface->texinfo->texture->height; 
		}

		verts_p[VERTEXSIZE * ( i + 1 ) + 3] = s;
		verts_p[VERTEXSIZE * ( i + 1 ) + 4] = t;
 
		total_s += s;
		total_t += t;

		// for speed reasons
		if( !( warpface->flags & SURF_DRAWTURB ))
		{
			// lightmap texture coordinates
			s = DotProduct( verts, warpface->texinfo->vecs[0] ) + warpface->texinfo->vecs[0][3];
			s -= warpface->texturemins[0];
			s += warpface->light_s * LM_SAMPLE_SIZE;
			s += LM_SAMPLE_SIZE >> 1;
			s /= BLOCK_SIZE * LM_SAMPLE_SIZE; //fa->texinfo->texture->width;

			t = DotProduct( verts, warpface->texinfo->vecs[1] ) + warpface->texinfo->vecs[1][3];
			t -= warpface->texturemins[1];
			t += warpface->light_t * LM_SAMPLE_SIZE;
			t += LM_SAMPLE_SIZE >> 1;
			t /= BLOCK_SIZE * LM_SAMPLE_SIZE; //fa->texinfo->texture->height;

			verts_p[VERTEXSIZE * ( i + 1 ) + 5] = s;
			verts_p[VERTEXSIZE * ( i + 1 ) + 6] = t;

			total_ls += s;
			total_lt += t;
		}
	}

	vertsDiv = ( 1.0f / (float)numverts );

	VectorScale( total, vertsDiv, &verts_p[0] );
	verts_p[3] = total_s * vertsDiv;
	verts_p[4] = total_t * vertsDiv;

	if( !( warpface->flags & SURF_DRAWTURB ))
	{
		verts_p[5] = total_ls * vertsDiv;
		verts_p[6] = total_lt * vertsDiv;
	}

	// copy first vertex to last
	Q_memcpy(&verts_p[VERTEXSIZE * ( i + 1 )], &verts_p[VERTEXSIZE], VERTEXSIZE * sizeof( float ) );
}

void GL_SetupFogColorForSurfaces( void )
{
	vec3_t	fogColor;
	float	factor, div;

	Q_memset(fogColor, 0, 3 * sizeof(float)); //Result pow( RI.fogColor[i] / div, ( 1.0f / factor )) may be nan.

	if(( !RI.fogEnabled && !RI.fogCustom ) || RI.refdef.onlyClientDraw || !RI.currententity )
		return;

	if( RI.currententity->curstate.rendermode == kRenderTransTexture )
	{
		pglFogfv( GL_FOG_COLOR, RI.fogColor );
		return;
	}

	div = (r_detailtextures->integer) ? 2.0f : 1.0f;
	factor = (r_detailtextures->integer) ? 3.0f : 2.0f;
	fogColor[0] = pow( RI.fogColor[0] / div, ( 1.0f / factor ));
	fogColor[1] = pow( RI.fogColor[1] / div, ( 1.0f / factor ));
	fogColor[2] = pow( RI.fogColor[2] / div, ( 1.0f / factor ));
	pglFogfv( GL_FOG_COLOR, fogColor );
}

void GL_ResetFogColor( void )
{
	// restore fog here
	if(( RI.fogEnabled || RI.fogCustom ) && !RI.refdef.onlyClientDraw )
		pglFogfv( GL_FOG_COLOR, RI.fogColor );
}

/*
================
GL_SubdivideSurface

Breaks a polygon up along axial 64 unit
boundaries so that turbulent and sky warps
can be done reasonably.
================
*/
void GL_SubdivideSurface( msurface_t *fa )
{
	vec3_t	verts[SUBDIVIDE_SIZE];
	int	numverts;
	int	i, lindex;
	float	*vec;

	// convert edges back to a normal polygon
	numverts = 0;
	for( i = 0; i < fa->numedges; i++ )
	{
		lindex = loadmodel->surfedges[fa->firstedge + i];

		if( lindex > 0 ) vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
		else vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;
		VectorCopy( vec, verts[numverts] );
		numverts++;
	}

	// do subdivide
	SubdividePolygon_r( fa, numverts, verts[0] );
}

/*
================
GL_BuildPolygonFromSurface
================
*/
void GL_BuildPolygonFromSurface( model_t *mod, msurface_t *fa )
{
	int		i, lindex, lnumverts;
	medge_t		*pedges, *r_pedge;
	int		vertpage;
	texture_t		*tex;
	gltexture_t	*glt;
	float		*vec, *verts_p;
	float		s, t;
	glpoly_t		*poly;

	// already created
	if( !mod || fa->polys ) return;

	if( !fa->texinfo || !fa->texinfo->texture )
		return; // bad polygon ?

	if( fa->flags & SURF_CONVEYOR && fa->texinfo->texture->gl_texturenum != 0 )
	{
		glt = R_GetTexture( fa->texinfo->texture->gl_texturenum );
		tex = fa->texinfo->texture;
		ASSERT( glt != NULL && tex != NULL );

		// update conveyor widths for keep properly speed of scrolling
		glt->srcWidth = tex->width;
		glt->srcHeight = tex->height;
	}

	// reconstruct the polygon
	pedges = mod->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	// draw texture
	poly = Mem_Alloc( mod->mempool, sizeof( glpoly_t ) + ( lnumverts - 4 ) * VERTEXSIZE * sizeof( float ) );
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;
	verts_p = (float *)poly + (( sizeof( void* ) + sizeof( int ) ) >> 1); //pointer glpoly_t->verts

	for( i = 0; i < lnumverts; i++ )
	{
		lindex = mod->surfedges[fa->firstedge + i];

		if( lindex > 0 )
		{
			r_pedge = &pedges[lindex];
			vec = mod->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = mod->vertexes[r_pedge->v[1]].position;
		}

		s = DotProduct( vec, fa->texinfo->vecs[0] ) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		t = DotProduct( vec, fa->texinfo->vecs[1] ) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		VectorCopy( vec, &verts_p[VERTEXSIZE * i] );
		verts_p[VERTEXSIZE * i + 3] = s;
		verts_p[VERTEXSIZE * i + 4] = t;

		// lightmap texture coordinates
		s = DotProduct( vec, fa->texinfo->vecs[0] ) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * LM_SAMPLE_SIZE;
		s += LM_SAMPLE_SIZE >> 1;
		s /= BLOCK_SIZE * LM_SAMPLE_SIZE; //fa->texinfo->texture->width;

		t = DotProduct( vec, fa->texinfo->vecs[1] ) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * LM_SAMPLE_SIZE;
		t += LM_SAMPLE_SIZE >> 1;
		t /= BLOCK_SIZE * LM_SAMPLE_SIZE; //fa->texinfo->texture->height;

		verts_p[VERTEXSIZE * i + 5] = s;
		verts_p[VERTEXSIZE * i + 6] = t;
	}

	// remove co-linear points - Ed
	if( !gl_keeptjunctions->integer && !( fa->flags & SURF_UNDERWATER ))
	{
		for( i = 0; i < lnumverts; i++ )
		{
			vec3_t	v1, v2;
			float	*prev, *this, *next;

			prev = &verts_p[VERTEXSIZE * ((i + lnumverts - 1) % lnumverts)];
			next = &verts_p[VERTEXSIZE * ((i + 1) % lnumverts)];
			this = &verts_p[VERTEXSIZE * i];

			VectorSubtract( this, prev, v1 );
			VectorNormalize( v1 );
			VectorSubtract( next, prev, v2 );
			VectorNormalize( v2 );

			// skip co-linear points
			if(( fabs( v1[0] - v2[0] ) <= 0.001f) && (fabs( v1[1] - v2[1] ) <= 0.001f) && (fabs( v1[2] - v2[2] ) <= 0.001f))
			{
				int	j, k;

				for( j = i + 1; j < lnumverts; j++ )
				{
					for( k = 0; k < VERTEXSIZE; k++ )
					verts_p[VERTEXSIZE * ( j - 1 ) + k] = verts_p[VERTEXSIZE * j + k];
				}

				// retry next vertex next time, which is now current vertex
				lnumverts--;
				nColinElim++;
				i--;
			}
		}
	}

	poly->numverts = lnumverts;
}

_inline texture_t *R_TextureRandomTiling( texture_t *base, int surfacenum )
{
	int reletive;
	int count = 0;

	if( base->anim_total >= 0 )
		return base;

	reletive = abs( surfacenum ) % abs( base->anim_total );

	while( base->anim_min > reletive || base->anim_max <= reletive )
	{
		base = base->anim_next;
		if( !base ) Host_Error( "R_TextureRandomTiling: broken loop\n" );
		if( ++count > 100 ) Host_Error( "R_TextureRandomTiling: infinite loop\n" );
	}
	return base;
}

_inline texture_t *R_TextureAnim( texture_t *base )
{
	int	reletive;
	int	count, speed;

	if( base->anim_total < 0 )
		return base;
	if( RI.currententity->curstate.frame )
	{
		if( base->alternate_anims )
			base = base->alternate_anims;
	}

	if( !base->anim_total )
		return base;

	// GoldSrc and Quake1 has different animating speed
	if( world.sky_sphere || world.version == Q1BSP_VERSION )
		speed = 10;
	else speed = 20;

	reletive = (int)(cl.time * speed) % base->anim_total;
	count = 0;

	while( base->anim_min > reletive || base->anim_max <= reletive )
	{
		base = base->anim_next;
		if( !base ) Host_Error( "R_TextureAnimation: broken loop\n" );
		if( ++count > 100 ) Host_Error( "R_TextureAnimation: infinite loop\n" );
	}
	return base;
}

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t *R_TextureAnimation( texture_t *base, int surfacenum )
{
	// random tiling textures
	if( base->anim_total < 0 )
		return R_TextureRandomTiling( base, surfacenum );

	return R_TextureAnim( base );
}

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights( msurface_t *surf )
{
	float		dist, rad, minlight;
	int		lnum, s, t, sd, td, smax, tmax;
	float		sl, tl, sacc, tacc;
	vec3_t		impact, origin_l;
	mtexinfo_t	*tex;
	dlight_t		*dl;
	uint		*bl;

	// no dlighted surfaces here
	if( !R_CountSurfaceDlights( surf )) return;

	smax = (surf->extents[0] / LM_SAMPLE_SIZE) + 1;
	tmax = (surf->extents[1] / LM_SAMPLE_SIZE) + 1;
	tex = surf->texinfo;

	for( lnum = 0; lnum < MAX_DLIGHTS; lnum++ )
	{
		if(!( surf->dlightbits & BIT( lnum )))
			continue;	// not lit by this light

		dl = &cl_dlights[lnum];

		// transform light origin to local bmodel space
		if( !tr.modelviewIdentity )
			Matrix4x4_VectorITransform( RI.objectMatrix, dl->origin, origin_l );
		else VectorCopy( dl->origin, origin_l );

		rad = dl->radius;
		dist = PlaneDiff( origin_l, surf->plane );
		rad -= fabs( dist );

		// rad is now the highest intensity on the plane
		minlight = dl->minlight;
		if( rad < minlight )
			continue;

		minlight = rad - minlight;

		if( surf->plane->type < 3 )
		{
			VectorCopy( origin_l, impact );
			impact[surf->plane->type] -= dist;
		}
		else VectorMA( origin_l, -dist, surf->plane->normal, impact );

		sl = DotProduct( impact, tex->vecs[0] ) + tex->vecs[0][3] - surf->texturemins[0];
		tl = DotProduct( impact, tex->vecs[1] ) + tex->vecs[1][3] - surf->texturemins[1];

		bl = r_blocklights;
		for( t = 0, tacc = 0; t < tmax; t++, tacc += LM_SAMPLE_SIZE )
		{
			td = tl - tacc;
			if( td < 0 ) td = -td;

			for( s = 0, sacc = 0; s < smax; s++, sacc += LM_SAMPLE_SIZE, bl += 3 )
			{
				sd = sl - sacc;
				if( sd < 0 ) sd = -sd;

				if( sd > td ) dist = sd + (td >> 1);
				else dist = td + (sd >> 1);

				if( dist < minlight )
				{
					bl[0] += ( rad - dist ) * TextureToTexGamma( dl->color.r );
					bl[1] += ( rad - dist ) * TextureToTexGamma( dl->color.g );
					bl[2] += ( rad - dist ) * TextureToTexGamma( dl->color.b );
				}
			}
		}
	}
}

/*
================
R_SetCacheState
================
*/
void R_SetCacheState( msurface_t *surf )
{
	int	maps;

	for( maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++ )
	{
		surf->cached_light[maps] = RI.lightstylevalue[surf->styles[maps]];
	}
}

/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/
static void LM_InitBlock( void )
{
	Q_memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ));
}

static int LM_AllocBlock( int w, int h, int *x, int *y )
{
	int	i, j;
	int	best, best2;

	best = BLOCK_SIZE;

	for( i = 0; i < BLOCK_SIZE - w; i++ )
	{
		best2 = 0;

		for( j = 0; j < w; j++ )
		{
			if( gl_lms.allocated[i+j] >= best )
				break;
			if( gl_lms.allocated[i+j] > best2 )
				best2 = gl_lms.allocated[i+j];
		}

		if( j == w )
		{	
			// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if( best + h > BLOCK_SIZE )
		return false;

	for( i = 0; i < w; i++ )
		gl_lms.allocated[*x + i] = best + h;

	return true;
}

static void LM_UploadDynamicBlock()
{
	int	height = 0, i;

	for( i = 0; i < BLOCK_SIZE; i++ )
	{
		if( gl_lms.allocated[i] > height )
			height = gl_lms.allocated[i];
	}

	pglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, BLOCK_SIZE, height, GL_RGBA, GL_UNSIGNED_BYTE, gl_lms.lightmap_buffer );
}

static void LM_UploadBlock( qboolean dynamic )
{
	int	i;

	if( dynamic )
	{
		int	height = 0;

		for( i = 0; i < BLOCK_SIZE; i++ )
		{
			if( gl_lms.allocated[i] > height )
				height = gl_lms.allocated[i];
		}

		if( host.features & ENGINE_LARGE_LIGHTMAPS )
			GL_Bind( XASH_TEXTURE0, tr.dlightTexture2 );
		else GL_Bind( XASH_TEXTURE0, tr.dlightTexture );

		pglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, BLOCK_SIZE, height, GL_RGBA, GL_UNSIGNED_BYTE, gl_lms.lightmap_buffer );
	}
	else
	{
		rgbdata_t	r_lightmap = {0};
		char	lmName[16];

		i = gl_lms.current_lightmap_texture;

		// upload static lightmaps only during loading
		Q_snprintf( lmName, sizeof( lmName ), "*lightmap%i", i );

		r_lightmap.width = BLOCK_SIZE;
		r_lightmap.height = BLOCK_SIZE;
		r_lightmap.type = PF_RGBA_32;
		r_lightmap.size = r_lightmap.width * r_lightmap.height * 4;
		r_lightmap.flags = ( world.version == Q1BSP_VERSION ) ? 0 : IMAGE_HAS_COLOR;
		r_lightmap.buffer = gl_lms.lightmap_buffer;
		tr.lightmapTextures[i] = GL_LoadTextureInternal( lmName, &r_lightmap, TF_FONT, false );
		GL_SetTextureType( tr.lightmapTextures[i], TEX_LIGHTMAP );

		if( tr.deluxemap )
		{
			rgbdata_t	r_deluxemap = {0};
			r_deluxemap.width = BLOCK_SIZE;
			r_deluxemap.height = BLOCK_SIZE;
			r_deluxemap.type = PF_RGBA_32;
			r_deluxemap.size = r_deluxemap.width * r_deluxemap.height * 4;
			r_deluxemap.flags = IMAGE_HAS_COLOR;
			r_deluxemap.buffer = gl_lms.deluxemap_buffer;
			Q_snprintf( lmName, sizeof( lmName ), "*deluxemap%i", i );
			tr.deluxemapTextures[i] = GL_LoadTextureInternal( lmName, &r_deluxemap, TF_FONT, false );
		}

		if( ++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS )
			Host_Error( "AllocBlock: full\n" );
	}
}

/*
=================
R_BuildLightmap

Combine and scale multiple lightmaps into the floating
format in r_blocklights
=================
*/
static void R_BuildLightMap( msurface_t *surf, byte *dest, int stride, qboolean dynamic )
{
	int	smax, tmax;
	uint	*bl, scale;
	int	i, map, size, s, t;
	color24	*lm;

	smax = ( surf->extents[0] / LM_SAMPLE_SIZE ) + 1;
	tmax = ( surf->extents[1] / LM_SAMPLE_SIZE ) + 1;
	size = smax * tmax;

	lm = surf->samples;

	Q_memset( r_blocklights, 0, sizeof( uint ) * size * 3 );

	// add all the lightmaps
	for( map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255 && lm; map++ )
	{
		scale = RI.lightstylevalue[surf->styles[map]];

		for( i = 0, bl = r_blocklights; i < size; i++, bl += 3, lm++ )
		{
			bl[0] += TextureToTexGamma( lm->r ) * scale;
			bl[1] += TextureToTexGamma( lm->g ) * scale;
			bl[2] += TextureToTexGamma( lm->b ) * scale;
		}
	}

	// add all the dynamic lights
	if( surf->dlightframe == tr.framecount && dynamic )
		R_AddDynamicLights( surf );

	// Put into texture format
	stride -= (smax << 2);
	bl = r_blocklights;

	for( t = 0; t < tmax; t++, dest += stride )
	{
		for( s = 0; s < smax; s++ )
		{
			dest[0] = min((bl[0] >> 7), 255 );
			dest[1] = min((bl[1] >> 7), 255 );
			dest[2] = min((bl[2] >> 7), 255 );
			dest[3] = 255;

			bl += 3;
			dest += 4;
		}
	}
}

static void R_BuildDeluxeMap( msurface_t *surf, byte *dest, int stride )
{
	int	smax, tmax, *bl;
	unsigned int scale;
	int	i, map, size, s, t;
	color24	*lm, *dm;

	smax = ( surf->extents[0] / LM_SAMPLE_SIZE ) + 1;
	tmax = ( surf->extents[1] / LM_SAMPLE_SIZE ) + 1;
	size = smax * tmax;

	lm = surf->samples;
	dm = tr.deluxemap + ( surf->samples - cl.worldmodel->lightdata );

	Q_memset( r_blockdeluxe, 0, sizeof( int ) * size * 3 );

	// add all the lightmaps
	for( map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255 && lm; map++ )
	{
		scale = RI.lightstylevalue[surf->styles[map]];

		for( i = 0, bl = r_blockdeluxe; i < size; i++, bl += 3, lm++, dm++ )
		{
			int l;
			vec3_t v = { lm->r, lm->g, lm->b };

			l = (int)(VectorLength(v) * scale);
			bl[0] += ((int)dm->r - 128) * l;
			bl[1] += ((int)dm->g - 128) * l;
			bl[2] += ((int)dm->b - 128) * l;
		}
	}

	// Put into texture format
	stride -= (smax << 2);
	bl = r_blockdeluxe;

	for( t = 0; t < tmax; t++, dest += stride )
	{
		for( s = 0; s < smax; s++ )
		{
			int l;
			vec3_t n; // = *(Vector*)bl;
			VectorCopy(bl, n);

			VectorNormalize(n);

			l = (int)(n[0] * 128 + 128); dest[0] = bound(0, l, 255);
			l = (int)(n[1] * 128 + 128); dest[1] = bound(0, l, 255);
			l = (int)(n[2] * 128 + 128); dest[2] = bound(0, l, 255);
			dest[3] = 255;

			bl += 3;
			dest += 4;
		}
	}
}

/*
================
DrawGLPoly
================
*/
void DrawGLPoly( glpoly_t *p, float xScale, float yScale )
{
	float		*v;
	float		sOffset, sy;
	float		tOffset, cy;
	cl_entity_t	*e = RI.currententity;
	int		i, hasScale = false;
	
	if( !p )
		return;

	// special hack for non-lightmapped surfaces
	if( p->flags & SURF_DRAWTILED )
		GL_ResetFogColor();

	if( p->flags & SURF_CONVEYOR )
	{
		gltexture_t	*texture;
		float		flConveyorSpeed;
		float		flRate, flAngle;

		flConveyorSpeed = (e->curstate.rendercolor.g<<8|e->curstate.rendercolor.b) / 16.0f;
		if( e->curstate.rendercolor.r ) flConveyorSpeed = -flConveyorSpeed;
		texture = R_GetTexture( glState.currentTextures[glState.activeTMU] );

		flRate = fabs( flConveyorSpeed ) / (float)texture->srcWidth;
		flAngle = ( flConveyorSpeed >= 0 ) ? 180 : 0;

		SinCos( flAngle * ( M_PI / 180.0f ), &sy, &cy );
		sOffset = RI.refdef.time * cy * flRate;
		tOffset = RI.refdef.time * sy * flRate;
	
		// make sure that we are positive
		if( sOffset < 0.0f ) sOffset += 1.0f + -(int)sOffset;
		if( tOffset < 0.0f ) tOffset += 1.0f + -(int)tOffset;

		// make sure that we are in a [0,1] range
		sOffset = sOffset - (int)sOffset;
		tOffset = tOffset - (int)tOffset;
	}
	else
	{
		sOffset = tOffset = 0.0f;
	}

	if( xScale != 0.0f && yScale != 0.0f )
		hasScale = true;

	pglBegin( GL_POLYGON );

	for( i = 0, v = p->verts[0]; i < p->numverts; i++, v += VERTEXSIZE )
	{
		if( hasScale )
			pglTexCoord2f(( v[3] + sOffset ) * xScale, ( v[4] + tOffset ) * yScale );
		else pglTexCoord2f( v[3] + sOffset, v[4] + tOffset );

		pglVertex3fv( v );
	}

	pglEnd();

	// special hack for non-lightmapped surfaces
	if( p->flags & SURF_DRAWTILED )
		GL_SetupFogColorForSurfaces();
}

/*
================
DrawGLPolyChain

Render lightmaps
================
*/
void DrawGLPolyChain( glpoly_t *p, float soffset, float toffset )
{
	qboolean	dynamic = true;

	if( soffset == 0.0f && toffset == 0.0f )
		dynamic = false;

	for( ; p != NULL; p = p->chain )
	{
		float	*v;
		int	i;

		pglBegin( GL_POLYGON );

		v = p->verts[0];
		for( i = 0; i < p->numverts; i++, v += VERTEXSIZE )
		{
			if( !dynamic ) pglTexCoord2f( v[5], v[6] );
			else pglTexCoord2f( v[5] - soffset, v[6] - toffset );
			pglVertex3fv( v );
		}
		pglEnd ();
	}
}

/*
================
R_BlendLightmaps
================
*/
void R_BlendLightmaps( void )
{
	msurface_t	*surf, *newsurf = NULL;
	mextrasurf_t	*info;
	int		i;

	if( r_fullbright->integer || !cl.worldmodel->lightdata )
		return;

	GL_SetupFogColorForSurfaces ();

	if( RI.currententity )
	{
		// check for rendermode
		switch( RI.currententity->curstate.rendermode )
		{
		case kRenderTransTexture:
		case kRenderTransColor:
		case kRenderTransAdd:
		case kRenderGlow:
			return; // no lightmaps
		}

		if( RI.currententity->curstate.effects & EF_FULLBRIGHT )
			return;	// disabled by user
	}

	if( !r_lightmap->integer )
	{
		pglEnable( GL_BLEND );

		if( !glState.drawTrans )
		{
			// lightmapped solid surfaces
			pglDepthMask( GL_FALSE );
			pglDepthFunc( GL_EQUAL );
		}

		pglDisable( GL_ALPHA_TEST );

		if( gl_overbright->integer == 1 )
			pglColor4f( 1 / 1.5, 1 / 1.5, 1 / 1.5, 1.0 );
		if( gl_overbright->integer )
			pglBlendFunc( GL_DST_COLOR, GL_SRC_COLOR );
		else
			pglBlendFunc( GL_ZERO, GL_SRC_COLOR );

		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	}

	// render static lightmaps first
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		if( gl_lms.lightmap_surfaces[i] )
		{
			GL_Bind( XASH_TEXTURE0, tr.lightmapTextures[i] );

			for( surf = gl_lms.lightmap_surfaces[i]; surf != NULL; surf = surf->lightmapchain )
			{
				if( surf->polys ) DrawGLPolyChain( surf->polys, 0.0f, 0.0f );
			}
		}
	}

	// render dynamic lightmaps
	if( r_dynamic->integer )
	{
		LM_InitBlock();

		if( host.features & ENGINE_LARGE_LIGHTMAPS )
			GL_Bind( XASH_TEXTURE0, tr.dlightTexture2 );
		else GL_Bind( XASH_TEXTURE0, tr.dlightTexture );

		newsurf = gl_lms.dynamic_surfaces;

		for( surf = gl_lms.dynamic_surfaces; surf != NULL; surf = surf->lightmapchain )
		{
			int	smax, tmax;
			byte	*base;

			smax = ( surf->extents[0] / LM_SAMPLE_SIZE ) + 1;
			tmax = ( surf->extents[1] / LM_SAMPLE_SIZE ) + 1;
			info = SURF_INFO( surf, RI.currentmodel );

			if( LM_AllocBlock( smax, tmax, &info->dlight_s, &info->dlight_t ))
			{
				base = gl_lms.lightmap_buffer;
				base += ( info->dlight_t * BLOCK_SIZE + info->dlight_s ) * 4;

				R_BuildLightMap( surf, base, BLOCK_SIZE * 4, true );
			}
			else
			{
				msurface_t	*drawsurf;

				// upload what we have so far
				LM_UploadBlock( true );

				// draw all surfaces that use this lightmap
				for( drawsurf = newsurf; drawsurf != surf; drawsurf = drawsurf->lightmapchain )
				{
					if( drawsurf->polys )
					{
						info = SURF_INFO( drawsurf, RI.currentmodel );

						DrawGLPolyChain( drawsurf->polys,
						( drawsurf->light_s - info->dlight_s ) * ( 1.0f / (float)BLOCK_SIZE ), 
						( drawsurf->light_t - info->dlight_t ) * ( 1.0f / (float)BLOCK_SIZE ));
					}
				}

				newsurf = drawsurf;

				// clear the block
				LM_InitBlock();

				info = SURF_INFO( surf, RI.currentmodel );

				// try uploading the block now
				if( !LM_AllocBlock( smax, tmax, &info->dlight_s, &info->dlight_t ))
					Host_Error( "AllocBlock: full\n" );

				base = gl_lms.lightmap_buffer;
				base += ( info->dlight_t * BLOCK_SIZE + info->dlight_s ) * 4;

				R_BuildLightMap( surf, base, BLOCK_SIZE * 4, true );
			}
		}

		// draw remainder of dynamic lightmaps that haven't been uploaded yet
		if( newsurf ) LM_UploadBlock( true );

		for( surf = newsurf; surf != NULL; surf = surf->lightmapchain )
		{
			if( surf->polys )
			{
				info = SURF_INFO( surf, RI.currentmodel );

				DrawGLPolyChain( surf->polys,
				( surf->light_s - info->dlight_s ) * ( 1.0f / (float)BLOCK_SIZE ),
				( surf->light_t - info->dlight_t ) * ( 1.0f / (float)BLOCK_SIZE ));
			}
		}
	}

	if( !r_lightmap->integer )
	{
		pglDisable( GL_BLEND );

		if( !glState.drawTrans )
		{
			// restore depth state
			pglDepthMask( GL_TRUE );
			pglDepthFunc( GL_LEQUAL );
		}

		pglDisable( GL_ALPHA_TEST );
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	}

	// restore fog here
	GL_ResetFogColor();
}

/*
================
R_RenderFullbrights
================
*/
void R_RenderFullbrights( void )
{
	glpoly_t	*p;
	int	i;

	if( !draw_fullbrights )
		return;

	if( RI.fogEnabled && !RI.refdef.onlyClientDraw )
		pglDisable( GL_FOG );

	pglEnable( GL_BLEND );
	pglDepthMask( GL_FALSE );
	pglDisable( GL_ALPHA_TEST );
	pglBlendFunc( GL_ONE, GL_ONE );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	for( i = 1; i < MAX_TEXTURES; i++ )
	{
		if( !fullbright_polys[i] )
			continue;
		GL_Bind( XASH_TEXTURE0, i );

		for( p = fullbright_polys[i]; p; p = p->next )
		{
			if( p->flags & SURF_DRAWTURB )
				EmitWaterPolys( p, ( p->flags & SURF_NOCULL ));
			else DrawGLPoly( p, 0.0f, 0.0f );
		}

		fullbright_polys[i] = NULL;		
	}

	pglDisable( GL_BLEND );
	pglDepthMask( GL_TRUE );
	pglDisable( GL_ALPHA_TEST );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

	draw_fullbrights = false;

	// restore for here
	if( RI.fogEnabled && !RI.refdef.onlyClientDraw )
		pglEnable( GL_FOG );
}

/*
================
R_RenderDetails
================
*/
void R_RenderDetails( void )
{
	gltexture_t	*glt;
	mextrasurf_t	*es, *p;
	msurface_t	*fa;
	int		i;

	if( !draw_details )
		return;

	GL_SetupFogColorForSurfaces();

	pglEnable( GL_BLEND );
	pglBlendFunc( GL_DST_COLOR, GL_SRC_COLOR );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );

	if( RI.currententity->curstate.rendermode == kRenderTransAlpha )
		pglDepthFunc( GL_EQUAL );

	for( i = 1; i < MAX_TEXTURES; i++ )
	{
		es = detail_surfaces[i];
		if( !es ) continue;

		GL_Bind( XASH_TEXTURE0, i );

		for( p = es; p; p = p->detailchain )
		{
			fa = INFO_SURF( p, RI.currentmodel );
			glt = R_GetTexture( fa->texinfo->texture->gl_texturenum ); // get texture scale
			DrawGLPoly( fa->polys, glt->xscale, glt->yscale );
		}

		detail_surfaces[i] = NULL;
		es->detailchain = NULL;		
	}

	pglDisable( GL_BLEND );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

	if( RI.currententity->curstate.rendermode == kRenderTransAlpha )
		pglDepthFunc( GL_LEQUAL );

	draw_details = false;

	// restore fog here
	GL_ResetFogColor();
}

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly( msurface_t *fa )
{
	texture_t	*t;
	int	maps;
	qboolean	is_dynamic = false;
	qboolean	is_mirror = false;
	
	if( RI.currententity == clgame.entities )
		r_stats.c_world_polys++;
	else r_stats.c_brush_polys++; 

	if( fa->flags & SURF_DRAWSKY )
	{	
		if( world.sky_sphere )
		{
			// warp texture, no lightmaps
			EmitSkyLayers( fa );
		}
		return;
	}
		
	t = R_TextureAnimation( fa->texinfo->texture, fa - RI.currententity->model->surfaces );

	if( RP_NORMALPASS() && fa->flags & SURF_REFLECT )
	{
		if( SURF_INFO( fa, RI.currentmodel )->mirrortexturenum )
		{
			GL_Bind( XASH_TEXTURE0, SURF_INFO( fa, RI.currentmodel )->mirrortexturenum );
			is_mirror = true;

			// BEGIN WATER STUFF
			if( fa->flags & SURF_DRAWTURB )
			{
				R_BeginDrawMirror( fa );
				GL_Bind( XASH_TEXTURE1, t->gl_texturenum );
				pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
			}
		}
		else GL_Bind( XASH_TEXTURE0, t->gl_texturenum ); // dummy

		// DEBUG: reset the mirror texture after drawing
		SURF_INFO( fa, RI.currentmodel )->mirrortexturenum = 0;
	}
	else GL_Bind( XASH_TEXTURE0, t->gl_texturenum );

	if( fa->flags & SURF_DRAWTURB )
	{	
		// warp texture, no lightmaps
		EmitWaterPolys( fa->polys, ( fa->flags & SURF_NOCULL ));
		if( is_mirror ) R_EndDrawMirror();
		// END WATER STUFF
		return;
	}

	if( t->fb_texturenum )
	{
		// HACKHACK: store fullbrights in poly->next (only for non-water surfaces)
		fa->polys->next = fullbright_polys[t->fb_texturenum];
		fullbright_polys[t->fb_texturenum] = fa->polys;
		draw_fullbrights = true;
	}

	if( r_detailtextures->integer )
	{
		mextrasurf_t *es = SURF_INFO( fa, RI.currentmodel );

		if( RI.fogEnabled || RI.fogCustom )
		{
			// don't apply detail textures for windows in the fog
			if( RI.currententity->curstate.rendermode != kRenderTransTexture )
			{
				if( t->dt_texturenum )
				{
					es->detailchain = detail_surfaces[t->dt_texturenum];
					detail_surfaces[t->dt_texturenum] = es;
				}
				else
				{
					// draw stub detail texture for underwater surfaces
					es->detailchain = detail_surfaces[tr.grayTexture];
					detail_surfaces[tr.grayTexture] = es;
				}
				draw_details = true;
			}
		}
		else if( t->dt_texturenum )
		{
			es->detailchain = detail_surfaces[t->dt_texturenum];
			detail_surfaces[t->dt_texturenum] = es;
			draw_details = true;
		}
	}

	if( is_mirror ) R_BeginDrawMirror( fa );
	DrawGLPoly( fa->polys, 0.0f, 0.0f );
	if( is_mirror ) R_EndDrawMirror();
	DrawSurfaceDecals( fa );

	// NOTE: draw mirror through in mirror show dummy lightmapped texture
	if( fa->flags & SURF_REFLECT && RP_NORMALPASS() && r_lighting_extended->integer )
		return; // no lightmaps for mirror

	if( fa->flags & SURF_DRAWTILED )
		return; // no lightmaps anyway

	// check for lightmap modification
	for( maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++ )
	{
		if( RI.lightstylevalue[fa->styles[maps]] != fa->cached_light[maps] )
			goto dynamic;
	}

	if( maps == MAXLIGHTMAPS )
		maps--;

	// dynamic this frame or dynamic previously
	if( fa->dlightframe == tr.framecount )
	{
dynamic:
		// NOTE: at this point we have only valid textures
		if( r_dynamic->integer ) is_dynamic = true;
	}

	if( is_dynamic )
	{
		if(( fa->styles[maps] >= 32 || fa->styles[maps] == 0 ) && ( fa->dlightframe != tr.framecount ))
		{
			byte	temp[132*132*4];
			int	smax, tmax;

			smax = ( fa->extents[0] / LM_SAMPLE_SIZE ) + 1;
			tmax = ( fa->extents[1] / LM_SAMPLE_SIZE ) + 1;

			if( smax < 132 && tmax < 132 )
			{
				if( tr.deluxemap )
				{
					R_BuildDeluxeMap( fa, temp, smax * 4 );

					GL_Bind( XASH_TEXTURE0, tr.deluxemapTextures[fa->lightmaptexturenum] );

					pglTexSubImage2D( GL_TEXTURE_2D, 0, fa->light_s, fa->light_t, smax, tmax,
						GL_RGBA, GL_UNSIGNED_BYTE, temp );
				}

				R_BuildLightMap( fa, temp, smax * 4, true );
			}
			else
			{
				smax = min( smax, 132 );
				tmax = min( tmax, 132 );
				Host_MapDesignError( "R_RenderBrushPoly: bad surface extents: %d %d", fa->extents[0], fa->extents[1] );
				Q_memset( temp, 255, sizeof( temp ) );
			}
			R_SetCacheState( fa );

			GL_Bind( XASH_TEXTURE0, tr.lightmapTextures[fa->lightmaptexturenum] );

			pglTexSubImage2D( GL_TEXTURE_2D, 0, fa->light_s, fa->light_t, smax, tmax,
				GL_RGBA, GL_UNSIGNED_BYTE, temp );

			fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
			gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
		}
		else
		{
			fa->lightmapchain = gl_lms.dynamic_surfaces;
			gl_lms.dynamic_surfaces = fa;
		}
	}
	else
	{
		fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
		gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
	}
}

/*
================
R_DrawTextureChains
================
*/
void R_DrawTextureChains( void )
{
	int		i;
	msurface_t	*s;
	texture_t		*t;

	// make sure what color is reset
	pglColor4ub( 255, 255, 255, 255 );
	R_LoadIdentity();	// set identity matrix

	GL_SetupFogColorForSurfaces();

	// restore worldmodel
	RI.currententity = clgame.entities;
	RI.currentmodel = RI.currententity->model;

	// clip skybox surfaces
	for( s = skychain; s != NULL; s = s->texturechain )
		R_AddSkyBoxSurface( s );

	for( i = 0; i < cl.worldmodel->numtextures; i++ )
	{
		t = cl.worldmodel->textures[i];
		if( !t ) continue;

		s = t->texturechain;
		if( !s ) continue;

		if( i == tr.skytexturenum )
		{
			if( world.sky_sphere )
				R_DrawSkyChain( s );
		}
		else
		{
			if(( s->flags & SURF_DRAWTURB ) && cl.refdef.movevars->wateralpha < 1.0f )
				continue;	// draw translucent water later

			for( ; s != NULL; s = s->texturechain )
				R_RenderBrushPoly( s );
		}
		t->texturechain = NULL;
	}

	GL_ResetFogColor();
}

/*
================
R_DrawWaterSurfaces
================
*/
void R_DrawWaterSurfaces( void )
{
	int		i;
	msurface_t	*s;
	texture_t		*t;

	if( !RI.drawWorld || RI.refdef.onlyClientDraw )
		return;

	// non-transparent water is already drawed
	if( cl.refdef.movevars->wateralpha >= 1.0f )
		return;

	// go back to the world matrix
	R_LoadIdentity();

	pglEnable( GL_BLEND );
	pglDepthMask( GL_FALSE );
	pglDisable( GL_ALPHA_TEST );
	pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	pglColor4f( 1.0f, 1.0f, 1.0f, cl.refdef.movevars->wateralpha );

	for( i = 0; i < cl.worldmodel->numtextures; i++ )
	{
		t = cl.worldmodel->textures[i];
		if( !t ) continue;

		s = t->texturechain;
		if( !s ) continue;

		if(!( s->flags & SURF_DRAWTURB ))
			continue;

		// set modulate mode explicitly
		GL_Bind( XASH_TEXTURE0, t->gl_texturenum );

		for( ; s; s = s->texturechain )
			EmitWaterPolys( s->polys, ( s->flags & SURF_NOCULL ));
			
		t->texturechain = NULL;
	}

	pglDisable( GL_BLEND );
	pglDepthMask( GL_TRUE );
	pglDisable( GL_ALPHA_TEST );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	pglColor4ub( 255, 255, 255, 255 );
}

/*
=================
R_SurfaceCompare

compare translucent surfaces
=================
*/
static int R_SurfaceCompare( const msurface_t **a, const msurface_t **b )
{
	msurface_t	*surf1, *surf2;
	mextrasurf_t	*info1, *info2;
	vec3_t		org1, org2;
	float		len1, len2;

	surf1 = (msurface_t *)*a;
	surf2 = (msurface_t *)*b;

	info1 = SURF_INFO( surf1, RI.currentmodel );
	info2 = SURF_INFO( surf2, RI.currentmodel );

	VectorAdd( RI.currententity->origin, info1->origin, org1 );
	VectorAdd( RI.currententity->origin, info2->origin, org2 );

	// compare by plane dists
	len1 = DotProduct( org1, RI.vforward ) - RI.viewplanedist;
	len2 = DotProduct( org2, RI.vforward ) - RI.viewplanedist;

	if( len1 > len2 )
		return -1;
	if( len1 < len2 )
		return 1;

	return 0;
}

_inline qboolean R_HasLightmap( void )
{
	if( r_fullbright->integer || !cl.worldmodel->lightdata )
		return false;

	if( RI.currententity )
	{
		// check for rendermode
		switch( RI.currententity->curstate.rendermode )
		{
		case kRenderTransTexture:
		case kRenderTransColor:
		case kRenderTransAdd:
		case kRenderGlow:
			return false; // no lightmaps
		}

		if( RI.currententity->curstate.effects & EF_FULLBRIGHT )
			return false;	// disabled by user
	}

	return true;
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel( cl_entity_t *e )
{
	int		i, k, num_sorted;
	qboolean		need_sort = false;
	vec3_t		origin_l, oldorigin;
	vec3_t		mins, maxs;
	msurface_t	*psurf;
	model_t		*clmodel;
	qboolean		rotated;
	qboolean allow_vbo = r_vbo->integer;
	dlight_t		*l;

	if( !RI.drawWorld ) return;

	clmodel = e->model;

	// external models not loaded to VBO
	if( clmodel->surfaces != cl.worldmodel->surfaces )
		allow_vbo = false;

	RI.currentWaveHeight = RI.currententity->curstate.scale * 32.0f;

	// don't reflect this entity in mirrors
	if( e->curstate.effects & EF_NOREFLECT && RI.params & RP_MIRRORVIEW )
		return;

	// draw only in mirrors
	if( e->curstate.effects & EF_REFLECTONLY && !( RI.params & RP_MIRRORVIEW ))
		return;

	if( !VectorIsNull( e->angles ))
	{
		for( i = 0; i < 3; i++ )
		{
			mins[i] = e->origin[i] - clmodel->radius;
			maxs[i] = e->origin[i] + clmodel->radius;
		}
		rotated = true;
	}
	else
	{
		VectorAdd( e->origin, clmodel->mins, mins );
		VectorAdd( e->origin, clmodel->maxs, maxs );
		rotated = false;
	}

	if( R_CullBox( mins, maxs, RI.clipFlags ))
		return;

	Q_memset( gl_lms.lightmap_surfaces, 0, sizeof( gl_lms.lightmap_surfaces ));
	gl_lms.dynamic_surfaces = NULL;

	if( rotated ) R_RotateForEntity( e );
	else R_TranslateForEntity( e );

	e->visframe = tr.framecount; // visible

	if( rotated ) Matrix4x4_VectorITransform( RI.objectMatrix, RI.cullorigin, tr.modelorg );
	else VectorSubtract( RI.cullorigin, e->origin, tr.modelorg );

	// calculate dynamic lighting for bmodel
	for( k = 0, l = cl_dlights; k < MAX_DLIGHTS; k++, l++ )
	{
		if( l->die < cl.time || !l->radius )
			continue;

		VectorCopy( l->origin, oldorigin ); // save lightorigin
		Matrix4x4_VectorITransform( RI.objectMatrix, l->origin, origin_l );
		VectorCopy( origin_l, l->origin ); // move light in bmodel space
		R_MarkLights( l, 1U << k, clmodel->nodes + clmodel->hulls[0].firstclipnode );
		VectorCopy( oldorigin, l->origin ); // restore lightorigin
	}

	// setup the rendermode
	GL_SetRenderMode( e->curstate.rendermode );
	GL_SetupFogColorForSurfaces ();

	if( e->curstate.rendermode == kRenderTransTexture && r_lighting_extended->value >= 2.0f )
		pglBlendFunc( GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA );

	// setup the color and alpha
	switch( e->curstate.rendermode )
	{
	case kRenderTransAdd:
		if( RI.fogCustom )
			pglDisable( GL_FOG );
	case kRenderTransTexture:
		need_sort = true;
		allow_vbo = false;
	case kRenderGlow:
		pglColor4ub( 255, 255, 255, e->curstate.renderamt );
		break;
	case kRenderTransColor:
		pglDisable( GL_TEXTURE_2D );
		pglColor4ub( e->curstate.rendercolor.r, e->curstate.rendercolor.g,
			e->curstate.rendercolor.b, e->curstate.renderamt );
		allow_vbo = false;
		break;
	case kRenderTransAlpha:
		// NOTE: brushes can't change renderamt for 'Solid' mode
		pglAlphaFunc( GL_GEQUAL, 0.5f );
	default:	
		pglColor4ub( 255, 255, 255, 255 );
		break;
	}

	num_sorted = 0;

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];
	for( i = 0; i < clmodel->nummodelsurfaces; i++, psurf++ )
	{
		if( R_CullSurface( psurf, 0 ))
			continue;

		if( need_sort && !gl_nosort->integer )
		{
			world.draw_surfaces[num_sorted] = psurf;
			num_sorted++;
			ASSERT( world.max_surfaces >= num_sorted );
		}
		else
		{
			// render unsorted (solid)
			if( !allow_vbo || !R_AddSurfToVBO( psurf, true ) )
				R_RenderBrushPoly( psurf );
		}
	}

	if( need_sort && !gl_nosort->integer )
		qsort( world.draw_surfaces, num_sorted, sizeof( msurface_t* ), (void*)R_SurfaceCompare );

	// draw sorted translucent surfaces
	for( i = 0; i < num_sorted; i++ )
		R_RenderBrushPoly( world.draw_surfaces[i] );

	GL_ResetFogColor();

	if( allow_vbo && !num_sorted )
		R_DrawVBO( R_HasLightmap(), true );

	if( e->curstate.rendermode == kRenderTransColor )
		pglEnable( GL_TEXTURE_2D );

	R_BlendLightmaps();
	R_RenderFullbrights();
	R_RenderDetails();

	// restore fog here
	if( e->curstate.rendermode == kRenderTransAdd )
	{
		if( RI.fogCustom )
			pglEnable( GL_FOG );
	}

	R_LoadIdentity();	// restore worldmatrix
}

/*
=================
R_DrawStaticModel

Merge static model brushes with world surfaces
=================
*/
void R_DrawStaticModel( cl_entity_t *e )
{
	int		i, k;
	model_t		*clmodel;
	msurface_t	*psurf;
	dlight_t		*l;
	
	clmodel = e->model;
	if( R_CullBox( clmodel->mins, clmodel->maxs, RI.clipFlags ))
		return;

	// calculate dynamic lighting for bmodel
	for( k = 0, l = cl_dlights; k < MAX_DLIGHTS; k++, l++ )
	{
		if( l->die < cl.time || !l->radius )
			continue;
		R_MarkLights( l, 1U << k, clmodel->nodes + clmodel->hulls[0].firstclipnode );
	}

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];
	for( i = 0; i < clmodel->nummodelsurfaces; i++, psurf++ )
	{
		if( R_CullSurface( psurf, RI.clipFlags ))
			continue;

		if( psurf->flags & SURF_DRAWSKY && !world.sky_sphere )
		{
			// make sky chain to right clip the skybox
			psurf->texturechain = skychain;
			skychain = psurf;
		}
		else if( !R_AddSurfToVBO( psurf, true ) )
		{ 
			psurf->texturechain = psurf->texinfo->texture->texturechain;
			psurf->texinfo->texture->texturechain = psurf;
		}
	}
}

/*
=================
R_DrawStaticBrushes

Insert static brushes into world texture chains
=================
*/
void R_DrawStaticBrushes( void )
{
	uint	i;

	// draw static entities
	for( i = 0; i < tr.num_static_entities; i++ )
	{
		RI.currententity = tr.static_entities[i];
		RI.currentmodel = RI.currententity->model;
	
		ASSERT( RI.currententity != NULL );
		ASSERT( RI.currententity->model != NULL );

		switch( RI.currententity->model->type )
		{
		case mod_brush:
			R_DrawStaticModel( RI.currententity );
			break;
		default:
			Host_Error( "R_DrawStatics: non bsp model in static list!\n" );
			break;
		}
	}
}

/*
==============================

VBO

==============================
*/
/*
Bulld arrays (vboarray_t) for all map geometry on map load.
Store index base for every surface (vbosurfdata_t) to build index arrays
For each texture build index arrays (vbotexture_t) every frame.
*/
// vertex attribs
//#define NO_TEXTURE_MATRIX // need debug
typedef struct vbovertex_s
{
	vec3_t pos;
	vec2_t gl_tc;
	vec2_t lm_tc;
#ifdef NO_TEXTURE_MATRIX
	vec2_t dt_tc;
#endif
} vbovertex_t;

// store indexes for each texture
typedef struct vbotexture_s
{
	unsigned short *indexarray; // index array (generated instead of texture chains)
	uint curindex; // counter for index array
	uint len; // maximum index array length
	struct vbotexture_s *next; // if cannot fit into one array, allocate new one, as every array has own index space
	msurface_t *dlightchain; // list of dlight surfaces
	struct vboarray_s *vboarray; // debug
	uint lightmaptexturenum;
} vbotexture_t;

// array list
typedef struct vboarray_s
{
	uint glindex; // glGenBuffers
	int array_len; // allocation length
	vbovertex_t *array; // vertex attrib array
	struct vboarray_s *next; // split by 65536 vertices
} vboarray_t;

// every surface is linked to vbo texture
typedef struct vbosurfdata_s
{
	vbotexture_t *vbotexture;
	uint texturenum;
	uint startindex;
} vbosurfdata_t;

typedef struct vbodecal_s
{
	int numVerts;
} vbodecal_t;

#define DECAL_VERTS_MAX 32
#define DECAL_VERTS_CUT 8

typedef struct vbodecaldata_s
{
	vbodecal_t decals[MAX_RENDER_DECALS];
	vbovertex_t decalarray[MAX_RENDER_DECALS * DECAL_VERTS_CUT];
	uint decalvbo;
	msurface_t **lm;
} vbodecaldata_t;

// gl_decals.c
extern decal_t	gDecalPool[MAX_RENDER_DECALS];

struct vbo_static_s
{
	// quickly free all allocations on map change
	byte *mempool;

	// arays
	vbodecaldata_t *decaldata; // array
	vbotexture_t *textures; // array
	vbosurfdata_t *surfdata; // array
	vboarray_t *arraylist; // linked list

	// separate areay for dlights (build during draw)
	unsigned short *dlight_index; // array
	vec2_t *dlight_tc; // array
	unsigned int dlight_vbo;
	vbovertex_t decal_dlight[MAX_RENDER_DECALS * DECAL_VERTS_MAX];
	unsigned int decal_dlight_vbo;
	int decal_numverts[MAX_RENDER_DECALS * DECAL_VERTS_MAX];

	// prevent draining cpu on empty cycles
	int minlightmap;
	int maxlightmap;
	int mintexture;
	int maxtexture;

	// never skip array splits
	int minarraysplit_tex;
	int maxarraysplit_tex;
	int minarraysplit_lm;
	int maxarraysplit_lm;
} vbos;

/*
===================
R_GenerateVBO

Allocate memory for arrays, fill it with vertex attribs and upload to GPU
===================
*/
void R_GenerateVBO()
{
	int numtextures = cl.worldmodel->numtextures;
	int numlightmaps = gl_lms.current_lightmap_texture;
	int k, len = 0;
	vboarray_t *vbo;
	uint maxindex = 0;

	R_ClearVBO();

	// we do not want to write vbo code that does not use multitexture
	if( !GL_Support( GL_ARB_VERTEX_BUFFER_OBJECT_EXT ) || !GL_Support( GL_ARB_MULTITEXTURE ) || glConfig.max_texture_units < 2 )
	{
		Cvar_FullSet( "r_vbo", "0", CVAR_READ_ONLY );
		Cvar_FullSet( "r_bump", "0", CVAR_READ_ONLY );
		return;
	}

	// cannot do bump mapping without dot3, but still can use VBO
	if( !GL_Support( GL_DOT3_ARB_EXT ) || glConfig.max_texture_units < 4 )
		Cvar_FullSet( "r_bump", "0", CVAR_READ_ONLY );

	// save in config if enabled manually
	if( r_vbo->integer && host.developer > 3 )
		r_vbo->flags |= CVAR_ARCHIVE;

	vbos.mempool = Mem_AllocPool("Render VBO Zone");

	vbos.minarraysplit_tex = INT_MAX;
	vbos.maxarraysplit_tex = 0;
	vbos.minarraysplit_lm = MAXLIGHTMAPS;
	vbos.maxarraysplit_lm = 0;
	vbos.minlightmap = MAX_LIGHTMAPS;
	vbos.maxlightmap = 0;
	vbos.mintexture = INT_MAX;
	vbos.maxtexture = 0;

	vbos.textures = Mem_Alloc( vbos.mempool, numtextures * numlightmaps * sizeof( vbotexture_t ) );
	vbos.surfdata = Mem_Alloc( vbos.mempool, cl.worldmodel->numsurfaces * sizeof( vbosurfdata_t ) );
	vbos.arraylist = vbo = Mem_Alloc( vbos.mempool, sizeof( vboarray_t ) );
	vbos.decaldata = Mem_Alloc( vbos.mempool, sizeof( vbodecaldata_t ) );
	vbos.decaldata->lm = Mem_Alloc( vbos.mempool, sizeof( msurface_t* ) * numlightmaps );

	// count array lengths
	for( k = 0; k < numlightmaps; k++ )
	{
		int j;

		for( j = 0; j < numtextures; j++ )
		{
			int i;
			vbotexture_t *vbotex = &vbos.textures[k * numtextures + j];

			for( i = 0; i < cl.worldmodel->numsurfaces; i++ )
			{
				msurface_t *surf = &cl.worldmodel->surfaces[i];

				if( surf->flags & ( SURF_DRAWSKY | SURF_DRAWTURB | SURF_CONVEYOR | SURF_REFLECT ) )
					continue;

				if( surf->lightmaptexturenum != k )
					continue;

				if( R_TextureRandomTiling( surf->texinfo->texture, i ) != cl.worldmodel->textures[j] )
					continue;

				if( vbo->array_len + surf->polys->numverts > USHRT_MAX )
				{
					// generate new array and new vbotexture node
					vbo->array = Mem_Alloc( vbos.mempool, sizeof( vbovertex_t ) * vbo->array_len );
					MsgDev( D_NOTE, "R_GenerateVBOs: allocated array of %d verts, texture %d\n", vbo->array_len, j );
					vbo->next = Mem_Alloc( vbos.mempool, sizeof( vboarray_t ) );
					vbo = vbo->next;
					vbotex->next = Mem_Alloc( vbos.mempool, sizeof( vbotexture_t ) );
					vbotex = vbotex->next;

					// never skip this textures and lightmaps
					if( vbos.minarraysplit_tex > j )
						vbos.minarraysplit_tex = j;
					if( vbos.minarraysplit_lm > k )
						vbos.minarraysplit_lm = k;
					if( vbos.maxarraysplit_tex < j + 1 )
						vbos.maxarraysplit_tex = j + 1;
					if( vbos.maxarraysplit_lm < k + 1 )
						vbos.maxarraysplit_lm = k + 1;
				}
				vbos.surfdata[i].vbotexture = vbotex;
				vbos.surfdata[i].startindex = vbo->array_len;
				vbos.surfdata[i].texturenum = j;
				vbo->array_len += surf->polys->numverts;
				vbotex->len += surf->polys->numverts;
				vbotex->vboarray = vbo;
			}
		}
	}

	// allocate last array
	vbo->array = Mem_Alloc( vbos.mempool, sizeof( vbovertex_t ) * vbo->array_len );
	MsgDev( D_NOTE, "R_GenerateVBOs: allocated array of %d verts\n", vbo->array_len );

	// switch to list begin
	vbo = vbos.arraylist;
	
	// fill and upload
	for( k = 0; k < numlightmaps; k++ )
	{
		int j;

		for( j = 0; j < numtextures; j++ )
		{
			int i;
			vbotexture_t *vbotex = &vbos.textures[k * numtextures + j];

			// preallocate index arrays
			vbotex->indexarray = Mem_Alloc( vbos.mempool, sizeof( unsigned short ) * 6 *  vbotex->len );
			vbotex->lightmaptexturenum = k;

			if( maxindex < vbotex->len )
				maxindex = vbotex->len;

			for( i = 0; i < cl.worldmodel->numsurfaces; i++ )
			{
				msurface_t *surf = &cl.worldmodel->surfaces[i];
				int l;

				if( surf->flags & ( SURF_DRAWSKY | SURF_DRAWTURB | SURF_CONVEYOR | SURF_REFLECT ) )
					continue;

				if( surf->lightmaptexturenum != k )
					continue;

				if( R_TextureRandomTiling( surf->texinfo->texture, i ) != cl.worldmodel->textures[j] )
					continue;

				// switch to next array
				if( len + surf->polys->numverts > USHRT_MAX )
				{
					// upload last generated array
					pglGenBuffersARB( 1, &vbo->glindex );
					pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->glindex );
					pglBufferDataARB( GL_ARRAY_BUFFER_ARB, vbo->array_len * sizeof( vbovertex_t ), vbo->array, GL_STATIC_DRAW_ARB );

					ASSERT( len == vbo->array_len );

					vbo = vbo->next;
					vbotex = vbotex->next;
					vbotex->indexarray = Mem_Alloc( vbos.mempool, sizeof( unsigned short ) * 6 *  vbotex->len );
					vbotex->lightmaptexturenum = k;

					// calculate limits for dlights
					if( maxindex < vbotex->len )
						maxindex = vbotex->len;

					len = 0;
				}

				// fill vbovertex_t
				for( l = 0; l < surf->polys->numverts; l++ )
				{
					float *v = surf->polys->verts[l];

					VectorCopy( v, vbo->array[len + l].pos );
					vbo->array[len + l].gl_tc[0] = v[3];
					vbo->array[len + l].gl_tc[1] = v[4];
					vbo->array[len + l].lm_tc[0] = v[5];
					vbo->array[len + l].lm_tc[1] = v[6];
#ifdef NO_TEXTURE_MATRIX
					if( cl.worldmodel->textures[j]->dt_texturenum )
					{
						gltexture_t *glt = R_GetTexture( cl.worldmodel->textures[j]->gl_texturenum );
						vbo->array[len + l].dt_tc[0] = v[3] * glt->xscale;
						vbo->array[len + l].dt_tc[1] = v[4] * glt->yscale;
					}
#endif
				}

				len += surf->polys->numverts;

			}
		}
	}
	ASSERT( len == vbo->array_len );

	// upload last array
	pglGenBuffersARB( 1, &vbo->glindex );
	pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->glindex );
	pglBufferDataARB( GL_ARRAY_BUFFER_ARB, vbo->array_len * sizeof( vbovertex_t ), vbo->array, GL_STATIC_DRAW_ARB );

	// prepare decal array
	pglGenBuffersARB( 1, &vbos.decaldata->decalvbo );
	pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbos.decaldata->decalvbo );
	pglBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof( vbovertex_t ) * DECAL_VERTS_CUT * MAX_RENDER_DECALS, vbos.decaldata->decalarray, GL_STATIC_DRAW_ARB );

	// preallocate dlight arrays
	vbos.dlight_index = Mem_Alloc( vbos.mempool, maxindex * sizeof( unsigned short ) * 6 );

	// select maximum possible length for dlight
	vbos.dlight_tc = Mem_Alloc( vbos.mempool, sizeof( vec2_t ) * (int)(vbos.arraylist->next?USHRT_MAX + 1:vbos.arraylist->array_len + 1) );

	if( r_vbo_dlightmode->integer == 1 )
	{
		pglGenBuffersARB( 1, &vbos.dlight_vbo );
		pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbos.dlight_vbo );
		pglBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof( vec2_t ) * (int)(vbos.arraylist->next?USHRT_MAX + 1:vbos.arraylist->array_len + 1) , vbos.dlight_tc, GL_DYNAMIC_DRAW_ARB );
		pglGenBuffersARB( 1, &vbos.decal_dlight_vbo );
		pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbos.decal_dlight_vbo );
		pglBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof( vbos.decal_dlight ), vbos.decal_dlight, GL_DYNAMIC_DRAW_ARB );
	}


	// reset state
	pglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
}

/*
==============
R_AddDecalVBO

generate decal mesh and put it to array
==============
*/
void R_AddDecalVBO( decal_t *pdecal, msurface_t *surf )
{
	int numVerts, i;
	float *v;
	int decalindex = pdecal - &gDecalPool[0];

	if( !vbos.decaldata )
		return;

	v = R_DecalSetupVerts( pdecal, surf, pdecal->texture, &numVerts );

	if( numVerts > DECAL_VERTS_CUT )
	{
		// use client arrays
		vbos.decaldata->decals[decalindex].numVerts = -1;
		return;
	}

	for( i = 0; i < numVerts; i++ )
		Q_memcpy( &vbos.decaldata->decalarray[decalindex * DECAL_VERTS_CUT + i], v + i * VERTEXSIZE, VERTEXSIZE * 4 );

	pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbos.decaldata->decalvbo );
	pglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, decalindex * sizeof( vbovertex_t ) * DECAL_VERTS_CUT, sizeof( vbovertex_t ) * numVerts, &vbos.decaldata->decalarray[decalindex * DECAL_VERTS_CUT] );
	pglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

	vbos.decaldata->decals[decalindex].numVerts = numVerts;
}

/*
=============
R_ClearVBO

free all vbo data
=============
*/
void R_ClearVBO()
{
	vboarray_t *vbo;

	for( vbo = vbos.arraylist; vbo; vbo = vbo->next )
		pglDeleteBuffersARB( 1, &vbo->glindex );

	vbos.arraylist = NULL;

	if( vbos.decaldata )
		pglDeleteBuffersARB( 1, &vbos.decaldata->decalvbo );

	if( vbos.dlight_vbo )
		pglDeleteBuffersARB( 1, &vbos.dlight_vbo );

	if( vbos.decal_dlight_vbo )
		pglDeleteBuffersARB( 1, &vbos.decal_dlight_vbo );
	vbos.decal_dlight_vbo = vbos.dlight_vbo = 0;

	vbos.decaldata = NULL;
	Mem_FreePool( &vbos.mempool );
}

/*
====================

MULTITEXTURE HELPERS

====================
*/

struct multitexturestate_s
{
	int tmu_gl; // texture tmu
	int tmu_dt; // detail tmu
	int tmu_lm; // lightmap tmu
	qboolean details_enabled; // current texture has details
	qboolean bump_enabled; // current texture has bumpmap
	int lm; // current lightmap texture
	int dl; // current deluxemap texure
	qboolean skipbump;
	qboolean skiptexture;
	gltexture_t *glt; // details scale
} mtst;


// when bump enabled, TEXTURE0 is deluxemap and TEXTURE1 is normalmap
// texture and lightmap on TEXTURE2 and TEXTURE3
// detail uses TEXTURE2 when bump disabled

/*
===================
R_DisableDetail

disable detail tmu
===================
*/
static void R_DisableDetail( void )
{
	if( mtst.details_enabled && mtst.tmu_dt != -1 )
	{
		GL_SelectTexture( mtst.tmu_dt );
		pglDisableClientState( GL_TEXTURE_COORD_ARRAY );
		pglDisable( GL_TEXTURE_2D );
		pglLoadIdentity();
	}
}

/*
===================
R_EnableDetail

enable detail tmu if availiable
===================
*/
static void R_EnableDetail( void )
{
	if( mtst.details_enabled && mtst.tmu_dt != -1 )
	{
		GL_SelectTexture( mtst.tmu_dt );
		pglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		pglEnable( GL_TEXTURE_2D );
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
		pglTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE );
		pglTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB );
		pglTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE );
		pglTexEnvi( GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2 );

		// use transform matrix for details (undone)
#ifndef NO_TEXTURE_MATRIX
		pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, gl_tc ) );
		pglMatrixMode( GL_TEXTURE );
		pglLoadIdentity();
		pglScalef( mtst.glt->xscale, mtst.glt->yscale, 1 );
#else
		pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, dt_tc ) );
#endif
	}
}

/*
==============
R_SetLightmap

enable lightmap on current tmu
==============
*/
static void R_SetLightmap( void )
{
	if( mtst.skiptexture )
		return;

	if( gl_overbright->integer )
	{
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
		pglTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE );
		pglTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB );
		pglTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE );
		pglTexEnvi( GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2 );

	}
	else
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, lm_tc ) );
}

/*
==============
R_DisableBump

disable bump-mapping if current texture has normalmap
set tmu to lightmap
==============
*/
static void R_DisableBump()
{
	if( mtst.bump_enabled )
	{
		// disable highter tmu's
		GL_SelectTexture( XASH_TEXTURE3 );
		pglDisableClientState( GL_TEXTURE_COORD_ARRAY );
		pglDisable( GL_TEXTURE_2D );

		GL_SelectTexture( XASH_TEXTURE2 );
		pglDisableClientState( GL_TEXTURE_COORD_ARRAY );
		pglDisable( GL_TEXTURE_2D );

		// restore lightmap
		GL_Bind( XASH_TEXTURE1, mtst.lm );
		R_SetLightmap();

		// re-arrange tmus
		mtst.tmu_lm = XASH_TEXTURE1;
		mtst.tmu_gl = XASH_TEXTURE0;

		// now details availiable on tmu2
		if( mtst.details_enabled )
			mtst.tmu_dt = XASH_TEXTURE2;
	}
}

/*
==============
R_EnableBump

setup bump-mapping if current texture has normalmap
==============
*/
static void R_EnableBump()
{
	if( mtst.bump_enabled )
	{
		// move ligthmap and texture to higer tmus
		GL_Bind( XASH_TEXTURE3, mtst.lm );
		pglEnable( GL_TEXTURE_2D );
		pglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		R_SetLightmap();
	
		GL_SelectTexture( XASH_TEXTURE2 );
		pglEnable( GL_TEXTURE_2D );
		pglEnableClientState( GL_TEXTURE_COORD_ARRAY );

		// clear detail matrix
		pglMatrixMode( GL_TEXTURE );
		pglLoadIdentity();

		pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof( vbovertex_t, gl_tc ) );
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

		// update deluxemap and bind normalmap
		GL_Bind( XASH_TEXTURE0, mtst.dl );
		pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof( vbovertex_t, lm_tc ) );
		GL_SelectTexture( XASH_TEXTURE1 );
		pglEnable( GL_TEXTURE_2D );
		pglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof( vbovertex_t, gl_tc ) );

		// set dot3
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
		pglTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGB_ARB );
		pglTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB );
		pglTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR );
		pglTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE );
		pglTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR );
		pglTexEnvi( GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1 );

		// rearrange TMUs
		mtst.tmu_lm = XASH_TEXTURE3;
		mtst.tmu_gl = XASH_TEXTURE2;

		// 4 tmu not enough for bump and detail
		if( glConfig.max_texture_units <= 4 )
			mtst.tmu_dt = -1; // second pass
		else
			mtst.tmu_dt = XASH_TEXTURE4;
	}
}

/*
==============
R_SetDecalMode

When drawing decal, disable or restore bump and details
set tmu to lightmap when enabled
==============
*/
static void R_SetDecalMode( qboolean enable )
{
	// order is important to correctly rearrange TMUs
	if( enable )
	{
		// disable detail texture if enabled
		R_DisableDetail();
		R_DisableBump();
	}
	else
	{
		R_EnableBump();
		R_EnableDetail();
	}

}

/*
==============
R_SetupVBOTexture

setup multitexture mode before drawing VBOs
if tex is NULL, load texture by number
==============
*/
static texture_t *R_SetupVBOTexture( texture_t *tex, int number )
{
	if( mtst.skiptexture )
		return tex;

	if( !tex )
		tex = R_TextureAnim( cl.worldmodel->textures[number] );

	if( r_bump->integer && !mtst.skipbump && tr.deluxemap && tex->nm_texturenum )
	{
		if( !mtst.bump_enabled )
		{
			mtst.bump_enabled = true;
			R_EnableBump();
		}

		GL_Bind( XASH_TEXTURE1, tex->nm_texturenum );
	}
	else
	{
		R_DisableBump();
		mtst.bump_enabled = false;
	}

	if( r_detailtextures->integer && tex->dt_texturenum && mtst.tmu_dt != -1 )
	{
		mtst.details_enabled = true;
		GL_Bind( mtst.tmu_dt, tex->dt_texturenum );
		mtst.glt = R_GetTexture( tex->gl_texturenum );
		R_EnableDetail();
	}
	else R_DisableDetail();

	GL_Bind( mtst.tmu_gl, r_lightmap->integer?tr.whiteTexture:tex->gl_texturenum );

	return tex;
}

/*
===================
R_AdditionalPasses

draw details when not enough tmus
===================
*/
static void R_AdditionalPasses( vboarray_t *vbo, int indexlen, void *indexarray, texture_t *tex, qboolean resetvbo )
{
	// draw details in additional pass
	if( r_detailtextures->integer && mtst.tmu_dt == -1 && tex->dt_texturenum )
	{
		gltexture_t *glt = R_GetTexture( tex->gl_texturenum );

		// disable bump
		if( mtst.bump_enabled )
		{
			GL_SelectTexture( XASH_TEXTURE3 );
			pglDisable( GL_TEXTURE_2D );
			GL_SelectTexture( XASH_TEXTURE2 );
			pglDisable( GL_TEXTURE_2D );
		}

		GL_SelectTexture( XASH_TEXTURE1 );
		pglDisable( GL_TEXTURE_2D );

		// setup detail
		GL_Bind( XASH_TEXTURE0, tex->dt_texturenum );
		pglEnable( GL_BLEND );
		pglBlendFunc( GL_DST_COLOR, GL_SRC_COLOR );
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );

		// when drawing dlights, we need to bind array and unbind it again
		if( resetvbo )
			pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->glindex );

		pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof( vbovertex_t, gl_tc ) );

		// apply scale
		pglMatrixMode( GL_TEXTURE );
		pglLoadIdentity();
		pglScalef( glt->xscale, glt->yscale, 1 );

		// draw
#if !defined XASH_NANOGL || defined XASH_WES && defined __EMSCRIPTEN__ // WebGL need to know array sizes
		if( pglDrawRangeElements )
			pglDrawRangeElements( GL_TRIANGLES, 0, vbo->array_len, indexlen, GL_UNSIGNED_SHORT, indexarray );
		else
#endif
		pglDrawElements( GL_TRIANGLES, indexlen, GL_UNSIGNED_SHORT, indexarray );


		// restore state
		pglLoadIdentity();
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
		pglDisable( GL_BLEND );
		if( mtst.bump_enabled )
		{
			GL_Bind( XASH_TEXTURE0, mtst.dl );
			pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof( vbovertex_t, lm_tc ) );
			GL_SelectTexture( XASH_TEXTURE3 );
			pglEnable( GL_TEXTURE_2D );
			GL_SelectTexture( XASH_TEXTURE2 );
			pglEnable( GL_TEXTURE_2D );
		}
		else
		{
			GL_Bind( XASH_TEXTURE1, mtst.lm );
			pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof( vbovertex_t, lm_tc ) );
		}
		GL_SelectTexture( XASH_TEXTURE1 );
		pglEnable( GL_TEXTURE_2D );
		if( resetvbo )
			pglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	}
}

/*
=====================
R_DrawLightmappedVBO

Draw array for given vbotexture_t. build and draw dynamic lightmaps if present
=====================
*/
static void R_DrawLightmappedVBO( vboarray_t *vbo, vbotexture_t *vbotex, texture_t *texture, int lightmap, qboolean skiplighting )
{
#if !defined XASH_NANOGL || defined XASH_WES && defined __EMSCRIPTEN__ // WebGL need to know array sizes
	if( pglDrawRangeElements )
		pglDrawRangeElements( GL_TRIANGLES, 0, vbo->array_len, vbotex->curindex, GL_UNSIGNED_SHORT, vbotex->indexarray );
	else
#endif
	pglDrawElements( GL_TRIANGLES, vbotex->curindex, GL_UNSIGNED_SHORT, vbotex->indexarray );

	R_AdditionalPasses( vbo, vbotex->curindex, vbotex->indexarray, texture, false );

	// draw debug lines
	if( gl_wireframe->integer && !skiplighting )
	{
		R_SetDecalMode( true );
		pglDisable( GL_TEXTURE_2D );
		GL_SelectTexture( XASH_TEXTURE0 );
		pglDisable( GL_TEXTURE_2D );
		pglDisable( GL_DEPTH_TEST );
#if !defined XASH_NANOGL || defined XASH_WES && defined __EMSCRIPTEN__ // WebGL need to know array sizes
		if( pglDrawRangeElements )
			pglDrawRangeElements( GL_LINES, 0, vbo->array_len, vbotex->curindex, GL_UNSIGNED_SHORT, vbotex->indexarray );
		else
#endif
		pglDrawElements( GL_LINES, vbotex->curindex, GL_UNSIGNED_SHORT, vbotex->indexarray );
		pglEnable( GL_DEPTH_TEST );
		pglEnable( GL_TEXTURE_2D );
		GL_SelectTexture( XASH_TEXTURE1 );
		pglEnable( GL_TEXTURE_2D );
		R_SetDecalMode( false );
	}
	//Msg( "%d %d %d\n", vbo->array_len, vbotex->len, lightmap );
	if( skiplighting )
	{
		vbotex->curindex = 0;
		vbotex->dlightchain = NULL;
		return;
	}

	// draw dlights and dlighted decals
	if( vbotex->dlightchain )
	{
		unsigned short *dlightarray = vbos.dlight_index; // preallocated array
		unsigned int dlightindex = 0;
		msurface_t *surf, *newsurf;
		int decalcount = 0;

		if( host.features & ENGINE_LARGE_LIGHTMAPS )
			GL_Bind( mtst.tmu_lm, tr.dlightTexture2 );
		else GL_Bind( mtst.tmu_lm, tr.dlightTexture );

		// replace lightmap texcoord array by dlight array
		pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbos.dlight_vbo );
		if( vbos.dlight_vbo  )
			pglTexCoordPointer( 2, GL_FLOAT, sizeof( float ) * 2, 0 );
		else
			pglTexCoordPointer( 2, GL_FLOAT, sizeof( float ) * 2, vbos.dlight_tc );

		// clear the block
		LM_InitBlock();

		// accumulate indexes for every dlighted surface until dlight block full
		for( surf = newsurf = vbotex->dlightchain; surf; surf = surf->lightmapchain )
		{
			int	smax, tmax;
			byte	*base;
			uint indexbase = vbos.surfdata[((char*)surf - (char*)cl.worldmodel->surfaces) / sizeof( *surf )].startindex;
			uint index;
			mextrasurf_t *info; // this stores current dlight offset
			decal_t *pdecal;

			smax = ( surf->extents[0] / LM_SAMPLE_SIZE ) + 1;
			tmax = ( surf->extents[1] / LM_SAMPLE_SIZE ) + 1;
			info = SURF_INFO( surf, RI.currentmodel );

			// find space for this surface and get offsets
			if( LM_AllocBlock( smax, tmax, &info->dlight_s, &info->dlight_t ))
			{
				base = gl_lms.lightmap_buffer;
				base += ( info->dlight_t * BLOCK_SIZE + info->dlight_s ) * 4;

				R_BuildLightMap( surf, base, BLOCK_SIZE * 4, true );
			}
			else
			{
				// out of free block space. Draw all generated index array and clear it
				// upload already generated block
				LM_UploadDynamicBlock();
#if !defined XASH_NANOGL || defined XASH_WES && defined __EMSCRIPTEN__ // WebGL need to know array sizes
				if( pglDrawRangeElements )
					pglDrawRangeElements( GL_TRIANGLES, 0, vbo->array_len, dlightindex, GL_UNSIGNED_SHORT, dlightarray );
				else
#endif
				pglDrawElements( GL_TRIANGLES, dlightindex, GL_UNSIGNED_SHORT, dlightarray );

				// draw decals that lighted with this lightmap
				if( decalcount )
				{
					msurface_t *decalsurf;
					int decali = 0;

					pglDepthMask( GL_FALSE );
					pglEnable( GL_BLEND );
					pglEnable( GL_POLYGON_OFFSET_FILL );
					if( RI.currententity->curstate.rendermode == kRenderTransAlpha )
						pglDisable( GL_ALPHA_TEST );
					pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbos.decal_dlight_vbo );
					R_SetDecalMode( true );
					if( vbos.decal_dlight_vbo )
					{
						pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), offsetof( vbovertex_t, lm_tc ) );
						GL_SelectTexture( mtst.tmu_gl );
						pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), offsetof( vbovertex_t, gl_tc ) );
						pglVertexPointer( 3, GL_FLOAT, sizeof( vbovertex_t ), offsetof( vbovertex_t, pos ) );
					}
					else
					{
						pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), &vbos.decal_dlight[0].lm_tc );
						GL_SelectTexture( mtst.tmu_gl );
						pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), &vbos.decal_dlight[0].gl_tc );
						pglVertexPointer( 3, GL_FLOAT, sizeof( vbovertex_t ), &vbos.decal_dlight[0].pos);
					}

					for( decalsurf = newsurf; ( decali < decalcount ) && ( decalsurf != surf ); decalsurf = decalsurf->lightmapchain )
					{
						for( pdecal = decalsurf->pdecals; pdecal; pdecal = pdecal->pnext )
						{
							gltexture_t *glt;

							if( !pdecal->texture )
								continue;

							glt = R_GetTexture( pdecal->texture );

							GL_Bind( mtst.tmu_gl, pdecal->texture );

							// normal HL decal with alpha-channel
							if( glt->flags & TF_HAS_ALPHA )
							{
								// draw transparent decals with GL_MODULATE
								if( glt->fogParams[3] > 230 )
									pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
								else pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
								pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
							}
							else
							{
								// color decal like detail texture. Base color is 127 127 127
								pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
								pglBlendFunc( GL_DST_COLOR, GL_SRC_COLOR );
							}

							pglDrawArrays( GL_TRIANGLE_FAN, decali * DECAL_VERTS_MAX, vbos.decal_numverts[decali] );
							decali++;
						}
						newsurf = surf;

					}

					// restore states pointers for next dynamic lightmap
					pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
					pglDepthMask( GL_TRUE );
					pglDisable( GL_BLEND );
					pglDisable( GL_POLYGON_OFFSET_FILL );
					if( RI.currententity->curstate.rendermode == kRenderTransAlpha )
						pglEnable( GL_ALPHA_TEST );
					pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbos.dlight_vbo );
					R_SetDecalMode( false );
					GL_SelectTexture( mtst.tmu_lm );
					if( vbos.dlight_vbo )
						pglTexCoordPointer( 2, GL_FLOAT, sizeof( float ) * 2, 0 );
					else
						pglTexCoordPointer( 2, GL_FLOAT, sizeof( float ) * 2, vbos.dlight_tc );

					pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->glindex );
					pglVertexPointer( 3, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t,pos) );
					R_SetupVBOTexture( texture, 0 );
					pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, gl_tc ) );

					decalcount = 0;
				}

				// clear the block
				LM_InitBlock();
				dlightindex = 0;

				// try upload the block now
				if( !LM_AllocBlock( smax, tmax, &info->dlight_s, &info->dlight_t ))
					Host_Error( "AllocBlock: full\n" );

				base = gl_lms.lightmap_buffer;
				base += ( info->dlight_t * BLOCK_SIZE + info->dlight_s ) * 4;

				R_BuildLightMap( surf, base, BLOCK_SIZE * 4, true );
			}

			// build index and texcoords arrays
			vbos.dlight_tc[indexbase][0] = surf->polys->verts[0][5] - ( surf->light_s - info->dlight_s ) * ( 1.0f / (float)BLOCK_SIZE );
			vbos.dlight_tc[indexbase][1] = surf->polys->verts[0][6] - ( surf->light_t - info->dlight_t ) * ( 1.0f / (float)BLOCK_SIZE );
			vbos.dlight_tc[indexbase + 1][0] = surf->polys->verts[1][5] - ( surf->light_s - info->dlight_s ) * ( 1.0f / (float)BLOCK_SIZE );
			vbos.dlight_tc[indexbase + 1][1] = surf->polys->verts[1][6] - ( surf->light_t - info->dlight_t ) * ( 1.0f / (float)BLOCK_SIZE );

			for( index = indexbase + 2; index < indexbase + surf->polys->numverts; index++ )
			{
				dlightarray[dlightindex++] = indexbase;
				dlightarray[dlightindex++] = index - 1;
				dlightarray[dlightindex++] = index;
				vbos.dlight_tc[index][0] = surf->polys->verts[index - indexbase][5] - ( surf->light_s - info->dlight_s ) * ( 1.0f / (float)BLOCK_SIZE );
				vbos.dlight_tc[index][1] = surf->polys->verts[index - indexbase][6] - ( surf->light_t - info->dlight_t ) * ( 1.0f / (float)BLOCK_SIZE );
			}

			if( vbos.dlight_vbo )
			{
				pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbos.dlight_vbo );
				pglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, sizeof( vec2_t ) * indexbase, sizeof( vec2_t )* surf->polys->numverts, vbos.dlight_tc + indexbase );
			}

			// if surface has decals, build decal array
			for( pdecal = surf->pdecals; pdecal; pdecal = pdecal->pnext )
			{
				int decalindex = pdecal - &gDecalPool[0];
				int numVerts = vbos.decaldata->decals[decalindex].numVerts;
				int i;

				if( numVerts == -1 )
				{
					// build decal array
					float *v = R_DecalSetupVerts( pdecal, surf, pdecal->texture, &numVerts );

					for( i = 0; i < numVerts; i++, v += VERTEXSIZE )
					{
						VectorCopy( v, vbos.decal_dlight[decalcount * DECAL_VERTS_MAX + i].pos );
						vbos.decal_dlight[decalcount * DECAL_VERTS_MAX + i].gl_tc[0] = v[3];
						vbos.decal_dlight[decalcount * DECAL_VERTS_MAX + i].gl_tc[1] = v[4];
						vbos.decal_dlight[decalcount * DECAL_VERTS_MAX + i].lm_tc[0] = v[5] - ( surf->light_s - info->dlight_s ) * ( 1.0f / (float)BLOCK_SIZE );
						vbos.decal_dlight[decalcount * DECAL_VERTS_MAX + i].lm_tc[1] = v[6] - ( surf->light_t - info->dlight_t ) * ( 1.0f / (float)BLOCK_SIZE );
					}
				}
				else
				{
					// copy from vbo
					for( i = 0; i < numVerts; i++ )
					{
						VectorCopy( vbos.decaldata->decalarray[decalindex * DECAL_VERTS_CUT + i].pos, vbos.decal_dlight[decalcount * DECAL_VERTS_MAX + i].pos );
						vbos.decal_dlight[decalcount * DECAL_VERTS_MAX + i].gl_tc[0] = vbos.decaldata->decalarray[decalindex * DECAL_VERTS_CUT + i].gl_tc[0];
						vbos.decal_dlight[decalcount * DECAL_VERTS_MAX + i].gl_tc[1] = vbos.decaldata->decalarray[decalindex * DECAL_VERTS_CUT + i].gl_tc[1];
						vbos.decal_dlight[decalcount * DECAL_VERTS_MAX + i].lm_tc[0] = vbos.decaldata->decalarray[decalindex * DECAL_VERTS_CUT + i].lm_tc[0] - ( surf->light_s - info->dlight_s ) * ( 1.0f / (float)BLOCK_SIZE );
						vbos.decal_dlight[decalcount * DECAL_VERTS_MAX + i].lm_tc[1] = vbos.decaldata->decalarray[decalindex * DECAL_VERTS_CUT + i].lm_tc[1] - ( surf->light_t - info->dlight_t ) * ( 1.0f / (float)BLOCK_SIZE );
					}
				}
				if( vbos.dlight_vbo )
				{
					pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbos.decal_dlight_vbo );
					pglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, sizeof( vbovertex_t ) * decalcount * DECAL_VERTS_MAX, sizeof( vbovertex_t )* numVerts, vbos.decal_dlight + decalcount * DECAL_VERTS_MAX );
				}

				vbos.decal_numverts[decalcount] = numVerts;
				decalcount++;
			}
			//info->dlight_s = info->dlight_t = 0;
		}

		if( dlightindex )
		{
			// update block
			LM_UploadDynamicBlock();

			// draw remaining array
#if !defined XASH_NANOGL || defined XASH_WES && defined __EMSCRIPTEN__ // WebGL need to know array sizes
			if( pglDrawRangeElements )
				pglDrawRangeElements( GL_TRIANGLES, 0, vbo->array_len, dlightindex, GL_UNSIGNED_SHORT, dlightarray );
			else
#endif
				pglDrawElements( GL_TRIANGLES, dlightindex, GL_UNSIGNED_SHORT, dlightarray );

			R_AdditionalPasses( vbo, dlightindex, dlightarray, texture, true );

			// draw remaining decals
			if( decalcount )
			{
				msurface_t *decalsurf;
				int decali = 0;

				pglDepthMask( GL_FALSE );
				pglEnable( GL_BLEND );
				pglEnable( GL_POLYGON_OFFSET_FILL );
				if( RI.currententity->curstate.rendermode == kRenderTransAlpha )
					pglDisable( GL_ALPHA_TEST );
				pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbos.decal_dlight_vbo );
				R_SetDecalMode( true );
				if( vbos.decal_dlight_vbo )
				{
					pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), offsetof( vbovertex_t, lm_tc ) );
					GL_SelectTexture( mtst.tmu_gl );
					pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), offsetof( vbovertex_t, gl_tc ) );
					pglVertexPointer( 3, GL_FLOAT, sizeof( vbovertex_t ), offsetof( vbovertex_t, pos ) );
				}
				else
				{
					pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), &vbos.decal_dlight[0].lm_tc );
					GL_SelectTexture( mtst.tmu_gl );
					pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), &vbos.decal_dlight[0].gl_tc );
					pglVertexPointer( 3, GL_FLOAT, sizeof( vbovertex_t ), &vbos.decal_dlight[0].pos);
				}
				for( decalsurf = newsurf; decali < decalcount && decalsurf; decalsurf = decalsurf->lightmapchain )
				{
					decal_t *pdecal;

					for( pdecal = decalsurf->pdecals; pdecal; pdecal = pdecal->pnext )
					{
						gltexture_t *glt;

						if( !pdecal->texture )
							continue;

						glt = R_GetTexture( pdecal->texture );

						GL_Bind( mtst.tmu_gl, pdecal->texture );

						// normal HL decal with alpha-channel
						if( glt->flags & TF_HAS_ALPHA )
						{
							// draw transparent decals with GL_MODULATE
							if( glt->fogParams[3] > 230 )
								pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
							else pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
							pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
						}
						else
						{
							// color decal like detail texture. Base color is 127 127 127
							pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
							pglBlendFunc( GL_DST_COLOR, GL_SRC_COLOR );
						}

						pglDrawArrays( GL_TRIANGLE_FAN, decali * DECAL_VERTS_MAX, vbos.decal_numverts[decali] );

						decali++;
					}

				}

				// reset states
				pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
				pglDepthMask( GL_TRUE );
				pglDisable( GL_BLEND );
				pglDisable( GL_POLYGON_OFFSET_FILL );
				if( RI.currententity->curstate.rendermode == kRenderTransAlpha )
					pglEnable( GL_ALPHA_TEST );
				pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->glindex );
				R_SetDecalMode( false );
				pglVertexPointer( 3, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t,pos) );
				R_SetupVBOTexture( texture, 0 );
				pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, gl_tc ) );
			}
		}

		// restore static lightmap
		pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->glindex );
		GL_Bind( mtst.tmu_lm, tr.lightmapTextures[lightmap] );
		pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, lm_tc ) );

		// prepare to next frame
		vbotex->dlightchain = NULL;
	}

	// prepare to next frame
	vbotex->curindex = 0;
}

/*
=====================
R_DrawVBO

Draw generated index arrays
=====================
*/
void R_DrawVBO( qboolean drawlightmap, qboolean drawtextures )
{
	int numtextures = cl.worldmodel->numtextures;
	int numlightmaps =  gl_lms.current_lightmap_texture;
	int k;
	vboarray_t *vbo = vbos.arraylist;

	if( !r_vbo->integer )
		return;

	// bind array
	pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->glindex );
	pglEnableClientState( GL_VERTEX_ARRAY );
	pglVertexPointer( 3, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t,pos) );

	// setup multitexture
	if( drawtextures )
	{
		GL_SelectTexture( mtst.tmu_gl = XASH_TEXTURE0 );
		pglEnable( GL_TEXTURE_2D );
		pglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		pglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
		pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, gl_tc ) );
	}

	if( drawlightmap )
	{
		// set lightmap texenv
		GL_SelectTexture( mtst.tmu_lm = XASH_TEXTURE1 );
		pglEnable( GL_TEXTURE_2D );
		pglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		R_SetLightmap();
	}

	mtst.skipbump = !drawtextures || !drawlightmap || RI.currententity->curstate.rendermode == kRenderTransAlpha ;
	mtst.skiptexture = !drawtextures;
	mtst.tmu_dt = glConfig.max_texture_units > 2? XASH_TEXTURE2:-1;

	// setup limits
	if( vbos.minlightmap > vbos.minarraysplit_lm )
		vbos.minlightmap = vbos.minarraysplit_lm;
	if( vbos.maxlightmap < vbos.maxarraysplit_lm )
		vbos.maxlightmap = vbos.maxarraysplit_lm;
	if( vbos.maxlightmap > numlightmaps )
		vbos.maxlightmap = numlightmaps;
	if( vbos.mintexture > vbos.minarraysplit_tex )
		vbos.mintexture = vbos.minarraysplit_tex;
	if( vbos.maxtexture < vbos.maxarraysplit_tex )
		vbos.maxtexture = vbos.maxarraysplit_tex;
	if( vbos.maxtexture > numtextures )
		vbos.maxtexture = numtextures;

	for( k = vbos.minlightmap; k < vbos.maxlightmap; k++ )
	{
		int j;
		msurface_t *lightmapchain;

		if( drawlightmap )
		{
			if( tr.deluxemap )
			{
				mtst.dl = tr.deluxemapTextures[k];
				if( mtst.bump_enabled )
					GL_Bind( XASH_TEXTURE0, tr.deluxemapTextures[k] );
			}

			GL_Bind( mtst.tmu_lm, mtst.lm = tr.lightmapTextures[k] );
		}

		for( j = vbos.mintexture; j < vbos.maxtexture; j++ )
		{
			vbotexture_t *vbotex = &vbos.textures[k * numtextures + j];
			texture_t *tex = NULL;
			if( !vbotex->vboarray )
				continue;

			ASSERT( vbotex->vboarray == vbo );

			if( vbotex->curindex || vbotex->dlightchain )
			{
				// draw textures static lightmap first
				if( drawtextures )
				{
					qboolean bump = mtst.bump_enabled;

					tex = R_SetupVBOTexture( NULL, j );

					// update texcoord if bump state changed
					if( bump != mtst.bump_enabled )
					{
						pglEnable( GL_TEXTURE_2D );
						pglEnable( GL_TEXTURE_COORD_ARRAY );
						pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, gl_tc ) );
					}
				}

				R_DrawLightmappedVBO( vbo, vbotex, tex, k, !drawlightmap );
			}

			// if we need to switch to next array (only if map has >65536 vertices)
			while( vbotex->next )
			{

				vbotex = vbotex->next;
				vbo = vbo->next;

				// bind new vertex and index arrays
				pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->glindex );
				pglVertexPointer( 3, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t,pos) );

				// update texcoord pointers
				if( drawtextures )
				{
					tex = R_SetupVBOTexture( tex, 0 );
					pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, gl_tc ) );
				}

				if( drawlightmap )
				{
					if( tr.deluxemap )
					{
						if( mtst.bump_enabled )
							GL_Bind( XASH_TEXTURE0, tr.deluxemapTextures[k] );
						mtst.dl = tr.deluxemapTextures[k];
					}

					GL_Bind( mtst.tmu_lm, tr.lightmapTextures[k] );
					pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, lm_tc ) );
				}

				// draw new array
				if( (vbotex->curindex || vbotex->dlightchain) )
					R_DrawLightmappedVBO( vbo, vbotex, tex, k, !drawlightmap );
			}
		}

		if( drawtextures && drawlightmap && vbos.decaldata->lm[k] )
		{
			// prepare for decal draw
			pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbos.decaldata->decalvbo );
			pglDepthMask( GL_FALSE );
			pglEnable( GL_BLEND );
			pglEnable( GL_POLYGON_OFFSET_FILL );

			if( RI.currententity->curstate.rendermode == kRenderTransAlpha )
				pglDisable( GL_ALPHA_TEST );

			R_SetDecalMode( true );

			// Set pointers to vbodecaldata->decalvbo
			if( drawlightmap )
			{
				GL_SelectTexture( mtst.tmu_lm );
				pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, lm_tc ) );
				GL_SelectTexture( mtst.tmu_gl );
				pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, gl_tc ) );
				pglVertexPointer( 3, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, pos ) );
			}

			// all surfaces having decals and this lightmap
			for( lightmapchain = vbos.decaldata->lm[k]; lightmapchain; lightmapchain = lightmapchain->lightmapchain )
			{
				decal_t *pdecal;

				// all decals of surface
				for( pdecal = lightmapchain->pdecals; pdecal; pdecal = pdecal->pnext )
				{
					gltexture_t *glt;
					int decalindex = pdecal - &gDecalPool[0];

					if( !pdecal->texture )
						continue;

					glt = R_GetTexture( pdecal->texture );

					GL_Bind( mtst.tmu_gl, pdecal->texture );

					// normal HL decal with alpha-channel
					if( glt->flags & TF_HAS_ALPHA )
					{
						// draw transparent decals with GL_MODULATE
						if( glt->fogParams[3] > 230 )
							pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
						else pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
						pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
					}
					else
					{
						// color decal like detail texture. Base color is 127 127 127
						pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
						pglBlendFunc( GL_DST_COLOR, GL_SRC_COLOR );
					}

					if( vbos.decaldata->decals[decalindex].numVerts == -1 )
					{
						int numVerts;
						float *v;

						v = R_DecalSetupVerts( pdecal, lightmapchain, pdecal->texture, &numVerts );

						// to many verts to keep in sparse array, so build it now
						pglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
						pglVertexPointer( 3, GL_FLOAT, VERTEXSIZE * 4, v );
						pglTexCoordPointer( 2, GL_FLOAT, VERTEXSIZE * 4, v + 3 );
						if( drawlightmap )
						{
							GL_SelectTexture( mtst.tmu_lm );
							pglTexCoordPointer( 2, GL_FLOAT, VERTEXSIZE * 4, v + 5 );
						}

						pglDrawArrays( GL_TRIANGLE_FAN, 0, numVerts );

						pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbos.decaldata->decalvbo );
						pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, lm_tc ) );
						GL_SelectTexture( mtst.tmu_gl );
						pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, gl_tc ) );
						pglVertexPointer( 3, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, pos ) );
					}
					else // just draw VBO
						pglDrawArrays( GL_TRIANGLE_FAN, decalindex * DECAL_VERTS_CUT, vbos.decaldata->decals[decalindex].numVerts );
				}
			}

			// prepare for next frame
			vbos.decaldata->lm[k] = NULL;

			// prepare for next texture
			pglDepthMask( GL_TRUE );
			pglDisable( GL_BLEND );
			pglDisable( GL_POLYGON_OFFSET_FILL );
		
			// restore vbo
			pglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->glindex );
			pglVertexPointer( 3, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t,pos) );

			// restore bump if needed
			R_SetDecalMode( false );

			// restore texture
			GL_SelectTexture( mtst.tmu_gl );
			pglEnableClientState( GL_TEXTURE_COORD_ARRAY );
			pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, gl_tc ) );

			// restore lightmap
			GL_SelectTexture( mtst.tmu_lm );
			pglTexCoordPointer( 2, GL_FLOAT, sizeof( vbovertex_t ), (void*)offsetof(vbovertex_t, lm_tc ) );

			if( RI.currententity->curstate.rendermode == kRenderTransAlpha )
				pglEnable( GL_ALPHA_TEST );
		}
		if( !drawtextures || !drawlightmap )
			vbos.decaldata->lm[k] = NULL;
	}
	ASSERT( !vbo->next );

	// restore states
	R_DisableBump();
	R_DisableDetail();

	if( drawlightmap )
	{
		// reset states
		GL_SelectTexture( XASH_TEXTURE1 );
		pglDisableClientState( GL_TEXTURE_COORD_ARRAY );
		pglDisable( GL_TEXTURE_2D );
		if( drawtextures )
		{
			GL_SelectTexture( XASH_TEXTURE0 );
			if( mtst.bump_enabled )
			{
				GL_SelectTexture( XASH_TEXTURE3 );
				pglDisable( GL_TEXTURE_2D );
				GL_SelectTexture( XASH_TEXTURE2 );
				pglDisable( GL_TEXTURE_2D );
			}
			GL_SelectTexture( XASH_TEXTURE0 );
			pglEnable( GL_TEXTURE_2D );
		}
	}

	if( drawtextures )
		pglDisableClientState( GL_TEXTURE_COORD_ARRAY );

	mtst.bump_enabled = mtst.details_enabled = false;

	vbos.minlightmap = MAX_LIGHTMAPS;
	vbos.maxlightmap = 0;
	vbos.mintexture = INT_MAX;
	vbos.maxtexture = 0;

	pglDisableClientState( GL_VERTEX_ARRAY );
	pglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
}

/*
================
R_CheckLightMap

update surface's lightmap if needed and return true if it is dynamic
================
*/
static qboolean R_CheckLightMap( msurface_t *fa )
{
	int maps;
	qboolean is_dynamic = false;

	// check for lightmap modification
	for( maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++ )
	{
		if( RI.lightstylevalue[fa->styles[maps]] != fa->cached_light[maps] )
		{
			is_dynamic = true;
			break;
		}
	}

	// already up to date
	if( !is_dynamic && ( fa->dlightframe != tr.framecount || maps == MAX_LIGHTMAPS ) )
		return false;

	// build lightmap
	if(( fa->styles[maps] >= 32 || fa->styles[maps] == 0 ) && ( fa->dlightframe != tr.framecount ))
	{
		byte	temp[132*132*4];
		int	smax, tmax;

		smax = ( fa->extents[0] / LM_SAMPLE_SIZE ) + 1;
		tmax = ( fa->extents[1] / LM_SAMPLE_SIZE ) + 1;

		if( smax < 132 && tmax < 132 )
		{
			if( tr.deluxemap )
			{
				R_BuildDeluxeMap( fa, temp, smax * 4 );

				GL_Bind( XASH_TEXTURE0, tr.deluxemapTextures[fa->lightmaptexturenum] );
#ifdef XASH_WES
				pglTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
#endif

				pglTexSubImage2D( GL_TEXTURE_2D, 0, fa->light_s, fa->light_t, smax, tmax,
				GL_RGBA, GL_UNSIGNED_BYTE, temp );
			}

			R_BuildLightMap( fa, temp, smax * 4, true );
		}
		else
		{
			smax = min( smax, 132 );
			tmax = min( tmax, 132 );
			Host_MapDesignError( "R_RenderBrushPoly: bad surface extents: %d %d", fa->extents[0], fa->extents[1] );
			Q_memset( temp, 255, sizeof( temp ) );
		}

		R_SetCacheState( fa );
#ifdef XASH_WES
		GL_Bind( XASH_TEXTURE1, tr.lightmapTextures[fa->lightmaptexturenum] );

		pglTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
#else
		GL_Bind( XASH_TEXTURE0, tr.lightmapTextures[fa->lightmaptexturenum] );
#endif

		pglTexSubImage2D( GL_TEXTURE_2D, 0, fa->light_s, fa->light_t, smax, tmax,
		GL_RGBA, GL_UNSIGNED_BYTE, temp );
#ifdef XASH_WES
		GL_SelectTexture( XASH_TEXTURE0 );
#endif
	}
	// add to dynamic chain
	else
		return true;

	// updated
	return false;
}

qboolean R_AddSurfToVBO( msurface_t *surf, qboolean buildlightmap )
{
	if( r_vbo->integer && vbos.surfdata[surf - cl.worldmodel->surfaces].vbotexture )
	{
		// find vbotexture_t assotiated with this surface
		int idx = surf - cl.worldmodel->surfaces;
		vbotexture_t *vbotex = vbos.surfdata[idx].vbotexture;
		int texturenum = vbos.surfdata[idx].texturenum;

		if( !surf->polys )
			return true;

		if( vbos.maxlightmap < surf->lightmaptexturenum + 1 )
			vbos.maxlightmap = surf->lightmaptexturenum + 1;
		if( vbos.minlightmap > surf->lightmaptexturenum )
			vbos.minlightmap = surf->lightmaptexturenum;
		if( vbos.maxtexture < texturenum + 1 )
			vbos.maxtexture = texturenum + 1;
		if( vbos.mintexture > texturenum )
			vbos.mintexture = texturenum;

		buildlightmap &= !r_fullbright->integer && !!cl.worldmodel->lightdata;

		if( buildlightmap && R_CheckLightMap( surf ) )
		{
			// every vbotex has own lightmap chain (as we sorted if by textures to use multitexture)
			surf->lightmapchain = vbotex->dlightchain;
			vbotex->dlightchain = surf;
		}
		else
		{
			uint indexbase = vbos.surfdata[idx].startindex;
			uint index;

			// GL_TRIANGLE_FAN: 0 1 2 0 2 3 0 3 4 ...
			for( index = indexbase + 2; index < indexbase + surf->polys->numverts; index++ )
			{
				vbotex->indexarray[vbotex->curindex++] = indexbase;
				vbotex->indexarray[vbotex->curindex++] = index - 1;
				vbotex->indexarray[vbotex->curindex++] = index;
			}

			// if surface has decals, add it to decal lightmapchain
			if( surf->pdecals )
			{
				surf->lightmapchain = vbos.decaldata->lm[vbotex->lightmaptexturenum];
				vbos.decaldata->lm[vbotex->lightmaptexturenum] = surf;
			}
		}
		return true;
	}
	return false;
}


/*
=============================================================

	WORLD MODEL

=============================================================
*/
/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode( mnode_t *node, uint clipflags )
{
	const mplane_t	*clipplane;
	int		i, clipped;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	int		c, side;
	float		dot;

	if( node->contents == CONTENTS_SOLID )
		return; // hit a solid leaf

	if( node->visframe != tr.visframecount )
		return;

	if( clipflags )
	{
		for( i = 0, clipplane = RI.frustum; i < 6; i++, clipplane++ )
		{
			if(!( clipflags & ( 1U << i )))
				continue;

			clipped = BoxOnPlaneSide( node->minmaxs, node->minmaxs + 3, clipplane );
			if( clipped == 2 ) return;
			if( clipped == 1 ) clipflags &= ~(1U << i);
		}
	}

	// if a leaf node, draw stuff
	if( node->contents < 0 )
	{
		pleaf = (mleaf_t *)node;

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if( c )
		{
			do
			{
				(*mark)->visframe = tr.framecount;
				mark++;
			} while( --c );
		}

		// deal with model fragments in this leaf
		if( pleaf->efrags )
			R_StoreEfrags( &pleaf->efrags, tr.framecount );

		r_stats.c_world_leafs++;
		return;
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	dot = PlaneDiff( tr.modelorg, node->plane );
	side = (dot >= 0.0f) ? 0 : 1;

	// recurse down the children, front side first
	R_RecursiveWorldNode( node->children[side], clipflags );

	// draw stuff
	for( c = node->numsurfaces, surf = cl.worldmodel->surfaces + node->firstsurface; c; c--, surf++ )
	{
		if( R_CullSurface( surf, clipflags ))
			continue;

		if( surf->flags & SURF_DRAWSKY && !world.sky_sphere )
		{
			// make sky chain to right clip the skybox
			surf->texturechain = skychain;
			skychain = surf;
		}
		else if( !R_AddSurfToVBO( surf, true ) )
		{
			surf->texturechain = surf->texinfo->texture->texturechain;
			surf->texinfo->texture->texturechain = surf;
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode( node->children[!side], clipflags );
}

/*
================
R_CullNodeTopView

cull node by user rectangle (simple scissor)
================
*/
qboolean R_CullNodeTopView( mnode_t *node )
{
	vec2_t	delta, size;
	vec3_t	center, half;

	// build the node center and half-diagonal
	VectorAverage( node->minmaxs, node->minmaxs + 3, center );
	VectorSubtract( node->minmaxs + 3, center, half );

	// cull against the screen frustum or the appropriate area's frustum.
	Vector2Subtract( center, world_orthocenter, delta );
	Vector2Add( half, world_orthohalf, size );

	return ( fabs( delta[0] ) > size[0] ) || ( fabs( delta[1] ) > size[1] );
}

/*
================
R_DrawTopViewLeaf
================
*/
static void R_DrawTopViewLeaf( mleaf_t *pleaf, uint clipflags )
{
	msurface_t	**mark, *surf;
	int		i;

	for( i = 0, mark = pleaf->firstmarksurface; i < pleaf->nummarksurfaces; i++, mark++ )
	{
		surf = *mark;

		// don't process the same surface twice
		if( surf->visframe == tr.framecount )
			continue;

		surf->visframe = tr.framecount;

		if( R_CullSurface( surf, clipflags ))
			continue;

		if(!( surf->flags & SURF_DRAWSKY ))
		{ 
			surf->texturechain = surf->texinfo->texture->texturechain;
			surf->texinfo->texture->texturechain = surf;
		}
	}

	// deal with model fragments in this leaf
	if( pleaf->efrags )
		R_StoreEfrags( &pleaf->efrags, tr.framecount );

	r_stats.c_world_leafs++;
}

/*
================
R_DrawWorldTopView
================
*/
void R_DrawWorldTopView( mnode_t *node, uint clipflags )
{
	const mplane_t	*clipplane;
	int		c, clipped;
	msurface_t	*surf;

	do
	{
		if( node->contents == CONTENTS_SOLID )
			return;	// hit a solid leaf

		if( node->visframe != tr.visframecount )
			return;

		if( clipflags )
		{
			for( c = 0, clipplane = RI.frustum; c < 6; c++, clipplane++ )
			{
				if(!( clipflags & ( 1U << c )))
					continue;

				clipped = BoxOnPlaneSide( node->minmaxs, node->minmaxs + 3, clipplane );
				if( clipped == 2 ) return;
				if( clipped == 1 ) clipflags &= ~(1U << c);
			}
		}

		// cull against the screen frustum or the appropriate area's frustum.
		if( R_CullNodeTopView( node ))
			return;

		// if a leaf node, draw stuff
		if( node->contents < 0 )
		{
			R_DrawTopViewLeaf( (mleaf_t *)node, clipflags );
			return;
		}

		// draw stuff
		for( c = node->numsurfaces, surf = cl.worldmodel->surfaces + node->firstsurface; c; c--, surf++ )
		{
			// don't process the same surface twice
			if( surf->visframe == tr.framecount )
				continue;

			surf->visframe = tr.framecount;

			if( R_CullSurface( surf, clipflags ))
				continue;

			if(!( surf->flags & SURF_DRAWSKY ))
			{ 
				surf->texturechain = surf->texinfo->texture->texturechain;
				surf->texinfo->texture->texturechain = surf;
			}
		}

		// recurse down both children, we don't care the order...
		R_DrawWorldTopView( node->children[0], clipflags );
		node = node->children[1];

	} while( node );
}

/*
=============
R_DrawTriangleOutlines
=============
*/
void R_DrawTriangleOutlines( void )
{
	int		i, j;
	msurface_t	*surf;
	glpoly_t		*p;
	float		*v;
		
	if( !gl_wireframe->integer )
		return;

	pglDisable( GL_TEXTURE_2D );
	pglDisable( GL_DEPTH_TEST );
	pglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	pglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	// render static surfaces first
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		for( surf = gl_lms.lightmap_surfaces[i]; surf != NULL; surf = surf->lightmapchain )
		{
			p = surf->polys;
			for( ; p != NULL; p = p->chain )
			{
				pglBegin( GL_POLYGON );
				v = p->verts[0];
				for( j = 0; j < p->numverts; j++, v += VERTEXSIZE )
					pglVertex3fv( v );
				pglEnd ();
			}
		}
	}

	// render surfaces with dynamic lightmaps
	for( surf = gl_lms.dynamic_surfaces; surf != NULL; surf = surf->lightmapchain )
	{
		p = surf->polys;

		for( ; p != NULL; p = p->chain )
		{
			pglBegin( GL_POLYGON );
			v = p->verts[0];
			for( j = 0; j < p->numverts; j++, v += VERTEXSIZE )
				pglVertex3fv( v );
			pglEnd ();
		}
	}

	pglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	pglEnable( GL_DEPTH_TEST );
	pglEnable( GL_TEXTURE_2D );
}

/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld( void )
{
	// paranoia issues: when gl_renderer is "0" we need have something valid for currententity
	// to prevent crashing until HeadShield drawing.
	RI.currententity = clgame.entities;
	RI.currentmodel = RI.currententity->model;

	if( !RI.drawWorld || RI.refdef.onlyClientDraw )
		return;

	VectorCopy( RI.cullorigin, tr.modelorg );
	Q_memset( gl_lms.lightmap_surfaces, 0, sizeof( gl_lms.lightmap_surfaces ));
	Q_memset( fullbright_polys, 0, sizeof( fullbright_polys ));
	Q_memset( detail_surfaces, 0, sizeof( detail_surfaces ));

	RI.currentWaveHeight = RI.waveHeight;
	GL_SetRenderMode( kRenderNormal );
	gl_lms.dynamic_surfaces = NULL;

	R_ClearSkyBox ();

	// draw the world fog
	R_DrawFog ();

	if( RI.drawOrtho )
	{
		R_DrawWorldTopView( cl.worldmodel->nodes, RI.clipFlags );
	}
	else
	{
		R_RecursiveWorldNode( cl.worldmodel->nodes, RI.clipFlags );
	}

	R_DrawStaticBrushes();

	R_DrawVBO( !r_fullbright->integer && !!cl.worldmodel->lightdata, true );

	R_DrawTextureChains();


	R_BlendLightmaps();

	R_RenderFullbrights();
	R_RenderDetails();

	if( skychain )
		R_DrawSkyBox();
	skychain = NULL;

	R_DrawTriangleOutlines ();
}

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current leaf
===============
*/
void R_MarkLeaves( void )
{
	byte	*vis;
	mnode_t	*node;
	int	i;

	if( !RI.drawWorld ) return;

	if( r_novis->modified || tr.fResetVis )
	{
		// force recalc viewleaf
		r_novis->modified = false;
		tr.fResetVis = false;
		r_viewleaf = NULL;
	}

	if( r_viewleaf == r_oldviewleaf && r_viewleaf2 == r_oldviewleaf2 && !r_novis->integer && r_viewleaf != NULL )
		return;

	// development aid to let you run around
	// and see exactly where the pvs ends
	if( r_lockpvs->integer ) return;

	tr.visframecount++;
	r_oldviewleaf = r_viewleaf;
	r_oldviewleaf2 = r_viewleaf2;
		
	if( r_novis->integer || RI.drawOrtho || !r_viewleaf || !cl.worldmodel->visdata )
	{
		// mark everything
		for( i = 0; i < cl.worldmodel->numleafs; i++ )
			cl.worldmodel->leafs[i+1].visframe = tr.visframecount;
		for( i = 0; i < cl.worldmodel->numnodes; i++ )
			cl.worldmodel->nodes[i].visframe = tr.visframecount;
		return;
	}

	// may have to combine two clusters
	// because of solid water boundaries
	vis = Mod_LeafPVS( r_viewleaf, cl.worldmodel );

	if( r_viewleaf != r_viewleaf2 )
	{
		int	longs = ( cl.worldmodel->numleafs + 31 ) >> 5;

		Q_memcpy( visbytes, vis, longs << 2 );
		vis = Mod_LeafPVS( r_viewleaf2, cl.worldmodel );

		for( i = 0; i < longs; i++ )
			((int *)visbytes)[i] |= ((int *)vis)[i];

		vis = visbytes;
	}

	for( i = 0; i < cl.worldmodel->numleafs; i++ )
	{
		if( vis[i>>3] & ( 1U << ( i & 7 )))
		{
			node = (mnode_t *)&cl.worldmodel->leafs[i+1];
			do
			{
				if( node->visframe == tr.visframecount )
					break;
				node->visframe = tr.visframecount;
				node = node->parent;
			} while( node );
		}
	}
}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
void GL_CreateSurfaceLightmap( msurface_t *surf )
{
	int	smax, tmax;
	byte	*base;

	if( !cl.worldmodel->lightdata ) return;
	if( surf->flags & SURF_DRAWTILED )
		return;

	smax = ( surf->extents[0] / LM_SAMPLE_SIZE ) + 1;
	tmax = ( surf->extents[1] / LM_SAMPLE_SIZE ) + 1;

	if( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ))
	{
		LM_UploadBlock( false );
		LM_InitBlock();

		if( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ))
			Host_Error( "AllocBlock: full\n" );
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	if( tr.deluxemap )
	{
		base = gl_lms.deluxemap_buffer;
		base += ( surf->light_t * BLOCK_SIZE + surf->light_s ) * 4;

		R_BuildDeluxeMap( surf, base, BLOCK_SIZE * 4 );
	}


	base = gl_lms.lightmap_buffer;
	base += ( surf->light_t * BLOCK_SIZE + surf->light_s ) * 4;

	R_SetCacheState( surf );
	R_BuildLightMap( surf, base, BLOCK_SIZE * 4, false );

	// moved here in case we need valid lightmap coords
	if( host.features & ENGINE_BUILD_SURFMESHES )
		Mod_BuildSurfacePolygons( surf, SURF_INFO( surf, loadmodel ));
}


/*
==================
R_LoadDeluxeMap
==================
*/
void R_LoadDeluxeMap( void )
{
	int compare;
	fs_offset_t filesize;
	char file[80];
	byte *deluxemap;

	Q_strncpy( file, cl.worldmodel->name, 80 );
	FS_StripExtension( file );
	FS_DefaultExtension( file, ".dlit" );

	tr.deluxemap = NULL;

	if( !COM_CompareFileTime( file, cl.worldmodel->name, &compare ) )
		return;

	if( compare < 0 )
	{
		MsgDev( D_NOTE, "R_LoadDeluxeMap(): %s  is out of date!\n", file );
		return;
	}

	deluxemap = FS_LoadFile( file, &filesize, true );

	if( !deluxemap )
	{
		MsgDev( D_NOTE, "R_LoadDeluxeMap(): %s not found.\n", file );
		return;
	}

	if( deluxemap[0] != 'Q' || deluxemap[1] != 'L' ||
		deluxemap[2] != 'I' || deluxemap[3] != 'T' ||
		*(int *)( deluxemap + 4 ) != 1 )
	{
		MsgDev( D_NOTE, "R_LoadDeluxeMap(): %s is invalid file.\n", file );
		COM_FreeFile( deluxemap );
		return;
	}

	tr.deluxemap = (color24 *)Mem_Alloc( cl.worldmodel->mempool, filesize - 8 );
	Q_memcpy( tr.deluxemap, deluxemap + 8, filesize - 8 );
	Mem_Free( deluxemap );

	MsgDev( D_NOTE, "R_LoadDeluxeMap(): %s loaded succesfully.\n", file );
}

/*
==================
GL_RebuildLightmaps

Rebuilds the lightmap texture
when gamma is changed
==================
*/
void GL_RebuildLightmaps( void )
{
	int	i, j;
	model_t	*m;

	if( !cl.world ) return;	// wait for worldmodel
	vid_gamma->modified = false;

	// release old lightmaps
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		if( !tr.lightmapTextures[i] ) break;
		GL_FreeTexture( tr.lightmapTextures[i] );
	}
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		if( !tr.deluxemapTextures[i] ) break;
		GL_FreeTexture( tr.deluxemapTextures[i] );
	}

	Q_memset( tr.lightmapTextures, 0, sizeof( tr.lightmapTextures ));
	Q_memset( tr.deluxemapTextures, 0, sizeof( tr.deluxemapTextures ));
	gl_lms.current_lightmap_texture = 0;

	// setup all the lightstyles
	R_AnimateLight();

	LM_InitBlock();	

	for( i = 1; i < MAX_MODELS; i++ )
	{
		if(( m = Mod_Handle( i )) == NULL )
			continue;

		if( m->name[0] == '*' || m->type != mod_brush )
			continue;

		loadmodel = m;

		for( j = 0; j < m->numsurfaces; j++ )
			GL_CreateSurfaceLightmap( m->surfaces + j );
	}
	LM_UploadBlock( false );

	if( clgame.drawFuncs.GL_BuildLightmaps )
	{
		// build lightmaps on the client-side
		clgame.drawFuncs.GL_BuildLightmaps( );
	}
}

/*
==================
GL_BuildLightmaps

Builds the lightmap texture
with all the surfaces from all brush models
==================
*/
void GL_BuildLightmaps( void )
{
	int	i, j;
	model_t	*m;

	// release old lightmaps
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		if( !tr.lightmapTextures[i] ) break;
		GL_FreeTexture( tr.lightmapTextures[i] );
	}

	// release old mirror textures
	for( i = 0; i < MAX_MIRRORS; i++ )
	{
		if( !tr.mirrorTextures[i] ) break;
		GL_FreeTexture( tr.mirrorTextures[i] );
	}

	// release old deluxemap textures
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		if( !tr.deluxemapTextures[i] ) break;
		GL_FreeTexture( tr.deluxemapTextures[i] );
	}

	Q_memset( tr.lightmapTextures, 0, sizeof( tr.lightmapTextures ));
	Q_memset( tr.deluxemapTextures, 0, sizeof( tr.deluxemapTextures ));
	Q_memset( tr.mirror_entities, 0, sizeof( tr.mirror_entities ));
	Q_memset( tr.mirrorTextures, 0, sizeof( tr.mirrorTextures ));
	Q_memset( visbytes, 0x00, sizeof( visbytes ));
	
	skychain = NULL;

	tr.framecount = tr.visframecount = 1;	// no dlight cache
	gl_lms.current_lightmap_texture = 0;
	tr.num_mirror_entities = 0;
	tr.num_mirrors_used = 0;
	nColinElim = 0;
	R_LoadDeluxeMap();

	// setup all the lightstyles
	R_AnimateLight();

	LM_InitBlock();


	for( i = 1; i < MAX_MODELS; i++ )
	{
		if(( m = Mod_Handle( i )) == NULL )
			continue;

		if( m->name[0] == '*' || m->type != mod_brush )
			continue;

		for( j = 0; j < m->numsurfaces; j++ )
		{
			// clearing all decal chains
			m->surfaces[j].pdecals = NULL;
			m->surfaces[j].visframe = 0;
			loadmodel = m;

			GL_CreateSurfaceLightmap( m->surfaces + j );

			if( m->surfaces[j].flags & SURF_DRAWTURB )
				continue;

			if( m->surfaces[j].flags & SURF_DRAWSKY && world.sky_sphere )
				continue;

			GL_BuildPolygonFromSurface( m, m->surfaces + j );
		}

		// clearing visframe
		for( j = 0; j < m->numleafs; j++ )
			m->leafs[j+1].visframe = 0;
		for( j = 0; j < m->numnodes; j++ )
			m->nodes[j].visframe = 0;
	}

	LM_UploadBlock( false );

	if( clgame.drawFuncs.GL_BuildLightmaps )
	{
		// build lightmaps on the client-side
		clgame.drawFuncs.GL_BuildLightmaps( );
	}

	if( !gl_keeptjunctions->integer )
		MsgDev( D_INFO, "Eliminated %i vertices\n", nColinElim );
}
#endif // XASH_DEDICATED
