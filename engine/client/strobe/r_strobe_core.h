/*
r_strobe_core.h - Software based strobing implementation

Copyright (C) 2018 - fuzun * github/fuzun

For information:
	https://forums.blurbusters.com/viewtopic.php?f=7&t=3815&p=30401

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.
*/

#if !defined R_STROBE_CORE_H && !defined STROBE_DISABLED
#define R_STROBE_CORE_H
#include "r_strobe_api.h"


#define STROBE_CORE STROBE_IMPL( CORE )


struct STROBE_IMPL_STRUCT( STROBE_CORE );
struct STROBE_IMPL_PRIVATE_STRUCT( STROBE_CORE );

struct STROBE_IMPL_STRUCT( STROBE_CORE )
{
	StrobeAPI_t base;
	struct STROBE_IMPL_PRIVATE_STRUCT( STROBE_CORE ) * private;
	void ( *STROBE_IMPL_FUNC_MAIN )( STROBE_IMPL_THIS_PARAM( STROBE_CORE ) );
	void ( *STROBE_IMPL_FUNC_DEBUGHANDLER )( STROBE_IMPL_THIS_PARAM( STROBE_CORE ) );
};
extern struct STROBE_IMPL_STRUCT( STROBE_CORE ) * STROBE_CORE;

void STROBE_IMPL_EXPORTEDFUNC_constructor( STROBE_CORE )( const void *const *const STROBE_CORE );
void STROBE_IMPL_EXPORTEDFUNC_destructor( STROBE_CORE )( const void *const *const STROBE_CORE );
void STROBE_IMPL_EXPORTEDFUNC_reinit( STROBE_CORE )( const void *const *const STROBE_CORE );
void STROBE_IMPL_EXPORTEDFUNC_main( STROBE_CORE )( const void *const *const STROBE_CORE );

#endif
