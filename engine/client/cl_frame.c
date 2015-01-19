/*
cl_frame.c - client world snapshot
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
#include "client.h"
#include "net_encode.h"
#include "entity_types.h"
#include "gl_local.h"
#include "pm_local.h"
#include "cl_tent.h"
#include "studio.h"
#include "dlight.h"
#include "input.h"

#define MAX_FORWARD		6

qboolean CL_IsPlayerIndex( int idx )
{
	if( idx > 0 && idx <= cl.maxclients )
		return true;
	return false;
}

qboolean CL_IsPredicted( void )
{
	if( !cl_predict->integer || !cl.frame.valid )
		return false;

	if(( cls.netchan.outgoing_sequence - cls.netchan.incoming_sequence ) >= ( CL_UPDATE_BACKUP - 1 ))
		return false;

	return true;
}

int CL_PushMoveFilter( physent_t *pe )
{
	if( !pe || pe->solid != SOLID_BSP || pe->movetype != MOVETYPE_PUSH )
		return 1;

	// optimization. Ignore world to avoid
	// unneeded transformations
	if( pe->info == 0 )
		return 1;

	return 0;
}

/*
=========================================================================

FRAME PARSING

=========================================================================
*/
void CL_UpdateEntityFields( cl_entity_t *ent )
{
	// parametric rockets code
	if( ent->curstate.starttime != 0.0f && ent->curstate.impacttime != 0.0f )
	{
		float	lerp = ( cl.time - ent->curstate.starttime ) / ( ent->curstate.impacttime - ent->curstate.starttime );
		vec3_t	dir;

		lerp = bound( 0.0f, lerp, 1.0f );

		// first we need to calc actual origin
		VectorLerp( ent->curstate.startpos, lerp, ent->curstate.endpos, ent->curstate.origin );
		VectorSubtract( ent->curstate.endpos, ent->curstate.startpos, dir );
		VectorAngles( dir, ent->curstate.angles ); // re-aim projectile		
	}

	VectorCopy( ent->curstate.origin, ent->origin );
	VectorCopy( ent->curstate.angles, ent->angles );

	ent->model = Mod_Handle( ent->curstate.modelindex );
	ent->curstate.msg_time = cl.time;

	if( ent->player && RP_LOCALCLIENT( ent )) // stupid Half-Life bug
		ent->angles[PITCH] = -ent->angles[PITCH] / 3.0f;

	// make me lerp
	if( ent->model && ent->model->type == mod_brush && ent->curstate.animtime != 0.0f )
	{
		float		d, f = 0.0f;
		int		i;

		// don't do it if the goalstarttime hasn't updated in a while.
		// NOTE: Because we need to interpolate multiplayer characters, the interpolation time limit
		// was increased to 1.0 s., which is 2x the max lag we are accounting for.
		if(( cl.time < ent->curstate.animtime + 1.0f ) && ( ent->curstate.animtime != ent->latched.prevanimtime ))
			f = ( cl.time - ent->curstate.animtime ) / ( ent->curstate.animtime - ent->latched.prevanimtime );

		f = f - 1.0f;

		ent->origin[0] += ( ent->origin[0] - ent->latched.prevorigin[0] ) * f;
		ent->origin[1] += ( ent->origin[1] - ent->latched.prevorigin[1] ) * f;
		ent->origin[2] += ( ent->origin[2] - ent->latched.prevorigin[2] ) * f;

		for( i = 0; i < 3; i++ )
		{
			float	ang1, ang2;

			ang1 = ent->angles[i];
			ang2 = ent->latched.prevangles[i];
			d = ang1 - ang2;
			if( d > 180.0f ) d -= 360.0f;
			else if( d < -180.0f ) d += 360.0f;
			ent->angles[i] += d * f;
		}
	}
	else if( ent->curstate.eflags & EFLAG_SLERP )
	{
		float		d, f = 0.0f;
		cl_entity_t	*m_pGround = NULL;
		int		i;

		// don't do it if the goalstarttime hasn't updated in a while.
		// NOTE: Because we need to interpolate multiplayer characters, the interpolation time limit
		// was increased to 1.0 s., which is 2x the max lag we are accounting for.
		if(( cl.time < ent->curstate.animtime + 1.0f ) && ( ent->curstate.animtime != ent->latched.prevanimtime ))
			f = ( cl.time - ent->curstate.animtime ) / ( ent->curstate.animtime - ent->latched.prevanimtime );

		f = f - 1.0f;

		if( ent->curstate.movetype == MOVETYPE_FLY )
		{
			ent->origin[0] += ( ent->curstate.origin[0] - ent->latched.prevorigin[0] ) * f;
			ent->origin[1] += ( ent->curstate.origin[1] - ent->latched.prevorigin[1] ) * f;
			ent->origin[2] += ( ent->curstate.origin[2] - ent->latched.prevorigin[2] ) * f;

			for( i = 0; i < 3; i++ )
			{
				float	ang1, ang2;

				ang1 = ent->curstate.angles[i];
				ang2 = ent->latched.prevangles[i];
				d = ang1 - ang2;
				if( d > 180.0f ) d -= 360.0f;
				else if( d < -180.0f ) d += 360.0f;
				ent->angles[i] += d * f;
			}
		}
		else if( ent->curstate.movetype == MOVETYPE_STEP )
		{
			vec3_t	vecSrc, vecEnd;
			pmtrace_t	trace;

			if( ent->model )
			{
				CL_SetTraceHull( 0 ); // g-cont. player hull for better detect moving platforms
				VectorSet( vecSrc, ent->origin[0], ent->origin[1], ent->origin[2] + ent->model->maxs[2] );
				VectorSet( vecEnd, vecSrc[0], vecSrc[1], vecSrc[2] - ent->model->mins[2] - 8.0f );		
				CL_PlayerTraceExt( vecSrc, vecEnd, PM_STUDIO_IGNORE, CL_PushMoveFilter, &trace );
				m_pGround = CL_GetEntityByIndex( pfnIndexFromTrace( &trace ));
			}

			if( m_pGround && m_pGround->curstate.movetype == MOVETYPE_PUSH )
			{
				qboolean	applyVel, applyAvel;

				applyVel = !VectorCompare( m_pGround->curstate.origin, m_pGround->prevstate.origin );
				applyAvel = !VectorCompare( m_pGround->curstate.angles, m_pGround->prevstate.angles );

				if( applyVel || applyAvel )
				{
					ent->origin[0] += ( m_pGround->curstate.origin[0] - m_pGround->prevstate.origin[0] ) * -1.0f;
					ent->origin[1] += ( m_pGround->curstate.origin[1] - m_pGround->prevstate.origin[1] ) * -1.0f;
//					ent->origin[2] += ( m_pGround->curstate.origin[2] - m_pGround->prevstate.origin[2] ) * -1.0f;
					ent->latched.prevorigin[2] = ent->origin[2];
				}

				if( applyAvel )
				{
					for( i = 0; i < 3; i++ )
					{
						float	ang1, ang2;

						ang1 = m_pGround->curstate.angles[i];
						ang2 = m_pGround->prevstate.angles[i];
						d = ang1 - ang2;
						if( d > 180.0f ) d -= 360.0f;
						else if( d < -180.0f ) d += 360.0f;
						ent->angles[i] += d * -1.0f;
					}
				}
			}

			// move code from StudioSetupTransform here
			if( host.features & ENGINE_COMPUTE_STUDIO_LERP )
			{
				ent->origin[0] += ( ent->curstate.origin[0] - ent->latched.prevorigin[0] ) * f;
				ent->origin[1] += ( ent->curstate.origin[1] - ent->latched.prevorigin[1] ) * f;
				ent->origin[2] += ( ent->curstate.origin[2] - ent->latched.prevorigin[2] ) * f;

				for( i = 0; i < 3; i++ )
				{
					float	ang1, ang2;

					ang1 = ent->angles[i];
					ang2 = ent->latched.prevangles[i];
					d = ang1 - ang2;
					if( d > 180.0f ) d -= 360.0f;
					else if( d < -180.0f ) d += 360.0f;
					ent->angles[i] += d * f;
				}
			}
		}
	}
}

qboolean CL_AddVisibleEntity( cl_entity_t *ent, int entityType )
{
	if( !ent || !ent->model )
		return false;

	if( entityType == ET_TEMPENTITY )
	{
		// copy actual origin and angles back to let StudioModelRenderer
		// get actual value directly from curstate
		VectorCopy( ent->origin, ent->curstate.origin );
		VectorCopy( ent->angles, ent->curstate.angles );
	}

	if( CL_IsInMenu( ) && ( !cl.background || ent->player ))
	{
		// menu entities ignores client filter
		if( !R_AddEntity( ent, entityType ))
			return false;
	}
	else
	{
		// check for adding this entity
		if( !clgame.dllFuncs.pfnAddEntity( entityType, ent, ent->model->name ))
			return false;

		// don't add himself on firstperson
		if( RP_LOCALCLIENT( ent ) && !cl.thirdperson && cls.key_dest != key_menu && cl.refdef.viewentity == ( cl.playernum + 1 ))
		{
			if( gl_allow_mirrors->integer && world.has_mirrors )
			{
				if( !R_AddEntity( ent, entityType ))
					return false;
			}
			// otherwise just pass to player effects like flashlight, particles etc
		}
		else if( entityType == ET_BEAM )
		{
			CL_AddCustomBeam( ent );
			return true;
		}
		else if( !R_AddEntity( ent, entityType ))
		{
			return false;
		}
	}

	// set actual entity type
	ent->curstate.entityType = entityType;

	// apply effects
	if( ent->curstate.effects & EF_BRIGHTFIELD )
		CL_EntityParticles( ent );

	// add in muzzleflash effect
	if( ent->curstate.effects & EF_MUZZLEFLASH )
	{
		dlight_t	*dl;

		if( ent == &clgame.viewent )
			ent->curstate.effects &= ~EF_MUZZLEFLASH;

		dl = CL_AllocElight( 0 );

		VectorCopy( ent->attachment[0], dl->origin );
		dl->die = cl.time + 0.05f;
		dl->color.r = 255;
		dl->color.g = 180;
		dl->color.b = 64;
		dl->radius = 100;
	}

	// add light effect
	if( ent->curstate.effects & EF_LIGHT )
	{
		dlight_t	*dl = CL_AllocDlight( ent->curstate.number );
		VectorCopy( ent->origin, dl->origin );
		dl->die = cl.time;	// die at next frame
		dl->color.r = 100;
		dl->color.g = 100;
		dl->color.b = 100;
		dl->radius = 200;
		CL_RocketFlare( ent->origin );
	}

	// add dimlight
	if( ent->curstate.effects & EF_DIMLIGHT )
	{
		if( entityType == ET_PLAYER )
		{
			CL_UpdateFlashlight( ent );
		}
		else
		{
			dlight_t	*dl = CL_AllocDlight( ent->curstate.number );
			VectorCopy( ent->origin, dl->origin );
			dl->die = cl.time;	// die at next frame
			dl->color.r = 255;
			dl->color.g = 255;
			dl->color.b = 255;
			dl->radius = Com_RandomLong( 200, 230 );
		}
	}	

	if( ent->curstate.effects & EF_BRIGHTLIGHT )
	{			
		dlight_t	*dl = CL_AllocDlight( 0 );
		VectorSet( dl->origin, ent->origin[0], ent->origin[1], ent->origin[2] + 16.0f );
		dl->die = cl.time + 0.001f; // die at next frame
		dl->color.r = 255;
		dl->color.g = 255;
		dl->color.b = 255;

		if( entityType == ET_PLAYER )
			dl->radius = 430;
		else dl->radius = Com_RandomLong( 400, 430 );
	}

	if( ent->model->type == mod_studio )
	{
		if( ent->model->flags & STUDIO_ROTATE )
			ent->angles[1] = anglemod( 100.0f * cl.time );

		if( ent->model->flags & STUDIO_GIB )
			CL_RocketTrail( ent->prevstate.origin, ent->curstate.origin, 2 );
		else if( ent->model->flags & STUDIO_ZOMGIB )
			CL_RocketTrail( ent->prevstate.origin, ent->curstate.origin, 4 );
		else if( ent->model->flags & STUDIO_TRACER )
			CL_RocketTrail( ent->prevstate.origin, ent->curstate.origin, 3 );
		else if( ent->model->flags & STUDIO_TRACER2 )
			CL_RocketTrail( ent->prevstate.origin, ent->curstate.origin, 5 );
		else if( ent->model->flags & STUDIO_ROCKET )
		{
			dlight_t	*dl = CL_AllocDlight( ent->curstate.number );
			VectorCopy( ent->origin, dl->origin );
			dl->color.r = 255;
			dl->color.g = 255;
			dl->color.b = 255;

			// HACKHACK: get radius from head entity
			if( ent->curstate.rendermode != kRenderNormal )
				dl->radius = max( 0, ent->curstate.renderamt - 55 );
			else dl->radius = 200;
			dl->die = cl.time + 0.01f;

			CL_RocketTrail( ent->prevstate.origin, ent->curstate.origin, 0 );
		}
		else if( ent->model->flags & STUDIO_GRENADE )
			CL_RocketTrail( ent->prevstate.origin, ent->curstate.origin, 1 );
		else if( ent->model->flags & STUDIO_TRACER3 )
			CL_RocketTrail( ent->prevstate.origin, ent->curstate.origin, 6 );
	}

	return true;
}

/*
==================
CL_WeaponAnim

Set new weapon animation
==================
*/
void CL_WeaponAnim( int iAnim, int body )
{
	cl_entity_t	*view = &clgame.viewent;

	view->curstate.modelindex = cl.frame.local.client.viewmodel;

	// anim is changed. update latchedvars
	if( iAnim != view->curstate.sequence )
	{
		int	i;
			
		// save current blends to right lerping from last sequence
		for( i = 0; i < 2; i++ )
			view->latched.prevseqblending[i] = view->curstate.blending[i];
		view->latched.prevsequence = view->curstate.sequence; // save old sequence

		// save animtime
		view->latched.prevanimtime = view->curstate.animtime;
		view->latched.sequencetime = 0.0f;
	}

	view->curstate.animtime = cl.time;	// start immediately
	view->curstate.framerate = 1.0f;
	view->curstate.sequence = iAnim;
	view->latched.prevframe = 0.0f;
	view->curstate.scale = 1.0f;
	view->curstate.frame = 0.0f;
	view->curstate.body = body;

#if 0	// g-cont. for GlowShell testing
	view->curstate.renderfx = kRenderFxGlowShell;
	view->curstate.rendercolor.r = 255;
	view->curstate.rendercolor.g = 128;
	view->curstate.rendercolor.b = 0;
	view->curstate.renderamt = 100;
#endif
}

/*
==================
CL_UpdateStudioVars

Update studio latched vars so interpolation work properly
==================
*/
void CL_UpdateStudioVars( cl_entity_t *ent, entity_state_t *newstate, qboolean noInterp )
{
	int	i;

	if( newstate->effects & EF_NOINTERP || noInterp )
	{
		ent->latched.sequencetime = 0.0f; // no lerping between sequences
		ent->latched.prevsequence = newstate->sequence; // keep an actual
		ent->latched.prevanimtime = newstate->animtime;

		VectorCopy( newstate->origin, ent->latched.prevorigin );
		VectorCopy( newstate->angles, ent->latched.prevangles );

		// copy controllers
		for( i = 0; i < 4; i++ )
			ent->latched.prevcontroller[i] = newstate->controller[i];

		// copy blends
		for( i = 0; i < 2; i++ )
			ent->latched.prevblending[i] = newstate->blending[i];
		return;
	}

	// sequence has changed, hold the previous sequence info
	if( newstate->sequence != ent->curstate.sequence )
	{
		if( ent->index > 0 && ent->index <= cl.maxclients )
			ent->latched.sequencetime = ent->curstate.animtime + 0.01f;
		else ent->latched.sequencetime = ent->curstate.animtime + 0.1f;
			
		// save current blends to right lerping from last sequence
		for( i = 0; i < 2; i++ )
			ent->latched.prevseqblending[i] = ent->curstate.blending[i];
		ent->latched.prevsequence = ent->curstate.sequence; // save old sequence	
	}

	if( newstate->animtime != ent->curstate.animtime )
	{
		// client got new packet, shuffle animtimes
		ent->latched.prevanimtime = ent->curstate.animtime;
		VectorCopy( newstate->origin, ent->latched.prevorigin );
		VectorCopy( newstate->angles, ent->latched.prevangles );

		for( i = 0; i < 4; i++ )
			ent->latched.prevcontroller[i] = newstate->controller[i];
	}

	// copy controllers
	for( i = 0; i < 4; i++ )
	{
		if( ent->curstate.controller[i] != newstate->controller[i] )
			ent->latched.prevcontroller[i] = ent->curstate.controller[i];
	}

	// copy blends
	for( i = 0; i < 2; i++ )
		ent->latched.prevblending[i] = ent->curstate.blending[i];

	if( !VectorCompare( newstate->origin, ent->curstate.origin ))
		VectorCopy( ent->curstate.origin, ent->latched.prevorigin );

	if( !VectorCompare( newstate->angles, ent->curstate.angles ))
		VectorCopy( ent->curstate.angles, ent->latched.prevangles );
}

/*
==================
CL_UpdateBmodelVars

Using studio latched vars for interpolate bmodels
==================
*/
void CL_UpdateBmodelVars( cl_entity_t *ent, entity_state_t *newstate, qboolean noInterp )
{
	if( newstate->effects & EF_NOINTERP || noInterp )
	{
		ent->latched.prevanimtime = newstate->animtime;
		VectorCopy( newstate->origin, ent->latched.prevorigin );
		VectorCopy( newstate->angles, ent->latched.prevangles );
		return;
	}

	if( newstate->animtime != ent->curstate.animtime )
	{
		// client got new packet, shuffle animtimes
		ent->latched.prevanimtime = ent->curstate.animtime;
		VectorCopy( newstate->origin, ent->latched.prevorigin );
		VectorCopy( newstate->angles, ent->latched.prevangles );
	}

	// NOTE: store prevorigin for interpolate monsters on moving platforms
	if( !VectorCompare( newstate->origin, ent->curstate.origin ))
		VectorCopy( ent->curstate.origin, ent->latched.prevorigin );
	if( !VectorCompare( newstate->angles, ent->curstate.angles ))
		VectorCopy( ent->curstate.angles, ent->latched.prevangles );
}

void CL_DeltaEntity( sizebuf_t *msg, frame_t *frame, int newnum, entity_state_t *old, qboolean unchanged )
{
	cl_entity_t	*ent;
	entity_state_t	*state;
	qboolean		newent = (old) ? false : true;
	qboolean		result = true;

	ent = CL_EDICT_NUM( newnum );
	state = &cls.packet_entities[cls.next_client_entities % cls.num_client_entities];
	ent->index = newnum;

	if( newent ) old = &ent->baseline;

	if( unchanged ) *state = *old;
	else result = MSG_ReadDeltaEntity( msg, old, state, newnum, CL_IsPlayerIndex( newnum ), cl.mtime[0] );

	if( !result )
	{
		if( newent ) Host_Error( "Cl_DeltaEntity: tried to release new entity\n" );

		CL_KillDeadBeams( ent ); // release dead beams
#if 0
		// this is for reference
		if( state->number == -1 )
			Msg( "Entity %i was removed from server\n", newnum );
		else Msg( "Entity %i was removed from delta-message\n", newnum );
#endif
		if( state->number == -1 )
		{
			ent->curstate.messagenum = 0;
			ent->baseline.number = 0;
		}

		// entity was delta removed
		return;
	}

	// entity is present in newframe
	state->messagenum = cl.parsecount;
	state->msg_time = cl.mtime[0];
	
	cls.next_client_entities++;
	frame->num_entities++;

	// set player state
	ent->player = CL_IsPlayerIndex( ent->index );

	if( state->effects & EF_NOINTERP || newent )
	{	
		// duplicate the current state so lerping doesn't hurt anything
		ent->prevstate = *state;
	}
	else
	{	
		// shuffle the last state to previous
		ent->prevstate = ent->curstate;
	}

	// NOTE: always check modelindex for new state not current
	if( Mod_GetType( state->modelindex ) == mod_studio )
	{
		CL_UpdateStudioVars( ent, state, newent );
	}
	else if( Mod_GetType( state->modelindex ) == mod_brush )
	{
		CL_UpdateBmodelVars( ent, state, newent );
	}

	// set right current state
	ent->curstate = *state;
}

/*
=================
CL_FlushEntityPacket
=================
*/
void CL_FlushEntityPacket( sizebuf_t *msg )
{
	int		newnum;
	entity_state_t	from, to;

	MsgDev( D_INFO, "FlushEntityPacket()\n" );
	Q_memset( &from, 0, sizeof( from ));

	cl.frames[cl.parsecountmod].valid = false;
	cl.validsequence = 0; // can't render a frame

	// read it all, but ignore it
	while( 1 )
	{
		newnum = BF_ReadWord( msg );
		if( !newnum ) break; // done

		if( BF_CheckOverflow( msg ))
			Host_Error( "CL_FlushEntityPacket: read overflow\n" );

		MSG_ReadDeltaEntity( msg, &from, &to, newnum, CL_IsPlayerIndex( newnum ), cl.mtime[0] );
	}
}

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
void CL_ParsePacketEntities( sizebuf_t *msg, qboolean delta )
{
	frame_t		*newframe, *oldframe;
	int		oldindex, newnum, oldnum;
	int		oldpacket, newpacket;
	cl_entity_t	*player;
	entity_state_t	*oldent;
	int		i, count;

	// save first uncompressed packet as timestamp
	if( cls.changelevel && !delta && cls.demorecording )
		CL_WriteDemoJumpTime();

	// first, allocate packet for new frame
	count = BF_ReadWord( msg );

	newpacket = cl.parsecountmod;
	newframe = &cl.frames[newpacket];

	// allocate parse entities
	newframe->first_entity = cls.next_client_entities;
	newframe->num_entities = 0;
	newframe->valid = true; // assume valid

	if( delta )
	{
		int	subtracted;

		oldpacket = BF_ReadByte( msg );
		subtracted = ((( cls.netchan.incoming_sequence & 0xFF ) - oldpacket ) & 0xFF );

		if( subtracted == 0 )
		{
			Host_Error( "CL_DeltaPacketEntities: update too old, connection dropped.\n" );
			return;
		}

		if( subtracted >= CL_UPDATE_MASK )
		{	
			// we can't use this, it is too old
			Con_NPrintf( 2, "^3Warning:^1 delta frame is too old^7\n" );
			CL_FlushEntityPacket( msg );
			return;
		}

		oldframe = &cl.frames[oldpacket & CL_UPDATE_MASK];

		if(( cls.next_client_entities - oldframe->first_entity ) > ( cls.num_client_entities - 128 ))
		{
			Con_NPrintf( 2, "^3Warning:^1 delta frame is too old^7\n" );
			CL_FlushEntityPacket( msg );
			return;
		}
	}
	else
	{
		// this is a full update that we can start delta compressing from now
		oldframe = NULL;
		oldpacket = -1;		// delta too old or is initial message
		cl.force_send_usercmd = true;	// send reply
		cls.demowaiting = false;	// we can start recording now
	}

	// mark current delta state
	cl.validsequence = cls.netchan.incoming_sequence;

	oldent = NULL;
	oldindex = 0;

	if( !oldframe )
	{
		oldnum = MAX_ENTNUMBER;
	}
	else
	{
		if( oldindex >= oldframe->num_entities )
		{
			oldnum = MAX_ENTNUMBER;
		}
		else
		{
			oldent = &cls.packet_entities[(oldframe->first_entity+oldindex) % cls.num_client_entities];
			oldnum = oldent->number;
		}
	}

	while( 1 )
	{
		newnum = BF_ReadWord( msg );
		if( !newnum ) break; // end of packet entities

		if( BF_CheckOverflow( msg ))
			Host_Error( "CL_ParsePacketEntities: read overflow\n" );

		while( oldnum < newnum )
		{	
			// one or more entities from the old packet are unchanged
			CL_DeltaEntity( msg, newframe, oldnum, oldent, true );
			
			oldindex++;

			if( oldindex >= oldframe->num_entities )
			{
				oldnum = MAX_ENTNUMBER;
			}
			else
			{
				oldent = &cls.packet_entities[(oldframe->first_entity+oldindex) % cls.num_client_entities];
				oldnum = oldent->number;
			}
		}

		if( oldnum == newnum )
		{	
			// delta from previous state
			CL_DeltaEntity( msg, newframe, newnum, oldent, false );
			oldindex++;

			if( oldindex >= oldframe->num_entities )
			{
				oldnum = MAX_ENTNUMBER;
			}
			else
			{
				oldent = &cls.packet_entities[(oldframe->first_entity+oldindex) % cls.num_client_entities];
				oldnum = oldent->number;
			}
			continue;
		}

		if( oldnum > newnum )
		{	
			// delta from baseline ?
			CL_DeltaEntity( msg, newframe, newnum, NULL, false );
			continue;
		}
	}

	// any remaining entities in the old frame are copied over
	while( oldnum != MAX_ENTNUMBER )
	{	
		// one or more entities from the old packet are unchanged
		CL_DeltaEntity( msg, newframe, oldnum, oldent, true );
		oldindex++;

		if( oldindex >= oldframe->num_entities )
		{
			oldnum = MAX_ENTNUMBER;
		}
		else
		{
			oldent = &cls.packet_entities[(oldframe->first_entity+oldindex) % cls.num_client_entities];
			oldnum = oldent->number;
		}
	}

	cl.frame = *newframe;

	if( !cl.frame.valid ) return;

	player = CL_GetLocalPlayer();
		
	if( cls.state != ca_active )
	{
		// client entered the game
		cls.state = ca_active;
		cl.force_refdef = true;
		cls.changelevel = false;		// changelevel is done
		cls.changedemo = false;		// changedemo is done

		SCR_MakeLevelShot();		// make levelshot if needs
		Cvar_SetFloat( "scr_loading", 0.0f );	// reset progress bar	

		if(( cls.demoplayback || cls.disable_servercount != cl.servercount ) && cl.video_prepped )
			SCR_EndLoadingPlaque(); // get rid of loading plaque
	}

	// update local player states
	if( player != NULL )
	{
		entity_state_t	*ps, *pps;
		clientdata_t	*pcd, *ppcd;
		weapon_data_t	*wd, *pwd;

		pps = &player->curstate;
		ppcd = &newframe->local.client;
		pwd = newframe->local.weapondata;

		ps = &cl.predict[cl.predictcount & CL_UPDATE_MASK].playerstate;
		pcd = &cl.predict[cl.predictcount & CL_UPDATE_MASK].client;
		wd = cl.predict[cl.predictcount & CL_UPDATE_MASK].weapondata;
		
		clgame.dllFuncs.pfnTxferPredictionData( ps, pps, pcd, ppcd, wd, pwd ); 
		clgame.dllFuncs.pfnTxferLocalOverrides( &player->curstate, pcd );
	}

	// update state for all players
	for( i = 0; i < cl.maxclients; i++ )
	{
		cl_entity_t *ent = CL_GetEntityByIndex( i + 1 );
		if( !ent ) continue;
		clgame.dllFuncs.pfnProcessPlayerState( &newframe->playerstate[i], &ent->curstate );
		newframe->playerstate[i].number = ent->index;
	}

	cl.frame = *newframe;
	cl.predict[cl.predictcount & CL_UPDATE_MASK] = cl.frame.local;
}

/*
==========================================================================

INTERPOLATE BETWEEN FRAMES TO GET RENDERING PARMS

==========================================================================
*/
/*
===============
CL_SetIdealPitch
===============
*/
void CL_SetIdealPitch( void )
{
	float	angleval, sinval, cosval;
	vec3_t	top, bottom;
	float	z[MAX_FORWARD];
	int	i, j;
	int	step, dir, steps;
	pmtrace_t	tr;

	if( !( cl.frame.local.client.flags & FL_ONGROUND ))
		return;
		
	angleval = cl.frame.local.playerstate.angles[YAW] * M_PI2 / 360.0f;
	SinCos( angleval, &sinval, &cosval );

	for( i = 0; i < MAX_FORWARD; i++ )
	{
		top[0] = cl.frame.local.client.origin[0] + cosval * (i + 3.0f) * 12.0f;
		top[1] = cl.frame.local.client.origin[1] + sinval * (i + 3.0f) * 12.0f;
		top[2] = cl.frame.local.client.origin[2] + cl.frame.local.client.view_ofs[2];
		
		bottom[0] = top[0];
		bottom[1] = top[1];
		bottom[2] = top[2] - 160.0f;

		// skip any monsters (only world and brush models)
		tr = CL_TraceLine( top, bottom, PM_STUDIO_IGNORE );
		if( tr.allsolid ) return; // looking at a wall, leave ideal the way is was

		if( tr.fraction == 1.0f )
			return;	// near a dropoff
		
		z[i] = top[2] + tr.fraction * (bottom[2] - top[2]);
	}
	
	dir = 0;
	steps = 0;

	for( j = 1; j < i; j++ )
	{
		step = z[j] - z[j-1];
		if( step > -ON_EPSILON && step < ON_EPSILON )
			continue;

		if( dir && ( step-dir > ON_EPSILON || step-dir < -ON_EPSILON ))
			return; // mixed changes

		steps++;	
		dir = step;
	}
	
	if( !dir )
	{
		cl.refdef.idealpitch = 0.0f;
		return;
	}
	
	if( steps < 2 ) return;
	cl.refdef.idealpitch = -dir * cl_idealpitchscale->value;
}

/*
===============
CL_AddPacketEntities

===============
*/
void CL_AddPacketEntities( frame_t *frame )
{
	cl_entity_t	*ent, *clent;
	int		i, e, entityType;

	clent = CL_GetLocalPlayer();
	if( !clent ) return;

	for( i = 0; i < cl.frame.num_entities; i++ )
	{
		e = cls.packet_entities[(cl.frame.first_entity + i) % cls.num_client_entities].number;
		ent = CL_GetEntityByIndex( e );

		if( !ent || ent == clgame.entities )
			continue;

		CL_UpdateEntityFields( ent );

		if( ent->player ) entityType = ET_PLAYER;
		else if( ent->curstate.entityType == ENTITY_BEAM )
			entityType = ET_BEAM;
		else entityType = ET_NORMAL;

		CL_AddVisibleEntity( ent, entityType );
	}
}

/*
===============
CL_AddEntities

Emits all entities, particles, and lights to the refresh
===============
*/
void CL_AddEntities( void )
{
	if( cls.state != ca_active )
		return;

	cl.num_custombeams = 0;

	CL_SetIdealPitch ();
	clgame.dllFuncs.CAM_Think ();

	CL_AddPacketEntities( &cl.frame );
	clgame.dllFuncs.pfnCreateEntities();

	CL_FireEvents();	// so tempents can be created immediately
	CL_AddTempEnts();

	// perfomance test
	CL_TestLights();
}

//
// sound engine implementation
//
qboolean CL_GetEntitySpatialization( int entnum, vec3_t origin, float *pradius )
{
	cl_entity_t	*ent;
	qboolean		valid_origin;

	ASSERT( origin != NULL );

	if( entnum == 0 ) return true; // static sound

	if(( entnum - 1 ) == cl.playernum )
	{
		VectorCopy( cl.frame.local.client.origin, origin );
		return true;
	}

	valid_origin = VectorIsNull( origin ) ? false : true;          
	ent = CL_GetEntityByIndex( entnum );

	// entity is not present on the client but has valid origin
	if( !ent || !ent->index ) return valid_origin;

	if( ent->curstate.messagenum == 0 )
	{
		// entity is never has updates on the client
		// so we should use static origin instead
		return valid_origin;
	}
#if 0
	// uncomment this if you want enable additional check by PVS
	if( ent->curstate.messagenum != cl.parsecount )
		return valid_origin;
#endif
	// setup origin
	VectorAverage( ent->curstate.mins, ent->curstate.maxs, origin );
	VectorAdd( origin, ent->curstate.origin, origin );

	// setup radius
	if( pradius )
	{
		if( ent->model != NULL && ent->model->radius ) *pradius = ent->model->radius;
		else *pradius = RadiusFromBounds( ent->curstate.mins, ent->curstate.maxs );
	}

	return true;
}

void CL_ExtraUpdate( void )
{
	clgame.dllFuncs.IN_Accumulate();
	S_ExtraUpdate();
}