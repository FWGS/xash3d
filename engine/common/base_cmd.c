/*
base_cmd.c - command & cvar hashmap. Insipred by Doom III
Copyright (C) 2016 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#define XASH_HASHED_VARS
#include <base_cmd.h>
#include <common.h>

typedef struct base_command_hashmap_s
{
	base_command_t *basecmd;
	const char *name;
	base_command_type_e     type;
	struct base_command_hashmap_s *next;
} base_command_hashmap_t;

#define HASH_SIZE 300
base_command_hashmap_t *hashed_cmds[HASH_SIZE];


/*
============
BaseCmd_Find
============
*/
base_command_t *BaseCmd_Find( base_command_type_e type, const char *name )
{
	uint hash = Com_HashKey( name, HASH_SIZE );
	base_command_hashmap_t *i;

	for( i = hashed_cmds[hash]; i &&
		 ( i->type != type || Q_stricmp( name, i->name ) ); // filter out
		 i = i->next );

	if( i )
		return i->basecmd;
	return NULL;
}

/*
============
BaseCmd_Insert
============
*/
void BaseCmd_Insert( base_command_type_e type, base_command_t *basecmd, const char *name )
{
	uint hash = Com_HashKey( name, HASH_SIZE );
	base_command_hashmap_t *elem;

	elem = Z_Malloc( sizeof( base_command_hashmap_t ) );
	elem->basecmd = basecmd;
	elem->type = type;
	elem->name = name;
	elem->next = hashed_cmds[hash];
	hashed_cmds[hash] = elem;
}

/*
============
BaseCmd_Replace
============
*/
void BaseCmd_Replace( base_command_type_e type, base_command_t *basecmd, const char *name )
{
	uint hash = Com_HashKey( name, HASH_SIZE );
	base_command_hashmap_t *i;

	for( i = hashed_cmds[hash]; i &&
		 ( i->type != type || Q_stricmp( name, i->name ) ) ; // filter out
		 i = i->next );

	if( !i )
	{
		MsgDev( D_ERROR, "BaseCmd_Replace: couldn't find %s\n", name);
		return;
	}

	i->basecmd = basecmd;
	i->name = name; // may be freed after
}

/*
============
BaseCmd_Remove
============
*/
void BaseCmd_Remove( base_command_type_e type, base_command_t *basecmd, const char *name )
{
	uint hash = Com_HashKey( name, HASH_SIZE );
	base_command_hashmap_t *i, *prev;

	for( prev = NULL, i = hashed_cmds[hash]; i &&
		 ( i->basecmd != basecmd || i->type != type); // filter out
		 prev = i, i = i->next );

	if( !i )
	{
		MsgDev( D_ERROR, "Couldn't find %s in buckets", name );
		return;
	}

	if( prev )
		prev->next = i->next;
	else
		hashed_cmds[hash] = i->next;

	Z_Free( i );
}

/*
============
BaseCmd_Init
============
*/
void BaseCmd_Init( void )
{
	Q_memset( hashed_cmds, 0, sizeof( hashed_cmds ) );
}
