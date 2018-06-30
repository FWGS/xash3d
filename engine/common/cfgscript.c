/*
cfgscript.c - "Valve script" parsing routines
Copyright (C) 2016 mittorn

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

typedef enum
{
	T_NONE = 0,
	T_BOOL,
	T_NUMBER,
	T_LIST,
	T_STRING,
	T_COUNT
} cvartype_t;

char *cvartypes[] = { NULL, "BOOL" , "NUMBER", "LIST", "STRING" };

typedef struct parserstate_s
{
	char *buf;
	char token[MAX_STRING];
	const char *filename;
} parserstate_t;

typedef struct scrvardef_s
{
	int flags;
	char name[MAX_STRING];
	char value[MAX_STRING];
	char desc[MAX_STRING];
	float fMin, fMax;
	cvartype_t type;
	qboolean fHandled;
} scrvardef_t;

/*
===================
CSCR_ExpectString

Return true if next token is pExpext and skip it
===================
*/
qboolean CSCR_ExpectString( parserstate_t *ps, const char *pExpect, qboolean skip, qboolean error )
{
	char *tmp = COM_ParseFile( ps->buf, ps->token );

	if( !Q_stricmp( ps->token, pExpect ) )
	{
		ps->buf = tmp;
		return true;
	}

	if( skip )
		ps->buf = tmp;

	if( error )
		MsgDev( D_ERROR, "Syntax error in %s: got \"%s\" instead of \"%s\"\n", ps->filename, ps->token, pExpect );

	return false;
}

/*
===================
CSCR_ParseType

Determine script variable type
===================
*/
cvartype_t CSCR_ParseType( parserstate_t *ps )
{
	int i;

	for ( i = 1; i < T_COUNT; ++i )
	{
		if( CSCR_ExpectString( ps, cvartypes[i], false, false ) )
			return i;
	}

	MsgDev( D_ERROR, "Cannot parse %s: Bad type %s\n", ps->filename, ps->token );
	return T_NONE;
}



/*
=========================
CSCR_ParseSingleCvar
=========================
*/
qboolean CSCR_ParseSingleCvar( parserstate_t *ps, scrvardef_t *result )
{
	// read the name
	ps->buf = COM_ParseFile( ps->buf, result->name );

	if( !CSCR_ExpectString( ps, "{", false, true ) )
		return false;

	// read description
	ps->buf = COM_ParseFile( ps->buf, result->desc );

	if( !CSCR_ExpectString( ps, "{", false, true ) )
		return false;

	result->type = CSCR_ParseType( ps );

	switch( result->type )
	{
	case T_BOOL:
		// bool only has description
		if( !CSCR_ExpectString( ps, "}", false, true ) )
			return false;
		break;
	case T_NUMBER:
		// min
		ps->buf = COM_ParseFile( ps->buf, ps->token );
		result->fMin = Q_atof( ps->token );

		// max
		ps->buf = COM_ParseFile( ps->buf, ps->token );
		result->fMax = Q_atof( ps->token );

		if( !CSCR_ExpectString( ps, "}", false, true ) )
			return false;
		break;
	case T_STRING:
		if( !CSCR_ExpectString( ps, "}", false, true ) )
			return false;
		break;
	case T_LIST:
		while( !CSCR_ExpectString( ps, "}", true, false ) )
		{
			// Read token for each item here
		}
		break;
	default:
		return false;
	}

	if( !CSCR_ExpectString( ps, "{", false, true ) )
		return false;

	// default value
	ps->buf = COM_ParseFile( ps->buf, result->value );

	if( !CSCR_ExpectString( ps, "}", false, true ) )
		return false;

	if( CSCR_ExpectString( ps, "SetInfo", false, false ) )
		result->flags |= CVAR_USERINFO;

	if( !CSCR_ExpectString( ps, "}", false, true ) )
		return false;

	return true;
}

/*
======================
CSCR_ParseHeader

Check version and seek to first cvar name
======================
*/
qboolean CSCR_ParseHeader( parserstate_t *ps )
{
	if( !CSCR_ExpectString( ps, "VERSION", false, true ) )
		return false;

	// Parse in the version #
	// Get the first token.
	ps->buf = COM_ParseFile( ps->buf, ps->token );

	if( Q_atof( ps->token ) != 1 )
	{
		MsgDev( D_ERROR, "File %s has wrong version %s!\n", ps->filename, ps->token );
		return false;
	}

	if( !CSCR_ExpectString( ps, "DESCRIPTION", false, true ) )
		return false;

	ps->buf = COM_ParseFile( ps->buf, ps->token );

	if( Q_stricmp( ps->token, "INFO_OPTIONS") && Q_stricmp( ps->token, "SERVER_OPTIONS" ) )
	{
		MsgDev( D_ERROR, "DESCRIPTION must be INFO_OPTIONS or SERVER_OPTIONS\n");
		return false;
	}

	if( !CSCR_ExpectString( ps, "{", false, true ) )
		return false;

	return true;
}

/*
======================
CSCR_WriteGameCVars

Print all cvars declared in script to gamesettings.cfg file
======================
*/
int CSCR_WriteGameCVars( file_t *cfg, const char *scriptfilename )
{
	int count = 0;
	fs_offset_t length = 0;
	char *start;
	parserstate_t state = {0};
	qboolean success = false;

	state.filename = scriptfilename;

	state.buf = (char*)FS_LoadFile( scriptfilename, &length, true );

	start = state.buf;

	if( state.buf == 0 || length == 0 )
	{
		if( start )
			Mem_Free( start );
		return 0;
	}
	MsgDev( D_INFO, "Reading config script file %s\n", scriptfilename );

	if( !CSCR_ParseHeader( &state ) )
	{
		MsgDev( D_ERROR, "Failed to	parse header!\n" );
		goto finish;
	}

	FS_Printf( cfg, "// declared in %s:\n", scriptfilename );

	while( !CSCR_ExpectString( &state, "}", false, false ) )
	{
		scrvardef_t var = { 0 };

		if( CSCR_ParseSingleCvar( &state, &var ) )
		{
			convar_t *cvar = Cvar_FindVar( var.name );
			if( cvar && !( cvar->flags & ( CVAR_SERVERNOTIFY | CVAR_ARCHIVE ) ) )
			{
				// cvars will be placed in gamesettings.cfg and restored on map start
				if( var.flags & CVAR_USERINFO )
					FS_Printf( cfg, "// %s ( %s )\nsetu %s \"%s\"\n", var.desc, var.value, var.name, cvar->string );
				else
					FS_Printf( cfg, "// %s ( %s )\nset %s \"%s\"\n", var.desc, var.value, var.name, cvar->string );
			}
			count++;
		}
		else
			break;

		if( count > 1024 )
			break;
	}

	if( COM_ParseFile( state.buf, state.token ) )
		MsgDev( D_ERROR, "Got extra tokens!\n" );
	else
		success = true;

finish:
	if( !success )
	{
		state.token[ sizeof( state.token ) - 1 ] = 0;
		if( start && state.buf )
			MsgDev( D_ERROR, "Parse error in %s, byte %d, token %s\n", scriptfilename, (int)( state.buf - start ), state.token );
		else
			MsgDev( D_ERROR, "Parse error in %s, token %s\n", scriptfilename, state.token );
	}
	if( start )
		Mem_Free( start );

	return count;
}

/*
======================
CSCR_LoadDefaultCVars

Register all cvars declared in config file and set default values
======================
*/
int CSCR_LoadDefaultCVars( const char *scriptfilename )
{
	int count = 0;
	fs_offset_t length = 0;
	char *start;
	parserstate_t state = {0};
	qboolean success = false;


	state.filename = scriptfilename;

	state.buf = (char*)FS_LoadFile( scriptfilename, &length, true );

	start = state.buf;

	if( state.buf == 0 || length == 0)
	{
		if( start )
			Mem_Free( start );
		return 0;
	}

	MsgDev( D_INFO, "Reading config script file %s\n", scriptfilename );

	if( !CSCR_ParseHeader( &state ) )
	{
		MsgDev( D_ERROR, "Failed to	parse header!\n" );
		goto finish;
	}

	while( !CSCR_ExpectString( &state, "}", false, false ) )
	{
		scrvardef_t var = { 0 };

		// Create a new object
		if( CSCR_ParseSingleCvar( &state, &var ) )
		{
			Cvar_Get( var.name, var.value, var.flags, var.desc );
			count++;
		}
		else
			break;

		if( count > 1024 )
			break;
	}

	if( COM_ParseFile( state.buf, state.token ) )
		MsgDev( D_ERROR, "Got extra tokens!\n" );
	else
		success = true;

finish:
	if( !success )
	{
		state.token[ sizeof( state.token ) - 1 ] = 0;
		if( start && state.buf )
			MsgDev( D_ERROR, "Parse error in %s, byte %d, token %s\n", scriptfilename, (int)( state.buf - start ), state.token );
		else
			MsgDev( D_ERROR, "Parse error in %s, token %s\n", scriptfilename, state.token );
	}
	if( start )
		Mem_Free( start );

	return count;
}
