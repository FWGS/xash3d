/*
crtlib.h - internal stdlib
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

#ifndef STDLIB_H
#define STDLIB_H

#include <stdarg.h>
#ifndef XASH_SDL
#include <string.h>
#include <ctype.h>
#endif

// timestamp modes
enum
{
	TIME_FULL = 0,
	TIME_DATE_ONLY,
	TIME_TIME_ONLY,
	TIME_NO_SECONDS,
	TIME_YEAR_ONLY,
	TIME_FILENAME,
};

#define CMD_EXTDLL		BIT( 0 )		// added by game.dll
#define CMD_CLIENTDLL	BIT( 1 )		// added by client.dll
#define CMD_LOCALONLY   BIT( 2 )        // should be executed only from local buffers

typedef void (*setpair_t)( const char *key, const char *value, void *buffer, void *numpairs );
typedef void (*xcommand_t)( void );

typedef enum
{
	src_client,	// came in over a net connection as a clc_stringcmd
			// host_client will be valid during this state.
	src_command,	// from the command buffer
	src_server, // from svc_stufftext
} cmd_source_t;

extern cmd_source_t		cmd_source;

// NOTE: if this is changed, it must be changed in cvardef.h too
typedef struct convar_s
{
	// this part shared with cvar_t
	char		*name;
	char		*string;
	int		flags;
	float		value;
	struct convar_s	*next;

	// this part unique for convar_t
	int		integer;		// atoi( string )
	qboolean		modified;		// set each time the cvar is changed
	char		*reset_string;	// cvar_restart will reset to this value
	char		*latched_string;	// for CVAR_LATCH vars
	char		*description;	// variable descrition info
} convar_t;

// cvar flags
typedef enum
{
	CVAR_ARCHIVE	= BIT(0),	// set to cause it to be saved to config.cfg
	CVAR_USERINFO	= BIT(1),	// added to userinfo  when changed
	CVAR_SERVERNOTIFY	= BIT(2),	// notifies players when changed
	CVAR_EXTDLL	= BIT(3),	// defined by external DLL
	CVAR_CLIENTDLL	= BIT(4),	// defined by the client dll
	CVAR_PROTECTED	= BIT(5),	// it's a server cvar, but we don't send the data since it's a password, etc.
	CVAR_SPONLY	= BIT(6),	// this cvar cannot be changed by clients connected to a multiplayer server.
	CVAR_PRINTABLEONLY	= BIT(7),	// this cvar's string cannot contain unprintable characters ( player name )
	CVAR_UNLOGGED	= BIT(8),	// if this is a FCVAR_SERVER, don't log changes to the log file / console
	CVAR_NOEXTRAWHITESPACE = BIT(9), // strip trailing/leading white space from this cvar
	CVAR_SERVERINFO	= BIT(10),	// added to serverinfo when changed
	CVAR_PHYSICINFO	= BIT(11),// added to physinfo when changed
	CVAR_RENDERINFO	= BIT(12),// save to a seperate config called opengl.cfg
	CVAR_CHEAT	= BIT(13),// can not be changed if cheats are disabled
	CVAR_INIT		= BIT(14),// don't allow change from console at all, but can be set from the command line
	CVAR_LATCH	= BIT(15),// save changes until server restart
	CVAR_READ_ONLY	= BIT(16),// display only, cannot be set by user at all
	CVAR_LATCH_VIDEO	= BIT(17),// save changes until render restart
	CVAR_USER_CREATED	= BIT(18),// created by a set command (dll's used)
	CVAR_GLCONFIG	= BIT(19),// set to cause it to be saved to opengl.cfg
	CVAR_LOCALONLY  = BIT(20), // can be set only from local buffers
} cvar_flags_t;

#include "cvardef.h"

//
// cvar.c
//
convar_t *Cvar_FindVar( const char *var_name );
void Cvar_RegisterVariable( cvar_t *variable );
convar_t *Cvar_Get( const char *var_name, const char *value, int flags, const char *description );
void Cvar_Set( const char *var_name, const char *value );
convar_t *Cvar_Set2( const char *var_name, const char *value, qboolean force );
void Cvar_LookupVars( int checkbit, void *buffer, void *ptr, setpair_t callback );
void Cvar_FullSet( const char *var_name, const char *value, int flags );
void Cvar_SetLatched( const char *var_name, const char *value );
void Cvar_SetFloat( const char *var_name, float value );
float Cvar_VariableValue( const char *var_name );
int Cvar_VariableInteger( const char *var_name );
char *Cvar_VariableString( const char *var_name );
void Cvar_DirectSet( cvar_t *var, const char *value );
void Cvar_Reset( const char *var_name );
void Cvar_SetCheatState( qboolean force );
qboolean Cvar_Command( convar_t *v );
void Cvar_WriteVariables( file_t *f );
void Cvar_Init( void );
char *Cvar_Userinfo( void );
char *Cvar_Serverinfo( void );
void Cvar_Unlink( void );

//
// cmd.c
//
void Cbuf_Init( void );
void Cbuf_Clear( void );
void Cbuf_AddText( const char *text );
void Cbuf_AddFilterText( const char *text );
void Cbuf_InsertText( const char *text );
void Cbuf_Execute (void);
int Cmd_Argc( void );
char *Cmd_Args( void );
char *Cmd_Argv( int arg );
void Cmd_Init( void );
void Cmd_Shutdown( void );
void Cmd_Unlink( int group );
void Cmd_AddCommand( const char *cmd_name, xcommand_t function, const char *cmd_desc );
void Cmd_AddRestrictedCommand( const char *cmd_name, xcommand_t function, const char *cmd_desc );
void Cmd_AddGameCommand( const char *cmd_name, xcommand_t function );
void Cmd_AddClientCommand( const char *cmd_name, xcommand_t function );
void Cmd_RemoveCommand( const char *cmd_name );
qboolean Cmd_Exists( const char *cmd_name );
void Cmd_LookupCmds( char *buffer, void *ptr, setpair_t callback );
qboolean Cmd_GetMapList( const char *s, char *completedname, int length );
qboolean Cmd_GetDemoList( const char *s, char *completedname, int length );
qboolean Cmd_GetMovieList( const char *s, char *completedname, int length );
void Cmd_TokenizeString( const char *text );
void Cmd_ExecuteString( const char *text, cmd_source_t src );
void Cmd_ForwardToServer( void );

//
// crtlib.c
//

#define Q_strupr( in, out ) Q_strnupr( in, out, 99999 )
void Q_strnupr( const char *in, char *out, size_t size_out );
#define Q_strlwr( in, out ) Q_strnlwr( in, out, 99999 )
void Q_strnlwr( const char *in, char *out, size_t size_out );

#ifndef XASH_SKIPCRTLIB
char Q_toupper( const char in );
char Q_tolower( const char in );
#else
static inline int Q_strlen( const char *str )
{
	if( !str )
		return 0;
	return strlen(str);
}
#define Q_toupper toupper
#define Q_tolower tolower
#endif

#ifndef XASH_FORCEINLINE
size_t Q_strncat( char *dst, const char *src, size_t siz );
size_t Q_strncpy( char *dst, const char *src, size_t siz );
size_t Q_strcat( char *dst, const char *src );
size_t Q_strcpy( char *dst, const char *src );
int Q_strlen( const char *string );
#else
#include "crtlib_inline.h"
#endif
#define copystring( s ) _copystring( host.mempool, s, __FILE__, __LINE__ )
char *_copystring( byte *mempool, const char *s, const char *filename, int fileline );
qboolean Q_isdigit( const char *str );
#ifndef XASH_SKIPCRTLIB
int Q_atoi( const char *str );
float Q_atof( const char *str );
#else
#define Q_atoi atoi
#define Q_atof atof
#endif
void Q_atov( float *vec, const char *str, size_t siz );

#ifndef XASH_SKIPCRTLIB

#ifndef XASH_FORCEINLINE
char *Q_strchr( const char *s, char c );
char *Q_strrchr( const char *s, char c );
int Q_strnicmp( const char *s1, const char *s2, int n );
int Q_strncmp( const char *s1, const char *s2, int n );
int Q_stricmp( const char *s1, const char *s2 );
int Q_strcmp( const char *s1, const char *s2 );
#endif

#else // XASH_SKIPCRTLIB
static inline char *Q_strchr( const char *s, char c )
{
	if( !s )
		return NULL;
	return strchr( s, c );
}
static inline char *Q_strrchr( const char *s, char c )
{
	if( !s )
		return NULL;
	return strrchr( s, c );
}
static inline int Q_stricmp( const char *s1, const char *s2 )
{
	if( s1 == NULL )
	{
		if( s2 == NULL )
			return 0;
		else return -1;
	}
	else if( s2 == NULL )
	{
		return 1;
	}
	return strcasecmp( s1, s2 );
}
static inline int Q_strnicmp( const char *s1, const char *s2, int n )
{
	if( s1 == NULL )
	{
		if( s2 == NULL )
			return 0;
		else return -1;
	}
	else if( s2 == NULL )
	{
		return 1;
	}
	return strncasecmp( s1, s2, n );
}
static inline int Q_strcmp( const char *s1, const char *s2 )
{
	if( s1 == NULL )
	{
		if( s2 == NULL )
			return 0;
		else return -1;
	}
	else if( s2 == NULL )
	{
		return 1;
	}
	return strcmp( s1, s2 );
}
static inline int Q_strncmp( const char *s1, const char *s2, int n )
{
	if( s1 == NULL )
	{
		if( s2 == NULL )
			return 0;
		else return -1;
	}
	else if( s2 == NULL )
	{
		return 1;
	}
	return strncmp( s1, s2, n );
}
#endif

qboolean Q_stricmpext( const char *s1, const char *s2 );
const char *Q_timestamp( int format );

#ifndef XASH_SKIPCRTLIB
char *Q_stristr( const char *string, const char *string2 );
char *Q_strstr( const char *string, const char *string2 );
#define Q_vsprintf( buffer, format, args ) Q_vsnprintf( buffer, 99999, format, args )
int Q_vsnprintf( char *buffer, size_t buffersize, const char *format, va_list args );
int Q_snprintf( char *buffer, size_t buffersize, const char *format, ... ) _format(3);
int Q_sprintf( char *buffer, const char *format, ... ) _format(2);
#else // XASH_SKIPCRTLIB
#define Q_stristr strcasestr
#define Q_strstr strstr
#define Q_vsprintf vsprintf
#define Q_vsnprintf vsnprintf
#define Q_snprintf snprintf
#define Q_sprintf sprintf
#endif

#define Q_memprint( val ) Q_pretifymem( val, 2 )
char *Q_pretifymem( float value, int digitsafterdecimal );
char *va( const char *format, ... ) _format(1);
#if 0 // debug mem*
#define Q_memcpy( dest, src, size ) _Q_memcpy( dest, src, size, __FILE__, __LINE__ )
#define Q_memset( dest, val, size ) _Q_memset( dest, val, size, __FILE__, __LINE__ )
#define Q_memcmp( src0, src1, siz ) _Q_memcmp( src0, src1, siz, __FILE__, __LINE__ )
#define Q_memmove( dest, src, size ) _Q_memmove( dest, src, size, __FILE__, __LINE__ )
#else
#define Q_memcpy memcpy
#define Q_memset memset
#define Q_memcmp memcmp
#define Q_memmove memmove
#endif
void _Q_memset( void *dest, int set, size_t count, const char *filename, int fileline );
void _Q_memcpy( void *dest, const void *src, size_t count, const char *filename, int fileline );
int _Q_memcmp( const void *src0, const void *src1, size_t count, const char *filename, int fileline );
void _Q_memmove( void *dest, const void *src, size_t count, const char *filename, int fileline );

//
// zone.c
//
void *_Mem_Realloc( byte *poolptr, void *memptr, size_t size, const char *filename, int fileline );
void *_Mem_Alloc( byte *poolptr, size_t size, const char *filename, int fileline );
byte *_Mem_AllocPool( const char *name, const char *filename, int fileline );
void _Mem_FreePool( byte **poolptr, const char *filename, int fileline );
void _Mem_EmptyPool( byte *poolptr, const char *filename, int fileline );
void _Mem_Free( void *data, const char *filename, int fileline );
void _Mem_Check( const char *filename, int fileline );
qboolean Mem_IsAllocatedExt( byte *poolptr, void *data );
void Mem_PrintList( size_t minallocationsize );
void Mem_PrintStats( void );

#define Mem_Alloc( pool, size ) _Mem_Alloc( pool, size, __FILE__, __LINE__ )
#define Mem_Realloc( pool, ptr, size ) _Mem_Realloc( pool, ptr, size, __FILE__, __LINE__ )
#define Mem_Free( mem ) _Mem_Free( mem, __FILE__, __LINE__ )
#define Mem_AllocPool( name ) _Mem_AllocPool( name, __FILE__, __LINE__ )
#define Mem_FreePool( pool ) _Mem_FreePool( pool, __FILE__, __LINE__ )
#define Mem_EmptyPool( pool ) _Mem_EmptyPool( pool, __FILE__, __LINE__ )
#define Mem_IsAllocated( mem ) Mem_IsAllocatedExt( NULL, mem )
#define Mem_Check() _Mem_Check( __FILE__, __LINE__ )
	
#endif//STDLIB_H
