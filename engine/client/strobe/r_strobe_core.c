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
#include "r_strobe_api_protected_.h"

#ifdef _this
#undef _this
#endif
#define _this STROBE_IMPL_THIS( STROBE_CORE )

struct STROBE_IMPL_STRUCT( STROBE_CORE ) *STROBE_CORE = NULL;

struct STROBE_IMPL_PRIVATE_STRUCT( STROBE_CORE )
{
	double recentTime, recentTime2;
	double delta[STROBE_CORE_DEVIATION_SIZE];
	size_t fCounterSnapshot;
	char debugStr[2048]; // Heap allocation ?
	int offsetX;
	double nexttime, lasttime;
	int strobeInterval;
	int swapInterval;
	double initialTime;
};

/*
===============
R_Strobe

TODO:
* Make timers global (at StrobeAPI level).
* Make swapping transition seamless by rendering non-opaque frames.
* Implement high precision timer to keep the internal phase on track with monitor. (In case of freezes etc.)
===============
*/
static void R_Strobe( STROBE_IMPL_THIS_PARAM( STROBE_CORE ) )
{
	double delta, delta2;
	double currentTime          = Sys_DoubleTime( );
	delta2                      = currentTime - _this->private->recentTime2;
	_this->private->recentTime2 = currentTime;
	_this->base.protected->elapsedTime = currentTime - _this->base.protected->initialTime;

	if ( CL_IsInMenu( ) )
	{
		R_Set2DMode( false );
		return;
	}

	if ( _this->base.protected->cdTimer >= 0.0 && delta2 > 0.0 )
		_this->base.protected->cdTimer += delta2;
	if ( _this->base.protected->fCounter - _this->private->fCounterSnapshot == 1 )
	{
		_this->private->delta[_this->base.protected->fCounter % ARRAYSIZE( _this->private->delta )] = delta2;
		_this->base.protected->deviation                                                            = _this->base.Helpers.StandardDeviation( _this->private->delta, ARRAYSIZE( _this->private->delta ) ) * 1000;
	}
	_this->private->fCounterSnapshot = _this->base.protected->fCounter;

	if ( StrobeAPI.r_strobe_cooldown->integer > 0 )
	{
		if ( ( _this->base.protected->cdTimer > (double)abs( StrobeAPI.r_strobe_cooldown->integer ) ) && _this->base.protected->cdTriggered == true )
		{
			_this->base.protected->cdTriggered = false;
			_this->base.protected->cdTimer     = -1.0;
		}

		if ( _this->base.protected->fCounter > ARRAYSIZE( _this->private->delta ) )
		{
			if ( _this->base.protected->deviation > STROBE_CORE_DEVIATION_LIMIT )
			{
				_this->base.protected->cdTriggered = true;
				_this->base.protected->cdTimer     = 0.0;
			}
		}
	}
	else
	{
		_this->base.protected->cdTriggered = false;
	}

	if ( ( ( _this->private->strobeInterval != StrobeAPI.r_strobe->integer ) && ( _this->private->strobeInterval ) ) ||
	     /*((swapInterval != r_strobe_swapinterval->integer) && (swapInterval != 0)) || */
	     _this->base.protected->fCounter == UINT_MAX )
	{
		STROBE_IMPL_EXPORTEDFUNC_reinit( STROBE_CORE )( *(void ***)&_STROBE_IMPL_THIS );
		R_Strobe( &_this );
	}

	_this->private->strobeInterval = StrobeAPI.r_strobe->integer;
	_this->private->swapInterval   = StrobeAPI.r_strobe_swapinterval->integer;

	if ( ( _this->private->strobeInterval == 0 ) ||
	     ( ( gl_swapInterval->integer == 0 ) && ( _this->private->strobeInterval ) ) )
	{
		if ( !gl_swapInterval->integer )
			MsgDev( D_WARN, "Strobing requires V-SYNC not being turned off! (gl_swapInterval != 0) \n" );

		if ( _this->private->strobeInterval ) // If v-sync is off, turn off strobing
		{
			Cvar_Set( "r_strobe", "0" );
		}
		_this->base.protected->fCounter = 0;

		R_Set2DMode( false );
		return;
	}

	if ( ( _this->base.protected->fCounter % 2 ) == 0 )
	{
		++_this->base.protected->pCounter;
		_this->base.protected->frameInfo |= PHASE_POSITIVE;
	}
	else
	{
		++_this->base.protected->nCounter;
		_this->base.protected->frameInfo &= ~PHASE_POSITIVE;
	}

	if ( _this->private->swapInterval < 0 )
		_this->private->swapInterval = abs( _this->private->swapInterval );

	if ( ( _this->private->swapInterval ) && ( _this->private->strobeInterval % 2 ) ) // Swapping not enabled for even intervals as it is neither necessary nor works as intended
	{
		delta = currentTime - _this->private->recentTime;                                                                                      // New Currenttime for _delta ?
		if ( ( delta >= (double)( _this->private->swapInterval ) ) && ( delta < (double)( 2 * _this->private->swapInterval ) ) ) // Basic timer
		{
			_this->base.protected->frameInfo |= PHASE_INVERTED;
		}
		else if ( delta < (double)( _this->private->swapInterval ) )
		{
			_this->base.protected->frameInfo &= ~PHASE_INVERTED;
		}
		else //if (delta >= (double)(2 * swapInterval))
		{
			_this->private->recentTime = currentTime;
		}
	}

	switch ( _this->base.protected->frameInfo & ( PHASE_POSITIVE | PHASE_INVERTED ) )
	{
	case ( PHASE_POSITIVE | PHASE_INVERTED ):
		if ( ( abs( _this->private->strobeInterval ) % 2 ) == 0 )
			_this->base.protected->frameInfo = ( ( ( _this->base.protected->pCounter - 1 ) % ( abs( _this->private->strobeInterval ) + 1 ) ) == ( abs( _this->private->strobeInterval ) / 2 ) ) ? _this->base.protected->frameInfo | FRAME_RENDER : _this->base.protected->frameInfo & ~FRAME_RENDER; //even
		else
			_this->base.protected->frameInfo &= ~FRAME_RENDER;
		break;

	case ( PHASE_POSITIVE & ~PHASE_INVERTED ):
		if ( abs( _this->private->strobeInterval ) % 2 == 0 )
			_this->base.protected->frameInfo = ( ( ( _this->base.protected->pCounter - 1 ) % ( abs( _this->private->strobeInterval ) + 1 ) ) == 0 ) ? _this->base.protected->frameInfo | FRAME_RENDER : _this->base.protected->frameInfo & ~FRAME_RENDER; //even
		else
		{
			if ( abs( _this->private->strobeInterval ) == 1 )
				_this->base.protected->frameInfo |= FRAME_RENDER;
			else
				_this->base.protected->frameInfo = ( ( ( _this->base.protected->pCounter - 1 ) % ( ( abs( _this->private->strobeInterval ) + 1 ) / 2 ) ) == 0 ) ? _this->base.protected->frameInfo | FRAME_RENDER : _this->base.protected->frameInfo & ~FRAME_RENDER; //odd
		}
		break;

	case ( ~PHASE_POSITIVE & PHASE_INVERTED ):
		if ( abs( _this->private->strobeInterval ) % 2 == 0 )
			_this->base.protected->frameInfo = ( ( ( _this->base.protected->nCounter - 1 ) % ( abs( _this->private->strobeInterval ) + 1 ) ) == 0 ) ? _this->base.protected->frameInfo | FRAME_RENDER : _this->base.protected->frameInfo & ~FRAME_RENDER; //even
		else
		{
			if ( abs( _this->private->strobeInterval ) == 1 )
				_this->base.protected->frameInfo |= FRAME_RENDER;
			else
				_this->base.protected->frameInfo = ( ( ( _this->base.protected->nCounter - 1 ) % ( ( abs( _this->private->strobeInterval ) + 1 ) / 2 ) ) == 0 ) ? _this->base.protected->frameInfo | FRAME_RENDER : _this->base.protected->frameInfo & ~FRAME_RENDER; //odd
		}
		break;

	case 0:
		if ( ( abs( _this->private->strobeInterval ) % 2 ) == 0 )
			_this->base.protected->frameInfo = ( ( ( _this->base.protected->nCounter - 1 ) % ( abs( _this->private->strobeInterval ) + 1 ) ) == ( abs( _this->private->strobeInterval ) / 2 ) ) ? _this->base.protected->frameInfo | FRAME_RENDER : _this->base.protected->frameInfo & ~FRAME_RENDER; //even
		else
			_this->base.protected->frameInfo &= ~FRAME_RENDER;
		break;

	default:
		_this->base.protected->frameInfo = ( PHASE_POSITIVE | FRAME_RENDER );
	}

	if ( _this->private->strobeInterval < 0 )
		_this->base.protected->frameInfo ^= FRAME_RENDER;

	_this->base.ProcessFrame( &_this->base );
}

_inline void debugDrawer( STROBE_IMPL_THIS_PARAM( STROBE_CORE ) )
{
	rgba_t color;
	int offsetY;
	int fixer;
	double newtime;
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
		newtime = Sys_DoubleTime( );
		if ( newtime >= _this->private->nexttime )
		{
			_this->base.Helpers.GenerateDebugStatistics( &_this->base, _this->private->debugStr, ARRAYSIZE( _this->private->debugStr ) );
			_this->private->lasttime = newtime;
			_this->private->nexttime = max( _this->private->nexttime + 0.15, _this->private->lasttime - 0.15 ); // Make this configurable
		}
	}
	else if ( cl_showfps->integer )
	{
		Q_snprintf( _this->private->debugStr, sizeof( _this->private->debugStr ), "%3d eFPS", (int)round( _this->base.Helpers.effectiveFPS( &_this->base ) ) );
	}

	MakeRGBA( color, 255, 255, 255, 255 );
	Con_DrawStringLen( _this->private->debugStr, &fixer, &offsetY );
	if ( strobeDebug )
		Con_DrawString( scr_width->integer - _this->private->offsetX - 50, 4, _this->private->debugStr, color );
	else
		Con_DrawString( scr_width->integer - _this->private->offsetX - 2, offsetY + 8, _this->private->debugStr, color );
	if ( abs( fixer - _this->private->offsetX ) > 50 || _this->private->offsetX == 0 ) // 50 is for 1080p ! Needs to be dynamic !
		_this->private->offsetX = fixer;
}

void STROBE_IMPL_EXPORTEDFUNC_constructor( STROBE_CORE )( void **STROBE_CORE )
{
	struct STROBE_IMPL_STRUCT( STROBE_CORE ) **instance = *(struct STROBE_IMPL_STRUCT( STROBE_CORE ) ***)&STROBE_CORE;

	*instance                                    = (struct STROBE_IMPL_STRUCT( STROBE_CORE ) *)malloc( sizeof(struct STROBE_IMPL_STRUCT(STROBE_CORE)) );
	( *instance )->private                       = (struct STROBE_IMPL_PRIVATE_STRUCT( STROBE_CORE ) *)calloc( 1, sizeof(struct STROBE_IMPL_PRIVATE_STRUCT(STROBE_CORE)) );
	( *instance )->private->initialTime =		Sys_DoubleTime();
	( *instance )->STROBE_IMPL_FUNC_MAIN         = R_Strobe;
	( *instance )->STROBE_IMPL_FUNC_DEBUGHANDLER = debugDrawer;

	StrobeAPI.Constructor( &( *instance )->base );
}

void STROBE_IMPL_EXPORTEDFUNC_destructor( STROBE_CORE )( void **STROBE_CORE )
{
	struct STROBE_IMPL_STRUCT( STROBE_CORE ) **instance = *(struct STROBE_IMPL_STRUCT( STROBE_CORE ) ***)&STROBE_CORE;

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

void STROBE_IMPL_EXPORTEDFUNC_reinit( STROBE_CORE )( void **STROBE_CORE )
{
	if ( !( *STROBE_CORE ) )
	{
		STROBE_IMPL_EXPORTEDFUNC_destructor( STROBE_CORE )( STROBE_CORE );
	}
	STROBE_IMPL_EXPORTEDFUNC_constructor( STROBE_CORE )( STROBE_CORE );
}

void STROBE_IMPL_EXPORTEDFUNC_main( STROBE_CORE )( void **STROBE_CORE )
{
	struct STROBE_IMPL_STRUCT( STROBE_CORE ) **instance = *(struct STROBE_IMPL_STRUCT( STROBE_CORE ) ***)&STROBE_CORE;
	if ( *instance )
	{
		( *instance )->STROBE_IMPL_FUNC_MAIN( instance );
	}
}

#undef _this

#endif
