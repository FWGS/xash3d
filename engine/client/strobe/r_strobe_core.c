/*
r_strobe_core.c - Software based strobing implementation

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

#if !defined XASH_DEDICATED && !defined STROBE_DISABLED

#include "r_strobe_core.h"
#include "client.h"
#include "gl_local.h"
#include "r_strobe_base_protected_.h"

STROBE_CORE_t *STROBE_CORE = NULL;

typedef struct STROBE_CORE_private_s
{
	double recentTime, recentTime2;
	double delta[STROBE_CORE_DEVIATION_SIZE]; // double *delta;
	size_t fCounterSnapshot;
} STROBE_CORE_private_t;

/*
===============
R_Strobe

TODO:
* Make timers global (at StrobeAPI level).
* Make swapping transition seamless by rendering non-opaque frames.
* Implement high precision timer to keep the internal phase on track with monitor. (In case of freezes etc.)
===============
*/
static void R_Strobe( STROBE_CORE_THIS_PARAM )
{
	double delta, delta2;
	double currentTime                     = Sys_DoubleTime( );
	delta2                                 = currentTime - STROBE_CORE_THIS->private->recentTime2;
	STROBE_CORE_THIS->private->recentTime2 = currentTime;

	if ( CL_IsInMenu( ) )
	{
		R_Set2DMode( false );
		return;
	}

	if ( STROBE_CORE_THIS->base.protected->cdTimer >= 0.0 && delta2 > 0.0 )
		STROBE_CORE_THIS->base.protected->cdTimer += delta2;
	if ( STROBE_CORE_THIS->base.protected->fCounter - STROBE_CORE_THIS->private->fCounterSnapshot == 1 )
	{
		STROBE_CORE_THIS->private->delta[STROBE_CORE_THIS->base.protected->fCounter % ARRAYSIZE( STROBE_CORE_THIS->private->delta )] = delta2;
		STROBE_CORE_THIS->base.protected->deviation                                                                                  = STROBE_CORE_THIS->base.Helpers.StandardDeviation( STROBE_CORE_THIS->private->delta, ARRAYSIZE( STROBE_CORE_THIS->private->delta ) ) * 1000;
	}
	STROBE_CORE_THIS->private->fCounterSnapshot = STROBE_CORE_THIS->base.protected->fCounter;

	if ( StrobeAPI.r_strobe_cooldown->integer > 0 )
	{
		if ( ( STROBE_CORE_THIS->base.protected->cdTimer > (double)abs( StrobeAPI.r_strobe_cooldown->integer ) ) && STROBE_CORE_THIS->base.protected->cdTriggered == true )
		{
			STROBE_CORE_THIS->base.protected->cdTriggered = false;
			STROBE_CORE_THIS->base.protected->cdTimer     = -1.0;
		}

		if ( STROBE_CORE_THIS->base.protected->fCounter > ARRAYSIZE( STROBE_CORE_THIS->private->delta ) )
		{
			if ( STROBE_CORE_THIS->base.protected->deviation > STROBE_CORE_DEVIATION_LIMIT )
			{
				STROBE_CORE_THIS->base.protected->cdTriggered = false;
				STROBE_CORE_THIS->base.protected->cdTimer     = 0.0;
			}
		}
	}
	else
	{
		STROBE_CORE_THIS->base.protected->cdTriggered = false;
	}

	if ( ( ( STROBE_CORE_THIS->base.protected->strobeInterval != StrobeAPI.r_strobe->integer ) && ( STROBE_CORE_THIS->base.protected->strobeInterval ) ) ||
	     /*((swapInterval != r_strobe_swapinterval->integer) && (swapInterval != 0)) || */
	     STROBE_CORE_THIS->base.protected->fCounter == UINT_MAX )
	{
		STROBE_CORE_EXPORTEDFUNC_reinit( *(void ***)&_STROBE_CORE_THIS );
		R_Strobe( &STROBE_CORE_THIS );
	}

	STROBE_CORE_THIS->base.protected->strobeInterval = StrobeAPI.r_strobe->integer;
	STROBE_CORE_THIS->base.protected->swapInterval   = StrobeAPI.r_strobe_swapinterval->integer;

	if ( ( STROBE_CORE_THIS->base.protected->strobeInterval == 0 ) ||
	     ( ( gl_swapInterval->integer == 0 ) && ( STROBE_CORE_THIS->base.protected->strobeInterval ) ) )
	{
		if ( !gl_swapInterval->integer )
			MsgDev( D_WARN, "Strobing requires V-SYNC not being turned off! (gl_swapInterval != 0) \n" );

		if ( STROBE_CORE_THIS->base.protected->strobeInterval ) // If v-sync is off, turn off strobing
		{
			Cvar_Set( "r_strobe", "0" );
		}
		STROBE_CORE_THIS->base.protected->fCounter = 0;

		R_Set2DMode( false );
		return;
	}

	if ( ( STROBE_CORE_THIS->base.protected->fCounter % 2 ) == 0 )
	{
		++STROBE_CORE_THIS->base.protected->pCounter;
		STROBE_CORE_THIS->base.protected->frameInfo |= p_positive;
	}
	else
	{
		++STROBE_CORE_THIS->base.protected->nCounter;
		STROBE_CORE_THIS->base.protected->frameInfo &= ~p_positive;
	}

	if ( STROBE_CORE_THIS->base.protected->swapInterval < 0 )
		STROBE_CORE_THIS->base.protected->swapInterval = abs( STROBE_CORE_THIS->base.protected->swapInterval );

	if ( ( STROBE_CORE_THIS->base.protected->swapInterval ) && ( STROBE_CORE_THIS->base.protected->strobeInterval % 2 ) ) // Swapping not enabled for even intervals as it is neither necessary nor works as intended
	{
		delta = currentTime - STROBE_CORE_THIS->private->recentTime;                                                                                                 // New Currenttime for _delta ?
		if ( ( delta >= (double)( STROBE_CORE_THIS->base.protected->swapInterval ) ) && ( delta < (double)( 2 * STROBE_CORE_THIS->base.protected->swapInterval ) ) ) // Basic timer
		{
			STROBE_CORE_THIS->base.protected->frameInfo |= p_inverted;
		}
		else if ( delta < (double)( STROBE_CORE_THIS->base.protected->swapInterval ) )
		{
			STROBE_CORE_THIS->base.protected->frameInfo &= ~p_inverted;
		}
		else //if (delta >= (double)(2 * swapInterval))
		{
			STROBE_CORE_THIS->private->recentTime = currentTime;
		}
	}

	switch ( STROBE_CORE_THIS->base.protected->frameInfo & ( p_positive | p_inverted ) )
	{
	case ( p_positive | p_inverted ):
		if ( ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) % 2 ) == 0 )
			STROBE_CORE_THIS->base.protected->frameInfo = ( ( ( STROBE_CORE_THIS->base.protected->pCounter - 1 ) % ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) + 1 ) ) == ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) / 2 ) ) ? STROBE_CORE_THIS->base.protected->frameInfo | f_normal : STROBE_CORE_THIS->base.protected->frameInfo & ~f_normal; //even
		else
			STROBE_CORE_THIS->base.protected->frameInfo &= ~f_normal;
		break;

	case ( p_positive & ~p_inverted ):
		if ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) % 2 == 0 )
			STROBE_CORE_THIS->base.protected->frameInfo = ( ( ( STROBE_CORE_THIS->base.protected->pCounter - 1 ) % ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) + 1 ) ) == 0 ) ? STROBE_CORE_THIS->base.protected->frameInfo | f_normal : STROBE_CORE_THIS->base.protected->frameInfo & ~f_normal; //even
		else
		{
			if ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) == 1 )
				STROBE_CORE_THIS->base.protected->frameInfo |= f_normal;
			else
				STROBE_CORE_THIS->base.protected->frameInfo = ( ( ( STROBE_CORE_THIS->base.protected->pCounter - 1 ) % ( ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) + 1 ) / 2 ) ) == 0 ) ? STROBE_CORE_THIS->base.protected->frameInfo | f_normal : STROBE_CORE_THIS->base.protected->frameInfo & ~f_normal; //odd
		}
		break;

	case ( ~p_positive & p_inverted ):
		if ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) % 2 == 0 )
			STROBE_CORE_THIS->base.protected->frameInfo = ( ( ( STROBE_CORE_THIS->base.protected->nCounter - 1 ) % ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) + 1 ) ) == 0 ) ? STROBE_CORE_THIS->base.protected->frameInfo | f_normal : STROBE_CORE_THIS->base.protected->frameInfo & ~f_normal; //even
		else
		{
			if ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) == 1 )
				STROBE_CORE_THIS->base.protected->frameInfo |= f_normal;
			else
				STROBE_CORE_THIS->base.protected->frameInfo = ( ( ( STROBE_CORE_THIS->base.protected->nCounter - 1 ) % ( ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) + 1 ) / 2 ) ) == 0 ) ? STROBE_CORE_THIS->base.protected->frameInfo | f_normal : STROBE_CORE_THIS->base.protected->frameInfo & ~f_normal; //odd
		}
		break;

	case 0:
		if ( ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) % 2 ) == 0 )
			STROBE_CORE_THIS->base.protected->frameInfo = ( ( ( STROBE_CORE_THIS->base.protected->nCounter - 1 ) % ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) + 1 ) ) == ( abs( STROBE_CORE_THIS->base.protected->strobeInterval ) / 2 ) ) ? STROBE_CORE_THIS->base.protected->frameInfo | f_normal : STROBE_CORE_THIS->base.protected->frameInfo & ~f_normal; //even
		else
			STROBE_CORE_THIS->base.protected->frameInfo &= ~f_normal;
		break;

	default:
		STROBE_CORE_THIS->base.protected->frameInfo = ( p_positive | f_normal );
	}

	if ( STROBE_CORE_THIS->base.protected->strobeInterval < 0 )
		STROBE_CORE_THIS->base.protected->frameInfo ^= f_normal;

	STROBE_CORE_THIS->base.ProcessFrame( &STROBE_CORE_THIS->base );
}

_inline void debugDrawer( STROBE_CORE_THIS_PARAM )
{
	rgba_t color;
	char debugStr[2048]; // Heap allocation ?
	static int offsetX;
	int offsetY;
	int fixer;

	qboolean strobeDebug = StrobeAPI.r_strobe_debug->integer ? true : false;

	if ( cls.state != ca_active )
		return;
	if ( ( !strobeDebug && !cl_showfps->integer ) || cl.background )
		return;

	switch ( cls.scrshot_action )
	{
	case scrshot_normal:
	case scrshot_snapshot:
	case scrshot_inactive:
		break;
	default:
		return;
	}

	if ( strobeDebug )
	{
		STROBE_CORE_THIS->base.Helpers.GenerateDebugStatistics( &STROBE_CORE_THIS->base, debugStr, ARRAYSIZE( debugStr ) );
	}
	else if ( cl_showfps->integer )
	{
		Q_snprintf( debugStr, sizeof( debugStr ), "%3d eFPS", (int)round( STROBE_CORE_THIS->base.Helpers.effectiveFPS( &STROBE_CORE_THIS->base ) ) );
	}

	MakeRGBA( color, 255, 255, 255, 255 );
	Con_DrawStringLen( debugStr, &fixer, &offsetY );
	if ( strobeDebug )
		Con_DrawString( scr_width->integer - offsetX - 50, 4, debugStr, color );
	else
		Con_DrawString( scr_width->integer - offsetX - 2, offsetY + 8, debugStr, color );
	if(abs(fixer - offsetX) > 30 || offsetX == 0)
		offsetX = fixer;
}

void STROBE_CORE_EXPORTEDFUNC_constructor( void **STROBE_CORE )
{
	STROBE_CORE_t **instance = *(STROBE_CORE_t ***)&STROBE_CORE;

	*instance                                    = (STROBE_CORE_t *)malloc( sizeof( STROBE_CORE_t ) );
	( *instance )->private                       = (STROBE_CORE_private_t *)calloc( 1, sizeof( STROBE_CORE_private_t ) );
	( *instance )->STROBE_CORE_FUNC_main         = R_Strobe;
	( *instance )->STROBE_CORE_FUNC_debughandler = debugDrawer;

	StrobeAPI.Constructor( &( *instance )->base );
}

void STROBE_CORE_EXPORTEDFUNC_destructor( void **STROBE_CORE )
{
	STROBE_CORE_t **instance = *(STROBE_CORE_t ***)&STROBE_CORE;

	if ( *instance )
	{
		StrobeAPI.Destructor( &( *instance )->base );
		if ( ( *instance )->private )
		{
			free( ( *instance )->private );
			( *instance )->private = NULL;
		}
		free( *instance );
		*instance = NULL;
	}
}

void STROBE_CORE_EXPORTEDFUNC_reinit( void **STROBE_CORE )
{
	if ( !( *STROBE_CORE ) )
	{
		STROBE_CORE_EXPORTEDFUNC_destructor( STROBE_CORE );
	}
	STROBE_CORE_EXPORTEDFUNC_constructor( STROBE_CORE );
}

void STROBE_CORE_EXPORTEDFUNC_main( void **STROBE_CORE )
{
	STROBE_CORE_t **instance = *(STROBE_CORE_t ***)&STROBE_CORE;
	if ( *instance )
	{
		( *instance )->STROBE_CORE_FUNC_main( instance );
	}
}

#endif
