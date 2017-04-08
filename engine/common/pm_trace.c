/*
pm_trace.c - pmove player trace code
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
#include "mod_local.h"
#include "pm_local.h"
#include "pm_movevars.h"
#include "engine_features.h"
#include "studio.h"
#include "world.h"

static mplane_t	pm_boxplanes[6];
static dclipnode_t	pm_boxclipnodes[6];
static hull_t	pm_boxhull;

void Pmove_Init( void )
{
	PM_InitBoxHull ();
}

/*
===================
PM_InitBoxHull

Set up the planes and clipnodes so that the six floats of a bounding box
can just be stored out and get a proper hull_t structure.
===================
*/
void PM_InitBoxHull( void )
{
	int	i, side;

	pm_boxhull.clipnodes = pm_boxclipnodes;
	pm_boxhull.planes = pm_boxplanes;
	pm_boxhull.firstclipnode = 0;
	pm_boxhull.lastclipnode = 5;

	for( i = 0; i < 6; i++ )
	{
		pm_boxclipnodes[i].planenum = i;

		side = i & 1;

		pm_boxclipnodes[i].children[side] = CONTENTS_EMPTY;
		if( i != 5 ) pm_boxclipnodes[i].children[side^1] = i + 1;
		else pm_boxclipnodes[i].children[side^1] = CONTENTS_SOLID;

		pm_boxplanes[i].type = i>>1;
		pm_boxplanes[i].normal[i>>1] = 1.0f;
		pm_boxplanes[i].signbits = 0;
	}
}

/*
===================
PM_HullForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
hull_t *PM_HullForBox( const vec3_t mins, const vec3_t maxs )
{
	pm_boxplanes[0].dist = maxs[0];
	pm_boxplanes[1].dist = mins[0];
	pm_boxplanes[2].dist = maxs[1];
	pm_boxplanes[3].dist = mins[1];
	pm_boxplanes[4].dist = maxs[2];
	pm_boxplanes[5].dist = mins[2];

	return &pm_boxhull;
}

/*
==================
PM_HullPointContents

==================
*/
int PM_HullPointContents( hull_t *hull, int num, const vec3_t p )
{
	mplane_t		*plane;

	if( !hull || !hull->planes )	// fantom bmodels?
		return CONTENTS_NONE;

	while( num >= 0 )
	{
		plane = &hull->planes[hull->clipnodes[num].planenum];
		num = hull->clipnodes[num].children[PlaneDiff( p, plane ) < 0];
	}
	return num;
}

/*
==================
PM_HullForBsp

assume physent is valid
==================
*/
hull_t *PM_HullForBsp( physent_t *pe, playermove_t *pmove, float *offset )
{
	hull_t	*hull;

	ASSERT( pe && pe->model != NULL );

	switch( pmove->usehull )
	{
	case 1:
		hull = &pe->model->hulls[3];
		break;
	case 2:
		hull = &pe->model->hulls[0];
		break;
	case 3:
		hull = &pe->model->hulls[2];
		break;
	default:
		hull = &pe->model->hulls[1];
		break;
	}

	ASSERT( hull != NULL );

	// force to use hull0 because other hulls doesn't exist for water
	if( pe->model->flags & MODEL_LIQUID && pe->solid != SOLID_TRIGGER )
		hull = &pe->model->hulls[0];

	// calculate an offset value to center the origin
	VectorSubtract( hull->clip_mins, pmove->player_mins[pmove->usehull], offset );
	VectorAdd( offset, pe->origin, offset );

	return hull;
}

/*
==================
PM_HullForStudio

generate multiple hulls as hitboxes
==================
*/
hull_t *PM_HullForStudio( physent_t *pe, playermove_t *pmove, int *numhitboxes )
{
	vec3_t	size;

	VectorSubtract( pmove->player_maxs[pmove->usehull], pmove->player_mins[pmove->usehull], size );
	VectorScale( size, 0.5f, size );

	if( pe->player )
		 return PM_HullForBox( pe->mins, pe->maxs );

	return Mod_HullForStudio( pe->studiomodel, pe->frame, pe->sequence, pe->angles, pe->origin, size, pe->controller, pe->blending, numhitboxes, NULL );
}
/* "Not a number" possible here.
 * Enable this macro to debug it */
#ifdef DEBUGNAN
static void _assertNAN(const char *f, int l, const char *x)
{
	MsgDev( D_WARN, "NAN detected at %s:%i (%s)\n", f, l, x );
}
#define ASSERTNAN(x) if( !finitef(x) ) _assertNAN( __FILE__, __LINE__, #x );
#else
#define ASSERTNAN(x)
#endif
/*
==================
PM_RecursiveHullCheck
==================
*/
qboolean PM_RecursiveHullCheck( hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, pmtrace_t *trace )
{
	dclipnode_t	*node;
	mplane_t		*plane;
	float		t1, t2;
	float		frac, midf;
	int		side;
	vec3_t		mid;

	// check for empty
	if( num < 0 )
	{
		if( num != CONTENTS_SOLID )
		{
			trace->allsolid = false;
			if( num == CONTENTS_EMPTY )
				trace->inopen = true;
			else trace->inwater = true;
		}
		else trace->startsolid = true;
		return true; // empty
	}

	if( hull->firstclipnode >= hull->lastclipnode )
	{
		// studiotrace issues
		trace->allsolid = false;
		trace->inopen = true;
		return true;
	}

	if( num < hull->firstclipnode || num > hull->lastclipnode )
		Sys_Error( "PM_RecursiveHullCheck: bad node number\n" );

	// find the point distances
	node = hull->clipnodes + num;
	plane = hull->planes + node->planenum;

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

	ASSERTNAN( t1 )
	ASSERTNAN( t2 )

	if( t1 >= 0.0f && t2 >= 0.0f )
		return PM_RecursiveHullCheck( hull, node->children[0], p1f, p2f, p1, p2, trace );
	if( t1 < 0.0f && t2 < 0.0f )
		return PM_RecursiveHullCheck( hull, node->children[1], p1f, p2f, p1, p2, trace );

	ASSERTNAN( t1 )
	ASSERTNAN( t2 )

	// put the crosspoint DIST_EPSILON pixels on the near side
	side = (t1 < 0.0f);

	if( side ) frac = ( t1 + DIST_EPSILON ) / ( t1 - t2 );
	else frac = ( t1 - DIST_EPSILON ) / ( t1 - t2 );

	ASSERTNAN( frac )

	if( frac < 0.0f ) frac = 0.0f;
	if( frac > 1.0f ) frac = 1.0f;

	midf = p1f + ( p2f - p1f ) * frac;
	VectorLerp( p1, frac, p2, mid );

	// move up to the node
	if( !PM_RecursiveHullCheck( hull, node->children[side], p1f, midf, p1, mid, trace ))
		return false;

	// this recursion can not be optimized because mid would need to be duplicated on a stack
	if( PM_HullPointContents( hull, node->children[side^1], mid ) != CONTENTS_SOLID )
	{
		// go past the node
		return PM_RecursiveHullCheck( hull, node->children[side^1], midf, p2f, mid, p2, trace );
	}	

	// never got out of the solid area
	if( trace->allsolid )
		return false;

	// the other side of the node is solid, this is the impact point
	if( !side )
	{
		VectorCopy( plane->normal, trace->plane.normal );
		trace->plane.dist = plane->dist;
	}
	else
	{
		VectorNegate( plane->normal, trace->plane.normal );
		trace->plane.dist = -plane->dist;
	}

	while( PM_HullPointContents( hull, hull->firstclipnode, mid ) == CONTENTS_SOLID )
	{
		ASSERTNAN( frac )
		// shouldn't really happen, but does occasionally
		frac -= 0.1f;

		if( ( frac < 0.0f )  || IS_NAN( frac ) )
		{
			trace->fraction = midf;
			VectorCopy( mid, trace->endpos );
			MsgDev( D_WARN, "trace backed up past 0.0\n" );
			return false;
		}

		midf = p1f + ( p2f - p1f ) * frac;
		VectorLerp( p1, frac, p2, mid );
	}

	trace->fraction = midf;
	VectorCopy( mid, trace->endpos );

	return false;
}

pmtrace_t PM_PlayerTraceExt( playermove_t *pmove, vec3_t start, vec3_t end, int flags, int numents, physent_t *ents, int ignore_pe, pfnIgnore pmFilter )
{
	physent_t	*pe;
	matrix4x4	matrix;
	pmtrace_t	trace_bbox;
	pmtrace_t	trace_hitbox;
	pmtrace_t	trace_total;
	vec3_t	offset, start_l, end_l;
	vec3_t	temp, mins, maxs;
	int	i, j, hullcount;
	qboolean	rotated, transform_bbox;
	hull_t	*hull = NULL;

	Q_memset( &trace_total, 0, sizeof( trace_total ));
	VectorCopy( end, trace_total.endpos );
	trace_total.fraction = 1.0f;
	trace_total.ent = -1;

	for( i = 0; i < numents; i++ )
	{
		pe = &ents[i];

		if( i != 0 && ( flags & PM_WORLD_ONLY ))
			break;

		// run custom user filter
		if( pmFilter != NULL )
		{
			if( pmFilter( pe ))
				continue;
		}
		else if( ignore_pe != -1 )
		{
			if( i == ignore_pe )
				continue;
		}

		if( pe->model != NULL && pe->solid == SOLID_NOT && pe->skin != CONTENTS_NONE )
			continue;

		if(( flags & PM_GLASS_IGNORE ) && pe->rendermode != kRenderNormal )
			continue;

		if(( flags & PM_CUSTOM_IGNORE ) && pe->solid == SOLID_CUSTOM )
			continue;

		hullcount = 1;

		if( pe->solid == SOLID_CUSTOM )
		{
			VectorCopy( pmove->player_mins[pmove->usehull], mins );
			VectorCopy( pmove->player_maxs[pmove->usehull], maxs );
			VectorClear( offset );
		}
		else if( !pe->model )
		{
			if( !pe->studiomodel )
			{
				VectorSubtract( pe->mins, pmove->player_maxs[pmove->usehull], mins );
				VectorSubtract( pe->maxs, pmove->player_mins[pmove->usehull], maxs );

				hull = PM_HullForBox( mins, maxs );
				VectorCopy( pe->origin, offset );
			}
			else
			{
				if( flags & PM_STUDIO_IGNORE )
					continue;

				if( pe->studiomodel->type != mod_studio || (!( pe->studiomodel->flags & STUDIO_TRACE_HITBOX ) && ( pmove->usehull != 2 || flags & PM_STUDIO_BOX )))
				{
					VectorSubtract( pe->mins, pmove->player_maxs[pmove->usehull], mins );
					VectorSubtract( pe->maxs, pmove->player_mins[pmove->usehull], maxs );

					hull = PM_HullForBox( mins, maxs );
					VectorCopy( pe->origin, offset );
				}
				else
				{
					hull = PM_HullForStudio( pe, pmove, &hullcount );
					VectorClear( offset );
				}
			}
		}
		else
		{
			hull = PM_HullForBsp( pe, pmove, offset );
		}

		if( pe->solid == SOLID_BSP && !VectorIsNull( pe->angles ))
			rotated = true;
		else rotated = false;

		if( host.features & ENGINE_TRANSFORM_TRACE_AABB )
		{
			if(( check_angles( pe->angles[0] ) || check_angles( pe->angles[2] )) && pmove->usehull != 2 )
				transform_bbox = true;
			else transform_bbox = false;
		}
		else transform_bbox = false;

		if( rotated )
		{
			if( transform_bbox )
				Matrix4x4_CreateFromEntity( matrix, pe->angles, pe->origin, 1.0f );
			else Matrix4x4_CreateFromEntity( matrix, pe->angles, offset, 1.0f );

			Matrix4x4_VectorITransform( matrix, start, start_l );
			Matrix4x4_VectorITransform( matrix, end, end_l );

			if( transform_bbox )
			{
				World_TransformAABB( matrix, pmove->player_mins[pmove->usehull], pmove->player_maxs[pmove->usehull], mins, maxs );
				VectorSubtract( hull[0].clip_mins, mins, offset );	// calc new local offset

				for( j = 0; j < 3; j++ )
				{
					if( start_l[j] >= 0.0f )
						start_l[j] -= offset[j];
					else start_l[j] += offset[j];
					if( end_l[j] >= 0.0f )
						end_l[j] -= offset[j];
					else end_l[j] += offset[j];
				}
			}
		}
		else
		{
			VectorSubtract( start, offset, start_l );
			VectorSubtract( end, offset, end_l );
		}

		Q_memset( &trace_bbox, 0, sizeof( trace_bbox ));
		VectorCopy( end, trace_bbox.endpos );
		trace_bbox.allsolid = true;
		trace_bbox.fraction = 1.0f;

		if( hullcount < 1 )
		{
			// g-cont. probably this never happens
			trace_bbox.allsolid = false;
		}
		else if( pe->solid == SOLID_CUSTOM )
		{
			// run custom sweep callback
			if( pmove->server || Host_IsLocalClient( ) || Host_IsDedicated() )
				SV_ClipPMoveToEntity( pe, start, mins, maxs, end, &trace_bbox );
#ifndef XASH_DEDICATED
			else CL_ClipPMoveToEntity( pe, start, mins, maxs, end, &trace_bbox );
#endif
		}
		else if( hullcount == 1 )
		{
			PM_RecursiveHullCheck( hull, hull[0].firstclipnode, 0, 1, start_l, end_l, &trace_bbox );
		}
		else
		{
			int	last_hitgroup;

			for( last_hitgroup = 0, j = 0; j < hullcount; j++ )
			{
				Q_memset( &trace_hitbox, 0, sizeof( trace_hitbox ));
				VectorCopy( end, trace_hitbox.endpos );
				trace_hitbox.allsolid = true;
				trace_hitbox.fraction = 1.0f;

				PM_RecursiveHullCheck( &hull[j], hull[j].firstclipnode, 0, 1, start_l, end_l, &trace_hitbox );

				if( j == 0 || trace_hitbox.allsolid || trace_hitbox.startsolid || trace_hitbox.fraction < trace_bbox.fraction )
				{
					if( trace_bbox.startsolid )
					{
						trace_bbox = trace_hitbox;
						trace_bbox.startsolid = true;
					}
					else trace_bbox = trace_hitbox;

					last_hitgroup = j;
				}
			}

			trace_bbox.hitgroup = Mod_HitgroupForStudioHull( last_hitgroup );
		}

		if( trace_bbox.allsolid )
			trace_bbox.startsolid = true;

		if( trace_bbox.startsolid )
			trace_bbox.fraction = 0.0f;

		if( !trace_bbox.startsolid )
		{
			VectorLerp( start, trace_bbox.fraction, end, trace_bbox.endpos );

			if( rotated )
			{
				VectorCopy( trace_bbox.plane.normal, temp );
				Matrix4x4_TransformPositivePlane( matrix, temp, trace_bbox.plane.dist, trace_bbox.plane.normal, &trace_bbox.plane.dist );
			}
			else
			{
				trace_bbox.plane.dist = DotProduct( trace_bbox.endpos, trace_bbox.plane.normal );
			}
		}

		if( trace_bbox.fraction < trace_total.fraction )
		{
			trace_total = trace_bbox;
			trace_total.ent = i;
		}
	}

	return trace_total;
}

int PM_TestPlayerPosition( playermove_t *pmove, vec3_t pos, pmtrace_t *ptrace, pfnIgnore pmFilter )
{
	int	i, j, hullcount;
	vec3_t	pos_l, offset;
	vec3_t	mins, maxs;
	pmtrace_t trace;
	hull_t	*hull = NULL;
	physent_t *pe;

	trace = PM_PlayerTraceExt( pmove, pmove->origin, pmove->origin, 0, pmove->numphysent, pmove->physents, -1, pmFilter );
	if( ptrace ) *ptrace = trace;

	for( i = 0; i < pmove->numphysent; i++ )
	{
		pe = &pmove->physents[i];

		// run custom user filter
		if( pmFilter != NULL )
		{
			if( pmFilter( pe ))
				continue;
		}

		if( pe->model != NULL && pe->solid == SOLID_NOT && pe->skin != CONTENTS_NONE )
			continue;

		hullcount = 1;

		if( pe->solid == SOLID_CUSTOM )
		{
			VectorCopy( pmove->player_mins[pmove->usehull], mins );
			VectorCopy( pmove->player_maxs[pmove->usehull], maxs );
			VectorClear( offset );
		}
		else if( pe->model )
		{
			hull = PM_HullForBsp( pe, pmove, offset );
		}
		else if( !pe->studiomodel || pe->studiomodel->type != mod_studio || (!( pe->studiomodel->flags & STUDIO_TRACE_HITBOX ) && pmove->usehull != 2 ))
		{
			VectorSubtract( pe->mins, pmove->player_maxs[pmove->usehull], mins );
			VectorSubtract( pe->maxs, pmove->player_mins[pmove->usehull], maxs );

			hull = PM_HullForBox( mins, maxs );
			VectorCopy( pe->origin, offset );
		}
		else
		{
			hull = PM_HullForStudio( pe, pmove, &hullcount );
			VectorClear( offset );
		}

		// CM_TransformedPointContents :-)
		if( pe->solid == SOLID_BSP && !VectorIsNull( pe->angles ))
		{
			qboolean	transform_bbox = false;
			matrix4x4	matrix;

			if( host.features & ENGINE_TRANSFORM_TRACE_AABB )
			{
				if(( check_angles( pe->angles[0] ) || check_angles( pe->angles[2] )) && pmove->usehull != 2 )
					transform_bbox = true;
                              }

			if( transform_bbox )
				Matrix4x4_CreateFromEntity( matrix, pe->angles, pe->origin, 1.0f );
			else Matrix4x4_CreateFromEntity( matrix, pe->angles, offset, 1.0f );

			Matrix4x4_VectorITransform( matrix, pos, pos_l );

			if( transform_bbox )
			{
				World_TransformAABB( matrix, pmove->player_mins[pmove->usehull], pmove->player_maxs[pmove->usehull], mins, maxs );
				VectorSubtract( hull[0].clip_mins, mins, offset );	// calc new local offset

				for( j = 0; j < 3; j++ )
				{
					if( pos_l[j] >= 0.0f )
						pos_l[j] -= offset[j];
					else pos_l[j] += offset[j];
				}
			}
		}
		else
		{
			// offset the test point appropriately for this hull.
			VectorSubtract( pos, offset, pos_l );
		}

		if( pe->solid == SOLID_CUSTOM )
		{
			Q_memset( &trace, 0, sizeof( trace ));
			VectorCopy( pos, trace.endpos );
			trace.allsolid = true;
			trace.fraction = 1.0f;

			// run custom sweep callback
			if( pmove->server || Host_IsLocalClient( ) || Host_IsDedicated() )
				SV_ClipPMoveToEntity( pe, pos, mins, maxs, pos, &trace );
#ifndef XASH_DEDICATED
			else CL_ClipPMoveToEntity( pe, pos, mins, maxs, pos, &trace );
#endif
			// if we inside the custom hull
			if( trace.allsolid )
				return i;
		}
		else if( hullcount == 1 )
		{
			if( PM_HullPointContents( hull, hull[0].firstclipnode, pos_l ) == CONTENTS_SOLID )
				return i;
		}
		else
		{
			for( j = 0; j < hullcount; j++ )
			{
				if( PM_HullPointContents( &hull[j], hull[j].firstclipnode, pos_l ) == CONTENTS_SOLID )
					return i;
			}
		}
	}

	return -1; // didn't hit anything
}
