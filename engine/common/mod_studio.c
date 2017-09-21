/*
sv_studio.c - server studio utilities
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
#include "server.h"
#include "studio.h"
#include "r_studioint.h"
#include "library.h"

typedef int (*STUDIOAPI)( int, sv_blending_interface_t**, server_studio_api_t*,  float (*transform)[3][4], float (*bones)[MAXSTUDIOBONES][3][4] );

typedef struct mstudiocache_s
{
	float	frame;
	int	sequence;
	vec3_t	angles;
	vec3_t	origin;
	vec3_t	size;
	byte	controler[4];
	byte	blending[2];
	model_t	*model;
	uint	current_hull;
	uint	current_plane;
	uint	numhitboxes;
} mstudiocache_t;

#define STUDIO_CACHESIZE		16
#define STUDIO_CACHEMASK		(STUDIO_CACHESIZE - 1)

// trace global variables
static sv_blending_interface_t	*pBlendAPI = NULL;
static studiohdr_t			*mod_studiohdr;
static matrix3x4			studio_transform;
static hull_t			cache_hull[MAXSTUDIOBONES];
static hull_t			studio_hull[MAXSTUDIOBONES];
static matrix3x4			studio_bones[MAXSTUDIOBONES];
static uint			studio_hull_hitgroup[MAXSTUDIOBONES];
static uint			cache_hull_hitgroup[MAXSTUDIOBONES];
static mstudiocache_t		cache_studio[STUDIO_CACHESIZE];
static dclipnode_t			studio_clipnodes[6];
static mplane_t			studio_planes[768];
static mplane_t			cache_planes[768];

// current cache state
static int			cache_current;
static int			cache_current_hull;
static int			cache_current_plane;

/*
====================
Mod_InitStudioHull
====================
*/
void Mod_InitStudioHull( void )
{
	int	i, side;

	if( studio_hull[0].planes != NULL )
		return;	// already initailized

	for( i = 0; i < 6; i++ )
	{
		studio_clipnodes[i].planenum = i;

		side = i & 1;

		studio_clipnodes[i].children[side] = CONTENTS_EMPTY;
		if( i != 5 ) studio_clipnodes[i].children[side^1] = i + 1;
		else studio_clipnodes[i].children[side^1] = CONTENTS_SOLID;
	}

	for( i = 0; i < MAXSTUDIOBONES; i++ )
	{
		studio_hull[i].clipnodes = studio_clipnodes;
		studio_hull[i].planes = &studio_planes[i*6];
		studio_hull[i].firstclipnode = 0;
		studio_hull[i].lastclipnode = 5;
	}
}

/*
===============================================================================

	STUDIO MODELS CACHE

===============================================================================
*/
/*
====================
ClearStudioCache
====================
*/
void Mod_ClearStudioCache( void )
{
	Q_memset( cache_studio, 0, sizeof( cache_studio ));
	cache_current_hull = cache_current_plane = 0;

	cache_current = 0;
}

/*
====================
AddToStudioCache
====================
*/
void Mod_AddToStudioCache( float frame, int sequence, vec3_t angles, vec3_t origin, vec3_t size, byte *pcontroller, byte *pblending, model_t *model, hull_t *hull, int numhitboxes )
{
	mstudiocache_t *pCache;

	if( numhitboxes + cache_current_hull >= MAXSTUDIOBONES )
		Mod_ClearStudioCache();

	cache_current++;
	pCache = &cache_studio[cache_current & STUDIO_CACHEMASK];

	pCache->frame = frame;
	pCache->sequence = sequence;
	VectorCopy( angles, pCache->angles );
	VectorCopy( origin, pCache->origin );
	VectorCopy( size, pCache->size );

	Q_memcpy( pCache->controler, pcontroller, 4 );
	Q_memcpy( pCache->blending, pblending, 2 );

	pCache->model = model;
	pCache->current_hull = cache_current_hull;
	pCache->current_plane = cache_current_plane;

	Q_memcpy( &cache_hull[cache_current_hull], hull, numhitboxes * sizeof( hull_t ));
	Q_memcpy( &cache_planes[cache_current_plane], studio_planes, numhitboxes * sizeof( mplane_t ) * 6 );
	Q_memcpy( &cache_hull_hitgroup[cache_current_hull], studio_hull_hitgroup, numhitboxes * sizeof( uint ));

	cache_current_hull += numhitboxes;
	cache_current_plane += numhitboxes * 6;
	pCache->numhitboxes = numhitboxes;
}

/*
====================
CheckStudioCache
====================
*/
mstudiocache_t *Mod_CheckStudioCache( model_t *model, float frame, int sequence, vec3_t angles, vec3_t origin, vec3_t size, byte *pcontroller, byte *pblending )
{
	mstudiocache_t	*pCache;
	int		i;

	for( i = 0; i < STUDIO_CACHESIZE; i++ )
	{
		pCache = &cache_studio[(cache_current - i) & STUDIO_CACHEMASK];

		if( pCache->model == model && pCache->frame == frame && pCache->sequence == sequence &&
		VectorCompare( angles, pCache->angles ) && VectorCompare( origin, pCache->origin ) && VectorCompare( size, pCache->size ) &&
		!Q_memcmp( pCache->controler, pcontroller, 4 ) && !Q_memcmp( pCache->blending, pblending, 2 ))
		{
			return pCache;
		}
	}
	return NULL;
}

/*
===============================================================================

	STUDIO MODELS TRACING

===============================================================================
*/
/*
====================
SetStudioHullPlane
====================
*/
void Mod_SetStudioHullPlane( mplane_t *pl, int bone, int axis, float offset )
{
	pl->type = 5;

	pl->normal[0] = studio_bones[bone][0][axis];
	pl->normal[1] = studio_bones[bone][1][axis];
	pl->normal[2] = studio_bones[bone][2][axis];

	pl->dist = (pl->normal[0] * studio_bones[bone][0][3]) + (pl->normal[1] * studio_bones[bone][1][3]) + (pl->normal[2] * studio_bones[bone][2][3]) + offset;
}

/*
====================
HullForStudio

NOTE: pEdict may be NULL
====================
*/
hull_t *Mod_HullForStudio( model_t *model, float frame, int sequence, vec3_t angles, vec3_t origin, vec3_t size, byte *pcontroller, byte *pblending, int *numhitboxes, edict_t *pEdict )
{
	vec3_t		angles2;
	mstudiocache_t	*bonecache;
	mstudiobbox_t	*phitbox;
	int		i, j;
	qboolean bSkipShield = 0;

	ASSERT( numhitboxes );

	*numhitboxes = 0; // assume error

	if((sv_skipshield->integer == 1 && pEdict && pEdict->v.gamestate == 1) || sv_skipshield->integer == 2)
		bSkipShield = 1;

	if( mod_studiocache->integer )
	{
		bonecache = Mod_CheckStudioCache( model, frame, sequence, angles, origin, size, pcontroller, pblending );

		if( bonecache != NULL )
		{
			Q_memcpy( studio_planes, &cache_planes[bonecache->current_plane], bonecache->numhitboxes * sizeof( mplane_t ) * 6 );
			Q_memcpy( studio_hull_hitgroup, &cache_hull_hitgroup[bonecache->current_hull], bonecache->numhitboxes * sizeof( uint ));
			Q_memcpy( studio_hull, &cache_hull[bonecache->current_hull], bonecache->numhitboxes * sizeof( hull_t ));

			*numhitboxes = bonecache->numhitboxes;
			return studio_hull;
		}
	}

	mod_studiohdr = Mod_Extradata( model );
	if( !mod_studiohdr ) return NULL; // probably not a studiomodel

	ASSERT( pBlendAPI != NULL );

	VectorCopy( angles, angles2 );

	if( !( host.features & ENGINE_COMPENSATE_QUAKE_BUG ))
		angles2[PITCH] = -angles2[PITCH]; // stupid quake bug

	pBlendAPI->SV_StudioSetupBones( model, frame, sequence, angles2, origin, pcontroller, pblending, -1, pEdict );
	phitbox = (mstudiobbox_t *)((byte *)mod_studiohdr + mod_studiohdr->hitboxindex);

	for( i = j = 0; i < mod_studiohdr->numhitboxes; i++, j += 6 )
	{
		studio_hull_hitgroup[i] = phitbox[i].group;

		Mod_SetStudioHullPlane( &studio_planes[j+0], phitbox[i].bone, 0, phitbox[i].bbmax[0] );
		Mod_SetStudioHullPlane( &studio_planes[j+1], phitbox[i].bone, 0, phitbox[i].bbmin[0] );
		Mod_SetStudioHullPlane( &studio_planes[j+2], phitbox[i].bone, 1, phitbox[i].bbmax[1] );
		Mod_SetStudioHullPlane( &studio_planes[j+3], phitbox[i].bone, 1, phitbox[i].bbmin[1] );
		Mod_SetStudioHullPlane( &studio_planes[j+4], phitbox[i].bone, 2, phitbox[i].bbmax[2] );
		Mod_SetStudioHullPlane( &studio_planes[j+5], phitbox[i].bone, 2, phitbox[i].bbmin[2] );

		studio_planes[j+0].dist += DotProductFabs( studio_planes[j+0].normal, size );
		studio_planes[j+1].dist -= DotProductFabs( studio_planes[j+1].normal, size );
		studio_planes[j+2].dist += DotProductFabs( studio_planes[j+2].normal, size );
		studio_planes[j+3].dist -= DotProductFabs( studio_planes[j+3].normal, size );
		studio_planes[j+4].dist += DotProductFabs( studio_planes[j+4].normal, size );
		studio_planes[j+5].dist -= DotProductFabs( studio_planes[j+5].normal, size );
	}

	// tell trace code about hitbox count
	*numhitboxes = (bSkipShield == true) ? mod_studiohdr->numhitboxes - 1 : mod_studiohdr->numhitboxes;

	if( mod_studiocache->integer )
	{
		Mod_AddToStudioCache( frame, sequence, angles, origin, size, pcontroller, pblending, model, studio_hull, *numhitboxes );
	}

	return studio_hull;
}

/*
===============================================================================

	STUDIO MODELS SETUP BONES

===============================================================================
*/
/*
====================
StudioCalcBoneAdj

====================
*/
static void Mod_StudioCalcBoneAdj( float *adj, const byte *pcontroller )
{
	int			i, j;
	float			value = 0.0f;
	mstudiobonecontroller_t	*pbonecontroller;
	
	pbonecontroller = (mstudiobonecontroller_t *)((byte *)mod_studiohdr + mod_studiohdr->bonecontrollerindex);

	for( j = 0; j < mod_studiohdr->numbonecontrollers; j++ )
	{
		i = pbonecontroller[j].index;

		if( i == STUDIO_MOUTH )
			continue; // ignore mouth

		if( i <= MAXSTUDIOCONTROLLERS )
		{
			// check for 360% wrapping
			if( pbonecontroller[j].type & STUDIO_RLOOP )
			{
				value = pcontroller[i] * (360.0f / 256.0f) + pbonecontroller[j].start;
			}
			else 
			{
				value = pcontroller[i] / 255.0f;
				value = bound( 0.0f, value, 1.0f );
				value = (1.0f - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			}
		}

		switch( pbonecontroller[j].type & STUDIO_TYPES )
		{
		case STUDIO_XR:
		case STUDIO_YR:
		case STUDIO_ZR:
			adj[j] = value * (M_PI / 180.0f);
			break;
		case STUDIO_X:
		case STUDIO_Y:
		case STUDIO_Z:
			adj[j] = value;
			break;
		}
	}
}

/*
====================
StudioCalcBoneQuaterion

====================
*/
static void Mod_StudioCalcBoneQuaterion( int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *q )
{
	int		j, k;
	vec4_t		q1, q2;
	vec3_t		angle1, angle2;
	mstudioanimvalue_t	*panimvalue;

	for( j = 0; j < 3; j++ )
	{
		if( panim->offset[j+3] == 0 )
		{
			angle2[j] = angle1[j] = pbone->value[j+3]; // default;
		}
		else
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j+3]);
			k = frame;
			
			// debug
			if( panimvalue->num.total < panimvalue->num.valid )
				k = 0;
			
			while( panimvalue->num.total <= k )
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
				// DEBUG
				if( panimvalue->num.total < panimvalue->num.valid )
					k = 0;
			}
			// Bah, missing blend!
			if( panimvalue->num.valid > k )
			{
				angle1[j] = panimvalue[k+1].value;

				if( panimvalue->num.valid > k + 1 )
				{
					angle2[j] = panimvalue[k+2].value;
				}
				else
				{
					if( panimvalue->num.total > k + 1 )
						angle2[j] = angle1[j];
					else angle2[j] = panimvalue[panimvalue->num.valid+2].value;
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;
				if( panimvalue->num.total > k + 1 )
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			angle1[j] = pbone->value[j+3] + angle1[j] * pbone->scale[j+3];
			angle2[j] = pbone->value[j+3] + angle2[j] * pbone->scale[j+3];
		}

		if( pbone->bonecontroller[j+3] != -1 )
		{
			angle1[j] += adj[pbone->bonecontroller[j+3]];
			angle2[j] += adj[pbone->bonecontroller[j+3]];
		}
	}

	if( !VectorCompare( angle1, angle2 ))
	{
		AngleQuaternion( angle1, q1 );
		AngleQuaternion( angle2, q2 );
		QuaternionSlerp( q1, q2, s, q );
	}
	else
	{
		AngleQuaternion( angle1, q );
	}
}

/*
====================
StudioCalcBonePosition

====================
*/
static void Mod_StudioCalcBonePosition( int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *pos )
{
	int		j, k;
	mstudioanimvalue_t	*panimvalue;

	for( j = 0; j < 3; j++ )
	{
		pos[j] = pbone->value[j]; // default;
		if( panim->offset[j] != 0.0f )
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j]);
			
			k = frame;

			// debug
			if( panimvalue->num.total < panimvalue->num.valid )
				k = 0;

			// find span of values that includes the frame we want
			while( panimvalue->num.total <= k )
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;

  				// DEBUG
				if( panimvalue->num.total < panimvalue->num.valid )
					k = 0;
			}

			// if we're inside the span
			if( panimvalue->num.valid > k )
			{
				// and there's more data in the span
				if( panimvalue->num.valid > k + 1 )
				{
					pos[j] += (panimvalue[k+1].value * (1.0f - s) + s * panimvalue[k+2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[k+1].value * pbone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if( panimvalue->num.total <= k + 1 )
				{
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0f - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
				}
			}
		}

		if( pbone->bonecontroller[j] != -1 && adj )
		{
			pos[j] += adj[pbone->bonecontroller[j]];
		}
	}
}

/*
====================
StudioCalcRotations

====================
*/
static void Mod_StudioCalcRotations( int boneused[], int numbones, const byte *pcontroller, float pos[][3], vec4_t *q, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f )
{
	int		i, j, frame;
	mstudiobone_t	*pbone;
	float		adj[MAXSTUDIOCONTROLLERS];
	float		s;

	Q_memset( adj, 0, MAXSTUDIOCONTROLLERS * sizeof( float ) );

	if( f > pseqdesc->numframes - 1 )
		f = 0.0f;
	else if( f < -0.01f )
		f = -0.01f;

	frame = (int)f;
	s = (f - frame);

	// add in programtic controllers
	pbone = (mstudiobone_t *)((byte *)mod_studiohdr + mod_studiohdr->boneindex);

	Mod_StudioCalcBoneAdj( adj, pcontroller );

	for( j = numbones - 1; j >= 0; j-- )
	{
		i = boneused[j];
		Mod_StudioCalcBoneQuaterion( frame, s, &pbone[i], &panim[i], adj, q[i] );
		Mod_StudioCalcBonePosition( frame, s, &pbone[i], &panim[i], adj, pos[i] );
	}

	if( pseqdesc->motiontype & STUDIO_X ) pos[pseqdesc->motionbone][0] = 0.0f;
	if( pseqdesc->motiontype & STUDIO_Y ) pos[pseqdesc->motionbone][1] = 0.0f;
	if( pseqdesc->motiontype & STUDIO_Z ) pos[pseqdesc->motionbone][2] = 0.0f;
}

/*
====================
StudioEstimateFrame

====================
*/
static float Mod_StudioEstimateFrame( float frame, mstudioseqdesc_t *pseqdesc )
{
	double	f;
	
	if( pseqdesc->numframes <= 1 )
		f = 0.0f;
	else f = ( frame * ( pseqdesc->numframes - 1 )) / 256.0f;
 
	if( pseqdesc->flags & STUDIO_LOOPING ) 
	{
		if( pseqdesc->numframes > 1 )
			f -= (int)(f / (pseqdesc->numframes - 1)) *  (pseqdesc->numframes - 1);
		if( f < 0.0f ) f += (pseqdesc->numframes - 1);
	}
	else 
	{
		if( f >= pseqdesc->numframes - 1.001f )
			f = pseqdesc->numframes - 1.001f;
		if( f < 0.0f )  f = 0.0f;
	}

	return f;
}

/*

====================
StudioSlerpBones

====================
*/
static void Mod_StudioSlerpBones( vec4_t q1[], float pos1[][3], vec4_t q2[], float pos2[][3], float s )
{
	int	i;
	vec4_t	q3;
	float	s1;

	s = bound( 0.0f, s, 1.0f );
	s1 = 1.0f - s;

	for( i = 0; i < mod_studiohdr->numbones; i++ )
	{
		QuaternionSlerp( q1[i], q2[i], s, q3 );
		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
	}
}


#ifdef XASH_BIG_ENDIAN

// swap all fields in ibuffer starting given field (4byte type)
#define SWAP_INTS(type, startfield) \
for( i = offsetof(type, startfield)/4; i < sizeof(type)/4; i++ ) \
	LittleLongSW(ibuffer[i])

// swap all fields in sbuffer starting given field (2byte type)
#define SWAP_SHORTS(type, startfield) \
for( i = offsetof(type, startfield)/2; i < sizeof(type)/2; i++ ) \
	LittleShortSW(sbuffer[i])

/*
=========================
Mod_StudioBigEndian

Swap all model fields for big endian system
=========================
*/
void Mod_StudioBigEndian( model_t *model, byte *buffer )
{
	studiohdr_t	*phdr = buffer;
	int *ibuffer = buffer;
	short *sbuffer;
	int i,j,k, bodyCount = 0;
	mstudiomodel_t *pSubModel;

	if( LittleLong(phdr->ident) != IDSTUDIOHEADER )
		return;

	LittleLongSW(phdr->ident);
	LittleLongSW(phdr->length);

	SWAP_INTS(studiohdr_t,eyeposition);

	for( j = 0; j < phdr->numbones; j++ )
	{
		ibuffer = (byte*)phdr + phdr->boneindex + sizeof(mstudiobone_t) * j;
		SWAP_INTS(mstudiobone_t,parent);
	}

	for( j = 0; j < phdr->numbonecontrollers; j++ )
	{
		ibuffer = (byte*)phdr + phdr->bonecontrollerindex + sizeof(mstudiobonecontroller_t) * j;
		SWAP_INTS(mstudiobonecontroller_t,bone);
	}

	for( j = 0; j < phdr->numhitboxes; j++ )
	{
		ibuffer = (byte*)phdr + phdr->hitboxindex + sizeof(mstudiobbox_t) * j;
		SWAP_INTS(mstudiobbox_t,bone);
	}

	for( j = 0; j < phdr->numtextures; j++ )
	{
		ibuffer = (byte*)phdr + phdr->textureindex + sizeof(mstudiotexture_t) * j;
		SWAP_INTS(mstudiotexture_t,flags);
	}


	for( j = 0; j < phdr->numseq; j++ )
	{
		mstudioseqdesc_t *pseqdesc = (byte*)phdr + phdr->seqindex + sizeof(mstudioseqdesc_t) * j;
		mstudioanim_t *panim;//, *panimend;
		int m;

		ibuffer = (byte*)phdr + phdr->seqindex + sizeof(mstudioseqdesc_t) * j;
		SWAP_INTS(mstudioseqdesc_t,fps);

		for( k = 0; k < pseqdesc->numpivots; k++ )
		{
			ibuffer = (byte*)phdr + pseqdesc->pivotindex +  sizeof(mstudiopivot_t) * j;
			SWAP_INTS(mstudiopivot_t,org);
		}

		// preload and swap external seqgroups
		if( pseqdesc->seqgroup )
		{
			mstudioseqgroup_t	*pseqgroup;
			fs_offset_t		filesize;
			byte		*buf;
			cache_user_t	*paSequences;

			if( !model )
				continue; // not loading model

			pseqgroup = (mstudioseqgroup_t *)((byte *)phdr + phdr->seqgroupindex) + pseqdesc->seqgroup;

			paSequences = (cache_user_t *)model->submodels;
			if( paSequences == NULL )
			{
				paSequences = (cache_user_t *)Mem_Alloc( com_studiocache, MAXSTUDIOGROUPS * sizeof( cache_user_t ));
				model->submodels = (void *)paSequences;
			}

			// check for already loaded
			if( !paSequences[pseqdesc->seqgroup].data )
			{
				string	filepath, modelname, modelpath;

				FS_FileBase( model->name, modelname );
				FS_ExtractFilePath( model->name, modelpath );

				// NOTE: here we build real sub-animation filename because stupid user may rename model without recompile
				Q_snprintf( filepath, sizeof( filepath ), "%s/%s%i%i.mdl", modelpath, modelname, pseqdesc->seqgroup / 10, pseqdesc->seqgroup % 10 );

				buf = FS_LoadFile( filepath, &filesize, false );
				if( !buf || !filesize )
					Host_Error( "StudioGetAnim: can't load %s\n", filepath );
				else if( IDSEQGRPHEADER != LittleLong(*(uint *)buf ))
					Host_Error( "StudioGetAnim: %s is corrupted\n", filepath );

				MsgDev( D_INFO, "loading: %s\n", filepath );

				paSequences[pseqdesc->seqgroup].data = Mem_Alloc( com_studiocache, filesize );
				Q_memcpy( paSequences[pseqdesc->seqgroup].data, buf, (size_t)filesize );
				Mem_Free( buf );
			}

			panim = (mstudioanim_t *)((byte *)paSequences[pseqdesc->seqgroup].data + pseqdesc->animindex);
			//panimend = (mstudioanim_t *)((byte *)paSequences[pseqdesc->seqgroup].data + filesize);
		}
		else
		{
			panim = (byte*)phdr + pseqdesc->animindex;
			//panimend = pseqdesc;
		}

		// swap animations
		for( m = 0; m < phdr->numbones * pseqdesc->numblends; m++ )
		{
			/*mstudioanimvalue_t *panim1 = panimend;
			if( m < phdr->numbones * pseqdesc->numblends - 1 )
				panim1 = (byte*)(panim + 1) + LittleShort((panim+1)->offset[0]);
			if( panim1 > panimend )
				panim1 = panimend;*/
			sbuffer = panim;
			SWAP_SHORTS(mstudioanim_t,offset);
			for( k = 0; k < 6; k++ )
			{
				if(panim->offset[k])
				{
					int frame;
					byte swapped[512] = {0};

					// swap animvalues only single time
#define TRYSWAP(x) if(!swapped[&x - panimvalue2]) /* && (&x - panimvalue2) < panim1 ) */ \
				{ \
					LittleShortSW(x.value); \
					swapped[&x - panimvalue2] = true; \
				}
					mstudioanimvalue_t *panimvalue2 = (((byte*)panim) + panim->offset[k]);
					mstudioanimvalue_t *panimvalue;

					// run all frames to process all animvalues
					for( frame = 0; frame < pseqdesc->numframes - 1 || frame == 0; frame++ )
					{
						int f = frame;
						panimvalue = panimvalue2;

						// debug
						if( panimvalue->num.total < panimvalue->num.valid )
							f = 0;

						// find span of values that includes the frame we want
						while( panimvalue->num.total <= f )
						{
							f -= panimvalue->num.total;
							panimvalue += panimvalue->num.valid + 1;

							// debug
							if( panimvalue->num.total < panimvalue->num.valid )
								f = 0;
						}

						// if we're inside the span
						if( panimvalue->num.valid > f )
						{
							// and there's more data in the span
							if( panimvalue->num.valid > f + 1 )
							{
								TRYSWAP(panimvalue[f+1]);
								TRYSWAP(panimvalue[f+2]);
							}

							else
								TRYSWAP(panimvalue[f+1]);
						}
						else
						{
							// are we at the end of the repeating values section and there's another section with data?
							if( panimvalue->num.total <= f + 1 )
							{
								TRYSWAP(panimvalue[panimvalue->num.valid]);
								TRYSWAP(panimvalue[panimvalue->num.valid+2]);


							}
							else TRYSWAP(panimvalue[panimvalue->num.valid]);
						}
					}
#undef TRYSWAP
				}
			}

			panim++;
		}

	}

	for( j = 0; j < phdr->numseqgroups; j++ )
	{
		ibuffer = (byte*)phdr + phdr->seqgroupindex + sizeof(mstudioseqgroup_t) * j;
		SWAP_INTS(mstudioseqgroup_t,data);
	}

	for( j = 0; j < phdr->numbodyparts; j++ )
	{
		int index;

		mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)((byte *)phdr + phdr->bodypartindex) + j;
		ibuffer = pbodypart;
		SWAP_INTS(mstudiobodyparts_t,nummodels);
		bodyCount += pbodypart->nummodels;

		for( k = 0; k < pbodypart->nummodels; k++ )
		{
			int *vindex;
			int m;

			pSubModel = (mstudiomodel_t *)((byte *)phdr + pbodypart->modelindex) + k;
			ibuffer = pSubModel;
			SWAP_INTS(mstudiomodel_t, type);

			vindex = (float *)((byte *)phdr + pSubModel->vertindex);

			for( m = 0; m < pSubModel->numverts * 3; m++ )
				LittleLongSW(vindex[m]);

			vindex = (float *)((byte *)phdr + pSubModel->normindex);

			for( m = 0; m < pSubModel->numnorms * 3; m++ )
				LittleLongSW(vindex[m]);

			// swap tri commands
			for( m = 0; m < pSubModel->nummesh; m++ )
			{
				mstudiomesh_t *pmesh = ibuffer = ((byte *)phdr + pSubModel->meshindex + sizeof(mstudiomesh_t) * m);
				SWAP_INTS(mstudiomesh_t,numtris);

				sbuffer =  (short *)((byte *)phdr + pmesh->triindex);
				while( i = LittleShort(*sbuffer) )
				{
					LittleShortSW(*sbuffer);
					sbuffer++;
					i = abs(i) * 4;
					for( ; i > 0; i--, sbuffer++ )
						LittleShortSW(*sbuffer);
				}
			}
		}
	}

	for( j = 0; j < phdr->numattachments; j++ )
	{
		ibuffer = (byte*)phdr + phdr->attachmentindex + sizeof(mstudioattachment_t) * j;
		SWAP_INTS(mstudioattachment_t,type);
	}
}
#endif

/*
====================
StudioGetAnim

====================
*/
static mstudioanim_t *Mod_StudioGetAnim( model_t *m_pSubModel, mstudioseqdesc_t *pseqdesc )
{
	mstudioseqgroup_t	*pseqgroup;
	fs_offset_t		filesize;
	byte		*buf;
	cache_user_t *paSequences;

	pseqgroup = (mstudioseqgroup_t *)((byte *)mod_studiohdr + mod_studiohdr->seqgroupindex) + pseqdesc->seqgroup;
	if( pseqdesc->seqgroup == 0 )
		return (mstudioanim_t *)((byte *)mod_studiohdr + pseqdesc->animindex);

	paSequences = (cache_user_t *)m_pSubModel->submodels;

	if( paSequences == NULL )
	{
		paSequences = (cache_user_t *)Mem_Alloc( com_studiocache, MAXSTUDIOGROUPS * sizeof( cache_user_t ));
		m_pSubModel->submodels = (void *)paSequences;
	}

	// check for already loaded
	if( !paSequences[pseqdesc->seqgroup].data )
	{
		string	filepath, modelname, modelpath;

		FS_FileBase( m_pSubModel->name, modelname );
		FS_ExtractFilePath( m_pSubModel->name, modelpath );
		Q_snprintf( filepath, sizeof( filepath ), "%s/%s%i%i.mdl", modelpath, modelname, pseqdesc->seqgroup / 10, pseqdesc->seqgroup % 10 );

		buf = FS_LoadFile( filepath, &filesize, false );
		if( !buf || !filesize )
			Host_Error( "StudioGetAnim: can't load %s\n", filepath );
		else if( IDSEQGRPHEADER != LittleLong(*(uint *)buf) )
			Host_Error( "StudioGetAnim: %s is corrupted\n", filepath );

		MsgDev( D_INFO, "loading: %s\n", filepath );

		paSequences[pseqdesc->seqgroup].data = Mem_Alloc( com_studiocache, filesize );
		Q_memcpy( paSequences[pseqdesc->seqgroup].data, buf, (size_t)filesize );
		Mem_Free( buf );
	}
	return (mstudioanim_t *)((byte *)paSequences[pseqdesc->seqgroup].data + pseqdesc->animindex);
}

/*
====================
StudioSetupBones

NOTE: pEdict is unused
====================
*/
static void SV_StudioSetupBones( model_t *pModel,	float frame, int sequence, const vec3_t angles, const vec3_t origin,
	const byte *pcontroller, const byte *pblending, int iBone, const edict_t *pEdict )
{
	int		i, j, numbones = 0;
	int		boneused[MAXSTUDIOBONES];
	double		f;

	mstudiobone_t	*pbones;
	mstudioseqdesc_t	*pseqdesc;
	mstudioanim_t	*panim;

	static float	pos[MAXSTUDIOBONES][3];
	static vec4_t	q[MAXSTUDIOBONES];
	matrix3x4		bonematrix;

	static float	pos2[MAXSTUDIOBONES][3];
	static vec4_t	q2[MAXSTUDIOBONES];
	static float	pos3[MAXSTUDIOBONES][3];
	static vec4_t	q3[MAXSTUDIOBONES];
	static float	pos4[MAXSTUDIOBONES][3];
	static vec4_t	q4[MAXSTUDIOBONES];

	if( sequence < 0 || sequence >= mod_studiohdr->numseq )
	{
		MsgDev( D_WARN, "SV_StudioSetupBones: sequence %i/%i out of range for model %s\n", sequence, mod_studiohdr->numseq, mod_studiohdr->name );
		sequence = 0;
	}

	pseqdesc = (mstudioseqdesc_t *)((byte *)mod_studiohdr + mod_studiohdr->seqindex) + sequence;
	pbones = (mstudiobone_t *)((byte *)mod_studiohdr + mod_studiohdr->boneindex);
	panim = Mod_StudioGetAnim( pModel, pseqdesc );

	if( iBone < -1 || iBone >= mod_studiohdr->numbones )
		iBone = 0;

	if( iBone == -1 )
	{
		numbones = mod_studiohdr->numbones;
		for( i = 0; i < mod_studiohdr->numbones; i++ )
			boneused[(numbones - i) - 1] = i;
	}
	else
	{
		// only the parent bones
		for( i = iBone; i != -1; i = pbones[i].parent )
			boneused[numbones++] = i;
	}

	f = Mod_StudioEstimateFrame( frame, pseqdesc );
	Mod_StudioCalcRotations( boneused, numbones, pcontroller, pos, q, pseqdesc, panim, f );

	if( pseqdesc->numblends > 1 )
	{
		float	s;

		panim += mod_studiohdr->numbones;
		Mod_StudioCalcRotations( boneused, numbones, pcontroller, pos2, q2, pseqdesc, panim, f );

		s = (float)pblending[0] / 255.0f;

		Mod_StudioSlerpBones( q, pos, q2, pos2, s );

		if( pseqdesc->numblends == 4 )
		{
			panim += mod_studiohdr->numbones;
			Mod_StudioCalcRotations( boneused, numbones, pcontroller, pos3, q3, pseqdesc, panim, f );

			panim += mod_studiohdr->numbones;
			Mod_StudioCalcRotations( boneused, numbones, pcontroller, pos4, q4, pseqdesc, panim, f );

			s = (float)pblending[0] / 255.0f;
			Mod_StudioSlerpBones( q3, pos3, q4, pos4, s );

			s = (float)pblending[1] / 255.0f;
			Mod_StudioSlerpBones( q, pos, q3, pos3, s );
		}
	}

	Matrix3x4_CreateFromEntity( studio_transform, angles, origin, 1.0f );

	for( j = numbones - 1; j >= 0; j-- )
	{
		i = boneused[j];

		Matrix3x4_FromOriginQuat( bonematrix, q[i], pos[i] );
		if( pbones[i].parent == -1 ) 
			Matrix3x4_ConcatTransforms( studio_bones[i], studio_transform, bonematrix );
		else Matrix3x4_ConcatTransforms( studio_bones[i], studio_bones[pbones[i].parent], bonematrix );
	}
}

/*
====================
StudioGetAttachment
====================
*/
void Mod_StudioGetAttachment( const edict_t *e, int iAttachment, float *origin, float *angles )
{
	mstudioattachment_t		*pAtt;
	vec3_t			angles2;
	model_t			*mod;

	mod = Mod_Handle( e->v.modelindex );
	mod_studiohdr = (studiohdr_t *)Mod_Extradata( mod );
	if( !mod_studiohdr ) return;

	if( mod_studiohdr->numattachments <= 0 )
		return;

	ASSERT( pBlendAPI != NULL );

	if( mod_studiohdr->numattachments > MAXSTUDIOATTACHMENTS )
	{
		mod_studiohdr->numattachments = MAXSTUDIOATTACHMENTS; // reduce it
		MsgDev( D_WARN, "SV_StudioGetAttahment: too many attachments on %s\n", mod_studiohdr->name );
	}

	iAttachment = bound( 0, iAttachment, mod_studiohdr->numattachments );

	// calculate attachment origin and angles
	pAtt = (mstudioattachment_t *)((byte *)mod_studiohdr + mod_studiohdr->attachmentindex);

	VectorCopy( e->v.angles, angles2 );

	if( !( host.features & ENGINE_COMPENSATE_QUAKE_BUG ))
		angles2[PITCH] = -angles2[PITCH];

	pBlendAPI->SV_StudioSetupBones( mod, e->v.frame, e->v.sequence, angles2, e->v.origin,
		e->v.controller, e->v.blending, pAtt[iAttachment].bone, e );

	// compute pos and angles
	if( origin != NULL )
		Matrix3x4_VectorTransform( studio_bones[pAtt[iAttachment].bone], pAtt[iAttachment].org, origin );

	if( sv_allow_studio_attachment_angles->integer && origin != NULL && angles != NULL )
	{
		vec3_t	forward, bonepos;

		Matrix3x4_OriginFromMatrix( studio_bones[pAtt[iAttachment].bone], bonepos );
		VectorSubtract( origin, bonepos, forward ); // make forward
		VectorNormalizeFast( forward );
		VectorAngles( forward, angles );
	}
}

/*
====================
GetBonePosition
====================
*/
void Mod_GetBonePosition( const edict_t *e, int iBone, float *origin, float *angles )
{
	model_t	*mod;

	mod = Mod_Handle( e->v.modelindex );
	mod_studiohdr = (studiohdr_t *)Mod_Extradata( mod );
	if( !mod_studiohdr ) return;

	ASSERT( pBlendAPI != NULL );

	pBlendAPI->SV_StudioSetupBones( mod, e->v.frame, e->v.sequence, e->v.angles, e->v.origin,
		e->v.controller, e->v.blending, iBone, e );

	if( origin ) Matrix3x4_OriginFromMatrix( studio_bones[iBone], origin );
	if( angles ) VectorAngles( studio_bones[iBone][0], angles ); // bone forward to angles
}

/*
====================
HitgroupForStudioHull
====================
*/
int Mod_HitgroupForStudioHull( int index )
{
	return studio_hull_hitgroup[index];
}

/*
====================
StudioBoundVertex
====================
*/
void Mod_StudioBoundVertex( vec3_t out_mins, vec3_t out_maxs, int *counter, const vec3_t vertex )
{
	if( *counter == 0 )
	{
		// init bounds
		VectorCopy( vertex, out_mins );
		VectorCopy( vertex, out_maxs );
	}
	else
	{
		AddPointToBounds( vertex, out_mins, out_maxs );
	}

	(*counter)++;
}

/*
====================
StudioAccumulateBoneVerts
====================
*/
void Mod_StudioAccumulateBoneVerts( vec3_t mins1, vec3_t maxs1, int *counter1, vec3_t mins2, vec3_t maxs2, int *counter2 )
{
	vec3_t	midpoint;

	if( *counter2 <= 0 )
		return;

	// calculate the midpoint of the second vertex,
	VectorSubtract( maxs2, mins2, midpoint );
	VectorScale( midpoint, 0.5f, midpoint );

	Mod_StudioBoundVertex( mins1, maxs1, counter1, midpoint );

	// negate the midpoint, for whatever reason, and bind it again
	VectorNegate( midpoint, midpoint );
	Mod_StudioBoundVertex( mins1, maxs1, counter1, midpoint );

	VectorClear( mins2 );
	VectorClear( maxs2 );
	*counter2 = 0;
}

/*
====================
StudioComputeBounds
====================
*/
void Mod_StudioComputeBounds( void *buffer, vec3_t mins, vec3_t maxs )
{
	int		i, j, k;
	studiohdr_t	*pstudiohdr;
	mstudiobodyparts_t	*pbodypart;
	mstudiomodel_t	*m_pSubModel;
	mstudioseqdesc_t	*pseqdesc;
	mstudiobone_t	*pbones;
	vec3_t		vecmins1, vecmaxs1;
	vec3_t		vecmins2, vecmaxs2;
	int		counter1, counter2;
	int		bodyCount = 0;
	vec3_t		pos;

	counter1 = counter2 = 0;
	VectorClear( vecmins1 );
	VectorClear( vecmaxs1 );
	VectorClear( vecmins2 );
	VectorClear( vecmaxs2 );

	// Get the body part portion of the model
	pstudiohdr = (studiohdr_t *)buffer;
#ifdef XASH_BIG_ENDIAN
	Mod_StudioBigEndian( NULL, buffer );
#endif
	pbodypart = (mstudiobodyparts_t *)((byte *)pstudiohdr + pstudiohdr->bodypartindex);

	// each body part has nummodels variations so there are as many total variations as there
	// are in a matrix of each part by each other part
	for( i = 0; i < pstudiohdr->numbodyparts; i++ )
		bodyCount += pbodypart[i].nummodels;

	// The studio models we want are rvec3_t mins, vec3_t maxsight after the bodyparts (still need to
	// find a detailed breakdown of the mdl format).  Move pointer there.
	m_pSubModel = (mstudiomodel_t *)(&pbodypart[pstudiohdr->numbodyparts]);

	for( i = 0; i < bodyCount; i++ )
	{
		float *vertIndex = (float *)((byte *)pstudiohdr + m_pSubModel[i].vertindex);

		for( j = 0; j < m_pSubModel[i].numverts; j++ )
			Mod_StudioBoundVertex( vecmins1, vecmaxs1, &counter1, vertIndex + (3 * j));
	}

	pbones = (mstudiobone_t *)((byte *)pstudiohdr + pstudiohdr->boneindex);
	pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex);

	for( i = 0; i < pstudiohdr->numseq; i++ )
	{
		mstudioanim_t *panim = (mstudioanim_t *) (((byte *)buffer) + pseqdesc[i].animindex);

		for( j = 0; j < pstudiohdr->numbones; j++ )
		{
			for( k = 0; k < pseqdesc[i].numframes; k++ )
			{
				Mod_StudioCalcBonePosition( k, 0, &pbones[j], panim, NULL, pos );
				Mod_StudioBoundVertex( vecmins2, vecmaxs2, &counter2, pos );
			}
		}
		Mod_StudioAccumulateBoneVerts( vecmins1, vecmaxs1, &counter1, vecmins2, vecmaxs2, &counter2 );
	}

	VectorCopy( vecmins1, mins );
	VectorCopy( vecmaxs1, maxs );
}

/*
====================
Mod_GetStudioBounds
====================
*/
qboolean Mod_GetStudioBounds( const char *name, vec3_t mins, vec3_t maxs )
{
	int	result = false;
	byte	*f;

	if( !Q_strstr( name, "models" ) || !Q_strstr( name, ".mdl" ))
		return false;

	f = FS_LoadFile( name, NULL, false );
	if( !f ) return false;

	if( LittleLong(*(uint *)f) == IDSTUDIOHEADER )
	{
		VectorClear( mins );
		VectorClear( maxs );
		Mod_StudioComputeBounds( f, mins, maxs );
		result = true;
	}
	Mem_Free( f );

	return result;
}

static sv_blending_interface_t gBlendAPI =
{
	SV_BLENDING_INTERFACE_VERSION,
	SV_StudioSetupBones,
};

static server_studio_api_t gStudioAPI =
{
	Mod_Calloc,
	Mod_CacheCheck,
	(void*)Mod_LoadCacheFile,
	Mod_Extradata,
};
   
/*
===============
Mod_InitStudioAPI

Initialize server studio (blending interface)
===============
*/
void Mod_InitStudioAPI( void )
{
	static STUDIOAPI	pBlendIface;

	pBlendAPI = &gBlendAPI;
	pBlendIface = (STUDIOAPI)Com_GetProcAddress( svgame.hInstance, "Server_GetBlendingInterface" );
	if( pBlendIface && pBlendIface( SV_BLENDING_INTERFACE_VERSION, &pBlendAPI, &gStudioAPI, &studio_transform, &studio_bones ))
	{
		MsgDev( D_AICONSOLE, "SV_LoadProgs: ^2initailized Server Blending interface ^7ver. %i\n", SV_BLENDING_INTERFACE_VERSION );
		return;
	}

	// just restore pointer to builtin function
	pBlendAPI = &gBlendAPI;
}

/*
===============
Mod_ResetStudioAPI

Returns to default callbacks
===============
*/
void Mod_ResetStudioAPI( void )
{
	pBlendAPI = &gBlendAPI;
}
