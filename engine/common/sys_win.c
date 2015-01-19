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
	static LARGE_INTEGER	g_PerformanceFrequency;
	static LARGE_INTEGER	g_ClockStart;
	LARGE_INTEGER		CurrentTime;

	if( !g_PerformanceFrequency.QuadPart )
	{
		QueryPerformanceFrequency( &g_PerformanceFrequency );
		QueryPerformanceCounter( &g_ClockStart );
	}
	QueryPerformanceCounter( &CurrentTime );

	return (double)( CurrentTime.QuadPart - g_ClockStart.QuadPart ) / (double)( g_PerformanceFrequency.QuadPart );
}

/*
================
Sys_GetClipboardData

create buffer, that contain clipboard
================
*/
char *Sys_GetClipboardData( void )
{
	char	*data = NULL;
	char	*cliptext;

	if( OpenClipboard( NULL ) != 0 )
	{
		HANDLE	hClipboardData;

		if(( hClipboardData = GetClipboardData( CF_TEXT )) != 0 )
		{
			if(( cliptext = GlobalLock( hClipboardData )) != 0 ) 
			{
				data = Z_Malloc( GlobalSize( hClipboardData ) + 1 );
				Q_strcpy( data, cliptext );
				GlobalUnlock( hClipboardData );
			}
		}
		CloseClipboard();
	}
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
	EmptyClipboard();

	if( OpenClipboard( NULL ) != 0 )
	{
		HGLOBAL hResult = GlobalAlloc( GMEM_MOVEABLE, size ); 
		byte *bufferCopy = (byte *)GlobalLock( hResult ); 

		Q_memcpy( bufferCopy, buffer, size ); 
		GlobalUnlock( hResult ); 

		if( SetClipboardData( CF_DIB, hResult ) == NULL )
		{
			MsgDev( D_ERROR, "unable to write screenshot\n" );
			GlobalFree( hResult );
		}
		CloseClipboard();
	}
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
	Sleep( msec );
}

/*
================
Sys_GetCurrentUser

returns username for current profile
================
*/
char *Sys_GetCurrentUser( void )
{
	static string	s_userName;
	dword		size = sizeof( s_userName );

	if( !GetUserName( s_userName, &size ) || !s_userName[0] )
		Q_strcpy( s_userName, "player" );

	return s_userName;
}

/*
=================
Sys_ShellExecute
=================
*/
void Sys_ShellExecute( const char *path, const char *parms, qboolean exit )
{
	ShellExecute( NULL, "open", path, parms, NULL, SW_SHOW );

	if( exit ) Sys_Quit();
}

/*
==================
Sys_ParseCommandLine

==================
*/
void Sys_ParseCommandLine( LPSTR lpCmdLine )
{
	const char	*blank = "censored";
	static char	commandline[MAX_SYSPATH];
	int		i;

	host.argc = 1;
	host.argv[0] = "exe";

	Q_strncpy( commandline, lpCmdLine, Q_strlen( lpCmdLine ) + 1 );
	lpCmdLine = commandline; // to prevent modify original commandline

	while( *lpCmdLine && ( host.argc < MAX_NUM_ARGVS ))
	{
		while( *lpCmdLine && *lpCmdLine <= ' ' )
			lpCmdLine++;
		if( !*lpCmdLine ) break;

		if( *lpCmdLine == '\"' )
		{
			// quoted string
			lpCmdLine++;
			host.argv[host.argc] = lpCmdLine;
			host.argc++;
			while( *lpCmdLine && ( *lpCmdLine != '\"' ))
				lpCmdLine++;
		}
		else
		{
			// unquoted word
			host.argv[host.argc] = lpCmdLine;
			host.argc++;
			while( *lpCmdLine && *lpCmdLine > ' ')
				lpCmdLine++;
		}

		if( *lpCmdLine )
		{
			*lpCmdLine = 0;
			lpCmdLine++;
		}
	}

	if( !host.change_game ) return;

	for( i = 0; i < host.argc; i++ )
	{
		// we wan't return to first game
		if( !Q_stricmp( "-game", host.argv[i] )) host.argv[i] = (char *)blank;
		// probably it's timewaster, because engine rejected second change
		if( !Q_stricmp( "+game", host.argv[i] )) host.argv[i] = (char *)blank;
		// you sure what is map exists in new game?
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
void Sys_MergeCommandLine( LPSTR lpCmdLine )
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
	MSG	msg;

	while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ))
	{
		if( !GetMessage( &msg, NULL, 0, 0 ))
			Sys_Quit ();

      		TranslateMessage( &msg );
      		DispatchMessage( &msg );
	}
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
}

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
		return; // don't multiple executes

	// make sure what console received last message
	if( host.change_game ) Sys_Sleep( 200 );

	error_on_exit = true;
	host.state = HOST_ERR_FATAL;	
	va_start( argptr, error );
	Q_vsprintf( text, error, argptr );
	va_end( argptr );

	SV_SysError( text );

	if( host.type == HOST_NORMAL )
	{
		if( host.hWnd ) ShowWindow( host.hWnd, SW_HIDE );
		VID_RestoreGamma();
	}

	if( host.developer > 0 )
	{
		Con_ShowConsole( true );
		Con_DisableInput();	// disable input line for dedicated server
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
		if( host.hWnd ) ShowWindow( host.hWnd, SW_HIDE );
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