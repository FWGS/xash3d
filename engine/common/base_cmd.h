/*
base_cmd.h - command & cvar hashmap. Insipred by Doom III
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

#pragma once
#ifndef BASE_CMD_H
#define BASE_CMD_H

// TODO: Find cases when command hashmap works incorrect
// and maybe disable it
#define XASH_HASHED_VARS

#ifdef XASH_HASHED_VARS

typedef enum base_command_type
{
	HM_DONTCARE = 0,
	HM_CVAR,
	HM_CMD,
	HM_CMDALIAS
} base_command_type_e;

typedef void base_command_t;

typedef struct base_command_hashmap_s
{
	base_command_t          *basecmd; // base command: cvar, alias or command
	const char              *name;    // key for searching
	base_command_type_e     type;     // type for faster searching
	struct base_command_hashmap_s *next;
} base_command_hashmap_t;


void BaseCmd_Init( void );
base_command_hashmap_t *BaseCmd_GetBucket( const char *name );
base_command_hashmap_t *BaseCmd_FindInBucket( base_command_hashmap_t *bucket, base_command_type_e type, const char *name );
base_command_t *BaseCmd_Find( base_command_type_e type, const char *name );
void BaseCmd_FindAll( const char *name,
	base_command_t **cmd, base_command_t **alias, base_command_t **cvar );
void BaseCmd_Insert ( base_command_type_e type, base_command_t *basecmd, const char *name );
qboolean BaseCmd_Replace( base_command_type_e type, base_command_t *basecmd, const char *name ); // only if same name
void BaseCmd_Remove ( base_command_type_e type, base_command_t *basecmd, const char *name );

#endif // XASH_HASHED_VARS

#endif // BASE_CMD_H
