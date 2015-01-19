/*
network.c - network interface
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

#include <winsock.h>
#include <wsipx.h>
#include "common.h"
#include "netchan.h"

#define PORT_ANY		-1
#define MAX_LOOPBACK	4
#define MASK_LOOPBACK	(MAX_LOOPBACK - 1)

// wsock32.dll exports
static int (_stdcall *pWSACleanup)( void );
static word (_stdcall *pNtohs)( word netshort );
static int (_stdcall *pWSAGetLastError)( void );
static int (_stdcall *pCloseSocket)( SOCKET s );
static word (_stdcall *pHtons)( word hostshort );
static dword (_stdcall *pInet_Addr)( const char* cp );
static char* (_stdcall *pInet_Ntoa)( struct in_addr in );
static SOCKET (_stdcall *pSocket)( int af, int type, int protocol );
static struct hostent *(_stdcall *pGetHostByName)( const char* name );
static int (_stdcall *pIoctlSocket)( SOCKET s, long cmd, dword* argp );
static int (_stdcall *pWSAStartup)( word wVersionRequired, LPWSADATA lpWSAData );
static int (_stdcall *pBind)( SOCKET s, const struct sockaddr* addr, int namelen );
static int (_stdcall *pSetSockopt)( SOCKET s, int level, int optname, const char* optval, int optlen );
static int (_stdcall *pRecvFrom)( SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen );
static int (_stdcall *pSendTo)( SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen );
static int (_stdcall *pSelect)( int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timeval* timeout );
static int (_stdcall *pConnect)( SOCKET s, const struct sockaddr *name, int namelen );
static int (_stdcall *pGetSockName)( SOCKET s, struct sockaddr *name, int *namelen );
static int (_stdcall *pSend)( SOCKET s, const char *buf, int len, int flags );
static int (_stdcall *pRecv)( SOCKET s, char *buf, int len, int flags );
static int (_stdcall *pGetHostName)( char *name, int namelen );
static dword (_stdcall *pNtohl)( dword netlong );

static dllfunc_t winsock_funcs[] =
{
{ "bind", (void **) &pBind },
{ "send", (void **) &pSend },
{ "recv", (void **) &pRecv },
{ "ntohs", (void **) &pNtohs },
{ "htons", (void **) &pHtons },
{ "ntohl", (void **) &pNtohl },
{ "socket", (void **) &pSocket },
{ "select", (void **) &pSelect },
{ "sendto", (void **) &pSendTo },
{ "connect", (void **) &pConnect },
{ "recvfrom", (void **) &pRecvFrom },
{ "inet_addr", (void **) &pInet_Addr },
{ "inet_ntoa", (void **) &pInet_Ntoa },
{ "WSAStartup", (void **) &pWSAStartup },
{ "WSACleanup", (void **) &pWSACleanup },
{ "setsockopt", (void **) &pSetSockopt },
{ "ioctlsocket", (void **) &pIoctlSocket },
{ "closesocket", (void **) &pCloseSocket },
{ "gethostname", (void **) &pGetHostName },
{ "getsockname", (void **) &pGetSockName },
{ "gethostbyname", (void **) &pGetHostByName },
{ "WSAGetLastError", (void **) &pWSAGetLastError },
{ NULL, NULL }
};

dll_info_t winsock_dll = { "wsock32.dll", winsock_funcs, false };

typedef struct
{
	byte	data[NET_MAX_PAYLOAD];
	int	datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t	msgs[MAX_LOOPBACK];
	int	get, send;
} loopback_t;

static loopback_t	loopbacks[2];
static int	ip_sockets[2];
static int	ipx_sockets[2];

static WSADATA winsockdata;
static qboolean winsockInitialized = false;
static const char *net_src[2] = { "client", "server" };
static qboolean noip = false;
static qboolean noipx = false;
static convar_t *net_ip;
static convar_t *net_hostport;
static convar_t *net_clientport;
static convar_t *net_showpackets;
void NET_Restart_f( void );

qboolean NET_OpenWinSock( void )
{
	// initialize the Winsock function vectors (we do this instead of statically linking
	// so we can run on Win 3.1, where there isn't necessarily Winsock)
	if( Sys_LoadLibrary( &winsock_dll ))
		return true;
	return false;
}

void NET_FreeWinSock( void )
{
	Sys_FreeLibrary( &winsock_dll );
}

/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString( void )
{
	switch( pWSAGetLastError( ))
	{
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
}

static void NET_NetadrToSockadr( netadr_t *a, struct sockaddr *s )
{
	Q_memset( s, 0, sizeof( *s ));

	if( a->type == NA_BROADCAST )
	{
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_port = a->port;
		((struct sockaddr_in *)s)->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if( a->type == NA_IP )
	{
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = *(int *)&a->ip;
		((struct sockaddr_in *)s)->sin_port = a->port;
	}
	else if( a->type == NA_IPX )
	{
		((struct sockaddr_ipx *)s)->sa_family = AF_IPX;
		Q_memcpy(((struct sockaddr_ipx *)s)->sa_netnum, &a->ipx[0], 4 );
		Q_memcpy(((struct sockaddr_ipx *)s)->sa_nodenum, &a->ipx[4], 6 );
		((struct sockaddr_ipx *)s)->sa_socket = a->port;
	}
	else if( a->type == NA_BROADCAST_IPX )
	{
		((struct sockaddr_ipx *)s)->sa_family = AF_IPX;
		Q_memset(((struct sockaddr_ipx *)s)->sa_netnum, 0, 4 );
		Q_memset(((struct sockaddr_ipx *)s)->sa_nodenum, 0xff, 6 );
		((struct sockaddr_ipx *)s)->sa_socket = a->port;
	}
}


static void NET_SockadrToNetadr( struct sockaddr *s, netadr_t *a )
{
	if( s->sa_family == AF_INET )
	{
		a->type = NA_IP;
		*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		a->port = ((struct sockaddr_in *)s)->sin_port;
	}
	else if( s->sa_family == AF_IPX )
	{
		a->type = NA_IPX;
		Q_memcpy( &a->ipx[0], ((struct sockaddr_ipx *)s)->sa_netnum, 4 );
		Q_memcpy( &a->ipx[4], ((struct sockaddr_ipx *)s)->sa_nodenum, 6 );
		a->port = ((struct sockaddr_ipx *)s)->sa_socket;
	}
}

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
#define DO(src,dest)		\
	copy[0] = s[src];		\
	copy[1] = s[src + 1];	\
	sscanf( copy, "%x", &val );	\
	((struct sockaddr_ipx *)sadr)->dest = val

static qboolean NET_StringToSockaddr( const char *s, struct sockaddr *sadr )
{
	struct hostent	*h;
	char		*colon;
	char		copy[MAX_SYSPATH];
	int		val;
	
	Q_memset( sadr, 0, sizeof( *sadr ));

	if((Q_strlen( s ) >= 23 ) && ( s[8] == ':' ) && ( s[21] == ':' )) // check for an IPX address
	{
		((struct sockaddr_ipx *)sadr)->sa_family = AF_IPX;
		copy[2] = 0;
		DO( 0, sa_netnum[0] );
		DO( 2, sa_netnum[1] );
		DO( 4, sa_netnum[2] );
		DO( 6, sa_netnum[3] );
		DO( 9, sa_nodenum[0] );
		DO( 11, sa_nodenum[1] );
		DO( 13, sa_nodenum[2] );
		DO( 15, sa_nodenum[3] );
		DO( 17, sa_nodenum[4] );
		DO( 19, sa_nodenum[5] );
		sscanf( &s[22], "%u", &val );
		((struct sockaddr_ipx *)sadr)->sa_socket = pHtons((word)val);
	}
	else
	{
		((struct sockaddr_in *)sadr)->sin_family = AF_INET;
		((struct sockaddr_in *)sadr)->sin_port = 0;

		Q_strncpy( copy, s, sizeof( copy ));

		// strip off a trailing :port if present
		for( colon = copy; *colon; colon++ )
		{
			if( *colon == ':' )
			{
				*colon = 0;
				((struct sockaddr_in *)sadr)->sin_port = pHtons((short)Q_atoi( colon + 1 ));	
			}
		}
		
		if( copy[0] >= '0' && copy[0] <= '9' )
		{
			*(int *)&((struct sockaddr_in *)sadr)->sin_addr = pInet_Addr( copy );
		}
		else
		{
			if(!( h = pGetHostByName( copy )))
				return false;
			*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
		}
	}
	return true;
}
#undef DO

char *NET_AdrToString( const netadr_t a )
{
	if( a.type == NA_LOOPBACK )
		return "loopback";
	else if( a.type == NA_IP )
		return va( "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], pNtohs( a.port ));
	return va( "%02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x:%i", a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9], pNtohs( a.port ));
}

char *NET_BaseAdrToString( const netadr_t a )
{
	if( a.type == NA_LOOPBACK )
		return "loopback";
	else if( a.type == NA_IP )
		return va( "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3] );
	return va( "%02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x", a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9] );
}

/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
qboolean NET_CompareBaseAdr( const netadr_t a, const netadr_t b )
{
	if( a.type != b.type )
		return false;

	if( a.type == NA_LOOPBACK )
		return true;

	if( a.type == NA_IP )
	{
		if( !Q_memcmp( a.ip, b.ip, 4 ))
			return true;
		return false;
	}

	if( a.type == NA_IPX )
	{
		if( !Q_memcmp( a.ipx, b.ipx, 10 ))
			return true;
		return false;
	}

	MsgDev( D_ERROR, "NET_CompareBaseAdr: bad address type\n" );
	return false;
}

qboolean NET_CompareAdr( const netadr_t a, const netadr_t b )
{
	if( a.type != b.type )
		return false;

	if( a.type == NA_LOOPBACK )
		return true;

	if( a.type == NA_IP )
	{
		if(!Q_memcmp( a.ip, b.ip, 4 ) && a.port == b.port )
			return true;
		return false;
	}

	if( a.type == NA_IPX )
	{
		if(!Q_memcmp( a.ipx, b.ipx, 10 ) && a.port == b.port )
			return true;
		return false;
	}

	MsgDev( D_ERROR, "NET_CompareAdr: bad address type\n" );
	return false;
}

qboolean NET_IsLocalAddress( netadr_t adr )
{
	return adr.type == NA_LOOPBACK;
}

/*
=============
NET_StringToAdr

idnewt
192.246.40.70
=============
*/
qboolean NET_StringToAdr( const char *string, netadr_t *adr )
{
	struct sockaddr s;

	if( !Q_stricmp( string, "localhost" ))
	{
		Q_memset( adr, 0, sizeof( netadr_t ));
		adr->type = NA_LOOPBACK;
		return true;
	}

	if( !NET_StringToSockaddr( string, &s ))
		return false;
	NET_SockadrToNetadr( &s, adr );

	return true;
}

/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/
static qboolean NET_GetLoopPacket( netsrc_t sock, netadr_t *from, byte *data, size_t *length )
{
	loopback_t	*loop;
	int		i;

	if( !data || !length )
		return false;

	loop = &loopbacks[sock];

	if( loop->send - loop->get > MAX_LOOPBACK )
		loop->get = loop->send - MAX_LOOPBACK;

	if( loop->get >= loop->send )
		return false;
	i = loop->get & MASK_LOOPBACK;
	loop->get++;

	Q_memcpy( data, loop->msgs[i].data, loop->msgs[i].datalen );
	*length = loop->msgs[i].datalen;

	Q_memset( from, 0, sizeof( *from ));
	from->type = NA_LOOPBACK;

	return true;
}

static void NET_SendLoopPacket( netsrc_t sock, size_t length, const void *data, netadr_t to )
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock^1];

	i = loop->send & MASK_LOOPBACK;
	loop->send++;

	Q_memcpy( loop->msgs[i].data, data, length );
	loop->msgs[i].datalen = length;
}

static void NET_ClearLoopback( void )
{
	loopbacks[0].send = loopbacks[0].get = 0;
	loopbacks[1].send = loopbacks[1].get = 0;
}

/*
==================
NET_GetPacket

Never called by the game logic, just the system event queing
==================
*/
qboolean NET_GetPacket( netsrc_t sock, netadr_t *from, byte *data, size_t *length )
{
	uint 		ret;
	struct sockaddr	addr;
	int		err, addr_len;
	int		net_socket;
	int		protocol;

	if( !data || !length )
		return false;

	if( NET_GetLoopPacket( sock, from, data, length ))
		return true;

	for( protocol = 0; protocol < 2; protocol++ )
	{
		if( !protocol) net_socket = ip_sockets[sock];
		else net_socket = ipx_sockets[sock];

		if( !net_socket ) continue;

		addr_len = sizeof( addr );
		ret = pRecvFrom( net_socket, data, NET_MAX_PAYLOAD, 0, (struct sockaddr *)&addr, &addr_len );

		NET_SockadrToNetadr( &addr, from );

		if( ret == SOCKET_ERROR )
		{
			err = pWSAGetLastError();

			// WSAEWOULDBLOCK and WSAECONNRESET are silent
			if( err == WSAEWOULDBLOCK || err == WSAECONNRESET )
				return false;

			MsgDev( D_ERROR, "NET_GetPacket: %s from %s\n", NET_ErrorString(), NET_AdrToString( *from ));
			continue;
		}

		if( ret == NET_MAX_PAYLOAD )
		{
			MsgDev( D_ERROR, "NET_GetPacket: oversize packet from %s\n", NET_AdrToString( *from ));
			continue;
		}

		*length = ret;
		return true;
	}

	return false;
}

/*
==================
NET_SendPacket
==================
*/
void NET_SendPacket( netsrc_t sock, size_t length, const void *data, netadr_t to )
{
	int		ret, err;
	struct sockaddr	addr;
	SOCKET		net_socket;

	// sequenced packets are shown in netchan, so just show oob
	if( net_showpackets->integer && *(int *)data == -1 )
		MsgDev( D_INFO, "send packet %4i\n", length );

	if( to.type == NA_LOOPBACK )
	{
		NET_SendLoopPacket( sock, length, data, to );
		return;
	}
	else if( to.type == NA_BROADCAST )
	{
		net_socket = ip_sockets[sock];
		if( !net_socket ) return;
	}
	else if( to.type == NA_IP )
	{
		net_socket = ip_sockets[sock];
		if( !net_socket ) return;
	}
	else if( to.type == NA_IPX )
	{
		net_socket = ipx_sockets[sock];
		if( !net_socket ) return;
	}
	else if( to.type == NA_BROADCAST_IPX )
	{
		net_socket = ipx_sockets[sock];
		if( !net_socket ) return;
	}
	else Host_Error( "NET_SendPacket: bad address type %i\n", to.type );

	NET_NetadrToSockadr( &to, &addr );

	ret = pSendTo( net_socket, data, length, 0, &addr, sizeof( addr ));

	if( ret == SOCKET_ERROR )
	{
		err = pWSAGetLastError();

		// WSAEWOULDBLOCK is silent
		if( err == WSAEWOULDBLOCK )
			return;

		// some PPP links don't allow broadcasts
		if(( err == WSAEADDRNOTAVAIL ) && (( to.type == NA_BROADCAST ) || ( to.type == NA_BROADCAST_IPX )))
			return;

		MsgDev( D_ERROR, "NET_SendPacket: %s to %s\n", NET_ErrorString(), NET_AdrToString( to ));
	}
}

/*
====================
NET_IPSocket
====================
*/
static int NET_IPSocket( const char *netInterface, int port )
{
	int		err, net_socket;
	struct sockaddr_in	addr;
	dword		_true = 1;

	MsgDev( D_NOTE, "NET_UDPSocket( %s, %i )\n", netInterface, port );
	
	if(( net_socket = pSocket( PF_INET, SOCK_DGRAM, IPPROTO_UDP )) == SOCKET_ERROR )
	{
		err = pWSAGetLastError();
		if( err != WSAEAFNOSUPPORT )
			MsgDev( D_WARN, "NET_UDPSocket: socket = %s\n", NET_ErrorString( ));
		return 0;
	}

	if( pIoctlSocket( net_socket, FIONBIO, &_true ) == SOCKET_ERROR )
	{
		MsgDev( D_WARN, "NET_UDPSocket: ioctlsocket FIONBIO = %s\n", NET_ErrorString( ));
		pCloseSocket( net_socket );
		return 0;
	}

	// make it broadcast capable
	if( pSetSockopt( net_socket, SOL_SOCKET, SO_BROADCAST, (char *)&_true, sizeof( _true )) == SOCKET_ERROR )
	{
		MsgDev( D_WARN, "NET_UDPSocket: setsockopt SO_BROADCAST = %s\n", NET_ErrorString( ));
		pCloseSocket( net_socket );
		return 0;
	}

	if( !netInterface[0] || !Q_stricmp( netInterface, "localhost" ))
		addr.sin_addr.s_addr = INADDR_ANY;
	else NET_StringToSockaddr( netInterface, (struct sockaddr *)&addr );

	if( port == PORT_ANY ) addr.sin_port = 0;
	else addr.sin_port = pHtons((short)port);

	addr.sin_family = AF_INET;

	if( pBind( net_socket, (void *)&addr, sizeof( addr )) == SOCKET_ERROR )
	{
		MsgDev( D_WARN, "NET_UDPSocket: bind = %s\n", NET_ErrorString( ));
		pCloseSocket( net_socket );
		return 0;
	}
	return net_socket;
}

/*
====================
NET_OpenIP
====================
*/
static void NET_OpenIP( void )
{
	int	port;

	net_ip = Cvar_Get( "ip", "localhost", CVAR_INIT, "network ip address" );

	if( !ip_sockets[NS_SERVER] )
	{
		port = Cvar_Get( "ip_hostport", "0", CVAR_INIT, "network server port" )->integer;
		if( !port ) port = Cvar_Get( "port", va( "%i", PORT_SERVER ), CVAR_INIT, "network default port" )->integer;

		ip_sockets[NS_SERVER] = NET_IPSocket( net_ip->string, port );
		if( !ip_sockets[NS_SERVER] && host.type == HOST_DEDICATED )
			Host_Error( "couldn't allocate dedicated server IP port\n" );
	}

	// dedicated servers don't need client ports
	if( host.type == HOST_DEDICATED ) return;

	if( !ip_sockets[NS_CLIENT] )
	{
		port = Cvar_Get( "ip_clientport", "0", CVAR_INIT, "network client port" )->integer;
		if( !port )
		{
			port = Cvar_Get( "clientport", va( "%i", PORT_CLIENT ), CVAR_INIT, "network client port" )->integer;
			if( !port ) port = PORT_ANY;
		}
		ip_sockets[NS_CLIENT] = NET_IPSocket( net_ip->string, port );
		if( !ip_sockets[NS_CLIENT] ) ip_sockets[NS_CLIENT] = NET_IPSocket( net_ip->string, PORT_ANY );
	}
}

/*
====================
NET_IPXSocket
====================
*/
static int NET_IPXSocket( int port )
{
	int		net_socket;
	struct sockaddr_ipx	addr;
	int		_true = 1;
	int		err;

	MsgDev( D_NOTE, "NET_IPXSocket( %i )\n", port );

	if(( net_socket = pSocket( PF_IPX, SOCK_DGRAM, NSPROTO_IPX )) == SOCKET_ERROR )
	{
		err = pWSAGetLastError();
		if( err != WSAEAFNOSUPPORT )
			MsgDev( D_WARN, "NET_IPXSocket: socket = %s\n", NET_ErrorString( ));
		return 0;
	}

	// make it non-blocking
	if( pIoctlSocket( net_socket, FIONBIO, &_true ) == SOCKET_ERROR )
	{
		MsgDev( D_WARN, "NET_IPXSocket: ioctlsocket FIONBIO = %s\n", NET_ErrorString( ));
		pCloseSocket( net_socket );
		return 0;
	}

	// make it broadcast capable
	if( pSetSockopt( net_socket, SOL_SOCKET, SO_BROADCAST, (char *)&_true, sizeof( _true )) == SOCKET_ERROR )
	{
		MsgDev( D_WARN, "NET_IPXSocket: setsockopt SO_BROADCAST = %s\n", NET_ErrorString( ));
		pCloseSocket( net_socket );
		return 0;
	}

	addr.sa_family = AF_IPX;
	Q_memset( addr.sa_netnum, 0, 4 );
	Q_memset( addr.sa_nodenum, 0, 6 );

	if( port == PORT_ANY ) addr.sa_socket = 0;
	else addr.sa_socket = pHtons((short)port );

	if( pBind( net_socket, (void *)&addr, sizeof( addr )) == SOCKET_ERROR )
	{
		MsgDev( D_WARN, "NET_IPXSocket: bind = %s\n", NET_ErrorString( ));
		pCloseSocket( net_socket );
		return 0;
	}

	return net_socket;
}


/*
====================
NET_OpenIPX
====================
*/
void NET_OpenIPX( void )
{
	int	port;

	if( !ipx_sockets[NS_SERVER] )
	{
		port = Cvar_Get( "ipx_hostport", "0", CVAR_INIT, "network server port" )->integer;
		if( !port ) port = Cvar_Get( "port", va( "%i", PORT_SERVER ), CVAR_INIT, "network default port" )->integer;
		ipx_sockets[NS_SERVER] = NET_IPXSocket( port );
	}

	// dedicated servers don't need client ports
	if( host.type == HOST_DEDICATED ) return;

	if( !ipx_sockets[NS_CLIENT] )
	{
		port = Cvar_Get( "ipx_clientport", "0", CVAR_INIT, "network client port" )->integer;
		if( !port )
		{
			port = Cvar_Get( "clientport", va( "%i", PORT_CLIENT ), CVAR_INIT, "network client port" )->integer;
			if( !port ) port = PORT_ANY;
		}

		ipx_sockets[NS_CLIENT] = NET_IPXSocket( port );
		if( !ipx_sockets[NS_CLIENT] ) ipx_sockets[NS_CLIENT] = NET_IPXSocket( PORT_ANY );
	}
}

/*
================
NET_GetLocalAddress

Returns the servers' ip address as a string.
================
*/
void NET_GetLocalAddress( void )
{
	char		buff[512];
	struct sockaddr_in	address;
	int		namelen;

	Q_memset( &net_local, 0, sizeof( netadr_t ));

	if( noip )
	{
		MsgDev( D_INFO, "TCP/IP Disabled.\n" );
	}
	else
	{
		// If we have changed the ip var from the command line, use that instead.
		if( Q_strcmp( net_ip->string, "localhost" ))
		{
			Q_strcpy( buff, net_ip->string );
		}
		else
		{
			pGetHostName( buff, 512 );
		}

		// ensure that it doesn't overrun the buffer
		buff[511] = 0;

		NET_StringToAdr( buff, &net_local );
		namelen = sizeof( address );

		if( pGetSockName( ip_sockets[NS_SERVER], (struct sockaddr *)&address, &namelen ) != 0 )
		{
			MsgDev( D_ERROR, "Could not get TCP/IP address, TCP/IP disabled\nReason:  %s\n", NET_ErrorString( ));
			noip = true;
		}
		else
		{
			net_local.port = address.sin_port;
			Msg( "Server IP address %s\n", NET_AdrToString( net_local ));
		}
	}
}

/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void NET_Config( qboolean multiplayer )
{
	static qboolean old_config;
	static qboolean bFirst = true;

	if( old_config == multiplayer )
		return;

	old_config = multiplayer;

	if( !multiplayer )
	{	
		int	i;

		// shut down any existing sockets
		for( i = 0; i < 2; i++ )
		{
			if( ip_sockets[i] )
			{
				pCloseSocket( ip_sockets[i] );
				ip_sockets[i] = 0;
			}

			if( ipx_sockets[i] )
			{
				pCloseSocket( ipx_sockets[i] );
				ipx_sockets[i] = 0;
			}
		}
	}
	else
	{	
		// open sockets
		if( !noip ) NET_OpenIP();
		if( !noipx ) NET_OpenIPX();

		// Get our local address, if possible
		if( bFirst )
		{
			bFirst = false;
			NET_GetLocalAddress();
		}
	}

	NET_ClearLoopback ();
}

// sleeps msec or until net socket is ready
void NET_Sleep( int msec )
{
	struct timeval	timeout;
	fd_set		fdset;
	int		i = 0;

	if( host.type == HOST_NORMAL )
		return; // we're not a dedicated server, just run full speed

	FD_ZERO( &fdset );

	if( ip_sockets[NS_SERVER] )
	{
		FD_SET( ip_sockets[NS_SERVER], &fdset ); // network socket
		i = ip_sockets[NS_SERVER];
	}

	if( ipx_sockets[NS_SERVER] )
	{
		FD_SET( ipx_sockets[NS_SERVER], &fdset ); // network socket
		if( ipx_sockets[NS_SERVER] > i )
			i = ipx_sockets[NS_SERVER];
	}

	timeout.tv_sec = msec / 1000;
	timeout.tv_usec = (msec % 1000) * 1000;
	pSelect( i+1, &fdset, NULL, NULL, &timeout );
}

/*
=================
NET_ShowIP_f
=================
*/
void NET_ShowIP_f( void )
{
	string		s;
	int		i;
	struct hostent	*h;
	struct in_addr	in;

	pGetHostName( s, sizeof( s ));

	if( !( h = pGetHostByName( s )))
	{
		Msg( "Can't get host\n" );
		return;
	}

	Msg( "HostName: %s\n", h->h_name );

	for( i = 0; h->h_addr_list[i]; i++ )
	{
		in.s_addr = *(int *)h->h_addr_list[i];
		Msg( "IP: %s\n", pInet_Ntoa( in ));
	}
}

/*
====================
NET_Init
====================
*/
void NET_Init( void )
{
	int	r;

	if( !NET_OpenWinSock())	// loading wsock32.dll
	{
		MsgDev( D_WARN, "NET_Init: wsock32.dll can't loaded\n" );
		return;
	}

	r = pWSAStartup( MAKEWORD( 1, 1 ), &winsockdata );
	if( r )
	{
		MsgDev( D_WARN, "NET_Init: winsock initialization failed: %d\n", r );
		return;
	}

	net_showpackets = Cvar_Get( "net_showpackets", "0", 0, "show network packets" );
	Cmd_AddCommand( "net_showip", NET_ShowIP_f,  "show hostname and ip's" );
	Cmd_AddCommand( "net_restart", NET_Restart_f, "restart the network subsystem" );

	if( Sys_CheckParm( "-noip" )) noip = true;
	if( Sys_CheckParm( "-noipx" )) noipx = true;

	winsockInitialized = true;
	MsgDev( D_NOTE, "NET_Init()\n" );
}


/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown( void )
{
	if( !winsockInitialized )
		return;

	Cmd_RemoveCommand( "net_showip" );
	Cmd_RemoveCommand( "net_restart" );

	NET_Config( false );
	pWSACleanup();
	NET_FreeWinSock();
	winsockInitialized = false;
}

/*
=================
NET_Restart_f
=================
*/
void NET_Restart_f( void )
{
	NET_Shutdown();
	NET_Init();
}