/*
sv_filter.c - server ID/IP filter
Copyright (C) 2017 a1batross

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
typedef struct ipfilter_s
{
	float time;
	float endTime; // -1 for permanent ban
	struct ipban_s *prev;
	struct ipban_s *next;
	uint mask;
	uint compar;
} ipfilter_t;

ipfilter_t *ipfilter = NULL, *ipfilterLast = NULL;
*/

// TODO: Is IP filter really needed?
// TODO: Make it IPv6 compatible, for future expansion

typedef struct cidfilter_s
{
	float endTime;
	struct cidfilter_s *next;
	string id;
} cidfilter_t;

cidfilter_t *cidfilter = NULL;

void SV_RemoveID( const char *id )
{
	cidfilter_t *filter, *prevfilter;

	for( filter = cidfilter; filter; filter = filter->next )
	{
		if( Q_strcmp( filter->id, id ) )
		{
			prevfilter = filter;
			continue;
		}

		if( filter == cidfilter )
		{
			cidfilter = cidfilter->next;
			Mem_Free( filter );
			return;
		}

		prevfilter->next = filter->next;
		Mem_Free( filter );
		return;
	}
}

qboolean SV_CheckID( const char *id )
{
	qboolean ret = false;
	cidfilter_t *filter;

	for( filter = cidfilter; filter; filter = filter->next )
	{
		while( filter->endTime && host.realtime > filter->endTime )
		{
			char *fid = filter->id;
			filter = filter->next;
			SV_RemoveID( fid );
		}

		if( !Q_strcmp( id, filter->id ) )
		{
			ret = true;
			break;
		}
	}

	return ret;
}

// qboolean SV_CheckIP( netadr_t * ) { }

void SV_BanID_f( void )
{
	float time = Q_atof( Cmd_Argv( 1 ) );
	char *id = Cmd_Argv( 2 );
	sv_client_t *cl = NULL;
	cidfilter_t *filter;

	if( time )
		time = host.realtime + time * 60.0f;

	if( !id[0] )
	{
		Msg( "Usage: banid <minutes> <#userid or unique id>\n0 minutes for permanent ban\n");
		return;
	}

	if( !Q_strnicmp( id, "STEAM_", 6 ) || !Q_strnicmp( id, "VALVE_", 6 ) )
		id += 6;
	if( !Q_strnicmp( id, "XASH_", 5 ) )
		id += 5;

	if( svs.clients )
	{
		if( id[0] == '#' )
			cl = SV_ClientById( Q_atoi( id + 1 ) );

		if( !cl )
		{
			int i;
			sv_client_t *cl1;

			for( i = 0, cl1 = svs.clients; i < sv_maxclients->integer; i++, cl1++ )
				if( !Q_strcmp( id, Info_ValueForKey( cl1->useragent, "i") ) )
				{
					cl = cl1;
					break;
				}
		}

		if( !cl )
		{
			MsgDev( D_WARN, "banid: no such player\n");
		}
		else
			id = Info_ValueForKey( cl->useragent, "i" );

		if( !id[0] )
		{
			MsgDev( D_ERROR, "Could not ban, not implemented yet\n");
			return;
		}
	}

	if( !id[0] || id[0] == '#' )
	{
		MsgDev( D_ERROR, "banid: bad id\n");
		return;
	}

	SV_RemoveID( id );

	filter = Mem_Alloc( host.mempool, sizeof( cidfilter_t ) );
	filter->endTime = time;
	filter->next = cidfilter;
	Q_strncpy( filter->id, id, sizeof( filter->id ) );
	cidfilter = filter;

	if( cl && !Q_stricmp( Cmd_Argv( Cmd_Argc() - 1 ), "kick" ) )
		Cbuf_AddText( va( "kick #%d \"Kicked and banned\"\n", cl->userid ) );
}

void SV_ListID_f( void )
{
	cidfilter_t *filter;
	Msg( "id ban list\n" );
	Msg( "-----------\n" );

	for( filter = cidfilter; filter; filter = filter->next )
	{
		if( filter->endTime && host.realtime > filter->endTime )
			continue; // no negative time

		if( filter->endTime )
			Msg( "%s expries in %f minutes\n", filter->id, ( filter->endTime - host.realtime ) / 60.0f );
		else
			Msg( "%s permanent\n", filter->id );
	}
}
void SV_RemoveID_f( void )
{
	char *id = Cmd_Argv( 1 );

	if( id[0] == '#' && svs.clients )
	{
		int num = Q_atoi( id + 1 );

		if( num >= sv_maxclients->integer || num < 0 )
			return;

		id = Info_ValueForKey( svs.clients[num].useragent, "i" );
	}

	if( !id[0] )
	{
		Msg("Usage: removeid <#slotnumber or uniqueid>\n");
		return;
	}

	SV_RemoveID( id );
}
void SV_WriteID_f( void )
{
	file_t *f = FS_Open( Cvar_VariableString( "bannedcfgfile" ), "w", false );
	cidfilter_t *filter;

	if( !f )
	{
		MsgDev( D_ERROR, "Could not write %s\n", Cvar_VariableString( "bannedcfgfile" ) );
		return;
	}

	FS_Printf( f, "//=======================================================================\n" );
	FS_Printf( f, "//\t\t\tCopyright Flying With Gauss Team %s ©\n", Q_timestamp( TIME_YEAR_ONLY ));
	FS_Printf( f, "//\t\t    %s - archive of id blacklist\n", Cvar_VariableString( "bannedcfgfile" ) );
	FS_Printf( f, "//=======================================================================\n" );

	for( filter = cidfilter; filter; filter = filter->next )
		if( !filter->endTime ) // only permanent
			FS_Printf( f, "banid 0 %s\n", filter->id );

	FS_Close( f );
}
void SV_AddIP_f( void ) { }
void SV_ListIP_f( void ) { }
void SV_RemoveIP_f( void ) { }
void SV_WriteIP_f( void ) { }

void SV_InitFilter( void )
{
	Cmd_AddCommand( "banid", SV_BanID_f, "ban player by ID" );
	Cmd_AddCommand( "listid", SV_ListID_f, "list banned players" );
	Cmd_AddCommand( "removeid", SV_RemoveID_f, "remove player from banned list" );
	Cmd_AddCommand( "writeid", SV_WriteID_f, "write banned.cfg" );
	/*Cmd_AddCommand( "addip", SV_AddIP_f, "add entry to IP filter" );
	Cmd_AddCommand( "listip", SV_ListIP_f, "list current IP filter" );
	Cmd_AddCommand( "removeip", SV_RemoveIP_f, "remove IP filter" );
	Cmd_AddCommand( "writeip", SV_WriteIP_f, "write listip.cfg" );*/
}

void SV_ShutdownFilter( void )
{
	//ipfilter_t *ipList, *ipNext;
	cidfilter_t *cidList, *cidNext;

	//SV_WriteIP_f();
	SV_WriteID_f();

	/*for( ipList = ipfilter; ipList; ipList = ipNext )
	{
		ipNext = ipList->next;
		Mem_Free( ipList );
	}*/

	for( cidList = cidfilter; cidList; cidList = cidNext )
	{
		cidNext = cidList->next;
		Mem_Free( cidList );
	}

	cidfilter = NULL;
}
