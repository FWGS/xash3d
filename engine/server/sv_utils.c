/*
sv_utils.c - server utils
Copyright (C) 2016

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

sv_client_t *SV_ClientBySlotNum( int num )
{
	if ( num < 0 || num > svgame.globals->maxClients )
	{
		MsgDev( D_WARN, "SV_ClientBySlot: Bad client slot number\n" );
		return NULL;
	}

	if ( !svs.clients[num].state )
	{
		MsgDev( D_WARN, "SV_ClientBySlotNum: Client in slot %s is not active\n", num );
		return NULL;
	}

	return &svs.clients[num];
}

sv_client_t *SV_ClientById( int id )
{
	sv_client_t *cl = svs.clients;
	int i           = 0;

	if ( id < 0 )
	{
		MsgDev( D_WARN, "SV_ClientById: id  < 0\n" );
		return NULL;
	}

	for ( ; i < svgame.globals->maxClients; i++, cl++ )
	{
		if ( !cl->state )
			continue;

		if ( cl->userid == id )
			return cl;
	}

	return NULL;
}

sv_client_t *SV_ClientByName( const char *name )
{
	sv_client_t *cl = svs.clients;
	int i           = 0;

	if ( !name || !*name )
	{
		MsgDev( D_WARN, "SV_ClientByName: name is NULL or empty\n" );
		return NULL;
	}

	for ( ; i < svgame.globals->maxClients; i++, cl++ )
	{
		if ( !cl->state )
			continue;

		if ( !Q_strcmp( cl->name, name ) )
			return cl;
	}

	return NULL;
}

qboolean SV_SetCurrentClient( sv_client_t *cl )
{
	if ( !cl || cl < svs.clients || cl > ( svs.clients + svgame.globals->maxClients ) )
	{
		MsgDev( D_WARN, "SV_SetCurrentClient: client is NULL or not valid\n" );
		return false;
	}

	svs.currentPlayer    = cl;
	svs.currentPlayerNum = cl - svs.clients;

	return true;
}
