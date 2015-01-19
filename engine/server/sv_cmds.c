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
	Q_vsprintf( string, fmt, argptr );
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
	Q_vsprintf( string, fmt, argptr );
	va_end( argptr );
	
	// echo to console
	if( host.type == HOST_DEDICATED ) Msg( "%s", string );

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
	Q_vsprintf( string, fmt, argptr );
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

	if( !svs.clients )
	{
		Msg( "^3no server running.\n" );
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

	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: map <mapname>\n" );
		return;
	}

	// hold mapname to other place
	Q_strncpy( mapname, Cmd_Argv( 1 ), sizeof( mapname ));
	
	// determine spawn entity classname
	if( sv_maxclients->integer == 1 )
		spawn_entity = GI->sp_entity;
	else spawn_entity = GI->mp_entity;

	flags = SV_MapIsValid( mapname, spawn_entity, NULL );

	if( flags & MAP_INVALID_VERSION )
	{
		Msg( "SV_NewMap: map %s is invalid or not supported\n", mapname );
		return;
	}
	
	if(!( flags & MAP_IS_EXIST ))
	{
		Msg( "SV_NewMap: map %s doesn't exist\n", mapname );
		return;
	}

	if(!( flags & MAP_HAS_SPAWNPOINT ))
	{
		Msg( "SV_NewMap: map %s doesn't have a valid spawnpoint\n", mapname );
		return;
	}

	// init network stuff
	NET_Config(( sv_maxclients->integer > 1 ));

	// changing singleplayer to multiplayer or back. refresh the player count
	if(( sv_maxclients->modified ) || ( deathmatch->modified ) || ( coop->modified ) || ( teamplay->modified ))
		Host_ShutdownServer();

	SCR_BeginLoadingPlaque( false );

	sv.changelevel = false;
	sv.background = false;
	sv.loadgame = false; // set right state
	SV_ClearSaveDir ();	// delete all temporary *.hl files

	SV_DeactivateServer();
	SV_SpawnServer( mapname, NULL );
	SV_LevelInit( mapname, NULL, NULL, false );
	SV_ActivateServer ();
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
	NET_Config ( false ); // close network sockets

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
	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: load <savename>\n" );
		return;
	}
	SV_LoadGame( Cmd_Argv( 1 ));
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
		Msg( "Usage: delsave <name>\n" );
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

	mapname = Cmd_Argv( 1 );

	// determine spawn entity classname
	if( sv_maxclients->integer == 1 )
		spawn_entity = GI->sp_entity;
	else spawn_entity = GI->mp_entity;

	flags = SV_MapIsValid( mapname, spawn_entity, Cmd_Argv( 2 ));

	if( flags & MAP_INVALID_VERSION )
	{
		Msg( "SV_ChangeLevel: map %s is invalid or not supported\n", mapname );
		return;
	}
	
	if(!( flags & MAP_IS_EXIST ))
	{
		Msg( "SV_ChangeLevel: map %s doesn't exist\n", mapname );
		return;
	}

	if( c >= 3 && !( flags & MAP_HAS_LANDMARK ))
	{
		if( sv_validate_changelevel->integer )
		{
			// NOTE: we find valid map but specified landmark it's doesn't exist
			// run simple changelevel like in q1, throw warning
			MsgDev( D_INFO, "SV_ChangeLevel: map %s is exist but doesn't contain\n", mapname );
			MsgDev( D_INFO, "landmark with name %s. Run classic quake changelevel\n", Cmd_Argv( 2 ));
			c = 2; // reduce args
		}
	}

	if( c >= 3 && !Q_stricmp( sv.name, Cmd_Argv( 1 )))
	{
		MsgDev( D_INFO, "SV_ChangeLevel: can't changelevel with same map. Ignored.\n" );
		return;	
	}

	if( c == 2 && !( flags & MAP_HAS_SPAWNPOINT ))
	{
		if( sv_validate_changelevel->integer )
		{
			MsgDev( D_INFO, "SV_ChangeLevel: map %s doesn't have a valid spawnpoint. Ignored.\n", mapname );
			return;	
		}
	}

	// bad changelevel position invoke enables in one-way transtion
	if( sv.net_framenum < 15 )
	{
		if( sv_validate_changelevel->integer )
		{
			MsgDev( D_INFO, "SV_ChangeLevel: a infinite changelevel detected.\n" );
			MsgDev( D_INFO, "Changelevel will be disabled until a next save\\restore.\n" );
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
	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: kick <userid>\n" );
		return;
	}

	if( !svs.clients || sv.background )
	{
		Msg( "^3no server running.\n" );
		return;
	}

	if( !SV_SetPlayer( )) return;

	SV_BroadcastPrintf( PRINT_HIGH, "%s was kicked\n", svs.currentPlayer->name );
	SV_ClientPrintf( svs.currentPlayer, PRINT_HIGH, "You were kicked from the game\n" );
	SV_DropClient( svs.currentPlayer );

	// min case there is a funny zombie
	svs.currentPlayer->lastmessage = host.realtime;
}

/*
==================
SV_Kill_f
==================
*/
void SV_Kill_f( void )
{
	if( !SV_SetPlayer() || sv.background )
		return;

	if( !svs.currentPlayer || !SV_IsValidEdict( svs.currentPlayer->edict ))
		return;

	if( svs.currentPlayer->edict->v.health <= 0.0f )
	{
		SV_ClientPrintf( svs.currentPlayer, PRINT_HIGH, "Can't suicide -- allready dead!\n");
		return;
	}

	svgame.dllFuncs.pfnClientKill( svs.currentPlayer->edict );	
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
		Msg( "^3no server running.\n" );
		return;
	}

	Msg( "map: %s\n", sv.name );
	Msg( "num score ping    name            lastmsg address               port \n" );
	Msg( "--- ----- ------- --------------- ------- --------------------- ------\n" );

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		int	j, l, ping;
		char	*s;

		if( !cl->state ) continue;

		Msg( "%3i ", i );
		Msg( "%5i ", (int)cl->edict->v.frags );

		if( cl->state == cs_connected ) Msg( "Connect" );
		else if( cl->state == cs_zombie ) Msg( "Zombie " );
		else if( cl->fakeclient ) Msg( "Bot   " );
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Msg( "%7i ", ping );
		}

		Msg( "%s", cl->name );
		l = 24 - Q_strlen( cl->name );
		for( j = 0; j < l; j++ ) Msg( " " );
		Msg( "%g ", ( host.realtime - cl->lastmessage ));
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
		Msg( "^3no server running.\n" );
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
	NET_Config ( false ); // close network sockets
}

/*
===============
SV_PlayersOnly_f

disable plhysics but players
===============
*/
void SV_PlayersOnly_f( void )
{
	if( !Cvar_VariableInteger( "sv_cheats" )) return;
	sv.hostflags = sv.hostflags ^ SVF_PLAYERSONLY;

	if(!( sv.hostflags & SVF_PLAYERSONLY ))
		SV_BroadcastPrintf( D_INFO, "Resume server physic\n" );
	else SV_BroadcastPrintf( D_INFO, "Freeze server physic\n" );
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
		Msg( "^3no server running.\n" );
		return;
	}

	active = pfnNumberOfEntities(); 
	Msg( "%5i edicts is used\n", active );
	Msg( "%5i edicts is free\n", svgame.globals->maxEntities - active );
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
		Msg( "^3no server running.\n" );
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
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands( void )
{
	Cmd_AddCommand( "heartbeat", SV_Heartbeat_f, "send a heartbeat to the master server" );
	Cmd_AddCommand( "kick", SV_Kick_f, "kick a player off the server by number or name" );
	Cmd_AddCommand( "kill", SV_Kill_f, "die instantly" );
	Cmd_AddCommand( "status", SV_Status_f, "print server status information" );
	Cmd_AddCommand( "serverinfo", SV_ServerInfo_f, "print server settings" );
	Cmd_AddCommand( "clientinfo", SV_ClientInfo_f, "print user infostring (player num required)" );
	Cmd_AddCommand( "playersonly", SV_PlayersOnly_f, "freezes time, except for players" );

	Cmd_AddCommand( "map", SV_Map_f, "start new level" );
	Cmd_AddCommand( "newgame", SV_NewGame_f, "begin new game" );
	Cmd_AddCommand( "endgame", SV_EndGame_f, "end current game" );
	Cmd_AddCommand( "killgame", SV_KillGame_f, "end current game" );
	Cmd_AddCommand( "hazardcourse", SV_HazardCourse_f, "starting a Hazard Course" );
	Cmd_AddCommand( "changelevel", SV_ChangeLevel_f, "changing level" );
	Cmd_AddCommand( "restart", SV_Restart_f, "restarting current level" );
	Cmd_AddCommand( "reload", SV_Reload_f, "continue from latest save or restart level" );
	Cmd_AddCommand( "entpatch", SV_EntPatch_f, "write entity patch to allow external editing" );
	Cmd_AddCommand( "edicts_info", SV_EdictsInfo_f, "show info about edicts" );
	Cmd_AddCommand( "entity_info", SV_EntityInfo_f, "show more info about edicts" );

	if( host.type == HOST_DEDICATED )
	{
		Cmd_AddCommand( "say", SV_ConSay_f, "send a chat message to everyone on the server" );
		Cmd_AddCommand( "killserver", SV_KillServer_f, "shutdown current server" );
	}
	else
	{
		Cmd_AddCommand( "map_background", SV_MapBackground_f, "set background map" );
		Cmd_AddCommand( "save", SV_Save_f, "save the game to a file" );
		Cmd_AddCommand( "load", SV_Load_f, "load a saved game file" );
		Cmd_AddCommand( "savequick", SV_QuickSave_f, "save the game to the quicksave" );
		Cmd_AddCommand( "loadquick", SV_QuickLoad_f, "load a quick-saved game file" );
		Cmd_AddCommand( "killsave", SV_DeleteSave_f, "delete a saved game file and saveshot" );
		Cmd_AddCommand( "autosave", SV_AutoSave_f, "save the game to 'autosave' file" );
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
	Cmd_RemoveCommand( "kill" );
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
	Cmd_RemoveCommand( "restart" );
	Cmd_RemoveCommand( "reload" );
	Cmd_RemoveCommand( "entpatch" );
	Cmd_RemoveCommand( "edicts_info" );
	Cmd_RemoveCommand( "entity_info" );

	if( host.type == HOST_DEDICATED )
	{
		Cmd_RemoveCommand( "say" );
		Cmd_RemoveCommand( "killserver" );
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