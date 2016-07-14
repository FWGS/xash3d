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
#include <pwd.h>
#endif

#else
#include <stdlib.h>
#include <time.h>
#endif

#include "common.h"
#include "mathlib.h"

qboolean	error_on_exit = false;	// arg for exit();

#if defined _WIN32 && !defined XASH_SDL
#include <winbase.h>
/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime( void )
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
#else
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
#endif

/*
================
Sys_GetClipboardData

create buffer, that contain clipboard
================
*/
char *Sys_GetClipboardData( void )
{
	static char data[1024] = "";
#ifdef XASH_SDL
	char *buffer = SDL_GetClipboardText();
	if( buffer )
	{
		Q_strncpy( data, buffer = SDL_GetClipboardText(), 1024 );
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
	unsigned long size = sizeof( s_userName );

	if( GetUserName( s_userName, &size ))
		return s_userName;
	return "Player";
#elif !defined(__ANDROID__)
	uid_t uid = geteuid();
	struct passwd *pw = getpwuid( uid );

	if ( pw ) return pw->pw_name;

	return "Player";
#else
	return "Player";
#endif
}

#if (defined(__linux__) && !defined(__ANDROID__)) || defined (__FreeBSD__) || defined (__NetBSD__) || defined(__OpenBSD__)

qboolean findExecutable( const char *baseName, char *buf, size_t size )
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
	ShellExecute( NULL, "open", path, parms, NULL, SW_SHOW );
#elif (defined(__linux__) && !defined __ANDROID__) || defined (__FreeBSD__) || defined (__NetBSD__) || defined(__OpenBSD__)
	char xdgOpen[128];
	if( findExecutable( "xdg-open", xdgOpen, sizeof( xdgOpen ) ) )
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
	else MsgDev( D_WARN, "Could not find xdg-open utility\n" );
#endif
//TODO: Use 'open' on OS X?
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
#define DEBUG_BREAK
/// TODO: implement on windows too

#ifdef _WIN32
#ifdef DBGHELP
#pragma comment( lib, "dbghelp" )
#pragma comment( lib, "psapi" )
#include <winnt.h>
#include <dbghelp.h>
#include <psapi.h>

int ModuleName( HANDLE process, char *name, void *address, int len )
{
	DWORD_PTR   baseAddress = 0;
	static HMODULE     *moduleArray;
	static unsigned int moduleCount;
	LPBYTE      moduleArrayBytes;
	DWORD       bytesRequired;
	int i;

	if(len < 3)
		return 0;

	if ( !moduleArray && EnumProcessModules( process, NULL, 0, &bytesRequired ) )
	{
		if ( bytesRequired )
		{
			moduleArrayBytes = (LPBYTE)LocalAlloc( LPTR, bytesRequired );

			if ( moduleArrayBytes )
			{
				if( EnumProcessModules( process, (HMODULE *)moduleArrayBytes, bytesRequired, &bytesRequired ) )
				{
					moduleCount = bytesRequired / sizeof( HMODULE );
					moduleArray = (HMODULE *)moduleArrayBytes;
				}
			}
		}
	}

	for( i = 0; i<moduleCount; i++ )
	{
		MODULEINFO info;
		GetModuleInformation( process, moduleArray[i], &info, sizeof(MODULEINFO) );

		if( ( address > info.lpBaseOfDll ) &&
				( (DWORD64)address < (DWORD64)info.lpBaseOfDll + (DWORD64)info.SizeOfImage ) )
			return GetModuleBaseName( process, moduleArray[i], name, len );
	}
	return snprintf(name, len, "???");
}
static void stack_trace( PEXCEPTION_POINTERS pInfo )
{
	char message[1024];
	int len = 0;
	size_t i;
	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();
	IMAGEHLP_LINE64 line;
	DWORD dline = 0;
	DWORD options;
	CONTEXT context;
	STACKFRAME64 stackframe;
	DWORD image;

	memcpy( &context, pInfo->ContextRecord, sizeof(CONTEXT) );
	options = SymGetOptions(); 
	options |= SYMOPT_DEBUG;
	options |= SYMOPT_LOAD_LINES;
	SymSetOptions( options ); 

	SymInitialize( process, NULL, TRUE );

	

	ZeroMemory( &stackframe, sizeof(STACKFRAME64) );

#ifdef _M_IX86
	image = IMAGE_FILE_MACHINE_I386;
	stackframe.AddrPC.Offset = context.Eip;
	stackframe.AddrPC.Mode = AddrModeFlat;
	stackframe.AddrFrame.Offset = context.Ebp;
	stackframe.AddrFrame.Mode = AddrModeFlat;
	stackframe.AddrStack.Offset = context.Esp;
	stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
	image = IMAGE_FILE_MACHINE_AMD64;
	stackframe.AddrPC.Offset = context.Rip;
	stackframe.AddrPC.Mode = AddrModeFlat;
	stackframe.AddrFrame.Offset = context.Rsp;
	stackframe.AddrFrame.Mode = AddrModeFlat;
	stackframe.AddrStack.Offset = context.Rsp;
	stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
	image = IMAGE_FILE_MACHINE_IA64;
	stackframe.AddrPC.Offset = context.StIIP;
	stackframe.AddrPC.Mode = AddrModeFlat;
	stackframe.AddrFrame.Offset = context.IntSp;
	stackframe.AddrFrame.Mode = AddrModeFlat;
	stackframe.AddrBStore.Offset = context.RsBSP;
	stackframe.AddrBStore.Mode = AddrModeFlat;
	stackframe.AddrStack.Offset = context.IntSp;
	stackframe.AddrStack.Mode = AddrModeFlat;
#endif
	len += snprintf( message + len, 1024 - len, "Sys_Crash: address %p, code %p\n", pInfo->ExceptionRecord->ExceptionAddress, pInfo->ExceptionRecord->ExceptionCode );
	if( SymGetLineFromAddr64( process, (DWORD64)pInfo->ExceptionRecord->ExceptionAddress, &dline, &line ) )
	{
		len += snprintf(message + len, 1024 - len,"Exception: %s:%d:%d\n", (char*)line.FileName, (int)line.LineNumber, (int)dline);
	}
	if( SymGetLineFromAddr64( process, stackframe.AddrPC.Offset, &dline, &line ) )
	{
		len += snprintf(message + len, 1024 - len,"PC: %s:%d:%d\n", (char*)line.FileName, (int)line.LineNumber, (int)dline);
	}
	if( SymGetLineFromAddr64( process, stackframe.AddrFrame.Offset, &dline, &line ) )
	{
		len += snprintf(message + len, 1024 - len,"Frame: %s:%d:%d\n", (char*)line.FileName, (int)line.LineNumber, (int)dline);
	}
	for( i = 0; i < 25; i++ )
	{
		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
		BOOL result = StackWalk64(
			image, process, thread,
			&stackframe, &context, NULL,
			SymFunctionTableAccess64, SymGetModuleBase64, NULL);

		DWORD64 displacement = 0;
		if( !result )
			break;

		
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = MAX_SYM_NAME;

		len += snprintf( message + len, 1024 - len, "% 2d %p", i, (void*)stackframe.AddrPC.Offset );
		if( SymFromAddr( process, stackframe.AddrPC.Offset, &displacement, symbol ) )
		{
			len += snprintf( message + len, 1024 - len, " %s ", symbol->Name );
		}
		if( SymGetLineFromAddr64( process, stackframe.AddrPC.Offset, &dline, &line ) )
		{
			len += snprintf(message + len, 1024 - len,"(%s:%d:%d) ", (char*)line.FileName, (int)line.LineNumber, (int)dline);
		}
		len += snprintf( message + len, 1024 - len, "(");
		len += ModuleName( process, message + len, (void*)stackframe.AddrPC.Offset, 1024 - len );
		len += snprintf( message + len, 1024 - len, ")\n");
	}
#ifdef XASH_SDL
	if( host.type != HOST_DEDICATED ) // let system to restart server automaticly
		SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR,"Sys_Crash", message, host.hWnd );
#endif
	Sys_PrintLog(message);

	SymCleanup(process);
}
#endif //DBGHELP
LPTOP_LEVEL_EXCEPTION_FILTER       oldFilter;
long _stdcall Sys_Crash( PEXCEPTION_POINTERS pInfo )
{
	// save config
	if( host.state != HOST_CRASHED )
	{
		// check to avoid recursive call
		error_on_exit = true;
		host.crashed = true;

#ifdef DBGHELP
		stack_trace( pInfo );
#else
		Sys_Warn( "Sys_Crash: call %p at address %p", pInfo->ExceptionRecord->ExceptionAddress, pInfo->ExceptionRecord->ExceptionCode );
#endif

		if( host.type == HOST_NORMAL )
			CL_Crashed(); // tell client about crash
		else host.state = HOST_CRASHED;

		if( host.developer <= 0 )
		{
			// no reason to call debugger in release build - just exit
			Sys_Quit();
			return EXCEPTION_CONTINUE_EXECUTION;
		}

		// all other states keep unchanged to let debugger find bug
		Con_DestroyConsole();
	}

	if( oldFilter )
		return oldFilter( pInfo );
	return EXCEPTION_CONTINUE_EXECUTION;
}

void Sys_SetupCrashHandler( void )
{
	SetErrorMode( SEM_FAILCRITICALERRORS );	// no abort/retry/fail errors
	oldFilter = SetUnhandledExceptionFilter( Sys_Crash );
	host.hInst = GetModuleHandle( NULL );
}

void Sys_RestoreCrashHandler( void )
{
	// restore filter
	if( oldFilter ) SetUnhandledExceptionFilter( oldFilter );
}

#elif defined (CRASHHANDLER)
// Posix signal handler
#include "library.h"
#if defined(__FreeBSD__) || defined(__NetBSD__)
#define HAVE_UCONTEXT_H 1
#endif

#ifdef HAVE_UCONTEXT_H
#include <ucontext.h>
#endif
#include <sys/mman.h>

#ifdef GDB_BREAK
#include <fcntl.h>
qboolean Sys_DebuggerPresent( void )
{
    char buf[1024];
    qboolean debugger_present = false;

    int status_fd = open( "/proc/self/status", O_RDONLY );
    if ( status_fd == -1 )
        return 0;

    ssize_t num_read = read( status_fd, buf, sizeof( buf ) );

    if ( num_read > 0 )
    {
        static const char TracerPid[] = "TracerPid:";
        char *tracer_pid;

        buf[num_read] = 0;
        tracer_pid    = Q_strstr( buf, TracerPid );
        if ( tracer_pid )
            debugger_present = !!Q_atoi( tracer_pid + sizeof( TracerPid ));
    }

    return debugger_present;
}

#undef DEBUG_BREAK
#define DEBUG_BREAK \
	if( Sys_DebuggerPresent() ) \
		asm volatile("int $3;")
		//raise( SIGINT )
#endif

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

struct sigaction oldFilter;

static void Sys_Crash( int signal, siginfo_t *si, void *context)
{
	void *trace[32];
	char message[1024], stackframe[256];
	int len, stacklen, logfd, i = 0;
#if defined(__OpenBSD__)
	struct sigcontext *ucontext = (struct sigcontext*)context;
#else
	ucontext_t *ucontext = (ucontext_t*)context;
#endif
#if defined(__amd64__)
	#if defined(__FreeBSD__)
		void *pc = (void*)ucontext->uc_mcontext.mc_rip, **bp = (void**)ucontext->uc_mcontext.mc_rbp, **sp = (void**)ucontext->uc_mcontext.mc_rsp;
	#elif defined(__NetBSD__)
		void *pc = (void*)ucontext->uc_mcontext.__gregs[REG_RIP], **bp = (void**)ucontext->uc_mcontext.__gregs[REG_RBP], **sp = (void**)ucontext->uc_mcontext.__gregs[REG_RSP];
	#elif defined(__OpenBSD__)
		void *pc = (void*)ucontext->sc_rip, **bp = (void**)ucontext->sc_rbp, **sp = (void**)ucontext->sc_rsp;
	#else
		void *pc = (void*)ucontext->uc_mcontext.gregs[REG_RIP], **bp = (void**)ucontext->uc_mcontext.gregs[REG_RBP], **sp = (void**)ucontext->uc_mcontext.gregs[REG_RSP];
	#endif
#elif defined(__i386__)
	#if defined(__FreeBSD__)
		void *pc = (void*)ucontext->uc_mcontext.mc_eip, **bp = (void**)ucontext->uc_mcontext.mc_ebp, **sp = (void**)ucontext->uc_mcontext.mc_esp;
	#elif defined(__NetBSD__)
		void *pc = (void*)ucontext->uc_mcontext.__gregs[REG_EIP], **bp = (void**)ucontext->uc_mcontext.__gregs[REG_EBP], **sp = (void**)ucontext->uc_mcontext.__gregs[REG_ESP];
	#elif defined(__OpenBSD__)
		void *pc = (void*)ucontext->sc_eip, **bp = (void**)ucontext->sc_ebp, **sp = (void**)ucontext->sc_esp;
	#else
		void *pc = (void*)ucontext->uc_mcontext.gregs[REG_EIP], **bp = (void**)ucontext->uc_mcontext.gregs[REG_EBP], **sp = (void**)ucontext->uc_mcontext.gregs[REG_ESP];
	#endif
#elif defined(__arm__) // arm not tested
	void *pc = (void*)ucontext->uc_mcontext.arm_pc, **bp = (void*)ucontext->uc_mcontext.arm_r10, **sp = (void*)ucontext->uc_mcontext.arm_sp;
#endif
	// Safe actions first, stack and memory may be corrupted
	#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
		len = snprintf( message, 1024, "Sys_Crash: signal %d, err %d with code %d at %p\n", signal, si->si_errno, si->si_code, si->si_addr );
	#else
		len = snprintf( message, 1024, "Sys_Crash: signal %d, err %d with code %d at %p %p\n", signal, si->si_errno, si->si_code, si->si_addr, si->si_ptr );
	#endif
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
	size_t pagesize = sysconf(_SC_PAGESIZE);
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
	Msg( "%s\n", message );
#ifdef XASH_SDL
	SDL_SetWindowGrab( host.hWnd, SDL_FALSE );
#endif
	MSGBOX( message );

	// Log saved, now we can try to save configs and close log correctly, it may crash
	if( host.type == HOST_NORMAL )
			CL_Crashed();
	host.state = HOST_CRASHED;
	error_on_exit = true;
	host.crashed = true;
	Con_DestroyConsole();
	Sys_Quit();
}

void Sys_SetupCrashHandler()
{
	struct sigaction act;
	act.sa_sigaction = Sys_Crash;
	act.sa_flags = SA_SIGINFO | SA_ONSTACK;
	sigaction(SIGSEGV, &act, &oldFilter);
	sigaction(SIGABRT, &act, &oldFilter);
	sigaction(SIGBUS, &act, &oldFilter);
	sigaction(SIGILL, &act, &oldFilter);
}

void Sys_RestoreCrashHandler( void )
{
	// stub
}

#else
void Sys_SetupCrashHandler( void )
{
	// stub
}

void Sys_RestoreCrashHandler( void )
{
	// stub
}
#endif

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
	Q_vsprintf( text, format, argptr );
	va_end( argptr );
	if( !Host_IsDedicated() ) // dedicated server should not hang on messagebox
		MSGBOX(text);
	Msg( "Sys_Warn: %s\n", text );
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
	Q_vsprintf( text, format, argptr );
	va_end( argptr );

	SV_SysError( text );

	if( !Host_IsDedicated() )
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

		Sys_Print( text );	// print error message
		MSGBOX( text );
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
	Q_vsprintf( text, format, argptr );
	va_end( argptr );

	if( !Host_IsDedicated() )
	{
#ifdef XASH_SDL
		if( host.hWnd ) SDL_HideWindow( host.hWnd );
#endif
		VID_RestoreGamma();
	}

	if( Host_IsDedicated() || host.developer > 0 )
	{
		Con_ShowConsole( true );
		Con_DisableInput();	// disable input line for dedicated server
		Sys_Print( text );
		MSGBOX( text );
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
	if( !Host_IsDedicated() )
		Con_Print( pMsg );
#ifdef _WIN32

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
		Con_WinPrint( buffer );
	}
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
