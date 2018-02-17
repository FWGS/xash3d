/*
cl_tent.c - temp entity effects management
Copyright (C) 2009 Uncle Mike

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
#include "r_efx.h"
#include "entity_types.h"
#include "triangleapi.h"
#include "cl_tent.h"
#include "pm_local.h"
#include "gl_local.h"
#include "studio.h"
#include "wadfile.h"	// acess decal size
#include "sound.h"

/*
==============================================================

TEMPENTS MANAGEMENT

==============================================================
*/
#define MAX_MUZZLEFLASH		4
#define SHARD_VOLUME		12.0f	// on shard ever n^3 units
#define SF_FUNNEL_REVERSE		1

TEMPENTITY	*cl_active_tents;
TEMPENTITY	*cl_free_tents;
TEMPENTITY	*cl_tempents = NULL;		// entities pool
int		cl_muzzleflash[MAX_MUZZLEFLASH];	// muzzle flashes

/*
================
CL_InitTempents

================
*/
void CL_InitTempEnts( void )
{
	cl_tempents = Mem_Alloc( cls.mempool, sizeof( TEMPENTITY ) * GI->max_tents );
	CL_ClearTempEnts();
}

/*
================
CL_RegisterMuzzleFlashes

================
*/
void CL_RegisterMuzzleFlashes( void )
{
	// update muzzleflash indexes
	cl_muzzleflash[0] = CL_FindModelIndex( "sprites/muzzleflash1.spr" );
	cl_muzzleflash[1] = CL_FindModelIndex( "sprites/muzzleflash2.spr" );
	cl_muzzleflash[2] = CL_FindModelIndex( "sprites/muzzleflash3.spr" );
	cl_muzzleflash[3] = CL_FindModelIndex( "sprites/muzzleflash.spr" );

	// update registration for shellchrome
	cls.hChromeSprite = pfnSPR_Load( "sprites/shellchrome.spr" );
}

/*
================
CL_ClearTempEnts

================
*/
void CL_ClearTempEnts( void )
{
	int	i;

	if( !cl_tempents ) return;

	for( i = 0; i < GI->max_tents - 1; i++ )
	{
		cl_tempents[i].next = &cl_tempents[i+1];
		cl_tempents[i].entity.trivial_accept = INVALID_HANDLE;
	}

	cl_tempents[GI->max_tents-1].next = NULL;
	cl_free_tents = cl_tempents;
	cl_active_tents = NULL;
}

/*
================
CL_FreeTempEnts

================
*/
void CL_FreeTempEnts( void )
{
	if( cl_tempents )
		Mem_Free( cl_tempents );
	cl_tempents = NULL;
}

/*
==============
CL_PrepareTEnt

set default values
==============
*/
void CL_PrepareTEnt( TEMPENTITY *pTemp, model_t *pmodel )
{
	int	frameCount = 0;
	int	modelIndex = 0;
	int	modelHandle = pTemp->entity.trivial_accept;

	Q_memset( pTemp, 0, sizeof( *pTemp ));

	// use these to set per-frame and termination conditions / actions
	pTemp->entity.trivial_accept = modelHandle; // keep unchanged
	pTemp->flags = FTENT_NONE;		
	pTemp->die = cl.time + 0.75f;

	if( pmodel )
	{
		modelIndex = CL_FindModelIndex( pmodel->name );
		Mod_GetFrames( modelIndex, &frameCount );
	}
	else
	{
		pTemp->flags |= FTENT_NOMODEL;
	}

	pTemp->entity.curstate.modelindex = modelIndex;
	pTemp->entity.curstate.rendermode = kRenderNormal;
	pTemp->entity.curstate.renderfx = kRenderFxNone;
	pTemp->entity.curstate.rendercolor.r = 255;
	pTemp->entity.curstate.rendercolor.g = 255;
	pTemp->entity.curstate.rendercolor.b = 255;
	pTemp->frameMax = max( 0, frameCount - 1 );
	pTemp->entity.curstate.renderamt = 255;
	pTemp->entity.curstate.body = 0;
	pTemp->entity.curstate.skin = 0;
	pTemp->entity.model = pmodel;
	pTemp->fadeSpeed = 0.5f;
	pTemp->hitSound = 0;
	pTemp->clientIndex = 0;
	pTemp->bounceFactor = 1;
	pTemp->entity.curstate.scale = 1.0f;
}

/*
==============
CL_TEntPlaySound

play collide sound
==============
*/
void CL_TEntPlaySound( TEMPENTITY *pTemp, float damp )
{
	float	fvol;
	char	soundname[32];
	qboolean	isshellcasing = false;
	int	zvel;

	ASSERT( pTemp != NULL );

	switch( pTemp->hitSound )
	{
	case BOUNCE_GLASS:
		Q_snprintf( soundname, sizeof( soundname ), "debris/glass%i.wav", Com_RandomLong( 1, 4 ));
		break;
	case BOUNCE_METAL:
		Q_snprintf( soundname, sizeof( soundname ), "debris/metal%i.wav", Com_RandomLong( 1, 6 ));
		break;
	case BOUNCE_FLESH:
		Q_snprintf( soundname, sizeof( soundname ), "debris/flesh%i.wav", Com_RandomLong( 1, 7 ));
		break;
	case BOUNCE_WOOD:
		Q_snprintf( soundname, sizeof( soundname ), "debris/wood%i.wav", Com_RandomLong( 1, 4 ));
		break;
	case BOUNCE_SHRAP:
		Q_snprintf( soundname, sizeof( soundname ), "weapons/ric%i.wav", Com_RandomLong( 1, 5 ));
		break;
	case BOUNCE_SHOTSHELL:
		Q_snprintf( soundname, sizeof( soundname ), "weapons/sshell%i.wav", Com_RandomLong( 1, 3 ));
		isshellcasing = true; // shell casings have different playback parameters
		break;
	case BOUNCE_SHELL:
		Q_snprintf( soundname, sizeof( soundname ), "player/pl_shell%i.wav", Com_RandomLong( 1, 3 ));
		isshellcasing = true; // shell casings have different playback parameters
		break;
	case BOUNCE_CONCRETE:
		Q_snprintf( soundname, sizeof( soundname ), "debris/concrete%i.wav", Com_RandomLong( 1, 3 ));
		break;
	default:	// null sound
		return;
	}

	zvel = fabs( pTemp->entity.baseline.origin[2] );
		
	// only play one out of every n
	if( isshellcasing )
	{	
		// play first bounce, then 1 out of 3		
		if( zvel < 200 && Com_RandomLong( 0, 3 ))
			return;
	}
	else
	{
		if( Com_RandomLong( 0, 5 )) 
			return;
	}

	fvol = 1.0f;

	if( damp > 0.0f )
	{
		int	pitch;
		sound_t	handle;
		
		if( isshellcasing )
			fvol *= min ( 1.0f, ((float)zvel) / 350.0f ); 
		else fvol *= min ( 1.0f, ((float)zvel) / 450.0f ); 
		
		if( !Com_RandomLong( 0, 3 ) && !isshellcasing )
			pitch = Com_RandomLong( 95, 105 );
		else pitch = PITCH_NORM;

		handle = S_RegisterSound( soundname );
		S_StartSound( pTemp->entity.origin, -(pTemp - cl_tempents), CHAN_BODY, handle, fvol, ATTN_NORM, pitch, SND_STOP_LOOPING );
	}
}

/*
==============
CL_TEntAddEntity

add entity to renderlist
==============
*/
int CL_TEntAddEntity( cl_entity_t *pEntity )
{
	ASSERT( pEntity != NULL );

	r_stats.c_active_tents_count++;

	return CL_AddVisibleEntity( pEntity, ET_TEMPENTITY );
}

/*
==============
CL_TEntAddEntity

free the first low priority tempent it finds.
==============
*/
qboolean CL_FreeLowPriorityTempEnt( void )
{
	TEMPENTITY	*pActive = cl_active_tents;
	TEMPENTITY	*pPrev = NULL;

	while( pActive )
	{
		if( pActive->priority == TENTPRIORITY_LOW )
		{
			// remove from the active list.
			if( pPrev ) pPrev->next = pActive->next;
			else cl_active_tents = pActive->next;

			// add to the free list.
			pActive->next = cl_free_tents;
			cl_free_tents = pActive;

			return true;
		}

		pPrev = pActive;
		pActive = pActive->next;
	}
	return false;
}


/*
==============
CL_AddTempEnts

temp-entities will be added on a user-side
setup client callback
==============
*/
void CL_AddTempEnts( void )
{
	double	ft = cl.time - cl.oldtime;
	float	gravity = clgame.movevars.gravity;

	clgame.dllFuncs.pfnTempEntUpdate( ft, cl.time, gravity, &cl_free_tents, &cl_active_tents, CL_TEntAddEntity, CL_TEntPlaySound );	// callbacks
}

/*
==============
CL_TempEntAlloc

alloc normal\low priority tempentity
==============
*/
TEMPENTITY *GAME_EXPORT CL_TempEntAlloc( const vec3_t org, model_t *pmodel )
{
	TEMPENTITY	*pTemp;

	if( !cl_free_tents )
	{
		MsgDev( D_NOTE, "Overflow %d temporary ents!\n", GI->max_tents );
		return NULL;
	}

	pTemp = cl_free_tents;
	cl_free_tents = pTemp->next;

	CL_PrepareTEnt( pTemp, pmodel );

	pTemp->priority = TENTPRIORITY_LOW;
	if( org ) VectorCopy( org, pTemp->entity.origin );

	pTemp->next = cl_active_tents;
	cl_active_tents = pTemp;

	return pTemp;
}

/*
==============
CL_TempEntAllocHigh

alloc high priority tempentity
==============
*/
TEMPENTITY *GAME_EXPORT CL_TempEntAllocHigh( const vec3_t org, model_t *pmodel )
{
	TEMPENTITY	*pTemp;

	if( !cl_free_tents )
	{
		// no temporary ents free, so find the first active low-priority temp ent 
		// and overwrite it.
		CL_FreeLowPriorityTempEnt();
	}

	if( !cl_free_tents )
	{
		// didn't find anything? The tent list is either full of high-priority tents
		// or all tents in the list are still due to live for > 10 seconds. 
		MsgDev( D_INFO, "Couldn't alloc a high priority TENT!\n" );
		return NULL;
	}

	// Move out of the free list and into the active list.
	pTemp = cl_free_tents;
	cl_free_tents = pTemp->next;

	CL_PrepareTEnt( pTemp, pmodel );

	pTemp->priority = TENTPRIORITY_HIGH;
	if( org ) VectorCopy( org, pTemp->entity.origin );

	pTemp->next = cl_active_tents;
	cl_active_tents = pTemp;

	return pTemp;
}

/*
==============
CL_TempEntAlloc

alloc normal priority tempentity with no model
==============
*/
TEMPENTITY *GAME_EXPORT CL_TempEntAllocNoModel( const vec3_t org )
{
	return CL_TempEntAlloc( org, NULL );
}

/*
==============
CL_TempEntAlloc

custom tempentity allocation
==============
*/
TEMPENTITY *GAME_EXPORT CL_TempEntAllocCustom( const vec3_t org, model_t *model, int high, void (*pfn)( TEMPENTITY*, float, float ))
{
	TEMPENTITY	*pTemp;

	if( high )
	{
		pTemp = CL_TempEntAllocHigh( org, model );
	}
	else
	{
		pTemp = CL_TempEntAlloc( org, model );
	}

	if( pTemp && pfn )
	{
		pTemp->flags |= FTENT_CLIENTCUSTOM;
		pTemp->callback = pfn;
	}

	return pTemp;
}

/*
==============================================================

	EFFECTS BASED ON TEMPENTS (presets)

==============================================================
*/
/*
==============
CL_FizzEffect

Create a fizz effect
==============
*/
void GAME_EXPORT CL_FizzEffect( cl_entity_t *pent, int modelIndex, int density )
{
	TEMPENTITY	*pTemp;
	int		i, width, depth, count, frameCount;
	float		angle, maxHeight, speed;
	float		xspeed, yspeed, zspeed;
	vec3_t		origin, mins, maxs;

	if( !pent || Mod_GetType( modelIndex ) == mod_bad )
		return;

	if( pent->curstate.modelindex <= 0 )
		return;

	count = density + 1;
	density = count * 3 + 6;

	Mod_GetBounds( pent->curstate.modelindex, mins, maxs );

	maxHeight = maxs[2] - mins[2];
	width = maxs[0] - mins[0];
	depth = maxs[1] - mins[1];
	speed = ( pent->curstate.rendercolor.r<<8 | pent->curstate.rendercolor.g );
	if( pent->curstate.rendercolor.b ) speed = -speed;
	if( speed == 0.0f ) speed = 100.0f;	// apply default value

	if( pent->angles[YAW] != 0.0f )
	{
		angle = pent->angles[YAW] * M_PI / 180;
		yspeed = sin( angle );
		xspeed = cos( angle );

		xspeed *= speed;
		yspeed *= speed;
	}
	else xspeed = yspeed = 0.0f;	// z only

	Mod_GetFrames( modelIndex, &frameCount );

	for( i = 0; i < count; i++ )
	{
		origin[0] = mins[0] + Com_RandomLong( 0, width - 1 );
		origin[1] = mins[1] + Com_RandomLong( 0, depth - 1 );
		origin[2] = mins[2];
		pTemp = CL_TempEntAlloc( origin, Mod_Handle( modelIndex ));

		if ( !pTemp ) return;

		pTemp->flags |= FTENT_SINEWAVE;

		pTemp->x = origin[0];
		pTemp->y = origin[1];

		zspeed = Com_RandomLong( 80, 140 );
		VectorSet( pTemp->entity.baseline.origin, xspeed, yspeed, zspeed );
		pTemp->die = cl.time + ( maxHeight / zspeed ) - 0.1f;
		pTemp->entity.curstate.frame = Com_RandomLong( 0, frameCount - 1 );
		// Set sprite scale
		pTemp->entity.curstate.scale = 1.0f / Com_RandomFloat( 2.0f, 5.0f );
		pTemp->entity.curstate.rendermode = kRenderTransAlpha;
		pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = 255;
	}
}

/*
==============
CL_Bubbles

Create bubbles
==============
*/
void GAME_EXPORT CL_Bubbles( const vec3_t mins, const vec3_t maxs, float height, int modelIndex, int count, float speed )
{
	TEMPENTITY	*pTemp;
	int		i, frameCount;
	float		sine, cosine, zspeed;
	float		angle;
	vec3_t		origin;

	if( Mod_GetType( modelIndex ) == mod_bad )
		return;

	Mod_GetFrames( modelIndex, &frameCount );

	for ( i = 0; i < count; i++ )
	{
		origin[0] = Com_RandomLong( mins[0], maxs[0] );
		origin[1] = Com_RandomLong( mins[1], maxs[1] );
		origin[2] = Com_RandomLong( mins[2], maxs[2] );
		pTemp = CL_TempEntAlloc( origin, Mod_Handle( modelIndex ));
		if( !pTemp ) return;

		pTemp->flags |= FTENT_SINEWAVE;

		pTemp->x = origin[0];
		pTemp->y = origin[1];
		angle = Com_RandomFloat( -M_PI, M_PI );
		SinCos( angle, &sine, &cosine );
		
		zspeed = Com_RandomLong( 80, 140 );
		VectorSet( pTemp->entity.baseline.origin, speed * cosine, speed * sine, zspeed );
		pTemp->die = cl.time + ((height - (origin[2] - mins[2])) / zspeed) - 0.1f;
		pTemp->entity.curstate.frame = Com_RandomLong( 0, frameCount - 1 );
		
		// Set sprite scale
		pTemp->entity.curstate.scale = 1.0f / Com_RandomFloat( 4.0f, 16.0f );
		pTemp->entity.curstate.rendermode = kRenderTransAlpha;
		pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = 192; // g-cont. why difference with FizzEffect ???		
	}
}

/*
==============
CL_BubbleTrail

Create bubble trail
==============
*/
void GAME_EXPORT CL_BubbleTrail( const vec3_t start, const vec3_t end, float flWaterZ, int modelIndex, int count, float speed )
{
	TEMPENTITY	*pTemp;
	int		i, frameCount;
	float		dist, angle, zspeed;
	vec3_t		origin;

	if( Mod_GetType( modelIndex ) == mod_bad )
		return;

	Mod_GetFrames( modelIndex, &frameCount );

	for( i = 0; i < count; i++ )
	{
		dist = Com_RandomFloat( 0, 1.0 );
		VectorLerp( start, dist, end, origin );
		pTemp = CL_TempEntAlloc( origin, Mod_Handle( modelIndex ));
		if( !pTemp ) return;

		pTemp->flags |= FTENT_SINEWAVE;

		pTemp->x = origin[0];
		pTemp->y = origin[1];
		angle = Com_RandomFloat( -M_PI, M_PI );

		zspeed = Com_RandomLong( 80, 140 );
		VectorSet( pTemp->entity.baseline.origin, speed * cos( angle ), speed * sin( angle ), zspeed );
		pTemp->die = cl.time + ((flWaterZ - (origin[2] - start[2])) / zspeed) - 0.1f;
		pTemp->entity.curstate.frame = Com_RandomLong( 0, frameCount - 1 );
		// Set sprite scale
		pTemp->entity.curstate.scale = 1.0 / Com_RandomFloat( 4, 8 );
		pTemp->entity.curstate.rendermode = kRenderTransAlpha;
		pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = 192;
	}
}

/*
==============
CL_AttachTentToPlayer

Attaches entity to player
==============
*/
void GAME_EXPORT CL_AttachTentToPlayer( int client, int modelIndex, float zoffset, float life )
{
	TEMPENTITY	*pTemp;
	vec3_t		position;
	cl_entity_t	*pClient;

	if( client <= 0 || client > cl.maxclients )
	{
		MsgDev( D_ERROR, "Bad client %i in AttachTentToPlayer()!\n", client );
		return;
	}

	pClient = CL_GetEntityByIndex( client );
	if( !pClient )
	{
		MsgDev( D_INFO, "Couldn't get ClientEntity for %i\n", client );
		return;
	}

	if( Mod_GetType( modelIndex ) == mod_bad )
	{
		MsgDev( D_INFO, "No model %d!\n", modelIndex );
		return;
	}

	VectorCopy( pClient->origin, position );
	position[2] += zoffset;

	pTemp = CL_TempEntAllocHigh( position, Mod_Handle( modelIndex ));

	if( !pTemp )
	{
		MsgDev( D_INFO, "No temp ent.\n" );
		return;
	}

	pTemp->entity.curstate.rendermode = kRenderNormal;
	pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = 192;
	pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
	
	pTemp->clientIndex = client;
	pTemp->tentOffset[0] = 0;
	pTemp->tentOffset[1] = 0;
	pTemp->tentOffset[2] = zoffset;
	pTemp->die = cl.time + life;
	pTemp->flags |= FTENT_PLYRATTACHMENT|FTENT_PERSIST;

	// is the model a sprite?
	if( Mod_GetType( pTemp->entity.curstate.modelindex ) == mod_sprite )
	{
		pTemp->flags |= FTENT_SPRANIMATE|FTENT_SPRANIMATELOOP;
		pTemp->entity.curstate.framerate = 10;
	}
	else
	{
		// no animation support for attached clientside studio models.
		pTemp->frameMax = 0;
	}

	pTemp->entity.curstate.frame = 0;
}

/*
==============
CL_KillAttachedTents

Detach entity from player
==============
*/
void GAME_EXPORT CL_KillAttachedTents( int client )
{
	int	i;

	if( client <= 0 || client > cl.maxclients )
	{
		MsgDev( D_ERROR, "Bad client %i in KillAttachedTents()!\n", client );
		return;
	}

	for( i = 0; i < GI->max_tents; i++ )
	{
		TEMPENTITY *pTemp = &cl_tempents[i];

		if( pTemp->flags & FTENT_PLYRATTACHMENT )
		{
			// this TEMPENTITY is player attached.
			// if it is attached to this client, set it to die instantly.
			if( pTemp->clientIndex == client )
			{
				pTemp->die = cl.time; // good enough, it will die on next tent update. 
			}
		}
	}
}

/*
==============
CL_RicochetSprite

Create ricochet sprite
==============
*/
void GAME_EXPORT CL_RicochetSprite( const vec3_t pos, model_t *pmodel, float duration, float scale )
{
	TEMPENTITY	*pTemp;

	pTemp = CL_TempEntAlloc( pos, pmodel );
	if( !pTemp ) return;

	pTemp->entity.curstate.rendermode = kRenderGlow;
	pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = 200;
	pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
	pTemp->entity.curstate.scale = scale;
	pTemp->die = cl.time + duration;
	pTemp->flags = FTENT_FADEOUT;
	pTemp->fadeSpeed = 8;

	pTemp->entity.curstate.frame = 0;
	pTemp->entity.angles[ROLL] = 45 * Com_RandomLong( 0, 7 );
}

/*
==============
CL_RocketFlare

Create rocket flare
==============
*/
void GAME_EXPORT CL_RocketFlare( const vec3_t pos )
{
	TEMPENTITY	*pTemp;
	model_t		*pmodel;
	int		modelIndex;
	int		nframeCount;

	modelIndex = CL_FindModelIndex( "sprites/animglow01.spr" );
	pmodel = Mod_Handle( modelIndex );
	if( !pmodel ) return;

	Mod_GetFrames( modelIndex, &nframeCount );

	pTemp = CL_TempEntAlloc( pos, pmodel );
	if ( !pTemp ) return;

	pTemp->frameMax = nframeCount - 1;
	pTemp->entity.curstate.rendermode = kRenderGlow;
	pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
	pTemp->entity.curstate.renderamt = 200;
	pTemp->entity.curstate.framerate = 1.0;
	pTemp->entity.curstate.frame = Com_RandomLong( 0, nframeCount - 1 );
	pTemp->entity.curstate.scale = 1.0;
	pTemp->die = cl.time + 0.01f;	// when 100 fps die at next frame
	pTemp->flags |= FTENT_SPRANIMATE;
}

/*
==============
CL_MuzzleFlash

Do muzzleflash
==============
*/
void GAME_EXPORT CL_MuzzleFlash( const vec3_t pos, int type )
{
	TEMPENTITY	*pTemp;
	int		index, modelIndex, frameCount;
	float		scale;

	index = bound( 0, type % 5, MAX_MUZZLEFLASH - 1 );
	scale = (type / 10) * 0.1f;
	if( scale == 0.0f ) scale = 0.5f;

	modelIndex = cl_muzzleflash[index];
	if( !modelIndex ) return;

	Mod_GetFrames( modelIndex, &frameCount );

	// must set position for right culling on render
	pTemp = CL_TempEntAllocHigh( pos, Mod_Handle( modelIndex ));
	if( !pTemp ) return;
	pTemp->entity.curstate.rendermode = kRenderTransAdd;
	pTemp->entity.curstate.renderamt = 255;
	pTemp->entity.curstate.framerate = 10;
	pTemp->entity.curstate.renderfx = 0;
	pTemp->die = cl.time + 0.01; // die at next frame
	pTemp->entity.curstate.frame = Com_RandomLong( 0, frameCount - 1 );
	pTemp->flags |= FTENT_SPRANIMATE|FTENT_SPRANIMATELOOP;
	pTemp->frameMax = frameCount - 1;

	if( index == 0 )
	{
		// Rifle flash
		pTemp->entity.curstate.scale = scale * Com_RandomFloat( 0.5f, 0.6f );
		pTemp->entity.angles[2] = 90 * Com_RandomLong( 0, 3 );
	}
	else
	{
		pTemp->entity.curstate.scale = scale;
		pTemp->entity.angles[2] = Com_RandomLong( 0, 359 );
	}

	// play playermodel muzzleflashes only for mirror pass
	if( RP_LOCALCLIENT( RI.currententity ) && !RI.thirdPerson && ( RI.params & RP_MIRRORVIEW ))
		pTemp->entity.curstate.effects |= EF_REFLECTONLY;

	CL_TEntAddEntity( &pTemp->entity );
}

/*
==============
CL_BloodSprite

Create a high priority blood sprite
and some blood drops. This is high-priority tent
==============
*/
void GAME_EXPORT CL_BloodSprite( const vec3_t org, int colorIndex, int modelIndex, int modelIndex2, float size )
{
	TEMPENTITY	*pTemp;

	if( Mod_GetType( modelIndex ) == mod_bad )
		return;

	// large, single blood sprite is a high-priority tent
	if(( pTemp = CL_TempEntAllocHigh( org, Mod_Handle( modelIndex ))) != NULL )
	{
		int	i, frameCount, drips;

		colorIndex = bound( 0, colorIndex, 256 );

		drips = size + Com_RandomLong( 1, 16 );

		Mod_GetFrames( modelIndex, &frameCount );
		pTemp->entity.curstate.rendermode = kRenderTransTexture;
		pTemp->entity.curstate.renderfx = kRenderFxClampMinScale;
		pTemp->entity.curstate.scale = Com_RandomFloat(( size / 25.0f ), ( size / 35.0f ));
		pTemp->flags = FTENT_SPRANIMATE;

		pTemp->entity.curstate.rendercolor.r = clgame.palette[colorIndex][0];
		pTemp->entity.curstate.rendercolor.g = clgame.palette[colorIndex][1];
		pTemp->entity.curstate.rendercolor.b = clgame.palette[colorIndex][2];
		pTemp->entity.curstate.framerate = frameCount * 4; // Finish in 0.250 seconds
		// play the whole thing once
		pTemp->die = cl.time + (frameCount / pTemp->entity.curstate.framerate);

		pTemp->entity.angles[2] = Com_RandomLong( 0, 360 );
		pTemp->bounceFactor = 0;

		Mod_GetFrames( modelIndex2, &frameCount );

		// create blood drops
		for( i = 0; i < drips; i++ )
		{
			pTemp = CL_TempEntAlloc( org, Mod_Handle( modelIndex2 ));

			if( !pTemp )
				return;

			pTemp->flags = FTENT_COLLIDEWORLD|FTENT_SLOWGRAVITY|FTENT_ROTATE;

			pTemp->entity.curstate.rendermode = kRenderTransTexture;
			pTemp->entity.curstate.renderfx = kRenderFxClampMinScale;
			pTemp->entity.curstate.scale = Com_RandomFloat(( size / 25.0f ), ( size / 15.0f ));
			pTemp->entity.curstate.rendercolor.r = clgame.palette[colorIndex][0];
			pTemp->entity.curstate.rendercolor.g = clgame.palette[colorIndex][1];
			pTemp->entity.curstate.rendercolor.b = clgame.palette[colorIndex][2];
			pTemp->entity.curstate.frame = Com_RandomLong( 0, frameCount - 1 );
			pTemp->die = cl.time + Com_RandomFloat( 1.0f, 2.0f );

			VectorSet( pTemp->entity.baseline.origin,
				Com_RandomFloat( -96, 95 ), Com_RandomFloat( -96, 95 ), Com_RandomFloat( -32, 95 ) );

			VectorSet( pTemp->entity.baseline.angles,
				Com_RandomFloat( -256, -255 ), Com_RandomFloat( -256, -255 ), Com_RandomFloat( -256, -255 ));

			pTemp->entity.angles[2] = Com_RandomLong( 0, 360 );
			pTemp->bounceFactor = 0;
		}
	}
}

/*
==============
CL_BreakModel

Create a shards
==============
*/
void GAME_EXPORT CL_BreakModel( const vec3_t pos, const vec3_t size, const vec3_t direction, float random, float life, int count, int modelIndex, char flags )
{
	int		i, frameCount;
	int		numtries = 0;
	TEMPENTITY	*pTemp;
	char		type;
	vec3_t		dir;

	if( !modelIndex ) return;
	type = flags & BREAK_TYPEMASK;

	if( Mod_GetType( modelIndex ) == mod_bad )
		return;

	Mod_GetFrames( modelIndex, &frameCount );
		
	if( count == 0 )
	{
		// assume surface (not volume)
		count = (size[0] * size[1] + size[1] * size[2] + size[2] * size[0]) / (3 * SHARD_VOLUME * SHARD_VOLUME);
	}

	VectorCopy( direction, dir );

	// limit to 100 pieces
	if( count > 100 ) count = 100;

	if( VectorIsNull( direction ))
		random *= 10;

	for( i = 0; i < count; i++ ) 
	{
		vec3_t	vecSpot;

		if( VectorIsNull( direction ))
		{
			// random direction for each piece
			dir[0] = Com_RandomFloat( -1.0f, 1.0f );
			dir[1] = Com_RandomFloat( -1.0f, 1.0f );
			dir[2] = Com_RandomFloat( -1.0f, 1.0f );

			VectorNormalize( dir );
		}
		numtries = 0;
tryagain:
		// fill up the box with stuff
		vecSpot[0] = pos[0] + Com_RandomFloat( -0.5f, 0.5f ) * size[0];
		vecSpot[1] = pos[1] + Com_RandomFloat( -0.5f, 0.5f ) * size[1];
		vecSpot[2] = pos[2] + Com_RandomFloat( -0.5f, 0.5f ) * size[2];

		if( CL_PointContents( vecSpot ) == CONTENTS_SOLID )
		{
			if( ++numtries < 32 )
				goto tryagain;
			continue;	// a piece completely stuck in the wall, ignore it
		}

		pTemp = CL_TempEntAlloc( vecSpot, Mod_Handle( modelIndex ));
		if( !pTemp ) return;

		// keep track of break_type, so we know how to play sound on collision
		pTemp->hitSound = type;
		
		if( Mod_GetType( modelIndex ) == mod_sprite )
			pTemp->entity.curstate.frame = Com_RandomLong( 0, frameCount - 1 );
		else if( Mod_GetType( modelIndex ) == mod_studio )
			pTemp->entity.curstate.body = Com_RandomLong( 0, frameCount - 1 );

		pTemp->flags |= FTENT_COLLIDEWORLD | FTENT_FADEOUT | FTENT_SLOWGRAVITY;

		if ( Com_RandomLong( 0, 255 ) < 200 ) 
		{
			pTemp->flags |= FTENT_ROTATE;
			pTemp->entity.baseline.angles[0] = Com_RandomFloat( -256, 255 );
			pTemp->entity.baseline.angles[1] = Com_RandomFloat( -256, 255 );
			pTemp->entity.baseline.angles[2] = Com_RandomFloat( -256, 255 );
		}

		if (( Com_RandomLong( 0, 255 ) < 100 ) && ( flags & BREAK_SMOKE ))
			pTemp->flags |= FTENT_SMOKETRAIL;

		if (( type == BREAK_GLASS ) || ( flags & BREAK_TRANS ))
		{
			pTemp->entity.curstate.rendermode = kRenderTransTexture;
			pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = 128;
		}
		else
		{
			pTemp->entity.curstate.rendermode = kRenderNormal;
			pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = 255; // set this for fadeout
		}

		pTemp->entity.baseline.origin[0] = dir[0] + Com_RandomFloat( -random, random );
		pTemp->entity.baseline.origin[1] = dir[1] + Com_RandomFloat( -random, random );
		pTemp->entity.baseline.origin[2] = dir[2] + Com_RandomFloat( 0, random );

		pTemp->die = cl.time + life + Com_RandomFloat( 0.0f, 1.0f ); // Add an extra 0-1 secs of life
	}
}

/*
==============
CL_TempModel

Create a temp model with gravity, sounds and fadeout
==============
*/
TEMPENTITY *GAME_EXPORT CL_TempModel( const vec3_t pos, const vec3_t dir, const vec3_t angles, float life, int modelIndex, int soundtype )
{
	// alloc a new tempent
	TEMPENTITY	*pTemp;

	pTemp = CL_TempEntAlloc( pos, Mod_Handle( modelIndex ));
	if( !pTemp ) return NULL;

	// keep track of shell type
	switch( soundtype )
	{
	case TE_BOUNCE_SHELL:
		pTemp->hitSound = BOUNCE_SHELL;
		break;
	case TE_BOUNCE_SHOTSHELL:
		pTemp->hitSound = BOUNCE_SHOTSHELL;
		break;
	}

	VectorCopy( pos, pTemp->entity.origin );
	VectorCopy( angles, pTemp->entity.angles );
	VectorCopy( dir, pTemp->entity.baseline.origin );

	pTemp->entity.curstate.body = 0;
	pTemp->flags = (FTENT_COLLIDEWORLD|FTENT_GRAVITY|FTENT_ROTATE);
	pTemp->entity.baseline.angles[0] = Com_RandomFloat( -255, 255 );
	pTemp->entity.baseline.angles[1] = Com_RandomFloat( -255, 255 );
	pTemp->entity.baseline.angles[2] = Com_RandomFloat( -255, 255 );
	pTemp->entity.curstate.rendermode = kRenderNormal;
	pTemp->entity.baseline.renderamt = 255;
	pTemp->die = cl.time + life;

	return pTemp;
}

/*
==============
CL_DefaultSprite

Create an animated sprite
==============
*/
TEMPENTITY *GAME_EXPORT CL_DefaultSprite( const vec3_t pos, int spriteIndex, float framerate )
{
	TEMPENTITY	*pTemp;
	int		frameCount;

	if( !spriteIndex || Mod_GetType( spriteIndex ) != mod_sprite )
	{
		MsgDev( D_INFO, "No Sprite %d!\n", spriteIndex );
		return NULL;
	}

	Mod_GetFrames( spriteIndex, &frameCount );

	pTemp = CL_TempEntAlloc( pos, Mod_Handle( spriteIndex ));
	if( !pTemp ) return NULL;

	pTemp->frameMax = frameCount - 1;
	pTemp->entity.curstate.scale = 1.0f;
	pTemp->flags |= FTENT_SPRANIMATE;
	if( framerate == 0 ) framerate = 10;

	pTemp->entity.curstate.framerate = framerate;
	pTemp->die = cl.time + (float)frameCount / framerate;
	pTemp->entity.curstate.frame = 0;

	return pTemp;
}

/*
===============
CL_TempSprite

Create an animated moving sprite 
===============
*/
TEMPENTITY *GAME_EXPORT CL_TempSprite( const vec3_t pos, const vec3_t dir, float scale, int modelIndex, int rendermode, int renderfx, float a, float life, int flags )
{
	TEMPENTITY	*pTemp;
	int		frameCount;

	if( !modelIndex ) 
		return NULL;

	if( Mod_GetType( modelIndex ) == mod_bad )
	{
		MsgDev( D_INFO, "No model %d!\n", modelIndex );
		return NULL;
	}

	Mod_GetFrames( modelIndex, &frameCount );

	pTemp = CL_TempEntAlloc( pos, Mod_Handle( modelIndex ));
	if( !pTemp ) return NULL;

	pTemp->frameMax = frameCount - 1;
	pTemp->entity.curstate.framerate = 10;
	pTemp->entity.curstate.rendermode = rendermode;
	pTemp->entity.curstate.renderfx = renderfx;
	pTemp->entity.curstate.scale = scale;
	pTemp->entity.baseline.renderamt = a * 255;
	pTemp->entity.curstate.renderamt = a * 255;
	pTemp->flags |= flags;

	VectorCopy( pos, pTemp->entity.origin );
	VectorCopy( dir, pTemp->entity.baseline.origin );

	if( life ) pTemp->die = cl.time + life;
	else pTemp->die = cl.time + ( frameCount * 0.1f ) + 1.0f;
	pTemp->entity.curstate.frame = 0;

	return pTemp;
}

/*
===============
CL_Sprite_Explode

apply params for exploding sprite
===============
*/
void GAME_EXPORT CL_Sprite_Explode( TEMPENTITY *pTemp, float scale, int flags )
{
	if( !pTemp ) return;

	if( flags & TE_EXPLFLAG_NOADDITIVE )
	{
		// solid sprite
		pTemp->entity.curstate.rendermode = kRenderNormal;
		pTemp->entity.curstate.renderamt = 255; 
	}
	else if( flags & TE_EXPLFLAG_DRAWALPHA )
	{
		// alpha sprite (came from hl2)
		pTemp->entity.curstate.rendermode = kRenderTransAlpha;
		pTemp->entity.curstate.renderamt = 180;
	}
	else
	{
		// additive sprite
		pTemp->entity.curstate.rendermode = kRenderTransAdd;
		pTemp->entity.curstate.renderamt = 120;
	}

	if( flags & TE_EXPLFLAG_ROTATE )
	{
		// came from hl2
		pTemp->entity.angles[2] = Com_RandomLong( 0, 360 );
	}

	pTemp->entity.curstate.renderfx = kRenderFxNone;
	pTemp->entity.baseline.origin[2] = 8;
	pTemp->entity.origin[2] += 10;
	pTemp->entity.curstate.scale = scale;
}

/*
===============
CL_Sprite_Smoke

apply params for smoke sprite
===============
*/
void GAME_EXPORT CL_Sprite_Smoke( TEMPENTITY *pTemp, float scale )
{
	int	iColor;

	if( !pTemp ) return;

	iColor = Com_RandomLong( 20, 35 );
	pTemp->entity.curstate.rendermode = kRenderTransAlpha;
	pTemp->entity.curstate.renderfx = kRenderFxNone;
	pTemp->entity.baseline.origin[2] = 30;
	pTemp->entity.curstate.rendercolor.r = iColor;
	pTemp->entity.curstate.rendercolor.g = iColor;
	pTemp->entity.curstate.rendercolor.b = iColor;
	pTemp->entity.origin[2] += 20;
	pTemp->entity.curstate.scale = scale;
}

/*
===============
CL_Spray

Throws a shower of sprites or models
===============
*/
void GAME_EXPORT CL_Spray( const vec3_t pos, const vec3_t dir, int modelIndex, int count, int speed, int iRand, int renderMode )
{
	TEMPENTITY	*pTemp;
	float		noise;
	float		znoise;
	int		i, frameCount;

	noise = (float)iRand / 100;

	// more vertical displacement
	znoise = noise * 1.5f;
	if( znoise > 1 ) znoise = 1;

	if( Mod_GetType( modelIndex ) == mod_bad )
	{
		MsgDev( D_INFO, "No model %d!\n", modelIndex );
		return;
	}

	Mod_GetFrames( modelIndex, &frameCount );

	for( i = 0; i < count; i++ )
	{
		vec3_t	velocity;
		float	scale;

		pTemp = CL_TempEntAlloc( pos, Mod_Handle( modelIndex ));
		if( !pTemp ) return;

		pTemp->entity.curstate.rendermode = renderMode;
		pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
		pTemp->entity.curstate.scale = 0.5f;
		pTemp->entity.baseline.renderamt = 255;
		pTemp->flags |= FTENT_FADEOUT|FTENT_SLOWGRAVITY;
		pTemp->fadeSpeed = 2.0f;

		// make the spittle fly the direction indicated, but mix in some noise.
		velocity[0] = dir[0] + Com_RandomFloat( -noise, noise );
		velocity[1] = dir[1] + Com_RandomFloat( -noise, noise );
		velocity[2] = dir[2] + Com_RandomFloat( 0, znoise );
		scale = Com_RandomFloat(( speed * 0.8f ), ( speed * 1.2f ));
		VectorScale( velocity, scale, pTemp->entity.baseline.origin );
		pTemp->die = cl.time + 0.35f;
		pTemp->entity.curstate.frame = Com_RandomLong( 0, frameCount - 1 );
	}
}

/*
===============
CL_Sprite_Spray

Spray of alpha sprites
===============
*/
void CL_Sprite_Spray( const vec3_t pos, const vec3_t dir, int modelIndex, int count, int speed, int iRand )
{
	CL_Spray( pos, dir, modelIndex, count, speed, iRand, kRenderTransAlpha );
}

/*
===============
CL_Sprite_Trail

Line of moving glow sprites with gravity,
fadeout, and collisions
===============
*/
void GAME_EXPORT CL_Sprite_Trail( int type, const vec3_t vecStart, const vec3_t vecEnd, int modelIndex, int nCount, float flLife, float flSize, float flAmplitude, int nRenderamt, float flSpeed )
{
	TEMPENTITY	*pTemp;
	vec3_t		vecDelta, vecDir;
	int		i, flFrameCount;

	if( Mod_GetType( modelIndex ) == mod_bad )
	{
		MsgDev( D_INFO, "No model %d!\n", modelIndex );
		return;
	}	

	Mod_GetFrames( modelIndex, &flFrameCount );

	VectorSubtract( vecEnd, vecStart, vecDelta );
	VectorNormalize2( vecDelta, vecDir );

	flAmplitude /= 256.0f;

	for( i = 0; i < nCount; i++ )
	{
		vec3_t	vecPos, vecVel;

		// Be careful of divide by 0 when using 'count' here...
		if( i == 0 ) VectorCopy( vecStart, vecPos );
		else VectorMA( vecStart, ( i / ( nCount - 1.0f )), vecDelta, vecPos );

		pTemp = CL_TempEntAlloc( vecPos, Mod_Handle( modelIndex ));
		if( !pTemp ) return;

		pTemp->flags = (FTENT_COLLIDEWORLD|FTENT_SPRCYCLE|FTENT_FADEOUT|FTENT_SLOWGRAVITY);

		VectorScale( vecDir, flSpeed, vecVel );
		vecVel[0] += Com_RandomFloat( -127.0f, 128.0f ) * flAmplitude;
		vecVel[1] += Com_RandomFloat( -127.0f, 128.0f ) * flAmplitude;
		vecVel[2] += Com_RandomFloat( -127.0f, 128.0f ) * flAmplitude;
		VectorCopy( vecVel, pTemp->entity.baseline.origin );
		VectorCopy( vecPos, pTemp->entity.origin );

		pTemp->entity.curstate.scale = flSize;
		pTemp->entity.curstate.rendermode = kRenderGlow;
		pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
		pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = nRenderamt;

		pTemp->entity.curstate.frame = Com_RandomLong( 0, flFrameCount - 1 );
		pTemp->frameMax = flFrameCount - 1;
		pTemp->die = cl.time + flLife + Com_RandomFloat( 0.0f, 4.0f );
	}
}

/*
===============
CL_Large_Funnel

Create a funnel effect (particles only)
===============
*/
void GAME_EXPORT CL_Large_Funnel( const vec3_t pos, int flags )
{
	CL_FunnelSprite( pos, 0, flags );
}

/*
===============
CL_FunnelSprite

Create a funnel effect with custom sprite
===============
*/
void GAME_EXPORT CL_FunnelSprite( const vec3_t pos, int spriteIndex, int flags )
{
	TEMPENTITY	*pTemp = NULL;
	particle_t	*pPart = NULL;
	vec3_t		dir, dest;
	vec3_t		m_vecPos;
	float		flDist, life, vel;
	int		i, j, colorIndex;

	colorIndex = CL_LookupColor( 0, 255, 0 ); // green color

	for( i = -256; i <= 256; i += 32 )
	{
		for( j = -256; j <= 256; j += 32 )
		{
			if( flags & SF_FUNNEL_REVERSE )
			{
				VectorCopy( pos, m_vecPos );

				dest[0] = pos[0] + i;
				dest[1] = pos[1] + j;
				dest[2] = pos[2] + Com_RandomFloat( 100, 800 );

				// send particle heading to dest at a random speed
				VectorSubtract( dest, m_vecPos, dir );

				// velocity based on how far particle has to travel away from org
				vel = dest[2] / 8;
			}
			else
			{
				m_vecPos[0] = pos[0] + i;
				m_vecPos[1] = pos[1] + j;
				m_vecPos[2] = pos[2] + Com_RandomFloat( 100, 800 );

				// send particle heading to org at a random speed
				VectorSubtract( pos, m_vecPos, dir );

				// velocity based on how far particle starts from org
				vel = m_vecPos[2] / 8;
			}

			if( pPart && spriteIndex && CL_PointContents( m_vecPos ) == CONTENTS_EMPTY )
			{
				pTemp = CL_TempEntAlloc( pos, Mod_Handle( spriteIndex ));
				pPart = NULL;
			}
			else
			{
				pPart = CL_AllocParticle( NULL );
				pTemp = NULL;
			}

			if( pTemp || pPart )
			{
				flDist = VectorNormalizeLength( dir );	// save the distance
				if( vel < 64 ) vel = 64;
				
				vel += Com_RandomFloat( 64, 128  );
				life = ( flDist / vel );

				if( pTemp )
				{
					VectorCopy( m_vecPos, pTemp->entity.origin );
					VectorScale( dir, vel, pTemp->entity.baseline.origin );
					pTemp->entity.curstate.rendermode = kRenderTransAdd;
					pTemp->flags |= FTENT_FADEOUT;
					pTemp->fadeSpeed = 3.0f;
					pTemp->die = cl.time + life - Com_RandomFloat( 0.5f, 0.6f );
					pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = 255;
					pTemp->entity.curstate.scale = 0.75f;
				}
				
				if( pPart )
				{
					VectorCopy( m_vecPos, pPart->org );
					pPart->color = colorIndex;
					pPart->type = pt_static;

					VectorScale( dir, vel, pPart->vel );
					// die right when you get there
					pPart->die += life;
				}
			}
		}
	}
}

/*
===============
CL_SparkEffect

Create a custom sparkle effect
===============
*/
void GAME_EXPORT CL_SparkEffect( const vec3_t pos, int count, int velocityMin, int velocityMax )
{
	vec3_t	m_vecDir;
	model_t	*pmodel;
	float	vel;
	int	i;

	pmodel = Mod_Handle( CL_FindModelIndex( "sprites/richo1.spr" ));
	CL_RicochetSprite( pos, pmodel, 0.0f, Com_RandomFloat( 0.4f, 0.6f ));

	for( i = 0; i < Com_RandomLong( 1, count ); i++ )
	{
		m_vecDir[0] = Com_RandomFloat( velocityMin, velocityMax );
		m_vecDir[1] = Com_RandomFloat( velocityMin, velocityMax );
		m_vecDir[2] = Com_RandomFloat( velocityMin, velocityMax );
		vel = VectorNormalizeLength( m_vecDir );

		CL_SparkleTracer( pos, m_vecDir, vel );
	}
}

/*
===============
CL_StreakSplash

Create a streak tracers
===============
*/
void GAME_EXPORT CL_StreakSplash( const vec3_t pos, const vec3_t dir, int color, int count, float speed, int velMin, int velMax )
{
	int	i;

	for( i = 0; i < count; i++ )
	{
		vec3_t	vel;

		vel[0] = (dir[0] * speed) + Com_RandomFloat( velMin, velMax );
		vel[1] = (dir[1] * speed) + Com_RandomFloat( velMin, velMax );
		vel[2] = (dir[2] * speed) + Com_RandomFloat( velMin, velMax );

		CL_StreakTracer( pos, vel, color );
	}
}

/*
===============
CL_StreakSplash

Create a sparks like streaks
===============
*/
void GAME_EXPORT CL_SparkStreaks( const vec3_t pos, int count, int velocityMin, int velocityMax )
{
	particle_t	*p;
	float		speed;
	int		i, j;

	for( i = 0; i < count; i++ )
	{
		vec3_t	vel;

		vel[0] = Com_RandomFloat( velocityMin, velocityMax );
		vel[1] = Com_RandomFloat( velocityMin, velocityMax );
		vel[2] = Com_RandomFloat( velocityMin, velocityMax );
		speed = VectorNormalizeLength( vel );

		CL_SparkleTracer( pos, vel, speed );
	}

	for( i = 0; i < 12; i++ )
	{
		p = CL_AllocParticle( NULL );
		if( !p ) return;

		p->die += 1.0f;
		p->color = 0; // black

		p->type = pt_grav;
		for( j = 0; j < 3; j++ )
		{
			p->org[j] = pos[j] + Com_RandomFloat( -2.0f, 3.0f );
			p->vel[j] = Com_RandomFloat( velocityMin, velocityMax );
		}
	}
}

/*
==============
CL_RicochetSound

Make a random ricochet sound
==============
*/
void GAME_EXPORT CL_RicochetSound( const vec3_t pos )
{
	char	soundpath[32];
	int	iPitch = Com_RandomLong( 90, 105 );
	float	fvol = Com_RandomFloat( 0.7f, 0.9f );
	sound_t	handle;

	Q_snprintf( soundpath, sizeof( soundpath ), "weapons/ric%i.wav", Com_RandomLong( 1, 5 ));
	handle = S_RegisterSound( soundpath );

	S_StartSound( pos, 0, CHAN_AUTO, handle, fvol, ATTN_NORM, iPitch, 0 );
}

/*
==============
CL_Projectile

Create an projectile entity
==============
*/
void GAME_EXPORT CL_Projectile( const vec3_t origin, const vec3_t velocity, int modelIndex, int life, int owner, void (*hitcallback)( TEMPENTITY*, pmtrace_t* ))
{
	// alloc a new tempent
	TEMPENTITY	*pTemp;

	pTemp = CL_TempEntAlloc( origin, Mod_Handle( modelIndex ));
	if( !pTemp ) return;

	VectorAngles( velocity, pTemp->entity.angles );
	VectorCopy( velocity, pTemp->entity.baseline.origin );

	pTemp->entity.curstate.body = 0;
	pTemp->flags = FTENT_COLLIDEALL|FTENT_COLLIDEKILL;
	pTemp->entity.curstate.rendermode = kRenderNormal;
	pTemp->entity.baseline.renderamt = 255;
	pTemp->hitcallback = hitcallback;
	pTemp->die = cl.time + life;
	pTemp->clientIndex = owner;
}

/*
==============
CL_TempSphereModel

Spherical shower of models, picks from set
==============
*/
void GAME_EXPORT CL_TempSphereModel( const vec3_t pos, float speed, float life, int count, int modelIndex )
{
	vec3_t		forward, dir;
	TEMPENTITY	*pTemp;
	int		i;
	
	VectorSet( forward, 0.0f, 0.0f, -1.0f ); // down-vector

	// create temp models
	for( i = 0; i < count; i++ )
	{
		pTemp = CL_TempEntAlloc( pos, Mod_Handle( modelIndex ));
		if( !pTemp ) return;

		dir[0] = forward[0] + Com_RandomFloat( -0.3f, 0.3f );
		dir[1] = forward[1] + Com_RandomFloat( -0.3f, 0.3f );
		dir[2] = forward[2] + Com_RandomFloat( -0.3f, 0.3f );

		VectorScale( dir, Com_RandomFloat( 30.0f, 40.0f ), pTemp->entity.baseline.origin );
		pTemp->entity.curstate.body = 0;
		pTemp->flags = (FTENT_COLLIDEWORLD|FTENT_SMOKETRAIL|FTENT_FLICKER|FTENT_GRAVITY|FTENT_ROTATE);
		pTemp->entity.baseline.angles[0] = Com_RandomFloat( -255, 255 );
		pTemp->entity.baseline.angles[1] = Com_RandomFloat( -255, 255 );
		pTemp->entity.baseline.angles[2] = Com_RandomFloat( -255, 255 );
		pTemp->entity.curstate.rendermode = kRenderNormal;
		pTemp->entity.baseline.renderamt = 255;
		pTemp->die = cl.time + life;
	}
}

/*
==============
CL_Explosion

Create an explosion (scale is magnitude)
==============
*/
void GAME_EXPORT CL_Explosion( vec3_t pos, int model, float scale, float framerate, int flags )
{
	TEMPENTITY	*pTemp;
	sound_t		hSound;

	if( scale != 0.0f )
	{
		// create explosion sprite
		pTemp = CL_DefaultSprite( pos, model, framerate );
		CL_Sprite_Explode( pTemp, scale, flags );

		if( !( flags & TE_EXPLFLAG_NODLIGHTS ))
		{
			dlight_t	*dl;

			// big flash
			dl = CL_AllocDlight( 0 );
			VectorCopy( pos, dl->origin );
			dl->radius = 200;
			dl->color.r = dl->color.g = 250;
			dl->color.b = 150;
			dl->die = cl.time + 0.25f;
			dl->decay = 800;

			// red glow
			dl = CL_AllocDlight( 0 );
			VectorCopy( pos, dl->origin );
			dl->radius = 150;
			dl->color.r = 255;
			dl->color.g= 190;
			dl->color.b = 40;
			dl->die = cl.time + 1.0f;
			dl->decay = 200;
		}
	}

	if(!( flags & TE_EXPLFLAG_NOPARTICLES ))
		CL_FlickerParticles( pos );

	if( flags & TE_EXPLFLAG_NOSOUND ) return;

	hSound = S_RegisterSound( va( "weapons/explode%i.wav", Com_RandomLong( 3, 5 )));
	S_StartSound( pos, 0, CHAN_AUTO, hSound, VOL_NORM, ATTN_NORM, PITCH_NORM, 0 );
}

/*
==============
CL_PlayerSprites

Create a particle smoke around player
==============
*/
void GAME_EXPORT CL_PlayerSprites( int client, int modelIndex, int count, int size )
{
	TEMPENTITY	*pTemp;
	cl_entity_t	*pEnt;
	float		vel;
	int		i;

	pEnt = CL_GetEntityByIndex( client );

	if( !pEnt || !pEnt->player )
	{
		MsgDev( D_INFO, "Bad ent %i in R_PlayerSprites()!\n", client );
		return;
	}

	vel = 128;

	for( i = 0; i < count; i++ )
	{
		pTemp = CL_DefaultSprite( pEnt->origin, modelIndex, 15 );
		if( !pTemp ) return;

		pTemp->entity.curstate.rendermode = kRenderTransAlpha;
		pTemp->entity.curstate.renderfx = kRenderFxNone;
		pTemp->entity.baseline.origin[0] = Com_RandomFloat( -1.0f, 1.0f ) * vel;
		pTemp->entity.baseline.origin[1] = Com_RandomFloat( -1.0f, 1.0f ) * vel;
		pTemp->entity.baseline.origin[2] = Com_RandomFloat( 0.0f, 1.0f ) * vel;
		pTemp->entity.curstate.rendercolor.r = 192;
		pTemp->entity.curstate.rendercolor.g = 192;
		pTemp->entity.curstate.rendercolor.b = 192;
		pTemp->entity.curstate.renderamt = 64;
		pTemp->entity.curstate.scale = size;
	}
}

/*
==============
CL_FireField

Makes a field of fire
==============
*/
void GAME_EXPORT CL_FireField( float *org, int radius, int modelIndex, int count, int flags, float life )
{
	TEMPENTITY	*pTemp;
	float		radius2;
	vec3_t		dir, m_vecPos;
	int		i;

	for( i = 0; i < count; i++ )
	{
		dir[0] = Com_RandomFloat( -1.0f, 1.0f );
		dir[1] = Com_RandomFloat( -1.0f, 1.0f );

		if( flags & TEFIRE_FLAG_PLANAR ) dir[2] = 0.0f;
		else dir[2] = Com_RandomFloat( -1.0f, 1.0f );
		VectorNormalize( dir );

		radius2 = Com_RandomFloat( 0.0f, radius );
		VectorMA( org, -radius2, dir, m_vecPos );

		pTemp = CL_DefaultSprite( m_vecPos, modelIndex, 0 );
		if( !pTemp ) return;

		if( flags & TEFIRE_FLAG_ALLFLOAT )
			pTemp->entity.baseline.origin[2] = 30; // drift sprite upward
		else if( flags & TEFIRE_FLAG_SOMEFLOAT && Com_RandomLong( 0, 1 ))
			pTemp->entity.baseline.origin[2] = 30; // drift sprite upward

		if( flags & TEFIRE_FLAG_LOOP )
		{
			pTemp->entity.curstate.framerate = 15;
			pTemp->flags |= FTENT_SPRANIMATELOOP;
		}

		if( flags & TEFIRE_FLAG_ALPHA )
		{
			pTemp->entity.curstate.rendermode = kRenderTransTexture;
			pTemp->entity.curstate.renderamt = 128;
		}
		else if ( flags & TEFIRE_FLAG_ADDITIVE )
		{
			pTemp->entity.curstate.rendermode = kRenderTransAdd;
			pTemp->entity.curstate.renderamt = 80;
		}

		pTemp->die += life;
	}
}

/*
==============
CL_MultiGunshot

Client version of shotgun shot
==============
*/
void GAME_EXPORT CL_MultiGunshot( const vec3_t org, const vec3_t dir, const vec3_t noise, int count, int decalCount, int *decalIndices )
{
	pmtrace_t	trace;
	vec3_t	right, up;
	vec3_t	vecSrc, vecDir, vecEnd;
	int	i, j, decalIndex;

	VectorVectors( dir, right, up );
	VectorCopy( org, vecSrc );

	for( i = 1; i <= count; i++ )
	{
		// get circular gaussian spread
		float x, y, z;
		do {
			x = Com_RandomFloat( -0.5f, 0.5f ) + Com_RandomFloat( -0.5f, 0.5f );
			y = Com_RandomFloat( -0.5f, 0.5f ) + Com_RandomFloat( -0.5f, 0.5f );
			z = x * x + y * y;
		} while( z > 1.0f );

		for( j = 0; j < 3; j++ )
		{
			vecDir[j] = dir[i] + x * noise[0] * right[j] + y * noise[1] * up[j];
			vecEnd[j] = vecSrc[j] + 2048.0f * vecDir[j];
		}

		trace = CL_TraceLine( vecSrc, vecEnd, PM_STUDIO_BOX );

		// paint decals
		if( trace.fraction != 1.0f )
		{
			physent_t	*pe = NULL;

			if( trace.ent >= 0 && trace.ent < clgame.pmove->numphysent )
				pe = &clgame.pmove->physents[trace.ent];

			if( pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ))
			{
				cl_entity_t *e = CL_GetEntityByIndex( pe->info );
				decalIndex = CL_DecalIndex( decalIndices[Com_RandomLong( 0, decalCount-1 )] );
				CL_DecalShoot( decalIndex, e->index, 0, trace.endpos, 0 );
			}
		}
	}
}

/*
==============
CL_Sprite_WallPuff

Create a wallpuff
==============
*/
void GAME_EXPORT CL_Sprite_WallPuff( TEMPENTITY *pTemp, float scale )
{
	if( !pTemp ) return;

	pTemp->entity.curstate.renderamt = 255;
	pTemp->entity.curstate.rendermode = kRenderTransAlpha;
	pTemp->entity.angles[ROLL] = Com_RandomLong( 0, 359 );
	pTemp->entity.curstate.scale = scale;
	pTemp->die = cl.time + 0.01f;
}

/*
==============
CL_ParseTempEntity

handle temp-entity messages
==============
*/
void CL_ParseTempEntity( sizebuf_t *msg )
{
	sizebuf_t		buf;
	byte		pbuf[256];
	int		iSize = BF_ReadByte( msg );
	int		type, color, count, flags;
	int		decalIndex, modelIndex, entityIndex;
	int		iLife;
	float		scale, life, frameRate, vel, random;
	float		brightness, r, g, b;
	vec3_t		pos, pos2, ang;
	int		decalIndices[1];	// just stub
	TEMPENTITY	*pTemp;
	cl_entity_t	*pEnt;
	dlight_t		*dl;

	decalIndex = modelIndex = entityIndex = 0;

	// parse user message into buffer
	BF_ReadBytes( msg, pbuf, iSize );

	// init a safe tempbuffer
	BF_Init( &buf, "TempEntity", pbuf, iSize );

	type = BF_ReadByte( &buf );

	switch( type )
	{
	case TE_BEAMPOINTS:
	case TE_BEAMENTPOINT:
	case TE_LIGHTNING:
	case TE_BEAMENTS:
	case TE_BEAM:
	case TE_BEAMSPRITE:
	case TE_BEAMTORUS:
	case TE_BEAMDISK:
	case TE_BEAMCYLINDER:
	case TE_BEAMFOLLOW:
	case TE_BEAMRING:
	case TE_BEAMHOSE:
	case TE_KILLBEAM:
		CL_ParseViewBeam( &buf, type );
		break;
	case TE_GUNSHOT:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		CL_RicochetSound( pos );
		CL_RunParticleEffect( pos, vec3_origin, 0, 20 );
		break;
	case TE_EXPLOSION:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		modelIndex = BF_ReadShort( &buf );
		scale = (float)(BF_ReadByte( &buf ) * 0.1f);
		frameRate = BF_ReadByte( &buf );
		flags = BF_ReadByte( &buf );
		CL_Explosion( pos, modelIndex, scale, frameRate, flags );
		break;
	case TE_TAREXPLOSION:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		CL_BlobExplosion( pos );
		break;
	case TE_SMOKE:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		modelIndex = BF_ReadShort( &buf );
		scale = (float)(BF_ReadByte( &buf ) * 0.1f);
		frameRate = BF_ReadByte( &buf );
		pTemp = CL_DefaultSprite( pos, modelIndex, frameRate );
		CL_Sprite_Smoke( pTemp, scale );
		break;
	case TE_TRACER:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		CL_TracerEffect( pos, pos2 );
		break;
	case TE_SPARKS:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		CL_SparkShower( pos );
		break;
	case TE_LAVASPLASH:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		CL_LavaSplash( pos );
		break;
	case TE_TELEPORT:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		CL_TeleportSplash( pos );
		break;
	case TE_EXPLOSION2:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		color = BF_ReadByte( &buf );
		count = BF_ReadByte( &buf );
		CL_ParticleExplosion2( pos, color, count );
		break;
	case TE_BSPDECAL:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		decalIndex = BF_ReadShort( &buf );
		entityIndex = BF_ReadShort( &buf );
		if( entityIndex ) modelIndex = BF_ReadShort( &buf );
		CL_DecalShoot( CL_DecalIndex( decalIndex ), entityIndex, modelIndex, pos, FDECAL_PERMANENT );
		break;
	case TE_IMPLOSION:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		scale = BF_ReadByte( &buf );
		count = BF_ReadByte( &buf );
		life = (float)(BF_ReadByte( &buf ) * 0.1f);
		CL_Implosion( pos, scale, count, life );
		break;
	case TE_SPRITETRAIL:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		modelIndex = BF_ReadShort( &buf );
		count = BF_ReadByte( &buf );
		life = (float)(BF_ReadByte( &buf ) * 0.1f);
		scale = (float)(BF_ReadByte( &buf ) * 0.1f);
		vel = (float)BF_ReadByte( &buf );
		random = (float)BF_ReadByte( &buf );
		CL_Sprite_Trail( type, pos, pos2, modelIndex, count, life, scale, random, 255, vel );
		break;
	case TE_SPRITE:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		modelIndex = BF_ReadShort( &buf );
		scale = (float)(BF_ReadByte( &buf ) * 0.1f);
		brightness = (float)BF_ReadByte( &buf );

		if(( pTemp = CL_DefaultSprite( pos, modelIndex, 0 )) != NULL )
		{
			pTemp->entity.curstate.scale = scale;
			pTemp->entity.baseline.renderamt = brightness;
			pTemp->entity.curstate.renderamt = brightness;
			pTemp->entity.curstate.rendermode = kRenderTransAdd;
		}
		break;
	case TE_GLOWSPRITE:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		modelIndex = BF_ReadShort( &buf );
		life = (float)(BF_ReadByte( &buf ) * 0.1f);
		scale = (float)(BF_ReadByte( &buf ) * 0.1f);
		brightness = (float)BF_ReadByte( &buf );

		if(( pTemp = CL_DefaultSprite( pos, modelIndex, 0 )) != NULL )
		{
			pTemp->entity.curstate.scale = scale;
			pTemp->entity.curstate.rendermode = kRenderGlow;
			pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
			pTemp->entity.baseline.renderamt = brightness;
			pTemp->entity.curstate.renderamt = brightness;
			pTemp->flags = FTENT_FADEOUT;
			pTemp->die = cl.time + life;
		}
		break;
	case TE_STREAK_SPLASH:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		color = BF_ReadByte( &buf );
		count = BF_ReadShort( &buf );
		vel = (float)BF_ReadShort( &buf );
		random = (float)BF_ReadShort( &buf );
		CL_StreakSplash( pos, pos2, color, count, vel, -random, random );
		break;
	case TE_DLIGHT:
		dl = CL_AllocDlight( 0 );
		dl->origin[0] = BF_ReadCoord( &buf );
		dl->origin[1] = BF_ReadCoord( &buf );
		dl->origin[2] = BF_ReadCoord( &buf );
		dl->radius = (float)(BF_ReadByte( &buf ) * 10.0f);
		dl->color.r = BF_ReadByte( &buf );
		dl->color.g = BF_ReadByte( &buf );
		dl->color.b = BF_ReadByte( &buf );
		dl->die = cl.time + (float)(BF_ReadByte( &buf ) * 0.1f);
		dl->decay = (float)(BF_ReadByte( &buf ) * 10.0f);
		break;
	case TE_ELIGHT:
		dl = CL_AllocElight( 0 );
		entityIndex = BF_ReadShort( &buf );
		dl->origin[0] = BF_ReadCoord( &buf );
		dl->origin[1] = BF_ReadCoord( &buf );
		dl->origin[2] = BF_ReadCoord( &buf );
		dl->radius = BF_ReadCoord( &buf );
		dl->color.r = BF_ReadByte( &buf );
		dl->color.g = BF_ReadByte( &buf );
		dl->color.b = BF_ReadByte( &buf );
		dl->die = cl.time + (float)(BF_ReadByte( &buf ) * 0.1f);
		dl->decay = BF_ReadCoord( &buf );
		break;
	case TE_TEXTMESSAGE:
		CL_ParseTextMessage( &buf );
		break;
	case TE_LINE:
	case TE_BOX:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		life = (float)(BF_ReadShort( &buf ) * 0.1f);
		r = BF_ReadByte( &buf );
		g = BF_ReadByte( &buf );
		b = BF_ReadByte( &buf );
		if( type == TE_LINE ) CL_ParticleLine( pos, pos2, r, g, b, life );
		else CL_ParticleBox( pos, pos2, r, g, b, life );
		break;
	case TE_LARGEFUNNEL:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		modelIndex = BF_ReadShort( &buf );
		flags = BF_ReadShort( &buf );
		CL_FunnelSprite( pos, modelIndex, flags );
		break;
	case TE_BLOODSTREAM:
	case TE_BLOOD:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		color = BF_ReadByte( &buf );
		vel = (float)BF_ReadByte( &buf );
		if( type == TE_BLOOD ) CL_Blood( pos, pos2, color, vel );
		else CL_BloodStream( pos, pos2, color, vel );
		break;
	case TE_SHOWLINE:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		CL_ShowLine( pos, pos2 );
		break;
	case TE_DECAL:
	case TE_DECALHIGH:
	case TE_WORLDDECAL:
	case TE_WORLDDECALHIGH:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		decalIndex = BF_ReadByte( &buf );
		if( type == TE_DECAL || type == TE_DECALHIGH )
			entityIndex = BF_ReadShort( &buf );
		else entityIndex = 0;
		if( type == TE_DECALHIGH || type == TE_WORLDDECALHIGH )
			decalIndex += 256;
		pEnt = CL_GetEntityByIndex( entityIndex );
		if( pEnt ) modelIndex = pEnt->curstate.modelindex;
		CL_DecalShoot( CL_DecalIndex( decalIndex ), entityIndex, modelIndex, pos, 0 );
		break;
	case TE_FIZZ:
		entityIndex = BF_ReadShort( &buf );
		modelIndex = BF_ReadShort( &buf );
		scale = BF_ReadByte( &buf );	// same as density
		pEnt = CL_GetEntityByIndex( entityIndex );
		CL_FizzEffect( pEnt, modelIndex, scale );
		break;
	case TE_MODEL:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		VectorSet( ang, 0.0f, BF_ReadAngle( &buf ), 0.0f ); // yaw angle
		modelIndex = BF_ReadShort( &buf );
		flags = BF_ReadByte( &buf );	// sound flags
		life = (float)(BF_ReadByte( &buf ) * 0.1f);
		CL_TempModel( pos, pos2, ang, life, modelIndex, flags );
		break;
	case TE_EXPLODEMODEL:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		vel = BF_ReadCoord( &buf );
		modelIndex = BF_ReadShort( &buf );
		count = BF_ReadShort( &buf );
		life = (float)(BF_ReadByte( &buf ) * 0.1f);
		CL_TempSphereModel( pos, vel, life, count, modelIndex );
		break;
	case TE_BREAKMODEL:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		ang[0] = BF_ReadCoord( &buf );
		ang[1] = BF_ReadCoord( &buf );
		ang[2] = BF_ReadCoord( &buf );
		random = (float)BF_ReadByte( &buf );
		modelIndex = BF_ReadShort( &buf );
		count = BF_ReadByte( &buf );
		life = (float)(BF_ReadByte( &buf ) * 0.1f);
		flags = BF_ReadByte( &buf );
		CL_BreakModel( pos, pos2, ang, random, life, count, modelIndex, (char)flags );
		break;
	case TE_GUNSHOTDECAL:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		entityIndex = BF_ReadShort( &buf );
		decalIndex = BF_ReadByte( &buf );
		pEnt = CL_GetEntityByIndex( entityIndex );
		CL_DecalShoot( CL_DecalIndex( decalIndex ), entityIndex, 0, pos, 0 );
		CL_BulletImpactParticles( pos );
		CL_RicochetSound( pos );
		break;
	case TE_SPRAY:
	case TE_SPRITE_SPRAY:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		modelIndex = BF_ReadShort( &buf );
		count = BF_ReadByte( &buf );
		vel = (float)BF_ReadByte( &buf );
		random = (float)BF_ReadByte( &buf );
		if( type == TE_SPRAY )
		{
			flags = BF_ReadByte( &buf );	// rendermode
			CL_Spray( pos, pos2, modelIndex, count, vel, random, flags );
		}
		else CL_Sprite_Spray( pos, pos2, modelIndex, count, vel, random );
		break;
	case TE_ARMOR_RICOCHET:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		scale = (float)(BF_ReadByte( &buf ) * 0.1f);
		modelIndex = CL_FindModelIndex( "sprites/richo1.spr" );
		CL_RicochetSprite( pos, Mod_Handle( modelIndex ), 0.1f, scale );
		CL_RicochetSound( pos );
		break;
	case TE_PLAYERDECAL:
		color = BF_ReadByte( &buf );	// playernum
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		entityIndex = BF_ReadShort( &buf );
		decalIndex = BF_ReadByte( &buf );
		CL_PlayerDecal( CL_DecalIndex( decalIndex ), entityIndex, pos );
		break;
	case TE_BUBBLES:
	case TE_BUBBLETRAIL:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		scale = BF_ReadCoord( &buf );	// water height
		modelIndex = BF_ReadShort( &buf );
		count = BF_ReadByte( &buf );
		vel = BF_ReadCoord( &buf );
		if( type == TE_BUBBLES ) CL_Bubbles( pos, pos2, scale, modelIndex, count, vel );
		else CL_BubbleTrail( pos, pos2, scale, modelIndex, count, vel );
		break;
	case TE_BLOODSPRITE:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		modelIndex = BF_ReadShort( &buf );	// sprite #1
		decalIndex = BF_ReadShort( &buf );	// sprite #2
		color = BF_ReadByte( &buf );
		scale = (float)BF_ReadByte( &buf );
		CL_BloodSprite( pos, color, modelIndex, decalIndex, scale );
		break;
	case TE_PROJECTILE:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		modelIndex = BF_ReadShort( &buf );
		iLife = BF_ReadByte( &buf );
		color = BF_ReadByte( &buf );	// playernum
		CL_Projectile( pos, pos2, modelIndex, iLife, color, NULL );
		break;
	case TE_PLAYERSPRITES:
		color = BF_ReadShort( &buf );	// entitynum
		modelIndex = BF_ReadShort( &buf );
		count = BF_ReadByte( &buf );
		random = (float)BF_ReadByte( &buf );
		CL_PlayerSprites( color, modelIndex, count, random );
		break;
	case TE_PARTICLEBURST:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		scale = (float)BF_ReadShort( &buf );
		color = BF_ReadByte( &buf );
		life = (float)(BF_ReadByte( &buf ) * 0.1f);
		CL_ParticleBurst( pos, scale, color, life );
		break;
	case TE_FIREFIELD:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		scale = (float)BF_ReadShort( &buf );
		modelIndex = BF_ReadShort( &buf );
		count = BF_ReadByte( &buf );
		flags = BF_ReadByte( &buf );
		life = (float)(BF_ReadByte( &buf ) * 0.1f);
		CL_FireField( pos, scale, modelIndex, count, flags, life );
		break;
	case TE_PLAYERATTACHMENT:
		color = BF_ReadByte( &buf );	// playernum
		scale = BF_ReadCoord( &buf );	// height
		modelIndex = BF_ReadShort( &buf );
		life = (float)(BF_ReadShort( &buf ) * 0.1f);
		CL_AttachTentToPlayer( color, modelIndex, scale, life );
		break;
	case TE_KILLPLAYERATTACHMENTS:
		color = BF_ReadByte( &buf );	// playernum
		CL_KillAttachedTents( color );
		break;
	case TE_MULTIGUNSHOT:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		ang[0] = BF_ReadCoord( &buf ) * 0.01f;
		ang[1] = BF_ReadCoord( &buf ) * 0.01f;
		ang[2] = 0.0f;
		count = BF_ReadByte( &buf );
		decalIndices[0] = BF_ReadByte( &buf );
		CL_MultiGunshot( pos, pos2, ang, count, 1, decalIndices );
		break;
	case TE_USERTRACER:
		pos[0] = BF_ReadCoord( &buf );
		pos[1] = BF_ReadCoord( &buf );
		pos[2] = BF_ReadCoord( &buf );
		pos2[0] = BF_ReadCoord( &buf );
		pos2[1] = BF_ReadCoord( &buf );
		pos2[2] = BF_ReadCoord( &buf );
		life = (float)(BF_ReadByte( &buf ) * 0.1f);
		color = BF_ReadByte( &buf );
		scale = (float)(BF_ReadByte( &buf ) * 0.1f);
		CL_UserTracerParticle( pos, pos2, life, color, scale, 0, NULL );
		break;
	default:
		MsgDev( D_ERROR, "ParseTempEntity: illegible TE message %i\n", type );
		break;
	}

	// throw warning
	if( BF_CheckOverflow( &buf )) MsgDev( D_WARN, "ParseTempEntity: overflow TE message\n" );
}


/*
==============================================================

LIGHT STYLE MANAGEMENT

==============================================================
*/
#define STYLE_LERPING_THRESHOLD	3.0f // because we wan't interpolate fast sequences (like on\off)
		
/*
================
CL_ClearLightStyles
================
*/
void CL_ClearLightStyles( void )
{
	Q_memset( cl.lightstyles, 0, sizeof( cl.lightstyles ));
}

void CL_SetLightstyle( int style, const char *s, float f )
{
	int		i, k;
	lightstyle_t	*ls;
	float		val1, val2;

	ASSERT( s );
	ASSERT( style >= 0 && style < MAX_LIGHTSTYLES );

	ls = &cl.lightstyles[style];

	Q_strncpy( ls->pattern, s, sizeof( ls->pattern ));

	ls->length = Q_strlen( s );
	ls->time = f; // set local time

	for( i = 0; i < ls->length; i++ )
		ls->map[i] = (float)(s[i] - 'a');

	ls->interp = (ls->length <= 1) ? false : true;

	// check for allow interpolate
	// NOTE: fast flickering styles looks ugly when interpolation is running
	for( k = 0; k < (ls->length - 1); k++ )
	{
		val1 = ls->map[(k+0) % ls->length];
		val2 = ls->map[(k+1) % ls->length];

		if( fabs( val1 - val2 ) > STYLE_LERPING_THRESHOLD )
		{
			ls->interp = false;
			break;
		}
	}
	MsgDev( D_AICONSOLE, "Lightstyle %i (%s), interp %s\n", style, ls->pattern, ls->interp ? "Yes" : "No" );
}

/*
==============================================================

DLIGHT MANAGEMENT

==============================================================
*/
dlight_t	cl_dlights[MAX_DLIGHTS];
dlight_t	cl_elights[MAX_ELIGHTS];

/*
================
CL_ClearDlights
================
*/
void CL_ClearDlights( void )
{
	Q_memset( cl_dlights, 0, sizeof( cl_dlights ));
	Q_memset( cl_elights, 0, sizeof( cl_elights ));
}

/*
===============
CL_AllocDlight

===============
*/
dlight_t *GAME_EXPORT CL_AllocDlight( int key )
{
	dlight_t	*dl;
	int	i;

	// first look for an exact key match
	if( key )
	{
		for( i = 0, dl = cl_dlights; i < MAX_DLIGHTS; i++, dl++ )
		{
			if( dl->key == key )
			{
				// reuse this light
				Q_memset( dl, 0, sizeof( *dl ));
				dl->key = key;
				return dl;
			}
		}
	}

	// then look for anything else
	for( i = 0, dl = cl_dlights; i < MAX_DLIGHTS; i++, dl++ )
	{
		if( dl->die < cl.time && dl->key == 0 )
		{
			Q_memset( dl, 0, sizeof( *dl ));
			dl->key = key;
			return dl;
		}
	}

	// otherwise grab first dlight
	dl = &cl_dlights[0];
	Q_memset( dl, 0, sizeof( *dl ));
	dl->key = key;

	return dl;
}

/*
===============
CL_AllocElight

===============
*/
dlight_t *GAME_EXPORT CL_AllocElight( int key )
{
	dlight_t	*dl;
	int	i;

	// first look for an exact key match
	if( key )
	{
		for( i = 0, dl = cl_elights; i < MAX_ELIGHTS; i++, dl++ )
		{
			if( dl->key == key )
			{
				// reuse this light
				Q_memset( dl, 0, sizeof( *dl ));
				dl->key = key;
				return dl;
			}
		}
	}

	// then look for anything else
	for( i = 0, dl = cl_elights; i < MAX_ELIGHTS; i++, dl++ )
	{
		if( dl->die < cl.time && dl->key == 0 )
		{
			Q_memset( dl, 0, sizeof( *dl ));
			dl->key = key;
			return dl;
		}
	}

	// otherwise grab first dlight
	dl = &cl_elights[0];
	Q_memset( dl, 0, sizeof( *dl ));
	dl->key = key;

	return dl;
}

/*
===============
CL_DecayLights

===============
*/
void CL_DecayLights( void )
{
	dlight_t	*dl;
	float	time;
	int	i;
	
	time = cl.time - cl.oldtime;

	for( i = 0, dl = cl_dlights; i < MAX_DLIGHTS; i++, dl++ )
	{
		if( !dl->radius ) continue;

		dl->radius -= time * dl->decay;
		if( dl->radius < 0 ) dl->radius = 0;

		if( dl->die < cl.time || !dl->radius ) 
			Q_memset( dl, 0, sizeof( *dl ));
	}

	for( i = 0, dl = cl_elights; i < MAX_ELIGHTS; i++, dl++ )
	{
		if( !dl->radius ) continue;

		dl->radius -= time * dl->decay;
		if( dl->radius < 0 ) dl->radius = 0;

		if( dl->die < cl.time || !dl->radius ) 
			Q_memset( dl, 0, sizeof( *dl ));
	}
}

#define FLASHLIGHT_DISTANCE		2048	// in units

/*
================
CL_UpdateFlashlight

update client flashlight
================
*/
void CL_UpdateFlashlight( cl_entity_t *pEnt )
{
	vec3_t	vecSrc, vecEnd;
	vec3_t	forward, view_ofs;
	float	falloff;
	pmtrace_t	trace;
	dlight_t	*dl;
	int	key;

	if(( pEnt->index - 1 ) == cl.playernum )
	{
		// get the predicted angles
		AngleVectors( cl.refdef.cl_viewangles, forward, NULL, NULL );
	}
	else
	{
		vec3_t	v_angle;

		// restore viewangles from angles
		v_angle[PITCH] = -pEnt->angles[PITCH] * 3;
		v_angle[YAW] = pEnt->angles[YAW];
		v_angle[ROLL] = 0; 	// no roll

		AngleVectors( v_angle, forward, NULL, NULL );
	}

	VectorClear( view_ofs );

	if(( pEnt->index - 1 ) == cl.playernum )
		VectorCopy( cl.refdef.viewheight, view_ofs );

	VectorAdd( pEnt->origin, view_ofs, vecSrc );
	VectorMA( vecSrc, FLASHLIGHT_DISTANCE, forward, vecEnd );

	trace = CL_TraceLine( vecSrc, vecEnd, PM_STUDIO_BOX );
	falloff = trace.fraction * FLASHLIGHT_DISTANCE;

	if( falloff < 250.0f ) falloff = 1.0f;
	else falloff = 250.0f / falloff;

	falloff *= falloff;

	if( cl.maxclients == 1 )
		key = cl.playernum + 1;
	else key = pEnt->index;

	// update flashlight endpos
	dl = CL_AllocDlight( key );
	VectorCopy( trace.endpos, dl->origin );
	dl->die = cl.time + 0.01f; // die on next frame
	dl->color.r = bound( 0, 255 * falloff, 255 );
	dl->color.g = bound( 0, 255 * falloff, 255 );
	dl->color.b = bound( 0, 255 * falloff, 255 );
	dl->radius = 72;
}

/*
================
CL_TestLights

if cl_testlights is set, create 32 lights models
================
*/
void CL_TestLights( void )
{
	int	i, j;
	float	f, r;
	dlight_t	*dl;

	if( !cl_testlights->integer ) return;
	
	for( i = 0; i < bound( 1, cl_testlights->integer, MAX_DLIGHTS ); i++ )
	{
		dl = &cl_dlights[i];

		r = 64 * ((i % 4) - 1.5f );
		f = 64 * ( i / 4) + 128;

		for( j = 0; j < 3; j++ )
			dl->origin[j] = cl.refdef.vieworg[j] + cl.refdef.forward[j] * f + cl.refdef.right[j] * r;

		dl->color.r = ((((i % 6) + 1) & 1)>>0) * 255;
		dl->color.g = ((((i % 6) + 1) & 2)>>1) * 255;
		dl->color.b = ((((i % 6) + 1) & 4)>>2) * 255;
		dl->die = cl.time + host.frametime;
		dl->radius = 200;
	}
}

/*
==============================================================

DECAL MANAGEMENT

==============================================================
*/
/*
===============
CL_DecalShoot

normal temporary decal
===============
*/
void GAME_EXPORT CL_DecalShoot( int textureIndex, int entityIndex, int modelIndex, float *pos, int flags )
{
	R_DecalShoot( textureIndex, entityIndex, modelIndex, pos, flags, NULL, 1.0f );
}

/*
===============
CL_FireCustomDecal

custom temporary decal
===============
*/
void GAME_EXPORT CL_FireCustomDecal( int textureIndex, int entityIndex, int modelIndex, float *pos, int flags, float scale )
{
	R_DecalShoot( textureIndex, entityIndex, modelIndex, pos, flags, NULL, scale );
}

/*
===============
CL_PlayerDecal

spray custom colored decal (clan logo etc)
===============
*/
void CL_PlayerDecal( int textureIndex, int entityIndex, float *pos )
{
	R_DecalShoot( textureIndex, entityIndex, 0, pos, 0, NULL, 1.0f );
}

/*
===============
CL_DecalIndexFromName

get decal global index from decalname
===============
*/
int GAME_EXPORT CL_DecalIndexFromName( const char *name )
{
	int	i;

	if( !name || !name[0] )
		return 0;

	// look through the loaded sprite name list for SpriteName
	for( i = 0; i < MAX_DECALS - 1 && host.draw_decals[i+1][0]; i++ )
	{
		if( !Q_stricmp( name, (char *)host.draw_decals[i+1] ))
			return i+1;
	}
	return 0; // invalid decal
}

/*
===============
CL_DecalIndex

get texture index from decal global index
===============
*/
int GAME_EXPORT CL_DecalIndex( int id )
{
	id = bound( 0, id, MAX_DECALS - 1 );

	host.decal_loading = true;
	if( !cl.decal_index[id] )
	{
		qboolean	load_external = false;

		if( Mod_AllowMaterials( ))
		{
			char	decalname[64];
			int	gl_texturenum = 0;

			Q_snprintf( decalname, sizeof( decalname ), "materials/decals/%s.tga", host.draw_decals[id] );

			if( FS_FileExists( decalname, false ))
				gl_texturenum = GL_LoadTexture( decalname, NULL, 0, TF_DECAL, NULL );

			if( gl_texturenum )
			{
				byte	*fin;
				mip_t	*mip;

				// find real decal dimensions and store it into texture srcWidth\srcHeight
				if(( fin = FS_LoadFile( va( "decals.wad/%s", host.draw_decals[id] ), NULL, false )) != NULL )
				{
					mip = (mip_t *)fin;
					R_GetTexture( gl_texturenum )->srcWidth = mip->width;
					R_GetTexture( gl_texturenum )->srcHeight = mip->height;
					Mem_Free( fin ); // release low-quality decal
				}

				cl.decal_index[id] = gl_texturenum;
				load_external = true; // sucessfully loaded
			}
		}

		if( !load_external ) cl.decal_index[id] = GL_LoadTexture( (char *)host.draw_decals[id], NULL, 0, TF_DECAL, NULL );
	}
	host.decal_loading = false;

	return cl.decal_index[id];
}

/*
===============
CL_DecalRemoveAll

remove all decals with specified texture
===============
*/
void GAME_EXPORT CL_DecalRemoveAll( int textureIndex )
{
	int id = bound( 0, textureIndex, MAX_DECALS - 1 );	
	R_DecalRemoveAll( cl.decal_index[id] );
}

/*
==============================================================

EFRAGS MANAGEMENT

==============================================================
*/
efrag_t	cl_efrags[MAX_EFRAGS];

/*
==============
CL_ClearEfrags
==============
*/
void CL_ClearEfrags( void )
{
	int	i;

	Q_memset( cl_efrags, 0, sizeof( cl_efrags ));

	// allocate the efrags and chain together into a free list
	clgame.free_efrags = cl_efrags;
	for( i = 0; i < MAX_EFRAGS - 1; i++ )
		clgame.free_efrags[i].entnext = &clgame.free_efrags[i+1];
	clgame.free_efrags[i].entnext = NULL;
}
	
/*
==============
CL_ClearEffects
==============
*/
void CL_ClearEffects( void )
{
	CL_ClearEfrags ();
	CL_ClearDlights ();
	CL_ClearTempEnts ();
	CL_ClearViewBeams ();
	CL_ClearParticles ();
	CL_ClearLightStyles ();
}
#endif // XASH_DEDICATED
