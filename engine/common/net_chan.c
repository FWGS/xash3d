/*
net_chan.c - network channel
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
#include "netchan.h"
#include "mathlib.h"
#include "net_encode.h"

#define MAKE_FRAGID( id, count )	((( id & 0xffff ) << 16 ) | ( count & 0xffff ))
#define FRAG_GETID( fragid )		(( fragid >> 16 ) & 0xffff )
#define FRAG_GETCOUNT( fragid )	( fragid & 0xffff )

#define UDP_HEADER_SIZE		28

#define FLOW_AVG			( 2.0 / 3.0 )	// how fast to converge flow estimates
#define FLOW_INTERVAL		0.1		// don't compute more often than this    
#define MAX_RELIABLE_PAYLOAD		1200		// biggest packet that has frag and or reliable data
#define MAX_RESEND_PAYLOAD		1400		// biggest packet on a resend

// forward declarations
void Netchan_FlushIncoming( netchan_t *chan, int stream );
void Netchan_AddBufferToList( fragbuf_t **pplist, fragbuf_t *pbuf );

/*
packet header ( size in bits )
-------------
31	sequence
1	does this message contain a reliable payload
31	acknowledge sequence
1	acknowledge receipt of even/odd message
16	qport

The remote connection never knows if it missed a reliable message, the
local side detects that it has been dropped by seeing a sequence acknowledge
higher thatn the last reliable sequence, but without the correct evon/odd
bit for the reliable set.

If the sender notices that a reliable message has been dropped, it will be
retransmitted.  It will not be retransmitted again until a message after
the retransmit has been acknowledged and the reliable still failed to get there.

if the sequence number is -1, the packet should be handled without a netcon

The reliable message can be added to at any time by doing
BF_Write* (&netchan->message, <data>).

If the message buffer is overflowed, either by a single message, or by
multiple frames worth piling up while the last reliable transmit goes
unacknowledged, the netchan signals a fatal error.

Reliable messages are allways placed first in a packet, then the unreliable
message is included if there is sufficient room.

To the receiver, there is no distinction between the reliable and unreliable
parts of the message, they are just processed out as a single larger message.

Illogical packet sequence numbers cause the packet to be dropped, but do
not kill the connection.  This, combined with the tight window of valid
reliable acknowledgement numbers provides protection against malicious
address spoofing.

The qport field is a workaround for bad address translating routers that
sometimes remap the client's source port on a packet during gameplay.

If the base part of the net address matches and the qport matches, then the
channel matches even if the IP port differs.  The IP port should be updated
to the new value before sending out any replies.


If there is no information that needs to be transfered on a given frame,
such as during the connection stage while waiting for the client to load,
then a packet only needs to be delivered if there is something in the
unacknowledged reliable
*/
convar_t	*net_showpackets;
convar_t	*net_chokeloopback;
convar_t	*net_drawslider;
convar_t	*net_blocksize;
convar_t	*net_showdrop;
convar_t	*net_speeds;
convar_t	*net_qport;

int	net_drop;
netadr_t	net_from;
netadr_t	net_local;
sizebuf_t	net_message;
byte	*net_mempool;
byte	net_message_buffer[NET_MAX_PAYLOAD];

/*
=================================

NETWORK PACKET SPLIT

=================================
*/

/*
======================
NetSplit_GetLong

Collect fragmrnts with signature 0xFFFFFFFE to single packet
return true when got full packet
======================
*/
qboolean NetSplit_GetLong( netsplit_t *ns, netadr_t *from, byte *data, size_t *length, qboolean decompress )
{
	netsplit_packet_t *packet = (netsplit_packet_t*)data;
	netsplit_chain_packet_t * p;

	//ASSERT( *length > NETSPLIT_HEADER_SIZE );
	if( *length <= NETSPLIT_HEADER_SIZE ) return false;

	p = &ns->packets[packet->id & NETSPLIT_BACKUP_MASK];
	// MsgDev( D_NOTE, "NetSplit_GetLong: packet from %s, id %d, index %d length %d\n", NET_AdrToString( *from ), (int)packet->id, (int)packet->index, (int)*length );

	// no packets with this id received
	if( packet->id != p->id )
	{
		// warn if previous packet not received
		if( p->received < p->count )
			MsgDev( D_WARN, "NetSplit_GetLong: lost packet %d\n", p->id );

		p->id = packet->id;
		p->count = packet->count;
		p->received = 0;
		Q_memset( p->recieved_v, 0, 32 );
	}

	// use bool vector to detect dup packets
	if( p->recieved_v[packet->index >> 5 ] & ( 1 << ( packet->index & 31 ) ) )
	{
		MsgDev( D_WARN, "NetSplit_GetLong: dup packet from %s\n", NET_AdrToString( *from ) );
		return false;
	}

	p->received++;

	// mark as received
	p->recieved_v[packet->index >> 5] |= 1 << ( packet->index & 31 );

	// prevent overflow
	if( packet->part * packet->index > NET_MAX_PAYLOAD )
	{
		MsgDev( D_WARN, "NetSplit_GetLong: packet out fo bounds from %s (part %d index %d)\n", NET_AdrToString( *from ), packet->part, packet->index );
		return false;
	}

	if( packet->length > NET_MAX_PAYLOAD )
	{
		MsgDev( D_WARN, "NetSplit_GetLong: packet out fo bounds from %s (length %d)\n", NET_AdrToString( *from ), packet->length );
		return false;
	}

	Q_memcpy( p->data + packet->part * packet->index, packet->data, *length - 18 );

	// rewrite results of NET_GetPacket
	if( p->received == packet->count )
	{
		//ASSERT( packet->length % packet->part == (*length - NETSPLIT_HEADER_SIZE) % packet->part );
		size_t len = packet->length;

		ns->total_received += len;

		if( decompress )
			Huff_DecompressData( p->data, &len );

		ns->total_received_uncompressed += len;
		*length = len;

		// MsgDev( D_NOTE, "NetSplit_GetLong: packet from %s, id %d received %d length %d\n", NET_AdrToString( *from ), (int)packet->id, (int)p->received, (int)packet->length );
		Q_memcpy( data, p->data, len );
		return true;
	}
	else
		*length = NETSPLIT_HEADER_SIZE + packet->part;


	return false;
}

/*
======================
NetSplit_SendLong

Send parts that are less or equal maxpacket
======================
*/
void NetSplit_SendLong( netsrc_t sock, size_t length, void *data, netadr_t to, unsigned int maxpacket, unsigned int id, qboolean compress )
{
	netsplit_packet_t packet = {0};
	unsigned int part = maxpacket - NETSPLIT_HEADER_SIZE;

	if( compress )
		Huff_CompressData( data, &length );

	packet.signature = 0xFFFFFFFE;
	packet.id = id;
	packet.length = length;
	packet.part = part;
	packet.count = ( length - 1 ) / part + 1;

	//MsgDev( D_NOTE, "NetSplit_SendLong: packet to %s, count %d, length %d\n", NET_AdrToString( to ), (int)packet.count, (int)packet.length );

	while( packet.index < packet.count  )
	{
		unsigned int size = part;

		if( size > length )
			size = length;

		length -= size;

		Q_memcpy( packet.data, (const byte*)data + packet.index * part, size );
		//MsgDev( D_NOTE, "NetSplit_SendLong: packet to %s, id %d, index %d\n", NET_AdrToString( to ), (int)packet.id, (int)packet.index );
		NET_SendPacket( sock, size + NETSPLIT_HEADER_SIZE, &packet, to );
		packet.index++;
	}

}

/*
===============
Netchan_Init
===============
*/
void Netchan_Init( void )
{
	int	port;

	// pick a port value that should be nice and random
	port = Com_RandomLong( 1, 65535 );

	net_showpackets = Cvar_Get ("net_showpackets", "0", 0, "show network packets" );
	net_chokeloopback = Cvar_Get( "net_chokeloop", "0", 0, "apply bandwidth choke to loopback packets" );
	net_drawslider = Cvar_Get( "net_drawslider", "0", CVAR_ARCHIVE, "draw completion slider during signon" );
	net_blocksize = Cvar_Get( "net_blocksize", "1024", 0, "network file fragmentation block size" );
	net_showdrop = Cvar_Get( "net_showdrop", "0", 0, "show packets that are dropped" );
	net_speeds = Cvar_Get( "net_speeds", "0", CVAR_ARCHIVE, "show network packets" );
	net_qport = Cvar_Get( "net_qport", va( "%i", port ), CVAR_INIT, "current quake netport" );

	net_mempool = Mem_AllocPool( "Network Pool" );

	Huff_Init ();	// initialize huffman compression
	BF_InitMasks ();	// initialize bit-masks
}

void Netchan_Shutdown( void )
{
	Mem_FreePool( &net_mempool );
}

void Netchan_ReportFlow( netchan_t *chan )
{
	char	incoming[CS_SIZE];
	char	outgoing[CS_SIZE];

	ASSERT( chan != NULL );

	Q_strcpy( incoming, Q_pretifymem((float)chan->flow[FLOW_INCOMING].totalbytes, 3 ));
	Q_strcpy( outgoing, Q_pretifymem((float)chan->flow[FLOW_OUTGOING].totalbytes, 3 ));

	MsgDev( D_INFO, "Signon network traffic:  %s from server, %s to server\n", incoming, outgoing );
}

/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup( netsrc_t sock, netchan_t *chan, netadr_t adr, int qport )
{
	Netchan_Clear( chan );

	Q_memset( chan, 0, sizeof( *chan ));

	chan->sock = sock;
	chan->remote_address = adr;
	chan->last_received = host.realtime;
	chan->connect_time = host.realtime;
	chan->incoming_sequence = 0;
	chan->outgoing_sequence = 1;
	chan->rate = DEFAULT_RATE;
	chan->compress = false;	// work but low efficiency
	chan->qport = qport;

	BF_Init( &chan->message, "NetData", chan->message_buf, sizeof( chan->message_buf ));
}

/*
==============================
Netchan_IncomingReady

==============================
*/
qboolean Netchan_IncomingReady( netchan_t *chan )
{
	int	i;

	for( i = 0; i < MAX_STREAMS; i++ )
	{
		if( chan->incomingready[i] )
		{
			return true;
		}
	}
	return false;
}

/*
===============
Netchan_CanPacket

Returns true if the bandwidth choke isn't active
================
*/
qboolean Netchan_CanPacket( netchan_t *chan )
{
	// never choke loopback packets.
	if( !net_chokeloopback->integer && NET_IsLocalAddress( chan->remote_address ))
	{
		chan->cleartime = host.realtime;
		return true;
	}

	if( chan->cleartime < host.realtime )
	{
		return true;
	}
	return false;
}

/*
==============================
Netchan_UnlinkFragment

==============================
*/
void Netchan_UnlinkFragment( fragbuf_t *buf, fragbuf_t **list )
{
	fragbuf_t	*search;

	if( !list )
	{
		MsgDev( D_WARN, "Netchan_UnlinkFragment: Asked to unlink fragment from empty list, ignored\n" );
		return;
	}

	// at head of list
	if( buf == *list )
	{
		// remove first element
		*list = buf->next;

		// destroy remnant
		Mem_Free( buf );
		return;
	}

	search = *list;
	while( search->next )
	{
		if( search->next == buf )
		{
			search->next = buf->next;

			// destroy remnant
			Mem_Free( buf );
			return;
		}
		search = search->next;
	}

	MsgDev( D_ERROR, "Netchan_UnlinkFragment:  Couldn't find fragment\n" );
}
/*
==============================
Netchan_ClearFragbufs

==============================
*/
void Netchan_ClearFragbufs( fragbuf_t **ppbuf )
{
	fragbuf_t	*buf, *n;

	if( !ppbuf ) return;

	// Throw away any that are sitting around
	buf = *ppbuf;

	while( buf )
	{
		n = buf->next;
		Mem_Free( buf );
		buf = n;
	}

	*ppbuf = NULL;
}

/*
==============================
Netchan_ClearFragments

==============================
*/
void Netchan_ClearFragments( netchan_t *chan )
{
	fragbufwaiting_t	*wait;
	int		i;

	for( i = 0; i < MAX_STREAMS; i++ )
	{
		wait = chan->waitlist[i];

		while( wait )
		{
			Netchan_ClearFragbufs( &wait->fragbufs );
			wait = wait->next;
		}
		chan->waitlist[i] = NULL;

		Netchan_ClearFragbufs( &chan->fragbufs[i] );
		Netchan_FlushIncoming( chan, i );
	}
}

/*
==============================
Netchan_Clear

==============================
*/
void Netchan_Clear( netchan_t *chan )
{
	int	i;

	Netchan_ClearFragments( chan );

	chan->cleartime = 0.0;
	chan->reliable_length = 0;

	for( i = 0; i < MAX_STREAMS; i++ )
	{
		chan->reliable_fragid[i] = 0;
		chan->reliable_fragment[i] = 0;
		chan->fragbufcount[i] = 0;
		chan->frag_startpos[i] = 0;
		chan->frag_length[i] = 0;
		chan->incomingready[i] = false;
	}

	Q_memset( chan->flow, 0, sizeof( chan->flow ));
}

/*
===============
Netchan_OutOfBand

Sends an out-of-band datagram
================
*/
void Netchan_OutOfBand( int net_socket, netadr_t adr, int length, byte *data )
{
	sizebuf_t	send;
	byte	send_buf[NET_MAX_PAYLOAD];

	// write the packet header
	BF_Init( &send, "SequencePacket", send_buf, sizeof( send_buf ));

	BF_WriteLong( &send, -1 );	// -1 sequence means out of band
	BF_WriteBytes( &send, data, length );

	if( !CL_IsPlaybackDemo( ))
	{
		// send the datagram
		NET_SendPacket( net_socket, BF_GetNumBytesWritten( &send ), BF_GetData( &send ), adr );
	}
}

/*
===============
Netchan_OutOfBandPrint

Sends a text message in an out-of-band datagram
================
*/
void Netchan_OutOfBandPrint( int net_socket, netadr_t adr, char *format, ... )
{
	va_list	argptr;
	char	string[MAX_SYSPATH];

	va_start( argptr, format );
	Q_vsprintf( string, format, argptr );
	va_end( argptr );

	Netchan_OutOfBand( net_socket, adr, Q_strlen( string ), (byte *)string );
}

/*
==============================
Netchan_AllocFragbuf

==============================
*/
fragbuf_t *Netchan_AllocFragbuf( void )
{
	fragbuf_t	*buf;

	buf = (fragbuf_t *)Mem_Alloc( net_mempool, sizeof( fragbuf_t ));
	BF_Init( &buf->frag_message, "Frag Message", buf->frag_message_buf, sizeof( buf->frag_message_buf ));

	return buf;
}

/*
==============================
Netchan_AddFragbufToTail

==============================
*/
void Netchan_AddFragbufToTail( fragbufwaiting_t *wait, fragbuf_t *buf )
{
	fragbuf_t	*p;

	buf->next = NULL;
	wait->fragbufcount++;

	if( !wait->fragbufs )
	{
		wait->fragbufs = buf;
		return;
	}

	p = wait->fragbufs;
	while( p->next )
	{
		p = p->next;
	}

	p->next = buf;
}

/*
==============================
Netchan_UpdateFlow

==============================
*/
void Netchan_UpdateFlow( netchan_t *chan )
{
	int		flow, start;
	int		i, bytes = 0;
	float		faccumulatedtime = 0.0;
	flowstats_t	*pstat, *pprev;
	flow_t		*pflow;

	if( !chan ) return;

	for( flow = 0; flow < 2; flow++ )
	{
		pflow = &chan->flow[flow];

		if(( host.realtime - pflow->nextcompute ) < FLOW_INTERVAL )
			continue;

		pflow->nextcompute = host.realtime + FLOW_INTERVAL;

		start = pflow->current - 1;

		pprev = &pflow->stats[start & ( MAX_LATENT - 1 )];

		// compute data flow rate
		for( i = 1; i < MAX_LATENT / 2; i++ )
		{
			// Most recent message then backward from there
			pstat = &pflow->stats[( start - i ) & ( MAX_LATENT - 1 )];

			bytes += ( pstat->size );
			faccumulatedtime += ( pprev->time - pstat->time );
			pprev = pstat;
		}

		if( !faccumulatedtime )
		{
			pflow->kbytespersec = 0.0;
		}
		else
		{
			pflow->kbytespersec = (float)bytes / faccumulatedtime;
			pflow->kbytespersec /= 1024.0;
		}

		pflow->avgkbytespersec = ( FLOW_AVG ) * pflow->avgkbytespersec + ( 1.0 - FLOW_AVG ) * pflow->kbytespersec;
	}
}

/*
==============================
Netchan_FragSend

Fragmentation buffer is full and user is prepared to send
==============================
*/
void Netchan_FragSend( netchan_t *chan )
{
	fragbufwaiting_t	*wait;
	int		i;

	if( !chan ) return;

	for( i = 0; i < MAX_STREAMS; i++ )
	{
		// already something queued up, just leave in waitlist
		if( chan->fragbufs[i] )
		{
			continue;
		}

		// nothing to queue?
		if( !chan->waitlist[i] )
		{
			continue;
		}

		wait = chan->waitlist[i] ;
		chan->waitlist[i] = chan->waitlist[i]->next;

		wait->next = NULL;

		// copy in to fragbuf
		chan->fragbufs[i] = wait->fragbufs;
		chan->fragbufcount[i] = wait->fragbufcount;

		// throw away wait list
		Mem_Free( wait );
	}
}

/*
==============================
Netchan_AddBufferToList

==============================
*/
void Netchan_AddBufferToList( fragbuf_t **pplist, fragbuf_t *pbuf )
{
	// Find best slot
	fragbuf_t	*pprev, *n;
	int	id1, id2;

	pbuf->next = NULL;

	if( !pplist )
		return;

	if( !*pplist )
	{
		pbuf->next = *pplist;
		*pplist = pbuf;
		return;
	}

	pprev = *pplist;
	while( pprev->next )
	{
		n = pprev->next; // next item in list
		id1 = FRAG_GETID( n->bufferid );
		id2 = FRAG_GETID( pbuf->bufferid );

		if( id1 > id2 )
		{
			// insert here
			pbuf->next = n->next;
			pprev->next = pbuf;
			return;
		}
		pprev = pprev->next;
	}

	// insert at end
	pprev->next = pbuf;
}

/*
==============================
Netchan_CreateFragments_

==============================
*/
void Netchan_CreateFragments_( qboolean server, netchan_t *chan, sizebuf_t *msg )
{
	fragbuf_t		*buf;
	int		chunksize;
	int		send, pos;
	int		remaining;
	int		bufferid = 1;
	fragbufwaiting_t	*wait, *p;

	if( BF_GetNumBytesWritten( msg ) == 0 )
		return;

	chunksize = bound( 16, net_blocksize->integer, 1400 );

	wait = (fragbufwaiting_t *)Mem_Alloc( net_mempool, sizeof( fragbufwaiting_t ));

	remaining = BF_GetNumBytesWritten( msg );
	pos = 0;

	while( remaining > 0 )
	{
		send = min( remaining, chunksize );
		remaining -= send;

		buf = Netchan_AllocFragbuf();
		buf->bufferid = bufferid++;

		// Copy in data
		BF_Clear( &buf->frag_message );
		BF_WriteBits( &buf->frag_message, msg->pData + pos, send << 3 );
		pos += send;

		Netchan_AddFragbufToTail( wait, buf );
	}

	// now add waiting list item to end of buffer queue
	if( !chan->waitlist[FRAG_NORMAL_STREAM] )
	{
		chan->waitlist[FRAG_NORMAL_STREAM] = wait;
	}
	else
	{
		p = chan->waitlist[FRAG_NORMAL_STREAM];

		while( p->next )
		{
			p = p->next;
		}

		p->next = wait;
	}
}

/*
==============================
Netchan_CreateFragments

==============================
*/
void Netchan_CreateFragments( qboolean server, netchan_t *chan, sizebuf_t *msg )
{
	// always queue any pending reliable data ahead of the fragmentation buffer
	if( BF_GetNumBytesWritten( &chan->message ) > 0 )
	{
		Netchan_CreateFragments_( server, chan, &chan->message );
		BF_Clear( &chan->message );
	}

	Netchan_CreateFragments_( server, chan, msg );
}

/*
==============================
Netchan_FindBufferById

==============================
*/
fragbuf_t *Netchan_FindBufferById( fragbuf_t **pplist, int id, qboolean allocate )
{
	fragbuf_t	*list = *pplist;
	fragbuf_t	*pnewbuf;

	while( list )
	{
		if( list->bufferid == id )
			return list;

		list = list->next;
	}

	if( !allocate )
		return NULL;

	// create new entry
	pnewbuf = Netchan_AllocFragbuf();
	pnewbuf->bufferid = id;
	Netchan_AddBufferToList( pplist, pnewbuf );

	return pnewbuf;
}

/*
==============================
Netchan_CheckForCompletion

==============================
*/
void Netchan_CheckForCompletion( netchan_t *chan, int stream, int intotalbuffers )
{
	int	c, id;
	int	size;
	fragbuf_t	*p;

	size = 0;
	c = 0;

	p = chan->incomingbufs[stream];
	if( !p ) return;

	while( p )
	{
		size += BF_GetNumBytesWritten( &p->frag_message );
		c++;

		id = FRAG_GETID( p->bufferid );
		if( id != c )
		{
			if( chan->sock == NS_CLIENT )
			{
				MsgDev( D_ERROR, "Lost/dropped fragment would cause stall, retrying connection\n" );
				Cbuf_AddText( "reconnect\n" );
			}
		}
		p = p->next;
	}

	// received final message
	if( c == intotalbuffers )
	{
		chan->incomingready[stream] = true;
		MsgDev( D_NOTE, "\nincoming is complete, %i bytes waiting\n", size );
	}
}

/*
==============================
Netchan_CreateFileFragmentsFromBuffer

==============================
*/
void Netchan_CreateFileFragmentsFromBuffer( qboolean server, netchan_t *chan, char *filename, byte *pbuf, int size )
{
	int		chunksize;
	int		send, pos;
	int		remaining;
	int		bufferid = 1;
	qboolean		firstfragment = true;
	fragbufwaiting_t	*wait, *p;
	fragbuf_t 	*buf;

	if( !size ) return;

	chunksize = bound( 16, net_blocksize->integer, 512 );
	wait = ( fragbufwaiting_t * )Mem_Alloc( net_mempool, sizeof( fragbufwaiting_t ));
	remaining = size;
	pos = 0;

	while( remaining > 0 )
	{
		send = min( remaining, chunksize );

		buf = Netchan_AllocFragbuf();
		buf->bufferid = bufferid++;

		// copy in data
		BF_Clear( &buf->frag_message );

		if( firstfragment )
		{
			firstfragment = false;

			// write filename
			BF_WriteString( &buf->frag_message, filename );

			// send a bit less on first package
			send -= BF_GetNumBytesWritten( &buf->frag_message );
		}

		buf->isbuffer = true;
		buf->isfile = true;
		buf->size = send;
		buf->foffset = pos;

		BF_WriteBits( &buf->frag_message, pbuf + pos, send << 3 );

		pos += send;
		remaining -= send;

		Netchan_AddFragbufToTail( wait, buf );
	}

	// now add waiting list item to end of buffer queue
	if( !chan->waitlist[FRAG_FILE_STREAM] )
	{
		chan->waitlist[FRAG_FILE_STREAM] = wait;
	}
	else
	{
		p = chan->waitlist[FRAG_FILE_STREAM];

		while( p->next )
		{
			p = p->next;
		}
		p->next = wait;
	}
}

/*
==============================
Netchan_CreateFileFragments

==============================
*/
int Netchan_CreateFileFragments( qboolean server, netchan_t *chan, const char *filename )
{
	int		chunksize;
	int		send, pos;
	int		remaining;
	int		bufferid = 1;
	int		filesize = 0;
	qboolean		firstfragment = true;
	fragbufwaiting_t	*wait, *p;
	fragbuf_t		*buf;

	chunksize = bound( 16, net_blocksize->integer, 512 );
	filesize = FS_FileSize( filename, false );

	if( filesize <= 0 )
	{
		MsgDev( D_WARN, "Unable to open %s for transfer\n", filename );
		return 0;
	}

	wait = (fragbufwaiting_t *)Mem_Alloc( net_mempool, sizeof( fragbufwaiting_t ));
	remaining = filesize;
	pos = 0;

	while( remaining > 0 )
	{
		send = min( remaining, chunksize );

		buf = Netchan_AllocFragbuf();
		buf->bufferid = bufferid++;

		// copy in data
		BF_Clear( &buf->frag_message );

		if( firstfragment )
		{
			firstfragment = false;

			// Write filename
			BF_WriteString( &buf->frag_message, filename );

			// Send a bit less on first package
			send -= BF_GetNumBytesWritten( &buf->frag_message );
		}

		buf->isfile = true;
		buf->size = send;
		buf->foffset = pos;
		Q_strncpy( buf->filename, filename, sizeof( buf->filename ));

		pos += send;
		remaining -= send;

		Netchan_AddFragbufToTail( wait, buf );
	}

	// now add waiting list item to end of buffer queue
	if( !chan->waitlist[FRAG_FILE_STREAM] )
	{
		chan->waitlist[FRAG_FILE_STREAM] = wait;
	}
	else
	{
		p = chan->waitlist[FRAG_FILE_STREAM];
		while( p->next )
		{
			p = p->next;
		}

		p->next = wait;
	}

	return 1;
}

/*
==============================
Netchan_FlushIncoming

==============================
*/
void Netchan_FlushIncoming( netchan_t *chan, int stream )
{
	fragbuf_t *p, *n;

	BF_Clear( &net_message );

	p = chan->incomingbufs[ stream ];
	while( p )
	{
		n = p->next;
		Mem_Free( p );
		p = n;
	}
	chan->incomingbufs[stream] = NULL;
	chan->incomingready[stream] = false;
}

/*
==============================
Netchan_CopyNormalFragments

==============================
*/
qboolean Netchan_CopyNormalFragments( netchan_t *chan, sizebuf_t *msg )
{
	fragbuf_t	*p, *n;

	if( !chan->incomingready[FRAG_NORMAL_STREAM] )
		return false;

	if( !chan->incomingbufs[FRAG_NORMAL_STREAM] )
	{
		MsgDev( D_ERROR, "Netchan_CopyNormalFragments:  Called with no fragments readied\n" );
		chan->incomingready[FRAG_NORMAL_STREAM] = false;
		return false;
	}

	p = chan->incomingbufs[FRAG_NORMAL_STREAM];

	BF_Init( msg, "NetMessage", net_message_buffer, sizeof( net_message_buffer ));

	while( p )
	{
		n = p->next;

		// copy it in
		BF_WriteBits( msg, BF_GetData( &p->frag_message ), BF_GetNumBitsWritten( &p->frag_message ));

		Mem_Free( p );
		p = n;
	}

	chan->incomingbufs[FRAG_NORMAL_STREAM] = NULL;

	// reset flag
	chan->incomingready[FRAG_NORMAL_STREAM] = false;

	return true;
}

/*
==============================
Netchan_CopyFileFragments

==============================
*/
qboolean Netchan_CopyFileFragments( netchan_t *chan, sizebuf_t *msg )
{
	fragbuf_t	*p, *n;
	char	filename[CS_SIZE];
	int	nsize;
	byte	*buffer;
	int	pos;

	if( !chan->incomingready[FRAG_FILE_STREAM] )
		return false;

	if( !chan->incomingbufs[FRAG_FILE_STREAM] )
	{
		MsgDev( D_WARN, "Netchan_CopyFileFragments:  Called with no fragments readied\n" );
		chan->incomingready[FRAG_FILE_STREAM] = false;
		return false;
	}

	p = chan->incomingbufs[FRAG_FILE_STREAM];

	BF_Init( msg, "NetMessage", net_message_buffer, sizeof( net_message_buffer ));

	// copy in first chunk so we can get filename out
	BF_WriteBits( msg, BF_GetData( &p->frag_message ), BF_GetNumBitsWritten( &p->frag_message ));
	BF_SeekToBit( msg, 0 ); // rewind buffer

	//Q_strncpy( filename, BF_ReadString( msg ), sizeof( filename ));
	Q_snprintf( filename, sizeof( filename ), "downloaded/%s", BF_ReadString( msg ) );

	if( Q_strlen( filename ) <= 0 )
	{
		MsgDev( D_ERROR, "File fragment received with no filename\nFlushing input queue\n" );

		// clear out bufs
		Netchan_FlushIncoming( chan, FRAG_FILE_STREAM );
		return false;
	}
	else if( Q_strstr( filename, ".." ))
	{
		MsgDev( D_ERROR, "File fragment received with relative path, ignoring\n" );

		// clear out bufs
		Netchan_FlushIncoming( chan, FRAG_FILE_STREAM );
		return false;
	}

	Q_strncpy( chan->incomingfilename, filename, sizeof( chan->incomingfilename ));

	if( FS_FileExists( filename, false ))
	{
		MsgDev( D_ERROR, "Can't download %s, already exists\n", filename );

		// clear out bufs
		Netchan_FlushIncoming( chan, FRAG_FILE_STREAM );
		return true;
	}

	// create file from buffers
	nsize = 0;
	while ( p )
	{
		nsize += BF_GetNumBytesWritten( &p->frag_message ); // Size will include a bit of slop, oh well
		if( p == chan->incomingbufs[FRAG_FILE_STREAM] )
		{
			nsize -= BF_GetNumBytesRead( msg );
		}
		p = p->next;
	}

	buffer = Mem_Alloc( net_mempool, nsize + 1 );
	p = chan->incomingbufs[ FRAG_FILE_STREAM ];
	pos = 0;

	while( p )
	{
		int	cursize;

		n = p->next;

		cursize = BF_GetNumBytesWritten( &p->frag_message );

		// first message has the file name, don't write that into the data stream,
		// just write the rest of the actual data
		if( p == chan->incomingbufs[FRAG_FILE_STREAM] ) 
		{
			// copy it in
			cursize -= BF_GetNumBytesRead( msg );
			Q_memcpy( &buffer[pos], &p->frag_message.pData[BF_GetNumBytesRead( msg )], cursize );
		}
		else
		{
			Q_memcpy( &buffer[pos], p->frag_message.pData, cursize );
		}

		pos += cursize;

		Mem_Free( p );
		p = n;
	}

	FS_WriteFile( filename, buffer, pos );
	Mem_Free( buffer );

	// clear remnants
	BF_Clear( msg );

	chan->incomingbufs[FRAG_FILE_STREAM] = NULL;

	// reset flag
	chan->incomingready[FRAG_FILE_STREAM] = false;

	return true;
}

/*
==============================
Netchan_UpdateProgress

==============================
*/
void Netchan_UpdateProgress( netchan_t *chan )
{
	fragbuf_t *p;
	int	i, c = 0;
	int	total = 0;
	float	bestpercent = 0.0;

	if ( net_drawslider->integer != 1 )
	{
		// do show slider for file downloads.
		if( !chan->incomingbufs[FRAG_FILE_STREAM] )
		{
			return;
		}
	}

	for( i = MAX_STREAMS - 1; i >= 0; i-- )
	{
		// receiving data
		if( chan->incomingbufs[i] )
		{
			p = chan->incomingbufs[i];

			total = FRAG_GETCOUNT( p->bufferid );

			while( p )
			{
				c++;
				p = p->next;
			}

			if( total )
			{
				float	percent;

				percent = 100.0f * ( float )c / ( float )total;

				if( percent > bestpercent )
				{
					bestpercent = percent;
				}
			}

			p = chan->incomingbufs[i];

			if( i == FRAG_FILE_STREAM ) 
			{
				char	sz[MAX_SYSPATH];
				char	*in, *out;
				int	len = 0;

				in = (char *)BF_GetData( &p->frag_message );
				out = sz;

				while( *in )
				{
					*out++ = *in++;
					len++;
					if( len > 128 )
						break;
				}
				*out = '\0';
			}
		}
		else if( chan->fragbufs[i] )	// Sending data
		{
			if( chan->fragbufcount[i] )
			{
				float	percent;

				percent = 100.0f * (float)chan->fragbufs[i]->bufferid / (float)chan->fragbufcount[i];

				if( percent > bestpercent )
				{
					bestpercent = percent;
				}
			}
		}
	}

	if( bestpercent )
		Cvar_SetFloat( "scr_download", bestpercent );
}

/*
===============
Netchan_TransmitBits

tries to send an unreliable message to a connection, and handles the
transmition / retransmition of the reliable messages.

A 0 length will still generate a packet and deal with the reliable messages.
================
*/
void Netchan_TransmitBits( netchan_t *chan, int length, byte *data )
{
	sizebuf_t	send;
	byte	send_buf[NET_MAX_MESSAGE];
	qboolean	send_reliable_fragment;
	qboolean	send_resending = false;
	qboolean	send_reliable;
	size_t	size1, size2;
	uint	w1, w2, hdr_size;
	int	i, j;
	float	fRate;

	// check for message overflow
	// check for message overflow
	if( BF_CheckOverflow( &chan->message ))
	{
		MsgDev( D_ERROR, "%s: outgoing message overflow\n", NET_AdrToString( chan->remote_address ));
		return;
	}

	// if the remote side dropped the last reliable message, resend it
	send_reliable = false;

	if( chan->incoming_acknowledged > chan->last_reliable_sequence && chan->incoming_reliable_acknowledged != chan->reliable_sequence )
	{
		send_reliable = true;
		send_resending = true;
	}

	// A packet can have "reliable payload + frag payload + unreliable payload
	// frag payload can be a file chunk, if so, it needs to be parsed on the receiving end and reliable payload + unreliable payload need
	// to be passed on to the message queue.  The processing routine needs to be able to handle the case where a message comes in and a file
	// transfer completes

	// if the reliable transmit buffer is empty, copy the current message out
	if( !chan->reliable_length )
	{
		qboolean	send_frag = false;
		fragbuf_t	*pbuf;

		// will be true if we are active and should let chan->message get some bandwidth
		int	send_from_frag[MAX_STREAMS] = { 0, 0 };
		int	send_from_regular = false;

		// if we have data in the waiting list(s) and we have cleared the current queue(s), then 
		// push the waitlist(s) into the current queue(s)
		Netchan_FragSend( chan );

		// sending regular payload
		send_from_regular = BF_GetNumBytesWritten( &chan->message ) ? 1 : 0;

		// check to see if we are sending a frag payload
		for( i = 0; i < MAX_STREAMS; i++ )
		{
			if( chan->fragbufs[i] )
			{
				send_from_frag[i] = 1;
			}
		}

		// stall reliable payloads if sending from frag buffer
		if( send_from_regular && ( send_from_frag[FRAG_NORMAL_STREAM] ))
		{	
			send_from_regular = false;

			// if the reliable buffer has gotten too big, queue it at the end of everything and clear out buffer
			if( BF_GetNumBytesWritten( &chan->message ) > MAX_RELIABLE_PAYLOAD )
			{
				Netchan_CreateFragments(( chan->sock == NS_SERVER ), chan, &chan->message );
				BF_Clear( &chan->message );
			}
		}

		// startpos will be zero if there is no regular payload
		for( i = 0; i < MAX_STREAMS; i++ )
		{
			chan->frag_startpos[i] = 0;

			// assume no fragment is being sent
			chan->reliable_fragment[i] = 0;
			chan->reliable_fragid[i] = 0;
			chan->frag_length[i] = 0;

			if( send_from_frag[i] )
			{
				send_frag = true;
			}
		}

		if( send_from_regular || send_frag )
		{
			chan->reliable_sequence ^= 1;
			send_reliable = true;
		}

		if( send_from_regular )
		{
			Q_memcpy( chan->reliable_buf, chan->message_buf, BF_GetNumBytesWritten( &chan->message ));
			chan->reliable_length = BF_GetNumBitsWritten( &chan->message );
			BF_Clear( &chan->message );

			// if we send fragments, this is where they'll start
			for( i = 0; i < MAX_STREAMS; i++ )
			{
				chan->frag_startpos[i] = chan->reliable_length;
			}
		}

		for( i = 0; i < MAX_STREAMS; i++ )
		{
			int	fragment_size;
			int	newpayloadsize;

			// is there someting in the fragbuf?
			pbuf = chan->fragbufs[i];
			fragment_size = 0;

			if( pbuf )
			{
				fragment_size = BF_GetNumBytesWritten( &pbuf->frag_message );

				// files set size a bit differently.
				if( pbuf->isfile && !pbuf->isbuffer )
				{
					fragment_size = pbuf->size;
				}
			}

			newpayloadsize = (( chan->reliable_length + ( fragment_size << 3 )) + 7 ) >> 3;

			// make sure we have enought space left
			if( send_from_frag[i] && pbuf && ( newpayloadsize < MAX_RELIABLE_PAYLOAD ))
			{
				sizebuf_t	temp;

				// which buffer are we sending ?
				chan->reliable_fragid[i] = MAKE_FRAGID( pbuf->bufferid, chan->fragbufcount[i] );

				// if it's not in-memory, then we'll need to copy it in frame the file handle.
				if( pbuf->isfile && !pbuf->isbuffer )
				{
					byte	filebuffer[2048];
					file_t	*hfile;

					hfile = FS_Open( pbuf->filename, "rb", false );
					FS_Seek( hfile, pbuf->foffset, SEEK_SET );
					FS_Read( hfile, filebuffer, pbuf->size );

					BF_WriteBits( &pbuf->frag_message, filebuffer, pbuf->size << 3 );
					FS_Close( hfile );
				}

				// copy frag stuff on top of current buffer
				BF_StartWriting( &temp, chan->reliable_buf, sizeof( chan->reliable_buf ), chan->reliable_length, -1 );

				BF_WriteBits( &temp, BF_GetData( &pbuf->frag_message ), BF_GetNumBitsWritten( &pbuf->frag_message ));

				chan->reliable_length += BF_GetNumBitsWritten( &pbuf->frag_message );
				chan->frag_length[i] = BF_GetNumBitsWritten( &pbuf->frag_message );

				// unlink pbuf
				Netchan_UnlinkFragment( pbuf, &chan->fragbufs[i] );	

				chan->reliable_fragment[i] = 1;

				// offset the rest of the starting positions
				for( j = i + 1; j < MAX_STREAMS; j++ )
				{
					chan->frag_startpos[j] += chan->frag_length[i];
				}
			}
		}
	}

	Q_memset( send_buf, 0, NET_MAX_MESSAGE );
	BF_Init( &send, "NetSend", send_buf, sizeof( send_buf ));

	// prepare the packet header
	w1 = chan->outgoing_sequence | (send_reliable << 31);
	w2 = chan->incoming_sequence | ((uint)chan->incoming_reliable_sequence << 31);

	send_reliable_fragment = false;

	for( i = 0; i < MAX_STREAMS; i++ )
	{
		if( chan->reliable_fragment[i] )
		{
			send_reliable_fragment = true;
			break;
		}
	}

	if( send_reliable && send_reliable_fragment )
	{
		w1 |= ( 1U << 30 );
	}

	chan->outgoing_sequence++;
	chan->last_sent = host.realtime;

	BF_WriteLong( &send, w1 );
	BF_WriteLong( &send, w2 );

	// send the qport if we are a client
	if( chan->sock == NS_CLIENT )
	{
		BF_WriteWord( &send, net_qport->integer);
	}	

	if( send_reliable && send_reliable_fragment )
	{
		for( i = 0; i < MAX_STREAMS; i++ )
		{
			if( chan->reliable_fragment[i] )
			{
				BF_WriteByte( &send, 1 );
				BF_WriteLong( &send, chan->reliable_fragid[i] );
				BF_WriteLong( &send, chan->frag_startpos[i] );
				BF_WriteLong( &send, chan->frag_length[i] );
			}
			else 
			{
				BF_WriteByte( &send, 0 );
			}
		}
	}

	hdr_size = BF_GetNumBytesWritten( &send );

	// copy the reliable message to the packet first
	if( send_reliable )
	{
		BF_WriteBits( &send, chan->reliable_buf, chan->reliable_length );
		chan->last_reliable_sequence = chan->outgoing_sequence - 1;
	}

	// is there room for the unreliable payload?
	if( BF_GetNumBitsLeft( &send ) >= length )
	{
		BF_WriteBits( &send, data, length );
	}
	else
	{
		MsgDev( D_WARN, "Netchan_Transmit: unreliable message overflow\n" );
	}

	// deal with packets that are too small for some networks
	if( BF_GetNumBytesWritten( &send ) < 16 ) // packet too small for some networks
	{
		// go ahead and pad a full 16 extra bytes -- this only happens during authentication / signon
		for( i = BF_GetNumBytesWritten( &send ); i < 16; i++ )		
		{
			// NOTE: that the server can parse svc_nop, too.
			BF_WriteByte( &send, 1 );
		}
	}

	chan->flow[FLOW_OUTGOING].stats[chan->flow[FLOW_OUTGOING].current & ( MAX_LATENT-1 )].size = BF_GetNumBytesWritten( &send ) + UDP_HEADER_SIZE;
	chan->flow[FLOW_OUTGOING].stats[chan->flow[FLOW_OUTGOING].current & ( MAX_LATENT-1 )].time = host.realtime;
	chan->flow[FLOW_OUTGOING].totalbytes += ( BF_GetNumBytesWritten( &send ) + UDP_HEADER_SIZE );
	chan->flow[FLOW_OUTGOING].current++;

	Netchan_UpdateFlow( chan );

	size1 = BF_GetNumBytesWritten( &send );
	if( chan->compress ) Huff_CompressPacket( &send, hdr_size );
	size2 = BF_GetNumBytesWritten( &send );

	chan->total_sended += size2;
	chan->total_sended_uncompressed += size1;

	// send the datagram
	if( !CL_IsPlaybackDemo( ))
	{
		unsigned int size = BF_GetNumBytesWritten( &send );

		if( chan->split && size > chan->maxpacket )
			NetSplit_SendLong( chan->sock, size, BF_GetData( &send ), chan->remote_address, chan->maxpacket, chan->splitid++, chan->splitcompress );
		else
			NET_SendPacket( chan->sock, size, BF_GetData( &send ), chan->remote_address );
	}

	if( chan->rate == 0)
		chan->rate = DEFAULT_RATE;

	fRate = 1.0f / chan->rate;

	if( chan->cleartime < host.realtime )
	{
		chan->cleartime = host.realtime;
	}

	chan->cleartime += ( BF_GetNumBytesWritten( &send ) + UDP_HEADER_SIZE ) * fRate;

	if( net_showpackets->integer == 1 )
	{
		char	c;
		int	mask = 63;

		c = ( chan->sock == NS_CLIENT ) ? 'c' : 's';

		Msg( " %c --> sz=%i seq=%i ack=%i rel=%i tm=%f\n"
			, c
			, BF_GetNumBytesWritten( &send )
			, ( chan->outgoing_sequence - 1 ) & mask
			, chan->incoming_sequence & mask
			, send_reliable ? 1 : 0
			, (float)Sys_DoubleTime( ));
	}
}

/*
===============
Netchan_Transmit

tries to send an unreliable message to a connection, and handles the
transmition / retransmition of the reliable messages.

A 0 length will still generate a packet and deal with the reliable messages.
================
*/
void Netchan_Transmit( netchan_t *chan, int lengthInBytes, byte *data )
{
	Netchan_TransmitBits( chan, lengthInBytes << 3, data );
}

/*
=================
Netchan_Process

called when the current net_message is from remote_address
modifies net_message so that it points to the packet payload
=================
*/
qboolean Netchan_Process( netchan_t *chan, sizebuf_t *msg )
{
	uint	sequence, sequence_ack, hdr_size;
	uint	reliable_ack, reliable_message;
	uint	fragid[MAX_STREAMS] = { 0, 0 };
	qboolean	frag_message[MAX_STREAMS] = { false, false };
	int	frag_offset[MAX_STREAMS] = { 0, 0 };
	int	frag_length[MAX_STREAMS] = { 0, 0 };
	qboolean	message_contains_fragments;
	size_t	size1, size2;
	int	i, qport;

	if( !CL_IsPlaybackDemo() && !NET_CompareAdr( net_from, chan->remote_address ))
	{
		return false;
	}

	// get sequence numbers
	BF_Clear( msg );
	sequence = BF_ReadLong( msg );
	sequence_ack = BF_ReadLong( msg );

	// read the qport if we are a server
	if( chan->sock == NS_SERVER )
	{
		qport = BF_ReadShort( msg );
	}

	reliable_message = sequence >> 31;
	reliable_ack = sequence_ack >> 31;

	message_contains_fragments = sequence & ( 1U << 30 ) ? true : false;

	if( message_contains_fragments )
	{
		for( i = 0; i < MAX_STREAMS; i++ )
		{
			if( BF_ReadByte( msg ))
			{
				frag_message[i] = true;
				fragid[i] = BF_ReadLong( msg );
				frag_offset[i] = (int)BF_ReadLong( msg );
				frag_length[i] = (int)BF_ReadLong( msg );
			}
		}
	}

	sequence &= ~(1U<<31);	
	sequence &= ~(1U<<30);
	sequence_ack &= ~(1U<<31);	

	if( net_showpackets->integer == 2 )
	{
		char	c;

		c = ( chan->sock == NS_CLIENT ) ? 'c' : 's';

		Msg( " %c <-- sz=%i seq=%i ack=%i rel=%i tm=%f\n"
			, c
			, BF_GetMaxBytes( msg )
			, sequence & 63
			, sequence_ack & 63 
			, reliable_message
			, (float)Sys_DoubleTime( ));
	}

	// discard stale or duplicated packets
	if( sequence <= (uint)chan->incoming_sequence )
	{
		if( net_showdrop->integer )
		{
			if( sequence == (uint)chan->incoming_sequence )
			{
				Msg( "%s:duplicate packet %i at %i\n"
					, NET_AdrToString( chan->remote_address )
					,  sequence
					, chan->incoming_sequence );
			}
			else
			{
				Msg( "%s:out of order packet %i at %i\n"
					, NET_AdrToString( chan->remote_address )
					,  sequence
					, chan->incoming_sequence );
			}
		}
		return false;
	}

	// dropped packets don't keep the message from being used
	net_drop = sequence - ( chan->incoming_sequence + 1 );
	if( net_drop > 0 )
	{
		chan->drop_count += 1;

		if( net_showdrop->integer )
		{
			Msg( "%s:Dropped %i packets at %i\n"
			, NET_AdrToString( chan->remote_address )
			, sequence - (chan->incoming_sequence + 1)
			, sequence );
		}
	}

	// if the current outgoing reliable message has been acknowledged
	// clear the buffer to make way for the next
	if( reliable_ack == (uint)chan->reliable_sequence )
	{
		// make sure we actually could have ack'd this message
		if( chan->incoming_acknowledged + 1 >= chan->last_reliable_sequence )
		{
			chan->reliable_length = 0;	// it has been received
		}
	}

	// if this message contains a reliable message, bump incoming_reliable_sequence 
	chan->incoming_sequence = sequence;
	chan->incoming_acknowledged = sequence_ack;
	chan->incoming_reliable_acknowledged = reliable_ack;
	if( reliable_message )
	{
		chan->incoming_reliable_sequence ^= 1;
	}

	chan->last_received = host.realtime;

	// Update data flow stats
	chan->flow[FLOW_INCOMING].stats[chan->flow[FLOW_INCOMING].current & ( MAX_LATENT-1 )].size = BF_GetMaxBytes( msg ) + UDP_HEADER_SIZE;
	chan->flow[FLOW_INCOMING].stats[chan->flow[FLOW_INCOMING].current & ( MAX_LATENT-1 )].time = host.realtime;
	chan->flow[FLOW_INCOMING].totalbytes += ( BF_GetMaxBytes( msg ) + UDP_HEADER_SIZE );
	chan->flow[FLOW_INCOMING].current++;

	Netchan_UpdateFlow( chan );
	hdr_size = BF_GetNumBytesRead( msg );

	size1 = BF_GetMaxBytes( msg );
	if( chan->compress ) Huff_DecompressPacket( msg, hdr_size );
	size2 = BF_GetMaxBytes( msg );

	chan->total_received += size1;
	chan->total_received_uncompressed += size2;
	chan->good_count += 1;

	if( message_contains_fragments )
	{
		for( i = 0; i < MAX_STREAMS; i++ )
		{
			fragbuf_t	*pbuf;
			int	j, inbufferid;
			int	intotalbuffers;
			int	oldpos, curbit;
			int	numbitstoremove;

			if( !frag_message[i] )
				continue;

			inbufferid = FRAG_GETID( fragid[i] );
			intotalbuffers = FRAG_GETCOUNT( fragid[i] );

			if( fragid[i] != 0 )
			{
				pbuf = Netchan_FindBufferById( &chan->incomingbufs[i], fragid[i], true );

				if( pbuf )
				{
					int	bits;
					sizebuf_t	temp;
					byte	buffer[2048];

					bits = frag_length[i];

					// copy in data
					BF_Clear( &pbuf->frag_message );

					BF_StartReading( &temp, msg->pData, BF_GetMaxBytes( msg ), BF_GetNumBitsRead( msg ) + frag_offset[i], -1 );
					BF_ReadBits( msg, buffer, bits );
					BF_WriteBits( &pbuf->frag_message, buffer, bits );
				}
				else
				{
					MsgDev( D_ERROR, "Netchan_Process: Couldn't allocate or find buffer %i\n", inbufferid );
				}

				// count # of incoming bufs we've queued? are we done?
				Netchan_CheckForCompletion( chan, i, intotalbuffers );
			}

			// rearrange incoming data to not have the frag stuff in the middle of it
			oldpos = BF_GetNumBitsRead( msg );
			curbit = BF_GetNumBitsRead( msg ) + frag_offset[i];
			numbitstoremove = frag_length[i];

			BF_ExciseBits( msg, curbit, numbitstoremove );
			BF_SeekToBit( msg, oldpos );

			for( j = i + 1; j < MAX_STREAMS; j++ )
			{
				frag_offset[j] -= frag_length[i];
			}
		}

		// is there anything left to process?
		if( BF_GetNumBitsLeft( msg ) <= 0 )
		{
			return false;
		}
	}
	return true;
}
