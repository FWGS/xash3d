/*
sv_log.c - server logging
Copyright (C) 2016

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
#include "server.h"
#include "net_encode.h"

#include "errno.h"
#include <time.h>

convar_t *mp_logfile;
convar_t *mp_logecho;
convar_t *sv_log_singleplayer;
convar_t *sv_log_onefile;

void Log_InitCvars ( void )
{
	mp_logfile = Cvar_Get( "mp_logfile", "1", CVAR_ARCHIVE, "log server information in the log file" );
	mp_logecho = Cvar_Get( "mp_logecho", "1", CVAR_ARCHIVE, "echoes log information to the console" );

	sv_log_singleplayer = Cvar_Get( "sv_log_singleplayer", "0", CVAR_ARCHIVE, "allows logging in singleplayer games" );
	sv_log_onefile = Cvar_Get( "sv_log_onefile", "0", CVAR_ARCHIVE, "logs server information to only one file" );
}

void Log_Printf( const char *fmt, ... )
{
	va_list argptr;
	char string[ MAX_SYSPATH ];
	time_t ltime;

	struct tm *today;

	if ( !svs.log.network_logging && !svs.log.active )
		return;

	time( &ltime );
	today = localtime( &ltime );

	va_start( argptr, fmt );
	Q_snprintf( string,sizeof( string ), "L %02i/%02i/%04i - %02i:%02i:%02i: ", today->tm_mon + 1, today->tm_mday, today->tm_year + 1900, today->tm_hour, today->tm_min, today->tm_sec );
	Q_vsnprintf( &string[ Q_strlen( string ) ], sizeof( string ) - Q_strlen( string ), fmt, argptr );
	va_end( argptr );

	if ( svs.log.network_logging )
		Netchan_OutOfBandPrint( NS_SERVER, svs.log.net_address, "log %s", string );

	if ( svs.log.active && (sv_maxclients->integer > 1 || sv_log_singleplayer->integer != 0 ) )
	{
		if ( mp_logecho->integer != 0 )
			Con_Printf( "%s", string );

		if( svs.log.file )
		{
			if ( mp_logfile->integer != 0 )
				FS_Printf( svs.log.file, "%s", string );
		}
	}
}

void Log_PrintServerVars( void )
{
	cvar_t *var;

	if ( !svs.log.active )
		return;

	Log_Printf( "Server cvars start\n" );

	for ( var = Cvar_GetList(); var != NULL; var = var->next )
	{
		if ( var->flags & FCVAR_SERVER )
			Log_Printf( "Server cvar \"%s\" = \"%s\"\n", var->name, var->string );
	}
	Log_Printf( "Server cvars end\n" );
}

void Log_Close( void )
{
	if ( svs.log.file )
	{
		Log_Printf( "Log file closed\n" );
		FS_Close( svs.log.file );
	}
	svs.log.file = NULL;
}

void Log_Open( void )
{
	time_t ltime;
	struct tm *today;

	char file_base[ MAX_SYSPATH ];
	char test_file[ MAX_SYSPATH ];

	int i;
	file_t *fp;
	char *temp;

	if ( !svs.log.active || ( sv_log_onefile->integer != 0 && svs.log.file ) )
		return;

	if ( mp_logfile->integer == 0 )
		Con_Printf( "Server logging data to console.\n" );
	else
	{
		Log_Close();
		time( &ltime );
		today = localtime( &ltime );

		temp = Cvar_VariableString( "logsdir" );

		if ( !temp || Q_strlen(temp) <= 0 || Q_strstr( temp, ":" ) || Q_strstr( temp, ".." ) )
			Q_snprintf( file_base, sizeof( file_base ), "logs/L%02i%02i", today->tm_mon + 1, today->tm_mday );

		else Q_snprintf( file_base , sizeof( file_base ), "%s/L%02i%02i", temp, today->tm_mon + 1, today->tm_mday );

		for (i = 0; i < 1000; i++)
		{
			Q_snprintf( test_file, sizeof( test_file ), "%s%03i.log", file_base, i );

			COM_FixSlashes( test_file );

			fp = FS_Open( test_file, "r", true );
			if ( !fp )
			{
				fp = FS_Open( test_file, "w", true );

				if ( fp )
				{
					svs.log.file = fp;

					Con_Printf( "Server logging data to file %s\n", test_file );
					Log_Printf( "Log file started (file \"%s\") (game \"%s\") (version \"%i/%s/%d\")\n", test_file, GI->gamefolder, PROTOCOL_VERSION, XASH_VERSION, Q_buildnum () );
				}
				return;
			}
			FS_Close( fp );
		}
		Con_Printf( "Unable to open logfiles under %s\nLogging disabled\n", file_base );
		svs.log.active = false;
	}
}

void SV_SetLogAddress_f ( void )
{
	const char *s;
	int numeric_port;
	char addr[ MAX_SYSPATH ];
	netadr_t adr;

	if ( Cmd_Argc() != 3 )
	{
		Con_Printf( "logaddress:  usage\nlogaddress ip port\n" );
		if ( svs.log.active )
			Con_Printf( "current:  %s\n", NET_AdrToString (svs.log.net_address) );
		return;
	}

	numeric_port = Q_atoi( Cmd_Argv (2) );
	if ( !numeric_port )
	{
		Con_Printf( "logaddress:  must specify a valid port\n" );
		return;
	}

	s = Cmd_Argv ( 1 );
	if ( !s || *s == '\0' )
	{
		Con_Printf( "logaddress:  unparseable address\n" );
		return;
	}

	Q_snprintf( addr, sizeof (addr), "%s:%i", s, numeric_port );

	if ( !NET_StringToAdr( addr, &adr ) )
	{
		Con_Printf( "logaddress:  unable to resolve %s\n", addr );
		return;
	}

	svs.log.network_logging = true;
	Q_memcpy( &svs.log.net_address, &adr, sizeof( netadr_t ) );
	Con_Printf( "logaddress:  %s\n", NET_AdrToString(adr) );
}

void SV_ServerLog_f(void)
{
	const char *s;
	if ( Cmd_Argc() != 2 )
	{
		Con_Printf( "usage:  log < on | off >\n" );

		if ( svs.log.active )
			Con_Printf( "currently logging\n" );

		else
			Con_Printf( "not currently logging\n" );

		return;
	}
	s = Cmd_Argv( 1 );

	if( Q_stricmp( s,"off" ) )
	{
		if ( Q_stricmp(s, "on" ) )
			Con_Printf( "log:  unknown parameter %s, 'on' and 'off' are valid\n", s );
		else
		{
			svs.log.active = true;
			Log_Open();
		}
	}
	else if ( svs.log.active )
	{
		Log_Close();
		Con_Printf( "Server logging disabled.\n" );
		svs.log.active = false;
	}
}
