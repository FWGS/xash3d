/*
r_strobe_base_protected_.h - Software based strobing implementation

Copyright (C) 2018 - fuzun * github/fuzun

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

#if !defined R_STROBE_BASE_PROTECTED_ && !defined STROBE_DISABLED
#define R_STROBE_BASE_PROTECTED_

#include "common.h"

typedef enum {
	p_positive = BIT(0), // Phase: Positive
	p_inverted = BIT(1), // Phase: Inverted
	f_normal = BIT(2) // Frame: Normal
} fstate_e; // Frame State


typedef struct StrobeAPI_protected_s {
	size_t fCounter; // Frame counter
	size_t pCounter, pNCounter, pBCounter; // Positive phase counters
	size_t nCounter, nNCounter, nBCounter; // Negative phase counters
	double deviation; // deviation
	double cdTimer; // Cooldown timer
	char cdTriggered;
	fstate_e frameInfo; // Frame info
} StrobeAPI_protected_t; // Protected members


#endif