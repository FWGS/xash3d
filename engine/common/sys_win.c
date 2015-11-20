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
#include "port.h"
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
#endif

#else
#include <stdlib.h>
#include <time.h>
#endif

#include "common.h"
#include "mathlib.h"

qboolean	error_on_exit = false;	// arg for exit();

/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime( void )
{
	static longtime_t g_PerformanceFrequency;
	static longtime_t g_ClockStart;
	longtime_t CurrentTime;
#ifdef XASH_SDL
	if( !g_PerformanceFrequency )
	{
		g_PerformanceFrequency = SDL_GetPerformanceFrequency();
		g_ClockStart = SDL_GetPerformanceCounter();
	}
	CurrentTime = SDL_GetPerformanceCounter();
	return (double)( CurrentTime - g_ClockStart ) / (double)( g_PerformanceFrequency );
#else
	struct timespec ts;
	if( !g_PerformanceFrequency )
	{
		struct timespec res;
		if( !clock_getres(CLOCK_MONOTONIC, &res) )
			g_PerformanceFrequency = 1000000000LL/res.tv_nsec;
	}
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec + (double) ts.tv_nsec/1000000000.0;
#endif
}

/*
================
Sys_GetClipboardData

create buffer, that contain clipboard
================
*/
char *Sys_GetClipboardData( void )
{
#ifdef XASH_SDL
	return SDL_GetClipboardText();
#else
	return NULL;
#endif
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
	SDL_SetClipboardText(buffer);
#endif
}

/*
================
Sys_Sleep

freeze application for some time
================
*/
void Sys_Sleep( int msec )
{
	msec = bound( 1, msec, 1000 );
#ifdef XASH_SDL
	SDL_Delay( msec );
#else
	usleep(msec * 1000);
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
	dword			size = sizeof( s_userName );

	if( GetUserName( s_userName, &size ))
		return s_userName;
	return "Player";
#elif !defined(__ANDROID__)
	return cuserid( NULL );
#else
	return "Player";
#endif
}

/*
=================
Sys_ShellExecute
=================
*/
void Sys_ShellExecute( const char *path, const char *parms, qboolean shouldExit )
{
#ifdef _WIN32
	ShellExecute( NULL, "open", path, parms, NULL, SW_SHOW );
#elif !defined __ANDROID__
	//signal(SIGCHLD, SIG_IGN);
	const char* xdgOpen = "/usr/bin/xdg-open"; //TODO: find xdg-open in PATH
	
	const char* argv[] = {xdgOpen, path, NULL};
	pid_t id = fork();
	if (id == 0) {
		execve(xdgOpen, argv, environ);
		printf("error opening %s %s", xdgOpen, path);
		exit(1);
	}
#endif

	if( shouldExit ) Sys_Quit();
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
		if( !Q_stricmp( "+game", host.argv[i] )) host.argv[i] = (char *)blank;
		// you sure that map exists in new game?
		if( !Q_stricmp( "+map", host.argv[i] )) host.argv[i] = (char *)blank;
		// just stupid action
		if( !Q_stricmp( "+load", host.argv[i] )) host.argv[i] = (char *)blank;
		// changelevel beetwen games? wow it's great idea!
		if( !Q_stricmp( "+changelevel", host.argv[i] )) host.argv[i] = (char *)blank;
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
		if( host.type == HOST_DEDICATED && !Q_strnicmp( "+menu_", host.argv[i], 6 ))
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
	if( dll->crash ) Sys_Error( errorstring );
	else MsgDev( D_ERROR, errorstring );			

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

	Con_RegisterHotkeys();		

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
Sys_Crash

Crash handler, called from system
================
*/
#ifdef _WIN32
long _stdcall Sys_Crash( PEXCEPTION_POINTERS pInfo )
{
	// save config
	if( host.state != HOST_CRASHED )
	{
		// check to avoid recursive call
		error_on_exit = true;
		host.crashed = true;

		if( host.type == HOST_NORMAL )
			CL_Crashed(); // tell client about crash
		else host.state = HOST_CRASHED;

		Msg( "Sys_Crash: call %p at address %p\n", pInfo->ExceptionRecord->ExceptionAddress, pInfo->ExceptionRecord->ExceptionCode );

		if( host.developer <= 0 )
		{
			// no reason to call debugger in release build - just exit
			Sys_Quit();
			return EXCEPTION_CONTINUE_EXECUTION;
		}

		// all other states keep unchanged to let debugger find bug
		Con_DestroyConsole();
	}

	if( host.oldFilter )
		return host.oldFilter( pInfo );
	return EXCEPTION_CONTINUE_EXECUTION;
}
#else //if !defined (__ANDROID__) // Android will have other handler
// Posix signal handler
#include <ucontext.h>
#include <sys/mman.h>
int printframe( char *buf, int len, int i, void *addr )
{
	Dl_info dlinfo;
	if( len <= 0 ) return 0; // overflow
	if( dladdr( addr, &dlinfo ))
	{
		if( dlinfo.dli_sname )
			return snprintf( buf, len, "% 2d: %p <%s+%lu> (%s)\n", i, addr, dlinfo.dli_sname,
					(unsigned long)addr - (unsigned long)dlinfo.dli_saddr, dlinfo.dli_fname ); // print symbol, module and address
		else
			return snprintf( buf, len, "% 2d: %p (%s)\n", i, addr, dlinfo.dli_fname ); // print module and address
	}
	else
		return snprintf( buf, len, "% 2d: %p\n", i, addr ); // print only address
}

void Sys_Crash( int signal, siginfo_t *si, void *context)
{
	void *trace[32];
	char message[1024], stackframe[256];
	int len, stacklen, logfd, i = 0;
	ucontext_t *ucontext = (ucontext_t*)context;
#if __i386__
	void *pc = (void*)ucontext->uc_mcontext.gregs[REG_EIP], **bp = (void**)ucontext->uc_mcontext.gregs[REG_EBP], **sp = (void**)ucontext->uc_mcontext.gregs[REG_ESP];
#elif defined (__arm__) // arm not tested
	void *pc = (void*)ucontext->uc_mcontext.arm_pc, **bp = (void*)ucontext->uc_mcontext.arm_r10, **sp = (void*)ucontext->uc_mcontext.arm_sp;
#endif
	// Safe actions first, stack and memory may be corrupted
	len = snprintf(message, 1024, "Sys_Crash: signal %d, err %d with code %d at %p %p\n", signal, si->si_errno, si->si_code, si->si_addr, si->si_ptr);
	write(2, message, len);
	// Flush buffers before writing directly to descriptors
	fflush( stdout );
	fflush( stderr );
	// Now get log fd and write trace directly to log
	logfd = Sys_LogFileNo();
	write( logfd, message, len );
	write( 2, "Stack backtrace:\n", 17 );
	write( logfd, "Stack backtrace:\n", 17 );
	strncpy(message + len, "Stack backtrace:\n", 1024 - len);
	len += 17;
	long pagesize = sysconf(_SC_PAGESIZE);
	do
	{
		int line = printframe( message + len, 1024 - len, ++i, pc);
		write( 2, message + len, line );
		write( logfd, message + len, line );
		len += line;
		//if( !dladdr(bp,0) ) break; // Only when bp is in module
		if( mprotect((char *)(((int) bp + pagesize-1) & ~(pagesize-1)), pagesize, PROT_READ) == -1) break;
		if( mprotect((char *)(((int) bp[0] + pagesize-1) & ~(pagesize-1)), pagesize, PROT_READ) == -1) break;
		pc = bp[1];
		bp = (void**)bp[0];
	}
	while (bp);
	// Try to print stack
	write( 2, "Stack dump:\n", 12 );
	write( logfd, "Stack dump:\n", 12 );
	strncpy(message + len, "Stack dump:\n", 1024 - len);
	len += 12;
	if( mprotect((char *)(((int) sp + pagesize-1) & ~(pagesize-1)), pagesize, PROT_READ) != -1)
		for( i = 0; i < 32; i++ )
		{
			int line = printframe( message + len, 1024 - len, i, sp[i] );
			write( 2, message + len, line );
			write( logfd, message + len, line );
			len += line;
		}
	// Put MessageBox as Sys_Error
	Msg( message );
#ifdef XASH_SDL
	SDL_SetWindowGrab(host.hWnd, false);
	//SDL_MouseQuit();
	MSGBOX( message );
#endif
	// Log saved, now we can try to save configs and close log correctly, it may crash
	if( host.type == HOST_NORMAL )
			CL_Crashed();
	host.state = HOST_CRASHED;
	error_on_exit = true;
	host.crashed = true;
	Con_DestroyConsole();
	Sys_Quit();
}
#endif

/*
================
Sys_Error

NOTE: we must prepare engine to shutdown
before call this
================
*/
void Sys_Error( const char *error, ... )
{
	va_list	argptr;
	char	text[MAX_SYSPATH];
         
	if( host.state == HOST_ERR_FATAL )
		return; // don't execute more than once

	// make sure that console received last message
	if( host.change_game ) Sys_Sleep( 200 );

	error_on_exit = true;
	host.state = HOST_ERR_FATAL;	
	va_start( argptr, error );
	Q_vsprintf( text, error, argptr );
	va_end( argptr );

	SV_SysError( text );

	if( host.type == HOST_NORMAL )
	{
#ifdef XASH_SDL
		if( host.hWnd ) SDL_HideWindow( host.hWnd );
#endif
		VID_RestoreGamma();
	}

	if( host.developer > 0 )
	{
		Con_ShowConsole( true );
		Con_DisableInput();	// disable input line for dedicated server
		MSGBOX( text );
		Sys_Print( text );	// print error message
		Sys_WaitForQuit();
	}
	else
	{
		Con_ShowConsole( false );
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
void Sys_Break( const char *error, ... )
{
	va_list	argptr;
	char	text[MAX_SYSPATH];

	if( host.state == HOST_ERR_FATAL )
		return; // don't multiple executes

	error_on_exit = true;	
	host.state = HOST_ERR_FATAL;         
	va_start( argptr, error );
	Q_vsprintf( text, error, argptr );
	va_end( argptr );

	if( host.type == HOST_NORMAL )
	{
#ifdef XASH_SDL
		if( host.hWnd ) SDL_HideWindow( host.hWnd );
#endif
		VID_RestoreGamma();
	}

	if( host.type != HOST_NORMAL || host.developer > 0 )
	{
		Con_ShowConsole( true );
		Con_DisableInput();	// disable input line for dedicated server
		Sys_Print( text );
		Sys_WaitForQuit();
	}
	else
	{
		Con_ShowConsole( false );
		MSGBOX( text );
	}

	Sys_Quit();
}
/*
================
Sys_Quit
================
*/
void Sys_Quit( void )
{
	MsgDev(D_INFO, "Shutting down...\n");
	Host_Shutdown();
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
	const char	*msg;
	char		buffer[32768];
	char		logbuf[32768];
	char		*b = buffer;
	char		*c = logbuf;	
	int		i = 0;

	if( host.type == HOST_NORMAL )
		Con_Print( pMsg );
#ifdef _WIN32
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
	Con_WinPrint( buffer );
#else
	Sys_PrintLog( pMsg );
#endif
	if( host.rd.target )
	{
		if(( Q_strlen( pMsg ) + Q_strlen( host.rd.buffer )) > ( host.rd.buffersize - 1 ))
		{
			if( host.rd.flush )
			{
				host.rd.flush( host.rd.address, host.rd.target, host.rd.buffer );
				*host.rd.buffer = 0;
			}
		}
		Q_strcat( host.rd.buffer, pMsg );
		return;
	}
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
