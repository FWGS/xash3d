/*
common.c - misc functions used by dlls'
Copyright (C) 2008 Uncle Mike

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
#include "studio.h"
#include "mathlib.h"
#include "const.h"
#include "client.h"
#include "library.h"

/*
==============
COM_ParseFile

text parser
==============
*/
char *COM_ParseFile( char *data, char *token )
{
	int	c, len;

	if( !token )
		return NULL;
	
	len = 0;
	token[0] = 0;
	
	if( !data )
		return NULL;
// skip whitespace
skipwhite:
	while(( c = ((byte)*data)) <= ' ' )
	{
		if( c == 0 )
			return NULL;	// end of file;
		data++;
	}
	
	// skip // comments
	if( c=='/' && data[1] == '/' )
	{
		while( *data && *data != '\n' )
			data++;
		goto skipwhite;
	}

	// handle quoted strings specially
	if( c == '\"' )
	{
		data++;
		while( 1 )
		{
			c = (byte)*data++;
			if( c == '\"' || !c )
			{
				token[len] = 0;
				return data;
			}
			token[len] = c;
			len++;
		}
	}

	// parse single characters
	if( c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',' )
	{
		token[len] = c;
		len++;
		token[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do
	{
		token[len] = c;
		data++;
		len++;
		c = ((byte)*data);

		if( c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',' )
			break;
	} while( c > 32 );
	
	token[len] = 0;

	return data;
}

/*
=============
COM_FileSize

=============
*/
int COM_FileSize( const char *filename )
{
	return FS_FileSize( filename, false );
}

/*
=============
COM_AddAppDirectoryToSearchPath

=============
*/
void COM_AddAppDirectoryToSearchPath( const char *pszBaseDir, const char *appName )
{
	string	dir;

	if( !pszBaseDir || !appName )
	{
		MsgDev( D_ERROR, "COM_AddDirectorySearchPath: bad directory or appname\n" );
		return;
	}

	Q_snprintf( dir, sizeof( dir ), "%s/%s", pszBaseDir, appName );
	FS_AddGameDirectory( dir, FS_GAMEDIR_PATH );
}

/*
===========
COM_ExpandFilename

Finds the file in the search path, copies over the name with the full path name.
This doesn't search in the pak file.
===========
*/
int COM_ExpandFilename( const char *fileName, char *nameOutBuffer, int nameOutBufferSize )
{
	const char	*path;
	char		result[MAX_SYSPATH];

	if( !fileName || !*fileName || !nameOutBuffer || nameOutBufferSize <= 0 )
		return 0;

	// filename examples:
	// media\sierra.avi - D:\Xash3D\valve\media\sierra.avi
	// models\barney.mdl - D:\Xash3D\bshift\models\barney.mdl
	if(( path = FS_GetDiskPath( fileName, false )) != NULL )
	{
		Q_sprintf( result, "%s/%s", host.rootdir, path );		

		// check for enough room
		if( Q_strlen( result ) > nameOutBufferSize )
			return 0;

		Q_strncpy( nameOutBuffer, result, nameOutBufferSize );
		return 1;
	}
	return 0;
}

/*
============
COM_FixSlashes

Changes all '/' characters into '\' characters, in place.
============
*/
void COM_FixSlashes( char *pname )
{
	while( *pname )
	{
		if( *pname == '\\' )
			*pname = '/';
		pname++;
	}
}

/*
=============
COM_MemFgets

=============
*/
char *COM_MemFgets( byte *pMemFile, int fileSize, int *filePos, char *pBuffer, int bufferSize )
{
	int	i, last, stop;

	if( !pMemFile || !pBuffer || !filePos )
		return NULL;

	if( *filePos >= fileSize )
		return NULL;

	i = *filePos;
	last = fileSize;

	// fgets always NULL terminates, so only read bufferSize-1 characters
	if( last - *filePos > ( bufferSize - 1 ))
		last = *filePos + ( bufferSize - 1);

	stop = 0;

	// stop at the next newline (inclusive) or end of buffer
	while( i < last && !stop )
	{
		if( pMemFile[i] == '\n' )
			stop = 1;
		i++;
	}


	// if we actually advanced the pointer, copy it over
	if( i != *filePos )
	{
		// we read in size bytes
		int	size = i - *filePos;

		// copy it out
		Q_memcpy( pBuffer, pMemFile + *filePos, size );
		
		// If the buffer isn't full, terminate (this is always true)
		if( size < bufferSize ) pBuffer[size] = 0;

		// update file pointer
		*filePos = i;
		return pBuffer;
	}

	return NULL;
}

/*
====================
Cache_Check

consistency check
====================
*/
void *Cache_Check( byte *mempool, cache_user_t *c )
{
	if( !c->data )
		return NULL;

	if( !Mem_IsAllocatedExt( mempool, c->data ))
		return NULL;

	return c->data;
}

/*
=============
COM_LoadFileForMe

=============
*/
byte* COM_LoadFileForMe( const char *filename, int *pLength )
{
	string	name;
	byte	*file, *pfile;
	int	iLength;

	if( !filename || !*filename )
	{
		if( pLength ) *pLength = 0;
		return NULL;
	}

	Q_strncpy( name, filename, sizeof( name ));
	COM_FixSlashes( name );

	pfile = FS_LoadFile( name, &iLength, false );
	if( pLength ) *pLength = iLength;

	if( pfile )
	{
		file = malloc( iLength + 1 );
		Q_memcpy( file, pfile, iLength );
		file[iLength] = '\0';
		Mem_Free( pfile );
		pfile = file;
	}

	return pfile;
}

/*
=============
COM_LoadFile

=============
*/
byte *COM_LoadFile( const char *filename, int usehunk, int *pLength )
{
	string	name;
	byte	*file, *pfile;
	int	iLength;

	ASSERT( usehunk == 5 );

	if( !filename || !*filename )
	{
		if( pLength ) *pLength = 0;
		return NULL;
	}

	Q_strncpy( name, filename, sizeof( name ));
	COM_FixSlashes( name );

	pfile = FS_LoadFile( name, &iLength, false );
	if( pLength ) *pLength = iLength;

	if( pfile )
	{
		file = malloc( iLength + 1 );
		Q_memcpy( file, pfile, iLength );
		file[iLength] = '\0';
		Mem_Free( pfile );
		pfile = file;
	}

	return pfile;
}

/*
=============
COM_FreeFile

=============
*/
void COM_FreeFile( void *buffer )
{
	free( buffer ); 
}

/*
=============
pfnGetModelType

=============
*/
int pfnGetModelType( model_t *mod )
{
	if( !mod ) return mod_bad;
	return mod->type;
}

/*
=============
pfnGetModelBounds

=============
*/
void pfnGetModelBounds( model_t *mod, float *mins, float *maxs )
{
	if( mod )
	{
		if( mins ) VectorCopy( mod->mins, mins );
		if( maxs ) VectorCopy( mod->maxs, maxs );
	}
	else
	{
		MsgDev( D_ERROR, "Mod_GetBounds: NULL model\n" );
		if( mins ) VectorClear( mins );
		if( maxs ) VectorClear( maxs );
	}
}
	
/*
=============
pfnCvar_RegisterVariable

=============
*/
cvar_t *pfnCvar_RegisterVariable( const char *szName, const char *szValue, int flags )
{
	return (cvar_t *)Cvar_Get( szName, szValue, flags|CVAR_CLIENTDLL, "client cvar" );
}

/*
=============
pfnCVarGetPointer

can return NULL
=============
*/
cvar_t *pfnCVarGetPointer( const char *szVarName )
{
	cvar_t	*cvPtr;

	cvPtr = (cvar_t *)Cvar_FindVar( szVarName );

	return cvPtr;
}

/*
=============
pfnAddClientCommand

=============
*/
int pfnAddClientCommand( const char *cmd_name, xcommand_t func )
{
	if( !cmd_name || !*cmd_name )
		return 0;

	// NOTE: if( func == NULL ) cmd will be forwarded to a server
	Cmd_AddClientCommand( cmd_name, func );

	return 1;
}

/*
=============
Con_Printf

=============
*/
void Con_Printf( char *szFmt, ... )
{
	static char	buffer[16384];	// must support > 1k messages
	va_list		args;

	if( host.developer <= 0 )
		return;

	va_start( args, szFmt );
	Q_vsnprintf( buffer, 16384, szFmt, args );
	va_end( args );

	Sys_Print( buffer );
}

/*
=============
Con_DPrintf

=============
*/
void Con_DPrintf( char *szFmt, ... )
{
	static char	buffer[16384];	// must support > 1k messages
	va_list		args;

	if( host.developer < D_INFO )
		return;

	va_start( args, szFmt );
	Q_vsnprintf( buffer, 16384, szFmt, args );
	va_end( args );

	Sys_Print( buffer );
}

/*
=============
COM_CompareFileTime

=============
*/
int COM_CompareFileTime( const char *filename1, const char *filename2, int *iCompare )
{
	int	bRet = 0;

	*iCompare = 0;

	if( filename1 && filename2 )
	{
		long ft1 = FS_FileTime( filename1, false );
		long ft2 = FS_FileTime( filename2, false );

		// one of files is missing
		if( ft1 == -1 || ft2 == -1 )
			return bRet;

		*iCompare = Host_CompareFileTime( ft1,  ft2 );
		bRet = 1;
	}
	return bRet;
}

/*
=============
pfnGetGameDir

=============
*/
void pfnGetGameDir( char *szGetGameDir )
{
	if( !szGetGameDir ) return;
	Q_sprintf( szGetGameDir, "%s/%s", host.rootdir, GI->gamedir );
}

/*
=============
pfnSequenceGet

used by CS:CZ
=============
*/
void *pfnSequenceGet( const char *fileName, const char *entryName )
{
	Msg( "Sequence_Get: file %s, entry %s\n", fileName, entryName );
	return NULL;
}

/*
=============
pfnSequencePickSentence

used by CS:CZ
=============
*/
void *pfnSequencePickSentence( const char *groupName, int pickMethod, int *picked )
{
	Msg( "Sequence_PickSentence: group %s, pickMethod %i\n", groupName, pickMethod );
	*picked = 0;

	return NULL;
}

/*
=============
pfnIsCareerMatch

used by CS:CZ (client stub)
=============
*/
int pfnIsCareerMatch( void )
{
	return 0;
}

/*
=============
pfnRegisterTutorMessageShown

only exists in PlayStation version
=============
*/
void pfnRegisterTutorMessageShown( int mid )
{
}

/*
=============
pfnGetTimesTutorMessageShown

only exists in PlayStation version
=============
*/
int pfnGetTimesTutorMessageShown( int mid )
{
	return 0;
}

/*
=============
pfnProcessTutorMessageDecayBuffer

only exists in PlayStation version
=============
*/
void pfnProcessTutorMessageDecayBuffer( int *buffer, int bufferLength )
{
}

/*
=============
pfnConstructTutorMessageDecayBuffer

only exists in PlayStation version
=============
*/
void pfnConstructTutorMessageDecayBuffer( int *buffer, int bufferLength )
{
}

/*
=============
pfnResetTutorMessageDecayData

only exists in PlayStation version
=============
*/
void pfnResetTutorMessageDecayData( void )
{
}