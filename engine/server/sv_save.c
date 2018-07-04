/*
sv_save.c - save\restore implementation
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
#include "library.h"
#include "const.h"
#include "render_api.h"	// decallist_t
#include "sound.h"		// S_GetDynamicSounds

/*
==============================================================================
SAVE FILE

half-life implementation of saverestore system
==============================================================================
*/
#define SAVEFILE_HEADER		(('V'<<24)+('L'<<16)+('A'<<8)+'V')	// little-endian "VALV"
#define SAVEGAME_HEADER		(('V'<<24)+('A'<<16)+('S'<<8)+'J')	// little-endian "JSAV"
#define SAVEGAME_VERSION		0x0065				// Version 0.65
#define CLIENT_SAVEGAME_VERSION	0x0068				// Version 0.68

#define SAVE_AGED_COUNT		1
#define SAVENAME_LENGTH		128				// matches with MAX_OSPATH

#define LUMP_DECALS_OFFSET		0
#define LUMP_STATIC_OFFSET		1
#define LUMP_SOUNDS_OFFSET		2
#define LUMP_MUSIC_OFFSET		3
#define NUM_CLIENT_OFFSETS		4

void (__cdecl *pfnSaveGameComment)( char *buffer, int max_length ) = NULL;

typedef struct
{
	int	nBytesSymbols;
	int	nSymbols;
	int	nBytesDataHeaders;
	int	nBytesData;
} SaveFileSectionsInfo_t;

typedef struct
{
	char	*pSymbols;
	char	*pDataHeaders;
	char	*pData;
} SaveFileSections_t;

typedef struct
{
	int	offsets[NUM_CLIENT_OFFSETS];
} ClientSections_t;

typedef struct
{
	char	mapName[32];
	char	comment[80];
	int	mapCount;
} GAME_HEADER;

typedef struct
{
	int	skillLevel;
	int	entityCount;
	int	connectionCount;
	int	lightStyleCount;
	float	time;
	char	mapName[32];
	char	skyName[32];
	int	skyColor_r;
	int	skyColor_g;
	int	skyColor_b;
	float	skyVec_x;
	float	skyVec_y;
	float	skyVec_z;
	int	viewentity;	// Xash3D added
	int	serverflags;	// converted to float and back
	float	wateralpha;
	float	skyDir_x;		// rotating sky support
	float	skyDir_y;
	float	skyDir_z;
	float	skyAngle;
	float	skySpeed;
} SAVE_HEADER;

typedef struct
{
	int	index;
	char	style[256];
	float	time;
} SAVE_LIGHTSTYLE;

static TYPEDESCRIPTION gGameHeader[] =
{
	DEFINE_ARRAY( GAME_HEADER, mapName, FIELD_CHARACTER, 32 ),
	DEFINE_ARRAY( GAME_HEADER, comment, FIELD_CHARACTER, 80 ),
	DEFINE_FIELD( GAME_HEADER, mapCount, FIELD_INTEGER ),
};

static TYPEDESCRIPTION gSaveHeader[] =
{
	DEFINE_FIELD( SAVE_HEADER, skillLevel, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, entityCount, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, connectionCount, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, lightStyleCount, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, time, FIELD_TIME ),
	DEFINE_ARRAY( SAVE_HEADER, mapName, FIELD_CHARACTER, 32 ),
	DEFINE_ARRAY( SAVE_HEADER, skyName, FIELD_CHARACTER, 32 ),
	DEFINE_FIELD( SAVE_HEADER, skyColor_r, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, skyColor_g, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, skyColor_b, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, skyVec_x, FIELD_FLOAT ),
	DEFINE_FIELD( SAVE_HEADER, skyVec_y, FIELD_FLOAT ),
	DEFINE_FIELD( SAVE_HEADER, skyVec_z, FIELD_FLOAT ),
	DEFINE_FIELD( SAVE_HEADER, viewentity, FIELD_SHORT ),
	DEFINE_FIELD( SAVE_HEADER, serverflags, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, wateralpha, FIELD_FLOAT ),
	DEFINE_FIELD( SAVE_HEADER, skyDir_x, FIELD_FLOAT ),
	DEFINE_FIELD( SAVE_HEADER, skyDir_y, FIELD_FLOAT ),
	DEFINE_FIELD( SAVE_HEADER, skyDir_z, FIELD_FLOAT ),
	DEFINE_FIELD( SAVE_HEADER, skyAngle, FIELD_FLOAT ),
	DEFINE_FIELD( SAVE_HEADER, skySpeed, FIELD_FLOAT ),
};

static TYPEDESCRIPTION gAdjacency[] =
{
	DEFINE_ARRAY( LEVELLIST, mapName, FIELD_CHARACTER, 32 ),
	DEFINE_ARRAY( LEVELLIST, landmarkName, FIELD_CHARACTER, 32 ),
	DEFINE_FIELD( LEVELLIST, pentLandmark, FIELD_EDICT ),
	DEFINE_FIELD( LEVELLIST, vecLandmarkOrigin, FIELD_VECTOR ),
};

static TYPEDESCRIPTION gLightStyle[] =
{
	DEFINE_FIELD( SAVE_LIGHTSTYLE, index, FIELD_INTEGER ),
	DEFINE_ARRAY( SAVE_LIGHTSTYLE, style, FIELD_CHARACTER, 256 ),
	DEFINE_FIELD( SAVE_LIGHTSTYLE, time, FIELD_FLOAT ),
};

static TYPEDESCRIPTION gEntityTable[] =
{
	DEFINE_FIELD( ENTITYTABLE, id, FIELD_INTEGER ),
	DEFINE_FIELD( ENTITYTABLE, location, FIELD_INTEGER ),
	DEFINE_FIELD( ENTITYTABLE, size, FIELD_INTEGER ),
	DEFINE_FIELD( ENTITYTABLE, flags, FIELD_INTEGER ),
	DEFINE_FIELD( ENTITYTABLE, classname, FIELD_STRING ),
};

static TYPEDESCRIPTION gEntvarsDescription[] =
{
	DEFINE_ENTITY_FIELD( classname, FIELD_STRING ),
	DEFINE_ENTITY_GLOBAL_FIELD( globalname, FIELD_STRING ),
};

int SumBytes( SaveFileSectionsInfo_t *section )
{
	return ( section->nBytesSymbols + section->nBytesDataHeaders + section->nBytesData );
}

void SV_InitSaveRestore( void )
{
	pfnSaveGameComment = Com_GetProcAddress( svgame.hInstance, "SV_SaveGameComment" );
}

/*
----------------------------------------------------------
		SaveRestore helpers

	       assume pSaveData is valid
----------------------------------------------------------
*/
void SaveRestore_Init( SAVERESTOREDATA *pSaveData, void *pNewBase, int nBytes )
{
	pSaveData->pCurrentData = pSaveData->pBaseData = (char *)pNewBase;
	pSaveData->bufferSize = nBytes;
	pSaveData->size = 0;
}

void SaveRestore_MoveCurPos( SAVERESTOREDATA *pSaveData, int nBytes )
{
	pSaveData->pCurrentData += nBytes;
	pSaveData->size += nBytes;
}

void SaveRestore_Rebase( SAVERESTOREDATA *pSaveData )
{
	pSaveData->pBaseData = pSaveData->pCurrentData;
	pSaveData->bufferSize -= pSaveData->size;
	pSaveData->size = 0;
}

void SaveRestore_Rewind( SAVERESTOREDATA *pSaveData, int nBytes )
{
	if( pSaveData->size < nBytes )
		nBytes = pSaveData->size;

	SaveRestore_MoveCurPos( pSaveData, -nBytes );
}

char *SaveRestore_GetBuffer( SAVERESTOREDATA *pSaveData )
{
	return pSaveData->pBaseData;
}

int SaveRestore_BytesAvailable( SAVERESTOREDATA *pSaveData )
{
	return (pSaveData->bufferSize - pSaveData->size);
}

int SaveRestore_SizeBuffer( SAVERESTOREDATA *pSaveData )
{
	return pSaveData->bufferSize;
}

qboolean SaveRestore_Write( SAVERESTOREDATA *pSaveData, const void *pData, int nBytes )
{
	if( nBytes > SaveRestore_BytesAvailable( pSaveData ))
	{
		pSaveData->size = pSaveData->bufferSize;
		return false;
	}

	Q_memcpy( pSaveData->pCurrentData, pData, nBytes );
	SaveRestore_MoveCurPos( pSaveData, nBytes );

	return true;
}

qboolean SaveRestore_Read( SAVERESTOREDATA *pSaveData, void *pOutput, int nBytes )
{
	if( !SaveRestore_BytesAvailable( pSaveData ))
		return false;

	if( nBytes > SaveRestore_BytesAvailable( pSaveData ))
	{
		pSaveData->size = pSaveData->bufferSize;
		return false;
	}

	if( pOutput ) Q_memcpy( pOutput, pSaveData->pCurrentData, nBytes );
	SaveRestore_MoveCurPos( pSaveData, nBytes );

	return true;
}

int SaveRestore_GetCurPos( SAVERESTOREDATA *pSaveData )
{
	return pSaveData->size;
}

char *SaveRestore_AccessCurPos( SAVERESTOREDATA *pSaveData )
{
	return pSaveData->pCurrentData;
}

qboolean SaveRestore_Seek( SAVERESTOREDATA *pSaveData, int absPosition )
{
	if( absPosition < 0 || absPosition >= pSaveData->bufferSize )
		return false;
	
	pSaveData->size = absPosition;
	pSaveData->pCurrentData = pSaveData->pBaseData + pSaveData->size;

	return true;
}

void SaveRestore_InitEntityTable( SAVERESTOREDATA *pSaveData, ENTITYTABLE *pNewTable, int entityCount )
{
	ENTITYTABLE	*pTable;
	int		i;

	ASSERT( pSaveData->pTable == NULL );

	pSaveData->tableCount = entityCount;
	pSaveData->pTable = pNewTable;

	// setup entitytable
	for( i = 0; i < entityCount; i++ )
	{
		pTable = &pSaveData->pTable[i];		
		pTable->pent = EDICT_NUM( i );
	}
}

ENTITYTABLE *SaveRestore_DetachEntityTable( SAVERESTOREDATA *pSaveData )
{
	ENTITYTABLE *pReturn = pSaveData->pTable;

	pSaveData->pTable = NULL;
	pSaveData->tableCount = 0;

	return pReturn;
}
	
void SaveRestore_InitSymbolTable( SAVERESTOREDATA *pSaveData, char **pNewTokens, int sizeTable )
{
	ASSERT( pSaveData->pTokens == NULL );

	pSaveData->tokenCount = sizeTable;
	pSaveData->pTokens = pNewTokens;
}

char **SaveRestore_DetachSymbolTable( SAVERESTOREDATA *pSaveData )
{
	char **pResult = pSaveData->pTokens;

	pSaveData->tokenCount = 0;
	pSaveData->pTokens = NULL;

	return pResult;
}

qboolean SaveRestore_DefineSymbol( SAVERESTOREDATA *pSaveData, const char *pszToken, int token )
{
	if( pSaveData->pTokens[token] == NULL )
	{
		pSaveData->pTokens[token] = (char *)pszToken;
		return true;
	}

	ASSERT( 0 );
	return false;
}

const char *SaveRestore_StringFromSymbol( SAVERESTOREDATA *pSaveData, int token )
{
	if( token >= 0 && token < pSaveData->tokenCount )
		return pSaveData->pTokens[token];
	return "<<bad string>>";
}

void SV_BuildSaveComment( char *text, int maxlength )
{
	if( pfnSaveGameComment != NULL )
	{
		// get save comment from gamedll
		pfnSaveGameComment( text, maxlength );
	}
	else
	{
		const char	*pName;
		edict_t		*pWorld = EDICT_NUM( 0 );
		float		time = sv.time;

		if( pWorld && pWorld->v.message )
		{
			// trying to extract message from the world
			pName = STRING( pWorld->v.message );
		}
		else
		{
			// or use mapname
			pName = STRING( svgame.globals->mapname );
		}

		Q_snprintf( text, maxlength, "%-64.64s %02d:%02d", pName, (int)(time / 60.0f ), (int)fmod( time, 60.0f ));
	}
}

int SV_MapCount( const char *pPath )
{
	search_t	*t;
	int	count = 0;
	
	t = FS_Search( pPath, true, true );	// lookup only in gamedir
	if( !t ) return count; // empty

	count = t->numfilenames;
	Mem_Free( t );

	return count;
}

int EntryInTable( SAVERESTOREDATA *pSaveData, const char *pMapName, int index )
{
	int	i;

	index++;

	for( i = index; i < pSaveData->connectionCount; i++ )
	{
		if ( !Q_strcmp( pSaveData->levelList[i].mapName, pMapName ))
			return i;
	}

	return -1;
}

void LandmarkOrigin( SAVERESTOREDATA *pSaveData, vec3_t output, const char *pLandmarkName )
{
	int	i;

	for( i = 0; i < pSaveData->connectionCount; i++ )
	{
		if( !Q_strcmp( pSaveData->levelList[i].landmarkName, pLandmarkName ))
		{
			VectorCopy( pSaveData->levelList[i].vecLandmarkOrigin, output );
			return;
		}
	}

	VectorClear( output );
}

int EntityInSolid( edict_t *ent )
{
	edict_t	*pParent = ent->v.aiment;
	vec3_t	point;

	// if you're attached to a client, always go through
	if( SV_IsValidEdict( pParent ))
	{
		if( pParent->v.flags & FL_CLIENT )
			return 0;
	}

	VectorAverage( ent->v.absmin, ent->v.absmax, point );
	return (SV_PointContents( point ) == CONTENTS_SOLID);
}

void ReapplyDecal( SAVERESTOREDATA *pSaveData, decallist_t *entry, qboolean adjacent )
{
	int	flags = entry->flags;
	int	decalIndex, entityIndex = 0;
	int	modelIndex = 0;
	edict_t	*pEdict;

	// never transition permanent decals
	if( adjacent && ( flags & FDECAL_PERMANENT ))
		return;

	if( entry->entityIndex < 0 || entry->entityIndex > pSaveData->tableCount )
		return;

	// restore entity and model index
	pEdict = pSaveData->pTable[entry->entityIndex].pent;
	if( SV_IsValidEdict( pEdict ) )
	{
		modelIndex = pEdict->v.modelindex;
		entityIndex = NUM_FOR_EDICT( pEdict );
	}

	if( SV_RestoreCustomDecal( entry, pEdict, adjacent ))
		return; // decal was sucessfully restored at the game-side

	// NOTE: at this point all decal indexes is valid
	decalIndex = pfnDecalIndex( entry->name );

	if( flags & FDECAL_STUDIO )
	{
		// NOTE: studio decal trace start saved into impactPlaneNormal
		SV_CreateStudioDecal( &sv.signon, entry->position, entry->impactPlaneNormal, decalIndex, entityIndex, modelIndex, flags, &entry->studio_state );
	}
	else if( adjacent && entityIndex != 0 && !SV_IsValidEdict( pEdict ))
	{
		vec3_t	testspot, testend;
		trace_t	tr;

		// these entities might not exist over transitions,
		// so we'll use the saved plane and do a traceline instead
		flags |= FDECAL_DONTSAVE;

		MsgDev( D_ERROR, "couldn't restore entity index %i, do trace for decal\n", entityIndex );

		VectorCopy( entry->position, testspot );
		VectorMA( testspot, 5.0f, entry->impactPlaneNormal, testspot );

		VectorCopy( entry->position, testend );
		VectorMA( testend, -5.0f, entry->impactPlaneNormal, testend );

		tr = SV_Move( testspot, vec3_origin, vec3_origin, testend, MOVE_NOMONSTERS, NULL );

		// NOTE: this code may does wrong result on moving brushes e.g. func_tracktrain
		if( tr.fraction != 1.0f && !tr.allsolid )
		{
			// check impact plane normal
			float	dot = DotProduct( entry->impactPlaneNormal, tr.plane.normal );

			if( dot >= 0.95f )
			{
				entityIndex = pfnIndexOfEdict( tr.ent );
				if( entityIndex > 0 ) modelIndex = tr.ent->v.modelindex;
				SV_CreateDecal( &sv.signon, tr.endpos, decalIndex, entityIndex, modelIndex, flags, entry->scale );
			}
		}
	}
	else
	{
		// global entity is exist on new level so we can apply decal in local space
		// NOTE: this case also used for transition world decals
		SV_CreateDecal( &sv.signon, entry->position, decalIndex, entityIndex, modelIndex, flags, entry->scale );
	}
}

void RestoreSound( soundlist_t *entry )
{
	int	soundIndex;
	int	flags = 0;
	edict_t	*ent;

	// this can happens if serialized map contain 4096 static decals...
	if(( BF_GetNumBytesWritten( &sv.signon ) + 20 ) >= BF_GetMaxBytes( &sv.signon ))
		return;

	if( entry->name[0] == '!' && Q_isdigit( entry->name + 1 ))
	{
		flags |= SND_SENTENCE;
		soundIndex = Q_atoi( entry->name + 1 );
	}
	else if( entry->name[0] == '#' && Q_isdigit( entry->name + 1 ))
	{
		flags |= SND_SENTENCE;
		soundIndex = Q_atoi( entry->name + 1 ) + 1536;
	}
	else
	{
		// precache_sound can be used twice: cache sounds when loading
		// and return sound index when server is active
		soundIndex = SV_SoundIndex( entry->name );
	}

	ent = EDICT_NUM( entry->entnum );

	if( entry->attenuation < 0.0f || entry->attenuation > 4.0f )
	{
		MsgDev( D_ERROR, "SV_RestoreSound: attenuation %g must be in range 0-4\n", entry->attenuation );
		return;
	}

	if( entry->channel > 7 )
	{
		MsgDev( D_ERROR, "SV_RestoreSound: channel must be in range 0-7\n" );
		return;
	}

	if( !SV_IsValidEdict( ent ))
	{
		MsgDev( D_ERROR, "SV_RestoreSound: edict == NULL\n" );
		return;
	}

	if( entry->volume != VOL_NORM ) flags |= SND_VOLUME;
	if( entry->attenuation != ATTN_NONE ) flags |= SND_ATTENUATION;
	if( entry->pitch != PITCH_NORM ) flags |= SND_PITCH;
	if( !entry->looping ) flags |= SND_STOP_LOOPING;	// just in case

	if( soundIndex > 255 ) flags |= SND_LARGE_INDEX;

	BF_WriteByte( &sv.signon, svc_restoresound );
	BF_WriteWord( &sv.signon, flags );
	if( flags & SND_LARGE_INDEX )
		BF_WriteWord( &sv.signon, soundIndex );
	else BF_WriteByte( &sv.signon, soundIndex );
	BF_WriteByte( &sv.signon, entry->channel );

	if( flags & SND_VOLUME ) BF_WriteByte( &sv.signon, entry->volume * 255 );
	if( flags & SND_ATTENUATION ) BF_WriteByte( &sv.signon, entry->attenuation * 64 );
	if( flags & SND_PITCH ) BF_WriteByte( &sv.signon, entry->pitch );

	BF_WriteWord( &sv.signon, entry->entnum );
	BF_WriteVec3Coord( &sv.signon, entry->origin );
	BF_WriteByte( &sv.signon, entry->wordIndex );

	// send two doubles as raw-data
	BF_WriteBytes( &sv.signon, &entry->samplePos, sizeof( entry->samplePos ));
	BF_WriteBytes( &sv.signon, &entry->forcedEnd, sizeof( entry->forcedEnd ));
}

void SV_ClearSaveDir( void )
{
	search_t	*t;
	int	i;

	// just delete all HL? files
	t = FS_Search( "save/*.hl?", true, true );	// lookup only in gamedir
	if( !t ) return; // already empty

	for( i = 0; i < t->numfilenames; i++ )
		FS_Delete( t->filenames[i] );

	Mem_Free( t );
}

int SV_IsValidSave( void )
{
	if( sv.background )
		return 0;

	// don't parse autosave/transition save/restores during playback!
	if( CL_IsPlaybackDemo( ))
		return 0;

	if( !svs.initialized || sv.state != ss_active )
	{
		Msg( "Not playing a local game.\n" );
		return 0;
	}

	if( svgame.physFuncs.SV_AllowSaveGame != NULL )
	{
		if( !svgame.physFuncs.SV_AllowSaveGame( ))
		{
			Msg( "Savegame is not allowed.\n" );
			return 0;
		}
	}

	if( !Host_IsDedicated() )
	{
		// Enable save/load in xashds

		if( !CL_Active( ))
		{
			Msg( "Can't save if not active.\n" );
			return 0;
		}

		if( CL_IsIntermission( ))
		{
			Msg( "Can't save during intermission.\n" );
			return 0;
		}

		if( sv_maxclients->integer != 1 )
		{
			Msg( "Can't save multiplayer games.\n" );
			return 0;
		}

	}

	if( svs.clients && svs.clients[0].state == cs_spawned )
	{
		edict_t	*pl = svs.clients[0].edict;
		
		if( !pl )
		{
			Msg( "Can't savegame without a player!\n" );
			return 0;
		}
			
		if( pl->v.deadflag != false || pl->v.health <= 0.0f )
		{
			Msg( "Can't savegame with a dead player.\n" );
			return 0;
		}

		// Passed all checks, it's ok to save
		return 1;
	}

	Msg( "Can't savegame without a client!\n" );

	return 0;
}

void SV_AgeSaveList( const char *pName, int count )
{
	string	newName, oldName, newImage, oldImage;

	// delete last quick/autosave (e.g. quick05.sav)
	Q_snprintf( newName, sizeof( newName ), "save/%s%02d.sav", pName, count );
	Q_snprintf( newImage, sizeof( newImage ), "save/%s%02d.bmp", pName, count );

	// only delete from game directory, basedir is read-only
	FS_Delete( newName );
	FS_Delete( newImage );

	GL_FreeImage( newImage );

	while( count > 0 )
	{
		if( count == 1 )
		{	
			// quick.sav
			Q_snprintf( oldName, sizeof( oldName ), "save/%s.sav", pName );
			Q_snprintf( oldImage, sizeof( oldImage ), "save/%s.bmp", pName );
		}
		else
		{	
			// quick04.sav, etc.
			Q_snprintf( oldName, sizeof( oldName ), "save/%s%02d.sav", pName, count - 1 );
			Q_snprintf( oldImage, sizeof( oldImage ), "save/%s%02d.bmp", pName, count - 1 );
		}

		Q_snprintf( newName, sizeof( newName ), "save/%s%02d.sav", pName, count );
		Q_snprintf( newImage, sizeof( newImage ), "save/%s%02d.bmp", pName, count );

		GL_FreeImage( oldImage );

		// scroll the name list down (rename quick04.sav to quick05.sav)
		FS_Rename( oldName, newName );
		FS_Rename( oldImage, newImage );
		count--;
	}
}

void SV_DirectoryCopy( const char *pPath, file_t *pFile )
{
	search_t	*t;
	file_t	*pCopy;
	int	i, fileSize;
	char	szName[SAVENAME_LENGTH];

	t = FS_Search( pPath, true, true );
	if( !t ) return;

	for( i = 0; i < t->numfilenames; i++ )
	{
		Q_memset( szName, 0, SAVENAME_LENGTH );
		fileSize = FS_FileSize( t->filenames[i], true );
		pCopy = FS_Open( t->filenames[i], "rb", true );

		// filename can only be as long as a map name + extension
		Q_strncpy( szName, FS_FileWithoutPath( t->filenames[i] ), SAVENAME_LENGTH );		
		FS_Write( pFile, szName, SAVENAME_LENGTH );
		FS_Write( pFile, &fileSize, sizeof( int ));
		FS_FileCopy( pFile, pCopy, fileSize );
		FS_Close( pCopy );
	}

	Mem_Free( t );
}

void SV_DirectoryExtract( file_t *pFile, int fileCount )
{
	char	szName[SAVENAME_LENGTH], fileName[SAVENAME_LENGTH];
	int	i, fileSize;
	file_t	*pCopy;

	for( i = 0; i < fileCount; i++ )
	{
		// filename can only be as long as a map name + extension
		FS_Read( pFile, fileName, SAVENAME_LENGTH );
		FS_Read( pFile, &fileSize, sizeof( int ));
		Q_snprintf( szName, sizeof( szName ), "save/%s", fileName );

		pCopy = FS_Open( szName, "wb", true );
		FS_FileCopy( pCopy, pFile, fileSize );
		FS_Close( pCopy );
	}
}

void SV_SaveFinish( SAVERESTOREDATA *pSaveData )
{
	char 		**pTokens;
	ENTITYTABLE	*pEntityTable;

	pTokens = SaveRestore_DetachSymbolTable( pSaveData );
	if( pTokens ) Mem_Free( pTokens );

	pEntityTable = SaveRestore_DetachEntityTable( pSaveData );
	if( pEntityTable ) Mem_Free( pEntityTable );

	if( pSaveData ) Mem_Free( pSaveData );

	svgame.globals->pSaveData = NULL;
}

SAVERESTOREDATA *SV_SaveInit( int size )
{
	SAVERESTOREDATA	*pSaveData;
	const int		nTokens = 0xfff;	// Assume a maximum of 4K-1 symbol table entries(each of some length)
	int		numents;

	if( size <= 0 ) size = 0x200000;	// Reserve 2Mb for now
	numents = svgame.numEntities;

	pSaveData = Mem_Alloc( host.mempool, sizeof(SAVERESTOREDATA) + ( sizeof(ENTITYTABLE) * numents ) + size );
	SaveRestore_Init( pSaveData, (char *)(pSaveData + 1), size ); // skip the save structure
	SaveRestore_InitSymbolTable( pSaveData, (char **)Mem_Alloc( host.mempool, nTokens * sizeof( char* )), nTokens );

	pSaveData->time = svgame.globals->time;	// Use DLL time
	VectorClear( pSaveData->vecLandmarkOffset );
	pSaveData->fUseLandmark = false;
	pSaveData->connectionCount = 0;
		
	// shared with dlls	
	svgame.globals->pSaveData = pSaveData;

	return pSaveData;
}

void SV_SaveGameStateGlobals( SAVERESTOREDATA *pSaveData )
{
	sv_client_t	*cl;
	SAVE_HEADER	header;
	SAVE_LIGHTSTYLE	light;
	int		i;
	
	Q_memset( &header, 0, sizeof( SAVE_HEADER ) );
	Q_memset( &light, 0, sizeof( SAVE_LIGHTSTYLE ) );

	// write global data
	header.skillLevel = Cvar_VariableValue( "skill" ); // This is created from an int even though it's a float
	header.connectionCount = pSaveData->connectionCount;
	header.time = svgame.globals->time;

	if( sv_skyname->string[0] )
		Q_strncpy( header.skyName, sv_skyname->string, sizeof( header.skyName ));
	else Q_strncpy( header.skyName, "", sizeof( header.skyName ));

	Q_strncpy( header.mapName, sv.name, sizeof( header.mapName ));
	header.lightStyleCount = 0;
	header.entityCount = svgame.numEntities;

	for( i = 0; i < MAX_LIGHTSTYLES; i++ )
	{
		if( sv.lightstyles[i].pattern[0] )
			header.lightStyleCount++;
	}

	// sky variables
	header.skyColor_r = Cvar_VariableValue( "sv_skycolor_r" );
	header.skyColor_g = Cvar_VariableValue( "sv_skycolor_g" );
	header.skyColor_b = Cvar_VariableValue( "sv_skycolor_b" );
	header.skyVec_x = Cvar_VariableValue( "sv_skyvec_x" );
	header.skyVec_y = Cvar_VariableValue( "sv_skyvec_y" );
	header.skyVec_z = Cvar_VariableValue( "sv_skyvec_z" );
	header.skyDir_x = Cvar_VariableValue( "sv_skydir_x" );
	header.skyDir_y = Cvar_VariableValue( "sv_skydir_y" );
	header.skyDir_z = Cvar_VariableValue( "sv_skydir_z" );
	header.skyAngle = Cvar_VariableValue( "sv_skyangle" );
	header.skySpeed = Cvar_VariableValue( "sv_skyspeed" );

	// save viewentity to allow camera works after save\restore
	if(( cl = SV_ClientFromEdict( EDICT_NUM( 1 ), true )) != NULL )
	{
		if( cl->pViewEntity )
			header.viewentity = NUM_FOR_EDICT( cl->pViewEntity );
		else header.viewentity = 1;
	}
	else header.viewentity = 1;

	header.serverflags = (int)svgame.globals->serverflags;
	header.wateralpha = Cvar_VariableValue( "sv_wateralpha" );

	pSaveData->time = 0.0f; // prohibits rebase of header.time (why not just save time as a field_float and ditch this hack?)
	svgame.dllFuncs.pfnSaveWriteFields( pSaveData, "Save Header", &header, gSaveHeader, ARRAYSIZE( gSaveHeader ));
	pSaveData->time = header.time;

	// write entity table
	for( i = 0; i < pSaveData->tableCount; i++ )
		svgame.dllFuncs.pfnSaveWriteFields( pSaveData, "ETABLE", pSaveData->pTable + i, gEntityTable, ARRAYSIZE( gEntityTable ));

	// write adjacency list
	for( i = 0; i < pSaveData->connectionCount; i++ )
		svgame.dllFuncs.pfnSaveWriteFields( pSaveData, "ADJACENCY", pSaveData->levelList + i, gAdjacency, ARRAYSIZE( gAdjacency ));

	// write the lightstyles
	for( i = 0; i < MAX_LIGHTSTYLES; i++ )
	{
		if( sv.lightstyles[i].pattern[0] )
		{
			light.index = i;
			light.time = sv.lightstyles[i].time;
			Q_strncpy( light.style, sv.lightstyles[i].pattern, sizeof( light.style ));
			svgame.dllFuncs.pfnSaveWriteFields( pSaveData, "LIGHTSTYLE", &light, gLightStyle, ARRAYSIZE( gLightStyle ));
		}
	}
}

SAVERESTOREDATA *SV_LoadSaveData( const char *level )
{
	string			name;
	file_t			*pFile;
	SaveFileSectionsInfo_t	sectionsInfo;
	SAVERESTOREDATA		*pSaveData;
	char			*pszTokenList;
	int			i, id, size, version;
	
	Q_snprintf( name, sizeof( name ), "save/%s.hl1", level );
	MsgDev( D_INFO, "Loading game from %s...\n", name );

	pFile = FS_Open( name, "rb", true );
	if( !pFile )
	{
		MsgDev( D_INFO, "ERROR: couldn't open.\n" );
		return NULL;
	}

	// Read the header
	FS_Read( pFile, &id, sizeof( int ));
	FS_Read( pFile, &version, sizeof( int ));

	// is this a valid save?
	if( id != SAVEFILE_HEADER || version != SAVEGAME_VERSION )
	{
		FS_Close( pFile );
		return NULL;
	}

	// Read the sections info and the data
	FS_Read( pFile, &sectionsInfo, sizeof( sectionsInfo ));

	pSaveData = Mem_Alloc( host.mempool, sizeof(SAVERESTOREDATA) + SumBytes( &sectionsInfo ));
	Q_strncpy( pSaveData->szCurrentMapName, level, sizeof( pSaveData->szCurrentMapName ));
	
	FS_Read( pFile, (char *)(pSaveData + 1), SumBytes( &sectionsInfo ));
	FS_Close( pFile );
	
	// Parse the symbol table
	pszTokenList = (char *)(pSaveData + 1);	// Skip past the CSaveRestoreData structure

	if( sectionsInfo.nBytesSymbols > 0 )
	{
		SaveRestore_InitSymbolTable( pSaveData, (char **)Mem_Alloc( host.mempool, sectionsInfo.nSymbols * sizeof( char* )), sectionsInfo.nSymbols );

		// make sure the token strings pointed to by the pToken hashtable.
		for( i = 0; i < sectionsInfo.nSymbols; i++ )
		{
			if( *pszTokenList )
			{
				ASSERT( SaveRestore_DefineSymbol( pSaveData, pszTokenList, i ));
			}

			// find next token (after next null)
			while( *pszTokenList++ );
		}
	}
	else
	{
		SaveRestore_InitSymbolTable( pSaveData, NULL, 0 );
	}

	ASSERT( pszTokenList - (char *)(pSaveData + 1) == sectionsInfo.nBytesSymbols );

	// set up the restore basis
	size = SumBytes( &sectionsInfo ) - sectionsInfo.nBytesSymbols;

	// the point pszTokenList was incremented to the end of the tokens
	SaveRestore_Init( pSaveData, (char *)(pszTokenList), size );

	pSaveData->connectionCount = 0;
	pSaveData->fUseLandmark = true;
	pSaveData->time = 0.0f;
	VectorClear( pSaveData->vecLandmarkOffset );

	// shared with dlls	
	svgame.globals->pSaveData = pSaveData;

	return pSaveData;
}

void SV_ReadEntityTable( SAVERESTOREDATA *pSaveData )
{
	ENTITYTABLE	*pEntityTable;
	int		i;

	pEntityTable = (ENTITYTABLE *)Mem_Alloc( host.mempool, sizeof( ENTITYTABLE ) * pSaveData->tableCount );
	SaveRestore_InitEntityTable( pSaveData, pEntityTable, pSaveData->tableCount );

	for( i = 0; i < pSaveData->tableCount; i++ )
		svgame.dllFuncs.pfnSaveReadFields( pSaveData, "ETABLE", pSaveData->pTable + i, gEntityTable, ARRAYSIZE( gEntityTable ));
}

void SV_ParseSaveTables( SAVERESTOREDATA *pSaveData, SAVE_HEADER *pHeader, int setupLightstyles )
{
	SAVE_LIGHTSTYLE	light;
	int		i;

	// process SAVE_HEADER
	svgame.dllFuncs.pfnSaveReadFields( pSaveData, "Save Header", pHeader, gSaveHeader, ARRAYSIZE( gSaveHeader ));

	pSaveData->connectionCount = pHeader->connectionCount;
	pSaveData->time = pHeader->time;
	pSaveData->fUseLandmark = true;
	VectorClear( pSaveData->vecLandmarkOffset );
	pSaveData->tableCount = pHeader->entityCount;

	SV_ReadEntityTable( pSaveData );

	// read adjacency list
	for( i = 0; i < pSaveData->connectionCount; i++ )
	{
		LEVELLIST	*pList = &pSaveData->levelList[i];		
		svgame.dllFuncs.pfnSaveReadFields( pSaveData, "ADJACENCY", pList, gAdjacency, ARRAYSIZE( gAdjacency ));
	}

	if( setupLightstyles )
		Q_memset( sv.lightstyles, 0, sizeof( sv.lightstyles ));

	for( i = 0; i < pHeader->lightStyleCount; i++ )
	{
		svgame.dllFuncs.pfnSaveReadFields( pSaveData, "LIGHTSTYLE", &light, gLightStyle, ARRAYSIZE( gLightStyle ));
		if( setupLightstyles ) SV_SetLightStyle( light.index, light.style, light.time );
	}
}

/*
=============
SV_EntityPatchWrite

write out the list of entities that are no longer in the save file for this level
(they've been moved to another level)
=============
*/
void SV_EntityPatchWrite( SAVERESTOREDATA *pSaveData, const char *level )
{
	string		name;
	file_t		*pFile;
	int		i, size;

	Q_snprintf( name, sizeof( name ), "save/%s.hl3", level );

	pFile = FS_Open( name, "wb", true );
	if( !pFile ) return;

	for( i = size = 0; i < pSaveData->tableCount; i++ )
	{
		if( pSaveData->pTable[i].flags & FENTTABLE_REMOVED )
			size++;
	}

	// patch count
	FS_Write( pFile, &size, sizeof( int ));

	for( i = 0; i < pSaveData->tableCount; i++ )
	{
		if( pSaveData->pTable[i].flags & FENTTABLE_REMOVED )
			FS_Write( pFile, &i, sizeof( int ));
	}

	FS_Close( pFile );
}

/*
=============
SV_EntityPatchRead

read the list of entities that are no longer in the save file for this level
(they've been moved to another level)
=============
*/
void SV_EntityPatchRead( SAVERESTOREDATA *pSaveData, const char *level )
{
	string	name;
	file_t	*pFile;
	int	i, size, entityId;

	Q_snprintf( name, sizeof( name ), "save/%s.hl3", level );

	pFile = FS_Open( name, "rb", true );
	if( !pFile ) return;

	// patch count
	FS_Read( pFile, &size, sizeof( int ));

	for( i = 0; i < size; i++ )
	{
		FS_Read( pFile, &entityId, sizeof( int ));
		pSaveData->pTable[entityId].flags = FENTTABLE_REMOVED;
	}

	FS_Close( pFile );
}

/*
=============
SV_SaveClientState

write out the list of premanent decals for this level
=============
*/
void SV_SaveClientState( SAVERESTOREDATA *pSaveData, const char *level )
{
	string		name;
	file_t		*pFile;
	decallist_t	*decalList;
	ClientSections_t	sections;
	int		i, decalCount;
	int		id, version;
	fs_offset_t	header_offset, position;
	soundlist_t	soundInfo[MAX_CHANNELS];
	string		curtrack, looptrack;
	int		soundCount = 0;

	Q_snprintf( name, sizeof( name ), "save/%s.hl2", level );

	pFile = FS_Open( name, "wb", true );
	if( !pFile ) return;

	id = SAVEFILE_HEADER;
	version = CLIENT_SAVEGAME_VERSION;

	// write the header
	FS_Write( pFile, &id, sizeof( int ));
	FS_Write( pFile, &version, sizeof( int ));

	Q_memset( &sections, -1, sizeof( sections ));
	header_offset = FS_Tell( pFile );	// save header offset

	// write offsets (will be merged later)
	FS_Write( pFile, &sections, sizeof( sections ));

	// g-cont. add space for studiodecals if present
	decalList = (decallist_t *)Z_Malloc( sizeof( decallist_t ) * MAX_RENDER_DECALS * 2 );
	decalCount = R_CreateDecalList( decalList, svgame.globals->changelevel );

	// DECALS SECTION
	sections.offsets[LUMP_DECALS_OFFSET] = FS_Tell( pFile );
	FS_Write( pFile, &decalCount, sizeof( int ));

	// we can't use SaveRestore system here...
	for( i = 0; i < decalCount; i++ )
	{
		vec3_t		localPos;
		decallist_t	*entry;
		word		decalScale;
		byte		nameSize;

		entry = &decalList[i];

		if( pSaveData->fUseLandmark && ( entry->flags & FDECAL_USE_LANDMARK ))
			VectorSubtract( entry->position, pSaveData->vecLandmarkOffset, localPos );
		else VectorCopy( entry->position, localPos );

		nameSize = Q_strlen( entry->name ) + 1;
		decalScale = (entry->scale * 4096);

		FS_Write( pFile, localPos, sizeof( localPos ));
		FS_Write( pFile, &nameSize, sizeof( nameSize ));
		FS_Write( pFile, entry->name, nameSize ); 
		FS_Write( pFile, &entry->entityIndex, sizeof( entry->entityIndex ));
		FS_Write( pFile, &entry->flags, sizeof( entry->flags ));
		FS_Write( pFile, &decalScale, sizeof( decalScale ));
		FS_Write( pFile, entry->impactPlaneNormal, sizeof( entry->impactPlaneNormal ));

		if( entry->flags & FDECAL_STUDIO )
		{
			// write additional data for studio decals
			FS_Write( pFile, &entry->studio_state, sizeof( entry->studio_state ));
		}
	}

	Z_Free( decalList );

	// STATIC ENTITIES SECTION
	if( sv.num_static_entities != 0 )
	{
		sections.offsets[LUMP_STATIC_OFFSET] = FS_Tell( pFile );
		FS_Write( pFile, &sv.num_static_entities, sizeof( int ));
	}

	for( i = 0; i < sv.num_static_entities; i++ )
	{
		sv_static_entity_t	*entry;
		byte		nameSize;

		entry = &sv.static_entities[i];

		nameSize = Q_strlen( entry->model ) + 1;

		FS_Write( pFile, &nameSize, sizeof( nameSize ));
		FS_Write( pFile, entry->model, nameSize ); 
		FS_Write( pFile, entry->origin, sizeof( entry->origin ));
		FS_Write( pFile, entry->angles, sizeof( entry->angles ));
		FS_Write( pFile, &entry->sequence, sizeof( entry->sequence ));
		FS_Write( pFile, &entry->frame, sizeof( entry->frame ));
		FS_Write( pFile, &entry->colormap, sizeof( entry->colormap ));
		FS_Write( pFile, &entry->skin, sizeof( entry->skin ));
		FS_Write( pFile, &entry->rendermode, sizeof( entry->rendermode ));

		if( entry->rendermode != kRenderNormal )
		{
			// write additional render data
			FS_Write( pFile, &entry->renderamt, sizeof( entry->renderamt ));
			FS_Write( pFile, &entry->rendercolor, sizeof( entry->rendercolor ));
			FS_Write( pFile, &entry->renderfx, sizeof( entry->renderfx ));
		}
	}

	// DYNAMIC SOUNDS SECTION (don't go across transition)
	if( !svgame.globals->changelevel && ( soundCount = S_GetCurrentDynamicSounds( soundInfo, MAX_CHANNELS )) != 0 )
	{
		sections.offsets[LUMP_SOUNDS_OFFSET] = FS_Tell( pFile );
		FS_Write( pFile, &soundCount, sizeof( int ));
	}

	for( i = 0; i < soundCount; i++ )
	{
		soundlist_t	*entry;
		byte		nameSize;

		entry = &soundInfo[i];

		nameSize = Q_strlen( entry->name ) + 1;

		FS_Write( pFile, &nameSize, sizeof( nameSize ));
		FS_Write( pFile, entry->name, nameSize ); 
		FS_Write( pFile, entry->origin, sizeof( entry->origin ));
		FS_Write( pFile, &entry->entnum, sizeof( entry->entnum ));
		FS_Write( pFile, &entry->volume, sizeof( entry->volume ));
		FS_Write( pFile, &entry->attenuation, sizeof( entry->attenuation ));
		FS_Write( pFile, &entry->looping, sizeof( entry->looping ));
		FS_Write( pFile, &entry->channel, sizeof( entry->channel ));
		FS_Write( pFile, &entry->pitch, sizeof( entry->pitch ));
		FS_Write( pFile, &entry->wordIndex, sizeof( entry->wordIndex ));
		FS_Write( pFile, &entry->samplePos, sizeof( entry->samplePos ));
		FS_Write( pFile, &entry->forcedEnd, sizeof( entry->forcedEnd ));
	}

	// BACKGROUND MUSIC SECTION (don't go across transition)
	if( !svgame.globals->changelevel && S_StreamGetCurrentState( curtrack, looptrack, &position ))
	{
		byte	nameSize;

		sections.offsets[LUMP_MUSIC_OFFSET] = FS_Tell( pFile );

		// write current track
		nameSize = Q_strlen( curtrack ) + 1;
		FS_Write( pFile, &nameSize, sizeof( nameSize ));
		FS_Write( pFile, curtrack, nameSize );

		// write loop track
		nameSize = Q_strlen( looptrack ) + 1;
		FS_Write( pFile, &nameSize, sizeof( nameSize ));
		FS_Write( pFile, looptrack, nameSize );

		// write current track position
		FS_Write( pFile, &position, sizeof( position ));
	}

	// AT END
	FS_Seek( pFile, header_offset, SEEK_SET );
	FS_Write( pFile, &sections, sizeof( sections ));	// write real sections info	

	FS_Close( pFile );
}

/*
=============
SV_LoadClientState

read the list of decals and reapply them again
=============
*/
void SV_LoadClientState( SAVERESTOREDATA *pSaveData, const char *level, qboolean adjacent )
{
	string		name;
	file_t		*pFile;
	int		i, tag;
	decallist_t	*decalList;
	int		decalCount;
	ClientSections_t	sections;
	soundlist_t	soundInfo[MAX_CHANNELS];
	int		soundCount;
	
	Q_snprintf( name, sizeof( name ), "save/%s.hl2", level );

	pFile = FS_Open( name, "rb", true );
	if( !pFile ) return;

	FS_Read( pFile, &tag, sizeof( int ));
	if( tag != SAVEFILE_HEADER )
	{
		FS_Close( pFile );
		return;
	}
		
	FS_Read( pFile, &tag, sizeof( int ));
	if( tag != CLIENT_SAVEGAME_VERSION )
	{
		FS_Close( pFile );
		return;
	}

	// read offsets
	FS_Read( pFile, &sections, sizeof( sections ));

	if( adjacent ) MsgDev( D_INFO, "Loading decals from %s\n", level );

	if( sections.offsets[LUMP_DECALS_OFFSET] != -1 )
	{
		// jump to decals description
		FS_Seek( pFile, sections.offsets[LUMP_DECALS_OFFSET], SEEK_SET );

		// read the decalCount
		FS_Read( pFile, &decalCount, sizeof( int ));
		decalList = (decallist_t *)Z_Malloc( sizeof( decallist_t ) * decalCount );

		// we can't use SaveRestore system here...
		for( i = 0; i < decalCount; i++ )
		{
			decallist_t	*entry;
			vec3_t		localPos;
			word		decalScale;
			byte		nameSize;

			entry = &decalList[i];

			FS_Read( pFile, localPos, sizeof( localPos ));
			FS_Read( pFile, &nameSize, sizeof( nameSize ));
			FS_Read( pFile, entry->name, nameSize ); 
			FS_Read( pFile, &entry->entityIndex, sizeof( entry->entityIndex ));
			FS_Read( pFile, &entry->flags, sizeof( entry->flags ));
			FS_Read( pFile, &decalScale, sizeof( decalScale ));
			FS_Read( pFile, entry->impactPlaneNormal, sizeof( entry->impactPlaneNormal ));

			if( pSaveData->fUseLandmark && ( entry->flags & FDECAL_USE_LANDMARK ))
				VectorAdd( localPos, pSaveData->vecLandmarkOffset, entry->position );
			else VectorCopy( localPos, entry->position );

			entry->scale = ((float)decalScale / 4096.0f);

			if( entry->flags & FDECAL_STUDIO )
			{
				// read additional data for studio decals
				FS_Read( pFile, &entry->studio_state, sizeof( entry->studio_state ));
			}

			ReapplyDecal( pSaveData, entry, adjacent );
		}

		Z_Free( decalList );
	}

	// NOTE: static entities can't be moved across the levels because they are static :-)
	if( sections.offsets[LUMP_STATIC_OFFSET] != -1 && !adjacent )
	{
		// jump to static entities description
		FS_Seek( pFile, sections.offsets[LUMP_STATIC_OFFSET], SEEK_SET );

		// put static entities back to global array so we can save it again
		FS_Read( pFile, &sv.num_static_entities, sizeof( int ));

		// clear old entities
		Q_memset( sv.static_entities, 0, sizeof( sv.static_entities ));

		for( i = 0; i < sv.num_static_entities; i++ )
		{
			sv_static_entity_t	*entry;
			byte		nameSize;

			if( i >= MAX_STATIC_ENTITIES )
			{
				MsgDev( D_ERROR, "SV_LoadClientState: too many static entities %i\n", sv.num_static_entities );
				break;
			}

			entry = &sv.static_entities[i];

			FS_Read( pFile, &nameSize, sizeof( nameSize ));
			FS_Read( pFile, entry->model, nameSize ); 
			FS_Read( pFile, entry->origin, sizeof( entry->origin ));
			FS_Read( pFile, entry->angles, sizeof( entry->angles ));
			FS_Read( pFile, &entry->sequence, sizeof( entry->sequence ));
			FS_Read( pFile, &entry->frame, sizeof( entry->frame ));
			FS_Read( pFile, &entry->colormap, sizeof( entry->colormap ));
			FS_Read( pFile, &entry->skin, sizeof( entry->skin ));
			FS_Read( pFile, &entry->rendermode, sizeof( entry->rendermode ));

			if( entry->rendermode != kRenderNormal )
			{
				// write additional render data
				FS_Read( pFile, &entry->renderamt, sizeof( entry->renderamt ));
				FS_Read( pFile, &entry->rendercolor, sizeof( entry->rendercolor ));
				FS_Read( pFile, &entry->renderfx, sizeof( entry->renderfx ));
			}

			SV_CreateStaticEntity( &sv.signon, entry );
		}
	}

	// NOTE: sounds can't be moved across level
	if( sections.offsets[LUMP_SOUNDS_OFFSET] != -1 && !adjacent )
	{
		// jump to sounds description
		FS_Seek( pFile, sections.offsets[LUMP_SOUNDS_OFFSET], SEEK_SET );

		FS_Read( pFile, &soundCount, sizeof( int ));

		for( i = 0; i < soundCount; i++ )
		{
			soundlist_t	*entry;
			byte		nameSize;

			entry = &soundInfo[i];

			FS_Read( pFile, &nameSize, sizeof( nameSize ));
			FS_Read( pFile, entry->name, nameSize ); 
			FS_Read( pFile, entry->origin, sizeof( entry->origin ));
			FS_Read( pFile, &entry->entnum, sizeof( entry->entnum ));
			FS_Read( pFile, &entry->volume, sizeof( entry->volume ));
			FS_Read( pFile, &entry->attenuation, sizeof( entry->attenuation ));
			FS_Read( pFile, &entry->looping, sizeof( entry->looping ));
			FS_Read( pFile, &entry->channel, sizeof( entry->channel ));
			FS_Read( pFile, &entry->pitch, sizeof( entry->pitch ));
			FS_Read( pFile, &entry->wordIndex, sizeof( entry->wordIndex ));
			FS_Read( pFile, &entry->samplePos, sizeof( entry->samplePos ));
			FS_Read( pFile, &entry->forcedEnd, sizeof( entry->forcedEnd ));

			RestoreSound( entry );
		}
	}

	// NOTE: music automatically goes across transition, never restore it on changelevel
	if( sections.offsets[LUMP_MUSIC_OFFSET] != -1 && !adjacent )
	{
		string	curtrack, looptrack;
		int	position;
		byte	nameSize;

		// jump to music description
		FS_Seek( pFile, sections.offsets[LUMP_MUSIC_OFFSET], SEEK_SET );

		// read current track
		FS_Read( pFile, &nameSize, sizeof( nameSize ));
		FS_Read( pFile, curtrack, nameSize );

		// read loop track
		FS_Read( pFile, &nameSize, sizeof( nameSize ));
		FS_Read( pFile, looptrack, nameSize );

		// read current track position
		FS_Read( pFile, &position, sizeof( position ));

		BF_WriteByte( &sv.signon, svc_stufftext );
		BF_WriteString( &sv.signon, va( "music \"%s\" \"%s\" %i\n", curtrack, looptrack, position ));
	}

	FS_Close( pFile );
}

/*
=============
SV_SaveGameState

save current game state
=============
*/
SAVERESTOREDATA *SV_SaveGameState( void )
{
	SaveFileSectionsInfo_t	sectionsInfo;
	SaveFileSections_t		sections;
	SAVERESTOREDATA		*pSaveData;
	ENTITYTABLE		*pTable;
	file_t			*pFile;
	int			i, numents;
	int			id, version;

	pSaveData = SV_SaveInit( 0 );

	// Save the data
	sections.pData = SaveRestore_AccessCurPos( pSaveData );

	numents = svgame.numEntities;

	SaveRestore_InitEntityTable( pSaveData, Mem_Alloc( host.mempool, sizeof(ENTITYTABLE) * numents ), numents );

	// Build the adjacent map list (after entity table build by game in presave)
	svgame.dllFuncs.pfnParmsChangeLevel();

	// write entity descriptions
	for( i = 0; i < svgame.numEntities; i++ )
	{
		edict_t	*pent = EDICT_NUM( i );

		pTable = &pSaveData->pTable[pSaveData->currentIndex];

		svgame.dllFuncs.pfnSave( pent, pSaveData );

		if( pent->v.flags & FL_CLIENT )	// mark client
			pTable->flags |= FENTTABLE_PLAYER;

		if( pTable->classname && pTable->size )
			pTable->id = NUM_FOR_EDICT( pent );

		pSaveData->currentIndex++; // move pointer
	}

	sectionsInfo.nBytesData = SaveRestore_AccessCurPos( pSaveData ) - sections.pData;
	
	// Save necessary tables/dictionaries/directories
	sections.pDataHeaders = SaveRestore_AccessCurPos( pSaveData );

	SV_SaveGameStateGlobals( pSaveData );

	sectionsInfo.nBytesDataHeaders = SaveRestore_AccessCurPos( pSaveData ) - sections.pDataHeaders;

	// Write the save file symbol table
	sections.pSymbols = SaveRestore_AccessCurPos( pSaveData );
	for( i = 0; i < pSaveData->tokenCount; i++ )
	{
		const char *pszToken = (SaveRestore_StringFromSymbol( pSaveData, i ));

		if( !pszToken ) pszToken = "";

		if( !SaveRestore_Write( pSaveData, pszToken, Q_strlen( pszToken ) + 1 ))
			break;
	}	

	sectionsInfo.nBytesSymbols = SaveRestore_AccessCurPos( pSaveData ) - sections.pSymbols;
	sectionsInfo.nSymbols = pSaveData->tokenCount;

	id = SAVEFILE_HEADER;
	version = SAVEGAME_VERSION;

	// output to disk
	pFile = FS_Open( va( "save/%s.hl1", sv.name ), "wb", true );
	if( !pFile ) return NULL;

	// write the header
	FS_Write( pFile, &id, sizeof( int ));
	FS_Write( pFile, &version, sizeof( int ));

	// Write out the tokens and table FIRST so they are loaded in the right order,
	// then write out the rest of the data in the file.
	FS_Write( pFile, &sectionsInfo, sizeof( sectionsInfo ));
	FS_Write( pFile, sections.pSymbols, sectionsInfo.nBytesSymbols );
	FS_Write( pFile, sections.pDataHeaders, sectionsInfo.nBytesDataHeaders );
	FS_Write( pFile, sections.pData, sectionsInfo.nBytesData );
	FS_Close( pFile );

	SV_EntityPatchWrite( pSaveData, sv.name );

	SV_SaveClientState( pSaveData, sv.name );

	return pSaveData;
}

int SV_LoadGameState( char const *level, qboolean createPlayers )
{
	SAVE_HEADER	header;
	SAVERESTOREDATA	*pSaveData;
	ENTITYTABLE	*pEntInfo;
	edict_t		*pent;
	int		i;

	pSaveData = SV_LoadSaveData( level );
	if( !pSaveData ) return 0; // couldn't load the file

	SV_ParseSaveTables( pSaveData, &header, true );

	SV_EntityPatchRead( pSaveData, level );

	Cvar_SetFloat( "skill", header.skillLevel );
	Q_strncpy( sv.name, header.mapName, sizeof( sv.name ));
	svgame.globals->mapname = MAKE_STRING( sv.name );
	Cvar_Set( "sv_skyname", header.skyName );

	// restore sky parms
	Cvar_SetFloat( "sv_skycolor_r", header.skyColor_r );
	Cvar_SetFloat( "sv_skycolor_g", header.skyColor_g );
	Cvar_SetFloat( "sv_skycolor_b", header.skyColor_b );
	Cvar_SetFloat( "sv_skyvec_x", header.skyVec_x );
	Cvar_SetFloat( "sv_skyvec_y", header.skyVec_y );
	Cvar_SetFloat( "sv_skyvec_z", header.skyVec_z );
	Cvar_SetFloat( "sv_skydir_x", header.skyDir_x );
	Cvar_SetFloat( "sv_skydir_y", header.skyDir_y );
	Cvar_SetFloat( "sv_skydir_z", header.skyDir_z );
	Cvar_SetFloat( "sv_skyangle", header.skyAngle );
	Cvar_SetFloat( "sv_skyspeed", header.skySpeed );

	// restore serverflags
	svgame.globals->serverflags = header.serverflags;

	if( header.wateralpha <= 0.0f ) header.wateralpha = 1.0f; // make compatibility with old saves
	Cvar_SetFloat( "sv_wateralpha", header.wateralpha );

	// re-base the savedata since we re-ordered the entity/table / restore fields
	SaveRestore_Rebase( pSaveData );

	// create entity list
	if( svgame.physFuncs.pfnCreateEntitiesInRestoreList != NULL )
	{
		svgame.physFuncs.pfnCreateEntitiesInRestoreList( pSaveData, createPlayers );
	}
	else
	{
		for( i = 0; i < pSaveData->tableCount; i++ )
		{
			pEntInfo = &pSaveData->pTable[i];

			if( pEntInfo->classname != 0 && pEntInfo->size && !( pEntInfo->flags & FENTTABLE_REMOVED ))
			{
				if( pEntInfo->id == 0 ) // worldspawn
				{
					ASSERT( i == 0 );

					pent = EDICT_NUM( 0 );

					SV_InitEdict( pent );
					pent = SV_AllocPrivateData( pent, pEntInfo->classname );
				}
				else if(( pEntInfo->id > 0 ) && ( pEntInfo->id < svgame.globals->maxClients + 1 ))
				{
					edict_t	*ed;

					if(!( pEntInfo->flags & FENTTABLE_PLAYER ))
					{
						MsgDev( D_WARN, "ENTITY IS NOT A PLAYER: %d\n", i );
						//ASSERT( 0 );
					}

					ed = EDICT_NUM( pEntInfo->id );

					if( ed && createPlayers )
					{
						ASSERT( ed->free == false );
						// create the player
						pent = SV_AllocPrivateData( ed, pEntInfo->classname );
					}
					else pent = NULL;
				}
				else
				{
					pent = SV_AllocPrivateData( NULL, pEntInfo->classname );
				}

				pEntInfo->pent = pent;
			}
			else
			{
				pEntInfo->pent = NULL; // invalid
			}
		}
	}

	// now spawn entities
	for( i = 0; i < pSaveData->tableCount; i++ )
	{
		pEntInfo = &pSaveData->pTable[i];

		pent = pEntInfo->pent;
		SaveRestore_Seek( pSaveData, pEntInfo->location );

		// prevent crash on recursive saverestore
		svgame.globals->pSaveData = pSaveData;

		if( pent )
		{
			if( svgame.dllFuncs.pfnRestore( pent, pSaveData, false ) < 0 )
			{
				pEntInfo->pent = NULL;
				pent->v.flags |= FL_KILLME;
			}
		}
	}

	// restore camera view here
	pent = pSaveData->pTable[bound( 0, (word)header.viewentity, pSaveData->tableCount )].pent;

	// don't go camera across the levels
	if( SV_IsValidEdict( pent ) && !svgame.globals->changelevel )
		sv.viewentity = NUM_FOR_EDICT( pent );
	else sv.viewentity = 0;

	// just use normal client view
	if( sv.viewentity == 1 ) sv.viewentity = 0;

	SV_LoadClientState( pSaveData, level, false );

	SV_SaveFinish( pSaveData );

	// restore server time
	sv.time = header.time;
	
	return 1;
}

// ripped out from the hl.dll
edict_t *SV_FindGlobalEntity( string_t classname, string_t globalname )
{
	edict_t *pent = SV_FindEntityByString( NULL,  "globalname", STRING( globalname ));

	if( SV_IsValidEdict( pent ))
	{
		if( Q_strcmp( SV_ClassName( pent ), STRING( classname )))
		{
			MsgDev( D_ERROR, "Global entity found %s, wrong class %s\n", STRING( globalname ), SV_ClassName( pent ));
			pent = NULL;
		}
	}

	return pent;
}

//-----------------------------------------------------------------------------
int SV_CreateEntityTransitionList( SAVERESTOREDATA *pSaveData, int levelMask )
{
	edict_t		*pent;
	ENTITYTABLE	*pEntInfo;
	int		i, movedCount, active;

	movedCount = 0;

	// create entity list
	if( svgame.physFuncs.pfnCreateEntitiesInTransitionList != NULL )
	{
		svgame.physFuncs.pfnCreateEntitiesInTransitionList( pSaveData, levelMask );
	}
	else
	{
		for( i = 0; i < pSaveData->tableCount; i++ )
		{
			pEntInfo = &pSaveData->pTable[i];
			pent = NULL;

			if( pEntInfo->size && pEntInfo->id != 0 )
			{
				if( pEntInfo->classname != 0 )
				{
					active = (pEntInfo->flags & levelMask) ? 1 : 0;

					// spawn players
					if(( pEntInfo->id > 0) && ( pEntInfo->id < svgame.globals->maxClients + 1 ))	
					{
						edict_t	*ed = EDICT_NUM( pEntInfo->id );

						if( active && ed && !ed->free )
						{
							if(!( pEntInfo->flags & FENTTABLE_PLAYER ))
							{
								MsgDev( D_WARN, "ENTITY IS NOT A PLAYER: %d\n", i );
								ASSERT( 0 );
							}

							pent = SV_AllocPrivateData( ed, pEntInfo->classname );
						}
					}
					else if( active )
					{
						// create named entity
						pent = SV_AllocPrivateData( NULL, pEntInfo->classname );
					}
				}
				else
				{
					MsgDev( D_WARN, "Entity with data saved, but with no classname\n" );
				}
			}

			pEntInfo->pent = pent;
		}
	}

	// re-base the savedata since we re-ordered the entity/table / restore fields
	SaveRestore_Rebase( pSaveData );
	
	// now spawn entities
	for( i = 0; i < pSaveData->tableCount; i++ )
	{
		pEntInfo = &pSaveData->pTable[i];
		pent = pEntInfo->pent;
		pSaveData->currentIndex = i;
		SaveRestore_Seek( pSaveData, pEntInfo->location );
		
		if( SV_IsValidEdict( pent ) && ( pEntInfo->flags & levelMask )) // screen out the player if he's not to be spawned
		{
			if( pEntInfo->flags & FENTTABLE_GLOBAL )
			{
				entvars_t	tmpVars;
				edict_t	*pNewEnt;

				// NOTE: we need to update table pointer so decals on the global entities with brush models can be correctly moved
				// found the classname and the globalname for our globalentity
				svgame.dllFuncs.pfnSaveReadFields( pSaveData, "ENTVARS", &tmpVars, gEntvarsDescription, ARRAYSIZE( gEntvarsDescription ));

				// reset the save pointers, so dll can read this too
				pSaveData->size = pSaveData->pTable[pSaveData->currentIndex].location;
				pSaveData->pCurrentData = pSaveData->pBaseData + pSaveData->size;

				// IMPORTANT: we should find the already spawned or local restored global entity
				pNewEnt = SV_FindGlobalEntity( tmpVars.classname, tmpVars.globalname );

				MsgDev( D_INFO, "Merging changes for global: %s\n", STRING( pEntInfo->classname ));

				// -------------------------------------------------------------------------
				// Pass the "global" flag to the DLL to indicate this entity should only override
				// a matching entity, not be spawned
				if( svgame.dllFuncs.pfnRestore( pent, pSaveData, true ) > 0 )
				{
					movedCount++;
				}
				else
				{
					if( SV_IsValidEdict( pNewEnt )) // update the table so decals can find entity
						pSaveData->pTable[pSaveData->currentIndex].pent = pNewEnt;
					pent->v.flags |= FL_KILLME;
				}
			}
			else 
			{
				MsgDev( D_INFO, "Transferring %s (%d)\n", STRING( pEntInfo->classname ), NUM_FOR_EDICT( pent ));

				if( svgame.dllFuncs.pfnRestore( pent, pSaveData, false ) < 0 )
				{
					pent->v.flags |= FL_KILLME;
				}
				else
				{
					if( !( pEntInfo->flags & FENTTABLE_PLAYER ) && EntityInSolid( pent ))
					{
						// this can happen during normal processing - PVS is just a guess,
						// some map areas won't exist in the new map
						MsgDev( D_INFO, "Suppressing %s\n", STRING( pEntInfo->classname ));
						pent->v.flags |= FL_KILLME;
					}
					else
					{
						pEntInfo->flags = FENTTABLE_REMOVED;
						movedCount++;
					}
				}
			}

			// remove any entities that were removed using UTIL_Remove()
			// as a result of the above calls to UTIL_RemoveImmediate()
			SV_FreeOldEntities ();
		}
	}

	return movedCount;
}

void SV_LoadAdjacentEnts( const char *pOldLevel, const char *pLandmarkName )
{
	SAVE_HEADER	header;
	SAVERESTOREDATA	currentLevelData, *pSaveData;
	int		i, test, flags, index, movedCount = 0;
	qboolean		foundprevious = false;
	vec3_t		landmarkOrigin;
	
	Q_memset( &currentLevelData, 0, sizeof( SAVERESTOREDATA ));
	svgame.globals->pSaveData = &currentLevelData;

	// build the adjacent map list
	svgame.dllFuncs.pfnParmsChangeLevel();

	for( i = 0; i < currentLevelData.connectionCount; i++ )
	{
		// make sure the previous level is in the connection list so we can
		// bring over the player.
		if( !Q_stricmp( currentLevelData.levelList[i].mapName, pOldLevel ))
		{
			foundprevious = true;
		}

		for( test = 0; test < i; test++ )
		{
			// only do maps once
			if( !Q_strcmp( currentLevelData.levelList[i].mapName, currentLevelData.levelList[test].mapName ))
				break;
		}

		// map was already in the list
		if( test < i ) continue;

		MsgDev( D_NOTE, "Merging entities from %s ( at %s )\n", currentLevelData.levelList[i].mapName, currentLevelData.levelList[i].landmarkName );
		pSaveData = SV_LoadSaveData( currentLevelData.levelList[i].mapName );

		if( pSaveData )
		{
			SV_ParseSaveTables( pSaveData, &header, false );
			SV_EntityPatchRead( pSaveData, currentLevelData.levelList[i].mapName );

			pSaveData->time = sv.time; // - header.time;
			pSaveData->fUseLandmark = true;

			// calculate landmark offset
			LandmarkOrigin( &currentLevelData, landmarkOrigin, pLandmarkName );
			LandmarkOrigin( pSaveData, pSaveData->vecLandmarkOffset, pLandmarkName );
			VectorSubtract( landmarkOrigin, pSaveData->vecLandmarkOffset, pSaveData->vecLandmarkOffset );

			flags = 0;
			index = -1;

			if( !Q_strcmp( currentLevelData.levelList[i].mapName, pOldLevel ))
				flags |= FENTTABLE_PLAYER;

			while( 1 )
			{
				index = EntryInTable( pSaveData, sv.name, index );
				if( index < 0 ) break;
				flags |= (1U << index);
			}

			if( flags ) movedCount = SV_CreateEntityTransitionList( pSaveData, flags );

			// if ents were moved, rewrite entity table to save file
			if( movedCount ) SV_EntityPatchWrite( pSaveData, currentLevelData.levelList[i].mapName );

			// move the decals from another level
			SV_LoadClientState( pSaveData, currentLevelData.levelList[i].mapName, true );

			SV_SaveFinish( pSaveData );
		}
	}

	svgame.globals->pSaveData = NULL;

	if( !foundprevious )
	{
		Host_Error( "Level transition ERROR\nCan't find connection to %s from %s\n", pOldLevel, sv.name );
	}
}

/*
=============
SV_ChangeLevel
=============
*/
void SV_ChangeLevel( qboolean loadfromsavedgame, const char *mapname, const char *start )
{
	string		level;
	string		oldlevel;
	string		_startspot;
	char		*startspot;
	SAVERESTOREDATA	*pSaveData = NULL;
	
	if( sv.state != ss_active )
	{
		Msg( "SV_ChangeLevel: server not running\n");
		return;
	}

	if( !start )
	{
		startspot = NULL;
	}
	else
	{
		Q_strncpy( _startspot, start, MAX_STRING );
		startspot = _startspot;
	}

	// init network stuff
	NET_Config( ( sv_maxclients->integer > 1 ), false );
	Q_strncpy( level, mapname, MAX_STRING );
	Q_strncpy( oldlevel, sv.name, MAX_STRING );
	sv.background = false;
	sv.changelevel = true;	// NOTE: this is used to indicate changelevel for classic Quake changelevel
				// because demos wan't properly update clock on a new level while recording

	if( loadfromsavedgame )
	{
		// smooth transition in-progress
		svgame.globals->changelevel = true;

		// save the current level's state
		pSaveData = SV_SaveGameState();
		sv.loadgame = true;
	}

	SV_InactivateClients ();
	SV_DeactivateServer ();

	if( !SV_SpawnServer( level, startspot ))
		return;

	if( loadfromsavedgame )
	{
		// Finish saving gamestate
		SV_SaveFinish( pSaveData );

		svgame.globals->changelevel = true;
		SV_LevelInit( level, oldlevel, startspot, true );
		sv.paused = true; // pause until all clients connect
		sv.loadgame = true;
	}
	else
	{
		SV_LevelInit( level, NULL, NULL, false );
	}

	SV_ActivateServer ();
}

int SV_SaveGameSlot( const char *pSaveName, const char *pSaveComment )
{
	string		hlPath, name;
	char		*pTokenData;
	SAVERESTOREDATA	*pSaveData;
	GAME_HEADER	gameHeader;
	int		i, tag, tokenSize;
	file_t		*pFile;

	pSaveData = SV_SaveGameState();
	if( !pSaveData ) return 0;

	Q_memset( &gameHeader, 0, sizeof( GAME_HEADER ) );

	SV_SaveFinish( pSaveData );

	pSaveData = SV_SaveInit( 0 );

	Q_strncpy( hlPath, "save/*.hl?", sizeof( hlPath ));
	gameHeader.mapCount = SV_MapCount( hlPath );
	Q_strncpy( gameHeader.mapName, sv.name, sizeof( gameHeader.mapName ));
	Q_strncpy( gameHeader.comment, pSaveComment, sizeof( gameHeader.comment ));

	svgame.dllFuncs.pfnSaveWriteFields( pSaveData, "GameHeader", &gameHeader, gGameHeader, ARRAYSIZE( gGameHeader ));
	svgame.dllFuncs.pfnSaveGlobalState( pSaveData );

	// write entity string token table
	pTokenData = SaveRestore_AccessCurPos( pSaveData );
	for( i = 0; i < pSaveData->tokenCount; i++ )
	{
		const char *pszToken = (SaveRestore_StringFromSymbol( pSaveData, i ));
		if( !pszToken ) pszToken = "";

		if( !SaveRestore_Write( pSaveData, pszToken, Q_strlen( pszToken ) + 1 ))
		{
			MsgDev( D_ERROR, "Token Table Save/Restore overflow!\n" );
			break;
		}
	}	

	tokenSize = SaveRestore_AccessCurPos( pSaveData ) - pTokenData;
	SaveRestore_Rewind( pSaveData, tokenSize );

	Q_snprintf( name, sizeof( name ), "save/%s.sav", pSaveName );
	MsgDev( D_INFO, "Saving game to %s...\n", name );

	Cbuf_AddText( va( "saveshot \"%s\"\n", pSaveName ));

	// output to disk
	if( !Q_stricmp( pSaveName, "quick" ) || !Q_stricmp( pSaveName, "autosave" ))
		SV_AgeSaveList( pSaveName, SAVE_AGED_COUNT );

	pFile = FS_Open( name, "wb", true );

	tag = SAVEGAME_HEADER;
	FS_Write( pFile, &tag, sizeof( int ));
	tag = SAVEGAME_VERSION;
	FS_Write( pFile, &tag, sizeof( int ));
	tag = SaveRestore_GetCurPos( pSaveData );
	FS_Write( pFile, &tag, sizeof( int )); // does not include token table

	// write out the tokens first so we can load them before we load the entities
	tag = pSaveData->tokenCount;
	FS_Write( pFile, &tag, sizeof( int ));
	FS_Write( pFile, &tokenSize, sizeof( int ));
	FS_Write( pFile, pTokenData, tokenSize );

	// save gamestate
	FS_Write( pFile, SaveRestore_GetBuffer( pSaveData ), SaveRestore_GetCurPos( pSaveData ));

	SV_DirectoryCopy( hlPath, pFile );
	FS_Close( pFile );
	SV_SaveFinish( pSaveData );

	return 1;
}

int SV_SaveReadHeader( file_t *pFile, GAME_HEADER *pHeader, int readGlobalState )
{
	int		i, tag, size, tokenCount, tokenSize;
	char		*pszTokenList;
	SAVERESTOREDATA	*pSaveData;

	FS_Read( pFile, &tag, sizeof( int ));
	if( tag != SAVEGAME_HEADER )
	{
		FS_Close( pFile );
		return 0;
	}
		
	FS_Read( pFile, &tag, sizeof( int ));
	if( tag != SAVEGAME_VERSION )
	{
		FS_Close( pFile );
		return 0;
	}

	FS_Read( pFile, &size, sizeof( int ));
	FS_Read( pFile, &tokenCount, sizeof( int ));
	FS_Read( pFile, &tokenSize, sizeof( int ));

	pSaveData = Mem_Alloc( host.mempool, sizeof( SAVERESTOREDATA ) + tokenSize + size );
	pSaveData->connectionCount = 0;
	pszTokenList = (char *)(pSaveData + 1);

	if( tokenSize > 0 )
	{
		FS_Read( pFile, pszTokenList, tokenSize );

		SaveRestore_InitSymbolTable( pSaveData, (char **)Mem_Alloc( host.mempool, tokenCount * sizeof( char* )), tokenCount );

		// make sure the token strings pointed to by the pToken hashtable.
		for( i = 0; i < tokenCount; i++ )
		{
			if( *pszTokenList )
			{
				ASSERT( SaveRestore_DefineSymbol( pSaveData, pszTokenList, i ));
			}

			while( *pszTokenList++ ); // find next token (after next null)
		}
	}
	else
	{
		SaveRestore_InitSymbolTable( pSaveData, NULL, 0 );
	}

	pSaveData->fUseLandmark = false;
	pSaveData->time = 0.0f;

	// pszTokenList now points after token data
	SaveRestore_Init( pSaveData, (char *)(pszTokenList), size );
	FS_Read( pFile, SaveRestore_GetBuffer( pSaveData ), size );

	if( readGlobalState )
		svgame.dllFuncs.pfnResetGlobalState();

	svgame.dllFuncs.pfnSaveReadFields( pSaveData, "GameHeader", pHeader, gGameHeader, ARRAYSIZE( gGameHeader ));	

	if( readGlobalState )
		svgame.dllFuncs.pfnRestoreGlobalState( pSaveData );

	SV_SaveFinish( pSaveData );
	
	return 1;
}

qboolean SV_LoadGame( const char *pName )
{
	file_t		*pFile;
	qboolean		validload = false;
	GAME_HEADER	gameHeader;
	string		name;

	if( !pName || !pName[0] )
		return false;

	Q_snprintf( name, sizeof( name ), "save/%s.sav", pName );

	// silently ignore if missed
	if( !FS_FileExists( name, true ))
		return false;

	NET_Config ( false, false ); // close network sockets

	if( sv.background || sv_maxclients->integer > 1 )
		SV_Shutdown( true );
	sv.background = false;

	SCR_BeginLoadingPlaque ( false );
	S_StopBackgroundTrack();

	MsgDev( D_INFO, "Loading game from %s...\n", name );
	SV_ClearSaveDir();

	if( !svs.initialized ) SV_InitGame ();
	if( !svs.initialized ) return false;

	pFile = FS_Open( name, "rb", true );

	if( pFile )
	{
		if( SV_SaveReadHeader( pFile, &gameHeader, 1 ))
		{
			SV_DirectoryExtract( pFile, gameHeader.mapCount );
			validload = true;
		}
		FS_Close( pFile );
	}
	else MsgDev( D_ERROR, "File not found or failed to open.\n" );

	if( !validload )
	{
		Q_snprintf( host.finalmsg, MAX_STRING, "Couldn't load %s.sav\n", pName );
		SV_Shutdown( false );
		return false;
	}

	if( !Host_IsDedicated() )
	{
		Cvar_FullSet( "coop", "0", CVAR_LATCH );
		Cvar_FullSet( "teamplay", "0", CVAR_LATCH );
		Cvar_FullSet( "deathmatch", "0", CVAR_LATCH );
		Cvar_FullSet( "maxplayers", "1", CVAR_LATCH );
	}

	return Host_NewGame( gameHeader.mapName, true );
}

/*
================== 
SV_SaveGetName
================== 
*/  
void SV_SaveGetName( int lastnum, char *filename )
{
	if( !filename )
		return;

	if( lastnum < 0 || lastnum > 999 )
	{
		// bound
		Q_strcpy( filename, "error" );
		return;
	}

	Q_sprintf( filename, "save%03d", lastnum );
}

void SV_SaveGame( const char *pName )
{
	char	comment[80];
	string	savename;
	int	n;

	if( !pName || !*pName )
		return;

	// can we save at this point?
	if( !SV_IsValidSave( ))
		return;

	if( !Q_stricmp( pName, "new" ))
	{
		// scan for a free filename
		for( n = 0; n < 999; n++ )
		{
			SV_SaveGetName( n, savename );

			if( !FS_FileExists( va( "save/%s.sav", savename ), true ))
				break;
		}

		if( n == 1000 )
		{
			Msg( "^3ERROR: no free slots for savegame\n" );
			return;
		}
	}
	else Q_strncpy( savename, pName, sizeof( savename ));

	// HACKHACK: unload previous image from memory
	GL_FreeImage( va( "save/%s.bmp", savename ));

	comment[0] = '\0';

	SV_BuildSaveComment( comment, sizeof( comment ));
	SV_SaveGameSlot( savename, comment );

	// HACKHACK: send usermessage from engine
	if( Q_stricmp( pName, "autosave" ) && svgame.gmsgHudText != -1 )
	{
		const char *pMsg = "GAMESAVED"; // defined in titles.txt
		sv_client_t *cl;

		if(( cl = SV_ClientFromEdict( EDICT_NUM( 1 ), true )) != NULL )
		{
			BF_WriteByte( &cl->netchan.message, svgame.gmsgHudText );
			BF_WriteByte( &cl->netchan.message, Q_strlen( pMsg ) + 1 );
			BF_WriteString( &cl->netchan.message, pMsg );
		}
	}
}

/* 
================== 
SV_GetLatestSave

used for reload game after player death
================== 
*/
const char *SV_GetLatestSave( void )
{
	search_t	*f = FS_Search( "save/*.sav", true, true );	// lookup only in gamedir
	int	i, found = 0;
	int	newest = 0, ft;
	string	savename;	

	if( !f ) return NULL;

	for( i = 0; i < f->numfilenames; i++ )
	{
		ft = FS_FileTime( f->filenames[i], true );
		
		// found a match?
		if( ft > 0 )
		{
			// should we use the matche?
			if( !found || Host_CompareFileTime( newest, ft ) < 0 )
			{
				newest = ft;
				Q_strncpy( savename, f->filenames[i], MAX_STRING );
				found = 1;
			}
		}
	}

	Mem_Free( f ); // release search

	if( found )
		return va( "%s", savename ); // move to static memory
	return NULL; 
}

qboolean SV_GetComment( const char *savename, char *comment )
{
	int	i, tag, size, nNumberOfFields, nFieldSize, tokenSize, tokenCount;
	char	*pData, *pSaveData, *pFieldName, **pTokenList;
	string	name, description;
	file_t	*f;
	short shortpool;

	f = FS_Open( savename, "rb", true );
	if( !f )
	{
		// just not exist - clear comment
		Q_strncpy( comment, "", MAX_STRING );
		return 0;
	}

	FS_Read( f, &tag, sizeof( int ));
	if( tag != SAVEGAME_HEADER )
	{
		// invalid header
		Q_strncpy( comment, "<corrupted>", MAX_STRING );
		FS_Close( f );
		return 0;
	}
		
	FS_Read( f, &tag, sizeof( int ));

	if( tag == 0x0071 )
	{
		Q_strncpy( comment, "Gold Source <unsupported>", MAX_STRING );
		FS_Close( f );
		return 0;
	}

	if( tag < SAVEGAME_VERSION )
	{
		Q_strncpy( comment, "<old version>", MAX_STRING );
		FS_Close( f );
		return 0;
	}

	if( tag > SAVEGAME_VERSION )
	{
		// old xash version ?
		Q_strncpy( comment, "<unknown version>", MAX_STRING );
		FS_Close( f );
		return 0;
	}

	name[0] = '\0';
	comment[0] = '\0';

	FS_Read( f, &size, sizeof( int ));
	FS_Read( f, &tokenCount, sizeof( int ));	// These two ints are the token list
	FS_Read( f, &tokenSize, sizeof( int ));
	size += tokenSize;

	// sanity check.
	if( tokenCount <= 0 || tokenCount > ( 1024 * 1024 * 32 ))
	{
		Q_strncpy( comment, "<corrupted>", MAX_STRING );
		FS_Close( f );
		return 0;
	}

	if( tokenSize <= 0 || tokenSize > ( 1024 * 1024 * 32 ))
	{
		Q_strncpy( comment, "<corrupted>", MAX_STRING );
		FS_Close( f );
		return 0;
	}

	pSaveData = (char *)Mem_Alloc( host.mempool, size );
	FS_Read( f, pSaveData, size );
	pData = pSaveData;

	// allocate a table for the strings, and parse the table
	pTokenList = Mem_Alloc( host.mempool, tokenCount * sizeof( char* ));

	if( !pTokenList || !pSaveData )
	{
		Q_strncpy( comment, "<corrupted>", MAX_STRING );

		FS_Close( f );
		Z_Free( pTokenList );
		Z_Free( pSaveData );
		return 0;
	}

	// make sure the token strings pointed to by the pToken hashtable.
	for( i = 0; i < tokenCount; i++ )
	{
		pTokenList[i] = *pData ? pData : NULL;	// point to each string in the pToken table
		while( *pData++ );			// find next token (after next null)
	}

	// short, short (size, index of field name)
	Q_memcpy(&shortpool, pData, sizeof(short));
	nFieldSize = shortpool;
	pData += sizeof( short );

	Q_memcpy(&shortpool, pData, sizeof(short));
	pFieldName = pTokenList[shortpool];

	if( Q_stricmp( pFieldName, "GameHeader" ))
	{
		Q_strncpy( comment, "<missing GameHeader>", MAX_STRING );
		Mem_Free( pTokenList );
		Mem_Free( pSaveData );
		FS_Close( f );
		return 0;
	}

	// int (fieldcount)
	pData += sizeof( short );
	nNumberOfFields = (int)*pData;
	pData += nFieldSize;

	// each field is a short (size), short (index of name), binary string of "size" bytes (data)
	for( i = 0; i < nNumberOfFields; i++ )
	{
		// Data order is:
		// Size
		// szName
		// Actual Data
		Q_memcpy(&shortpool, pData, sizeof(short));
		nFieldSize = shortpool;
		pData += sizeof( short );

		Q_memcpy(&shortpool, pData, sizeof(short));
		pFieldName = pTokenList[shortpool];
		pData += sizeof( short );

		if( !Q_stricmp( pFieldName, "comment" ))
		{
			Q_strncpy( description, pData, nFieldSize );
		}
		else if( !Q_stricmp( pFieldName, "mapName" ))
		{
			Q_strncpy( name, pData, nFieldSize );
		}

		// move to start of next field.
		pData += nFieldSize;
	}

	// delete the string table we allocated
	Mem_Free( pTokenList );
	Mem_Free( pSaveData );
	FS_Close( f );	

	if( Q_strlen( name ) > 0 && Q_strlen( description ) > 0 )
	{
		time_t		fileTime;
		const struct tm	*file_tm;
		string		timestring;
	
		fileTime = FS_FileTime( savename, true );
		file_tm = localtime( &fileTime );

		// split comment to sections
		if( Q_strstr( savename, "quick" ))
			Q_strncat( comment, "[quick]", CS_SIZE );
		else if( Q_strstr( savename, "autosave" ))
			Q_strncat( comment, "[autosave]", CS_SIZE );
		Q_strncat( comment, description, CS_SIZE );
		strftime( timestring, sizeof ( timestring ), "%b%d %Y", file_tm );
		Q_strncpy( comment + CS_SIZE, timestring, CS_TIME );
		strftime( timestring, sizeof( timestring ), "%H:%M", file_tm );
		Q_strncpy( comment + CS_SIZE + CS_TIME, timestring, CS_TIME );
		Q_strncpy( comment + CS_SIZE + (CS_TIME * 2), description + CS_SIZE, CS_SIZE );

		return 1;
	}	

	Q_strncpy( comment, "<unknown version>", MAX_STRING );

	return 0;
}
