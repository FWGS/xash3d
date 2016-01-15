/*
launcher.c - direct xash3d launcher
Copyright (C) 2015 Mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifdef SINGLE_BINARY

#include <stdlib.h>
#include <string.h>
char szGameDir[128]; // safe place to keep gamedir
int szArgc;

void Host_Shutdown( void );

char **szArgv;


void Launcher_ChangeGame( const char *progname )
{
	strncpy( szGameDir, progname, sizeof( szGameDir ) - 1 );
	Host_Shutdown( );
	exit( Host_Main( szArgc, szArgv, szGameDir, 1, &Launcher_ChangeGame ) );
}

int main( int argc, char** argv )
{
	const char *gamedir = getenv("XASH3D_GAMEDIR");
	if(!gamedir)
		gamedir = "valve";
	szArgc = argc;
	szArgv = argv;
	return Host_Main( argc, argv, gamedir, 0, &Launcher_ChangeGame );
}

#endif
