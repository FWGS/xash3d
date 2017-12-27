/*
sv_query.c - Source-like server querying
Copyright (C) 2018

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

#define SOURCE_QUERY_INFO 'T'
#define SOURCE_QUERY_DETAILS 'I'

#define SOURCE_QUERY_RULES 'V'
#define SOURCE_QUERY_RULES_RESPONSE 'E'

#define SOURCE_QUERY_PLAYERS 'U'
#define SOURCE_QUERY_PLAYERS_RESPONSE 'D'

#define SOURCE_QUERY_PING 'i'
#define SOURCE_QUERY_ACK 'j'

#define SOURCE_QUERY_CONNECTIONLESS -1

void SV_SourceQuery_Details( netadr_t from )
{
	sizebuf_t buf;
	char answer[2048] = "";
	char version[128] = "";

	int i, bot_count = 0, client_count = 0;
	int is_private = 0;

	if ( svs.clients )
	{
		for ( i = 0; i < sv_maxclients->integer; i++ )
		{
			if ( svs.clients[i].state >= cs_connected )
			{
				if ( svs.clients[i].fakeclient )
					bot_count++;
				else
					client_count++;
			}
		}
	}
	is_private = ( sv_password->string[0] && ( Q_stricmp( sv_password->string, "none" ) || !Q_strlen( sv_password->string ) ) ? 1 : 0 );

	BF_Init( &buf, "XASH->INFO", answer, sizeof( answer ) );

	BF_WriteLong( &buf, SOURCE_QUERY_CONNECTIONLESS );
	BF_WriteByte( &buf, SOURCE_QUERY_DETAILS );
	BF_WriteByte( &buf, PROTOCOL_VERSION );

	BF_WriteString( &buf, hostname->string );
	BF_WriteString( &buf, sv.name );
	BF_WriteString( &buf, GI->gamefolder );
	BF_WriteString( &buf, svgame.dllFuncs.pfnGetGameDescription( ) );

	BF_WriteShort( &buf, 0 );
	BF_WriteByte( &buf, client_count );
	BF_WriteByte( &buf, sv_maxclients->integer );
	BF_WriteByte( &buf, bot_count );

	BF_WriteByte( &buf, Host_IsDedicated( ) ? 'd' : 'l' );
#if defined( _WIN32 )
	BF_WriteByte( &buf, 'w' );
#elif defined( __APPLE__ )
	BF_WriteByte( &buf, 'm' );
#else
	BF_WriteByte( &buf, 'l' );
#endif
	BF_WriteByte( &buf, is_private );
	BF_WriteByte( &buf, 0 );
	BF_WriteString( &buf, XASH_VERSION );

	NET_SendPacket( NS_SERVER, BF_GetNumBytesWritten( &buf ), BF_GetData( &buf ), from );
}

void SV_SourceQuery_Rules( netadr_t from )
{
	sizebuf_t buf;
	char answer[1024 * 8] = "";

	cvar_t *cvar;
	int cvar_count = 0;

	for ( cvar = Cvar_GetList( ); cvar; cvar = cvar->next )
	{
		if ( cvar->flags & ( CVAR_SERVERNOTIFY | CVAR_SERVERINFO ) )
			cvar_count++;
	}
	if ( cvar_count <= 0 )
		return;

	BF_Init( &buf, "XASH->RULES", answer, sizeof( answer ) );

	BF_WriteLong( &buf, SOURCE_QUERY_CONNECTIONLESS );
	BF_WriteByte( &buf, SOURCE_QUERY_RULES_RESPONSE );
	BF_WriteShort( &buf, cvar_count );

	for ( cvar = Cvar_GetList( ); cvar; cvar = cvar->next )
	{
		if ( !( cvar->flags & ( CVAR_SERVERNOTIFY | CVAR_SERVERINFO ) ) )
			continue;

		BF_WriteString( &buf, cvar->name );

		if ( cvar->flags & CVAR_PROTECTED )
			BF_WriteString( &buf, ( Q_strlen( cvar->string ) > 0 && Q_stricmp( cvar->string, "none" ) ) ? "1" : "0" );
		else
			BF_WriteString( &buf, cvar->string );
	}
	NET_SendPacket( NS_SERVER, BF_GetNumBytesWritten( &buf ), BF_GetData( &buf ), from );
}

void SV_SourceQuery_Players( netadr_t from )
{
	sizebuf_t buf;
	char answer[1024 * 8] = "";

	int i, client_count = 0;

	if ( svs.clients )
	{
		for ( i = 0; i < sv_maxclients->integer; i++ )
		{
			if ( svs.clients[i].state >= cs_connected )
				client_count++;
		}
	}
	if ( client_count <= 0 )
		return;

	BF_Init( &buf, "XASH->PLAYERS", answer, sizeof( answer ) );

	BF_WriteLong( &buf, SOURCE_QUERY_CONNECTIONLESS );
	BF_WriteByte( &buf, SOURCE_QUERY_PLAYERS_RESPONSE );
	BF_WriteByte( &buf, client_count );

	for ( i = 0; i < sv_maxclients->integer; i++ )
	{
		sv_client_t *cl = &svs.clients[i];

		if ( cl->state < cs_connected )
			continue;

		BF_WriteByte( &buf, i );
		BF_WriteString( &buf, cl->name );
		BF_WriteLong( &buf, cl->edict->v.frags );
		BF_WriteFloat( &buf, cl->fakeclient ? -1.0 : ( host.realtime - cl->lastconnect ) );
	}
	NET_SendPacket( NS_SERVER, BF_GetNumBytesWritten( &buf ), BF_GetData( &buf ), from );
}

qboolean SV_SourceQuery_HandleConnnectionlessPacket( char *c, netadr_t from )
{
	int request = c[0];

	switch ( request )
	{
	case SOURCE_QUERY_INFO:
		SV_SourceQuery_Details( from );
		return TRUE;

	case SOURCE_QUERY_PING:
		Netchan_OutOfBandPrint( NS_SERVER, from, "%c00000000000000", SOURCE_QUERY_ACK );
		return TRUE;

	case SOURCE_QUERY_ACK:
		return TRUE;

	case SOURCE_QUERY_RULES:
		SV_SourceQuery_Rules( from );
		return TRUE;

	case SOURCE_QUERY_PLAYERS:
		SV_SourceQuery_Players( from );
		return TRUE;

	default:
		return FALSE;
	}
	return FALSE;
}
