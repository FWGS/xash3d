/*
gl_decals.c - decal paste and rendering
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
#include "cl_tent.h"

#define DECAL_OVERLAP_DISTANCE	2
#define DECAL_DISTANCE		4	// too big values produce more clipped polygons
#define MAX_DECALCLIPVERT		32	// produced vertexes of fragmented decal
#define DECAL_CACHEENTRY		256	// MUST BE POWER OF 2 or code below needs to change!
#define DECAL_TRANSPARENT_THRESHOLD	230	// transparent decals draw with GL_MODULATE

// empirically determined constants for minimizing overalpping decals
#define MAX_OVERLAP_DECALS		6
#define DECAL_OVERLAP_DIST		8
#define MIN_DECAL_SCALE		0.01f
#define MAX_DECAL_SCALE		16.0f

// clip edges
#define LEFT_EDGE			0
#define RIGHT_EDGE			1
#define TOP_EDGE			2
#define BOTTOM_EDGE			3
		
// This structure contains the information used to create new decals
typedef struct
{
	vec3_t		m_Position;	// world coordinates of the decal center
	vec3_t		m_SAxis;		// the s axis for the decal in world coordinates
	model_t		*m_pModel;	// the model the decal is going to be applied in
	int		m_iTexture;	// The decal material
	int		m_Size;		// Size of the decal (in world coords)
	int		m_Flags;
	int		m_Entity;		// Entity the decal is applied to.
	float		m_scale;
	int		m_decalWidth;
	int		m_decalHeight;
	vec3_t		m_Basis[3];
} decalinfo_t;

static float	g_DecalClipVerts[MAX_DECALCLIPVERT][VERTEXSIZE];
static float	g_DecalClipVerts2[MAX_DECALCLIPVERT][VERTEXSIZE];

static decal_t	gDecalPool[MAX_RENDER_DECALS];
static int	gDecalCount;

void R_ClearDecals( void )
{
	Q_memset( gDecalPool, 0, sizeof( gDecalPool ));
	gDecalCount = 0;
}

// unlink pdecal from any surface it's attached to
static void R_DecalUnlink( decal_t *pdecal )
{
	decal_t	*tmp;

	if( pdecal->psurface )
	{
		if( pdecal->psurface->pdecals == pdecal )
		{
			pdecal->psurface->pdecals = pdecal->pnext;
		}
		else 
		{
			tmp = pdecal->psurface->pdecals;
			if( !tmp ) 
				Host_Error( "D_DecalUnlink: bad decal list\n" );
			else
			{
				while( tmp->pnext ) 
				{
					if( tmp->pnext == pdecal ) 
					{
						tmp->pnext = pdecal->pnext;
						break;
					}
					tmp = tmp->pnext;
				}
			}
		}
	}

	if( pdecal->mesh )
	{
		Mem_Free( pdecal->mesh );
	}

	pdecal->psurface = NULL;
	pdecal->mesh = NULL;
}

// Just reuse next decal in list
// A decal that spans multiple surfaces will use multiple decal_t pool entries,
// as each surface needs it's own.
static decal_t *R_DecalAlloc( decal_t *pdecal )
{
	int	limit = MAX_RENDER_DECALS;

	if( r_decals->integer < limit )
		limit = r_decals->integer;
	
	if( !limit ) return NULL;

	if( !pdecal ) 
	{
		int	count = 0;

		// check for the odd possiblity of infinte loop
		do 
		{
			if( gDecalCount >= limit )
				gDecalCount = 0;

			pdecal = &gDecalPool[gDecalCount]; // reuse next decal
			gDecalCount++;
			count++;
		} while(( pdecal->flags & FDECAL_PERMANENT ) && count < limit );
	}
	
	// if decal is already linked to a surface, unlink it.
	R_DecalUnlink( pdecal );

	return pdecal;	
}

//-----------------------------------------------------------------------------
// find decal image and grab size from it
//-----------------------------------------------------------------------------
static void R_GetDecalDimensions( int texture, int *width, int *height )
{
	if( width ) *width = 1;	// to avoid divide by zero
	if( height ) *height = 1;

	R_GetTextureParms( width, height, texture );
}

//-----------------------------------------------------------------------------
// compute the decal basis based on surface normal, and preferred saxis
//-----------------------------------------------------------------------------
void R_DecalComputeBasis( msurface_t *surf, vec3_t pSAxis, vec3_t textureSpaceBasis[3] )
{
	vec3_t	surfaceNormal;

	// setup normal
	if( surf->flags & SURF_PLANEBACK )
		VectorNegate( surf->plane->normal, surfaceNormal );
	else VectorCopy( surf->plane->normal, surfaceNormal );

	VectorCopy( surfaceNormal, textureSpaceBasis[2] );

	if( pSAxis )
	{
		// T = S cross N
		CrossProduct( pSAxis, textureSpaceBasis[2], textureSpaceBasis[1] );

		// Name sure they aren't parallel or antiparallel
		// In that case, fall back to the normal algorithm.
		if( DotProduct( textureSpaceBasis[1], textureSpaceBasis[1] ) > 1e-6 )
		{
			// S = N cross T
			CrossProduct( textureSpaceBasis[2], textureSpaceBasis[1], textureSpaceBasis[0] );

			VectorNormalizeFast( textureSpaceBasis[0] );
			VectorNormalizeFast( textureSpaceBasis[1] );
			return;
		}
		// Fall through to the standard algorithm for parallel or antiparallel
	}

	// original Half-Life algorithm: get textureBasis from linked surface
	VectorCopy( surf->texinfo->vecs[0], textureSpaceBasis[0] );
	VectorCopy( surf->texinfo->vecs[1], textureSpaceBasis[1] );
	VectorNormalizeFast( textureSpaceBasis[0] );
	VectorNormalizeFast( textureSpaceBasis[1] );
}

void R_SetupDecalTextureSpaceBasis( decal_t *pDecal, msurface_t *surf, int texture, vec3_t textureSpaceBasis[3], float decalWorldScale[2] )
{
	float	*sAxis = NULL;
	int	width, height;

	if( pDecal->flags & FDECAL_USESAXIS )
		sAxis = pDecal->saxis;

	// Compute the non-scaled decal basis
	R_DecalComputeBasis( surf, sAxis, textureSpaceBasis );
	R_GetDecalDimensions( texture, &width, &height );

	// world width of decal = ptexture->width / pDecal->scale
	// world height of decal = ptexture->height / pDecal->scale
	// scale is inverse, scales world space to decal u/v space [0,1]
	// OPTIMIZE: Get rid of these divides
	decalWorldScale[0] = (float)pDecal->scale / width;
	decalWorldScale[1] = (float)pDecal->scale / height;
	
	VectorScale( textureSpaceBasis[0], decalWorldScale[0], textureSpaceBasis[0] );
	VectorScale( textureSpaceBasis[1], decalWorldScale[1], textureSpaceBasis[1] );
}

// Build the initial list of vertices from the surface verts into the global array, 'verts'.
void R_SetupDecalVertsForMSurface( decal_t *pDecal, msurface_t *surf,	vec3_t textureSpaceBasis[3], float *verts )
{
	float	*v;
	int	i;

	for( i = 0, v = surf->polys->verts[0]; i < surf->polys->numverts; i++, v += VERTEXSIZE, verts += VERTEXSIZE )
	{
		VectorCopy( v, verts ); // copy model space coordinates
		verts[3] = DotProduct( verts, textureSpaceBasis[0] ) - pDecal->dx + 0.5f;
		verts[4] = DotProduct( verts, textureSpaceBasis[1] ) - pDecal->dy + 0.5f;
		verts[5] = verts[6] = 0.0f;
	}
}

// Figure out where the decal maps onto the surface.
void R_SetupDecalClip( decal_t *pDecal, msurface_t *surf, int texture, vec3_t textureSpaceBasis[3], float decalWorldScale[2] )
{
	R_SetupDecalTextureSpaceBasis( pDecal, surf, texture, textureSpaceBasis, decalWorldScale );

	// Generate texture coordinates for each vertex in decal s,t space
	// probably should pre-generate this, store it and use it for decal-decal collisions
	// as in R_DecalsIntersect()
	pDecal->dx = DotProduct( pDecal->position, textureSpaceBasis[0] );
	pDecal->dy = DotProduct( pDecal->position, textureSpaceBasis[1] );
}

// Quick and dirty sutherland Hodgman clipper
// Clip polygon to decal in texture space
// JAY: This code is lame, change it later.  It does way too much work per frame
// It can be made to recursively call the clipping code and only copy the vertex list once
int R_ClipInside( float *vert, int edge )
{
	switch( edge )
	{
	case LEFT_EDGE:
		if( vert[3] > 0.0f )
			return 1;
		return 0;
	case RIGHT_EDGE:
		if( vert[3] < 1.0f )
			return 1;
		return 0;
	case TOP_EDGE:
		if( vert[4] > 0.0f )
			return 1;
		return 0;
	case BOTTOM_EDGE:
		if( vert[4] < 1.0f )
			return 1;
		return 0;
	}
	return 0;
}

void R_ClipIntersect( float *one, float *two, float *out, int edge )
{
	float	t;

	// t is the parameter of the line between one and two clipped to the edge
	// or the fraction of the clipped point between one & two
	// vert[0], vert[1], vert[2] is X, Y, Z
	// vert[3] is u
	// vert[4] is v
	// vert[5] is lightmap u
	// vert[6] is lightmap v

	if( edge < TOP_EDGE )
	{
		if( edge == LEFT_EDGE )
		{
			// left
			t = ((one[3] - 0.0f) / (one[3] - two[3]));
			out[3] = out[5] = 0.0f;
		}
		else
		{	
			// right
			t = ((one[3] - 1.0f) / (one[3] - two[3]));
			out[3] = out[5] = 1.0f;
		}

		out[4] = one[4] + (two[4] - one[4]) * t;
		out[6] = one[6] + (two[6] - one[6]) * t;
	}
	else
	{
		if( edge == TOP_EDGE )
		{
			// top
			t = ((one[4] - 0.0f)  / (one[4] - two[4]));
			out[4] = out[6] = 0.0f;
		}
		else
		{	
			// bottom
			t = ((one[4] - 1.0f) / (one[4] - two[4]));
			out[4] = out[6] = 1.0f;
		}

		out[3] = one[3] + (two[3] - one[3]) * t;
		out[5] = one[5] + (two[4] - one[5]) * t;
	}

	VectorLerp( one, t, two, out );
}

static int SHClip( float *vert, int vertCount, float *out, int edge )
{
	int	j, outCount;
	float	*s, *p;

	outCount = 0;

	s = &vert[(vertCount - 1) * VERTEXSIZE];

	for( j = 0; j < vertCount; j++ )
	{
		p = &vert[j * VERTEXSIZE];

		if( R_ClipInside( p, edge ))
		{
			if( R_ClipInside( s, edge ))
			{
				// Add a vertex and advance out to next vertex
				Q_memcpy( out, p, sizeof( float ) * VERTEXSIZE );
				out += VERTEXSIZE;
				outCount++;
			}
			else
			{
				R_ClipIntersect( s, p, out, edge );
				out += VERTEXSIZE;
				outCount++;

				Q_memcpy( out, p, sizeof( float ) * VERTEXSIZE );
				out += VERTEXSIZE;
				outCount++;
			}
		}
		else
		{
			if( R_ClipInside( s, edge ))
			{
				R_ClipIntersect( p, s, out, edge );
				out += VERTEXSIZE;
				outCount++;
			}
		}

		s = p;
	}

	return outCount;
}

float *R_DoDecalSHClip( float *pInVerts, decal_t *pDecal, int nStartVerts, int *pVertCount )
{
	float	*pOutVerts = g_DecalClipVerts[0];
	int	outCount;

	// clip the polygon to the decal texture space
	outCount = SHClip( pInVerts, nStartVerts, g_DecalClipVerts2[0], LEFT_EDGE );
	outCount = SHClip( g_DecalClipVerts2[0], outCount, g_DecalClipVerts[0], RIGHT_EDGE );
	outCount = SHClip( g_DecalClipVerts[0], outCount, g_DecalClipVerts2[0], TOP_EDGE );
	outCount = SHClip( g_DecalClipVerts2[0], outCount, pOutVerts, BOTTOM_EDGE );

	if( pVertCount )
		*pVertCount = outCount;

	return pOutVerts;
}

//-----------------------------------------------------------------------------
// Generate clipped vertex list for decal pdecal projected onto polygon psurf
//-----------------------------------------------------------------------------
float *R_DecalVertsClip( decal_t *pDecal, msurface_t *surf, int texture, int *pVertCount )
{
	float	decalWorldScale[2];
	vec3_t	textureSpaceBasis[3]; 

	// figure out where the decal maps onto the surface.
	R_SetupDecalClip( pDecal, surf, texture, textureSpaceBasis, decalWorldScale );

	// build the initial list of vertices from the surface verts.
	R_SetupDecalVertsForMSurface( pDecal, surf, textureSpaceBasis, g_DecalClipVerts[0] );

	return R_DoDecalSHClip( g_DecalClipVerts[0], pDecal, surf->polys->numverts, pVertCount );
}

// Generate lighting coordinates at each vertex for decal vertices v[] on surface psurf
static void R_DecalVertsLight( float *v, msurface_t *surf, int vertCount )
{
	float		s, t;
	mtexinfo_t	*tex;
	int		j;

	tex = surf->texinfo;

	for( j = 0; j < vertCount; j++, v += VERTEXSIZE )
	{
		// lightmap texture coordinates
		s = DotProduct( v, tex->vecs[0] ) + tex->vecs[0][3] - surf->texturemins[0];
		s += surf->light_s * LM_SAMPLE_SIZE;
		s += LM_SAMPLE_SIZE >> 1;
		s /= BLOCK_SIZE * LM_SAMPLE_SIZE; //fa->texinfo->texture->width;

		t = DotProduct( v, tex->vecs[1] ) + tex->vecs[1][3] - surf->texturemins[1];
		t += surf->light_t * LM_SAMPLE_SIZE;
		t += LM_SAMPLE_SIZE >> 1;
		t /= BLOCK_SIZE * LM_SAMPLE_SIZE; //fa->texinfo->texture->height;

		v[5] = s;
		v[6] = t;
	}
}

// Check for intersecting decals on this surface
static decal_t *R_DecalIntersect( decalinfo_t *decalinfo, msurface_t *surf, int *pcount )
{
	int		texture;
	decal_t		*plast, *pDecal;
	vec3_t		decalExtents[2];
	float		lastArea = 2;
	int		mapSize[2];

	plast = NULL;
	*pcount = 0;

	// (Same as R_SetupDecalClip).
	texture = decalinfo->m_iTexture;
	
	// precalculate the extents of decalinfo's decal in world space.
	R_GetDecalDimensions( texture, &mapSize[0], &mapSize[1] );
	VectorScale( decalinfo->m_Basis[0], ((mapSize[0] / decalinfo->m_scale) * 0.5f), decalExtents[0] );
	VectorScale( decalinfo->m_Basis[1], ((mapSize[1] / decalinfo->m_scale) * 0.5f), decalExtents[1] );

	pDecal = surf->pdecals;

	while( pDecal ) 
	{
		texture = pDecal->texture;

		// Don't steal bigger decals and replace them with smaller decals
		// Don't steal permanent decals
		if(!( pDecal->flags & FDECAL_PERMANENT ))
		{
			vec3_t	testBasis[3];
			vec3_t	testPosition[2];
			float	testWorldScale[2];
			vec2_t	vDecalMin, vDecalMax;
			vec2_t	vUnionMin, vUnionMax;

			R_SetupDecalTextureSpaceBasis( pDecal, surf, texture, testBasis, testWorldScale );

			VectorSubtract( decalinfo->m_Position, decalExtents[0], testPosition[0] );
			VectorSubtract( decalinfo->m_Position, decalExtents[1], testPosition[1] );

			// Here, we project the min and max extents of the decal that got passed in into
			// this decal's (pDecal's) [0,0,1,1] clip space, just like we would if we were
			// clipping a triangle into pDecal's clip space.
			Vector2Set( vDecalMin,
				DotProduct( testPosition[0], testBasis[0] ) - pDecal->dx + 0.5f,
				DotProduct( testPosition[1], testBasis[1] ) - pDecal->dy + 0.5f );

			VectorAdd( decalinfo->m_Position, decalExtents[0], testPosition[0] );
			VectorAdd( decalinfo->m_Position, decalExtents[1], testPosition[1] );

			Vector2Set( vDecalMax,
				DotProduct( testPosition[0], testBasis[0] ) - pDecal->dx + 0.5f,
				DotProduct( testPosition[1], testBasis[1] ) - pDecal->dy + 0.5f );	

			// Now figure out the part of the projection that intersects pDecal's
			// clip box [0,0,1,1].
			Vector2Set( vUnionMin, max( vDecalMin[0], 0 ), max( vDecalMin[1], 0 ));
			Vector2Set( vUnionMax, min( vDecalMax[0], 1 ), min( vDecalMax[1], 1 ));

			if( vUnionMin[0] < 1 && vUnionMin[1] < 1 && vUnionMax[0] > 0 && vUnionMax[1] > 0 )
			{
				// Figure out how much of this intersects the (0,0) - (1,1) bbox.			
				float	flArea = (vUnionMax[0] - vUnionMin[1]) * (vUnionMax[1] - vUnionMin[1]);

				if( flArea > 0.6f )
				{
					*pcount += 1;

					if( !plast || flArea <= lastArea ) 
					{
						plast = pDecal;
						lastArea =  flArea;
					}
				}
			}
		}
		pDecal = pDecal->pnext;
	}
	return plast;
}

/*
====================
R_BuildMeshForDecal

creates mesh for decal on first rendering
====================
*/
msurfmesh_t *R_DecalCreateMesh( decalinfo_t *decalinfo, decal_t *pdecal, msurface_t *surf )
{
	float		*v;
	uint		i, bufSize;
	qboolean	createSTverts = false;
	int		numVerts, numElems;
	byte		*buffer;
	msurfmesh_t	*mesh;

	if( pdecal->mesh )
	{
		// already have mesh
		return pdecal->mesh;
	}

	v = R_DecalSetupVerts( pdecal, surf, pdecal->texture, &numVerts );
	if( !numVerts ) return NULL;	// probably this never happens

	// allocate mesh
	numElems = (numVerts - 2) * 3;

	bufSize = sizeof( msurfmesh_t ) + numVerts * sizeof( glvert_t ) + numElems * sizeof( word );
	buffer = Mem_Alloc( cls.mempool, bufSize );

	mesh = (msurfmesh_t *)buffer;
	buffer += sizeof( msurfmesh_t );
	mesh->numVerts = numVerts;
	mesh->numElems = numElems;

	// setup pointers
	mesh->verts = (glvert_t *)buffer;
	buffer += numVerts * sizeof( glvert_t );
	mesh->elems = (word *)buffer;
	buffer += numElems * sizeof( word );

	mesh->surf = surf;	// NOTE: meshchains can be linked with one surface

	// create indices
	for( i = 0; i < mesh->numVerts - 2; i++ )
	{
		mesh->elems[i*3+0] = 0;
		mesh->elems[i*3+1] = i + 1;
		mesh->elems[i*3+2] = i + 2;
	}
	#ifdef __arm__
	float inv_w = 1.0f / surf->texinfo->texture->width;
	float inv_h = 1.0f / surf->texinfo->texture->height;
	#endif

	// fill the mesh
	for( i = 0; i < numVerts; i++, v += VERTEXSIZE )
	{
		glvert_t	*out = &mesh->verts[i];
		VectorCopy( v, out->vertex );
		VectorCopy( decalinfo->m_Basis[0], out->tangent );
		VectorCopy( decalinfo->m_Basis[1], out->binormal );
		VectorCopy( decalinfo->m_Basis[2], out->normal );

		out->stcoord[0] = v[3];
		out->stcoord[1] = v[4];
		out->lmcoord[0] = v[5];
		out->lmcoord[1] = v[6];
		out->sccoord[0] = (( DotProduct( v , surf->texinfo->vecs[0] ) + surf->texinfo->vecs[0][3] ) 
		#ifdef __arm__
			* inv_w );
		#else
			/ surf->texinfo->texture->width );
		#endif
		out->sccoord[1] = (( DotProduct( v , surf->texinfo->vecs[1] ) + surf->texinfo->vecs[1][3] ) 
		#ifdef __arm__
			* inv_h );
		#else
			/ surf->texinfo->texture->height );
		#endif

		// clear colors (it can be used for vertex lighting)
		Q_memset( out->color, 0xFF, sizeof( out->color ));
	}

	pdecal->mesh = mesh;

	return mesh;
}

// Add the decal to the surface's list of decals.
static void R_AddDecalToSurface( decal_t *pdecal, msurface_t *surf, decalinfo_t *decalinfo )
{
	decal_t	*pold;

	pdecal->pnext = NULL;
	pold = surf->pdecals;

	if( pold ) 
	{
		while( pold->pnext ) 
			pold = pold->pnext;
		pold->pnext = pdecal;
	}
	else
	{
		surf->pdecals = pdecal;
	}

	// tag surface
	pdecal->psurface = surf;

	// at this point decal are linked with surface
	// and will be culled, drawing and sorting
	// together with surface

	// build mesh for decal if allowed
	if( host.features & ENGINE_BUILD_SURFMESHES )
		pdecal->mesh = R_DecalCreateMesh( decalinfo, pdecal, surf );
}

static void R_DecalCreate( decalinfo_t *decalinfo, msurface_t *surf, float x, float y )
{
	decal_t	*pdecal, *pold;
	int	count, vertCount;

	if( !surf )
	{
		MsgDev( D_ERROR, "psurface NULL in R_DecalCreate!\n" );
		return;
	}
	
	pold = R_DecalIntersect( decalinfo, surf, &count );
	if( count < MAX_OVERLAP_DECALS ) pold = NULL;

	pdecal = R_DecalAlloc( pold );
	if( !pdecal ) return; // r_decals == 0 ???

	pdecal->flags = decalinfo->m_Flags;

	VectorCopy( decalinfo->m_Position, pdecal->position );

	if( pdecal->flags & FDECAL_USESAXIS )
		VectorCopy( decalinfo->m_SAxis, pdecal->saxis );

	pdecal->dx = x;
	pdecal->dy = y;
	pdecal->texture = decalinfo->m_iTexture;

	// set scaling
	pdecal->scale = decalinfo->m_scale;
	pdecal->entityIndex = decalinfo->m_Entity;

	// check to see if the decal actually intersects the surface
	// if not, then remove the decal
	R_DecalVertsClip( pdecal, surf, decalinfo->m_iTexture, &vertCount );
	
	if( !vertCount )
	{
		R_DecalUnlink( pdecal );
		return;
	}

	// add to the surface's list
	R_AddDecalToSurface( pdecal, surf, decalinfo );
}

void R_DecalSurface( msurface_t *surf, decalinfo_t *decalinfo )
{
	// get the texture associated with this surface
	mtexinfo_t	*tex = surf->texinfo;
	vec4_t		textureU, textureV;
	float		*sAxis = NULL;
	float		s, t, w, h;
	decal_t		*decal = surf->pdecals;

	// we in restore mode
	if( cls.state == ca_connected )
	{
		// NOTE: we may have the decal on this surface that come from another level.
		// check duplicate with same position and texture
		while( decal != NULL )
		{
			if( VectorCompare( decal->position, decalinfo->m_Position ) && decal->texture == decalinfo->m_iTexture )
				return; // decal already exists, don't place it again
			decal = decal->pnext;
		}
	}

	Vector4Copy( tex->vecs[0], textureU );
	Vector4Copy( tex->vecs[1], textureV );

	// project decal center into the texture space of the surface
	s = DotProduct( decalinfo->m_Position, textureU ) + textureU[3] - surf->texturemins[0];
	t = DotProduct( decalinfo->m_Position, textureV ) + textureV[3] - surf->texturemins[1];

	// Determine the decal basis (measured in world space)
	// Note that the decal basis vectors 0 and 1 will always lie in the same
	// plane as the texture space basis vectorstextureVecsTexelsPerWorldUnits.

	if( decalinfo->m_Flags & FDECAL_USESAXIS )
		sAxis = decalinfo->m_SAxis;

	R_DecalComputeBasis( surf, sAxis, decalinfo->m_Basis );

	// Compute an effective width and height (axis aligned) in the parent texture space
	// How does this work? decalBasis[0] represents the u-direction (width)
	// of the decal measured in world space, decalBasis[1] represents the 
	// v-direction (height) measured in world space.
	// textureVecsTexelsPerWorldUnits[0] represents the u direction of 
	// the surface's texture space measured in world space (with the appropriate
	// scale factor folded in), and textureVecsTexelsPerWorldUnits[1]
	// represents the texture space v direction. We want to find the dimensions (w,h)
	// of a square measured in texture space, axis aligned to that coordinate system.
	// All we need to do is to find the components of the decal edge vectors
	// (decalWidth * decalBasis[0], decalHeight * decalBasis[1])
	// in texture coordinates:

	w = fabs( decalinfo->m_decalWidth  * DotProduct( textureU, decalinfo->m_Basis[0] )) +
	    fabs( decalinfo->m_decalHeight * DotProduct( textureU, decalinfo->m_Basis[1] ));
	
	h = fabs( decalinfo->m_decalWidth  * DotProduct( textureV, decalinfo->m_Basis[0] )) +
	    fabs( decalinfo->m_decalHeight * DotProduct( textureV, decalinfo->m_Basis[1] ));

	// move s,t to upper left corner
	s -= ( w * 0.5f );
	t -= ( h * 0.5f );

	// Is this rect within the surface? -- tex width & height are unsigned
	if( s <= -w || t <= -h || s > (surf->extents[0] + w) || t > (surf->extents[1] + h))
	{
		return; // nope
	}

	// stamp it
	R_DecalCreate( decalinfo, surf, s, t );
}

//-----------------------------------------------------------------------------
// iterate over all surfaces on a node, looking for surfaces to decal
//-----------------------------------------------------------------------------
static void R_DecalNodeSurfaces( model_t *model, mnode_t *node, decalinfo_t *decalinfo )
{
	// iterate over all surfaces in the node
	msurface_t	*surf;
	int		i;

	surf = model->surfaces + node->firstsurface;

	for( i = 0; i < node->numsurfaces; i++, surf++ ) 
	{
		// never apply decals on the water or sky surfaces
		if( surf->flags & (SURF_DRAWTURB|SURF_DRAWSKY|SURF_CONVEYOR))
			continue;

		if( surf->flags & SURF_TRANSPARENT && !glState.stencilEnabled )
			continue;

		R_DecalSurface( surf, decalinfo );
	}
}

//-----------------------------------------------------------------------------
// Recursive routine to find surface to apply a decal to.  World coordinates of 
// the decal are passed in r_recalpos like the rest of the engine.  This should 
// be called through R_DecalShoot()
//-----------------------------------------------------------------------------
static void R_DecalNode( model_t *model, mnode_t *node, decalinfo_t *decalinfo )
{
	mplane_t	*splitplane;
	float	dist;
	
	ASSERT( node );

	if( node->contents < 0 )
	{
		// hit a leaf
		return;
	}

	splitplane = node->plane;
	dist = DotProduct( decalinfo->m_Position, splitplane->normal ) - splitplane->dist;

	// This is arbitrarily set to 10 right now. In an ideal world we'd have the 
	// exact surface but we don't so, this tells me which planes are "sort of 
	// close" to the gunshot -- the gunshot is actually 4 units in front of the 
	// wall (see dlls\weapons.cpp). We also need to check to see if the decal 
	// actually intersects the texture space of the surface, as this method tags
	// parallel surfaces in the same node always.
	// JAY: This still tags faces that aren't correct at edges because we don't 
	// have a surface normal
	if( dist > decalinfo->m_Size )
	{
		R_DecalNode( model, node->children[0], decalinfo );
	}
	else if( dist < -decalinfo->m_Size )
	{
		R_DecalNode( model, node->children[1], decalinfo );
	}
	else 
	{
		if( dist < DECAL_DISTANCE && dist > -DECAL_DISTANCE )
			R_DecalNodeSurfaces( model, node, decalinfo );

		R_DecalNode( model, node->children[0], decalinfo );
		R_DecalNode( model, node->children[1], decalinfo );
	}
}

// Shoots a decal onto the surface of the BSP.  position is the center of the decal in world coords
void R_DecalShoot( int textureIndex, int entityIndex, int modelIndex, vec3_t pos, int flags, vec3_t saxis, float scale )
{
	decalinfo_t	decalInfo;
	hull_t		*hull;
	cl_entity_t	*ent = NULL;
	model_t		*model = NULL;
	int		width, height;

	if( textureIndex <= 0 || textureIndex >= MAX_TEXTURES )
	{
		MsgDev( D_ERROR, "Decal has invalid texture!\n" );
		return;
	}

	if( entityIndex > 0 )
	{
		ent = CL_GetEntityByIndex( entityIndex );

		if( modelIndex > 0 ) model = Mod_Handle( modelIndex );
		else if( ent != NULL ) model = Mod_Handle( ent->curstate.modelindex );
		else return;
	}
	else if( modelIndex > 0 )
		model = Mod_Handle( modelIndex );
	else model = cl.worldmodel;

	if( !model ) return;
	
	if( model->type != mod_brush )
	{
		MsgDev( D_ERROR, "Decals must hit mod_brush!\n" );
		return;
	}

	decalInfo.m_pModel = model;
	hull = &model->hulls[0];	// always use #0 hull

	if( ent && !( flags & FDECAL_LOCAL_SPACE ))
	{
		vec3_t	pos_l;

		// transform decal position in local bmodel space
		if( !VectorIsNull( ent->angles ))
		{
			matrix4x4	matrix;

			Matrix4x4_CreateFromEntity( matrix, ent->angles, ent->origin, 1.0f );
			Matrix4x4_VectorITransform( matrix, pos, pos_l );
		}
		else
		{
			VectorSubtract( pos, ent->origin, pos_l );
		}

		VectorCopy( pos_l, decalInfo.m_Position );
		flags |= FDECAL_LOCAL_SPACE; // decal position moved into local space
	}
	else
	{
		// pass position in global
		VectorCopy( pos, decalInfo.m_Position );
	}

	// deal with the s axis if one was passed in
	if( saxis )
	{
		flags |= FDECAL_USESAXIS;
		VectorCopy( saxis, decalInfo.m_SAxis );
	}

	// this decal must use landmark for correct transition
	if(!( model->flags & MODEL_HAS_ORIGIN ))
	{
		flags |= FDECAL_USE_LANDMARK;
	}

	// more state used by R_DecalNode()
	decalInfo.m_iTexture = textureIndex;
	decalInfo.m_Entity = entityIndex;
	decalInfo.m_Flags = flags;

	R_GetDecalDimensions( textureIndex, &width, &height );
	decalInfo.m_Size = width >> 1;
	if(( height >> 1 ) > decalInfo.m_Size )
		decalInfo.m_Size = height >> 1;

	decalInfo.m_scale = bound( MIN_DECAL_SCALE, scale, MAX_DECAL_SCALE );

	// compute the decal dimensions in world space
	decalInfo.m_decalWidth = width / decalInfo.m_scale;
	decalInfo.m_decalHeight = height / decalInfo.m_scale;

	R_DecalNode( model, &model->nodes[hull->firstclipnode], &decalInfo );
}

// Build the vertex list for a decal on a surface and clip it to the surface.
// This is a template so it can work on world surfaces and dynamic displacement 
// triangles the same way.
float *R_DecalSetupVerts( decal_t *pDecal, msurface_t *surf, int texture, int *outCount )
{
	float	*v;
	int	i, count;

	if( pDecal->mesh )
	{
		count = pDecal->mesh->numVerts;

		// if we have mesh so skip clipping and just copy vertexes out (perf)
		for( i = 0, v = g_DecalClipVerts[0]; i < count; i++, v += VERTEXSIZE )
		{
			glvert_t	*p = &pDecal->mesh->verts[i];

			VectorCopy( p->vertex, v );
			v[3] = p->stcoord[0];
			v[4] = p->stcoord[1];
			v[5] = p->lmcoord[0];
			v[6] = p->lmcoord[1];
		}

		// restore pointer
		v = g_DecalClipVerts[0];
	}
	else
	{
		v = R_DecalVertsClip( pDecal, surf, texture, &count );
		R_DecalVertsLight( v, surf, count );
	}

	if( outCount )
		*outCount = count;

	return v;
}

void DrawSingleDecal( decal_t *pDecal, msurface_t *fa )
{
	float	*v;
	int	i, numVerts;

	v = R_DecalSetupVerts( pDecal, fa, pDecal->texture, &numVerts );
	if( !numVerts ) return;

	GL_Bind( XASH_TEXTURE0, pDecal->texture );

	pglBegin( GL_POLYGON );

	for( i = 0; i < numVerts; i++, v += VERTEXSIZE )
	{
		pglTexCoord2f( v[3], v[4] );
		pglVertex3fv( v );
	}

	pglEnd();
}

void DrawSurfaceDecals( msurface_t *fa )
{
	decal_t		*p;
	cl_entity_t	*e;

	if( !fa->pdecals ) return;

	e = RI.currententity;
	ASSERT( e != NULL );

	if( e->curstate.rendermode == kRenderNormal || e->curstate.rendermode == kRenderTransAlpha )
	{
		pglDepthMask( GL_FALSE );
		pglEnable( GL_BLEND );

		if( e->curstate.rendermode == kRenderTransAlpha )
			pglDisable( GL_ALPHA_TEST );
	}

	if( e->curstate.rendermode == kRenderTransColor )
		pglEnable( GL_TEXTURE_2D );

	if( e->curstate.rendermode == kRenderTransTexture || e->curstate.rendermode == kRenderTransAdd )
		GL_Cull( GL_NONE );

	pglEnable( GL_POLYGON_OFFSET_FILL );
	pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	if( fa->flags & SURF_TRANSPARENT && glState.stencilEnabled )
	{
		mtexinfo_t	*tex = fa->texinfo;

		for( p = fa->pdecals; p; p = p->pnext )
		{
			if( p->texture )
			{
				float *o, *v;
				int i, numVerts;
				o = R_DecalSetupVerts( p, fa, p->texture, &numVerts );

				pglEnable( GL_STENCIL_TEST );
				pglStencilFunc( GL_ALWAYS, 1, 0xFFFFFFFF );
				pglColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

				pglStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );
				pglBegin( GL_POLYGON );

				for( i = 0, v = o; i < numVerts; i++, v += VERTEXSIZE )
				{
					v[5] = ( DotProduct( v, tex->vecs[0] ) + tex->vecs[0][3] ) / tex->texture->width;
					v[6] = ( DotProduct( v, tex->vecs[1] ) + tex->vecs[1][3] ) / tex->texture->height;

					pglTexCoord2f( v[5], v[6] );
					pglVertex3fv( v );
				}

				pglEnd();
				pglStencilOp( GL_KEEP, GL_KEEP, GL_DECR );

				pglEnable( GL_ALPHA_TEST );
				pglBegin( GL_POLYGON );

				for( i = 0, v = o; i < numVerts; i++, v += VERTEXSIZE )
				{
					pglTexCoord2f( v[5], v[6] );
					pglVertex3fv( v );
				}

				pglEnd();
				pglDisable( GL_ALPHA_TEST );

				pglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
				pglStencilFunc( GL_EQUAL, 0, 0xFFFFFFFF );
				pglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
			}
		}
	}

	for( p = fa->pdecals; p; p = p->pnext )
	{
		if( p->texture )
		{
			gltexture_t *glt = R_GetTexture( p->texture );

			// normal HL decal with alpha-channel
			if( glt->flags & TF_HAS_ALPHA )
			{
				// draw transparent decals with GL_MODULATE
				if( glt->fogParams[3] > DECAL_TRANSPARENT_THRESHOLD )
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

			DrawSingleDecal( p, fa );
		}
	}

	if( fa->flags & SURF_TRANSPARENT && glState.stencilEnabled )
		pglDisable( GL_STENCIL_TEST );

	if( e->curstate.rendermode == kRenderNormal || e->curstate.rendermode == kRenderTransAlpha )
	{
		pglDepthMask( GL_TRUE );
		pglDisable( GL_BLEND );

		if( e->curstate.rendermode == kRenderTransAlpha )
			pglEnable( GL_ALPHA_TEST );
	}

	pglDisable( GL_POLYGON_OFFSET_FILL );

	if( e->curstate.rendermode == kRenderTransTexture || e->curstate.rendermode == kRenderTransAdd )
		GL_Cull( GL_FRONT );

	if( e->curstate.rendermode == kRenderTransColor )
		pglDisable( GL_TEXTURE_2D );

	// restore blendfunc here
	if( e->curstate.rendermode == kRenderTransAdd || e->curstate.rendermode == kRenderGlow )
		pglBlendFunc( GL_SRC_ALPHA, GL_ONE );
}

/*
=============================================================

  DECALS SERIALIZATION

=============================================================
*/
static qboolean R_DecalUnProject( decal_t *pdecal, decallist_t *entry )
{
	if( !pdecal || !( pdecal->psurface ))
		return false;

	VectorCopy( pdecal->position, entry->position );
	entry->entityIndex = pdecal->entityIndex;

	// Grab surface plane equation
	if( pdecal->psurface->flags & SURF_PLANEBACK )
		VectorNegate( pdecal->psurface->plane->normal, entry->impactPlaneNormal );
	else VectorCopy( pdecal->psurface->plane->normal, entry->impactPlaneNormal );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pList - 
//			count - 
// Output : static int
//-----------------------------------------------------------------------------
static int DecalListAdd( decallist_t *pList, int count )
{
	vec3_t		tmp;
	decallist_t	*pdecal;
	int		i;

	pdecal = pList + count;

	for( i = 0; i < count; i++ )
	{
		if( !Q_strcmp( pdecal->name, pList[i].name ) &&  pdecal->entityIndex == pList[i].entityIndex )
		{
			VectorSubtract( pdecal->position, pList[i].position, tmp );	// Merge

			if( VectorLength( tmp ) < DECAL_OVERLAP_DISTANCE )
				return count;
		}
	}

	// this is a new decal
	return count + 1;
}

static int DecalDepthCompare( const void *a, const void *b )
{
	const decallist_t	*elem1, *elem2;

	elem1 = (const decallist_t *)a;
	elem2 = (const decallist_t *)b;

	if( elem1->depth > elem2->depth )
		return 1;
	if( elem1->depth < elem2->depth )
		return -1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Called by CSaveRestore::SaveClientState
// Input  : *pList - 
// Output : int
//-----------------------------------------------------------------------------
int R_CreateDecalList( decallist_t *pList, qboolean changelevel )
{
	int	total = 0;
	int	i, depth;

	if( cl.worldmodel )
	{
		for( i = 0; i < MAX_RENDER_DECALS; i++ )
		{
			decal_t	*decal = &gDecalPool[i];
			decal_t	*pdecals;
			
			// decal is in use and is not a custom decal
			if( decal->psurface == NULL || ( decal->flags & FDECAL_DONTSAVE ))	
				 continue;

			// compute depth
			depth = 0;
			pdecals = decal->psurface->pdecals;

			while( pdecals && pdecals != decal )
			{
				depth++;
				pdecals = pdecals->pnext;
			}

			pList[total].depth = depth;
			pList[total].flags = decal->flags;
			pList[total].scale = decal->scale;
			
			R_DecalUnProject( decal, &pList[total] );
			FS_FileBase( R_GetTexture( decal->texture )->name, pList[total].name );

			// check to see if the decal should be added
			total = DecalListAdd( pList, total );
		}

		if( clgame.drawFuncs.R_CreateStudioDecalList )
		{
			total += clgame.drawFuncs.R_CreateStudioDecalList( pList, total, changelevel );
		}
	}

	// sort the decals lowest depth first, so they can be re-applied in order
	qsort( pList, total, sizeof( decallist_t ), DecalDepthCompare );

	return total;
}

/*
===============
R_DecalRemoveAll

remove all decals with specified texture
===============
*/
void R_DecalRemoveAll( int textureIndex )
{
	decal_t	*pdecal;
	int	i;

	if( textureIndex < 0 || textureIndex >= MAX_TEXTURES )
	{
		MsgDev( D_ERROR, "Decal has invalid texture!\n" );
		return;
	}

	for( i = 0; i < gDecalCount; i++ )
	{
		pdecal = &gDecalPool[i];

		// don't remove permanent decals
		if( pdecal->flags & FDECAL_PERMANENT )
			continue;

		if( !textureIndex || (pdecal->texture == textureIndex) )
			R_DecalUnlink( pdecal );
	}
}

/*
===============
R_EntityRemoveDecals

remove all decals from specified entity
===============
*/
void R_EntityRemoveDecals( model_t *mod )
{
	msurface_t	*psurf;
	decal_t		*p;
	int		i;

	if( !mod || mod->type != mod_brush )
		return;

	psurf = &mod->surfaces[mod->firstmodelsurface];
	for( i = 0; i < mod->nummodelsurfaces; i++, psurf++ )
	{
		for( p = psurf->pdecals; p; p = p->pnext )
			R_DecalUnlink( p );
	}
}

/*
===============
R_ClearAllDecals

remove all decals from anything
used for full decals restart 
===============
*/
void R_ClearAllDecals( void )
{
	decal_t	*pdecal;
	int	i;

	// because gDecalCount may be zeroed after recach the decal limit
	for( i = 0; i < MAX_RENDER_DECALS; i++ )
	{
		pdecal = &gDecalPool[i];
		R_DecalUnlink( pdecal );
	}

	if( clgame.drawFuncs.R_ClearStudioDecals )
	{
		clgame.drawFuncs.R_ClearStudioDecals();
	}
}

#endif // XASH_DEDICATED
