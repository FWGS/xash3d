/*
cmd.c - script command processing module
Copyright (C) 2007 Uncle Mike

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
#include "client.h"
#include "server.h"

#define MAX_CMD_BUFFER	32768
#define MAX_CMD_LINE	1024

typedef struct
{
	byte	*data;
	int	cursize;
	int	maxsize;
} cmdbuf_t;

typedef struct cmd_s
{
	char		 *name; // must be first, to match cvar_t
	struct cmd_s *next;
	xcommand_t	 function;
	char		 *desc;
	int          flags;
} cmd_t;
static cmd_t		*cmd_functions;			// possible commands to execute
static qboolean		cmd_wait;
static cmdbuf_t		cmd_text;
static byte		cmd_text_buf[MAX_CMD_BUFFER];
static cmdalias_t	*cmd_alias;
static int			maxcmdnamelen; // this is used to nicely format command list output
extern convar_t		*cvar_vars;
extern convar_t *cmd_scripting;

// condition checking
uint64_t cmd_cond;
int cmd_condlevel;

/*
=============================================================================

			COMMAND BUFFER

=============================================================================
*/
/*
============
Cbuf_Init
============
*/
void Cbuf_Init( void )
{
	cmd_text.data = cmd_text_buf;
	cmd_text.maxsize = MAX_CMD_BUFFER;
	cmd_text.cursize = 0;
	Q_memset( cmd_text_buf, 0, sizeof( cmd_text_buf ) );
}

/*
============
Cbuf_Clear
============
*/
void Cbuf_Clear( void )
{
	Q_memset( cmd_text.data, 0, sizeof( cmd_text_buf ));
	cmd_text.cursize = 0;
}

/*
============
Cbuf_GetSpace
============
*/
static void *Cbuf_GetSpace( cmdbuf_t *buf, int length )
{
	void    *data;
	
	if( buf->cursize + length > buf->maxsize )
	{
		buf->cursize = 0;
		Host_Error( "Cbuf_GetSpace: overflow\n" );
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;
	
	return data;
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText( const char *text )
{
	int	l;

	l = Q_strlen( text );

	if( cmd_text.cursize + l >= cmd_text.maxsize )
	{
		MsgDev( D_WARN, "Cbuf_AddText: overflow\n" );
	}
	else
	{
		Q_memcpy( Cbuf_GetSpace( &cmd_text, l ), text, l );
	}
}

/*
=================
Cbuf_AddFilterText

Remove restricted commands from buffer and just add other
=================
*/
void Cbuf_AddFilterText( const char *text )
{
	static int aliasDepth = 0;
	qboolean quotes = false;
	char	line[MAX_CMD_LINE] = { 0 };
	int i = 0;

	ASSERT( text );

	if( aliasDepth >= 3 )
	{
		MsgDev( D_NOTE, "AddFilterText(alias, recursion)\n" );
		return;
	}

	while( *text ) // stufftext should end by newline
	{
		if( i >= MAX_CMD_LINE - 2 )
		{
			MsgDev( D_ERROR, "Cbuf_AddFilterText: overflow!\n" );
			return;
		}

		if( *text == '"' )
		{
			quotes = !quotes;
		}

		if( ( *text == ';' || *text == '\n' ) && !quotes )
		{
			// have anything in the buffer
			if( line[0] )
			{
				int arg = 0;
				cmd_t	*cmd;
				cmdalias_t	*a;
				convar_t *cvar;
				
				line[i] = '\n';
				line[++i] = '\0';

				Cmd_TokenizeString( line );

				// prevent changing from "set" commands family
				if( !Q_strnicmp( Cmd_Argv( 0 ), "set", 3 ) )
					arg = 1;

				// find commands
#if defined(XASH_HASHED_VARS)
				BaseCmd_FindAll( Cmd_Argv( arg ),
					( base_command_t** ) &cmd,
					( base_command_t** ) &a,
					( base_command_t** ) &cvar );
#else
				for( a = cmd_alias; a; a = a->next )
				{
					if( !Q_stricmp( Cmd_Argv( arg ), a->name ) )
						break;
				}

				for( cmd = cmd_functions; cmd; cmd = cmd->next )
				{
					if( !Q_stricmp( Cmd_Argv( arg ), cmd->name ) && cmd->function )
						break;
				}

				cvar = Cvar_FindVar( Cmd_Argv( arg ) );
#endif

				if( a )
				{
					// recursively expose and validate an alias
					MsgDev( D_NOTE, "AddFilterText(alias): %s => %s", a->name, a->value );
					aliasDepth++;
					Cbuf_AddFilterText( a->value );
					aliasDepth--;
				}
				else if( cmd )
				{
					if( !( cmd->flags & CMD_LOCALONLY ) )
					{
						MsgDev( D_NOTE, "AddFilterText(cmd, allowed): %s", line );
						Cbuf_AddText( line );
					}
					else
					{
						MsgDev( D_NOTE, "AddFilterText(cmd, restricted): %s", line );
					}
				}
				else if( cvar )
				{
					if( !( cvar->flags & CVAR_LOCALONLY ) )
					{
						MsgDev( D_NOTE, "AddFilterText(cvar, allowed): %s", line );
						Cbuf_AddText( line );
					}
					else
					{
						MsgDev( D_NOTE, "AddFilterText(cvar, restricted): %s", line );
					}
				}
				else
				{
					// add server forwards
					MsgDev( D_NOTE, "AddFilterText(forwards, allowed): %s", line );
					Cbuf_AddText( line );
				}
			}
						
			// clear buffers
			i = 0;
			line[0] = 0;
		}
		else
		{
			line[i] = *text;
			i++;
		}

		text++;
	}

	// cmd_text.haveServerCmds = true;
}

/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
============
*/
void Cbuf_InsertText( const char *text )
{
	int l;

	l = Q_strlen( text );

	if( cmd_text.cursize + l >= cmd_text.maxsize )
	{
		MsgDev( D_WARN, "Cbuf_InsertText: overflow\n" );
	}
	else
	{
		Q_memmove( cmd_text.data + l, cmd_text.data, cmd_text.cursize );
		cmd_text.cursize += l;
		Q_memcpy( cmd_text.data, text, l );
	}
}

/*
============
Cbuf_Execute
============
*/
void GAME_EXPORT Cbuf_Execute( void )
{
	int		i;
	char	*text;
	char	line[MAX_CMD_LINE];
	qboolean quotes;
	char	*comment;

	while( cmd_text.cursize )
	{
		// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = false;
		comment = NULL;

		for( i = 0; i < cmd_text.cursize; i++ )
		{
			if( !comment )
			{
				if( text[i] == '"' )
					quotes = !quotes;

				if( quotes )
				{
					// make sure i doesn't get > cursize which causes a negative
					// size in memmove, which is fatal --blub
					if ( i < ( cmd_text.cursize-1 ) && ( text[i] == '\\' && (text[i+1] == '"' || text[i+1] == '\\' )))
						i++;
				}
				else
				{
					if( text[i] == '/' && text[i + 1] == '/' && (i == 0 || (byte)text[i - 1] <= ' ' ))
						comment = &text[i];
					if( text[i] == ';' )
						break;	// don't break if inside a quoted string or comment
				}
			}
			if( text[i] == '\r' || text[i] == '\n' )
				break;
		}

		// better than CRASHING on overlong input lines that may SOMEHOW enter the buffer
		if( i >= MAX_CMD_LINE )
		{
			MsgDev( D_WARN, "Console input buffer had an overlong line. Ignored.\n" );
			line[0] = 0;
		}
		else
		{
			Q_memcpy( line, text, comment ? (comment - text) : i );
			line[comment ? (comment - text) : i] = 0;
		}

		// delete the text from the command buffer and move remaining commands down
		// this is necessary because commands (exec) can insert data at the
		// beginning of the text buffer
		if( i == cmd_text.cursize )
		{
			cmd_text.cursize = 0;
		}
		else
		{
			i++;
			cmd_text.cursize -= i;
			Q_memmove( cmd_text.data, text + i, cmd_text.cursize );
		}

		// execute the command line
		// Cmd_ExecuteString( line, cmd_text.haveServerCmds ? src_server : src_command );
		Cmd_ExecuteString( line, src_command );

		if( cmd_wait )
		{
			// skip out while text still remains in buffer,
			// leaving it for next frame
			cmd_wait = false;
			break;
		}
		else
		{
			// completely flushed command buffer, so remove server commands flags
			// cmd_text.haveServerCmds = false;
		}
	}
}

/*
==============================================================================

			SCRIPT COMMANDS

==============================================================================
*/
/*
===============
Cmd_StuffCmds_f

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
xash -dev 3 +map c1a0d
xash -nosound -game bshift
===============
*/
static void Cmd_StuffCmds_f( void )
{
	int	i, j, l = 0;
	char	build[MAX_CMD_LINE]; // this is for all commandline options combined (and is bounds checked)

	if( Cmd_Argc() != 1 )
	{
		Msg( "Usage: stuffcmds : execute command line parameters\n" );
		return;
	}

	// no reason to run the commandline arguments twice
	if( host.stuffcmdsrun ) return;

	host.stuffcmdsrun = true;
	build[0] = 0;

	for( i = 0; i < host.argc; i++ )
	{
		if( host.argv[i] && host.argv[i][0] == '+' && ( host.argv[i][1] < '0' || host.argv[i][1] > '9' ) && l + Q_strlen( host.argv[i] ) - 1 <= sizeof( build ) - 1 )
		{
			j = 1;

			while( host.argv[i][j] )
				build[l++] = host.argv[i][j++];

			for( i++; i < host.argc; i++ )
			{
				if( !host.argv[i] ) continue;
				if(( host.argv[i][0] == '+' || host.argv[i][0] == '-' ) && ( host.argv[i][1] < '0' || host.argv[i][1] > '9' ))
					break;
				if( l + Q_strlen( host.argv[i] ) + 4 > sizeof( build ) - 1 )
					break;
				build[l++] = ' ';
	
				if( Q_strchr( host.argv[i], ' ' ))
					build[l++] = '\"';
	
				for( j = 0; host.argv[i][j]; j++ )
					build[l++] = host.argv[i][j];
	
				if( Q_strchr( host.argv[i], ' ' ))
					build[l++] = '\"';
			}
			build[l++] = '\n';
			i--;
		}
	}

	// now terminate the combined string and prepend it to the command buffer
	// we already reserved space for the terminator
	build[l++] = 0;
	Cbuf_InsertText( build );
}

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "cmd use rocket ; +attack ; wait ; -attack ; cmd use blaster"
============
*/
static void Cmd_Wait_f( void )
{
	cmd_wait = true;
}

/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
static void Cmd_Echo_f( void )
{
	int	i;

	for( i = 1; i < Cmd_Argc(); i++ )
		Msg( "%s ", Cmd_Argv( i ));
	Sys_Print( "\n" );
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
static void Cmd_Alias_f( void )
{
	cmdalias_t	*a;
	char		cmd[MAX_CMD_LINE];
	int			i, c;
	const char	*s;
	size_t		alloclen;

	if( Cmd_Argc() == 1 )
	{
		Msg( "Current alias commands:\n" );
		for( a = cmd_alias; a; a = a->next )
			Msg( "^2%s^7 : ^3%s^7\n", a->name, a->value );
		return;
	}

	s = Cmd_Argv( 1 );

	if( Q_strlen( s ) >= MAX_ALIAS_NAME )
	{
		Msg( "Alias name is too long\n" );
		return;
	}

	// if the alias already exists, reuse it
	// we don't need to search it in BaseCmd too,
	// as BaseCmd just need for fast searching during exec
	for( a = cmd_alias; a; a = a->next )
	{
		if( !Q_strcmp( s, a->name ))
		{
			Mem_Free( a->value );
			break;
		}
	}

	if( !a )
	{
		cmdalias_t *prev, *current;

		a = Z_Malloc( sizeof( cmdalias_t ));
		Q_strncpy( a->name, s, sizeof( a->name ));
		// insert it at the right alphanumeric position
		for( prev = NULL, current = cmd_alias ; current && Q_strcmp( current->name, a->name ) < 0 ; prev = current, current = current->next )
			;
		if( prev ) {
			prev->next = a;
		} else {
			cmd_alias = a;
		}
		a->next = current;

#if defined( XASH_HASHED_VARS )
		BaseCmd_Insert( HM_CMDALIAS, a, a->name );
#endif
	}

	// copy the rest of the command line
	cmd[0] = 0; // start out with a null string

	c = Cmd_Argc();

	for( i = 2; i < c; i++ )
	{
		if( i != 2 )
			Q_strncat( cmd, " ", sizeof( cmd ));
		Q_strncat( cmd, Cmd_Argv( i ), sizeof( cmd ));
	}
	Q_strncat( cmd, "\n", sizeof( cmd ));

	alloclen = Q_strlen( cmd ) + 1;
	if( alloclen >= 2 )
		cmd[alloclen - 2] = '\n'; // to make sure a newline is appended even if too long
	a->value = Z_Malloc( alloclen );
	Q_memcpy( a->value, cmd, alloclen );
}

/*
===============
Cmd_UnAlias_f

Remove existing aliases.
===============
*/
static void Cmd_UnAlias_f ( void )
{
	cmdalias_t *a, *p;
	int i;
	const char *s;

	if( Cmd_Argc() == 1 )
	{
		Msg( "Usage: unalias alias1 [alias2 ...]\n" );
		return;
	}

	for( i = 1; i < Cmd_Argc(); ++i )
	{
		s = Cmd_Argv( i );
		for( p = NULL, a = cmd_alias; a; p = a, a = a->next )
		{
			if( !Q_strcmp( s, a->name ))
			{
#if defined( XASH_HASHED_VARS )
				BaseCmd_Remove( HM_CMDALIAS, a->name );
#endif
				if( a == cmd_alias )
					cmd_alias = a->next;
				if( p )
					p->next = a->next;
				Mem_Free( a->value );
				Mem_Free( a );
				break;
			}
		}
		if( !a )
			Msg( "unalias: %s alias not found\n", s );
	}
}

/*
=============================================================================

			COMMAND EXECUTION

=============================================================================
*/

static int		cmd_argc;
static char		*cmd_args;
static char		*cmd_argv[MAX_CMD_TOKENS];
//static char		cmd_tokenized[MAX_CMD_BUFFER];	// will have 0 bytes inserted
cmd_source_t		cmd_source;

/*
============
Cmd_Argc
============
*/
int GAME_EXPORT Cmd_Argc( void )
{
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char *GAME_EXPORT Cmd_Argv( int arg )
{
	if( arg >= cmd_argc )
		return "";
	return cmd_argv[arg];	
}

/*
============
Cmd_Args
============
*/
char *GAME_EXPORT Cmd_Args( void )
{
	return cmd_args;
}

/*
============
Cmd_AliasGetList
============
*/
cmdalias_t *GAME_EXPORT Cmd_AliasGetList( void )
{
	return cmd_alias;
}

/*
============
Cmd_GetList
============
*/
cmd_t *GAME_EXPORT Cmd_GetFirstFunctionHandle( void )
{
	return cmd_functions;
}

/*
============
Cmd_GetNext
============
*/
cmd_t *GAME_EXPORT Cmd_GetNextFunctionHandle( cmd_t *cmd )
{
	return (cmd) ? cmd->next : NULL;
}

/*
============
Cmd_GetName
============
*/
char *GAME_EXPORT Cmd_GetName( cmd_t *cmd )
{
	return cmd->name;
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
The text is copied to a seperate buffer and 0 characters
are inserted in the apropriate place, The argv array
will point into this temporary buffer.
============
*/
void Cmd_TokenizeString( const char *text )
{
	char	cmd_token[MAX_CMD_BUFFER];
	int	i;

	// clear the args from the last string
	for( i = 0; i < cmd_argc; i++ )
		Z_Free( cmd_argv[i] );

	cmd_argc = 0; // clear previous args
	cmd_args = NULL;

	if( !text ) return;

	while( 1 )
	{
		// skip whitespace up to a /n
		while( *text && ((byte)*text) <= ' ' && *text != '\r' && *text != '\n' )
			text++;
		
		if( *text == '\n' || *text == '\r' )
		{	
			// a newline seperates commands in the buffer
			if( *text == '\r' && text[1] == '\n' )
				text++;
			text++;
			break;
		}

		if( !*text )
			return;
	
		if( cmd_argc == 1 )
			 cmd_args = (char*)text;
		text = COM_ParseFile( (char*)text, cmd_token );
		if( !text ) return;

		if( cmd_argc < MAX_CMD_TOKENS )
		{
			cmd_argv[cmd_argc] = copystring( cmd_token );
			cmd_argc++;
		}
	}
}

/*
============
Cmd_AddCommandEx
============
*/
static void Cmd_AddCommandEx( const char *funcname, const char *cmd_name, xcommand_t function, 
	const char *cmd_desc, int iFlags )
{
	cmd_t	*cmd;
	cmd_t	*prev, *current;
	int		cmdnamelen;

	// fail if the command is a variable name
	if( Cvar_FindVar( cmd_name ) )
	{
		MsgDev( D_INFO, "%s: %s already defined as a var\n", funcname, cmd_name );
		return;
	}

	// fail if the command already exists
	if( Cmd_Exists( cmd_name ) )
	{
		MsgDev( D_INFO, "%s: %s already defined\n", funcname, cmd_name );
		return;
	}

	cmdnamelen = Q_strlen( cmd_name );
	if( cmdnamelen > maxcmdnamelen )
		maxcmdnamelen = cmdnamelen;

	// use a small malloc to avoid zone fragmentation
	cmd = Z_Malloc( sizeof( cmd_t ) );
	cmd->name = copystring( cmd_name );
	cmd->desc = copystring( cmd_desc );
	cmd->function = function;
	cmd->flags = iFlags;
	cmd->next = cmd_functions;

	// insert it at the right alphanumeric position
	for( prev = NULL, current = cmd_functions; current && Q_strcmp( current->name, cmd_name ) < 0; prev = current, current = current->next )
		;
	if( prev ) {
		prev->next = cmd;
	}
	else {
		cmd_functions = cmd;
	}
	cmd->next = current;
#if defined(XASH_HASHED_VARS)
	BaseCmd_Insert( HM_CMD, cmd, cmd->name );
#endif
}

/*
============
Cmd_AddCommand
============
*/
void Cmd_AddCommand( const char *cmd_name, xcommand_t function, const char *cmd_desc )
{
	Cmd_AddCommandEx( "Cmd_AddCommand", cmd_name, function, cmd_desc, 0 );
}

/*
============
Cmd_AddRestrictedCommand
============
*/
void Cmd_AddRestrictedCommand( const char *cmd_name, xcommand_t function, const char *cmd_desc )
{
	Cmd_AddCommandEx( "Cmd_AddRestrictedCommand", cmd_name, function, cmd_desc, CMD_LOCALONLY );
}


/*
============
Cmd_AddGameCommand
============
*/
void GAME_EXPORT Cmd_AddGameCommand( const char *cmd_name, xcommand_t function )
{
	Cmd_AddCommandEx( "Cmd_AddGameCommand", cmd_name, function, "game command", CMD_EXTDLL );
}

/*
============
Cmd_AddClientCommand
============
*/
void GAME_EXPORT Cmd_AddClientCommand( const char *cmd_name, xcommand_t function )
{
	Cmd_AddCommandEx( "Cmd_AddClientCommand", cmd_name, function, "client command", CMD_CLIENTDLL );
}

/*
============
Cmd_RemoveCommand
============
*/
void Cmd_RemoveCommand( const char *cmd_name )
{
	cmd_t	*cmd, **prev;

	for( prev = &cmd_functions; ( cmd = *prev ); prev = &cmd->next )
	{
		if( !Q_strcmp( cmd_name, cmd->name ))
		{
#if defined(XASH_HASHED_VARS)
			BaseCmd_Remove( HM_CMD, cmd->name );
#endif

			*prev = cmd->next;

			Mem_Free( cmd->name );
			Mem_Free( cmd->desc );
			Mem_Free( cmd );

			return;
		}
	}
}

/*
============
Cmd_LookupCmds
============
*/
void Cmd_LookupCmds( char *buffer, void *ptr, setpair_t callback )
{
	cmd_t	*cmd;
	cmdalias_t	*alias;

	// nothing to process ?
	if( !callback ) return;
	
	for( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if( !buffer ) callback( cmd->name, (char *)cmd->function, cmd->desc, ptr );
		else callback( cmd->name, (char *)cmd->function, buffer, ptr );
	}

	// lookup an aliases too
	for( alias = cmd_alias; alias; alias = alias->next )
		callback( alias->name, alias->value, buffer, ptr );
}

/*
============
Cmd_Exists
============
*/
qboolean Cmd_Exists( const char *cmd_name )
{
#if defined(XASH_HASHED_VARS)
	return BaseCmd_Find( HM_CMD, cmd_name ) != NULL;
#else
	cmd_t	*cmd;

	for( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if( !Q_strcmp( cmd_name, cmd->name ))
			return true;
	}
	return false;
#endif
}

/*
============
Cmd_If_f

Compare and et condition bit if true
============
*/
#define BIT64(x) ( (uint64_t)1 << x )
void Cmd_If_f( void )
{
	// reset bit first
	cmd_cond &= ~BIT64( cmd_condlevel );

	// usage
	if( cmd_argc == 1 )
	{
		Msg("Usage: if <op1> [ <operator> <op2> ]\n");
		Msg(":<action1>\n" );
		Msg(":<action2>\n" );
		Msg("else\n" );
		Msg(":<action3>\n" );
		Msg("operands are string or float values\n" );
		Msg("and substituted cvars like '$cl_lw'\n" );
		Msg("operator is '='', '==', '>', '<', '>=', '<=' or '!='\n" );
	}
	// one argument - check if nonzero
	else if( cmd_argc == 2 )
	{
		if( Q_atof( cmd_argv[1] ) )
			cmd_cond |= BIT64( cmd_condlevel );
	}
	else if( cmd_argc == 4 )
	{
		// simple compare
		float f1 = Q_atof( cmd_argv[1] );
		float f2 = Q_atof( cmd_argv[3] );

		if( !cmd_argv[2][0] ) // this is wrong
			return;

		if( ( cmd_argv[2][0] == '=' ) ||
			( cmd_argv[2][1] == '=' )  )				// =, ==, >=, <=
		{
			if( !Q_strcmp( cmd_argv[1], cmd_argv[3] ) || 
				( ( f1 || f2 ) && ( f1 == f2 ) ) )
				cmd_cond |= BIT64( cmd_condlevel );
		}

		if( cmd_argv[2][0] == '!' ) 					// !=
		{
			cmd_cond ^= BIT64( cmd_condlevel );
			return;
		}

		if( ( cmd_argv[2][0] == '>' ) && ( f1 > f2 ) )	// >, >=
			cmd_cond |= BIT64( cmd_condlevel );
		
		if( ( cmd_argv[2][0] == '<' ) && ( f1 < f2 ) )	// <, <=
			cmd_cond |= BIT64( cmd_condlevel );
	}
}


/*
============
Cmd_Else_f

Invert condition bit
============
*/
void Cmd_Else_f( void )
{
	cmd_cond ^= BIT64( cmd_condlevel );
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
============
*/
void Cmd_ExecuteString( const char *text, cmd_source_t src )
{	
	cmd_t	*cmd;
	cmdalias_t	*a;
	convar_t *cvar;
	char command[MAX_CMD_LINE], *pcmd = command;
	int len = 0;;

	// set cmd source
	cmd_source = src;
	cmd_condlevel = 0;

	// cvar value substitution
	if( cmd_scripting && cmd_scripting->integer )
	{
		while( *text )
		{
			// check for escape
			if( ( *text == '\\' || *text == '$') && ( *(text + 1) == '$' ))
			{
				text ++;
			}
			else if( *text == '$' )
			{
				char token[MAX_CMD_LINE], *ptoken = token;

				// check for correct cvar name
				text++;
				while( ( *text >= '0' && *text <= '9' ) ||
					   ( *text >= 'A' && *text <= 'Z' ) ||
					   ( *text >= 'a' && *text <= 'z' ) ||
					   (*text == '_' ) )
					*ptoken++ = *text++;
				*ptoken = 0;

				len += Q_strncpy( pcmd, Cvar_VariableString( token ), MAX_CMD_LINE - len );
				pcmd = command + len;

				if( !*text )
					break;
			}
			*pcmd++ = *text++;
			len++;
		}
		*pcmd = 0;
		text = command;

		while( *text == ':' )
		{
			if( !( cmd_cond & BIT64( cmd_condlevel ) ) )
				return;
			cmd_condlevel++;
			text++;
		}
	}
	
	// execute the command line
	Cmd_TokenizeString( text );

	if( !Cmd_Argc()) return; // no tokens

#if defined(XASH_HASHED_VARS)
	BaseCmd_FindAll( cmd_argv[0],
		(base_command_t**)&cmd,
		(base_command_t**)&a,
		(base_command_t**)&cvar );

	if( a )
	{
		Cbuf_InsertText( a->value );
		return;
	}
#else
	for( a = cmd_alias; a; a = a->next )
	{
		if( !Q_stricmp( cmd_argv[0], a->name ))
		{
			Cbuf_InsertText( a->value );
			return;
		}
	}
#endif

	// check functions
#if defined(XASH_HASHED_VARS)
	if( cmd && cmd->function )
	{
		cmd->function();
		return;
	}
#else
	for( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if( !Q_stricmp( cmd_argv[0], cmd->name ) && cmd->function )
		{
			cmd->function();
			return;
		}
	}
#endif

	// check cvars
#if defined(XASH_HASHED_VARS)
	if( Cvar_Command( cvar )) return;
#else
	if( Cvar_Command( NULL )) return;
#endif

#ifndef XASH_DEDICATED
	// forward the command line to the server, so the entity DLL can parse it
	if( cmd_source != src_client && !Host_IsDedicated() )
	{
		if( cls.state >= ca_connected )
			Cmd_ForwardToServer();
	}
#endif
}

/*
===================
Cmd_ForwardToServer

adds the current command line as a clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer( void )
{
	char	str[MAX_CMD_BUFFER];

#ifndef XASH_DEDICATED
	if( cls.demoplayback )
	{
		if( !Q_stricmp( Cmd_Argv( 0 ), "pause" ))
			cl.refdef.paused ^= 1;
		return;
	}

	if( cls.state != ca_connected && cls.state != ca_active )
	{
		MsgDev( D_INFO, "Can't \"%s\", not connected\n", Cmd_Argv( 0 ));
		return; // not connected
	}

	BF_WriteByte( &cls.netchan.message, clc_stringcmd );

	str[0] = 0;
	if( Q_stricmp( Cmd_Argv( 0 ), "cmd" ))
	{
		Q_strcat( str, Cmd_Argv( 0 ));
		Q_strcat( str, " " );
	}
	
	if( Cmd_Argc() > 1 )
		Q_strcat( str, Cmd_Args( ));
	else Q_strcat( str, "\n" );

	BF_WriteString( &cls.netchan.message, str );
#endif
}

/*
============
Cmd_List_f
============
*/
void Cmd_List_f( void )
{
	cmd_t		*cmd;
	const char	*partial;
	size_t		len;
	int		i = 0, j = 0;
	qboolean	ispattern;

	if( Cmd_Argc() > 1 )
	{
		partial = Cmd_Argv( 1 );
		len = Q_strlen( partial );
		ispattern = ( Q_strchr( partial, '*' ) || Q_strchr( partial, '?' ));
	}
	else
	{
		partial = NULL;
		len = 0;
		ispattern = false;
	}

	for( cmd = cmd_functions; cmd; cmd = cmd->next, i++ )
	{
		if( cmd->name[0] == '@' )
			continue;	// never show system cmds

		if( partial && ( ispattern ? !matchpattern_with_separator( cmd->name, partial, false, "", false ) : Q_strncmp( partial, cmd->name, len )))
			continue;

		// doesn't look exactly as anticipated, but still better
		Msg( "%-*s %s\n", maxcmdnamelen, cmd->name, cmd->desc );
		j++;
	}

	if( len )
	{
		if( ispattern )
			Msg( "\n%i command%s matching \"%s\"\n", j, ( j > 1 ) ? "s" : "", partial );
		else
			Msg( "\n%i command%s beginning with \"%s\"\n", j, ( j > 1 ) ? "s" : "", partial );
	}
	else
	{
		Msg( "\n%i command%s\n", j, ( j > 1 ) ? "s" : "" );
	}

	Msg( "%i total commands\n", i );
}

static void Cmd_Apropos_f( void )
{
	cmd_t *cmd;
	convar_t *var;
	cmdalias_t *alias;
	const char *partial;
	int count = 0;
	qboolean ispattern;

	if( Cmd_Argc() > 1 )
		partial = Cmd_Args();
	else
	{
		Msg( "usage: apropos <string>\n" );
		return;
	}

	ispattern = partial && ( Q_strchr( partial, '*' ) || Q_strchr( partial, '?' ));
	if( !ispattern )
		partial = va( "*%s*", partial );

	for( var = cvar_vars; var; var = var->next )
	{
		if( var->name[0] == '@' )
			continue;	// never shows system cvars

		if( !matchpattern_with_separator( var->name, partial, true, "", false ) )
		{
			char *desc;

			if( var->flags & CVAR_EXTDLL )
				desc = "game cvar";
			else desc = var->description;

			if( !desc )
				desc = "user cvar";

			if( !matchpattern_with_separator( desc, partial, true, "", false ))
				continue;
		}
		
		// TODO: maybe add flags output like cvarlist, also
		// fix inconsistencies in output from different commands
		Msg( "cvar ^3%s^7 is \"%s\" [\"%s\"] %s\n", var->name, var->string, ( var->flags & CVAR_EXTDLL ) ? "" : var->reset_string, ( var->flags & CVAR_EXTDLL ) ? "game cvar" : var->description );
		count++;
	}

	for( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if( cmd->name[0] == '@' )
			continue;	// never show system cmds

		if( !matchpattern_with_separator( cmd->name, partial, true, "", false ))
		if( !matchpattern_with_separator( cmd->desc, partial, true, "", false ))
			continue;

		Msg( "command ^2%s^7: %s\n", cmd->name, cmd->desc );
		count++;
	}

	for( alias = cmd_alias; alias; alias = alias->next )
	{
		// proceed a bit differently here as an alias value always got a final \n
		if( !matchpattern_with_separator( alias->name, partial, true, "", false ))
		if( !matchpattern_with_separator( alias->value, partial, true, "\n", false )) // when \n is a separator, wildcards don't match it //-V666
			continue;

		Msg( "alias ^5%s^7: %s", alias->name, alias->value ); // do not print an extra \n
		count++;
	}

	Msg( "\n%i result%s\n\n", count, (count > 1) ? "s" : "" );
}

/*
============
Cmd_Unlink

unlink all commands with flag CVAR_EXTDLL
============
*/
void Cmd_Unlink( int group )
{
	cmd_t	*cmd;
	cmd_t	**prev;

	if( Cvar_VariableInteger( "host_gameloaded" ) && ( group & CMD_EXTDLL ))
	{
		Msg( "Can't unlink commands while game is loaded\n" );
		return;
	}

	if( Cvar_VariableInteger( "host_clientloaded" ) && ( group & CMD_CLIENTDLL ))
	{
		Msg( "Can't unlink commands while client is loaded\n" );
		return;
	}

	for( prev = &cmd_functions; ( cmd = *prev ); )
	{
		if( group && !( cmd->flags & group ))
		{
			prev = &cmd->next;
			continue;
		}

#if defined(XASH_HASHED_VARS)
		BaseCmd_Remove( HM_CMD, cmd->name );
#endif

		*prev = cmd->next;

		Mem_Free( cmd->name );
		Mem_Free( cmd->desc );
		Mem_Free( cmd );
	}
}

/*
============
Cmd_Null_f

null function for some cmd stubs
============
*/
void Cmd_Null_f( void )
{
}

void Cmd_Test_f( void )
{
	Cbuf_AddFilterText( Cmd_Argv( 1 ) );
}

/*
============
Cmd_Init
============
*/
void Cmd_Init( void )
{
	cvar_vars = NULL;
	cmd_functions = NULL;
	cmd_argc = 0;
	cmd_args = NULL;
	cmd_alias = NULL;
	cmd_cond = 0;
	Cbuf_Init();

	// register our commands
	Cmd_AddCommand( "echo", Cmd_Echo_f, "print a message to the console (useful in scripts)" );
	Cmd_AddCommand( "wait", Cmd_Wait_f, "make script execution wait until next rendered frame" );
	Cmd_AddCommand( "cmdlist", Cmd_List_f, "display all console commands beginning with the specified prefix or matching the specified wildcard pattern" );
	Cmd_AddCommand( "apropos", Cmd_Apropos_f, "lists all console variables/commands/aliases containing the specified string in the name or description" );
	Cmd_AddCommand( "stuffcmds", Cmd_StuffCmds_f, va( "execute commandline parameters (must be present in %s.rc script)", SI.ModuleName ));
	Cmd_AddCommand( "cmd", Cmd_ForwardToServer, "send a console commandline to the server" );
	Cmd_AddCommand( "alias", Cmd_Alias_f, "create a script function, without arguments show the list of all aliases" );
	Cmd_AddCommand( "unalias", Cmd_UnAlias_f, "remove a script function" );
	Cmd_AddCommand( "if", Cmd_If_f, "compare and set condition bits" );
	Cmd_AddCommand( "else", Cmd_Else_f, "invert condition bit" );
	Cmd_AddCommand( "testfilter", Cmd_Test_f, "" );
}

void Cmd_Shutdown( void )
{
	int i;
	for( i = 0; i < cmd_argc; i++ )
		Z_Free( cmd_argv[i] );
	cmd_argc = 0; // clear previous args
	cmd_args = NULL;
	//Q_memset( cmd_text_buf, 0, sizeof( cmd_text_buf ) );
}
