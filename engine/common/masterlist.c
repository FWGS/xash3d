
#include "common.h"



typedef struct master_s
{
	struct master_s *next;
	qboolean sent;
	string address;
} master_t;

struct masterlist_s
{
	master_t *list;
} ml;

/*
========================
NET_SendToMasters

Send request to all masterservers list
return true if would block
========================
*/
qboolean Net_SendToMasters( netsrc_t sock, size_t len, const void *data )
{
	master_t *list;
	qboolean wait = false;

	for( list = ml.list; list; list = list->next )
	{
		netadr_t adr;
		int res;

		if( list->sent )
			continue;

		res = NET_StringToAdrNB( list->address, &adr );

		if( !res )
		{
			MsgDev( D_INFO, "Can't resolve adr: %s\n", list->address );
			list->sent = true;
			continue;
		}

		if( res == 2 )
		{
			list->sent = false;
			wait = true;
			continue;
		}

		list->sent = true;

		NET_SendPacket( sock, len, data, adr );
	}

	if( !wait )
	{
		list = ml.list;

		while( list )
		{
			list->sent = false;
			list = list->next;
		}
	}

	return wait;
}

static void NET_AddMaster( char *addr )
{
	master_t *master = Mem_Alloc( host.mempool, sizeof( master_t ) );

	Q_strncpy( master->address, addr, MAX_STRING );
	master->sent = false;
	master->next = ml.list;
	ml.list = master;
}

static void NET_AddMaster_f( void )
{
	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: addmaster <address>\n");
		return;
	}

	NET_AddMaster( Cmd_Argv(1) );
}

static void NET_ClearMasters_f( void )
{
	while( ml.list )
	{
		master_t *prev = ml.list;
		ml.list = ml.list->next;
		Mem_Free( prev );
	}
}

static void NET_ListMasters_f( void )
{
	master_t *list = ml.list;
	int i = 0;

	Msg( "Master servers\n=============\n" );

	while( list )
	{
		Msg( "%d. %s\n", i, list->address );
		list = list->next;
		i++;
	}
}

void NET_InitMasters()
{
	Cmd_AddCommand( "addmaster", NET_AddMaster_f, "add address to masterserver list" );
	Cmd_AddCommand( "clearmasters", NET_ClearMasters_f, "clear masterserver list" );
	Cmd_AddCommand( "listmasters", NET_ListMasters_f, "list masterservers" );

	NET_AddMaster( "ms.xash.su:27010" );
	NET_AddMaster( "ms.csat.ml:27010" );
}
