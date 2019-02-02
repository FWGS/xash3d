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

static void SV_UserinfoChanged( sv_client_t *cl, const char *userinfo );

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
		svs.challenges[oldest].challenge = ((uint)rand() << 16) ^ rand();
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
	char		physinfostr[512];
	int		i, edictnum;
	int		qport, version;
	int		count = 0;
	int		challenge;
	unsigned int requested_extensions, extensions = 0;
	edict_t		*ent;
	char *errorpacket = "print";

	if( Cmd_Argc() > 6 )
		errorpacket = "errormsg";

	if( !svs.initialized )
	{
		Netchan_OutOfBandPrint( NS_SERVER, from, "%s\nServer not running any map!\n", errorpacket );
		Netchan_OutOfBandPrint( NS_SERVER, from, "disconnect\n" );
		return;
	}

	version = Q_atoi( Cmd_Argv( 1 ));

	if( version != PROTOCOL_VERSION )
	{
		Netchan_OutOfBandPrint( NS_SERVER, from, "print\nServer uses protocol version %i.\n", PROTOCOL_VERSION );
		Netchan_OutOfBandPrint( NS_SERVER, from, "errormsg\nServer uses protocol version %i.\n", PROTOCOL_VERSION );

		MsgDev( D_ERROR, "SV_DirectConnect: rejected connect from version %i\n", version );
		Netchan_OutOfBandPrint( NS_SERVER, from, "disconnect\n" );
		return;
	}

	if( !SV_ProcessUserAgent( from, Cmd_Argv( 6 ) ) )
	{
		Netchan_OutOfBandPrint( NS_SERVER, from, "disconnect\n" );
		return;
	}

	qport = Q_atoi( Cmd_Argv( 2 ));
	challenge = Q_atoi( Cmd_Argv( 3 ));
	Q_strncpy( userinfo, Cmd_Argv( 4 ), sizeof( userinfo ));

	requested_extensions = Q_atoi( Cmd_Argv( 5 ) );

	// quick reject
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state == cs_free || cl->state == cs_zombie )
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
		const char *password;

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
			Netchan_OutOfBandPrint( NS_SERVER, from, "%s\nNo or bad challenge for address.\n", errorpacket );
			Netchan_OutOfBandPrint( NS_SERVER, from, "disconnect\n" );
			return;
		}

		MsgDev( D_NOTE, "Client %i connecting with challenge %x\n", i, challenge );
		svs.challenges[i].connected = true;

		password = sv_password->string;

		if( password[0] && Q_stricmp( password, "none" ) && // does server have password
			Q_stricmp( password, Info_ValueForKey( userinfo, "password" ) ) ) // does user have match it
		{
			Netchan_OutOfBandPrint( NS_SERVER, from, "%s\nInvalid server password.\n", errorpacket );
			Netchan_OutOfBandPrint( NS_SERVER, from, "disconnect\n" );
			return;
		}
	}

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey( userinfo, "ip", NET_AdrToString( from ), sizeof( userinfo ) );

	newcl = &temp;
	Q_memset( newcl, 0, sizeof( sv_client_t ));

	// if there is already a slot for this ip, reuse it
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state == cs_free || cl->state == cs_zombie )
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
		Netchan_OutOfBandPrint( NS_SERVER, from, "%s\nServer is full.\n", errorpacket );
		MsgDev( D_INFO, "SV_DirectConnect: rejected a connection.\n" );
		Netchan_OutOfBandPrint( NS_SERVER, from, "disconnect\n" );
		return;
	}

	// build a new connection
	// accept the new client
gotnewcl:
	// this is the only place a sv_client_t is ever initialized

	if( sv_maxclients->integer == 1 ) // save physinfo for singleplayer
		Q_strncpy( physinfostr, newcl->physinfo, sizeof( physinfostr ));

	*newcl = temp;

	if( sv_maxclients->integer == 1 ) // restore physinfo for singleplayer
		Q_strncpy( newcl->physinfo, physinfostr, sizeof( physinfostr ));

	if( Q_strncpy( newcl->useragent, Cmd_Argv( 6 ), MAX_INFO_STRING ) )
	{
		char *id = Info_ValueForKey( newcl->useragent, "i" );

		if( *id )
		{
			sscanf( id, "%llx", &newcl->WonID );
		}

		Q_strncpy( cl->auth_id, id, sizeof( cl->auth_id ) );
	}

	svs.currentPlayer = newcl;
	svs.currentPlayerNum = (newcl - svs.clients);
	edictnum = svs.currentPlayerNum + 1;

	ent = EDICT_NUM( edictnum );
	newcl->edict = ent;
	newcl->challenge = challenge; // save challenge for checksumming
	if( newcl->frames )
		Mem_Free( newcl->frames );
	newcl->frames = (client_frame_t *)Z_Malloc( sizeof( client_frame_t ) * SV_UPDATE_BACKUP );
	newcl->userid = g_userid++;	// create unique userid
	newcl->authentication_method = 2;

	// initailize netchan here because SV_DropClient will clear network buffer
	Netchan_Setup( NS_SERVER, &newcl->netchan, from, qport );

	if( sv_allow_compress->integer && ( requested_extensions & NET_EXT_HUFF ) )
	{
		extensions |= NET_EXT_HUFF;
		newcl->netchan.compress = true;
	}

	if( sv_allow_split->integer && ( requested_extensions & NET_EXT_SPLIT ) )
	{
		unsigned int maxpacket = Q_atoi( Info_ValueForKey( userinfo, "cl_maxpacket") );
		extensions |= NET_EXT_SPLIT;
		newcl->netchan.split = true;
		if( maxpacket < 100 || maxpacket >= NET_MAX_PAYLOAD / 2 )
			maxpacket = 1400;
		if( maxpacket > sv_maxpacket->integer )
			maxpacket = sv_maxpacket->integer;
		newcl->netchan.maxpacket = maxpacket;

		if( sv_allow_compress->integer && sv_allow_split->integer
				&& !( requested_extensions & NET_EXT_HUFF )
				&& ( requested_extensions & NET_EXT_SPLITHUFF ) )
			newcl->netchan.splitcompress = true, extensions |= NET_EXT_SPLITHUFF;
	}

	BF_Init( &newcl->datagram, "Datagram", newcl->datagram_buf, sizeof( newcl->datagram_buf )); // datagram buf
	newcl->cl_updaterate = 0.05;	// 20 fps as default

	// parse some info from the info strings (this can override cl_updaterate)
	SV_UserinfoChanged( newcl, userinfo );

	// send the connect packet to the client
	Netchan_OutOfBandPrint( NS_SERVER, from, "client_connect %d\n", extensions );

	Log_Printf( "\"%s<%i><%s><>\" connected, address \"%s\"\n", Info_ValueForKey( userinfo, "name" ),
				newcl->userid, SV_GetClientIDString( newcl ), NET_AdrToString( newcl->netchan.remote_address ) );

	newcl->state = cs_connected;
	newcl->lastmessage = host.realtime;
	newcl->lastconnect = host.realtime;
	newcl->delta_sequence = -1;
	newcl->resources_sent = 1;
	newcl->hasusrmsgs = false;
	newcl->next_messagetime = host.realtime + newcl->cl_updaterate;

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
#if 0
	/* mittorn: pvPrivateData must be cleaned on edict remove
	 * If it cleaned here, server will crash later */
	if( pClient->pvPrivateData != NULL )
	{
		// NOTE: new interface can be missing
		if( svgame.dllFuncs2.pfnOnFreeEntPrivateData != NULL )
			svgame.dllFuncs2.pfnOnFreeEntPrivateData( pClient );

		// clear any dlls data but keep engine data
		Mem_Free( pClient->pvPrivateData );
		pClient->pvPrivateData = NULL;
	}
	// HACK: invalidate serial number
	pClient->serialnumber++;
#endif
}

/*
==================
SV_FakeConnect

A connection request that came from the game module
==================
*/
edict_t *GAME_EXPORT SV_FakeConnect( const char *netname )
{
	int		i, edictnum;
	char		userinfo[MAX_INFO_STRING];
	sv_client_t	temp, *cl, *newcl;
	edict_t		*ent;

	if( !netname ) netname = "";
	userinfo[0] = '\0';

	// setup fake client params
	Info_SetValueForKey( userinfo, "name", netname, sizeof( userinfo ) );
	Info_SetValueForKey( userinfo, "model", "gordon", sizeof( userinfo ) );
	Info_SetValueForKey( userinfo, "topcolor", "0", sizeof( userinfo ) );
	Info_SetValueForKey( userinfo, "bottomcolor", "0", sizeof( userinfo ) );

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey( userinfo, "ip", "127.0.0.1", sizeof( userinfo ) );

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

	MsgDev( D_NOTE, "Bot %i connecting with challenge %x\n", i, -1 );

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
	if( szRejectReason[0] ) Info_SetValueForKey( userinfo, "rejmsg", szRejectReason, sizeof( userinfo ) );

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

	if( NET_CompareBaseAdr( drop->netchan.remote_address, host.rd.address ) )
		SV_EndRedirect();

	SV_ClearCustomizationList( &drop->customization );

	// throw away any residual garbage in the channel.
	Netchan_Clear( &drop->netchan );

	// Clean client data on disconnect
	Q_memset( drop->userinfo, 0, MAX_INFO_STRING );
	Q_memset( drop->physinfo, 0, MAX_INFO_STRING );
	//drop->edict = 0;
	//Q_memset( &drop->edict->v, 0, sizeof( drop->edict->v ) );
	if( drop->edict )
		drop->edict->v.frags = 0;

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
	if( !host.rd.lines )
		host.rd.lines = -1;
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
	if( host.rd.lines > 0 )
		return;

	if( host.rd.flush )
		host.rd.flush( host.rd.address, host.rd.target, host.rd.buffer );

	host.rd.target = 0;
	host.rd.buffer = NULL;
	host.rd.buffersize = 0;
	host.rd.flush = NULL;
}

/*
================
Rcon_Print

Print message to rcon buffer and send to rcon redirect target
================
*/

void Rcon_Print( const char *pMsg )
{
	if( host.rd.target && host.rd.lines && host.rd.flush )
	{
		int len = Q_strncat( host.rd.buffer, pMsg, host.rd.buffersize );

		if( len && host.rd.buffer[len-1] == '\n' )
		{
			host.rd.flush( host.rd.address, host.rd.target, host.rd.buffer );
			if( host.rd.lines > 0 )
				host.rd.lines--;
			host.rd.buffer[0] = 0;
			if( !host.rd.lines )
				Msg( "End of redirection!\n" );
		}
	}
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
			Q_sprintf( player, "%i %i \"%s\"\n", (int)cl->edict->v.frags, (int)cl->ping, cl->name );
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
		Q_snprintf( result, sizeof( result ), "%010llu", cl->WonID );
	}
	else if( cl->authentication_method == 2 )
	{
		if( NET_IsLocalAddress( cl->netchan.remote_address ))
		{
			Q_strncpy( result, "VALVE_ID_LOOPBACK", sizeof( result ));
		}
		else if( cl->auth_id[0] )
		{
			Q_snprintf( result, sizeof( result ), "VALVE_XASH_%s", cl->auth_id );
		}
		else if( cl->WonID == 0 )
		{
			Q_strncpy( result, "VALVE_ID_PENDING", sizeof( result ));
		}
		else
		{
			Q_snprintf( result, sizeof( result ), "VALVE_%010llu", cl->WonID );
		}
	}
	else Q_strncpy( result, "UNKNOWN", sizeof( result ));

	return result;
}

sv_client_t *SV_ClientById( int id )
{
	sv_client_t *cl;
	int i;

	ASSERT( id >= 0 );

	for( i = 0, cl = svs.clients; i < svgame.globals->maxClients; i++, cl++ )
	{
		if( !cl->state )
			continue;

		if( cl->userid == id )
			return cl;
	}

	return NULL;
}

sv_client_t *SV_ClientByName( const char *name )
{
	sv_client_t *cl;
	int i;

	ASSERT( name && *name );

	for( i = 0, cl = svs.clients; i < svgame.globals->maxClients; i++, cl++ )
	{
		if( !cl->state )
			continue;

		if( !Q_strcmp( cl->name, name ) )
			return cl;
	}

	return NULL;
}

qboolean SV_SetCurrentClient( sv_client_t *cl )
{
	ASSERT( cl && cl >= svs.clients && cl < ( svs.clients + svgame.globals->maxClients ) );

	svs.currentPlayer = cl;
	svs.currentPlayerNum = cl - svs.clients;

	return true;
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
void SV_Info( netadr_t from, int version )
{
	char	string[MAX_INFO_STRING];
	int	i, count = 0;
	char *gamedir = GI->gamefolder;
	qboolean havePassword;

	// ignore in single player
	if( sv_maxclients->integer == 1 || !svs.initialized )
		return;

	string[0] = '\0';

	if( version != PROTOCOL_VERSION )
	{
		Q_snprintf( string, sizeof( string ), "%s: wrong version\n", hostname->string );
	}
	else
	{
		for( i = 0; i < sv_maxclients->integer; i++ )
			if( svs.clients[i].state >= cs_connected && !svs.clients[i].fakeclient )
				count++;

		havePassword = sv_password->string[0] && Q_stricmp( sv_password->string, "none" );

		Info_SetValueForKey( string, "host", hostname->string, sizeof( string ) );
		Info_SetValueForKey( string, "map", sv.name, sizeof( string ) );
		Info_SetValueForKey( string, "dm", va( "%i", (int)svgame.globals->deathmatch ), sizeof( string ) );
		Info_SetValueForKey( string, "team", va( "%i", (int)svgame.globals->teamplay ), sizeof( string ) );
		Info_SetValueForKey( string, "coop", va( "%i", (int)svgame.globals->coop ), sizeof( string ) );
		Info_SetValueForKey( string, "numcl", va( "%i", count ), sizeof( string ) );
		Info_SetValueForKey( string, "maxcl", va( "%i", sv_maxclients->integer ), sizeof( string ) );
		Info_SetValueForKey( string, "gamedir", gamedir, sizeof( string ) );

		// a1ba: extend to password
		Info_SetValueForKey( string, "password", havePassword ? "1" : "0", sizeof( string ));
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
	if( sv_maxclients->integer == 1 || !svs.initialized )
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
				Q_strncat( string, va( "%i\\%s\\%i\\%f\\", count, svs.clients[i].name, (int)ed->v.frags, time ), sizeof( string ) );
				count++;
			}
		}

		// send playernames
		Q_snprintf( answer, sizeof( answer ), "netinfo %i %i %s\n", context, type, string );
		Netchan_OutOfBandPrint( NS_SERVER, from, answer ); // no info string
	}
	else if( type == NETAPI_REQUEST_DETAILS )
	{
		qboolean havePassword;

		for( i = 0; i < sv_maxclients->integer; i++ )
			if( svs.clients[i].state >= cs_connected )
				count++;

		havePassword = sv_password->string[0] && Q_stricmp( sv_password->string, "none" );

		string[0] = '\0';
		Info_SetValueForKey( string, "hostname", hostname->string, sizeof( string ) );
		Info_SetValueForKey( string, "gamedir", GI->gamefolder, sizeof( string ) );
		Info_SetValueForKey( string, "current", va( "%i", count ), sizeof( string ) );
		Info_SetValueForKey( string, "max", va( "%i", sv_maxclients->integer ), sizeof( string ) );
		Info_SetValueForKey( string, "map", sv.name, sizeof( string ) );

		// a1ba: add password
		Info_SetValueForKey( string, "password", havePassword ? "1" : "0", sizeof( string ) );

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
	SV_BeginRedirect( from, RD_PACKET, outputbuf, sizeof( outputbuf ), SV_FlushRedirect );

	if( Rcon_Validate( ))
	{
		remaining[0] = 0;
		for( i = 2; i < Cmd_Argc(); i++ )
		{
			string command;

			Com_EscapeCommand( command, Cmd_Argv( i ), MAX_STRING );

			Q_strncat( remaining, "\"", 1024 );
			Q_strncat( remaining, command, 1024 );
			Q_strncat( remaining, "\" ", 1024 );
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
	int		i, count, back;
	client_frame_t	*frame;

	// bots don't have a real ping
	if( cl->fakeclient )
		return 0;

	// client has no frame data
	if( !cl->frames )
		return 0;

	count = 0;

	if( SV_UPDATE_BACKUP <= 31 )
	{
		back = SV_UPDATE_BACKUP / 2;
		if( back <= 0 ) return 0;
	}
	else back = 16;

	for( i = 0; i < back; i++ )
	{
		frame = &cl->frames[(cl->netchan.incoming_acknowledged - (i + 1)) & SV_UPDATE_MASK];

		if( frame->ping_time > 0 )
		{
			ping += frame->ping_time;
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
void SV_EstablishTimeBase( sv_client_t *cl, usercmd_t *cmds, int dropped, int numbackup, int numcmds )
{
	double	runcmd_time = 0.0;
	int	cmdnum = dropped;

	if( dropped < 24 )
	{
		if( dropped > numbackup )
		{
			cmdnum = dropped - (dropped - numbackup);
			runcmd_time = (double)cl->lastcmd.msec * (dropped - numbackup) / 1000.0;
		}

		for( ; cmdnum > 0; cmdnum-- )
			runcmd_time += cmds[cmdnum - 1 + numcmds].msec / 1000.0;
	}

	for( ; numcmds > 0; numcmds-- )
		runcmd_time += cmds[numcmds - 1].msec / 1000.0;

	cl->timebase = sv.time + host.frametime - runcmd_time;
}

/*
===================
SV_CalcClientTime

compute latency for client
===================
*/
float SV_CalcClientTime( sv_client_t *cl )
{
	float	minping, maxping;
	float	ping = 0.0f;
	int	i, count = 0;
	int	backtrack;

	backtrack = sv_unlagsamples->integer;

	if( backtrack < 1 )
		backtrack = 1;

	if( backtrack > min( SV_UPDATE_BACKUP, 16 ) )
		backtrack = min( SV_UPDATE_BACKUP, 16 );

	if( backtrack <= 0 )
		return 0.0f;

	for( i = 0; i < backtrack; i++ )
	{
		client_frame_t	*frame = &cl->frames[SV_UPDATE_MASK & (cl->netchan.incoming_acknowledged - i)];
		if( frame->ping_time <= 0.0f )
			continue;

		ping += frame->ping_time;
		count++;
	}

	if( !count ) return 0.0f;

	minping =  9999.0f;
	maxping = -9999.0f;
	ping /= count;

	for( i = 0; i < min( SV_UPDATE_BACKUP, 4 ); i++ )
	{
		client_frame_t	*frame = &cl->frames[SV_UPDATE_MASK & (cl->netchan.incoming_acknowledged - i)];
		if( frame->ping_time <= 0.0f )
			continue;

		if( frame->ping_time < minping )
			minping = frame->ping_time;

		if( frame->ping_time > maxping )
			maxping = frame->ping_time;
	}

	if( maxping < minping || fabs( maxping - minping ) <= 0.2f )
		return ping;

	return 0.0f;
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
	if( host.realtime > cl->next_checkpingtime )
	{
		cl->next_checkpingtime = host.realtime + 2.0;

		return true;
		//return cl->lastcmd.buttons & IN_SCORE;	// they are viewing the scoreboard.  Send them pings.
	}
	// this means the same? Useless check
	//else if ( cl->next_checkpingtime - host.realtime > 2.0 )
	//	cl->next_checkpingtime = host.realtime + 2.0;

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
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void SV_PutClientInServer( edict_t *ent )
{
	sv_client_t	*client;

	client = SV_ClientFromEdict( ent, false );

	if( client == NULL )
	{
		MsgDev( D_ERROR, "SV_PutClientInServer: you have broken clients!\n");
		return;
	}

	if( !sv.loadgame )
	{
		//client->hltv_proxy = Q_atoi( Info_ValueForKey( client->userinfo, "hltv" )) ? true : false;

		if( ent->pvPrivateData != NULL )
		{
			// NOTE: new interface can be missing
			if( svgame.dllFuncs2.pfnOnFreeEntPrivateData != NULL )
				svgame.dllFuncs2.pfnOnFreeEntPrivateData( ent );

			// clear any dlls data but keep engine data
			Mem_Free( ent->pvPrivateData );
			ent->pvPrivateData = NULL;
			//ent->serialnumber++;
		}

		Q_memset( &ent->v, 0, sizeof( ent->v ) );

		ent->v.colormap = NUM_FOR_EDICT( ent );
		if( client->hltv_proxy )
			ent->v.flags |= FL_PROXY;
		else ent->v.flags = 0;

		ent->v.netname = MAKE_STRING( client->name );

		// fisrt entering
		svgame.globals->time = sv.time;
		ent->v.pContainingEntity = ent;

		svgame.dllFuncs.pfnClientPutInServer( ent );

		if( sv.background )	// don't attack player in background mode
			ent->v.flags |= (FL_GODMODE|FL_NOTARGET);

		client->pViewEntity = NULL; // reset pViewEntity

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

	client->state = cs_spawned;

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
		MsgDev( D_INFO, "Level loaded in %.2f sec\n", Sys_DoubleTime() - svs.timestart );
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

	// send audio track info
	if( svgame.globals->cdAudioTrack )
	{
		//BF_WriteByte( &client->netchan.message, svc_stufftext );
		//BF_WriteString( &client->netchan.message, va( "cd loop %3d\n", svgame.globals->cdAudioTrack ));

		BF_WriteByte( &cl->netchan.message, svc_cdtrack );
		BF_WriteByte( &cl->netchan.message, svgame.globals->cdAudioTrack );
		BF_WriteByte( &cl->netchan.message, svgame.globals->cdAudioTrack );
		svgame.globals->cdAudioTrack = 0;
	}

	// refresh userinfo on spawn
	SV_RefreshUserinfo();

	// get the game a chance to reject this connection or modify the userinfo
	if( !SV_ClientConnect( cl->edict, cl->userinfo ) )
	{
		char *errorpacket = cl->useragent[0]?"errormsg":"print";

		if( *Info_ValueForKey( cl->userinfo, "rejmsg" ))
			Netchan_OutOfBandPrint( NS_SERVER, cl->netchan.remote_address, "%s\n%s\nConnection refused.\n", errorpacket, Info_ValueForKey( cl->userinfo, "rejmsg" ));
		else Netchan_OutOfBandPrint( NS_SERVER, cl->netchan.remote_address, "%s\nConnection refused.\n", errorpacket );

		MsgDev( D_ERROR, "game rejected a connection.\n");
		Netchan_OutOfBandPrint( NS_SERVER, cl->netchan.remote_address, "disconnect\n" );
		SV_DropClient( cl );
		return;
	}


	// game server
	if( sv.state == ss_active )
	{
		// set up the entity for the client
		ent = EDICT_NUM( playernum + 1 );
		cl->edict = ent;

		// NOTE: enable for testing
		// Still have problems with download reject
		// It's difficult to implement fastdl and forbid direct download
		if( sv_maxclients->integer == 1 || sv_allow_download->integer == 0 )
		{
			Q_memset( &cl->lastcmd, 0, sizeof( cl->lastcmd ));

			// begin fetching modellist
			BF_WriteByte( &cl->netchan.message, svc_stufftext );
			BF_WriteString( &cl->netchan.message, va( "cmd modellist %i %i\n", svs.spawncount, 0 ));
		}
		else
		{
			// transfer fastdl servers list
			if( *sv_downloadurl->string )
			{
				char *data = sv_downloadurl->string;
				char token[256];
				while( ( data = COM_ParseFile( data, token ) ) )
				{
					BF_WriteByte( &cl->netchan.message, svc_stufftext );
					BF_WriteString( &cl->netchan.message, va( "http_addcustomserver %s\n", token ));
				}
			}
			// request resource list
			BF_WriteByte( &cl->netchan.message, svc_stufftext );
			BF_WriteString( &cl->netchan.message, va( "cmd getresourcelist\n" ));
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
	if( cl->resources_sent < cl->resources_count )
	{
		SV_SendResourceList_f(cl);
		return;
	}

	// begin fetching modellist
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, va( "cmd modellist %i %i\n", svs.spawncount, 0 ));
}

void SV_AppendToResourceList( resourcelist_t *reslist, const resourcetype_t type, const char *file )
{
	if( reslist->rescount > MAX_RESOURCES )
	{
		MsgDev( D_WARN, "Too big resource list! Skipping %s of type %i\n", file, type );
		return;
	}

	Q_strncpy( reslist->resnames[reslist->rescount], file, CS_SIZE );
	reslist->restype[reslist->rescount] = type;
	reslist->rescount++;
}

void SV_ParseResListFile( resourcelist_t *reslist, const char *name )
{
	char *afile, *pfile;
	char token[MAX_SYSPATH];
	resourcetype_t type;

	afile = pfile = FS_LoadFile( name, NULL, true );

	while(( pfile = COM_ParseFile( pfile, token )))
	{
		int i;

		if( !FS_FileExists( token, true ))
			continue;

		if( !Q_memcmp( token, "sound/", 6))
		{
			type = t_sound;
			Q_memmove( token, token + 6, sizeof( token ) - 7 );
		}
		else
		{
			type = t_generic;
		}

		// prevent resource duplication
		for( i = 0; i < reslist->rescount; i++ )
		{
			if( !Q_strcmp( reslist->resnames[i], token ) )
				break;
		}

		// don't found dupe, ok, add to list
		if( i == reslist->rescount )
			SV_AppendToResourceList( reslist, type, token );
	}

	Z_Free( afile );
}

void SV_UpdateResourceList( void )
{
	int index;
	string mapresfilename;

	MsgDev( D_INFO, "Updating resource list\n" );

		SV_AppendToResourceList( &sv.reslist, t_world, "NULL" ); // terminator

		// write models
		for ( index = 1; index < MAX_MODELS && sv.model_precache[index][0]; index++ )
		{
			if ( sv.model_precache[index][0] == '*' ) // internal bmodel
				continue;

			if ( !FS_FileExists( sv.model_precache[index], true ) )
				continue;

			SV_AppendToResourceList( &sv.reslist, t_model, sv.model_precache[index] );
		}

		// write sounds
		for ( index = 1; index < MAX_SOUNDS && sv.sound_precache[index][0]; index++ )
		{
			if ( !FS_FileExists( va( "sound/%s", sv.sound_precache[index] ), true ) )
				continue;

			SV_AppendToResourceList( &sv.reslist, t_sound, sv.sound_precache[index] );
		}

		// write eventscripts
		for ( index = 1; index < MAX_EVENTS && sv.event_precache[index][0]; index++ )
		{
			if ( !FS_FileExists( sv.event_precache[index], true ) )
				continue;

			SV_AppendToResourceList( &sv.reslist, t_eventscript, sv.event_precache[index] );
		}

		// write generic
		for ( index = 1; index < MAX_CUSTOM && sv.files_precache[index][0]; index++ )
		{
			if ( !FS_FileExists( sv.files_precache[index], true ) )
				continue;

			SV_AppendToResourceList( &sv.reslist, t_generic, sv.files_precache[index] );
		}

		// load common reslist.txt file form gamedir root
		SV_ParseResListFile( &sv.reslist, "reslist.txt");

		// write maps/<name>.res
		Q_strcpy( mapresfilename, sv.worldmodel->name );
		FS_StripExtension( mapresfilename );
		FS_DefaultExtension( mapresfilename, ".res" );
		SV_ParseResListFile( &sv.reslist, mapresfilename );

		sv.resourcelistcache = true;
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
	size_t		msg_size;
	int msg_start, msg_end;

	// transfer fastdl servers list
	if( *sv_downloadurl->string )
	{
		char *data = sv_downloadurl->string;
		char token[256];
		while( ( data = COM_ParseFile( data, token ) ) )
		{
			BF_WriteByte( &cl->netchan.message, svc_stufftext );
			BF_WriteString( &cl->netchan.message, va( "http_addcustomserver %s\n", token ));
		}
	}

	// generate new resource list, if it's not cached
	if ( !sv.resourcelistcache )
		SV_UpdateResourceList();

	msg_size = BF_GetRealBytesWritten( &cl->netchan.message ); // start

	index = cl->resources_sent;
	BF_WriteByte( &cl->netchan.message, svc_resourcelist );
	msg_start = BF_GetNumBitsWritten( &cl->netchan.message );
	BF_WriteWord( &cl->netchan.message, sv.reslist.rescount );

	for( ; ( index < sv.reslist.rescount ) && ( BF_GetNumBytesWritten( &cl->netchan.message ) < cl->maxpayload ); index++ )
	{
		BF_WriteWord( &cl->netchan.message, sv.reslist.restype[index] );
		BF_WriteString( &cl->netchan.message, sv.reslist.resnames[index] );
	}

	// change real sent resource count
	msg_end = BF_GetNumBitsWritten( &cl->netchan.message );
	BF_SeekToBit( &cl->netchan.message, msg_start );
	BF_WriteWord( &cl->netchan.message, index - cl->resources_sent + 1 );
	BF_SeekToBit( &cl->netchan.message, msg_end );

	cl->resources_sent = index;
	cl->resources_count = sv.reslist.rescount;

	Msg( "Count res: %d\n", sv.reslist.rescount );
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
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( cl->maxpayload ) && start < MAX_MODELS )
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
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( cl->maxpayload ) && start < MAX_SOUNDS )
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
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( cl->maxpayload ) && start < MAX_EVENTS )
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
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( cl->maxpayload ) && start < MAX_LIGHTSTYLES )
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
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( cl->maxpayload ) && start < MAX_USER_MESSAGES )
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

	if( start == MAX_USER_MESSAGES )
	{
		Q_snprintf( cmd, MAX_STRING, "cmd deltainfo %i 0 0\n", svs.spawncount );
		cl->hasusrmsgs = true;
	}
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
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( cl->maxpayload ) && tableIndex < Delta_NumTables( ))
	{
		dt = Delta_FindStructByIndex( tableIndex );

		for( ; fieldIndex < dt->numFields; fieldIndex++ )
		{
			Delta_WriteTableField( &cl->netchan.message, tableIndex, &dt->pFields[fieldIndex] );

			// it's time to send another portion
			if( BF_GetNumBytesWritten( &cl->netchan.message ) >= ( cl->maxpayload ))
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
	while( BF_GetNumBytesWritten( &cl->netchan.message ) < ( cl->maxpayload ) && start < svgame.numEntities )
	{
		base = &svs.baselines[start];
		// if baseline created in SV_CreateBaselines
		if( base->number || !start )
		{
			BF_WriteByte( &cl->netchan.message, svc_spawnbaseline );
			MSG_WriteDeltaEntity( &nullstate, base, &cl->netchan.message, true, SV_IsPlayerIndex( base->number ), sv.time );
		}
		start++;
	}

	if( start == svgame.numEntities )
	{
		// a1ba: force server cheats value
		Q_snprintf( cmd, MAX_STRING, "precache %i %i\n", svs.spawncount, Cvar_VariableInteger( "sv_cheats" ) );
	}
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
SV_ShouldUpdateUserinfo

=================
*/
static qboolean SV_ShouldUpdateUserinfo( sv_client_t *cl )
{
	qboolean allow = true; // predict state

	if( !sv_userinfo_enable_penalty->value )
		return allow;

	if( cl->fakeclient )
		return allow;

	// start from 1 second
	if( !cl->userinfo_penalty )
		cl->userinfo_penalty = sv_userinfo_penalty_time->value;

	// player changes userinfo after limit time window, but before
	// next timewindow
	// he seems to be spammer, so just increase change attempts
	if( host.realtime < cl->userinfo_next_changetime + cl->userinfo_penalty * sv_userinfo_penalty_multiplier->value )
	{
		// player changes userinfo too quick! ignore!
		if( host.realtime < cl->userinfo_next_changetime )
		{
			MsgDev( D_INFO, "SV_ShouldUpdateUserinfo: ignore userinfo update for %s: penalty %f, attempts %i\n",
				cl->name, cl->userinfo_penalty, cl->userinfo_change_attempts );
			allow = false;
		}

		cl->userinfo_change_attempts++;
	}

	// he spammed too fast, increase penalty
	if( cl->userinfo_change_attempts > sv_userinfo_penalty_attempts->value )
	{
		MsgDev( D_INFO, "SV_ShouldUpdateUserinfo: penalty set %f for %s\n",
			cl->userinfo_penalty, cl->name );
		cl->userinfo_penalty *= sv_userinfo_penalty_multiplier->value;
		cl->userinfo_change_attempts = 0;
	}

	cl->userinfo_next_changetime = host.realtime + cl->userinfo_penalty;

	return allow;
}

/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C freindly form.
=================
*/
static void SV_UserinfoChanged( sv_client_t *cl, const char *userinfo )
{
	int		i, dupc = 1;
	edict_t		*ent = cl->edict;
	string		temp1, temp2;
	sv_client_t	*current;
	char		*val;
	const char *model;

	if( !userinfo || !userinfo[0] ) return; // ignored

	if( !SV_ShouldUpdateUserinfo( cl )) return; // ignored

	Q_strncpy( cl->userinfo, userinfo, sizeof( cl->userinfo ));

	val = Info_ValueForKey( cl->userinfo, "name" );
	Q_strncpy( temp2, val, sizeof( temp2 ));
	TrimSpace( temp2, temp1 );

	Info_RemoveKey( cl->userinfo, "password" ); // hide password from other users

	if( !Q_stricmp( temp1, "console" )) // keyword came from OSHLDS
	{
		Info_SetValueForKey( cl->userinfo, "name", "unnamed", sizeof( cl->userinfo ) );
		val = Info_ValueForKey( cl->userinfo, "name" );
	}
	else if( Q_strcmp( temp1, val ))
	{
		Info_SetValueForKey( cl->userinfo, "name", temp1, sizeof( cl->userinfo ) );
		val = Info_ValueForKey( cl->userinfo, "name" );
	}

	if( !Q_strlen( temp1 ) )
	{
		Info_SetValueForKey( cl->userinfo, "name",	"unnamed", sizeof( cl->userinfo ) );
		val = Info_ValueForKey( cl->userinfo, "name" );
		Q_strncpy( temp2, "unnamed", sizeof( temp2 ));
		Q_strncpy( temp1, "unnamed", sizeof( temp1 ));
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
			Info_SetValueForKey( cl->userinfo, "name", temp2, sizeof( cl->userinfo ) );
			val = Info_ValueForKey( cl->userinfo, "name" );
			Q_strncpy( cl->name, temp2, sizeof( cl->name ) );
		}
		else
		{
			if( dupc == 1 ) // unchanged
				Q_strncpy( cl->name, temp1, sizeof( cl->name ) );
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

	cl->hltv_proxy = Q_atoi( Info_ValueForKey( cl->userinfo, "hltv" ));

	cl->local_weapons = Q_atoi( Info_ValueForKey( cl->userinfo, "cl_lw" )) ? true : false;
	cl->lag_compensation = Q_atoi( Info_ValueForKey( cl->userinfo, "cl_lc" )) ? true : false;

	// max payload calculation
	if( *( val = Info_ValueForKey( cl->userinfo, "cl_maxpayload" ) ) )
		cl->maxpayload = bound( 100, Q_atoi(val), sv_maxpacket->integer );
	else if( *( val = Info_ValueForKey( cl->userinfo, "cl_maxpacket" ) ) )
	{
		int maxpayload = Q_atoi( val );

		if( !cl->netchan.split ) // 1000 + overrun,  should be < 1400
			cl->maxpayload = bound( 100, maxpayload, ( sv_maxpacket->integer / 2 ) );
		// do not let send more than two packets at time, it may be killed by firewall
		else // 1960 bytes default
			cl->maxpayload = bound( 100, sv_maxpacket->integer, maxpayload * 1.4f );
	}
	else
		cl->maxpayload = sv_maxpacket->integer;

	if( sv_maxclients->integer == 1 )
		cl->maxpayload = NET_MAX_PAYLOAD / 2;

	val = Info_ValueForKey( cl->userinfo, "cl_updaterate" );

	if( Q_strlen( val ))
	{
		i = bound( 10, Q_atoi( val ), 300 );
		cl->cl_updaterate = 1.0f / i;
	}

	model = Info_ValueForKey( cl->userinfo, "model" );

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

	// Force reset player model to "player"
	if( cl->modelindex == 0 )
	{
		Info_SetValueForKey( cl->userinfo, "model", "player", sizeof( cl->userinfo ) );
		Mod_RegisterModel( "models/player.mdl", SV_ModelIndex( "models/player.mdl" ));
		SV_SetModel( ent, "models/player.mdl" );
	}

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
#if 0
static void SV_UpdateUserinfo_f( sv_client_t *cl )
{
	SV_UserinfoChanged( cl, Cmd_Argv( 1 ));
}
#endif

/*
==================
SV_Noclip_f
==================
*/
static void SV_Noclip_f( sv_client_t *cl )
{
	edict_t	*pEntity = cl->edict;

	if( !Cvar_VariableInteger( "sv_cheats" ) || sv.background || !sv_allow_noclip->integer )
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
SV_Kill_f
==================
*/
void SV_Kill_f( sv_client_t *cl )
{
	if( sv.background )
		return;

	if( !cl || !SV_IsValidEdict( cl->edict ))
		return;

	if( cl->edict->v.health <= 0.0f )
	{
		SV_ClientPrintf( cl, PRINT_HIGH, "Can't suicide -- already dead!\n");
		return;
	}

	svgame.dllFuncs.pfnClientKill( cl->edict );
}

/*
==================
SV_Godmode_f
==================
*/
static void SV_Godmode_f( sv_client_t *cl )
{
	edict_t	*pEntity = cl->edict;

	if( !Cvar_VariableInteger( "sv_cheats" ) || sv.background || !sv_allow_noclip->value )
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

static void SV_SendBuildInfo_f( sv_client_t *cl )
{
	SV_ClientPrintf( cl, PRINT_HIGH, "Server running Xash3D FWGS %s (build %i-%s, %s-%s)\n", XASH_VERSION, Q_buildnum(), Q_buildcommit(), Q_buildos(), Q_buildarch() );
}

edict_t *pfnFindEntityInSphere( edict_t *pStartEdict, const float *org, float flRadius );

static edict_t *SV_GetCrossEnt( edict_t *player )
{
	edict_t *ent = EDICT_NUM(1);
	edict_t *closest = NULL;
	float flMaxDot = 0.94;
	vec3_t forward;
	vec3_t viewPos;
	int i;
	float maxLen = 1000;

	AngleVectors( player->v.v_angle, forward, NULL, NULL );
	VectorAdd( player->v.origin, player->v.view_ofs, viewPos );

	// find bmodels by trace
	{
		trace_t trace;
		vec3_t target;

		VectorMA( viewPos, 1000, forward, target );
		trace = SV_Move( viewPos, vec3_origin, vec3_origin, target, 0, player );
		closest = trace.ent;
		VectorSubtract( viewPos, trace.endpos, target );
		maxLen = VectorLength(target) + 30;
	}

	// check untraceable entities
	for ( i = 1; i < svgame.numEntities; i++, ent++ )
	{
		vec3_t vecLOS;
		vec3_t vecOrigin;
		float flDot, traceLen;
		vec3_t boxSize;
		trace_t trace;
		vec3_t vecTrace;

		if( ent->free )
			continue;

		if( ent->v.solid == SOLID_BSP || ent->v.movetype == MOVETYPE_PUSHSTEP )
			continue; //bsp models will be found by trace later

		// do not touch following weapons
		if( ent->v.movetype == MOVETYPE_FOLLOW )
			continue;

		if( ent == player )
			continue;

		VectorAdd( ent->v.absmin, ent->v.absmax, vecOrigin );
		VectorScale( vecOrigin, 0.5, vecOrigin );

		VectorSubtract( vecOrigin, viewPos, vecLOS );
		traceLen = VectorLength(vecLOS);

		if( traceLen > maxLen )
			continue;

		VectorCopy( ent->v.size, boxSize);
		VectorScale( boxSize, 0.5, boxSize );

		if ( vecLOS[0] > boxSize[0] )
			vecLOS[0] -= boxSize[0];
		else if ( vecLOS[0] < -boxSize[0] )
			vecLOS[0] += boxSize[0];
		else
			vecLOS[0] = 0;

		if ( vecLOS[1] > boxSize[1] )
			vecLOS[1] -= boxSize[1];
		else if ( vecLOS[1] < -boxSize[1] )
			vecLOS[1] += boxSize[1];
		else
			vecLOS[1] = 0;

		if ( vecLOS[2] > boxSize[2] )
			vecLOS[2] -= boxSize[2];
		else if ( vecLOS[2] < -boxSize[2] )
			vecLOS[2] += boxSize[2];
		else
			vecLOS[2] = 0;
		VectorNormalize( vecLOS );

		flDot = DotProduct (vecLOS , forward);
		if ( flDot <= flMaxDot )
			continue;

		trace = SV_Move( viewPos, vec3_origin, vec3_origin, vecOrigin, 0, player );
		VectorSubtract( trace.endpos, viewPos, vecTrace );
		if( VectorLength( vecTrace ) + 30 < traceLen )
			continue;
		closest = ent, flMaxDot = flDot;
	}

	return closest;
}

/*
===============
SV_EntList_f

Print list of entities to client
===============
*/
void SV_EntList_f( sv_client_t *cl )
{
	edict_t	*ent = NULL;
	int	i;

	for( i = 0; i < svgame.numEntities; i++ )
	{
		vec3_t borigin;
		ent = EDICT_NUM( i );
		if( !SV_IsValidEdict( ent )) continue;

		// filter by string
		if( Cmd_Argc() > 1 )
			if( !Q_stricmpext( Cmd_Argv( 1 ), STRING( ent->v.classname ) ) && !Q_stricmpext( Cmd_Argv( 1 ), STRING( ent->v.targetname ) ) )
				continue;
		VectorAdd( ent->v.absmin, ent->v.absmax, borigin );
		VectorScale( borigin, 0.5, borigin );

		SV_ClientPrintf( cl, PRINT_LOW, "%5i origin: %.f %.f %.f", i, ent->v.origin[0], ent->v.origin[1], ent->v.origin[2] );
		SV_ClientPrintf( cl, PRINT_LOW, "%5i borigin: %.f %.f %.f", i, borigin[0], borigin[1], borigin[2] );

		if( ent->v.classname )
			SV_ClientPrintf( cl, PRINT_LOW, ", class: %s", STRING( ent->v.classname ));

		if( ent->v.globalname )
			SV_ClientPrintf( cl, PRINT_LOW, ", global: %s", STRING( ent->v.globalname ));

		if( ent->v.targetname )
			SV_ClientPrintf( cl, PRINT_LOW, ", name: %s", STRING( ent->v.targetname ));

		if( ent->v.target )
			SV_ClientPrintf( cl, PRINT_LOW, ", target: %s", STRING( ent->v.target ));

		if( ent->v.model )
			SV_ClientPrintf( cl, PRINT_LOW, ", model: %s", STRING( ent->v.model ));

		SV_ClientPrintf( cl, PRINT_LOW, "\n" );
	}
}


edict_t *SV_EntFindSingle( sv_client_t *cl, const char *pattern )
{
	edict_t	*ent = NULL;
	int	i = 0;

	if( Q_isdigit( pattern ) )
	{
		i = Q_atoi( pattern );

		if( i >= svgame.numEntities )
			return NULL;
	}
	else if( !Q_stricmp( pattern, "!cross" ) )
	{
		ent = SV_GetCrossEnt( cl->edict );

		if( !SV_IsValidEdict( ent ) )
			return NULL;

		i = NUM_FOR_EDICT( ent );
	}
	else if( pattern[0] == '!' ) // Check for correct instanse with !(num)_(serial)
	{
		const char *p = pattern + 1;
		i = Q_atoi( p );

		while( isdigit(*p) ) p++;

		if( *p++ != '_' )
			return NULL;

		if( i >= svgame.numEntities )
			return NULL;

		ent = EDICT_NUM( i );

		if( ent->serialnumber != Q_atoi( p ) )
			return NULL;
	}
	else
	{
		for( i = svgame.globals->maxClients + 1; i < svgame.numEntities; i++ )
		{
			ent = EDICT_NUM( i );

			if( !SV_IsValidEdict( ent ) )
				continue;

			if( Q_stricmpext( pattern, STRING( ent->v.targetname ) ) )
				break;
		}
	}

	ent = EDICT_NUM( i );

	if( !SV_IsValidEdict( ent ) )
		return NULL;

	return ent;
}


/*
===============
SV_EntInfo_f

Print specified entity information to client
===============
*/
void SV_EntInfo_f( sv_client_t *cl )
{
	edict_t	*ent = NULL;
	vec3_t borigin;

	if( Cmd_Argc() != 2 )
	{
		SV_ClientPrintf( cl, PRINT_LOW, "Use ent_info <index|name|inst>\n" );
		return;
	}

	ent = SV_EntFindSingle( cl, Cmd_Argv( 1 ) );

	if( !SV_IsValidEdict( ent )) return;

	VectorAdd( ent->v.absmin, ent->v.absmax, borigin );
	VectorScale( borigin, 0.5, borigin );

	SV_ClientPrintf( cl, PRINT_LOW, "origin: %.f %.f %.f\n", ent->v.origin[0], ent->v.origin[1], ent->v.origin[2] );

	SV_ClientPrintf( cl, PRINT_LOW, "angles: %.f %.f %.f\n", ent->v.angles[0], ent->v.angles[1], ent->v.angles[2] );

	SV_ClientPrintf( cl, PRINT_LOW, "borigin: %.f %.f %.f\n", borigin[0], borigin[1], borigin[2] );

	if( ent->v.classname )
		SV_ClientPrintf( cl, PRINT_LOW, "class: %s\n", STRING( ent->v.classname ));

	if( ent->v.globalname )
		SV_ClientPrintf( cl, PRINT_LOW, "global: %s\n", STRING( ent->v.globalname ));

	if( ent->v.targetname )
		SV_ClientPrintf( cl, PRINT_LOW, "name: %s\n", STRING( ent->v.targetname ));

	if( ent->v.target )
		SV_ClientPrintf( cl, PRINT_LOW, "target: %s\n", STRING( ent->v.target ));

	if( ent->v.model )
		SV_ClientPrintf( cl, PRINT_LOW, "model: %s\n", STRING( ent->v.model ));

	SV_ClientPrintf( cl, PRINT_LOW, "health: %.f\n", ent->v.health );

	if( ent->v.gravity != 1.0f )
		SV_ClientPrintf( cl, PRINT_LOW, "gravity: %.2f\n", ent->v.gravity );

	SV_ClientPrintf( cl, PRINT_LOW, "movetype: %d\n", ent->v.movetype );

	SV_ClientPrintf( cl, PRINT_LOW, "rendermode: %d\n", ent->v.rendermode );
	SV_ClientPrintf( cl, PRINT_LOW, "renderfx: %d\n", ent->v.renderfx );
	SV_ClientPrintf( cl, PRINT_LOW, "renderamt: %f\n", ent->v.renderamt );
	SV_ClientPrintf( cl, PRINT_LOW, "rendercolor: %f %f %f\n", ent->v.rendercolor[0], ent->v.rendercolor[1], ent->v.rendercolor[2] );

	SV_ClientPrintf( cl, PRINT_LOW, "maxspeed: %f\n", ent->v.maxspeed );

	if( ent->v.solid )
		SV_ClientPrintf( cl, PRINT_LOW, "solid: %d\n", ent->v.solid );
	SV_ClientPrintf( cl, PRINT_LOW, "flags: 0x%x\n", ent->v.flags );
	SV_ClientPrintf( cl, PRINT_LOW, "spawnflags: 0x%x\n", ent->v.spawnflags );
}

void SV_EntSendVars( sv_client_t *cl, edict_t *ent )
{
	if( !ent )
		return;
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, va( "set ent_last_name \"%s\"\n", STRING( ent->v.targetname ) ));
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, va( "set ent_last_num %i\n", NUM_FOR_EDICT( ent ) ));
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, va( "set ent_last_inst !%i_%i\n", NUM_FOR_EDICT( ent ), ent->serialnumber ));
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, va( "set ent_last_origin \"%f %f %f\"\n", ent->v.origin[0], ent->v.origin[1], ent->v.origin[2]));
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, va( "set ent_last_class \"%s\"\n", STRING( ent->v.classname )));
	BF_WriteByte( &cl->netchan.message, svc_stufftext );
	BF_WriteString( &cl->netchan.message, "ent_getvars_cb\n" );
}

void SV_EntGetVars_f( sv_client_t *cl )
{
	edict_t *ent = NULL;

	if( Cmd_Argc() != 2 )
	{
		SV_ClientPrintf( cl, PRINT_LOW, "Use ent_getvars <index|name|inst>\n" );
		return;
	}

	ent = SV_EntFindSingle( cl, Cmd_Argv( 1 ) );
	if( Cmd_Argc() )
	if( !SV_IsValidEdict( ent )) return;
	SV_EntSendVars( cl, ent );
}

/*
===============
SV_EntFire_f

Perform some actions
===============
*/
void SV_EntFire_f( sv_client_t *cl )
{
	edict_t	*ent = NULL;
	int	i = 1, count = 0;
	qboolean single; // true if user specified something that match single entity

	if( Cmd_Argc() < 3 )
	{
		SV_ClientPrintf( cl, PRINT_LOW, "Use ent_fire <index||pattern> <command> [<values>]\n"
			"Use ent_fire 0 help to get command list\n" );
		return;
	}

	if( ( single = Q_isdigit( Cmd_Argv( 1 ) ) ) )
	{
		i = Q_atoi( Cmd_Argv( 1 ) );

		if( i < 0 || i >= svgame.numEntities )
			return;

		ent = EDICT_NUM( i );
	}
	else if( ( single = !Q_stricmp( Cmd_Argv( 1 ), "!cross" ) ) )
	{
		ent = SV_GetCrossEnt( cl->edict );

		if( !SV_IsValidEdict( ent ) )
			return;

		i = NUM_FOR_EDICT( ent );
	}
	else if( ( single = ( Cmd_Argv( 1 )[0] == '!') ) ) // Check for correct instanse with !(num)_(serial)
	{
		char *cmd = Cmd_Argv( 1 ) + 1;
		i = Q_atoi( cmd );

		while( isdigit( *cmd ) ) cmd++;

		if( *cmd++ != '_' )
			return;

		if( i < 0 || i >= svgame.numEntities )
			return;

		ent = EDICT_NUM( i );
		if( ent->serialnumber != Q_atoi( cmd ) )
			return;
	}
	else
	{
		i = svgame.globals->maxClients + 1;
	}

	for( ; ( i <  svgame.numEntities ) && ( count < sv_enttools_maxfire->integer ); i++ )
	{
		ent = EDICT_NUM( i );
		if( !SV_IsValidEdict( ent ))
		{
			// SV_ClientPrintf( cl, PRINT_LOW, "Got invalid entity\n" );
			if( single )
				break;
			continue;
		}

		// if user specified not a number, try find such entity
		if( !single )
		{
			if( !Q_stricmpext( Cmd_Argv( 1 ), STRING( ent->v.targetname ) ) && !Q_stricmpext( Cmd_Argv( 1 ), STRING( ent->v.classname ) ))
				continue;
		}

		SV_ClientPrintf( cl, PRINT_LOW, "entity %i\n", i );

		count++;

		if( !Q_stricmp( Cmd_Argv( 2 ), "health" ) )
			ent->v.health = Q_atoi( Cmd_Argv ( 3 ) );
		else if( !Q_stricmp( Cmd_Argv( 2 ), "gravity" ) )
			ent->v.gravity = Q_atof( Cmd_Argv ( 3 ) );
		else if( !Q_stricmp( Cmd_Argv( 2 ), "movetype" ) )
			ent->v.movetype = Q_atoi( Cmd_Argv ( 3 ) );
		else if( !Q_stricmp( Cmd_Argv( 2 ), "solid" ) )
			ent->v.solid = Q_atoi( Cmd_Argv ( 3 ) );
		else if( !Q_stricmp( Cmd_Argv( 2 ), "rename" ) )
			ent->v.targetname = ALLOC_STRING( Cmd_Argv ( 3 ) );
		else if( !Q_stricmp( Cmd_Argv( 2 ), "settarget" ) )
			ent->v.target = ALLOC_STRING( Cmd_Argv ( 3 ) );
		else if( !Q_stricmp( Cmd_Argv( 2 ), "setmodel" ) )
			SV_SetModel( ent, Cmd_Argv( 3 ) );
		else if( !Q_stricmp( Cmd_Argv( 2 ), "set" ) )
		{
			char keyname[MAX_STRING];
			char value[MAX_STRING];
			KeyValueData	pkvd;
			if( Cmd_Argc() != 5 )
				return;
			pkvd.szClassName = (char*)STRING( ent->v.classname );
			Q_strncpy( keyname, Cmd_Argv( 3 ), MAX_STRING );
			Q_strncpy( value, Cmd_Argv( 4 ), MAX_STRING );
			pkvd.szKeyName = keyname;
			pkvd.szValue = value;
			pkvd.fHandled = false;
			svgame.dllFuncs.pfnKeyValue( ent, &pkvd );
			if( pkvd.fHandled )
				SV_ClientPrintf( cl, PRINT_LOW, "value set successfully!\n" );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "touch" ) )
		{
			if( Cmd_Argc() == 4 )
			{
				edict_t *other = SV_EntFindSingle( cl, Cmd_Argv( 3 ) );
				if( other && other->pvPrivateData )
					svgame.dllFuncs.pfnTouch( ent, other  );
			}
			else
				svgame.dllFuncs.pfnTouch( ent, cl->edict );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "use" ) )
		{
			if( Cmd_Argc() == 4 )
			{
				edict_t *other = SV_EntFindSingle( cl, Cmd_Argv( 3 ) );
				if( other && other->pvPrivateData )
					svgame.dllFuncs.pfnUse( ent, other );
			}
			else
				svgame.dllFuncs.pfnUse( ent, cl->edict );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "movehere" ) )
		{
			ent->v.origin[2] = cl->edict->v.origin[2] + 25;
			ent->v.origin[1] = cl->edict->v.origin[1] + 100 * sin( DEG2RAD( cl->edict->v.angles[1] ) );
			ent->v.origin[0] = cl->edict->v.origin[0] + 100 * cos( DEG2RAD( cl->edict->v.angles[1] ) );
			SV_LinkEdict( ent, true );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "drop2floor" ) )
		{
			pfnDropToFloor( ent );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "moveup" ) )
		{
			float dist = 25;
			if( Cmd_Argc() >= 4 )
				dist = Q_atof( Cmd_Argv( 3 ) );
			ent->v.origin[2] +=  dist;
			if( Cmd_Argc() >= 5 )
			{
				dist = Q_atof( Cmd_Argv( 4 ) );
				ent->v.origin[0] += dist * cos( DEG2RAD( cl->edict->v.angles[1] ) );
				ent->v.origin[1] += dist * sin( DEG2RAD( cl->edict->v.angles[1] ) );
			}

		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "becomeowner" ) )
		{
			if( Cmd_Argc() == 4 )
				ent->v.owner = SV_EntFindSingle( cl, Cmd_Argv( 3 ) );
			else
				ent->v.owner = cl->edict;
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "becomeenemy" ) )
		{
			if( Cmd_Argc() == 4 )
				ent->v.enemy = SV_EntFindSingle( cl, Cmd_Argv( 3 ) );
			else
				ent->v.enemy = cl->edict;
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "becomeaiment" ) )
		{
			if( Cmd_Argc() == 4 )
				ent->v.aiment= SV_EntFindSingle( cl, Cmd_Argv( 3 ) );
			else
				ent->v.aiment = cl->edict;
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "hullmin" ) )
		{
			if( Cmd_Argc() != 6 )
				return;
			ent->v.mins[0] = Q_atof( Cmd_Argv( 3 ) );
			ent->v.mins[1] = Q_atof( Cmd_Argv( 4 ) );
			ent->v.mins[2] = Q_atof( Cmd_Argv( 5 ) );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "hullmax" ) )
		{
			if( Cmd_Argc() != 6 )
				return;
			ent->v.maxs[0] = Q_atof( Cmd_Argv( 3 ) );
			ent->v.maxs[1] = Q_atof( Cmd_Argv( 4 ) );
			ent->v.maxs[2] = Q_atof( Cmd_Argv( 5 ) );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "rendercolor" ) )
		{
			if( Cmd_Argc() != 6 )
				return;
			ent->v.rendercolor[0] = Q_atof( Cmd_Argv( 3 ) );
			ent->v.rendercolor[1] = Q_atof( Cmd_Argv( 4 ) );
			ent->v.rendercolor[2] = Q_atof( Cmd_Argv( 5 ) );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "renderamt" ) )
		{
			ent->v.renderamt = Q_atof( Cmd_Argv( 3 ) );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "renderfx" ) )
		{
			ent->v.renderfx = Q_atoi( Cmd_Argv( 3 ) );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "rendermode" ) )
		{
			ent->v.rendermode = Q_atoi( Cmd_Argv( 3 ) );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "angles" ) )
		{
			ent->v.angles[0] = Q_atof( Cmd_Argv( 3 ) );
			ent->v.angles[1] = Q_atof( Cmd_Argv( 4 ) );
			ent->v.angles[2] = Q_atof( Cmd_Argv( 5 ) );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "setflag" ) )
		{
			ent->v.flags |= 1U << Q_atoi( Cmd_Argv ( 3 ) );
			SV_ClientPrintf( cl, PRINT_LOW, "flags set to 0x%x\n", ent->v.flags );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "clearflag" ) )
		{
			ent->v.flags &= ~( 1U << Q_atoi( Cmd_Argv ( 3 ) ) );
			SV_ClientPrintf( cl, PRINT_LOW, "flags set to 0x%x\n", ent->v.flags );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "setspawnflag" ) )
		{
			ent->v.spawnflags |= 1U << Q_atoi( Cmd_Argv ( 3 ) );
			SV_ClientPrintf( cl, PRINT_LOW, "spawnflags set to 0x%x\n", ent->v.spawnflags );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "clearspawnflag" ) )
		{
			ent->v.spawnflags &= ~( 1U << Q_atoi( Cmd_Argv ( 3 ) ) );
			SV_ClientPrintf( cl, PRINT_LOW, "spawnflags set to 0x%x\n", ent->v.flags );
		}
		else if( !Q_stricmp( Cmd_Argv( 2 ), "help" ) )
		{
			SV_ClientPrintf( cl, PRINT_LOW, "Available commands:\n"
				"Set fields:\n"
				"        (Only set entity field, does not call any functions)\n"
				"    health\n"
				"    gravity\n"
				"    movetype\n"
				"    solid\n"
				"    rendermode\n"
				"    rendercolor (vector)\n"
				"    renderfx\n"
				"    renderamt\n"
				"    hullmin (vector)\n"
				"    hullmax (vector)\n"
				"Actions\n"
				"    rename: set entity targetname\n"
				"    settarget: set entity target (only targetnames)\n"
				"    setmodel: set entity model\n"
				"    set: set <key> <value> by server library\n"
				"        See game FGD to get list.\n"
				"        command takes two arguments\n"
				"    touch: touch entity by current player.\n"
				"    use: use entity by current player.\n"
				"    movehere: place entity in player fov.\n"
				"    drop2floor: place entity to nearest floor surface\n"
				"    moveup: move entity to 25 units up\n"
				"Flags:\n"
				"        (Set/clear specified flag bit, arg is bit number)\n"
				"    setflag\n"
				"    clearflag\n"
				"    setspawnflag\n"
				"    clearspawnflag\n"
			);
			return;
		}
		else
		{
			SV_ClientPrintf( cl, PRINT_LOW, "Unknown command %s!\nUse \"ent_fire 0 help\" to list commands.\n", Cmd_Argv( 2 ) );
			return;
		}
		if( single )
			break;
	}
}

/*
===============
SV_EntCreate_f

Create new entity with specified name.
===============
*/
void SV_EntCreate_f( sv_client_t *cl )
{
	edict_t	*ent = NULL;
	int	i = 0;
	string_t classname;

	if( Cmd_Argc() < 2 )
	{
		SV_ClientPrintf( cl, PRINT_LOW, "Use ent_create <classname> <key1> <value1> <key2> <value2> ...\n" );
		return;
	}

	classname = ALLOC_STRING( Cmd_Argv( 1 ) );

	ent = SV_AllocPrivateData( 0, classname );

	// Xash3D extesion
	if( !ent && svgame.physFuncs.SV_CreateEntity )
	{
		ent = SV_AllocEdict();
		ent->v.classname = classname;
		if( svgame.physFuncs.SV_CreateEntity( ent, (char*)STRING( classname ) ) == -1 )
		{
			if( ent && !ent->free )
				SV_FreeEdict( ent );
			ent = NULL;
		}
	}

	// XashXT does not implement SV_CreateEntity, use saverestore export
	if( !ent && svgame.physFuncs.pfnCreateEntitiesInRestoreList )
	{
		SAVERESTOREDATA data = { 0 };
		ENTITYTABLE table = { 0 };
		data.tableCount = 1;
		data.pTable = &table;
		table.classname = classname;
		table.id = -1;
		table.size = 1;
		svgame.physFuncs.pfnCreateEntitiesInRestoreList( &data, 0 );
		ent = table.pent;
	}

	if( !ent )
	{
		SV_ClientPrintf( cl, PRINT_LOW, "Invalid entity!\n" );
		return;
	}

	// choose default origin
	ent->v.origin[2] = cl->edict->v.origin[2] + 25;
	ent->v.origin[1] = cl->edict->v.origin[1] + 100 * sin( DEG2RAD( cl->edict->v.angles[1] ) );
	ent->v.origin[0] = cl->edict->v.origin[0] + 100 * cos( DEG2RAD( cl->edict->v.angles[1] ) );

	SV_LinkEdict( ent, false );

	// apply keyvales if supported
	if( svgame.dllFuncs.pfnKeyValue )
	{
		for( i = 2; i < Cmd_Argc() - 1; i++ )
		{
			KeyValueData	pkvd;

			// allow split keyvalues to prespawn and postspawn
			if( !Q_strcmp( Cmd_Argv( i ), "|" ) )
				break;

			pkvd.fHandled = false;
			pkvd.szClassName = (char*)STRING( ent->v.classname );
			pkvd.szKeyName = Cmd_Argv( i );
			i++;
			pkvd.szValue = Cmd_Argv( i );
			svgame.dllFuncs.pfnKeyValue( ent, &pkvd );
			if( pkvd.fHandled )
				SV_ClientPrintf( cl, PRINT_LOW, "value \"%s\" set to \"%s\"!\n", pkvd.szKeyName, pkvd.szValue );
		}
	}

	// set default targetname
	if( !ent->v.targetname )
	{
		char newname[256], clientname[256];
		int j;

		for( j = 0; j < sizeof( cl->name ); j++ )
		{
			char c = Q_tolower( cl->name[j] );
			if( c < 'a' || c > 'z' )
				c = '_';
			if( !cl->name[j] )
			{
				clientname[j] = 0;
				break;
			}
			clientname[j] = c;
		}

		// generate name based on nick name and index
		Q_snprintf( newname, 256,  "%s_%i_e%i", clientname, cl->userid, NUM_FOR_EDICT( ent ) );

		// i know, it may break strict aliasing rules
		// but we will not lose anything in this case.
		Q_strnlwr( newname, newname, 256 );
		ent->v.targetname = ALLOC_STRING( newname );
		SV_EntSendVars( cl, ent );
	}

	SV_ClientPrintf( cl, PRINT_LOW, "Created %i: %s, targetname %s\n", NUM_FOR_EDICT( ent ), Cmd_Argv( 1 ), STRING( ent->v.targetname ) );

	if( svgame.dllFuncs.pfnSpawn )
		svgame.dllFuncs.pfnSpawn( ent );

	// now drop entity to floor.
	pfnDropToFloor( ent );

	// force think. Otherwise given weapon may crash server if player touch it before.
	svgame.dllFuncs.pfnThink( ent );
	pfnDropToFloor( ent );

	// apply postspawn keyvales if supported
	if( svgame.dllFuncs.pfnKeyValue )
	{
		for( i = i + 1; i < Cmd_Argc() - 1; i++ )
		{
			KeyValueData	pkvd;

			pkvd.fHandled = false;
			pkvd.szClassName = (char*)STRING( ent->v.classname );
			pkvd.szKeyName = Cmd_Argv( i );
			i++;
			pkvd.szValue = Cmd_Argv( i );
			svgame.dllFuncs.pfnKeyValue( ent, &pkvd );
			if( pkvd.fHandled )
				SV_ClientPrintf( cl, PRINT_LOW, "value \"%s\" set to \"%s\"!\n", pkvd.szKeyName, pkvd.szValue );
		}
	}

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
//{ "userinfo", SV_UpdateUserinfo_f },
{ "lightstyles", SV_WriteLightstyles_f },
{ "getresourcelist", SV_SendResourceList_f },
{ "continueloading", SV_ContinueLoading_f },
{ "kill", SV_Kill_f },
{ "_sv_build_info", SV_SendBuildInfo_f },
{ NULL, NULL }
};

ucmd_t enttoolscmds[] =
{
{ "ent_list", SV_EntList_f },
{ "ent_info", SV_EntInfo_f },
{ "ent_fire", SV_EntFire_f },
{ "ent_create", SV_EntCreate_f },
{ "ent_getvars", SV_EntGetVars_f },
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

	if( !u->name && sv_enttools_enable->integer && !sv.background )
	{
		for( u = enttoolscmds; u->name; u++ )
		{
			if( !Q_strcmp( Cmd_Argv( 0 ), u->name ))
			{
				MsgDev( D_NOTE, "enttools->%s(): %s\n", u->name, s );

				Log_Printf( "\"%s<%i><%s><>\" performed: %s\n", Info_ValueForKey( cl->userinfo, "name" ),
							cl->userid, SV_GetClientIDString( cl ), NET_AdrToString( cl->netchan.remote_address ), s );

				if( u->func ) u->func( cl );
				break;
			}
		}
	}

	if( !u->name && sv.state == ss_active )
	{
		qboolean isFullUpdate = !Q_strcmp( Cmd_Argv( 0 ), "fullupdate");
		// custom client commands
		svgame.dllFuncs.pfnClientCommand( cl->edict );

		if( isFullUpdate )
		{
			// resend the ambient sounds for demo recording
			Host_RestartAmbientSounds();
			// resend all the decals for demo recording
			Host_RestartDecals();
			// resend all the static ents for demo recording
			SV_RestartStaticEnts();
			// resend userinfo
			SV_RefreshUserinfo();
		}
	}
}

void SV_TSourceEngineQuery( netadr_t from )
{
	// A2S_INFO
	char answer[1024] = "";
	sizebuf_t buf;
	int count = 0, bots = 0, index;
	int havePassword;

	if( svs.clients )
	{
		for( index = 0; index < sv_maxclients->integer; index++ )
		{
			if( svs.clients[index].state >= cs_connected )
			{
				if( svs.clients[index].fakeclient )
					bots++;
				else count++;
			}
		}
	}

	havePassword = ( sv_password->string[0] && Q_stricmp( sv_password->string, "none" ) ) ? 0 : 1;

	BF_Init( &buf, "TSourceEngineQuery", answer, sizeof( answer ));

#if 1 // Source format
	BF_WriteLong  ( &buf, -1 ); // Mark as connectionless
	BF_WriteByte  ( &buf, 'I' );
	BF_WriteByte  ( &buf, PROTOCOL_VERSION );
	BF_WriteString( &buf, hostname->string );
	BF_WriteString( &buf, sv.name );
	BF_WriteString( &buf, GI->gamefolder );
	BF_WriteString( &buf, svgame.dllFuncs.pfnGetGameDescription() );
	BF_WriteShort ( &buf, 0 ); // steam id
	BF_WriteByte  ( &buf, count );
	BF_WriteByte  ( &buf, sv_maxclients->integer );
	BF_WriteByte  ( &buf, bots );
	BF_WriteByte  ( &buf, Host_IsDedicated() ? 'd' : 'l');
#if defined(_WIN32)
	BF_WriteByte  ( &buf, 'w' );
#elif defined(__APPLE__)
	BF_WriteByte  ( &buf, 'm' );
#else
	BF_WriteByte  ( &buf, 'l' );
#endif
	BF_WriteByte  ( &buf, havePassword ); // visibility
	BF_WriteByte  ( &buf, 0 ); // secure
#else // GS format
	/*
	**	This will work in monitoring,
	**	but does not appear in the list of GoldSource servers
	*/
	BF_WriteLong(&buf, -1);// Fixed this
	BF_WriteByte( &buf, 'm' );
	BF_WriteString( &buf, NET_AdrToString( net_local ) );
	BF_WriteString( &buf, hostname->string );
	BF_WriteString( &buf, sv.name );
	BF_WriteString( &buf, GI->gamefolder );
	BF_WriteString( &buf, GI->title );
	BF_WriteByte( &buf, count );
	BF_WriteByte( &buf, sv_maxclients->integer );
	BF_WriteByte( &buf, PROTOCOL_VERSION );
	BF_WriteByte( &buf, Host_IsDedicated() ? 'D' : 'L');
#if defined(_WIN32)
	BF_WriteByte( &buf, 'W' );
#else
	BF_WriteByte( &buf, 'L' );
#endif
	BF_WriteByte( &buf, havePassword );
	if( Q_stricmp( GI->gamedir, "valve" ) )
	{
		BF_WriteByte( &buf, 1 ); // mod
		BF_WriteString( &buf, GI->game_url );
		BF_WriteString( &buf, GI->update_url );
		BF_WriteByte( &buf, 0 );
		BF_WriteLong( &buf, (long)GI->version );
		BF_WriteLong( &buf, GI->size );
		if( GI->gamemode == 2 )
			BF_WriteByte( &buf, 1 ); // multiplayer_only
		else
			BF_WriteByte( &buf, 0 );
		if( Q_strstr(SI.gamedll, "hl." ) )
			BF_WriteByte( &buf, 0 ); // Half-Life DLL
		else
			BF_WriteByte( &buf, 1 ); // Own DLL
	}
	else
	{
		BF_WriteByte( &buf, 0 ); // Half-Life
	}
	BF_WriteByte( &buf, 0 ); // unsecure
	BF_WriteByte( &buf, bots );
#endif
	NET_SendPacket( NS_SERVER, BF_GetNumBytesWritten( &buf ), BF_GetData( &buf ), from );
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

	// prevent flooding from banned address
	if( SV_CheckIP( &from ) )
		return;

	BF_Clear( msg );
	BF_ReadLong( msg );// skip the -1 marker

	args = BF_ReadStringLine( msg );
	Cmd_TokenizeString( args );

	c = Cmd_Argv( 0 );
	MsgDev( D_NOTE, "SV_ConnectionlessPacket: %s : %s\n", NET_AdrToString( from ), c );

	if( !Q_strcmp( c, "ping" )) SV_Ping( from );
	else if( !Q_strcmp( c, "ack" )) SV_Ack( from );
	else if( !Q_strcmp( c, "status" )) SV_Status( from );
	else if( !Q_strcmp( c, "info" )) SV_Info( from, Q_atoi( Cmd_Argv(1) ) );
	else if( !Q_strcmp( c, "getchallenge" )) SV_GetChallenge( from );
	else if( !Q_strcmp( c, "connect" )) SV_DirectConnect( from );
	else if( !Q_strcmp( c, "rcon" )) SV_RemoteCommand( from, msg );
	else if( !Q_strcmp( c, "netinfo" )) SV_BuildNetAnswer( from );
	else if( !Q_strcmp( c, "s")) SV_AddToMaster( from, msg );
	else if( !Q_strcmp( c, "T" "Source" ) ) SV_TSourceEngineQuery( from );
	else if( !Q_strcmp( c, "c" ) )
	{
		netadr_t to;

		if( NET_StringToAdr( Cmd_Argv( 1 ), &to ) )
			SV_Info( to, PROTOCOL_VERSION );
	}
	else if( !Q_strcmp( c, "i" ) )
	{
		// A2A_PING
		Netchan_OutOfBandPrint( NS_SERVER, from, "j" );
	}
	else if( svgame.dllFuncs.pfnConnectionlessPacket( &from, args, buf, &len ))
	{
		// user out of band message (must be handled in CL_ConnectionlessPacket)
		if( len > 0 ) Netchan_OutOfBand( NS_SERVER, from, len, (byte *)buf );
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
		MsgDev( D_ERROR, "%s sent too many commands: %i\n", cl->name, numcmds );
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
#ifndef XASH_BIG_ENDIAN
	checksum2 = CRC32_BlockSequence( msg->pData + key + 1, size, cl->netchan.incoming_sequence );

	if( checksum2 != checksum1 )
	{
		MsgDev( D_ERROR, "SV_UserMove: failed command checksum for %s (%d != %d)\n", cl->name, checksum2, checksum1 );
		return;
	}
#endif

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
	// Fragment download is unstable
	if( sv_allow_fragment->integer )
	{
		Netchan_CreateFileFragments( true, &cl->netchan, BF_ReadString( msg ));
		Netchan_FragSend( &cl->netchan );
	}
	else
	{
		SV_ClientPrintf( cl, PRINT_HIGH, "Direct download not allowed on this sever\n" );
		SV_DropClient( cl );
	}
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
	frame->ping_time = host.realtime - frame->senttime - cl->cl_updaterate;

	// on first frame ( no senttime ) don't skew ping
	if( frame->senttime == 0.0f )
	{
		frame->latency = 0.0f;
		frame->ping_time = 0.0f;
	}

	// don't skew ping based on signon stuff either
	if(( host.realtime - cl->lastconnect ) < 2.0f && ( frame->latency > 0.0 ))
	{
		frame->latency = 0.0f;
		frame->ping_time = 0.0f;
	}

	cl->latency = SV_CalcClientTime( cl );
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
