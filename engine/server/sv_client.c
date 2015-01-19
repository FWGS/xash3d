/*
sv_client.c - client interactions
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
#include "const.h"
#include "server.h"
#include "net_encode.h"
#include "net_api.h"

const char *clc_strings[11] =
{
	"clc_bad",
	"clc_nop",
	"clc_move",
	"clc_stringcmd",
	"clc_delta",
	"clc_resourcelist",
	"clc_userinfo",
	"clc_fileconsistency",
	"clc_voicedata",
	"clc_requestcvarvalue",
	"clc_requestcvarvalue2",
};

typedef struct ucmd_s
{
	const char	*name;
	void		(*func)( sv_client_t *cl );
} ucmd_t;

static int	g_userid = 1;

/*
=================
SV_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
void SV_GetChallenge( netadr_t from )
{
	int	i, oldest = 0;
	double	oldestTime;

	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for( i = 0; i < MAX_CHALLENGES; i++ )
	{
		if( !svs.challenges[i].connected && NET_CompareAdr( from, svs.challenges[i].adr ))
			break;

		if( svs.challenges[i].time < oldestTime )
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if( i == MAX_CHALLENGES )
	{
		// this is the first time this client has asked for a challenge
		svs.challenges[oldest].challenge = (rand()<<16) ^ rand();
		svs.challenges[oldest].adr = from;
		svs.challenges[oldest].time = host.realtime;
		svs.challenges[oldest].connected = false;
		i = oldest;
	}

	// send it back
	Netchan_OutOfBandPrint( NS_SERVER, svs.challenges[i].adr, "challenge %i", svs.challenges[i].challenge );
}

/*
==================
SV_DirectConnect

A connection request that did not come from the master
==================
*/
void SV_DirectConnect( netadr_t from )
{
	char		userinfo[MAX_INFO_STRING];
	sv_client_t	temp, *cl, *newcl;
	char		physinfo[512];
	int		i, edictnum;
	int		qport, version;
	int		count = 0;
	int		challenge;
	edict_t		*ent;

	version = Q_atoi( Cmd_Argv( 1 ));

	if( version != PROTOCOL_VERSION )
	{
		Netchan_OutOfBandPrint( NS_SERVER, from, "print\nServer uses protocol version %i.\n", PROTOCOL_VERSION );
		MsgDev( D_ERROR, "SV_DirectConnect: rejected connect from version %i\n", version );
		Netchan_OutOfBandPrint( NS_SERVER, from, "disconnect\n" );
		return;
	}

	qport = Q_atoi( Cmd_Argv( 2 ));
	challenge = Q_atoi( Cmd_Argv( 3 ));
	Q_strncpy( userinfo, Cmd_Argv( 4 ), sizeof( userinfo ));

	// quick reject
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state == cs_free )
			continue;

		if( NET_CompareBaseAdr( from, cl->netchan.remote_address ) && ( cl->netchan.qport == qport || from.port == cl->netchan.remote_address.port ))
		{
			if( !NET_IsLocalAddress( from ) && ( host.realtime - cl->lastconnect ) < sv_reconnect_limit->value )
			{
				MsgDev( D_INFO, "%s:reconnect rejected : too soon\n", NET_AdrToString( from ));
				Netchan_OutOfBandPrint( NS_SERVER, from, "disconnect\n" );
				return;
			}
			break;
		}
	}
		
	// see if the challenge is valid (LAN clients don't need to challenge)
	if( !NET_IsLocalAddress( from ))
	{
		for( i = 0; i < MAX_CHALLENGES; i++ )
		{
			if( NET_CompareAdr( from, svs.challenges[i].adr ))
			{
				if( challenge == svs.challenges[i].challenge )
					break; // valid challenge
			}
		}

		if( i == MAX_CHALLENGES )
		{
			Netchan_OutOfBandPrint( NS_SERVER, from, "print\nNo or bad challenge for address.\n" );
			Netchan_OutOfBandPrint( NS_SERVER, from, "disconnect\n" );
			return;
		}

		MsgDev( D_NOTE, "Client %i connecting with challenge %p\n", i, challenge );
		svs.challenges[i].connected = true;
	}

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey( userinfo, "ip", NET_AdrToString( from ));

	newcl = &temp;
	Q_memset( newcl, 0, sizeof( sv_client_t ));

	// if there is already a slot for this ip, reuse it
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state == cs_free )
			continue;

		if( NET_CompareBaseAdr( from, cl->netchan.remote_address ) && ( cl->netchan.qport == qport || from.port == cl->netchan.remote_address.port ))
		{
			MsgDev( D_INFO, "%s:reconnect\n", NET_AdrToString( from ));
			newcl = cl;
			goto gotnewcl;
		}
	}

	// find a client slot
	newcl = NULL;

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state == cs_free )
		{
			newcl = cl;
			break;
		}
	}

	if( !newcl )
	{
		Netchan_OutOfBandPrint( NS_SERVER, from, "print\nServer is full.\n" );
		MsgDev( D_INFO, "SV_DirectConnect: rejected a connection.\n" );
		Netchan_OutOfBandPrint( NS_SERVER, from, "disconnect\n" );
		return;
	}

	// build a new connection
	// accept the new client
gotnewcl:	
	// this is the only place a sv_client_t is ever initialized

	if( sv_maxclients->integer == 1 ) // save physinfo for singleplayer
		Q_strncpy( physinfo, newcl->physinfo, sizeof( physinfo ));

	*newcl = temp;

	if( sv_maxclients->integer == 1 ) // restore physinfo for singleplayer
		Q_strncpy( newcl->physinfo, physinfo, sizeof( physinfo ));

	svs.currentPlayer = newcl;
	svs.currentPlayerNum = (newcl - svs.clients);
	edictnum = svs.currentPlayerNum + 1;

	ent = EDICT_NUM( edictnum );
	newcl->edict = ent;
	newcl->challenge = challenge; // save challenge for checksumming
	newcl->frames = (client_frame_t *)Z_Malloc( sizeof( client_frame_t ) * SV_UPDATE_BACKUP );
	newcl->userid = g_userid++;	// create unique userid
	newcl->authentication_method = 2;

	// initailize netchan here because SV_DropClient will clear network buffer
	Netchan_Setup( NS_SERVER, &newcl->netchan, from, qport );
	BF_Init( &newcl->datagram, "Datagram", newcl->datagram_buf, sizeof( newcl->datagram_buf )); // datagram buf

	// get the game a chance to reject this connection or modify the userinfo
	if( !( SV_ClientConnect( ent, userinfo )))
	{
		if( *Info_ValueForKey( userinfo, "rejmsg" )) 
			Netchan_OutOfBandPrint( NS_SERVER, from, "print\n%s\nConnection refused.\n", Info_ValueForKey( userinfo, "rejmsg" ));
		else Netchan_OutOfBandPrint( NS_SERVER, from, "print\nConnection refused.\n" );

		MsgDev( D_ERROR, "SV_DirectConnect: game rejected a connection.\n");
		Netchan_OutOfBandPrint( NS_SERVER, from, "disconnect\n" );
		SV_DropClient( newcl );
		return;
	}

	// parse some info from the info strings
	SV_UserinfoChanged( newcl, userinfo );

	// send the connect packet to the client
	Netchan_OutOfBandPrint( NS_SERVER, from, "client_connect" );

	newcl->state = cs_connected;
	newcl->cl_updaterate = 0.05;	// 20 fps as default
	newcl->lastmessage = host.realtime;
	newcl->lastconnect = host.realtime;
	newcl->next_messagetime = host.realtime + newcl->cl_updaterate;
	newcl->delta_sequence = -1;

	// if this was the first client on the server, or the last client
	// the server can hold, send a heartbeat to the master.
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
		if( cl->state >= cs_connected ) count++;

	if( count == 1 || count == sv_maxclients->integer )
		svs.last_heartbeat = MAX_HEARTBEAT;
}

/*
==================
SV_DisconnectClient

Disconnect client callback
==================
*/
void SV_DisconnectClient( edict_t *pClient )
{
	if( !pClient ) return;

	svgame.dllFuncs.pfnClientDisconnect( pClient );

	// don't send to other clients
	pClient->v.modelindex = 0;

	if( pClient->pvPrivateData != NULL )
	{
		// NOTE: new interface can be missing
		if( svgame.dllFuncs2.pfnOnFreeEntPrivateData != NULL )
			svgame.dllFuncs2.pfnOnFreeEntPrivateData( pClient );

		// clear any dlls data but keep engine data
		Mem_Free( pClient->pvPrivateData );
		pClient->pvPrivateData = NULL;
	}
}

/*
==================
SV_FakeConnect

A connection request that came from the game module
==================
*/
edict_t *SV_FakeConnect( const char *netname )
{
	int		i, edictnum;
	char		userinfo[MAX_INFO_STRING];
	sv_client_t	temp, *cl, *newcl;
	edict_t		*ent;

	if( !netname ) netname = "";
	userinfo[0] = '\0';

	// setup fake client params
	Info_SetValueForKey( userinfo, "name", netname );
	Info_SetValueForKey( userinfo, "model", "gordon" );
	Info_SetValueForKey( userinfo, "topcolor", "0" );
	Info_SetValueForKey( userinfo, "bottomcolor", "0" );

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey( userinfo, "ip", "127.0.0.1" );

	// find a client slot
	newcl = &temp;
	Q_memset( newcl, 0, sizeof( sv_client_t ));

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state == cs_free )
		{
			newcl = cl;
			break;
		}
	}

	if( i == sv_maxclients->integer )
	{
		MsgDev( D_INFO, "SV_DirectConnect: rejected a connection.\n");
		return NULL;
	}

	// build a new connection
	// accept the new client
	// this is the only place a sv_client_t is ever initialized
	*newcl = temp;
	svs.currentPlayer = newcl;
	svs.currentPlayerNum = (newcl - svs.clients);
	edictnum = svs.currentPlayerNum + 1;

	if( newcl->frames )
		Mem_Free( newcl->frames );	// fakeclients doesn't have frames
	newcl->frames = NULL;

	ent = EDICT_NUM( edictnum );
	newcl->edict = ent;
	newcl->challenge = -1;		// fake challenge
	newcl->fakeclient = true;
	newcl->delta_sequence = -1;
	newcl->userid = g_userid++;		// create unique userid

	// get the game a chance to reject this connection or modify the userinfo
	if( !SV_ClientConnect( ent, userinfo ))
	{
		MsgDev( D_ERROR, "SV_DirectConnect: game rejected a connection.\n" );
		return NULL;
	}

	// parse some info from the info strings
	SV_UserinfoChanged( newcl, userinfo );

	MsgDev( D_NOTE, "Bot %i connecting with challenge %p\n", i, -1 );

	ent->v.flags |= FL_FAKECLIENT;	// mark it as fakeclient
	newcl->state = cs_spawned;
	newcl->lastmessage = host.realtime;	// don't timeout
	newcl->lastconnect = host.realtime;
	newcl->sendinfo = true;
	
	return ent;
}

/*
=====================
SV_ClientCconnect

QC code can rejected a connection for some reasons
e.g. ipban
=====================
*/
qboolean SV_ClientConnect( edict_t *ent, char *userinfo )
{
	qboolean	result = true;
	char	*pszName, *pszAddress;
	char	szRejectReason[MAX_INFO_STRING];

	// make sure we start with known default
	if( !sv.loadgame ) ent->v.flags = 0;
	szRejectReason[0] = '\0';

	pszName = Info_ValueForKey( userinfo, "name" );
	pszAddress = Info_ValueForKey( userinfo, "ip" );

	MsgDev( D_NOTE, "SV_ClientConnect()\n" );
	result = svgame.dllFuncs.pfnClientConnect( ent, pszName, pszAddress, szRejectReason );
	if( szRejectReason[0] ) Info_SetValueForKey( userinfo, "rejmsg", szRejectReason );

	return result;
}

/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing.
=====================
*/
void SV_DropClient( sv_client_t *drop )
{
	int	i;
	
	if( drop->state == cs_zombie )
		return;	// already dropped

	// add the disconnect
	if( !drop->fakeclient )
	{
		BF_WriteByte( &drop->netchan.message, svc_disconnect );
	}

	// let the game known about client state
	SV_DisconnectClient( drop->edict );

	drop->fakeclient = false;
	drop->hltv_proxy = false;
	drop->state = cs_zombie; // become free in a few seconds
	drop->name[0] = 0;

	if( drop->frames )
		Mem_Free( drop->frames );	// fakeclients doesn't have frames
	drop->frames = NULL;

	// throw away any residual garbage in the channel.
	Netchan_Clear( &drop->netchan );

	// send notification to all other clients
	SV_FullClientUpdate( drop, &sv.reliable_datagram );

	// if this was the last client on the server, send a heartbeat
	// to the master so it is known the server is empty
	// send a heartbeat now so the master will get up to date info
	// if there is already a slot for this ip, reuse it
	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		if( svs.clients[i].state >= cs_connected )
			break;
	}

	if( i == sv_maxclients->integer )
		svs.last_heartbeat = MAX_HEARTBEAT;
}

/*
==============================================================================

SVC COMMAND REDIRECT

==============================================================================
*/
void SV_BeginRedirect( netadr_t adr, int target, char *buffer, int buffersize, void (*flush))
{
	if( !target || !buffer || !buffersize || !flush )
		return;

	host.rd.target = target;
	host.rd.buffer = buffer;
	host.rd.buffersize = buffersize;
	host.rd.flush = flush;
	host.rd.address = adr;
	host.rd.buffer[0] = 0;
}

void SV_FlushRedirect( netadr_t adr, int dest, char *buf )
{
	if( svs.currentPlayer && svs.currentPlayer->fakeclient )
		return;

	switch( dest )
	{
	case RD_PACKET:
		Netchan_OutOfBandPrint( NS_SERVER, adr, "print\n%s", buf );
		break;
	case RD_CLIENT:
		if( !svs.currentPlayer ) return; // client not set
		BF_WriteByte( &svs.currentPlayer->netchan.message, svc_print );
		BF_WriteByte( &svs.currentPlayer->netchan.message, PRINT_HIGH );
		BF_WriteString( &svs.currentPlayer->netchan.message, buf );
		break;
	case RD_NONE:
		MsgDev( D_ERROR, "SV_FlushRedirect: %s: invalid destination\n", NET_AdrToString( adr ));
		break;
	}
}

void SV_EndRedirect( void )
{
	host.rd.flush( host.rd.address, host.rd.target, host.rd.buffer );

	host.rd.target = 0;
	host.rd.buffer = NULL;
	host.rd.buffersize = 0;
	host.rd.flush = NULL;
}

/*
===============
SV_StatusString

Builds the string that is sent as heartbeats and status replies
===============
*/
char *SV_StatusString( void )
{
	char		player[1024];
	static char	status[4096];
	int		statusLength;
	int		playerLength;
	sv_client_t	*cl;
	int		i;

	Q_strcpy( status, Cvar_Serverinfo( ));
	Q_strcat( status, "\n" );
	statusLength = Q_strlen( status );

	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		cl = &svs.clients[i];
		if( cl->state == cs_connected || cl->state == cs_spawned )
		{
			Q_sprintf( player, "%i %i \"%s\"\n", (int)cl->edict->v.frags, cl->ping, cl->name );
			playerLength = Q_strlen( player );
			if( statusLength + playerLength >= sizeof( status ))
				break; // can't hold any more

			Q_strcpy( status + statusLength, player );
			statusLength += playerLength;
		}
	}
	return status;
}

/*
===============
SV_GetClientIDString

Returns a pointer to a static char for most likely only printing.
===============
*/
const char *SV_GetClientIDString( sv_client_t *cl )
{
	static char	result[CS_SIZE];

	result[0] = '\0';

	if( !cl )
	{
		MsgDev( D_ERROR, "SV_GetClientIDString: invalid client\n" );
		return result;
	}

	if( cl->authentication_method == 0 )
	{
		// probably some old compatibility code.
		Q_snprintf( result, sizeof( result ), "%010lu", cl->WonID );
	}
	else if( cl->authentication_method == 2 )
	{
		if( NET_IsLocalAddress( cl->netchan.remote_address ))
		{
			Q_strncpy( result, "VALVE_ID_LOOPBACK", sizeof( result ));
		}
		else if( cl->WonID == 0 )
		{
			Q_strncpy( result, "VALVE_ID_PENDING", sizeof( result ));
		}
		else
		{
			Q_snprintf( result, sizeof( result ), "VALVE_%010lu", cl->WonID );
		}
	}
	else Q_strncpy( result, "UNKNOWN", sizeof( result ));

	return result;
}

/*
================
SV_Status

Responds with all the info that qplug or qspy can see
================
*/
void SV_Status( netadr_t from )
{
	Netchan_OutOfBandPrint( NS_SERVER, from, "print\n%s", SV_StatusString( ));
}

/*
================
SV_Ack

================
*/
void SV_Ack( netadr_t from )
{
	Msg( "ping %s\n", NET_AdrToString( from ));
}

/*
================
SV_Info

Responds with short info for broadcast scans
The second parameter should be the current protocol version number.
================
*/
void SV_Info( netadr_t from )
{
	char	string[MAX_INFO_STRING];
	int	i, count = 0;
	int	version;

	// ignore in single player
	if( sv_maxclients->integer == 1 )
		return;

	version = Q_atoi( Cmd_Argv( 1 ));
	string[0] = '\0';

	if( version != PROTOCOL_VERSION )
	{
		Q_snprintf( string, sizeof( string ), "%s: wrong version\n", hostname->string );
	}
	else
	{
		for( i = 0; i < sv_maxclients->integer; i++ )
			if( svs.clients[i].state >= cs_connected )
				count++;

		Info_SetValueForKey( string, "host", hostname->string );
		Info_SetValueForKey( string, "map", sv.name );
		Info_SetValueForKey( string, "dm", va( "%i", svgame.globals->deathmatch ));
		Info_SetValueForKey( string, "team", va( "%i", svgame.globals->teamplay ));
		Info_SetValueForKey( string, "coop", va( "%i", svgame.globals->coop ));
		Info_SetValueForKey( string, "numcl", va( "%i", count ));
		Info_SetValueForKey( string, "maxcl", va( "%i", sv_maxclients->integer ));
		Info_SetValueForKey( string, "gamedir", GI->gamefolder );
	}

	Netchan_OutOfBandPrint( NS_SERVER, from, "info\n%s", string );
}

/*
================
SV_BuildNetAnswer

Responds with long info for local and broadcast requests
================
*/
void SV_BuildNetAnswer( netadr_t from )
{
	char	string[MAX_INFO_STRING], answer[512];
	int	version, context, type;
	int	i, count = 0;

	// ignore in single player
	if( sv_maxclients->integer == 1 )
		return;

	version = Q_atoi( Cmd_Argv( 1 ));
	context = Q_atoi( Cmd_Argv( 2 ));
	type = Q_atoi( Cmd_Argv( 3 ));

	if( version != PROTOCOL_VERSION )
		return;

	if( type == NETAPI_REQUEST_PING )
	{
		Q_snprintf( answer, sizeof( answer ), "netinfo %i %i\n", context, type );
		Netchan_OutOfBandPrint( NS_SERVER, from, answer ); // no info string
	}
	else if( type == NETAPI_REQUEST_RULES )
	{
		// send serverinfo
		Q_snprintf( answer, sizeof( answer ), "netinfo %i %i %s\n", context, type, Cvar_Serverinfo( ));
		Netchan_OutOfBandPrint( NS_SERVER, from, answer ); // no info string
	}
	else if( type == NETAPI_REQUEST_PLAYERS )
	{
		string[0] = '\0';

		for( i = 0; i < sv_maxclients->integer; i++ )
		{
			if( svs.clients[i].state >= cs_connected )
			{
				edict_t *ed = svs.clients[i].edict;
				float time = host.realtime - svs.clients[i].lastconnect;
				Q_strncat( string, va( "%c\\%s\\%i\\%f\\", count, svs.clients[i].name, ed->v.frags, time ), sizeof( string )); 
				count++;
			}
		}

		// send playernames
		Q_snprintf( answer, sizeof( answer ), "netinfo %i %i %s\n", context, type, string );
		Netchan_OutOfBandPrint( NS_SERVER, from, answer ); // no info string
	}
	else if( type == NETAPI_REQUEST_DETAILS )
	{
		for( i = 0; i < sv_maxclients->integer; i++ )
			if( svs.clients[i].state >= cs_connected )
				count++;

		string[0] = '\0';
		Info_SetValueForKey( string, "hostname", hostname->string );
		Info_SetValueForKey( string, "gamedir", GI->gamefolder );
		Info_SetValueForKey( string, "current", va( "%i", count ));
		Info_SetValueForKey( string, "max", va( "%i", sv_maxclients->integer ));
		Info_SetValueForKey( string, "map", sv.name );

		// send serverinfo
		Q_snprintf( answer, sizeof( answer ), "netinfo %i %i %s\n", context, type, string );
		Netchan_OutOfBandPrint( NS_SERVER, from, answer ); // no info string
	}
}

/*
================
SV_Ping

Just responds with an acknowledgement
================
*/
void SV_Ping( netadr_t from )
{
	Netchan_OutOfBandPrint( NS_SERVER, from, "ack" );
}

/*
================
Rcon_Validate
================
*/
qboolean Rcon_Validate( void )
{
	if( !Q_strlen( rcon_password->string ))
		return false;
	if( Q_strcmp( Cmd_Argv( 1 ), rcon_password->string ))
		return false;
	return true;
}

/*
===============
SV_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/
void SV_RemoteCommand( netadr_t from, sizebuf_t *msg )
{
	char		remaining[1024];
	static char	outputbuf[2048];
	int		i;

	MsgDev( D_INFO, "Rcon from %s:\n%s\n", NET_AdrToString( from ), BF_GetData( msg ) + 4 );
	SV_BeginRedirect( from, RD_PACKET, outputbuf, sizeof( outputbuf ) - 16, SV_FlushRedirect );

	if( Rcon_Validate( ))
	{
		remaining[0] = 0;
		for( i = 2; i < Cmd_Argc(); i++ )
		{
			Q_strcat( remaining, Cmd_Argv( i ));
			Q_strcat( remaining, " " );
		}
		Cmd_ExecuteString( remaining, src_command );
	}
	else MsgDev( D_ERROR, "Bad rcon_password.\n" );

	SV_EndRedirect();
}

/*
===================
SV_CalcPing

recalc ping on current client
===================
*/
int SV_CalcPing( sv_client_t *cl )
{
	float		ping = 0;
	int		i, count;
	client_frame_t	*frame;

	// bots don't have a real ping
	if( cl->fakeclient )
		return 5;

	count = 0;

	for( i = 0; i < SV_UPDATE_BACKUP; i++ )
	{
		frame = &cl->frames[(cl->netchan.incoming_acknowledged - (i + 1)) & SV_UPDATE_MASK];

		if( frame->raw_ping > 0 )
		{
			ping += frame->raw_ping;
			count++;
		}
	}

	if( !count )
		return 0;

	return (( ping / count ) * 1000 );
}

/*
===================
SV_EstablishTimeBase

Finangles latency and the like. 
===================
*/
void SV_EstablishTimeBase( sv_client_t *cl, usercmd_t *ucmd, int numdrops, int numbackup, int newcmds )
{
	double	start;
	double	end;
	int	i;

	start = 0;
	end = sv.time + host.frametime;

	if( numdrops < 24 )
	{
		while( numdrops > numbackup )
		{
			start += cl->lastcmd.msec / 1000.0;
			numdrops--;
		}

		while( numdrops > 0 )
		{
			start += ucmd[numdrops + newcmds - 1].msec / 1000.0;
			numdrops--;
		}
	}

	for( i = newcmds - 1; i >= 0; i-- )
	{
		start += ucmd[i].msec / 1000.0;
	}

	cl->timebase = end - start;
}

/*
===================
SV_FullClientUpdate

Writes all update values to a bitbuf
===================
*/
void SV_FullClientUpdate( sv_client_t *cl, sizebuf_t *msg )
{
	char	info[MAX_INFO_STRING];
	int	i;	

	i = cl - svs.clients;

	BF_WriteByte( msg, svc_updateuserinfo );
	BF_WriteUBitLong( msg, i, MAX_CLIENT_BITS );

	if( cl->name[0] )
	{
		BF_WriteOneBit( msg, 1 );

		Q_strncpy( info, cl->userinfo, sizeof( info ));

		// remove server passwords, etc.
		Info_RemovePrefixedKeys( info, '_' );
		BF_WriteString( msg, info );
	}
	else BF_WriteOneBit( msg, 0 );
}

/*
===================
SV_RefreshUserinfo

===================
*/
void SV_RefreshUserinfo( void )
{
	int		i;
	sv_client_t	*cl;

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
		if( cl->state >= cs_connected ) cl->sendinfo = true;
}

/*
===================
SV_FullUpdateMovevars

this is send all movevars values when client connected
otherwise see code SV_UpdateMovevars()
===================
*/
void SV_FullUpdateMovevars( sv_client_t *cl, sizebuf_t *msg )
{
	movevars_t	nullmovevars;

	Q_memset( &nullmovevars, 0, sizeof( nullmovevars ));
	MSG_WriteDeltaMovevars( msg, &nullmovevars, &svgame.movevars );
}

/*
===================
SV_ShouldUpdatePing

this calls SV_CalcPing, and returns true
if this person needs ping data.
===================
*/
qboolean SV_ShouldUpdatePing( sv_client_t *cl )
{
	if( !cl->hltv_proxy )
	{
		SV_CalcPing( cl );
		return cl->lastcmd.buttons & IN_SCORE;	// they are viewing the scoreboard.  Send them pings.
	}

	if( host.realtime > cl->next_checkpingtime )
	{
		cl->next_checkpingtime = host.realtime + 2.0;
		return true;
	}

	return false;
}

/*
===================
SV_IsPlayerIndex

===================
*/
qboolean SV_IsPlayerIndex( int idx )
{
	if( idx > 0 && idx <= sv_maxclients->integer )
		return true;
	return false;
}

/*
===================
SV_GetPlayerStats

This function and its static vars track some of the networking
conditions.  I haven't bothered to trace it beyond that, because
this fucntion sucks pretty badly.
===================
*/
void SV_GetPlayerStats( sv_client_t *cl, int *ping, int *packet_loss )
{
	static int	last_ping[MAX_CLIENTS];
	static int	last_loss[MAX_CLIENTS];
	int		i;

	i = cl - svs.clients;

	if( cl->next_checkpingtime < host.realtime )
	{
		cl->next_checkpingtime = host.realtime + 2.0;
		last_ping[i] = SV_CalcPing( cl );
		last_loss[i] = cl->packet_loss;
	}

	if( ping ) *ping = last_ping[i];
	if( packet_loss ) *packet_loss = last_loss[i];
}

/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void SV_PutClientInServer( edict_t *ent )
{
	sv_client_t	*client;

	client = SV_ClientFromEdict( ent, true );
	ASSERT( client != NULL );

	if( !sv.loadgame )
	{	
		client->hltv_proxy = Q_atoi( Info_ValueForKey( client->userinfo, "hltv" )) ? true : false;

		if( client->hltv_proxy )
			ent->v.flags |= FL_PROXY;
		else ent->v.flags = 0;

		ent->v.netname = MAKE_STRING( client->name );

		// fisrt entering
		svgame.globals->time = sv.time;
		svgame.dllFuncs.pfnClientPutInServer( ent );

		if( sv.background )	// don't attack player in background mode
			ent->v.flags |= (FL_GODMODE|FL_NOTARGET);

		client->pViewEntity = NULL; // reset pViewEntity

		if( svgame.globals->cdAudioTrack )
		{
			BF_WriteByte( &client->netchan.message, svc_stufftext );
			BF_WriteString( &client->netchan.message, va( "cd loop %3d\n", svgame.globals->cdAudioTrack ));
			svgame.globals->cdAudioTrack = 0;
		}
	}
	else
	{
		// enable dev-mode to prevent crash cheat-protecting from Invasion mod
		if( ent->v.flags & (FL_GODMODE|FL_NOTARGET) && !Q_stricmp( GI->gamefolder, "invasion" ))
			SV_ExecuteClientCommand( client, "test\n" );

		// NOTE: we needs to setup angles on restore here
		if( ent->v.fixangle == 1 )
		{
			BF_WriteByte( &client->netchan.message, svc_setangle );
			BF_WriteBitAngle( &client->netchan.message, ent->v.angles[0], 16 );
			BF_WriteBitAngle( &client->netchan.message, ent->v.angles[1], 16 );
			BF_WriteBitAngle( &client->netchan.message, ent->v.angles[2], 16 );
			ent->v.fixangle = 0;
		}
		ent->v.effects |= EF_NOINTERP;

		// reset weaponanim
		BF_WriteByte( &client->netchan.message, svc_weaponanim );
		BF_WriteByte( &client->netchan.message, 0 );
		BF_WriteByte( &client->netchan.message, 0 );

		// trigger_camera restored here
		if( sv.viewentity > 0 && sv.viewentity < GI->max_edicts )
			client->pViewEntity = EDICT_NUM( sv.viewentity );
		else client->pViewEntity = NULL;
	}

	// reset client times
	client->last_cmdtime = 0.0;
	client->last_movetime = 0.0;
	client->next_movetime = 0.0;

	if( !client->fakeclient )
	{
		int	viewEnt;

		// resend the signon
		BF_WriteBits( &client->netchan.message, BF_GetData( &sv.signon ), BF_GetNumBitsWritten( &sv.signon ));

		if( client->pViewEntity )
			viewEnt = NUM_FOR_EDICT( client->pViewEntity );
		else viewEnt = NUM_FOR_EDICT( client->edict );
	
		BF_WriteByte( &client->netchan.message, svc_setview );
		BF_WriteWord( &client->netchan.message, viewEnt );
	}

	// clear any temp states
	sv.changelevel = false;
	sv.loadgame = false;
	sv.paused = false;

	if( sv_maxclients->integer == 1 ) // singleplayer profiler
		MsgDev( D_INFO, "level loaded at %.2f sec\n", Sys_DoubleTime() - svs.timestart );
}

/*
==================
SV_TogglePause
==================
*/
void SV_TogglePause( const char *msg )
{
	if( sv.background ) return;

	sv.paused ^= 1;

	if( msg ) SV_BroadcastPrintf( PRINT_HIGH, "%s", msg );

	// send notification to all clients
	BF_WriteByte( &sv.reliable_datagram, svc_setpause );
	BF_WriteOneBit( &sv.reliable_datagram, sv.paused );
}

/*
============================================================

CLIENT COMMAND EXECUTION

============================================================
*/
/*
================
SV_New_f

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_New_f( sv_client_t *cl )
{
	int	playernum;
	edict_t	*ent;

	if( cl->state != cs_connected )
	{
		MsgDev( D_INFO, "new is not valid from the console\n" );
		return;
	}

	playernum = cl - svs.clients;

	// send the serverdata
	BF_WriteByte( &cl->netchan.message, svc_serverdata );
	BF_WriteLong( &cl->netchan.message, PROTOCOL_VERSION );
	BF_WriteLong( &cl->netchan.message, svs.spawncount );
	BF_WriteLong( &cl->netchan.message, sv.checksum );
	BF_WriteByte( &cl->netchan.message, playernum );
	BF_WriteByte( &cl->netchan.message, svgame.globals->maxClients );
	BF_WriteWord( &cl->netchan.message, svgame.globals->maxEntities );
	BF_WriteString( &cl->netchan.message, sv.name );
	BF_WriteString( &cl->netchan.message, STRING( EDICT_NUM( 0 )->v.message )); // Map Message
	BF_WriteOneBit( &cl->netchan.message, sv.background ); // tell client about background map
	BF_WriteString( &cl->netchan.message, GI->gamefolder );
	BF_WriteLong( &cl->netchan.message, host.features );

	// refresh userinfo on spawn
	SV_RefreshUserinfo();

	// game server
	if( sv.state == ss_active )
	{
		// set up the entity for the client
		ent = EDICT_NUM( playernum + 1 );
		cl->edict = ent;

		// NOTE: custom resources download is disabled until is done
		if( /*sv_maxclients->integer ==*/ 1 )
		{
			Q_memset( &cl->lastcmd, 0, sizeof( cl->lastcmd ));

			// begin fetching modellist
			BF_WriteByte( &cl->netchan.message, svc_stufftext );
			BF_WriteString( &cl->netchan.message, va( "cmd modellist %i %i\n", svs.spawncount, 0 ));
		}
		else
		{
			// request resource list
			BF_WriteByte( &cl->netchan.message, svc_stufftext );
			BF_WriteString( &cl->netchan.message, va( "cmd getresourelist\n" ));
		}
	}
}

/*
==================
SV_ContinueLoading_f
==================
*/
void SV_ContinueLoading_f( sv_client_t *cl )
{
	if( cl->state != cs_connected )
	{
		MsgDev( D_INFO, "continueloading is not valid from the console\n" );
		return;
	}

	Q_memset( &cl->lastcmd, 0, sizeof( cl->lastcmd ));

	// begin fetching modellist
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, va( "cmd modellist %i %i\n", svs.spawncount, 0 ));
}

/*
=======================
SV_SendResourceList

NOTE: Sending the list of cached resources.
g-cont. this is fucking big message!!! i've rewriting this code
=======================
*/
void SV_SendResourceList_f( sv_client_t *cl )
{
	int		index = 0;
	int		rescount = 0;
	resourcelist_t	reslist;	// g-cont. what about stack???
	size_t		msg_size;

	Q_memset( &reslist, 0, sizeof( resourcelist_t ));

	reslist.restype[rescount] = t_world; // terminator
	Q_strcpy( reslist.resnames[rescount], "NULL" );
	rescount++;

	for( index = 1; index < MAX_MODELS && sv.model_precache[index][0]; index++ )
	{
		if( sv.model_precache[index][0] == '*' ) // internal bmodel
			continue;

		reslist.restype[rescount] = t_model;
		Q_strcpy( reslist.resnames[rescount], sv.model_precache[index] );
		rescount++;
	}

	for( index = 1; index < MAX_SOUNDS && sv.sound_precache[index][0]; index++ )
	{
		reslist.restype[rescount] = t_sound;
		Q_strcpy( reslist.resnames[rescount], sv.sound_precache[index] );
		rescount++;
	}

	for( index = 1; index < MAX_EVENTS && sv.event_precache[index][0]; index++ )
	{
		reslist.restype[rescount] = t_eventscript;
		Q_strcpy( reslist.resnames[rescount], sv.event_precache[index] );
		rescount++;
	}

	for( index = 1; index < MAX_CUSTOM && sv.files_precache[index][0]; index++ )
	{
		reslist.restype[rescount] = t_generic;
		Q_strcpy( reslist.resnames[rescount], sv.files_precache[index] );
		rescount++;
	}

	msg_size = BF_GetRealBytesWritten( &cl->netchan.message ); // start

	BF_WriteByte( &cl->netchan.message, svc_resourcelist );
	BF_WriteWord( &cl->netchan.message, rescount );

	for( index = 1; index < rescount; index++ )
	{
		BF_WriteWord( &cl->netchan.message, reslist.restype[index] );
		BF_WriteString( &cl->netchan.message, reslist.resnames[index] );
	}

	Msg( "Count res: %d\n", rescount );
	Msg( "ResList size: %s\n", Q_memprint( BF_GetRealBytesWritten( &cl->netchan.message ) - msg_size ));
}

/*
==================
SV_WriteModels_f
==================
*/
void SV_WriteModels_f( sv_client_t *cl )
{
	int	start;
	string	cmd;

	if( cl->state != cs_connected )
	{
		MsgDev( D_INFO, "modellist is not valid from the console\n" );
		return;
	}

	// handle the case of a level changing while a client was connecting
	if( Q_atoi( Cmd_Argv( 1 )) != svs.spawncount )
	{
		MsgDev( D_INFO, "modellist from different level\n" );
		SV_New_f( cl );
		return;
	}
	
	start = Q_atoi( Cmd_Argv( 2 ));

	// write a packet full of data
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( NET_MAX_PAYLOAD / 2 ) && start < MAX_MODELS )
	{
		if( sv.model_precache[start][0] )
		{
			BF_WriteByte( &cl->netchan.message, svc_modelindex );
			BF_WriteUBitLong( &cl->netchan.message, start, MAX_MODEL_BITS );
			BF_WriteString( &cl->netchan.message, sv.model_precache[start] );
		}
		start++;
	}

	if( start == MAX_MODELS ) Q_snprintf( cmd, MAX_STRING, "cmd soundlist %i %i\n", svs.spawncount, 0 );
	else Q_snprintf( cmd, MAX_STRING, "cmd modellist %i %i\n", svs.spawncount, start );

	// send next command
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, cmd );
}

/*
==================
SV_WriteSounds_f
==================
*/
void SV_WriteSounds_f( sv_client_t *cl )
{
	int	start;
	string	cmd;

	if( cl->state != cs_connected )
	{
		MsgDev( D_INFO, "soundlist is not valid from the console\n" );
		return;
	}

	// handle the case of a level changing while a client was connecting
	if( Q_atoi( Cmd_Argv( 1 )) != svs.spawncount )
	{
		MsgDev( D_INFO, "soundlist from different level\n" );
		SV_New_f( cl );
		return;
	}
	
	start = Q_atoi( Cmd_Argv( 2 ));

	// write a packet full of data
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( NET_MAX_PAYLOAD / 2 ) && start < MAX_SOUNDS )
	{
		if( sv.sound_precache[start][0] )
		{
			BF_WriteByte( &cl->netchan.message, svc_soundindex );
			BF_WriteUBitLong( &cl->netchan.message, start, MAX_SOUND_BITS );
			BF_WriteString( &cl->netchan.message, sv.sound_precache[start] );
		}
		start++;
	}

	if( start == MAX_SOUNDS ) Q_snprintf( cmd, MAX_STRING, "cmd eventlist %i %i\n", svs.spawncount, 0 );
	else Q_snprintf( cmd, MAX_STRING, "cmd soundlist %i %i\n", svs.spawncount, start );

	// send next command
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, cmd );
}

/*
==================
SV_WriteEvents_f
==================
*/
void SV_WriteEvents_f( sv_client_t *cl )
{
	int	start;
	string	cmd;

	if( cl->state != cs_connected )
	{
		MsgDev( D_INFO, "eventlist is not valid from the console\n" );
		return;
	}

	// handle the case of a level changing while a client was connecting
	if( Q_atoi( Cmd_Argv( 1 )) != svs.spawncount )
	{
		MsgDev( D_INFO, "eventlist from different level\n" );
		SV_New_f( cl );
		return;
	}
	
	start = Q_atoi( Cmd_Argv( 2 ));

	// write a packet full of data
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( NET_MAX_PAYLOAD / 2 ) && start < MAX_EVENTS )
	{
		if( sv.event_precache[start][0] )
		{
			BF_WriteByte( &cl->netchan.message, svc_eventindex );
			BF_WriteUBitLong( &cl->netchan.message, start, MAX_EVENT_BITS );
			BF_WriteString( &cl->netchan.message, sv.event_precache[start] );
		}
		start++;
	}

	if( start == MAX_EVENTS ) Q_snprintf( cmd, MAX_STRING, "cmd lightstyles %i %i\n", svs.spawncount, 0 );
	else Q_snprintf( cmd, MAX_STRING, "cmd eventlist %i %i\n", svs.spawncount, start );

	// send next command
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, cmd );
}

/*
==================
SV_WriteLightstyles_f
==================
*/
void SV_WriteLightstyles_f( sv_client_t *cl )
{
	int	start;
	string	cmd;

	if( cl->state != cs_connected )
	{
		MsgDev( D_INFO, "lightstyles is not valid from the console\n" );
		return;
	}

	// handle the case of a level changing while a client was connecting
	if( Q_atoi( Cmd_Argv( 1 )) != svs.spawncount )
	{
		MsgDev( D_INFO, "lightstyles from different level\n" );
		SV_New_f( cl );
		return;
	}
	
	start = Q_atoi( Cmd_Argv( 2 ));

	// write a packet full of data
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( NET_MAX_PAYLOAD / 2 ) && start < MAX_LIGHTSTYLES )
	{
		if( sv.lightstyles[start].pattern[0] )
		{
			BF_WriteByte( &cl->netchan.message, svc_lightstyle );
			BF_WriteByte( &cl->netchan.message, start );
			BF_WriteString( &cl->netchan.message, sv.lightstyles[start].pattern );
			BF_WriteFloat( &cl->netchan.message, sv.lightstyles[start].time );
		}
		start++;
	}

	if( start == MAX_LIGHTSTYLES ) Q_snprintf( cmd, MAX_STRING, "cmd usermsgs %i %i\n", svs.spawncount, 0 );
	else Q_snprintf( cmd, MAX_STRING, "cmd lightstyles %i %i\n", svs.spawncount, start );

	// send next command
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, cmd );
}

/*
==================
SV_UserMessages_f
==================
*/
void SV_UserMessages_f( sv_client_t *cl )
{
	int		start;
	sv_user_message_t	*message;
	string		cmd;

	if( cl->state != cs_connected )
	{
		MsgDev( D_INFO, "usermessages is not valid from the console\n" );
		return;
	}
	
	// handle the case of a level changing while a client was connecting
	if( Q_atoi( Cmd_Argv( 1 )) != svs.spawncount )
	{
		MsgDev( D_INFO, "usermessages from different level\n" );
		SV_New_f( cl );
		return;
	}
	
	start = Q_atoi( Cmd_Argv( 2 ));

	// write a packet full of data
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( NET_MAX_PAYLOAD / 2 ) && start < MAX_USER_MESSAGES )
	{
		message = &svgame.msg[start];
		if( message->name[0] )
		{
			BF_WriteByte( &cl->netchan.message, svc_usermessage );
			BF_WriteByte( &cl->netchan.message, message->number );
			BF_WriteByte( &cl->netchan.message, (byte)message->size );
			BF_WriteString( &cl->netchan.message, message->name );
		}
		start++;
	}

	if( start == MAX_USER_MESSAGES ) Q_snprintf( cmd, MAX_STRING, "cmd deltainfo %i 0 0\n", svs.spawncount );
	else Q_snprintf( cmd, MAX_STRING, "cmd usermsgs %i %i\n", svs.spawncount, start );

	// send next command
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, cmd );
}

/*
==================
SV_DeltaInfo_f
==================
*/
void SV_DeltaInfo_f( sv_client_t *cl )
{
	delta_info_t	*dt;
	string		cmd;
	int		tableIndex;
	int		fieldIndex;

	if( cl->state != cs_connected )
	{
		MsgDev( D_INFO, "deltainfo is not valid from the console\n" );
		return;
	}
	
	// handle the case of a level changing while a client was connecting
	if( Q_atoi( Cmd_Argv( 1 )) != svs.spawncount )
	{
		MsgDev( D_INFO, "deltainfo from different level\n" );
		SV_New_f( cl );
		return;
	}
	
	tableIndex = Q_atoi( Cmd_Argv( 2 ));
	fieldIndex = Q_atoi( Cmd_Argv( 3 ));

	// write a packet full of data
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( NET_MAX_PAYLOAD / 2 ) && tableIndex < Delta_NumTables( ))
	{
		dt = Delta_FindStructByIndex( tableIndex );

		for( ; fieldIndex < dt->numFields; fieldIndex++ )
		{
			Delta_WriteTableField( &cl->netchan.message, tableIndex, &dt->pFields[fieldIndex] );

			// it's time to send another portion
			if( BF_GetNumBytesWritten( &cl->netchan.message ) >= ( NET_MAX_PAYLOAD / 2 ))
				break;
		}

		if( fieldIndex == dt->numFields )
		{
			// go to the next table
			fieldIndex = 0;
			tableIndex++;
		}
	}

	if( tableIndex == Delta_NumTables() )
	{
		// send movevars here because we need loading skybox early than HLFX 0.6 may override him
		SV_FullUpdateMovevars( cl, &cl->netchan.message );
		Q_snprintf( cmd, MAX_STRING, "cmd baselines %i %i\n", svs.spawncount, 0 );
	}
	else Q_snprintf( cmd, MAX_STRING, "cmd deltainfo %i %i %i\n", svs.spawncount, tableIndex, fieldIndex );

	// send next command
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, cmd );
}

/*
==================
SV_Baselines_f
==================
*/
void SV_Baselines_f( sv_client_t *cl )
{
	int		start;
	entity_state_t	*base, nullstate;
	string		cmd;

	if( cl->state != cs_connected )
	{
		MsgDev( D_INFO, "baselines is not valid from the console\n" );
		return;
	}
	
	// handle the case of a level changing while a client was connecting
	if( Q_atoi( Cmd_Argv( 1 )) != svs.spawncount )
	{
		MsgDev( D_INFO, "baselines from different level\n" );
		SV_New_f( cl );
		return;
	}
	
	start = Q_atoi( Cmd_Argv( 2 ));

	Q_memset( &nullstate, 0, sizeof( nullstate ));

	// write a packet full of data
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( NET_MAX_PAYLOAD / 2 ) && start < svgame.numEntities )
	{
		base = &svs.baselines[start];
		if( base->number && ( base->modelindex || base->effects != EF_NODRAW ))
		{
			BF_WriteByte( &cl->netchan.message, svc_spawnbaseline );
			MSG_WriteDeltaEntity( &nullstate, base, &cl->netchan.message, true, SV_IsPlayerIndex( base->number ), sv.time );
		}
		start++;
	}

	if( start == svgame.numEntities ) Q_snprintf( cmd, MAX_STRING, "precache %i\n", svs.spawncount );
	else Q_snprintf( cmd, MAX_STRING, "cmd baselines %i %i\n", svs.spawncount, start );

	// send next command
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, cmd );
}

/*
==================
SV_Begin_f
==================
*/
void SV_Begin_f( sv_client_t *cl )
{
	if( cl->state != cs_connected )
	{
		MsgDev( D_INFO, "begin is not valid from the console\n" );
		return;
	}

	// handle the case of a level changing while a client was connecting
	if( Q_atoi( Cmd_Argv( 1 )) != svs.spawncount )
	{
		Msg( "begin from different level\n" );
		SV_New_f( cl );
		return;
	}

	cl->state = cs_spawned;
	SV_PutClientInServer( cl->edict );

	// if we are paused, tell the client
	if( sv.paused )
	{
		BF_WriteByte( &sv.reliable_datagram, svc_setpause );
		BF_WriteByte( &sv.reliable_datagram, sv.paused );
		SV_ClientPrintf( cl, PRINT_HIGH, "Server is paused.\n" );
	}
}

/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately
=================
*/
void SV_Disconnect_f( sv_client_t *cl )
{
	SV_DropClient( cl );	
}

/*
==================
SV_ShowServerinfo_f

Dumps the serverinfo info string
==================
*/
void SV_ShowServerinfo_f( sv_client_t *cl )
{
	Info_Print( Cvar_Serverinfo( ));
}

/*
==================
SV_Pause_f
==================
*/
void SV_Pause_f( sv_client_t *cl )
{
	string	message;

	if( UI_CreditsActive( )) return;

	if( !sv_pausable->integer )
	{
		SV_ClientPrintf( cl, PRINT_HIGH, "Pause not allowed.\n" );
		return;
	}

	if( cl->hltv_proxy )
	{
		SV_ClientPrintf( cl, PRINT_HIGH, "Spectators can not pause.\n" );
		return;
	}

	if( !sv.paused ) Q_snprintf( message, MAX_STRING, "^2%s^7 paused the game\n", cl->name );
	else Q_snprintf( message, MAX_STRING, "^2%s^7 unpaused the game\n", cl->name );

	SV_TogglePause( message );
}


/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C freindly form.
=================
*/
void SV_UserinfoChanged( sv_client_t *cl, const char *userinfo )
{
	int		i, dupc = 1;
	edict_t		*ent = cl->edict;
	string		temp1, temp2;	
	sv_client_t	*current;
	char		*val;

	if( !userinfo || !userinfo[0] ) return; // ignored

	Q_strncpy( cl->userinfo, userinfo, sizeof( cl->userinfo ));

	val = Info_ValueForKey( cl->userinfo, "name" );
	Q_strncpy( temp2, val, sizeof( temp2 ));
	TrimSpace( temp2, temp1 );

	if( !Q_stricmp( temp1, "console" )) // keyword came from OSHLDS
	{
		Info_SetValueForKey( cl->userinfo, "name", "unnamed" );
		val = Info_ValueForKey( cl->userinfo, "name" );
	}
	else if( Q_strcmp( temp1, val ))
	{
		Info_SetValueForKey( cl->userinfo, "name", temp1 );
		val = Info_ValueForKey( cl->userinfo, "name" );
	}

	// check to see if another user by the same name exists
	while( 1 )
	{
		for( i = 0, current = svs.clients; i < sv_maxclients->integer; i++, current++ )
		{
			if( current == cl || current->state != cs_spawned )
				continue;

			if( !Q_stricmp( current->name, val ))
				break;
		}

		if( i != sv_maxclients->integer )
		{
			// dup name
			Q_snprintf( temp2, sizeof( temp2 ), "%s (%u)", temp1, dupc++ );
			Info_SetValueForKey( cl->userinfo, "name", temp2 );
			val = Info_ValueForKey( cl->userinfo, "name" );
			Q_strcpy( cl->name, temp2 );
		}
		else
		{
			if( dupc == 1 ) // unchanged
				Q_strcpy( cl->name, temp1 );
			break;
		}
	}

	// rate command
	val = Info_ValueForKey( cl->userinfo, "rate" );
	if( Q_strlen( val ))
		cl->netchan.rate = bound( MIN_RATE, Q_atoi( val ), MAX_RATE );
	else cl->netchan.rate = DEFAULT_RATE;
	
	// msg command
	val = Info_ValueForKey( cl->userinfo, "msg" );
	if( Q_strlen( val )) cl->messagelevel = Q_atoi( val );

	cl->local_weapons = Q_atoi( Info_ValueForKey( cl->userinfo, "cl_lw" )) ? true : false;
	cl->lag_compensation = Q_atoi( Info_ValueForKey( cl->userinfo, "cl_lc" )) ? true : false;

	val = Info_ValueForKey( cl->userinfo, "cl_updaterate" );

	if( Q_strlen( val ))
	{
		int i = bound( 10, Q_atoi( val ), 300 );
		cl->cl_updaterate = 1.0f / i;
	}

	if( sv_maxclients->integer > 1 )
	{
		const char *model = Info_ValueForKey( cl->userinfo, "model" );

		// apply custom playermodel
		if( Q_strlen( model ) && Q_stricmp( model, "player" ))
		{
			const char *path = va( "models/player/%s/%s.mdl", model, model );
			if( FS_FileExists( path, false ))
			{
				Mod_RegisterModel( path, SV_ModelIndex( path )); // register model
				SV_SetModel( ent, path );
				cl->modelindex = ent->v.modelindex;
			}
			else cl->modelindex = 0;
		}
		else cl->modelindex = 0;
	}
	else cl->modelindex = 0;

	// call prog code to allow overrides
	svgame.dllFuncs.pfnClientUserInfoChanged( cl->edict, cl->userinfo );
	ent->v.netname = MAKE_STRING( cl->name );

	if( cl->state >= cs_connected ) cl->sendinfo = true; // needs for update client info 
}

/*
==================
SV_UpdateUserinfo_f
==================
*/
static void SV_UpdateUserinfo_f( sv_client_t *cl )
{
	SV_UserinfoChanged( cl, Cmd_Argv( 1 ));
}

/*
==================
SV_Noclip_f
==================
*/
static void SV_Noclip_f( sv_client_t *cl )
{
	edict_t	*pEntity = cl->edict;

	if( !Cvar_VariableInteger( "sv_cheats" ) || sv.background )
		return;

	if( pEntity->v.movetype != MOVETYPE_NOCLIP )
	{
		pEntity->v.movetype = MOVETYPE_NOCLIP;
		SV_ClientPrintf( cl, PRINT_HIGH, "noclip ON\n" );
	}
	else
	{
		pEntity->v.movetype =  MOVETYPE_WALK;
		SV_ClientPrintf( cl, PRINT_HIGH, "noclip OFF\n" );
	}
}

/*
==================
SV_Godmode_f
==================
*/
static void SV_Godmode_f( sv_client_t *cl )
{
	edict_t	*pEntity = cl->edict;

	if( !Cvar_VariableInteger( "sv_cheats" ) || sv.background )
		return;

	pEntity->v.flags = pEntity->v.flags ^ FL_GODMODE;

	if ( !( pEntity->v.flags & FL_GODMODE ))
		SV_ClientPrintf( cl, PRINT_HIGH, "godmode OFF\n" );
	else SV_ClientPrintf( cl, PRINT_HIGH, "godmode ON\n" );
}

/*
==================
SV_Notarget_f
==================
*/
static void SV_Notarget_f( sv_client_t *cl )
{
	edict_t	*pEntity = cl->edict;

	if( !Cvar_VariableInteger( "sv_cheats" ) || sv.background )
		return;

	pEntity->v.flags = pEntity->v.flags ^ FL_NOTARGET;

	if ( !( pEntity->v.flags & FL_NOTARGET ))
		SV_ClientPrintf( cl, PRINT_HIGH, "notarget OFF\n" );
	else SV_ClientPrintf( cl, PRINT_HIGH, "notarget ON\n" );
}

/*
==================
SV_SendRes_f
==================
*/
void SV_SendRes_f( sv_client_t *cl )
{
	sizebuf_t		msg;
	static byte	buffer[65535];

	if( cl->state != cs_connected )
	{
		MsgDev( D_INFO, "sendres is not valid from the console\n" );
		return;
	}

	BF_Init( &msg, "SendResources", buffer, sizeof( buffer ));

	SV_SendResources( &msg );
	Netchan_CreateFragments( 1, &cl->netchan, &msg );
	Netchan_FragSend( &cl->netchan );
}

ucmd_t ucmds[] =
{
{ "new", SV_New_f },
{ "god", SV_Godmode_f },
{ "begin", SV_Begin_f },
{ "pause", SV_Pause_f },
{ "noclip", SV_Noclip_f },
{ "sendres", SV_SendRes_f },
{ "notarget", SV_Notarget_f },
{ "baselines", SV_Baselines_f },
{ "deltainfo", SV_DeltaInfo_f },
{ "info", SV_ShowServerinfo_f },
{ "modellist", SV_WriteModels_f },
{ "soundlist", SV_WriteSounds_f },
{ "eventlist", SV_WriteEvents_f },
{ "disconnect", SV_Disconnect_f },
{ "usermsgs", SV_UserMessages_f },
{ "userinfo", SV_UpdateUserinfo_f },
{ "lightstyles", SV_WriteLightstyles_f },
{ "getresourelist", SV_SendResourceList_f },
{ "continueloading", SV_ContinueLoading_f },
{ NULL, NULL }
};

/*
==================
SV_ExecuteUserCommand
==================
*/
void SV_ExecuteClientCommand( sv_client_t *cl, char *s )
{
	ucmd_t	*u;

	svs.currentPlayer = cl;
	svs.currentPlayerNum = (cl - svs.clients);

	Cmd_TokenizeString( s );

	for( u = ucmds; u->name; u++ )
	{
		if( !Q_strcmp( Cmd_Argv( 0 ), u->name ))
		{
			MsgDev( D_NOTE, "ucmd->%s()\n", u->name );
			if( u->func ) u->func( cl );
			break;
		}
	}

	if( !u->name && sv.state == ss_active )
	{
		// custom client commands
		svgame.dllFuncs.pfnClientCommand( cl->edict );

		if( !Q_strcmp( Cmd_Argv( 0 ), "fullupdate" ))
		{
			// resend the ambient sounds for demo recording
			Host_RestartAmbientSounds();
			// resend all the decals for demo recording
			Host_RestartDecals();
			// resend all the static ents for demo recording
			SV_RestartStaticEnts();
		}
	}
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket( netadr_t from, sizebuf_t *msg )
{
	char	*args;
	char	*c, buf[MAX_SYSPATH];
	int	len = sizeof( buf );
	dword	challenge;
	int	index, count = 0;
	char	query[512];
	word	port;

	BF_Clear( msg );
	BF_ReadLong( msg );// skip the -1 marker

	args = BF_ReadStringLine( msg );
	Cmd_TokenizeString( args );

	c = Cmd_Argv( 0 );
	MsgDev( D_NOTE, "SV_ConnectionlessPacket: %s : %s\n", NET_AdrToString( from ), c );

	if( !Q_strcmp( c, "ping" )) SV_Ping( from );
	else if( !Q_strcmp( c, "ack" )) SV_Ack( from );
	else if( !Q_strcmp( c, "status" )) SV_Status( from );
	else if( !Q_strcmp( c, "info" )) SV_Info( from );
	else if( !Q_strcmp( c, "getchallenge" )) SV_GetChallenge( from );
	else if( !Q_strcmp( c, "connect" )) SV_DirectConnect( from );
	else if( !Q_strcmp( c, "rcon" )) SV_RemoteCommand( from, msg );
	else if( !Q_strcmp( c, "netinfo" )) SV_BuildNetAnswer( from );
	else if( msg->pData[0] == 0xFF && msg->pData[1] == 0xFF && msg->pData[2] == 0xFF && msg->pData[3] == 0xFF && msg->pData[4] == 0x4E && msg->pData[5] == 0x0A )
	{
		challenge = *(dword *)&msg->pData[6];

		port = Cvar_Get( "ip_hostport", "0", CVAR_INIT, "network server port" )->integer;
		if( !port ) port = Cvar_Get( "port", va( "%i", PORT_SERVER ), CVAR_INIT, "network default port" )->integer;

		for( index = 0; index < sv_maxclients->integer; index++ )
		{
			if( svs.clients[index].state >= cs_connected )
				count++;
		}

		Q_snprintf( query, sizeof( query ),
		"0\n\\protocol\\7\\challenge\\%ld\\players\\%d\\max\\%d\\bots\\0\\gamedir\\%s_xash\\map\\%s\\password\\0\\os\\w\\lan\\0\\region\\255\\gameport\\%d\\specport\\27015\\dedicated\\1\\appid\\70\\type\\d\\secure\\0\\version\\1.1.2.1\\product\\valve\n",
		challenge, count, sv_maxclients->integer, GI->gamefolder, sv.name, port );

		NET_SendPacket( NS_SERVER, Q_strlen( query ), query, from );
	}
	else if( svgame.dllFuncs.pfnConnectionlessPacket( &from, args, buf, &len ))
	{
		// user out of band message (must be handled in CL_ConnectionlessPacket)
		if( len > 0 ) Netchan_OutOfBand( NS_SERVER, from, len, buf );
	}
	else MsgDev( D_ERROR, "bad connectionless packet from %s:\n%s\n", NET_AdrToString( from ), args );
}

/*
==================
SV_ParseClientMove

The message usually contains all the movement commands 
that were in the last three packets, so that the information
in dropped packets can be recovered.

On very fast clients, there may be multiple usercmd packed into
each of the backup packets.
==================
*/
static void SV_ParseClientMove( sv_client_t *cl, sizebuf_t *msg )
{
	client_frame_t	*frame;
	int		key, size, checksum1, checksum2;
	int		i, numbackup, newcmds, numcmds;
	usercmd_t		nullcmd, *from;
	usercmd_t		cmds[32], *to;
	edict_t		*player;

	numbackup = 2;
	player = cl->edict;

	frame = &cl->frames[cl->netchan.incoming_acknowledged & SV_UPDATE_MASK];
	Q_memset( &nullcmd, 0, sizeof( usercmd_t ));
	Q_memset( cmds, 0, sizeof( cmds ));

	key = BF_GetRealBytesRead( msg );
	checksum1 = BF_ReadByte( msg );
	cl->packet_loss = BF_ReadByte( msg );

	numbackup = BF_ReadByte( msg );
	newcmds = BF_ReadByte( msg );

	numcmds = numbackup + newcmds;
	net_drop = net_drop + 1 - newcmds;

	if( numcmds < 0 || numcmds > 28 )
	{
		MsgDev( D_ERROR, "%s sending too many commands %i\n", cl->name, numcmds );
		SV_DropClient( cl );
		return;
	}

	from = &nullcmd;	// first cmd are starting from null-comressed usercmd_t

	for( i = numcmds - 1; i >= 0; i-- )
	{
		to = &cmds[i];
		MSG_ReadDeltaUsercmd( msg, from, to );
		from = to; // get new baseline
	}

	if( cl->state != cs_spawned )
	{
		cl->delta_sequence = -1;
		return;
	}

	// if the checksum fails, ignore the rest of the packet
	size = BF_GetRealBytesRead( msg ) - key - 1;
	checksum2 = CRC32_BlockSequence( msg->pData + key + 1, size, cl->netchan.incoming_sequence );

	if( checksum2 != checksum1 )
	{
		MsgDev( D_ERROR, "SV_UserMove: failed command checksum for %s (%d != %d)\n", cl->name, checksum2, checksum1 );
		return;
	}

	// check for pause or frozen
	if( sv.paused || sv.loadgame || sv.background || !CL_IsInGame() || ( player->v.flags & FL_FROZEN ))
	{
		for( i = 0; i < newcmds; i++ )
		{
			cmds[i].msec = 0;
			cmds[i].forwardmove = 0;
			cmds[i].sidemove = 0;
			cmds[i].upmove = 0;
			cmds[i].buttons = 0;

			if( player->v.flags & FL_FROZEN || sv.background )
				cmds[i].impulse = 0;

			VectorCopy( cmds[i].viewangles, player->v.v_angle );
		}
		net_drop = 0;
	}
	else
	{
		if( !player->v.fixangle )
			VectorCopy( cmds[0].viewangles, player->v.v_angle );
	}

	player->v.button = cmds[0].buttons;
	player->v.light_level = cmds[0].lightlevel;

	SV_EstablishTimeBase( cl, cmds, net_drop, numbackup, newcmds );

	if( net_drop < 24 )
	{
		while( net_drop > numbackup )
		{
			SV_RunCmd( cl, &cl->lastcmd, 0 );
			net_drop--;
		}

		while( net_drop > 0 )
		{
			i = net_drop + newcmds - 1;
			SV_RunCmd( cl, &cmds[i], cl->netchan.incoming_sequence - i );
			net_drop--;
		}
	}

	for( i = newcmds - 1; i >= 0; i-- )
	{
		SV_RunCmd( cl, &cmds[i], cl->netchan.incoming_sequence - i );
	}

	cl->lastcmd = cmds[0];
	cl->lastcmd.buttons = 0; // avoid multiple fires on lag

	// adjust latency time by 1/2 last client frame since
	// the message probably arrived 1/2 through client's frame loop
	frame->latency -= cl->lastcmd.msec * 0.5f / 1000.0f;
	frame->latency = max( 0.0f, frame->latency );

	if( player->v.animtime > sv.time + host.frametime )
		player->v.animtime = sv.time + host.frametime;
}

/*
===================
SV_ParseResourceList

Parse resource list
===================
*/
void SV_ParseResourceList( sv_client_t *cl, sizebuf_t *msg )
{
	Netchan_CreateFileFragments( true, &cl->netchan, BF_ReadString( msg ));
	Netchan_FragSend( &cl->netchan );
}

/*
===================
SV_ParseCvarValue

Parse a requested value from client cvar 
===================
*/
void SV_ParseCvarValue( sv_client_t *cl, sizebuf_t *msg )
{
	const char *value = BF_ReadString( msg );

	if( svgame.dllFuncs2.pfnCvarValue != NULL )
		svgame.dllFuncs2.pfnCvarValue( cl->edict, value );
	MsgDev( D_AICONSOLE, "Cvar query response: name:%s, value:%s\n", cl->name, value );
}

/*
===================
SV_ParseCvarValue2

Parse a requested value from client cvar 
===================
*/
void SV_ParseCvarValue2( sv_client_t *cl, sizebuf_t *msg )
{
	string name, value;
	int requestID = BF_ReadLong( msg );
	Q_strcpy( name, BF_ReadString( msg ));
	Q_strcpy( value, BF_ReadString( msg ));

	if( svgame.dllFuncs2.pfnCvarValue2 != NULL )
		svgame.dllFuncs2.pfnCvarValue2( cl->edict, requestID, name, value );
	MsgDev( D_AICONSOLE, "Cvar query response: name:%s, request ID %d, cvar:%s, value:%s\n", cl->name, requestID, name, value );
}

/*
===================
SV_ExecuteClientMessage

Parse a client packet
===================
*/
void SV_ExecuteClientMessage( sv_client_t *cl, sizebuf_t *msg )
{
	int		c, stringCmdCount = 0;
	qboolean		move_issued = false;
	client_frame_t	*frame;
	char		*s;

	// calc ping time
	frame = &cl->frames[cl->netchan.incoming_acknowledged & SV_UPDATE_MASK];

	// raw ping doesn't factor in message interval, either
	frame->raw_ping = host.realtime - frame->senttime;

	// on first frame ( no senttime ) don't skew ping
	if( frame->senttime == 0.0f )
	{
		frame->latency = 0.0f;
		frame->raw_ping = 0.0f;
	}

	// don't skew ping based on signon stuff either
	if(( host.realtime - cl->lastconnect ) < 2.0f && ( frame->latency > 0.0 ))
	{
		frame->latency = 0.0f;
		frame->raw_ping = 0.0f;
	}

	cl->delta_sequence = -1; // no delta unless requested

	// set the current client
	svs.currentPlayer = cl;
	svs.currentPlayerNum = (cl - svs.clients);
				
	// read optional clientCommand strings
	while( cl->state != cs_zombie )
	{
		if( BF_CheckOverflow( msg ))
		{
			MsgDev( D_ERROR, "SV_ReadClientMessage: clc_bad\n" );
			SV_DropClient( cl );
			return;
		}

		// end of message
		if( BF_GetNumBitsLeft( msg ) < 8 )
			break;

		c = BF_ReadByte( msg );

		switch( c )
		{
		case clc_nop:
			break;
		case clc_userinfo:
			SV_UserinfoChanged( cl, BF_ReadString( msg ));
			break;
		case clc_delta:
			cl->delta_sequence = BF_ReadByte( msg );
			break;
		case clc_move:
			if( move_issued ) return; // someone is trying to cheat...
			move_issued = true;
			SV_ParseClientMove( cl, msg );
			break;
		case clc_stringcmd:	
			s = BF_ReadString( msg );
			// malicious users may try using too many string commands
			if( ++stringCmdCount < 8 ) SV_ExecuteClientCommand( cl, s );
			if( cl->state == cs_zombie ) return; // disconnect command
			break;
		case clc_resourcelist:
			SV_ParseResourceList( cl, msg );
			break;
		case clc_requestcvarvalue:
			SV_ParseCvarValue( cl, msg );
			break;
		case clc_requestcvarvalue2:
			SV_ParseCvarValue2( cl, msg );
			break;
		default:
			MsgDev( D_ERROR, "SV_ReadClientMessage: clc_bad\n" );
			SV_DropClient( cl );
			return;
		}
	}
}