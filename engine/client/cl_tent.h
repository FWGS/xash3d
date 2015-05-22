/*
cl_tent.h - efx api set
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

#ifndef CL_TENT_H
#define CL_TENT_H

#define SPARK_ELECTRIC_MINSPEED	64.0f
#define SPARK_ELECTRIC_MAXSPEED	100.0f

// EfxAPI
void CL_DrawTracer( vec3_t start, vec3_t delta, float width, rgb_t color, int alpha, float startV, float endV );
struct particle_s *CL_AllocParticle( void (*callback)( struct particle_s*, float ));
void CL_Explosion( vec3_t pos, int model, float scale, float framerate, int flags );
void CL_ParticleExplosion( const vec3_t org );
void CL_ParticleExplosion2( const vec3_t org, int colorStart, int colorLength );
void CL_Implosion( const vec3_t end, float radius, int count, float life );
void CL_Blood( const vec3_t org, const vec3_t dir, int pcolor, int speed );
void CL_BloodStream( const vec3_t org, const vec3_t dir, int pcolor, int speed );
void CL_BlobExplosion( const vec3_t org );
void CL_EntityParticles( cl_entity_t *ent );
void CL_FlickerParticles( const vec3_t org );
void CL_RunParticleEffect( const vec3_t org, const vec3_t dir, int color, int count );
void CL_ParticleBurst( const vec3_t org, int size, int color, float life );
void CL_LavaSplash( const vec3_t org );
void CL_TeleportSplash( const vec3_t org );
void CL_RocketTrail( vec3_t start, vec3_t end, int type );
short CL_LookupColor( byte r, byte g, byte b );
void CL_GetPackedColor( short *packed, short color );
void CL_SparkleTracer( const vec3_t pos, const vec3_t dir, float vel );
void CL_StreakTracer( const vec3_t pos, const vec3_t velocity, int colorIndex );
void CL_TracerEffect( const vec3_t start, const vec3_t end );
void CL_UserTracerParticle( float *org, float *vel, float life, int colorIndex, float length, byte deathcontext, void (*deathfunc)( struct particle_s* ));
struct particle_s *CL_TracerParticles( float *org, float *vel, float life );
void CL_ParticleLine( const vec3_t start, const vec3_t end, byte r, byte g, byte b, float life );
void CL_ParticleBox( const vec3_t mins, const vec3_t maxs, byte r, byte g, byte b, float life );
void CL_ShowLine( const vec3_t start, const vec3_t end );
void CL_BulletImpactParticles( const vec3_t pos );
void CL_SparkShower( const vec3_t org );
struct tempent_s *CL_TempEntAlloc( const vec3_t org, model_t *pmodel );
struct tempent_s *CL_TempEntAllocHigh( const vec3_t org, model_t *pmodel );
struct tempent_s *CL_TempEntAllocNoModel( const vec3_t org );
struct tempent_s *CL_TempEntAllocCustom( const vec3_t org, model_t *model, int high, void (*callback)( struct tempent_s*, float, float ));
void CL_FizzEffect( cl_entity_t *pent, int modelIndex, int density );
void CL_Bubbles( const vec3_t mins, const vec3_t maxs, float height, int modelIndex, int count, float speed );
void CL_BubbleTrail( const vec3_t start, const vec3_t end, float flWaterZ, int modelIndex, int count, float speed );
void CL_AttachTentToPlayer( int client, int modelIndex, float zoffset, float life );
void CL_KillAttachedTents( int client );
void CL_RicochetSprite( const vec3_t pos, model_t *pmodel, float duration, float scale );
void CL_RocketFlare( const vec3_t pos );
void CL_MuzzleFlash( const vec3_t pos, int type );
void CL_BloodSprite( const vec3_t org, int colorIndex, int modelIndex, int modelIndex2, float size );
void CL_BreakModel( const vec3_t pos, const vec3_t size, const vec3_t dir, float random, float life, int count, int modelIndex, char flags );
struct tempent_s *CL_TempModel( const vec3_t pos, const vec3_t dir, const vec3_t angles, float life, int modelIndex, int soundtype );
struct tempent_s *CL_TempSprite( const vec3_t pos, const vec3_t dir, float scale, int modelIndex, int rendermode, int renderfx, float a, float life, int flags );
struct tempent_s *CL_DefaultSprite( const vec3_t pos, int spriteIndex, float framerate );
void CL_Sprite_Explode( struct tempent_s *pTemp, float scale, int flags );
void CL_Sprite_Smoke( struct tempent_s *pTemp, float scale );
void CL_Spray( const vec3_t pos, const vec3_t dir, int modelIndex, int count, int speed, int iRand, int renderMode );
void CL_Sprite_Spray( const vec3_t pos, const vec3_t dir, int modelIndex, int count, int speed, int iRand );
void CL_Sprite_Trail( int type, const vec3_t vecStart, const vec3_t vecEnd, int modelIndex, int nCount, float flLife, float flSize, float flAmplitude, int nRenderamt, float flSpeed );
void CL_FunnelSprite( const vec3_t pos, int spriteIndex, int flags );
void CL_Large_Funnel( const vec3_t pos, int flags );
void CL_SparkEffect( const vec3_t pos, int count, int velocityMin, int velocityMax );
void CL_StreakSplash( const vec3_t pos, const vec3_t dir, int color, int count, float speed, int velMin, int velMax );
void CL_SparkStreaks( const vec3_t pos, int count, int velocityMin, int velocityMax );
void CL_Projectile( const vec3_t origin, const vec3_t velocity, int modelIndex, int life, int owner, void (*hitcallback)( struct tempent_s*, struct pmtrace_s* ));
void CL_TempSphereModel( const vec3_t pos, float speed, float life, int count, int modelIndex );
void CL_MultiGunshot( const vec3_t org, const vec3_t dir, const vec3_t noise, int count, int decalCount, int *decalIndices );
void CL_FireField( float *org, int radius, int modelIndex, int count, int flags, float life );
void CL_PlayerSprites( int client, int modelIndex, int count, int size );
void CL_Sprite_WallPuff( struct tempent_s *pTemp, float scale );
void CL_DebugParticle( const vec3_t pos, byte r, byte g, byte b );
void CL_RicochetSound( const vec3_t pos );
struct dlight_s *CL_AllocDlight( int key );
struct dlight_s *CL_AllocElight( int key );
void CL_UpdateFlashlight( cl_entity_t *pEnt );
void CL_DecalShoot( int textureIndex, int entityIndex, int modelIndex, float *pos, int flags );
void CL_DecalRemoveAll( int textureIndex );
int CL_DecalIndexFromName( const char *name );
int CL_DecalIndex( int id );

// Beams
struct beam_s *CL_BeamLightning( const vec3_t start, const vec3_t end, int modelIndex, float life, float width, float amplitude, float brightness, float speed );
struct beam_s *CL_BeamEnts( int startEnt, int endEnt, int modelIndex, float life, float width, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b );
struct beam_s *CL_BeamPoints( const vec3_t start, const vec3_t end, int modelIndex, float life, float width, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b );
struct beam_s *CL_BeamCirclePoints( int type, const vec3_t start, const vec3_t end, int modelIndex, float life, float width, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b );
struct beam_s *CL_BeamEntPoint( int startEnt, const vec3_t end, int modelIndex, float life, float width, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b );
struct beam_s *CL_BeamRing( int startEnt, int endEnt, int modelIndex, float life, float width, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b );
struct beam_s *CL_BeamFollow( int startEnt, int modelIndex, float life, float width, float r, float g, float b, float brightness );
void CL_BeamKill( int deadEntity );


// TriAPI
void TriVertex3fv( const float *v );
void TriNormal3fv( const float *v );
void TriColor4f( float r, float g, float b, float a );
int TriSpriteTexture( model_t *pSpriteModel, int frame );
int TriWorldToScreen( float *world, float *screen );

#endif//CL_TENT_H