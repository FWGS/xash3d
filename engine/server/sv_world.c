/*
sv_world.c - world query functions
Copyright (C) 2008 Uncle Mike

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
#include "server.h"
#include "const.h"
#include "pm_local.h"
#include "studio.h"

typedef struct moveclip_s
{
	vec3_t		boxmins, boxmaxs;	// enclose the test object along entire move
	float		*mins, *maxs;	// size of the moving object
	vec3_t		mins2, maxs2;	// size when clipping against mosnters
	const float	*start, *end;
	edict_t		*passedict;
	trace_t		trace;
	int		type;		// move type
	int		flags;		// trace flags
} moveclip_t;

/*
===============================================================================

HULL BOXES

===============================================================================
*/

static hull_t	box_hull;
static dclipnode_t	box_clipnodes[6];
static mplane_t	box_planes[6];

/*
===================
SV_InitBoxHull

Set up the planes and clipnodes so that the six floats of a bounding box
can just be stored out and get a proper hull_t structure.
===================
*/
void SV_InitBoxHull( void )
{
	int	i, side;

	box_hull.clipnodes = box_clipnodes;
	box_hull.planes = box_planes;
	box_hull.firstclipnode = 0;
	box_hull.lastclipnode = 5;

	for( i = 0; i < 6; i++ )
	{
		box_clipnodes[i].planenum = i;
		
		side = i & 1;
		
		box_clipnodes[i].children[side] = CONTENTS_EMPTY;
		if( i != 5 ) box_clipnodes[i].children[side^1] = i + 1;
		else box_clipnodes[i].children[side^1] = CONTENTS_SOLID;
		
		box_planes[i].type = i>>1;
		box_planes[i].normal[i>>1] = 1;
		box_planes[i].signbits = 0;
	}
	
}

/*
====================
StudioPlayerBlend

====================
*/
void SV_StudioPlayerBlend( mstudioseqdesc_t *pseqdesc, int *pBlend, float *pPitch )
{
	// calc up/down pointing
	*pBlend = (*pPitch * 3);

	if( *pBlend < pseqdesc->blendstart[0] )
	{
		*pPitch -= pseqdesc->blendstart[0] / 3.0f;
		*pBlend = 0;
	}
	else if( *pBlend > pseqdesc->blendend[0] )
	{
		*pPitch -= pseqdesc->blendend[0] / 3.0f;
		*pBlend = 255;
	}
	else
	{
		if( pseqdesc->blendend[0] - pseqdesc->blendstart[0] < 0.1f ) // catch qc error
			*pBlend = 127;
		else *pBlend = 255.0f * (*pBlend - pseqdesc->blendstart[0]) / (pseqdesc->blendend[0] - pseqdesc->blendstart[0]);
		*pPitch = 0;
	}
}

/*
===================
SV_HullForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
hull_t *SV_HullForBox( const vec3_t mins, const vec3_t maxs )
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = mins[0];
	box_planes[2].dist = maxs[1];
	box_planes[3].dist = mins[1];
	box_planes[4].dist = maxs[2];
	box_planes[5].dist = mins[2];

	return &box_hull;
}

/*
==================
SV_HullAutoSelect

select the apropriate hull automatically
==================
*/
hull_t *SV_HullAutoSelect( model_t *model, const vec3_t mins, const vec3_t maxs, const vec3_t size, vec3_t offset )
{
	float	curdiff;
	float	lastdiff = 999;
	int	i, hullNumber = 0;	// assume we fail
	hull_t	*hull;

	// select the hull automatically
	for( i = 0; i < MAX_MAP_HULLS; i++ )
	{
		curdiff = floor( VectorAvg( size )) - floor( VectorAvg( world.hull_sizes[i] ));
		curdiff = fabs( curdiff );

		if( curdiff < lastdiff )
		{
			hullNumber = i;
			lastdiff = curdiff;
		}
	}

	// TraceHull stuff
	hull = &model->hulls[hullNumber];

	// calculate an offset value to center the origin
	// NOTE: never get offset of drawing hull
	if( !hullNumber ) VectorCopy( hull->clip_mins, offset );
	else VectorSubtract( hull->clip_mins, mins, offset );

	return hull;
}

/*
==================
SV_HullForBsp

forcing to select BSP hull
==================
*/
hull_t *SV_HullForBsp( edict_t *ent, const vec3_t mins, const vec3_t maxs, float *offset )
{
	hull_t		*hull;
	model_t		*model;
	vec3_t		size;
			
	// decide which clipping hull to use, based on the size
	model = Mod_Handle( ent->v.modelindex );

	if( !model || model->type != mod_brush )
	{
		Host_MapDesignError( "Entity %i SOLID_BSP with a non bsp model %i\n", NUM_FOR_EDICT( ent ), (model) ? model->type : mod_bad );
		return NULL;
	}

	VectorSubtract( maxs, mins, size );

#ifdef RANDOM_HULL_NULLIZATION
	// author: The FiEctro
	hull = &model->hulls[Com_RandomLong( 0, 0 )];
#endif
	if( sv_quakehulls->integer == 1 )
	{
		// Using quake-style hull select for my Quake remake
		if( size[0] < 3.0f || ( model->flags & MODEL_LIQUID && ent->v.solid != SOLID_TRIGGER ))
			hull = &model->hulls[0];
		else if( size[0] <= 32.0f )
			hull = &model->hulls[1];
		else hull = &model->hulls[2];

		VectorSubtract( hull->clip_mins, mins, offset );
	}
	else if( sv_quakehulls->integer == 2 )
	{
		// undocumented feature: auto hull select
		hull = SV_HullAutoSelect( model, mins, maxs, size, offset );
	}
	else
	{
		if( size[0] <= 8.0f || ( model->flags & MODEL_LIQUID && ent->v.solid != SOLID_TRIGGER ))
		{
			hull = &model->hulls[0];
			VectorCopy( hull->clip_mins, offset ); 
		}
		else
		{
			if( size[0] <= 36.0f )
			{
				if( size[2] <= 36.0f )
					hull = &model->hulls[3];
				else hull = &model->hulls[1];
			}
			else hull = &model->hulls[2];

			VectorSubtract( hull->clip_mins, mins, offset );
		}
	}

	VectorAdd( offset, ent->v.origin, offset );

	return hull;
}

/*
================
SV_HullForEntity

Returns a hull that can be used for testing or clipping an object of mins/maxs
size.
Offset is filled in to contain the adjustment that must be added to the
testing object's origin to get a point to use with the returned hull.
================
*/
hull_t *SV_HullForEntity( edict_t *ent, vec3_t mins, vec3_t maxs, vec3_t offset )
{
	hull_t	*hull;
	vec3_t	hullmins, hullmaxs;

	if( ent->v.solid == SOLID_BSP )
	{
		if( ent->v.movetype != MOVETYPE_PUSH && ent->v.movetype != MOVETYPE_PUSHSTEP )
		{
			Host_MapDesignError( "'%s' has SOLID_BSP without MOVETYPE_PUSH or MOVETYPE_PUSHSTEP\n", SV_ClassName( ent ) );
		}
		hull = SV_HullForBsp( ent, mins, maxs, offset );
	}
	else
	{
		// create a temp hull from bounding box sizes
		VectorSubtract( ent->v.mins, maxs, hullmins );
		VectorSubtract( ent->v.maxs, mins, hullmaxs );
		hull = SV_HullForBox( hullmins, hullmaxs );
		
		VectorCopy( ent->v.origin, offset );
	}

	return hull;
}

/*
====================
SV_HullForStudioModel

====================
*/
hull_t *SV_HullForStudioModel( edict_t *ent, vec3_t mins, vec3_t maxs, vec3_t offset, int *numhitboxes )
{
	int		isPointTrace;
	float		scale = 0.5f;
	hull_t		*hull = NULL;
	vec3_t		size;
	model_t		*mod;

	if(( mod = Mod_Handle( ent->v.modelindex )) == NULL )
	{
		*numhitboxes = 1;
		return SV_HullForEntity( ent, mins, maxs, offset );
	}

	VectorSubtract( maxs, mins, size );
	isPointTrace = false;

	if( VectorIsNull( size ) && !( svgame.globals->trace_flags & FTRACE_SIMPLEBOX ))
	{
		isPointTrace = true;

		if( ent->v.flags & ( FL_CLIENT|FL_FAKECLIENT ))
		{
			if( sv_clienttrace->value == 0.0f )
			{
				// so no way to trace studiomodels by hitboxes
				// use bbox instead
				isPointTrace = false;
			}
			else
			{
				scale = sv_clienttrace->value * 0.5f;
				VectorSet( size, 1.0f, 1.0f, 1.0f );
			}
		}
	}

	if( mod->flags & STUDIO_TRACE_HITBOX || isPointTrace )
	{
		VectorScale( size, scale, size );
		VectorClear( offset );

		if( ent->v.flags & ( FL_CLIENT|FL_FAKECLIENT ))
		{
			studiohdr_t	*pstudio;
			mstudioseqdesc_t	*pseqdesc;
			byte		controller[4];
			byte		blending[2];
			vec3_t		angles;
			int		iBlend;

			pstudio = Mod_Extradata( mod );
			pseqdesc = (mstudioseqdesc_t *)((byte *)pstudio + pstudio->seqindex) + ent->v.sequence;
			VectorCopy( ent->v.angles, angles );

			SV_StudioPlayerBlend( pseqdesc, &iBlend, &angles[PITCH] );

			controller[0] = controller[1] = 0x7F;
			controller[2] = controller[3] = 0x7F;
			blending[0] = (byte)iBlend;
			blending[1] = 0;

			hull = Mod_HullForStudio( mod, ent->v.frame, ent->v.sequence, angles, ent->v.origin, size, controller, blending, numhitboxes, ent );
		}
		else
		{
			hull = Mod_HullForStudio( mod, ent->v.frame, ent->v.sequence, ent->v.angles, ent->v.origin, size, ent->v.controller, ent->v.blending, numhitboxes, ent );
		}
	}

	if( hull ) return hull;

	*numhitboxes = 1;
	return SV_HullForEntity( ent, mins, maxs, offset );
}

/*
===============================================================================

ENTITY AREA CHECKING

===============================================================================
*/
static int	iTouchLinkSemaphore = 0;	// prevent recursion when SV_TouchLinks is active
areanode_t	sv_areanodes[AREA_NODES];
static int	sv_numareanodes;

/*
===============
SV_CreateAreaNode

builds a uniformly subdivided tree for the given world size
===============
*/
areanode_t *SV_CreateAreaNode( int depth, vec3_t mins, vec3_t maxs )
{
	areanode_t	*anode;
	vec3_t		size;
	vec3_t		mins1, maxs1;
	vec3_t		mins2, maxs2;

	anode = &sv_areanodes[sv_numareanodes++];

	ClearLink( &anode->trigger_edicts );
	ClearLink( &anode->solid_edicts );
	ClearLink( &anode->water_edicts );
	
	if( depth == AREA_DEPTH )
	{
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;
		return anode;
	}
	
	VectorSubtract( maxs, mins, size );
	if( size[0] > size[1] )
		anode->axis = 0;
	else anode->axis = 1;
	
	anode->dist = 0.5f * ( maxs[anode->axis] + mins[anode->axis] );
	VectorCopy( mins, mins1 );	
	VectorCopy( mins, mins2 );	
	VectorCopy( maxs, maxs1 );	
	VectorCopy( maxs, maxs2 );	
	
	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;
	anode->children[0] = SV_CreateAreaNode( depth+1, mins2, maxs2 );
	anode->children[1] = SV_CreateAreaNode( depth+1, mins1, maxs1 );

	return anode;
}

/*
===============
SV_ClearWorld

===============
*/
void SV_ClearWorld( void )
{
	int	i;

	SV_InitBoxHull(); // for box testing

	// clear lightstyles
	for( i = 0; i < MAX_LIGHTSTYLES; i++ )
	{
		sv.lightstyles[i].value = 256.0f;
		sv.lightstyles[i].time = 0.0f;
	}

	Q_memset( sv_areanodes, 0, sizeof( sv_areanodes ));
	iTouchLinkSemaphore = 0;
	sv_numareanodes = 0;

	SV_CreateAreaNode( 0, sv.worldmodel->mins, sv.worldmodel->maxs );
}

/*
===============
SV_UnlinkEdict
===============
*/
void SV_UnlinkEdict( edict_t *ent )
{
	// not linked in anywhere
	if( !ent->area.prev ) return;

	RemoveLink( &ent->area );
	ent->area.prev = NULL;
	ent->area.next = NULL;
}

/*
====================
SV_TouchLinks
====================
*/
void SV_TouchLinks( edict_t *ent, areanode_t *node )
{
	link_t	*l, *next;
	edict_t	*touch;
	hull_t	*hull;
	vec3_t	test, offset;

	// touch linked edicts
	for( l = node->trigger_edicts.next; l != &node->trigger_edicts; l = next )
	{
		next = l->next;
		touch = (edict_t *)((byte *)l - ADDRESS_OF_AREA);

		if( svgame.physFuncs.SV_TriggerTouch != NULL )
		{
			// user dll can override trigger checking (Xash3D extension)
			if( !svgame.physFuncs.SV_TriggerTouch( ent, touch ))
				continue;
		}
		else
		{
			if( touch == ent || touch->v.solid != SOLID_TRIGGER ) // disabled ?
				continue;

			if( touch->v.groupinfo && ent->v.groupinfo )
			{
				if(( !svs.groupop && !(touch->v.groupinfo & ent->v.groupinfo)) ||
				(svs.groupop == 1 && (touch->v.groupinfo & ent->v.groupinfo)))
					continue;
			}

			if( !BoundsIntersect( ent->v.absmin, ent->v.absmax, touch->v.absmin, touch->v.absmax ))
				continue;

			// check brush triggers accuracy
			if( Mod_GetType( touch->v.modelindex ) == mod_brush )
			{
				model_t *mod = Mod_Handle( touch->v.modelindex );

				// force to select bsp-hull
				hull = SV_HullForBsp( touch, ent->v.mins, ent->v.maxs, offset );

				// support for rotational triggers
				if(( mod->flags & MODEL_HAS_ORIGIN ) && !VectorIsNull( touch->v.angles ))
				{
					matrix4x4	matrix;
					Matrix4x4_CreateFromEntity( matrix, touch->v.angles, offset, 1.0f );
					Matrix4x4_VectorITransform( matrix, ent->v.origin, test );
				}
				else
				{
					// offset the test point appropriately for this hull.
					VectorSubtract( ent->v.origin, offset, test );
				}

				// test hull for intersection with this model
				if( PM_HullPointContents( hull, hull->firstclipnode, test ) != CONTENTS_SOLID )
					continue;
			}
		}

		// never touch the triggers when "playersonly" is active
		if( !( sv.hostflags & SVF_PLAYERSONLY ))
		{
			svgame.globals->time = sv.time;
			svgame.dllFuncs.pfnTouch( touch, ent );
		}
	}
	
	// recurse down both sides
	if( node->axis == -1 ) return;
	
	if( ent->v.absmax[node->axis] > node->dist )
		SV_TouchLinks( ent, node->children[0] );
	if( ent->v.absmin[node->axis] < node->dist )
		SV_TouchLinks( ent, node->children[1] );
}

/*
===============
SV_FindTouchedLeafs

===============
*/
void SV_FindTouchedLeafs( edict_t *ent, mnode_t *node, int *headnode )
{
	mplane_t	*splitplane;
	int	sides, leafnum;
	mleaf_t	*leaf;

	if( !node ) // if no collision model
		return;

	if( node->contents == CONTENTS_SOLID )
		return;
	
	// add an efrag if the node is a leaf
	if(  node->contents < 0 )
	{
		if( ent->num_leafs > ( MAX_ENT_LEAFS - 1 ))
		{
			// continue counting leafs,
			// so we know how many it's overrun
			ent->num_leafs = (MAX_ENT_LEAFS + 1);
		}
		else
		{
			leaf = (mleaf_t *)node;
			leafnum = leaf - sv.worldmodel->leafs - 1;
			ent->leafnums[ent->num_leafs] = leafnum;
			ent->num_leafs++;			
		}
		return;
	}
	
	// NODE_MIXED
	splitplane = node->plane;
	sides = BOX_ON_PLANE_SIDE( ent->v.absmin, ent->v.absmax, splitplane );

	if( sides == 3 && *headnode == -1 )
		*headnode = node - sv.worldmodel->nodes;
	
	// recurse down the contacted sides
	if( sides & 1 ) SV_FindTouchedLeafs( ent, node->children[0], headnode );
	if( sides & 2 ) SV_FindTouchedLeafs( ent, node->children[1], headnode );
}

/*
=============
SV_HeadnodeVisible
=============
*/
qboolean SV_HeadnodeVisible( mnode_t *node, byte *visbits, int *lastleaf )
{
	int	leafnum;

	if( !node || node->contents == CONTENTS_SOLID )
		return false;

	if( node->contents < 0 )
	{
		leafnum = ((mleaf_t *)node - sv.worldmodel->leafs) - 1;

		if(!( visbits[leafnum >> 3] & (1U << ( leafnum & 7 ))))
			return false;

		if( lastleaf )
			*lastleaf = leafnum;
		return true;
	}

	if( SV_HeadnodeVisible( node->children[0], visbits, lastleaf ))
		return true;

	return SV_HeadnodeVisible( node->children[1], visbits, lastleaf );
}

/*
===============
SV_LinkEdict
===============
*/
void SV_LinkEdict( edict_t *ent, qboolean touch_triggers )
{
	areanode_t	*node;
	int		headnode;

	if( ent->area.prev ) SV_UnlinkEdict( ent );	// unlink from old position
	if( ent == svgame.edicts ) return;		// don't add the world
	if( !SV_IsValidEdict( ent )) return;		// never add freed ents

	// set the abs box
	svgame.dllFuncs.pfnSetAbsBox( ent );

	if( ent->v.movetype == MOVETYPE_FOLLOW && SV_IsValidEdict( ent->v.aiment ))
	{
		ent->headnode = ent->v.aiment->headnode;
		ent->num_leafs = ent->v.aiment->num_leafs;
		Q_memcpy( ent->leafnums, ent->v.aiment->leafnums, sizeof( ent->leafnums ));
	}
	else
	{
		// link to PVS leafs
		ent->num_leafs = 0;
		ent->headnode = -1;
		headnode = -1;

		if( ent->v.modelindex )
			SV_FindTouchedLeafs( ent, sv.worldmodel->nodes, &headnode );

		if( ent->num_leafs > MAX_ENT_LEAFS )
		{
			Q_memset( ent->leafnums, -1, sizeof( ent->leafnums ));
			ent->num_leafs = 0;	// so we use headnode instead
			ent->headnode = headnode;
		}
	}

	// ignore non-solid bodies
	if( ent->v.solid == SOLID_NOT && ent->v.skin >= CONTENTS_EMPTY )
		return;

	// find the first node that the ent's box crosses
	node = sv_areanodes;

	while( 1 )
	{
		if( node->axis == -1 ) break;
		if( ent->v.absmin[node->axis] > node->dist )
			node = node->children[0];
		else if( ent->v.absmax[node->axis] < node->dist )
			node = node->children[1];
		else break; // crosses the node
	}
	
	// link it in	
	if( ent->v.solid == SOLID_TRIGGER )
		InsertLinkBefore( &ent->area, &node->trigger_edicts );
	else if( ent->v.solid == SOLID_NOT && ent->v.skin < CONTENTS_EMPTY )
		InsertLinkBefore( &ent->area, &node->water_edicts );
	else InsertLinkBefore( &ent->area, &node->solid_edicts );

	if( touch_triggers && !iTouchLinkSemaphore )
	{
		iTouchLinkSemaphore = true;
		SV_TouchLinks( ent, sv_areanodes );
		iTouchLinkSemaphore = false;
	}
}

/*
===============================================================================

POINT TESTING IN HULLS

===============================================================================
*/
void SV_WaterLinks( const vec3_t origin, int *pCont, areanode_t *node )
{
	link_t	*l, *next;
	edict_t	*touch;
	hull_t	*hull;
	vec3_t	test, offset;
	model_t	*mod;

	Q_memset( offset, 0, 3 * sizeof( float ) );

	// get water edicts
	for( l = node->water_edicts.next; l != &node->water_edicts; l = next )
	{
		next = l->next;
		touch = (edict_t *)((byte *)l - ADDRESS_OF_AREA);

		if( touch->v.solid != SOLID_NOT ) // disabled ?
			continue;

		if( touch->v.groupinfo )
		{
			if(( !svs.groupop && !(touch->v.groupinfo & svs.groupmask)) ||
			(svs.groupop == 1 && (touch->v.groupinfo & svs.groupmask)))
				continue;
		}

		// only brushes can have special contents
		if( Mod_GetType( touch->v.modelindex ) != mod_brush )
			continue;

		if( !BoundsIntersect( origin, origin, touch->v.absmin, touch->v.absmax ))
			continue;

		mod = Mod_Handle( touch->v.modelindex );

		// check water brushes accuracy
		if( Mod_GetType( touch->v.modelindex ) == mod_brush )
			hull = SV_HullForBsp( touch, vec3_origin, vec3_origin, offset );
		else
		{
			Host_MapDesignError( "Water must have BSP model!\n" );
			hull = SV_HullForBox( touch->v.mins, touch->v.maxs );
		}

		// support for rotational water
		if(( mod->flags & MODEL_HAS_ORIGIN ) && !VectorIsNull( touch->v.angles ))
		{
			matrix4x4	matrix;
			Matrix4x4_CreateFromEntity( matrix, touch->v.angles, offset, 1.0f );
			Matrix4x4_VectorITransform( matrix, origin, test );
		}
		else
		{
			// offset the test point appropriately for this hull.
			VectorSubtract( origin, offset, test );
		}

		// test hull for intersection with this model
		if( PM_HullPointContents( hull, hull->firstclipnode, test ) == CONTENTS_EMPTY )
			continue;

		// compare contents ranking
		if( RankForContents( touch->v.skin ) > RankForContents( *pCont ))
			*pCont = touch->v.skin; // new content has more priority
	}
	
	// recurse down both sides
	if( node->axis == -1 ) return;
	
	if( origin[node->axis] > node->dist )
		SV_WaterLinks( origin, pCont, node->children[0] );
	if( origin[node->axis] < node->dist )
		SV_WaterLinks( origin, pCont, node->children[1] );
}

/*
=============
SV_TruePointContents

=============
*/
int SV_TruePointContents( const vec3_t p )
{
	int	cont;

	// sanity check
	if( !p ) return CONTENTS_NONE;

	// get base contents from world
	cont = PM_HullPointContents( &sv.worldmodel->hulls[0], 0, p );

	// check all water entities
	SV_WaterLinks( p, &cont, sv_areanodes );

	return cont;
}

/*
=============
SV_PointContents

=============
*/
int SV_PointContents( const vec3_t p )
{
	int cont = SV_TruePointContents( p );

	if( cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN )
		cont = CONTENTS_WATER;
	return cont;
}

//===========================================================================

/*
============
SV_TestEntityPosition

returns true if the entity is in solid currently
============
*/
qboolean SV_TestEntityPosition( edict_t *ent, edict_t *blocker )
{
	trace_t	trace;

	if( ent->v.flags & (FL_CLIENT|FL_FAKECLIENT))
	{
		// to avoid falling through tracktrain update client mins\maxs here
		if( ent->v.flags & FL_DUCKING ) 
			SV_SetMinMaxSize( ent, svgame.pmove->player_mins[1], svgame.pmove->player_maxs[1] );
		else SV_SetMinMaxSize( ent, svgame.pmove->player_mins[0], svgame.pmove->player_maxs[0] );
	}

	trace = SV_Move( ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, MOVE_NORMAL|FMOVE_SIMPLEBOX, ent );

	if( SV_IsValidEdict( blocker ) && SV_IsValidEdict( trace.ent ))
	{
		if( trace.ent->v.movetype == MOVETYPE_PUSH || trace.ent == blocker )
			return trace.startsolid;
		return false;
	}
	return trace.startsolid;
}

/*
===============================================================================

LINE TESTING IN HULLS

===============================================================================
*/

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
SV_RecursiveHullCheck

==================
*/
qboolean SV_RecursiveHullCheck( hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace )
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

	if( num < hull->firstclipnode || num > hull->lastclipnode )
		Host_Error( "SV_RecursiveHullCheck: bad node number\n" );

	if( !hull->clipnodes )
		return false;
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
	
	if( t1 >= 0 && t2 >= 0 )
		return SV_RecursiveHullCheck( hull, node->children[0], p1f, p2f, p1, p2, trace );
	if( t1 < 0 && t2 < 0 )
		return SV_RecursiveHullCheck( hull, node->children[1], p1f, p2f, p1, p2, trace );

	ASSERTNAN( t1 )
	ASSERTNAN( t2 )

	// put the crosspoint DIST_EPSILON pixels on the near side
	if( t1 < 0 ) frac = ( t1 + DIST_EPSILON ) / ( t1 - t2 );
	else frac = ( t1 - DIST_EPSILON ) / ( t1 - t2 );

	ASSERTNAN( frac )

	if( frac < 0.0f ) frac = 0.0f;
	if( frac > 1.0f ) frac = 1.0f;
		
	midf = p1f + ( p2f - p1f ) * frac;
	VectorLerp( p1, frac, p2, mid );

	side = (t1 < 0);

	ASSERTNAN( frac )

	// move up to the node
	if( !SV_RecursiveHullCheck( hull, node->children[side], p1f, midf, p1, mid, trace ))
		return false;

	if( PM_HullPointContents( hull, node->children[side^1], mid ) != CONTENTS_SOLID )
	{
		// go past the node
		return SV_RecursiveHullCheck (hull, node->children[side^1], midf, p2f, mid, p2, trace);
	}	

	if( trace->allsolid )
		return false; // never got out of the solid area
		
	//==================
	// the other side of the node is solid, this is the impact point
	//==================
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

		if( ( frac < 0.0f ) || IS_NAN( frac ) )
		{
			trace->fraction = midf;
			VectorCopy( mid, trace->endpos );
			MsgDev( D_WARN, "trace backed up past 0.0\n" );
			return false;
		}

		midf = p1f + (p2f - p1f) * frac;
		VectorLerp( p1, frac, p2, mid );
	}

	trace->fraction = midf;
	VectorCopy( mid, trace->endpos );

	return false;
}

/*
==================
SV_ClipMoveToEntity

Handles selection or creation of a clipping hull, and offseting (and
eventually rotation) of the end points
==================
*/
void SV_ClipMoveToEntity( edict_t *ent, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, trace_t *trace )
{
	hull_t	*hull = NULL;
	model_t	*model;
	vec3_t	start_l, end_l;
	vec3_t	offset, temp;
	int	last_hitgroup;
	trace_t	trace_hitbox;
	int	i, j, hullcount;
	qboolean	rotated, transform_bbox;
	matrix4x4	matrix;

	Q_memset( trace, 0, sizeof( trace_t ));
	VectorCopy( end, trace->endpos );
	trace->fraction = 1.0f;
	trace->allsolid = 1;

	model = Mod_Handle( ent->v.modelindex );

	if( model && model->type == mod_studio )
	{
		hull = SV_HullForStudioModel( ent, mins, maxs, offset, &hullcount );
	}
	else
	{
		hull = SV_HullForEntity( ent, mins, maxs, offset );
		hullcount = 1;
	}

	// prevent crash on incorrect hull
	if( !hull )
		return;

	// rotate start and end into the models frame of reference
	if( ent->v.solid == SOLID_BSP && !VectorIsNull( ent->v.angles ))
		rotated = true;
	else rotated = false;

	if( host.features & ENGINE_TRANSFORM_TRACE_AABB )
	{
		// keep untransformed bbox less than 45 degress or train on subtransit.bsp will stop working
		if(( check_angles( ent->v.angles[0] ) || check_angles( ent->v.angles[2] )) && !VectorIsNull( mins ))
			transform_bbox = true;
		else transform_bbox = false;
	}
	else transform_bbox = false;

	if( rotated )
	{
		vec3_t	out_mins, out_maxs;

		if( transform_bbox )
			Matrix4x4_CreateFromEntity( matrix, ent->v.angles, ent->v.origin, 1.0f );
		else Matrix4x4_CreateFromEntity( matrix, ent->v.angles, offset, 1.0f );

		Matrix4x4_VectorITransform( matrix, start, start_l );
		Matrix4x4_VectorITransform( matrix, end, end_l );

		if( transform_bbox )
		{
			World_TransformAABB( matrix, mins, maxs, out_mins, out_maxs );
			VectorSubtract( hull[0].clip_mins, out_mins, offset ); // calc new local offset

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

	if( hullcount == 1 )
	{
		SV_RecursiveHullCheck( hull, hull->firstclipnode, 0.0f, 1.0f, start_l, end_l, trace );
	}
	else
	{
		last_hitgroup = 0;

		for( i = 0; i < hullcount; i++ )
		{
			Q_memset( &trace_hitbox, 0, sizeof( trace_t ));
			VectorCopy( end, trace_hitbox.endpos );
			trace_hitbox.fraction = 1.0;
			trace_hitbox.allsolid = 1;

			SV_RecursiveHullCheck( &hull[i], hull[i].firstclipnode, 0.0f, 1.0f, start_l, end_l, &trace_hitbox );

			if( i == 0 || trace_hitbox.allsolid || trace_hitbox.startsolid || trace_hitbox.fraction < trace->fraction )
			{
				if( trace->startsolid )
				{
					*trace = trace_hitbox;
					trace->startsolid = true;
				}
				else *trace = trace_hitbox;

				last_hitgroup = i;
			}
		}

		trace->hitgroup = Mod_HitgroupForStudioHull( last_hitgroup );
	}

	if( trace->fraction != 1.0f )
	{
		// compute endpos (generic case)
		VectorLerp( start, trace->fraction, end, trace->endpos );

		if( rotated )
		{
			// transform plane
			VectorCopy( trace->plane.normal, temp );
			Matrix4x4_TransformPositivePlane( matrix, temp, trace->plane.dist, trace->plane.normal, &trace->plane.dist );
		}
		else
		{
			trace->plane.dist = DotProduct( trace->endpos, trace->plane.normal );
		}
	}

	if( trace->fraction < 1.0f || trace->startsolid )
		trace->ent = ent;
}

/*
==================
SV_CustomClipMoveToEntity

A part of physics engine implementation
or custom physics implementation
==================
*/
void SV_CustomClipMoveToEntity( edict_t *ent, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, trace_t *trace )
{
	Q_memset( trace, 0, sizeof( trace_t ));
	VectorCopy( end, trace->endpos );
	trace->allsolid = true;
	trace->fraction = 1.0f;

	if( svgame.physFuncs.ClipMoveToEntity != NULL )
	{
		// do custom sweep test
		svgame.physFuncs.ClipMoveToEntity( ent, start, mins, maxs, end, trace );
	}
	else
	{
		// function is missed, so we didn't hit anything
		trace->allsolid = false;
	}
}

/*
====================
SV_ClipToLinks

Mins and maxs enclose the entire area swept by the move
====================
*/
static void SV_ClipToLinks( areanode_t *node, moveclip_t *clip )
{
	link_t	*l, *next;
	edict_t	*touch;
	trace_t	trace;

	// touch linked edicts
	for( l = node->solid_edicts.next; l != &node->solid_edicts; l = next )
	{
		next = l->next;

		touch = (edict_t *)((byte *)l - ADDRESS_OF_AREA);

		if( touch->v.groupinfo != 0 && SV_IsValidEdict( clip->passedict ) && clip->passedict->v.groupinfo != 0 )
		{
			if(( svs.groupop == 0 && ( touch->v.groupinfo & clip->passedict->v.groupinfo ) == 0) ||
			( svs.groupop == 1 && (touch->v.groupinfo & clip->passedict->v.groupinfo ) != 0 ))
				continue;
		}

		if( touch == clip->passedict || touch->v.solid == SOLID_NOT )
			continue;

		if( touch->v.solid == SOLID_TRIGGER )
		{
			Host_MapDesignError( "trigger in clipping list\n" );
			touch->v.solid = SOLID_NOT;
		}

		// custom user filter
		if( svgame.dllFuncs2.pfnShouldCollide )
		{
			if( !svgame.dllFuncs2.pfnShouldCollide( touch, clip->passedict ))
				continue;	// originally this was 'return' but is completely wrong!
		}

		// monsterclip filter (solid custom is a static or dynamic bodies)
		if( touch->v.solid == SOLID_BSP || touch->v.solid == SOLID_CUSTOM )
		{
			if( touch->v.flags & FL_MONSTERCLIP )
			{
				// func_monsterclip works only with monsters that have same flag!
				if( !SV_IsValidEdict( clip->passedict ) || !( clip->passedict->v.flags & FL_MONSTERCLIP ))
					continue;
			}
		}
		else
		{
			// ignore all monsters but pushables
			if( clip->type == MOVE_NOMONSTERS && touch->v.movetype != MOVETYPE_PUSHSTEP )
				continue;
		}

		if( Mod_GetType( touch->v.modelindex ) == mod_brush && clip->flags & FMOVE_IGNORE_GLASS )
		{
			// we ignore brushes with rendermode != kRenderNormal and without FL_WORLDBRUSH set
			if( touch->v.rendermode != kRenderNormal && !( touch->v.flags & FL_WORLDBRUSH ))
				continue;
		}

		if( !BoundsIntersect( clip->boxmins, clip->boxmaxs, touch->v.absmin, touch->v.absmax ))
			continue;

		// Xash3D extension
		if( SV_IsValidEdict( clip->passedict ) && clip->passedict->v.solid == SOLID_TRIGGER )
		{
			// never collide items and player (because call "give" always stuck item in player
			// and total trace returns fail (old half-life bug)
			// items touch should be done in SV_TouchLinks not here
			if( touch->v.flags & ( FL_CLIENT|FL_FAKECLIENT ))
				continue;
		}

		// g-cont. make sure what size is really zero - check all the components
		if( SV_IsValidEdict( clip->passedict ) && !VectorIsNull( clip->passedict->v.size ) && VectorIsNull( touch->v.size ))
			continue;	// points never interact

		// might intersect, so do an exact clip
		if( clip->trace.allsolid ) return;

		if( SV_IsValidEdict( clip->passedict ))
		{
		 	if( touch->v.owner == clip->passedict )
				continue;	// don't clip against own missiles
			if( clip->passedict->v.owner == touch )
				continue;	// don't clip against owner
		}

		if( touch->v.solid == SOLID_CUSTOM )
			SV_CustomClipMoveToEntity( touch, clip->start, clip->mins, clip->maxs, clip->end, &trace );
		else if( touch->v.flags & FL_MONSTER )
			SV_ClipMoveToEntity( touch, clip->start, clip->mins2, clip->maxs2, clip->end, &trace );
		else SV_ClipMoveToEntity( touch, clip->start, clip->mins, clip->maxs, clip->end, &trace );

		clip->trace = World_CombineTraces( &clip->trace, &trace, touch );
	}
	
	// recurse down both sides
	if( node->axis == -1 ) return;

	if( clip->boxmaxs[node->axis] > node->dist )
		SV_ClipToLinks( node->children[0], clip );
	if( clip->boxmins[node->axis] < node->dist )
		SV_ClipToLinks( node->children[1], clip );
}

/*
====================
SV_ClipToWorldBrush

Mins and maxs enclose the entire area swept by the move
====================
*/
void SV_ClipToWorldBrush( areanode_t *node, moveclip_t *clip )
{
	link_t	*l, *next;
	edict_t	*touch;
	trace_t	trace;

	for( l = node->solid_edicts.next; l != &node->solid_edicts; l = next )
	{
		next = l->next;

		touch = (edict_t *)((byte *)l - ADDRESS_OF_AREA);

		if( touch->v.solid != SOLID_BSP || touch == clip->passedict || !( touch->v.flags & FL_WORLDBRUSH ))
			continue;

		if( !BoundsIntersect( clip->boxmins, clip->boxmaxs, touch->v.absmin, touch->v.absmax ))
			continue;

		if( clip->trace.allsolid ) return;

		SV_ClipMoveToEntity( touch, clip->start, clip->mins, clip->maxs, clip->end, &trace );

		clip->trace = World_CombineTraces( &clip->trace, &trace, touch );
	}

	// recurse down both sides
	if( node->axis == -1 ) return;

	if( clip->boxmaxs[node->axis] > node->dist )
		SV_ClipToWorldBrush( node->children[0], clip );

	if( clip->boxmins[node->axis] < node->dist )
		SV_ClipToWorldBrush( node->children[1], clip );
}

/*
==================
SV_Move
==================
*/
trace_t SV_Move( const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, int type, edict_t *e )
{
	moveclip_t	clip;
	vec3_t		trace_endpos;
	float		trace_fraction;

	Q_memset( &clip, 0, sizeof( moveclip_t ));
	SV_ClipMoveToEntity( EDICT_NUM( 0 ), start, mins, maxs, end, &clip.trace );

	if( clip.trace.fraction != 0.0f )
	{
		VectorCopy( clip.trace.endpos, trace_endpos );
		trace_fraction = clip.trace.fraction;
		clip.trace.fraction = 1.0f;
		clip.start = start;
		clip.end = trace_endpos;
		clip.type = (type & 0xFF);
		clip.flags = (type & 0xFF00);
		clip.passedict = (e) ? e : EDICT_NUM( 0 );
		clip.mins = mins;
		clip.maxs = maxs;

		if( clip.type == MOVE_MISSILE )
		{
			VectorSet( clip.mins2, -15.0f, -15.0f, -15.0f );
			VectorSet( clip.maxs2,  15.0f,  15.0f,  15.0f );
		}
		else
		{
			VectorCopy( mins, clip.mins2 );
			VectorCopy( maxs, clip.maxs2 );
		}

		World_MoveBounds( start, clip.mins2, clip.maxs2, trace_endpos, clip.boxmins, clip.boxmaxs );
		SV_ClipToLinks( sv_areanodes, &clip );

		clip.trace.fraction *= trace_fraction;
		svgame.globals->trace_ent = clip.trace.ent;
	}

	SV_CopyTraceToGlobal( &clip.trace );

	return clip.trace;
}

/*
==================
SV_MoveNoEnts
==================
*/
trace_t SV_MoveNoEnts( const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, int type, edict_t *e )
{
	moveclip_t	clip;
	vec3_t		trace_endpos;
	float		trace_fraction;

	Q_memset( &clip, 0, sizeof( moveclip_t ));
	SV_ClipMoveToEntity( EDICT_NUM( 0 ), start, mins, maxs, end, &clip.trace );

	if( clip.trace.fraction != 0.0f )
	{
		VectorCopy( clip.trace.endpos, trace_endpos );
		trace_fraction = clip.trace.fraction;
		clip.trace.fraction = 1.0f;
		clip.start = start;
		clip.end = trace_endpos;
		clip.type = (type & 0xFF);
		clip.flags = (type & 0xFF00);
		clip.passedict = (e) ? e : EDICT_NUM( 0 );
		clip.mins = mins;
		clip.maxs = maxs;

		VectorCopy( mins, clip.mins2 );
		VectorCopy( maxs, clip.maxs2 );

		World_MoveBounds( start, clip.mins2, clip.maxs2, trace_endpos, clip.boxmins, clip.boxmaxs );
		SV_ClipToWorldBrush( sv_areanodes, &clip );

		clip.trace.fraction *= trace_fraction;
		svgame.globals->trace_ent = clip.trace.ent;
	}

	SV_CopyTraceToGlobal( &clip.trace );

	return clip.trace;
}

/*
==================
SV_TraceSurface

find the face where the traceline hit
assume pTextureEntity is valid
==================
*/
msurface_t *SV_TraceSurface( edict_t *ent, const vec3_t start, const vec3_t end )
{
	matrix4x4		matrix;
	model_t		*bmodel;
	hull_t		*hull;
	vec3_t		start_l, end_l;
	vec3_t		offset;

	bmodel = Mod_Handle( ent->v.modelindex );
	if( !bmodel || bmodel->type != mod_brush )
		return NULL;

	hull = SV_HullForBsp( ent, vec3_origin, vec3_origin, offset );

	VectorSubtract( start, offset, start_l );
	VectorSubtract( end, offset, end_l );

	// rotate start and end into the models frame of reference
	if( !VectorIsNull( ent->v.angles ))
	{
		Matrix4x4_CreateFromEntity( matrix, ent->v.angles, offset, 1.0f );
		Matrix4x4_VectorITransform( matrix, start, start_l );
		Matrix4x4_VectorITransform( matrix, end, end_l );
	}

	return PM_RecursiveSurfCheck( bmodel, &bmodel->nodes[hull->firstclipnode], start_l, end_l );
}

/*
==================
SV_TraceTexture

find the face where the traceline hit
assume pTextureEntity is valid
==================
*/
const char *SV_TraceTexture( edict_t *ent, const vec3_t start, const vec3_t end )
{
	msurface_t	*surf;
	matrix4x4		matrix;
	model_t		*bmodel;
	hull_t		*hull;
	vec3_t		start_l, end_l;
	vec3_t		offset;

	bmodel = Mod_Handle( ent->v.modelindex );
	if( !bmodel || bmodel->type != mod_brush )
		return NULL;

	hull = SV_HullForBsp( ent, vec3_origin, vec3_origin, offset );

	VectorSubtract( start, offset, start_l );
	VectorSubtract( end, offset, end_l );

	// rotate start and end into the models frame of reference
	if( !VectorIsNull( ent->v.angles ))
	{
		Matrix4x4_CreateFromEntity( matrix, ent->v.angles, offset, 1.0f );
		Matrix4x4_VectorITransform( matrix, start, start_l );
		Matrix4x4_VectorITransform( matrix, end, end_l );
	}

	surf = PM_RecursiveSurfCheck( bmodel, &bmodel->nodes[hull->firstclipnode], start_l, end_l );

	if( !surf || !surf->texinfo || !surf->texinfo->texture )
		return NULL;

	return surf->texinfo->texture->name;
}

/*
==================
SV_MoveToss
==================
*/
trace_t SV_MoveToss( edict_t *tossent, edict_t *ignore )
{
	float 	gravity;
	vec3_t	move, end;
	vec3_t	original_origin;
	vec3_t	original_velocity;
	vec3_t	original_angles;
	vec3_t	original_avelocity;
	trace_t	trace;
	int	i;

	Q_memset( &trace, 0, sizeof( trace_t ) );

	VectorCopy( tossent->v.origin, original_origin );
	VectorCopy( tossent->v.velocity, original_velocity );
	VectorCopy( tossent->v.angles, original_angles );
	VectorCopy( tossent->v.avelocity, original_avelocity );
	gravity = tossent->v.gravity * svgame.movevars.gravity * 0.05f;

	for( i = 0; i < 200; i++ )
	{
		SV_CheckVelocity( tossent );
		tossent->v.velocity[2] -= gravity;
		VectorMA( tossent->v.angles, 0.05f, tossent->v.avelocity, tossent->v.angles );
		VectorScale( tossent->v.velocity, 0.05f, move );
		VectorAdd( tossent->v.origin, move, end );
		trace = SV_Move( tossent->v.origin, tossent->v.mins, tossent->v.maxs, end, MOVE_NORMAL, tossent );
		VectorCopy( trace.endpos, tossent->v.origin );
		if( trace.fraction < 1.0f ) break;
	}

	VectorCopy( original_origin, tossent->v.origin );
	VectorCopy( original_velocity, tossent->v.velocity );
	VectorCopy( original_angles, tossent->v.angles );
	VectorCopy( original_avelocity, tossent->v.avelocity );

	return trace;
}

/*
===============================================================================

	LIGHTING INFO

===============================================================================
*/

static vec3_t	sv_pointColor;

/*
=================
SV_RecursiveLightPoint
=================
*/
static qboolean SV_RecursiveLightPoint( model_t *model, mnode_t *node, const vec3_t start, const vec3_t end )
{
	int		side;
	mplane_t		*plane;
	msurface_t	*surf;
	mtexinfo_t	*tex;
	float		front, back, scale, frac;
	int		i, map, size, s, t;
	color24		*lm;
	vec3_t		mid;

	// didn't hit anything
	if( node->contents < 0 )
		return false;

	// calculate mid point
	plane = node->plane;
	if( plane->type < 3 )
	{
		front = start[plane->type] - plane->dist;
		back = end[plane->type] - plane->dist;
	}
	else
	{
		front = DotProduct( start, plane->normal ) - plane->dist;
		back = DotProduct( end, plane->normal ) - plane->dist;
	}

	side = front < 0.0f;
	if(( back < 0.0f ) == side )
		return SV_RecursiveLightPoint( model, node->children[side], start, end );

	frac = front / ( front - back );

	VectorLerp( start, frac, end, mid );

	// co down front side	
	if( SV_RecursiveLightPoint( model, node->children[side], start, mid ))
		return true; // hit something

	if(( back < 0.0f ) == side )
		return false;// didn't hit anything

	// check for impact on this node
	surf = model->surfaces + node->firstsurface;

	for( i = 0; i < node->numsurfaces; i++, surf++ )
	{
		tex = surf->texinfo;

		if( surf->flags & SURF_DRAWTILED )
			continue;	// no lightmaps

		s = DotProduct( mid, tex->vecs[0] ) + tex->vecs[0][3] - surf->texturemins[0];
		t = DotProduct( mid, tex->vecs[1] ) + tex->vecs[1][3] - surf->texturemins[1];

		if(( s < 0.0f || s > surf->extents[0] ) || ( t < 0.0f || t > surf->extents[1] ))
			continue;

		s /= LM_SAMPLE_SIZE;
		t /= LM_SAMPLE_SIZE;

		if( !surf->samples )
			return true;

		VectorClear( sv_pointColor );

		lm = surf->samples + (t * ((surf->extents[0] / LM_SAMPLE_SIZE) + 1) + s);
		size = ((surf->extents[0] / LM_SAMPLE_SIZE) + 1) * ((surf->extents[1] / LM_SAMPLE_SIZE) + 1);

		for( map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++ )
		{
			scale = sv.lightstyles[surf->styles[map]].value;

			sv_pointColor[0] += lm->r * scale;
			sv_pointColor[1] += lm->g * scale;
			sv_pointColor[2] += lm->b * scale;

			lm += size; // skip to next lightmap
		}
		return true;
	}

	// go down back side
	return SV_RecursiveLightPoint( model, node->children[!side], mid, end );
}

void SV_RunLightStyles( void )
{
	int		i, ofs;
	lightstyle_t	*ls;
	float		scale;

	scale = sv_lighting_modulate->value;

	// run lightstyles animation
	for( i = 0, ls = sv.lightstyles; i < MAX_LIGHTSTYLES; i++, ls++ )
	{
		ls->time += host.frametime;

		if( ls->length == 0 )
		{
			ls->value = scale; // disable this light
		}
		else if( ls->length == 1 )
		{
			ls->value = ( ls->map[0] / 12.0f ) * scale;
		}
		else
		{
			ofs = (ls->time * 10);
			ls->value = ( ls->map[ofs % ls->length] / 12.0f ) * scale;
		}
	}
}

/*
==================
SV_SetLightStyle

needs to get correct working SV_LightPoint
==================
*/
void SV_SetLightStyle( int style, const char* s, float f )
{
	int	j, k;

	Q_strncpy( sv.lightstyles[style].pattern, s, sizeof( sv.lightstyles[0].pattern ));
	sv.lightstyles[style].time = f;

	j = Q_strlen( s );
	sv.lightstyles[style].length = j;

	for( k = 0; k < j; k++ )
		sv.lightstyles[style].map[k] = (float)(s[k] - 'a');

	if( sv.state != ss_active ) return;

	// tell the clients about changed lightstyle
	BF_WriteByte( &sv.reliable_datagram, svc_lightstyle );
	BF_WriteByte( &sv.reliable_datagram, style );
	BF_WriteString( &sv.reliable_datagram, sv.lightstyles[style].pattern );
	BF_WriteFloat( &sv.reliable_datagram, sv.lightstyles[style].time );
}

/*
==================
SV_GetLightStyle

needs to get correct working SV_LightPoint
==================
*/
const char *SV_GetLightStyle( int style )
{
	if( style < 0 ) style = 0;
	if( style >= MAX_LIGHTSTYLES )
		Host_Error( "SV_GetLightStyle: style: %i >= %d", style, MAX_LIGHTSTYLES );

	return sv.lightstyles[style].pattern;
}

/*
==================
SV_LightForEntity

grab the ambient lighting color for current point
==================
*/
int SV_LightForEntity( edict_t *pEdict )
{
	vec3_t	start, end;

	if( !pEdict ) return 0;
	if( pEdict->v.effects & EF_FULLBRIGHT || !sv.worldmodel->lightdata )
	{
		return 255;
	}

	if( pEdict->v.flags & FL_CLIENT )
	{
		// player has more precision light level
		// that come from client-side
		return pEdict->v.light_level;
	}

	VectorCopy( pEdict->v.origin, start );
	VectorCopy( pEdict->v.origin, end );

	if( pEdict->v.effects & EF_INVLIGHT )
		end[2] = start[2] + world.size[2];
	else end[2] = start[2] - world.size[2];
	VectorSet( sv_pointColor, 1.0f, 1.0f, 1.0f );

	SV_RecursiveLightPoint( sv.worldmodel, sv.worldmodel->nodes, start, end );

	return VectorAvg( sv_pointColor );
}
