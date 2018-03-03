/*
r_strobe_core.h - Software based strobing implementation

Copyright (C) 2018 - fuzun * github.com/fuzun

For information:
https://forums.blurbusters.com

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

#define STROBE_CORE Strobe_Core
#define _STROBE_CORE_THIS self
#define STROBE_CORE_FUNC_main main
#define STROBE_CORE_FUNC_debughandler debugDrawer


#define STROBE_CORE_s STROBE_CORE ## _s
#define STROBE_CORE_t STROBE_CORE ## _t
#define STROBE_CORE_priv_s STROBE_CORE ## _priv_s
#define STROBE_CORE_priv_t STROBE_CORE ## _priv_t
#define STROBE_CORE_THIS (* ## _STROBE_CORE_THIS ## )
#define STROBE_CORE_THIS_PARAM STROBE_CORE_t ## ** ## _STROBE_CORE_THIS
#define STROBE_CORE_EXPORTEDFUNC_main STROBE_CORE ## _ ## STROBE_CORE_FUNC_main
#define STROBE_CORE_EXPORTEDFUNC_constructor STROBE_CORE ## _constructor
#define STROBE_CORE_EXPORTEDFUNC_destructor STROBE_CORE ## _destructor
#define STROBE_CORE_EXPORTEDFUNC_reinit STROBE_CORE ## _reinit


typedef struct STROBE_CORE_s STROBE_CORE_t;
typedef struct STROBE_CORE_priv_s STROBE_CORE_priv_t;


typedef struct STROBE_CORE_s {
	StrobeAPI_t base;
	STROBE_CORE_priv_t *private;
	void(*STROBE_CORE_FUNC_main)(STROBE_CORE_THIS_PARAM);
	void(*STROBE_CORE_FUNC_debughandler)(STROBE_CORE_THIS_PARAM);
} STROBE_CORE_t;

extern STROBE_CORE_t *STROBE_CORE;

void STROBE_CORE_EXPORTEDFUNC_constructor(void **STROBE_CORE);
void STROBE_CORE_EXPORTEDFUNC_destructor(void **STROBE_CORE);
void STROBE_CORE_EXPORTEDFUNC_reinit(void **STROBE_CORE);
void STROBE_CORE_EXPORTEDFUNC_main(void **STROBE_CORE);


#endif