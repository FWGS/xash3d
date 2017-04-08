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

void BaseCmd_Init( void );
base_command_t *BaseCmd_Find( base_command_type_e type, const char *name );
void BaseCmd_Insert ( base_command_type_e type, base_command_t *basecmd, const char *name );
void BaseCmd_Replace( base_command_type_e type, base_command_t *basecmd, const char *name ); // only if same name
void BaseCmd_Remove ( base_command_type_e type, base_command_t *basecmd, const char *name );
#endif
#endif
