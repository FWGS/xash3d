/*
cl_pmove.c - client-side player physic
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
#include "const.h"
#include "cl_tent.h"
#include "pm_local.h"
#include "particledef.h"
#include "studio.h"

void CL_ClearPhysEnts( void )
{
	clgame.pmove->numtouch = 0;
	clgame.pmove->numvisent = 0;
	clgame.pmove->nummoveent = 0;
	clgame.pmove->numphysent = 0;
}

void CL_ClipPMoveToEntity( physent_t *pe, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, pmtrace_t *tr )
{
	ASSERT( tr != NULL );

	if( clgame.dllFuncs.pfnClipMoveToEntity != NULL )
	{
		// do custom sweep test
		clgame.dllFuncs.pfnClipMoveToEntity( pe, start, mins, maxs, end, tr );
	}
	else
	{
		// function is missed, so we didn't hit anything
		tr->allsolid = false;
	}
}

qboolean CL_CopyEntityToPhysEnt( physent_t *pe, cl_entity_t *ent )
{
	model_t	*mod = Mod_Handle( ent->curstate.modelindex );

	if( !mod ) return false;
	pe->player = false;

	if( ent->player )
	{
		// client or bot
		Q_strncpy( pe->name, "player", sizeof( pe->name ));
		pe->player = (int)(ent - clgame.entities);
	}
	else
	{
		// otherwise copy the modelname
		Q_strncpy( pe->name, mod->name, sizeof( pe->name ));
	}

	pe->model = pe->studiomodel = NULL;

	switch( ent->curstate.solid )
	{
	case SOLID_NOT:
	case SOLID_BSP:
		pe->model = mod;
		VectorClear( pe->mins );
		VectorClear( pe->maxs );
		break;
	case SOLID_BBOX:
		if( mod && mod->type == mod_studio && mod->flags & STUDIO_TRACE_HITBOX )
			pe->studiomodel = mod;
		VectorCopy( ent->curstate.mins, pe->mins );
		VectorCopy( ent->curstate.maxs, pe->maxs );
		break;
	case SOLID_CUSTOM:
		pe->model = (mod->type == mod_brush) ? mod : NULL;
		pe->studiomodel = (mod->type == mod_studio) ? mod : NULL;
		VectorCopy( ent->curstate.mins, pe->mins );
		VectorCopy( ent->curstate.maxs, pe->maxs );
		break;
	default:
		pe->studiomodel = (mod->type == mod_studio) ? mod : NULL;
		VectorCopy( ent->curstate.mins, pe->mins );
		VectorCopy( ent->curstate.maxs, pe->maxs );
		break;
	}

	pe->info = (int)(ent - clgame.entities);
	VectorCopy( ent->curstate.origin, pe->origin );
	VectorCopy( ent->curstate.angles, pe->angles );

	pe->solid = ent->curstate.solid;
	pe->rendermode = ent->curstate.rendermode;
	pe->skin = ent->curstate.skin;
	pe->frame = ent->curstate.frame;
	pe->sequence = ent->curstate.sequence;

	Q_memcpy( &pe->controller[0], &ent->curstate.controller[0], 4 * sizeof( byte ));
	Q_memcpy( &pe->blending[0], &ent->curstate.blending[0], 2 * sizeof( byte ));

	pe->movetype = ent->curstate.movetype;
	pe->takedamage = ( pe->player ) ? DAMAGE_AIM : DAMAGE_YES;
	pe->team = ent->curstate.team;
	pe->classnumber = ent->curstate.playerclass;
	pe->blooddecal = 0;	// unused in GoldSrc

	// for mods
	pe->iuser1 = ent->curstate.iuser1;
	pe->iuser2 = ent->curstate.iuser2;
	pe->iuser3 = ent->curstate.iuser3;
	pe->iuser4 = ent->curstate.iuser4;
	pe->fuser1 = ent->curstate.fuser1;
	pe->fuser2 = ent->curstate.fuser2;
	pe->fuser3 = ent->curstate.fuser3;
	pe->fuser4 = ent->curstate.fuser4;

	VectorCopy( ent->curstate.vuser1, pe->vuser1 );
	VectorCopy( ent->curstate.vuser2, pe->vuser2 );
	VectorCopy( ent->curstate.vuser3, pe->vuser3 );
	VectorCopy( ent->curstate.vuser4, pe->vuser4 );

	return true;
}

/*
====================
CL_AddLinksToPmove

collect solid entities
====================
*/
void CL_AddLinksToPmove( void )
{
	cl_entity_t	*check;
	physent_t		*pe;
	int		i, solid, idx;

	for( i = 0; i < cl.frame.num_entities; i++ )
	{
		idx = cls.packet_entities[(cl.frame.first_entity + i) % cls.num_client_entities].number;
		check = CL_GetEntityByIndex( idx );

		// don't add the world and clients here
		if( !check || check == &clgame.entities[0] )
			continue;

		if( clgame.pmove->numvisent < MAX_PHYSENTS )
		{
			pe = &clgame.pmove->visents[clgame.pmove->numvisent];
			if( CL_CopyEntityToPhysEnt( pe, check ))
				clgame.pmove->numvisent++;
		}

		// players will be added later
		if( check->player ) continue;

		// can't collide with zeroed hull
		if( VectorIsNull( check->curstate.mins ) && VectorIsNull( check->curstate.maxs ))
			continue;

		solid = check->curstate.solid;

		if( solid == SOLID_BSP || solid == SOLID_BBOX || solid == SOLID_SLIDEBOX || solid == SOLID_CUSTOM )
		{
			// reserve slots for all the clients
			if( clgame.pmove->numphysent < ( MAX_PHYSENTS - cl.maxclients ))
			{
				pe = &clgame.pmove->physents[clgame.pmove->numphysent];
				if( CL_CopyEntityToPhysEnt( pe, check ))
					clgame.pmove->numphysent++;
			}
		}
		else if( solid == SOLID_NOT && check->curstate.skin != CONTENTS_NONE )
		{
			if( clgame.pmove->nummoveent < MAX_MOVEENTS )
			{
				pe = &clgame.pmove->moveents[clgame.pmove->nummoveent];
				if( CL_CopyEntityToPhysEnt( pe, check ))
					clgame.pmove->nummoveent++;
			}
		}
	}
}

/*
===============
CL_SetSolid

Builds all the pmove physents for the current frame
Note that CL_SetUpPlayerPrediction() must be called first!
pmove must be setup with world and solid entity hulls before calling
(via CL_PredictMove)
===============
*/
void CL_SetSolidPlayers( int playernum )
{
	int		j;
	extern	vec3_t	player_mins;
	extern	vec3_t	player_maxs;
	cl_entity_t	*ent;
	physent_t		*pe;

	if( !cl_solid_players->integer )
		return;

	for( j = 0; j < cl.maxclients; j++ )
	{
		// the player object never gets added
		if( j == playernum ) continue;

		ent = CL_GetEntityByIndex( j + 1 );		

		if( !ent || !ent->player )
			continue; // not present this frame

		pe = &clgame.pmove->physents[clgame.pmove->numphysent];
		if( CL_CopyEntityToPhysEnt( pe, ent ))
			clgame.pmove->numphysent++;
	}
}

/*
=============
CL_TruePointContents

=============
*/
int CL_TruePointContents( const vec3_t p )
{
	int	i, contents;
	int	oldhull;
	hull_t	*hull;
	vec3_t	test, offset;
	physent_t	*pe;

	// sanity check
	if( !p ) return CONTENTS_NONE;

	oldhull = clgame.pmove->usehull;

	// get base contents from world
	contents = PM_HullPointContents( &cl.worldmodel->hulls[0], 0, p );

	for( i = 0; i < clgame.pmove->nummoveent; i++ )
	{
		pe = &clgame.pmove->moveents[i];

		if( pe->solid != SOLID_NOT ) // disabled ?
			continue;

		// only brushes can have special contents
		if( !pe->model || pe->model->type != mod_brush )
			continue;

		// check water brushes accuracy
		clgame.pmove->usehull = 2;
		hull = PM_HullForBsp( pe, clgame.pmove, offset );
		clgame.pmove->usehull = oldhull;

		// offset the test point appropriately for this hull.
		VectorSubtract( p, offset, test );

		if( (pe->model->flags & MODEL_HAS_ORIGIN) && !VectorIsNull( pe->angles ))
		{
			matrix4x4	matrix;
	
			Matrix4x4_CreateFromEntity( matrix, pe->angles, offset, 1.0f );
			Matrix4x4_VectorITransform( matrix, p, test );
		}

		// test hull for intersection with this model
		if( PM_HullPointContents( hull, hull->firstclipnode, test ) == CONTENTS_EMPTY )
			continue;

		// compare contents ranking
		if( RankForContents( pe->skin ) > RankForContents( contents ))
			contents = pe->skin; // new content has more priority
	}

	return contents;
}

/*
=============
CL_WaterEntity

=============
*/
int CL_WaterEntity( const float *rgflPos )
{
	physent_t		*pe;
	hull_t		*hull;
	vec3_t		test, offset;
	int		i, oldhull;

	if( !rgflPos ) return -1;

	oldhull = clgame.pmove->usehull;

	for( i = 0; i < clgame.pmove->nummoveent; i++ )
	{
		pe = &clgame.pmove->moveents[i];

		if( pe->solid != SOLID_NOT ) // disabled ?
			continue;

		// only brushes can have special contents
		if( !pe->model || pe->model->type != mod_brush )
			continue;

		// check water brushes accuracy
		clgame.pmove->usehull = 2;
		hull = PM_HullForBsp( pe, clgame.pmove, offset );
		clgame.pmove->usehull = oldhull;

		// offset the test point appropriately for this hull.
		VectorSubtract( rgflPos, offset, test );

		if( (pe->model->flags & MODEL_HAS_ORIGIN) && !VectorIsNull( pe->angles ))
		{
			matrix4x4	matrix;
	
			Matrix4x4_CreateFromEntity( matrix, pe->angles, offset, 1.0f );
			Matrix4x4_VectorITransform( matrix, rgflPos, test );
		}

		// test hull for intersection with this model
		if( PM_HullPointContents( hull, hull->firstclipnode, test ) == CONTENTS_EMPTY )
			continue;

		// found water entity
		return pe->info;
	}
	return -1;
}

/*
=============
CL_TraceLine

a simple engine traceline
=============
*/
pmtrace_t CL_TraceLine( vec3_t start, vec3_t end, int flags )
{
	int	old_usehull;
	pmtrace_t	tr;
	
	old_usehull = clgame.pmove->usehull;
	clgame.pmove->usehull = 2;
	tr = PM_PlayerTraceExt( clgame.pmove, start, end, flags, clgame.pmove->numphysent, clgame.pmove->physents, -1, NULL );
	clgame.pmove->usehull = old_usehull;

	return tr;
}

/*
=============
CL_GetWaterEntity

returns water brush where inside pos
=============
*/
cl_entity_t *CL_GetWaterEntity( const float *rgflPos )
{
	int	entnum;

	entnum = CL_WaterEntity( rgflPos );
	if( entnum <= 0 ) return NULL; // world or not water

	return CL_GetEntityByIndex( entnum );
}

static void pfnParticle( float *origin, int color, float life, int zpos, int zvel )
{
	particle_t	*p;
		
	if( !origin )
	{
		MsgDev( D_ERROR, "CL_StartParticle: NULL origin. Ignored\n" );
		return;
	}

	p = CL_AllocParticle( NULL );
	if( !p ) return;

	p->die += life;
	p->color = color;
	p->type = pt_static;

	VectorCopy( origin, p->org );
	VectorSet( p->vel, 0.0f, 0.0f, ( zpos * zvel ));
}

static int pfnTestPlayerPosition( float *pos, pmtrace_t *ptrace )
{
	return PM_TestPlayerPosition( clgame.pmove, pos, ptrace, NULL );
}

static void pfnStuckTouch( int hitent, pmtrace_t *tr )
{
	int	i;

	for( i = 0; i < clgame.pmove->numtouch; i++ )
	{
		if( clgame.pmove->touchindex[i].ent == hitent )
			return;
	}

	if( clgame.pmove->numtouch >= MAX_PHYSENTS )
	{
		MsgDev( D_ERROR, "PM_StuckTouch: MAX_TOUCHENTS limit exceeded\n" );
		return;
	}

	VectorCopy( clgame.pmove->velocity, tr->deltavelocity );
	tr->ent = hitent;

	clgame.pmove->touchindex[clgame.pmove->numtouch++] = *tr;
}

static int pfnPointContents( float *p, int *truecontents )
{
	int	cont, truecont;

	truecont = cont = CL_TruePointContents( p );
	if( truecontents ) *truecontents = truecont;

	if( cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN )
		cont = CONTENTS_WATER;
	return cont;
}

static int pfnTruePointContents( float *p )
{
	return CL_TruePointContents( p );
}

static int pfnHullPointContents( struct hull_s *hull, int num, float *p )
{
	return PM_HullPointContents( hull, num, p );
}

static pmtrace_t pfnPlayerTrace( float *start, float *end, int traceFlags, int ignore_pe )
{
	return PM_PlayerTraceExt( clgame.pmove, start, end, traceFlags, clgame.pmove->numphysent, clgame.pmove->physents, ignore_pe, NULL );
}

static pmtrace_t *pfnTraceLine( float *start, float *end, int flags, int usehull, int ignore_pe )
{
	static pmtrace_t	tr;
	int		old_usehull;

	old_usehull = clgame.pmove->usehull;
	clgame.pmove->usehull = usehull;	

	switch( flags )
	{
	case PM_TRACELINE_PHYSENTSONLY:
		tr = PM_PlayerTraceExt( clgame.pmove, start, end, 0, clgame.pmove->numphysent, clgame.pmove->physents, ignore_pe, NULL );
		break;
	case PM_TRACELINE_ANYVISIBLE:
		tr = PM_PlayerTraceExt( clgame.pmove, start, end, 0, clgame.pmove->numvisent, clgame.pmove->visents, ignore_pe, NULL );
		break;
	}

	clgame.pmove->usehull = old_usehull;

	return &tr;
}

static hull_t *pfnHullForBsp( physent_t *pe, float *offset )
{
	return PM_HullForBsp( pe, clgame.pmove, offset );
}

static float pfnTraceModel( physent_t *pe, float *start, float *end, trace_t *trace )
{
	int	old_usehull;
	vec3_t	start_l, end_l;
	vec3_t	offset, temp;
	qboolean	rotated;
	matrix4x4	matrix;
	hull_t	*hull;

	old_usehull = clgame.pmove->usehull;
	clgame.pmove->usehull = 2;

	hull = PM_HullForBsp( pe, clgame.pmove, offset );

	clgame.pmove->usehull = old_usehull;

	if( pe->solid == SOLID_BSP && !VectorIsNull( pe->angles ))
		rotated = true;
	else rotated = false;

 	if( rotated )
 	{
 		Matrix4x4_CreateFromEntity( matrix, pe->angles, offset, 1.0f );
 		Matrix4x4_VectorITransform( matrix, start, start_l );
 		Matrix4x4_VectorITransform( matrix, end, end_l );
 	}
 	else
 	{
 		VectorSubtract( start, offset, start_l );
 		VectorSubtract( end, offset, end_l );
 	}

	SV_RecursiveHullCheck( hull, hull->firstclipnode, 0, 1, start_l, end_l, trace );
	trace->ent = NULL;

	if( rotated )
	{
		VectorCopy( trace->plane.normal, temp );
		Matrix4x4_TransformPositivePlane( matrix, temp, trace->plane.dist, trace->plane.normal, &trace->plane.dist );
	}

	VectorLerp( start, trace->fraction, end, trace->endpos );

	return trace->fraction;
}

static const char *pfnTraceTexture( int ground, float *vstart, float *vend )
{
	physent_t *pe;

	if( ground < 0 || ground >= clgame.pmove->numphysent )
		return NULL; // bad ground

	pe = &clgame.pmove->physents[ground];
	return PM_TraceTexture( pe, vstart, vend );
}			

static void pfnPlaySound( int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch )
{
	sound_t	snd = S_RegisterSound( sample );

	S_StartSound( NULL, clgame.pmove->player_index + 1, channel, snd, volume, attenuation, pitch, fFlags );
}

static void pfnPlaybackEventFull( int flags, int clientindex, word eventindex, float delay, float *origin,
	float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 )
{
	cl_entity_t	*ent;

	ent = CL_GetEntityByIndex( clientindex + 1 );
	if( ent == NULL ) return;

	CL_PlaybackEvent( flags, (edict_t *)ent, eventindex,
		delay, origin, angles,
		fparam1, fparam2,
		iparam1, iparam2,
		bparam1, bparam2 );
}

static pmtrace_t pfnPlayerTraceEx( float *start, float *end, int traceFlags, pfnIgnore pmFilter )
{
	return PM_PlayerTraceExt( clgame.pmove, start, end, traceFlags, clgame.pmove->numphysent, clgame.pmove->physents, -1, pmFilter );
}

static int pfnTestPlayerPositionEx( float *pos, pmtrace_t *ptrace, pfnIgnore pmFilter )
{
	return PM_TestPlayerPosition( clgame.pmove, pos, ptrace, pmFilter );
}

static pmtrace_t *pfnTraceLineEx( float *start, float *end, int flags, int usehull, pfnIgnore pmFilter )
{
	static pmtrace_t	tr;
	int		old_usehull;

	old_usehull = clgame.pmove->usehull;
	clgame.pmove->usehull = usehull;	

	switch( flags )
	{
	case PM_TRACELINE_PHYSENTSONLY:
		tr = PM_PlayerTraceExt( clgame.pmove, start, end, 0, clgame.pmove->numphysent, clgame.pmove->physents, -1, pmFilter );
		break;
	case PM_TRACELINE_ANYVISIBLE:
		tr = PM_PlayerTraceExt( clgame.pmove, start, end, 0, clgame.pmove->numvisent, clgame.pmove->visents, -1, pmFilter );
		break;
	}

	clgame.pmove->usehull = old_usehull;

	return &tr;
}

/*
===============
CL_InitClientMove

===============
*/
void CL_InitClientMove( void )
{
	int	i;

	Pmove_Init ();

	clgame.pmove->server = false;	// running at client
	clgame.pmove->movevars = &clgame.movevars;
	clgame.pmove->runfuncs = false;

	Mod_SetupHulls( clgame.player_mins, clgame.player_maxs );

	// enumerate client hulls
	for( i = 0; i < MAX_MAP_HULLS; i++ )
	{
		if( clgame.dllFuncs.pfnGetHullBounds( i, clgame.player_mins[i], clgame.player_maxs[i] ))
			MsgDev( D_NOTE, "CL: hull%i, player_mins: %g %g %g, player_maxs: %g %g %g\n", i,
			clgame.player_mins[i][0], clgame.player_mins[i][1], clgame.player_mins[i][2],
			clgame.player_maxs[i][0], clgame.player_maxs[i][1], clgame.player_maxs[i][2] );
	}

	Q_memcpy( clgame.pmove->player_mins, clgame.player_mins, sizeof( clgame.player_mins ));
	Q_memcpy( clgame.pmove->player_maxs, clgame.player_maxs, sizeof( clgame.player_maxs ));

	// common utilities
	clgame.pmove->PM_Info_ValueForKey = Info_ValueForKey;
	clgame.pmove->PM_Particle = pfnParticle;
	clgame.pmove->PM_TestPlayerPosition = pfnTestPlayerPosition;
	clgame.pmove->Con_NPrintf = Con_NPrintf;
	clgame.pmove->Con_DPrintf = Con_DPrintf;
	clgame.pmove->Con_Printf = Con_Printf;
	clgame.pmove->Sys_FloatTime = Sys_DoubleTime;
	clgame.pmove->PM_StuckTouch = pfnStuckTouch;
	clgame.pmove->PM_PointContents = pfnPointContents;
	clgame.pmove->PM_TruePointContents = pfnTruePointContents;
	clgame.pmove->PM_HullPointContents = pfnHullPointContents; 
	clgame.pmove->PM_PlayerTrace = pfnPlayerTrace;
	clgame.pmove->PM_TraceLine = pfnTraceLine;
	clgame.pmove->RandomLong = Com_RandomLong;
	clgame.pmove->RandomFloat = Com_RandomFloat;
	clgame.pmove->PM_GetModelType = pfnGetModelType;
	clgame.pmove->PM_GetModelBounds = pfnGetModelBounds;	
	clgame.pmove->PM_HullForBsp = pfnHullForBsp;
	clgame.pmove->PM_TraceModel = pfnTraceModel;
	clgame.pmove->COM_FileSize = COM_FileSize;
	clgame.pmove->COM_LoadFile = COM_LoadFile;
	clgame.pmove->COM_FreeFile = COM_FreeFile;
	clgame.pmove->memfgets = COM_MemFgets;
	clgame.pmove->PM_PlaySound = pfnPlaySound;
	clgame.pmove->PM_TraceTexture = pfnTraceTexture;
	clgame.pmove->PM_PlaybackEventFull = pfnPlaybackEventFull;
	clgame.pmove->PM_PlayerTraceEx = pfnPlayerTraceEx;
	clgame.pmove->PM_TestPlayerPositionEx = pfnTestPlayerPositionEx;
	clgame.pmove->PM_TraceLineEx = pfnTraceLineEx;

	// initalize pmove
	clgame.dllFuncs.pfnPlayerMoveInit( clgame.pmove );
}

static void PM_CheckMovingGround( clientdata_t *cd, entity_state_t *state, float frametime )
{
	if(!( cd->flags & FL_BASEVELOCITY ))
	{
		// apply momentum (add in half of the previous frame of velocity first)
		VectorMA( cd->velocity, 1.0f + (frametime * 0.5f), state->basevelocity, cd->velocity );
		VectorClear( state->basevelocity );
	}
	cd->flags &= ~FL_BASEVELOCITY;
}

void CL_SetSolidEntities( void )
{
	// world not initialized
	if( !clgame.entities )
		return;

	// setup physents
	clgame.pmove->numvisent = 0;
	clgame.pmove->numphysent = 0;
	clgame.pmove->nummoveent = 0;

	CL_CopyEntityToPhysEnt( &clgame.pmove->physents[0], &clgame.entities[0] );
	clgame.pmove->visents[0] = clgame.pmove->physents[0];
	clgame.pmove->numphysent = 1;	// always have world
	clgame.pmove->numvisent = 1;	

	if( cls.state == ca_active && cl.frame.valid )
	{
		CL_AddLinksToPmove();
	}
}

void CL_SetupPMove( playermove_t *pmove, clientdata_t *cd, entity_state_t *state, usercmd_t *ucmd )
{
	pmove->player_index = cl.playernum;
	pmove->multiplayer = (cl.maxclients > 1) ? true : false;
	pmove->time = cl.time; // probably never used
	VectorCopy( cd->origin, pmove->origin );
	VectorCopy( cl.refdef.cl_viewangles, pmove->angles );
	VectorCopy( cl.refdef.cl_viewangles, pmove->oldangles );
	VectorCopy( cd->velocity, pmove->velocity );
	VectorCopy( state->basevelocity, pmove->basevelocity );
	VectorCopy( cd->view_ofs, pmove->view_ofs );
	VectorClear( pmove->movedir );
	pmove->flDuckTime = cd->flDuckTime;
	pmove->bInDuck = cd->bInDuck;
	pmove->usehull = (cd->flags & FL_DUCKING) ? 1 : 0; // reset hull
	pmove->flTimeStepSound = cd->flTimeStepSound;
	pmove->iStepLeft = state->iStepLeft;
	pmove->flFallVelocity = state->flFallVelocity;
	pmove->flSwimTime = cd->flSwimTime;
	VectorCopy( cd->punchangle, pmove->punchangle );
	pmove->flSwimTime = cd->flSwimTime;
	pmove->flNextPrimaryAttack = 0.0f; // not used by PM_ code
	pmove->effects = state->effects;
	pmove->flags = cd->flags;
	pmove->gravity = state->gravity;
	pmove->friction = state->friction;
	pmove->oldbuttons = state->oldbuttons;
	pmove->waterjumptime = cd->waterjumptime;
	pmove->dead = (cd->health <= 0.0f ) ? true : false;
	pmove->deadflag = cd->deadflag;
	pmove->spectator = (state->spectator != 0);
	pmove->movetype = state->movetype;
	pmove->onground = -1; // will be set by PM_ code
	pmove->waterlevel = cd->waterlevel;
	pmove->watertype = cd->watertype;
	pmove->maxspeed = clgame.movevars.maxspeed;
	pmove->clientmaxspeed = cd->maxspeed;
	pmove->iuser1 = cd->iuser1;
	pmove->iuser2 = cd->iuser2;
	pmove->iuser3 = cd->iuser3;
	pmove->iuser4 = cd->iuser4;
	pmove->fuser1 = cd->fuser1;
	pmove->fuser2 = cd->fuser2;
	pmove->fuser3 = cd->fuser3;
	pmove->fuser4 = cd->fuser4;
	VectorCopy( cd->vuser1, pmove->vuser1 );
	VectorCopy( cd->vuser2, pmove->vuser2 );
	VectorCopy( cd->vuser3, pmove->vuser3 );
	VectorCopy( cd->vuser4, pmove->vuser4 );
	pmove->cmd = *ucmd;	// setup current cmds	

	Q_strncpy( pmove->physinfo, cd->physinfo, MAX_INFO_STRING );
}

/*
===========
CL_PostRunCmd

Done after running a player command.
===========
*/
void CL_PostRunCmd( usercmd_t *ucmd, int random_seed )
{
	local_state_t	*from, *to;

	// TODO: write real predicting code

	from = &cl.predict[cl.predictcount & CL_UPDATE_MASK];
	to = &cl.predict[(cl.predictcount + 1) & CL_UPDATE_MASK];

	*to = *from;

	clgame.dllFuncs.pfnPostRunCmd( from, to, ucmd, clgame.pmove->runfuncs, cl.time, random_seed );
	cl.predictcount++;
}

/*
=================
CL_PredictMovement

Sets cl.predicted_origin and cl.predicted_angles
=================
*/
void CL_PredictMovement( void )
{
	int		frame = 1;
	int		ack, outgoing_command;
	int		current_command;
	int		current_command_mod;
	cl_entity_t	*player, *viewent;
	clientdata_t	*cd;

	if( cls.state != ca_active ) return;

	if( cls.demoplayback && cl.refdef.cmd != NULL )
	{
		// restore viewangles from cmd.angles
		VectorCopy( cl.refdef.cmd->viewangles, cl.refdef.cl_viewangles );
	}

	if( cl.refdef.paused || cls.key_dest == key_menu ) return;

	player = CL_GetLocalPlayer ();
	viewent = CL_GetEntityByIndex( cl.refdef.viewentity );
	cd = &cl.frame.local.client;

	// unpredicted pure angled values converted into axis
	AngleVectors( cl.refdef.cl_viewangles, cl.refdef.forward, cl.refdef.right, cl.refdef.up );

	if( !CL_IsPredicted( ))
	{	
		// run commands even if client predicting is disabled - client expected it
		clgame.pmove->runfuncs = true;
		CL_PostRunCmd( cl.refdef.cmd, cls.lastoutgoingcommand );
		return;
	}

	ack = cls.netchan.incoming_acknowledged;
	outgoing_command = cls.netchan.outgoing_sequence;

	ASSERT( cl.refdef.cmd != NULL );

	// setup initial pmove state
	CL_SetupPMove( clgame.pmove, cd, &player->curstate, cl.refdef.cmd );
	clgame.pmove->runfuncs = false;

	while( 1 )
	{
		// we've run too far forward
		if( frame >= CL_UPDATE_BACKUP - 1 )
			break;

		// Incoming_acknowledged is the last usercmd the server acknowledged having acted upon
		current_command = ack + frame;
		current_command_mod = current_command & CL_UPDATE_MASK;

		// we've caught up to the current command.
		if( current_command > outgoing_command )
			break;

		clgame.pmove->cmd = cl.cmds[frame];

		// motor!
		clgame.dllFuncs.pfnPlayerMove( clgame.pmove, false ); // run frames
		clgame.pmove->runfuncs = ( current_command > outgoing_command - 1 ) ? true : false;

		frame++;
	}

	CL_PostRunCmd( cl.refdef.cmd, frame );
		
	// copy results out for rendering
	VectorCopy( clgame.pmove->view_ofs, cl.predicted_viewofs );
	VectorCopy( clgame.pmove->origin, cl.predicted_origin );
	VectorCopy( clgame.pmove->velocity, cl.predicted_velocity );
}