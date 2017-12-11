/*
sv_init.c - server initialize operations
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

#include "common.h"
#include "server.h"
#include "library.h"
#include "Sequence.h"
#include "net_encode.h"

int SV_UPDATE_BACKUP = SINGLEPLAYER_BACKUP;

server_static_t	svs;	// persistant server info
svgame_static_t	svgame;	// persistant game info
server_t		sv;	// local server

int SV_ModelIndex( const char *filename )
{
	char	name[64];
	int	i;

	if( !filename || !filename[0] )
		return 0;

	if( *filename == '!' ) filename++;
	Q_strncpy( name, filename, sizeof( name ));
	COM_FixSlashes( name );

	for( i = 1; i < MAX_MODELS && sv.model_precache[i][0]; i++ )
	{
		if( !Q_stricmp( sv.model_precache[i], name ))
			return i;
	}

	if( i == MAX_MODELS )
	{
		Host_Error( "SV_ModelIndex: MAX_MODELS limit exceeded\n" );
		return 0;
	}

	// register new model
	Q_strncpy( sv.model_precache[i], name, sizeof( sv.model_precache[i] ));

	if( sv.state != ss_loading )
	{	
		// send the update to everyone
		BF_WriteByte( &sv.reliable_datagram, svc_modelindex );
		BF_WriteUBitLong( &sv.reliable_datagram, i, MAX_MODEL_BITS );
		BF_WriteString( &sv.reliable_datagram, name );
	}

	return i;
}

int GAME_EXPORT SV_SoundIndex( const char *filename )
{
	char	name[64];
	int	i;

	// don't precache sentence names!
	if( !filename || !filename[0] || filename[0] == '!' )
		return 0;

	Q_strncpy( name, filename, sizeof( name ));
	COM_FixSlashes( name );

	for( i = 1; i < MAX_SOUNDS && sv.sound_precache[i][0]; i++ )
	{
		if( !Q_stricmp( sv.sound_precache[i], name ))
			return i;
	}

	if( i == MAX_SOUNDS )
	{
		Host_Error( "SV_SoundIndex: MAX_SOUNDS limit exceeded\n" );
		return 0;
	}

	sv.resourcelistcache = false;
	sv.reslist.rescount = 0;

	// register new sound
	Q_strncpy( sv.sound_precache[i], name, sizeof( sv.sound_precache[i] ));

	if( sv.state != ss_loading )
	{	
		// send the update to everyone
		BF_WriteByte( &sv.reliable_datagram, svc_soundindex );
		BF_WriteUBitLong( &sv.reliable_datagram, i, MAX_SOUND_BITS );
		BF_WriteString( &sv.reliable_datagram, name );
	}

	return i;
}

int SV_EventIndex( const char *filename )
{
	char	name[64];
	int	i;

	if( !filename || !filename[0] )
		return 0;

	Q_strncpy( name, filename, sizeof( name ));
	COM_FixSlashes( name );

	for( i = 1; i < MAX_EVENTS && sv.event_precache[i][0]; i++ )
	{
		if( !Q_stricmp( sv.event_precache[i], name ))
			return i;
	}

	if( i == MAX_EVENTS )
	{
		Host_Error( "SV_EventIndex: MAX_EVENTS limit exceeded\n" );
		return 0;
	}

	// register new event
	Q_strncpy( sv.event_precache[i], name, sizeof( sv.event_precache[i] ));

	if( sv.state != ss_loading )
	{
		// send the update to everyone
		BF_WriteByte( &sv.reliable_datagram, svc_eventindex );
		BF_WriteUBitLong( &sv.reliable_datagram, i, MAX_EVENT_BITS );
		BF_WriteString( &sv.reliable_datagram, name );
	}

	return i;
}

int GAME_EXPORT SV_GenericIndex( const char *filename )
{
	char	name[64];
	int	i;

	if( !filename || !filename[0] )
		return 0;

	Q_strncpy( name, filename, sizeof( name ));
	COM_FixSlashes( name );

	for( i = 1; i < MAX_CUSTOM && sv.files_precache[i][0]; i++ )
	{
		if( !Q_stricmp( sv.files_precache[i], name ))
			return i;
	}

	if( i == MAX_CUSTOM )
	{
		Host_Error( "SV_GenericIndex: MAX_RESOURCES limit exceeded\n" );
		return 0;
	}

	if( sv.state != ss_loading )
	{
		// g-cont. can we downloading resources in-game ? need testing
		Host_Error( "SV_PrecacheGeneric: ( %s ). Precache can only be done in spawn functions.", name );
		return 0;
	}

	sv.resourcelistcache = false;
	sv.reslist.rescount = 0;

	// register new generic resource
	Q_strncpy( sv.files_precache[i], name, sizeof( sv.files_precache[i] ));

	return i;
}

/*
================
SV_EntityScript

get entity script for current map
================
*/
char *SV_EntityScript( void )
{
	string	entfilename;
	char	*ents;
	size_t	ft1, ft2;

	if( !sv.worldmodel )
		return NULL;

	// check for entfile too
	Q_strncpy( entfilename, sv.worldmodel->name, sizeof( entfilename ));
	FS_StripExtension( entfilename );
	FS_DefaultExtension( entfilename, ".ent" );

	// make sure what entity patch is never than bsp
	ft1 = FS_FileTime( sv.worldmodel->name, false );
	ft2 = FS_FileTime( entfilename, true );

	if( ft2 !=(unsigned long) -1 )
	{
		if( ft1 > ft2 )
		{
			MsgDev( D_INFO, "^1Entity patch %s is older than BSP. Ignored.\n", entfilename );
		}
		else if(( ents = (char *)FS_LoadFile( entfilename, NULL, true )) != NULL )
		{
			MsgDev( D_INFO, "^2Read entity patch:^7 %s\n", entfilename );
			return ents;
		}
	}

	// use internal entities
	return sv.worldmodel->entities;
}

/*
================
SV_CreateBaseline

Entity baselines are used to compress the update messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
void SV_CreateBaseline( void )
{
	edict_t	*pEdict;
	int	e;	

	for( e = 0; e < svgame.numEntities; e++ )
	{
		pEdict = EDICT_NUM( e );
		if( !SV_IsValidEdict( pEdict )) continue;
		SV_BaselineForEntity( pEdict );
	}

	// create the instanced baselines
	svgame.dllFuncs.pfnCreateInstancedBaselines();
}

/*
================
SV_FreeOldEntities

remove immediate entities
================
*/
void SV_FreeOldEntities( void )
{
	edict_t	*ent;
	int	i;

	// at end of frame kill all entities which supposed to it 
	for( i = svgame.globals->maxClients + 1; i < svgame.numEntities; i++ )
	{
		ent = EDICT_NUM( i );
		if( ent->free ) continue;

		if( ent->v.flags & FL_KILLME )
			SV_FreeEdict( ent );
	}

	// decrement svgame.numEntities if the highest number entities died
	for( ; EDICT_NUM( svgame.numEntities - 1 )->free; svgame.numEntities-- );
}

/*
================
SV_ActivateServer

activate server on changed map, run physics
================
*/
void SV_ActivateServer( void )
{
	int	i, numFrames;

	if( !svs.initialized )
		return;

	// custom muzzleflashes
	pfnPrecacheModel( "sprites/muzzleflash.spr" );
	pfnPrecacheModel( "sprites/muzzleflash1.spr" );
	pfnPrecacheModel( "sprites/muzzleflash2.spr" );
	pfnPrecacheModel( "sprites/muzzleflash3.spr" );

	// rocket flare
	pfnPrecacheModel( "sprites/animglow01.spr" );

	// ricochet sprite
	pfnPrecacheModel( "sprites/richo1.spr" );

	// Activate the DLL server code
	svgame.dllFuncs.pfnServerActivate( svgame.edicts, svgame.numEntities, svgame.globals->maxClients );

	SV_SetStringArrayMode( true );

	// create a baseline for more efficient communications
	SV_CreateBaseline();

	// check and count all files that marked by user as unmodified (typically is a player models etc)
	sv.num_consistency_resources = SV_TransferConsistencyInfo();

	// send serverinfo to all connected clients
	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		if( svs.clients[i].state >= cs_connected )
		{
			Netchan_Clear( &svs.clients[i].netchan );
			svs.clients[i].delta_sequence = -1;
		}
	}

	numFrames = (sv.loadgame) ? 1 : 2;
	if( !sv.loadgame || svgame.globals->changelevel )
		host.frametime = 0.1f;			

	// GoldSrc rules
	// NOTE: this stuff is breaking sound from func_rotating in multiplayer
	// e.g. ambience\boomer.wav on snark_pit.bsp
	numFrames *= sv_maxclients->integer;

	// run some frames to allow everything to settle
	for( i = 0; i < numFrames; i++ )
	{
		SV_Physics();
	}

	// invoke to refresh all movevars
	Q_memset( &svgame.oldmovevars, 0, sizeof( movevars_t ));
	svgame.globals->changelevel = false; // changelevel ends here

	// setup hostflags
	sv.hostflags = 0;

	// tell what kind of server has been started.
	if( svgame.globals->maxClients > 1 )
	{
		MsgDev( D_INFO, "%i player server started\n", svgame.globals->maxClients );
		Cvar_Reset( "clockwindow" );
	}
	else
	{
		// clear the ugly moving delay in singleplayer
		if( !Host_IsDedicated() )
			Cvar_SetFloat( "clockwindow", 0.0f );
		MsgDev( D_INFO, "Game started\n" );
	}
	Log_Printf( "Started map \"%s\" (CRC \"0\")\n", STRING( svgame.globals->mapname ) );

	if( Host_IsDedicated() )
	{
		Mod_FreeUnused ();
	}

	sv.state = ss_active;
	physinfo->modified = true;
	sv.changelevel = false;
	sv.paused = false;

	Host_SetServerState( sv.state );

	if( sv_maxclients->integer > 1 )
	{
		// listenserver is executed on every map change in multiplayer
		if( !Host_IsDedicated() )
		{
			char *plservercfgfile = Cvar_VariableString( "lservercfgfile" );
			if( *plservercfgfile )
				Cbuf_AddText( va( "exec %s\n", plservercfgfile ) );
		}

		if( public_server->integer )
		{
			MsgDev( D_INFO, "Adding your server to master server list\n" );
			Master_Add( );
		}
	}

	// mapchangecfgfile
	{
		char *mapchangecfgfile = Cvar_VariableString( "mapchangecfgfile" );
		if( *mapchangecfgfile )
			Cbuf_AddText( va( "exec %s\n", mapchangecfgfile ) );
	}
}

/*
================
SV_DeactivateServer

deactivate server, free edicts, strings etc
================
*/
void SV_DeactivateServer( void )
{
	int	i;

	if( !svs.initialized || sv.state == ss_dead )
		return;

	svgame.dllFuncs.pfnServerDeactivate();

	sv.state = ss_dead;

	SV_FreeEdicts ();

	SV_ClearPhysEnts ();

	SV_EmptyStringPool();

	if( sv_maxclients->integer > 32 )
		Cvar_SetFloat( "maxplayers", 32.0f );

	for( i = 0; i < svgame.globals->maxClients; i++ )
	{
		// release client frames
		if( svs.clients[i].frames )
			Mem_Free( svs.clients[i].frames );
		svs.clients[i].frames = NULL;
	}

	svgame.globals->maxEntities = GI->max_edicts;
	svgame.globals->maxClients = sv_maxclients->integer;
	svgame.numEntities = svgame.globals->maxClients + 1; // clients + world
	svgame.globals->startspot = 0;
	svgame.globals->mapname = 0;
}

/*
================
SV_LevelInit

Spawn all entities
================
*/
void SV_LevelInit( const char *pMapName, char const *pOldLevel, char const *pLandmarkName, qboolean loadGame )
{
	if( !svs.initialized )
		return;

	// reset string pool on 64bit systems
	SV_SetStringArrayMode( false );

	if( loadGame )
	{
		if( !SV_LoadGameState( pMapName, 1 ))
		{
			SV_SpawnEntities( pMapName, SV_EntityScript( ));
		}

		if( pOldLevel )
		{
			SV_LoadAdjacentEnts( pOldLevel, pLandmarkName );
		}

		if( sv_newunit->integer )
		{
			SV_ClearSaveDir();
		}
	}
	else
	{
		svgame.dllFuncs.pfnResetGlobalState();
		SV_SpawnEntities( pMapName, SV_EntityScript( ));
		svgame.globals->frametime = 0.0f;

		if( sv_newunit->integer )
		{
			SV_ClearSaveDir();
		}
	}

	sv.resourcelistcache = false;
	sv.reslist.rescount = 0;

	// always clearing newunit variable
	Cvar_SetFloat( "sv_newunit", 0 );

	// relese all intermediate entities
	SV_FreeOldEntities ();
}

/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
================
*/
qboolean SV_SpawnServer( const char *mapname, const char *startspot )
{
	int	i, current_skill;
	qboolean	loadgame, paused;
	qboolean	background, changelevel;

	// save state
	loadgame = sv.loadgame;
	background = sv.background;
	changelevel = sv.changelevel;
	paused = sv.paused;

	if( sv.state == ss_dead )
		SV_InitGame(); // the game is just starting
	else if( !sv_maxclients->modified )
		Cmd_ExecuteString( "latch\n", src_command );
	else MsgDev( D_ERROR, "SV_SpawnServer: while 'maxplayers' was modified.\n" );

	sv_maxclients->modified = false;
	deathmatch->modified = false;
	teamplay->modified = false;
	coop->modified = false;

	if( !svs.initialized )
		return false;

	svgame.globals->changelevel = false; // will be restored later if needed
	svs.timestart = Sys_DoubleTime();
	svs.spawncount++; // any partially connected client will be restarted

	if( startspot )
	{
		MsgDev( D_INFO, "Spawn Server: %s [%s]\n", mapname, startspot );
	}
	else
	{
		MsgDev( D_INFO, "Spawn Server: %s\n", mapname );
	}

	Log_Open();
	Log_Printf( "Loading map \"%s\"\n", mapname );
	Log_PrintServerVars();

	sv.state = ss_dead;
	Host_SetServerState( sv.state );
	Q_memset( &sv, 0, sizeof( sv ));	// wipe the entire per-level structure

	// restore state
	sv.paused = paused;
	sv.loadgame = loadgame;
	sv.background = background;
	sv.changelevel = changelevel;
	sv.time = 1.0f;			// server spawn time it's always 1.0 second
	svgame.globals->time = sv.time;
	
	// initialize buffers
	BF_Init( &sv.datagram, "Datagram", sv.datagram_buf, sizeof( sv.datagram_buf ));
	BF_Init( &sv.reliable_datagram, "Datagram R", sv.reliable_datagram_buf, sizeof( sv.reliable_datagram_buf ));
	BF_Init( &sv.multicast, "Multicast", sv.multicast_buf, sizeof( sv.multicast_buf ));
	BF_Init( &sv.signon, "Signon", sv.signon_buf, sizeof( sv.signon_buf ));
	BF_Init( &sv.spectator_datagram, "Spectator Datagram", sv.spectator_buf, sizeof( sv.spectator_buf ));

	// leave slots at start for clients only
	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		// needs to reconnect
		if( svs.clients[i].state > cs_connected )
			svs.clients[i].state = cs_connected;
	}

	// make cvars consistant
	if( Cvar_VariableInteger( "coop" )) Cvar_SetFloat( "deathmatch", 0 );
	current_skill = (int)(Cvar_VariableValue( "skill" ) + 0.5f);
	current_skill = bound( 0, current_skill, 3 );

	Cvar_SetFloat( "skill", (float)current_skill );

	if( sv.background )
	{
		// tell the game parts about background state
		Cvar_FullSet( "sv_background", "1", CVAR_READ_ONLY );
		Cvar_FullSet( "cl_background", "1", CVAR_READ_ONLY );
	}
	else
	{
		Cvar_FullSet( "sv_background", "0", CVAR_READ_ONLY );
		Cvar_FullSet( "cl_background", "0", CVAR_READ_ONLY );
	}

	// make sure what server name doesn't contain path and extension
	FS_MapFileBase( mapname, sv.name );


	if( startspot )
		Q_strncpy( sv.startspot, startspot, sizeof( sv.startspot ));
	else sv.startspot[0] = '\0';

	Q_snprintf( sv.model_precache[1], sizeof( sv.model_precache[0] ), "maps/%s.bsp", sv.name );
	Mod_LoadWorld( sv.model_precache[1], &sv.checksum, sv_maxclients->integer > 1 );
	sv.worldmodel = Mod_Handle( 1 ); // get world pointer

	Sequence_OnLevelLoad( sv.name );

	for( i = 1; i < sv.worldmodel->numsubmodels; i++ )
	{
		Q_sprintf( sv.model_precache[i+1], "*%i", i );
		Mod_RegisterModel( sv.model_precache[i+1], i+1 );
	}

	// precache and static commands can be issued during map initialization
	sv.state = ss_loading;

	Host_SetServerState( sv.state );

	// clear physics interaction links
	SV_ClearWorld();

	// unused in GoldSrc
#if 0
	// tell dlls about new level started
	svgame.dllFuncs.pfnParmsNewLevel();
#endif
	return true;
}

/*
==============
SV_InitGame

A brand new game has been started
==============
*/
void SV_InitGame( void )
{
	edict_t	*ent;
	int	i;
	
	if( svs.initialized )
	{
		// cause any connected clients to reconnect
		Q_strncpy( host.finalmsg, "Server restarted", MAX_STRING );
		SV_Shutdown( true );
	}
	else
	{
		// init game after host error
		if( !svgame.hInstance )
		{
			Com_ResetLibraryError();
			if( !SV_LoadProgs( SI.gamedll ))
			{
				if( CL_IsInMenu() )
					Sys_Warn( "SV_InitGame: can't initialize \"%s\":\n%s", SI.gamedll, Com_GetLibraryError() );
				else
					Msg( "SV_InitGame: can't initialize \"%s\":\n%s", SI.gamedll, Com_GetLibraryError() );
				return; // can't load
			}
			MsgDev( D_INFO, "Server loaded\n" );
		}

		// make sure the client is down
		CL_Drop();

		Delta_Init ();

		// register custom encoders
		svgame.dllFuncs.pfnRegisterEncoders();

	}

	// now apply latched commands
	Cmd_ExecuteString( "latch\n", src_command );

	if( Cvar_VariableValue( "coop" ) && Cvar_VariableValue ( "deathmatch" ) && Cvar_VariableValue( "teamplay" ))
	{
		MsgDev( D_WARN, "Deathmatch, Teamplay and Coop set, defaulting to Deathmatch\n");
		Cvar_FullSet( "coop", "0",  CVAR_LATCH );
		Cvar_FullSet( "teamplay", "0", CVAR_LATCH );
	}

	// dedicated servers are can't be single player and are usually DM
	// so unless they explicity set coop, force it to deathmatch
	if( Host_IsDedicated() )
	{
		if( !Cvar_VariableValue( "coop" ) && !Cvar_VariableValue( "teamplay" ))
			Cvar_FullSet( "deathmatch", "1",  CVAR_LATCH );
	}

	// init clients
	if( Cvar_VariableValue( "deathmatch" ) || Cvar_VariableValue( "teamplay" ))
	{
		if( sv_maxclients->integer <= 1 )
			Cvar_FullSet( "maxplayers", "8", CVAR_LATCH );
		else if( sv_maxclients->integer > MAX_CLIENTS )
			Cvar_FullSet( "maxplayers", "32", CVAR_LATCH );
	}
	else if( Cvar_VariableValue( "coop" ))
	{
		if( sv_maxclients->integer < 1 )
			Cvar_FullSet( "maxplayers", "4", CVAR_LATCH );
	}
	else	
	{
		// non-deathmatch, non-coop is one player
		Cvar_FullSet( "maxplayers", "1", CVAR_LATCH );
	}

	svgame.globals->maxClients = sv_maxclients->integer;
	SV_UPDATE_BACKUP = ( svgame.globals->maxClients == 1 && !Host_IsDedicated() ) ? SINGLEPLAYER_BACKUP : MULTIPLAYER_BACKUP;

	svs.clients = Z_Malloc( sizeof( sv_client_t ) * sv_maxclients->integer );
	svs.num_client_entities = sv_maxclients->integer * SV_UPDATE_BACKUP * 64;
	svs.packet_entities = Z_Malloc( sizeof( entity_state_t ) * svs.num_client_entities );
	svs.baselines = Z_Malloc( sizeof( entity_state_t ) * GI->max_edicts );

	// client frames will be allocated in SV_DirectConnect

	// init network stuff
	NET_Config( ( sv_maxclients->integer > 1 ), true );

	// copy gamemode into svgame.globals
	svgame.globals->deathmatch = Cvar_VariableInteger( "deathmatch" );
	svgame.globals->teamplay = Cvar_VariableInteger( "teamplay" );
	svgame.globals->coop = ( sv_maxclients->integer > 1 ) ? Cvar_VariableInteger( "coop" ):0;

	// heartbeats will always be sent to the id master
	svs.last_heartbeat = MAX_HEARTBEAT; // send immediately

	// set client fields on player ents
	for( i = 0; i < svgame.globals->maxClients; i++ )
	{
		// setup all the clients
		ent = EDICT_NUM( i + 1 );
		SV_InitEdict( ent );
		svs.clients[i].edict = ent;
	}

	// get actual movevars
	SV_UpdateMovevars( true );

	svgame.numEntities = svgame.globals->maxClients + 1; // clients + world
	svs.initialized = true;
}

qboolean SV_Active( void )
{
	return svs.initialized;
}

void SV_ForceError( void )
{
	// this is only for singleplayer testing
	if( sv_maxclients->integer != 1 ) return;
	sv.write_bad_message = true;
}

void SV_InitGameProgs( void )
{
	if( svgame.hInstance ) return; // already loaded

	// just try to initialize
	SV_LoadProgs( SI.gamedll );
	Com_ResetLibraryError();
}

void SV_FreeGameProgs( void )
{
	if( svs.initialized ) return;	// server is active

	// unload progs (free cvars and commands)
	SV_UnloadProgs();
}

qboolean SV_NewGame( const char *mapName, qboolean loadGame )
{
	if( !loadGame )
	{
		if( !SV_MapIsValid( mapName, GI->sp_entity, NULL )) {
			return false;
		}
		SV_ClearSaveDir ();
		SV_Shutdown( true );
	}
	else
	{
		S_StopAllSounds ();
		SV_DeactivateServer ();
	}

	if( host_xashds_hacks->integer )
	{
		Cbuf_InsertText(va("wait;rcon map %s\n", mapName));
		Cbuf_AddText("wait;connect 127.0.0.1\n");
		return true;
	}


	sv.loadgame = loadGame;
	sv.background = false;
	sv.changelevel = false;

	if( !SV_SpawnServer( mapName, NULL ))
		return false;

	SV_LevelInit( mapName, NULL, NULL, loadGame );
	sv.loadgame = loadGame;

	SV_ActivateServer();

	if( sv.state != ss_active )
		return false;

	return true;
}
