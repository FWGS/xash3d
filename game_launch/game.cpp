/*
game.cpp -- executable to run Xash Engine
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

#include <windows.h>

#define GAME_PATH	"valve"	// default dir to start from

typedef void (*pfnChangeGame)( const char *progname );
typedef int (*pfnInit)( const char *progname, int bChangeGame, pfnChangeGame func );
typedef void (*pfnShutdown)( void );

pfnInit Host_Main;
pfnShutdown Host_Shutdown = NULL;
char szGameDir[128]; // safe place to keep gamedir
HINSTANCE	hEngine;

void Sys_Error( const char *errorstring )
{
	MessageBox( NULL, errorstring, "Xash Error", MB_OK|MB_SETFOREGROUND|MB_ICONSTOP );
	exit( 1 );
}

void Sys_LoadEngine( void )
{
	if(( hEngine = LoadLibrary( "xash.dll" )) == NULL )
	{
		Sys_Error( "Unable to load the xash.dll" );
	}

	if(( Host_Main = (pfnInit)GetProcAddress( hEngine, "Host_Main" )) == NULL )
	{
		Sys_Error( "xash.dll missed 'Host_Main' export" );
	}

	// this is non-fatal for us but change game will not working
	Host_Shutdown = (pfnShutdown)GetProcAddress( hEngine, "Host_Shutdown" );
}

void Sys_UnloadEngine( void )
{
	if( Host_Shutdown ) Host_Shutdown( );
	if( hEngine ) FreeLibrary( hEngine );
}

void Sys_ChangeGame( const char *progname )
{
	if( !progname || !progname[0] ) Sys_Error( "Sys_ChangeGame: NULL gamedir" );
	if( Host_Shutdown == NULL ) Sys_Error( "Sys_ChangeGame: missed 'Host_Shutdown' export\n" );
	strncpy( szGameDir, progname, sizeof( szGameDir ) - 1 );

	Sys_UnloadEngine ();
	Sys_LoadEngine ();
	
	Host_Main( szGameDir, TRUE, ( Host_Shutdown != NULL ) ? Sys_ChangeGame : NULL );
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	Sys_LoadEngine();

	return Host_Main( GAME_PATH, FALSE, ( Host_Shutdown != NULL ) ? Sys_ChangeGame : NULL );
}