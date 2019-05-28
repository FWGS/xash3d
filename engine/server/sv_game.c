/*
sv_game.c - gamedll interaction
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
#include "net_encode.h"
#include "event_flags.h"
#include "library.h"
#include "pm_defs.h"
#include "studio.h"
#include "const.h"
#include "render_api.h"	// modelstate_t


// fatpvs stuff
static byte fatpvs[MAX_MAP_LEAFS/8];
static byte fatphs[MAX_MAP_LEAFS/8];
static byte clientpvs[MAX_MAP_LEAFS/8];	// for find client in PVS
static vec3_t viewPoint[MAX_CLIENTS];
static byte *bitvector;
static int fatbytes;

// exports
typedef void (__cdecl *LINK_ENTITY_FUNC)( entvars_t *pev );
#ifdef _WIN32
typedef void (__stdcall *GIVEFNPTRSTODLL)( enginefuncs_t* engfuncs, globalvars_t *pGlobals );
#else
typedef void (__cdecl *GIVEFNPTRSTODLL)( enginefuncs_t* engfuncs, globalvars_t *pGlobals );
#endif
/*
=============
EntvarsDescription

entavrs table for FindEntityByString
=============
*/
#define ENTVARS_COUNT	( sizeof( gEntvarsDescription ) / sizeof( gEntvarsDescription[0] ))

static TYPEDESCRIPTION gEntvarsDescription[] = 
{
	DEFINE_ENTITY_FIELD( classname, FIELD_STRING ),
	DEFINE_ENTITY_FIELD( globalname, FIELD_STRING ),
	DEFINE_ENTITY_FIELD( model, FIELD_MODELNAME ),
	DEFINE_ENTITY_FIELD( viewmodel, FIELD_MODELNAME ),
	DEFINE_ENTITY_FIELD( weaponmodel, FIELD_MODELNAME ),
	DEFINE_ENTITY_FIELD( target, FIELD_STRING ),
	DEFINE_ENTITY_FIELD( targetname, FIELD_STRING ),
	DEFINE_ENTITY_FIELD( netname, FIELD_STRING ),
	DEFINE_ENTITY_FIELD( message, FIELD_STRING ),
	DEFINE_ENTITY_FIELD( noise, FIELD_SOUNDNAME ),
	DEFINE_ENTITY_FIELD( noise1, FIELD_SOUNDNAME ),
	DEFINE_ENTITY_FIELD( noise2, FIELD_SOUNDNAME ),
	DEFINE_ENTITY_FIELD( noise3, FIELD_SOUNDNAME ),
};

/*
=============
SV_GetEntvarsDescription

entavrs table for FindEntityByString
=============
*/
TYPEDESCRIPTION *SV_GetEntvarsDescirption( int number )
{
	if( number < 0 && number >= ENTVARS_COUNT )
		return NULL;
	return &gEntvarsDescription[number];
}

void SV_SysError( const char *error_string )
{
	if( svgame.hInstance != NULL )
		svgame.dllFuncs.pfnSys_Error( error_string );

	Log_Printf ("FATAL ERROR (shutting down): %s\n", error_string);
}

void SV_SetMinMaxSize( edict_t *e, const float *min, const float *max )
{
	int	i;

	ASSERT( min != NULL && max != NULL );

	if( !SV_IsValidEdict( e ))
		return;

	for( i = 0; i < 3; i++ )
	{
		if( min[i] > max[i] )
		{
			MsgDev( D_ERROR, "SV_SetMinMaxSize: %s backwards mins/maxs\n", SV_ClassName( e ));
			SV_LinkEdict( e, false ); // just relink edict and exit
			return;
		}
	}

	VectorCopy( min, e->v.mins );
	VectorCopy( max, e->v.maxs );
	VectorSubtract( max, min, e->v.size );

	SV_LinkEdict( e, false );
}

void SV_CopyTraceToGlobal( trace_t *trace )
{
	svgame.globals->trace_allsolid = trace->allsolid;
	svgame.globals->trace_startsolid = trace->startsolid;
	svgame.globals->trace_fraction = trace->fraction;
	svgame.globals->trace_plane_dist = trace->plane.dist;
	svgame.globals->trace_flags = 0;
	svgame.globals->trace_inopen = trace->inopen;
	svgame.globals->trace_inwater = trace->inwater;
	VectorCopy( trace->endpos, svgame.globals->trace_endpos );
	VectorCopy( trace->plane.normal, svgame.globals->trace_plane_normal );
	svgame.globals->trace_hitgroup = trace->hitgroup;

	if( SV_IsValidEdict( trace->ent ))
		svgame.globals->trace_ent = trace->ent;
	else svgame.globals->trace_ent = svgame.edicts;
}

void SV_SetModel( edict_t *ent, const char *name )
{
	vec3_t	mins = { 0.0f, 0.0f, 0.0f };
	vec3_t	maxs = { 0.0f, 0.0f, 0.0f };
	int	i, mod_type;

	i = SV_ModelIndex( name );
	if( i == 0 ) return;

	ent->v.model = MAKE_STRING( sv.model_precache[i] );
	ent->v.modelindex = i;

	mod_type = Mod_GetType( ent->v.modelindex );

	// studio models set to zero sizes as default
	switch( mod_type )
	{
	case mod_brush:
	case mod_sprite:
		Mod_GetBounds( ent->v.modelindex, mins, maxs );
		break;
	}

	SV_SetMinMaxSize( ent, mins, maxs );
}

float SV_AngleMod( float ideal, float current, float speed )
{
	float	move;

	if( anglemod( current ) == ideal ) // already there?
		return current; 

	move = ideal - current;

	if( ideal > current )
	{
		if( move >= 180 )
			move = move - 360;
	}
	else
	{
		if( move <= -180 )
			move = move + 360;
	}

	if( move > 0 )
	{
		if( move > speed )
			move = speed;
	}
	else
	{
		if( move < -speed )
			move = -speed;
	}

	return anglemod( current + move );
}

/*
=============
SV_ConvertTrace

convert trace_t to TraceResult
=============
*/
void SV_ConvertTrace( TraceResult *dst, trace_t *src )
{
	dst->fAllSolid = src->allsolid;
	dst->fStartSolid = src->startsolid;
	dst->fInOpen = src->inopen;
	dst->fInWater = src->inwater;
	dst->flFraction = src->fraction;
	VectorCopy( src->endpos, dst->vecEndPos );
	dst->flPlaneDist = src->plane.dist;
	VectorCopy( src->plane.normal, dst->vecPlaneNormal );
	dst->pHit = src->ent;
	dst->iHitgroup = src->hitgroup;

	// reset trace flags
	svgame.globals->trace_flags = 0;
}

/*
=============
SV_CheckClientVisiblity

Check visibility through client camera, portal camera, etc
=============
*/
qboolean SV_CheckClientVisiblity( sv_client_t *cl, const byte *mask )
{
	int	i, leafnum, clientnum;
	float	*viewOrg = NULL;

	if( !mask ) return true; // full visibility

	clientnum = cl - svs.clients;
	viewOrg = viewPoint[clientnum];

	// Invasion issues: wrong camera position received in ENGINE_SET_PVS
	if( cl->pViewEntity && !VectorCompare( viewOrg, cl->pViewEntity->v.origin ))
		viewOrg = cl->pViewEntity->v.origin;

	// -1 is because pvs rows are 1 based, not 0 based like leafs
	leafnum = Mod_PointLeafnum( viewOrg ) - 1;
	if( leafnum == -1 || (mask[leafnum>>3] & (1U << ( leafnum & 7 ))))
		return true; // visible from player view or camera view

	// now check all the portal cameras
	for( i = 0; i < cl->num_cameras; i++ )
	{
		edict_t *cam = cl->cameras[i];

		if( !SV_IsValidEdict( cam ))
			continue;

		leafnum = Mod_PointLeafnum( cam->v.origin ) - 1;
		// g-cont. probably camera in bad leaf... allow to send message here?
		if( leafnum == -1 || (mask[leafnum>>3] & (1U << ( leafnum & 7 ))))
			return true;
	}

	// not visible from any viewpoint
	return false;
}

/*
=================
SV_Send

Sends the contents of sv.multicast to a subset of the clients,
then clears sv.multicast.

MSG_ONE	send to one client (ent can't be NULL)
MSG_ALL	same as broadcast (origin can be NULL)
MSG_PVS	send to clients potentially visible from org
MSG_PHS	send to clients potentially hearable from org

if excludeSource is true, ent can't be NULL, dest should not be MSG_ONE
=================
*/
qboolean SV_Send( int dest, const vec3_t origin, const edict_t *ent, qboolean excludeSource )
{
	byte		*mask = NULL;
	int		j, numclients = sv_maxclients->integer;
	sv_client_t	*cl, *current = svs.clients;
	qboolean		reliable = false;
	qboolean		specproxy = false;
	int		numsends = 0;
	mleaf_t		*leaf;

	switch( dest )
	{
	case MSG_INIT:
		if( sv.state == ss_loading )
		{
			// copy to signon buffer
			BF_WriteBits( &sv.signon, BF_GetData( &sv.multicast ), BF_GetNumBitsWritten( &sv.multicast ));
			BF_Clear( &sv.multicast );
			return true;
		}
		// intentional fallthrough (in-game MSG_INIT it's a MSG_ALL reliable)
	case MSG_ALL:
		reliable = true;
		// intentional fallthrough
	case MSG_BROADCAST:
		// nothing to sort	
		break;
	case MSG_PAS_R:
		reliable = true;
		// intentional fallthrough
	case MSG_PAS:
		if( origin == NULL ) return false;
		leaf = Mod_PointInLeaf( origin, sv.worldmodel->nodes );
		mask = Mod_LeafPHS( leaf, sv.worldmodel );
		break;
	case MSG_PVS_R:
		reliable = true;
		// intentional fallthrough
	case MSG_PVS:
		if( origin == NULL ) return false;
		leaf = Mod_PointInLeaf( origin, sv.worldmodel->nodes );
		mask = Mod_LeafPVS( leaf, sv.worldmodel );
		break;
	case MSG_ONE:
		reliable = true;
		// intentional fallthrough
	case MSG_ONE_UNRELIABLE:
		if( ent == NULL ) return false;
		j = NUM_FOR_EDICT( ent );
		if( j < 1 || j > numclients ) return false;
		current = svs.clients + (j - 1);
		numclients = 1; // send to one
		if( excludeSource )
		{
			MsgDev( D_ERROR, "SV_Send: trying to send message to nobody. Will send to requested entity anyway.\n" );
			excludeSource = false;
		}
		break;
	case MSG_SPEC:
		specproxy = reliable = true;
		break;
	default:
		Host_Error( "SV_Send: bad dest: %i\n", dest );
		return false;
	}

	if( !current )
	{
		MsgDev( D_ERROR, "SV_Send: client == NULL. Dest: %i\n", dest );
		return false;
	}

	// send the data to all relevent clients (or once only)
	for( j = 0, cl = current; j < numclients; j++, cl++ )
	{
		if( cl->state == cs_free || cl->state == cs_zombie )
			continue;

		if( cl->state != cs_spawned && !reliable )
			continue;

		// do not send unregistered messages to client
		if( sv_fixmulticast->integer )
		{
			if( cl->state == cs_connected && reliable && !cl->hasusrmsgs )
				continue;
		}

		if( specproxy && !cl->hltv_proxy )
			continue;

		if( !cl->edict || cl->fakeclient )
			continue;

		if( excludeSource && cl->edict == ent )
		{
			//MsgDev( D_INFO, "%i excluded\n", j );
			continue;
		}

		if( ent != NULL && ent->v.groupinfo && cl->edict->v.groupinfo )
		{
			if(( !svs.groupop && !( cl->edict->v.groupinfo & ent->v.groupinfo )) || (svs.groupop == 1 && ( cl->edict->v.groupinfo & ent->v.groupinfo ) != 0 ))
				continue;
		}

		if( !SV_CheckClientVisiblity( cl, mask ))
			continue;

		if( specproxy ) BF_WriteBits( &sv.spectator_datagram, BF_GetData( &sv.multicast ), BF_GetNumBitsWritten( &sv.multicast ));
		else if( reliable ) BF_WriteBits( &cl->netchan.message, BF_GetData( &sv.multicast ), BF_GetNumBitsWritten( &sv.multicast ));
		else BF_WriteBits( &cl->datagram, BF_GetData( &sv.multicast ), BF_GetNumBitsWritten( &sv.multicast ));
		numsends++;
	}

	BF_Clear( &sv.multicast );

	return numsends;	// debug
}

/*
=======================
SV_GetReliableDatagram

Get shared reliable buffer
=======================
*/
sizebuf_t *SV_GetReliableDatagram( void )
{
	return &sv.reliable_datagram;
}

qboolean SV_RestoreCustomDecal( decallist_t *entry, edict_t *pEdict, qboolean adjacent )
{
	if( svgame.physFuncs.pfnRestoreDecal != NULL )
	{
		if( !pEdict ) pEdict = EDICT_NUM( entry->entityIndex );

		// true if decal was sucessfully restored at the game-side
		return svgame.physFuncs.pfnRestoreDecal( entry, pEdict, adjacent );
	}

	return false;
}

/*
=======================
SV_CreateDecal

NOTE: static decals only accepted when game is loading
=======================
*/
void SV_CreateDecal( sizebuf_t *msg, const float *origin, int decalIndex, int entityIndex, int modelIndex, int flags, float scale )
{
	if( msg == &sv.signon && sv.state != ss_loading )
		return;

	// this can happen if serialized map contains 4096 static decals...
	if(( BF_GetNumBytesWritten( msg ) + 20 ) >= BF_GetMaxBytes( msg ))
		return;

	// static decals are posters, it's always reliable
	BF_WriteByte( msg, svc_bspdecal );
	BF_WriteVec3Coord( msg, origin );
	BF_WriteWord( msg, decalIndex );
	BF_WriteShort( msg, entityIndex );
	if( entityIndex > 0 )
		BF_WriteWord( msg, modelIndex );
	BF_WriteByte( msg, flags );
	BF_WriteWord( msg, scale * 4096 );
}

/*
=======================
SV_CreateStudioDecal

NOTE: static decals only accepted when game is loading
=======================
*/
void SV_CreateStudioDecal( sizebuf_t *msg, const float *origin, const float *start, int decalIndex, int entityIndex, int modelIndex, int flags, modelstate_t *state )
{
	if( msg == &sv.signon && sv.state != ss_loading )
		return;

	// bad model or bad entity (e.g. changelevel)
	if( !entityIndex || !modelIndex )
		return;

	ASSERT( origin );
	ASSERT( start );

	// this can happen if serialized map contains 4096 static decals...
	if(( BF_GetNumBytesWritten( msg ) + 30 ) >= BF_GetMaxBytes( msg ))
		return;

	// static decals are posters, it's always reliable
	BF_WriteByte( msg, svc_studiodecal );
	BF_WriteVec3Coord( msg, origin );
	BF_WriteVec3Coord( msg, start );
	BF_WriteWord( msg, decalIndex );
	BF_WriteWord( msg, entityIndex );
	BF_WriteByte( msg, flags );

	// write model state
	BF_WriteShort( msg, state->sequence );
	BF_WriteShort( msg, state->frame );
	BF_WriteByte( msg, state->blending[0] );
	BF_WriteByte( msg, state->blending[1] );
	BF_WriteByte( msg, state->controller[0] );
	BF_WriteByte( msg, state->controller[1] );
	BF_WriteByte( msg, state->controller[2] );
	BF_WriteByte( msg, state->controller[3] );
	BF_WriteWord( msg, modelIndex );
	BF_WriteByte( msg, state->body );
	BF_WriteByte( msg, state->skin );
}

/*
=======================
SV_CreateStaticEntity

NOTE: static entities only accepted when game is loading
=======================
*/
void SV_CreateStaticEntity( sizebuf_t *msg, sv_static_entity_t *ent )
{
	int	index, i;

	// this can happen if serialized map contains too many static entities...
	if(( BF_GetNumBytesWritten( msg ) + 64 ) >= BF_GetMaxBytes( msg ))
		return;

	index = SV_ModelIndex( ent->model );

	BF_WriteByte( msg, svc_spawnstatic );
	BF_WriteShort(msg, index );
	BF_WriteByte( msg, ent->sequence );
	BF_WriteByte( msg, ent->frame );
	BF_WriteWord( msg, ent->colormap );
	BF_WriteByte( msg, ent->skin );

	for( i = 0; i < 3; i++ )
	{
		BF_WriteCoord( msg, ent->origin[i] );
		BF_WriteBitAngle( msg, ent->angles[i], 16 );
	}

	BF_WriteByte( msg, ent->rendermode );

	if( ent->rendermode != kRenderNormal )
	{
		BF_WriteByte( msg, ent->renderamt );
		BF_WriteByte( msg, ent->rendercolor.r );
		BF_WriteByte( msg, ent->rendercolor.g );
		BF_WriteByte( msg, ent->rendercolor.b );
		BF_WriteByte( msg, ent->renderfx );
	}
}

/*
=================
SV_RestartStaticEnts

Write all the static ents into demo
=================
*/
void SV_RestartStaticEnts( void )
{
	sv_static_entity_t	*clent;
	int		i;

	// remove all the static entities on the client
	R_ClearStaticEntities();

	// resend them again
	for( i = 0; i < sv.num_static_entities; i++ )
	{
		clent = &sv.static_entities[i];
		SV_CreateStaticEntity( &sv.reliable_datagram, clent );
	}
}

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/
static void SV_AddToFatPVS( const vec3_t org, int type, mnode_t *node )
{
	byte	*vis;
	float	d;

	while( 1 )
	{
		// if this is a leaf, accumulate the pvs bits
		if( node->contents < 0 )
		{
			if( node->contents != CONTENTS_SOLID )
			{
				mleaf_t	*leaf;
				int	i;

				leaf = (mleaf_t *)node;			

				if( type == DVIS_PVS )
					vis = Mod_LeafPVS( leaf, sv.worldmodel );
				else if( type == DVIS_PHS )
					vis = Mod_LeafPHS( leaf, sv.worldmodel );
				else vis = Mod_DecompressVis( NULL ); // get full visibility

				for( i = 0; i < fatbytes; i++ )
					bitvector[i] |= vis[i];
			}
			return;
		}
	
		d = PlaneDiff( org, node->plane );
		if( d > 8.0f ) node = node->children[0];
		else if( d < -8.0f ) node = node->children[1];
		else
		{
			// go down both
			SV_AddToFatPVS( org, type, node->children[0] );
			node = node->children[1];
		}
	}
}

/*
==============
SV_BoxInPVS

check brush boxes in fat pvs
==============
*/
static qboolean SV_BoxInPVS( const vec3_t org, const vec3_t absmin, const vec3_t absmax )
{
	mleaf_t	*leaf = Mod_PointInLeaf( org, sv.worldmodel->nodes );
	byte	*vis = Mod_LeafPVS( leaf, sv.worldmodel );

	if( !Mod_BoxVisible( absmin, absmax, vis ))
		return false;
	return true;
}

/*
==============
SV_WriteEntityPatch

Create entity patch for selected map
==============
*/
void SV_WriteEntityPatch( const char *filename )
{
	dheader_t		*header;
	int		ver = -1, lumpofs = 0, lumplen = 0;
	byte		buf[MAX_SYSPATH]; // 1 kb
	file_t		*f;
			
	f = FS_Open( va( "maps/%s.bsp", filename ), "rb", false );
	if( !f ) return;

	Q_memset( buf, 0, MAX_SYSPATH );
	FS_Read( f, buf, MAX_SYSPATH );
	ver = *(uint *)buf;

	switch( ver )
	{
	case Q1BSP_VERSION:
	case HLBSP_VERSION:
	case XTBSP_VERSION:
		header = (dheader_t *)buf;
		if( header->lumps[LUMP_ENTITIES].fileofs <= 1024 && (header->lumps[LUMP_ENTITIES].filelen % sizeof( dplane_t )) == 0 )
		{
			// Blue-Shift ordering
			lumpofs = header->lumps[LUMP_PLANES].fileofs;
			lumplen = header->lumps[LUMP_PLANES].filelen;
		}
		else
		{
			lumpofs = header->lumps[LUMP_ENTITIES].fileofs;
			lumplen = header->lumps[LUMP_ENTITIES].filelen;
		}
		break;
	default:
		FS_Close( f );
		return;
	}

	if( lumplen >= 10 )
	{
		char	*entities = NULL;
		
		FS_Seek( f, lumpofs, SEEK_SET );
		entities = (char *)Z_Malloc( lumplen + 1 );
		FS_Read( f, entities, lumplen );
		FS_WriteFile( va( "maps/%s.ent", filename ), entities, lumplen );
		Msg( "Write 'maps/%s.ent'\n", filename );
		Mem_Free( entities );
	}

	FS_Close( f );
}

/*
==============
SV_ReadEntityScript

pfnMapIsValid use this
==============
*/
char *SV_ReadEntityScript( const char *filename, int *flags )
{
	dheader_t		*header;
	char		*ents = NULL;
	string		bspfilename, entfilename;
	int		ver = -1, lumpofs = 0, lumplen = 0;
	byte		buf[MAX_SYSPATH]; // 1 kb
	size_t		ft1, ft2;
	file_t		*f;
	int i;

	*flags = 0;

	Q_strncpy( bspfilename, va( "maps/%s.bsp", filename ), sizeof( entfilename ));			
	f = FS_Open( bspfilename, "rb", false );
	if( !f ) return NULL;

	*flags |= MAP_IS_EXIST;

	Q_memset( buf, 0, MAX_SYSPATH );
	FS_Read( f, buf, MAX_SYSPATH );
	ver = LittleLong(*(uint *)buf);

	switch( ver )
	{
	case Q1BSP_VERSION:
	case HLBSP_VERSION:
	case XTBSP_VERSION:
		header = (dheader_t *)buf;

#ifdef XASH_BIG_ENDIAN
		for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
			LittleLongSW(((int *)header)[i]);
#endif

		if( header->lumps[LUMP_ENTITIES].fileofs <= 1024 && (header->lumps[LUMP_ENTITIES].filelen % sizeof( dplane_t )) == 0 )
		{
			// Blue-Shift ordering
			lumpofs = header->lumps[LUMP_PLANES].fileofs;
			lumplen = header->lumps[LUMP_PLANES].filelen;
		}
		else
		{
			lumpofs = header->lumps[LUMP_ENTITIES].fileofs;
			lumplen = header->lumps[LUMP_ENTITIES].filelen;
		}
		break;
	default:
		*flags |= MAP_INVALID_VERSION;
		FS_Close( f );
		return NULL;
	}

	// check for entfile too
	Q_strncpy( entfilename, va( "maps/%s.ent", filename ), sizeof( entfilename ));

	// make sure what entity patch is never than bsp
	ft1 = FS_FileTime( bspfilename, false );
	ft2 = FS_FileTime( entfilename, true );

	if( ft2 != (unsigned long)-1 && ft1 < ft2 )
	{
		// grab .ent files only from gamedir
		ents = (char *)FS_LoadFile( entfilename, NULL, true ); 
	}

	if( !ents && lumplen >= 10 )
	{
		FS_Seek( f, lumpofs, SEEK_SET );
		ents = (char *)Z_Malloc( lumplen + 1 );
		FS_Read( f, ents, lumplen );
	}

	FS_Close( f ); // all done

	return ents;
}

/*
==============
SV_MapIsValid

Validate map
==============
*/
int SV_MapIsValid( const char *filename, const char *spawn_entity, const char *landmark_name )
{
	char	*ents = NULL;
	char	*pfile;
	int	flags = 0;

	ents = SV_ReadEntityScript( filename, &flags );

	if( ents )
	{
		// if there are entities to parse, a missing message key just
		// means there is no title, so clear the message string now
		char	token[2048];
		string	check_name;
		qboolean	need_landmark = Q_strlen( landmark_name ) > 0 ? true : false;

		if( !need_landmark && host.developer >= 2 )
		{
			// not transition 
			Mem_Free( ents );

			// skip spawnpoint checks in devmode
			return (flags|MAP_HAS_SPAWNPOINT);
		}

		pfile = ents;

		while(( pfile = COM_ParseFile( pfile, token )) != NULL )
		{
			if( !Q_strcmp( token, "classname" ))
			{
				// check classname for spawn entity
				pfile = COM_ParseFile( pfile, check_name );
				if( !Q_strcmp( spawn_entity, check_name ))
				{
					flags |= MAP_HAS_SPAWNPOINT;

					// we already find landmark, stop the parsing
					if( need_landmark && flags & MAP_HAS_LANDMARK )
						break;
				}
			}
			else if( need_landmark && !Q_strcmp( token, "targetname" ))
			{
				// check targetname for landmark entity
				pfile = COM_ParseFile( pfile, check_name );

				if( !Q_strcmp( landmark_name, check_name ))
				{
					flags |= MAP_HAS_LANDMARK;

					// we already find spawnpoint, stop the parsing
					if( flags & MAP_HAS_SPAWNPOINT )
						break;
				}
			}
		}

		Mem_Free( ents );
	}

	return flags;
}

void SV_FreePrivateData( edict_t *pEdict )
{
	if( !pEdict || !pEdict->pvPrivateData )
		return;

	// NOTE: new interface can be missing
	if( svgame.dllFuncs2.pfnOnFreeEntPrivateData != NULL )
		svgame.dllFuncs2.pfnOnFreeEntPrivateData( pEdict );

	if( Mem_IsAllocatedExt( svgame.mempool, pEdict->pvPrivateData ))
		Mem_Free( pEdict->pvPrivateData );

	pEdict->pvPrivateData = NULL;
}

void SV_InitEdict( edict_t *pEdict )
{
	ASSERT( pEdict );

	SV_FreePrivateData( pEdict );
	Q_memset( &pEdict->v, 0, sizeof( entvars_t ));

	// g-cont. trying to setup controllers here...
	pEdict->v.controller[0] = 0x7F;
	pEdict->v.controller[1] = 0x7F;
	pEdict->v.controller[2] = 0x7F;
	pEdict->v.controller[3] = 0x7F;

	pEdict->v.pContainingEntity = pEdict; // make cross-links for consistency
	pEdict->free = false;
}

void SV_FreeEdict( edict_t *pEdict )
{
	ASSERT( pEdict );
	ASSERT( pEdict->free == false );

	// unlink from world
	SV_UnlinkEdict( pEdict );

	// never remove global entities from map (make dormant instead)
	if( pEdict->v.globalname && sv.state == ss_active )
	{
		pEdict->v.solid = SOLID_NOT;
		pEdict->v.flags &= ~FL_KILLME;
		pEdict->v.effects = EF_NODRAW;
		pEdict->v.movetype = MOVETYPE_NONE;
		pEdict->v.modelindex = 0;
		pEdict->v.nextthink = -1;
		return;
	}

	SV_FreePrivateData( pEdict );

	// NOTE: don't clear all edict fields on releasing
	// because gamedll may trying to use edict pointers and crash game (e.g. Opposing Force)

	// mark edict as freed
	pEdict->freetime = sv.time;
	pEdict->serialnumber++; // invalidate EHANDLE's
	pEdict->v.solid = SOLID_NOT;
	pEdict->v.flags = 0;
	pEdict->v.model = 0;
	pEdict->v.takedamage = 0;
	pEdict->v.modelindex = 0;
	pEdict->v.nextthink = -1;
	// reset vars
	pEdict->v.colormap = 0;
	pEdict->v.takedamage = 0;
	pEdict->v.frame = 0;
	pEdict->v.scale = 0;
	pEdict->v.gravity = 0;
	VectorClear(pEdict->v.angles);
	VectorClear(pEdict->v.origin);
	pEdict->free = true;
}

edict_t *GAME_EXPORT SV_AllocEdict( void )
{
	edict_t	*pEdict;
	int	i;

	for( i = svgame.globals->maxClients + 1; i < svgame.numEntities; i++ )
	{
		pEdict = EDICT_NUM( i );
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if( pEdict->free && ( pEdict->freetime < 2.0f || ( sv.time - pEdict->freetime ) > 0.5f ))
		{
			SV_InitEdict( pEdict );
			return pEdict;
		}
	}

	if( i >= svgame.globals->maxEntities )
		Sys_Error( "ED_AllocEdict: no free edicts\n" );

	svgame.numEntities++;
	pEdict = EDICT_NUM( i );
	SV_InitEdict( pEdict );

	return pEdict;
}

edict_t* SV_AllocPrivateData( edict_t *ent, string_t className )
{
	const char	*pszClassName = NULL;
	LINK_ENTITY_FUNC	SpawnEdict = NULL;

	pszClassName = STRING( className );

	if( !ent )
	{
		// allocate a new one
		ent = SV_AllocEdict();
	}
	else if( ent->free )
	{
		SV_InitEdict( ent ); // re-init edict
		MsgDev( D_WARN, "SV_AllocPrivateData: entity %s is freed!\n", STRING( className ));
	}

	ent->v.classname = className;
	ent->v.pContainingEntity = ent; // re-link
	
	// allocate edict private memory (passed by dlls)
	if( pszClassName )
		SpawnEdict = (LINK_ENTITY_FUNC)Com_GetProcAddress( svgame.hInstance, pszClassName );

	if( !SpawnEdict )
	{
		// attempt to create custom entity (Xash3D extension)
		if( svgame.physFuncs.SV_CreateEntity && svgame.physFuncs.SV_CreateEntity( ent, pszClassName ) != -1 )
			return ent;

		MsgDev( D_ERROR, "No spawn function for %s\n", STRING( className ));

		// kill entity immediately
		SV_FreeEdict( ent );

		return NULL;
	}
	else SpawnEdict( &ent->v );

	return ent;
}

void SV_FreeEdicts( void )
{
	int	i = 0;
	edict_t	*ent;

	for( i = 0; i < svgame.numEntities; i++ )
	{
		ent = EDICT_NUM( i );
		if( ent->free ) continue;
		SV_FreeEdict( ent );
	}
}

void SV_PlaybackReliableEvent( sizebuf_t *msg, word eventindex, float delay, event_args_t *args )
{
	event_args_t nullargs;

	Q_memset( &nullargs, 0, sizeof( nullargs ));

	BF_WriteByte( msg, svc_event_reliable );

	// send event index
	BF_WriteUBitLong( msg, eventindex, MAX_EVENT_BITS );

	if( delay )
	{
		// send event delay
		BF_WriteOneBit( msg, 1 );
		BF_WriteWord( msg, Q_rint( delay * 100.0f ));
	}
	else BF_WriteOneBit( msg, 0 );

	// reliable events not use delta-compression just null-compression
	MSG_WriteDeltaEvent( msg, &nullargs, args );
}

const char *SV_ClassName( const edict_t *e )
{
	if( !e ) return "(null)";
	if( e->free ) return "freed";
	return STRING( e->v.classname );
}

static qboolean SV_IsValidCmd( const char *pCmd )
{
	size_t	len = Q_strlen( pCmd );

	// valid commands all have a ';' or newline '\n' as their last character
	if( len && ( pCmd[len-1] == '\n' || pCmd[len-1] == ';' ))
		return true;
	return false;
}

sv_client_t *SV_ClientFromEdict( const edict_t *pEdict, qboolean spawned_only )
{
	int	i;

	if( !SV_IsValidEdict( pEdict ))
		return NULL;

	i = NUM_FOR_EDICT( pEdict ) - 1;

	if( i < 0 || i >= sv_maxclients->integer )
		return NULL;

	if( spawned_only )
	{
		if( svs.clients[i].state != cs_spawned )
			return NULL;
	}

	return (svs.clients + i);
}

/*
=========
SV_BaselineForEntity

assume pEdict is valid
=========
*/
void SV_BaselineForEntity( edict_t *pEdict )
{
	int		usehull, player;
	int		modelindex;
	entity_state_t	baseline;
	float		*mins, *maxs;
	sv_client_t	*cl;

	if( pEdict->v.flags & FL_CLIENT && ( cl = SV_ClientFromEdict( pEdict, false )))
	{
		usehull = ( pEdict->v.flags & FL_DUCKING ) ? true : false;
		modelindex = cl->modelindex ? cl->modelindex : pEdict->v.modelindex;
		mins = svgame.player_mins[usehull]; 
		maxs = svgame.player_maxs[usehull]; 
		player = true;
	}
	else
	{
		if( pEdict->v.effects == EF_NODRAW )
			return;

		if( !pEdict->v.modelindex || !STRING( pEdict->v.model ))
			return; // invisible

		modelindex = pEdict->v.modelindex;
		mins = pEdict->v.mins; 
		maxs = pEdict->v.maxs; 
		player = false;
	}

	// take current state as baseline
	Q_memset( &baseline, 0, sizeof( baseline )); 
	baseline.number = NUM_FOR_EDICT( pEdict );

	svgame.dllFuncs.pfnCreateBaseline( player, baseline.number, &baseline, pEdict, modelindex, mins, maxs );

	// set entity type
	if( pEdict->v.flags & FL_CUSTOMENTITY )
		baseline.entityType = ENTITY_BEAM;
	else baseline.entityType = ENTITY_NORMAL;

	svs.baselines[baseline.number] = baseline;
}

void SV_SetClientMaxspeed( sv_client_t *cl, float fNewMaxspeed )
{
	// fakeclients must be changed speed too
	fNewMaxspeed = bound( -svgame.movevars.maxspeed, fNewMaxspeed, svgame.movevars.maxspeed );

	cl->edict->v.maxspeed = fNewMaxspeed;
	Info_SetValueForKey( cl->physinfo, "maxspd", va( "%.f", fNewMaxspeed ), sizeof( cl->physinfo ) );
}

/*
===============================================================================

	Game Builtin Functions

===============================================================================
*/
/*
=========
pfnPrecacheModel

=========
*/
int GAME_EXPORT pfnPrecacheModel( const char *s )
{
	int modelIndex = SV_ModelIndex( s );

	Mod_RegisterModel( s, modelIndex );

	sv.resourcelistcache = false;
	sv.reslist.rescount = 0;

	return modelIndex;
}

/*
=================
pfnSetModel

=================
*/
void GAME_EXPORT pfnSetModel( edict_t *e, const char *m )
{
	if( !SV_IsValidEdict( e ))
	{
		MsgDev( D_WARN, "SV_SetModel: invalid entity %s\n", SV_ClassName( e ));
		return;
	}

	if( e == svgame.edicts )
	{
		MsgDev( D_ERROR, "SV_SetModel: world model can't be changed\n" );
		return;
	}

	if( !m || m[0] <= ' ' )
	{
		MsgDev( D_WARN, "SV_SetModel: null name\n" );
		return;
	}

	SV_SetModel( e, m ); 
}

/*
=================
pfnModelIndex

=================
*/
int GAME_EXPORT pfnModelIndex( const char *m )
{
	int	i;

	if( !m || !m[0] )
		return 0;

	for( i = 1; i < MAX_MODELS && sv.model_precache[i][0]; i++ )
	{
		if( !Q_stricmp( sv.model_precache[i], m ))
			return i;
	}

	MsgDev( D_ERROR, "SV_ModelIndex: %s not precached\n", m );
	return 0; 
}

/*
=================
pfnModelFrames

=================
*/
int GAME_EXPORT pfnModelFrames( int modelIndex )
{
	int	numFrames = 0;

	Mod_GetFrames( modelIndex, &numFrames );

	return numFrames;
}

/*
=================
pfnSetSize

=================
*/
void GAME_EXPORT pfnSetSize( edict_t *e, const float *rgflMin, const float *rgflMax )
{
	if( !SV_IsValidEdict( e ))
	{
		MsgDev( D_WARN, "SV_SetSize: invalid entity %s\n", SV_ClassName( e ));
		return;
	}

	SV_SetMinMaxSize( e, rgflMin, rgflMax );
}

/*
=================
pfnChangeLevel

=================
*/
void GAME_EXPORT pfnChangeLevel( const char* s1, const char* s2 )
{
	static uint	last_spawncount = 0;

	if( !s1 || s1[0] <= ' ' || sv.background )
		return;

	// make sure we don't issue two changelevels at one time
	if( svs.changelevel_next_time > host.realtime )
		return;

	svs.changelevel_next_time = host.realtime + 0.5f;		// rest 0.5f secs if failed

	// make sure we don't issue two changelevels
	if( svs.spawncount == last_spawncount )
		return;

	last_spawncount = svs.spawncount;

	SV_SkipUpdates ();

	if( !s2 ) Cbuf_AddText( va( "changelevel %s\n", s1 ));	// Quake changlevel
	else Cbuf_AddText( va( "changelevel %s %s\n", s1, s2 ));	// Half-Life changelevel
}

/*
=================
pfnGetSpawnParms

obsolete
=================
*/
void GAME_EXPORT pfnGetSpawnParms( edict_t *ent )
{
	Host_Error( "SV_GetSpawnParms: %s [%i]\n", SV_ClassName( ent ), NUM_FOR_EDICT( ent ));
}

/*
=================
pfnSaveSpawnParms

obsolete
=================
*/
void GAME_EXPORT pfnSaveSpawnParms( edict_t *ent )
{
	Host_Error( "SV_SaveSpawnParms: %s [%i]\n", SV_ClassName( ent ), NUM_FOR_EDICT( ent ));
}

/*
=================
pfnVecToYaw

=================
*/
float GAME_EXPORT pfnVecToYaw( const float *rgflVector )
{
	if( !rgflVector ) return 0.0f;
	return SV_VecToYaw( rgflVector );
}

/*
=================
pfnMoveToOrigin

=================
*/
void GAME_EXPORT pfnMoveToOrigin( edict_t *ent, const float *pflGoal, float dist, int iMoveType )
{
	if( !SV_IsValidEdict( ent ))
	{
		MsgDev( D_WARN, "SV_MoveToOrigin: invalid entity %s\n", SV_ClassName( ent ));
		return;
	}

	if( !pflGoal )
	{
		MsgDev( D_WARN, "SV_MoveToOrigin: invalid goal pos\n" );
		return;
	}

	SV_MoveToOrigin( ent, pflGoal, dist, iMoveType );
}

/*
==============
pfnChangeYaw

==============
*/
void GAME_EXPORT pfnChangeYaw( edict_t* ent )
{
	if( !SV_IsValidEdict( ent ))
	{
		MsgDev( D_WARN, "SV_ChangeYaw: invalid entity %s\n", SV_ClassName( ent ));
		return;
	}

	ent->v.angles[YAW] = SV_AngleMod( ent->v.ideal_yaw, ent->v.angles[YAW], ent->v.yaw_speed );
}

/*
==============
pfnChangePitch

==============
*/
void GAME_EXPORT pfnChangePitch( edict_t* ent )
{
	if( !SV_IsValidEdict( ent ))
	{
		MsgDev( D_WARN, "SV_ChangePitch: invalid entity %s\n", SV_ClassName( ent ));
		return;
	}

	ent->v.angles[PITCH] = SV_AngleMod( ent->v.idealpitch, ent->v.angles[PITCH], ent->v.pitch_speed );	
}

/*
=========
SV_FindEntityByString

=========
*/
edict_t* GAME_EXPORT SV_FindEntityByString( edict_t *pStartEdict, const char *pszField, const char *pszValue )
{
	int		index = 0, e = 0;
	TYPEDESCRIPTION	*desc = NULL;
	edict_t		*ed;
	const char	*t;

	if( pStartEdict ) e = NUM_FOR_EDICT( pStartEdict );
	if( !pszValue || !*pszValue ) return svgame.edicts;

	while(( desc = SV_GetEntvarsDescirption( index++ )) != NULL )
	{
		if( !Q_strcmp( pszField, desc->fieldName ))
			break;
	}

	if( desc == NULL )
	{
		MsgDev( D_ERROR, "SV_FindEntityByString: field %s not a string\n", pszField );
		return svgame.edicts;
	}
	
	for( e++; e < svgame.numEntities; e++ )
	{
		ed = EDICT_NUM( e );
		if( !SV_IsValidEdict( ed )) continue;

		if( e <= sv_maxclients->integer && !SV_ClientFromEdict( ed, ( sv_maxclients->integer != 1 )))
			continue;

		switch( desc->fieldType )
		{
		case FIELD_STRING:
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
			t = STRING( *(string_t *)&((byte *)&ed->v)[desc->fieldOffset] );
			if( t != NULL && t != svgame.globals->pStringBase )
			{
				if( !Q_strcmp( t, pszValue ))
					return ed;
			}
			break;
		default:
			break;
		}
	}

	return svgame.edicts;
}

/*
==============
pfnGetEntityIllum

returns weighted lightvalue for entity position
==============
*/
int GAME_EXPORT pfnGetEntityIllum( edict_t* pEnt )
{
	if( !SV_IsValidEdict( pEnt ))
	{
		MsgDev( D_WARN, "SV_GetEntityIllum: invalid entity %s\n", SV_ClassName( pEnt ));
		return 0;
	}

	return SV_LightForEntity( pEnt );
}

/*
=================
pfnFindEntityInSphere

return NULL instead of world!
=================
*/
edict_t *GAME_EXPORT pfnFindEntityInSphere( edict_t *pStartEdict, const float *org, float flRadius )
{
	edict_t	*ent;
	float	distSquared;
	float	eorg;
	int	j, e = 0;

	flRadius *= flRadius;

	if( SV_IsValidEdict( pStartEdict ))
		e = NUM_FOR_EDICT( pStartEdict );

	for( e++; e < svgame.numEntities; e++ )
	{
		ent = EDICT_NUM( e );

		if( !SV_IsValidEdict( ent ))
			continue;

		// ignore clients that not in a game
		if( e <= sv_maxclients->integer && !SV_ClientFromEdict( ent, true ))
			continue;

		distSquared = 0.0f;

		for( j = 0; j < 3 && distSquared <= flRadius; j++ )
		{
			if( org[j] < ent->v.absmin[j] )
				eorg = org[j] - ent->v.absmin[j];
			else if( org[j] > ent->v.absmax[j] )
				eorg = org[j] - ent->v.absmax[j];
			else eorg = 0;

			distSquared += eorg * eorg;
		}

		if( distSquared > flRadius )
			continue;
		return ent;
	}

	return EDICT_NUM( 0 );
}

/*
=================
SV_CheckClientPVS

build the new client PVS
=================
*/
int SV_CheckClientPVS( int check, qboolean bMergePVS )
{
	byte		*pvs;
	edict_t		*ent;
	mleaf_t		*leaf;
	vec3_t		view;
	sv_client_t	*cl;
	int		i, j, k;
	int		pvsbytes;

	// cycle to the next one
	check = bound( 1, check, svgame.globals->maxClients );

	if( check == svgame.globals->maxClients )
		i = 1;
	else i = check + 1;

	for( ;; i++ )
	{
		if( i == svgame.globals->maxClients + 1 )
			i = 1;

		ent = EDICT_NUM( i );

		if( i == check )
			break; // didn't find anything else

		if( ent->free || !ent->pvPrivateData || ( ent->v.flags & FL_NOTARGET ))
			continue;

		// anything that is a client, or has a client as an enemy
		break;
	}

	cl = SV_ClientFromEdict( ent, true );
	pvsbytes = (sv.worldmodel->numleafs + 7) >> 3;

	// get the PVS for the entity
	VectorAdd( ent->v.origin, ent->v.view_ofs, view );
	leaf = Mod_PointInLeaf( view, sv.worldmodel->nodes );
	pvs = Mod_LeafPVS( leaf, sv.worldmodel );
	Q_memcpy( clientpvs, pvs, pvsbytes );

	// transition in progress
	if( !cl ) return i;

	// now merge PVS with all portal cameras
	for( k = 0; k < cl->num_cameras && bMergePVS; k++ )
	{
		edict_t *cam = cl->cameras[k];

		if( !SV_IsValidEdict( cam ))
			continue;

		VectorAdd( cam->v.origin, cam->v.view_ofs, view );
		leaf = Mod_PointInLeaf( view, sv.worldmodel->nodes );
		if( leaf == NULL ) continue; // skip outside cameras
		pvs = Mod_LeafPVS( leaf, sv.worldmodel );

		for( j = 0; j < pvsbytes; j++ )
			clientpvs[j] |= pvs[j];
	}

	return i;
}

/*
=================
pfnFindClientInPVS

=================
*/
edict_t* GAME_EXPORT pfnFindClientInPVS( edict_t *pEdict )
{
	edict_t	*pClient;
	vec3_t	view;
	float	delta;
	model_t	*mod;
	qboolean	bMergePVS;
	int	i;

	if( !SV_IsValidEdict( pEdict ))
		return svgame.edicts;

	delta = ( sv.time - sv.lastchecktime );

	// don't merge visibility for portal entity, only for monsters
	bMergePVS = (pEdict->v.flags & FL_MONSTER) ? true : false;

	// find a new check if on a new frame
	if( delta < 0.0f || delta >= 0.1f )
	{
		sv.lastcheck = SV_CheckClientPVS( sv.lastcheck, bMergePVS );
		sv.lastchecktime = sv.time;
	}

	// return check if it might be visible	
	pClient = EDICT_NUM( sv.lastcheck );
	if( !SV_ClientFromEdict( pClient, true ))
		return svgame.edicts;

	mod = Mod_Handle( pEdict->v.modelindex );

	// portals & monitors
	// NOTE: this specific break "radiaton tick" in normal half-life. use only as feature
	if(( host.features & ENGINE_TRANSFORM_TRACE_AABB ) && mod && mod->type == mod_brush && !( mod->flags & MODEL_HAS_ORIGIN ))
	{
		// handle PVS origin for bmodels
		VectorAverage( pEdict->v.mins, pEdict->v.maxs, view );
		VectorAdd( view, pEdict->v.origin, view );
	}
	else
	{
		VectorAdd( pEdict->v.origin, pEdict->v.view_ofs, view );
	}

	if( pEdict->v.effects & EF_INVLIGHT )
		view[2] -= 1.0f; // HACKHACK for barnacle

	i = Mod_PointLeafnum( view ) - 1;

	if( i < 0 || !((clientpvs[i>>3]) & (1U << (i & 7))))
		return svgame.edicts;

	// client which currently in PVS
	return pClient;
}

/*
=================
pfnEntitiesInPVS

=================
*/
edict_t *GAME_EXPORT pfnEntitiesInPVS( edict_t *pview )
{
	edict_t	*chain;
	edict_t	*pEdict, *pEdict2;
	vec3_t	viewpoint;
	int	i;

	if( !SV_IsValidEdict( pview ))
		return NULL;

	VectorAdd( pview->v.origin, pview->v.view_ofs, viewpoint );

	for( chain = EDICT_NUM( 0 ), i = 1; i < svgame.numEntities; i++ )
	{
		pEdict = EDICT_NUM( i );

		if( !SV_IsValidEdict( pEdict ))
			continue;

		if( pEdict->v.movetype == MOVETYPE_FOLLOW && SV_IsValidEdict( pEdict->v.aiment ))
		{
			// force all items across level even it is not visible
			if( pEdict->v.aiment == EDICT_NUM( 1 ))
			{
				pEdict->v.chain = chain;
				chain = pEdict;
				continue;
			}

			pEdict2 = pEdict->v.aiment;
		}
		else
		{
			pEdict2 = pEdict;
		}

		if( SV_BoxInPVS( viewpoint, pEdict2->v.absmin, pEdict2->v.absmax ))
		{
			pEdict->v.chain = chain;
			chain = pEdict;
		}
	}

	return chain;
}

/*
==============
pfnMakeVectors

==============
*/
void GAME_EXPORT pfnMakeVectors( const float *rgflVector )
{
	AngleVectors( rgflVector, svgame.globals->v_forward, svgame.globals->v_right, svgame.globals->v_up );
}

/*
==============
pfnRemoveEntity

free edict private mem, unlink physics etc
==============
*/
void GAME_EXPORT pfnRemoveEntity( edict_t* e )
{
	if( !SV_IsValidEdict( e ))
	{
		MsgDev( D_ERROR, "SV_RemoveEntity: entity already freed\n" );
		return;
	}

	// never free client or world entity
	if( NUM_FOR_EDICT( e ) < ( svgame.globals->maxClients + 1 ))
	{
		MsgDev( D_ERROR, "SV_RemoveEntity: can't delete %s\n", (e == EDICT_NUM( 0 )) ? "world" : "client" );
		return;
	}

	SV_FreeEdict( e );
}

/*
==============
pfnCreateNamedEntity

==============
*/
edict_t* GAME_EXPORT pfnCreateNamedEntity( string_t className )
{
	return SV_AllocPrivateData( NULL, className );
}

/*
=============
pfnMakeStatic

move entity to client (Q1 legacy)
=============
*/
static void GAME_EXPORT pfnMakeStatic( edict_t *ent )
{
	sv_static_entity_t	*clent;

	if( !SV_IsValidEdict( ent ))
	{
		MsgDev( D_WARN, "SV_MakeStatic: invalid entity %s\n", SV_ClassName( ent ));
		return;
	}

	if( sv.num_static_entities >= MAX_STATIC_ENTITIES )
	{
		MsgDev( D_WARN, "SV_MakeStatic: too many static entities. Ignored\n" );
		return;
	}

	clent = &sv.static_entities[sv.num_static_entities++];

	Q_strncpy( clent->model, STRING( ent->v.model ), sizeof( clent->model ));
	VectorCopy( ent->v.origin, clent->origin );
	VectorCopy( ent->v.angles, clent->angles );

	clent->sequence = ent->v.sequence;
	clent->frame = ent->v.frame;
	clent->colormap = ent->v.colormap;
	clent->skin = ent->v.skin;
	clent->rendermode = ent->v.rendermode;
	clent->renderamt = ent->v.renderamt;
	clent->rendercolor.r = ent->v.rendercolor[0];
	clent->rendercolor.g = ent->v.rendercolor[1];
	clent->rendercolor.b = ent->v.rendercolor[2];
	clent->renderfx = ent->v.renderfx;

	SV_CreateStaticEntity( &sv.signon, clent );

	// remove at end of the frame
	ent->v.flags |= FL_KILLME;
}

/*
=============
pfnEntIsOnFloor

legacy builtin
=============
*/
static int GAME_EXPORT pfnEntIsOnFloor( edict_t *e )
{
	if( !SV_IsValidEdict( e ))
	{
		MsgDev( D_WARN, "SV_CheckBottom: invalid entity %s\n", SV_ClassName( e ));
		return 0;
	}

	return SV_CheckBottom( e, MOVE_NORMAL );
}
	
/*
===============
pfnDropToFloor

===============
*/
int GAME_EXPORT pfnDropToFloor( edict_t* e )
{
	vec3_t	end;
	trace_t	trace;

	if( !SV_IsValidEdict( e ))
	{
		MsgDev( D_ERROR, "SV_DropToFloor: invalid entity %s\n", SV_ClassName( e ));
		return 0;
	}

	VectorCopy( e->v.origin, end );
	end[2] -= 256;

	trace = SV_Move( e->v.origin, e->v.mins, e->v.maxs, end, MOVE_NORMAL, e );

	if( trace.allsolid )
		return -1;

	if( trace.fraction == 1.0f )
		return 0;

	VectorCopy( trace.endpos, e->v.origin );
	SV_LinkEdict( e, false );
	e->v.flags |= FL_ONGROUND;
	e->v.groundentity = trace.ent;

	return 1;
}

/*
===============
pfnWalkMove

===============
*/
int GAME_EXPORT pfnWalkMove( edict_t *ent, float yaw, float dist, int iMode )
{
	vec3_t	move;

	if( !SV_IsValidEdict( ent ))
	{
		MsgDev( D_WARN, "SV_WalkMove: invalid entity %s\n", SV_ClassName( ent ));
		return false;
	}

	if(!( ent->v.flags & ( FL_FLY|FL_SWIM|FL_ONGROUND )))
		return false;

	yaw = yaw * M_PI2 / 360.0f;
	VectorSet( move, cos( yaw ) * dist, sin( yaw ) * dist, 0.0f );

	switch( iMode )
	{
	case WALKMOVE_NORMAL:
		return SV_MoveStep( ent, move, true );
	case WALKMOVE_WORLDONLY:
		return SV_MoveTest( ent, move, true );
	case WALKMOVE_CHECKONLY:
		return SV_MoveStep( ent, move, false);
	default:
		MsgDev( D_ERROR, "SV_WalkMove: invalid walk mode %i.\n", iMode );
		break;
	}
	return false;
}

/*
=================
pfnSetOrigin

=================
*/
void GAME_EXPORT pfnSetOrigin( edict_t *e, const float *rgflOrigin )
{
	if( !SV_IsValidEdict( e ))
	{
		MsgDev( D_WARN, "SV_SetOrigin: invalid entity %s\n", SV_ClassName( e ));
		return;
	}

	VectorCopy( rgflOrigin, e->v.origin );
	SV_LinkEdict( e, false );
}

/*
=================
SV_BuildSoundMsg

=================
*/
int SV_BuildSoundMsg( edict_t *ent, int chan, const char *samp, int vol, float attn, int flags, int pitch, const vec3_t pos )
{
	int	sound_idx;
	int	entityIndex;

	if( vol < 0 || vol > 255 )
	{
		MsgDev( D_ERROR, "SV_StartSound: volume = %i\n", vol );
		return 0;
	}

	if( attn < 0.0f || attn > 4.0f )
	{
		MsgDev( D_ERROR, "SV_StartSound: attenuation = %g\n", attn );
		return 0;
	}

	if( chan < 0 || chan > 7 )
	{
		MsgDev( D_ERROR, "SV_StartSound: channel = %i\n", chan );
		return 0;
	}

	if( pitch < 0 || pitch > 255 )
	{
		MsgDev( D_ERROR, "SV_StartSound: pitch = %i\n", pitch );
		return 0;
	}

	if( !samp || !*samp )
	{
		MsgDev( D_ERROR, "SV_StartSound: passed NULL sample\n" );
		return 0;
	}

	if( samp[0] == '!' && Q_isdigit( samp + 1 ))
	{
		flags |= SND_SENTENCE;
		sound_idx = Q_atoi( samp + 1 );

		if( sound_idx >= 1536 )
		{
			MsgDev( D_ERROR, "SV_StartSound: invalid sentence number %s.\n", samp );
			return 0;
		}
	}
	else if( samp[0] == '#' && Q_isdigit( samp + 1 ))
	{
		flags |= SND_SENTENCE;
		sound_idx = Q_atoi( samp + 1 ) + 1536;
	}
	else
	{
		// precache_sound can be used twice: cache sounds when loading
		// and return sound index when server is active
		sound_idx = SV_SoundIndex( samp );
	}

	if( !ent->v.modelindex || !ent->v.model )
		entityIndex = 0;
	else if( SV_IsValidEdict( ent->v.aiment ))
		entityIndex = NUM_FOR_EDICT( ent->v.aiment );
	else entityIndex = NUM_FOR_EDICT( ent );

	if( vol != 255 ) flags |= SND_VOLUME;
	if( attn != ATTN_NONE ) flags |= SND_ATTENUATION;
	if( pitch != PITCH_NORM ) flags |= SND_PITCH;

	if( sound_idx > 255 ) flags |= SND_LARGE_INDEX;

	// not sending (because this is out of range)
	flags &= ~SND_SPAWNING;

	BF_WriteByte( &sv.multicast, svc_sound );
	BF_WriteWord( &sv.multicast, flags );
	if( flags & SND_LARGE_INDEX )
		BF_WriteWord( &sv.multicast, sound_idx );
	else BF_WriteByte( &sv.multicast, sound_idx );
	BF_WriteByte( &sv.multicast, chan );

	if( flags & SND_VOLUME ) BF_WriteByte( &sv.multicast, vol );
	if( flags & SND_ATTENUATION ) BF_WriteByte( &sv.multicast, attn * 64 );
	if( flags & SND_PITCH ) BF_WriteByte( &sv.multicast, pitch );

	BF_WriteWord( &sv.multicast, entityIndex );
	BF_WriteVec3Coord( &sv.multicast, pos );

	return 1;
}

/*
=================
SV_StartSound

=================
*/
void SV_StartSoundEx( edict_t *ent, int chan, const char *sample, float vol, float attn, int flags, int pitch, qboolean excludeSource )
{
	int 	sound_idx;
	int	entityIndex;
	int	msg_dest;
	vec3_t	origin;

	if( !sample ) return;

	if( attn < 0.0f || attn > 4.0f )
	{
		MsgDev( D_ERROR, "SV_StartSound: attenuation %g must be in range 0-4\n", attn );
		return;
	}

	if( chan < 0 || chan > 7 )
	{
		MsgDev( D_ERROR, "SV_StartSound: channel must be in range 0-7\n" );
		return;
	}

	if( !SV_IsValidEdict( ent ))
	{
		MsgDev( D_ERROR, "SV_StartSound: edict == NULL\n" );
		return;
	}

	if( vol != VOL_NORM ) flags |= SND_VOLUME;
	if( attn != ATTN_NONE ) flags |= SND_ATTENUATION;
	if( pitch != PITCH_NORM ) flags |= SND_PITCH;

	VectorAverage( ent->v.mins, ent->v.maxs, origin );
	VectorAdd( origin, ent->v.origin, origin );

	if( flags & SND_SPAWNING )
		msg_dest = MSG_INIT;
	else if( chan == CHAN_STATIC )
		msg_dest = MSG_ALL;
	else msg_dest = MSG_PAS_R;

	// always sending stop sound command
	if( flags & SND_STOP ) msg_dest = MSG_ALL;

	if( sample[0] == '!' && Q_isdigit( sample + 1 ))
	{
		flags |= SND_SENTENCE;
		sound_idx = Q_atoi( sample + 1 );
	}
	else if( sample[0] == '#' && Q_isdigit( sample + 1 ))
	{
		flags |= SND_SENTENCE;
		sound_idx = Q_atoi( sample + 1 ) + 1536;
	}
	else
	{
		// precache_sound can be used twice: cache sounds when loading
		// and return sound index when server is active
		sound_idx = SV_SoundIndex( sample );
	}

	if( SV_IsValidEdict( ent->v.aiment ))
		entityIndex = NUM_FOR_EDICT( ent->v.aiment );
	else entityIndex = NUM_FOR_EDICT( ent );

	if( sound_idx > 255 ) flags |= SND_LARGE_INDEX;

	// not sending (because this is out of range)
	flags &= ~SND_SPAWNING;

	BF_WriteByte( &sv.multicast, svc_sound );
	BF_WriteWord( &sv.multicast, flags );
	if( flags & SND_LARGE_INDEX )
		BF_WriteWord( &sv.multicast, sound_idx );
	else BF_WriteByte( &sv.multicast, sound_idx );
	BF_WriteByte( &sv.multicast, chan );

	if( flags & SND_VOLUME ) BF_WriteByte( &sv.multicast, vol * 255 );
	if( flags & SND_ATTENUATION ) BF_WriteByte( &sv.multicast, attn * 64 );
	if( flags & SND_PITCH ) BF_WriteByte( &sv.multicast, pitch );

	BF_WriteWord( &sv.multicast, entityIndex );
	BF_WriteVec3Coord( &sv.multicast, origin );

	SV_Send( msg_dest, origin, ent, excludeSource );
}

/*
=================
SV_StartSound

=================
*/
void GAME_EXPORT SV_StartSound( edict_t *ent, int chan, const char *sample, float vol, float attn, int flags, int pitch )
{
	SV_StartSoundEx( ent, chan, sample, vol, attn, flags, pitch, false );
}

/*
=================
pfnEmitAmbientSound

=================
*/
void GAME_EXPORT pfnEmitAmbientSound( edict_t *ent, float *pos, const char *sample, float vol, float attn, int flags, int pitch )
{
	int 	number = 0, sound_idx;
	int	msg_dest = MSG_PAS_R;

	if( !sample ) return;

	if( attn < 0.0f || attn > 4.0f )
	{
		MsgDev( D_ERROR, "SV_AmbientSound: attenuation must be in range 0-4\n" );
		return;
	}

	if( !pos )
	{
		MsgDev( D_ERROR, "SV_AmbientSound: pos == NULL!\n" );
		return;
	}

	if( sv.state == ss_loading ) flags |= SND_SPAWNING;
	if( vol != VOL_NORM ) flags |= SND_VOLUME;
	if( attn != ATTN_NONE ) flags |= SND_ATTENUATION;
	if( pitch != PITCH_NORM ) flags |= SND_PITCH;

	if( flags & SND_SPAWNING )
		msg_dest = MSG_INIT;
	else msg_dest = MSG_ALL;

	if( SV_IsValidEdict( ent ))
		number = NUM_FOR_EDICT( ent );

	// always sending stop sound command
	if( flags & SND_STOP ) msg_dest = MSG_ALL;

	if( sample[0] == '!' && Q_isdigit( sample + 1 ))
	{
		flags |= SND_SENTENCE;
		sound_idx = Q_atoi( sample + 1 );
	}
	else if( sample[0] == '#' && Q_isdigit( sample + 1 ))
	{
		flags |= SND_SENTENCE;
		sound_idx = Q_atoi( sample + 1 ) + 1536;
	}
	else
	{
		// precache_sound can be used twice: cache sounds when loading
		// and return sound index when server is active
		sound_idx = SV_SoundIndex( sample );
	}

	if( sound_idx > 255 ) flags |= SND_LARGE_INDEX;

	// not sending (because this is out of range)
	flags &= ~SND_SPAWNING;

	BF_WriteByte( &sv.multicast, svc_ambientsound );
	BF_WriteWord( &sv.multicast, flags );
	if( flags & SND_LARGE_INDEX )
		BF_WriteWord( &sv.multicast, sound_idx );
	else BF_WriteByte( &sv.multicast, sound_idx );
	BF_WriteByte( &sv.multicast, CHAN_STATIC );

	if( flags & SND_VOLUME ) BF_WriteByte( &sv.multicast, vol * 255 );
	if( flags & SND_ATTENUATION ) BF_WriteByte( &sv.multicast, attn * 64 );
	if( flags & SND_PITCH ) BF_WriteByte( &sv.multicast, pitch );

	// plays from fixed position
	BF_WriteWord( &sv.multicast, number );
	BF_WriteVec3Coord( &sv.multicast, pos );

	SV_Send( msg_dest, pos, NULL, false );
}

/*
=================
SV_StartMusic

=================
*/
void SV_StartMusic( const char *curtrack, const char *looptrack, fs_offset_t position )
{
	BF_WriteByte( &sv.multicast, svc_stufftext );
	BF_WriteString( &sv.multicast, va( "music \"%s\" \"%s\" %i\n", curtrack, looptrack, (int)position ));
	SV_Send( MSG_ALL, NULL, NULL, false );
}

/*
=================
pfnTraceLine

=================
*/
static void GAME_EXPORT pfnTraceLine( const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr )
{
	trace_t	trace;

	if( !ptr ) return;

	trace = SV_Move( v1, vec3_origin, vec3_origin, v2, fNoMonsters, pentToSkip );
	if( !SV_IsValidEdict( trace.ent )) trace.ent = svgame.edicts;
	SV_ConvertTrace( ptr, &trace );
}

/*
=================
pfnTraceToss

=================
*/
static void GAME_EXPORT pfnTraceToss( edict_t* pent, edict_t* pentToIgnore, TraceResult *ptr )
{
	trace_t	trace;

	if( !ptr ) return;

	if( !SV_IsValidEdict( pent ))
	{
		MsgDev( D_WARN, "SV_MoveToss: invalid entity %s\n", SV_ClassName( pent ));
		return;
	}

	trace = SV_MoveToss( pent, pentToIgnore );
	SV_ConvertTrace( ptr, &trace );
}

/*
=================
pfnTraceHull

=================
*/
static void GAME_EXPORT pfnTraceHull( const float *v1, const float *v2, int fNoMonsters, int hullNumber, edict_t *pentToSkip, TraceResult *ptr )
{
	float	*mins, *maxs;
	trace_t	trace;

	if( !ptr ) return;

	if( hullNumber < 0 || hullNumber > 3 )
		hullNumber = 0;

	mins = sv.worldmodel->hulls[hullNumber].clip_mins;
	maxs = sv.worldmodel->hulls[hullNumber].clip_maxs;

	trace = SV_Move( v1, mins, maxs, v2, fNoMonsters, pentToSkip );
	SV_ConvertTrace( ptr, &trace );
}

/*
=============
pfnTraceMonsterHull

=============
*/
static int GAME_EXPORT pfnTraceMonsterHull( edict_t *pEdict, const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr )
{
	trace_t	trace;

	if( !SV_IsValidEdict( pEdict ))
	{
		MsgDev( D_WARN, "SV_TraceMonsterHull: invalid entity %s\n", SV_ClassName( pEdict ));
		return 1;
	}

	trace = SV_Move( v1, pEdict->v.mins, pEdict->v.maxs, v2, fNoMonsters, pentToSkip );
	if( ptr ) SV_ConvertTrace( ptr, &trace );

	if( trace.allsolid || trace.fraction != 1.0f )
		return true;
	return false;
}

/*
=============
pfnTraceModel

=============
*/
static void GAME_EXPORT pfnTraceModel( const float *v1, const float *v2, int hullNumber, edict_t *pent, TraceResult *ptr )
{
	float	*mins, *maxs;
	trace_t	trace;

	if( !ptr ) return;

	if( !SV_IsValidEdict( pent ))
	{
		MsgDev( D_WARN, "TraceModel: invalid entity %s\n", SV_ClassName( pent ));
		return;
	}

	if( hullNumber < 0 || hullNumber > 3 )
		hullNumber = 0;

	mins = sv.worldmodel->hulls[hullNumber].clip_mins;
	maxs = sv.worldmodel->hulls[hullNumber].clip_maxs;

	if( pent->v.solid == SOLID_CUSTOM )
	{
		// NOTE: always goes through custom clipping move
		// even if our callbacks is not initialized
		SV_CustomClipMoveToEntity( pent, v1, mins, maxs, v2, &trace );
	}
	else if( Mod_GetType( pent->v.modelindex ) == mod_brush )
	{
		int oldmovetype = pent->v.movetype;
		int oldsolid = pent->v.solid;
		pent->v.movetype = MOVETYPE_PUSH;
		pent->v.solid = SOLID_BSP;

      		SV_ClipMoveToEntity( pent, v1, mins, maxs, v2, &trace );

		pent->v.movetype = oldmovetype;
		pent->v.solid = oldsolid;
	}
	else
	{
      		SV_ClipMoveToEntity( pent, v1, mins, maxs, v2, &trace );
	}

	SV_ConvertTrace( ptr, &trace );
}

/*
=============
pfnTraceTexture

returns texture basename
=============
*/
static const char *GAME_EXPORT pfnTraceTexture( edict_t *pTextureEntity, const float *v1, const float *v2 )
{
	if( !SV_IsValidEdict( pTextureEntity ))
	{
		MsgDev( D_WARN, "TraceTexture: invalid entity %s\n", SV_ClassName( pTextureEntity ));
		return NULL;
	}

	return SV_TraceTexture( pTextureEntity, v1, v2 );
}

/*
=============
pfnTraceSphere

trace sphere instead of bbox
=============
*/
void GAME_EXPORT pfnTraceSphere( const float *v1, const float *v2, int fNoMonsters, float radius, edict_t *pentToSkip, TraceResult *ptr )
{
	Host_Error( "TraceSphere not yet implemented!\n" );
}

/*
=============
pfnGetAimVector

NOTE: speed is unused
=============
*/
void GAME_EXPORT pfnGetAimVector( edict_t* ent, float speed, float *rgflReturn )
{
	edict_t		*check;
	vec3_t		start, dir, end, bestdir;
	float		dist, bestdist;
	int		i, j;
	trace_t		tr;

	VectorCopy( svgame.globals->v_forward, rgflReturn );	// assume failure if it returns early

	if( !SV_IsValidEdict( ent ) || (ent->v.flags & FL_FAKECLIENT))
		return;

	VectorCopy( ent->v.origin, start );
	VectorAdd( start, ent->v.view_ofs, start );

	// try sending a trace straight
	VectorCopy( svgame.globals->v_forward, dir );
	VectorMA( start, 2048, dir, end );
	tr = SV_Move( start, vec3_origin, vec3_origin, end, MOVE_NORMAL, ent );

	if( tr.ent && (tr.ent->v.takedamage == DAMAGE_AIM || ent->v.team <= 0 || ent->v.team != tr.ent->v.team ))
		return;

	// try all possible entities
	VectorCopy( dir, bestdir );
	bestdist = Cvar_VariableValue( "sv_aim" );

	check = EDICT_NUM( 1 ); // start at first client
	for( i = 1; i < svgame.numEntities; i++, check++ )
	{
		if( check->v.takedamage != DAMAGE_AIM ) continue;
      		if( check->v.flags & FL_FAKECLIENT ) continue;
		if( ent->v.team > 0 && ent->v.team == check->v.team ) continue;
		if( check == ent ) continue;
		for( j = 0; j < 3; j++ )
			end[j] = check->v.origin[j] + 0.5f * (check->v.mins[j] + check->v.maxs[j]);
		VectorSubtract( end, start, dir );
		VectorNormalize( dir );
		dist = DotProduct( dir, svgame.globals->v_forward );
		if( dist < bestdist ) continue; // to far to turn
		tr = SV_Move( start, vec3_origin, vec3_origin, end, MOVE_NORMAL, ent );
		if( tr.ent == check )
		{	
			bestdist = dist;
			VectorCopy( dir, bestdir );
		}
	}

	VectorCopy( bestdir, rgflReturn );
}

/*
=========
pfnServerCommand

=========
*/
void GAME_EXPORT pfnServerCommand( const char* str )
{
	if( SV_IsValidCmd( str )) Cbuf_AddText( str );
	else MsgDev( D_ERROR, "bad server command %s\n", str );
}

/*
=========
pfnClientCommand

=========
*/
void GAME_EXPORT pfnClientCommand( edict_t* pEdict, char* szFmt, ... )
{
	sv_client_t	*client;
	string		buffer;
	va_list		args;

	if( sv.state != ss_active )
	{
		MsgDev( D_ERROR, "SV_ClientCommand: server is not active!\n" );
		return;
	}

	if(( client = SV_ClientFromEdict( pEdict, false )) == NULL )
	{
		MsgDev( D_ERROR, "SV_ClientCommand: invalid client!\n" );
		return;
	}

	if( client->fakeclient )
		return;

	va_start( args, szFmt );
	Q_vsnprintf( buffer, MAX_STRING, szFmt, args );
	va_end( args );
	if( SV_IsValidCmd( buffer ))
	{
		BF_WriteByte( &client->netchan.message, svc_stufftext );
		BF_WriteString( &client->netchan.message, buffer );
	}
	else MsgDev( D_ERROR, "Tried to stuff bad command %s\n", buffer );
}

/*
=================
pfnParticleEffect

Make sure the event gets sent to all clients
=================
*/
void GAME_EXPORT pfnParticleEffect( const float *org, const float *dir, float color, float count )
{
	int	i, v;

	if( !org || !dir )
	{
		if( !org ) MsgDev( D_ERROR, "SV_StartParticle: NULL origin. Ignored\n" );
		if( !dir ) MsgDev( D_ERROR, "SV_StartParticle: NULL dir. Ignored\n" );
		return;
	}

	BF_WriteByte( &sv.datagram, svc_particle );
	BF_WriteVec3Coord( &sv.datagram, org );

	for( i = 0; i < 3; i++ )
	{
		v = bound( -128, dir[i] * 16.0f, 127 );
		BF_WriteChar( &sv.datagram, v );
	}

	BF_WriteByte( &sv.datagram, count );
	BF_WriteByte( &sv.datagram, color );
	BF_WriteByte( &sv.datagram, 0 );
}

/*
===============
pfnLightStyle

===============
*/
void GAME_EXPORT pfnLightStyle( int style, const char* val )
{
	if( style < 0 ) style = 0;
	if( style >= MAX_LIGHTSTYLES )
		Host_Error( "SV_LightStyle: style: %i >= %d", style, MAX_LIGHTSTYLES );

	SV_SetLightStyle( style, val, 0.0f ); // set correct style
}

/*
=================
pfnDecalIndex

register decal shader on client
=================
*/
int GAME_EXPORT pfnDecalIndex( const char *m )
{
	int	i;

	if( !m || !m[0] )
		return 0;

	for( i = 1; i < MAX_DECALS && host.draw_decals[i][0]; i++ )
	{
		if( !Q_stricmp( (char *)host.draw_decals[i], m ))
			return i;
	}

	// throw warning (this can happens if decal not present in decals.wad)
	MsgDev( D_WARN, "Can't find decal %s\n", m );

	return 0;	
}

/*
=============
pfnPointContents

=============
*/
static int GAME_EXPORT pfnPointContents( const float *rgflVector )
{
	if( !rgflVector ) return CONTENTS_NONE;
	return SV_PointContents( rgflVector );
}

qboolean gIsUserMsg;

/*
=============
pfnMessageBegin

=============
*/
void GAME_EXPORT pfnMessageBegin( int msg_dest, int msg_num, const float *pOrigin, edict_t *ed )
{
	int	i, iSize;

	if( svgame.msg_started )
		Host_Error( "MessageBegin: New message started when msg '%s' has not been sent yet\n", svgame.msg_name );
	svgame.msg_started = true;

	// check range
	msg_num = bound( svc_bad, msg_num, 255 );

	if( msg_num < svc_lastmsg )
	{
		svgame.msg_index = -msg_num; // this is a system message
		svgame.msg_name = svc_strings[msg_num];

		if( msg_num == svc_temp_entity )
			iSize = -1; // temp entity have variable size
		else iSize = 0;
	}
	else
	{
		// check for existing
		for( i = 0; i < MAX_USER_MESSAGES && svgame.msg[i].name[0]; i++ )
		{
			if( svgame.msg[i].number == msg_num )
				break; // found
		}

		if( i == MAX_USER_MESSAGES )
		{
			Host_Error( "MessageBegin: tried to send unregistered message %i\n", msg_num );
			return;
		}

		svgame.msg_name = svgame.msg[i].name;
		iSize = svgame.msg[i].size;
		svgame.msg_index = i;
	}

	BF_WriteByte( &sv.multicast, msg_num );

	// save message destination
	if( pOrigin ) VectorCopy( pOrigin, svgame.msg_org );
	else VectorClear( svgame.msg_org );

	if( iSize == -1 )
	{
		// variable sized messages sent size as first byte
		svgame.msg_size_index = BF_GetNumBytesWritten( &sv.multicast );
		BF_WriteByte( &sv.multicast, 0 ); // reserve space for now
	}
	else svgame.msg_size_index = -1; // message has constant size

	svgame.msg_realsize = 0;
	svgame.msg_dest = msg_dest;
	svgame.msg_ent = ed;

	// remove engine messages.
	// also remove ReqState, because it's spamming
	gIsUserMsg = (sv_trace_messages->value > 0.0f) && (msg_num > svc_lastmsg) && Q_strcmp( svgame.msg_name, "ReqState" );

	if( gIsUserMsg ) MsgDev( D_AICONSOLE, "^3MessageBegin( %i, %s )\n", msg_dest, svgame.msg_name );
}

/*
=============
pfnMessageEnd

=============
*/
void GAME_EXPORT pfnMessageEnd( void )
{
	const char	*name = "Unknown";
	float		*org = NULL;

	if( svgame.msg_name ) name = svgame.msg_name;
	if( !svgame.msg_started ) Host_Error( "MessageEnd: called with no active message\n" );
	svgame.msg_started = false;

	// HACKHACK: clearing HudText in background mode
	if( sv.background && svgame.msg_index >= 0 && svgame.msg[svgame.msg_index].number == svgame.gmsgHudText )
	{
		BF_Clear( &sv.multicast );
		return;
	}

	// check for system message
	if( svgame.msg_index < 0 )
	{
		if( svgame.msg_size_index != -1 )
		{
			// variable sized message
			if( svgame.msg_realsize > 255 )
			{
				MsgDev( D_ERROR, "SV_Message: %s too long (more than 255 bytes)\n", name );
				BF_Clear( &sv.multicast );
				return;
			}
			else if( svgame.msg_realsize < 0 )
			{
				MsgDev( D_ERROR, "SV_Message: %s writes NULL message\n", name );
				BF_Clear( &sv.multicast );
				return;
			}

			sv.multicast.pData[svgame.msg_size_index] = svgame.msg_realsize;
		}
	}
	else if( svgame.msg[svgame.msg_index].size != -1 )
	{
		int	expsize = svgame.msg[svgame.msg_index].size;
		int	realsize = svgame.msg_realsize;
	
		// compare sizes
		if( expsize != realsize )
		{
			MsgDev( D_ERROR, "SV_Message: %s expected %i bytes, it written %i. Ignored.\n", name, expsize, realsize );
			BF_Clear( &sv.multicast );
			return;
		}
	}
	else if( svgame.msg_size_index != -1 )
	{
		// variable sized message
		if( svgame.msg_realsize > 255 )
		{
			MsgDev( D_ERROR, "SV_Message: %s too long (more than 255 bytes)\n", name );
			BF_Clear( &sv.multicast );
			return;
		}
		else if( svgame.msg_realsize < 0 )
		{
			MsgDev( D_ERROR, "SV_Message: %s writes NULL message\n", name );
			BF_Clear( &sv.multicast );
			return;
		}

		sv.multicast.pData[svgame.msg_size_index] = svgame.msg_realsize;
	}
	else
	{
		// this should never happen
		MsgDev( D_ERROR, "SV_Message: %s have encountered error\n", name );
		BF_Clear( &sv.multicast );
		return;
	}

	if( svgame.msg_index < 0 && abs( svgame.msg_index ) == svc_studiodecal && svgame.msg_realsize == 27 )
	{
		// oldstyle message for svc_studiodecal has missed four additional bytes:
		// modelIndex, skin and body. Write it here for backward compatibility
		BF_WriteWord( &sv.multicast, 0 );
		BF_WriteByte( &sv.multicast, 0 );
		BF_WriteByte( &sv.multicast, 0 );
	}

	if( !VectorIsNull( svgame.msg_org )) org = svgame.msg_org;
	svgame.msg_dest = bound( MSG_BROADCAST, svgame.msg_dest, MSG_SPEC );

	SV_Send( svgame.msg_dest, org, svgame.msg_ent, false );
	if( gIsUserMsg ) MsgDev( D_AICONSOLE, "^3MessageEnd( )\n");
}

/*
=============
pfnWriteByte

=============
*/
void GAME_EXPORT pfnWriteByte( int iValue )
{
	if( iValue == -1 ) iValue = 0xFF; // convert char to byte 
	BF_WriteByte( &sv.multicast, (byte)iValue );
	if( gIsUserMsg ) MsgDev( D_AICONSOLE, "^3    WriteByte( %i )\n", iValue );
	svgame.msg_realsize++;
}

/*
=============
pfnWriteChar

=============
*/
void GAME_EXPORT pfnWriteChar( int iValue )
{
	BF_WriteChar( &sv.multicast, (signed char)iValue );
	if( gIsUserMsg ) MsgDev( D_AICONSOLE, "^3    WriteChar( %i )\n", iValue );
	svgame.msg_realsize++;
}

/*
=============
pfnWriteShort

=============
*/
void GAME_EXPORT pfnWriteShort( int iValue )
{
	BF_WriteShort( &sv.multicast, (short)iValue );
	if( gIsUserMsg ) MsgDev( D_AICONSOLE, "^3    WriteShort( %i )\n", iValue );
	svgame.msg_realsize += 2;
}

/*
=============
pfnWriteLong

=============
*/
void GAME_EXPORT pfnWriteLong( int iValue )
{
	BF_WriteLong( &sv.multicast, iValue );
	if( gIsUserMsg ) MsgDev( D_AICONSOLE, "^3    WriteLong( %i )\n", iValue );
	svgame.msg_realsize += 4;
}

/*
=============
pfnWriteAngle

this is low-res angle
=============
*/
void GAME_EXPORT pfnWriteAngle( float flValue )
{
	int	iAngle = ((int)(( flValue ) * 256 / 360) & 255);

	BF_WriteChar( &sv.multicast, iAngle );
	if( gIsUserMsg ) MsgDev( D_AICONSOLE, "^3    WriteAngle( %f )\n", flValue );
	svgame.msg_realsize += 1;
}

/*
=============
pfnWriteCoord

=============
*/
void GAME_EXPORT pfnWriteCoord( float flValue )
{
	BF_WriteCoord( &sv.multicast, flValue );
	if( gIsUserMsg ) MsgDev( D_AICONSOLE, "^3    WriteCoord( %f )\n", flValue );
	svgame.msg_realsize += 2;
}

/*
=============
pfnWriteString

=============
*/
void GAME_EXPORT pfnWriteString( const char *src )
{
	char	*dst, string[MAX_SYSPATH];
	int	len = Q_strlen( src ) + 1;
	int	rem = (255 - svgame.msg_realsize);

	if( len >= rem )
	{
		MsgDev( D_ERROR, "pfnWriteString: exceeds %i symbols\n", rem );
		BF_WriteChar( &sv.multicast, 0 );
		svgame.msg_realsize += 1; 
		return;
	}

	// prepare string to sending
	dst = string;

	while( src )
	{
		// some escaped chars parsed as two symbols - merge it here
		if( src[0] == '\\' && src[1] == 'n' )
		{
			*dst++ = '\n';
			src += 2;
			len -= 1;
		}
		else if( src[0] == '\\' && src[1] == 'r' )
		{
			*dst++ = '\r';
			src += 2;
			len -= 1;
		}
		else if( src[0] == '\\' && src[1] == 't' )
		{
			*dst++ = '\t';
			src += 2;
			len -= 1;
		}
		else if(( *dst++ = *src++ ) == 0 )
			break;
	}

	*dst = '\0'; // string end (not included in count)
	BF_WriteString( &sv.multicast, string );

	if( gIsUserMsg ) MsgDev( D_AICONSOLE, "^3    WriteString( %s )\n", string );

	// NOTE: some messages with constant string length can be marked as known sized
	svgame.msg_realsize += len;
}

/*
=============
pfnWriteEntity

=============
*/
void GAME_EXPORT pfnWriteEntity( int iValue )
{
	if( iValue < 0 || iValue >= svgame.numEntities )
		Host_Error( "BF_WriteEntity: invalid entnumber %i\n", iValue );
	BF_WriteShort( &sv.multicast, (short)iValue );
	if( gIsUserMsg ) MsgDev( D_AICONSOLE, "^3    WriteEntity( %i )\n", iValue );
	svgame.msg_realsize += 2;
}

/*
=============
pfnAlertMessage

=============
*/
static void GAME_EXPORT pfnAlertMessage( ALERT_TYPE level, char *szFmt, ... )
{
	char	buffer0[2048];	// must support > 1k messages
	va_list	args;
	char *buffer = buffer0;
	va_start( args, szFmt );
	Q_vsnprintf( buffer0, 2048, szFmt, args );
	va_end( args );

	if( ( level == at_logged ) && sv_maxclients->integer > 1 )
	{
		Log_Printf("%s", buffer0 );
		return;
	}

	// check message for pass
	switch( level )
	{
	case at_notice:
		break;	// passed always
	case at_console:
		if( host.developer < D_INFO )
			return;
		break;
	case at_aiconsole:
		if( host.developer < D_AICONSOLE )
			return;
		break;
	case at_warning:
		if( host.developer < D_WARN )
			return;
		break;
	case at_error:
		if( host.developer < D_ERROR )
			return;
		break;
	default:
		break;
	}

	if( *buffer == '\n' ) // skip \n in line start
		buffer++;

	if( level == at_warning )
	{
		Sys_Print( va( "^3Warning:^7 %s", buffer ));
	}
	else if( level == at_error  )
	{
		Sys_Print( va( "^1Error:^7 %s", buffer ));
	}
	else if( level == at_aiconsole  )
	{
		Sys_Print( va( "server(ai): %s", buffer ));
	}
	else
	{
		Sys_Print( va( "server: %s", buffer ));
	}

}

/*
=============
pfnEngineFprintf

legacy. probably was a part of early save\restore system
=============
*/
static void GAME_EXPORT pfnEngineFprintf( FILE *pfile, char *szFmt, ... )
{
	char	buffer[2048];
	va_list	args;

	va_start( args, szFmt );
	Q_vsnprintf( buffer, 2048, szFmt, args );
	va_end( args );

	fputs( buffer, pfile );
}
	
/*
=============
pfnBuildSoundMsg

Customizable sound message
=============
*/
void GAME_EXPORT pfnBuildSoundMsg( edict_t *pSource, int chan, const char *samp, float fvol, float attn, int fFlags, int pitch, int msg_dest, int msg_type, const float *pOrigin, edict_t *pSend )
{
	pfnMessageBegin( msg_dest, msg_type, pOrigin, pSend );
	SV_BuildSoundMsg( pSource, chan, samp, fvol * 255, attn, fFlags, pitch, pOrigin );
	pfnMessageEnd();
}

/*
=============
pfnPvAllocEntPrivateData

=============
*/
void *GAME_EXPORT pfnPvAllocEntPrivateData( edict_t *pEdict, long cb )
{
	ASSERT( pEdict );

	SV_FreePrivateData( pEdict );

	if( cb > 0 )
	{
		// a poke646 have memory corrupt in somewhere - this is trashed last four bytes :(
		pEdict->pvPrivateData = Mem_Alloc( svgame.mempool, (cb + 15) & ~15 );
	}

	return pEdict->pvPrivateData;
}

/*
=============
pfnPvEntPrivateData

we already have copy of this function in 'enginecallback.h' :-)
=============
*/
void *GAME_EXPORT pfnPvEntPrivateData( edict_t *pEdict )
{
	if( pEdict )
		return pEdict->pvPrivateData;
	return NULL;
}

/*
=============
pfnFreeEntPrivateData

=============
*/
void GAME_EXPORT pfnFreeEntPrivateData( edict_t *pEdict )
{
	SV_FreePrivateData( pEdict );
}

#ifdef XASH_64BIT
static struct str64_s
{
	size_t maxstringarray;
	qboolean allowdup;
	char *staticstringarray;
	char *pstringarray;
	char *pstringarraystatic;
	char *pstringbase;
	char *poldstringbase;
	char *plast;
	qboolean dynamic;
	size_t maxalloc;
	size_t numdups;
	size_t numoverflows;
	size_t totalalloc;
} str64;
#endif

/*
==================
SV_EmptyStringPool

Free strings on server stop. Reset string pointer on 64 bits
==================
*/
void SV_EmptyStringPool( void )
{
#ifdef XASH_64BIT
	if( str64.dynamic ) // switch only after array fill (more space for multiplayer games)
		str64.pstringbase = str64.pstringarray;
	else
	{
		str64.pstringbase = str64.poldstringbase = str64.pstringarraystatic;
		str64.plast = str64.pstringbase + 1;
	}
#else
	Mem_EmptyPool( svgame.stringspool );
#endif
}

/*
===============
SV_SetStringArrayMode

use different arrays on 64 bit platforms
set dynamic after complete server spawn
this helps not to lose strings that belongs to static game part
===============
*/
void SV_SetStringArrayMode( qboolean dynamic )
{
#ifdef XASH_64BIT
	MsgDev( D_NOTE, "SV_SetStringArrayMode(%d) %d\n", dynamic, str64.dynamic );

	if( dynamic == str64.dynamic )
		return;

	str64.dynamic = dynamic;

	SV_EmptyStringPool();
#endif
}

#ifdef XASH_64BIT
#if !defined(_WIN32) && !defined(__APPLE__)
#define USE_MMAP
#include <sys/mman.h>
#endif
#endif

/*
==================
SV_AllocStringPool

alloc string pool on 32bit platforms
alloc string array near the server library on 64bit platforms if possible
alloc string array somewhere if not (MAKE_STRING will not work. Always call ALLOC_STRING instead, or crash)
this case need patched game dll with MAKE_STRING checking ptrdiff size
==================
*/
void SV_AllocStringPool( void )
{
#ifdef XASH_64BIT
	void *ptr = NULL;
	string lenstr;

	MsgDev( D_NOTE, "SV_AllocStringPool()\n" );
	if( Sys_GetParmFromCmdLine( "-str64alloc", lenstr ) )
	{
		str64.maxstringarray = Q_atoi( lenstr );
		if( str64.maxstringarray < 1024 || str64.maxstringarray >= INT_MAX )
			str64.maxstringarray = 65536;
	}
	else str64.maxstringarray = 65536;
	if( Sys_CheckParm( "-str64dup" ) )
		str64.allowdup = true;

#ifdef USE_MMAP
	{
		size_t pagesize = sysconf( _SC_PAGESIZE );
		int arrlen = ( str64.maxstringarray * 2 ) & ~( pagesize - 1 );
		void *base = svgame.dllFuncs.pfnGameInit;
		void *start = svgame.hInstance - arrlen;

		while( start - base > INT_MIN )
		{
			void *mapptr = mmap((void*)((unsigned long)start & ~(pagesize - 1)), arrlen, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0 );
			if( mapptr && mapptr != (void*)-1 && mapptr - base > INT_MIN && mapptr - base < INT_MAX )
			{
				ptr = mapptr;
				break;
			}
			if( mapptr ) munmap( mapptr, arrlen );
			start -= arrlen;
		}

		if( !ptr )
		{
			start = base;
			while( start - base < INT_MAX )
			{
				void *mapptr = mmap((void*)((unsigned long)start & ~(pagesize - 1)), arrlen, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0 );
				if( mapptr && mapptr != (void*)-1  && mapptr - base > INT_MIN && mapptr - base < INT_MAX )
				{
					ptr = mapptr;
					break;
				}
				if( mapptr ) munmap( mapptr, arrlen );
				start += arrlen;
			}
		}


		if( ptr )
		{
			MsgDev( D_NOTE, "SV_AllocStringPool: Allocated string array near the server library: %p %p\n", base, ptr );

		}
		else
		{
			MsgDev( D_WARN, "SV_AllocStringPool: Failed to allocate string array near the server library!\n" );
			ptr = str64.staticstringarray = Mem_Alloc(host.mempool, str64.maxstringarray * 2);
		}
	}
#else
	ptr = str64.staticstringarray = Mem_Alloc(host.mempool, str64.maxstringarray * 2);
#endif

	str64.pstringarray = ptr;
	str64.pstringarraystatic = (byte *)ptr + str64.maxstringarray;
	str64.pstringbase = str64.poldstringbase = ptr;
	str64.plast = (byte *)ptr + 1;
	svgame.globals->pStringBase = ptr;
#else
	svgame.stringspool = Mem_AllocPool( "Server Strings" );
	svgame.globals->pStringBase = "";
#endif
}

void SV_FreeStringPool( void )
{
#ifdef XASH_64BIT
	MsgDev( D_NOTE, "SV_FreeStringPool()\n" );

#ifdef USE_MMAP
	if( str64.pstringarray != str64.staticstringarray )
		munmap( str64.pstringarray, (str64.maxstringarray * 2) & ~(sysconf( _SC_PAGESIZE ) - 1) );
	else
#endif // USE_MMAP
		Mem_Free( str64.staticstringarray );
#else
	Mem_FreePool( &svgame.stringspool );
#endif
}

/*
=============
SV_AllocString

allocate new engine string
on 64bit platforms find in array string if deduplication enabled (default)
if not found, add to array
use -str64dup to disable deduplication, -str64alloc to set array size
=============
*/
string_t GAME_EXPORT SV_AllocString( const char *szValue )
{
	const char *newString = NULL;

	if( svgame.physFuncs.pfnAllocString != NULL )
		return svgame.physFuncs.pfnAllocString( szValue );
#ifdef XASH_64BIT
	int cmp = 1;

	if( !str64.allowdup )
		for( newString = str64.poldstringbase + 1; newString < str64.plast && ( cmp = Q_strcmp( newString, szValue ) ); newString += Q_strlen( newString ) + 1 );

	if( cmp )
	{
		uint len = Q_strlen( szValue );

		if( str64.plast - str64.poldstringbase + len + 2 > str64.maxstringarray )
		{
			str64.plast = str64.pstringbase + 1;
			str64.poldstringbase = str64.pstringbase;
			str64.numoverflows++;
		}

		//MsgDev( D_NOTE, "SV_AllocString: %ld %s\n", str64.plast - svgame.globals->pStringBase, szValue );
		Q_memcpy( str64.plast, szValue, len + 1 );
		str64.totalalloc += len + 1;

		newString = str64.plast;
		str64.plast += len + 1;
	}
	else
		str64.numdups++;
		//MsgDev( D_NOTE, "SV_AllocString: dup %ld %s\n", newString - svgame.globals->pStringBase, szValue );

	if( newString - str64.pstringarray > str64.maxalloc )
		str64.maxalloc = newString - str64.pstringarray;

	return newString - svgame.globals->pStringBase;
#else
	newString = _copystring( svgame.stringspool, szValue, __FILE__, __LINE__ );
	return newString - svgame.globals->pStringBase;
#endif
}

#ifdef XASH_64BIT
void SV_PrintStr64Stats_f( void )
{
	Msg( "====================\n" );
	Msg( "64 bit string pool statistics\n" );
	Msg( "====================\n" );
	Msg( "string array size: %lu\n", str64.maxstringarray );
	Msg( "total alloc %lu\n", str64.totalalloc );
	Msg( "maximum array usage: %lu\n", str64.maxalloc );
	Msg( "overflow counter: %lu\n", str64.numoverflows );
	Msg( "dup string counter: %lu\n", str64.numdups );
}
#endif

/*
=============
SV_MakeString

make constant string
=============
*/
string_t SV_MakeString( const char *szValue )
{
	if( svgame.physFuncs.pfnMakeString != NULL )
		return svgame.physFuncs.pfnMakeString( szValue );
#ifdef XASH_64BIT
	{
		long long ptrdiff = szValue - svgame.globals->pStringBase;
		if( ptrdiff > INT_MAX || ptrdiff < INT_MIN )
			return SV_AllocString(szValue);
		else
			return (int)ptrdiff;
	}
#else
	return szValue - svgame.globals->pStringBase;
#endif
}


/*
=============
SV_GetString

=============
*/
const char *GAME_EXPORT SV_GetString( string_t iString )
{
	if( svgame.physFuncs.pfnGetString != NULL )
		return svgame.physFuncs.pfnGetString( iString );
	return (svgame.globals->pStringBase + iString);
}

/*
=============
pfnGetVarsOfEnt

=============
*/
entvars_t *GAME_EXPORT pfnGetVarsOfEnt( edict_t *pEdict )
{
	if( pEdict )
		return &pEdict->v;
	return NULL;
}

/*
=============
pfnPEntityOfEntOffset

=============
*/
edict_t* GAME_EXPORT pfnPEntityOfEntOffset( int iEntOffset )
{
	return (&((edict_t*)svgame.vp)[iEntOffset]);
}

/*
=============
pfnEntOffsetOfPEntity

=============
*/
int GAME_EXPORT pfnEntOffsetOfPEntity( const edict_t *pEdict )
{
	return ((byte *)pEdict - (byte *)svgame.vp);
}

/*
=============
pfnIndexOfEdict

=============
*/
int GAME_EXPORT pfnIndexOfEdict( const edict_t *pEdict )
{
	int	number;

	number = NUM_FOR_EDICT( pEdict );
	if( number < 0 || number >= svgame.numEntities )
		return 0;	// out of range
	return number;
}

/*
=============
pfnPEntityOfEntIndex

=============
*/
edict_t* GAME_EXPORT pfnPEntityOfEntIndex( int iEntIndex )
{
	if( iEntIndex < 0 || iEntIndex >= svgame.numEntities )
		return NULL; // out of range

	return EDICT_NUM( iEntIndex );
}

/*
=============
pfnFindEntityByVars

debug routine
=============
*/
edict_t* GAME_EXPORT pfnFindEntityByVars( entvars_t *pvars )
{
	edict_t	*e;
	int	i;

	// don't pass invalid arguments
	if( !pvars ) return NULL;

	for( i = 0; i < svgame.numEntities; i++ )
	{
		e = EDICT_NUM( i );

		if( &e->v == pvars )
		{
			if( e->v.pContainingEntity != e )
			{
				MsgDev( D_NOTE, "fixing pContainingEntity for %s\n", SV_ClassName( e ));
				e->v.pContainingEntity = e;
			}
			return e;	// found it
		}
	}
	return NULL;
}

/*
=============
pfnGetModelPtr

returns pointer to a studiomodel
=============
*/
static void *GAME_EXPORT pfnGetModelPtr( edict_t *pEdict )
{
	model_t	*mod;

	if( !SV_IsValidEdict( pEdict ))
		return NULL;

	mod = Mod_Handle( pEdict->v.modelindex );
	return Mod_Extradata( mod );
}

/*
=============
pfnRegUserMsg

=============
*/
int GAME_EXPORT pfnRegUserMsg( const char *pszName, int iSize )
{
	int	i;
	
	if( !pszName || !pszName[0] )
		return svc_bad;

	if( Q_strlen( pszName ) >= sizeof( svgame.msg[0].name ))
	{
		MsgDev( D_ERROR, "REG_USER_MSG: too long name %s\n", pszName );
		return svc_bad; // force error
	}

	if( iSize > 255 )
	{
		MsgDev( D_ERROR, "REG_USER_MSG: %s has too big size %i\n", pszName, iSize );
		return svc_bad; // force error
	}

	// make sure what size inrange
	iSize = bound( -1, iSize, 255 );

	// message 0 is reserved for svc_bad
	for( i = 0; i < MAX_USER_MESSAGES && svgame.msg[i].name[0]; i++ )
	{
		// see if already registered
		if( !Q_strcmp( svgame.msg[i].name, pszName ))
			return svc_lastmsg + i; // offset
	}

	if( i == MAX_USER_MESSAGES ) 
	{
		MsgDev( D_ERROR, "REG_USER_MSG: user messages limit exceeded\n" );
		return svc_bad;
	}

	// register new message
	Q_strncpy( svgame.msg[i].name, pszName, sizeof( svgame.msg[i].name ));
	svgame.msg[i].number = svc_lastmsg + i;
	svgame.msg[i].size = iSize;

	// catch some user messages
	if( !Q_strcmp( pszName, "HudText" ))
		svgame.gmsgHudText = svc_lastmsg + i;

	if( sv.state == ss_active )
	{
		// tell the client about new user message
		BF_WriteByte( &sv.multicast, svc_usermessage );
		BF_WriteByte( &sv.multicast, svgame.msg[i].number );
		BF_WriteByte( &sv.multicast, (byte)iSize );
		BF_WriteString( &sv.multicast, svgame.msg[i].name );
		SV_Send( MSG_ALL, NULL, NULL, false );
	}

	if( sv_trace_messages->integer )
	{
		MsgDev( D_NOTE, "^3Added user message: %s (size == %i)\n", pszName, iSize );
	}

	return svgame.msg[i].number;
}

/*
=============
pfnAnimationAutomove

animating studiomodel
=============
*/
void GAME_EXPORT pfnAnimationAutomove( const edict_t* pEdict, float flTime )
{
	// this is empty in the original HL
}

/*
=============
pfnGetBonePosition

=============
*/
static void GAME_EXPORT pfnGetBonePosition( const edict_t* pEdict, int iBone, float *rgflOrigin, float *rgflAngles )
{
	if( !SV_IsValidEdict( pEdict ))
	{
		MsgDev( D_WARN, "SV_GetBonePos: invalid entity %s\n", SV_ClassName( pEdict ));
		return;
	}

	Mod_GetBonePosition( pEdict, iBone, rgflOrigin, rgflAngles );
}

/*
=============
pfnFunctionFromName

=============
*/
void *GAME_EXPORT pfnFunctionFromName( const char *pName )
{
	return Com_FunctionFromName_SR( svgame.hInstance, pName );
}

/*
=============
pfnNameForFunction

=============
*/
const char *GAME_EXPORT pfnNameForFunction( void *function )
{
	return Com_NameForFunction( svgame.hInstance, (void *)function );
}

/*
=============
pfnClientPrintf

=============
*/
void GAME_EXPORT pfnClientPrintf( edict_t* pEdict, PRINT_TYPE ptype, const char *szMsg )
{
	sv_client_t	*client;

	if( sv.state != ss_active )
	{
		// send message into console during loading
		MsgDev( D_INFO, "%s\n", szMsg );
		return;
	}

	if(( client = SV_ClientFromEdict( pEdict, true )) == NULL )
	{
		MsgDev( D_ERROR, "SV_ClientPrintf: client is not spawned!\n" );
		return;
	}

	switch( ptype )
	{
	case print_console:
		if( client->fakeclient ) MsgDev( D_INFO, "%s", szMsg );
		else SV_ClientPrintf( client, PRINT_HIGH, "%s", szMsg );
		break;
	case print_chat:
		if( client->fakeclient ) return;
		SV_ClientPrintf( client, PRINT_CHAT, "%s", szMsg );
		break;
	case print_center:
		if( client->fakeclient ) return;
		BF_WriteByte( &client->netchan.message, svc_centerprint );
		BF_WriteString( &client->netchan.message, szMsg );
		break;
	}
}

/*
=============
pfnServerPrint

=============
*/
void GAME_EXPORT pfnServerPrint( const char *szMsg )
{
#if 0
	// while loading in-progress we can sending message only for local client
	if( sv.state != ss_active ) MsgDev( D_INFO, "%s", szMsg );
	else SV_BroadcastPrintf( PRINT_HIGH, "%s", szMsg );
#else
	MsgDev( D_INFO, "%s", szMsg );
#endif
}

/*
=============
pfnGetAttachment

=============
*/
static void GAME_EXPORT pfnGetAttachment( const edict_t *pEdict, int iAttachment, float *rgflOrigin, float *rgflAngles )
{
	if( !SV_IsValidEdict( pEdict ))
	{
		MsgDev( D_WARN, "SV_GetAttachment: invalid entity %s\n", SV_ClassName( pEdict ));
		return;
	}

	Mod_StudioGetAttachment( pEdict, iAttachment, rgflOrigin, rgflAngles );
}

/*
=============
pfnCRC32_Final

=============
*/
dword GAME_EXPORT pfnCRC32_Final( dword pulCRC )
{
	CRC32_Final( &pulCRC );

	return pulCRC;
}				

/*
=============
pfnCrosshairAngle

=============
*/
void GAME_EXPORT pfnCrosshairAngle( const edict_t *pClient, float pitch, float yaw )
{
	sv_client_t	*client;

	if(( client = SV_ClientFromEdict( pClient, true )) == NULL )
	{
		MsgDev( D_ERROR, "SV_SetCrosshairAngle: invalid client!\n" );
		return;
	}

	// fakeclients ignores it silently
	if( client->fakeclient ) return;

	if( pitch > 180.0f ) pitch -= 360;
	if( pitch < -180.0f ) pitch += 360;
	if( yaw > 180.0f ) yaw -= 360;
	if( yaw < -180.0f ) yaw += 360;

	BF_WriteByte( &client->netchan.message, svc_crosshairangle );
	BF_WriteChar( &client->netchan.message, pitch * 5 );
	BF_WriteChar( &client->netchan.message, yaw * 5 );
}

/*
=============
pfnSetView

=============
*/
void GAME_EXPORT pfnSetView( const edict_t *pClient, const edict_t *pViewent )
{
	sv_client_t	*client;

	if( pClient == NULL || pClient->free )
	{
		MsgDev( D_ERROR, "PF_SetView: invalid client!\n" );
		return;
	}

	if(( client = SV_ClientFromEdict( pClient, true )) == NULL )
	{
		MsgDev( D_ERROR, "PF_SetView: not a client!\n" );
		return;
	}

	if( !SV_IsValidEdict( pViewent ))
	{
		MsgDev( D_ERROR, "PF_SetView: invalid viewent!\n" );
		return;
	}

	if( pClient == pViewent ) client->pViewEntity = NULL;
	else client->pViewEntity = (edict_t *)pViewent;

	// fakeclients ignore to send client message (but can see into the trigger_camera through the PVS)
	if( client->fakeclient ) return;

	BF_WriteByte( &client->netchan.message, svc_setview );
	BF_WriteWord( &client->netchan.message, NUM_FOR_EDICT( pViewent ));
}

/*
=============
pfnTime

=============
*/
float GAME_EXPORT pfnTime( void )
{
	return (float)Sys_DoubleTime();
}

/*
=============
pfnStaticDecal

=============
*/
void GAME_EXPORT pfnStaticDecal( const float *origin, int decalIndex, int entityIndex, int modelIndex )
{
	if( !origin )
	{
		MsgDev( D_ERROR, "SV_StaticDecal: NULL origin. Ignored\n" );
		return;
	}

	SV_CreateDecal( &sv.signon, origin, decalIndex, entityIndex, modelIndex, FDECAL_PERMANENT, 1.0f );
}

/*
=============
pfnIsDedicatedServer

=============
*/
int GAME_EXPORT pfnIsDedicatedServer( void )
{
	return (Host_IsDedicated());
}

/*
=============
pfnGetPlayerWONId

=============
*/
uint GAME_EXPORT pfnGetPlayerWONId( edict_t *e )
{
	sv_client_t	*cl;
	int		i;

	if( sv.state != ss_active )
		return -1;

	if( !SV_ClientFromEdict( e, false ))
		return -1;

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->edict == e && cl->WonID )
			return cl->WonID;
	}

	return -1;
}

/*
=============
pfnIsMapValid

vaild map must contain one info_player_deatchmatch
=============
*/
int GAME_EXPORT pfnIsMapValid( const char *filename )
{
	char	*spawn_entity;
	int	flags;

	// determine spawn entity classname
	if( sv_maxclients->integer == 1 )
		spawn_entity = GI->sp_entity;
	else spawn_entity = GI->mp_entity;

	flags = SV_MapIsValid( filename, spawn_entity, NULL );

	if(( flags & MAP_IS_EXIST ) && ( flags & MAP_HAS_SPAWNPOINT ))
		return true;
	return false;
}

/*
=============
pfnFadeClientVolume

=============
*/
void GAME_EXPORT pfnFadeClientVolume( const edict_t *pEdict, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds )
{
	sv_client_t	*cl;

	if(( cl = SV_ClientFromEdict( pEdict, true )) == NULL )
	{
		MsgDev( D_ERROR, "SV_FadeClientVolume: client is not spawned!\n" );
		return;
	}

	if( cl->fakeclient ) return;

	BF_WriteByte( &cl->netchan.message, svc_soundfade );
	BF_WriteByte( &cl->netchan.message, fadePercent );
	BF_WriteByte( &cl->netchan.message, holdTime );
	BF_WriteByte( &cl->netchan.message, fadeOutSeconds );
	BF_WriteByte( &cl->netchan.message, fadeInSeconds );
}

/*
=============
pfnSetClientMaxspeed

fakeclients can be changed speed to
=============
*/
void GAME_EXPORT pfnSetClientMaxspeed( const edict_t *pEdict, float fNewMaxspeed )
{
	sv_client_t	*cl;

	// not spawned clients allowed
	if(( cl = SV_ClientFromEdict( pEdict, false )) == NULL )
	{
		MsgDev( D_ERROR, "SV_SetClientMaxspeed: client is not active!\n" );
		return;
	}

	SV_SetClientMaxspeed( cl, fNewMaxspeed );
}

/*
=============
pfnRunPlayerMove

=============
*/
void GAME_EXPORT pfnRunPlayerMove( edict_t *pClient, const float *v_angle, float fmove, float smove, float upmove, word buttons, byte impulse, byte msec )
{
	sv_client_t	*cl, *oldcl;
	usercmd_t		cmd;
	uint		seed;

	if( sv.paused ) return;

	if(( cl = SV_ClientFromEdict( pClient, true )) == NULL )
	{
		MsgDev( D_ERROR, "SV_ClientThink: fakeclient is not spawned!\n" );
		return;
	}

	if( !cl->fakeclient )
		return;	// only fakeclients allows

	oldcl = svs.currentPlayer;

	svs.currentPlayer = SV_ClientFromEdict( pClient, true );
	svs.currentPlayerNum = (svs.currentPlayer - svs.clients);
	svs.currentPlayer->timebase = (sv.time + host.frametime) - (msec / 1000.0f);

	Q_memset( &cmd, 0, sizeof( cmd ));
	if( v_angle ) VectorCopy( v_angle, cmd.viewangles );
	cmd.forwardmove = fmove;
	cmd.sidemove = smove;
	cmd.upmove = upmove;
	cmd.buttons = buttons;
	cmd.impulse = impulse;
	cmd.msec = msec;

	seed = Com_RandomLong( 0, 0x7fffffff ); // full range

	SV_RunCmd( cl, &cmd, seed );

	cl->lastcmd = cmd;
	cl->lastcmd.buttons = 0; // avoid multiple fires on lag

	svs.currentPlayer = oldcl;
	svs.currentPlayerNum = (svs.currentPlayer - svs.clients);
}

/*
=============
pfnNumberOfEntities

returns actual entity count
=============
*/
int GAME_EXPORT pfnNumberOfEntities( void )
{
	int	i, total = 0;

	for( i = 0; i < svgame.numEntities; i++ )
	{
		if( svgame.edicts[i].free )
			continue;
		total++;
	}

	return total;
}

/*
=============
pfnGetInfoKeyBuffer

=============
*/
char *GAME_EXPORT pfnGetInfoKeyBuffer( edict_t *e )
{
	sv_client_t *cl;

	if ( !SV_IsValidEdict( e ) )
		return localinfo;

	if ( !NUM_FOR_EDICT( e ) )
		return Cvar_Serverinfo( );

	// pfnUserInfoChanged passed
	if ( ( cl = SV_ClientFromEdict( e, false ) ) == NULL )
	{
		MsgDev( D_ERROR, "SV_GetClientUserinfo: client is not connected!\n" );
		return "";
	}

	return cl->userinfo;
}

/*
=============
pfnSetKeyValue

=============
*/
void pfnSetKeyValue( char *infobuffer, const char *key, const char *value )
{
	if ( infobuffer == localinfo )
	{
		Info_SetValueForKey( infobuffer, key, value, MAX_LOCALINFO );
		return;
	}
	else
	{
		Info_SetValueForKey( infobuffer, key, value, MAX_INFO_STRING );
		return;
	}
}

/*
=============
pfnSetClientKeyValue

=============
*/
void GAME_EXPORT pfnSetClientKeyValue( int clientIndex, char *infobuffer, char *key, char *value )
{
	clientIndex -= 1;

	if( clientIndex < 0 || clientIndex >= sv_maxclients->integer )
		return;

	if( svs.clients[clientIndex].state < cs_spawned || infobuffer == NULL )
		return;

	Info_SetValueForStarKey( infobuffer, key, value, MAX_INFO_STRING );
	svs.clients[clientIndex].sendinfo = true;
}

/*
=============
pfnGetPhysicsKeyValue

=============
*/
const char *GAME_EXPORT pfnGetPhysicsKeyValue( const edict_t *pClient, const char *key )
{
	sv_client_t	*cl;

	// pfnUserInfoChanged passed
	if(( cl = SV_ClientFromEdict( pClient, false )) == NULL )
	{
		MsgDev( D_ERROR, "SV_GetClientPhysKey: client is not connected!\n" );
		return "";
	}

	return Info_ValueForKey( cl->physinfo, key );
}

/*
=============
pfnSetPhysicsKeyValue

=============
*/
void GAME_EXPORT pfnSetPhysicsKeyValue( const edict_t *pClient, const char *key, const char *value )
{
	sv_client_t	*cl;

	// pfnUserInfoChanged passed
	if(( cl = SV_ClientFromEdict( pClient, false )) == NULL )
	{
		MsgDev( D_ERROR, "SV_SetClientPhysinfo: client is not connected!\n" );
		return;
	}

	Info_SetValueForKey( cl->physinfo, key, value, sizeof( cl->physinfo ) );
}

/*
=============
pfnGetPhysicsInfoString

=============
*/
const char *GAME_EXPORT pfnGetPhysicsInfoString( const edict_t *pClient )
{
	sv_client_t	*cl;

	// pfnUserInfoChanged passed
	if(( cl = SV_ClientFromEdict( pClient, false )) == NULL )
	{
		MsgDev( D_ERROR, "SV_GetClientPhysinfo: client is not connected!\n" );
		return "";
	}

	return cl->physinfo;
}

/*
=============
pfnPrecacheEvent

register or returns already registered event id
a type of event is ignored at this moment
=============
*/
word GAME_EXPORT pfnPrecacheEvent( int type, const char *psz )
{
	return (word)SV_EventIndex( psz );
}

/*
=============
pfnPlaybackEvent

=============
*/
void GAME_EXPORT SV_PlaybackEventFull( int flags, const edict_t *pInvoker, word eventindex, float delay, float *origin,
	float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 )
{
	sv_client_t	*cl;
	event_state_t	*es;
	event_args_t	args;
	event_info_t	*ei = NULL;
	int		j, slot, bestslot;
	int		invokerIndex;
	byte		*mask = NULL;
	vec3_t		pvspoint;

	if( flags & FEV_CLIENT )
		return;	// someone stupid joke

	// first check event for out of bounds
	if( eventindex < 1 || eventindex > MAX_EVENTS )
	{
		MsgDev( D_ERROR, "SV_PlaybackEvent: invalid eventindex %i\n", eventindex );
		return;
	}

	// check event for precached
	if( !sv.event_precache[eventindex][0] )
	{
		MsgDev( D_ERROR, "SV_PlaybackEvent: event %i was not precached\n", eventindex );
		return;		
	}

	Q_memset( &args, 0, sizeof( args ));

	if( origin && !VectorIsNull( origin ))
	{
		VectorCopy( origin, args.origin );
		args.flags |= FEVENT_ORIGIN;
	}

	if( angles && !VectorIsNull( angles ))
	{
		VectorCopy( angles, args.angles );
		args.flags |= FEVENT_ANGLES;
	}

	// copy other parms
	args.fparam1 = fparam1;
	args.fparam2 = fparam2;
	args.iparam1 = iparam1;
	args.iparam2 = iparam2;
	args.bparam1 = bparam1;
	args.bparam2 = bparam2;

	VectorClear( pvspoint );

	if( SV_IsValidEdict( pInvoker ))
	{
		// add the view_ofs to avoid problems with crossed contents line
		VectorAdd( pInvoker->v.origin, pInvoker->v.view_ofs, pvspoint );
		args.entindex = invokerIndex = NUM_FOR_EDICT( pInvoker );

		// g-cont. allow 'ducking' param for all entities
		args.ducking = (pInvoker->v.flags & FL_DUCKING) ? true : false;

		// this will be send only for reliable event
		if(!( args.flags & FEVENT_ORIGIN ))
			VectorCopy( pInvoker->v.origin, args.origin );

		// this will be send only for reliable event
		if(!( args.flags & FEVENT_ANGLES ))
			VectorCopy( pInvoker->v.angles, args.angles );

		if( sv_sendvelocity->integer )
			VectorCopy( pInvoker->v.velocity, args.velocity );
	}
	else
	{
		VectorCopy( args.origin, pvspoint );
		args.entindex = 0;
		invokerIndex = -1;
	}

	if(!( flags & FEV_GLOBAL ) && VectorIsNull( pvspoint ))
	{
		MsgDev( D_ERROR, "%s: not a FEV_GLOBAL event missing origin. Ignored.\n", sv.event_precache[eventindex] );
		return;
	}

	// check event for some user errors
	if( flags & (FEV_NOTHOST|FEV_HOSTONLY))
	{
		if( !SV_ClientFromEdict( pInvoker, true ))
		{
			const char *ev_name = sv.event_precache[eventindex];
			if( flags & FEV_NOTHOST )
			{
				MsgDev( D_WARN, "%s: specified FEV_NOTHOST when invoker not a client\n", ev_name );
				flags &= ~FEV_NOTHOST;
			}

			if( flags & FEV_HOSTONLY )
			{
				MsgDev( D_WARN, "%s: specified FEV_HOSTONLY when invoker not a client\n", ev_name );
				flags &= ~FEV_HOSTONLY;
			}
		}
	}

	flags |= FEV_SERVER;		// it's a server event!
	if( delay < 0.0f ) delay = 0.0f;	// fixup negative delays

	if(!( flags & FEV_GLOBAL ))
	{
		mleaf_t	*leaf;

		// setup pvs cluster for invoker
		leaf = Mod_PointInLeaf( pvspoint, sv.worldmodel->nodes );
		mask = Mod_LeafPVS( leaf, sv.worldmodel );
	}

	// process all the clients
	for( slot = 0, cl = svs.clients; slot < sv_maxclients->integer; slot++, cl++ )
	{
		if( cl->state != cs_spawned || !cl->edict || cl->fakeclient )
			continue;

		if( SV_IsValidEdict( pInvoker ) && pInvoker->v.groupinfo && cl->edict->v.groupinfo )
		{
			if(( svs.groupop == 0 && (cl->edict->v.groupinfo & pInvoker->v.groupinfo) == 0 )
			|| ( svs.groupop == 1 && (cl->edict->v.groupinfo & pInvoker->v.groupinfo) == 1 ))
				continue;
		}

		if( SV_IsValidEdict( pInvoker ))
		{
			if( !SV_CheckClientVisiblity( cl, mask ))
				continue;
		}

		if( flags & FEV_NOTHOST && cl == svs.currentPlayer && cl->local_weapons )
			continue;	// will be played on client side

		if( flags & FEV_HOSTONLY && cl->edict != pInvoker )
			continue;	// sending only to invoker

		// all checks passed, send the event

		// reliable event
		if( flags & FEV_RELIABLE )
		{
			// skipping queue, write direct into reliable datagram
			SV_PlaybackReliableEvent( &cl->netchan.message, eventindex, delay, &args );
			continue;
		}

		// unreliable event (stores in queue)
		es = &cl->events;
		bestslot = -1;

		if( flags & FEV_UPDATE )
		{
			for( j = 0; j < MAX_EVENT_QUEUE; j++ )
			{
				ei = &es->ei[j];
				if( ei->index == eventindex && invokerIndex != -1 && invokerIndex == ei->entity_index )
				{
					bestslot = j;
					break;
				}
			}
		}

		if( bestslot == -1 )
		{
			for( j = 0; j < MAX_EVENT_QUEUE; j++ )
			{
				ei = &es->ei[j];
		
				if( ei->index == 0 )
				{
					// found an empty slot
					bestslot = j;
					break;
				}
			}

			// g-cont. probably this code never calls (and not needs)
			if( bestslot != -1 && j == MAX_EVENT_QUEUE )
			{
				ei = &es->ei[bestslot];
			}
		}
				
		// no slot found for this player, oh well
		if( bestslot == -1 ) continue;

		// add event to queue
		ei->index = eventindex;
		ei->fire_time = delay;
		ei->entity_index = invokerIndex;
		ei->packet_index = -1;
		ei->flags = flags;
		ei->args = args;
	}
}

/*
=============
pfnSetFatPVS

The client will interpolate the view position,
so we can't use a single PVS point
=============
*/
byte *GAME_EXPORT pfnSetFatPVS( const float *org )
{
	if( !sv.worldmodel->visdata || sv_novis->integer || !org || CL_DisableVisibility( ))
		return Mod_DecompressVis( NULL );

	ASSERT( svs.currentPlayerNum >= 0 && svs.currentPlayerNum < MAX_CLIENTS );

	fatbytes = (sv.worldmodel->numleafs+31)>>3;
	bitvector = fatpvs;

	// portals can't change viewpoint!
	if(!( sv.hostflags & SVF_PORTALPASS ))
	{
		vec3_t	viewPos, offset;

		// see code from client.cpp for understanding:
		// org = pView->v.origin + pView->v.view_ofs;
		// if ( pView->v.flags & FL_DUCKING )
		// {
		//	org = org + ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
		// }
		// so we have unneeded duck calculations who have affect when player
		// is ducked into water. Remove offset to restore right PVS position
		if( svs.currentPlayer->edict->v.flags & FL_DUCKING )
		{
			VectorSubtract( svgame.pmove->player_mins[0], svgame.pmove->player_mins[1], offset );
			VectorSubtract( org, offset, viewPos );
		}
		else VectorCopy( org, viewPos );

		// build a new PVS frame
		Q_memset( bitvector, 0, fatbytes );

		SV_AddToFatPVS( viewPos, DVIS_PVS, sv.worldmodel->nodes );
		VectorCopy( viewPos, viewPoint[svs.currentPlayerNum] );
	}
	else
	{
		SV_AddToFatPVS( org, DVIS_PVS, sv.worldmodel->nodes );
	}

	return bitvector;
}

/*
=============
pfnSetFatPHS

The client will interpolate the hear position,
so we can't use a single PHS point
=============
*/
byte *GAME_EXPORT pfnSetFatPAS( const float *org )
{
	if( !sv.worldmodel->visdata || sv_novis->integer || !org || CL_DisableVisibility( ))
		return Mod_DecompressVis( NULL );

	ASSERT( svs.currentPlayerNum >= 0 && svs.currentPlayerNum < MAX_CLIENTS );

	fatbytes = (sv.worldmodel->numleafs+31)>>3;
	bitvector = fatphs;

	// portals can't change viewpoint!
	if(!( sv.hostflags & SVF_PORTALPASS ))
	{
		vec3_t	viewPos, offset;

		// see code from client.cpp for understanding:
		// org = pView->v.origin + pView->v.view_ofs;
		// if ( pView->v.flags & FL_DUCKING )
		// {
		//	org = org + ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
		// }
		// so we have unneeded duck calculations who have affect when player
		// is ducked into water. Remove offset to restore right PVS position
		if( svs.currentPlayer->edict->v.flags & FL_DUCKING )
		{
			VectorSubtract( svgame.pmove->player_mins[0], svgame.pmove->player_mins[1], offset );
			VectorSubtract( org, offset, viewPos );
		}
		else VectorCopy( org, viewPos );

		// build a new PHS frame
		Q_memset( bitvector, 0, fatbytes );

		SV_AddToFatPVS( viewPos, DVIS_PHS, sv.worldmodel->nodes );
	}
	else
	{
		// merge PVS
		SV_AddToFatPVS( org, DVIS_PHS, sv.worldmodel->nodes );
	}

	return bitvector;
}

/*
=============
pfnCheckVisibility

=============
*/
int GAME_EXPORT pfnCheckVisibility( const edict_t *ent, byte *pset )
{
	int	i;

	if( !SV_IsValidEdict( ent ))
	{
		MsgDev( D_WARN, "SV_CheckVisibility: invalid entity %s\n", SV_ClassName( ent ));
		return 0;
	}

	// vis not set - fullvis enabled
	if( !pset ) return 1;

	if( ent->v.flags & FL_CUSTOMENTITY && ent->v.owner && ent->v.owner->v.flags & FL_CLIENT )
		ent = ent->v.owner;	// upcast beams to my owner

	if( ent->headnode < 0 )
	{
		// check individual leafs
		for( i = 0; i < ent->num_leafs; i++ )
		{
			if( pset[ent->leafnums[i] >> 3] & (1U << (ent->leafnums[i] & 7 )))
				return 1;	// visible passed by leaf
		}

		return 0;
	}
	else
	{
		int	leafnum;

		for( i = 0; i < MAX_ENT_LEAFS; i++ )
		{
			leafnum = ent->leafnums[i];
			if( leafnum == -1 ) break;
			if( pset[leafnum >> 3] & (1U << ( leafnum & 7 )))
				return 1;	// visible passed by leaf
		}

		// too many leafs for individual check, go by headnode
		if( !SV_HeadnodeVisible( &sv.worldmodel->nodes[ent->headnode], pset, &leafnum ))
			return 0;

		((edict_t *)ent)->leafnums[ent->num_leafs] = leafnum;
		((edict_t *)ent)->num_leafs = (ent->num_leafs + 1) % MAX_ENT_LEAFS;

		return 2;	// visible passed by headnode
	}
}

/*
=============
pfnCanSkipPlayer

=============
*/
int GAME_EXPORT pfnCanSkipPlayer( const edict_t *player )
{
	sv_client_t	*cl;

	if(( cl = SV_ClientFromEdict( player, false )) == NULL )
		return false;

	return cl->local_weapons;
}

/*
=============
pfnGetCurrentPlayer

=============
*/
int GAME_EXPORT pfnGetCurrentPlayer( void )
{
	if( svs.currentPlayer )
		return (svs.currentPlayer - svs.clients);
	return -1;
}

/*
=============
pfnSetGroupMask

=============
*/
void GAME_EXPORT pfnSetGroupMask( int mask, int op )
{
	svs.groupmask = mask;
	svs.groupop = op;
}

/*
=============
pfnCreateInstancedBaseline

=============
*/
int GAME_EXPORT pfnCreateInstancedBaseline( int classname, struct entity_state_s *baseline )
{
	int	i;

	if( !baseline ) return -1;

	i = sv.instanced.count;
	if( i > 62 ) return 0;

	sv.instanced.classnames[i] = classname;
	sv.instanced.baselines[i] = *baseline;
	sv.instanced.count++;

	return i+1;
}

/*
=============
pfnEndSection

=============
*/
void GAME_EXPORT pfnEndSection( const char *pszSection )
{
	if( !Q_stricmp( "oem_end_credits", pszSection ))
		Host_Credits ();
	else Cbuf_AddText( va( "endgame \"%s\"\n", pszSection ));
}

/*
=============
pfnGetPlayerUserId

=============
*/
int GAME_EXPORT pfnGetPlayerUserId( edict_t *e )
{
	sv_client_t	*cl;
	int		i;
		
	if( sv.state != ss_active )
		return -1;

	if( !SV_ClientFromEdict( e, false ))
		return -1;

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->edict == e )
			return cl->userid;
	}

	// couldn't find it
	return -1;
}

/*
=============
pfnGetPlayerStats

=============
*/
void GAME_EXPORT pfnGetPlayerStats( const edict_t *pClient, int *ping, int *packet_loss )
{
	sv_client_t	*cl;

	if(( cl = SV_ClientFromEdict( pClient, false )) == NULL )
	{
		MsgDev( D_ERROR, "SV_GetPlayerStats: client is not connected!\n" );
		return;
	}

	if( ping ) *ping = cl->latency * 1000; // this is should be cl->latency not ping!
	if( packet_loss ) *packet_loss = cl->packet_loss;
}
	
/*
=============
pfnForceUnmodified

=============
*/
void GAME_EXPORT pfnForceUnmodified( FORCE_TYPE type, float *mins, float *maxs, const char *filename )
{
	sv_consistency_t	*pData;
	int		i;

	if( !filename || !*filename )
		Host_Error( "SV_ForceUnmodified: bad filename string.\n" );

	if( sv.state == ss_loading )
	{
		for( i = 0, pData = sv.consistency_files; i < MAX_MODELS; i++, pData++ )
		{
			if( !pData->name )
			{
				pData->name = filename;
				pData->force_state = type;

				if( mins ) VectorCopy( mins, pData->mins );
				if( maxs ) VectorCopy( maxs, pData->maxs );
				return;
			}
			else if( !Q_strcmp( filename, pData->name ))
				return;
		}

		Host_Error( "SV_ForceUnmodified: MAX_MODELS limit exceeded\n" );
	}
	else
	{
		for( i = 0, pData = sv.consistency_files; i < MAX_MODELS; i++, pData++ )
		{
			if( pData->name && !Q_strcmp( filename, pData->name ))
				return;
		}

		Host_Error( "SV_ForceUnmodified: can only be done during precache\n" );
	}
}

/*
=============
pfnVoice_GetClientListening

=============
*/
qboolean GAME_EXPORT pfnVoice_GetClientListening( int iReceiver, int iSender )
{
	int	iMaxClients = sv_maxclients->integer;

	if( !svs.initialized ) return false;

	if( iReceiver <= 0 || iReceiver > iMaxClients || iSender <= 0 || iSender > iMaxClients )
	{
		MsgDev( D_ERROR, "Voice_GetClientListening: invalid client indexes (%i, %i).\n", iReceiver, iSender );
		return false;
	}

	return ((svs.clients[iSender-1].listeners & ( 1U << iReceiver )) != 0 );
}

/*
=============
pfnVoice_SetClientListening

=============
*/
qboolean GAME_EXPORT pfnVoice_SetClientListening( int iReceiver, int iSender, qboolean bListen )
{
	int	iMaxClients = sv_maxclients->integer;

	if( !svs.initialized ) return false;

	if( iReceiver <= 0 || iReceiver > iMaxClients || iSender <= 0 || iSender > iMaxClients )
	{
		MsgDev( D_ERROR, "Voice_SetClientListening: invalid client indexes (%i, %i).\n", iReceiver, iSender );
		return false;
	}

	if( bListen )
	{
		svs.clients[iSender-1].listeners |= (1U << iReceiver);
	}
	else
	{
		svs.clients[iSender-1].listeners &= ~(1U << iReceiver);
	}
	return true;
}

/*
=============
pfnGetPlayerAuthId

These function must returns cd-key hashed value
but Xash3D currently doesn't have any security checks
return nullstring for now
=============
*/
const char *GAME_EXPORT pfnGetPlayerAuthId( edict_t *e )
{
	static string	authIds[8];
	static int count = -1;

	sv_client_t	*cl;
	char *result;
	int i;

	count = (count + 1) & 7;

	result = authIds[count];
	result[0] = '\0';

	if( sv.state != ss_active || !SV_IsValidEdict( e ))
		return result;

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->edict == e )
		{
			if( cl->fakeclient )
			{
				Q_strncat( result, "BOT", MAX_STRING );
			}
			else if( cl->hltv_proxy )
			{
				Q_strncat( result, "HLTV", MAX_STRING );
			}
			else if( cl->authentication_method == 0 )
			{
				Q_snprintf( result, MAX_STRING, "%u", (uint)cl->WonID );
			}
			else
			{
				Q_snprintf( result, MAX_STRING, "%s", SV_GetClientIDString( cl ));
			}

			return result;
		}
	}

	return result;
}

/*
=============
pfnGetFileSize

returns the filesize in bytes
=============
*/
int GAME_EXPORT pfnGetFileSize( char *filename )
{
	return FS_FileSize( filename, false );
}

/*
=============
pfnGetLocalizedStringLength

used by CS:CZ (client stub)
=============
*/
int GAME_EXPORT pfnGetLocalizedStringLength( const char *label )
{
	return 0;
}

/*
=============
pfnQueryClientCvarValue

request client cvar value
=============
*/
void GAME_EXPORT pfnQueryClientCvarValue( const edict_t *player, const char *cvarName )
{
	sv_client_t *cl;

	if( !cvarName || !*cvarName )
	{
		MsgDev( D_ERROR, "QueryClientCvarValue: NULL cvar name!\n" );
		return;
	}

	if(( cl = SV_ClientFromEdict( player, false )) != NULL )
	{
		BF_WriteByte( &cl->netchan.message, svc_querycvarvalue );
		BF_WriteString( &cl->netchan.message, cvarName );
	}
	else
	{
		if( svgame.dllFuncs2.pfnCvarValue )
			svgame.dllFuncs2.pfnCvarValue( player, "Bad Player" );
		MsgDev( D_ERROR, "QueryClientCvarValue: tried to send to a non-client!\n" );
	}
}

/*
=============
pfnQueryClientCvarValue2

request client cvar value (bugfixed)
=============
*/
void GAME_EXPORT pfnQueryClientCvarValue2( const edict_t *player, const char *cvarName, int requestID )
{
	sv_client_t *cl;

	if( !cvarName || !*cvarName )
	{
		MsgDev( D_ERROR, "QueryClientCvarValue: NULL cvar name!\n" );
		return;
	}

	if(( cl = SV_ClientFromEdict( player, false )) != NULL )
	{
		BF_WriteByte( &cl->netchan.message, svc_querycvarvalue2 );
		BF_WriteLong( &cl->netchan.message, requestID );
		BF_WriteString( &cl->netchan.message, cvarName );
	}
	else
	{
		if( svgame.dllFuncs2.pfnCvarValue2 )
			svgame.dllFuncs2.pfnCvarValue2( player, requestID, cvarName, "Bad Player" );
		MsgDev( D_ERROR, "QueryClientCvarValue: tried to send to a non-client!\n" );
	}
}

/*
=============
pfnCheckParm

=============
*/
static int GAME_EXPORT pfnCheckParm( char *parm, char **ppnext )
{
	int i = Sys_CheckParm( parm );

	if( ppnext != NULL )
	{
		if( i > 0 && i < host.argc - 1 )
			*ppnext = (char*)host.argv[i + 1];
		else
			*ppnext = NULL;
	}
	return i;
}
					
// engine callbacks
static enginefuncs_t gEngfuncs = 
{
	(void*)pfnPrecacheModel,
	(void*)SV_SoundIndex,
	pfnSetModel,
	pfnModelIndex,
	pfnModelFrames,
	pfnSetSize,	
	(void*)pfnChangeLevel,
	pfnGetSpawnParms,
	pfnSaveSpawnParms,
	pfnVecToYaw,
	VectorAngles,
	pfnMoveToOrigin,
	pfnChangeYaw,
	pfnChangePitch,
	SV_FindEntityByString,
	pfnGetEntityIllum,
	pfnFindEntityInSphere,
	pfnFindClientInPVS,
	pfnEntitiesInPVS,
	pfnMakeVectors,
	AngleVectors,
	SV_AllocEdict,
	pfnRemoveEntity,
	pfnCreateNamedEntity,
	pfnMakeStatic,
	pfnEntIsOnFloor,
	pfnDropToFloor,
	pfnWalkMove,
	pfnSetOrigin,
	SV_StartSound,
	pfnEmitAmbientSound,
	pfnTraceLine,
	pfnTraceToss,
	pfnTraceMonsterHull,
	pfnTraceHull,
	pfnTraceModel,
	pfnTraceTexture,
	pfnTraceSphere,
	pfnGetAimVector,
	(void*)pfnServerCommand,
	Cbuf_Execute,
	pfnClientCommand,
	pfnParticleEffect,
	(void*)pfnLightStyle,
	pfnDecalIndex,
	pfnPointContents,
	pfnMessageBegin,
	pfnMessageEnd,
	pfnWriteByte,
	pfnWriteChar,
	pfnWriteShort,
	pfnWriteLong,
	pfnWriteAngle,
	pfnWriteCoord,
	pfnWriteString,
	pfnWriteEntity,
	Cvar_RegisterVariable,
	Cvar_VariableValue,
	(void*)Cvar_VariableString,
	Cvar_SetFloat,
	Cvar_Set,
	pfnAlertMessage,
	pfnEngineFprintf,
	pfnPvAllocEntPrivateData,
	pfnPvEntPrivateData,
	pfnFreeEntPrivateData,
	SV_GetString,
	SV_AllocString,
	pfnGetVarsOfEnt,
	pfnPEntityOfEntOffset,
	pfnEntOffsetOfPEntity,
	pfnIndexOfEdict,
	pfnPEntityOfEntIndex,
	pfnFindEntityByVars,
	pfnGetModelPtr,
	pfnRegUserMsg,
	pfnAnimationAutomove,
	pfnGetBonePosition,
	(void*)pfnFunctionFromName,
	(void*)pfnNameForFunction,
	pfnClientPrintf,
	pfnServerPrint,	
	(void*)Cmd_Args,
	(void*)Cmd_Argv,
	Cmd_Argc,
	pfnGetAttachment,
	(void*)CRC32_Init,
	(void*)CRC32_ProcessBuffer,
	(void*)CRC32_ProcessByte,
	(void*)pfnCRC32_Final,
	(void*)Com_RandomLong,
	Com_RandomFloat,
	pfnSetView,
	pfnTime,
	pfnCrosshairAngle,
	(void*)COM_LoadFileForMe,
	COM_FreeFile,
	pfnEndSection,
	(void*)COM_CompareFileTime,
	pfnGetGameDir,
	Cvar_RegisterVariable,
	pfnFadeClientVolume,
	pfnSetClientMaxspeed,
	SV_FakeConnect,
	pfnRunPlayerMove,
	pfnNumberOfEntities,
	pfnGetInfoKeyBuffer,
	(void*)Info_ValueForKey,
	(void*)pfnSetKeyValue,
	pfnSetClientKeyValue,
	pfnIsMapValid,
	pfnStaticDecal,
	(void*)SV_GenericIndex,
	pfnGetPlayerUserId,
	pfnBuildSoundMsg,
	pfnIsDedicatedServer,
	pfnCVarGetPointer,
	pfnGetPlayerWONId,
	(void*)Info_RemoveKey,
	pfnGetPhysicsKeyValue,
	pfnSetPhysicsKeyValue,
	pfnGetPhysicsInfoString,
	pfnPrecacheEvent,
	SV_PlaybackEventFull,
	(void*)pfnSetFatPVS,
	(void*)pfnSetFatPAS,
	pfnCheckVisibility,
	Delta_SetField,
	Delta_UnsetField,
	Delta_AddEncoder,
	pfnGetCurrentPlayer,
	pfnCanSkipPlayer,
	Delta_FindField,
	Delta_SetFieldByIndex,
	Delta_UnsetFieldByIndex,
	pfnSetGroupMask,	
	pfnCreateInstancedBaseline,
	(void*)Cvar_DirectSet,
	pfnForceUnmodified,
	pfnGetPlayerStats,
	(void*)Cmd_AddGameCommand,
	pfnVoice_GetClientListening,
	pfnVoice_SetClientListening,
	pfnGetPlayerAuthId,
	pfnSequenceGet,
	pfnSequencePickSentence,
	pfnGetFileSize,
	Sound_GetApproxWavePlayLen,
	pfnIsCareerMatch,
	pfnGetLocalizedStringLength,
	pfnRegisterTutorMessageShown,
	pfnGetTimesTutorMessageShown,
	pfnProcessTutorMessageDecayBuffer,
	pfnConstructTutorMessageDecayBuffer,
	pfnResetTutorMessageDecayData,
	pfnQueryClientCvarValue,
	pfnQueryClientCvarValue2,
	pfnCheckParm,
};

/*
====================
SV_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
====================
*/
qboolean SV_ParseEdict( char **pfile, edict_t *ent )
{
	KeyValueData	pkvd[256]; // per one entity
	int		i, numpairs = 0;
	const char	*classname = NULL;
	char		token[2048];

	// go through all the dictionary pairs
	while( 1 )
	{	
		string	keyname;

		// parse key
		if(( *pfile = COM_ParseFile( *pfile, token )) == NULL )
			Host_Error( "ED_ParseEdict: EOF without closing brace\n" );
		if( token[0] == '}' ) break; // end of desc

		Q_strncpy( keyname, token, sizeof( keyname ));

		// parse value	
		if(( *pfile = COM_ParseFile( *pfile, token )) == NULL ) 
			Host_Error( "ED_ParseEdict: EOF without closing brace\n" );

		if( token[0] == '}' )
			Host_Error( "ED_ParseEdict: closing brace without data\n" );

		// ignore attempts to set key ""
		if( !keyname[0] ) continue;

		// "wad" field is completely ignored in Xash3D
		if( !Q_strcmp( keyname, "wad" ))
			continue;

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by engine
		if( world.version == Q1BSP_VERSION && keyname[0] == '_' )
			continue;

		// ignore attempts to set value ""
		if( !token[0] ) continue;

		// create keyvalue strings
		pkvd[numpairs].szClassName = (char *)classname;	// unknown at this moment
		pkvd[numpairs].szKeyName = copystring( keyname );
		pkvd[numpairs].szValue = copystring( token );
		pkvd[numpairs].fHandled = false;		

		if( !Q_strcmp( keyname, "classname" ) && classname == NULL )
			classname = pkvd[numpairs].szValue;
		if( ++numpairs >= 256 ) break;
	}
	
	ent = SV_AllocPrivateData( ent, ALLOC_STRING( classname ));

	if( !SV_IsValidEdict( ent ) || ent->v.flags & FL_KILLME )
	{
		// release allocated strings
		for( i = 0; i < numpairs; i++ )
		{
			Mem_Free( pkvd[i].szKeyName );
			Mem_Free( pkvd[i].szValue );
		}
		return false;
	}

	for( i = 0; i < numpairs; i++ )
	{
		if( !Q_strcmp( pkvd[i].szKeyName, "angle" ))
		{
			float	flYawAngle = Q_atof( pkvd[i].szValue );

			Mem_Free( pkvd[i].szKeyName ); // will be replace with 'angles'
			Mem_Free( pkvd[i].szValue );	// release old value, so we don't need these
			pkvd[i].szKeyName = copystring( "angles" );

			if( flYawAngle >= 0.0f )
				pkvd[i].szValue = copystring( va( "%g %g %g", ent->v.angles[0], flYawAngle, ent->v.angles[2] ));
			else if( flYawAngle == -1.0f )
				pkvd[i].szValue = copystring( "-90 0 0" );
			else if( flYawAngle == -2.0f )
				pkvd[i].szValue = copystring( "90 0 0" );
			else pkvd[i].szValue = copystring( "0 0 0" ); // technically an error
		}

		if( !Q_strcmp( pkvd[i].szKeyName, "light" ))
		{
			Mem_Free( pkvd[i].szKeyName );
			pkvd[i].szKeyName = copystring( "light_level" );
		}

		if( !pkvd[i].fHandled )
		{
			pkvd[i].szClassName = (char *)classname;
			svgame.dllFuncs.pfnKeyValue( ent, &pkvd[i] );
		}

		// no reason to keep this data
		Mem_Free( pkvd[i].szKeyName );
		Mem_Free( pkvd[i].szValue );
	}

	return true;
}

/*
================
SV_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
================
*/
void SV_LoadFromFile( const char *mapname, char *entities )
{
	char	token[2048];
	qboolean	create_world = true;
	int	inhibited;
	edict_t	*ent;

	ASSERT( entities != NULL );

	// user dll can override spawn entities function (Xash3D extension)
	if( !svgame.physFuncs.SV_LoadEntities || !svgame.physFuncs.SV_LoadEntities( mapname, entities ))
	{
		inhibited = 0;

		// parse ents
		while(( entities = COM_ParseFile( entities, token )) != NULL )
		{
			if( token[0] != '{' )
				Host_Error( "ED_LoadFromFile: found %s when expecting {\n", token );

			if( create_world )
			{
				create_world = false;
				ent = EDICT_NUM( 0 ); // already initialized
			}
			else ent = SV_AllocEdict();

			if( !SV_ParseEdict( &entities, ent ))
				continue;

			if( svgame.dllFuncs.pfnSpawn( ent ) == -1 )
			{
				// game rejected the spawn
				if( !( ent->v.flags & FL_KILLME ))
				{
					SV_FreeEdict( ent );
					inhibited++;
				}
			}
		}

		MsgDev( D_INFO, "SV_LoadFromFile: %i entities inhibited\n", inhibited );
	}

	// reset world origin and angles for some reason
	VectorClear( svgame.edicts->v.origin );
	VectorClear( svgame.edicts->v.angles );
}

/*
==============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
==============
*/
void SV_SpawnEntities( const char *mapname, char *entities )
{
	edict_t	*ent;

	MsgDev( D_NOTE, "SV_SpawnEntities( %s )\n", sv.name );

	// reset misc parms
	Cvar_Reset( "sv_zmax" );
	Cvar_Reset( "sv_wateramp" );
	Cvar_Reset( "sv_wateralpha" );

	// reset sky parms	
	Cvar_Reset( "sv_skycolor_r" );
	Cvar_Reset( "sv_skycolor_g" );
	Cvar_Reset( "sv_skycolor_b" );
	Cvar_Reset( "sv_skyvec_x" );
	Cvar_Reset( "sv_skyvec_y" );
	Cvar_Reset( "sv_skyvec_z" );
	Cvar_Reset( "sv_skyname" );
	Cvar_Reset( "sv_skydir_x" );
	Cvar_Reset( "sv_skydir_y" );
	Cvar_Reset( "sv_skydir_z" );
	Cvar_Reset( "sv_skyangle" );
	Cvar_Reset( "sv_skyspeed" );

	ent = EDICT_NUM( 0 );
	if( ent->free ) SV_InitEdict( ent );
	ent->v.model = MAKE_STRING( sv.model_precache[1] );
	ent->v.modelindex = 1; // world model
	ent->v.solid = SOLID_BSP;
	ent->v.movetype = MOVETYPE_PUSH;
	svgame.movevars.fog_settings = 0;

	svgame.globals->maxEntities = GI->max_edicts;
	svgame.globals->maxClients = sv_maxclients->integer;
	svgame.globals->mapname = MAKE_STRING( sv.name );
	svgame.globals->startspot = MAKE_STRING( sv.startspot );
	svgame.globals->time = sv.time;

	// spawn the rest of the entities on the map
	SV_LoadFromFile( mapname, entities );

	// free memory that allocated by entpatch only
	if( !Mem_IsAllocatedExt( sv.worldmodel->mempool, entities ))
		Mem_Free( entities );

	MsgDev( D_NOTE, "Total %i entities spawned\n", svgame.numEntities );
}

void SV_UnloadProgs( void )
{
	MsgDev( D_NOTE, "SV_UnloadProgs()\n");

	if( !svgame.hInstance )
		return;
	
	SV_DeactivateServer ();
	Delta_Shutdown ();

	SV_FreeStringPool();


	if( svgame.dllFuncs2.pfnGameShutdown != NULL )
		svgame.dllFuncs2.pfnGameShutdown ();

	// now we can unload cvars
	Cvar_FullSet( "host_gameloaded", "0", CVAR_INIT );
	Cvar_FullSet( "sv_background", "0", CVAR_READ_ONLY );

	// must unlink all game cvars,
	// before pointers on them will be lost...
	Cmd_ExecuteString( "@unlink\n", src_command );
	Cmd_Unlink( CMD_EXTDLL );

	Mod_ResetStudioAPI ();
	Com_FreeLibrary( svgame.hInstance );
	Mem_FreePool( &svgame.mempool );
	Q_memset( &svgame, 0, sizeof( svgame ));
}

qboolean SV_LoadProgs( const char *name )
{
	int			i, version;
	static APIFUNCTION		GetEntityAPI;
	static APIFUNCTION2		GetEntityAPI2;
#ifdef DLL_LOADER
	static GIVEFNPTRSTODLL 	__attribute__((__stdcall__)) GiveFnptrsToDll_w32;
#endif
	static GIVEFNPTRSTODLL GiveFnptrsToDll;
	static NEW_DLL_FUNCTIONS_FN	GiveNewDllFuncs;
	static enginefuncs_t	gpEngfuncs;
	static globalvars_t		gpGlobals;
	static playermove_t		gpMove;
	edict_t			*e;
#ifdef DLL_LOADER
	qboolean dll;
#endif

	if( svgame.hInstance ) SV_UnloadProgs();

	// fill it in
	svgame.pmove = &gpMove;
	svgame.globals = &gpGlobals;
	svgame.mempool = Mem_AllocPool( "Server Edicts Zone" );
	svgame.hInstance = Com_LoadLibrary( name, true );
#ifdef DLL_LOADER
	dll = host.enabledll && Loader_GetDllHandle( svgame.hInstance );
#endif
	if( !svgame.hInstance ) return false;

	// make sure what new dll functions is cleared
	Q_memset( &svgame.dllFuncs2, 0, sizeof( svgame.dllFuncs2 ));

	// make sure what physic functions is cleared
	Q_memset( &svgame.physFuncs, 0, sizeof( svgame.physFuncs ));

	// make local copy of engfuncs to prevent overwrite it with bots.dll
	Q_memcpy( &gpEngfuncs, &gEngfuncs, sizeof( gpEngfuncs ));

	GetEntityAPI = (APIFUNCTION)Com_GetProcAddress( svgame.hInstance, "GetEntityAPI" );
	GetEntityAPI2 = (APIFUNCTION2)Com_GetProcAddress( svgame.hInstance, "GetEntityAPI2" );
	GiveNewDllFuncs = (NEW_DLL_FUNCTIONS_FN)Com_GetProcAddress( svgame.hInstance, "GetNewDLLFunctions" );
	if( !GetEntityAPI && !GetEntityAPI2 )
	{
		Com_FreeLibrary( svgame.hInstance );
         		MsgDev( D_NOTE, "SV_LoadProgs: failed to get address of GetEntityAPI proc\n" );
		svgame.hInstance = NULL;
		return false;
	}
#ifdef DLL_LOADER
	if(dll)
	{
		GiveFnptrsToDll_w32 = (void *)Com_GetProcAddress( svgame.hInstance, "GiveFnptrsToDll" );
		if( !GiveFnptrsToDll_w32 )
		{
			Com_FreeLibrary(svgame.hInstance);
			MsgDev( D_NOTE, "SV_LoadProgs: failed to get address of GiveFnptrsToDll proc\n" );
			svgame.hInstance = NULL;
			return false;
		}
		GiveFnptrsToDll_w32( &gpEngfuncs, svgame.globals );
	}
	else
#endif
	{
		GiveFnptrsToDll = (GIVEFNPTRSTODLL)Com_GetProcAddress( svgame.hInstance, "GiveFnptrsToDll" );
		
		if( !GiveFnptrsToDll )
		{
			Com_FreeLibrary( svgame.hInstance );
			MsgDev( D_NOTE, "SV_LoadProgs: failed to get address of GiveFnptrsToDll proc\n" );
			svgame.hInstance = NULL;
			return false;
		}
		

		GiveFnptrsToDll( &gpEngfuncs, svgame.globals );
	}
	if( GiveNewDllFuncs )
	{
		version = NEW_DLL_FUNCTIONS_VERSION;
		if( !GiveNewDllFuncs( &svgame.dllFuncs2, &version ))
		{
			if( version != NEW_DLL_FUNCTIONS_VERSION )
				MsgDev( D_WARN, "SV_LoadProgs: new interface version %i should be %i\n", NEW_DLL_FUNCTIONS_VERSION, version );
			Q_memset( &svgame.dllFuncs2, 0, sizeof( svgame.dllFuncs2 ));
		}
	}

	version = INTERFACE_VERSION;
	if( GetEntityAPI2 )
	{
		if( !GetEntityAPI2( &svgame.dllFuncs, &version ))
		{
			MsgDev( D_WARN, "SV_LoadProgs: interface version %i should be %i\n", INTERFACE_VERSION, version );

			// fallback to old API
			if( !GetEntityAPI( &svgame.dllFuncs, version ))
			{
				Com_FreeLibrary( svgame.hInstance );
				MsgDev( D_ERROR, "SV_LoadProgs: couldn't get entity API\n" );
				svgame.hInstance = NULL;
				return false;
			}
		}
		else MsgDev( D_AICONSOLE, "SV_LoadProgs: ^2initailized extended EntityAPI ^7ver. %i\n", version );
	}
	else if( !GetEntityAPI( &svgame.dllFuncs, version ))
	{
		Com_FreeLibrary( svgame.hInstance );
		MsgDev( D_ERROR, "SV_LoadProgs: couldn't get entity API\n" );
		svgame.hInstance = NULL;
		return false;
	}

	Mod_InitStudioAPI();

	if( !SV_InitPhysicsAPI( ))
	{
		MsgDev( D_WARN, "SV_LoadProgs: couldn't get physics API\n" );
	}

	// grab function SV_SaveGameComment
	SV_InitSaveRestore ();
	svgame.globals->maxEntities = GI->max_edicts;
	svgame.globals->maxClients = sv_maxclients->integer;
	svgame.edicts = Mem_Alloc( svgame.mempool, sizeof( edict_t ) * svgame.globals->maxEntities );
	svgame.numEntities = svgame.globals->maxClients + 1; // clients + world

	for( i = 0, e = svgame.edicts; i < svgame.globals->maxEntities; i++, e++ )
		e->free = true; // mark all edicts as freed

	// clear user messages
	svgame.gmsgHudText = -1;

	Cvar_FullSet( "host_gameloaded", "1", CVAR_INIT );
	SV_AllocStringPool();

	// fire once
	MsgDev( D_INFO, "Dll loaded for mod %s\n", svgame.dllFuncs.pfnGetGameDescription( ));

	// all done, initialize game
	svgame.dllFuncs.pfnGameInit();

	// initialize pm_shared
	SV_InitClientMove();

	return true;
}
