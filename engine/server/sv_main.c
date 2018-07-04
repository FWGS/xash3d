/*
sv_main.c - server main loop
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
#include "net_encode.h"

#include "errno.h"
#include "Sequence.h"
#define HEARTBEAT_SECONDS	(sv_nat->integer?60.0f:300.0f) 		// 1 or 5 minutes

convar_t	*sv_zmax;
convar_t	*sv_novis;			// disable server culling entities by vis
convar_t	*sv_unlag;
convar_t	*sv_maxunlag;
convar_t	*sv_unlagpush;
convar_t	*sv_unlagsamples;
convar_t	*sv_pausable;
convar_t	*sv_newunit;
convar_t	*sv_wateramp;
convar_t	*sv_timeout;				// seconds without any message
convar_t	*zombietime;			// seconds to sink messages after disconnect
convar_t	*rcon_password;			// password for remote server commands
convar_t	*sv_airaccelerate;
convar_t	*sv_wateraccelerate;
convar_t	*sv_maxvelocity;
convar_t	*sv_gravity;
convar_t	*sv_stepsize;
convar_t	*sv_rollangle;
convar_t	*sv_rollspeed;
convar_t	*sv_wallbounce;
convar_t	*sv_maxspeed;
convar_t	*sv_spectatormaxspeed;


convar_t	*sv_accelerate;
convar_t	*sv_friction;


convar_t	*sv_edgefriction;
convar_t	*sv_waterfriction;
convar_t	*sv_stopspeed;
convar_t	*hostname;
convar_t	*sv_lighting_modulate;
convar_t	*sv_maxclients;
convar_t	*sv_check_errors;
convar_t	*sv_footsteps;
convar_t	*public_server;			// should heartbeats be sent
convar_t	*sv_reconnect_limit;		// minimum seconds between connect messages
convar_t	*sv_failuretime;
convar_t	*sv_allow_upload;
convar_t	*sv_allow_download;
convar_t	*sv_allow_fragment;
convar_t	*sv_downloadurl;
convar_t	*sv_allow_studio_attachment_angles;
convar_t	*sv_allow_rotate_pushables;
convar_t	*sv_allow_godmode;
convar_t	*sv_allow_noclip;
convar_t	*sv_allow_impulse;
convar_t	*sv_enttools_enable;
convar_t	*sv_enttools_maxfire;
convar_t	*sv_validate_changelevel;
convar_t	*sv_clienttrace;
convar_t	*sv_send_resources;
convar_t	*sv_send_logos;
convar_t	*sv_sendvelocity;
convar_t	*sv_airmove;
convar_t	*sv_quakehulls;
convar_t	*mp_consistency;
convar_t	*clockwindow;
convar_t	*deathmatch;
convar_t	*teamplay;
convar_t	*skill;
convar_t	*coop;
convar_t	*sv_skipshield; // HACK for shield
convar_t	*sv_trace_messages;
convar_t	*sv_corpse_solid;
convar_t	*sv_fixmulticast;
convar_t	*sv_allow_split;
convar_t	*sv_allow_compress;
convar_t	*sv_maxpacket;
convar_t	*sv_forcesimulating;
convar_t	*sv_nat;
convar_t	*sv_password;
convar_t	*sv_userinfo_enable_penalty;
convar_t	*sv_userinfo_penalty_time;
convar_t	*sv_userinfo_penalty_multiplier;
convar_t	*sv_userinfo_penalty_attempts;

// sky variables
convar_t	*sv_skycolor_r;
convar_t	*sv_skycolor_g;
convar_t	*sv_skycolor_b;
convar_t	*sv_skyvec_x;
convar_t	*sv_skyvec_y;
convar_t	*sv_skyvec_z;
convar_t	*sv_skyname;
convar_t	*sv_wateralpha;
convar_t	*sv_skydir_x;
convar_t	*sv_skydir_y;
convar_t	*sv_skydir_z;
convar_t	*sv_skyangle;
convar_t	*sv_skyspeed;


convar_t	*sv_allow_noinputdevices;
convar_t	*sv_allow_touch;
convar_t	*sv_allow_mouse;
convar_t	*sv_allow_joystick;
convar_t	*sv_allow_vr;

void Master_Shutdown( void );

char localinfo[MAX_LOCALINFO];


//============================================================================

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings( void )
{
	sv_client_t	*cl;
	int			i;

	// clamp fps counter
	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		cl = &svs.clients[i];

		if( cl->state != cs_spawned )
			continue;

		cl->ping = SV_CalcPing( cl );
	}
}

/*
===================
SV_CalcPacketLoss

determine % of packets that were not ack'd.
===================
*/
int SV_CalcPacketLoss( sv_client_t *cl )
{
	int	i, lost, count;
	float	losspercent;
	register client_frame_t *frame;
	int	numsamples;

	lost  = 0;
	count = 0;

	if( cl->fakeclient )
		return 0;

	numsamples = SV_UPDATE_BACKUP / 2;

	for( i = 0; i < numsamples; i++ )
	{
		frame = &cl->frames[(cl->netchan.incoming_acknowledged - 1 - i) & SV_UPDATE_MASK];
		count++;
		if( frame->latency == -1 )
			lost++;
	}

	if( !count ) return 100;
	losspercent = 100.0f * ( float )lost / ( float )count;

	return (int)losspercent;
}

/*
================
SV_HasActivePlayers

returns true if server have spawned players
================
*/
qboolean SV_HasActivePlayers( void )
{
	int	i;

	// server inactive
	if( !svs.clients ) return false;

	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		if( svs.clients[i].state == cs_spawned )
			return true;
	}
	return false;
}

/*
===================
SV_UpdateMovevars

check movevars for changes every frame
send updates to client if changed
===================
*/
void SV_UpdateMovevars( qboolean initialize )
{
	if( !initialize && !physinfo->modified )
		return;

	// check range
	if( sv_zmax->value < 256.0f ) Cvar_SetFloat( "sv_zmax", 256.0f );

	// clamp it right
	if( host.features & ENGINE_WRITE_LARGE_COORD )
	{
		if( sv_zmax->value > 131070.0f )
			Cvar_SetFloat( "sv_zmax", 131070.0f );
	}
	else
	{
		if( sv_zmax->value > 32767.0f )
			Cvar_SetFloat( "sv_zmax", 32767.0f );
	}

	svgame.movevars.gravity = sv_gravity->value;
	svgame.movevars.stopspeed = sv_stopspeed->value;
	svgame.movevars.maxspeed = sv_maxspeed->value;
	svgame.movevars.spectatormaxspeed = sv_spectatormaxspeed->value;
	svgame.movevars.accelerate = sv_accelerate->value;
	svgame.movevars.airaccelerate = sv_airaccelerate->value;
	svgame.movevars.wateraccelerate = sv_wateraccelerate->value;
	svgame.movevars.friction = sv_friction->value;
	svgame.movevars.edgefriction = sv_edgefriction->value;
	svgame.movevars.waterfriction = sv_waterfriction->value;
	svgame.movevars.bounce = sv_wallbounce->value;
	svgame.movevars.stepsize = sv_stepsize->value;
	svgame.movevars.maxvelocity = sv_maxvelocity->value;
	svgame.movevars.zmax = sv_zmax->value;
	svgame.movevars.waveHeight = sv_wateramp->value;
	Q_strncpy( svgame.movevars.skyName, sv_skyname->string, sizeof( svgame.movevars.skyName ));
	svgame.movevars.footsteps = sv_footsteps->integer;
	svgame.movevars.rollangle = sv_rollangle->value;
	svgame.movevars.rollspeed = sv_rollspeed->value;
	svgame.movevars.skycolor_r = sv_skycolor_r->value;
	svgame.movevars.skycolor_g = sv_skycolor_g->value;
	svgame.movevars.skycolor_b = sv_skycolor_b->value;
	svgame.movevars.skyvec_x = sv_skyvec_x->value;
	svgame.movevars.skyvec_y = sv_skyvec_y->value;
	svgame.movevars.skyvec_z = sv_skyvec_z->value;
	svgame.movevars.skydir_x = sv_skydir_x->value;
	svgame.movevars.skydir_y = sv_skydir_y->value;
	svgame.movevars.skydir_z = sv_skydir_z->value;
	svgame.movevars.skyangle = sv_skyangle->value;
	svgame.movevars.wateralpha = sv_wateralpha->value;
	svgame.movevars.features = host.features; // just in case. not really need

	if( initialize ) return; // too early

	if( MSG_WriteDeltaMovevars( &sv.reliable_datagram, &svgame.oldmovevars, &svgame.movevars ))
		Q_memcpy( &svgame.oldmovevars, &svgame.movevars, sizeof( movevars_t )); // oldstate changed

	physinfo->modified = false;
}

void pfnUpdateServerInfo( const char *szKey, const char *szValue, const char *unused, void *unused2 )
{
	convar_t	*cv = Cvar_FindVar( szKey );

	if( !cv || !cv->modified ) return; // this cvar not changed

	BF_WriteByte( &sv.reliable_datagram, svc_serverinfo );
	BF_WriteString( &sv.reliable_datagram, szKey );
	BF_WriteString( &sv.reliable_datagram, szValue );
	cv->modified = false; // reset state
}

void SV_UpdateServerInfo( void )
{
	if( !serverinfo->modified ) return;

	Cvar_LookupVars( CVAR_SERVERINFO, NULL, NULL, (setpair_t)pfnUpdateServerInfo );

	serverinfo->modified = false;
}

/*
=================
SV_CheckCmdTimes
=================
*/
void SV_CheckCmdTimes( void )
{
	sv_client_t	*cl;
	static double	lastreset = 0;
	double		timewindow;
	int		i;

	if(( host.realtime - lastreset ) < 1.0 )
		return;

	lastreset = host.realtime;

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state != cs_spawned )
			continue;

		if( cl->last_cmdtime == 0.0 )
		{
			cl->last_cmdtime = host.realtime;
		}

		timewindow = cl->last_movetime + cl->last_cmdtime - host.realtime;

		if( timewindow > clockwindow->value )
		{
			cl->next_movetime = clockwindow->value + host.realtime;
			cl->last_movetime = host.realtime - cl->last_cmdtime;
		}
		else if( timewindow < -clockwindow->value )
		{
			cl->last_movetime = host.realtime - cl->last_cmdtime;
		}
	}
}

/*
=================
SV_ReadPackets
=================
*/
void SV_ReadPackets( void )
{
	sv_client_t	*cl;
	int		i, qport;
	size_t curSize;

	while( NET_GetPacket( NS_SERVER, &net_from, net_message_buffer, &curSize ))
	{
		if( !svs.initialized )
		{
			BF_Init( &net_message, "ClientPacket", net_message_buffer, curSize );

			// check for rcon here
			if( BF_GetMaxBytes( &net_message ) >= 4 && *(int *)net_message.pData == -1 )
			{
				char *args, *c;
				BF_Clear( &net_message  );
				BF_ReadLong( &net_message  );// skip the -1 marker

				args = BF_ReadStringLine( &net_message  );
				Cmd_TokenizeString( args );
				c = Cmd_Argv( 0 );

				if( !Q_strcmp( c, "rcon" ))
					SV_RemoteCommand( net_from, &net_message );
			}
			continue;
		}
		if( LittleLong(*((int *)&net_message_buffer)) == 0xFFFFFFFE )
		{
			if( curSize <= NETSPLIT_HEADER_SIZE )
				continue;

			// find client with this address and enabled netsplit
			// netsplit packets does not allow changing ports

			for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
			{
				if( cl->state == cs_free || cl->fakeclient )
					continue;

				if( !NET_CompareBaseAdr( net_from, cl->netchan.remote_address ))
					continue;

				if( !cl->netchan.split )
					continue;
			}

			// not a client
			if( i >= sv_maxclients->integer )
			{
				MsgDev( D_INFO, "netsplit from unknown addr %s\n", NET_AdrToString( net_from ) );
				continue;
			}

			// Will rewrite existing packet by merged
			// Client will not send compressed split packets
			if( !NetSplit_GetLong( &cl->netchan.netsplit, &net_from, net_message_buffer, &curSize, false ) )
				continue;
		}
		BF_Init( &net_message, "ClientPacket", net_message_buffer, curSize );

		// check for connectionless packet (0xffffffff) first
		if( BF_GetMaxBytes( &net_message ) >= 4 && *(int *)net_message.pData == -1 )
		{
			SV_ConnectionlessPacket( net_from, &net_message );
			continue;
		}


		// read the qport out of the message so we can fix up
		// stupid address translating routers
		BF_Clear( &net_message );
		BF_ReadLong( &net_message );	// sequence number
		BF_ReadLong( &net_message );	// sequence number
		qport = (int)BF_ReadShort( &net_message ) & 0xffff;

		// check for packets from connected clients
		for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
		{
			if( cl->state == cs_free || cl->fakeclient )
				continue;

			if( !NET_CompareBaseAdr( net_from, cl->netchan.remote_address ))
				continue;

			if( cl->netchan.qport != qport )
				continue;

			if( cl->netchan.remote_address.port != net_from.port )
			{
				MsgDev( D_INFO, "SV_ReadPackets: fixing up a translated port\n");
				cl->netchan.remote_address.port = net_from.port;
			}

			if( Netchan_Process( &cl->netchan, &net_message ))
			{
				if( sv_maxclients->integer == 1 || cl->state != cs_spawned )
					cl->send_message = true; // reply at end of frame

				// this is a valid, sequenced packet, so process it
				if( cl->state != cs_zombie )
				{
					cl->lastmessage = host.realtime; // don't timeout
					SV_ExecuteClientMessage( cl, &net_message );
					svgame.globals->frametime = host.frametime;
					svgame.globals->time = sv.time;
				}
			}

			// fragmentation/reassembly sending takes priority over all game messages, want this in the future?
			if( Netchan_IncomingReady( &cl->netchan ))
			{
				if( Netchan_CopyNormalFragments( &cl->netchan, &net_message ))
				{
					BF_Clear( &net_message );
					SV_ExecuteClientMessage( cl, &net_message );
				}

				if( Netchan_CopyFileFragments( &cl->netchan, &net_message ))
				{
					SV_ProcessFile( cl, cl->netchan.incomingfilename );
				}
			}
			break;
		}

		if( i != sv_maxclients->integer )
			continue;
	}
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->value
seconds, drop the conneciton.

When a client is normally dropped, the sv_client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts( void )
{
	sv_client_t	*cl;
	float		droppoint;
	float		zombiepoint;
	int		i, numclients;

	droppoint = host.realtime - sv_timeout->value;
	zombiepoint = host.realtime - zombietime->value;

	for( i = 0, numclients = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state >= cs_connected )
		{
			if( cl->edict && !( cl->edict->v.flags & (FL_SPECTATOR|FL_FAKECLIENT)))
				numclients++;
					}

		// fake clients do not timeout
		if( cl->fakeclient ) cl->lastmessage = host.realtime;

		// message times may be wrong across a changelevel
		if( cl->lastmessage > host.realtime )
			cl->lastmessage = host.realtime;

		if( cl->state == cs_zombie && cl->lastmessage < zombiepoint )
		{
			//if( cl->edict && !cl->edict->pvPrivateData )
				cl->state = cs_free; // can now be reused
			// Does not work too, as entity may be referenced
			// But you may increase zombietime
			#if 0
			if( cl->edict && cl->edict->pvPrivateData != NULL )
			{
				// NOTE: new interface can be missing
				if( svgame.dllFuncs2.pfnOnFreeEntPrivateData != NULL )
					svgame.dllFuncs2.pfnOnFreeEntPrivateData( cl->edict );

				// clear any dlls data but keep engine data
				Mem_Free( cl->edict->pvPrivateData );
				cl->edict->pvPrivateData = NULL;
			}
			#endif
			continue;
		}

		if(( cl->state == cs_connected || cl->state == cs_spawned ) && cl->lastmessage < droppoint && !NET_IsLocalAddress( cl->netchan.remote_address ))
		{
			SV_BroadcastPrintf( PRINT_HIGH, "%s timed out\n", cl->name );
			SV_DropClient( cl );
			cl->state = cs_free; // don't bother with zombie state
		}
	}

	if( sv.paused && !numclients )
	{
		// nobody left, unpause the server
		SV_TogglePause( "Pause released since no players are left." );
	}
}

/*
================
SV_PrepWorldFrame

This has to be done before the world logic, because
player processing happens outside RunWorldFrame
================
*/
void SV_PrepWorldFrame( void )
{
	edict_t	*ent;
	int	i;

	for( i = 1; i < svgame.numEntities; i++ )
	{
		ent = EDICT_NUM( i );
		if( ent->free ) continue;

		ent->v.effects &= ~(EF_MUZZLEFLASH|EF_NOINTERP);
	}
}

/*
=================
SV_ProcessFile
=================
*/
void SV_ProcessFile( sv_client_t *cl, char *filename )
{
	// some other file...
	MsgDev( D_INFO, "Received file %s from %s\n", filename, cl->name );
}

void SV_DownloadResources_f( void )
{
	int index;
	if( Q_strchr( download_types->string, 'm' ) )
		for( index = 1; index < MAX_MODELS && sv.model_precache[index][0]; index++ )
		{
			if( sv.model_precache[index][0] == '*' ) // internal bmodel
				continue;
			if( !FS_FileExists( sv.model_precache[index], true ) )
				HTTP_AddDownload( sv.model_precache[index], -1, false );
		}
	if( Q_strchr( download_types->string, 's' ) )
		for( index = 1; index < MAX_SOUNDS && sv.sound_precache[index][0]; index++ )
		{
			char *sndname = va( "sound/%s", sv.sound_precache[index]);
			if( !FS_FileExists( sndname, true ) )
				HTTP_AddDownload( sndname, -1, false );
		}
	if( Q_strchr( download_types->string, 'e' ) )
		for( index = 1; index < MAX_EVENTS && sv.event_precache[index][0]; index++ )
		{
			if( !FS_FileExists( sv.event_precache[index], true ) )
				HTTP_AddDownload( sv.event_precache[index], -1, false );
		}
	if( Q_strchr( download_types->string, 'c' ) )
		for( index = 1; index < MAX_CUSTOM && sv.files_precache[index][0]; index++ )
		{
			if( !FS_FileExists( sv.files_precache[index], true ) )
				HTTP_AddDownload( sv.files_precache[index], -1, false );
		}
}

/*
=================
SV_IsSimulating
=================
*/
qboolean SV_IsSimulating( void )
{
	if ( sv_forcesimulating->integer )
		return true;

	if( sv.background && SV_Active() && CL_Active())
	{
		if( CL_IsInConsole( ))
			return false;
		return true; // force simulating for background map
	}

	if( sv.hostflags & SVF_PLAYERSONLY )
		return false;
	if( !SV_HasActivePlayers())
		return false;
	if( !sv.paused && CL_IsInGame( ))
		return true;
	return false;
}

/*
=================
SV_RunGameFrame
=================
*/
void SV_RunGameFrame( void )
{
	if( !SV_IsSimulating( )) return;

	SV_Physics();

	sv.time += host.frametime;
}

/*
==================
Host_ServerFrame

==================
*/
void Host_ServerFrame( void )
{
	// if server is not active, do nothing
	if( !svs.initialized )
	{
		SV_ReadPackets (); // allow rcon
		return;
	}

	svgame.globals->frametime = host.frametime;

	// check timeouts
	SV_CheckTimeouts ();

	// check clients timewindow
	SV_CheckCmdTimes ();

	// read packets from clients
	SV_ReadPackets ();

	// update ping based on the last known frame from all clients
	SV_CalcPings ();

	// refresh serverinfo on the client side
	SV_UpdateServerInfo ();

	// refresh physic movevars on the client side
	SV_UpdateMovevars ( false );

	// let everything in the world think and move
	SV_RunGameFrame ();

	// send messages back to the clients that had packets read this frame
	SV_SendClientMessages ();

	// clear edict flags for next frame
	SV_PrepWorldFrame ();

	// send a heartbeat to the master if needed
	Master_Heartbeat ();
}

//============================================================================

/*
=================
Master_Add
=================
*/
void Master_Add( void )
{
	if( NET_SendToMasters( NS_SERVER, 2, "q\xFF" ) )
		svs.last_heartbeat = MAX_HEARTBEAT; // try next frame
}


/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
void Master_Heartbeat( void )
{
	if( !public_server->integer || sv_maxclients->integer == 1 )
		return; // only public servers send heartbeats

	// check for time wraparound
	if( svs.last_heartbeat > host.realtime )
		svs.last_heartbeat = host.realtime;

	if(( host.realtime - svs.last_heartbeat ) < HEARTBEAT_SECONDS )
		return; // not time to send yet

	svs.last_heartbeat = host.realtime;

	Master_Add();
}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void Master_Shutdown( void )
{
	NET_Config( true, false ); // allow remote
	while( NET_SendToMasters( NS_SERVER, 2, "\x62\x0A" ) );
}

/*
=================
SV_AddToMaster

A server info answer to master server.
Master will validate challenge and this server to public list
=================
*/
void SV_AddToMaster( netadr_t from, sizebuf_t *msg )
{
	uint challenge;
	char s[4096] = "0\n"; // skip 2 bytes of header
	int clients = 0, bots = 0, index;
	qboolean havePassword;

	if( svs.clients )
	{
		for( index = 0; index < sv_maxclients->integer; index++ )
		{
			if( svs.clients[index].state >= cs_connected )
			{
				if( svs.clients[index].fakeclient )
					bots++;
				else clients++;
			}
		}
	}

	challenge = BF_ReadUBitLong( msg, sizeof( uint ) << 3 );
	havePassword = sv_password->string[0] && Q_stricmp( sv_password->string, "none" );

	Info_SetValueForKey(s, "protocol",  va( "%d", PROTOCOL_VERSION ), sizeof( s ) ); // protocol version
	Info_SetValueForKey(s, "challenge", va( "%u", challenge ), sizeof( s )  ); // challenge number
	Info_SetValueForKey(s, "players",   va( "%d", clients ), sizeof( s ) ); // current player number, without bots
	Info_SetValueForKey(s, "max",       sv_maxclients->string, sizeof( s ) ); // max_players
	Info_SetValueForKey(s, "bots",      va( "%d", bots ), sizeof( s ) ); // bot count
	Info_SetValueForKey(s, "gamedir",   GI->gamefolder, sizeof( s ) ); // gamedir
	Info_SetValueForKey(s, "map",       sv.name, sizeof( s ) ); // current map
	Info_SetValueForKey(s, "type",      Host_IsDedicated() ? "d" : "l", sizeof( s ) ); // dedicated
	Info_SetValueForKey(s, "password",  havePassword       ? "1" : "0", sizeof( s ) ); // is password set

#ifdef _WIN32
	Info_SetValueForKey(s, "os",        "w", sizeof( s ) ); // Windows
#else
	Info_SetValueForKey(s, "os",        "l", sizeof( s ) ); // Linux
#endif

	Info_SetValueForKey(s, "secure",    "0", sizeof( s ) ); // server anti-cheat
	Info_SetValueForKey(s, "lan",       "0", sizeof( s ) ); // LAN servers doesn't send info to master
	Info_SetValueForKey(s, "version",   XASH_VERSION, sizeof( s ) ); // server region. 255 -- all regions
	Info_SetValueForKey(s, "region",    "255", sizeof( s ) ); // server region. 255 -- all regions
	Info_SetValueForKey(s, "product",   GI->gamefolder, sizeof( s ) ); // product? Where is the difference with gamedir?
	Info_SetValueForKey(s, "nat",       sv_nat->string, sizeof( s ) ); // Server running under NAT, use reverse connection

	NET_SendPacket( NS_SERVER, Q_strlen( s ), s, from );
}

/*
====================
SV_ProcessUserAgent

send error message and return false on wrong input devices
====================
*/
qboolean SV_ProcessUserAgent( netadr_t from, char *useragent )
{
	char *input_devices_str = Info_ValueForKey( useragent, "d" );
	char *id = Info_ValueForKey( useragent, "i" );

	if( !sv_allow_noinputdevices->integer && ( !input_devices_str || !input_devices_str[0] ) )
	{
		Netchan_OutOfBandPrint( NS_SERVER, from, "print\nThis server does not allow\nconnect without input devices list.\nPlease update your engine.\n" );
		return false;
	}

	if( input_devices_str )
	{
		int input_devices = Q_atoi( input_devices_str );

		if( !sv_allow_touch->integer && ( input_devices & INPUT_DEVICE_TOUCH ) )
		{
			Netchan_OutOfBandPrint( NS_SERVER, from, "errormsg\nThis server does not allow touch\nDisable it (touch_enable 0)\nto play on this server\n" );
			return false;
		}
		if( !sv_allow_mouse->integer && ( input_devices & INPUT_DEVICE_MOUSE) )
		{
			Netchan_OutOfBandPrint( NS_SERVER, from, "errormsg\nThis server does not allow mouse\nDisable it(m_ignore 1)\nto play on this server\n" );
			return false;
		}
		if( !sv_allow_joystick->integer && ( input_devices & INPUT_DEVICE_JOYSTICK) )
		{
			Netchan_OutOfBandPrint( NS_SERVER, from, "errormsg\nThis server does not allow joystick\nDisable it(joy_enable 0)\nto play on this server\n" );
			return false;
		}
		if( !sv_allow_vr->integer && ( input_devices & INPUT_DEVICE_VR) )
		{
			Netchan_OutOfBandPrint( NS_SERVER, from, "errormsg\nThis server does not allow VR\n" );
			return false;
		}
	}

	if( id )
	{
		qboolean banned = SV_CheckID( id );

		if( banned )
		{
			Netchan_OutOfBandPrint( NS_SERVER, from, "errormsg\nYou are banned!\n" );
			return false;
		}
	}

	return true;
}

//============================================================================

/*
===============
SV_Init

Only called at startup, not for each game
===============
*/
void SV_Init( void )
{
	SV_InitOperatorCommands();

	Log_InitCvars();

	skill = Cvar_Get ("skill", "1", CVAR_LATCH, "game skill level" );
	deathmatch = Cvar_Get ("deathmatch", "0", CVAR_LATCH|CVAR_SERVERINFO, "displays deathmatch state" );
	teamplay = Cvar_Get ("teamplay", "0", CVAR_LATCH|CVAR_SERVERINFO, "displays teamplay state" );
	coop = Cvar_Get ("coop", "0", CVAR_LATCH|CVAR_SERVERINFO, "displays cooperative state" );
	Cvar_Get ("protocol", va( "%i", PROTOCOL_VERSION ), CVAR_INIT, "displays server protocol version" );
	Cvar_Get ("defaultmap", "", CVAR_SERVERNOTIFY, "holds the multiplayer mapname" );
	Cvar_Get ("showtriggers", "0", CVAR_LATCH, "debug cvar shows triggers" );
	Cvar_Get ("sv_aim", "0", CVAR_ARCHIVE, "enable auto-aiming" );
	Cvar_Get ("mapcyclefile", "mapcycle.txt", 0, "name of multiplayer map cycle configuration file" );
	Cvar_Get ("servercfgfile","server.cfg", 0, "name of dedicated server configuration file" );
	Cvar_Get ("lservercfgfile","listenserver.cfg", 0, "name of listen server configuration file" );
	Cvar_Get ("mapchangecfgfile","", 0, "name of map change configuration file" );
	Cvar_Get ("bannedcfgfile", "banned.cfg", 0, "name of banned.cfg file" );
	Cvar_Get ("listipcfgfile", "listip.cfg", 0, "name of listip.cfg file" );
	Cvar_Get ("motdfile", "motd.txt", 0, "name of 'message of the day' file" );
	Cvar_Get ("sv_language", "0", 0, "game language (currently unused)" );
	Cvar_Get ("suitvolume", "0.25", CVAR_ARCHIVE, "HEV suit volume" );
	Cvar_Get ("sv_background", "0", CVAR_READ_ONLY, "indicates that background map is running" );
	Cvar_Get( "gamedir", GI->gamefolder, CVAR_SERVERINFO|CVAR_SERVERNOTIFY|CVAR_INIT, "game folder" );
	Cvar_Get( "sv_alltalk", "1", 0, "allow talking for all players (legacy, unused)" );
	Cvar_Get( "sv_airmove", "1", CVAR_SERVERNOTIFY, "enable airmovement (legacy, unused)" );
	Cvar_Get( "mp_autocrosshair", "0", 0, "allow auto crosshair in multiplayer (legacy, unused)" );
	Cvar_Get( "sv_allow_PhysX", "1", CVAR_ARCHIVE, "allow XashXT to use PhysX engine" ); // XashXT cvar
	Cvar_Get( "sv_precache_meshes", "1", CVAR_ARCHIVE, "cache SOLID_CUSTOM meshes before level loads" ); // Paranoia 2 cvar
	Cvar_Get( "mp_allowmonsters", "0", CVAR_SERVERNOTIFY | CVAR_LATCH, "allow monsters in multiplayer" );
	Cvar_Get( "port", va( "%i", PORT_SERVER ), 0, "network default port" );
	Cvar_Get( "ip_hostport", "0", 0, "network server port" );

	// half-life shared variables
	sv_zmax = Cvar_Get ("sv_zmax", "4096", CVAR_PHYSICINFO, "zfar server value" );
	sv_wateramp = Cvar_Get ("sv_wateramp", "0", CVAR_PHYSICINFO, "global water wave height" );
	sv_skycolor_r = Cvar_Get ("sv_skycolor_r", "255", CVAR_PHYSICINFO, "skycolor red" );
	sv_skycolor_g = Cvar_Get ("sv_skycolor_g", "255", CVAR_PHYSICINFO, "skycolor green" );
	sv_skycolor_b = Cvar_Get ("sv_skycolor_b", "255", CVAR_PHYSICINFO, "skycolor blue" );
	sv_skyvec_x = Cvar_Get ("sv_skyvec_x", "0", CVAR_PHYSICINFO, "skylight direction x" );
	sv_skyvec_y = Cvar_Get ("sv_skyvec_y", "0", CVAR_PHYSICINFO, "skylight direction y" );
	sv_skyvec_z = Cvar_Get ("sv_skyvec_z", "0", CVAR_PHYSICINFO, "skylight direction z" );
	sv_skyname = Cvar_Get ("sv_skyname", "desert", CVAR_PHYSICINFO, "skybox name (can be dynamically changed in-game)" );
	sv_skydir_x = Cvar_Get ("sv_skydir_x", "0", CVAR_PHYSICINFO, "sky rotation direction x" );
	sv_skydir_y = Cvar_Get ("sv_skydir_y", "0", CVAR_PHYSICINFO, "sky rotation direction y" );
	sv_skydir_z = Cvar_Get ("sv_skydir_z", "1", CVAR_PHYSICINFO, "sky rotation direction z" ); // g-cont. add default sky rotate direction
	sv_skyangle = Cvar_Get ("sv_skyangle", "0", CVAR_PHYSICINFO, "skybox rotational angle (in degrees)" );
	sv_skyspeed = Cvar_Get ("sv_skyspeed", "0", 0, "skybox rotational speed" );
	sv_footsteps = Cvar_Get ("mp_footsteps", "1", CVAR_PHYSICINFO, "can hear footsteps from other players" );
	sv_wateralpha = Cvar_Get ("sv_wateralpha", "1", CVAR_PHYSICINFO, "world surfaces water transparency factor. 1.0 - solid, 0.0 - fully transparent" );

	rcon_password = Cvar_Get( "rcon_password", "", 0, "remote connect password" );
	sv_stepsize = Cvar_Get( "sv_stepsize", "18", CVAR_ARCHIVE|CVAR_PHYSICINFO, "how high you can step up" );
	sv_newunit = Cvar_Get( "sv_newunit", "0", 0, "sets to 1 while new unit is loading" );
	hostname = Cvar_Get( "hostname", "unnamed", CVAR_SERVERNOTIFY|CVAR_ARCHIVE, "host name" );
	sv_timeout = Cvar_Get( "sv_timeout", "125", CVAR_SERVERNOTIFY, "connection timeout" );
	zombietime = Cvar_Get( "zombietime", "2", CVAR_SERVERNOTIFY, "timeout for clients-zombie (who died but not respawned)" );
	sv_pausable = Cvar_Get( "pausable", "1", CVAR_SERVERNOTIFY, "allow players to pause or not" );
	sv_allow_studio_attachment_angles = Cvar_Get( "sv_allow_studio_attachment_angles", "0", CVAR_ARCHIVE, "enable calc angles for attachment points (on studio models)" );
	sv_allow_rotate_pushables = Cvar_Get( "sv_allow_rotate_pushables", "0", CVAR_ARCHIVE, "let the pushers rotate pushables with included origin-brush" );
	sv_allow_godmode = Cvar_Get( "sv_allow_godmode", "1", CVAR_LATCH, "allow players to be a god when sv_cheats is \"1\"" );
	sv_allow_noclip = Cvar_Get( "sv_allow_noclip", "1", CVAR_LATCH, "allow players to use noclip when sv_cheats is \"1\"" );
	sv_enttools_enable = Cvar_Get( "sv_enttools_enable", "0", CVAR_ARCHIVE | CVAR_PROTECTED, "Enable powerful and dangerous entity tools" );
	sv_enttools_maxfire = Cvar_Get( "sv_enttools_maxfire", "5", CVAR_ARCHIVE | CVAR_PROTECTED, "Limit ent_fire actions count to prevent flooding" );
	sv_validate_changelevel = Cvar_Get( "sv_validate_changelevel", "1", CVAR_ARCHIVE, "test change level for level-designer errors" );
	sv_clienttrace = Cvar_Get( "sv_clienttrace", "1", CVAR_SERVERNOTIFY, "scaling factor for client hitboxes" );
	sv_wallbounce = Cvar_Get( "sv_wallbounce", "1.0", CVAR_PHYSICINFO, "bounce factor for client with MOVETYPE_BOUNCE" );
	sv_spectatormaxspeed = Cvar_Get( "sv_spectatormaxspeed", "500", CVAR_PHYSICINFO, "spectator maxspeed" );
	sv_waterfriction = Cvar_Get( "sv_waterfriction", "1", CVAR_PHYSICINFO, "how fast you slow down in water" );
	sv_wateraccelerate = Cvar_Get( "sv_wateraccelerate", "10", CVAR_PHYSICINFO, "rate at which a player accelerates to sv_maxspeed while in the water" );
	sv_rollangle = Cvar_Get( "sv_rollangle", "0", CVAR_PHYSICINFO, "how much to tilt the view when strafing" );
	sv_rollspeed = Cvar_Get( "sv_rollspeed", "200", CVAR_PHYSICINFO, "how much strafing is necessary to tilt the view" );
	sv_airaccelerate = Cvar_Get("sv_airaccelerate", "10", CVAR_PHYSICINFO, "player accelleration in air" );
	sv_maxvelocity = Cvar_Get( "sv_maxvelocity", "2000", CVAR_PHYSICINFO, "max world velocity" );
	sv_gravity = Cvar_Get( "sv_gravity", "800", CVAR_PHYSICINFO, "world gravity" );
	sv_maxspeed = Cvar_Get( "sv_maxspeed", "320", CVAR_PHYSICINFO, "maximum speed a player can accelerate to when on ground");
	sv_accelerate = Cvar_Get( "sv_accelerate", "10", CVAR_PHYSICINFO, "rate at which a player accelerates to sv_maxspeed" );
	sv_friction = Cvar_Get( "sv_friction", "4", CVAR_PHYSICINFO, "how fast you slow down" );
	sv_edgefriction = Cvar_Get( "edgefriction", "2", CVAR_PHYSICINFO, "how much you slow down when nearing a ledge you might fall off" );
	sv_stopspeed = Cvar_Get( "sv_stopspeed", "100", CVAR_PHYSICINFO, "how fast you come to a complete stop" );
	sv_maxclients = Cvar_Get( "maxplayers", "1", CVAR_LATCH|CVAR_SERVERNOTIFY, "server clients limit" );
	sv_check_errors = Cvar_Get( "sv_check_errors", "0", CVAR_ARCHIVE, "check edicts for errors" );
	physinfo = Cvar_Get( "@physinfo", "0", CVAR_READ_ONLY, "" ); // use ->modified value only
	serverinfo = Cvar_Get( "@serverinfo", "0", CVAR_READ_ONLY, "" ); // use ->modified value only
	public_server = Cvar_Get ("public", "0", 0, "change server type from private to public" );
	sv_lighting_modulate = Cvar_Get( "r_lighting_modulate", "0.6", CVAR_ARCHIVE, "lightstyles modulate scale" );
	sv_reconnect_limit = Cvar_Get ("sv_reconnect_limit", "3", CVAR_ARCHIVE, "max reconnect attempts" );
	sv_failuretime = Cvar_Get( "sv_failuretime", "0.5", 0, "after this long without a packet from client, don't send any more until client starts sending again" );
	sv_unlag = Cvar_Get( "sv_unlag", "1", 0, "allow lag compensation on server-side" );
	sv_maxunlag = Cvar_Get( "sv_maxunlag", "0.5", 0, "max latency which can be interpolated" );
	sv_unlagpush = Cvar_Get( "sv_unlagpush", "0.0", 0, "unlag push bias" );
	sv_unlagsamples = Cvar_Get( "sv_unlagsamples", "1", 0, "max samples to interpolate" );
	sv_allow_upload = Cvar_Get( "sv_allow_upload", "1", 0, "allow uploading custom resources from clients" );
	sv_allow_download = Cvar_Get( "sv_allow_download", "0", CVAR_ARCHIVE, "allow clients to download missing resources" );
	sv_allow_fragment = Cvar_Get( "sv_allow_fragment", "0", CVAR_ARCHIVE, "allow direct download from server" );
	sv_downloadurl = Cvar_Get( "sv_downloadurl", "", CVAR_ARCHIVE, "custom fastdl server to pass to client" );
	sv_send_logos = Cvar_Get( "sv_send_logos", "1", 0, "send custom player decals to other clients" );
	sv_send_resources = Cvar_Get( "sv_send_resources", "1", 0, "send generic resources that are specified in 'mapname.res'" );
	sv_sendvelocity = Cvar_Get( "sv_sendvelocity", "1", CVAR_ARCHIVE, "force to send velocity for event_t structure across network" );
	sv_quakehulls = Cvar_Get( "sv_quakehulls", "0", CVAR_ARCHIVE, "using quake style hull select instead of half-life style hull select" );
	mp_consistency = Cvar_Get( "mp_consistency", "1", CVAR_SERVERNOTIFY, "enable consistency check in multiplayer" );
	clockwindow = Cvar_Get( "clockwindow", "0.5", 0, "timewindow to execute client moves" );
	sv_novis = Cvar_Get( "sv_novis", "0", 0, "disable server-side visibility checking" );
	sv_skipshield = Cvar_Get( "sv_skipshield", "0", CVAR_ARCHIVE, "skip shield hitbox");
	sv_trace_messages = Cvar_Get( "sv_trace_messages", "0", CVAR_ARCHIVE|CVAR_LATCH, "enable server usermessages tracing (good for developers)" );
	sv_corpse_solid = Cvar_Get( "sv_corpse_solid", "0", CVAR_ARCHIVE, "make corpses solid" );
	sv_fixmulticast = Cvar_Get( "sv_fixmulticast", "1", CVAR_ARCHIVE, "do not send multicast to not spawned clients" );
	sv_allow_compress = Cvar_Get( "sv_allow_compress", "1", CVAR_ARCHIVE, "allow Huffman compression on server" );
	sv_allow_split= Cvar_Get( "sv_allow_split", "1", CVAR_ARCHIVE, "allow splitting packets on server" );
	sv_maxpacket = Cvar_Get( "sv_maxpacket", "2000", CVAR_ARCHIVE, "limit cl_maxpacket for all clients" );
	sv_forcesimulating = Cvar_Get( "sv_forcesimulating", DEFAULT_SV_FORCESIMULATING, 0, "forcing world simulating when server don't have active players" );
	sv_nat = Cvar_Get( "sv_nat", "0", 0, "enable NAT bypass for this server" );

	sv_allow_joystick = Cvar_Get( "sv_allow_joystick", "1", CVAR_ARCHIVE, "allow connect with joystick enabled" );
	sv_allow_mouse = Cvar_Get( "sv_allow_mouse", "1", CVAR_ARCHIVE, "allow connect with mouse" );
	sv_allow_touch = Cvar_Get( "sv_allow_touch", "1", CVAR_ARCHIVE, "allow connect with touch controls" );
	sv_allow_vr = Cvar_Get( "sv_allow_vr", "1", CVAR_ARCHIVE, "allow connect from vr version" );
	sv_allow_noinputdevices = Cvar_Get( "sv_allow_noinputdevices", "1", CVAR_ARCHIVE, "allow connect from old versions without useragent" );

	sv_password = Cvar_Get( "sv_password", "", CVAR_PROTECTED, "server password. Leave blank or set to \"none\" if none" );

	sv_userinfo_enable_penalty = Cvar_Get( "sv_userinfo_enable_penalty", "1", CVAR_ARCHIVE, "enable penalty time for too fast userinfo updates(name, model, etc)" );
	sv_userinfo_penalty_time = Cvar_Get( "sv_userinfo_penalty_time", "0.3", CVAR_ARCHIVE, "initial penalty time" );
	sv_userinfo_penalty_multiplier = Cvar_Get( "sv_userinfo_penalty_multiplier", "2", CVAR_ARCHIVE, "penalty time multiplier" );
	sv_userinfo_penalty_attempts = Cvar_Get( "sv_userinfo_penalty_attempts", "4", CVAR_ARCHIVE, "if max attempts count was exceeded, penalty time will be increased" );

	Cmd_AddCommand( "download_resources", SV_DownloadResources_f, "try to download missing resources to server");

	Cmd_AddCommand( "logaddress", SV_SetLogAddress_f, "sets address and port for remote logging host" );
	Cmd_AddCommand( "log", SV_ServerLog_f, "enables logging to file" );

#ifdef XASH_64BIT
	Cmd_AddCommand( "str64stats", SV_PrintStr64Stats_f, "show 64 bit string pool stats" );
#endif

	SV_InitFilter();
	SV_ClearSaveDir ();	// delete all temporary *.hl files
	BF_Init( &net_message, "NetMessage", net_message_buffer, sizeof( net_message_buffer ));
}

/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage( char *message, qboolean reconnect )
{
	sv_client_t	*cl;
	byte		msg_buf[1024];
	sizebuf_t		msg;
	int		i;

	Q_memset( msg_buf, 0, 1024 );
	BF_Init( &msg, "FinalMessage", msg_buf, sizeof( msg_buf ));
	BF_WriteByte( &msg, svc_print );
	BF_WriteByte( &msg, PRINT_HIGH );
	BF_WriteString( &msg, va( "%s\n", message ));

	if( reconnect )
	{
		BF_WriteByte( &msg, svc_changing );

		if( sv.loadgame || svgame.globals->maxClients > 1 || sv.changelevel )
			BF_WriteOneBit( &msg, 1 ); // changelevel
		else BF_WriteOneBit( &msg, 0 );
	}
	else
	{
		BF_WriteByte( &msg, svc_disconnect );
	}

	// send it twice
	// stagger the packets to crutch operating system limited buffers
	for( i = 0, cl = svs.clients; i < svgame.globals->maxClients; i++, cl++ )
		if( cl->state >= cs_connected && !cl->fakeclient )
			Netchan_Transmit( &cl->netchan, BF_GetNumBytesWritten( &msg ), BF_GetData( &msg ));

	for( i = 0, cl = svs.clients; i < svgame.globals->maxClients; i++, cl++ )
		if( cl->state >= cs_connected && !cl->fakeclient )
			Netchan_Transmit( &cl->netchan, BF_GetNumBytesWritten( &msg ), BF_GetData( &msg ));
}

/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown( qboolean reconnect )
{
	// already freed
	if( !SV_Active( ))
	{
		return;
	}

	// rcon will be disconnected
	SV_EndRedirect();

	if( Host_IsDedicated() )
		MsgDev( D_INFO, "SV_Shutdown: %s\n", host.finalmsg );

	if( svs.clients )
		SV_FinalMessage( host.finalmsg, reconnect );

	if( public_server->integer && sv_maxclients->integer != 1 )
		Master_Shutdown();

	Sequence_PurgeEntries( true ); // clear Sequence

	SV_DeactivateServer ();

	// free current level
	Q_memset( &sv, 0, sizeof( sv ));
	Host_SetServerState( sv.state );

	// free server static data
	if( svs.clients )
	{
		Mem_Free( svs.clients );
		svs.clients = NULL;
	}

	if( svs.baselines )
	{
		Mem_Free( svs.baselines );
		svs.baselines = NULL;
	}

	if( svs.packet_entities )
	{
		Mem_Free( svs.packet_entities );
		svs.packet_entities = NULL;
		svs.num_client_entities = 0;
		svs.next_client_entities = 0;
	}

	svs.initialized = false;
}
