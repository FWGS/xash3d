
#include "common.h"
#include "client.h"
#include "gl_local.h"

convar_t *r_strobe;
static convar_t *r_strobe_swapinterval;
static convar_t *r_strobe_debug;
static convar_t *r_strobe_cooldown;

typedef enum
{
	STROBE_CT_TotalFrame,
	STROBE_CT_PositiveFrame,
	STROBE_CT_PositiveNormalFrame,
	STROBE_CT_PositiveBlackFrame,
	STROBE_CT_NegativeFrame,
	STROBE_CT_NegativeNormalFrame,
	STROBE_CT_NegativeBlackFrame
} STROBE_counterType;

typedef enum
{
	PHASE_POSITIVE = BIT( 0 ), // Phase: Positive
	PHASE_INVERTED = BIT( 1 ), // Phase: Inverted
	FRAME_RENDER   = BIT( 2 )  // Frame: Rendered
} fstate_e;                // Frame State

struct strobe_s
{
	// protected
	size_t fCounter;                       // Frame counter
	size_t pCounter, pNCounter, pBCounter; // Positive phase counters
	size_t nCounter, nNCounter, nBCounter; // Negative phase counters
	double elapsedTime;
	double deviation;     // deviation
	double cdTimer;       // Cooldown timer
	qboolean cdTriggered; // Cooldown trigger status
	fstate_e frameInfo; // Frame info

	// private
	double nexttime, lasttime, framerate;
	size_t mark;
	size_t PositiveNormal, PositiveBlack, NegativeNormal, NegativeBlack;
	qboolean firstInverted;
	char strobemethod[128];
} strobe;


#ifdef _DEBUG
#define DEVIATION_LIMIT 1.5
#else
#define DEVIATION_LIMIT 0.15
#endif
#define DEVIATION_SIZE 120

struct strobecore_s
{
	double recentTime, recentTime2;
	double delta[DEVIATION_SIZE];
	size_t fCounterSnapshot;
	char debugStr[2048]; // Heap allocation ?
	int offsetX;
	double nexttime, lasttime;
	int strobeInterval;
	int swapInterval;
	double initialTime;
} strobecore;

#define lossCalculator( x, y ) ( ( ( x ) - ( y ) ) * 100.0 / ( x ) )


_inline double Strobe_GetCurrentFPS( void );

_inline double Strobe_StandardDeviation( const double *data, int n )
{
	double mean = 0.0, sum_deviation = 0.0;
	int i;

	for ( i = 0; i < n; ++i )
	{
		mean += data[i];
	}

	mean = mean / n;

	for ( i = 0; i < n; ++i )
		sum_deviation += ( data[i] - mean ) * ( data[i] - mean );

	return sqrt( sum_deviation / n );
}


/*
=====================
Strobe_GenerateBlackFrame

Generates partial or full black frame
=====================
*/
_inline void Strobe_GenerateBlackFrame( float opacity )
{
	if (opacity < 0.0f || opacity > 1.0f)
		opacity = 1.0f;

	if ( CL_IsInConsole() ) // No strobing on the console
	{
		if ( !vid_fullscreen->integer ) // Disable when not fullscreen due to viewport problems
		{
			R_Set2DMode( false );
			return;
		}
		pglEnable( GL_SCISSOR_TEST );
		pglScissor( con_rect.x, ( -con_rect.y ) - ( con_rect.h * 1.25 ), con_rect.w, con_rect.h ); // Preview strobe setting on static
		pglClearColor( 0.0f, 0.0f, 0.0f, opacity );
		pglClear( GL_COLOR_BUFFER_BIT );
		pglDisable( GL_SCISSOR_TEST );
	}
	else
	{
		//pglFlush(); // ?
		pglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
		pglClear( GL_COLOR_BUFFER_BIT );
	}
}


_inline double Strobe_GetCooldown( void )
{
	if ( 0 <= strobe.cdTimer )
	{
		return ( (double)abs( r_strobe_cooldown->integer ) - strobe.cdTimer );
	}
	else
	{
		return 0.0;
	}
}

_inline qboolean Strobe_IsPhaseInverted( void )
{
	if ( strobe.frameInfo & PHASE_INVERTED )
		return true;
	else
		return false;
}

_inline qboolean Strobe_IsNormal( void )
{
	if ( strobe.frameInfo & FRAME_RENDER )
		return true;
	else
		return false;
}

_inline qboolean Strobe_IsPositive( void )
{
	if ( strobe.frameInfo & PHASE_POSITIVE )
		return true;
	else
		return false;
}

_inline double Strobe_GetEffectiveFPS( void )
{
	int strobeInterval = r_strobe->integer;
	double eFPS;

	if ( strobeInterval > 0 )
	{
		eFPS = ( Strobe_GetCurrentFPS() ) / ( strobeInterval + 1 );
	}
	else if ( strobeInterval < 0 )
	{
		strobeInterval = abs( strobeInterval );
		eFPS           = ( Strobe_GetCurrentFPS() * strobeInterval ) / ( strobeInterval + 1 );
	}
	else
		eFPS = 0.0;

	return eFPS;
}


_inline void Strobe_GenerateDiffBar( char *src, int size, char type )
{
	char _barCounter = 0;
	int diff_NB      = 0;
	int diff         = 0;
	int _a, _b;
	qboolean Neg = false;

	switch ( type )
	{
	case ( 0 ): // Positive Difference
	{
		diff_NB = ( strobe.pNCounter - strobe.pBCounter );

		if ( strobe.pCounter )
			diff = round( abs( diff_NB ) * 100 / strobe.pCounter );

		break;
	}
	case ( 1 ): // Negative Difference
	{
		diff_NB = ( strobe.nNCounter - strobe.nBCounter );

		if ( strobe.nCounter )
			diff = round( abs( diff_NB ) * 100 / strobe.nCounter );

		break;
	}
	case ( 2 ): // Difference of difference
	{
		if ( strobe.nCounter && strobe.pCounter )
		{
			_a   = ( abs( strobe.pNCounter - strobe.pBCounter ) * 100 / strobe.pCounter );
			_b   = ( abs( strobe.nNCounter - strobe.nBCounter ) * 100 / strobe.nCounter );
			diff = abs( ( ( (int)( strobe.pNCounter - strobe.pBCounter ) > 0 ) ? _a : ( -_a ) ) - ( ( ( (int)( strobe.nNCounter - strobe.nBCounter ) > 0 ) ? _b : ( -_b ) ) ) ); // Min 0 Max 200
		}
		break;
	}
	default:
		break;
	}

	if ( diff_NB < 0 )
		Neg = true;

	Q_snprintf( src, size, "^3[" );

	for ( _barCounter = 0; _barCounter <= 20; ++_barCounter )
	{
		if ( _barCounter == 10 )
		{
			Q_strcat( src, "O" );
		}
		else if ( _barCounter < 10 )
		{
			if ( type == 2 )
			{
				if ( 100 - ( _barCounter * 11 ) <= diff / 2 )
					Q_strcat( src, "^4=^3" );
				else
					Q_strcat( src, "^3=" );
			}
			else
			{
				if ( Neg )
				{
					if ( 100 - ( _barCounter * 11 ) <= diff )
						Q_strcat( src, "^4=^3" );
					else
						Q_strcat( src, "^3=" );
				}
				else
				{
					Q_strcat( src, "^3=" );
				}
			}
		}
		else if ( _barCounter > 10 )
		{
			if ( type == 2 )
			{
				if ( ( ( _barCounter - 11 ) * 11 ) >= diff / 2 )
					Q_strcat( src, "^3=" );
				else
					Q_strcat( src, "^4=^3" );
			}
			else
			{
				if ( Neg )
				{
					Q_strcat( src, "^3=" );
				}
				else
				{
					if ( ( ( _barCounter - 11 ) * 11 ) >= diff )
						Q_strcat( src, "^3=" );
					else
						Q_strcat( src, "^4=^3" );
				}
			}
		}
	}
	if ( type == 2 )
	{
		Q_strcat( src, va( "] - %d / 200", diff ) );
	}
	else
	{
		Q_strcat( src, va( "] - %d%%", ( Neg ? -diff : diff ) ) );
	}
}


_inline double PWMSim_Frequency( void )
{
	return ( 1 / ( ( 1.0f / Strobe_GetCurrentFPS() ) * ( abs( r_strobe->integer ) + 1 ) ) );
}

_inline double PWMSim_DutyCycle( void )
{
	int strobeInterval = r_strobe->integer;
	return ( ( ( 1.0f / ( abs( strobeInterval ) + 1 ) ) * 100 ) * ( strobeInterval < 0 ? -strobeInterval : 1 ) );
}

_inline double PWMSim_PositivePhaseShift( void )
{
	if ( !!( strobe.frameInfo & PHASE_INVERTED ) )
		return ( 1.0f / Strobe_GetCurrentFPS() ) * 1000;
	else
		return 0.0f;
}

_inline double PWMSim_NegativePhaseShift( void )
{
	if ( !!( strobe.frameInfo & PHASE_INVERTED ) )
		return abs( r_strobe->integer ) * ( 1.0f / Strobe_GetCurrentFPS() ) * 1000;
	else
		return 0.0;
}

_inline double PWMSim_Period( void )
{
	return ( ( ( 1.0f / Strobe_GetCurrentFPS() ) * ( abs( r_strobe->integer ) + 1 ) ) * 1000 );
}

_inline double Strobe_GeometricMean( double x, double y )
{
	return sqrt( abs( x * y ) );
}

_inline double Strobe_ArithmeticMean( double x, double y )
{
	return ( x + y ) / 2;
}


_inline double BRed_ActualBrightnessReduction( void )
{
	return lossCalculator( Strobe_GetCurrentFPS(), Strobe_GetEffectiveFPS() );
}

_inline double BRed_LogarithmicBrightnessReduction( double base )
{
	return lossCalculator( log( base ), log( base * Strobe_GetEffectiveFPS() / Strobe_GetCurrentFPS() ) );
}

_inline double BRed_SquareBrightnessReduction( double base )
{
	return lossCalculator( sqrt( base ), sqrt( base * Strobe_GetEffectiveFPS() / Strobe_GetCurrentFPS() ) );
}

_inline double BRed_CubeBrightnessReduction( double base )
{
	return lossCalculator( cbrt( base ), cbrt( base * Strobe_GetEffectiveFPS() / Strobe_GetCurrentFPS() ) );
}

_inline double BRed_OtherBrightnessReduction( double base, double ( *reductionFunction )( double ) )
{
	return lossCalculator( reductionFunction( base ), reductionFunction( base * Strobe_GetEffectiveFPS() / Strobe_GetCurrentFPS() ) );
}


_inline double Experimental_Badness_Reducted( qboolean PWMInvolved )
{
	double badness, Diff;
	int diffP_NB, diffN_NB;
	double diffP = 0.0, diffN = 0.0;
	diffP_NB = ( strobe.pNCounter - strobe.pBCounter );
	diffN_NB = ( strobe.nNCounter - strobe.nBCounter );

	if ( strobe.pCounter )
		diffP = abs( diffP_NB ) * 100.0 / strobe.pCounter;

	if ( strobe.nCounter )
		diffN = abs( diffN_NB ) * 100.0 / strobe.nCounter;

	if ( diffP_NB < 0.0 )
		diffP = -diffP;
	if ( diffN_NB < 0.0 )
		diffN = -diffN;

	Diff = fabs( diffP - diffN );

	if ( Diff < 0.0 )
		Diff = 0.0;
	else if ( Diff > 200.0 )
		Diff = 200.0;

	badness = -log( ( 200 - Diff ) / ( Diff ) );

	if ( PWMInvolved )
		return ( badness * PWMSim_Period() );
	else
		return badness;
}

_inline double Experimental_Badness( qboolean PWMInvolved )
{
	int diffP_NB, diffN_NB;
	double diffP = 0.0, diffN = 0.0;
	double absoluteDifference = 0.0;
	double badness            = 0.0;

	diffP_NB = ( strobe.pNCounter - strobe.pBCounter );
	diffN_NB = ( strobe.nNCounter - strobe.nBCounter );

	if ( strobe.pCounter )
		diffP = abs( diffP_NB ) * 100.0 / strobe.pCounter;

	if ( strobe.nCounter )
		diffN = abs( diffN_NB ) * 100.0 / strobe.nCounter;

	absoluteDifference = fabs( diffP - diffN );
	if ( absoluteDifference > 100.0 )
		absoluteDifference = 100.0;

	badness =
		-log( ( ( absoluteDifference + Strobe_GeometricMean( ( 100.0 - diffP ), ( 100.0 - diffN ) ) ) / ( absoluteDifference + Strobe_GeometricMean( diffP, diffN ) ) ) );

	if ( PWMInvolved )
		return ( badness * PWMSim_Period() );
	else
		return badness;
}



_inline size_t Strobe_GetFrameCounter( STROBE_counterType type )
{
	switch ( type )
	{
	case ( STROBE_CT_PositiveFrame ):
	{
		return strobe.pCounter;
		break;
	}
	case ( STROBE_CT_PositiveNormalFrame ):
	{
		return strobe.pNCounter;
		break;
	}
	case ( STROBE_CT_PositiveBlackFrame ):
	{
		return strobe.pBCounter;
		break;
	}
	case ( STROBE_CT_NegativeFrame ):
	{
		return strobe.nCounter;
		break;
	}
	case ( STROBE_CT_NegativeNormalFrame ):
	{
		return strobe.nNCounter;
		break;
	}
	case ( STROBE_CT_NegativeBlackFrame ):
	{
		return strobe.nBCounter;
		break;
	}
	case ( STROBE_CT_TotalFrame ):
	default:
		return strobe.fCounter;
		break;
	}
}

_inline double Strobe_GetCurrentFPS( void )
{
	// Copied from SCR_DrawFps
	// This way until current fps becomes global!!!

	double newtime;

	newtime = Sys_DoubleTime();
	if ( newtime >= strobe.nexttime )
	{
		strobe.framerate = ( strobe.fCounter - strobe.mark ) / ( newtime - strobe.lasttime );
		strobe.lasttime = newtime;
		strobe.nexttime  = max(strobe.nexttime + 0.35, strobe.lasttime - 0.35 );
		strobe.mark      = strobe.fCounter;
	}

	return strobe.framerate;
}

_inline void Strobe_GenerateDebugStatistics( char *src, int size )
{
	char diffBarP[128], diffBarN[128], diffBarT[128];
	size_t nPositiveNormal, nPositiveBlack, nNegativeNormal, nNegativeBlack;
	int diffP_NB, diffN_NB;
	double diffP = 0.0, diffN = 0.0;
	double cooldown = strobe.cdTimer;
	int strobeMethod = r_strobe->integer;

	diffP_NB = ( strobe.pNCounter - strobe.pBCounter );
	diffN_NB = ( strobe.nNCounter - strobe.nBCounter );

	if ( strobe.pCounter )
		diffP = abs( diffP_NB ) * 100.0 / strobe.pCounter; // round( abs( diffP_NB ) * 100 / strobe.pCounter );

	if ( strobe.nCounter )
		diffN = abs( diffN_NB ) * 100.0 / strobe.nCounter; // round( abs( diffN_NB ) * 100 / strobe.nCounter );

	Strobe_GenerateDiffBar( diffBarP, sizeof( diffBarP ), 0 );
	Strobe_GenerateDiffBar( diffBarN, sizeof( diffBarN ), 1 );
	Strobe_GenerateDiffBar( diffBarT, sizeof( diffBarT ), 2 );

	nPositiveNormal = Strobe_GetFrameCounter( STROBE_CT_PositiveNormalFrame );
	nPositiveBlack  = Strobe_GetFrameCounter( STROBE_CT_PositiveBlackFrame );
	nNegativeNormal = Strobe_GetFrameCounter( STROBE_CT_NegativeNormalFrame );
	nNegativeBlack  = Strobe_GetFrameCounter( STROBE_CT_NegativeBlackFrame );


	if (!strlen(strobe.strobemethod))
	{
		Q_snprintf(strobe.strobemethod, sizeof(strobe.strobemethod), (strobeMethod > 0 ? "%d [NORMAL" : "%d [BLACK"), strobeMethod);
		int _k;
		for (_k = 1; _k <= abs(strobeMethod); ++_k)
		{
			Q_strcat(strobe.strobemethod, (strobeMethod > 0 ? " - BLACK" : " - NORMAL"));
		}
		Q_strcat(strobe.strobemethod, "]");
	}

	Q_snprintf( src,
				size,
				"%.2f FPS\n%.2f eFPS\n"
				"Strobe Method: %s\n"
				"Strobe Swap Interval: %d second(s)\n"
				"Strobe Cooldown Delay: %d second(s)\n"
				"Elapsed Time: %.2f second(s)\n"
				"isPhaseInverted = %d\n"
				"Total Frame Count: %zu\n"
				"^7(+) Phase Frame Count: %zu\n"
				"%s\n"
				"%s\n"
				"(-) Phase Frame Count: %zu\n"
				"%s\n"
				"%s\n"
				"^5=====ANALYSIS=====\n^3"
				"PWM Simulation:\n"
				" |-> Frequency: %.2f Hz\n"
				" |-> Duty Cycle: %.2f%%\n"
				" |-> Current Phase Shift: +%.2f msec || -%.2f msec\n"
				" |-> Period: %.2f msec\n"
				"Brightness Reduction:\n"
				" |-> [^7LINEAR^3] Actual Reduction: %.2f%%\n"
				" |-> [^7LOG^3] Realistic Reduction (400 cd/m2 base): %.2f%%\n"
				" |-> [^7SQUARE^3] Realistic Reduction (400 cd/m2 base): %.2f%%\n"
				" |-> [^7CUBE^3] Realistic Reduction (400 cd/m2 base): %.2f%%\n"
				"Difference (+): %s\nDifference (-): %s\nDifference (Total): %s\n"
				"Geometric Mean: %.4f\n"
				"G/A Difference: %.4f\n"
				"[^7EXPERIMENTAL^3] Badness: %.4f\n"
				"[^7EXPERIMENTAL^3] Badness x PWM Period: %.4f\n"
				"[^7EXPERIMENTAL^3] Badness (Reduced): %.4f\n"
				"[^7EXPERIMENTAL^3] Badness (Reduced) x PWM Period: %.4f\n"
				"Stability:\n"
				" |-> Standard Deviation: %.3f\n"
				" |-> Cooldown: %s\n"
				"^5=====ANALYSIS=====\n^3",
				Strobe_GetCurrentFPS(),
				Strobe_GetEffectiveFPS(),
				strobe.strobemethod,
				r_strobe_swapinterval->integer,
				r_strobe_cooldown->integer,
				strobe.elapsedTime,
				Strobe_IsPhaseInverted(),
				Strobe_GetFrameCounter( STROBE_CT_TotalFrame ),
				Strobe_GetFrameCounter( STROBE_CT_PositiveFrame ),
				( nPositiveNormal > strobe.PositiveNormal ? va( "^2|-> Normal Frame Count: %zu^7", nPositiveNormal ) : va( "|-> Normal Frame Count: %zu", nPositiveNormal ) ), // Should be white instead of ^7 but white is not available in the color table
				( nPositiveBlack > strobe.PositiveBlack ? va( "^2|-> Black Frame Count: %zu^7", nPositiveBlack ) : va( "|-> Black Frame Count: %zu", nPositiveBlack ) ),
				Strobe_GetFrameCounter( STROBE_CT_NegativeFrame ),
				( nNegativeNormal > strobe.NegativeNormal ? va( "^2|-> Normal Frame Count: %zu^7", nNegativeNormal ) : va( "|-> Normal Frame Count: %zu", nNegativeNormal ) ),
				( nNegativeBlack > strobe.NegativeBlack ? va( "^2|-> Black Frame Count: %zu^7", nNegativeBlack ) : va( "|-> Black Frame Count: %zu", nNegativeBlack ) ),
				PWMSim_Frequency(),
				PWMSim_DutyCycle(),
				PWMSim_PositivePhaseShift(),
				PWMSim_NegativePhaseShift(),
				PWMSim_Period(),
				BRed_ActualBrightnessReduction(),
				BRed_LogarithmicBrightnessReduction( 400.0 ),
				BRed_SquareBrightnessReduction( 400.0 ),
				BRed_CubeBrightnessReduction( 400.0 ),
				diffBarP,
				diffBarN,
				diffBarT,
				Strobe_GeometricMean( diffP, diffN ),
				Strobe_ArithmeticMean( diffP, diffN ) - Strobe_GeometricMean( diffP, diffN ),
				Experimental_Badness( false ),
				Experimental_Badness( true ),
				Experimental_Badness_Reducted( false ),
				Experimental_Badness_Reducted( true ),
				strobe.deviation,
				( cooldown >= 0.0 && strobe.cdTriggered ? va( "^1 %.2f secs\n[STROBING DISABLED] ^3", (double)r_strobe_cooldown->integer - cooldown ) : "0" ) );

	strobe.PositiveNormal = Strobe_GetFrameCounter( STROBE_CT_PositiveNormalFrame );
	strobe.PositiveBlack  = Strobe_GetFrameCounter( STROBE_CT_PositiveBlackFrame );
	strobe.NegativeNormal = Strobe_GetFrameCounter( STROBE_CT_NegativeNormalFrame );
	strobe.NegativeBlack  = Strobe_GetFrameCounter( STROBE_CT_NegativeBlackFrame );
}


_inline void Strobe_ProcessFrame( void )
{
	static qboolean phase = 0;
	if (phase != Strobe_IsPhaseInverted() && (r_strobe->integer == 1 || r_strobe->integer == -1) && !strobe.cdTriggered)
		strobe.firstInverted = true;
	else
		strobe.firstInverted = false;

	if (strobe.cdTriggered)
	{
		strobe.frameInfo = FRAME_RENDER | (strobe.frameInfo & PHASE_POSITIVE);
	}

	if ( strobe.frameInfo & FRAME_RENDER ) // Show normal
	{
		if ( strobe.frameInfo & PHASE_POSITIVE )
			++strobe.pNCounter;
		else
			++strobe.nNCounter;

		if(!strobe.firstInverted)
			R_Set2DMode( false );
	}
	else // Show black
	{
		if ( strobe.frameInfo & PHASE_POSITIVE )
			++strobe.pBCounter;
		else
			++strobe.nBCounter;

		if( !strobe.firstInverted )
			Strobe_GenerateBlackFrame( 1.0f );
	}

	if( strobe.firstInverted)
	{
		R_Set2DMode( false );
		Strobe_GenerateBlackFrame( 0.8f );
	}

	phase = Strobe_IsPhaseInverted();
	++strobe.fCounter;
}



void R_StrobeDrawDebug( void )
{
	rgba_t color;
	int offsetY;
	int fixer;
	double newtime;
	qboolean strobeDebug = r_strobe_debug->integer ? true : false;

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
		if ( newtime >= strobecore.nexttime )
		{
			Strobe_GenerateDebugStatistics( strobecore.debugStr, sizeof( strobecore.debugStr ) );
			strobecore.lasttime = newtime;
			strobecore.nexttime = max( strobecore.nexttime + 0.15, strobecore.lasttime - 0.15 ); // Make this configurable
		}
	}
	else if ( cl_showfps->integer )
	{
		Q_snprintf( strobecore.debugStr, sizeof( strobecore.debugStr ), "%3d eFPS", (int)round( Strobe_GetEffectiveFPS() ) );
	}

	MakeRGBA( color, 255, 255, 255, 255 );
	Con_DrawStringLen( strobecore.debugStr, &fixer, &offsetY );
	if ( strobeDebug )
		Con_DrawString( scr_width->integer - strobecore.offsetX - 50, 4, strobecore.debugStr, color );
	else
		Con_DrawString( scr_width->integer - strobecore.offsetX - 2, offsetY + 8, strobecore.debugStr, color );
	if ( abs( fixer - strobecore.offsetX ) > 50 || strobecore.offsetX == 0 ) // 50 is for 1080p ! Needs to be dynamic !
		strobecore.offsetX = fixer;
}

static void Strobe_Reset()
{
	Q_memset( &strobecore, 0, sizeof( strobecore ));
	Q_memset( &strobe, 0, sizeof( strobe ));
	strobe.frameInfo = ( PHASE_POSITIVE | FRAME_RENDER );
	strobecore.initialTime = Sys_DoubleTime();
}

void R_Strobe( void )
{
	double delta, delta2;
	double currentTime          = Sys_DoubleTime( );
	delta2                      = currentTime - strobecore.recentTime2;
	strobecore.recentTime2 = currentTime;
	strobe.elapsedTime = currentTime - strobecore.initialTime;

	if ( CL_IsInMenu( ) )
	{
		R_Set2DMode( false );
		return;
	}

	if ( strobe.cdTimer >= 0.0 && delta2 > 0.0 )
		strobe.cdTimer += delta2;
	if ( strobe.fCounter - strobecore.fCounterSnapshot == 1 )
	{
		strobecore.delta[strobe.fCounter % ARRAYSIZE( strobecore.delta )] = Strobe_GetCurrentFPS();
		strobe.deviation = Strobe_StandardDeviation( strobecore.delta, ARRAYSIZE( strobecore.delta ) );
	}
	strobecore.fCounterSnapshot = strobe.fCounter;

	if ( r_strobe_cooldown->integer > 0 )
	{
		if ( ( strobe.cdTimer > (double)abs( r_strobe_cooldown->integer ) ) && strobe.cdTriggered == true )
		{
			strobe.cdTriggered = false;
			strobe.cdTimer     = -1.0;
		}

		if ( strobe.fCounter > ARRAYSIZE( strobecore.delta ) )
		{
			if ( strobe.deviation > DEVIATION_LIMIT )
			{
				strobe.cdTriggered = true;
				strobe.cdTimer     = 0.0;
			}
		}
	}
	else
	{
		strobe.cdTriggered = false;
	}

	if ( ( ( strobecore.strobeInterval != r_strobe->integer ) && ( strobecore.strobeInterval ) ) ||
		 /*((swapInterval != r_strobe_swapinterval->integer) && (swapInterval != 0)) || */
		 strobe.fCounter == UINT_MAX )
	{
		Strobe_Reset();
		R_Strobe();
	}

	strobecore.strobeInterval = r_strobe->integer;
	strobecore.swapInterval   = r_strobe_swapinterval->integer;

	if ( ( strobecore.strobeInterval == 0 ) ||
		 ( ( gl_swapInterval->integer == 0 ) && ( strobecore.strobeInterval ) ) )
	{
		if ( !gl_swapInterval->integer )
			MsgDev( D_WARN, "Strobing requires V-SYNC not being turned off! (gl_swapInterval != 0) \n" );

		if ( strobecore.strobeInterval ) // If v-sync is off, turn off strobing
		{
			Cvar_Set( "r_strobe", "0" );
		}
		strobe.fCounter = 0;

		R_Set2DMode( false );
		return;
	}

	if ( ( strobe.fCounter % 2 ) == 0 )
	{
		++strobe.pCounter;
		strobe.frameInfo |= PHASE_POSITIVE;
	}
	else
	{
		++strobe.nCounter;
		strobe.frameInfo &= ~PHASE_POSITIVE;
	}

	if ( strobecore.swapInterval < 0 )
		strobecore.swapInterval = abs( strobecore.swapInterval );

	if ( ( strobecore.swapInterval ) && ( strobecore.strobeInterval % 2 ) ) // Swapping not enabled for even intervals as it is neither necessary nor works as intended
	{
		delta = currentTime - strobecore.recentTime;                                                                                      // New Currenttime for _delta ?
		if ( ( delta >= (double)( strobecore.swapInterval ) ) && ( delta < (double)( 2 * strobecore.swapInterval ) ) ) // Basic timer
		{
			strobe.frameInfo |= PHASE_INVERTED;
		}
		else if ( delta < (double)( strobecore.swapInterval ) )
		{
			strobe.frameInfo &= ~PHASE_INVERTED;
		}
		else //if (delta >= (double)(2 * swapInterval))
		{
			strobecore.recentTime = currentTime;
		}
	}

	switch ( strobe.frameInfo & ( PHASE_POSITIVE | PHASE_INVERTED ) )
	{
	case ( PHASE_POSITIVE | PHASE_INVERTED ):
		if ( ( abs( strobecore.strobeInterval ) % 2 ) == 0 )
			strobe.frameInfo = ( ( ( strobe.pCounter - 1 ) % ( abs( strobecore.strobeInterval ) + 1 ) ) == ( abs( strobecore.strobeInterval ) / 2 ) ) ? strobe.frameInfo | FRAME_RENDER : strobe.frameInfo & ~FRAME_RENDER; //even
		else
			strobe.frameInfo &= ~FRAME_RENDER;
		break;

	case ( PHASE_POSITIVE & ~PHASE_INVERTED ):
		if ( abs( strobecore.strobeInterval ) % 2 == 0 )
			strobe.frameInfo = ( ( ( strobe.pCounter - 1 ) % ( abs( strobecore.strobeInterval ) + 1 ) ) == 0 ) ? strobe.frameInfo | FRAME_RENDER : strobe.frameInfo & ~FRAME_RENDER; //even
		else
		{
			if ( abs( strobecore.strobeInterval ) == 1 )
				strobe.frameInfo |= FRAME_RENDER;
			else
				strobe.frameInfo = ( ( ( strobe.pCounter - 1 ) % ( ( abs( strobecore.strobeInterval ) + 1 ) / 2 ) ) == 0 ) ? strobe.frameInfo | FRAME_RENDER : strobe.frameInfo & ~FRAME_RENDER; //odd
		}
		break;

	case ( ~PHASE_POSITIVE & PHASE_INVERTED ):
		if ( abs( strobecore.strobeInterval ) % 2 == 0 )
			strobe.frameInfo = ( ( ( strobe.nCounter - 1 ) % ( abs( strobecore.strobeInterval ) + 1 ) ) == 0 ) ? strobe.frameInfo | FRAME_RENDER : strobe.frameInfo & ~FRAME_RENDER; //even
		else
		{
			if ( abs( strobecore.strobeInterval ) == 1 )
				strobe.frameInfo |= FRAME_RENDER;
			else
				strobe.frameInfo = ( ( ( strobe.nCounter - 1 ) % ( ( abs( strobecore.strobeInterval ) + 1 ) / 2 ) ) == 0 ) ? strobe.frameInfo | FRAME_RENDER : strobe.frameInfo & ~FRAME_RENDER; //odd
		}
		break;

	case 0:
		if ( ( abs( strobecore.strobeInterval ) % 2 ) == 0 )
			strobe.frameInfo = ( ( ( strobe.nCounter - 1 ) % ( abs( strobecore.strobeInterval ) + 1 ) ) == ( abs( strobecore.strobeInterval ) / 2 ) ) ? strobe.frameInfo | FRAME_RENDER : strobe.frameInfo & ~FRAME_RENDER; //even
		else
			strobe.frameInfo &= ~FRAME_RENDER;
		break;

	default:
		strobe.frameInfo = ( PHASE_POSITIVE | FRAME_RENDER );
	}

	if ( strobecore.strobeInterval < 0 )
		strobe.frameInfo ^= FRAME_RENDER;

	Strobe_ProcessFrame();
}


void R_InitStrobe( void )
{
	r_strobe              = Cvar_Get( "r_strobe", "0", CVAR_ARCHIVE, "black frame replacement algorithm interval" );
	r_strobe_swapinterval = Cvar_Get( "r_strobe_swapinterval", "0", CVAR_ARCHIVE, "swapping phase interval (seconds)" );
	r_strobe_debug        = Cvar_Get( "r_strobe_debug", "0", CVAR_ARCHIVE, "show strobe debug information" );
	r_strobe_cooldown     = Cvar_Get( "r_strobe_cooldown", "3", CVAR_ARCHIVE, "strobe cooldown period in seconds" );
	Strobe_Reset();
}
