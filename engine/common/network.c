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

#ifdef _WIN32
// Winsock
#include <winsock.h>
#include <wsipx.h>
#define socklen_t int //#include <ws2tcpip.h>
#else
// BSD sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
// Errors handling
#include <errno.h>
#include <fcntl.h>
#endif
#include "common.h"
#include "mathlib.h"
#include "netchan.h"

#define PORT_ANY		-1
#define MAX_LOOPBACK	4
#define MASK_LOOPBACK	(MAX_LOOPBACK - 1)

#ifndef _WIN32 // it seems we need to use WS2 to support it
#define HAVE_GETADDRINFO
#endif

#ifdef _WIN32
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
#ifdef HAVE_GETADDRINFO // todo: add definitions for msvc6
int (_stdcall *pGetAddrInfo)(const char *, const char *, const struct addrinfo *, struct addrinfo **);
#endif
static dword (_stdcall *pNtohl)( dword netlong );
static void (_stdcall *pInitializeCriticalSection)( void* );
static void (_stdcall *pEnterCriticalSection)( void* );
static void (_stdcall *pLeaveCriticalSection)( void* );
static void (_stdcall *pDeleteCriticalSection)( void* );
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
#ifdef HAVE_GETADDRINFO
{ "getaddrinfo", (void **) &pGetAddrInfo },
#endif
{ "WSAGetLastError", (void **) &pWSAGetLastError },
{ NULL, NULL }
};

dll_info_t winsock_dll = { "wsock32.dll", winsock_funcs, false };

static dllfunc_t kernel32_funcs[] =
{
	{ "InitializeCriticalSection", (void **) &pInitializeCriticalSection },
	{ "EnterCriticalSection", (void **) &pEnterCriticalSection },
	{ "LeaveCriticalSection", (void **) &pLeaveCriticalSection },
	{ "DeleteCriticalSection", (void **) &pDeleteCriticalSection },
	{ NULL, NULL }
};

dll_info_t kernel32_dll = { "kernel32.dll", kernel32_funcs, false };


static void NET_InitializeCriticalSections( void );

qboolean NET_OpenWinSock( void )
{
	if( Sys_LoadLibrary( &kernel32_dll ) )
		NET_InitializeCriticalSections();

	// initialize the Winsock function vectors (we do this instead of statically linking
	// so we can run on Win 3.1, where there isn't necessarily Winsock)
	return Sys_LoadLibrary( &winsock_dll );
}

void NET_FreeWinSock( void )
{
	Sys_FreeLibrary( &winsock_dll );
}
#else
#define SOCKET_ERROR -1
#define pHtons htons
#define pConnect connect
#define pInet_Addr inet_addr
#define pRecvFrom recvfrom
#define pSendTo sendto
#define pSocket socket
#define pIoctlSocket ioctl
#define pCloseSocket close
#define pSetSockopt setsockopt
#define pBind bind
#define pGetHostName gethostname
#define pGetSockName getsockname
#define pGetHs
#define pRecv recv
#define pSend send
#define pInet_Ntoa inet_ntoa
#define pNtohs ntohs
#define pGetHostByName gethostbyname
#define pSelect select
#define pGetAddrInfo getaddrinfo
#define SOCKET int
#endif

#ifdef __EMSCRIPTEN__
/* All socket operations are non-blocking already */
static int ioctl_stub( int d, unsigned long r, ...)
{
	return 0;
}
#undef pIoctlSocket
#define pIoctlSocket ioctl_stub
#endif

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

typedef struct packetlag_s
{
	byte		*data;	// Raw stream data is stored.
	int			size;
	netadr_t	from;
	float		receivedtime;
	struct packetlag_s	*next;
	struct packetlag_s	*prev;
} packetlag_t;

static loopback_t	loopbacks[NS_COUNT];
static packetlag_t lagdata[NS_COUNT];
static float fakelag = 0.0f; // actual lag value
static qboolean noip = false;
static int		ip_sockets[NS_COUNT];
static qboolean winsockInitialized = false;
//static const char *net_src[2] = { "client", "server" };
#ifdef XASH_IPX
	static qboolean noipx = false;
	static int	ipx_sockets[NS_COUNT];
#endif
static convar_t *net_ip;
static convar_t *net_hostport;
static convar_t *net_clientport;
static convar_t *net_port;
extern convar_t *net_showpackets;
static convar_t	*net_fakelag;
static convar_t	*net_fakeloss;
void NET_Restart_f( void );

#ifdef _WIN32
	static WSADATA winsockdata;
#endif

#ifdef _WIN32
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
#else
#define NET_ErrorString(x) strerror(errno)
#endif

_inline qboolean NET_IsSocketError( int retval )
{
#ifdef _WIN32
	return retval == SOCKET_ERROR ? true : false;
#else
	return retval < 0 ? true : false;
#endif
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
#ifdef XASH_IPX
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
#endif
}


static void NET_SockadrToNetadr( struct sockaddr *s, netadr_t *a )
{
	if( s->sa_family == AF_INET )
	{
		a->type = NA_IP;
		*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		a->port = ((struct sockaddr_in *)s)->sin_port;
	}
#ifdef XASH_IPX
	else if( s->sa_family == AF_IPX )
	{
		a->type = NA_IPX;
		Q_memcpy( &a->ipx[0], ((struct sockaddr_ipx *)s)->sa_netnum, 4 );
		Q_memcpy( &a->ipx[4], ((struct sockaddr_ipx *)s)->sa_nodenum, 6 );
		a->port = ((struct sockaddr_ipx *)s)->sa_socket;
	}
#endif
}

#if !defined XASH_NO_ASYNC_NS_RESOLVE && ( defined _WIN32 || !defined __EMSCRIPTEN__ )
#define CAN_ASYNC_NS_RESOLVE
#endif

#ifdef CAN_ASYNC_NS_RESOLVE
static void NET_ResolveThread( void );
#if !defined _WIN32
#include <pthread.h>
#define mutex_lock pthread_mutex_lock
#define mutex_unlock pthread_mutex_unlock
#define exit_thread( x ) pthread_exit(x)
#define create_thread( pfn ) !pthread_create( &nsthread.thread, NULL, (pfn), NULL )
#define detach_thread( x ) pthread_detach(x)
#define mutex_t  pthread_mutex_t
#define thread_t pthread_t

void *Net_ThreadStart( void *unused )
{
	NET_ResolveThread();
	return NULL;
}

#else // WIN32
struct cs {
	void* p1;
	int   i1, i2;
	void *p2, *p3;
	uint  i4;
};
#define mutex_lock pEnterCriticalSection
#define mutex_unlock pLeaveCriticalSection
#define detach_thread( x ) CloseHandle(x)
#define create_thread( pfn ) nsthread.thread = CreateThread( NULL, 0, pfn, NULL, 0, NULL )
#define mutex_t  struct cs
#define thread_t HANDLE
DWORD WINAPI Net_ThreadStart( LPVOID unused )
{
	NET_ResolveThread();
	ExitThread(0);
	return 0;
}
#endif

#ifdef DEBUG_RESOLVE
#define RESOLVE_DBG(x) Sys_PrintLog(x)
#else
#define RESOLVE_DBG(x)
#endif //  DEBUG_RESOLVE

static struct nsthread_s
{
	mutex_t mutexns;
	mutex_t mutexres;
	thread_t thread;
	int     result;
	string  hostname;
	qboolean busy;
} nsthread
#ifndef _WIN32
= { PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER }
#endif
;

#ifdef _WIN32
static void NET_InitializeCriticalSections( void )
{
	pInitializeCriticalSection( &nsthread.mutexns );
	pInitializeCriticalSection( &nsthread.mutexres );
}
#endif

void NET_ResolveThread( void )
{
#ifdef HAVE_GETADDRINFO
	struct addrinfo *ai = NULL, *cur;
	struct addrinfo hints;
	int sin_addr = 0;

	RESOLVE_DBG( "[resolve thread] starting resolve for " );
	RESOLVE_DBG( nsthread.hostname );
	RESOLVE_DBG( " with getaddrinfo\n" );
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_INET;
	if( !pGetAddrInfo( nsthread.hostname, NULL, &hints, &ai ) )
	{
		for( cur = ai; cur; cur = cur->ai_next ) {
			if( cur->ai_family == AF_INET ) {
				sin_addr = *((int*)&((struct sockaddr_in *)cur->ai_addr)->sin_addr);
				freeaddrinfo( ai );
				ai = NULL;
				break;
			}
		}

		if( ai )
			freeaddrinfo( ai );
	}

	if( sin_addr )
		RESOLVE_DBG( "[resolve thread] getaddrinfo success\n" );
	else
		RESOLVE_DBG( "[resolve thread] getaddrinfo failed\n" );
	mutex_lock( &nsthread.mutexres );
	nsthread.result = sin_addr;
	nsthread.busy = false;
	RESOLVE_DBG( "[resolve thread] returning result\n" );
	mutex_unlock( &nsthread.mutexres );
	RESOLVE_DBG( "[resolve thread] exiting thread\n" );
#else
	struct hostent *res;

	RESOLVE_DBG( "[resolve thread] starting resolve for " );
	RESOLVE_DBG( nsthread.hostname );
	RESOLVE_DBG( " with gethostbyname\n" );

	mutex_lock( &nsthread.mutexns );
	RESOLVE_DBG( "[resolve thread] locked gethostbyname mutex\n" );
	res = pGetHostByName( nsthread.hostname );
	if(res)
		RESOLVE_DBG( "[resolve thread] gethostbyname success\n" );
	else
		RESOLVE_DBG( "[resolve thread] gethostbyname failed\n" );

	mutex_lock( &nsthread.mutexres );
	RESOLVE_DBG( "[resolve thread] returning result\n" );
	if( res )
		nsthread.result = *(int *)res->h_addr_list[0];
	else
		nsthread.result = 0;

	nsthread.busy = false;

	mutex_unlock( &nsthread.mutexns );

	RESOLVE_DBG( "[resolve thread] unlocked gethostbyname mutex\n" );

	mutex_unlock( &nsthread.mutexres );

	RESOLVE_DBG( "[resolve thread] exiting thread\n" );
#endif
}

#endif // CAN_ASYNC_NS_RESOLVE

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

static int NET_StringToSockaddr( const char *s, struct sockaddr *sadr, qboolean nonblocking )
{
	int ip = 0;
	char		*colon;
	char		copy[MAX_SYSPATH];
	
	Q_memset( sadr, 0, sizeof( *sadr ));

#ifdef XASH_IPX
	if((Q_strlen( s ) >= 23 ) && ( s[8] == ':' ) && ( s[21] == ':' )) // check for an IPX address
	{
		int val;
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
#endif
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
#ifdef CAN_ASYNC_NS_RESOLVE
			qboolean asyncfailed = false;
#ifdef _WIN32
			if( pInitializeCriticalSection )
#endif // _WIN32
			{
				if( !nonblocking )
				{
#ifdef HAVE_GETADDRINFO
					struct addrinfo *ai = NULL, *cur;
					struct addrinfo hints;

					memset( &hints, 0, sizeof( hints ) );
					hints.ai_family = AF_INET;
					if( !pGetAddrInfo( copy, NULL, &hints, &ai ) )
					{
						for( cur = ai; cur; cur = cur->ai_next ) {
							if( cur->ai_family == AF_INET ) {
								ip = *((int*)&((struct sockaddr_in *)cur->ai_addr)->sin_addr);
								freeaddrinfo(ai);
								ai = NULL;
								break;
							}
						}

						if( ai )
							freeaddrinfo(ai);
					}
#else
					struct hostent *h;

					mutex_lock( &nsthread.mutexns );
					h = pGetHostByName( copy );
					if( !h )
					{
						mutex_unlock( &nsthread.mutexns );
						return 0;
					}

					ip = *(int *)h->h_addr_list[0];
					mutex_unlock( &nsthread.mutexns );
#endif
				}
				else
				{
					mutex_lock( &nsthread.mutexres );

					if( nsthread.busy )
					{
						mutex_unlock( &nsthread.mutexres );
						return 2;
					}

					if( !Q_strcmp( copy, nsthread.hostname ) )
					{
						ip = nsthread.result;
						nsthread.hostname[0] = 0;
						detach_thread( nsthread.thread );
					}
					else
					{
						Q_strncpy( nsthread.hostname, copy, MAX_STRING );
						nsthread.busy = true;
						mutex_unlock( &nsthread.mutexres );

						if( create_thread( Net_ThreadStart ) )
							return 2;
						else // failed to create thread
						{
							MsgDev( D_ERROR, "NET_StringToSockaddr: failed to create thread!\n");
							nsthread.busy = false;
							asyncfailed = true;
						}
					}

					mutex_unlock( &nsthread.mutexres );
				}
			}
#ifdef _WIN32
			else
				asyncfailed = true;
#else
			if( asyncfailed )
#endif // _WIN32
#endif // CAN_ASYNC_NS_RESOLVE
			{
#ifdef HAVE_GETADDRINFO
				struct addrinfo *ai = NULL, *cur;
				struct addrinfo hints;

				memset( &hints, 0, sizeof( hints ) );
				hints.ai_family = AF_INET;
				if( !pGetAddrInfo( copy, NULL, &hints, &ai ) )
				{
					for( cur = ai; cur; cur = cur->ai_next ) {
						if( cur->ai_family == AF_INET ) {
							ip = *((int*)&((struct sockaddr_in *)cur->ai_addr)->sin_addr);
							freeaddrinfo(ai);
							ai = NULL;
							break;
						}
					}

					if( ai )
						freeaddrinfo(ai);
				}
#else
				struct hostent *h;
				if(!( h = pGetHostByName( copy )))
					return 0;
				ip = *(int *)h->h_addr_list[0];
#endif
			}

			if( !ip )
				return 0;

			*(int *)&((struct sockaddr_in *)sadr)->sin_addr = ip;
		}
	}
	return 1;
}

#undef DO

char *NET_AdrToString( const netadr_t a )
{
	if( a.type == NA_LOOPBACK )
		return "loopback";
	else if( a.type == NA_IP )
		return va( "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], pNtohs( a.port ));
#ifdef XASH_IPX
	return va( "%02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x:%i", a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9], pNtohs( a.port ));
#else
	return NULL; // compiler warning
#endif
}

char *NET_BaseAdrToString( const netadr_t a )
{
	if( a.type == NA_LOOPBACK )
		return "loopback";
	else if( a.type == NA_IP )
		return va( "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3] );
#ifdef XASH_IPX
	return va( "%02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x", a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9] );
#else
	return NULL;
#endif
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
#ifdef XASH_IPX
	if( a.type == NA_IPX )
	{
		if( !Q_memcmp( a.ipx, b.ipx, 10 ))
			return true;
		return false;
	}
#endif

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

#ifdef XASH_IPX
	if( a.type == NA_IPX )
	{
		if(!Q_memcmp( a.ipx, b.ipx, 10 ) && a.port == b.port )
			return true;
		return false;
	}
#endif

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

	Q_memset( adr, 0, sizeof( netadr_t ));
	if( !Q_stricmp( string, "localhost" ) || !Q_stricmp( string, "loopback" ) )
	{
		adr->type = NA_LOOPBACK;
		return true;
	}

	if( !NET_StringToSockaddr( string, &s, false ))
		return false;
	NET_SockadrToNetadr( &s, adr );

	return true;
}

int NET_StringToAdrNB( const char *string, netadr_t *adr )
{
	struct sockaddr s;
	int res;

	Q_memset( adr, 0, sizeof( netadr_t ));
	if( !Q_stricmp( string, "localhost" )  || !Q_stricmp( string, "loopback" ) )
	{
		adr->type = NA_LOOPBACK;
		return true;
	}

	res = NET_StringToSockaddr( string, &s, true );

	if( res == 0 || res == 2 )
		return res;

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
=============================================================================

LAG & LOSS SIMULATION SYSTEM (network debugging)

=============================================================================
*/
/*
==================
NET_RemoveFromPacketList

double linked list remove entry
==================
*/
static void NET_RemoveFromPacketList( packetlag_t *p )
{
	p->prev->next = p->next;
	p->next->prev = p->prev;
	p->prev = NULL;
	p->next = NULL;
}

/*
==================
NET_ClearLaggedList

double linked list remove queue
==================
*/
static void NET_ClearLaggedList( packetlag_t *list )
{
	packetlag_t	*p, *n;

	p = list->next;
	while( p && p != list )
	{
		n = p->next;

		NET_RemoveFromPacketList( p );

		if( p->data )
		{
			Mem_Free( p->data );
			p->data = NULL;
		}

		Mem_Free( p );
		p = n;
	}

	list->prev = list;
	list->next = list;
}

/*
==================
NET_AddToLagged

add lagged packet to stream
==================
*/
static void NET_AddToLagged( netsrc_t sock, packetlag_t *list, packetlag_t *packet, netadr_t *from, size_t length, const void *data, float timestamp )
{
	byte	*pStart;

	if( packet->prev || packet->next )
		return;

	packet->prev = list->prev;
	list->prev->next = packet;
	list->prev = packet;
	packet->next = list;

	pStart = (byte *)Z_Malloc( length );
	memcpy( pStart, data, length );
	packet->data = pStart;
	packet->size = length;
	packet->receivedtime = timestamp;
	memcpy( &packet->from, from, sizeof( netadr_t ));
}

/*
==================
NET_AdjustLag

adjust time to next fake lag
==================
*/
static void NET_AdjustLag( void )
{
	static double	lasttime = 0.0;
	float		diff, converge;
	double		dt;

	dt = host.realtime - lasttime;
	dt = bound( 0.0, dt, 0.1 );
	lasttime = host.realtime;

	if( host.developer >= D_ERROR || !net_fakelag->value )
	{
		if( net_fakelag->value != fakelag )
		{
			diff = net_fakelag->value - fakelag;
			converge = dt * 200.0f;

			if( fabs( diff ) < converge )
				converge = fabs( diff );

			if( diff < 0.0 )
				converge = -converge;

			fakelag += converge;
		}
	}
	else
	{
		MsgDev( D_INFO, "Server must enable dev-mode to activate fakelag\n" );
		Cvar_SetFloat( "fakelag", 0.0 );
		fakelag = 0.0f;
	}
}

/*
====================
NET_ClearLagData

clear fakelag list
====================
*/
void NET_ClearLagData( qboolean bClient, qboolean bServer )
{
	if( bClient ) NET_ClearLaggedList( &lagdata[NS_CLIENT] );
	if( bServer ) NET_ClearLaggedList( &lagdata[NS_SERVER] );
}


/*
==================
NET_LagPacket

add fake lagged packet into rececived message
==================
*/
static qboolean NET_LagPacket( qboolean newdata, netsrc_t sock, netadr_t *from, size_t *length, void *data )
{
	static int losscount[2];
	packetlag_t	*newPacketLag;
	packetlag_t	*packet;
	int		ninterval;
	float		curtime;

	if( fakelag <= 0.0f )
	{
		NET_ClearLagData( true, true );
		return newdata;
	}

	curtime = host.realtime;

	if( newdata )
	{
		if( net_fakeloss->value != 0.0f )
		{
			if( host.developer >= D_ERROR )
			{
				losscount[sock]++;
				if( net_fakeloss->value <= 0.0f )
				{
					ninterval = fabs( net_fakeloss->value );
					if( ninterval < 2 ) ninterval = 2;

					if(( losscount[sock] % ninterval ) == 0 )
						return false;
				}
				else
				{
					if( Com_RandomLong( 0, 100 ) <= net_fakeloss->value )
						return false;
				}
			}
			else
			{
				Cvar_SetFloat( "fakeloss", 0.0 );
			}
		}

		newPacketLag = (packetlag_t *)Z_Malloc( sizeof( packetlag_t ));
		// queue packet to simulate fake lag
		NET_AddToLagged( sock, &lagdata[sock], newPacketLag, from, *length, data, curtime );
	}

	packet = lagdata[sock].next;

	while( packet != &lagdata[sock] )
	{
		if( packet->receivedtime <= curtime - ( fakelag / 1000.0 ))
			break;

		packet = packet->next;
	}

	if( packet == &lagdata[sock] )
		return false;

	NET_RemoveFromPacketList( packet );

	// delivery packet from fake lag queue
	memcpy( data, packet->data, packet->size );
	memcpy( &net_from, &packet->from, sizeof( netadr_t ));
	*length = packet->size;

	if( packet->data )
		Mem_Free( packet->data );

	Mem_Free( packet );

	return true;
}


/*
==================
NET_GetPacket

Never called by the game logic, just the system event queing
==================
*/
qboolean NET_GetPacket( netsrc_t sock, netadr_t *from, byte *data, size_t *length )
{
	int 		ret = SOCKET_ERROR;
	struct sockaddr	addr;
	socklen_t	addr_len;
	int		net_socket = 0;
	int		protocol;

	Q_memset( &addr, 0, sizeof( struct sockaddr ) );

	if( !data || !length )
		return false;

	NET_AdjustLag();

	if( NET_GetLoopPacket( sock, from, data, length ))
	{
		NET_LagPacket( true, sock, from, length, data );
		return true;
	}

	for( protocol = 0; protocol < 2; protocol++ )
	{
		if( !protocol) net_socket = ip_sockets[sock];
#ifdef XASH_IPX
		else net_socket = ipx_sockets[sock];
#endif

		if( !net_socket ) continue;

		addr_len = sizeof( addr );
		ret = pRecvFrom( net_socket, data, NET_MAX_PAYLOAD, 0, (struct sockaddr *)&addr, &addr_len );

		NET_SockadrToNetadr( &addr, from );

		if( NET_IsSocketError( ret ) )
		{
#ifdef _WIN32
			int err = pWSAGetLastError();

			// WSAEWOULDBLOCK and WSAECONNRESET are silent
			if( err == WSAEWOULDBLOCK || err == WSAECONNRESET )
#else
			// WSAEWOULDBLOCK and WSAECONNRESET are silent
			if( errno == EWOULDBLOCK || errno == ECONNRESET )
#endif
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

	if( NET_IsSocketError( ret ) )
	{
		return NET_LagPacket( false, sock, from, length, data );
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
	int		ret;
	struct sockaddr	addr;
	SOCKET		net_socket;

	// sequenced packets are shown in netchan, so just show oob
	if( net_showpackets->integer && *(int *)data == -1 )
		MsgDev( D_INFO, "send packet %4u\n", length );

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
#ifdef XASH_IPX
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
#endif
	else 
	{
		char buf[256];
		Q_strncpy( buf, data,  min( 256, length ));
		MsgDev( D_ERROR, "NET_SendPacket ( %d, %d, \"%s\", %i ): bad address type %i\n", sock, length, buf, to.type, to.type );
		return;
	}

	NET_NetadrToSockadr( &to, &addr );

	ret = pSendTo( net_socket, data, length, 0, &addr, sizeof( addr ));

#ifdef _WIN32
	if (ret == SOCKET_ERROR)
	{
		int err = pWSAGetLastError();

		// WSAEWOULDBLOCK is silent
		if (err == WSAEWOULDBLOCK)
			return;

		// some PPP links don't allow broadcasts
		if ((err == WSAEADDRNOTAVAIL) && ((to.type == NA_BROADCAST) || (to.type == NA_BROADCAST_IPX)))
			return;
	}
#else
	if( ret < 0 )
	{
		// WSAEWOULDBLOCK is silent
		if( errno == EWOULDBLOCK )
			return;

		// some PPP links don't allow broadcasts
		if(( errno == EADDRNOTAVAIL ) && (( to.type == NA_BROADCAST ) || ( to.type == NA_BROADCAST_IPX )))
			return;

		MsgDev( D_ERROR, "NET_SendPacket: %s to %s\n", NET_ErrorString(), NET_AdrToString( to ));
	}
#endif
}

/*
====================
NET_IPSocket
====================
*/
static int NET_IPSocket( const char *netInterface, int port )
{
	int		net_socket;
	struct sockaddr_in	addr;
	dword		_true = 1;

	Q_memset( &addr, 0, sizeof( struct sockaddr_in ) );

	MsgDev( D_NOTE, "NET_UDPSocket( %s, %i )\n", netInterface, port );

#ifdef _WIN32
	if(( net_socket = pSocket( PF_INET, SOCK_DGRAM, IPPROTO_UDP )) == SOCKET_ERROR )
	{
		int err = pWSAGetLastError();
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
#else
	if(( net_socket = pSocket( PF_INET, SOCK_DGRAM, IPPROTO_UDP )) < 0 )
	{
		if( errno != EAFNOSUPPORT )
			MsgDev( D_WARN, "NET_UDPSocket: socket = %s\n", NET_ErrorString( ));
		return 0;
	}

	if( pIoctlSocket( net_socket, FIONBIO, &_true ) < 0 )
	{
		MsgDev( D_WARN, "NET_UDPSocket: ioctlsocket FIONBIO = %s\n", NET_ErrorString( ));
		pCloseSocket( net_socket );
		return 0;
	}

	// make it broadcast capable
	if( pSetSockopt( net_socket, SOL_SOCKET, SO_BROADCAST, (char *)&_true, sizeof( _true )) < 0 )
	{
		MsgDev( D_WARN, "NET_UDPSocket: setsockopt SO_BROADCAST = %s\n", NET_ErrorString( ));
	}
#endif

	if( !netInterface[0] || !Q_stricmp( netInterface, "localhost" ))
		addr.sin_addr.s_addr = INADDR_ANY;
	else NET_StringToSockaddr( netInterface, (struct sockaddr *)&addr, false );

	if( port == PORT_ANY ) addr.sin_port = 0;
	else addr.sin_port = pHtons((short)port);

	addr.sin_family = AF_INET;

#ifdef _WIN32
	if( pBind( net_socket, (void *)&addr, sizeof( addr )) == SOCKET_ERROR )
#else
	if( pBind( net_socket, (void *)&addr, sizeof( addr )) < 0 )
#endif
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
static void NET_OpenIP( qboolean changeport )
{
	int	port;
	qboolean sv_nat = Cvar_VariableInteger( "sv_nat" );
	qboolean cl_nat = Cvar_VariableInteger( "cl_nat" );

	net_ip = Cvar_Get( "ip", "localhost", 0, "network ip address" );

	if( changeport && ( net_port->modified || sv_nat ) )
	{
		// reopen socket to set random port
		if( ip_sockets[NS_SERVER] )
			pCloseSocket( ip_sockets[NS_SERVER] );
		ip_sockets[NS_SERVER] = 0;
		net_port->modified = false;
	}

	if( !ip_sockets[NS_SERVER] )
	{
		port = Cvar_VariableInteger("ip_hostport");

		// nat servers selects random port until ip_hostport specified
		if( !port )
		{
			if( sv_nat )
				port = PORT_ANY;
			else
				port = Cvar_VariableInteger("port");
		}

		ip_sockets[NS_SERVER] = NET_IPSocket( net_ip->string, port );
		if( !ip_sockets[NS_SERVER] && Host_IsDedicated() )
			Host_Error( "Couldn't allocate dedicated server IP port.\nMaybe you're trying to run dedicated server twice?\n" );
	}

	// dedicated servers don't need client ports
	if( Host_IsDedicated() ) return;

	if( changeport && ( net_clientport->modified || cl_nat ) )
	{
		// reopen socket to set random port
		if( ip_sockets[NS_CLIENT] )
			pCloseSocket( ip_sockets[NS_CLIENT] );
		ip_sockets[NS_CLIENT] = 0;
		net_clientport->modified = false;
	}

	if( !ip_sockets[NS_CLIENT] )
	{
		port = Cvar_VariableInteger( "ip_clientport" );

		if( !port )
		{
			if( cl_nat )
				port = PORT_ANY;
			else
				port = net_clientport->integer;
		}

		ip_sockets[NS_CLIENT] = NET_IPSocket( net_ip->string, port );
		if( !ip_sockets[NS_CLIENT] ) ip_sockets[NS_CLIENT] = NET_IPSocket( net_ip->string, PORT_ANY );
	}
}

#ifdef XASH_IPX
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
		if( !port ) port = net_port->integer;
		ipx_sockets[NS_SERVER] = NET_IPXSocket( port );
	}

	// dedicated servers don't need client ports
	if( Host_IsDedicated() ) return;

	if( !ipx_sockets[NS_CLIENT] )
	{
		port = Cvar_Get( "ipx_clientport", "0", CVAR_INIT, "network client port" )->integer;
		if( !port )
		{
			port = net_clientport->integer;
			if( !port ) port = PORT_ANY;
		}

		ipx_sockets[NS_CLIENT] = NET_IPXSocket( port );
		if( !ipx_sockets[NS_CLIENT] ) ipx_sockets[NS_CLIENT] = NET_IPXSocket( PORT_ANY );
	}
}

#endif

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
	socklen_t		namelen;

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
			MsgDev( D_ERROR, "Could not get TCP/IP address, TCP/IP disabled\nReason: %s\n", NET_ErrorString( ));
			noip = true;
		}
		else
		{
			net_local.port = address.sin_port;
			Msg( "Server IP address: %s\n", NET_AdrToString( net_local ));
		}
	}
}

/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void NET_Config( qboolean multiplayer, qboolean changeport )
{
	static qboolean old_config;
	static qboolean bFirst = true;

	if( old_config == multiplayer && !Host_IsDedicated() && ( SV_Active() || CL_Active() ) )
		return;

	old_config = multiplayer;

	if( !multiplayer && !Host_IsDedicated() )
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
#ifdef XASH_IPX
			if( ipx_sockets[i] )
			{
				pCloseSocket( ipx_sockets[i] );
				ipx_sockets[i] = 0;
			}
#endif
		}
	}
	else
	{	
		// open sockets
		if( !noip ) NET_OpenIP( changeport );
#ifdef XASH_IPX
		if( !noipx ) NET_OpenIPX();
#endif

		// Get our local address, if possible
		if( bFirst )
		{
			bFirst = false;
			NET_GetLocalAddress();
		}
	}

	NET_ClearLoopback ();
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
	int i;
#ifdef _WIN32
	int	r;

	if( !NET_OpenWinSock())	// loading wsock32.dll
	{
		MsgDev( D_WARN, "NET_Init: failed to load wsock32.dll\n" );
		return;
	}

	r = pWSAStartup( MAKEWORD( 1, 1 ), &winsockdata );
	if( r )
	{
		MsgDev( D_WARN, "NET_Init: winsock initialization failed: %d\n", r );
		return;
	}
#endif

	net_showpackets = Cvar_Get( "net_showpackets", "0", 0, "show network packets" );
	net_clientport = Cvar_Get( "clientport", "27005", 0, "client tcp/ip port" );
	net_port = Cvar_Get( "port", "27015", 0, "server tcp/ip port" );
	net_ip = Cvar_Get( "ip", "localhost", 0, "local server ip" );

	Cmd_AddCommand( "net_showip", NET_ShowIP_f,  "show hostname and IPs" );
	Cmd_AddCommand( "net_restart", NET_Restart_f, "restart the network subsystem" );

	net_fakelag = Cvar_Get( "fakelag", "0", 0, "lag all incoming network data (including loopback) by xxx ms." );
	net_fakeloss = Cvar_Get( "fakeloss", "0", 0, "act like we dropped the packet this % of the time." );

	// prepare some network data
	for( i = 0; i < NS_COUNT; i++ )
	{
		lagdata[i].prev = &lagdata[i];
		lagdata[i].next = &lagdata[i];
	}


	if( Sys_CheckParm( "-noip" )) noip = true;
#ifdef XASH_IPX
	if( Sys_CheckParm( "-noipx" )) noipx = true;
#endif

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

	NET_ClearLagData( true, true );

	NET_Config( false, false );
#ifdef _WIN32
	pWSACleanup();
	NET_FreeWinSock();
#endif
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

/*
=================================================

HTTP downloader

=================================================
*/

typedef struct httpserver_s
{
	char host[256];
	int port;
	char path[PATH_MAX];
	qboolean needfree;
	struct httpserver_s *next;

} httpserver_t;

enum connectionstate
{
	HTTP_FREE = 0,
	HTTP_OPENED,
	HTTP_SOCKET,
	HTTP_NS_RESOLVED,
	HTTP_CONNECTED,
	HTTP_REQUEST,
	HTTP_REQUEST_SENT,
	HTTP_RESPONSE_RECEIVED,
};

typedef struct httpfile_s
{
	httpserver_t *server;
	char path[PATH_MAX];
	file_t *file;
	int socket;
	int size;
	int downloaded;
	int lastchecksize;
	float checktime;
	float blocktime;
	int id;
	enum connectionstate state;
	qboolean process;
	struct httpfile_s *next;
} httpfile_t;

struct http_static_s
{
	// file and server lists
	httpfile_t *first_file, *last_file;
	httpserver_t *first_server, *last_server;

	 // query or response
	char buf[BUFSIZ];
	int header_size, query_length, bytes_sent;
} http;


int downloadfileid, downloadcount;


convar_t *http_useragent;
convar_t *http_autoremove;
convar_t *http_timeout;



/*
========================
HTTP_ClearCustomServers
========================
*/
void HTTP_ClearCustomServers( void )
{
	if( http.first_file )
		return; // may be referenced

	while( http.first_server && http.first_server->needfree )
	{
		httpserver_t *tmp = http.first_server;

		http.first_server = http.first_server->next;
		Mem_Free( tmp );
	}
}

/*
==============
HTTP_FreeFile

Skip to next server/file, free list node if necessary
==============
*/
void HTTP_FreeFile( httpfile_t *file, qboolean error )
{
	char incname[256];

	// Allways close file and socket
	if( file->file )
		FS_Close( file->file );

	file->file = NULL;

	if( file->socket != -1 )
		pCloseSocket( file->socket );

	file->socket = -1;

	Q_snprintf( incname, 256, "downloaded/%s.incomplete", file->path );
	if( error )
	{
		// Switch to next fastdl server if present
		if( file->server && ( file->state > HTTP_FREE ) )
		{
			file->server = file->server->next;
			file->state = HTTP_FREE; // Reset download state, HTTP_Run() will open file again
			return;
		}

		// Called because there was no servers to download, free file now
		if( http_autoremove->integer == 1 ) // remove broken file
			FS_Delete( incname );
		else // autoremove disabled, keep file
			Msg( "HTTP: Cannot download %s from any server. "
				"You may remove %s now\n", file->path, incname ); // Warn about trash file

		if( file->process )
			CL_ProcessFile( false, file->path ); // Process file, increase counter
	}
	else
	{
		// Success, rename and process file
		char name[256];

		Q_snprintf( name, 256, "downloaded/%s", file->path );
		FS_Rename( incname, name );

		if( file->process )
			CL_ProcessFile( true, name );
		else
			Msg ( "HTTP: Successfully downloaded %s, processing disabled!\n", name );
	}
	// Now free list node
	if( http.first_file == file )
	{
		// Now only first_file is changing progress
		Cvar_SetFloat( "scr_download", -1 );

		if( http.last_file == http.first_file )
			http.last_file = http.first_file = 0;
		else
			http.first_file = file->next;
		Mem_Free( file );
	}
	else if( file->next )
	{
		httpfile_t *tmp = http.first_file, *tmp2;

		while( tmp && ( tmp->next != file ) )
			tmp = tmp->next;

		ASSERT( tmp );

		tmp2 = tmp->next;

		if( tmp2 )
		{
			tmp->next = tmp2->next;
			Mem_Free( tmp2 );
		}
		else
			tmp->next = 0;
	}
	else file->id = -1; // Tail file
}

/*
==============
HTTP_Run

Download next file block if download quered.
Call every frame
==============
*/
void HTTP_Run( void )
{
	int res;
	char buf[BUFSIZ+1];
	char *begin = 0;
	httpfile_t *curfile = http.first_file; // download is single-threaded now, but can be rewrited
	httpserver_t *server;
	float frametime;
	struct sockaddr addr;

	if( !curfile )
		return;

	if( curfile->id == -1) // Tail file
		return;

	server = curfile->server;

	if( !server )
	{
		Msg( "HTTP: No servers to download %s!\n", curfile->path );
		HTTP_FreeFile( curfile, true );
		return;
	}

	if( !curfile->file ) // state == 0
	{
		char name[PATH_MAX];

		Msg( "HTTP: Starting download %s from %s\n", curfile->path, server->host );
		Cbuf_AddText( va( "menu_connectionprogress dl \"%s\" \"%s%s\" %d %d \"(starting)\"\n", curfile->path, server->host, server->path, downloadfileid, downloadcount ) );
		Q_snprintf( name, PATH_MAX, "downloaded/%s.incomplete", curfile->path );

		curfile->file = FS_Open( name, "wb", true );

		if( !curfile->file )
		{
			Msg( "HTTP: Cannot open %s!\n", name );
			HTTP_FreeFile( curfile, true );
			return;
		}

		curfile->state = HTTP_OPENED;
		curfile->blocktime = 0;
		curfile->downloaded = 0;
		curfile->lastchecksize = 0;
		curfile->checktime = 0;
	}

	if( curfile->state < HTTP_SOCKET ) // Socket is not created
	{
		dword mode;

		curfile->socket = pSocket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

		// Now set non-blocking mode
		// You may skip this if not supported by system,
		// but download will lock engine, maybe you will need to add manual returns
#if defined(_WIN32) || defined(__APPLE__) || defined(__FreeBSD__) || defined __EMSCRIPTEN__
		mode = 1;
		pIoctlSocket( curfile->socket, FIONBIO, &mode );
#else
		// SOCK_NONBLOCK is not portable, so use fcntl
		fcntl( curfile->socket, F_SETFL, fcntl( curfile->socket, F_GETFL, 0 ) | O_NONBLOCK );
#endif
		curfile->state = HTTP_SOCKET;
	}

	if( curfile->state < HTTP_NS_RESOLVED )
	{
		res = NET_StringToSockaddr( va( "%s:%d", server->host, server->port ), &addr, true );

		if( res == 2 )
			return; // skip to next frame

		if( !res )
		{
			Msg( "HTTP: Failed to resolve server address for %s!\n", server->host );
			HTTP_FreeFile( curfile, true ); // Cannot connect
			return;
		}
		curfile->state = HTTP_NS_RESOLVED;
	}

	if( curfile->state < HTTP_CONNECTED ) // Connection not enstabilished
	{


		res = pConnect( curfile->socket, &addr, sizeof( struct sockaddr ) );

		if( res )
		{
#ifdef _WIN32
			if( pWSAGetLastError() == WSAEINPROGRESS || pWSAGetLastError() == WSAEWOULDBLOCK )
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined __EMSCRIPTEN__
			if( errno == EINPROGRESS || errno == EWOULDBLOCK )
#else
			if( errno == EINPROGRESS ) // Should give EWOOLDBLOCK if try recv too soon
#endif
				curfile->state = HTTP_CONNECTED;
			else
			{
				Msg( "HTTP: Cannot connect to server: %s\n", NET_ErrorString( ) );
				HTTP_FreeFile( curfile, true ); // Cannot connect
				return;
			}
			return; // skip to next frame
		}
		curfile->state = HTTP_CONNECTED;
	}

	if( curfile->state < HTTP_REQUEST ) // Request not formatted
	{
		http.query_length = Q_snprintf( http.buf, BUFSIZ,
			"GET %s%s HTTP/1.0\r\n"
			"Host: %s\r\n"
			"User-Agent: %s\r\n\r\n", server->path,
			curfile->path, server->host, http_useragent->string );
		http.header_size = 0;
		http.bytes_sent = 0;
		curfile->state = HTTP_REQUEST;
	}

	if( curfile->state < HTTP_REQUEST_SENT ) // Request not sent
	{
		while( http.bytes_sent < http.query_length )
		{
			Cbuf_AddText( va( "menu_connectionprogress dl \"%s\" \"%s%s\" %d %d \"(sending request)\"\n", curfile->path, server->host, server->path, downloadfileid, downloadcount ) );

			res = pSend( curfile->socket, http.buf + http.bytes_sent, http.query_length - http.bytes_sent, 0 );
			if( res < 0 )
			{
#ifdef _WIN32
				if( pWSAGetLastError() != WSAEWOULDBLOCK && pWSAGetLastError() != WSAENOTCONN )
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined __EMSCRIPTEN__
				if( errno != EWOULDBLOCK && errno != ENOTCONN )
#else
				if( errno != EWOULDBLOCK )
#endif
				{
					Msg( "HTTP: Failed to send request: %s\n", NET_ErrorString() );
					HTTP_FreeFile( curfile, true );
					return;
				}
				// increase counter when blocking
				curfile->blocktime += host.frametime;

				if( curfile->blocktime > http_timeout->value )
				{
					Msg( "HTTP: Timeout on request send:\n%s\n", http.buf );
					HTTP_FreeFile( curfile, true );
					return;
				}
				return;
			}
			else
			{
				http.bytes_sent += res;
				curfile->blocktime = 0;
			}
		}

		Msg( "HTTP: Request sent!\n");
		Q_memset( http.buf, 0, BUFSIZ );
		curfile->state = HTTP_REQUEST_SENT;
	}

	frametime = host.frametime; // save frametime to reset it after first iteration

	while( ( res = pRecv( curfile->socket, buf, BUFSIZ, 0 ) ) > 0) // if we got there, we are receiving data
	{
		//MsgDev(D_INFO,"res: %d\n", res);
		curfile->blocktime = 0;

		if( curfile->state < HTTP_RESPONSE_RECEIVED ) // Response still not received
		{
			buf[res] = 0; // string break to search \r\n\r\n
			Q_memcpy( http.buf + http.header_size, buf, res );
			//MsgDev( D_INFO, "%s\n", buf );
			begin = Q_strstr( http.buf, "\r\n\r\n" );
			if( begin ) // Got full header
			{
				int cutheadersize = begin - http.buf + 4; // after that begin of data
				char *length;

				Msg( "HTTP: Got response!\n" );

				if( !Q_strstr(http.buf, "200 OK") )
				{
					*begin = 0; // cut string to print out response
					Msg( "HTTP: Bad response:\n%s\n", http.buf );
					HTTP_FreeFile( curfile, true );
					return;
				}

				// print size
				length = Q_stristr(http.buf, "Content-Length: ");
				if( length )
				{
					int size = Q_atoi( length += 16 );

					Msg( "HTTP: File size is %d\n", size );
					Cbuf_AddText( va( "menu_connectionprogress dl \"%s\" \"%s%s\" %d %d \"(file size is %s)\"\n", curfile->path, server->host, server->path, downloadfileid, downloadcount, Q_pretifymem( size, 1 ) ) );

					if( ( curfile->size != -1 ) && ( curfile->size != size ) ) // check size if specified, not used
						MsgDev( D_WARN, "Server reports wrong file size!\n" );

					curfile->size = size;
				}

				if( curfile->size == -1 )
				{
					// Usually fastdl's reports file size if link is correct
					Msg( "HTTP: File size is unknown!\n" );
					HTTP_FreeFile( curfile, true );
					return;
				}

				curfile->state = HTTP_RESPONSE_RECEIVED; // got response, let's start download
				begin += 4;

				// Write remaining message part
				if( res - cutheadersize - http.header_size > 0 )
				{
					int ret = FS_Write( curfile->file, begin, res - cutheadersize - http.header_size );

					if( ret != res - cutheadersize - http.header_size ) // could not write file
					{
						// close it and go to next
						Msg( "HTTP: Write failed for %s!\n", curfile->path );
						curfile->state = HTTP_FREE;
						HTTP_FreeFile( curfile, true );
						return;
					}
					curfile->downloaded += ret;
				}
			}
			http.header_size += res;
		}
		else if( res > 0 )
		{
			// data download
			int ret = FS_Write( curfile->file, buf, res );

			if ( ret != res )
			{
				// close it and go to next
				Msg( "HTTP: Write failed for %s!\n", curfile->path );
				curfile->state = HTTP_FREE;
				HTTP_FreeFile( curfile, true );
				return;
			}

			curfile->downloaded += ret;
			curfile->lastchecksize += ret;
			curfile->checktime += frametime;
			frametime = 0; // only first iteration increases time, 

			// as after it will run in same frame
			if( curfile->checktime > 5 )
			{
				curfile->checktime = 0;
				Msg( "HTTP: %f KB/s\n", (float)curfile->lastchecksize / ( 5.0 * 1024 ) );
				Cbuf_AddText( va( "menu_connectionprogress dl \"%s\" \"%s%s\" %d %d \"(file size is %s, speed is %.2f KB/s)\"\n", curfile->path, server->host, server->path, downloadfileid, downloadcount, Q_pretifymem( curfile->size, 1 ), (float)curfile->lastchecksize / ( 5.0 * 1024 ) ) );
				curfile->lastchecksize = 0;
			}
		}
	}

	if( curfile->size > 0 )
		Cvar_SetFloat( "scr_download", (float)curfile->downloaded / curfile->size * 100 );

	if( curfile->size > 0 && curfile->downloaded >= curfile->size )
	{
		HTTP_FreeFile( curfile, false ); // success
		return;
	}
	else // if it is not blocking, inform user about problem
#ifdef _WIN32
	if( pWSAGetLastError() != WSAEWOULDBLOCK )
#else
	if( errno != EWOULDBLOCK )
#endif
		Msg( "HTTP: Problem downloading %s:\n%s\n", curfile->path, NET_ErrorString() );
	else
		curfile->blocktime += host.frametime;

	curfile->checktime += frametime;

	if( curfile->blocktime > http_timeout->value )
	{
		Msg( "HTTP: Timeout on receiving data!\n");
		HTTP_FreeFile( curfile, true );
		return;
	}
}

/*
===================
HTTP_AddDownload

Add new download to end of queue
===================
*/
void HTTP_AddDownload( char *path, int size, qboolean process )
{
	httpfile_t *httpfile = Mem_Alloc( net_mempool, sizeof( httpfile_t ) );

	MsgDev( D_INFO, "File %s queued to download\n", path );

	httpfile->size = size;
	httpfile->downloaded = 0;
	httpfile->socket = -1;
	Q_strncpy ( httpfile->path, path, sizeof( httpfile->path ) );

	if( http.last_file )
	{
		// Add next to last download
		httpfile->id = http.last_file->id + 1;
		http.last_file->next= httpfile;
		http.last_file = httpfile;
	}
	else
	{
		// It will be the only download
		httpfile->id = 0;
		http.last_file = http.first_file = httpfile;
	}

	httpfile->file = NULL;
	httpfile->next = NULL;
	httpfile->state = HTTP_FREE;
	httpfile->server = http.first_server;
	httpfile->process = process;
}

/*
===============
HTTP_Download_f

Console wrapper
===============
*/
static void HTTP_Download_f( void )
{
	if( Cmd_Argc() < 2 )
	{
		Msg("Use download <gamedir_path>\n");
		return;
	}

	HTTP_AddDownload( Cmd_Argv( 1 ), -1, false );
}

/*
==============
HTTP_ParseURL
==============
*/
httpserver_t *HTTP_ParseURL( const char *url )
{
	httpserver_t *server;
	int i;

	url = Q_strstr( url, "http://" );

	if( !url )
		return NULL;

	url += 7;
	server = Mem_Alloc( net_mempool, sizeof( httpserver_t ) );
	i = 0;

	while( *url && ( *url != ':' ) && ( *url != '/' ) && ( *url != '\r' ) && ( *url != '\n' ) )
	{
		if( i > sizeof( server->host ) )
			return NULL;

		server->host[i++] = *url++;
	}

	server->host[i] = 0;

	if( *url == ':' )
	{
		server->port = Q_atoi( ++url );

		while( *url && ( *url != '/' ) && ( *url != '\r' ) && ( *url != '\n' ) )
			url++;
	}
	else
		server->port = 80;

	i = 0;

	while( *url && ( *url != '\r' ) && ( *url != '\n' ) )
	{
		if( i > sizeof( server->path ) )
			return NULL;

		server->path[i++] = *url++;
	}

	server->path[i] = 0;
	server->next = NULL;
	server->needfree = false;

	return server;
}

/*
=======================
HTTP_AddCustomServer
=======================
*/
void HTTP_AddCustomServer( const char *url )
{
	httpserver_t *server = HTTP_ParseURL( url );

	if( !server )
	{
		MsgDev ( D_ERROR, "\"%s\" is not valid url!\n", url );
		return;
	}

	server->needfree = true;
	server->next = http.first_server;
	http.first_server = server;
}

/*
=======================
HTTP_AddCustomServer_f
=======================
*/
void HTTP_AddCustomServer_f( void )
{
	if( Cmd_Argc() == 2 )
	{
		HTTP_AddCustomServer( Cmd_Argv( 1 ) );
	}
}

/*
============
HTTP_Clear_f

Clear all queue
============
*/
void HTTP_Clear_f( void )
{
	http.last_file = NULL;
	downloadfileid = downloadcount = 0;

	while( http.first_file )
	{
		httpfile_t *file = http.first_file;

		http.first_file = http.first_file->next;

		if( file->file )
			FS_Close( file->file );

		if( file->socket != -1 )
			pCloseSocket ( file->socket );

		Mem_Free( file );
	}
}

/*
==============
HTTP_Cancel_f

Stop current download, skip to next file
==============
*/
void HTTP_Cancel_f( void )
{
	if( !http.first_file )
		return;

	// if download even not started, it will be removed completely
	http.first_file->state = HTTP_FREE;
	HTTP_FreeFile( http.first_file, true );
}

/*
=============
HTTP_Skip_f

Stop current download, skip to next server
=============
*/
void HTTP_Skip_f( void )
{
	if( http.first_file )
		HTTP_FreeFile( http.first_file, true );
}

/*
=============
HTTP_List_f

Print all pending downloads to console
=============
*/
void HTTP_List_f( void )
{
	httpfile_t *file = http.first_file;

	while( file )
	{
		if( file->id == -1 )
			Msg ( "\t(empty)\n");
		else if ( file->server )
			Msg ( "\t%d %d http://%s:%d/%s%s %d\n", file->id, file->state,
				file->server->host, file->server->port, file->server->path,
				file->path, file->downloaded );
		else
			Msg ( "\t%d %d (no server) %s\n", file->id, file->state, file->path );

		file = file->next;
	}
}

/*
================
HTTP_ResetProcessState

When connected to new server, all old files should not increase counter
================
*/
void HTTP_ResetProcessState( void )
{
	httpfile_t *file = http.first_file;

	while( file )
	{
		file->process = false;
		file = file->next;
	}
}

/*
=============
HTTP_Init
=============
*/
void HTTP_Init( void )
{
	char *serverfile, *line, token[1024];

	http.last_server = NULL;

	http.first_file = http.last_file = NULL;
	http.buf[0] = 0;
	http.header_size = 0;

	Cmd_AddCommand("http_download", &HTTP_Download_f, "Add file to download queue");
	Cmd_AddCommand("http_skip", &HTTP_Skip_f, "Skip current download server");
	Cmd_AddCommand("http_cancel", &HTTP_Cancel_f, "Cancel current download");
	Cmd_AddCommand("http_clear", &HTTP_Clear_f, "Cancel all downloads");
	Cmd_AddCommand("http_list", &HTTP_List_f, "List all queued downloads");
	Cmd_AddCommand("http_addcustomserver", &HTTP_AddCustomServer_f, "Add custom fastdl server");
	http_useragent = Cvar_Get( "http_useragent", "xash3d", CVAR_ARCHIVE, "User-Agent string" );
	http_autoremove = Cvar_Get( "http_autoremove", "1", CVAR_ARCHIVE, "Remove broken files" );
	http_timeout = Cvar_Get( "http_timeout", "45", CVAR_ARCHIVE, "Timeout for http downloader" );

	// Read servers from fastdl.txt
	line = serverfile = (char *)FS_LoadFile( "fastdl.txt", 0, false );

	if( serverfile )
	{
		while( ( line = COM_ParseFile( line, token ) ) )
		{
			httpserver_t *server = HTTP_ParseURL( token );

			if( !server )
				continue;

			if( !http.last_server )
				http.last_server = http.first_server = server;
			else
			{
				http.last_server->next = server;
				http.last_server = server;
			}
		}

		Mem_Free( serverfile );
	}
}

/*
====================
HTTP_Shutdown
====================
*/
void HTTP_Shutdown( void )
{
	HTTP_Clear_f();

	while( http.first_server )
	{
		httpserver_t *tmp = http.first_server;

		http.first_server = http.first_server->next;
		Mem_Free( tmp );
	}

	http.last_server = 0;
}
