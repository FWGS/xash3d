/*
gl_rmain.c - renderer main loop
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
#include "client.h"
#include "gl_local.h"
#include "mathlib.h"
#include "library.h"
#include "beamdef.h"
#include "particledef.h"
#include "entity_types.h"

#define IsLiquidContents( cnt )	( cnt == CONTENTS_WATER || cnt == CONTENTS_SLIME || cnt == CONTENTS_LAVA )

msurface_t	*r_debug_surface;
const char	*r_debug_hitbox;
float		gldepthmin, gldepthmax;
ref_params_t	r_lastRefdef;
ref_instance_t	RI, prevRI;

mleaf_t		*r_viewleaf, *r_oldviewleaf;
mleaf_t		*r_viewleaf2, *r_oldviewleaf2;

static int R_RankForRenderMode( cl_entity_t *ent )
{
	switch( ent->curstate.rendermode )
	{
	case kRenderTransTexture:
		return 1;	// draw second
	case kRenderTransAdd:
		return 2;	// draw third
	case kRenderGlow:
		return 3;	// must be last!
	}
	return 0;
}

/*
===============
R_StaticEntity

Static entity is the brush which has no custom origin and not rotated
typically is a func_wall, func_breakable, func_ladder etc
===============
*/
static qboolean R_StaticEntity( cl_entity_t *ent )
{
	if( !gl_allow_static->integer )
		return false;

	if( ent->curstate.rendermode != kRenderNormal )
		return false;

	if( ent->model->type != mod_brush )
		return false;

	if( ent->curstate.effects & ( EF_NOREFLECT|EF_REFLECTONLY ))
		return false;

	if( ent->curstate.frame || ent->model->flags & MODEL_CONVEYOR )
		return false;

	if( ent->curstate.scale ) // waveheight specified
		return false;

	if( !VectorIsNull( ent->origin ) || !VectorIsNull( ent->angles ))
		return false;

	return true;
}

/*
===============
R_FollowEntity

Follow entity is attached to another studiomodel and used last cached bones
from parent
===============
*/
static qboolean R_FollowEntity( cl_entity_t *ent )
{
	if( ent->model->type != mod_studio )
		return false;

	if( ent->curstate.movetype != MOVETYPE_FOLLOW )
		return false;

	if( ent->curstate.aiment <= 0 )
		return false;

	return true;
}

/*
===============
R_OpaqueEntity

Opaque entity can be brush or studio model but sprite
===============
*/
static qboolean R_OpaqueEntity( cl_entity_t *ent )
{
	if( ent->curstate.rendermode == kRenderNormal )
		return true;

	if( ent->model->type == mod_sprite )
		return false;

	if( ent->curstate.rendermode == kRenderTransAlpha )
		return true;

	return false;
}

/*
===============
R_SolidEntityCompare

Sorting opaque entities by model type
===============
*/
static int R_SolidEntityCompare( const cl_entity_t **a, const cl_entity_t **b )
{
	cl_entity_t	*ent1, *ent2;

	ent1 = (cl_entity_t *)*a;
	ent2 = (cl_entity_t *)*b;

	if( ent1->model->type > ent2->model->type )
		return 1;
	if( ent1->model->type < ent2->model->type )
		return -1;

	return 0;
}

/*
===============
R_TransEntityCompare

Sorting translucent entities by rendermode then by distance
===============
*/
static int R_TransEntityCompare( const cl_entity_t **a, const cl_entity_t **b )
{
	cl_entity_t	*ent1, *ent2;
	vec3_t		vecLen, org;
	float		len1, len2;

	ent1 = (cl_entity_t *)*a;
	ent2 = (cl_entity_t *)*b;

	// now sort by rendermode
	if( R_RankForRenderMode( ent1 ) > R_RankForRenderMode( ent2 ))
		return 1;
	if( R_RankForRenderMode( ent1 ) < R_RankForRenderMode( ent2 ))
		return -1;

	// then by distance
	if( ent1->model->type == mod_brush )
	{
		VectorAverage( ent1->model->mins, ent1->model->maxs, org );
		VectorAdd( ent1->origin, org, org );
		VectorSubtract( RI.vieworg, org, vecLen );
	}
	else VectorSubtract( RI.vieworg, ent1->origin, vecLen );
	len1 = VectorLength( vecLen );

	if( ent2->model->type == mod_brush )
	{
		VectorAverage( ent2->model->mins, ent2->model->maxs, org );
		VectorAdd( ent2->origin, org, org );
		VectorSubtract( RI.vieworg, org, vecLen );
	}
	else VectorSubtract( RI.vieworg, ent2->origin, vecLen );
	len2 = VectorLength( vecLen );

	if( len1 > len2 )
		return -1;
	if( len1 < len2 )
		return 1;

	return 0;
}

/*
===============
R_WorldToScreen

Convert a given point from world into screen space
===============
*/
qboolean R_WorldToScreen( const vec3_t point, vec3_t screen )
{
	matrix4x4	worldToScreen;
	qboolean	behind;
	float	w;

	if( !point || !screen )
		return false;

	Matrix4x4_Copy( worldToScreen, RI.worldviewProjectionMatrix );
	screen[0] = worldToScreen[0][0] * point[0] + worldToScreen[0][1] * point[1] + worldToScreen[0][2] * point[2] + worldToScreen[0][3];
	screen[1] = worldToScreen[1][0] * point[0] + worldToScreen[1][1] * point[1] + worldToScreen[1][2] * point[2] + worldToScreen[1][3];
	w = worldToScreen[3][0] * point[0] + worldToScreen[3][1] * point[1] + worldToScreen[3][2] * point[2] + worldToScreen[3][3];
	screen[2] = 0.0f; // just so we have something valid here

	if( w < 0.001f )
	{
		behind = true;
		screen[0] *= 100000;
		screen[1] *= 100000;
	}
	else
	{
		float invw = 1.0f / w;
		behind = false;
		screen[0] *= invw;
		screen[1] *= invw;
	}
	return behind;
}

/*
===============
R_ScreenToWorld

Convert a given point from screen into world space
===============
*/
void R_ScreenToWorld( const vec3_t screen, vec3_t point )
{
	matrix4x4	screenToWorld;
	float	w;

	if( !point || !screen )
		return;

	Matrix4x4_Invert_Full( screenToWorld, RI.worldviewProjectionMatrix );

	point[0] = screen[0] * screenToWorld[0][0] + screen[1] * screenToWorld[0][1] + screen[2] * screenToWorld[0][2] + screenToWorld[0][3];
	point[1] = screen[0] * screenToWorld[1][0] + screen[1] * screenToWorld[1][1] + screen[2] * screenToWorld[1][2] + screenToWorld[1][3];
	point[2] = screen[0] * screenToWorld[2][0] + screen[1] * screenToWorld[2][1] + screen[2] * screenToWorld[2][2] + screenToWorld[2][3];
	w = screen[0] * screenToWorld[3][0] + screen[1] * screenToWorld[3][1] + screen[2] * screenToWorld[3][2] + screenToWorld[3][3];
	if( w != 0.0f ) VectorScale( point, ( 1.0f / w ), point );
}

/*
===============
R_ComputeFxBlend
===============
*/
int R_ComputeFxBlend( cl_entity_t *e )
{
	int		blend = 0, renderAmt;
	float		offset, dist;
	vec3_t		tmp;

	offset = ((int)e->index ) * 363.0f; // Use ent index to de-sync these fx
	renderAmt = e->curstate.renderamt;

	switch( e->curstate.renderfx ) 
	{
	case kRenderFxPulseSlowWide:
		blend = renderAmt + 0x40 * sin( RI.refdef.time * 2 + offset );	
		break;
	case kRenderFxPulseFastWide:
		blend = renderAmt + 0x40 * sin( RI.refdef.time * 8 + offset );
		break;
	case kRenderFxPulseSlow:
		blend = renderAmt + 0x10 * sin( RI.refdef.time * 2 + offset );
		break;
	case kRenderFxPulseFast:
		blend = renderAmt + 0x10 * sin( RI.refdef.time * 8 + offset );
		break;
	// JAY: HACK for now -- not time based
	case kRenderFxFadeSlow:			
		if( renderAmt > 0 ) 
			renderAmt -= 1;
		else renderAmt = 0;
		blend = renderAmt;
		break;
	case kRenderFxFadeFast:
		if( renderAmt > 3 ) 
			renderAmt -= 4;
		else renderAmt = 0;
		blend = renderAmt;
		break;
	case kRenderFxSolidSlow:
		if( renderAmt < 255 ) 
			renderAmt += 1;
		else renderAmt = 255;
		blend = renderAmt;
		break;
	case kRenderFxSolidFast:
		if( renderAmt < 252 ) 
			renderAmt += 4;
		else renderAmt = 255;
		blend = renderAmt;
		break;
	case kRenderFxStrobeSlow:
		blend = 20 * sin( RI.refdef.time * 4 + offset );
		if( blend < 0 ) blend = 0;
		else blend = renderAmt;
		break;
	case kRenderFxStrobeFast:
		blend = 20 * sin( RI.refdef.time * 16 + offset );
		if( blend < 0 ) blend = 0;
		else blend = renderAmt;
		break;
	case kRenderFxStrobeFaster:
		blend = 20 * sin( RI.refdef.time * 36 + offset );
		if( blend < 0 ) blend = 0;
		else blend = renderAmt;
		break;
	case kRenderFxFlickerSlow:
		blend = 20 * (sin( RI.refdef.time * 2 ) + sin( RI.refdef.time * 17 + offset ));
		if( blend < 0 ) blend = 0;
		else blend = renderAmt;
		break;
	case kRenderFxFlickerFast:
		blend = 20 * (sin( RI.refdef.time * 16 ) + sin( RI.refdef.time * 23 + offset ));
		if( blend < 0 ) blend = 0;
		else blend = renderAmt;
		break;
	case kRenderFxHologram:
	case kRenderFxDistort:
		VectorCopy( e->origin, tmp );
		VectorSubtract( tmp, RI.refdef.vieworg, tmp );
		dist = DotProduct( tmp, RI.refdef.forward );
			
		// Turn off distance fade
		if( e->curstate.renderfx == kRenderFxDistort )
			dist = 1;

		if( dist <= 0 )
		{
			blend = 0;
		}
		else 
		{
			renderAmt = 180;
			if( dist <= 100 ) blend = renderAmt;
			else blend = (int) ((1.0f - ( dist - 100 ) * ( 1.0f / 400.0f )) * renderAmt );
			blend += Com_RandomLong( -32, 31 );
		}
		break;
	case kRenderFxGlowShell:	// safe current renderamt because it's shell scale!
	case kRenderFxDeadPlayer:	// safe current renderamt because it's player index!
		blend = renderAmt;
		break;
	case kRenderFxNone:
	case kRenderFxClampMinScale:
	default:
		if( e->curstate.rendermode == kRenderNormal )
			blend = 255;
		else blend = renderAmt;
		break;	
	}

	if( e->model->type != mod_brush )
	{
		// NOTE: never pass sprites with rendercolor '0 0 0' it's a stupid Valve Hammer Editor bug
		if( !e->curstate.rendercolor.r && !e->curstate.rendercolor.g && !e->curstate.rendercolor.b )
			e->curstate.rendercolor.r = e->curstate.rendercolor.g = e->curstate.rendercolor.b = 255;
	}

	// apply scale to studiomodels and sprites only
	if( e->model && e->model->type != mod_brush && !e->curstate.scale )
		e->curstate.scale = 1.0f;

	blend = bound( 0, blend, 255 );

	return blend;
}

/*
===============
R_ClearScene
===============
*/
void R_ClearScene( void )
{
	tr.num_solid_entities = tr.num_trans_entities = 0;
	tr.num_static_entities = tr.num_mirror_entities = 0;
	tr.num_child_entities = 0;
}

/*
===============
R_AddEntity
===============
*/
qboolean R_AddEntity( struct cl_entity_s *clent, int entityType )
{
	if( !r_drawentities->integer )
		return false; // not allow to drawing

	if( !clent || !clent->model )
		return false; // if set to invisible, skip

	if( clent->curstate.effects & EF_NODRAW )
		return false; // done

	clent->curstate.renderamt = R_ComputeFxBlend( clent );

	if( clent->curstate.rendermode != kRenderNormal && clent->curstate.renderamt <= 0.0f )
		return true; // invisible

	clent->curstate.entityType = entityType;

	if( entityType == ET_FRAGMENTED )
		r_stats.c_client_ents++;

	if( R_FollowEntity( clent ))
	{
		// follow entity
		if( tr.num_child_entities >= MAX_VISIBLE_PACKET )
			return false;

		tr.child_entities[tr.num_child_entities] = clent;
		tr.num_child_entities++;

		return true;
	}

	if( R_OpaqueEntity( clent ))
	{
		if( R_StaticEntity( clent ))
		{
			// opaque static
			if( tr.num_static_entities >= MAX_VISIBLE_PACKET )
				return false;

			tr.static_entities[tr.num_static_entities] = clent;
			tr.num_static_entities++;
		}
		else
		{
			// opaque moving
			if( tr.num_solid_entities >= MAX_VISIBLE_PACKET )
				return false;

			tr.solid_entities[tr.num_solid_entities] = clent;
			tr.num_solid_entities++;
		}
	}
	else
	{
		// translucent
		if( tr.num_trans_entities >= MAX_VISIBLE_PACKET )
			return false;

		tr.trans_entities[tr.num_trans_entities] = clent;
		tr.num_trans_entities++;
	}
	return true;
}

/*
=============
R_Clear
=============
*/
static void R_Clear( int bitMask )
{
	int	bits;

	if( gl_overview->integer )
		pglClearColor( 0.0f, 1.0f, 0.0f, 1.0f ); // green background (Valve rules)
	else pglClearColor( 0.5f, 0.5f, 0.5f, 1.0f );

	bits = GL_DEPTH_BUFFER_BIT;

	if( RI.drawWorld && r_fastsky->integer )
		bits |= GL_COLOR_BUFFER_BIT;
	if( glState.stencilEnabled )
		bits |= GL_STENCIL_BUFFER_BIT;

	bits &= bitMask;

	pglClear( bits );

	// change ordering for overview
	if( RI.drawOrtho )
	{
		gldepthmin = 1.0f;
		gldepthmax = 0.0f;
	}
	else
	{
		gldepthmin = 0.0f;
		gldepthmax = 1.0f;
	}

	pglDepthFunc( GL_LEQUAL );
	pglDepthRange( gldepthmin, gldepthmax );
}

//=============================================================================
/*
===============
R_GetFarClip
===============
*/
static float R_GetFarClip( void )
{
	if( cl.worldmodel && RI.drawWorld )
		return cl.refdef.movevars->zmax * 1.5f;
	return 2048.0f;
}

/*
===============
R_SetupFrustumOrtho
===============
*/
static void R_SetupFrustumOrtho( void )
{
	ref_overview_t	*ov = &clgame.overView;
	float		orgOffset;
	int		i;

	// 0 - left
	// 1 - right
	// 2 - down
	// 3 - up
	// 4 - farclip
	// 5 - nearclip

	// setup the near and far planes.
	orgOffset = DotProduct( RI.cullorigin, RI.cull_vforward );
	VectorNegate( RI.cull_vforward, RI.frustum[4].normal );
	RI.frustum[4].dist = -ov->zFar - orgOffset;

	VectorCopy( RI.cull_vforward, RI.frustum[5].normal );
	RI.frustum[5].dist = ov->zNear + orgOffset;

	// left and right planes...
	orgOffset = DotProduct( RI.cullorigin, RI.cull_vright );
	VectorCopy( RI.cull_vright, RI.frustum[0].normal );
	RI.frustum[0].dist = ov->xLeft + orgOffset;

	VectorNegate( RI.cull_vright, RI.frustum[1].normal );
	RI.frustum[1].dist = -ov->xRight - orgOffset;

	// top and buttom planes...
	orgOffset = DotProduct( RI.cullorigin, RI.cull_vup );
	VectorCopy( RI.cull_vup, RI.frustum[3].normal );
	RI.frustum[3].dist = ov->xTop + orgOffset;

	VectorNegate( RI.cull_vup, RI.frustum[2].normal );
	RI.frustum[2].dist = -ov->xBottom - orgOffset;

	for( i = 0; i < 6; i++ )
	{
		RI.frustum[i].type = PLANE_NONAXIAL;
		RI.frustum[i].signbits = SignbitsForPlane( RI.frustum[i].normal );
	}
}

/*
===============
R_SetupFrustum
===============
*/
void R_SetupFrustum( void )
{
	vec3_t	farPoint;
	int	i;

	// 0 - left
	// 1 - right
	// 2 - down
	// 3 - up
	// 4 - farclip
	// 5 - nearclip

	if( RI.drawOrtho )
	{
		R_SetupFrustumOrtho();
		return;
	}

	// rotate RI.vforward right by FOV_X/2 degrees
	RotatePointAroundVector( RI.frustum[0].normal, RI.cull_vup, RI.cull_vforward, -( 90 - RI.refdef.fov_x / 2 ));
	// rotate RI.vforward left by FOV_X/2 degrees
	RotatePointAroundVector( RI.frustum[1].normal, RI.cull_vup, RI.cull_vforward, 90 - RI.refdef.fov_x / 2 );
	// rotate RI.vforward up by FOV_X/2 degrees
	RotatePointAroundVector( RI.frustum[2].normal, RI.cull_vright, RI.cull_vforward, 90 - RI.refdef.fov_y / 2 );
	// rotate RI.vforward down by FOV_X/2 degrees
	RotatePointAroundVector( RI.frustum[3].normal, RI.cull_vright, RI.cull_vforward, -( 90 - RI.refdef.fov_y / 2 ));
	// negate forward vector
	VectorNegate( RI.cull_vforward, RI.frustum[4].normal );

	for( i = 0; i < 4; i++ )
	{
		RI.frustum[i].type = PLANE_NONAXIAL;
		RI.frustum[i].dist = DotProduct( RI.cullorigin, RI.frustum[i].normal );
		RI.frustum[i].signbits = SignbitsForPlane( RI.frustum[i].normal );
	}

	VectorMA( RI.cullorigin, R_GetFarClip(), RI.cull_vforward, farPoint );
	RI.frustum[i].type = PLANE_NONAXIAL;
	RI.frustum[i].dist = DotProduct( farPoint, RI.frustum[i].normal );
	RI.frustum[i].signbits = SignbitsForPlane( RI.frustum[i].normal );
}

/*
=============
R_SetupProjectionMatrix
=============
*/
static void R_SetupProjectionMatrix( const ref_params_t *fd, matrix4x4 m )
{
	GLdouble	xMin, xMax, yMin, yMax, zNear, zFar;

	if( RI.drawOrtho )
	{
		ref_overview_t *ov = &clgame.overView;
		Matrix4x4_CreateOrtho( m, ov->xLeft, ov->xRight, ov->xTop, ov->xBottom, ov->zNear, ov->zFar );
		RI.clipFlags = 0;
		return;
	}

	RI.farClip = R_GetFarClip();

	zNear = 4.0f;
	zFar = max( 256.0f, RI.farClip );

	yMax = zNear * tan( fd->fov_y * M_PI / 360.0 );
	yMin = -yMax;

	xMax = zNear * tan( fd->fov_x * M_PI / 360.0 );
	xMin = -xMax;

	Matrix4x4_CreateProjection( m, xMax, xMin, yMax, yMin, zNear, zFar );
}

/*
=============
R_SetupModelviewMatrix
=============
*/
static void R_SetupModelviewMatrix( const ref_params_t *fd, matrix4x4 m )
{
#if 0
	Matrix4x4_LoadIdentity( m );
	Matrix4x4_ConcatRotate( m, -90, 1, 0, 0 );
	Matrix4x4_ConcatRotate( m, 90, 0, 0, 1 );
#else
	Matrix4x4_CreateModelview( m );
#endif
	Matrix4x4_ConcatRotate( m, -fd->viewangles[2], 1, 0, 0 );
	Matrix4x4_ConcatRotate( m, -fd->viewangles[0], 0, 1, 0 );
	Matrix4x4_ConcatRotate( m, -fd->viewangles[1], 0, 0, 1 );
	Matrix4x4_ConcatTranslate( m, -fd->vieworg[0], -fd->vieworg[1], -fd->vieworg[2] );
}

/*
=============
R_LoadIdentity
=============
*/
void R_LoadIdentity( void )
{
	if( tr.modelviewIdentity ) return;

	Matrix4x4_LoadIdentity( RI.objectMatrix );
	Matrix4x4_Copy( RI.modelviewMatrix, RI.worldviewMatrix );

	pglMatrixMode( GL_MODELVIEW );
	GL_LoadMatrix( RI.modelviewMatrix );
	tr.modelviewIdentity = true;
}

/*
=============
R_RotateForEntity
=============
*/
void R_RotateForEntity( cl_entity_t *e )
{
	float	scale = 1.0f;

	if( e == clgame.entities || R_StaticEntity( e ))
	{
		R_LoadIdentity();
		return;
	}

	if( e->model->type != mod_brush && e->curstate.scale > 0.0f )
		scale = e->curstate.scale;

	Matrix4x4_CreateFromEntity( RI.objectMatrix, e->angles, e->origin, scale );
	Matrix4x4_ConcatTransforms( RI.modelviewMatrix, RI.worldviewMatrix, RI.objectMatrix );

	pglMatrixMode( GL_MODELVIEW );
	GL_LoadMatrix( RI.modelviewMatrix );
	tr.modelviewIdentity = false;
}

/*
=============
R_TranslateForEntity
=============
*/
void R_TranslateForEntity( cl_entity_t *e )
{
	float	scale = 1.0f;

	if( e == clgame.entities || R_StaticEntity( e ))
	{
		R_LoadIdentity();
		return;
	}

	if( e->model->type != mod_brush && e->curstate.scale > 0.0f )
		scale = e->curstate.scale;

	Matrix4x4_CreateFromEntity( RI.objectMatrix, vec3_origin, e->origin, scale );
	Matrix4x4_ConcatTransforms( RI.modelviewMatrix, RI.worldviewMatrix, RI.objectMatrix );

	pglMatrixMode( GL_MODELVIEW );
	GL_LoadMatrix( RI.modelviewMatrix );
	tr.modelviewIdentity = false;
}

/*
===============
R_FindViewLeaf
===============
*/
void R_FindViewLeaf( void )
{
	float	height;
	mleaf_t	*leaf;
	vec3_t	tmp;

	r_oldviewleaf = r_viewleaf;
	r_oldviewleaf2 = r_viewleaf2;
	leaf = Mod_PointInLeaf( RI.pvsorigin, cl.worldmodel->nodes );
	r_viewleaf2 = r_viewleaf = leaf;
	height = RI.waveHeight ? RI.waveHeight : 16;

	// check above and below so crossing solid water doesn't draw wrong
	if( leaf->contents == CONTENTS_EMPTY )
	{
		// look down a bit
		VectorCopy( RI.pvsorigin, tmp );
		tmp[2] -= height;
		leaf = Mod_PointInLeaf( tmp, cl.worldmodel->nodes );
		if(( leaf->contents != CONTENTS_SOLID ) && ( leaf != r_viewleaf2 ))
		r_viewleaf2 = leaf;
	}
	else
	{
		// look up a bit
		VectorCopy( RI.pvsorigin, tmp );
		tmp[2] += height;
		leaf = Mod_PointInLeaf( tmp, cl.worldmodel->nodes );
		if(( leaf->contents != CONTENTS_SOLID ) && ( leaf != r_viewleaf2 ))
		r_viewleaf2 = leaf;
	}
}

/*
===============
R_SetupFrame
===============
*/
static void R_SetupFrame( void )
{
	vec3_t	viewOrg, viewAng;

	if( RP_NORMALPASS() && cl.thirdperson )
	{
		vec3_t	cam_ofs, vpn;

		clgame.dllFuncs.CL_CameraOffset( cam_ofs );

		viewAng[PITCH] = cam_ofs[PITCH];
		viewAng[YAW] = cam_ofs[YAW];
		viewAng[ROLL] = 0;

		AngleVectors( viewAng, vpn, NULL, NULL );
		VectorMA( RI.refdef.vieworg, -cam_ofs[ROLL], vpn, viewOrg );
	}
	else
	{
		VectorCopy( RI.refdef.vieworg, viewOrg );
		VectorCopy( RI.refdef.viewangles, viewAng );
	}

	// build the transformation matrix for the given view angles
	VectorCopy( viewOrg, RI.vieworg );
	AngleVectors( viewAng, RI.vforward, RI.vright, RI.vup );

	// setup viewplane dist
	RI.viewplanedist = DotProduct( RI.vieworg, RI.vforward );

	VectorCopy( RI.vieworg, RI.pvsorigin );

	if( !r_lockcull->integer )
	{
		VectorCopy( RI.vieworg, RI.cullorigin );
		VectorCopy( RI.vforward, RI.cull_vforward );
		VectorCopy( RI.vright, RI.cull_vright );
		VectorCopy( RI.vup, RI.cull_vup );
	}

	R_AnimateLight();
	R_RunViewmodelEvents();

	// sort opaque entities by model type to avoid drawing model shadows under alpha-surfaces
	qsort( tr.solid_entities, tr.num_solid_entities, sizeof( cl_entity_t* ), R_SolidEntityCompare );

	if( !gl_nosort->integer )
	{
		// sort translucents entities by rendermode and distance
		qsort( tr.trans_entities, tr.num_trans_entities, sizeof( cl_entity_t* ), R_TransEntityCompare );
	}

	// current viewleaf
	if( RI.drawWorld )
	{
		RI.waveHeight = cl.refdef.movevars->waveHeight * 2.0f;	// set global waveheight
		RI.isSkyVisible = false; // unknown at this moment

		if(!( RI.params & RP_OLDVIEWLEAF ))
			R_FindViewLeaf();
	}
}

/*
=============
R_SetupGL
=============
*/
static void R_SetupGL( void )
{
	if( RI.refdef.waterlevel >= 3 )
	{
		float	f;
		f = sin( cl.time * 0.4f * ( M_PI * 2.7f ));
		RI.refdef.fov_x += f;
		RI.refdef.fov_y -= f;
	}

	R_SetupModelviewMatrix( &RI.refdef, RI.worldviewMatrix );
	R_SetupProjectionMatrix( &RI.refdef, RI.projectionMatrix );
//	if( RI.params & RP_MIRRORVIEW ) RI.projectionMatrix[0][0] = -RI.projectionMatrix[0][0];

	Matrix4x4_Concat( RI.worldviewProjectionMatrix, RI.projectionMatrix, RI.worldviewMatrix );

	if( RP_NORMALPASS( ))
	{
		int	x, x2, y, y2;

		// set up viewport (main, playersetup)
		x = floor( RI.viewport[0] * glState.width / glState.width );
		x2 = ceil(( RI.viewport[0] + RI.viewport[2] ) * glState.width / glState.width );
		y = floor( glState.height - RI.viewport[1] * glState.height / glState.height );
		y2 = ceil( glState.height - ( RI.viewport[1] + RI.viewport[3] ) * glState.height / glState.height );

		pglViewport( x, y2, x2 - x, y - y2 );
	}
	else
	{
		// envpass, mirrorpass
		pglViewport( RI.viewport[0], RI.viewport[1], RI.viewport[2], RI.viewport[3] );
	}

	pglMatrixMode( GL_PROJECTION );
	GL_LoadMatrix( RI.projectionMatrix );

	pglMatrixMode( GL_MODELVIEW );
	GL_LoadMatrix( RI.worldviewMatrix );

	if( RI.params & RP_CLIPPLANE )
	{
		GLdouble	clip[4];
		mplane_t	*p = &RI.clipPlane;

		clip[0] = p->normal[0];
		clip[1] = p->normal[1];
		clip[2] = p->normal[2];
		clip[3] = -p->dist;

		pglClipPlane( GL_CLIP_PLANE0, clip );
		pglEnable( GL_CLIP_PLANE0 );
	}

	if( RI.params & RP_FLIPFRONTFACE )
		GL_FrontFace( !glState.frontFace );

	GL_Cull( GL_FRONT );

	pglDisable( GL_BLEND );
	pglDisable( GL_ALPHA_TEST );
	pglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
}

/*
=============
R_EndGL
=============
*/
static void R_EndGL( void )
{
	if( RI.params & RP_FLIPFRONTFACE )
		GL_FrontFace( !glState.frontFace );

	if( RI.params & RP_CLIPPLANE )
		pglDisable( GL_CLIP_PLANE0 );
}

/*
=============
R_RecursiveFindWaterTexture

using to find source waterleaf with
watertexture to grab fog values from it
=============
*/
static gltexture_t *R_RecursiveFindWaterTexture( const mnode_t *node, const mnode_t *ignore, qboolean down )
{
	gltexture_t *tex = NULL;

	// assure the initial node is not null
	// we could check it here, but we would rather check it 
	// outside the call to get rid of one additional recursion level
	ASSERT( node != NULL );

	// ignore solid nodes
	if( node->contents == CONTENTS_SOLID )
		return NULL;

	if( node->contents < 0 )
	{
		mleaf_t		*pleaf;
		msurface_t	**mark;
		int		i, c;

		// ignore non-liquid leaves
		if( node->contents != CONTENTS_WATER && node->contents != CONTENTS_LAVA && node->contents != CONTENTS_SLIME )
			 return NULL;

		// find texture
		pleaf = (mleaf_t *)node;
		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;	

		for( i = 0; i < c; i++, mark++ )
		{
			if( (*mark)->flags & SURF_DRAWTURB && (*mark)->texinfo && (*mark)->texinfo->texture )
				return R_GetTexture( (*mark)->texinfo->texture->gl_texturenum );
		}

		// texture not found
		return NULL;
	}

	// this is a regular node
	// traverse children
	if( node->children[0] && ( node->children[0] != ignore ))
	{
		tex = R_RecursiveFindWaterTexture( node->children[0], node, true );
		if( tex ) return tex;
	}

	if( node->children[1] && ( node->children[1] != ignore ))
	{
		tex = R_RecursiveFindWaterTexture( node->children[1], node, true );
		if( tex )	return tex;
	}

	// for down recursion, return immediately
	if( down ) return NULL;

	// texture not found, step up if any
	if( node->parent )
		return R_RecursiveFindWaterTexture( node->parent, node, false );

	// top-level node, bail out
	return NULL;
}

/*
=============
R_CheckFog

check for underwater fog
Using backward recursion to find waterline leaf
from underwater leaf (idea: XaeroX)
=============
*/
static void R_CheckFog( void )
{
	cl_entity_t	*ent;
	gltexture_t	*tex;
	int		i, cnt, count;

	RI.fogEnabled = false;

	if( RI.refdef.waterlevel < 2 || !RI.drawWorld || !r_viewleaf )
		return;

	ent = CL_GetWaterEntity( RI.vieworg );
	if( ent && ent->model && ent->model->type == mod_brush && ent->curstate.skin < 0 )
		cnt = ent->curstate.skin;
	else cnt = r_viewleaf->contents;

	if( IsLiquidContents( RI.cached_contents ) && !IsLiquidContents( cnt ))
	{
		RI.cached_contents = CONTENTS_EMPTY;
		return;
	}

	if( RI.refdef.waterlevel < 3 ) return;

	if( !IsLiquidContents( RI.cached_contents ) && IsLiquidContents( cnt ))
	{
		tex = NULL;

		// check for water texture
		if( ent && ent->model && ent->model->type == mod_brush )
		{
			msurface_t	*surf;
	
			count = ent->model->nummodelsurfaces;

			for( i = 0, surf = &ent->model->surfaces[ent->model->firstmodelsurface]; i < count; i++, surf++ )
			{
				if( surf->flags & SURF_DRAWTURB && surf->texinfo && surf->texinfo->texture )
				{
					tex = R_GetTexture( surf->texinfo->texture->gl_texturenum );
					RI.cached_contents = ent->curstate.skin;
					break;
				}
			}
		}
		else
		{
			tex = R_RecursiveFindWaterTexture( r_viewleaf->parent, NULL, false );
			if( tex ) RI.cached_contents = r_viewleaf->contents;
		}

		if( !tex ) return;	// no valid fogs

		// copy fog params
		RI.fogColor[0] = tex->fogParams[0] / 255.0f;
		RI.fogColor[1] = tex->fogParams[1] / 255.0f;
		RI.fogColor[2] = tex->fogParams[2] / 255.0f;
		RI.fogDensity = tex->fogParams[3] * 0.000025f;
		RI.fogStart = RI.fogEnd = 0.0f;
		RI.fogCustom = false;
		RI.fogEnabled = true;
	}
	else
	{
		RI.fogCustom = false;
		RI.fogEnabled = true;
	}
}

/*
=============
R_DrawFog

=============
*/
void R_DrawFog( void )
{
	if( !RI.fogEnabled || RI.refdef.onlyClientDraw )
		return;

	pglEnable( GL_FOG );
	pglFogi( GL_FOG_MODE, GL_EXP );
	pglFogf( GL_FOG_DENSITY, RI.fogDensity );
	pglFogfv( GL_FOG_COLOR, RI.fogColor );
	pglHint( GL_FOG_HINT, GL_NICEST );
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList( void )
{
	int	i;

	glState.drawTrans = false;

	// draw the solid submodels fog
	R_DrawFog ();

	// first draw solid entities
	for( i = 0; i < tr.num_solid_entities; i++ )
	{
		if( RI.refdef.onlyClientDraw )
			break;

		RI.currententity = tr.solid_entities[i];
		RI.currentmodel = RI.currententity->model;
	
		ASSERT( RI.currententity != NULL );
		ASSERT( RI.currententity->model != NULL );

		switch( RI.currentmodel->type )
		{
		case mod_brush:
			R_DrawBrushModel( RI.currententity );
			break;
		case mod_studio:
			R_DrawStudioModel( RI.currententity );
			break;
		case mod_sprite:
			R_DrawSpriteModel( RI.currententity );
			break;
		default:
			break;
		}
	}

	if( !RI.refdef.onlyClientDraw )
          {
		CL_DrawBeams( false );
	}

	if( RI.drawWorld )
		clgame.dllFuncs.pfnDrawNormalTriangles();

	// NOTE: some mods with custom renderer may generate glErrors
	// so we clear it here
	while( pglGetError() != GL_NO_ERROR );

	// don't fogging translucent surfaces
	if( !RI.fogCustom )
		pglDisable( GL_FOG );
	pglDepthMask( GL_FALSE );
	glState.drawTrans = true;

	// then draw translucent entities
	for( i = 0; i < tr.num_trans_entities; i++ )
	{
		if( RI.refdef.onlyClientDraw )
			break;

		RI.currententity = tr.trans_entities[i];
		RI.currentmodel = RI.currententity->model;
	
		ASSERT( RI.currententity != NULL );
		ASSERT( RI.currententity->model != NULL );

		switch( RI.currentmodel->type )
		{
		case mod_brush:
			R_DrawBrushModel( RI.currententity );
			break;
		case mod_studio:
			R_DrawStudioModel( RI.currententity );
			break;
		case mod_sprite:
			R_DrawSpriteModel( RI.currententity );
			break;
		default:
			break;
		}
	}

	if( RI.drawWorld )
		clgame.dllFuncs.pfnDrawTransparentTriangles ();

	if( !RI.refdef.onlyClientDraw )
	{
		CL_DrawBeams( true );
		CL_DrawParticles();
	}

	// NOTE: some mods with custom renderer may generate glErrors
	// so we clear it here
	while( pglGetError() != GL_NO_ERROR );

	glState.drawTrans = false;
	pglDepthMask( GL_TRUE );
	pglDisable( GL_BLEND );	// Trinity Render issues

	R_DrawViewModel();

	CL_ExtraUpdate();
}

/*
================
R_RenderScene

RI.refdef must be set before the first call
================
*/
void R_RenderScene( const ref_params_t *fd )
{
	RI.refdef = *fd;

	if( !cl.worldmodel && RI.drawWorld )
		Host_Error( "R_RenderView: NULL worldmodel\n" );

	R_PushDlights();

	R_SetupFrame();
	R_SetupFrustum();
	R_SetupGL();
	R_Clear( ~0 );

	R_MarkLeaves();
	R_CheckFog();
	R_DrawWorld();

	CL_ExtraUpdate ();	// don't let sound get messed up if going slow

	R_DrawEntitiesOnList();

	R_DrawWaterSurfaces();

	R_EndGL();
}

/*
===============
R_BeginFrame
===============
*/
void R_BeginFrame( qboolean clearScene )
{
	if(( gl_clear->integer || gl_overview->integer ) && clearScene && cls.state != ca_cinematic )
	{
		pglClear( GL_COLOR_BUFFER_BIT );
	}

	// update gamma
	if( vid_gamma->modified )
	{
		if( glConfig.deviceSupportsGamma )
		{
			SCR_RebuildGammaTable();
			GL_UpdateGammaRamp();
			vid_gamma->modified = false;
		}
		else
		{
			BuildGammaTable( vid_gamma->value, vid_texgamma->value );
			GL_RebuildLightmaps();
		}
	}

	R_Set2DMode( true );

	// draw buffer stuff
	pglDrawBuffer( GL_BACK );

	// texturemode stuff
	// update texture parameters
	if( gl_texturemode->modified || gl_texture_anisotropy->modified || gl_texture_lodbias ->modified )
		R_SetTextureParameters();

	// swapinterval stuff
	GL_UpdateSwapInterval();

	CL_ExtraUpdate ();
}

/*
===============
R_RenderFrame
===============
*/
void R_RenderFrame( const ref_params_t *fd, qboolean drawWorld )
{
	if( r_norefresh->integer )
		return;

	tr.realframecount++;

	if( RI.drawOrtho != gl_overview->integer )
		tr.fResetVis = true;

	// completely override rendering
	if( clgame.drawFuncs.GL_RenderFrame != NULL )
	{
		if( clgame.drawFuncs.GL_RenderFrame( fd, drawWorld ))
		{
			RI.drawWorld = drawWorld;
			tr.fResetVis = true;
			return;
		}
	}

	if( drawWorld ) r_lastRefdef = *fd;

	RI.params = RP_NONE;
	RI.farClip = 0;
	RI.clipFlags = 15;
	RI.drawWorld = drawWorld;
	RI.thirdPerson = cl.thirdperson;
	RI.drawOrtho = (RI.drawWorld) ? gl_overview->integer : 0;

	GL_BackendStartFrame();

	if( !r_lockcull->integer )
		VectorCopy( fd->vieworg, RI.cullorigin );
	VectorCopy( fd->vieworg, RI.pvsorigin );

	// setup viewport
	RI.viewport[0] = fd->viewport[0];
	RI.viewport[1] = fd->viewport[1];
	RI.viewport[2] = fd->viewport[2];
	RI.viewport[3] = fd->viewport[3];

	if( gl_finish->integer && drawWorld )
		pglFinish();

	if( gl_allow_mirrors->integer )
	{
		// render mirrors
		R_FindMirrors( fd );
		R_DrawMirrors ();
	}

	R_RenderScene( fd );

	GL_BackendEndFrame();
}

/*
===============
R_EndFrame
===============
*/
void R_EndFrame( void )
{
	// flush any remaining 2D bits
	R_Set2DMode( false );

	if( !pwglSwapBuffers( glw_state.hDC ))
		Sys_Error( "wglSwapBuffers() failed!\n" );
}

/*
===============
R_DrawCubemapView
===============
*/
void R_DrawCubemapView( const vec3_t origin, const vec3_t angles, int size )
{
	ref_params_t *fd;

	if( clgame.drawFuncs.R_DrawCubemapView != NULL )
	{
		if( clgame.drawFuncs.R_DrawCubemapView( origin, angles, size ))
			return;
	}

	fd = &RI.refdef;
	*fd = r_lastRefdef;
	fd->time = 0;
	fd->viewport[0] = 0;
	fd->viewport[1] = 0;
	fd->viewport[2] = size;
	fd->viewport[3] = size;
	fd->fov_x = 90;
	fd->fov_y = 90;
	VectorCopy( origin, fd->vieworg );
	VectorCopy( angles, fd->viewangles );
	VectorCopy( fd->vieworg, RI.pvsorigin );
		
	// setup viewport
	RI.viewport[0] = fd->viewport[0];
	RI.viewport[1] = fd->viewport[1];
	RI.viewport[2] = fd->viewport[2];
	RI.viewport[3] = fd->viewport[3];

	R_RenderScene( fd );

	r_oldviewleaf = r_viewleaf = NULL;		// force markleafs next frame
}

static int GL_RenderGetParm( int parm, int arg )
{
	gltexture_t *glt;

	switch( parm )
	{
	case PARM_TEX_WIDTH:
		glt = R_GetTexture( arg );
		return glt->width;
	case PARM_TEX_HEIGHT:
		glt = R_GetTexture( arg );
		return glt->height;
	case PARM_TEX_SRC_WIDTH:
		glt = R_GetTexture( arg );
		return glt->srcWidth;
	case PARM_TEX_SRC_HEIGHT:
		glt = R_GetTexture( arg );
		return glt->srcHeight;
	case PARM_TEX_SKYBOX:
		ASSERT( arg >= 0 && arg < 6 );
		return tr.skyboxTextures[arg];
	case PARM_TEX_SKYTEXNUM:
		return tr.skytexturenum;
	case PARM_TEX_LIGHTMAP:
		ASSERT( arg >= 0 && arg < MAX_LIGHTMAPS );
		return tr.lightmapTextures[arg];
	case PARM_SKY_SPHERE:
		return world.sky_sphere;
	case PARM_WORLD_VERSION:
		if( cls.state != ca_active )
			return bmodel_version;
		return world.version;
	case PARM_WIDESCREEN:
		return glState.wideScreen;
	case PARM_FULLSCREEN:
		return glState.fullScreen;
	case PARM_SCREEN_WIDTH:
		return glState.width;
	case PARM_SCREEN_HEIGHT:
		return glState.height;
	case PARM_MAP_HAS_MIRRORS:
		return world.has_mirrors;
	case PARM_CLIENT_INGAME:
		return CL_IsInGame();
	case PARM_MAX_ENTITIES:
		return clgame.maxEntities;
	case PARM_TEX_TARGET:
		glt = R_GetTexture( arg );
		return glt->target;
	case PARM_TEX_TEXNUM:
		glt = R_GetTexture( arg );
		return glt->texnum;
	case PARM_TEX_FLAGS:
		glt = R_GetTexture( arg );
		return glt->flags;
	case PARM_FEATURES:
		return host.features;
	case PARM_ACTIVE_TMU:
		return glState.activeTMU;
	case PARM_TEX_CACHEFRAME:
		glt = R_GetTexture( arg );
		return glt->cacheframe;
	case PARM_MAP_HAS_DELUXE:
		return (world.deluxedata != NULL);
	case PARM_TEX_TYPE:
		glt = R_GetTexture( arg );
		return glt->texType;
	case PARM_CACHEFRAME:
		return world.load_sequence;
	case PARM_MAX_IMAGE_UNITS:
		return GL_MaxTextureUnits();
	case PARM_CLIENT_ACTIVE:
		return (cls.state == ca_active);
	}
	return 0;
}

static void R_GetDetailScaleForTexture( int texture, float *xScale, float *yScale )
{
	gltexture_t *glt = R_GetTexture( texture );

	if( xScale ) *xScale = glt->xscale;
	if( yScale ) *yScale = glt->yscale;
}

static void R_GetExtraParmsForTexture( int texture, byte *red, byte *green, byte *blue, byte *density )
{
	gltexture_t *glt = R_GetTexture( texture );

	if( red ) *red = glt->fogParams[0];
	if( green ) *green = glt->fogParams[1];
	if( blue ) *blue = glt->fogParams[2];
	if( density ) *density = glt->fogParams[3];
}

static void GL_TextureUpdateCache( unsigned int texture )
{
	gltexture_t *glt = R_GetTexture( texture );
	if( !glt || !glt->texnum ) return;
	glt->cacheframe = world.load_sequence;
}

/*
=================
R_EnvShot

=================
*/
static void R_EnvShot( const float *vieworg, const char *name, int skyshot, int shotsize )
{
	static vec3_t viewPoint;

	if( !name )
	{
		MsgDev( D_ERROR, "R_%sShot: bad name\n", skyshot ? "Sky" : "Env" );
		return; 
	}

	if( cls.scrshot_action != scrshot_inactive )
	{
		if( cls.scrshot_action != scrshot_skyshot && cls.scrshot_action != scrshot_envshot )
			MsgDev( D_ERROR, "R_%sShot: subsystem is busy, try later.\n", skyshot ? "Sky" : "Env" );
		return;
	}

	cls.envshot_vieworg = NULL; // use client view
	Q_strncpy( cls.shotname, name, sizeof( cls.shotname ));

	if( vieworg )
	{
		// make sure what viewpoint don't temporare
		VectorCopy( vieworg, viewPoint );
		cls.envshot_vieworg = viewPoint;
		cls.envshot_disable_vis = true;
	}

	// make request for envshot
	if( skyshot ) cls.scrshot_action = scrshot_skyshot;
	else cls.scrshot_action = scrshot_envshot;

	// catch negative values
	cls.envshot_viewsize = max( 0, shotsize );
}

static void R_SetCurrentEntity( cl_entity_t *ent )
{
	RI.currententity = ent;

	// set model also
	if( RI.currententity != NULL )
	{
		RI.currentmodel = RI.currententity->model;
	}
}

static void R_SetCurrentModel( model_t *mod )
{
	RI.currentmodel = mod;
}

static lightstyle_t *CL_GetLightStyle( int number )
{
	ASSERT( number >= 0 && number < MAX_LIGHTSTYLES );
	return &cl.lightstyles[number];
}

static dlight_t *CL_GetDynamicLight( int number )
{
	ASSERT( number >= 0 && number < MAX_DLIGHTS );
	return &cl_dlights[number];
}

static dlight_t *CL_GetEntityLight( int number )
{
	ASSERT( number >= 0 && number < MAX_ELIGHTS );
	return &cl_elights[number];
}

static void CL_GetBeamChains( BEAM ***active_beams, BEAM ***free_beams, particle_t ***free_trails )
{
	*active_beams = &cl_active_beams;
	*free_beams = &cl_free_beams;
	*free_trails = &cl_free_trails; 
}

static void GL_SetWorldviewProjectionMatrix( const float *glmatrix )
{
	if( !glmatrix ) return;

	Matrix4x4_FromArrayFloatGL( RI.worldviewProjectionMatrix, glmatrix );
}

static const char *GL_TextureName( unsigned int texnum )
{
	return R_GetTexture( texnum )->name;	
}

static const byte *GL_TextureData( unsigned int texnum )
{
	rgbdata_t *pic = R_GetTexture( texnum )->original;

	if( pic != NULL )
		return pic->buffer;
	return NULL;	
}

static int GL_LoadTextureNoFilter( const char *name, const byte *buf, size_t size, int flags )
{
	return GL_LoadTexture( name, buf, size, flags, NULL );	
}

static const ref_overview_t *GL_GetOverviewParms( void )
{
	return &clgame.overView;
}

static void *R_Mem_Alloc( size_t cb, const char *filename, const int fileline )
{
	return _Mem_Alloc( cls.mempool, cb, filename, fileline );
}

static void R_Mem_Free( void *mem, const char *filename, const int fileline )
{
	_Mem_Free( mem, filename, fileline );
}

/*
=========
pfnGetFilesList

=========
*/
static char **pfnGetFilesList( const char *pattern, int *numFiles, int gamedironly )
{
	static search_t	*t = NULL;

	if( t ) Mem_Free( t ); // release prev search

	t = FS_Search( pattern, true, gamedironly );

	if( !t )
	{
		if( numFiles ) *numFiles = 0;
		return NULL;
	}

	if( numFiles ) *numFiles = t->numfilenames;
	return t->filenames;
}
	
static render_api_t gRenderAPI =
{
	GL_RenderGetParm,
	R_GetDetailScaleForTexture,
	R_GetExtraParmsForTexture,
	CL_GetLightStyle,
	CL_GetDynamicLight,
	CL_GetEntityLight,
	TextureToTexGamma,
	CL_GetBeamChains,
	R_SetCurrentEntity,
	R_SetCurrentModel,
	GL_SetWorldviewProjectionMatrix,
	R_StoreEfrags,
	GL_FindTexture,
	GL_TextureName,
	GL_TextureData,
	GL_LoadTextureNoFilter,
	GL_CreateTexture,
	GL_SetTextureType,
	GL_TextureUpdateCache,
	GL_FreeTexture,
	DrawSingleDecal,
	R_DecalSetupVerts,
	R_EntityRemoveDecals,
	AVI_LoadVideoNoSound,
	AVI_GetVideoInfo,
	AVI_GetVideoFrameNumber,
	AVI_GetVideoFrame,
	R_UploadStretchRaw,
	AVI_FreeVideo,
	AVI_IsActive,
	GL_Bind,
	GL_SelectTexture,
	GL_LoadTexMatrixExt,
	GL_LoadIdentityTexMatrix,
	GL_CleanUpTextureUnits,
	GL_TexGen,
	GL_TextureTarget,
	GL_SetTexCoordArrayMode,
	NULL,
	NULL,
	NULL,
	NULL,
	CL_DrawParticlesExternal,
	R_EnvShot,
	COM_CompareFileTime,
	Host_Error,
	pfnSPR_LoadExt,
	Mod_TesselatePolygon,
	R_StudioGetTexture,
	GL_GetOverviewParms,
	S_FadeMusicVolume,
	COM_SetRandomSeed,
	R_Mem_Alloc,
	R_Mem_Free,
	pfnGetFilesList,
};

/*
===============
R_InitRenderAPI

Initialize client external rendering
===============
*/
qboolean R_InitRenderAPI( void )
{
	// make sure what render functions is cleared
	Q_memset( &clgame.drawFuncs, 0, sizeof( clgame.drawFuncs ));

	if( clgame.dllFuncs.pfnGetRenderInterface )
	{
		if( clgame.dllFuncs.pfnGetRenderInterface( CL_RENDER_INTERFACE_VERSION, &gRenderAPI, &clgame.drawFuncs ))
		{
			MsgDev( D_AICONSOLE, "CL_LoadProgs: ^2initailized extended RenderAPI ^7ver. %i\n", CL_RENDER_INTERFACE_VERSION );
			return true;
		}

		// make sure what render functions is cleared
		Q_memset( &clgame.drawFuncs, 0, sizeof( clgame.drawFuncs ));

		return false; // just tell user about problems
	}

	// render interface is missed
	return true;
}