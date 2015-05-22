/*
pm_surface.c - surface tracing
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

#include "common.h"
#include "mathlib.h"
#include "pm_local.h"

/*
==================
PM_RecursiveSurfCheck

==================
*/
msurface_t *PM_RecursiveSurfCheck( model_t *model, mnode_t *node, vec3_t p1, vec3_t p2 )
{
	float		t1, t2, frac;
	int		side, ds, dt;
	mplane_t		*plane;
	msurface_t	*surf;
	vec3_t		mid;
	int		i;

	if( node->contents < 0 )
		return NULL;

	plane = node->plane;

	if( plane->type < 3 )
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct( plane->normal, p1 ) - plane->dist;
		t2 = DotProduct( plane->normal, p2 ) - plane->dist;
	}

	if( t1 >= 0.0f && t2 >= 0.0f )
		return PM_RecursiveSurfCheck( model, node->children[0], p1, p2 );
	if( t1 < 0.0f && t2 < 0.0f )
		return PM_RecursiveSurfCheck( model, node->children[1], p1, p2 );

	frac = t1 / ( t1 - t2 );

	if( frac < 0.0f ) frac = 0.0f;
	if( frac > 1.0f ) frac = 1.0f;

	VectorLerp( p1, frac, p2, mid );

	side = (t1 < 0.0f);

	// now this is weird.
	surf = PM_RecursiveSurfCheck( model, node->children[side], p1, mid );

	if( surf != NULL || ( t1 >= 0.0f && t2 >= 0.0f ) || ( t1 < 0.0f && t2 < 0.0f ))
	{
		return surf;
	}

	surf = model->surfaces + node->firstsurface;

	for( i = 0; i < node->numsurfaces; i++, surf++ )
	{
		ds = (int)((float)DotProduct( mid, surf->texinfo->vecs[0] ) + surf->texinfo->vecs[0][3] );
		dt = (int)((float)DotProduct( mid, surf->texinfo->vecs[1] ) + surf->texinfo->vecs[1][3] );

		if( ds >= surf->texturemins[0] && dt >= surf->texturemins[1] )
		{
			int s = ds - surf->texturemins[0];
			int t = dt - surf->texturemins[1];

			if( s <= surf->extents[0] && t <= surf->extents[1] )
				return surf;
		}
	}

	return PM_RecursiveSurfCheck( model, node->children[side^1], mid, p2 );
}

/*
==================
PM_TraceTexture

find the face where the traceline hit
assume physentity is valid
==================
*/
msurface_t *PM_TraceSurface( physent_t *pe, vec3_t start, vec3_t end )
{
	matrix4x4		matrix;
	model_t		*bmodel;
	hull_t		*hull;
	vec3_t		start_l, end_l;
	vec3_t		offset;

	bmodel = pe->model;

	if( !bmodel || bmodel->type != mod_brush )
		return NULL;

	hull = &pe->model->hulls[0];
	VectorSubtract( hull->clip_mins, vec3_origin, offset );
	VectorAdd( offset, pe->origin, offset );

	VectorSubtract( start, offset, start_l );
	VectorSubtract( end, offset, end_l );

	// rotate start and end into the models frame of reference
	if( !VectorIsNull( pe->angles ))
	{
		Matrix4x4_CreateFromEntity( matrix, pe->angles, offset, 1.0f );
		Matrix4x4_VectorITransform( matrix, start, start_l );
		Matrix4x4_VectorITransform( matrix, end, end_l );
	}

	return PM_RecursiveSurfCheck( bmodel, &bmodel->nodes[hull->firstclipnode], start_l, end_l );
}

/*
==================
PM_TraceTexture

find the face where the traceline hit
assume physentity is valid
==================
*/
const char *PM_TraceTexture( physent_t *pe, vec3_t start, vec3_t end )
{
	msurface_t	*surf;
	matrix4x4		matrix;
	model_t		*bmodel;
	hull_t		*hull;
	vec3_t		start_l, end_l;
	vec3_t		offset;

	bmodel = pe->model;

	if( !bmodel || bmodel->type != mod_brush )
		return NULL;

	hull = &pe->model->hulls[0];
	VectorSubtract( hull->clip_mins, vec3_origin, offset );
	VectorAdd( offset, pe->origin, offset );

	VectorSubtract( start, offset, start_l );
	VectorSubtract( end, offset, end_l );

	// rotate start and end into the models frame of reference
	if( !VectorIsNull( pe->angles ))
	{
		Matrix4x4_CreateFromEntity( matrix, pe->angles, offset, 1.0f );
		Matrix4x4_VectorITransform( matrix, start, start_l );
		Matrix4x4_VectorITransform( matrix, end, end_l );
	}

	surf = PM_RecursiveSurfCheck( bmodel, &bmodel->nodes[hull->firstclipnode], start_l, end_l );

	if( !surf || !surf->texinfo || !surf->texinfo->texture )
		return NULL;

	return surf->texinfo->texture->name;
}
