/*
gl_cull.c - render culling routines
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
#include "entity_types.h"

/*
=============================================================

FRUSTUM AND PVS CULLING

=============================================================
*/
/*
=================
R_CullBox

Returns true if the box is completely outside the frustum
=================
*/
qboolean R_CullBox( const vec3_t mins, const vec3_t maxs, uint clipflags )
{
	uint		i, bit;
	const mplane_t	*p;

	// client.dll may use additional passes for render custom mirrors etc
	if( r_nocull->integer )
		return false;

	for( i = sizeof( RI.frustum ) / sizeof( RI.frustum[0] ), bit = 1, p = RI.frustum; i > 0; i--, bit<<=1, p++ )
	{
		if( !( clipflags & bit ))
			continue;

		switch( p->signbits )
		{
		case 0:
			if( p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist )
				return true;
			break;
		case 1:
			if( p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist )
				return true;
			break;
		case 2:
			if( p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist )
				return true;
			break;
		case 3:
			if( p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist )
				return true;
			break;
		case 4:
			if( p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist )
				return true;
			break;
		case 5:
			if( p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist )
				return true;
			break;
		case 6:
			if( p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist )
				return true;
			break;
		case 7:
			if( p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist )
				return true;
			break;
		default:
			return false;
		}
	}
	return false;
}

/*
=================
R_CullSphere

Returns true if the sphere is completely outside the frustum
=================
*/
qboolean R_CullSphere( const vec3_t centre, const float radius, const uint clipflags )
{
	uint	i, bit;
	const mplane_t *p;

	// client.dll may use additional passes for render custom mirrors etc
	if( r_nocull->integer )
		return false;

	for( i = sizeof( RI.frustum ) / sizeof( RI.frustum[0] ), bit = 1, p = RI.frustum; i > 0; i--, bit<<=1, p++ )
	{
		if(!( clipflags & bit )) continue;
		if( DotProduct( centre, p->normal ) - p->dist <= -radius )
			return true;
	}
	return false;
}

/*
=============
R_CullModel
=============
*/
int R_CullModel( cl_entity_t *e, vec3_t origin, vec3_t mins, vec3_t maxs, float radius )
{
	if( e == &clgame.viewent )
	{
		if( RI.params & RP_NONVIEWERREF )
			return 1;
		return 0;
	}

	// don't reflect this entity in mirrors
	if( e->curstate.effects & EF_NOREFLECT && RI.params & RP_MIRRORVIEW )
		return 1;

	// draw only in mirrors
	if( e->curstate.effects & EF_REFLECTONLY && !( RI.params & RP_MIRRORVIEW ))
		return 1;

	if( RP_LOCALCLIENT( e ) && !RI.thirdPerson && cl.refdef.viewentity == ( cl.playernum + 1 ))
	{
		if(!( RI.params & RP_MIRRORVIEW ))
			return 1;
	}

	if( R_CullSphere( origin, radius, RI.clipFlags ))
		return 1;

	return 0;
}

/*
=================
R_CullSurface

cull invisible surfaces
=================
*/
qboolean R_CullSurface( msurface_t *surf, uint clipflags )
{
	mextrasurf_t	*info;
	cl_entity_t	*e = RI.currententity;

	if( !surf || !surf->texinfo || !surf->texinfo->texture )
		return true;

	if( surf->flags & SURF_WATERCSG && !( e->curstate.effects & EF_NOWATERCSG ))
		return true;

	if( surf->flags & SURF_NOCULL )
		return false;

	// don't cull transparent surfaces because we should be draw decals on them
	if( surf->pdecals && ( e->curstate.rendermode == kRenderTransTexture || e->curstate.rendermode == kRenderTransAdd ))
		return false;

	if( r_nocull->integer )
		return false;

	// world surfaces can be culled by vis frame too
	if( RI.currententity == clgame.entities && surf->visframe != tr.framecount )
		return true;

	if( r_faceplanecull->integer && glState.faceCull != 0 )
	{
		if( RI.currentWaveHeight == 0.0f )
		{
			if( !VectorIsNull( surf->plane->normal ))
			{
				float	dist;

				if( RI.drawOrtho ) dist = surf->plane->normal[2];
				else dist = PlaneDiff( tr.modelorg, surf->plane );

				if( glState.faceCull == GL_FRONT || ( RI.params & RP_MIRRORVIEW ))
				{
					if( surf->flags & SURF_PLANEBACK )
					{
						if( dist >= -BACKFACE_EPSILON )
							return true; // wrong side
					}
					else
					{
						if( dist <= BACKFACE_EPSILON )
							return true; // wrong side
					}
				}
				else if( glState.faceCull == GL_BACK )
				{
					if( surf->flags & SURF_PLANEBACK )
					{
						if( dist <= BACKFACE_EPSILON )
							return true; // wrong side
					}
					else
					{
						if( dist >= -BACKFACE_EPSILON )
							return true; // wrong side
					}
				}
			}
		}
	}

	info = SURF_INFO( surf, RI.currentmodel );

	return ( clipflags && R_CullBox( info->mins, info->maxs, clipflags ));
}
#endif // XASH_DEDICATED
