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
	float time;
	float endTime;
	struct clientidban_s *prev;
	struct clientidban_s *next;
	char *id;
} cidfilter_t;

cidfilter_t *cidfilter = NULL, *cidfilterLast = NULL;

qboolean SV_CheckID( const char *id )
{
	qboolean ret = false;
	cidfilter_t *i;

	for( i = ipFilterLast; i; i = i->prev )
	{
		if( host.realtime > i->endTime )
			continue; // ban has expirted

		if( !Q_strcmp( id, i->id ) )
		{
			ret = true;
			break;
		}
	}

	return ret;
}

// qboolean SV_CheckIP( netadr_t * ) { }

void SV_BanID_f( void ) { }
void SV_ListID_f( void ) { }
void SV_RemoveID_f( void ) { }
void SV_WriteID_f( void ) { }
void SV_AddIP_f( void ) { }
void SV_ListIP_f( void ) { }
void SV_WriteIP_f( void ) { }

void SV_InitFilter( void )
{
	Cmd_AddCommand( "banid", SV_BanID_f, "ban player by ID" );
	Cmd_AddCommand( "listid", SV_ListID_f, "list banned players" );
	Cmd_AddCommand( "removeid", SV_RemoveID_f, "remove player from banned list" );
	Cmd_AddCommand( "writeid", SV_WriteID_f, "write banned.cfg" );
	Cmd_AddCommand( "addip", SV_AddIP_f, "add entry to IP filter" );
	Cmd_AddCommand( "listip", SV_ListIP_f, "list current IP filter" );
	Cmd_AddCommand( "removeip", SV_RemoveIP_f, "remove IP filter" );
	Cmd_AddCommand( "writeip", SV_WriteIP_f, "write listip.cfg" );
}

void SV_ShutdownFilter( void )
{
	ipfilter_t *ipList, *ipNext;
	cidfilter_t *cidList, *cidNext;

	SV_WriteIP_f();
	SV_WriteID_f();

	for( ipList = ipfilter; ipList; ipList = ipNext )
	{
		ipNext = ipList->next;
		Mem_Free( ipList );
	}

	for( cidList = cidfilter; cidList; cidList = cidNext )
	{
		cidNext = cidList->next;
		Mem_Free( cidList );
	}


}
