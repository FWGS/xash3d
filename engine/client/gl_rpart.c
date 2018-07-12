/*
cl_part.c - particles and tracers
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
#include "r_efx.h"
#include "event_flags.h"
#include "entity_types.h"
#include "triangleapi.h"
#include "cl_tent.h"
#include "studio.h"

/*
==============================================================

PARTICLES MANAGEMENT

==============================================================
*/
#define SPARK_COLORCOUNT	9
#define TRACER_WIDTH	0.5f
#define SIMSHIFT		10

// particle velocities
static const float	cl_avertexnormals[NUMVERTEXNORMALS][3] =
{
#include "anorms.h"
};

// particle ramps
static int ramp1[8] = { 0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61 };
static int ramp2[8] = { 0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66 };
static int ramp3[6] = { 0x6d, 0x6b, 6, 5, 4, 3 };

static int boxpnt[6][4] =
{
{ 0, 4, 6, 2 }, // +X
{ 0, 1, 5, 4 }, // +Y
{ 0, 2, 3, 1 }, // +Z
{ 7, 5, 1, 3 }, // -X
{ 7, 3, 2, 6 }, // -Y
{ 7, 6, 4, 5 }, // -Z
};

static rgb_t gTracerColors[] =
{
{ 255, 255, 255 },		// White
{ 255, 0, 0 },		// Red
{ 0, 255, 0 },		// Green
{ 0, 0, 255 },		// Blue
{ 0, 0, 0 },		// Tracer default, filled in from cvars, etc.
{ 255, 167, 17 },		// Yellow-orange sparks
{ 255, 130, 90 },		// Yellowish streaks (garg)
{ 55, 60, 144 },		// Blue egon streak
{ 255, 130, 90 },		// More Yellowish streaks (garg)
{ 255, 140, 90 },		// More Yellowish streaks (garg)
{ 200, 130, 90 },		// More red streaks (garg)
{ 255, 120, 70 },		// Darker red streaks (garg)
};

static int gSparkRamp[SPARK_COLORCOUNT][3] =
{
{ 255, 255, 255 },
{ 255, 247, 199 },
{ 255, 243, 147 },
{ 255, 243, 27 },
{ 239, 203, 31 },
{ 223, 171, 39 },
{ 207, 143, 43 },
{ 127, 59, 43 },
{ 35, 19, 7 }
};

convar_t		*tracerred;
convar_t		*tracergreen;
convar_t		*tracerblue;
convar_t		*traceralpha;
convar_t		*tracerspeed;
convar_t		*tracerlength;
convar_t		*traceroffset;

particle_t	*cl_active_particles;
particle_t	*cl_free_particles;
particle_t	*cl_particles = NULL;	// particle pool
static vec3_t	cl_avelocities[NUMVERTEXNORMALS];
#define		COL_SUM( pal, clr )	(pal - clr) * (pal - clr)

/*
================
CL_LookupColor

find nearest color in particle palette
================
*/
short GAME_EXPORT CL_LookupColor( byte r, byte g, byte b )
{
	int	i, fi, best_color = 0;
	int	f_min = 1000000;
	byte	*pal;
#if 0
	// apply gamma-correction
	r = clgame.ds.gammaTable[r];
	g = clgame.ds.gammaTable[g];
	b = clgame.ds.gammaTable[b];
#endif
	for( i = 0; i < 256; i++ )
	{
		pal = clgame.palette[i];

		fi = 30 * COL_SUM( pal[0], r ) + 59 * COL_SUM( pal[1], g ) + 11 * COL_SUM( pal[2], b );
		if( fi < f_min )
		{
			best_color = i,
			f_min = fi;
		}
	}

	// make sure what color is in-range
	return (word)(best_color & 255);
}

/*
================
CL_GetPackedColor

in hardware mode does nothing
================
*/
void GAME_EXPORT CL_GetPackedColor( short *packed, short color )
{
	if( packed ) *packed = 0;
}

/*
================
CL_InitParticles

================
*/
void CL_InitParticles( void )
{
	int	i;

	cl_particles = Mem_Alloc( cls.mempool, sizeof( particle_t ) * GI->max_particles );
	CL_ClearParticles ();

	// this is used for EF_BRIGHTFIELD
	for( i = 0; i < NUMVERTEXNORMALS; i++ )
	{
		cl_avelocities[i][0] = Com_RandomLong( 0, 255 ) * 0.01f;
		cl_avelocities[i][1] = Com_RandomLong( 0, 255 ) * 0.01f;
		cl_avelocities[i][2] = Com_RandomLong( 0, 255 ) * 0.01f;
	}

	tracerred = Cvar_Get( "tracerred", "0.8", 0, "tracer red component weight ( 0 - 1.0 )" );
	tracergreen = Cvar_Get( "tracergreen", "0.8", 0, "tracer green component weight ( 0 - 1.0 )" );
	tracerblue = Cvar_Get( "tracerblue", "0.4", 0, "tracer blue component weight ( 0 - 1.0 )" );
	traceralpha = Cvar_Get( "traceralpha", "0.5", 0, "tracer alpha amount ( 0 - 1.0 )" );
	tracerspeed = Cvar_Get( "tracerspeed", "6000", 0, "tracer speed" );
	tracerlength = Cvar_Get( "tracerlength", "0.8", 0, "tracer length factor" );
	traceroffset = Cvar_Get( "traceroffset", "30", 0, "tracer starting offset" );
}

/*
================
CL_ClearParticles

================
*/
void CL_ClearParticles( void )
{
	int	i;

	if( !cl_particles ) return;

	cl_free_particles = cl_particles;
	cl_active_particles = NULL;

	for( i = 0; i < GI->max_particles - 1; i++ )
		cl_particles[i].next = &cl_particles[i+1];

	cl_particles[GI->max_particles-1].next = NULL;
}

/*
================
CL_FreeParticles

================
*/
void CL_FreeParticles( void )
{
	if( cl_particles )
		Mem_Free( cl_particles );
	cl_particles = NULL;
}

/*
================
CL_FreeParticle

move particle to freelist
================
*/
void CL_FreeParticle( particle_t *p )
{
	if( p->deathfunc )
	{
		// call right the deathfunc before die
		p->deathfunc( p );
	}

	p->next = cl_free_particles;
	cl_free_particles = p;
}

/*
================
CL_AllocParticle

can return NULL if particles is out
================
*/
particle_t *GAME_EXPORT CL_AllocParticle( void (*callback)( particle_t*, float ))
{
	particle_t	*p;

	if( !cl_draw_particles->integer )
		return NULL;

	// never alloc particles when we not in game
	if( !CL_IsInGame( )) return NULL;

	if( !cl_free_particles )
	{
		MsgDev( D_NOTE, "Overflow %d particles\n", GI->max_particles );
		return NULL;
	}

	p = cl_free_particles;
	cl_free_particles = p->next;
	p->next = cl_active_particles;
	cl_active_particles = p;

	// clear old particle
	p->type = pt_static;
	VectorClear( p->vel );
	VectorClear( p->org );
	p->die = cl.time;
	p->ramp = 0;

	if( callback )
	{
		p->type = pt_clientcustom;
		p->callback = callback;
	}

	return p;
}

static void CL_SparkTracerDraw( particle_t *p, float frametime )
{
	float	lifePerc = p->die - cl.time;
	float	grav = frametime * clgame.movevars.gravity * 0.05f;
	float	length, width;
	int	alpha = 255;
	vec3_t	delta;

	VectorScale( p->vel, p->ramp, delta );
	length = VectorLength( delta );
	width = ( length < TRACER_WIDTH ) ? length : TRACER_WIDTH;
	if( lifePerc < 0.5f ) alpha = (lifePerc * 2) * 255;

	CL_DrawTracer( p->org, delta, width, clgame.palette[p->color], alpha, 0.0f, 0.8f );

	p->vel[2] -= grav * 8; // use vox gravity
	VectorMA( p->org, frametime, p->vel, p->org );
}

static void CL_TracerImplosion( particle_t *p, float frametime )
{
	float	lifePerc = p->die - cl.time;
	float	length;
	int	alpha = 255;
	vec3_t	delta;

	VectorScale( p->vel, p->ramp, delta );
	length = VectorLength( delta );
	if( lifePerc < 0.5f ) alpha = (lifePerc * 2) * 255;

	CL_DrawTracer( p->org, delta, 1.5f, clgame.palette[p->color], alpha, 0.0f, 0.8f );

	VectorMA( p->org, frametime, p->vel, p->org );
}

static void CL_BulletTracerDraw( particle_t *p, float frametime )
{
	vec3_t	lineDir, viewDir, cross;
	vec3_t	vecEnd, vecStart, vecDir;
	float	sDistance, eDistance, totalDist;
	float	dDistance, dTotal, fOffset;
	int	alpha = (int)(traceralpha->value * 255);
	float	width = 3.0f, life, frac, length;
	vec3_t	tmp;

	// calculate distance
	VectorCopy( p->vel, vecDir );
	totalDist = VectorNormalizeLength( vecDir );

	length = p->ramp; // ramp used as length

	// calculate fraction
	life = ( totalDist + length ) / ( max( 1.0f, tracerspeed->value ));
	frac = life - ( p->die - cl.time ) + frametime;

	// calculate our distance along our path
	sDistance = tracerspeed->value * frac;
	eDistance = sDistance - length;
	
	// clip to start
	sDistance = max( 0.0f, sDistance );
	eDistance = max( 0.0f, eDistance );

	if(( sDistance == 0.0f ) && ( eDistance == 0.0f ))
		return;

	// clip it
	if( totalDist != 0.0f )
	{
		sDistance = min( sDistance, totalDist );
		eDistance = min( eDistance, totalDist );
	}

	// get our delta to calculate the tc offset
	dDistance	= fabs( sDistance - eDistance );
	dTotal = ( length != 0.0f ) ? length : 0.01f;
	fOffset = ( dDistance / dTotal );

	// find our points along our path
	VectorMA( p->org, sDistance, vecDir, vecEnd );
	VectorMA( p->org, eDistance, vecDir, vecStart );

	// setup our info for drawing the line
	VectorSubtract( vecEnd, vecStart, lineDir );
	VectorSubtract( vecEnd, RI.vieworg, viewDir );
	
	CrossProduct( lineDir, viewDir, cross );
	VectorNormalize( cross );

	GL_SetRenderMode( kRenderTransTexture );

	GL_Bind( XASH_TEXTURE0, cls.particleImage );
	pglBegin( GL_QUADS );

	pglColor4ub( clgame.palette[p->color][0], clgame.palette[p->color][1], clgame.palette[p->color][2], alpha );

	VectorMA( vecStart, -width, cross, tmp );	
	pglTexCoord2f( 1.0f, 0.0f );
	pglVertex3fv( tmp );

	VectorMA( vecStart, width, cross, tmp );
	pglTexCoord2f( 0.0f, 0.0f );
	pglVertex3fv( tmp );

	VectorMA( vecEnd, width, cross, tmp );
	pglTexCoord2f( 0.0f, fOffset );
	pglVertex3fv( tmp );

	VectorMA( vecEnd, -width, cross, tmp );
	pglTexCoord2f( 1.0f, fOffset );
	pglVertex3fv( tmp );

	pglEnd();
}

/*
================
CL_UpdateParticle

update particle color, position etc
================
*/
void CL_UpdateParticle( particle_t *p, float ft )
{
	float	time3 = 15.0 * ft;
	float	time2 = 10.0 * ft;
	float	time1 = 5.0 * ft;
	float	dvel = 4 * ft;
	float	grav = ft * clgame.movevars.gravity * 0.05f;
	float	size = 1.5f;
	int	i, iRamp, alpha = 255;
	vec3_t	right, up;
	rgb_t	color;

	r_stats.c_particle_count++;

	switch( p->type )
	{
	case pt_static:
		break;
	case pt_tracer:
	case pt_clientcustom:
		if( p->callback )
		{
			p->callback( p, ft );
		}
		if( p->type == pt_tracer )
			return; // already drawed
		break;
	case pt_fire:
		p->ramp += time1;
		if( p->ramp >= 6 ) p->die = -1;
		else p->color = ramp3[(int)p->ramp];
		p->vel[2] += grav;
		break;
	case pt_explode:
		p->ramp += time2;
		if( p->ramp >= 8 ) p->die = -1;
		else p->color = ramp1[(int)p->ramp];
		for( i = 0; i < 3; i++ )
			p->vel[i] += p->vel[i] * dvel;
		p->vel[2] -= grav;
		break;
	case pt_explode2:
		p->ramp += time3;
		if( p->ramp >= 8 ) p->die = -1;
		else p->color = ramp2[(int)p->ramp];
		for( i = 0; i < 3; i++ )
			p->vel[i] -= p->vel[i] * ft;
		p->vel[2] -= grav;
		break;
	case pt_blob:
	case pt_blob2:
		p->ramp += time2;
		iRamp = (int)p->ramp >> SIMSHIFT;

		if( iRamp >= SPARK_COLORCOUNT )
		{
			p->ramp = 0.0f;
			iRamp = 0;
		}
		
		p->color = CL_LookupColor( gSparkRamp[iRamp][0], gSparkRamp[iRamp][1], gSparkRamp[iRamp][2] );

		for( i = 0; i < 2; i++ )		
			p->vel[i] -= p->vel[i] * 0.5f * ft;
		p->vel[2] -= grav * 5.0f;

		if( Com_RandomLong( 0, 3 ))
		{
			p->type = pt_blob;
			alpha = 0;
		}
		else
		{
			p->type = pt_blob2;
			alpha = 255;
		}
		break;
	case pt_grav:
		p->vel[2] -= grav * 20;
		break;
	case pt_slowgrav:
		p->vel[2] -= grav;
		break;
	case pt_vox_grav:
		p->vel[2] -= grav * 8;
		break;
	case pt_vox_slowgrav:
		p->vel[2] -= grav * 4;
		break;
	}

#if 0
	// HACKHACK a scale up to keep particles from disappearing
	size += (p->org[0] - RI.vieworg[0]) * RI.vforward[0];
	size += (p->org[1] - RI.vieworg[1]) * RI.vforward[1];
	size += (p->org[2] - RI.vieworg[2]) * RI.vforward[2];

	if( size < 20.0f ) size = 1.0f;
	else size = 1.0f + size * 0.004f;
#endif
 	// scale the axes by radius
	VectorScale( RI.vright, size, right );
	VectorScale( RI.vup, size, up );

	p->color = bound( 0, p->color, 255 );
	VectorSet( color, clgame.palette[p->color][0], clgame.palette[p->color][1], clgame.palette[p->color][2] );

	GL_SetRenderMode( kRenderTransTexture );
	pglColor4ub( color[0], color[1], color[2], alpha );

	if( r_oldparticles->integer == 1 )
		GL_Bind( XASH_TEXTURE0, cls.oldParticleImage );
	else
		GL_Bind( XASH_TEXTURE0, cls.particleImage );

	// add the 4 corner vertices.
	pglBegin( GL_QUADS );

	pglTexCoord2f( 0.0f, 1.0f );
	pglVertex3f( p->org[0] - right[0] + up[0], p->org[1] - right[1] + up[1], p->org[2] - right[2] + up[2] );
	pglTexCoord2f( 0.0f, 0.0f );
	pglVertex3f( p->org[0] + right[0] + up[0], p->org[1] + right[1] + up[1], p->org[2] + right[2] + up[2] );
	pglTexCoord2f( 1.0f, 0.0f );
	pglVertex3f( p->org[0] + right[0] - up[0], p->org[1] + right[1] - up[1], p->org[2] + right[2] - up[2] );
	pglTexCoord2f( 1.0f, 1.0f );
	pglVertex3f( p->org[0] - right[0] - up[0], p->org[1] - right[1] - up[1], p->org[2] - right[2] - up[2] );

	pglEnd();

	if( p->type != pt_clientcustom )
	{
		// update position.
		VectorMA( p->org, ft, p->vel, p->org );
	}
}

void CL_DrawParticles( void )
{
	particle_t	*p, *kill;
	float		frametime;
	static int	framecount = -1;

	if( !cl_draw_particles->integer )
		return;

	// don't evaluate particles when executed many times
	// at same frame e.g. mirror rendering
	if( framecount != tr.realframecount )
	{
		frametime = cl.time - cl.oldtime;
		framecount = tr.realframecount;
	}
	else frametime = 0.0f;

	if( tracerred->modified || tracergreen->modified || tracerblue->modified )
	{
		gTracerColors[4][0] = (byte)(tracerred->value * 255);
		gTracerColors[4][1] = (byte)(tracergreen->value * 255);
		gTracerColors[4][2] = (byte)(tracerblue->value * 255);
		tracerred->modified = tracergreen->modified = tracerblue->modified = false;
	}

	while( 1 ) 
	{
		// free time-expired particles
		kill = cl_active_particles;
		if( kill && kill->die < cl.time )
		{
			cl_active_particles = kill->next;
			CL_FreeParticle( kill );
			continue;
		}
		break;
	}

	for( p = cl_active_particles; p; p = p->next )
	{
		while( 1 )
		{
			kill = p->next;
			if( kill && kill->die < cl.time )
			{
				p->next = kill->next;
				CL_FreeParticle( kill );
				continue;
			}
			break;
		}

		CL_UpdateParticle( p, frametime );
	}
}

void CL_DrawParticlesExternal( const float *vieworg, const float *forward, const float *right, const float *up, uint clipFlags )
{
	if( vieworg ) VectorCopy( vieworg, RI.vieworg );
	if( forward ) VectorCopy( forward, RI.vforward );
	if( right ) VectorCopy( right, RI.vright );
	if( up ) VectorCopy( up, RI.vup );

	RI.clipFlags = clipFlags;

	CL_DrawParticles ();
}

/*
===============
CL_EntityParticles

set EF_BRIGHTFIELD effect
===============
*/
void GAME_EXPORT CL_EntityParticles( cl_entity_t *ent )
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;	
	particle_t	*p;
	int		i;

	for( i = 0; i < NUMVERTEXNORMALS; i++ )
	{
		p = CL_AllocParticle( NULL );
		if( !p ) return;

#ifdef XASH_VECTORIZE_SINCOS
		SinCosFastVector3(
			cl.time * cl_avelocities[i][0],
			cl.time * cl_avelocities[i][1],
			cl.time * cl_avelocities[i][2],
			&sy, &sp, &sr,
			&cy, &cp, &cr);
#else
		angle = cl.time * cl_avelocities[i][0];
		SinCos( angle, &sy, &cy );
		angle = cl.time * cl_avelocities[i][1];
		SinCos( angle, &sp, &cp );
		angle = cl.time * cl_avelocities[i][2];
		SinCos( angle, &sr, &cr );
#endif
	
		VectorSet( forward, cp * cy, cp * sy, -sp ); 

		p->die += 0.01f;
		p->color = 111;		// yellow
		p->type = pt_explode;

		p->org[0] = ent->origin[0] + cl_avertexnormals[i][0] * 64.0f + forward[0] * 16.0f;
		p->org[1] = ent->origin[1] + cl_avertexnormals[i][1] * 64.0f + forward[1] * 16.0f;		
		p->org[2] = ent->origin[2] + cl_avertexnormals[i][2] * 64.0f + forward[2] * 16.0f;
	}
}

/*
===============
CL_ParticleExplosion

===============
*/
void GAME_EXPORT CL_ParticleExplosion( const vec3_t org )
{
	particle_t	*p;
	int		i, j;
	int		hSound;

	if( !org ) return;

	hSound = S_RegisterSound( "weapons/explode3.wav" );
	S_StartSound( org, 0, CHAN_AUTO, hSound, VOL_NORM, ATTN_NORM, PITCH_NORM, 0 );
	
	for( i = 0; i < 1024; i++ )
	{
		p = CL_AllocParticle( NULL );
		if( !p ) return;

		p->die += 5.0f;
		p->color = ramp1[0];
		p->ramp = rand() & 3;

		if( i & 1 )
		{
			p->type = pt_explode;
			for( j = 0; j < 3; j++ )
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
		else
		{
			p->type = pt_explode2;
			for( j = 0; j < 3; j++ )
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
	}
}

/*
===============
CL_ParticleExplosion2

===============
*/
void GAME_EXPORT CL_ParticleExplosion2( const vec3_t org, int colorStart, int colorLength )
{
	int		i, j;
	int		colorMod = 0;
	particle_t	*p;
	int		hSound;

	if( !org ) return;

	hSound = S_RegisterSound( "weapons/explode3.wav" );
	S_StartSound( org, 0, CHAN_AUTO, hSound, VOL_NORM, ATTN_NORM, PITCH_NORM, 0 );

	for( i = 0; i < 512; i++ )
	{
		p = CL_AllocParticle( NULL );
		if( !p ) return;

		p->die += 0.3f;
		p->color = colorStart + ( colorMod % colorLength );
		colorMod++;

		p->type = pt_blob;

		for( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 512) - 256;
		}
	}
}

/*
===============
CL_BlobExplosion

===============
*/
void GAME_EXPORT CL_BlobExplosion( const vec3_t org )
{
	particle_t	*p;
	int		i, j;
	int		hSound;

	if( !org ) return;

	hSound = S_RegisterSound( "weapons/explode3.wav" );
	S_StartSound( org, 0, CHAN_AUTO, hSound, VOL_NORM, ATTN_NORM, PITCH_NORM, 0 );
	
	for( i = 0; i < 1024; i++ )
	{
		p = CL_AllocParticle( NULL );
		if( !p ) return;

		p->die += 1.0f + (rand() & 8) * 0.05f;

		if( i & 1 )
		{
			p->type = pt_explode;
			p->color = 66 + rand() % 6;

			for( j = 0; j < 3; j++ )
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
		else
		{
			p->type = pt_explode2;
			p->color = 150 + rand() % 6;

			for( j = 0; j < 3; j++ )
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
	}
}

/*
===============
ParticleEffect

PARTICLE_EFFECT on server
===============
*/
void GAME_EXPORT CL_RunParticleEffect( const vec3_t org, const vec3_t dir, int color, int count )
{
	particle_t	*p;
	int		i, j;

	if( count == 1024 )
	{
		// Quake hack: count == 255 it's a RocketExplode
		CL_ParticleExplosion( org );
		return;
	}
	
	for( i = 0; i < count; i++ )
	{
		p = CL_AllocParticle( NULL );
		if( !p ) return;

		p->die += 0.1f * (rand() % 5);
		p->color = (color & ~7) + (rand() & 7);
		p->type = pt_slowgrav;

		for( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ((rand() & 15) - 8);
			p->vel[j] = dir[j] * 15;
		}
	}
}

/*
===============
CL_Blood

particle spray
===============
*/
void GAME_EXPORT CL_Blood( const vec3_t org, const vec3_t dir, int pcolor, int speed )
{
	particle_t	*p;
	int		i, j;

	for( i = 0; i < speed * 20; i++ )
	{
		p = CL_AllocParticle( NULL );
		if( !p ) return;

		p->die += Com_RandomFloat( 0.1f, 0.5f );
		p->type = pt_slowgrav;
		p->color = pcolor;

		for( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + Com_RandomFloat( -8.0f, 8.0f );
			p->vel[j] = dir[j] * speed;
		}
	}
}

/*
===============
CL_BloodStream

particle spray 2
===============
*/
void GAME_EXPORT CL_BloodStream( const vec3_t org, const vec3_t dir, int pcolor, int speed )
{
	particle_t *p;
	vec3_t dirCopy;
	float arc, num;
	int count, count2, speedCopy = speed, i;

	for( count = 0, arc = 0.05; count < 100; count++, arc -= 0.005 )
	{
		p = CL_AllocParticle( NULL );
		if( !p )
			return;

		p->die = cl.time + 2;
		p->color = pcolor + Com_RandomLong( 0, 9 );
		p->type = pt_vox_grav;
		VectorCopy( org, p->org );

		VectorCopy( dir, dirCopy );
		dirCopy[2] -= arc;
		VectorScale( dirCopy, speedCopy, p->vel );
		// speedCopy -= 0.00001;
	}

	for ( count = 0, arc = 0.075; count < ( speed / 5 ); count++ )
	{
		p = CL_AllocParticle( NULL );
		if ( !p )
			return;

		p->die = cl.time + 3;
		p->color = pcolor + Com_RandomLong( 0, 9 );
		p->type = pt_vox_slowgrav;

		VectorCopy( dir, dirCopy );
		VectorCopy( org, p->org );

		dirCopy[2] -= arc;
		arc -= 0.005;

		num = Com_RandomFloat( 0, 1 );
		speedCopy = speed * num;

		num *= 1.7;

		VectorScale( dirCopy, num, dirCopy );
		VectorScale( dirCopy, speedCopy, p->vel );

		for( count2 = 0; count2 < 2; count2++ )
		{
			p = CL_AllocParticle( NULL );
			if ( !p )
				return;

			p->die = cl.time + 3;
			p->color = pcolor + Com_RandomLong( 0, 9 );
			p->type = pt_vox_slowgrav;

			for( i = 0; i < 3; i++ )
				p->org[i] = org[i] + Com_RandomFloat( -1, 1 );

			VectorCopy( dir, dirCopy );

			dirCopy[2] -= arc;

			VectorScale( dirCopy, num, dirCopy );
			VectorScale( dirCopy, speedCopy, p->vel );
		}
	}
}

/*
===============
CL_LavaSplash

===============
*/
void GAME_EXPORT CL_LavaSplash( const vec3_t org )
{
	particle_t	*p;
	float		vel;
	vec3_t		dir;
	int		i, j, k;

	for( i = -16; i < 16; i++ )
	{
		for( j = -16; j <16; j++ )
		{
			for( k = 0; k < 1; k++ )
			{
				p = CL_AllocParticle( NULL );
				if( !p ) return;

				p->die += 2.0f + (rand() & 31) * 0.02f;
				p->color = 224 + (rand() & 7);
				p->type = pt_slowgrav;
				
				dir[0] = j * 8.0f + (rand() & 7);
				dir[1] = i * 8.0f + (rand() & 7);
				dir[2] = 256;

				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand() & 63);

				VectorNormalize( dir );
				vel = 50 + (rand() & 63);
				VectorScale( dir, vel, p->vel );
			}
		}
	}
}

/*
===============
CL_ParticleBurst

===============
*/
void GAME_EXPORT CL_ParticleBurst( const vec3_t org, int size, int color, float life )
{
	particle_t	*p;
	float		vel;
	vec3_t		dir;
	int		i, j, k;

	for( i = -size; i < size; i++ )
	{
		for( j = -size; j < size; j++ )
		{
			for( k = 0; k < 1; k++ )
			{
				p = CL_AllocParticle( NULL );
				if( !p ) return;

				p->die += life + (rand() & 31) * 0.02f;
				p->color = color;
				p->type = pt_slowgrav;
				
				dir[0] = j * 8.0f + (rand() & 7);
				dir[1] = i * 8.0f + (rand() & 7);
				dir[2] = 256;

				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand() & 63);

				VectorNormalize( dir );
				vel = 50 + (rand() & 63);
				VectorScale( dir, vel, p->vel );
			}
		}
	}
}

/*
===============
CL_TeleportSplash

===============
*/
void GAME_EXPORT CL_TeleportSplash( const vec3_t org )
{
	particle_t	*p;
	vec3_t		dir;
	int		i, j, k;

	for( i = -16; i < 16; i += 4 )
	{
		for( j = -16; j < 16; j += 4 )
		{
			for( k = -24; k < 32; k += 4 )
			{
				p = CL_AllocParticle( NULL );
				if( !p ) return;
		
				p->die += 0.2f + (rand() & 7) * 0.02f;
				p->color = 7 + (rand() & 7);
				p->type = pt_slowgrav;
				
				dir[0] = j * 8.0f;
				dir[1] = i * 8.0f;
				dir[2] = k * 8.0f;
	
				p->org[0] = org[0] + i + (rand() & 3);
				p->org[1] = org[1] + j + (rand() & 3);
				p->org[2] = org[2] + k + (rand() & 3);
	
				VectorNormalize( dir );
				VectorScale( dir, (50 + (rand() & 63)), p->vel );
			}
		}
	}
}

/*
===============
CL_RocketTrail

===============
*/
void GAME_EXPORT CL_RocketTrail( vec3_t start, vec3_t end, int type )
{
	vec3_t		vec;
	float		len;
	particle_t	*p;
	int		j, dec;
	static int	tracercount;

	VectorSubtract( end, start, vec );
	len = VectorNormalizeLength( vec );

	if( type < 128 )
	{
		dec = 3;
	}
	else
	{
		dec = 1;
		type -= 128;
	}

	while( len > 0 )
	{
		len -= dec;

		p = CL_AllocParticle( NULL );
		if( !p ) return;
		
		p->die += 2.0f;

		switch( type )
		{
		case 0:	// rocket trail
			p->ramp = (rand() & 3);
			p->color = ramp3[(int)p->ramp];
			p->type = pt_fire;
			for( j = 0; j < 3; j++ )
				p->org[j] = start[j] + ((rand() % 6 ) - 3 );
			break;
		case 1:	// smoke smoke
			p->ramp = (rand() & 3) + 2;
			p->color = ramp3[(int)p->ramp];
			p->type = pt_fire;
			for( j = 0; j < 3; j++ )
				p->org[j] = start[j] + ((rand() % 6 ) - 3 );
			break;
		case 2:	// blood
			p->type = pt_grav;
			p->color = 67 + (rand() & 3);
			for( j = 0; j < 3; j++ )
				p->org[j] = start[j] + ((rand() % 6 ) - 3 );
			break;
		case 3:
		case 5:	// tracer
			p->die += 0.5f;
			p->type = pt_static;

			if( type == 3 ) p->color = 52 + (( tracercount & 4 )<<1 );
			else p->color = 230 + (( tracercount & 4 )<<1 );

			tracercount++;
			VectorCopy( start, p->org );

			if( tracercount & 1 )
			{
				p->vel[0] = 30 *  vec[1];
				p->vel[1] = 30 * -vec[0];
			}
			else
			{
				p->vel[0] = 30 * -vec[1];
				p->vel[1] = 30 *  vec[0];
			}
			break;
		case 4:	// slight blood
			p->type = pt_grav;
			p->color = 67 + (rand() & 3);
			for( j = 0; j < 3; j++ )
				p->org[j] = start[j] + ((rand() % 6) - 3);
			len -= 3;
			break;
		case 6:	// voor trail
			p->color = 9 * 16 + 8 + (rand() & 3);
			p->type = pt_static;
			p->die += 0.3f;
			for( j = 0; j < 3; j++ )
				p->org[j] = start[j] + ((rand() & 15) - 8);
			break;
		}
		VectorAdd( start, vec, start );
	}
}

/*
================
CL_DrawLine

================
*/
static void CL_DrawLine( const vec3_t start, const vec3_t end, int pcolor, float life, float gap )
{
	particle_t	*p;
	float		len, curdist;
	vec3_t		diff;
	int		i;

	// Determine distance;
	VectorSubtract( end, start, diff );
	len = VectorNormalizeLength( diff );
	curdist = 0;

	while( curdist <= len )
	{
		p = CL_AllocParticle( NULL );
		if( !p ) return;

		for( i = 0; i < 3; i++ )
			p->org[i] = start[i] + curdist * diff[i];

		p->color = pcolor;
		p->type = pt_static;
		p->die += life;
		curdist += gap;
	}
}

/*
================
CL_DrawRectangle

================
*/
void CL_DrawRectangle( const vec3_t tl, const vec3_t bl, const vec3_t tr, const vec3_t br, int pcolor, float life )
{
	CL_DrawLine( tl, bl, pcolor, life, 2.0f );
	CL_DrawLine( bl, br, pcolor, life, 2.0f );
	CL_DrawLine( br, tr, pcolor, life, 2.0f );
	CL_DrawLine( tr, tl, pcolor, life, 2.0f );
}

void GAME_EXPORT CL_ParticleLine( const vec3_t start, const vec3_t end, byte r, byte g, byte b, float life )
{
	int	pcolor;

	pcolor = CL_LookupColor( r, g, b );
	CL_DrawLine( start, end, pcolor, life, 2.0f );
}

/*
================
CL_ParticleBox

================
*/
void GAME_EXPORT CL_ParticleBox( const vec3_t mins, const vec3_t maxs, byte r, byte g, byte b, float life )
{
	vec3_t	tmp, p[8];
	int	i, col;

	col = CL_LookupColor( r, g, b );

	for( i = 0; i < 8; i++ )
	{
		tmp[0] = (i & 1) ? mins[0] : maxs[0];
		tmp[1] = (i & 2) ? mins[1] : maxs[1];
		tmp[2] = (i & 4) ? mins[2] : maxs[2];
		VectorCopy( tmp, p[i] );
	}

	for( i = 0; i < 6; i++ )
	{
		CL_DrawRectangle( p[boxpnt[i][1]], p[boxpnt[i][0]], p[boxpnt[i][2]], p[boxpnt[i][3]], col, life );
	}

}

/*
================
CL_ShowLine

================
*/
void GAME_EXPORT CL_ShowLine( const vec3_t start, const vec3_t end )
{
	int	pcolor;

	pcolor = CL_LookupColor( 192, 0, 0 );
	CL_DrawLine( start, end, pcolor, 30.0f, 5.0f );
}

/*
===============
CL_BulletImpactParticles

===============
*/
void GAME_EXPORT CL_BulletImpactParticles( const vec3_t org )
{
	particle_t	*p;
	vec3_t		pos, dir;
	float		vel;
	int		i, j;

	// do sparks
	// randomize position
	pos[0] = org[0] + Com_RandomFloat( -2.0f, 2.0f );
	pos[1] = org[1] + Com_RandomFloat( -2.0f, 2.0f );
	pos[2] = org[2] + Com_RandomFloat( -2.0f, 2.0f );

	// create a 8 random spakle tracers
	for( i = 0; i < 8; i++ )
	{
		dir[0] = Com_RandomFloat( -1.0f, 1.0f );
		dir[1] = Com_RandomFloat( -1.0f, 1.0f );
		dir[2] = Com_RandomFloat( -1.0f, 1.0f );
		vel = Com_RandomFloat( SPARK_ELECTRIC_MINSPEED, SPARK_ELECTRIC_MAXSPEED );

		CL_SparkleTracer( pos, dir, vel );
	}

	if (r_oldparticles->integer == 1)
	{ 
		for (i = 0; i < 12; i++)
		{
			int greyColors;
			p = CL_AllocParticle(NULL);
			if (!p) return;

			p->die += 1.0f;
			// Randomly make each particle one of three colors: dark grey, medium grey or light grey.
			greyColors = (rand() % 3 + 1) * 32;
			p->color = CL_LookupColor(greyColors, greyColors, greyColors);

			p->type = pt_grav;
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + Com_RandomFloat(-2.0f, 3.0f);
				p->vel[j] = Com_RandomFloat(-70.0f, 70.0f);
			}
		}
	}
	else
	{
		for (i = 0; i < 12; i++)
		{
			p = CL_AllocParticle(NULL);
			if (!p) return;

			p->die += 1.0f;
			p->color = 0; // black

			p->type = pt_grav;
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + Com_RandomFloat(-2.0f, 3.0f);
				p->vel[j] = Com_RandomFloat(-70.0f, 70.0f);
			}
		}
	}
}

/*
===============
CL_FlickerParticles

===============
*/
void GAME_EXPORT CL_FlickerParticles( const vec3_t org )
{
	particle_t	*p;
	int		i, j;

	for( i = 0; i < 16; i++ )
	{
		p = CL_AllocParticle( NULL );
		if( !p ) return;

		p->die += Com_RandomFloat( 0.5f, 2.0f );
		p->type = pt_blob;

		for( j = 0; j < 3; j++ )
			p->org[j] = org[j] + Com_RandomFloat( -32.0f, 32.0f );
		p->vel[2] = Com_RandomFloat( 64.0f, 100.0f );
	}
}

/*
===============
CL_DebugParticle

just for debug purposes
===============
*/
void CL_DebugParticle( const vec3_t pos, byte r, byte g, byte b )
{
	particle_t	*p;

	p = CL_AllocParticle( NULL );
	if( !p ) return;

	VectorCopy( pos, p->org );
	p->die += 10.0f;
	p->color = CL_LookupColor( r, g, b );
}

/*
==============================================================

TRACERS MANAGEMENT (particle extension)

==============================================================
*/
/*
================
CL_CullTracer

check tracer bbox
================
*/
static qboolean CL_CullTracer( const vec3_t start, const vec3_t end )
{
	vec3_t	mins, maxs;
	int	i;

	// compute the bounding box
	for( i = 0; i < 3; i++ )
	{
		if( start[i] < end[i] )
		{
			mins[i] = start[i];
			maxs[i] = end[i];
		}
		else
		{
			mins[i] = end[i];
			maxs[i] = start[i];
		}
		
		// don't let it be zero sized
		if( mins[i] == maxs[i] )
		{
			maxs[i] += 1.0f;
		}
	}

	// check bbox
	return R_CullBox( mins, maxs, RI.clipFlags );
}

/*
================
CL_TracerComputeVerts

compute four vertexes in screen space
================
*/
qboolean CL_TracerComputeVerts( const vec3_t start, const vec3_t delta, float width, vec3_t *pVerts )
{
	vec3_t	end, screenStart, screenEnd;
	vec3_t	tmp, tmp2, normal;

	VectorAdd( start, delta, end );

	// clip the tracer
	if( CL_CullTracer( start, end ))
		return false;

	// transform point into the screen space
	TriWorldToScreen( (float *)start, screenStart );
	TriWorldToScreen( (float *)end, screenEnd );

	VectorSubtract( screenStart, screenEnd, tmp );	
	tmp[2] = 0; // we don't need Z, we're in screen space

	VectorNormalize( tmp );

	// build point along noraml line (normal is -y, x)
	VectorScale( RI.vup, tmp[0], normal );
	VectorScale( RI.vright, -tmp[1], tmp2 );
	VectorSubtract( normal, tmp2, normal );

	// compute four vertexes
	VectorScale( normal, width, tmp );
	VectorSubtract( start, tmp, pVerts[0] ); 
	VectorAdd( start, tmp, pVerts[1] ); 
	VectorAdd( pVerts[0], delta, pVerts[2] ); 
	VectorAdd( pVerts[1], delta, pVerts[3] ); 

	return true;
}


/*
================
CL_DrawTracer

draws a single tracer
================
*/
void CL_DrawTracer( vec3_t start, vec3_t delta, float width, rgb_t color, int alpha, float startV, float endV )
{
	// Clip the tracer
	vec3_t	verts[4];

	if ( !CL_TracerComputeVerts( start, delta, width, verts ))
		return;

	// NOTE: Gotta get the winding right so it's not backface culled
	// (we need to turn of backface culling for these bad boys)

	GL_SetRenderMode( kRenderTransTexture );

	pglColor4ub( color[0], color[1], color[2], alpha );

	GL_Bind( XASH_TEXTURE0, cls.particleImage );
	pglBegin( GL_QUADS );

	pglTexCoord2f( 0.0f, endV );
	pglVertex3fv( verts[2] );

	pglTexCoord2f( 1.0f, endV );
	pglVertex3fv( verts[3] );

	pglTexCoord2f( 1.0f, startV );
	pglVertex3fv( verts[1] );

	pglTexCoord2f( 0.0f, startV );
	pglVertex3fv( verts[0] );

	pglEnd();
}

/*
===============
CL_SparkleTracer

===============
*/
void CL_SparkleTracer( const vec3_t pos, const vec3_t dir, float vel )
{
	particle_t	*p;
	byte		*color;

	p = CL_AllocParticle( CL_SparkTracerDraw );
	if( !p ) return;

	color = gTracerColors[5];	// Yellow-Orange
	VectorCopy( pos, p->org );
	p->die += Com_RandomFloat( 0.45f, 0.7f );
	p->color = CL_LookupColor( color[0], color[1], color[2] );
	p->ramp = Com_RandomFloat( 0.07f, 0.08f );	// ramp used as tracer length
	p->type = pt_tracer;
	VectorScale( dir, vel, p->vel );
}

/*
===============
CL_StreakTracer

===============
*/
void CL_StreakTracer( const vec3_t pos, const vec3_t velocity, int colorIndex )
{
	particle_t	*p;
	byte		*color;

	p = CL_AllocParticle( CL_SparkTracerDraw );
	if( !p ) return;

	if( colorIndex > ( sizeof( gTracerColors ) / sizeof( gTracerColors[0] )))
	{
		p->color = bound( 0, colorIndex, 255 );
	}
	else
	{ 
		color = gTracerColors[colorIndex];
		p->color = CL_LookupColor( color[0], color[1], color[2] );
	}

	p->die += Com_RandomFloat( 0.5f, 1.0f );
	VectorCopy( velocity, p->vel );
	VectorCopy( pos, p->org );
	p->ramp = Com_RandomFloat( 0.05f, 0.08f );
	p->type = pt_tracer;
}

/*
===============
CL_TracerEffect

===============
*/
void GAME_EXPORT CL_TracerEffect( const vec3_t start, const vec3_t end )
{
	particle_t	*p;
	byte		*color;
	vec3_t		dir;
	float		life, dist;

	p = CL_AllocParticle( CL_BulletTracerDraw );
	if( !p ) return;

	// get out shot direction and length
	VectorSubtract( end, start, dir );
	VectorCopy( dir, p->vel );

	dist = VectorNormalizeLength( dir );

	// don't make small tracers
	if( dist <= traceroffset->value )
		return;

	p->ramp = Com_RandomFloat( 200.0f, 256.0f ) * tracerlength->value;

	color = gTracerColors[4];
	life = ( dist + p->ramp ) / ( max( 1.0f, tracerspeed->value ));
	p->color = CL_LookupColor( color[0], color[1], color[2] );
	VectorCopy( start, p->org );
	p->type = pt_tracer;
	p->die += life;
}

/*
===============
CL_UserTracerParticle

===============
*/
void GAME_EXPORT CL_UserTracerParticle( float *org, float *vel, float life, int colorIndex, float length, byte deathcontext,
	void (*deathfunc)( particle_t *p ))
{
	particle_t	*p;
	byte		*color;

	p = CL_AllocParticle( CL_BulletTracerDraw );
	if( !p ) return;

	if( colorIndex > ( sizeof( gTracerColors ) / sizeof( gTracerColors[0] )))
	{
		p->color = bound( 0, colorIndex, 255 );
	}
	else
	{ 
		color = gTracerColors[colorIndex];
		p->color = CL_LookupColor( color[0], color[1], color[2] );
	}

	VectorCopy( org, p->org );
	VectorCopy( vel, p->vel );
	p->ramp = length; // ramp used as length
	p->context = deathcontext;
	p->deathfunc = deathfunc;
	p->type = pt_tracer;
	p->die += life;
}

/*
===============
CL_TracerParticles

allow more customization
===============
*/
particle_t *GAME_EXPORT CL_TracerParticles( float *org, float *vel, float life )
{
	particle_t	*p;
	byte		*color;

	p = CL_AllocParticle( CL_BulletTracerDraw );
	if( !p ) return NULL;

	p->ramp = Com_RandomFloat( 200.0f, 256.0f ) * tracerlength->value;

	color = gTracerColors[4];
	p->color = CL_LookupColor( color[0], color[1], color[2] );
	VectorCopy( org, p->org );
	VectorCopy( vel, p->vel );
	p->type = pt_tracer;
	p->die += life;

	return p;
}

/*
===============
CL_SparkShower

Creates 8 random tracers
===============
*/
void GAME_EXPORT CL_SparkShower( const vec3_t org )
{
	vec3_t	pos, dir;
	model_t	*pmodel;
	float	vel;
	int	i;

	// randomize position
	pos[0] = org[0] + Com_RandomFloat( -2.0f, 2.0f );
	pos[1] = org[1] + Com_RandomFloat( -2.0f, 2.0f );
	pos[2] = org[2] + Com_RandomFloat( -2.0f, 2.0f );

	pmodel = Mod_Handle( CL_FindModelIndex( "sprites/richo1.spr" ));
	CL_RicochetSprite( pos, pmodel, 0.0f, Com_RandomFloat( 0.4, 0.6f ));

	// create a 8 random spakle tracers
	for( i = 0; i < 8; i++ )
	{
		dir[0] = Com_RandomFloat( -1.0f, 1.0f );
		dir[1] = Com_RandomFloat( -1.0f, 1.0f );
		dir[2] = Com_RandomFloat( -1.0f, 1.0f );
		vel = Com_RandomFloat( SPARK_ELECTRIC_MINSPEED, SPARK_ELECTRIC_MAXSPEED );

		CL_SparkleTracer( pos, dir, vel );
	}
}

/*
===============
CL_Implosion

===============
*/
void GAME_EXPORT CL_Implosion( const vec3_t end, float radius, int count, float life )
{
	particle_t	*p;
	float		vel, radius2;
	vec3_t		dir, m_vecPos;
	int		i, colorIndex;

	colorIndex = CL_LookupColor( gTracerColors[5][0], gTracerColors[5][1], gTracerColors[5][2] );

	for( i = 0; i < count; i++ )
	{
		p = CL_AllocParticle( CL_TracerImplosion );
		if( !p ) return;

		dir[0] = Com_RandomFloat( -1.0f, 1.0f );
		dir[1] = Com_RandomFloat( -1.0f, 1.0f );
		dir[2] = Com_RandomFloat( -1.0f, 1.0f );

		radius2 = Com_RandomFloat( radius * 0.9f, radius * 1.1f );

		VectorNormalize( dir );
		VectorMA( end, -radius2, dir, m_vecPos );

		// velocity based on how far particle has to travel away from org
		if( life ) vel = (radius2 / life);
		else vel = Com_RandomFloat( radius2 * 0.5f, radius2 * 1.5f );

		VectorCopy( m_vecPos, p->org );
		p->color = colorIndex;
		p->ramp = (vel / radius2) * 0.1f; // length based on velocity
		p->ramp = bound( 0.1f, p->ramp, 1.0f );
		p->type = pt_tracer;
		VectorScale( dir, vel, p->vel );

		// die right when you get there
		p->die += ( life != 0.0f ) ? life : ( radius2 / vel );
	}
}

/*
===============
CL_ReadPointFile_f

===============
*/
void CL_ReadPointFile_f( void )
{
	char		*afile, *pfile;
	vec3_t		org;
	int		count;
	particle_t	*p;
	char		filename[64];
	string		token;
	
	Q_snprintf( filename, sizeof( filename ), "maps/%s.pts", clgame.mapname );
	afile = (char *)FS_LoadFile( filename, NULL, false );

	if( !afile )
	{
		MsgDev( D_ERROR, "couldn't open %s\n", filename );
		return;
	}
	
	Msg( "Reading %s...\n", filename );

	count = 0;
	pfile = afile;

	while( 1 )
	{
		pfile = COM_ParseFile( pfile, token );
		if( !pfile ) break;
		org[0] = Q_atof( token );

		pfile = COM_ParseFile( pfile, token );
		if( !pfile ) break;
		org[1] = Q_atof( token );

		pfile = COM_ParseFile( pfile, token );
		if( !pfile ) break;
		org[2] = Q_atof( token );

		count++;
		
		if( !cl_free_particles )
		{
			MsgDev( D_ERROR, "CL_ReadPointFile: not enough free particles!\n" );
			break;
		}

		// NOTE: can't use CL_AllocateParticles because running from the console
		p = cl_free_particles;
		cl_free_particles = p->next;
		p->next = cl_active_particles;
		cl_active_particles = p;

		p->ramp = 0;		
		p->die = 99999;
		p->color = (-count) & 15;
		p->type = pt_static;
		VectorClear( p->vel );
		VectorCopy( org, p->org );
	}

	Mem_Free( afile );

	if( count ) Msg( "%i points read\n", count );
	else Msg( "map %s has no leaks!\n", clgame.mapname );
}
#endif // XASH_DEDICATED
