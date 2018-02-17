/*
cvar.c - dynamic variable tracking
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

#define HASH_SIZE 256

convar_t	*cvar_vars; // head of list
convar_t	*userinfo, *physinfo, *serverinfo, *renderinfo;

/*
============
Cvar_GetList
============
*/
cvar_t *GAME_EXPORT Cvar_GetList( void )
{
	return (cvar_t *)cvar_vars;
}

/*
============
Cvar_GetName
============
*/
char *Cvar_GetName( cvar_t *cvar )
{
	return cvar->name;
}

/*
============
Cvar_ValidateString
============
*/
static qboolean Cvar_ValidateString( const char *s, qboolean isvalue )
{
	if( !s ) return false;
	if( Q_strstr( s, "\\" ) && !isvalue )
		return false;
	if( Q_strstr( s, "\"" )) return false;
	if( Q_strstr( s, ";" ) && !isvalue )
		return false;
	return true;
}

/*
============
Cvar_FindVar
============
*/
convar_t *Cvar_FindVar( const char *var_name )
{
#if defined(XASH_HASHED_VARS)
	return (convar_t *)BaseCmd_Find( HM_CVAR, var_name );
#else
	convar_t *var;

	for( var = cvar_vars; var; var = var->next )
	{
		if( !Q_stricmp( var_name, var->name ))
			return var;
	}
#endif

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float GAME_EXPORT Cvar_VariableValue( const char *var_name )
{
	convar_t	*var;

	var = Cvar_FindVar( var_name );
	if( !var ) return 0;
	return var->value;
}

/*
============
Cvar_VariableInteger
============
*/
int Cvar_VariableInteger( const char *var_name )
{
	convar_t	*var;

	var = Cvar_FindVar( var_name );
	if( !var ) return 0;

	return var->integer;
}

/*
============
Cvar_VariableString
============
*/
char *GAME_EXPORT Cvar_VariableString( const char *var_name )
{
	convar_t	*var;

	var = Cvar_FindVar( var_name );
	if( !var ) return "";

	return var->string;
}

/*
============
Cvar_LookupVars
============
*/
void Cvar_LookupVars( int checkbit, void *buffer, void *ptr, setpair_t callback )
{
	convar_t	*cvar;

	// nothing to process ?
	if( !callback ) return;

	// force checkbit to 0 for lookup all cvars
	for( cvar = cvar_vars; cvar; cvar = cvar->next )
	{
		if( checkbit && !( cvar->flags & checkbit ))
			continue;

		if( buffer )
		{
			callback( cvar->name, cvar->string, buffer, ptr );
		}
		else
		{
			char *desc;

			if( cvar->flags & CVAR_EXTDLL )
				desc = "game cvar";
			else
				desc = cvar->description;

			if( !desc )
				desc = "user cvar";

			callback( cvar->name, cvar->string, desc, ptr );
		}
	}
}

/*
============
Cvar_Get

If the variable already exists, the value will not be set
The flags will be or'ed in if the variable exists.
============
*/
convar_t *Cvar_Get( const char *var_name, const char *var_value, int flags, const char *var_desc )
{
	convar_t	*cvar;
	convar_t *current, *next;

	ASSERT( var_name != NULL );

	if( !var_value ) var_value = "0"; // just apply default value

	// all broadcast cvars must pass this check
	if( flags & ( CVAR_USERINFO|CVAR_SERVERINFO|CVAR_PHYSICINFO ))
	{
		if( !Cvar_ValidateString( var_name, false ))
		{
			MsgDev( D_ERROR, "invalid info cvar name string %s\n", var_name );
			return NULL;
		}

		if( !Cvar_ValidateString( var_value, true ))
		{
			MsgDev( D_WARN, "invalid cvar value string: %s\n", var_value );
			var_value = "0"; // just apply default value
		}
	}

	// check for overlap with a command
	if( Cmd_Exists( var_name ))
	{
		MsgDev( D_ERROR, "Cvar_Get: %s is a command\n", var_name );
		return NULL;
	}

	cvar = Cvar_FindVar( var_name );

	if( cvar )
	{
		// fast check for short cvars
		if( cvar->flags & CVAR_EXTDLL )
		{
			cvar->flags |= flags;
			return cvar;
		}

		// if the C code is now specifying a variable that the user already
		// set a value for, take the new value as the reset value
		if(( cvar->flags & CVAR_USER_CREATED ) && !( flags & CVAR_USER_CREATED ) && var_value[0] )
		{
			cvar->flags &= ~CVAR_USER_CREATED;
			Mem_Free( cvar->reset_string );
			cvar->reset_string = copystring( var_value );
		}

		cvar->flags |= flags;

		// only allow one non-empty reset string without a warning
		if( !cvar->reset_string[0] )
		{
			// we don't have a reset string yet
			Mem_Free( cvar->reset_string );
			cvar->reset_string = copystring( var_value );
		}

		// if we have a latched string, take that value now
		if( cvar->latched_string )
		{
			char *s = cvar->latched_string;
			cvar->latched_string = NULL; // otherwise cvar_set2 would free it
			Cvar_Set2( var_name, s, true );
			Mem_Free( s );
		}

		if( var_desc )
		{
			// update description if needed
			Z_Free( cvar->description );
			cvar->description = copystring( var_desc );
		}
		return cvar;
	}

	// allocate a new cvar
	cvar = Z_Malloc( sizeof( convar_t ));
	cvar->name = copystring( var_name );
	cvar->string = copystring( var_value );
	cvar->reset_string = copystring( var_value );
	if( var_desc ) cvar->description = copystring( var_desc );
	cvar->value = Q_atof( cvar->string );
	cvar->integer = Q_atoi( cvar->string );
	cvar->modified = true;
	cvar->flags = flags;

	// link the variable in alphanumerical order
	for( current = NULL, next = cvar_vars ; next && Q_strcmp( next->name, cvar->name ) < 0 ; current = next, next = next->next );

	if( current )
		current->next = cvar;
	else
		cvar_vars = cvar;

	cvar->next = next;

#if defined(XASH_HASHED_VARS)
	// add to map
	BaseCmd_Insert( HM_CVAR, cvar, cvar->name );
#endif

	return cvar;
}

/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void GAME_EXPORT Cvar_RegisterVariable( cvar_t *var )
{
	convar_t	*current, *next, *cvar;

	ASSERT( var != NULL );

	// first check to see if it has already been defined
	if(( cvar = Cvar_FindVar( var->name )) != NULL )
	{
		// this cvar is already registered with Cvar_RegisterVariable
		// so we can't replace it
		if( cvar->flags & CVAR_EXTDLL )
		{
			MsgDev( D_ERROR, "Can't register variable %s, already defined\n", var->name );
		}
		else
		{	
			var->string = cvar->string;	// we already have right string
			var->value = Q_atof( var->string );
			var->flags |= CVAR_EXTDLL;	// all cvars passed this function are game cvars
			var->next = (cvar_t *)cvar->next;

			if( cvar_vars == cvar )
			{
				// head of the list is easy to change
				cvar_vars = (convar_t *)var;
			}
			else
			{
				// otherwise find it somewhere in the list
				for( current = cvar_vars; current->next != cvar; current = current->next );

				current->next = (convar_t *)var;
			}

#if defined(XASH_HASHED_VARS)
			BaseCmd_Replace( HM_CVAR, var, var->name );
#endif
			// release current cvar (but keep string)
			Z_Free( cvar->name );
			Z_Free( cvar->latched_string );
			Z_Free( cvar->reset_string );
			Z_Free( cvar->description );
			Mem_Free( cvar );
		}
	}
	else if( Cmd_Exists( var->name ))
	{
		MsgDev( D_ERROR, "Cvar_Register: %s is a command\n", var->name );
	}
	else
	{
		// copy the value off, because future sets will Z_Free it
		var->string = copystring( var->string );
		var->value = Q_atof( var->string );
		var->flags |= CVAR_EXTDLL;		// all cvars passed this function are game cvars
	
		// link the variable in alphanumerical order
		for( current = NULL, next = cvar_vars ; next && Q_strcmp( next->name, var->name ) < 0 ; current = next, next = next->next );

		if( current )
			current->next = (convar_t *)var;
		else
			cvar_vars = (convar_t *)var;

		var->next = (cvar_t *)next;

		// add to bucket
		// we got a cvar_t from gamedll, where we have no left to chain in bucket
		// so disable it

#if defined(XASH_HASHED_VARS)
		BaseCmd_Insert( HM_CVAR, var, var->name );
#endif
	}
}
	
/*
============
Cvar_Set2
============
*/
convar_t *Cvar_Set2( const char *var_name, const char *value, qboolean force )
{
	convar_t		*var;
	const char	*pszValue;
	char		szNew[MAX_SYSPATH];
	qboolean		dll_variable = false;
	
	if( !Cvar_ValidateString( var_name, false ))
	{
		MsgDev( D_ERROR, "Invalid cvar name string: %s\n", var_name );
		return NULL;
	}

	var = Cvar_FindVar( var_name );
	if( !var )
	{
		// create it
		if( force ) return Cvar_Get( var_name, value, 0, NULL );
		return Cvar_Get( var_name, value, CVAR_USER_CREATED, NULL );

	}

	// use this check to prevent acessing for unexisting fields
	// for cvar_t: latched_string, description, etc
	if( var->flags & CVAR_EXTDLL )
		dll_variable = true;

	if( !value )
	{
		if( dll_variable ) value = "0";
		else value = var->reset_string;
	}

	if( !Q_strcmp( value, var->string ))
		return var;

	// any latched values not allowed for game cvars
	if( dll_variable ) force = true;

	if( !force )
	{
		if( var->flags & ( CVAR_READ_ONLY|CVAR_GLCONFIG ))
		{
			MsgDev( D_INFO, "%s is read only.\n", var_name );
			return var;
		}

		if( var->flags & CVAR_INIT )
		{
			MsgDev( D_INFO, "%s is write protected.\n", var_name );
			return var;
		}

		if( var->flags & ( CVAR_LATCH|CVAR_LATCH_VIDEO ))
		{
			if( var->latched_string )
			{
				if(!Q_strcmp( value, var->latched_string ))
					return var;
				Mem_Free( var->latched_string );
				var->latched_string = NULL;
			}
			else
			{
				if( !Q_strcmp( value, var->string ))
					return var;
			}

			if( var->flags & CVAR_LATCH && Cvar_VariableInteger( "host_serverstate" ))
			{
				MsgDev( D_INFO, "%s will be changed upon restarting.\n", var->name );
				var->latched_string = copystring( value );
			}
			else if( var->flags & CVAR_LATCH_VIDEO )
			{
				MsgDev( D_INFO, "%s will be changed upon restarting video.\n", var->name );
				var->latched_string = copystring( value );
			}
			else
			{
				Mem_Free( var->string );		// free the old value string
				var->string = copystring( value );
				var->value = Q_atof( var->string );
				var->integer = Q_atoi( var->string );
			}

			var->modified = true;
			return var;
		}

		if(( var->flags & CVAR_CHEAT ) && !Cvar_VariableInteger( "sv_cheats" ))
		{
			MsgDev( D_INFO, "%s is cheat protected.\n", var_name );
			return var;
		}
	}
	else
	{
		if( !dll_variable && var->latched_string )
		{
			Mem_Free( var->latched_string );
			var->latched_string = NULL;
		}
	}

	pszValue = value;

	// This cvar's string must only contain printable characters.
	// Strip out any other crap.
	// We'll fill in "empty" if nothing is left.
	if( var->flags & CVAR_PRINTABLEONLY )
	{
		const char *pS;
		char *pD;

		// clear out new string
		szNew[0] = '\0';

		pS = pszValue;
		pD = szNew;

		// step through the string, only copying back in characters that are printable
		while( *pS )
		{
			if( ((byte)*pS) < 32 )
			{
				pS++;
				continue;
			}
			*pD++ = *pS++;
		}

		// terminate the new string
		*pD = '\0';

		// if it's empty, then insert a marker string
		if( !Q_strlen( szNew ))
		{
			Q_strcpy( szNew, "default" );
		}

		// point the value here.
		pszValue = szNew;
	}

 	// nothing to change
	if( !Q_strcmp( pszValue, var->string ))
		return var;

	if( var->flags & CVAR_USERINFO )
		userinfo->modified = true;	// transmit at next opportunity

	if( var->flags & CVAR_PHYSICINFO )
		physinfo->modified = true;	// transmit at next opportunity

	if( var->flags & CVAR_SERVERINFO )
		serverinfo->modified = true;	// transmit at next opportunity

	if( var->flags & CVAR_RENDERINFO )
		renderinfo->modified = true;	// transmit at next opportunity
	
	// free the old value string
	Mem_Free( var->string );
	var->string = copystring( pszValue );
	var->value = Q_atof( var->string );

	if( !dll_variable )
	{
		var->integer = Q_atoi( var->string );
		var->modified = true;
	}

	return var;
}

/*
============
Cvar_Set
============
*/
void GAME_EXPORT Cvar_Set( const char *var_name, const char *value )
{
	Cvar_Set2( var_name, value, true );
}

/*
============
Cvar_SetLatched
============
*/
void Cvar_SetLatched( const char *var_name, const char *value )
{
	Cvar_Set2( var_name, value, false );
}

/*
============
Cvar_FullSet
============
*/
void Cvar_FullSet( const char *var_name, const char *value, int flags )
{
	convar_t	*var;
	qboolean	dll_variable = false;
		
	var = Cvar_FindVar( var_name );
	if( !var ) 
	{
		// create it
		Cvar_Get( var_name, value, flags, "" );
		return;
	}

	// use this check to prevent acessing of non-existing fields
	// for cvar_t: latched_string, description, etc.
	if( var->flags & CVAR_EXTDLL )
	{
		dll_variable = true;
	}

	if( var->flags & CVAR_USERINFO )
	{
		// transmit at next opportunity
		userinfo->modified = true;
	}	

	if( var->flags & CVAR_PHYSICINFO )
	{
		// transmit at next opportunity
		physinfo->modified = true;
	}

	if( var->flags & CVAR_SERVERINFO )
	{
		// transmit at next opportunity
		serverinfo->modified = true;
	}

	if( var->flags & CVAR_RENDERINFO )
	{
		// transmit at next opportunity
		renderinfo->modified = true;
	}

	Mem_Free( var->string ); // free the old value string
	var->string = copystring( value );
	var->value = Q_atof( var->string );
	var->flags = flags;

	if( dll_variable ) return;	// below fields don't exist in cvar_t

	var->integer = Q_atoi( var->string );
	var->modified = true;
}

/*
============
Cvar_DirectSet
============
*/
void GAME_EXPORT Cvar_DirectSet( cvar_t *var, const char *value )
{
	cvar_t		*test;
	const char	*pszValue;
	char		szNew[MAX_SYSPATH];
	
	if( !var ) return;	// GET_CVAR_POINTER has failed ?

	// make sure what is really pointer to the cvar
	test = (cvar_t *)Cvar_FindVar( var->name );
	ASSERT( var == test ); 

	if( value && !Cvar_ValidateString( value, true ))
	{
		MsgDev( D_WARN, "Invalid cvar value string: %s\n", value );
		value = "0";
	}

	if( !value ) value = "0";

	if( var->flags & ( CVAR_READ_ONLY|CVAR_GLCONFIG|CVAR_INIT|CVAR_RENDERINFO|CVAR_LATCH|CVAR_LATCH_VIDEO ))
	{
		// Cvar_DirectSet cannot change these cvars at all
		return;
	}
	
	if(( var->flags & CVAR_CHEAT ) && !Cvar_VariableInteger( "sv_cheats" ))
	{
		// cheats are disabled
		return;
	}

	pszValue = value;

	// This cvar's string must only contain printable characters.
	// Strip out any other crap.
	// We'll fill in "empty" if nothing is left.
	if( var->flags & CVAR_PRINTABLEONLY )
	{
		const char	*pS;
		char		*pD;

		// clear out new string
		szNew[0] = '\0';

		pS = pszValue;
		pD = szNew;

		// step through the string, only copying back in characters that are printable
		while( *pS )
		{
			if( *pS < 32 )
			{
				pS++;
				continue;
			}
			*pD++ = *pS++;
		}

		// terminate the new string
		*pD = '\0';

		// if it's empty, then insert a marker string
		if( !Q_strlen( szNew ))
		{
			Q_strcpy( szNew, "default" );
		}

		// point the value here.
		pszValue = szNew;
	}

	if( !Q_strcmp( pszValue, var->string ))
		return;

	if( var->flags & CVAR_USERINFO )
		userinfo->modified = true;	// transmit at next opportunity

	if( var->flags & CVAR_PHYSICINFO )
		physinfo->modified = true;	// transmit at next opportunity

	if( var->flags & CVAR_SERVERINFO )
		serverinfo->modified = true;	// transmit at next opportunity

	if( var->flags & CVAR_RENDERINFO )
		renderinfo->modified = true;	// transmit at next opportunity

	// free the old value string
	Mem_Free( var->string );
	var->string = copystring( pszValue );
	var->value = Q_atof( var->string );
}

/*
============
Cvar_SetFloat
============
*/
void GAME_EXPORT Cvar_SetFloat( const char *var_name, float value )
{
	char	val[32];

	if( value == (int)value )
		Q_sprintf( val, "%i", (int)value );
	else Q_sprintf( val, "%f", value );
	Cvar_Set( var_name, val );
}

/*
============
Cvar_Reset
============
*/
void Cvar_Reset( const char *var_name )
{
	Cvar_Set2( var_name, NULL, false );
}

/*
============
Cvar_SetCheatState

Any testing variables will be reset to the safe values
============
*/
void Cvar_SetCheatState( qboolean force )
{
	convar_t	*var;
	qboolean disable_cheats;

	var = Cvar_FindVar( "sv_cheats" );

	ASSERT( var );

	disable_cheats = !var->integer;

	if( var->latched_string )
		disable_cheats = !Q_atoi( var->latched_string );

	// when shutting down, restore all latched cvars before saving it to config
	if( force )
		disable_cheats = false;

	// set all default vars to the safe value
	for( var = cvar_vars; var; var = var->next )
	{
		// can't process dll cvars - missed latched_string, reset_string
		if( var->flags & CVAR_EXTDLL )
			continue;

		if( ( var->flags & CVAR_CHEAT ) && var->reset_string )
		{
			if( disable_cheats )
			{
				char *value = copystring( var->string );
				Cvar_FullSet( var->name, var->reset_string, var->flags );
				// keep values in latched field
				if( var->latched_string )
					Mem_Free( value );
				else
					var->latched_string = value;
			}
			else if( var->latched_string )
			{
				Cvar_FullSet( var->name, var->latched_string, var->flags );
				Mem_Free( var->latched_string );
				var->latched_string = NULL;
			}

		}
	}
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean Cvar_Command( convar_t *v )
{
	// check variables
#if !defined( XASH_HASHED_VARS ) // if don't have base cmd, ignore argument
	v = Cvar_FindVar( Cmd_Argv( 0 ));
#endif
	if( !v )
		return false;

	// perform a variable print or set
	if( Cmd_Argc() == 1 )
	{
		if( v->flags & ( CVAR_INIT|CVAR_EXTDLL )) Msg( "%s: %s\n", v->name, v->string );
		else Msg( "%s: %s ( ^3%s^7 )\n", v->name, v->string, v->reset_string );
		return true;
	}

	// set the value if forcing isn't required
	Cvar_Set2( v->name, Cmd_Argv( 1 ), false );
	return true;
}

/*
============
Cvar_Toggle_f

Toggles a cvar for easy single key binding
============
*/
void Cvar_Toggle_f( void )
{
	int	v;

	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: toggle <variable>\n" );
		return;
	}

	v = Cvar_VariableValue( Cmd_Argv( 1 ));
	v = !v;

	Cvar_Set2( Cmd_Argv( 1 ), va( "%i", v ), false );
}

/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console, even if they
weren't declared in C code.
============
*/
void Cvar_Set_f( void )
{
	int	i, c, l = 0, len;
	char	combined[MAX_CMD_TOKENS];

	c = Cmd_Argc();
	if( c < 3 )
	{
		Msg( "Usage: set <variable> <value>\n" );
		return;
	}
	combined[0] = 0;

	for( i = 2; i < c; i++ )
	{
		len = Q_strlen( Cmd_Argv(i) + 1 );
		if( l + len >= MAX_CMD_TOKENS - 2 )
			break;
		Q_strcat( combined, Cmd_Argv( i ));
		if( i != c-1 ) Q_strcat( combined, " " );
		l += len;
	}

	Cvar_Set2( Cmd_Argv( 1 ), combined, false );
}

/*
============
Cvar_SetU_f

As Cvar_Set, but also flags it as userinfo
============
*/
void Cvar_SetU_f( void )
{
	convar_t	*v;

	if( Cmd_Argc() != 3 )
	{
		Msg( "Usage: setu <variable> <value>\n" );
		return;
	}

	Cvar_Set_f();
	v = Cvar_FindVar( Cmd_Argv( 1 ));

	if( !v ) return;
	v->flags |= CVAR_USERINFO;
	userinfo->modified = true;
}

/*
============
Cvar_SetP_f

As Cvar_Set, but also flags it as physinfo
============
*/
void Cvar_SetP_f( void )
{
	convar_t	*v;

	if( Cmd_Argc() != 3 )
	{
		Msg( "Usage: setp <variable> <value>\n" );
		return;
	}

	Cvar_Set_f();
	v = Cvar_FindVar( Cmd_Argv( 1 ));

	if( !v ) return;
	v->flags |= CVAR_PHYSICINFO;
	physinfo->modified = true;
}

/*
============
Cvar_SetS_f

As Cvar_Set, but also flags it as serverinfo
============
*/
void Cvar_SetS_f( void )
{
	convar_t	*v;

	if( Cmd_Argc() != 3 )
	{
		Msg( "Usage: sets <variable> <value>\n" );
		return;
	}
	Cvar_Set_f();
	v = Cvar_FindVar( Cmd_Argv( 1 ));

	if( !v ) return;
	v->flags |= CVAR_SERVERINFO;
	serverinfo->modified = true;
}

/*
============
Cvar_SetA_f

As Cvar_Set, but also flags it as archived
============
*/
void Cvar_SetA_f( void )
{
	convar_t	*v;

	if( Cmd_Argc() < 3 )
	{
		Msg( "Usage: seta <variable> <value> [<description>]\n" );
		return;
	}

	Cvar_Set_f();
	v = Cvar_FindVar( Cmd_Argv( 1 ));

	if( !v ) return;
	v->flags |= CVAR_ARCHIVE;

	if( v->description )
		Mem_Free( v->description );

	// cvars without description are not saved, so add description
	v->description = copystring( "user archive cvar" );
}

/*
============
Cvar_SetR_f

As Cvar_Set, but also flags it as renderinfo
============
*/
void Cvar_SetR_f( void )
{
	if( Cmd_Argc() != 3 )
	{
		Msg( "Usage: setr <variable> <value>\n" );
		return;
	}

	Cvar_FullSet( Cmd_Argv( 1 ), Cmd_Argv( 2 ), CVAR_RENDERINFO );
}

/*
============
Cvar_SetGL_f

As Cvar_Set, but also flags it as glconfig
============
*/
void Cvar_SetGL_f( void )
{
	if( Cmd_Argc() != 3 )
	{
		Msg( "Usage: setgl <variable> <value>\n" );
		return;
	}

	Cvar_FullSet( Cmd_Argv( 1 ), Cmd_Argv( 2 ), CVAR_GLCONFIG );
}

/*
============
Cvar_Reset_f
============
*/
void Cvar_Reset_f( void )
{
	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: reset <variable>\n" );
		return;
	}
	Cvar_Reset( Cmd_Argv( 1 ));
}

/*
============
Cvar_List_f
============
*/
void Cvar_List_f( void )
{
	convar_t	*var;
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

	for( var = cvar_vars; var; var = var->next, i++ )
	{
		if( var->name[0] == '@' )
			continue;	// never shows system cvars

		if( len && ( ispattern ? !matchpattern_with_separator( var->name, partial, false, "", false ) : Q_strncmp( partial, var->name, len )))
			continue;

		// TODO: fix ugly formatting
		if( var->flags & CVAR_SERVERINFO ) Msg( "SV    " );
		else Msg( " " );

		if( var->flags & CVAR_USERINFO ) Msg( "USER  " );
		else Msg( " " );

		if( var->flags & CVAR_PHYSICINFO ) Msg( "PHYS  " );
		else Msg( " " );

		if( var->flags & CVAR_READ_ONLY ) Msg( "READ  " );
		else Msg( " " );

		if( var->flags & CVAR_INIT ) Msg( "INIT  " );
		else Msg( " " );

		if( var->flags & CVAR_ARCHIVE ) Msg( "ARCH  " );
		else Msg( " " );

		if( var->flags & CVAR_LATCH ) Msg( "LATCH " );
		else Msg( " " );

		if( var->flags & CVAR_LATCH_VIDEO ) Msg( "VIDEO " );
		else Msg( " " );

		if( var->flags & CVAR_GLCONFIG ) Msg( "OPENGL" );
		else Msg( " " );

		if( var->flags & CVAR_CHEAT ) Msg( "CHEAT " );
		else Msg( " " );

		if( var->flags & CVAR_EXTDLL )
			Msg(" %s \"%s\" %s\n", var->name, var->string, "game cvar" );
		else Msg(" %s \"%s\" %s\n", var->name, var->string, var->description );
		j++;
	}

	if( len )
	{
		if( ispattern )
			Msg( "\n%i cvar%s matching \"%s\"\n\n", j, ( j > 1 ) ? "s" : "", partial );
		else
			Msg( "\n%i cvar%s beginning with \"%s\"\n\n", j, ( j > 1 ) ? "s" : "", partial );
	}
	else
	{
		Msg( "\n%i cvar%s\n", j, ( j > 1 ) ? "s" : "" );
	}

	Msg( "%i total cvars\n", i );
}

/*
============
Cvar_Restart_f

Resets all cvars to their hardcoded values
============
*/
void Cvar_Restart_f( void )
{
	convar_t	*var;
	convar_t	**prev;

	for( prev = &cvar_vars; ( var = *prev ); )
	{
		// don't mess with rom values, or some inter-module
		// communication will get broken (cl.active, etc.)
		if( var->flags & ( CVAR_READ_ONLY|CVAR_GLCONFIG|CVAR_INIT|CVAR_RENDERINFO|CVAR_EXTDLL ))
		{
			prev = &var->next;
			continue;
		}

		// throw out any variables the user created
		if( var->flags & CVAR_USER_CREATED )
		{
#if defined(XASH_HASHED_VARS)
			BaseCmd_Remove( HM_CVAR, var->name );
#endif

			*prev = var->next;
			Z_Free( var->name );
			Z_Free( var->string );
			Z_Free( var->latched_string );
			Z_Free( var->reset_string );
			Z_Free( var->description );
			Mem_Free( var );
			continue;
		}

		Cvar_Set( var->name, var->reset_string );
		prev = &var->next;
	}
}

/*
============
Cvar_Latched_f

Now all latched strings are valid
============
*/
void Cvar_Latched_f( void )
{
	convar_t	*var;
	qboolean disable_cheats;

	var = Cvar_FindVar( "sv_cheats" );

	ASSERT( var );

	disable_cheats = !var->integer;

	if( var->latched_string )
		disable_cheats = !Q_atoi( var->latched_string );

	for( var = cvar_vars ; var ; var = var->next )
	{
		if( var->flags & CVAR_EXTDLL )
			continue;

		if( var->flags & CVAR_LATCH && var->latched_string )
		{
			Cvar_FullSet( var->name, var->latched_string, var->flags );
			Mem_Free( var->latched_string );
			var->latched_string = NULL;
		}
		if( ( var->flags & CVAR_CHEAT ) && var->reset_string )
		{
			if( disable_cheats )
			{
				char *value = copystring( var->string );
				Cvar_FullSet( var->name, var->reset_string, var->flags );
				// keep values in latched field
				if( var->latched_string )
					Mem_Free( value );
				else
					var->latched_string = value;
			}
			else if( var->latched_string )
			{
				Cvar_FullSet( var->name, var->latched_string, var->flags );
				Mem_Free( var->latched_string );
				var->latched_string = NULL;
			}

		}
	}
}

/*
============
Cvar_LatchedVideo_f

Now all latched video strings are valid
============
*/
void Cvar_LatchedVideo_f( void )
{
	convar_t	*var;

	for( var = cvar_vars ; var ; var = var->next )
	{
		if( var->flags & CVAR_EXTDLL )
			continue;

		if( var->flags & CVAR_LATCH_VIDEO && var->latched_string )
		{
			Cvar_FullSet( var->name, var->latched_string, var->flags );
			Mem_Free( var->latched_string );
			var->latched_string = NULL;
		}
	}
}

/*
============
Cvar_Unlink_f

unlink all cvars with flag CVAR_EXTDLL
============
*/
void Cvar_Unlink_f( void )
{
	convar_t	*var;
	convar_t	**prev;

	if( Cvar_VariableInteger( "host_gameloaded" ))
	{
		MsgDev( D_NOTE, "Can't unlink cvars while game is loaded.\n" );
		return;
	}

	for( prev = &cvar_vars; ( var = *prev ); )
	{

		// ignore all non-game cvars
		if( !( var->flags & CVAR_EXTDLL ))
		{
			prev = &var->next;
			continue;
		}

		// throw out any variables the game created
#if defined(XASH_HASHED_VARS)
		BaseCmd_Remove( HM_CVAR, var->name );
#endif
		*prev = var->next;
		Z_Free( var->string );
	}
}

/*
============
Cvar_Unlink

unlink all cvars with flag CVAR_CLIENTDLL
============
*/
void Cvar_Unlink( void )
{
	convar_t	*var;
	convar_t	**prev;

	if( Cvar_VariableInteger( "host_clientloaded" ))
	{
		MsgDev( D_NOTE, "Can't unlink cvars while client is loaded.\n" );
		return;
	}

	for( prev = &cvar_vars; ( var = *prev ); )
	{
		// ignore all non-client cvars
		if( !( var->flags & CVAR_CLIENTDLL ))
		{
			prev = &var->next;
			continue;
		}

		// throw out any variables the game created
#if defined(XASH_HASHED_VARS)
		BaseCmd_Remove( HM_CVAR, var->name );
#endif

		*prev = var->next;
		Z_Free( var->name );
		Z_Free( var->string );
		Z_Free( var->latched_string );
		Z_Free( var->reset_string );
		Z_Free( var->description );
		Mem_Free( var );
	}
}

/*
============
Cvar_Init

Reads in all archived cvars
============
*/
void Cvar_Init( void )
{
	cvar_vars = NULL;
	userinfo = Cvar_Get( "@userinfo", "0", CVAR_READ_ONLY, "" ); // use ->modified value only
	physinfo = Cvar_Get( "@physinfo", "0", CVAR_READ_ONLY, "" ); // use ->modified value only
	serverinfo = Cvar_Get( "@serverinfo", "0", CVAR_READ_ONLY, "" ); // use ->modified value only
	renderinfo = Cvar_Get( "@renderinfo", "0", CVAR_READ_ONLY, "" ); // use ->modified value only

	Cmd_AddCommand ("toggle", Cvar_Toggle_f, "toggles a console variable's value (use for more info)" );
	Cmd_AddCommand ("set", Cvar_Set_f, "create or change the value of a console variable" );
	Cmd_AddCommand ("sets", Cvar_SetS_f, "create or change the value of a serverinfo variable" );
	Cmd_AddCommand ("setu", Cvar_SetU_f, "create or change the value of a userinfo variable" );
	Cmd_AddCommand ("setinfo", Cvar_SetU_f, "create or change the value of a userinfo variable" );
	Cmd_AddCommand ("setp", Cvar_SetP_f, "create or change the value of a physicinfo variable" );
	Cmd_AddCommand ("setr", Cvar_SetR_f, "create or change the value of a renderinfo variable" );
	Cmd_AddCommand ("setgl", Cvar_SetGL_f, "create or change the value of a opengl variable" );
	Cmd_AddCommand ("seta", Cvar_SetA_f, "create or change the value of a console variable that will be saved to config.cfg" );
	Cmd_AddCommand ("reset", Cvar_Reset_f, "reset any type of variable to initial value" );
	Cmd_AddCommand ("latch", Cvar_Latched_f, "apply latched values" );
	Cmd_AddCommand ("vidlatch", Cvar_LatchedVideo_f, "apply latched values for video subsystem" );
	Cmd_AddCommand ("cvarlist", Cvar_List_f, "display all console variables beginning with the specified prefix" );
	Cmd_AddCommand ("unsetall", Cvar_Restart_f, "reset all console variables to their default values" );
	Cmd_AddCommand ("@unlink", Cvar_Unlink_f, "unlink static cvars defined in gamedll" );
}
