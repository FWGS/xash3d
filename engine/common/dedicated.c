/*
dedicated.c - stubs for dedicated server
Copyright (C) 2016 mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#ifdef XASH_DEDICATED
#include "common.h"
#include "mathlib.h"
const char *svc_strings[256] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_changing",
	"svc_version",
	"svc_setview",
	"svc_sound",
	"svc_time",
	"svc_print",
	"svc_stufftext",
	"svc_setangle",
	"svc_serverdata",
	"svc_lightstyle",
	"svc_updateuserinfo",
	"svc_deltatable",
	"svc_clientdata",
	"svc_stopsound",
	"svc_updatepings",
	"svc_particle",
	"svc_restoresound",
	"svc_spawnstatic",
	"svc_event_reliable",
	"svc_spawnbaseline",
	"svc_temp_entity",
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_event",
	"svc_soundindex",
	"svc_ambientsound",
	"svc_intermission",
	"svc_modelindex",
	"svc_cdtrack",
	"svc_serverinfo",
	"svc_eventindex",
	"svc_weaponanim",
	"svc_bspdecal",
	"svc_roomtype",
	"svc_addangle",
	"svc_usermessage",
	"svc_packetentities",
	"svc_deltapacketentities",
	"svc_chokecount",
	"svc_resourcelist",
	"svc_deltamovevars",
	"svc_customization",
	"svc_unused46",
	"svc_crosshairangle",
	"svc_soundfade",
	"svc_unused49",
	"svc_unused50",
	"svc_director",
	"svc_studiodecal",
	"svc_unused53",
	"svc_unused54",
	"svc_unused55",
	"svc_unused56",
	"svc_querycvarvalue",
	"svc_querycvarvalue2",
	"svc_unused59",
	"svc_unused60",
	"svc_unused61",
	"svc_unused62",
	"svc_unused63",
};


//======================================================================
qboolean CL_Active( void )
{
	return false;
}

//======================================================================
qboolean CL_IsInGame( void )
{
	return true;	// always active for dedicated servers

}

qboolean CL_IsInMenu( void )
{
	return false;
}

qboolean CL_IsInConsole( void )
{
	return false;
}

qboolean CL_IsIntermission( void )
{
	return false;
}

qboolean CL_IsPlaybackDemo( void )
{
	return false;
}

qboolean CL_DisableVisibility( void )
{
	return false;
}

qboolean CL_IsBackgroundDemo( void )
{
	return false;
}

qboolean CL_IsBackgroundMap( void )
{
	return false;
}

void Con_Print( const char *txt )
{
}

void CL_ProcessFile( qboolean successfully_received, const char *filename )
{
}

void CL_Init()
{

}

void Key_Init()
{

}

void IN_Init()
{

}

void CL_Drop()
{

}

void CL_ClearEdicts()
{

}

void Key_SetKeyDest(int key_dest)
{

}

void UI_SetActiveMenu( qboolean fActive )
{

}

void CL_WriteMessageHistory()
{

}

void Host_ClientFrame()
{

}

void Host_InputFrame()
{
	Cbuf_Execute();
}

void R_ClearAllDecals()
{

}
int R_CreateDecalList( struct decallist_s *pList, qboolean changelevel )
{
	return 0;
}

void S_StopSound(int entnum, int channel, const char *soundname)
{

}

int S_GetCurrentStaticSounds( soundlist_t *pout, int size )
{
	return 0;
}
qboolean S_StreamGetCurrentState(char *currentTrack, char *loopTrack, fs_offset_t *position)
{
	return false;
}

int CL_GetMaxClients()
{
	return 0;
}

void Touch_InitConfig()
{

}

void CL_Disconnect()
{

}

void CL_Shutdown()
{

}

void R_ClearStaticEntities()
{

}

void Host_Credits()
{

}

qboolean UI_CreditsActive()
{
	return false;
}

void GL_FreeImage( const char *name )
{

}

void S_StopBackgroundTrack()
{

}

void SCR_BeginLoadingPlaque( qboolean is_background )
{

}

int S_GetCurrentDynamicSounds( soundlist_t *pout, int size )
{
	return 0;
}

void S_StopAllSounds()
{

}

void Con_NPrintf( int idx, char *fmt, ... )
{

}

void Con_NXPrintf( struct  con_nprint_s *info, char *fmt, ... )
{

}

const byte *GL_TextureData( unsigned int texnum )
{
	return NULL;
}

void SCR_CheckStartupVids()
{

}


/*
================
GL_SubdivideSurface

Breaks a polygon up along axial 64 unit
boundaries so that turbulent and sky warps
can be done reasonably.
================
*/
#include "gl_local.h"
#include "mod_local.h"

static void BoundPoly( int numverts, float *verts, vec3_t mins, vec3_t maxs )
{
	int	i, j;
	float	*v;

	mins[0] = mins[1] = mins[2] = 99999.0f;
	maxs[0] = maxs[1] = maxs[2] = -99999.0f;

	for( i = 0, v = verts; i < numverts; i++ )
	{
		for( j = 0; j < 3; j++, v++ )
		{
			if( *v < mins[j] ) mins[j] = *v;
			if( *v > maxs[j] ) maxs[j] = *v;
		}
	}
}

static void SubdividePolygon_r( msurface_t *warpface, int numverts, float *verts )
{
	int	i, j, k, f, b;
	vec3_t	mins, maxs;
	float	m, frac, s, t, *v, vertsDiv, *verts_p;
	vec3_t	front[SUBDIVIDE_SIZE], back[SUBDIVIDE_SIZE], total;
	float	dist[SUBDIVIDE_SIZE], total_s, total_t, total_ls, total_lt;
	glpoly_t	*poly;

	Q_memset( dist, 0, SUBDIVIDE_SIZE * sizeof( float ) );

	if( numverts > ( SUBDIVIDE_SIZE - 4 ))
		Host_Error( "Mod_SubdividePolygon: too many vertexes on face ( %i )\n", numverts );

	BoundPoly( numverts, verts, mins, maxs );

	for( i = 0; i < 3; i++ )
	{
		m = ( mins[i] + maxs[i] ) * 0.5f;
		m = SUBDIVIDE_SIZE * floor( m / SUBDIVIDE_SIZE + 0.5f );
		if( maxs[i] - m < 8 ) continue;
		if( m - mins[i] < 8 ) continue;

		// cut it
		v = verts + i;
		for( j = 0; j < numverts; j++, v += 3 )
			dist[j] = *v - m;

		// wrap cases
		dist[j] = dist[0];
		v -= i;
		VectorCopy( verts, v );

		f = b = 0;
		v = verts;
		for( j = 0; j < numverts; j++, v += 3 )
		{
			if( dist[j] >= 0 )
			{
				VectorCopy( v, front[f] );
				f++;
			}

			if( dist[j] <= 0 )
			{
				VectorCopy (v, back[b]);
				b++;
			}

			if( dist[j] == 0 || dist[j+1] == 0 )
				continue;

			if(( dist[j] > 0 ) != ( dist[j+1] > 0 ))
			{
				// clip point
				frac = dist[j] / ( dist[j] - dist[j+1] );
				for( k = 0; k < 3; k++ )
					front[f][k] = back[b][k] = v[k] + frac * (v[3+k] - v[k]);
				f++;
				b++;
			}
		}

		SubdividePolygon_r( warpface, f, front[0] );
		SubdividePolygon_r( warpface, b, back[0] );
		return;
	}

	// add a point in the center to help keep warp valid
	poly = Mem_Alloc( loadmodel->mempool, sizeof( glpoly_t ) + ( ( numverts - 4 ) + 2) * VERTEXSIZE * sizeof( float ));
	poly->next = warpface->polys;
	poly->flags = warpface->flags;
	warpface->polys = poly;
	poly->numverts = numverts + 2;
	verts_p = (float *)poly + ( ( sizeof( void* ) + sizeof( int ) ) >> 1 ); // pointer glpoly_t->verts
	VectorClear( total );
	total_s = total_ls = 0.0f;
	total_t = total_lt = 0.0f;

	for( i = 0; i < numverts; i++, verts += 3 )
	{
		VectorCopy( verts, &verts_p[VERTEXSIZE * ( i + 1 )] );
		VectorAdd( total, verts, total );

		if( warpface->flags & SURF_DRAWTURB )
		{
			s = DotProduct( verts, warpface->texinfo->vecs[0] );
			t = DotProduct( verts, warpface->texinfo->vecs[1] );
		}
		else
		{
			s = DotProduct( verts, warpface->texinfo->vecs[0] ) + warpface->texinfo->vecs[0][3];
			t = DotProduct( verts, warpface->texinfo->vecs[1] ) + warpface->texinfo->vecs[1][3];
			s /= warpface->texinfo->texture->width;
			t /= warpface->texinfo->texture->height;
		}

		verts_p[VERTEXSIZE * ( i + 1 ) + 3] = s;
		verts_p[VERTEXSIZE * ( i + 1 ) + 4] = t;

		total_s += s;
		total_t += t;

		// for speed reasons
		if( !( warpface->flags & SURF_DRAWTURB ))
		{
			// lightmap texture coordinates
			s = DotProduct( verts, warpface->texinfo->vecs[0] ) + warpface->texinfo->vecs[0][3];
			s -= warpface->texturemins[0];
			s += warpface->light_s * LM_SAMPLE_SIZE;
			s += LM_SAMPLE_SIZE >> 1;
			s /= BLOCK_SIZE * LM_SAMPLE_SIZE; //fa->texinfo->texture->width;

			t = DotProduct( verts, warpface->texinfo->vecs[1] ) + warpface->texinfo->vecs[1][3];
			t -= warpface->texturemins[1];
			t += warpface->light_t * LM_SAMPLE_SIZE;
			t += LM_SAMPLE_SIZE >> 1;
			t /= BLOCK_SIZE * LM_SAMPLE_SIZE; //fa->texinfo->texture->height;

			verts_p[VERTEXSIZE * ( i + 1 ) + 5] = s;
			verts_p[VERTEXSIZE * ( i + 1 ) + 6] = t;

			total_ls += s;
			total_lt += t;
		}
	}

	vertsDiv = ( 1.0f / (float)numverts );

	VectorScale( total, vertsDiv, &verts_p[0] );
	verts_p[3] = total_s * vertsDiv;
	verts_p[4] = total_t * vertsDiv;

	if( !( warpface->flags & SURF_DRAWTURB ))
	{
		verts_p[5] = total_ls * vertsDiv;
		verts_p[6] = total_lt * vertsDiv;
	}

	// copy first vertex to last
	Q_memcpy(&verts_p[VERTEXSIZE * ( i + 1 )], &verts_p[VERTEXSIZE], VERTEXSIZE * sizeof( float ) );
}



void GL_SubdivideSurface( msurface_t *fa )
{
	vec3_t	verts[SUBDIVIDE_SIZE];
	int	numverts;
	int	i, lindex;
	float	*vec;

	// convert edges back to a normal polygon
	numverts = 0;
	for( i = 0; i < fa->numedges; i++ )
	{
		lindex = loadmodel->surfedges[fa->firstedge + i];

		if( lindex > 0 ) vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
		else vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;
		VectorCopy( vec, verts[numverts] );
		numverts++;
	}

	// do subdivide
	SubdividePolygon_r( fa, numverts, verts[0] );
}

int pfnDrawConsoleString( int x, int y, char *string )
{
	return 0;
}

void pfnDrawSetTextColor( float r, float g, float b )
{

}

void pfnDrawConsoleStringLen( const char *pText, int *length, int *height )
{

}

#include "studio.h"
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
	i = LittleLong(phdr->version);

	if( i != STUDIO_VERSION )
	{
		MsgDev( D_ERROR, "%s has wrong version number (%i should be %i)\n", mod->name, i, STUDIO_VERSION );
		return NULL;
	}

#ifdef XASH_BIG_ENDIAN
	Mod_StudioBigEndian( mod, phdr );
#endif

	return (studiohdr_t *)buffer;
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

	// just copy model into memory
	loadmodel->cache.data = Mem_Alloc( loadmodel->mempool, phdr->length );
	Q_memcpy( loadmodel->cache.data, buffer, phdr->length );

	phdr = loadmodel->cache.data;

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

	Mem_FreePool( &mod->mempool );
	Q_memset( mod, 0, sizeof( *mod ));
}

#include "sprite.h"

/*
====================
Mod_LoadSpriteModel

load sprite model
====================
*/
void Mod_LoadSpriteModel( model_t *mod, byte *buffer, qboolean *loaded, uint texFlags )
{
	dsprite_t		pin;
	short		numi;
	msprite_t		*psprite;
	int		i, size;

	if( loaded ) *loaded = false;
	Q_memcpy(&pin, buffer, sizeof(dsprite_t));
	mod->type = mod_sprite;
	i = LittleLong(pin.version);

	if( LittleLong(pin.ident) != IDSPRITEHEADER )
	{
		MsgDev( D_ERROR, "%s has wrong id (%x should be %x)\n", mod->name, pin.ident, IDSPRITEHEADER );
		return;
	}

	if( i != SPRITE_VERSION )
	{
		MsgDev( D_ERROR, "%s has wrong version number (%i should be %i)\n", mod->name, i, SPRITE_VERSION );
		return;
	}

	mod->mempool = Mem_AllocPool( va( "^2%s^7", mod->name ));
	size = sizeof( msprite_t ) + ( LittleLong(pin.numframes) - 1 ) * sizeof( psprite->frames );
	psprite = Mem_Alloc( mod->mempool, size );
	mod->cache.data = psprite;	// make link to extradata

	psprite->type = LittleLong(pin.type);
	psprite->texFormat = LittleLong(pin.texFormat);
	psprite->numframes = mod->numframes = LittleLong(pin.numframes);
	psprite->facecull = LittleLong(pin.facetype);
	psprite->radius = LittleLong(pin.boundingradius);
	psprite->synctype = LittleLong(pin.synctype);

	mod->mins[0] = mod->mins[1] = -LittleLong(pin.bounds[0]) / 2;
	mod->maxs[0] = mod->maxs[1] = LittleLong(pin.bounds[0]) / 2;
	mod->mins[2] = -LittleLong(pin.bounds[1]) / 2;
	mod->maxs[2] = LittleLong(pin.bounds[1]) / 2;
	buffer += sizeof(dsprite_t);
	Q_memcpy(&numi, buffer, sizeof(short));

	// skip frames loading
	if( loaded ) *loaded = true;	// done
	psprite->numframes = 0;
}
/*
====================
Mod_UnloadSpriteModel

release sprite model and frames
====================
*/
void Mod_UnloadSpriteModel( model_t *mod )
{
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int		i, j;

	ASSERT( mod != NULL );

	if( mod->type != mod_sprite )
		return; // not a sprite

	psprite = mod->cache.data;
	if( !psprite ) return; // already freed

	Mem_FreePool( &mod->mempool );
	Q_memset( mod, 0, sizeof( *mod ));
}

imgfilter_t *R_FindTexFilter( const char *texname )
{
	return NULL;
}

void CL_Crashed()
{

}

void CL_WarnLostSplitPacket( void )
{
}
#endif
