/*
sys_win.c - platform dependent code
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#define _GNU_SOURCE

#include "common.h"
#include "mathlib.h"

#ifdef XASH_SDL
#include <SDL_timer.h>
#include <SDL_clipboard.h>
#include <SDL_video.h>
#else
#include <time.h>
#endif
#ifndef _WIN32
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <dlfcn.h>
#ifndef __ANDROID__
extern char **environ;
#include <pwd.h>
#endif

#else
#include <stdlib.h>
#include <time.h>
#endif
#include "menu_int.h" // _UPDATE_PAGE macro


qboolean	error_on_exit = false;	// arg for exit();
#if defined _WIN32 && !defined XASH_SDL
#include <winbase.h>
#endif

/*
================
Sys_DoubleTime
================
*/
#if XASH_TIMER == TIMER_WIN32
#include <winbase.h>

double GAME_EXPORT Sys_DoubleTime( void )
{
	static LARGE_INTEGER g_PerformanceFrequency;
	static LARGE_INTEGER g_ClockStart;
	LARGE_INTEGER CurrentTime;

	if( !g_PerformanceFrequency.QuadPart )
	{
		QueryPerformanceFrequency( &g_PerformanceFrequency );
		QueryPerformanceCounter( &g_ClockStart );
	}

	QueryPerformanceCounter( &CurrentTime );
	return (double)( CurrentTime.QuadPart - g_ClockStart.QuadPart ) / (double)( g_PerformanceFrequency.QuadPart );
}

#elif XASH_TIMER == TIMER_SDL

double GAME_EXPORT Sys_DoubleTime( void )
{
	static longtime_t g_PerformanceFrequency;
	static longtime_t g_ClockStart;
	longtime_t CurrentTime;

	if( !g_PerformanceFrequency )
	{
		g_PerformanceFrequency = SDL_GetPerformanceFrequency();
		g_ClockStart = SDL_GetPerformanceCounter();
	}
	CurrentTime = SDL_GetPerformanceCounter();
	return (double)( CurrentTime - g_ClockStart ) / (double)( g_PerformanceFrequency );
}
#elif XASH_TIMER == TIMER_LINUX

double GAME_EXPORT Sys_DoubleTime( void )
{
	static longtime_t g_PerformanceFrequency;
	static longtime_t g_ClockStart;
	longtime_t CurrentTime;
	struct timespec ts;

	if( !g_PerformanceFrequency )
	{
		struct timespec res;
		if( !clock_getres(CLOCK_MONOTONIC, &res) )
			g_PerformanceFrequency = 1000000000LL/res.tv_nsec;
	}
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec + (double) ts.tv_nsec/1000000000.0;
}
#endif

#define DEBUG_BREAK

#if (defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)) && defined(GDB_BREAK)
#include <fcntl.h>
qboolean Sys_DebuggerPresent( void )
{
	char buf[1024];

	int status_fd = open( "/proc/self/status", O_RDONLY );
	if( status_fd == -1 )
		return 0;

	ssize_t num_read = read( status_fd, buf, sizeof( buf ) );

	if( num_read > 0 )
	{
		static const char TracerPid[] = "TracerPid:";
		const byte *tracer_pid;

		buf[num_read] = 0;
		tracer_pid    = (const byte*)Q_strstr( buf, TracerPid );
		if( !tracer_pid )
			return false;
		//printf( "%s\n", tracer_pid );
		while( *tracer_pid < '0' || *tracer_pid > '9'  )
			if( *tracer_pid++ == '\n' )
				return false;
		//printf( "%s\n", tracer_pid );
		return !!Q_atoi( (const char*)tracer_pid );
	}

	return false;
}

#undef DEBUG_BREAK
#ifdef __i386__
#define DEBUG_BREAK \
	if( Sys_DebuggerPresent() ) \
		asm volatile("int $3;")
#else
#define DEBUG_BREAK \
	if( Sys_DebuggerPresent() ) \
		raise( SIGINT )
#endif // __i386__
// __linux__/__FreeBSD__/__NetBSD__/__OpenBSD__

#elif defined(_WIN32) && !defined(XASH_64BIT)

#ifdef _MSC_VER
BOOL WINAPI IsDebuggerPresent(void);
#define DEBUG_BREAK	if( IsDebuggerPresent() ) \
		_asm{ int 3 }
#else
#define DEBUG_BREAK	if( IsDebuggerPresent() ) \
		asm volatile("int $3;")
#endif // _MSC_VER
// _WIN32 && !XASH_64BIT

#elif defined(__APPLE__) && defined(_DEBUG)

// For more information, see https://developer.apple.com/library/content/qa/qa1361/_index.html
// This must be protected by #if defined(_DEBUG), as the kinfo_proc structure is only available
// in debug.

#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>

static qboolean Sys_DebuggerPresent(void)
// Returns true if the current process is being debugged (either
// running under the debugger or has a debugger attached post facto).
{
	int                 junk;
	int                 mib[4];
	struct kinfo_proc   info;
	size_t              size;

	// Initialize the flags so that, if sysctl fails for some bizarre
	// reason, we get a predictable result.

	info.kp_proc.p_flag = 0;

	// Initialize mib, which tells sysctl the info we want, in this case
	// we're looking for information about a specific process ID.

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PID;
	mib[3] = getpid();

	// Call sysctl.

	size = sizeof(info);
	junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
	assert(junk == 0);

	// We're being debugged if the P_TRACED flag is set.

	return ( (info.kp_proc.p_flag & P_TRACED) != 0 );
}

#undef DEBUG_BREAK
#ifdef __i386__
#define DEBUG_BREAK \
	if( Sys_DebuggerPresent() ) { __asm__("int $3"); }
#else
#define DEBUG_BREAK \
	if( Sys_DebuggerPresent() ) { __asm__("trap"); }
#endif // __i386__

#endif // __APPLE__

/*
================
Sys_GetClipboardData

create buffer, that contain clipboard
================
*/
char *Sys_GetClipboardData( void )
{
	static char data[1024];
	char *buffer;

	data[0] = '\0';

#ifdef XASH_SDL
	buffer = SDL_GetClipboardText();
	if( buffer )
	{
		Q_strncpy( data, buffer, sizeof( data ) );
		SDL_free( buffer );
	}
#endif
	return data;
}

/*
================
Sys_SetClipboardData

write screenshot into clipboard
================
*/
void Sys_SetClipboardData( const byte *buffer, size_t size )
{
#ifdef XASH_SDL
	SDL_SetClipboardText((char *)buffer);
#endif
}

/*
================
Sys_Sleep

freeze application for some time
================
*/
void Sys_Sleep( unsigned int msec )
{
	if( !msec )
		return;

	msec = min( msec, 1000 );
#if XASH_TIMER == TIMER_SDL
	SDL_Delay( msec );
#elif XASH_TIMER == TIMER_WIN32
	Sleep( msec );
#elif XASH_TIMER == TIMER_LINUX
	usleep( msec * 1000 );
#endif
}

/*
================
Sys_GetCurrentUser

returns username for current profile
================
*/
char *Sys_GetCurrentUser( void )
{
#if defined(_WIN32)

	static string	s_userName;
	unsigned long size = sizeof( s_userName );

	if( GetUserName( s_userName, &size ))
		return s_userName;

#elif !defined(__ANDROID__)

	uid_t uid = geteuid();
	struct passwd *pw = getpwuid( uid );

	if( pw )
		return pw->pw_name;

#endif
	return "Player";
}

#if (defined(__linux__) && !defined(__ANDROID__)) || defined (__FreeBSD__) || defined (__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
qboolean Sys_FindExecutable( const char *baseName, char *buf, size_t size )
{
	char *envPath;
	char *part;
	size_t length;
	size_t baseNameLength;
	size_t needTrailingSlash;

	if( !baseName || !baseName[0] )
		return false;

	envPath = getenv( "PATH" );
	if( !envPath )
		return false;

	baseNameLength = Q_strlen( baseName );
	while( *envPath )
	{
		part = Q_strchr( envPath, ':' );
		if( part )
			length = part - envPath;
		else
			length = Q_strlen( envPath );

		if( length > 0 )
		{
			needTrailingSlash = ( envPath[length - 1] == '/' ) ? 0 : 1;
			if( length + baseNameLength + needTrailingSlash < size )
			{
				Q_strncpy( buf, envPath, length + 1 );
				if( needTrailingSlash )
					Q_strcpy( buf + length, "/" );
				Q_strcpy( buf + length + needTrailingSlash, baseName );
				buf[length + needTrailingSlash + baseNameLength] = '\0';
				if( access( buf, X_OK ) == 0 )
					return true;
			}
		}

		envPath += length;
		if( *envPath == ':' )
			envPath++;
	}
	return false;
}
#endif

/*
=================
Sys_ShellExecute
=================
*/
void Sys_ShellExecute( const char *path, const char *parms, qboolean shouldExit )
{
#ifdef _WIN32
	if( !Q_strcmp( path, GENERIC_UPDATE_PAGE ) || !Q_strcmp( path, PLATFORM_UPDATE_PAGE ))
		path = XASH_UPDATE_PAGE;

	ShellExecute( NULL, "open", path, parms, NULL, SW_SHOW );
#elif __EMSCRIPTEN__
	EM_ASM_INT({
				if( confirm( "Open game page?\n"+Pointer_stringify($0) ) )
					document.location.href = Pointer_stringify($0);
				return 0;
			}, (int)path );
#elif (defined(__linux__) && !defined (__ANDROID__)) || defined (__FreeBSD__) || defined (__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)

	if( !Q_strcmp( path, GENERIC_UPDATE_PAGE ) || !Q_strcmp( path, PLATFORM_UPDATE_PAGE ))
		path = XASH_UPDATE_PAGE;

	char xdgOpen[128];
	if( Sys_FindExecutable( OPEN_COMMAND, xdgOpen, sizeof( xdgOpen ) ) )
	{
		const char *argv[] = {xdgOpen, path, NULL};
		pid_t id = fork( );
		if( id == 0 )
		{
			execve( xdgOpen, (char **)argv, environ );
			fprintf( stderr, "error opening %s %s", xdgOpen, path );
			_exit( 1 );
		}
	}
	else MsgDev( D_WARN, "Could not find "OPEN_COMMAND" utility\n" );
#elif defined(__ANDROID__) && !defined(XASH_DEDICATED)
	Android_ShellExecute( path, parms );
#endif

	if( shouldExit )
		Sys_Quit();
}

/*
==================
Sys_ParseCommandLine

==================
*/
void Sys_ParseCommandLine( int argc, const char** argv )
{
	const char	*blank = "censored";
	int		i;

	host.argc = argc;
	host.argv = argv;

	if( !host.change_game ) return;

	for( i = 0; i < host.argc; i++ )
	{
		// we don't want to return to first game
			 if( !Q_stricmp( "-game", host.argv[i] )) host.argv[i] = (char *)blank;
		// probably it's timewaster, because engine rejected second change
		else if( !Q_stricmp( "+game", host.argv[i] )) host.argv[i] = (char *)blank;
		// you sure that map exists in new game?
		else if( !Q_stricmp( "+map", host.argv[i] )) host.argv[i] = (char *)blank;
		// just stupid action
		else if( !Q_stricmp( "+load", host.argv[i] )) host.argv[i] = (char *)blank;
		// changelevel beetwen games? wow it's great idea!
		else if( !Q_stricmp( "+changelevel", host.argv[i] )) host.argv[i] = (char *)blank;
	}
}

/*
==================
Sys_MergeCommandLine

==================
*/
void Sys_MergeCommandLine( )
{
	const char	*blank = "censored";
	int		i;

	if( !host.change_game ) return;

	for( i = 0; i < host.argc; i++ )
	{
		// second call
		if( Host_IsDedicated() && !Q_strnicmp( "+menu_", host.argv[i], 6 ))
			host.argv[i] = (char *)blank;
	}
}

/*
================
Sys_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int Sys_CheckParm( const char *parm )
{
	int	i;

	for( i = 1; i < host.argc; i++ )
	{
		if( !host.argv[i] ) continue;
		if( !Q_stricmp( parm, host.argv[i] ))
			return i;
	}
	return 0;
}

/*
================
Sys_GetParmFromCmdLine

Returns the argument for specified parm
================
*/
qboolean _Sys_GetParmFromCmdLine( char *parm, char *out, size_t size )
{
	int	argc = Sys_CheckParm( parm );

	if( !argc ) return false;
	if( !out ) return false;
	if( !host.argv[argc + 1] ) return false;
	if( argc < 0 || argc >= host.argc ) return false;
	if( argc + 1 == host.argc )
		Q_strncpy( out, "", size );
	else
		Q_strncpy( out, host.argv[argc+1], size );

	return true;
}

void Sys_SendKeyEvents( void )
{
#ifdef _WIN32
	MSG	msg;

	while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ))
	{
		if( !GetMessage( &msg, NULL, 0, 0 ))
			Sys_Quit ();

		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
#endif
}

//=======================================================================
//			DLL'S MANAGER SYSTEM
//=======================================================================
qboolean Sys_LoadLibrary( dll_info_t *dll )
{
	const dllfunc_t	*func;
	string		errorstring;

	// check errors
	if( !dll ) return false;	// invalid desc
	if( dll->link ) return true;	// already loaded

	if( !dll->name || !*dll->name )
		return false; // nothing to load

	MsgDev( D_NOTE, "Sys_LoadLibrary: Loading %s", dll->name );

	if( dll->fcts )
	{
		// lookup export table
		for( func = dll->fcts; func && func->name != NULL; func++ )
			*func->func = NULL;
	}

	if( !dll->link ) dll->link = LoadLibrary ( dll->name ); // environment pathes

	// no DLL found
	if( !dll->link )
	{
		Q_snprintf( errorstring, sizeof( errorstring ), "Sys_LoadLibrary: couldn't load %s\n", dll->name );
		goto error;
	}

	// Get the function adresses
	for( func = dll->fcts; func && func->name != NULL; func++ )
	{
		if( !( *func->func = Sys_GetProcAddress( dll, func->name )))
		{
			Q_snprintf( errorstring, sizeof( errorstring ), "Sys_LoadLibrary: %s missing or invalid function (%s)\n", dll->name, func->name );
			goto error;
		}
	}
	MsgDev( D_NOTE, " - ok\n" );

	return true;
error:
	MsgDev( D_NOTE, " - failed\n" );
	Sys_FreeLibrary( dll ); // trying to free
	if( dll->crash ) Sys_Error( "%s", errorstring );
	else MsgDev( D_ERROR, "%s", errorstring );

	return false;
}

void* Sys_GetProcAddress( dll_info_t *dll, const char* name )
{
	if( !dll || !dll->link ) // invalid desc
		return NULL;

	return (void *)GetProcAddress( dll->link, name );
}

qboolean Sys_FreeLibrary( dll_info_t *dll )
{
	// invalid desc or alredy freed
	if( !dll || !dll->link )
		return false;

	if( host.state == HOST_CRASHED )
	{
		// we need to hold down all modules, while MSVC can find error
		MsgDev( D_NOTE, "Sys_FreeLibrary: hold %s for debugging\n", dll->name );
		return false;
	}
	else MsgDev( D_NOTE, "Sys_FreeLibrary: Unloading %s\n", dll->name );

	FreeLibrary( dll->link );
	dll->link = NULL;

	return true;
}

/*
================
Sys_WaitForQuit

wait for 'Esc' key will be hit
================
*/
void Sys_WaitForQuit( void )
{
#ifdef _WIN32
	MSG	msg;

#ifdef XASH_W32CON
	Wcon_RegisterHotkeys();
#endif

	msg.message = 0;

	// wait for the user to quit
	while( msg.message != WM_QUIT )
	{
		if( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else Sys_Sleep( 20 );
	}
#endif
}

/*
================
Sys_Warn

Just messagebox
================
*/
void Sys_Warn( const char *format, ... )
{
	va_list	argptr;
	char	text[MAX_SYSPATH];

	DEBUG_BREAK;

	va_start( argptr, format );
	Q_vsnprintf( text, MAX_SYSPATH, format, argptr );
	va_end( argptr );
	Msg( "Sys_Warn: %s\n", text );
	if( !Host_IsDedicated() ) // dedicated server should not hang on messagebox
		MSGBOX(text);
}

/*
================
Sys_Error

NOTE: we must prepare engine to shutdown
before call this
================
*/
void Sys_Error( const char *format, ... )
{
	va_list	argptr;
	char	text[MAX_SYSPATH];

	DEBUG_BREAK;

	if( host.state == HOST_ERR_FATAL )
		return; // don't execute more than once

	// make sure that console received last message
	if( host.change_game ) Sys_Sleep( 200 );

	error_on_exit = true;
	host.state = HOST_ERR_FATAL;
	va_start( argptr, format );
	Q_vsnprintf( text, sizeof( text ), format, argptr );
	va_end( argptr );

	SV_SysError( text );

	if( !Host_IsDedicated() )
	{
#ifdef XASH_SDL
		if( host.hWnd ) SDL_HideWindow( host.hWnd );
#endif
	}

	if( host.developer > 0 || host.state != HOST_FRAME )
	{
#ifdef XASH_W32CON
		Wcon_ShowConsole( true );
		Wcon_DisableInput();	// disable input line for dedicated server
#endif
		Sys_Print( text );	// print error message
		MSGBOX( text );
		Sys_WaitForQuit();
	}
	else
	{
#ifdef XASH_W32CON
		Wcon_ShowConsole( false );
#endif
		MSGBOX( text );
	}

	Sys_Quit();
}

/*
================
Sys_Break

same as Error
================
*/
void Sys_Break( const char *format, ... )
{
	va_list	argptr;
	char	text[MAX_SYSPATH];
	DEBUG_BREAK;
	if( host.state == HOST_ERR_FATAL )
		return; // don't multiple executes

	error_on_exit = true;
	host.state = HOST_ERR_FATAL;
	va_start( argptr, format );
	Q_vsnprintf( text, sizeof( text ), format, argptr );
	va_end( argptr );

	if( !Host_IsDedicated() )
	{
#ifdef XASH_SDL
		if( host.hWnd ) SDL_HideWindow( host.hWnd );
#endif
	}

	if( Host_IsDedicated() || host.developer > 0 )
	{
#ifdef XASH_W32CON
		Wcon_ShowConsole( true );
		Wcon_DisableInput();	// disable input line for dedicated server
#endif
		Sys_Print( text );
		MSGBOX( text );
		Sys_WaitForQuit();
	}
	else
	{
#ifdef XASH_W32CON
		Wcon_ShowConsole( false );
#endif
		MSGBOX( text );
	}

	Sys_Quit();
}

#ifdef __EMSCRIPTEN__
/* strange glitchy bug on emscripten
_exit->_Exit->asm._exit->_exit
As we do not need atexit(), just throw hidden exception
*/
#define exit my_exit
void my_exit(int ret)
{
	emscripten_cancel_main_loop();
	printf("exit(%d)\n", ret);
	EM_ASM(if(showElement)showElement('reload', true);throw 'SimulateInfiniteLoop');
}
#endif

/*
================
Sys_Quit2
================
*/
void Sys_Quit( void )
{
	MsgDev( D_INFO, "Shutting down...\n" );
	Host_Shutdown();
	if( host.crashed )
		exit( 127 );
	exit( error_on_exit );
}

/*
================
Sys_Print

print into window console
================
*/
void Sys_Print( const char *pMsg )
{
	if( !Host_IsDedicated() )
		Con_Print( pMsg );

#ifdef XASH_W32CON

	{
		const char	*msg;
		char		buffer[32768];
		char		logbuf[32768];
		char		*b = buffer;
		char		*c = logbuf;
		int		i = 0;
		// if the message is REALLY long, use just the last portion of it
		if( Q_strlen( pMsg ) > sizeof( buffer ) - 1 )
			msg = pMsg + Q_strlen( pMsg ) - sizeof( buffer ) + 1;
		else msg = pMsg;

		// copy into an intermediate buffer
		while( msg[i] && (( b - buffer ) < sizeof( buffer ) - 1 ))
		{
			if( msg[i] == '\n' && msg[i+1] == '\r' )
			{
				b[0] = '\r';
				b[1] = '\n';
				c[0] = '\n';
				b += 2, c++;
				i++;
			}
			else if( msg[i] == '\r' )
			{
				b[0] = '\r';
				b[1] = '\n';
				b += 2;
			}
			else if( msg[i] == '\n' )
			{
				b[0] = '\r';
				b[1] = '\n';
				c[0] = '\n';
				b += 2, c++;
			}
			else if( msg[i] == '\35' || msg[i] == '\36' || msg[i] == '\37' )
			{
				i++; // skip console pseudo graph
			}
			else if( IsColorString( &msg[i] ))
			{
				i++; // skip color prefix
			}
			else
			{
				*b = *c = msg[i];
				b++, c++;
			}
			i++;
		}

		*b = *c = 0; // cutoff garbage

		Sys_PrintLog( logbuf );
		Wcon_Print( buffer );
	}
#else
	Sys_PrintLog( pMsg );
#endif
	Rcon_Print( pMsg );
}

/*
================
Msg

formatted message
================
*/
void Msg( const char *pMsg, ... )
{
	va_list	argptr;
	char	text[8192];

	va_start( argptr, pMsg );
	Q_vsnprintf( text, sizeof( text ), pMsg, argptr );
	va_end( argptr );

	Sys_Print( text );
}

/*
================
MsgDev

formatted developer message
================
*/
void MsgDev( int level, const char *pMsg, ... )
{
	va_list	argptr;
	char	text[8192];

	if( host.developer < level ) return;

	va_start( argptr, pMsg );
	Q_vsnprintf( text, sizeof( text ), pMsg, argptr );
	va_end( argptr );

	switch( level )
	{
	case D_WARN:
		Sys_Print( va( "^3Warning:^7 %s", text ));
		break;
	case D_ERROR:
		Sys_Print( va( "^1Error:^7 %s", text ));
		break;
	case D_INFO:
	case D_NOTE:
	case D_AICONSOLE:
		Sys_Print( text );
		break;
	}
}
