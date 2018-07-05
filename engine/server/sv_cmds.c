/*
sv_cmds.c - server console commands
Copyright (C) 2007 Uncle Mike

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

static qboolean startingdefmap;

/*
=================
SV_ClientPrintf

Sends text across to be displayed if the level passes
=================
*/
void SV_ClientPrintf( sv_client_t *cl, int level, char *fmt, ... )
{
	va_list	argptr;
	char	string[MAX_SYSPATH];

	if( level < cl->messagelevel || cl->fakeclient )
		return;
	
	va_start( argptr, fmt );
	Q_vsnprintf( string, sizeof(string), fmt, argptr );
	va_end( argptr );
	
	BF_WriteByte( &cl->netchan.message, svc_print );
	BF_WriteByte( &cl->netchan.message, level );
	BF_WriteString( &cl->netchan.message, string );
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf( int level, char *fmt, ... )
{
	char		string[MAX_SYSPATH];
	va_list		argptr;
	sv_client_t	*cl;
	int		i;

	if( !sv.state ) return;

	va_start( argptr, fmt );
	Q_vsnprintf( string, sizeof( string ), fmt, argptr );
	va_end( argptr );
	
	// echo to console
	if( Host_IsDedicated() ) Msg( "%s", string );

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( level < cl->messagelevel ) continue;
		if( cl->state != cs_spawned ) continue;
		if( cl->fakeclient ) continue;

		BF_WriteByte( &cl->netchan.message, svc_print );
		BF_WriteByte( &cl->netchan.message, level );
		BF_WriteString( &cl->netchan.message, string );
	}
}

/*
=================
SV_BroadcastCommand

Sends text to all active clients
=================
*/
void SV_BroadcastCommand( char *fmt, ... )
{
	va_list	argptr;
	char	string[MAX_SYSPATH];
	
	if( !sv.state ) return;
	va_start( argptr, fmt );
	Q_vsnprintf( string, sizeof( string ), fmt, argptr );
	va_end( argptr );

	BF_WriteByte( &sv.reliable_datagram, svc_stufftext );
	BF_WriteString( &sv.reliable_datagram, string );
}

/*
==================
SV_SetPlayer

Sets sv_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
qboolean SV_SetPlayer( void )
{
	char		*s;
	sv_client_t	*cl;
	int		i, idnum;

	if( !svs.clients || sv.background )
	{
		Msg( "^3No server running.\n" );
		return false;
	}

	if( sv_maxclients->integer == 1 || Cmd_Argc() < 2 )
	{
		// special case for local client
		svs.currentPlayer = svs.clients;
		svs.currentPlayerNum = 0;
		return true;
	}

	s = Cmd_Argv( 1 );

	// numeric values are just slot numbers
	if( Q_isdigit( s ) || (s[0] == '-' && Q_isdigit( s + 1 )))
	{
		idnum = Q_atoi( s );
		if( idnum < 0 || idnum >= sv_maxclients->integer )
		{
			Msg( "Bad client slot: %i\n", idnum );
			return false;
		}

		svs.currentPlayer = &svs.clients[idnum];
		svs.currentPlayerNum = idnum;

		if( !svs.currentPlayer->state )
		{
			Msg( "Client %i is not active\n", idnum );
			return false;
		}
		return true;
	}

	// check for a name match
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( !cl->state ) continue;
		if( !Q_strcmp( cl->name, s ))
		{
			svs.currentPlayer = cl;
			svs.currentPlayerNum = (cl - svs.clients);
			return true;
		}
	}

	Msg( "Userid %s is not on the server\n", s );
	svs.currentPlayer = NULL;
	svs.currentPlayerNum = 0;

	return false;
}

/*
==================
SV_Map_f

Goes directly to a given map without any savegame archiving.
For development work
==================
*/
void SV_Map_f( void )
{
	char	*spawn_entity;
	string	mapname;
	int	flags;
	static qboolean mapinit;

	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: map <mapname>\n" );
		return;
	}

	if( host_xashds_hacks->integer )
	{
		CL_Disconnect();
		Cbuf_InsertText(va("wait;rcon map %s\n",Cmd_Argv( 1 )));
		Cbuf_AddText("wait;connect 127.0.0.1\n");
		return;
	}

	if( mapinit )
	{
		Host_Error("Wrong usage of \"map\" in config files\n");
		return;
	}

	// hold mapname to other place
	Q_strncpy( mapname, Cmd_Argv( 1 ), sizeof( mapname ));
	
	// determine spawn entity classname
	if( sv_maxclients->integer == 1 )
		spawn_entity = GI->sp_entity;
	else spawn_entity = GI->mp_entity;

	if( Host_IsDedicated() && !startingdefmap )
	{
		// apply servercfgfile cvar on first dedicated server run
		if( !host.stuffcmdsrun )
			Cbuf_Execute();

		// dedicated servers are using settings from server.cfg file
		Cbuf_AddText( va( "exec %s\n", Cvar_VariableString( "servercfgfile" )));
	}
	else
		startingdefmap = false;

	mapinit = true;
	// make sure that all configs are executed
	Cbuf_Execute();
	mapinit = false;

	flags = SV_MapIsValid( mapname, spawn_entity, NULL );

	if( flags & MAP_INVALID_VERSION )
	{
		if( CL_IsInMenu() )
			Sys_Warn( "SV_NewMap: map %s is invalid or not supported\n", mapname );
		else
			Msg( "SV_NewMap: map %s is invalid or not supported\n", mapname );
		return;
	}
	
	if(!( flags & MAP_IS_EXIST ))
	{
		if( CL_IsInMenu() )
			Sys_Warn( "SV_NewMap: map %s doesn't exist\n", mapname );
		else
			Msg( "SV_NewMap: map %s doesn't exist\n", mapname );
		return;
	}

	if(!( flags & MAP_HAS_SPAWNPOINT ))
	{
		if( CL_IsInMenu() )
			Sys_Warn( "SV_NewMap: map %s doesn't have a valid spawnpoint\n", mapname );
		else
			Msg( "SV_NewMap: map %s doesn't have a valid spawnpoint\n", mapname );
		return;
	}

	// init network stuff
	NET_Config( ( sv_maxclients->integer > 1 ), true );

	// changing singleplayer to multiplayer or back. refresh the player count
	if(( sv_maxclients->modified ) || ( deathmatch->modified ) || ( coop->modified ) || ( teamplay->modified ))
		Host_ShutdownServer();

	SCR_BeginLoadingPlaque( false );

	sv.changelevel = false;
	sv.background = false;
	sv.loadgame = false; // set right state
	SV_ClearSaveDir ();	// delete all temporary *.hl files

	SV_DeactivateServer();
	if( !SV_SpawnServer( mapname, NULL ) )
	{
		Msg("Could not spawn server!\n");
		return;
	}
	SV_LevelInit( mapname, NULL, NULL, false );
	SV_ActivateServer ();
}

/*
==================

SV_Maps_f

Lists maps according to given substring.

TODO: Make it more convenient. (Timestamp check, temporary file, ...)

==================
*/
void SV_Maps_f(void)
{
	char mapName[256], *seperator = "-------------------";
	char *argStr = Cmd_Argv(1); //Substr
	int listIndex;
	search_t *mapList;

	if (Cmd_Argc() != 2)
	{
		Msg("Usage:  maps <substring>\nmaps * for full listing\n");
		return;
	}

        mapList = FS_Search(va("maps/*%s*.bsp", argStr), false, true);
	if (!mapList)
	{
		Msg("No related map found in \"%s/maps\"\n", GI->gamefolder);
		return;
	}
	Msg("%s\n", seperator);
	for (listIndex = 0; listIndex != mapList->numfilenames; ++listIndex)
	{
		const char *ext;
		Q_strncpy(mapName, mapList->filenames[listIndex], sizeof(mapName) - 1);
		ext = FS_FileExtension(mapName);
		if (Q_strcmp(ext, "bsp")) continue;
		if ( (Q_strcmp(argStr, "*") == 0) || (Q_stristr(mapName, argStr) != NULL) )
		{
			Msg("%s\n", &mapName[5]); //Do not show "maps/"
		}
	}
	Msg("%s\nDirectory: \"%s/maps\" - Maps listed: %d\n", seperator, GI->basedir, mapList->numfilenames);
}

/*
==================
SV_MapBackground_f

Set background map (enable physics in menu)
==================
*/
void SV_MapBackground_f( void )
{
	string	mapname;
	int	flags;

	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: map_background <mapname>\n" );
		return;
	}

	if( sv.state == ss_active && !sv.background )
	{
		Msg( "SV_NewMap: can't set background map while game is active\n" );
		return;
	}

	// hold mapname to other place
	Q_strncpy( mapname, Cmd_Argv( 1 ), sizeof( mapname ));
	flags = SV_MapIsValid( mapname, GI->sp_entity, NULL );

	if(!( flags & MAP_IS_EXIST ))
	{
		Msg( "SV_NewMap: map %s doesn't exist\n", mapname );
		return;
	}

	// background maps allow without spawnpoints (just throw warning)
	if(!( flags & MAP_HAS_SPAWNPOINT ))
		MsgDev( D_WARN, "SV_NewMap: map %s doesn't have a valid spawnpoint\n", mapname );
		
	Q_strncpy( host.finalmsg, "", MAX_STRING );
	SV_Shutdown( true );
	NET_Config ( false, false ); // close network sockets

	sv.changelevel = false;
	sv.background = true;
	sv.loadgame = false; // set right state

	// reset all multiplayer cvars
	Cvar_FullSet( "coop", "0",  CVAR_LATCH );
	Cvar_FullSet( "teamplay", "0",  CVAR_LATCH );
	Cvar_FullSet( "deathmatch", "0",  CVAR_LATCH );
	Cvar_FullSet( "maxplayers", "1", CVAR_LATCH );

	SCR_BeginLoadingPlaque( true );

	SV_SpawnServer( mapname, NULL );
	SV_LevelInit( mapname, NULL, NULL, false );
	SV_ActivateServer ();
}

/*
==============
SV_NewGame_f

==============
*/
void SV_NewGame_f( void )
{
	if( Cmd_Argc() != 1 )
	{
		Msg( "Usage: newgame\n" );
		return;
	}

	Host_NewGame( GI->startmap, false );
}

/*
==============
SV_StartDefaultMap_f

==============
*/
void SV_StartDefaultMap_f( void )
{
	char *defaultmap;

	if( Cmd_Argc() != 1 )
	{
		Msg( "Usage: startdefaultmap\n" );
		return;
	}

	// apply servercfgfile cvar on first dedicated server run
	if( !host.stuffcmdsrun )
		Cbuf_Execute();

	// get defaultmap cvar
	Cbuf_AddText( va( "exec %s\n", Cvar_VariableString( "servercfgfile" )));
	Cbuf_Execute();

	defaultmap = Cvar_VariableString( "defaultmap" );
	if( !defaultmap[0] )
		Msg( "Please add \"defaultmap\" cvar with default map name to your server.cfg!\n" );
	else
		Cbuf_AddText( va( "map %s\n", defaultmap ));
	startingdefmap = true;
}

/*
==============
SV_HazardCourse_f

==============
*/
void SV_HazardCourse_f( void )
{
	if( Cmd_Argc() != 1 )
	{
		Msg( "Usage: hazardcourse\n" );
		return;
	}

	Host_NewGame( GI->trainmap, false );
}

/*
==============
SV_EndGame_f

==============
*/
void SV_EndGame_f( void )
{
	Host_EndGame( Cmd_Argv( 1 ));
}

/*
==============
SV_KillGame_f

==============
*/
void SV_KillGame_f( void )
{
	Host_EndGame( "The End" );
}

/*
==============
SV_Load_f

==============
*/
void SV_Load_f( void )
{
	string arg;

	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: load <savename>\n" );
		return;
	}

	Q_strncpy( arg, Cmd_Argv( 1 ), sizeof(arg) );

	if( host_xashds_hacks->integer )
	{
		Cbuf_InsertText( va ( "rcon load %s\n", arg ) );
		Cbuf_AddText( "connect 127.0.0.1\n" );
		return;
	}

	if( Host_IsDedicated() )
	{
		SV_InactivateClients();
		SV_DeactivateServer();
	}

	SV_LoadGame( arg );

	if( Host_IsDedicated() )
		SV_ActivateServer();
}

/*
==============
SV_QuickLoad_f

==============
*/
void SV_QuickLoad_f( void )
{
	Cbuf_AddText( "echo Quick Loading...; wait; load quick" );
}

/*
==============
SV_Save_f

==============
*/
void SV_Save_f( void )
{
	const char *name;

	switch( Cmd_Argc() )
	{
	case 1: name = "new"; break;
	case 2: name = Cmd_Argv( 1 ); break;
	default:
		Msg( "Usage: save <savename>\n" );
		return;
	}

	if( host_xashds_hacks->integer )
	{
		Cbuf_InsertText(va("rcon save %s\n", Cmd_Argv( 1 )));
		return;
	}

	SV_SaveGame( name );
}

/*
==============
SV_QuickSave_f

==============
*/
void SV_QuickSave_f( void )
{
	Cbuf_AddText( "echo Quick Saving...; wait; save quick" );
}

/*
==============
SV_DeleteSave_f

==============
*/
void SV_DeleteSave_f( void )
{
	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: killsave <name>\n" );
		return;
	}

	// delete save and saveshot
	FS_Delete( va( "save/%s.sav", Cmd_Argv( 1 )));
	FS_Delete( va( "save/%s.bmp", Cmd_Argv( 1 )));
}

/*
==============
SV_AutoSave_f

==============
*/
void SV_AutoSave_f( void )
{
	if( Cmd_Argc() != 1 )
	{
		Msg( "Usage: autosave\n" );
		return;
	}

	SV_SaveGame( "autosave" );
}

/*
==================
SV_ChangeLevel_f

Saves the state of the map just being exited and goes to a new map.
==================
*/
void SV_ChangeLevel_f( void )
{
	char	*spawn_entity, *mapname;
	int	flags, c = Cmd_Argc();

	if( c < 2 )
	{
		Msg( "Usage: changelevel <map> [landmark]\n" );
		return;
	}

	if( host_xashds_hacks->integer )
	{
		Cbuf_InsertText(va("rcon changelevel %s %s\n",Cmd_Argv( 1 ), Cmd_Argv( 2 )));
		return;
	}

	mapname = Cmd_Argv( 1 );

	// determine spawn entity classname
	if( sv_maxclients->integer == 1 )
		spawn_entity = GI->sp_entity;
	else spawn_entity = GI->mp_entity;

	flags = SV_MapIsValid( mapname, spawn_entity, Cmd_Argv( 2 ));

	if( flags & MAP_INVALID_VERSION )
	{
		Msg( "SV_ChangeLevel: Map %s is invalid or not supported\n", mapname );
		return;
	}
	
	if(!( flags & MAP_IS_EXIST ))
	{
		Msg( "SV_ChangeLevel: Map %s doesn't exist\n", mapname );
		return;
	}

	if( c >= 3 && !( flags & MAP_HAS_LANDMARK ))
	{
		if( sv_validate_changelevel->integer )
		{
			// NOTE: we find valid map but specified landmark it's doesn't exist
			// run simple changelevel like in q1, throw warning
			MsgDev( D_INFO, "SV_ChangeLevel: map %s exists but doesn't contain\n", mapname );
			MsgDev( D_INFO, "landmark with name %s. Run classic Quake changelevel.\n", Cmd_Argv( 2 ));
			c = 2; // reduce args
		}
	}

	if( c >= 3 && !Q_stricmp( sv.name, Cmd_Argv( 1 )))
	{
		MsgDev( D_INFO, "SV_ChangeLevel: Can't changelevel with same map. Ignored.\n" );
		return;	
	}

	if( c == 2 && !( flags & MAP_HAS_SPAWNPOINT ))
	{
		if( sv_validate_changelevel->integer )
		{
			MsgDev( D_INFO, "SV_ChangeLevel: Map %s doesn't have a valid spawnpoint. Ignored.\n", mapname );
			return;	
		}
	}

	// bad changelevel position invoke enables in one-way transtion
	if( sv.net_framenum < 30 )
	{
		if( sv_validate_changelevel->integer && !Host_IsDedicated() )
		{
			MsgDev( D_INFO, "SV_ChangeLevel: An infinite changelevel detected.\n" );
			MsgDev( D_INFO, "Changelevel will be disabled until the next save\\restore.\n" );
			return; // lock with svs.spawncount here
		}
	}

	if( sv.state != ss_active )
	{
		MsgDev( D_INFO, "Only the server may changelevel\n" );
		return;
	}

	SCR_BeginLoadingPlaque( false );

	if( sv.background )
	{
		// just load map
		Cbuf_AddText( va( "map %s\n", mapname ));
		return;
	}

	if( c == 2 ) SV_ChangeLevel( false, Cmd_Argv( 1 ), NULL );
	else SV_ChangeLevel( true, Cmd_Argv( 1 ), Cmd_Argv( 2 ));
}

/*
==================
SV_ChangeLevel2_f

Saves the state of the map just being exited and goes to a new map. Force Half-Life style changelevel.
==================
*/

void SV_ChangeLevel2_f( void )
{
	char	*spawn_entity, *mapname;
	int	flags, c = Cmd_Argc();

	if( c < 2 )
	{
		Msg( "Usage: changelevel2 <map> [landmark]\n" );
		return;
	}

	if( host_xashds_hacks->integer )
	{
		Cbuf_InsertText(va("rcon changelevel2 %s %s\n",Cmd_Argv( 1 ), Cmd_Argv( 2 )));
		return;
	}

	mapname = Cmd_Argv( 1 );

	// determine spawn entity classname
	if( sv_maxclients->integer == 1 )
		spawn_entity = GI->sp_entity;
	else spawn_entity = GI->mp_entity;

	flags = SV_MapIsValid( mapname, spawn_entity, Cmd_Argv( 2 ));

	if( flags & MAP_INVALID_VERSION )
	{
		Msg( "SV_ChangeLevel: Map %s is invalid or not supported.\n", mapname );
		return;
	}
	
	if(!( flags & MAP_IS_EXIST ))
	{
		Msg( "SV_ChangeLevel: Map %s doesn't exist.\n", mapname );
		return;
	}

	if( c >= 3 && !Q_stricmp( sv.name, Cmd_Argv( 1 )))
	{
		MsgDev( D_INFO, "SV_ChangeLevel: Can't changelevel with same map. Ignored.\n" );
		return;	
	}

	// bad changelevel position invoke enables in one-way transtion
	if( sv.net_framenum < 30 )
	{
		if( sv_validate_changelevel->integer )
		{
			MsgDev( D_INFO, "SV_ChangeLevel: An infinite changelevel detected.\n" );
			MsgDev( D_INFO, "Changelevel will be disabled until the next save\\restore.\n" );
			return; // lock with svs.spawncount here
		}
	}

	/*if( sv.state != ss_active )
	{
		MsgDev( D_INFO, "Only the server may changelevel\n" );
		return;
	}*/

	SCR_BeginLoadingPlaque( false );

	SV_ChangeLevel( true, Cmd_Argv( 1 ), Cmd_Argv( 2 ));
}


/*
==================
SV_Restart_f

restarts current level
==================
*/
void SV_Restart_f( void )
{
	if( sv.state != ss_active )
		return;

	// just sending console command
	if( sv.background )
	{
		Cbuf_AddText( va( "map_background %s\n", sv.name ));
	}
	else
	{
		Cbuf_AddText( va( "map %s\n", sv.name ));
	}
}

/*
==================
SV_Reload_f

continue from latest savedgame
==================
*/
void SV_Reload_f( void )
{
	const char	*save;
	string		loadname;
	
	if( sv.state != ss_active || sv.background )
		return;

	save = SV_GetLatestSave();

	if( save )
	{
		FS_FileBase( save, loadname );
		Cbuf_AddText( va( "load %s\n", loadname ));
	}
	else Cbuf_AddText( "newgame\n" ); // begin new game
}

/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
void SV_Kick_f( void )
{
	sv_client_t *cl;
	const char *param, *clientId;
	char name[32];
	int userid;
	netadr_t adr;

	if( !SV_Active() )
	{
		Msg( "Can't kick when not running local server.");
		return;
	}

	if( Cmd_Argc() < 2 )
	{
		Msg( "Usage: kick <#id|name> [reason]\n" );
		return;
	}

	param = Cmd_Argv( 1 );

	if( *param == '#' && Q_isdigit( param + 1 ) )
		cl = SV_ClientById( Q_atoi( param + 1 ) );
	else cl = SV_ClientByName( param );

	if( !cl )
	{
		Msg( "Client is not on the server\n" );
		return;
	}

	if( NET_IsLocalAddress( cl->netchan.remote_address ))
	{
		Msg( "The local player cannot be kicked!\n" );
		return;
	}

	param = Cmd_Argv( 2 );

	if( *param )
		SV_ClientPrintf( cl, PRINT_HIGH, "You were kicked from the game with message: \"%s\"\n", param );
	else
		SV_ClientPrintf( cl, PRINT_HIGH, "You were kicked from the game\n" );

	Q_strcpy( name, cl->name );
	Q_memcpy( &adr, &cl->netchan.remote_address, sizeof( adr ) );
	userid = cl->userid;
	clientId = SV_GetClientIDString( cl );

	SV_DropClient( cl );

	if ( *param )
	{
		SV_BroadcastPrintf( PRINT_HIGH, "%s was kicked with message: \"%s\"\n", name, param );
		Log_Printf( "Kick: \"%s<%i><%s><>\" was kicked by \"Console\" (message \"%s\")\n", name, userid, clientId, param );
	}
	else
	{
		SV_BroadcastPrintf( PRINT_HIGH, "%s was kicked\n", name );
		Log_Printf( "Kick: \"%s<%i><%s><>\" was kicked by \"Console\"\n", name, userid, clientId );
	}
	if( cl->useragent[0] )
	{
		if( *param )
			Netchan_OutOfBandPrint( NS_SERVER, adr, "errormsg\nKicked with message:\n%s\n", param );
		else
			Netchan_OutOfBandPrint( NS_SERVER, adr, "errormsg\nYou were kicked from the game\n" );
	}

	// min case there is a funny zombie
	cl->lastmessage = host.realtime;
}

/*
==================
SV_EntPatch_f
==================
*/
void SV_EntPatch_f( void )
{
	const char	*mapname;

	if( Cmd_Argc() < 2 )
	{
		if( sv.state != ss_dead )
		{
			mapname = sv.name;
		}
		else
		{
			Msg( "Usage: entpatch <mapname>\n" );
			return;
		}
	}
	else mapname = Cmd_Argv( 1 );

	SV_WriteEntityPatch( mapname );
}

/*
================
SV_Status_f
================
*/
void SV_Status_f( void )
{
	int		i;
	sv_client_t	*cl;

	if( !svs.clients || sv.background )
	{
		Msg( "^3No server running.\n" );
		return;
	}

	Msg( "map: %s\n", sv.name );
	Msg( "num score ping    name                             lastmsg   address               port  \n" );
	Msg( "--- ----- ------- -------------------------------- --------- --------------------- ------\n" );

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		int	j, l, ping;
		char	*s;

		if( !cl->state ) continue;

		Msg( "%3i ", cl->userid );
		Msg( "%5i ", (int)cl->edict->v.frags );

		if( cl->state == cs_connected ) Msg( "Connect" );
		else if( cl->state == cs_zombie ) Msg( "Zombie " );
		else if( cl->fakeclient ) Msg( "Bot   " );
		else if( cl->netchan.remote_address.type == NA_LOOPBACK ) Msg( "Local ");
		else
		{
			ping = min( cl->ping, 9999 );
			Msg( "%7i ", ping );
		}

		Msg( "%s", cl->name );
		l = 33 - Q_strlen( cl->name );
		for( j = 0; j < l; j++ ) Msg( " " );
		Msg( "%9g ", ( host.realtime - cl->lastmessage ));
		s = NET_BaseAdrToString( cl->netchan.remote_address );
		Msg( "%s", s );
		l = 22 - Q_strlen( s );
		for( j = 0; j < l; j++ ) Msg( " " );
		Msg( "%5i", cl->netchan.qport );
		Msg( "\n" );
	}
	Msg( "\n" );
}

/*
==================
SV_ConSay_f
==================
*/
void SV_ConSay_f( void )
{
	char		*p, text[MAX_SYSPATH];
	sv_client_t	*client;
	int		i;

	if( Cmd_Argc() < 2 ) return;

	if( !svs.clients || sv.background )
	{
		Msg( "^3No server running.\n" );
		return;
	}

	Q_strncpy( text, "console: ", MAX_SYSPATH );
	p = Cmd_Args();

	if( *p == '"' )
	{
		p++;
		p[Q_strlen(p) - 1] = 0;
	}

	Q_strncat( text, p, MAX_SYSPATH );

	for( i = 0, client = svs.clients; i < sv_maxclients->integer; i++, client++ )
	{
		if( client->state != cs_spawned )
			continue;

		SV_ClientPrintf( client, PRINT_CHAT, "%s\n", text );
	}
}

/*
==================
SV_Heartbeat_f
==================
*/
void SV_Heartbeat_f( void )
{
	svs.last_heartbeat = MAX_HEARTBEAT;
}

/*
===========
SV_ServerInfo_f

Examine serverinfo string
===========
*/
void SV_ServerInfo_f( void )
{
	Msg( "Server info settings:\n" );
	Info_Print( Cvar_Serverinfo( ));
}

void SV_LocalInfo_f( void )
{
	char *value;

	if ( Cmd_Argc( ) > 3 )
	{
		Msg( "Usage: localinfo [ <key> [value] ]\n" );
		return;
	}

	if ( Cmd_Argc( ) == 1 )
	{
		Msg( "Local info settings:\n" );
		Info_Print( localinfo );
		return;
	}
	else if ( Cmd_Argc( ) == 2 )
	{
		value = Info_ValueForKey( localinfo, Cmd_Argv( 1 ) );
		Msg( "%s: %s\n", Cmd_Argv( 1 ), *value ? value : "Key not exists" );
		return;
	}

	if ( Cmd_Argv( 1 )[0] == '*' )
	{
		Msg( "Star variables cannot be changed.\n" );
		return;
	}

	Info_SetValueForKey( localinfo, Cmd_Argv( 1 ), Cmd_Argv( 2 ), MAX_LOCALINFO );
}

/*
===========
SV_ClientInfo_f

Examine all a users info strings
===========
*/
void SV_ClientInfo_f( void )
{
	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: clientinfo <userid>\n" );
		return;
	}

	if( !SV_SetPlayer( )) return;
	Msg( "userinfo\n" );
	Msg( "--------\n" );
	Info_Print( svs.currentPlayer->userinfo );
}

/*
===========
SV_ClientUserAgent_f

Examine useragent strings
===========
*/
void SV_ClientUserAgent_f( void )
{
	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: clientuseragent <userid>\n" );
		return;
	}

	if( !SV_SetPlayer( )) return;
	Msg( "useragent\n" );
	Msg( "---------\n" );
	Info_Print( svs.currentPlayer->useragent );
}

/*
===============
SV_KillServer_f

Kick everyone off, possibly in preparation for a new game
===============
*/
void SV_KillServer_f( void )
{
	if( !svs.initialized ) return;
	Q_strncpy( host.finalmsg, "Server was killed", MAX_STRING );
	SV_Shutdown( false );
	NET_Config ( false, false ); // close network sockets
}

/*
===============
SV_PlayersOnly_f

disable physics, except for players
===============
*/
void SV_PlayersOnly_f( void )
{
	if( !Cvar_VariableInteger( "sv_cheats" )) return;
	sv.hostflags = sv.hostflags ^ SVF_PLAYERSONLY;

	if(!( sv.hostflags & SVF_PLAYERSONLY ))
		SV_BroadcastPrintf( D_INFO, "Resume server physics\n" );
	else SV_BroadcastPrintf( D_INFO, "Freeze server physics\n" );
}

/*
===============
SV_EdictsInfo_f

===============
*/
void SV_EdictsInfo_f( void )
{
	int	active;

	if( sv.state != ss_active )
	{
		Msg( "^3No server running.\n" );
		return;
	}

	active = pfnNumberOfEntities(); 
	Msg( "%5i used edicts\n", active );
	Msg( "%5i free edicts\n", svgame.globals->maxEntities - active );
	Msg( "%5i total\n", svgame.globals->maxEntities );
}

/*
===============
SV_EntityInfo_f

===============
*/
void SV_EntityInfo_f( void )
{
	edict_t	*ent;
	int	i;

	if( sv.state != ss_active )
	{
		Msg( "^3No server running.\n" );
		return;
	}

	for( i = 0; i < svgame.numEntities; i++ )
	{
		ent = EDICT_NUM( i );
		if( !SV_IsValidEdict( ent )) continue;

		Msg( "%5i origin: %.f %.f %.f", i, ent->v.origin[0], ent->v.origin[1], ent->v.origin[2] );

		if( ent->v.classname )
			Msg( ", class: %s", STRING( ent->v.classname ));

		if( ent->v.globalname )
			Msg( ", global: %s", STRING( ent->v.globalname ));

		if( ent->v.targetname )
			Msg( ", name: %s", STRING( ent->v.targetname ));

		if( ent->v.target )
			Msg( ", target: %s", STRING( ent->v.target ));

		if( ent->v.model )
			Msg( ", model: %s", STRING( ent->v.model ));

		Msg( "\n" );
	}
}
/*
===================
SV_SendReconnect_f

Reconnect all clients (useful when adding resources)
===================
*/
void SV_SendReconnect_f( void )
{
	char *message = "Reconnect by console request!\n";

	if( Cmd_Argc() > 1 )
		message = Cmd_Argv( 1 );

	SV_FinalMessage( message, true );
}

void SV_DumpPrecache_f( void )
{
	int index;
	file_t *f = FS_Open( "precache-dump.txt", "w", false );

	if( !f )
	{
		Msg( "Could not write precache-dump.txt\n" );
		return;
	}

	// write models
	for ( index = 1; index < MAX_MODELS && sv.model_precache[index][0]; index++ )
	{
		if ( sv.model_precache[index][0] == '*' ) // internal bmodel
			continue;

		FS_Printf( f, "%s\n", sv.model_precache[index] );
	}

	// write sounds
	for( index = 1; index < MAX_SOUNDS && sv.sound_precache[index][0]; index++ )
		FS_Printf( f, "sound/%s\n", sv.sound_precache[index] );

	// write eventscripts
	for( index = 1; index < MAX_EVENTS && sv.event_precache[index][0]; index++ )
		FS_Printf( f, "%s\n", sv.event_precache[index] );

	// write generic
	for( index = 1; index < MAX_CUSTOM && sv.files_precache[index][0]; index++ )
		FS_Printf( f, "%s\n", sv.files_precache[index] );

	FS_Close( f );
	Msg( "Successfully created precache-dump.txt\n" );
}

void SV_DumpResList_f( void )
{
	int index;
	file_t *f = FS_Open( "reslist-dump.txt", "w", false );

	if( !f )
	{
		Msg( "Could not write reslist-dump.txt\n" );
		return;
	}

	// generate new resource list, if it's not cached
	if ( !sv.resourcelistcache )
		SV_UpdateResourceList();

	for( index = 0; index < sv.reslist.rescount; index++ )
		FS_Printf( f, sv.reslist.restype[index] == t_sound ? "sound/%s\n":"%s\n", sv.reslist.resnames[index] );

	FS_Close( f );
	Msg( "Successfully created precache-dump.txt\n" );

}

/*
================
Rcon_Redirect_f

Force redirect N lines of console output to client
================
*/
void Rcon_Redirect_f( void )
{
	int lines = 2000;

	if( !host.rd.target )
	{
		Msg( "redirect is only valid from rcon\n" );
		return;
	}

	if( Cmd_Argc() == 2 )
		lines = Q_atoi( Cmd_Argv( 1 ) );

	host.rd.lines = lines;
	Msg( "Redirection enabled for next %d lines\n", lines );
}

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands( void )
{
	Cmd_AddCommand( "heartbeat", SV_Heartbeat_f, "send a heartbeat to the master server" );
	Cmd_AddCommand( "kick", SV_Kick_f, "kick a player off the server by number or name" );
	Cmd_AddCommand( "status", SV_Status_f, "print server status information" );
	Cmd_AddCommand( "serverinfo", SV_ServerInfo_f, "print server settings" );
	Cmd_AddCommand( "localinfo", SV_LocalInfo_f, "print local info settings" );
	Cmd_AddCommand( "clientinfo", SV_ClientInfo_f, "print user infostring (player num required)" );
	Cmd_AddCommand( "clientuseragent", SV_ClientUserAgent_f, "print user agent (player num required)" );
	Cmd_AddCommand( "playersonly", SV_PlayersOnly_f, "freezes physics, except for players" );

	Cmd_AddCommand( "map", SV_Map_f, "start new level" );
	Cmd_AddCommand( "maps", SV_Maps_f, "list maps" );
	Cmd_AddCommand( "newgame", SV_NewGame_f, "begin new game" );
	Cmd_AddCommand( "endgame", SV_EndGame_f, "end current game, takes ending message" );
	Cmd_AddCommand( "killgame", SV_KillGame_f, "end current game" );
	Cmd_AddCommand( "hazardcourse", SV_HazardCourse_f, "start a Hazard Course" );
	Cmd_AddCommand( "sendreconnect", SV_SendReconnect_f, "send reconnect message to clients" );
	Cmd_AddCommand( "changelevel", SV_ChangeLevel_f, "change level" );
	Cmd_AddCommand( "changelevel2", SV_ChangeLevel2_f, "change level, in Half-Life style" );
	Cmd_AddCommand( "restart", SV_Restart_f, "restart current level" );
	Cmd_AddCommand( "reload", SV_Reload_f, "continue from latest save or restart level" );
	Cmd_AddCommand( "entpatch", SV_EntPatch_f, "write entity patch to allow external editing" );
	Cmd_AddCommand( "edicts_info", SV_EdictsInfo_f, "show info about edicts" );
	Cmd_AddCommand( "entity_info", SV_EntityInfo_f, "show more info about edicts" );
	Cmd_AddCommand( "save", SV_Save_f, "save the game to a file" );
	Cmd_AddCommand( "load", SV_Load_f, "load a saved game file" );
	Cmd_AddCommand( "savequick", SV_QuickSave_f, "save the game to the quicksave" );
	Cmd_AddCommand( "loadquick", SV_QuickLoad_f, "load a quick-saved game file" );
	Cmd_AddCommand( "killsave", SV_DeleteSave_f, "delete a saved game file and saveshot" );
	Cmd_AddCommand( "autosave", SV_AutoSave_f, "save the game to 'autosave' file" );
	Cmd_AddCommand( "redirect", Rcon_Redirect_f, "force enable rcon redirection" );
	Cmd_AddCommand( "updatereslist", SV_UpdateResourceList, "force update server resource list" );
	Cmd_AddCommand( "dumpreslist", SV_DumpResList_f, "dump resource list to reslist-dump.txt" );
	Cmd_AddCommand( "dumpprecache", SV_DumpPrecache_f, "dump precached resources to precache-dump.txt" );

	if( Host_IsDedicated() )
	{
		Cmd_AddCommand( "say", SV_ConSay_f, "send a chat message to everyone on the server" );
		Cmd_AddCommand( "killserver", SV_KillServer_f, "shutdown current server" );
		Cmd_AddCommand( "startdefaultmap", SV_StartDefaultMap_f, "start default map in dedicated server" );
	}
	else
	{
		Cmd_AddCommand( "map_background", SV_MapBackground_f, "set background map" );
	}
}

/*
==================
SV_KillOperatorCommands
==================
*/
void SV_KillOperatorCommands( void )
{
	Cmd_RemoveCommand( "heartbeat" );
	Cmd_RemoveCommand( "kick" );
	Cmd_RemoveCommand( "status" );
	Cmd_RemoveCommand( "serverinfo" );
	Cmd_RemoveCommand( "clientinfo" );
	Cmd_RemoveCommand( "playersonly" );

	Cmd_RemoveCommand( "map" );
	Cmd_RemoveCommand( "newgame" );
	Cmd_RemoveCommand( "endgame" );
	Cmd_RemoveCommand( "killgame" );
	Cmd_RemoveCommand( "hazardcourse" );
	Cmd_RemoveCommand( "changelevel" );
	Cmd_RemoveCommand( "changelevel2" );
	Cmd_RemoveCommand( "restart" );
	Cmd_RemoveCommand( "reload" );
	Cmd_RemoveCommand( "entpatch" );
	Cmd_RemoveCommand( "edicts_info" );
	Cmd_RemoveCommand( "entity_info" );
	Cmd_RemoveCommand( "sendreconnect" );

	if( Host_IsDedicated() )
	{
		Cmd_RemoveCommand( "say" );
		Cmd_RemoveCommand( "killserver" );
		Cmd_RemoveCommand( "startdefaultmap" );
	}
	else
	{
		Cmd_RemoveCommand( "map_background" );
		Cmd_RemoveCommand( "save" );
		Cmd_RemoveCommand( "load" );
		Cmd_RemoveCommand( "savequick" );
		Cmd_RemoveCommand( "loadquick" );
		Cmd_RemoveCommand( "killsave" );
		Cmd_RemoveCommand( "autosave" );
	}
}
