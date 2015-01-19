/*
gl_studio.c - studio model renderer
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
#include "mathlib.h"
#include "const.h"
#include "r_studioint.h"
#include "studio.h"
#include "pm_local.h"
#include "gl_local.h"
#include "cl_tent.h"

// NOTE: enable this if you want merge both 'model' and 'modelT' files into one model slot.
// otherwise it's uses two slots in models[] array for models with external textures
#define STUDIO_MERGE_TEXTURES

#define EVENT_CLIENT	5000	// less than this value it's a server-side studio events
#define MAXARRAYVERTS	16384	// used for draw shadows
#define LEGS_BONES_COUNT	8

static vec3_t hullcolor[8] = 
{
{ 1.0f, 1.0f, 1.0f },
{ 1.0f, 0.5f, 0.5f },
{ 0.5f, 1.0f, 0.5f },
{ 1.0f, 1.0f, 0.5f },
{ 0.5f, 0.5f, 1.0f },
{ 1.0f, 0.5f, 1.0f },
{ 0.5f, 1.0f, 1.0f },
{ 1.0f, 1.0f, 1.0f },
};

// enumerate all the bones that used for gait animation
const char *legs_bones[] =
{
{ "Bip01" },
{ "Bip01 Pelvis" },
{ "Bip01 L Leg" },
{ "Bip01 L Leg1" },
{ "Bip01 L Foot" },
{ "Bip01 R Leg" },
{ "Bip01 R Leg1" },
{ "Bip01 R Foot" },
};

typedef struct studiolight_s
{
	vec3_t		lightvec;			// light vector
	vec3_t		lightcolor;		// ambient light color
	vec3_t		lightspot;		// potential coords where placed lightsource

	vec3_t		blightvec[MAXSTUDIOBONES];	// ambient lightvectors per bone
	vec3_t		dlightvec[MAX_DLIGHTS][MAXSTUDIOBONES];
	vec3_t		dlightcolor[MAX_DLIGHTS];	// ambient dynamic light colors
	vec3_t		elightvec[MAX_ELIGHTS][MAXSTUDIOBONES];
	vec3_t		elightcolor[MAX_ELIGHTS];	// ambient entity light colors
	int		numdlights;
	int		numelights;
} studiolight_t;

typedef struct sortedmesh_s
{
	mstudiomesh_t	*mesh;
	int		flags;			// face flags
} sortedmesh_t;

convar_t			*r_studio_lerping;
convar_t			*r_studio_lambert;
convar_t			*r_studio_lighting;
convar_t			*r_studio_sort_textures;
convar_t			*r_drawviewmodel;
convar_t			*r_customdraw_playermodel;
convar_t			*cl_himodels;
cvar_t			r_shadows = { "r_shadows", "0", 0, 0 };	// dead cvar. especially disabled
cvar_t			r_shadowalpha = { "r_shadowalpha", "0.5", 0, 0.8f };
static r_studio_interface_t	*pStudioDraw;
static float		aliasXscale, aliasYscale;	// software renderer scale
static matrix3x4		g_aliastransform;		// software renderer transform
static matrix3x4		g_rotationmatrix;
static vec3_t		g_chrome_origin;
static vec2_t		g_chrome[MAXSTUDIOVERTS];	// texture coords for surface normals
static sortedmesh_t		g_sortedMeshes[MAXSTUDIOMESHES];
static matrix3x4		g_bonestransform[MAXSTUDIOBONES];
static matrix3x4		g_lighttransform[MAXSTUDIOBONES];
static matrix3x4		g_rgCachedBonesTransform[MAXSTUDIOBONES];
static matrix3x4		g_rgCachedLightTransform[MAXSTUDIOBONES];
static vec3_t		g_chromeright[MAXSTUDIOBONES];// chrome vector "right" in bone reference frames
static vec3_t		g_chromeup[MAXSTUDIOBONES];	// chrome vector "up" in bone reference frames
static int		g_chromeage[MAXSTUDIOBONES];	// last time chrome vectors were updated
static vec3_t		g_xformverts[MAXSTUDIOVERTS];
static vec3_t		g_xformnorms[MAXSTUDIOVERTS];
static vec3_t		g_xarrayverts[MAXARRAYVERTS];
static uint		g_xarrayelems[MAXARRAYVERTS*6];
static uint		g_nNumArrayVerts;
static uint		g_nNumArrayElems;
static vec3_t		g_lightvalues[MAXSTUDIOVERTS];
static studiolight_t	g_studiolight;
char			g_nCachedBoneNames[MAXSTUDIOBONES][32];
int			g_nCachedBones;		// number of bones in cache
int			g_nStudioCount;		// for chrome update
int			g_iRenderMode;		// currentmodel rendermode
int			g_iBackFaceCull;
vec3_t			studio_mins, studio_maxs;
float			studio_radius;

// global variables
qboolean			m_fDoInterp;
qboolean			m_fDoRemap;
mstudiomodel_t		*m_pSubModel;
mstudiobodyparts_t		*m_pBodyPart;
player_info_t		*m_pPlayerInfo;
studiohdr_t		*m_pStudioHeader;
studiohdr_t		*m_pTextureHeader;
float			m_flGaitMovement;
pmtrace_t			g_shadowTrace;
vec3_t			g_mvShadowVec;
int			g_nTopColor, g_nBottomColor;	// remap colors
int			g_nFaceFlags, g_nForceFaceFlags;

/*
====================
R_StudioInit

====================
*/
void R_StudioInit( void )
{
	float	pixelAspect;

	r_studio_lambert = Cvar_Get( "r_studio_lambert", "2", CVAR_ARCHIVE, "bonelighting lambert value" );
	r_studio_lerping = Cvar_Get( "r_studio_lerping", "1", CVAR_ARCHIVE, "enables studio animation lerping" );
	r_drawviewmodel = Cvar_Get( "r_drawviewmodel", "1", 0, "draw firstperson weapon model" );
	cl_himodels = Cvar_Get( "cl_himodels", "1", CVAR_ARCHIVE, "draw high-resolution player models in multiplayer" );
	r_studio_lighting = Cvar_Get( "r_studio_lighting", "1", CVAR_ARCHIVE, "studio lighting models ( 0 - normal, 1 - extended, 2 - experimental )" );
	r_studio_sort_textures = Cvar_Get( "r_studio_sort_textures", "0", CVAR_ARCHIVE, "sort additive and normal textures for right drawing" );

	// NOTE: some mods with custom studiomodel renderer may cause error when menu trying draw player model out of the loaded game
	r_customdraw_playermodel = Cvar_Get( "r_customdraw_playermodel", "0", CVAR_ARCHIVE, "allow to drawing playermodel in menu with client renderer" );

	// recalc software X and Y alias scale (this stuff is used only by HL software renderer but who knews...)
	pixelAspect = ((float)scr_height->integer / (float)scr_width->integer);
	if( scr_width->integer < 640 )
		pixelAspect *= (320.0f / 240.0f);
	else pixelAspect *= (640.0f / 480.0f);

	aliasXscale = (float)scr_width->integer / RI.refdef.fov_y;
	aliasYscale = aliasXscale * pixelAspect;

	Matrix3x4_LoadIdentity( g_aliastransform );
	Matrix3x4_LoadIdentity( g_rotationmatrix );

	g_nStudioCount = 0;
	m_fDoRemap = false;
}

/*
===============
R_StudioTexName

extract texture filename from modelname
===============
*/
const char *R_StudioTexName( model_t *mod )
{
	static char	texname[64];

	Q_strncpy( texname, mod->name, sizeof( texname ));
	FS_StripExtension( texname );
	Q_strncat( texname, "T.mdl", sizeof( texname ));

	return texname;
}

/*
================
R_StudioBodyVariations

calc studio body variations
================
*/
static int R_StudioBodyVariations( model_t *mod )
{
	studiohdr_t	*pstudiohdr;
	mstudiobodyparts_t	*pbodypart;
	int		i, count;

	pstudiohdr = (studiohdr_t *)Mod_Extradata( mod );
	if( !pstudiohdr ) return 0;

	count = 1;
	pbodypart = (mstudiobodyparts_t *)((byte *)pstudiohdr + pstudiohdr->bodypartindex);

	// each body part has nummodels variations so there are as many total variations as there
	// are in a matrix of each part by each other part
	for( i = 0; i < pstudiohdr->numbodyparts; i++ )
		count = count * pbodypart[i].nummodels;

	return count;
}

/*
================
R_StudioExtractBbox

Extract bbox from current sequence
================
*/
qboolean R_StudioExtractBbox( studiohdr_t *phdr, int sequence, float *mins, float *maxs )
{
	mstudioseqdesc_t	*pseqdesc;

	if( !phdr ) return false;

	// check sequence range
	if( sequence < 0 || sequence >= phdr->numseq )
		sequence = 0;

	pseqdesc = (mstudioseqdesc_t *)((byte *)phdr + phdr->seqindex);
	VectorCopy( pseqdesc[sequence].bbmin, mins );
	VectorCopy( pseqdesc[sequence].bbmax, maxs );

	return true;
}

/*
================
R_StudioComputeBBox

Compute a full bounding box for current sequence
================
*/
static qboolean R_StudioComputeBBox( cl_entity_t *e, vec3_t bbox[8] )
{
	vec3_t	tmp_mins, tmp_maxs;
	vec3_t	vectors[3], angles, p1, p2;
	int	i, seq = e->curstate.sequence;

	if( !R_StudioExtractBbox( m_pStudioHeader, seq, tmp_mins, tmp_maxs ))
		return false;

	// copy original bbox
	VectorCopy( m_pStudioHeader->bbmin, studio_mins );
	VectorCopy( m_pStudioHeader->bbmax, studio_maxs );

	// rotate the bounding box
	VectorCopy( e->angles, angles );

	if( e->player ) angles[PITCH] = 0.0f; // don't rotate player model, only aim
	AngleVectors( angles, vectors[0], vectors[1], vectors[2] );

	// compute a full bounding box
	for( i = 0; i < 8; i++ )
	{
  		p1[0] = ( i & 1 ) ? tmp_mins[0] : tmp_maxs[0];
  		p1[1] = ( i & 2 ) ? tmp_mins[1] : tmp_maxs[1];
  		p1[2] = ( i & 4 ) ? tmp_mins[2] : tmp_maxs[2];

		// rotate by YAW
		p2[0] = DotProduct( p1, vectors[0] );
		p2[1] = DotProduct( p1, vectors[1] );
		p2[2] = DotProduct( p1, vectors[2] );

		if( bbox ) VectorAdd( p2, e->origin, bbox[i] );

  		if( p2[0] < studio_mins[0] ) studio_mins[0] = p2[0];
  		if( p2[0] > studio_maxs[0] ) studio_maxs[0] = p2[0];
  		if( p2[1] < studio_mins[1] ) studio_mins[1] = p2[1];
  		if( p2[1] > studio_maxs[1] ) studio_maxs[1] = p2[1];
  		if( p2[2] < studio_mins[2] ) studio_mins[2] = p2[2];
  		if( p2[2] > studio_maxs[2] ) studio_maxs[2] = p2[2];
	}

	studio_radius = RadiusFromBounds( studio_mins, studio_maxs );

	return true;
}

/*
===============
pfnGetCurrentEntity

===============
*/
static cl_entity_t *pfnGetCurrentEntity( void )
{
	return RI.currententity;
}

/*
===============
pfnPlayerInfo

===============
*/
static player_info_t *pfnPlayerInfo( int index )
{
	if( cls.key_dest == key_menu && !index )
		return &menu.playerinfo;

	if( index < 0 || index > cl.maxclients )
		return NULL;
	return &cl.players[index];
}

/*
===============
pfnGetPlayerState

===============
*/
entity_state_t *R_StudioGetPlayerState( int index )
{
	if( index < 0 || index > cl.maxclients )
		return NULL;
	return &cl.frame.playerstate[index];
}

/*
===============
pfnGetViewEntity

===============
*/
static cl_entity_t *pfnGetViewEntity( void )
{
	return &clgame.viewent;
}

/*
===============
pfnGetEngineTimes

===============
*/
static void pfnGetEngineTimes( int *framecount, double *current, double *old )
{
	if( framecount ) *framecount = tr.framecount;
	if( current ) *current = cl.time;
	if( old ) *old = cl.oldtime;
}

/*
===============
pfnGetViewInfo

===============
*/
static void pfnGetViewInfo( float *origin, float *upv, float *rightv, float *forwardv )
{
	if( origin ) VectorCopy( RI.vieworg, origin );
	if( forwardv ) VectorCopy( RI.vforward, forwardv );
	if( rightv ) VectorCopy( RI.vright, rightv );
	if( upv ) VectorCopy( RI.vup, upv );
}

/*
===============
R_GetChromeSprite

===============
*/
static model_t *R_GetChromeSprite( void )
{
	if( cls.hChromeSprite <= 0 || cls.hChromeSprite > ( MAX_IMAGES - 1 ))
		return NULL; // bad sprite
	return &clgame.sprites[cls.hChromeSprite];
}

/*
===============
pfnGetModelCounters

===============
*/
static void pfnGetModelCounters( int **s, int **a )
{
	*s = &g_nStudioCount;
	*a = &r_stats.c_studio_models_drawn;
}

/*
===============
pfnGetAliasScale

===============
*/
static void pfnGetAliasScale( float *x, float *y )
{
	if( x ) *x = aliasXscale;
	if( y ) *y = aliasYscale;
}

/*
===============
pfnStudioGetBoneTransform

===============
*/
static float ****pfnStudioGetBoneTransform( void )
{
	return (float ****)g_bonestransform;
}

/*
===============
pfnStudioGetLightTransform

===============
*/
static float ****pfnStudioGetLightTransform( void )
{
	return (float ****)g_lighttransform;
}

/*
===============
pfnStudioGetAliasTransform

===============
*/
static float ***pfnStudioGetAliasTransform( void )
{
	return (float ***)g_aliastransform;
}

/*
===============
pfnStudioGetRotationMatrix

===============
*/
static float ***pfnStudioGetRotationMatrix( void )
{
	return (float ***)g_rotationmatrix;
}

/*
====================
CullStudioModel

====================
*/
qboolean R_CullStudioModel( cl_entity_t *e )
{
	vec3_t	origin;

	if( !e || !e->model || !e->model->cache.data )
		return true;

	if( e == &clgame.viewent && ( r_lefthand->integer >= 2 || gl_overview->integer ))
		return true; // hidden

	if( !R_StudioComputeBBox( e, NULL ))
		return true; // invalid sequence

	// NOTE: extract real drawing origin from rotation matrix
	Matrix3x4_OriginFromMatrix( g_rotationmatrix, origin );

	return R_CullModel( e, origin, studio_mins, studio_maxs, studio_radius );
}

/*
====================
StudioPlayerBlend

====================
*/
void R_StudioPlayerBlend( mstudioseqdesc_t *pseqdesc, int *pBlend, float *pPitch )
{
	// calc up/down pointing
	if( RI.params & RP_MIRRORVIEW )
		*pBlend = (*pPitch * -6.0f);
	else *pBlend = (*pPitch * 3.0f);

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
		else *pBlend = 255 * (*pBlend - pseqdesc->blendstart[0]) / (pseqdesc->blendend[0] - pseqdesc->blendstart[0]);
		*pPitch = 0.0f;
	}
}

/*
====================
StudioSetUpTransform

====================
*/
void R_StudioSetUpTransform( cl_entity_t *e )
{
	vec3_t	origin, angles;

	VectorCopy( e->origin, origin );
	VectorCopy( e->angles, angles );

	// interpolate monsters position (moved into UpdateEntityFields by user request)
	if( e->curstate.movetype == MOVETYPE_STEP && !( host.features & ENGINE_COMPUTE_STUDIO_LERP )) 
	{
		float	d, f = 0.0f;
		int	i;

		// don't do it if the goalstarttime hasn't updated in a while.
		// NOTE: Because we need to interpolate multiplayer characters, the interpolation time limit
		// was increased to 1.0 s., which is 2x the max lag we are accounting for.
		if( m_fDoInterp && ( RI.refdef.time < e->curstate.animtime + 1.0f ) && ( e->curstate.animtime != e->latched.prevanimtime ))
		{
			f = ( RI.refdef.time - e->curstate.animtime ) / ( e->curstate.animtime - e->latched.prevanimtime );
			// Msg( "%4.2f %.2f %.2f\n", f, e->curstate.animtime, RI.refdef.time );
		}

		if( m_fDoInterp )
		{
			// ugly hack to interpolate angle, position.
			// current is reached 0.1 seconds after being set
			f = f - 1.0f;
		}

		origin[0] += ( e->origin[0] - e->latched.prevorigin[0] ) * f;
		origin[1] += ( e->origin[1] - e->latched.prevorigin[1] ) * f;
		origin[2] += ( e->origin[2] - e->latched.prevorigin[2] ) * f;

		for( i = 0; i < 3; i++ )
		{
			float	ang1, ang2;

			ang1 = e->angles[i];
			ang2 = e->latched.prevangles[i];

			d = ang1 - ang2;

			if( d > 180.0f ) d -= 360.0f;
			else if( d < -180.0f ) d += 360.0f;

			angles[i] += d * f;
		}
	}

	if( !( host.features & ENGINE_COMPENSATE_QUAKE_BUG ))
		angles[PITCH] = -angles[PITCH]; // stupid quake bug

	// don't rotate clients, only aim
	if( e->player ) angles[PITCH] = 0.0f;

	Matrix3x4_CreateFromEntity( g_rotationmatrix, angles, origin, 1.0f );

	if( e == &clgame.viewent && r_lefthand->integer == 1 )
	{
		// inverse the right vector (should work in Opposing Force)
		g_rotationmatrix[0][1] = -g_rotationmatrix[0][1];
		g_rotationmatrix[1][1] = -g_rotationmatrix[1][1];
		g_rotationmatrix[2][1] = -g_rotationmatrix[2][1];
	}
}

/*
====================
StudioEstimateFrame

====================
*/
float R_StudioEstimateFrame( cl_entity_t *e, mstudioseqdesc_t *pseqdesc )
{
	double	dfdt, f;
	
	if( m_fDoInterp )
	{
		if( RI.refdef.time < e->curstate.animtime ) dfdt = 0.0;
		else dfdt = (RI.refdef.time - e->curstate.animtime) * e->curstate.framerate * pseqdesc->fps;
	}
	else dfdt = 0;

	if( pseqdesc->numframes <= 1 ) f = 0.0;
	else f = (e->curstate.frame * (pseqdesc->numframes - 1)) / 256.0;
 
	f += dfdt;

	if( pseqdesc->flags & STUDIO_LOOPING ) 
	{
		if( pseqdesc->numframes > 1 )
			f -= (int)(f / (pseqdesc->numframes - 1)) *  (pseqdesc->numframes - 1);
		if( f < 0 ) f += (pseqdesc->numframes - 1);
	}
	else 
	{
		if( f >= pseqdesc->numframes - 1.001 )
			f = pseqdesc->numframes - 1.001;
		if( f < 0.0 )  f = 0.0;
	}
	return f;
}

/*
====================
StudioEstimateInterpolant

====================
*/
float R_StudioEstimateInterpolant( cl_entity_t *e )
{
	float	dadt = 1.0f;

	if( m_fDoInterp && ( e->curstate.animtime >= e->latched.prevanimtime + 0.01f ))
	{
		dadt = ( RI.refdef.time - e->curstate.animtime ) / 0.1f;
		if( dadt > 2.0f ) dadt = 2.0f;
	}
	return dadt;
}

/*
====================
StudioGetAnim

====================
*/
mstudioanim_t *R_StudioGetAnim( model_t *m_pSubModel, mstudioseqdesc_t *pseqdesc )
{
	mstudioseqgroup_t	*pseqgroup;
	cache_user_t	*paSequences;
	size_t		filesize;
          byte		*buf;

	ASSERT( m_pSubModel );	

	pseqgroup = (mstudioseqgroup_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqgroupindex) + pseqdesc->seqgroup;
	if( pseqdesc->seqgroup == 0 )
		return (mstudioanim_t *)((byte *)m_pStudioHeader + pseqgroup->data + pseqdesc->animindex);

	paSequences = (cache_user_t *)m_pSubModel->submodels;

	if( paSequences == NULL )
	{
		paSequences = (cache_user_t *)Mem_Alloc( com_studiocache, MAXSTUDIOGROUPS * sizeof( cache_user_t ));
		m_pSubModel->submodels = (void *)paSequences;
	}

	// check for already loaded
	if( !Mod_CacheCheck(( cache_user_t *)&( paSequences[pseqdesc->seqgroup] )))
	{
		string	filepath, modelname, modelpath;

		FS_FileBase( m_pSubModel->name, modelname );
		FS_ExtractFilePath( m_pSubModel->name, modelpath );

		// NOTE: here we build real sub-animation filename because stupid user may rename model without recompile
		Q_snprintf( filepath, sizeof( filepath ), "%s/%s%i%i.mdl", modelpath, modelname, pseqdesc->seqgroup / 10, pseqdesc->seqgroup % 10 );

		buf = FS_LoadFile( filepath, &filesize, false );
		if( !buf || !filesize ) Host_Error( "StudioGetAnim: can't load %s\n", filepath );
		if( IDSEQGRPHEADER != *(uint *)buf ) Host_Error( "StudioGetAnim: %s is corrupted\n", filepath );

		MsgDev( D_INFO, "loading: %s\n", filepath );
			
		paSequences[pseqdesc->seqgroup].data = Mem_Alloc( com_studiocache, filesize );
		Q_memcpy( paSequences[pseqdesc->seqgroup].data, buf, filesize );
		Mem_Free( buf );
	}

	return (mstudioanim_t *)((byte *)paSequences[pseqdesc->seqgroup].data + pseqdesc->animindex);
}

/*
====================
StudioFxTransform

====================
*/
void R_StudioFxTransform( cl_entity_t *ent, matrix3x4 transform )
{
	switch( ent->curstate.renderfx )
	{
	case kRenderFxDistort:
	case kRenderFxHologram:
		if( !Com_RandomLong( 0, 49 ))
		{
			int	axis = Com_RandomLong( 0, 1 );

			if( axis == 1 ) axis = 2; // choose between x & z
			VectorScale( transform[axis], Com_RandomFloat( 1.0f, 1.484f ), transform[axis] );
		}
		else if( !Com_RandomLong( 0, 49 ))
		{
			float	offset;
			int	axis = Com_RandomLong( 0, 1 );

			if( axis == 1 ) axis = 2; // choose between x & z
			offset = Com_RandomFloat( -10.0f, 10.0f );
			transform[Com_RandomLong( 0, 2 )][3] += offset;
		}
		break;
	case kRenderFxExplode:
		{
			float	scale;

			scale = 1.0f + ( RI.refdef.time - ent->curstate.animtime ) * 10.0f;
			if( scale > 2.0f ) scale = 2.0f; // don't blow up more than 200%

			transform[0][1] *= scale;
			transform[1][1] *= scale;
			transform[2][1] *= scale;
		}
		break;
	}
}

/*
====================
StudioCalcBoneAdj

====================
*/
void R_StudioCalcBoneAdj( float dadt, float *adj, const byte *pcontroller1, const byte *pcontroller2, byte mouthopen )
{
	mstudiobonecontroller_t	*pbonecontroller;
	float			value;	
	int			i, j;

	pbonecontroller = (mstudiobonecontroller_t *)((byte *)m_pStudioHeader + m_pStudioHeader->bonecontrollerindex);

	for (j = 0; j < m_pStudioHeader->numbonecontrollers; j++)
	{
		i = pbonecontroller[j].index;

		if( i == STUDIO_MOUTH )
		{
			// mouth hardcoded at controller 4
			value = (float)mouthopen / 64.0f;
			if( value > 1.0f ) value = 1.0f;				
			value = (1.0f - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
		}
		else if( i <= MAXSTUDIOCONTROLLERS )
		{
			// check for 360% wrapping
			if( pbonecontroller[j].type & STUDIO_RLOOP )
			{
				if( abs( pcontroller1[i] - pcontroller2[i] ) > 128 )
				{
					float	a, b;

					a = fmod(( pcontroller1[j] + 128.0f ), 256.0f );
					b = fmod(( pcontroller2[j] + 128.0f ), 256.0f );
					value = ((a * dadt) + (b * (1 - dadt)) - 128.0f) * (360.0f / 256.0f) + pbonecontroller[j].start;
				}
				else 
				{
					value = ((pcontroller1[i] * dadt + (pcontroller2[i]) * (1.0f - dadt))) * (360.0f / 256.0f) + pbonecontroller[j].start;
				}
			}
			else 
			{
				value = (pcontroller1[i] * dadt + pcontroller2[i] * (1.0f - dadt)) / 255.0f;
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
void R_StudioCalcBoneQuaterion( int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, vec4_t q )
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

				// debug
				if( panimvalue->num.total < panimvalue->num.valid )
					k = 0;
			}

			// bah, missing blend!
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
void R_StudioCalcBonePosition( int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, vec3_t adj, vec3_t pos )
{
	mstudioanimvalue_t	*panimvalue;
	int		j, k;

	for( j = 0; j < 3; j++ )
	{
		pos[j] = pbone->value[j]; // default;

		if( panim->offset[j] != 0 )
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

  				// debug
				if( panimvalue->num.total < panimvalue->num.valid )
					k = 0;
			}

			// if we're inside the span
			if( panimvalue->num.valid > k )
			{
				// and there's more data in the span
				if( panimvalue->num.valid > k + 1 )
					pos[j] += (panimvalue[k+1].value * (1.0f - s) + s * panimvalue[k+2].value) * pbone->scale[j];
				else pos[j] += panimvalue[k+1].value * pbone->scale[j];
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if( panimvalue->num.total <= k + 1 )
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0f - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				else pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
			}
		}

		if( pbone->bonecontroller[j] != -1 && adj )
			pos[j] += adj[pbone->bonecontroller[j]];
	}
}

/*
====================
StudioSlerpBones

====================
*/
void R_StudioSlerpBones( vec4_t q1[], float pos1[][3], vec4_t q2[], float pos2[][3], float s )
{
	int	i;
	vec4_t	q3;
	float	s1;

	s = bound( 0.0f, s, 1.0f );
	s1 = 1.0f - s; // backlerp

	for( i = 0; i < m_pStudioHeader->numbones; i++ )
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

/*
====================
StudioCalcRotations

====================
*/
void R_StudioCalcRotations( cl_entity_t *e, float pos[][3], vec4_t *q, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f )
{
	int		i, frame;
	mstudiobone_t	*pbone;
	float		adj[MAXSTUDIOCONTROLLERS];
	float		s, dadt;

	if( f > pseqdesc->numframes - 1 )
	{
		f = 0.0f; // bah, fix this bug with changing sequences too fast
	}
	else if( f < -0.01f )
	{
		// this could cause a crash if the frame # is negative, so we'll go ahead
		// and clamp it here
		MsgDev( D_ERROR, "StudioCalcRotations: f = %g\n", f );
		f = -0.01f;
	}

	frame = (int)f;

	dadt = R_StudioEstimateInterpolant( e );
	s = (f - frame);

	// add in programtic controllers
	pbone = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	R_StudioCalcBoneAdj( dadt, adj, e->curstate.controller, e->latched.prevcontroller, e->mouth.mouthopen );

	for( i = 0; i < m_pStudioHeader->numbones; i++, pbone++, panim++ ) 
	{
		R_StudioCalcBoneQuaterion( frame, s, pbone, panim, adj, q[i] );
		R_StudioCalcBonePosition( frame, s, pbone, panim, adj, pos[i] );
	}

	if( pseqdesc->motiontype & STUDIO_X ) pos[pseqdesc->motionbone][0] = 0.0f;
	if( pseqdesc->motiontype & STUDIO_Y ) pos[pseqdesc->motionbone][1] = 0.0f;
	if( pseqdesc->motiontype & STUDIO_Z ) pos[pseqdesc->motionbone][2] = 0.0f;
#if 0
	s = 1.0f * ((1.0f - (f - (int)(f))) / (pseqdesc->numframes)) * e->curstate.framerate;

	if( pseqdesc->motiontype & STUDIO_LX ) pos[pseqdesc->motionbone][0] += s * pseqdesc->linearmovement[0];
	if( pseqdesc->motiontype & STUDIO_LY ) pos[pseqdesc->motionbone][1] += s * pseqdesc->linearmovement[1];
	if( pseqdesc->motiontype & STUDIO_LZ ) pos[pseqdesc->motionbone][2] += s * pseqdesc->linearmovement[2];
#endif
}

/*
====================
StudioMergeBones

====================
*/
void R_StudioMergeBones( cl_entity_t *e, model_t *m_pSubModel )
{
	int		i, j;
	mstudiobone_t	*pbones;
	mstudioseqdesc_t	*pseqdesc;
	mstudioanim_t	*panim;
	matrix3x4		bonematrix;
	static vec4_t	q[MAXSTUDIOBONES];
	static float	pos[MAXSTUDIOBONES][3];
	double		f;

	if( e->curstate.sequence >=  m_pStudioHeader->numseq )
		e->curstate.sequence = 0;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + e->curstate.sequence;

	f = R_StudioEstimateFrame( e, pseqdesc );

	panim = R_StudioGetAnim( m_pSubModel, pseqdesc );
	R_StudioCalcRotations( e, pos, q, pseqdesc, panim, f );
	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	for( i = 0; i < m_pStudioHeader->numbones; i++ ) 
	{
		for( j = 0; j < g_nCachedBones; j++ )
		{
			if( !Q_stricmp( pbones[i].name, g_nCachedBoneNames[j] ))
			{
				Matrix3x4_Copy( g_bonestransform[i], g_rgCachedBonesTransform[j] );
				Matrix3x4_Copy( g_lighttransform[i], g_rgCachedLightTransform[j] );
				break;
			}
		}

		if( j >= g_nCachedBones )
		{
			Matrix3x4_FromOriginQuat( bonematrix, q[i], pos[i] );
			if( pbones[i].parent == -1 ) 
			{
				Matrix3x4_ConcatTransforms( g_bonestransform[i], g_rotationmatrix, bonematrix );
				Matrix3x4_Copy( g_lighttransform[i], g_bonestransform[i] );

				// apply client-side effects to the transformation matrix
				R_StudioFxTransform( e, g_bonestransform[i] );
			} 
			else 
			{
				Matrix3x4_ConcatTransforms( g_bonestransform[i], g_bonestransform[pbones[i].parent], bonematrix );
				Matrix3x4_ConcatTransforms( g_lighttransform[i], g_lighttransform[pbones[i].parent], bonematrix );
			}
		}
	}
}

/*
====================
StudioSetupBones

====================
*/
void R_StudioSetupBones( cl_entity_t *e )
{
	double		f;
	mstudiobone_t	*pbones;
	mstudioseqdesc_t	*pseqdesc;
	mstudioanim_t	*panim;
	matrix3x4		bonematrix;
	static vec3_t	pos[MAXSTUDIOBONES];
	static vec4_t	q[MAXSTUDIOBONES];
	static vec3_t	pos2[MAXSTUDIOBONES];
	static vec4_t	q2[MAXSTUDIOBONES];
	static vec3_t	pos3[MAXSTUDIOBONES];
	static vec4_t	q3[MAXSTUDIOBONES];
	static vec3_t	pos4[MAXSTUDIOBONES];
	static vec4_t	q4[MAXSTUDIOBONES];
	int		i, j;

	if( e->curstate.sequence >= m_pStudioHeader->numseq )
		e->curstate.sequence = 0;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + e->curstate.sequence;

	f = R_StudioEstimateFrame( e, pseqdesc );

	panim = R_StudioGetAnim( e->model, pseqdesc );
	R_StudioCalcRotations( e, pos, q, pseqdesc, panim, f );

	if( pseqdesc->numblends > 1 )
	{
		float	s;
		float	dadt;

		panim += m_pStudioHeader->numbones;
		R_StudioCalcRotations( e, pos2, q2, pseqdesc, panim, f );

		dadt = R_StudioEstimateInterpolant( e );
		s = (e->curstate.blending[0] * dadt + e->latched.prevblending[0] * (1.0f - dadt)) / 255.0f;

		R_StudioSlerpBones( q, pos, q2, pos2, s );

		if( pseqdesc->numblends == 4 )
		{
			panim += m_pStudioHeader->numbones;
			R_StudioCalcRotations( e, pos3, q3, pseqdesc, panim, f );

			panim += m_pStudioHeader->numbones;
			R_StudioCalcRotations( e, pos4, q4, pseqdesc, panim, f );

			s = (e->curstate.blending[0] * dadt + e->latched.prevblending[0] * (1.0f - dadt)) / 255.0f;
			R_StudioSlerpBones( q3, pos3, q4, pos4, s );

			s = (e->curstate.blending[1] * dadt + e->latched.prevblending[1] * (1.0f - dadt)) / 255.0f;
			R_StudioSlerpBones( q, pos, q3, pos3, s );
		}
	}

	if( m_fDoInterp && e->latched.sequencetime && ( e->latched.sequencetime + 0.2f > RI.refdef.time) && ( e->latched.prevsequence < m_pStudioHeader->numseq ))
	{
		// blend from last sequence
		static vec3_t	pos1b[MAXSTUDIOBONES];
		static vec4_t	q1b[MAXSTUDIOBONES];
		float		s;

		pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + e->latched.prevsequence;
		panim = R_StudioGetAnim( e->model, pseqdesc );

		// clip prevframe
		R_StudioCalcRotations( e, pos1b, q1b, pseqdesc, panim, e->latched.prevframe );

		if( pseqdesc->numblends > 1 )
		{
			panim += m_pStudioHeader->numbones;
			R_StudioCalcRotations( e, pos2, q2, pseqdesc, panim, e->latched.prevframe );

			s = (e->latched.prevseqblending[0]) / 255.0f;
			R_StudioSlerpBones( q1b, pos1b, q2, pos2, s );

			if( pseqdesc->numblends == 4 )
			{
				panim += m_pStudioHeader->numbones;
				R_StudioCalcRotations( e, pos3, q3, pseqdesc, panim, e->latched.prevframe );

				panim += m_pStudioHeader->numbones;
				R_StudioCalcRotations( e, pos4, q4, pseqdesc, panim, e->latched.prevframe );

				s = (e->latched.prevseqblending[0]) / 255.0f;
				R_StudioSlerpBones( q3, pos3, q4, pos4, s );

				s = (e->latched.prevseqblending[1]) / 255.0f;
				R_StudioSlerpBones( q1b, pos1b, q3, pos3, s );
			}
		}

		s = 1.0f - ( RI.refdef.time - e->latched.sequencetime ) / 0.2f;
		R_StudioSlerpBones( q, pos, q1b, pos1b, s );
	}
	else
	{
		// store prevframe otherwise
		e->latched.prevframe = f;
	}

	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	// calc gait animation
	if( m_pPlayerInfo && m_pPlayerInfo->gaitsequence != 0 )
	{
		if( m_pPlayerInfo->gaitsequence >= m_pStudioHeader->numseq ) 
			m_pPlayerInfo->gaitsequence = 0;

		pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pPlayerInfo->gaitsequence;

		panim = R_StudioGetAnim( e->model, pseqdesc );
		R_StudioCalcRotations( e, pos2, q2, pseqdesc, panim, m_pPlayerInfo->gaitframe );

		for( i = 0; i < m_pStudioHeader->numbones; i++ )
		{
			for( j = 0; j < LEGS_BONES_COUNT; j++ )
			{
				if( !Q_strcmp( pbones[i].name, legs_bones[j] ))
					break;
			}

			if( j == LEGS_BONES_COUNT )
				continue;	// not used for legs

			VectorCopy( pos2[i], pos[i] );
			Vector4Copy( q2[i], q[i] );
		}
	}

	for( i = 0; i < m_pStudioHeader->numbones; i++ ) 
	{
		Matrix3x4_FromOriginQuat( bonematrix, q[i], pos[i] );

		if( pbones[i].parent == -1 ) 
		{
			Matrix3x4_ConcatTransforms( g_bonestransform[i], g_rotationmatrix, bonematrix );
			Matrix3x4_Copy( g_lighttransform[i], g_bonestransform[i] );

			// apply client-side effects to the transformation matrix
			R_StudioFxTransform( e, g_bonestransform[i] );
		} 
		else
		{
			Matrix3x4_ConcatTransforms( g_bonestransform[i], g_bonestransform[pbones[i].parent], bonematrix );
			Matrix3x4_ConcatTransforms( g_lighttransform[i], g_lighttransform[pbones[i].parent], bonematrix );
		}
	}
}

/*
====================
StudioSaveBones

====================
*/
static void R_StudioSaveBones( void )
{
	mstudiobone_t	*pbones;
	int		i;

	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	g_nCachedBones = m_pStudioHeader->numbones;

	for( i = 0; i < m_pStudioHeader->numbones; i++ ) 
	{
		Q_strcpy( g_nCachedBoneNames[i], pbones[i].name );
		Matrix3x4_Copy( g_rgCachedBonesTransform[i], g_bonestransform[i] );
		Matrix3x4_Copy( g_rgCachedLightTransform[i], g_lighttransform[i] );
	}
}

/*
====================
StudioSetupChrome

====================
*/
void R_StudioSetupChrome( float *pchrome, int bone, vec3_t normal )
{
	float	n;

	if( g_chromeage[bone] != g_nStudioCount )
	{
		// calculate vectors from the viewer to the bone. This roughly adjusts for position
		vec3_t	chromeupvec;	// g_chrome t vector in world reference frame
		vec3_t	chromerightvec;	// g_chrome s vector in world reference frame
		vec3_t	tmp, v_left;	// vector pointing at bone in world reference frame

		VectorCopy( g_chrome_origin, tmp );
		tmp[0] += g_bonestransform[bone][0][3];
		tmp[1] += g_bonestransform[bone][1][3];
		tmp[2] += g_bonestransform[bone][2][3];

		VectorNormalize( tmp );
		VectorNegate( RI.vright, v_left );

		if( g_nForceFaceFlags & STUDIO_NF_CHROME )
		{
			float	angle, sr, cr;
			int	i;

			angle = anglemod( RI.refdef.time * 40 ) * (M_PI2 / 360.0f);
			SinCos( angle, &sr, &cr );

			for( i = 0; i < 3; i++ )
			{
				chromerightvec[i] = (v_left[i] * cr + RI.vup[i] * sr);
				chromeupvec[i] = v_left[i] * -sr + RI.vup[i] * cr;
			}
		}
		else
		{
			CrossProduct( tmp, v_left, chromeupvec );
			VectorNormalize( chromeupvec );
			CrossProduct( tmp, chromeupvec, chromerightvec );
			VectorNormalize( chromerightvec );
			VectorNegate( chromeupvec, chromeupvec );
		}

		Matrix3x4_VectorIRotate( g_bonestransform[bone], chromeupvec, g_chromeup[bone] );
		Matrix3x4_VectorIRotate( g_bonestransform[bone], chromerightvec, g_chromeright[bone] );
		g_chromeage[bone] = g_nStudioCount;
	}

	// calc s coord
	n = DotProduct( normal, g_chromeright[bone] );
	pchrome[0] = (n + 1.0f) * 32.0f;

	// calc t coord
	n = DotProduct( normal, g_chromeup[bone] );
	pchrome[1] = (n + 1.0f) * 32.0f;
}

/*
====================
StudioCalcAttachments

====================
*/
static void R_StudioCalcAttachments( void )
{
	mstudioattachment_t	*pAtt;
	vec3_t		forward, bonepos;
	vec3_t		localOrg, localAng;
	int		i;

	if( m_pStudioHeader->numattachments <= 0 )
	{
		// clear attachments
		for( i = 0; i < MAXSTUDIOATTACHMENTS; i++ )
			VectorClear( RI.currententity->attachment[i] );
		return;
	}
	else if( m_pStudioHeader->numattachments > MAXSTUDIOATTACHMENTS )
	{
		m_pStudioHeader->numattachments = MAXSTUDIOATTACHMENTS; // reduce it
		MsgDev( D_WARN, "R_StudioCalcAttahments: too many attachments on %s\n", RI.currentmodel->name );
	}

	// calculate attachment points
	pAtt = (mstudioattachment_t *)((byte *)m_pStudioHeader + m_pStudioHeader->attachmentindex);

	for( i = 0; i < m_pStudioHeader->numattachments; i++ )
	{
		Matrix3x4_VectorTransform( g_lighttransform[pAtt[i].bone], pAtt[i].org, RI.currententity->attachment[i] );
		VectorSubtract( RI.currententity->attachment[i], RI.currententity->origin, localOrg );
		Matrix3x4_OriginFromMatrix( g_lighttransform[pAtt[i].bone], bonepos );
		VectorSubtract( localOrg, bonepos, forward );	// make forward
		VectorNormalizeFast( forward );
		VectorAngles( forward, localAng );
	}
}

/*
===============
pfnStudioSetupModel

===============
*/
static void R_StudioSetupModel( int bodypart, void **ppbodypart, void **ppsubmodel )
{
	int	index;

	if( bodypart > m_pStudioHeader->numbodyparts )
		bodypart = 0;

	m_pBodyPart = (mstudiobodyparts_t *)((byte *)m_pStudioHeader + m_pStudioHeader->bodypartindex) + bodypart;

	index = RI.currententity->curstate.body / m_pBodyPart->base;
	index = index % m_pBodyPart->nummodels;

	m_pSubModel = (mstudiomodel_t *)((byte *)m_pStudioHeader + m_pBodyPart->modelindex) + index;

	if( ppbodypart ) *ppbodypart = m_pBodyPart;
	if( ppsubmodel ) *ppsubmodel = m_pSubModel;
}

/*
===============
R_StudioCheckBBox

===============
*/
static int R_StudioCheckBBox( void )
{
	if( R_CullStudioModel( RI.currententity ))
		return false;
	return true;
}

/*
===============
R_StudioGetShadowImpactAndDir
===============
*/
void R_StudioGetShadowImpactAndDir( void )
{
	float		angle;
	vec3_t		skyAngles, origin, end;

	if( !cl.refdef.movevars ) return; // e.g. in menu

	// convert skyvec into angles then back into vector to avoid 0 0 0 direction
	VectorAngles( (float *)&cl.refdef.movevars->skyvec_x, skyAngles );
	angle = skyAngles[YAW] / 180 * M_PI;

	Matrix3x4_OriginFromMatrix( g_bonestransform[0], origin );
	SinCos( angle, &g_mvShadowVec[1], &g_mvShadowVec[0] );

	R_LightDir( origin, g_mvShadowVec, 256.0f );

	VectorSet( g_mvShadowVec, -g_mvShadowVec[0], -g_mvShadowVec[1], -1.0f );
	VectorNormalize( g_mvShadowVec );

	VectorMA( origin, 256.0f, g_mvShadowVec, end );

	g_shadowTrace = CL_TraceLine( origin, end, PM_WORLD_ONLY );
}

/*
===============
R_StudioDynamicLight

===============
*/
void R_StudioDynamicLight( cl_entity_t *ent, alight_t *lightinfo )
{
	uint		lnum, i;
	studiolight_t	*plight;
	qboolean		invLight;
	color24		ambient;
	float		dist, radius2;
	vec3_t		direction, origin;
	dlight_t		*dl;

	if( !lightinfo ) return;

	plight = &g_studiolight;
	plight->numdlights = 0;	// clear previous dlights

	if( r_studio_lighting->integer == 2 )
	{
		Matrix3x4_OriginFromMatrix( g_lighttransform[0], origin );

		// NOTE: in some cases bone origin may be stuck in the geometry
		// and produced completely black model. Run additional check for this case
		if( CL_PointContents( origin ) == CONTENTS_SOLID )
			Matrix3x4_OriginFromMatrix( g_rotationmatrix, origin );
	}
	else Matrix3x4_OriginFromMatrix( g_rotationmatrix, origin );

	// setup light dir
	if( cl.refdef.movevars )
	{
		// pre-defined light vector
		plight->lightvec[0] = cl.refdef.movevars->skyvec_x;
		plight->lightvec[1] = cl.refdef.movevars->skyvec_y;
		plight->lightvec[2] = cl.refdef.movevars->skyvec_z;
	}
	else VectorSet( plight->lightvec, 0.0f, 0.0f, -1.0f );

	if( VectorIsNull( plight->lightvec ))
		VectorSet( plight->lightvec, 0.0f, 0.0f, -1.0f );

	VectorCopy( plight->lightvec, lightinfo->plightvec );

	// setup ambient lighting
	invLight = (ent->curstate.effects & EF_INVLIGHT) ? true : false;
	R_LightForPoint( origin, &ambient, invLight, true, 0.0f ); // ignore dlights

	R_GetLightSpot( plight->lightspot );	// shadow stuff

	plight->lightcolor[0] = lightinfo->color[0] = ambient.r * (1.0f / 255.0f);
	plight->lightcolor[1] = lightinfo->color[1] = ambient.g * (1.0f / 255.0f);
	plight->lightcolor[2] = lightinfo->color[2] = ambient.b * (1.0f / 255.0f);

	VectorCopy( plight->lightcolor, lightinfo->color );
	lightinfo->shadelight = (ambient.r + ambient.g + ambient.b) / 3;
	lightinfo->ambientlight = lightinfo->shadelight;

	if( !ent || !ent->model || !r_dynamic->integer )
		return;

	for( lnum = 0, dl = cl_dlights; lnum < MAX_DLIGHTS; lnum++, dl++ )
	{
		if( dl->die < RI.refdef.time || !dl->radius )
			continue;

		VectorSubtract( dl->origin, origin, direction );
		dist = VectorLength( direction );

		if( !dist || dist > dl->radius + studio_radius )
			continue;

		radius2 = dl->radius * dl->radius; // squared radius

		for( i = 0; i < m_pStudioHeader->numbones; i++ )
		{
			vec3_t	vec, org;
			float	dist, atten;
				
			Matrix3x4_OriginFromMatrix( g_lighttransform[i], org );
			VectorSubtract( org, dl->origin, vec );
			
			dist = DotProduct( vec, vec );
			atten = (dist / radius2 - 1) * -1;
			if( atten < 0 ) atten = 0;
			dist = sqrt( dist );

			if( dist )
			{
				dist = 1.0f / dist;
				VectorScale( vec, dist, vec );
				lightinfo->ambientlight += atten;
				lightinfo->shadelight += atten;
			}
                                        
			Matrix3x4_VectorIRotate( g_lighttransform[i], vec, plight->dlightvec[plight->numdlights][i] );
			VectorScale( plight->dlightvec[plight->numdlights][i], atten, plight->dlightvec[plight->numdlights][i] );
		}

		plight->dlightcolor[plight->numdlights][0] = dl->color.r * (1.0f / 255.0f);
		plight->dlightcolor[plight->numdlights][1] = dl->color.g * (1.0f / 255.0f);
		plight->dlightcolor[plight->numdlights][2] = dl->color.b * (1.0f / 255.0f);
		plight->numdlights++;
	}

	// clamp lighting so it doesn't overbright as much
	if( lightinfo->ambientlight > 128 )
		lightinfo->ambientlight = 128;

	if( lightinfo->ambientlight + lightinfo->shadelight > 192 )
		lightinfo->shadelight = 192 - lightinfo->ambientlight;
}

/*
===============
pfnStudioEntityLight

===============
*/
void R_StudioEntityLight( alight_t *lightinfo )
{
	uint		lnum, i;
	studiolight_t	*plight;
	float		dist, radius2;
	vec3_t		direction, origin;
	cl_entity_t	*ent;
	dlight_t		*el;

	ent = RI.currententity;

	if( !ent || !ent->model || !r_dynamic->integer )
		return;

	plight = &g_studiolight;
	plight->numelights = 0;	// clear previous elights

	if( r_studio_lighting->integer == 2 )
	{
		Matrix3x4_OriginFromMatrix( g_lighttransform[0], origin );

		// NOTE: in some cases bone origin may be stuck in the geometry
		// and produced completely black model. Run additional check for this case
		if( CL_PointContents( origin ) == CONTENTS_SOLID )
			Matrix3x4_OriginFromMatrix( g_rotationmatrix, origin );
	}
	else Matrix3x4_OriginFromMatrix( g_rotationmatrix, origin );

	for( lnum = 0, el = cl_elights; lnum < MAX_ELIGHTS; lnum++, el++ )
	{
		if( el->die < RI.refdef.time || !el->radius )
			continue;

		VectorSubtract( el->origin, origin, direction );
		dist = VectorLength( direction );

		if( !dist || dist > el->radius + studio_radius )
			continue;

		radius2 = el->radius * el->radius; // squared radius

		for( i = 0; i < m_pStudioHeader->numbones; i++ )
		{
			vec3_t	vec, org;
			float	dist, atten;
				
			Matrix3x4_OriginFromMatrix( g_lighttransform[i], org );
			VectorSubtract( org, origin, vec );
			
			dist = DotProduct( vec, vec );
			atten = (dist / radius2 - 1) * -1;
			if( atten < 0 ) atten = 0;
			dist = sqrt( dist );

			if( dist )
			{
				dist = 1.0f / dist;
				VectorScale( vec, dist, vec );
				lightinfo->ambientlight += atten;
				lightinfo->shadelight += atten;
			}
                                        
			Matrix3x4_VectorIRotate( g_lighttransform[i], vec, plight->elightvec[plight->numelights][i] );
			VectorScale( plight->elightvec[plight->numelights][i], atten, plight->elightvec[plight->numelights][i] );
		}

		plight->elightcolor[plight->numelights][0] = el->color.r * (1.0f / 255.0f);
		plight->elightcolor[plight->numelights][1] = el->color.g * (1.0f / 255.0f);
		plight->elightcolor[plight->numelights][2] = el->color.b * (1.0f / 255.0f);
		plight->numelights++;
	}

	// clamp lighting so it doesn't overbright as much
	if( lightinfo->ambientlight > 128 )
		lightinfo->ambientlight = 128;

	if( lightinfo->ambientlight + lightinfo->shadelight > 192 )
		lightinfo->shadelight = 192 - lightinfo->ambientlight;
}

/*
===============
R_StudioSetupLighting

===============
*/
void R_StudioSetupLighting( alight_t *lightinfo )
{
	studiolight_t	*plight;
	int		i;

	if( !m_pStudioHeader || !lightinfo )
		return;

	plight = &g_studiolight; 

	for( i = 0; i < m_pStudioHeader->numbones; i++ )
		Matrix3x4_VectorIRotate( g_lighttransform[i], lightinfo->plightvec, plight->blightvec[i] );

	// copy custom alight color in case of mod-maker changed it
	plight->lightcolor[0] = lightinfo->color[0];
	plight->lightcolor[1] = lightinfo->color[1];
	plight->lightcolor[2] = lightinfo->color[2];

	R_StudioGetShadowImpactAndDir();
}

/*
===============
R_StudioLighting

===============
*/
void R_StudioLighting( float *lv, int bone, int flags, vec3_t normal )
{
	float		max, ambient;
	vec3_t		illum;
	studiolight_t	*plight;

	if( !RI.drawWorld || RI.currententity->curstate.effects & EF_FULLBRIGHT )
	{
		VectorSet( lv, 1.0f, 1.0f, 1.0f );
		return;
	}

	plight = &g_studiolight; 
	ambient = max( 0.1f, r_lighting_ambient->value ); // to avoid divison by zero
	VectorScale( plight->lightcolor, ambient, illum );

	if( flags & STUDIO_NF_FLATSHADE )
	{
		VectorMA( illum, 0.8f, plight->lightcolor, illum );
	}
          else
          {
		float	r, lightcos;
		int	i;

		lightcos = DotProduct( normal, plight->blightvec[bone] ); // -1 colinear, 1 opposite

		if( lightcos > 1.0f ) lightcos = 1;
		VectorAdd( illum, plight->lightcolor, illum );

		r = r_studio_lambert->value;
		if( r < 1.0f ) r = 1.0f;
		lightcos = (lightcos + ( r - 1.0f )) / r; // do modified hemispherical lighting
		if( lightcos > 0.0f ) VectorMA( illum, -lightcos, plight->lightcolor, illum );
		
		if( illum[0] <= 0.0f ) illum[0] = 0.0f;
		if( illum[1] <= 0.0f ) illum[1] = 0.0f;
		if( illum[2] <= 0.0f ) illum[2] = 0.0f;

		// now add all dynamic lights
		for( i = 0; i < plight->numdlights; i++)
		{
			lightcos = -DotProduct( normal, plight->dlightvec[i][bone] );
			if( lightcos > 0.0f ) VectorMA( illum, lightcos, plight->dlightcolor[i], illum );
		}

		// now add all entity lights
		for( i = 0; i < plight->numelights; i++)
		{
			lightcos = -DotProduct( normal, plight->elightvec[i][bone] );
			if( lightcos > 0.0f ) VectorMA( illum, lightcos, plight->elightcolor[i], illum );
		}
	}
	
	max = VectorMax( illum );

	if( max > 1.0f )
		VectorScale( illum, ( 1.0f / max ), lv );
	else VectorCopy( illum, lv ); 

}

/*
===============
R_StudioSetupTextureHeader

===============
*/
void R_StudioSetupTextureHeader( void )
{
#ifndef STUDIO_MERGE_TEXTURES
	if( !m_pStudioHeader->numtextures || !m_pStudioHeader->textureindex )
	{
		string	texturename;		
		model_t	*textures = NULL;

		Q_strncpy( texturename, R_StudioTexName( RI.currentmodel ), sizeof( texturename ));
		COM_FixSlashes( texturename );

		if( FS_FileExists( texturename, false ))
			textures = Mod_ForName( texturename, false );

		if( !textures )
		{
			m_pTextureHeader = NULL;
			return;
		}

		m_pTextureHeader = (studiohdr_t *)Mod_Extradata( textures );
	}
	else
#endif
	{
		m_pTextureHeader = m_pStudioHeader;
	}
}

/*
===============
R_StudioSetupSkin

===============
*/
static void R_StudioSetupSkin( mstudiotexture_t *ptexture, int index )
{
	short	*pskinref;
	int	m_skinnum;

	R_StudioSetupTextureHeader ();

	if( !m_pTextureHeader ) return;

	// NOTE: user can comment call StudioRemapColors and remap_info will be unavailable
	if( m_fDoRemap ) ptexture = CL_GetRemapInfoForEntity( RI.currententity )->ptexture;

	// safety bounding the skinnum
	m_skinnum = bound( 0, RI.currententity->curstate.skin, ( m_pTextureHeader->numskinfamilies - 1 ));
	pskinref = (short *)((byte *)m_pTextureHeader + m_pTextureHeader->skinindex);
	if( m_skinnum != 0 && m_skinnum < m_pTextureHeader->numskinfamilies )
		pskinref += (m_skinnum * m_pTextureHeader->numskinref);

	GL_Bind( GL_TEXTURE0, ptexture[pskinref[index]].index );
}

/*
===============
R_StudioGetTexture

Doesn't changes studio global state at all
===============
*/
mstudiotexture_t *R_StudioGetTexture( cl_entity_t *e )
{
	mstudiotexture_t	*ptexture;
	studiohdr_t	*phdr, *thdr;

	if(( phdr = Mod_Extradata( e->model )) == NULL )
		return NULL;

#ifndef STUDIO_MERGE_TEXTURES
	if( !m_pStudioHeader->numtextures || !m_pStudioHeader->textureindex )
	{
		string	texturename;		
		model_t	*textures = NULL;

		Q_strncpy( texturename, R_StudioTexName( RI.currentmodel ), sizeof( texturename ));
		COM_FixSlashes( texturename );

		if( FS_FileExists( texturename, false ))
			textures = Mod_ForName( texturename, false );

		if( !textures )
			return NULL;

		thdr = (studiohdr_t *)Mod_Extradata( textures );
	}
	else
#endif
	{
		thdr = m_pStudioHeader;
	}

	if( !thdr ) return NULL;	

	if( m_fDoRemap ) ptexture = CL_GetRemapInfoForEntity( e )->ptexture;
	else ptexture = (mstudiotexture_t *)((byte *)thdr + thdr->textureindex);

	return ptexture;
}

void R_StudioSetRenderamt( int iRenderamt )
{
	if( !RI.currententity ) return;

	RI.currententity->curstate.renderamt = iRenderamt;
	RI.currententity->curstate.renderamt = R_ComputeFxBlend( RI.currententity );
}

/*
===============
R_StudioSetCullState

sets true for enable backculling (for left-hand viewmodel)
===============
*/
void R_StudioSetCullState( int iCull )
{
	g_iBackFaceCull = iCull;
}

/*
===============
R_StudioRenderShadow

just a prefab for render shadow
===============
*/
void R_StudioRenderShadow( int iSprite, float *p1, float *p2, float *p3, float *p4 )
{
	if( !p1 || !p2 || !p3 || !p4 )
		return;

	if( TriSpriteTexture( Mod_Handle( iSprite ), 0 ))
	{
		pglEnable( GL_BLEND );
		pglDisable( GL_ALPHA_TEST );
		pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		pglColor4f( 0.0f, 0.0f, 0.0f, 1.0f ); // render only alpha

		pglBegin( GL_QUADS );
			pglTexCoord2f( 0.0f, 0.0f );
			pglVertex3fv( p1 );
			pglTexCoord2f( 0.0f, 1.0f );
			pglVertex3fv( p2 );
			pglTexCoord2f( 1.0f, 1.0f );
			pglVertex3fv( p3 );
			pglTexCoord2f( 1.0f, 0.0f );
			pglVertex3fv( p4 );
		pglEnd();

		pglDisable( GL_BLEND );
		pglDisable( GL_ALPHA_TEST );
	}
}

/*
===============
R_StudioMeshCompare

Sorting opaque entities by model type
===============
*/
static int R_StudioMeshCompare( const sortedmesh_t *a, const sortedmesh_t *b )
{
	if( a->flags & STUDIO_NF_ADDITIVE )
		return 1;

	if( a->flags & STUDIO_NF_TRANSPARENT )
		return -1;

	return 0;
}

/*
===============
R_StudioDrawPoints

===============
*/
static void R_StudioDrawPoints( void )
{
	int		i, j, m_skinnum;
	byte		*pvertbone;
	byte		*pnormbone;
	vec3_t		*pstudioverts;
	vec3_t		*pstudionorms;
	mstudiotexture_t	*ptexture;
	mstudiomesh_t	*pmesh;
	short		*pskinref;
	float		*av, *lv, *nv, scale;

	R_StudioSetupTextureHeader ();

	g_nNumArrayVerts = g_nNumArrayElems = 0;

	if( !m_pTextureHeader ) return;
	if( RI.currententity->curstate.renderfx == kRenderFxGlowShell )
		g_nStudioCount++;

	// safety bounding the skinnum
	m_skinnum = bound( 0, RI.currententity->curstate.skin, ( m_pTextureHeader->numskinfamilies - 1 ));	    
	pvertbone = ((byte *)m_pStudioHeader + m_pSubModel->vertinfoindex);
	pnormbone = ((byte *)m_pStudioHeader + m_pSubModel->norminfoindex);

	// NOTE: user can comment call StudioRemapColors and remap_info will be unavailable
	if( m_fDoRemap ) ptexture = CL_GetRemapInfoForEntity( RI.currententity )->ptexture;
	else ptexture = (mstudiotexture_t *)((byte *)m_pTextureHeader + m_pTextureHeader->textureindex);

	ASSERT( ptexture != NULL );

	pmesh = (mstudiomesh_t *)((byte *)m_pStudioHeader + m_pSubModel->meshindex);
	pstudioverts = (vec3_t *)((byte *)m_pStudioHeader + m_pSubModel->vertindex);
	pstudionorms = (vec3_t *)((byte *)m_pStudioHeader + m_pSubModel->normindex);

	pskinref = (short *)((byte *)m_pTextureHeader + m_pTextureHeader->skinindex);
	if( m_skinnum != 0 && m_skinnum < m_pTextureHeader->numskinfamilies )
		pskinref += (m_skinnum * m_pTextureHeader->numskinref);

	for( i = 0; i < m_pSubModel->numverts; i++ )
		Matrix3x4_VectorTransform( g_bonestransform[pvertbone[i]], pstudioverts[i], g_xformverts[i] );

	if( g_nForceFaceFlags & STUDIO_NF_CHROME )
	{
		scale = RI.currententity->curstate.renderamt * (1.0f / 255.0f);

		for( i = 0; i < m_pSubModel->numnorms; i++ )
			Matrix3x4_VectorRotate( g_bonestransform[pnormbone[i]], pstudionorms[i], g_xformnorms[i] );
	}

	lv = (float *)g_lightvalues;
	for( j = 0; j < m_pSubModel->nummesh; j++ ) 
	{
		g_nFaceFlags = ptexture[pskinref[pmesh[j].skinref]].flags;

		// fill in sortedmesh info
		g_sortedMeshes[j].mesh = &pmesh[j];
		g_sortedMeshes[j].flags = g_nFaceFlags;

		for( i = 0; i < pmesh[j].numnorms; i++, lv += 3, pstudionorms++, pnormbone++ )
		{
			R_StudioLighting( lv, *pnormbone, g_nFaceFlags, (float *)pstudionorms );

			if(( g_nFaceFlags & STUDIO_NF_CHROME ) || ( g_nForceFaceFlags & STUDIO_NF_CHROME ))
				R_StudioSetupChrome( g_chrome[(float (*)[3])lv - g_lightvalues], *pnormbone, (float *)pstudionorms );
		}
	}

	if( r_studio_sort_textures->integer )
	{
		// sort opaque and translucent for right results
		qsort( g_sortedMeshes, m_pSubModel->nummesh, sizeof( sortedmesh_t ), R_StudioMeshCompare );
	}

	for( j = 0; j < m_pSubModel->nummesh; j++ ) 
	{
		float	s, t, alpha;
		short	*ptricmds;

		pmesh = g_sortedMeshes[j].mesh;
		ptricmds = (short *)((byte *)m_pStudioHeader + pmesh->triindex);

		g_nFaceFlags = ptexture[pskinref[pmesh->skinref]].flags;
		s = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].width;
		t = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].height;

		if( g_iRenderMode != kRenderTransAdd )
			pglDepthMask( GL_TRUE );
		else pglDepthMask( GL_FALSE );

		// check bounds
		if( ptexture[pskinref[pmesh->skinref]].index < 0 || ptexture[pskinref[pmesh->skinref]].index > MAX_TEXTURES )
			ptexture[pskinref[pmesh->skinref]].index = tr.defaultTexture;

		if( g_nForceFaceFlags & STUDIO_NF_CHROME )
		{
			color24	*clr;
			clr = &RI.currententity->curstate.rendercolor;
			pglColor4ub( clr->r, clr->g, clr->b, 255 );
			alpha = 1.0f;
		}
		else if( g_nFaceFlags & STUDIO_NF_TRANSPARENT && R_StudioOpaque( RI.currententity ))
		{
			GL_SetRenderMode( kRenderTransAlpha );
			pglAlphaFunc( GL_GREATER, 0.0f );
			alpha = 1.0f;
		}
		else if( g_nFaceFlags & STUDIO_NF_ADDITIVE )
		{
			GL_SetRenderMode( kRenderTransAdd );
			alpha = RI.currententity->curstate.renderamt * (1.0f / 255.0f);
			pglBlendFunc( GL_SRC_ALPHA, GL_ONE );
			pglDepthMask( GL_FALSE );
		}
		else if( g_nFaceFlags & STUDIO_NF_ALPHA )
		{
			GL_SetRenderMode( kRenderTransTexture );
			alpha = RI.currententity->curstate.renderamt * (1.0f / 255.0f);
			pglDepthMask( GL_FALSE );
		}
		else
		{
			GL_SetRenderMode( g_iRenderMode );

			if( g_iRenderMode == kRenderNormal )
			{
				pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
				alpha = 1.0f;
			}
			else alpha = RI.currententity->curstate.renderamt * (1.0f / 255.0f);
		}

		if( !( g_nForceFaceFlags & STUDIO_NF_CHROME ))
		{
			GL_Bind( GL_TEXTURE0, ptexture[pskinref[pmesh->skinref]].index );
		}

		while( i = *( ptricmds++ ))
		{
			int	vertexState = 0;
			qboolean	tri_strip;

			if( i < 0 )
			{
				pglBegin( GL_TRIANGLE_FAN );
				tri_strip = false;
				i = -i;
			}
			else
			{
				pglBegin( GL_TRIANGLE_STRIP );
				tri_strip = true;
			}

			r_stats.c_studio_polys += (i - 2);

			for( ; i > 0; i--, ptricmds += 4 )
			{
				// build in indices
				if( vertexState++ < 3 )
				{
					g_xarrayelems[g_nNumArrayElems++] = g_nNumArrayVerts;
				}
				else if( tri_strip )
				{
					// flip triangles between clockwise and counter clockwise
					if( vertexState & 1 )
					{
						// draw triangle [n-2 n-1 n]
						g_xarrayelems[g_nNumArrayElems++] = g_nNumArrayVerts - 2;
						g_xarrayelems[g_nNumArrayElems++] = g_nNumArrayVerts - 1;
						g_xarrayelems[g_nNumArrayElems++] = g_nNumArrayVerts;
					}
					else
					{
						// draw triangle [n-1 n-2 n]
						g_xarrayelems[g_nNumArrayElems++] = g_nNumArrayVerts - 1;
						g_xarrayelems[g_nNumArrayElems++] = g_nNumArrayVerts - 2;
						g_xarrayelems[g_nNumArrayElems++] = g_nNumArrayVerts;
					}
				}
				else
				{
					// draw triangle fan [0 n-1 n]
					g_xarrayelems[g_nNumArrayElems++] = g_nNumArrayVerts - ( vertexState - 1 );
					g_xarrayelems[g_nNumArrayElems++] = g_nNumArrayVerts - 1;
					g_xarrayelems[g_nNumArrayElems++] = g_nNumArrayVerts;
				}

				if( g_nFaceFlags & STUDIO_NF_CHROME || ( g_nForceFaceFlags & STUDIO_NF_CHROME ))
					pglTexCoord2f( g_chrome[ptricmds[1]][0] * s, g_chrome[ptricmds[1]][1] * t );
				else pglTexCoord2f( ptricmds[2] * s, ptricmds[3] * t );

				if(!( g_nForceFaceFlags & STUDIO_NF_CHROME ))
                                        {
					if( g_iRenderMode == kRenderTransAdd )
					{
						pglColor4f( 1.0f, 1.0f, 1.0f, alpha );
					}
					else if( g_iRenderMode == kRenderTransColor )
					{
						color24	*clr;
						clr = &RI.currententity->curstate.rendercolor;
						pglColor4ub( clr->r, clr->g, clr->b, alpha * 255 );
					}
					else if( g_nFaceFlags & STUDIO_NF_FULLBRIGHT )
					{
						pglColor4f( 1.0f, 1.0f, 1.0f, alpha );
					}
					else
					{
						lv = (float *)g_lightvalues[ptricmds[1]];
						pglColor4f( lv[0], lv[1], lv[2], alpha );
					}
				}

				av = g_xformverts[ptricmds[0]];

				if( g_nForceFaceFlags & STUDIO_NF_CHROME )
				{
					vec3_t	scaled_vertex;
					nv = (float *)g_xformnorms[ptricmds[1]];
					VectorMA( av, scale, nv, scaled_vertex );
					pglVertex3fv( scaled_vertex );
				}
				else
				{
					pglVertex3f( av[0], av[1], av[2] );
					ASSERT( g_nNumArrayVerts < MAXARRAYVERTS ); 
					VectorCopy( av, g_xarrayverts[g_nNumArrayVerts] ); // store off vertex
					g_nNumArrayVerts++;
				}
			}
			pglEnd();
		}
	}

	// restore depthmask for next call StudioDrawPoints
	if( g_iRenderMode != kRenderTransAdd )
		pglDepthMask( GL_TRUE );
}

/*
===============
R_StudioDrawHulls

===============
*/
static void R_StudioDrawHulls( void )
{
	int	i, j;
	float	alpha;

	if( r_drawentities->integer == 4 )
		alpha = 0.5f;
	else alpha = 1.0f;

	pglDisable( GL_TEXTURE_2D );

	for( i = 0; i < m_pStudioHeader->numhitboxes; i++ )
	{
		mstudiobbox_t	*pbboxes = (mstudiobbox_t *)((byte *)m_pStudioHeader + m_pStudioHeader->hitboxindex);
		vec3_t		v[8], v2[8], bbmin, bbmax;

		VectorCopy( pbboxes[i].bbmin, bbmin );
		VectorCopy( pbboxes[i].bbmax, bbmax );

		v[0][0] = bbmin[0];
		v[0][1] = bbmax[1];
		v[0][2] = bbmin[2];

		v[1][0] = bbmin[0];
		v[1][1] = bbmin[1];
		v[1][2] = bbmin[2];

		v[2][0] = bbmax[0];
		v[2][1] = bbmax[1];
		v[2][2] = bbmin[2];

		v[3][0] = bbmax[0];
		v[3][1] = bbmin[1];
		v[3][2] = bbmin[2];

		v[4][0] = bbmax[0];
		v[4][1] = bbmax[1];
		v[4][2] = bbmax[2];

		v[5][0] = bbmax[0];
		v[5][1] = bbmin[1];
		v[5][2] = bbmax[2];

		v[6][0] = bbmin[0];
		v[6][1] = bbmax[1];
		v[6][2] = bbmax[2];

		v[7][0] = bbmin[0];
		v[7][1] = bbmin[1];
		v[7][2] = bbmax[2];

		Matrix3x4_VectorTransform( g_bonestransform[pbboxes[i].bone], v[0], v2[0] );
		Matrix3x4_VectorTransform( g_bonestransform[pbboxes[i].bone], v[1], v2[1] );
		Matrix3x4_VectorTransform( g_bonestransform[pbboxes[i].bone], v[2], v2[2] );
		Matrix3x4_VectorTransform( g_bonestransform[pbboxes[i].bone], v[3], v2[3] );
		Matrix3x4_VectorTransform( g_bonestransform[pbboxes[i].bone], v[4], v2[4] );
		Matrix3x4_VectorTransform( g_bonestransform[pbboxes[i].bone], v[5], v2[5] );
		Matrix3x4_VectorTransform( g_bonestransform[pbboxes[i].bone], v[6], v2[6] );
		Matrix3x4_VectorTransform( g_bonestransform[pbboxes[i].bone], v[7], v2[7] );

		j = (pbboxes[i].group % 8);

		// set properly color for hull
		pglColor4f( hullcolor[j][0], hullcolor[j][1], hullcolor[j][2], alpha );

		pglBegin( GL_QUAD_STRIP );
		for( j = 0; j < 10; j++ )
			pglVertex3fv( v2[j & 7] );
		pglEnd( );
	
		pglBegin( GL_QUAD_STRIP );
		pglVertex3fv( v2[6] );
		pglVertex3fv( v2[0] );
		pglVertex3fv( v2[4] );
		pglVertex3fv( v2[2] );
		pglEnd( );

		pglBegin( GL_QUAD_STRIP );
		pglVertex3fv( v2[1] );
		pglVertex3fv( v2[7] );
		pglVertex3fv( v2[3] );
		pglVertex3fv( v2[5] );
		pglEnd( );			
	}

	pglEnable( GL_TEXTURE_2D );
}

/*
===============
R_StudioDrawAbsBBox

===============
*/
static void R_StudioDrawAbsBBox( void )
{
	vec3_t	bbox[8];
	int	i;

	// looks ugly, skip
	if( RI.currententity == &clgame.viewent )
		return;

	if( !R_StudioComputeBBox( RI.currententity, bbox ))
		return;

	pglDisable( GL_TEXTURE_2D );
	pglDisable( GL_DEPTH_TEST );

	pglColor4f( 1.0f, 0.0f, 0.0f, 1.0f );	// red bboxes for studiomodels
	pglBegin( GL_LINES );

	for( i = 0; i < 2; i += 1 )
	{
		pglVertex3fv( bbox[i+0] );
		pglVertex3fv( bbox[i+2] );
		pglVertex3fv( bbox[i+4] );
		pglVertex3fv( bbox[i+6] );
		pglVertex3fv( bbox[i+0] );
		pglVertex3fv( bbox[i+4] );
		pglVertex3fv( bbox[i+2] );
		pglVertex3fv( bbox[i+6] );
		pglVertex3fv( bbox[i*2+0] );
		pglVertex3fv( bbox[i*2+1] );
		pglVertex3fv( bbox[i*2+4] );
		pglVertex3fv( bbox[i*2+5] );
	}

	pglEnd();
	pglEnable( GL_TEXTURE_2D );
	pglEnable( GL_DEPTH_TEST );
}

/*
===============
R_StudioDrawBones

===============
*/
static void R_StudioDrawBones( void )
{
	mstudiobone_t	*pbones = (mstudiobone_t *) ((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);
	vec3_t		point;
	int		i;

	pglDisable( GL_TEXTURE_2D );

	for( i = 0; i < m_pStudioHeader->numbones; i++ )
	{
		if( pbones[i].parent >= 0 )
		{
			pglPointSize( 3.0f );
			pglColor3f( 1, 0.7f, 0 );
			pglBegin( GL_LINES );
			
			Matrix3x4_OriginFromMatrix( g_bonestransform[pbones[i].parent], point );
			pglVertex3fv( point );
			Matrix3x4_OriginFromMatrix( g_bonestransform[i], point );
			pglVertex3fv( point );
			
			pglEnd();

			pglColor3f( 0, 0, 0.8f );
			pglBegin( GL_POINTS );
			if( pbones[pbones[i].parent].parent != -1 )
			{
				Matrix3x4_OriginFromMatrix( g_bonestransform[pbones[i].parent], point );
				pglVertex3fv( point );
			}
			Matrix3x4_OriginFromMatrix( g_bonestransform[i], point );
			pglVertex3fv( point );
			pglEnd();
		}
		else
		{
			// draw parent bone node
			pglPointSize( 5.0f );
			pglColor3f( 0.8f, 0, 0 );
			pglBegin( GL_POINTS );
			Matrix3x4_OriginFromMatrix( g_bonestransform[i], point );
			pglVertex3fv( point );
			pglEnd();
		}
	}

	pglPointSize( 1.0f );
	pglEnable( GL_TEXTURE_2D );
}

static void R_StudioDrawAttachments( void )
{
	int	i;
	
	pglDisable( GL_TEXTURE_2D );
	pglDisable( GL_DEPTH_TEST );
	
	for( i = 0; i < m_pStudioHeader->numattachments; i++ )
	{
		mstudioattachment_t	*pattachments;
		vec3_t		v[4];

		pattachments = (mstudioattachment_t *)((byte *)m_pStudioHeader + m_pStudioHeader->attachmentindex);		
		Matrix3x4_VectorTransform( g_bonestransform[pattachments[i].bone], pattachments[i].org, v[0] );
		Matrix3x4_VectorTransform( g_bonestransform[pattachments[i].bone], pattachments[i].vectors[0], v[1] );
		Matrix3x4_VectorTransform( g_bonestransform[pattachments[i].bone], pattachments[i].vectors[1], v[2] );
		Matrix3x4_VectorTransform( g_bonestransform[pattachments[i].bone], pattachments[i].vectors[2], v[3] );
		
		pglBegin( GL_LINES );
		pglColor3f( 1, 0, 0 );
		pglVertex3fv( v[0] );
		pglColor3f( 1, 1, 1 );
		pglVertex3fv (v[1] );
		pglColor3f( 1, 0, 0 );
		pglVertex3fv (v[0] );
		pglColor3f( 1, 1, 1 );
		pglVertex3fv (v[2] );
		pglColor3f( 1, 0, 0 );
		pglVertex3fv (v[0] );
		pglColor3f( 1, 1, 1 );
		pglVertex3fv( v[3] );
		pglEnd();

		pglPointSize( 5.0f );
		pglColor3f( 0, 1, 0 );
		pglBegin( GL_POINTS );
		pglVertex3fv( v[0] );
		pglEnd();
		pglPointSize( 1.0f );
	}

	pglEnable( GL_TEXTURE_2D );
	pglEnable( GL_DEPTH_TEST );
}

/*
===============
R_StudioSetRemapColors

===============
*/
void R_StudioSetRemapColors( int newTop, int newBottom )
{
	// update colors for viewentity
	if( RI.currententity == &clgame.viewent )
	{
		player_info_t	*pLocalPlayer;

		// copy top and bottom colors for viewmodel
		if(( pLocalPlayer = pfnPlayerInfo( clgame.viewent.curstate.number - 1 )) != NULL )
		{
			newTop = bound( 0, pLocalPlayer->topcolor, 360 );
			newBottom = bound( 0, pLocalPlayer->bottomcolor, 360 );
		}
	}

	CL_AllocRemapInfo( newTop, newBottom );

	if( CL_GetRemapInfoForEntity( RI.currententity ))
	{
		CL_UpdateRemapInfo( newTop, newBottom );
		m_fDoRemap = true;
	}
}

/*
===============
R_StudioSetupPlayerModel

===============
*/
static model_t *R_StudioSetupPlayerModel( int index )
{
	player_info_t	*info;
	string		modelpath;

	if( cls.key_dest == key_menu && !index )
	{
		// we are in menu.
		info = &menu.playerinfo;
	}
	else
	{
		if( index < 0 || index > cl.maxclients )
			return NULL; // bad client ?
		info = &cl.players[index];
	}

	// set to invisible, skip
	if( !info->model[0] ) return NULL;

	if( !Q_stricmp( info->model, "player" ))
	{
		Q_strncpy( modelpath, "models/player.mdl", sizeof( modelpath ));
	}
	else
	{
		Q_snprintf( modelpath, sizeof( modelpath ), "models/player/%s/%s.mdl", info->model, info->model );

		// replace with default if missed
		if( !FS_FileExists( modelpath, false ))
			Q_strncpy( modelpath, "models/player.mdl", sizeof( modelpath ));
	}

	if( !FS_FileExists( modelpath, false ))
		return NULL;

	return Mod_ForName( modelpath, false );
}

/*
===============
R_StudioClientEvents

===============
*/
static void R_StudioClientEvents( void )
{
	mstudioseqdesc_t	*pseqdesc;
	mstudioevent_t	*pevent;
	cl_entity_t	*e = RI.currententity;
	float		f, start;
	int		i;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + e->curstate.sequence;
	pevent = (mstudioevent_t *)((byte *)m_pStudioHeader + pseqdesc->eventindex);

	// no events for this animation or gamepaused
	if( pseqdesc->numevents == 0 || cl.time == cl.oldtime )
		return;

	f = R_StudioEstimateFrame( e, pseqdesc ) + 0.01f;	// get start offset
	start = f - e->curstate.framerate * host.frametime * pseqdesc->fps;

	for( i = 0; i < pseqdesc->numevents; i++ )
	{
		// ignore all non-client-side events
		if( pevent[i].event < EVENT_CLIENT )
			continue;

		if( (float)pevent[i].frame > start && f >= (float)pevent[i].frame )
			clgame.dllFuncs.pfnStudioEvent( &pevent[i], e );
	}
}

/*
===============
R_StudioGetForceFaceFlags

===============
*/
int R_StudioGetForceFaceFlags( void )
{
	return g_nForceFaceFlags;
}

/*
===============
R_StudioSetForceFaceFlags

===============
*/
void R_StudioSetForceFaceFlags( int flags )
{
	g_nForceFaceFlags = flags;
}

/*
===============
pfnStudioSetHeader

===============
*/
void R_StudioSetHeader( studiohdr_t *pheader )
{
	m_pStudioHeader = pheader;

	VectorClear( g_chrome_origin );
	m_fDoRemap = false;
}

/*
===============
R_StudioSetRenderModel

===============
*/
void R_StudioSetRenderModel( model_t *model )
{
	RI.currentmodel = model;
}

/*
===============
R_StudioSetupRenderer

===============
*/
static void R_StudioSetupRenderer( int rendermode )
{
	g_iRenderMode = bound( 0, rendermode, kRenderTransAdd );
	pglShadeModel( GL_SMOOTH );	// enable gouraud shading
	if( clgame.ds.cullMode != GL_NONE ) GL_Cull( GL_FRONT );

	// enable depthmask on studiomodels
	if( glState.drawTrans && g_iRenderMode != kRenderTransAdd )
		pglDepthMask( GL_TRUE );

	pglAlphaFunc( GL_GREATER, 0.0f );

	if( g_iBackFaceCull )
		GL_FrontFace( true );
}

/*
===============
R_StudioRestoreRenderer

===============
*/
static void R_StudioRestoreRenderer( void )
{
	pglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	pglShadeModel( GL_FLAT );

	// restore depthmask state for sprites etc
	if( glState.drawTrans && g_iRenderMode != kRenderTransAdd )
		pglDepthMask( GL_FALSE );
	else pglDepthMask( GL_TRUE );

	if( g_iBackFaceCull )
		GL_FrontFace( false );

	g_iBackFaceCull = false;
	m_fDoRemap = false;
}

/*
===============
R_StudioSetChromeOrigin

===============
*/
void R_StudioSetChromeOrigin( void )
{
	VectorNegate( RI.vieworg, g_chrome_origin );
}

/*
===============
pfnIsHardware

Xash3D is always works in hardware mode
===============
*/
static int pfnIsHardware( void )
{
	return 1;	// 0 is Software, 1 is OpenGL, 2 is Direct3D
}

/*
===============
R_StudioDeformShadow

Deform vertices by specified lightdir
===============
*/
void R_StudioDeformShadow( void )
{
	float		*verts, dist, dist2;
	int		numVerts;

	dist = g_shadowTrace.plane.dist + 1.0f;
	dist2 = -1.0f / DotProduct( g_mvShadowVec, g_shadowTrace.plane.normal );
	VectorScale( g_mvShadowVec, dist2, g_mvShadowVec );

	verts = g_xarrayverts[0];
	numVerts = g_nNumArrayVerts;

	for( numVerts = 0; numVerts < g_nNumArrayVerts; numVerts++, verts += 3 )
	{
		dist2 = DotProduct( verts, g_shadowTrace.plane.normal ) - dist;
		if( dist2 > 0.0f ) VectorMA( verts, dist2, g_mvShadowVec, verts );
	}
}

static void R_StudioDrawPlanarShadow( void )
{
	if( RI.currententity->curstate.effects & EF_NOSHADOW )
		return;

	R_StudioDeformShadow ();

	if( glState.stencilEnabled )
		pglEnable( GL_STENCIL_TEST );

	pglEnableClientState( GL_VERTEX_ARRAY );
	pglVertexPointer( 3, GL_FLOAT, 12, g_xarrayverts );

	if( GL_Support( GL_DRAW_RANGEELEMENTS_EXT ))
		pglDrawRangeElementsEXT( GL_TRIANGLES, 0, g_nNumArrayVerts, g_nNumArrayElems, GL_UNSIGNED_INT, g_xarrayelems );
	else pglDrawElements( GL_TRIANGLES, g_nNumArrayElems, GL_UNSIGNED_INT, g_xarrayelems );

	if( glState.stencilEnabled )
		pglDisable( GL_STENCIL_TEST );

	pglDisableClientState( GL_VERTEX_ARRAY );
}
	
/*
===============
GL_StudioDrawShadow

NOTE: this code sucessfully working with ShadowHack only in Release build
===============
*/
static void GL_StudioDrawShadow( void )
{
	int	rendermode;
	float	shadow_alpha;
	float	shadow_alpha2;
	GLenum	depthmode;
	GLenum	depthmode2;

	pglDepthMask( GL_TRUE );

	if( r_shadows.value != 0.0f )
	{
		if( RI.currententity->baseline.movetype != MOVETYPE_FLY )
		{
			rendermode = RI.currententity->baseline.rendermode;

			if( rendermode == kRenderNormal && RI.currententity != &clgame.viewent )
			{
				shadow_alpha = 1.0f - r_shadowalpha.value * 0.5f;
				pglDisable( GL_TEXTURE_2D );
				pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				pglEnable( GL_BLEND );
				pglShadeModel( GL_FLAT );
				shadow_alpha2 = 1.0f - shadow_alpha;

				pglColor4f( 0.0f, 0.0f, 0.0f, shadow_alpha2 );

				depthmode = GL_LESS;
				pglDepthFunc( depthmode );

				R_StudioDrawPlanarShadow();

				depthmode2 = GL_LEQUAL;
				pglDepthFunc( depthmode2 );

				pglEnable( GL_TEXTURE_2D );
				pglDisable( GL_BLEND );

				pglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
				pglShadeModel( GL_SMOOTH );
			}
		}
	}
}

/*
====================
StudioRenderFinal

====================
*/
void R_StudioRenderFinal( void )
{
	int	i, rendermode;

	rendermode = R_StudioGetForceFaceFlags() ? kRenderTransAdd : RI.currententity->curstate.rendermode;
	R_StudioSetupRenderer( rendermode );
	
	if( r_drawentities->integer == 2 )
	{
		R_StudioDrawBones();
	}
	else if( r_drawentities->integer == 3 )
	{
		R_StudioDrawHulls();
	}
	else
	{
		for( i = 0; i < m_pStudioHeader->numbodyparts; i++ )
		{
			R_StudioSetupModel( i, &m_pBodyPart, &m_pSubModel );

			GL_SetRenderMode( rendermode );
			R_StudioDrawPoints();
			GL_StudioDrawShadow();
		}
	}

	if( r_drawentities->integer == 4 )
	{
		GL_SetRenderMode( kRenderTransAdd );
		R_StudioDrawHulls( );
		GL_SetRenderMode( kRenderNormal );
	}

	if( r_drawentities->integer == 5 )
	{
		R_StudioDrawAbsBBox( );
	}

	if( r_drawentities->integer == 6 )
	{
		R_StudioDrawAttachments();
	}

	R_StudioRestoreRenderer();
}

/*
====================
StudioRenderModel

====================
*/
void R_StudioRenderModel( void )
{
	R_StudioSetChromeOrigin();
	R_StudioSetForceFaceFlags( 0 );

	if( RI.currententity->curstate.renderfx == kRenderFxGlowShell )
	{
		RI.currententity->curstate.renderfx = kRenderFxNone;

		R_StudioRenderFinal( );

		R_StudioSetForceFaceFlags( STUDIO_NF_CHROME );
		TriSpriteTexture( R_GetChromeSprite(), 0 );
		RI.currententity->curstate.renderfx = kRenderFxGlowShell;

		R_StudioRenderFinal( );
	}
	else
	{
		R_StudioRenderFinal( );
	}
}

/*
====================
StudioEstimateGait

====================
*/
void R_StudioEstimateGait( entity_state_t *pplayer )
{
	vec3_t	est_velocity;
	float	dt;

	dt = bound( 0.0f, (RI.refdef.time - cl.oldtime), 1.0f );

	if( dt == 0.0f || m_pPlayerInfo->renderframe == tr.framecount )
	{
		m_flGaitMovement = 0;
		return;
	}

	VectorSubtract( RI.currententity->origin, m_pPlayerInfo->prevgaitorigin, est_velocity );
	VectorCopy( RI.currententity->origin, m_pPlayerInfo->prevgaitorigin );
	m_flGaitMovement = VectorLength( est_velocity );

	if( dt <= 0.0f || m_flGaitMovement / dt < 5.0f )
	{
		m_flGaitMovement = 0.0f;
		est_velocity[0] = 0.0f;
		est_velocity[1] = 0.0f;
	}

	if( est_velocity[1] == 0.0f && est_velocity[0] == 0.0f )
	{
		float	flYawDiff = RI.currententity->angles[YAW] - m_pPlayerInfo->gaityaw;

		flYawDiff = flYawDiff - (int)(flYawDiff / 360) * 360;
		if( flYawDiff > 180.0f ) flYawDiff -= 360.0f;
		if( flYawDiff < -180.0f ) flYawDiff += 360.0f;

		if( dt < 0.25f )
			flYawDiff *= dt * 4.0f;
		else flYawDiff *= dt;

		m_pPlayerInfo->gaityaw += flYawDiff;
		m_pPlayerInfo->gaityaw = m_pPlayerInfo->gaityaw - (int)(m_pPlayerInfo->gaityaw / 360) * 360;

		m_flGaitMovement = 0.0f;
	}
	else
	{
		m_pPlayerInfo->gaityaw = ( atan2( est_velocity[1], est_velocity[0] ) * 180 / M_PI );
		if( m_pPlayerInfo->gaityaw > 180.0f ) m_pPlayerInfo->gaityaw = 180.0f;
		if( m_pPlayerInfo->gaityaw < -180.0f ) m_pPlayerInfo->gaityaw = -180.0f;
	}

}

/*
====================
StudioProcessGait

====================
*/
void R_StudioProcessGait( entity_state_t *pplayer )
{
	mstudioseqdesc_t	*pseqdesc;
	int		iBlend;
	float		dt, flYaw; // view direction relative to movement

	if( RI.currententity->curstate.sequence >= m_pStudioHeader->numseq ) 
		RI.currententity->curstate.sequence = 0;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + RI.currententity->curstate.sequence;

	R_StudioPlayerBlend( pseqdesc, &iBlend, &RI.currententity->angles[PITCH] );

	RI.currententity->latched.prevangles[PITCH] = RI.currententity->angles[PITCH];
	RI.currententity->curstate.blending[0] = iBlend;
	RI.currententity->latched.prevblending[0] = RI.currententity->curstate.blending[0];
	RI.currententity->latched.prevseqblending[0] = RI.currententity->curstate.blending[0];

	dt = bound( 0.0f, (RI.refdef.time - cl.oldtime), 1.0f );
	R_StudioEstimateGait( pplayer );

	// calc side to side turning
	flYaw = RI.currententity->angles[YAW] - m_pPlayerInfo->gaityaw;
	flYaw = flYaw - (int)(flYaw / 360) * 360;
	if( flYaw < -180.0f ) flYaw = flYaw + 360.0f;
	if( flYaw > 180.0f ) flYaw = flYaw - 360.0f;

	if( flYaw > 120.0f )
	{
		m_pPlayerInfo->gaityaw = m_pPlayerInfo->gaityaw - 180.0f;
		m_flGaitMovement = -m_flGaitMovement;
		flYaw = flYaw - 180.0f;
	}
	else if( flYaw < -120.0f )
	{
		m_pPlayerInfo->gaityaw = m_pPlayerInfo->gaityaw + 180.0f;
		m_flGaitMovement = -m_flGaitMovement;
		flYaw = flYaw + 180.0f;
	}

	// adjust torso
	RI.currententity->curstate.controller[0] = ((flYaw / 4.0f) + 30.0f) / (60.0f / 255.0f);
	RI.currententity->curstate.controller[1] = ((flYaw / 4.0f) + 30.0f) / (60.0f / 255.0f);
	RI.currententity->curstate.controller[2] = ((flYaw / 4.0f) + 30.0f) / (60.0f / 255.0f);
	RI.currententity->curstate.controller[3] = ((flYaw / 4.0f) + 30.0f) / (60.0f / 255.0f);
	RI.currententity->latched.prevcontroller[0] = RI.currententity->curstate.controller[0];
	RI.currententity->latched.prevcontroller[1] = RI.currententity->curstate.controller[1];
	RI.currententity->latched.prevcontroller[2] = RI.currententity->curstate.controller[2];
	RI.currententity->latched.prevcontroller[3] = RI.currententity->curstate.controller[3];

	RI.currententity->angles[YAW] = m_pPlayerInfo->gaityaw;
	if( RI.currententity->angles[YAW] < -0 ) RI.currententity->angles[YAW] += 360.0f;
	RI.currententity->latched.prevangles[YAW] = RI.currententity->angles[YAW];

	if( pplayer->gaitsequence >= m_pStudioHeader->numseq ) 
		pplayer->gaitsequence = 0;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + pplayer->gaitsequence;

	// calc gait frame
	if( pseqdesc->linearmovement[0] > 0 )
		m_pPlayerInfo->gaitframe += (m_flGaitMovement / pseqdesc->linearmovement[0]) * pseqdesc->numframes;
	else m_pPlayerInfo->gaitframe += pseqdesc->fps * dt;

	// do modulo
	m_pPlayerInfo->gaitframe = m_pPlayerInfo->gaitframe - (int)(m_pPlayerInfo->gaitframe / pseqdesc->numframes) * pseqdesc->numframes;
	if( m_pPlayerInfo->gaitframe < 0 ) m_pPlayerInfo->gaitframe += pseqdesc->numframes;
}

/*
===============
R_StudioDrawPlayer

===============
*/
static int R_StudioDrawPlayer( int flags, entity_state_t *pplayer )
{
	int	m_nPlayerIndex;
	float	gaitframe, gaityaw;
	vec3_t	dir, prevgaitorigin;
	alight_t	lighting;

	m_nPlayerIndex = pplayer->number - 1;

	if( m_nPlayerIndex < 0 || m_nPlayerIndex >= cl.maxclients )
		return 0;

	RI.currentmodel = R_StudioSetupPlayerModel( m_nPlayerIndex );
	if( RI.currentmodel == NULL )
		return 0;

	R_StudioSetHeader((studiohdr_t *)Mod_Extradata( RI.currentmodel ));

	if( !RP_NORMALPASS() )
	{
		m_pPlayerInfo = pfnPlayerInfo( m_nPlayerIndex );
		VectorCopy( m_pPlayerInfo->prevgaitorigin, prevgaitorigin );
		gaitframe = m_pPlayerInfo->gaitframe;
		gaityaw = m_pPlayerInfo->gaityaw;
		m_pPlayerInfo = NULL;
	}

	if( pplayer->gaitsequence )
	{
		vec3_t orig_angles;

		m_pPlayerInfo = pfnPlayerInfo( m_nPlayerIndex );
		VectorCopy( RI.currententity->angles, orig_angles );
	
		R_StudioProcessGait( pplayer );

		m_pPlayerInfo->gaitsequence = pplayer->gaitsequence;
		m_pPlayerInfo = NULL;

		R_StudioSetUpTransform( RI.currententity );
		VectorCopy( orig_angles, RI.currententity->angles );
	}
	else
	{
		RI.currententity->curstate.controller[0] = 127;
		RI.currententity->curstate.controller[1] = 127;
		RI.currententity->curstate.controller[2] = 127;
		RI.currententity->curstate.controller[3] = 127;
		RI.currententity->latched.prevcontroller[0] = RI.currententity->curstate.controller[0];
		RI.currententity->latched.prevcontroller[1] = RI.currententity->curstate.controller[1];
		RI.currententity->latched.prevcontroller[2] = RI.currententity->curstate.controller[2];
		RI.currententity->latched.prevcontroller[3] = RI.currententity->curstate.controller[3];
		
		m_pPlayerInfo = pfnPlayerInfo( m_nPlayerIndex );
		m_pPlayerInfo->gaitsequence = 0;

		R_StudioSetUpTransform( RI.currententity );
	}

	if( flags & STUDIO_RENDER )
	{
		// see if the bounding box lets us trivially reject, also sets
		if( !R_StudioCheckBBox( ))
		{
			if( !RP_NORMALPASS() )
			{
				m_pPlayerInfo = pfnPlayerInfo( m_nPlayerIndex );
				VectorCopy( prevgaitorigin, m_pPlayerInfo->prevgaitorigin );
				m_pPlayerInfo->gaitframe = gaitframe;
				m_pPlayerInfo->gaityaw = gaityaw;
				m_pPlayerInfo = NULL;
			}
			return 0;
		}

		r_stats.c_studio_models_drawn++;
		g_nStudioCount++; // render data cache cookie

		if( m_pStudioHeader->numbodyparts == 0 )
			return 1;
	}

	m_pPlayerInfo = pfnPlayerInfo( m_nPlayerIndex );
	R_StudioSetupBones( RI.currententity );
	R_StudioSaveBones( );

	m_pPlayerInfo->renderframe = tr.framecount;
	m_pPlayerInfo = NULL;

	if( flags & STUDIO_EVENTS )
	{
		R_StudioCalcAttachments( );
		R_StudioClientEvents( );

		// copy attachments into global entity array
		if( RI.currententity->index > 0 )
		{
			cl_entity_t *ent = CL_GetEntityByIndex( RI.currententity->index );
			Q_memcpy( ent->attachment, RI.currententity->attachment, sizeof( vec3_t ) * 4 );
		}
	}

	if( flags & STUDIO_RENDER )
	{
		if( cl_himodels->integer && RI.currentmodel != RI.currententity->model  )
		{
			// show highest resolution multiplayer model
			RI.currententity->curstate.body = 255;
		}

		if(!( host.developer == 0 && cl.maxclients == 1 ) && ( RI.currentmodel == RI.currententity->model ))
		{
			RI.currententity->curstate.body = 1; // force helmet
		}

		lighting.plightvec = dir;
		R_StudioDynamicLight( RI.currententity, &lighting );

		R_StudioEntityLight( &lighting );

		// model and frame independant
		R_StudioSetupLighting( &lighting );

		m_pPlayerInfo = pfnPlayerInfo( m_nPlayerIndex );

		// get remap colors
		g_nTopColor = m_pPlayerInfo->topcolor;
		g_nBottomColor = m_pPlayerInfo->bottomcolor;

		if( g_nTopColor < 0 ) g_nTopColor = 0;
		if( g_nTopColor > 360 ) g_nTopColor = 360;
		if( g_nBottomColor < 0 ) g_nBottomColor = 0;
		if( g_nBottomColor > 360 ) g_nBottomColor = 360;

		R_StudioSetRemapColors( g_nTopColor, g_nBottomColor );

		R_StudioRenderModel( );
		m_pPlayerInfo = NULL;

		if( pplayer->weaponmodel )
		{
			cl_entity_t	saveent = *RI.currententity;
			model_t		*pweaponmodel = Mod_Handle( pplayer->weaponmodel );

			m_pStudioHeader = (studiohdr_t *)Mod_Extradata( pweaponmodel );

			R_StudioMergeBones( RI.currententity, pweaponmodel );
			R_StudioSetupLighting( &lighting );
			R_StudioRenderModel( );
			R_StudioCalcAttachments( );

			*RI.currententity = saveent;
		}
	}

	if( !RP_NORMALPASS() )
	{
		m_pPlayerInfo = pfnPlayerInfo( m_nPlayerIndex );
		VectorCopy( prevgaitorigin, m_pPlayerInfo->prevgaitorigin );
		m_pPlayerInfo->gaitframe = gaitframe;
		m_pPlayerInfo->gaityaw = gaityaw;
		m_pPlayerInfo = NULL;
	}

	return 1;
}

/*
===============
R_StudioDrawModel

===============
*/
static int R_StudioDrawModel( int flags )
{
	alight_t	lighting;
	vec3_t	dir;

	if( RI.currententity->curstate.renderfx == kRenderFxDeadPlayer )
	{
		entity_state_t	deadplayer;
		int		save_interp;
		int		result;

		if( RI.currententity->curstate.renderamt <= 0 || RI.currententity->curstate.renderamt > cl.maxclients )
			return 0;

		// get copy of player
		deadplayer = *R_StudioGetPlayerState( RI.currententity->curstate.renderamt - 1 );

		// clear weapon, movement state
		deadplayer.number = RI.currententity->curstate.renderamt;
		deadplayer.weaponmodel = 0;
		deadplayer.gaitsequence = 0;

		deadplayer.movetype = MOVETYPE_NONE;
		VectorCopy( RI.currententity->curstate.angles, deadplayer.angles );
		VectorCopy( RI.currententity->curstate.origin, deadplayer.origin );

		save_interp = m_fDoInterp;
		m_fDoInterp = 0;
		
		// draw as though it were a player
		result = R_StudioDrawPlayer( flags, &deadplayer );
		
		m_fDoInterp = save_interp;
		return result;
	}

	R_StudioSetHeader((studiohdr_t *)Mod_Extradata( RI.currentmodel ));

	R_StudioSetUpTransform( RI.currententity );

	if( flags & STUDIO_RENDER )
	{
		// see if the bounding box lets us trivially reject, also sets
		if( !R_StudioCheckBBox( ))
			return 0;

		r_stats.c_studio_models_drawn++;
		g_nStudioCount++; // render data cache cookie

		if( m_pStudioHeader->numbodyparts == 0 )
			return 1;
	}

	if( RI.currententity->curstate.movetype == MOVETYPE_FOLLOW )
		R_StudioMergeBones( RI.currententity, RI.currentmodel );
	else R_StudioSetupBones( RI.currententity );

	R_StudioSaveBones();

	if( flags & STUDIO_EVENTS )
	{
		R_StudioCalcAttachments( );
		R_StudioClientEvents( );

		// copy attachments into global entity array
		if( RI.currententity->index > 0 )
		{
			cl_entity_t *ent = CL_GetEntityByIndex( RI.currententity->index );
			Q_memcpy( ent->attachment, RI.currententity->attachment, sizeof( vec3_t ) * 4 );
		}
	}

	if( flags & STUDIO_RENDER )
	{
		lighting.plightvec = dir;
		R_StudioDynamicLight( RI.currententity, &lighting );

		R_StudioEntityLight( &lighting );

		// model and frame independant
		R_StudioSetupLighting( &lighting );

		// get remap colors
		g_nTopColor = RI.currententity->curstate.colormap & 0xFF;
		g_nBottomColor = (RI.currententity->curstate.colormap & 0xFF00) >> 8;

		R_StudioSetRemapColors( g_nTopColor, g_nBottomColor );

		R_StudioRenderModel();
	}

	return 1;
}

/*
=================
R_DrawStudioModel
=================
*/
void R_DrawStudioModelInternal( cl_entity_t *e, qboolean follow_entity )
{
	int	i, flags, result;
	float	prevFrame;

	if( RI.params & RP_ENVVIEW )
		return;

	if( !Mod_Extradata( e->model ))
		return;

	ASSERT( pStudioDraw != NULL );

	if( e == &clgame.viewent )
		flags = STUDIO_RENDER;	
	else flags = STUDIO_RENDER|STUDIO_EVENTS;

	if( e == &clgame.viewent )
		m_fDoInterp = true;	// viewmodel can't properly animate without lerping
	else if( r_studio_lerping->integer )
		m_fDoInterp = (e->curstate.effects & EF_NOINTERP) ? false : true;
	else m_fDoInterp = false;

	prevFrame = e->latched.prevframe;

	// prevent to crash some mods like HLFX in menu Customize
	if( !RI.drawWorld && !r_customdraw_playermodel->integer )
	{
		if( e->player )
			result = R_StudioDrawPlayer( flags, &e->curstate );
		else result = R_StudioDrawModel( flags );
	}
	else
	{
		// select the properly method
		if( e->player )
			result = pStudioDraw->StudioDrawPlayer( flags, &e->curstate );
		else result = pStudioDraw->StudioDrawModel( flags );
	}

	// old frame must be restored
	if( !RP_NORMALPASS( )) e->latched.prevframe = prevFrame;

	if( !result || follow_entity ) return;

	// NOTE: we must draw all followed entities
	// immediately after drawing parent when cached bones is valid
	for( i = 0; i < tr.num_child_entities; i++ )
	{
		if( CL_GetEntityByIndex( tr.child_entities[i]->curstate.aiment ) == e )
		{
			// copy the parent origin for right frustum culling
			VectorCopy( e->origin, tr.child_entities[i]->origin );

			RI.currententity = tr.child_entities[i];
			RI.currentmodel = RI.currententity->model;
			R_DrawStudioModelInternal( RI.currententity, true );
		}
	} 
}

/*
=================
R_DrawStudioModel
=================
*/
void R_DrawStudioModel( cl_entity_t *e )
{
	R_DrawStudioModelInternal( e, false );
}

/*
=================
R_RunViewmodelEvents
=================
*/
void R_RunViewmodelEvents( void )
{
	if( cl.refdef.nextView || cl.thirdperson || RI.params & RP_NONVIEWERREF )
		return;

	if( !Mod_Extradata( clgame.viewent.model ))
		return;

	RI.currententity = &clgame.viewent;
	RI.currentmodel = RI.currententity->model;
	if( !RI.currentmodel ) return;

	pStudioDraw->StudioDrawModel( STUDIO_EVENTS );

	RI.currententity = NULL;
	RI.currentmodel = NULL;
}

/*
=================
R_DrawViewModel
=================
*/
void R_DrawViewModel( void )
{
	if( RI.refdef.onlyClientDraw || r_drawviewmodel->integer == 0 )
		return;

	// ignore in thirdperson, camera view or client is died
	if( cl.thirdperson || cl.refdef.health <= 0 || cl.refdef.viewentity != ( cl.playernum + 1 ))
		return;

	if( RI.params & RP_NONVIEWERREF )
		return;

	if( !Mod_Extradata( clgame.viewent.model ))
		return;

	RI.currententity = &clgame.viewent;
	RI.currentmodel = RI.currententity->model;
	if( !RI.currentmodel ) return;

	RI.currententity->curstate.renderamt = R_ComputeFxBlend( RI.currententity );

	// hack the depth range to prevent view model from poking into walls
	pglDepthRange( gldepthmin, gldepthmin + 0.3f * ( gldepthmax - gldepthmin ));

	// backface culling for left-handed weapons
	if( r_lefthand->integer == 1 || g_iBackFaceCull )
		GL_FrontFace( !glState.frontFace );

	pStudioDraw->StudioDrawModel( STUDIO_RENDER );

	// restore depth range
	pglDepthRange( gldepthmin, gldepthmax );

	// backface culling for left-handed weapons
	if( r_lefthand->integer == 1 || g_iBackFaceCull )
		GL_FrontFace( !glState.frontFace );

	RI.currententity = NULL;
	RI.currentmodel = NULL;
}

/*
====================
R_StudioLoadTexture

load model texture with unique name
====================
*/
static void R_StudioLoadTexture( model_t *mod, studiohdr_t *phdr, mstudiotexture_t *ptexture )
{
	size_t		size;
	int		flags = 0;
	qboolean		load_external = false;
	char		texname[128], name[128], mdlname[128];
	imgfilter_t	*filter = NULL;
	texture_t		*tx = NULL;
	
	if( ptexture->flags & STUDIO_NF_TRANSPARENT )
		flags |= (TF_CLAMP|TF_NOMIPMAP);

	if( ptexture->flags & STUDIO_NF_NORMALMAP )
		flags |= (TF_NORMALMAP);

	// store some textures for remapping
	if( !Q_strnicmp( ptexture->name, "DM_Base", 7 ) || !Q_strnicmp( ptexture->name, "remap", 5 ))
	{
		int	i, size;
		char	val[6];
		byte	*pixels;

		i = mod->numtextures;
		mod->textures = (texture_t **)Mem_Realloc( mod->mempool, mod->textures, ( i + 1 ) * sizeof( texture_t* ));
		size = ptexture->width * ptexture->height + 768;
		tx = Mem_Alloc( mod->mempool, sizeof( *tx ) + size );
		mod->textures[i] = tx;

		// parse ranges and store it
		// HACKHACK: store ranges into anim_min, anim_max etc
		if( !Q_strnicmp( ptexture->name, "DM_Base", 7 ))
		{
			Q_strncpy( tx->name, "DM_Base", sizeof( tx->name ));
			tx->anim_min = PLATE_HUE_START; // topcolor start
			tx->anim_max = PLATE_HUE_END; // topcolor end
			// bottomcolor start always equal is (topcolor end + 1)
			tx->anim_total = SUIT_HUE_END;// bottomcolor end 
		}
		else
		{
			Q_strncpy( tx->name, "DM_User", sizeof( tx->name ));	// custom remapped
			Q_strncpy( val, ptexture->name + 7, 4 );  
			tx->anim_min = bound( 0, Q_atoi( val ), 255 );	// topcolor start
			Q_strncpy( val, ptexture->name + 11, 4 ); 
			tx->anim_max = bound( 0, Q_atoi( val ), 255 );	// topcolor end
			// bottomcolor start always equal is (topcolor end + 1)
			Q_strncpy( val, ptexture->name + 15, 4 ); 
			tx->anim_total = bound( 0, Q_atoi( val ), 255 );	// bottomcolor end
		}

		tx->width = ptexture->width;
		tx->height = ptexture->height;

		// the pixels immediately follow the structures
		pixels = (byte *)phdr + ptexture->index;
		Q_memcpy( tx+1, pixels, size );

		ptexture->flags |= STUDIO_NF_COLORMAP;	// yes, this is colormap image
		flags |= TF_FORCE_COLOR;

		mod->numtextures++;	// done
	}

	Q_strncpy( mdlname, mod->name, sizeof( mdlname ));
	FS_FileBase( ptexture->name, name );
	FS_StripExtension( mdlname );

	// loading texture filter for studiomodel
	if( !( ptexture->flags & STUDIO_NF_COLORMAP ))
		filter = R_FindTexFilter( va( "%s.mdl/%s", mdlname, name )); // grab texture filter

	// NOTE: colormaps must have the palette for properly work. Ignore it.
	if( Mod_AllowMaterials( ) && !( ptexture->flags & STUDIO_NF_COLORMAP ))
	{
		int	gl_texturenum = 0;

		Q_snprintf( texname, sizeof( texname ), "materials/%s/%s.tga", mdlname, name );

		if( FS_FileExists( texname, false ))
			gl_texturenum = GL_LoadTexture( texname, NULL, 0, flags, filter );

		if( gl_texturenum )
		{
			ptexture->index = gl_texturenum;
			load_external = true; // sucessfully loaded
		}
	}

	if( !load_external )
	{
		// NOTE: replace index with pointer to start of imagebuffer, ImageLib expected it
		ptexture->index = (int)((byte *)phdr) + ptexture->index;
		size = sizeof( mstudiotexture_t ) + ptexture->width * ptexture->height + 768;

		// build the texname
		Q_snprintf( texname, sizeof( texname ), "#%s/%s.mdl", mdlname, name );
		ptexture->index = GL_LoadTexture( texname, (byte *)ptexture, size, flags, filter );
          }
          else MsgDev( D_NOTE, "loading HQ: %s\n", texname );
  
	if( !ptexture->index )
	{
		MsgDev( D_WARN, "%s has null texture %s\n", mod->name, ptexture->name );
		ptexture->index = tr.defaultTexture;
	}
	else
	{
		// duplicate texnum for easy acess 
		if( tx ) tx->gl_texturenum = ptexture->index;
		GL_SetTextureType( ptexture->index, TEX_STUDIO );
	}
}

/*
=================
R_StudioLoadHeader
=================
*/
studiohdr_t *R_StudioLoadHeader( model_t *mod, const void *buffer )
{
	byte		*pin;
	studiohdr_t	*phdr;
	mstudiotexture_t	*ptexture;
	int		i;

	if( !buffer ) return NULL;

	pin = (byte *)buffer;
	phdr = (studiohdr_t *)pin;
	i = phdr->version;

	if( i != STUDIO_VERSION )
	{
		MsgDev( D_ERROR, "%s has wrong version number (%i should be %i)\n", mod->name, i, STUDIO_VERSION );
		return NULL;
	}	

	if( host.type != HOST_DEDICATED )
	{
		ptexture = (mstudiotexture_t *)(((byte *)phdr) + phdr->textureindex);
		if( phdr->textureindex > 0 && phdr->numtextures <= MAXSTUDIOSKINS )
		{
			for( i = 0; i < phdr->numtextures; i++ )
				R_StudioLoadTexture( mod, phdr, &ptexture[i] );
		}
	}

	return (studiohdr_t *)buffer;
}

/*
=================
Mod_LoadStudioModel
=================
*/
void Mod_LoadStudioModel( model_t *mod, const void *buffer, qboolean *loaded )
{
	studiohdr_t	*phdr;

	if( loaded ) *loaded = false;
	loadmodel->mempool = Mem_AllocPool( va( "^2%s^7", loadmodel->name ));
	loadmodel->type = mod_studio;

	phdr = R_StudioLoadHeader( mod, buffer );
	if( !phdr ) return;	// bad model

#ifdef STUDIO_MERGE_TEXTURES
	if( phdr->numtextures == 0 )
	{
		studiohdr_t	*thdr;
		byte		*in, *out;
		void		*buffer2 = NULL;
		size_t		size1, size2;

		buffer2 = FS_LoadFile( R_StudioTexName( mod ), NULL, false );
		thdr = R_StudioLoadHeader( mod, buffer2 );

		if( !thdr )
		{
			MsgDev( D_WARN, "Mod_LoadStudioModel: %s missing textures file\n", mod->name ); 
			if( buffer2 ) Mem_Free( buffer2 );
		}
                    else
                    {
			// give space for textures and skinrefs
			size1 = thdr->numtextures * sizeof( mstudiotexture_t );
			size2 = thdr->numskinfamilies * thdr->numskinref * sizeof( short );
			mod->cache.data = Mem_Alloc( loadmodel->mempool, phdr->length + size1 + size2 );
			Q_memcpy( loadmodel->cache.data, buffer, phdr->length ); // copy main mdl buffer
			phdr = (studiohdr_t *)loadmodel->cache.data; // get the new pointer on studiohdr
			phdr->numskinfamilies = thdr->numskinfamilies;
			phdr->numtextures = thdr->numtextures;
			phdr->numskinref = thdr->numskinref;
			phdr->textureindex = phdr->length;
			phdr->skinindex = phdr->textureindex + size1;

			in = (byte *)thdr + thdr->textureindex;
			out = (byte *)phdr + phdr->textureindex;
			Q_memcpy( out, in, size1 + size2 );	// copy textures + skinrefs
			phdr->length += size1 + size2;
			Mem_Free( buffer2 ); // release T.mdl
		}
	}
	else
	{
		// NOTE: we wan't keep raw textures in memory. just cutoff model pointer above texture base
		loadmodel->cache.data = Mem_Alloc( loadmodel->mempool, phdr->texturedataindex );
		Q_memcpy( loadmodel->cache.data, buffer, phdr->texturedataindex );
		phdr->length = phdr->texturedataindex;	// update model size
	}
#else
	// just copy model into memory
	loadmodel->cache.data = Mem_Alloc( loadmodel->mempool, phdr->length );
	Q_memcpy( loadmodel->cache.data, buffer, phdr->length );
#endif
	// setup bounding box
	VectorCopy( phdr->bbmin, loadmodel->mins );
	VectorCopy( phdr->bbmax, loadmodel->maxs );

	loadmodel->numframes = R_StudioBodyVariations( loadmodel );
	loadmodel->radius = RadiusFromBounds( loadmodel->mins, loadmodel->maxs );
	loadmodel->flags = phdr->flags; // copy header flags

	// check for static model
	if( phdr->numseqgroups == 1 && phdr->numseq == 1 && phdr->numbones == 1 )
	{
		mstudioseqdesc_t	*pseqdesc = (mstudioseqdesc_t *)((byte *)phdr + phdr->seqindex);

		// HACKHACK: MilkShape created a default animations with 30 frames
		// TODO: analyze real frames for more predicatable results
		// TODO: analyze all the sequences
		if( pseqdesc->numframes == 1 || pseqdesc->numframes == 30 )
			pseqdesc->flags |= STUDIO_STATIC;
	}

	if( loaded ) *loaded = true;
}

/*
=================
Mod_UnloadStudioModel
=================
*/
void Mod_UnloadStudioModel( model_t *mod )
{
	studiohdr_t	*pstudio;
	mstudiotexture_t	*ptexture;
	int		i;

	ASSERT( mod != NULL );

	if( mod->type != mod_studio )
		return; // not a studio

	pstudio = mod->cache.data;
	if( !pstudio ) return; // already freed

	ptexture = (mstudiotexture_t *)(((byte *)pstudio) + pstudio->textureindex);

	// release all textures
	for( i = 0; i < pstudio->numtextures; i++ )
	{
		if( ptexture[i].index == tr.defaultTexture )
			continue;
		GL_FreeTexture( ptexture[i].index );
	}

	Mem_FreePool( &mod->mempool );
	Q_memset( mod, 0, sizeof( *mod ));
}
		
static engine_studio_api_t gStudioAPI =
{
	Mod_Calloc,
	Mod_CacheCheck,
	Mod_LoadCacheFile,
	Mod_ForName,
	Mod_Extradata,
	Mod_Handle,
	pfnGetCurrentEntity,
	pfnPlayerInfo,
	R_StudioGetPlayerState,
	pfnGetViewEntity,
	pfnGetEngineTimes,
	pfnCVarGetPointer,
	pfnGetViewInfo,
	R_GetChromeSprite,
	pfnGetModelCounters,
	pfnGetAliasScale,
	pfnStudioGetBoneTransform,
	pfnStudioGetLightTransform,
	pfnStudioGetAliasTransform,
	pfnStudioGetRotationMatrix,
	R_StudioSetupModel,
	R_StudioCheckBBox,
	R_StudioDynamicLight,
	R_StudioEntityLight,
	R_StudioSetupLighting,
	R_StudioDrawPoints,
	R_StudioDrawHulls,
	R_StudioDrawAbsBBox,
	R_StudioDrawBones,
	R_StudioSetupSkin,
	R_StudioSetRemapColors,
	R_StudioSetupPlayerModel,
	R_StudioClientEvents,
	R_StudioGetForceFaceFlags,
	R_StudioSetForceFaceFlags,
	R_StudioSetHeader,
	R_StudioSetRenderModel,
	R_StudioSetupRenderer,
	R_StudioRestoreRenderer,
	R_StudioSetChromeOrigin,
	pfnIsHardware,
	GL_StudioDrawShadow,
	GL_SetRenderMode,
	R_StudioSetRenderamt,
	R_StudioSetCullState,
	R_StudioRenderShadow,
};

static r_studio_interface_t gStudioDraw =
{
	STUDIO_INTERFACE_VERSION,
	R_StudioDrawModel,
	R_StudioDrawPlayer,
};

/*
===============
CL_InitStudioAPI

Initialize client studio
===============
*/
void CL_InitStudioAPI( void )
{
	pStudioDraw = &gStudioDraw;

	// Xash will be used internal StudioModelRenderer
	if( !clgame.dllFuncs.pfnGetStudioModelInterface )
		return;

	MsgDev( D_NOTE, "InitStudioAPI " );

	if( clgame.dllFuncs.pfnGetStudioModelInterface( STUDIO_INTERFACE_VERSION, &pStudioDraw, &gStudioAPI ))
	{
		MsgDev( D_NOTE, "- ok\n" );
		return;
	}

	MsgDev( D_NOTE, "- failed\n" );

	// NOTE: we always return true even if game interface was not correct
	// because we need Draw our StudioModels
	// just restore pointer to builtin function
	pStudioDraw = &gStudioDraw;
}