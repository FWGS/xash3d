
#include "common.h"

#include "client.h"
#include "gl_local.h"

#define DEVIATION_LIMIT 1.25
#define DEVIATION_SIZE 120

#define lossCalculator( x, y ) ( ( ( x ) - ( y ) ) * 100.0 / ( x ) )

convar_t *r_strobe;
static convar_t *r_strobe_swapinterval;
static convar_t *r_strobe_debug;
static convar_t *r_strobe_cooldown;

typedef enum
{
	TotalFrame,
	PositiveFrame,
	PositiveNormalFrame,
	PositiveBlackFrame,
	NegativeFrame,
	NegativeNormalFrame,
	NegativeBlackFrame
} CountType;

typedef enum
{
	PHASE_POSITIVE = BIT( 0 ),
	PHASE_INVERTED = BIT( 1 ),
	FRAME_RENDER   = BIT( 2 )
} FrameState; // Frame State

typedef struct
{
	qboolean cdTriggered; // Cooldown trigger status

	FrameState frameState;

	int fCounter;                       // Frame counter
	int pCounter, pNCounter, pBCounter; // Positive phase counters
	int nCounter, nNCounter, nBCounter; // Negative phase counters
	int strobeInterval;
	int swapInterval;

	double elapsedTime;
	double deviation;
	double cdTimer; // Cooldown timer
	double initialTime;
} Strobe;

static Strobe strobe;

static double _standardDeviation( double *data, int n )
{
	double mean = 0.0, sum_deviation = 0.0;
	int i;

	for ( i = 0; i < n; ++i )
	{
		mean += data[i];
	}

	mean /= n;

	for ( i = 0; i < n; ++i )
		sum_deviation += ( data[i] - mean ) * ( data[i] - mean );

	return sqrt( sum_deviation / n );
}

static void _makeFrameBlack( float opacity )
{
	if ( opacity < 0.0f || opacity > 1.0f )
		opacity = 1.0f;

	if ( CL_IsInConsole( ) ) // No strobing on the console
	{

		pglEnable( GL_SCISSOR_TEST );
		pglScissor( con_rect.x, ( -con_rect.y ) - ( con_rect.h * 1.25 ), con_rect.w, con_rect.h ); // Preview strobe setting on static
		pglClearColor( 0.0f, 0.0f, 0.0f, opacity );
		pglClear( GL_COLOR_BUFFER_BIT );
		pglDisable( GL_SCISSOR_TEST );
	}
	else
	{
		pglClearColor( 0.0f, 0.0f, 0.0f, opacity );
		pglClear( GL_COLOR_BUFFER_BIT );
	}
}

_inline double _cooldown( void )
{
	int cooldown = r_strobe_cooldown->integer; 
	if ( 0 < strobe.cdTimer )
	{
		return ( (double)abs( cooldown ) - strobe.cdTimer );
	}
	else
	{
		return cooldown;
	}
}

_inline qboolean __checkState( int i )
{
	if ( strobe.frameState & i )
		return true;
	else
		return false;
}

_inline qboolean _isPhaseInverted( void )
{
	return __checkState( PHASE_INVERTED );
}

_inline qboolean _isNormal( void )
{
	return __checkState( FRAME_RENDER );
}

_inline qboolean _isPositive( void )
{
	return __checkState( PHASE_POSITIVE );
}

static double _currentFPS( void )
{
	static double oldTime = 0;
	static int mark       = 0;
	static double oldVal  = 0;
	double val;
	double curTime = Sys_DoubleTime( );

	double diff = curTime - oldTime;

	if ( diff > 0.5 )
	{
		val     = ( strobe.fCounter - mark ) / ( diff );
		oldTime = curTime;
		mark    = strobe.fCounter;
	}
	else
	{
		val = oldVal;
	}

	oldVal = val;

	if ( val < 0.0 )
		val = 0.0;

	return val;
}

static double _effectiveFPS( void )
{
	int strobeInterval = strobe.strobeInterval;
	double eFPS;

	if ( strobeInterval > 0 )
	{
		eFPS = ( _currentFPS( ) ) / ( strobeInterval + 1 );
	}
	else if ( strobeInterval < 0 )
	{
		strobeInterval = abs( strobeInterval );
		eFPS           = ( _currentFPS( ) * strobeInterval ) / ( strobeInterval + 1 );
	}
	else
	{
		eFPS = 0.0;
	}

	return eFPS;
}

static void _generateDiffBar( char *dst, int size, char type )
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
			_a   = ( (strobe.pNCounter - strobe.pBCounter) * 100 / strobe.pCounter );
			_b   = ( (strobe.nNCounter - strobe.nBCounter) * 100 / strobe.nCounter );
			diff = abs( _a - _b );
		}
		break;
	}
	default:
		break;
	}

	if ( diff_NB < 0 )
		Neg = true;

	Q_snprintf( dst, size, "^3[" );

	for ( _barCounter = 0; _barCounter <= 20; ++_barCounter )
	{
		if ( _barCounter == 10 )
		{
			Q_strcat( dst, "O" );
		}
		else if ( _barCounter < 10 )
		{
			if ( type == 2 )
			{
				if ( 100 - ( _barCounter * 11 ) <= diff / 2 )
					Q_strcat( dst, "^4=^3" );
				else
					Q_strcat( dst, "^3=" );
			}
			else
			{
				if ( Neg )
				{
					if ( 100 - ( _barCounter * 11 ) <= diff )
						Q_strcat( dst, "^4=^3" );
					else
						Q_strcat( dst, "^3=" );
				}
				else
				{
					Q_strcat( dst, "^3=" );
				}
			}
		}
		else if ( _barCounter > 10 )
		{
			if ( type == 2 )
			{
				if ( ( ( _barCounter - 11 ) * 11 ) >= diff / 2 )
					Q_strcat( dst, "^3=" );
				else
					Q_strcat( dst, "^4=^3" );
			}
			else
			{
				if ( Neg )
				{
					Q_strcat( dst, "^3=" );
				}
				else
				{
					if ( ( ( _barCounter - 11 ) * 11 ) >= diff )
						Q_strcat( dst, "^3=" );
					else
						Q_strcat( dst, "^4=^3" );
				}
			}
		}
	}

	Q_strcat( dst, "]" );
}

_inline double _frequency( void )
{
	return ( 1 / ( ( 1.0f / _currentFPS( ) ) * ( abs( strobe.strobeInterval ) + 1 ) ) );
}

_inline double _dutyCycle( void )
{
	int strobeInterval = strobe.strobeInterval;
	return ( ( ( 1.0f / ( abs( strobeInterval ) + 1 ) ) * 100 ) * ( strobeInterval < 0 ? -strobeInterval : 1 ) );
}

_inline double _positivePhaseShift( void )
{
	if ( _isPhaseInverted( ) )
		return ( 1.0f / _currentFPS( ) ) * 1000;
	else
		return 0.0f;
}

_inline double _negativePhaseShift( void )
{
	return abs( strobe.strobeInterval ) * ( _positivePhaseShift( ) );
}

_inline double _period( void )
{
	return ( 1 / _frequency( ) * 1000 );
}

_inline double _geometricMean( double x, double y )
{
	return sqrt( abs( x * y ) );
}

_inline double _arithmeticMean( double x, double y )
{
	return ( x + y ) / 2;
}

_inline double _actualBrightnessReduction( void )
{
	return lossCalculator( _currentFPS( ), _effectiveFPS( ) );
}

_inline double _logarithmicBrightnessReduction( double base )
{
	return lossCalculator( log( base ), log( base * _effectiveFPS( ) / _currentFPS( ) ) );
}

_inline double _squareBrightnessReduction( double base )
{
	return lossCalculator( sqrt( base ), sqrt( base * _effectiveFPS( ) / _currentFPS( ) ) );
}

_inline double _cubeBrightnessReduction( double base )
{
	return lossCalculator( cbrt( base ), cbrt( base * _effectiveFPS( ) / _currentFPS( ) ) );
}

_inline double _otherBrightnessReduction( double base, double ( *reductionFunction )( double ) )
{
	return lossCalculator( reductionFunction( base ), reductionFunction( base * _effectiveFPS( ) / _currentFPS( ) ) );
}

static double _badness_reduced( qboolean PWMInvolved )
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
		return ( badness * _period( ) );
	else
		return badness;
}

static double _badness( qboolean PWMInvolved )
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
	    -log( ( ( absoluteDifference + _geometricMean( ( 100.0 - diffP ), ( 100.0 - diffN ) ) ) / ( absoluteDifference + _geometricMean( diffP, diffN ) ) ) );

	if ( PWMInvolved )
		return ( badness * _period( ) );
	else
		return badness;
}

static int _frameCount( CountType type )
{
	switch ( type )
	{
	case ( PositiveFrame ):
	{
		return strobe.pCounter;
		break;
	}
	case ( PositiveNormalFrame ):
	{
		return strobe.pNCounter;
		break;
	}
	case ( PositiveBlackFrame ):
	{
		return strobe.pBCounter;
		break;
	}
	case ( NegativeFrame ):
	{
		return strobe.nCounter;
		break;
	}
	case ( NegativeNormalFrame ):
	{
		return strobe.nNCounter;
		break;
	}
	case ( NegativeBlackFrame ):
	{
		return strobe.nBCounter;
		break;
	}
	case ( TotalFrame ):
	default:
		return strobe.fCounter;
		break;
	}
}

static void _generateDebugInfo( char *dst, int size )
{
	int _k;
	char diffBarP[128], diffBarN[128], diffBarT[128];
	int nPositiveNormal, nPositiveBlack, nNegativeNormal, nNegativeBlack;
	static int positiveNormal, positiveBlack, negativeNormal, negativeBlack;
	int diffP_NB, diffN_NB;
	double diffP = 0.0, diffN = 0.0, diffT = 0.0;
	int strobeMethod = strobe.strobeInterval;
	char strobemethod[128];

	diffP_NB = ( strobe.pNCounter - strobe.pBCounter );
	diffN_NB = ( strobe.nNCounter - strobe.nBCounter );

	if ( strobe.pCounter )
		diffP = diffP_NB * 100.0 / strobe.pCounter; // round( abs( diffP_NB ) * 100 / strobe.pCounter );

	if ( strobe.nCounter )
		diffN = diffN_NB * 100.0 / strobe.nCounter; // round( abs( diffN_NB ) * 100 / strobe.nCounter );
	
	
	diffT = fabs( diffP - diffN );
	diffP = fabs( diffP );
	diffN = fabs( diffN );

	_generateDiffBar( diffBarP, sizeof( diffBarP ), 0 );
	_generateDiffBar( diffBarN, sizeof( diffBarN ), 1 );
	_generateDiffBar( diffBarT, sizeof( diffBarT ), 2 );

	nPositiveNormal = _frameCount( PositiveNormalFrame );
	nPositiveBlack  = _frameCount( PositiveBlackFrame );
	nNegativeNormal = _frameCount( NegativeNormalFrame );
	nNegativeBlack  = _frameCount( NegativeBlackFrame );

	Q_snprintf( strobemethod, sizeof( strobemethod ), ( strobeMethod > 0 ? "%d [R" : "%d [B" ), strobeMethod );
	for ( _k = 1; _k <= abs( strobeMethod ); ++_k )
	{
		Q_strcat( strobemethod, ( strobeMethod > 0 ? " - B" : " - R" ) );
	}
	Q_strcat( strobemethod, "]" );

	Q_snprintf( dst,
	            size,
	            "%.2f FPS\n%.2f eFPS\n"
	            "Strobe Method: %s\n"
	            "Strobe Phase Swap Interval: %d second(s)\n"
	            "Strobe Cooldown Delay: %d second(s)\n"
	            "Elapsed Time: %.2f second(s)\n"
	            "isPhaseInverted = %d\n"
	            "Total Frame Count: %d\n"
	            "^7(+) Phase Frame Count: %d\n"
	            "%s\n"
	            "%s\n"
	            "(-) Phase Frame Count: %d\n"
	            "%s\n"
	            "%s\n"
	            "^5=====ANALYSIS=====\n^3"
	            "PWM Simulation:\n"
	            " |-> Frequency: %.2f Hz\n"
	            " |-> Duty Cycle: %.2f%%\n"
	            " |-> Phase Shift: +%.2f || -%.2f millisecond(s)\n"
	            " |-> Period: %.2f millisecond(s)\n"
	            "Brightness Reduction:\n"
	            " |-> [^7LINEAR^3] Actual Reduction: %.2f%%\n"
	            " |-> [^7LOG^3] Realistic Reduction (350 cd/m2 base): %.2f%%\n"
	            " |-> [^7SQUARE^3] Realistic Reduction (350 cd/m2 base): %.2f%%\n"
	            " |-> [^7CUBE^3] Realistic Reduction (350 cd/m2 base): %.2f%%\n"
	            "Difference (+): %s - %.2f%%\nDifference (-): %s - %.2f%%\nDifference (Total): %s - %.2f / 200\n"
	            "Geometric Mean: %.4f\n"
	            "Geometric / Arithmetic Mean Difference: %.4f\n"
	            "[^7EXPERIMENTAL^3] Badness: %.4f\n"
	            "[^7EXPERIMENTAL^3] Badness x PWM Period: %.4f\n"
	            "[^7EXPERIMENTAL^3] Badness (Reduced): %.4f\n"
	            "[^7EXPERIMENTAL^3] Badness (Reduced) x PWM Period: %.4f\n"
	            "Stability:\n"
	            " |-> Standard Deviation: %.3f\n"
	            " |-> Cooldown: %s\n"
	            "^5=====ANALYSIS=====\n^3",
	            _currentFPS( ),
	            _effectiveFPS( ),
	            strobemethod,
	            strobe.swapInterval,
	            r_strobe_cooldown->integer,
	            strobe.elapsedTime,
	            _isPhaseInverted( ),
	            _frameCount( TotalFrame ),
	            _frameCount( PositiveFrame ),
	            ( nPositiveNormal > positiveNormal ? va( "^2 |-> Normal Frame Count: %d^7", nPositiveNormal ) : va( " |-> Normal Frame Count: %d", nPositiveNormal ) ),
	            ( nPositiveBlack > positiveBlack ? va( "^2 |-> Black Frame Count: %d^7", nPositiveBlack ) : va( " |-> Black Frame Count: %d", nPositiveBlack ) ),
	            _frameCount( NegativeFrame ),
	            ( nNegativeNormal > negativeNormal ? va( "^2 |-> Normal Frame Count: %d^7", nNegativeNormal ) : va( " |-> Normal Frame Count: %d", nNegativeNormal ) ),
	            ( nNegativeBlack > negativeBlack ? va( "^2 |-> Black Frame Count: %d^7", nNegativeBlack ) : va( " |-> Black Frame Count: %d", nNegativeBlack ) ),
	            _frequency( ),
	            _dutyCycle( ),
	            _positivePhaseShift( ),
	            _negativePhaseShift( ),
	            _period( ),
	            _actualBrightnessReduction( ),
	            _logarithmicBrightnessReduction( 350.0 ),
	            _squareBrightnessReduction( 350.0 ),
	            _cubeBrightnessReduction( 350.0 ),
	            diffBarP, diffP,
	            diffBarN, diffN,
	            diffBarT, diffT,
	            _geometricMean( diffP, diffN ),
	            _arithmeticMean( diffP, diffN ) - _geometricMean( diffP, diffN ),
	            _badness( false ),
	            _badness( true ),
	            _badness_reduced( false ),
	            _badness_reduced( true ),
	            strobe.deviation,
	            ( strobe.cdTimer >= 0.0 && strobe.cdTriggered ? va( "^1 %.2f secs\n[STROBING DISABLED] ^3", _cooldown( ) ) : "0" ) );

	positiveNormal = nPositiveNormal;
	positiveBlack  = nPositiveBlack;
	negativeNormal = nNegativeNormal;
	negativeBlack  = nNegativeBlack;
}

static void _processFrame( void )
{
	qboolean firstInverted;
	static qboolean phase = false;

	if ( phase != _isPhaseInverted( ) && ( strobe.strobeInterval == 1 || strobe.strobeInterval == -1 ) && !strobe.cdTriggered )
		firstInverted = true;
	else
		firstInverted = false;

	if ( strobe.cdTriggered )
	{
		strobe.frameState = FRAME_RENDER | ( strobe.frameState & PHASE_POSITIVE );
	}

	if ( _isNormal( ) ) // Show normal
	{
		if ( _isPositive( ) )
			++strobe.pNCounter;
		else
			++strobe.nNCounter;

		if ( !firstInverted )
			R_Set2DMode( false );
	}
	else // Show black
	{
		if ( _isPositive( ) )
			++strobe.pBCounter;
		else
			++strobe.nBCounter;

		if ( !firstInverted )
			_makeFrameBlack( 1.0f );
	}

	if ( firstInverted )
	{
		R_Set2DMode( false );
		_makeFrameBlack( 0.25f );
	}

	phase = _isPhaseInverted( );
	++strobe.fCounter;
}

static void _reset( )
{
	Q_memset( &strobe, 0, sizeof( strobe ) );
	strobe.frameState  = ( PHASE_POSITIVE | FRAME_RENDER );
	strobe.initialTime = Sys_DoubleTime( );
}

void R_Strobe_DrawDebugInfo( void )
{
	rgba_t color;
	int offsetY;
	int fixer;
	double curTime             = Sys_DoubleTime( );
	static double oldTime      = 0;
	static int offsetX         = 0;
	static char debugStr[2048] = {0};

	if ( !r_strobe->integer )
		return;

	if ( cls.state != ca_active )
		return;
	if ( ( !r_strobe_debug->integer && !cl_showfps->integer ) || cl.background )
		return;

	/*
	switch ( cls.scrshot_action )
	{
	case scrshot_normal:
	case scrshot_snapshot:
	case scrshot_inactive:
		break;
	default:
		return;
	}
	*/

	if ( r_strobe_debug->integer )
	{
		if ( ( curTime - oldTime > 0.100 ) )
		{
			_generateDebugInfo( debugStr, sizeof( debugStr ) );
			oldTime = curTime;
		}
	}
	else if ( cl_showfps->integer )
	{
		Q_snprintf( debugStr, sizeof( debugStr ), "%3d eFPS", (int)round( _effectiveFPS( ) ) );
	}

	MakeRGBA( color, 255, 255, 255, 255 );
	Con_DrawStringLen( debugStr, &fixer, &offsetY );
	if ( r_strobe_debug->integer )
		Con_DrawString( scr_width->integer - offsetX - 50, 4, debugStr, color );
	else
		Con_DrawString( scr_width->integer - offsetX - 2, offsetY + 8, debugStr, color );
	if ( abs( fixer - offsetX ) > 50 || offsetX == 0 ) // 50 is for 1080p !
		offsetX = fixer;
}

void R_Strobe_Tick( void )
{
	double delta, delta2;
	double currentTime                       = Sys_DoubleTime( );
	static int frameSnapshot                 = 0;
	static double recentTime2                = 0.0;
	static double recentTime                 = 0.0;
	static double deltaArray[DEVIATION_SIZE] = {0.0};

	if ( CL_IsInMenu( ) || ( !vid_fullscreen->integer && CL_IsInConsole( ) ) ) // Disable when not in fullscreen due to viewport problems
	{
		R_Set2DMode( false );
		return;
	}

	delta2             = currentTime - recentTime2;
	recentTime2        = currentTime;
	strobe.elapsedTime = currentTime - strobe.initialTime;

	/*
	if (deltaArray[0] == -1.0)
	{
		for ( int k = 0; k < ARRAYSIZE( deltaArray ); ++k )
		{
			deltaArray[k] = _currentFPS( );
		}
	}
	*/

	if ( strobe.cdTimer >= 0.0 && delta2 > 0.0 )
		strobe.cdTimer += delta2;
	if ( strobe.fCounter - frameSnapshot == 1 )
	{
		deltaArray[strobe.fCounter % ARRAYSIZE( deltaArray )] = _currentFPS( );
		strobe.deviation                                      = _standardDeviation( deltaArray, ARRAYSIZE( deltaArray ) );
	}
	frameSnapshot = strobe.fCounter;

	if ( r_strobe_cooldown->integer > 0 )
	{
		if ( ( strobe.cdTimer > (double)abs( r_strobe_cooldown->integer ) ) && strobe.cdTriggered == true )
		{
			strobe.cdTriggered = false;
			strobe.cdTimer     = -1.0;
		}

		if ( strobe.fCounter > ARRAYSIZE( deltaArray ) )
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

	if ( ( ( strobe.strobeInterval != r_strobe->integer ) && ( strobe.strobeInterval ) ) ||
	     /*((swapInterval != r_strobe_swapinterval->integer) && (swapInterval != 0)) || */
	     strobe.fCounter == INT_MAX )
	{
		_reset( );
		R_Strobe_Tick( );
	}

	strobe.strobeInterval = r_strobe->integer;
	strobe.swapInterval   = r_strobe_swapinterval->integer;

	if ( ( strobe.strobeInterval == 0 ) ||
	     ( ( gl_swapInterval->integer == 0 ) && ( strobe.strobeInterval ) ) ||
	     ( strobe.strobeInterval > 25 || strobe.strobeInterval < -25 ) )
	{
		if ( !gl_swapInterval->integer )
			MsgDev( D_WARN, "Strobing requires V-SYNC enabled! (gl_swapInterval != 0) \n" );

		if ( strobe.strobeInterval )
		{
			Cvar_Set( "r_strobe", "0" );
		}

		R_Set2DMode( false );
		return;
	}

	if ( ( strobe.fCounter % 2 ) == 0 )
	{
		++strobe.pCounter;
		strobe.frameState |= PHASE_POSITIVE;
	}
	else
	{
		++strobe.nCounter;
		strobe.frameState &= ~PHASE_POSITIVE;
	}

	if ( strobe.swapInterval < 0 )
		strobe.swapInterval = abs( strobe.swapInterval );

	if ( ( strobe.swapInterval ) && ( strobe.strobeInterval % 2 ) ) // Swapping not enabled for even intervals as it is neither necessary nor works as intended
	{
		delta = currentTime - recentTime;
		if ( ( delta >= (double)( strobe.swapInterval ) ) && ( delta < (double)( 2 * strobe.swapInterval ) ) ) // Basic timer
		{
			strobe.frameState |= PHASE_INVERTED;
		}
		else if ( delta < (double)( strobe.swapInterval ) )
		{
			strobe.frameState &= ~PHASE_INVERTED;
		}
		else //if (delta >= (double)(2 * swapInterval))
		{
			recentTime = currentTime;
		}
	}

	switch ( strobe.frameState & ( PHASE_POSITIVE | PHASE_INVERTED ) )
	{
	case ( PHASE_POSITIVE | PHASE_INVERTED ):
		if ( ( abs( strobe.strobeInterval ) % 2 ) == 0 )
			strobe.frameState = ( ( ( strobe.pCounter - 1 ) % ( abs( strobe.strobeInterval ) + 1 ) ) == ( abs( strobe.strobeInterval ) / 2 ) ) ? strobe.frameState | FRAME_RENDER : strobe.frameState & ~FRAME_RENDER; //even
		else
			strobe.frameState &= ~FRAME_RENDER;
		break;

	case ( PHASE_POSITIVE & ~PHASE_INVERTED ):
		if ( abs( strobe.strobeInterval ) % 2 == 0 )
			strobe.frameState = ( ( ( strobe.pCounter - 1 ) % ( abs( strobe.strobeInterval ) + 1 ) ) == 0 ) ? strobe.frameState | FRAME_RENDER : strobe.frameState & ~FRAME_RENDER; //even
		else
		{
			if ( abs( strobe.strobeInterval ) == 1 )
				strobe.frameState |= FRAME_RENDER;
			else
				strobe.frameState = ( ( ( strobe.pCounter - 1 ) % ( ( abs( strobe.strobeInterval ) + 1 ) / 2 ) ) == 0 ) ? strobe.frameState | FRAME_RENDER : strobe.frameState & ~FRAME_RENDER; //odd
		}
		break;

	case ( ~PHASE_POSITIVE & PHASE_INVERTED ):
		if ( abs( strobe.strobeInterval ) % 2 == 0 )
			strobe.frameState = ( ( ( strobe.nCounter - 1 ) % ( abs( strobe.strobeInterval ) + 1 ) ) == 0 ) ? strobe.frameState | FRAME_RENDER : strobe.frameState & ~FRAME_RENDER; //even
		else
		{
			if ( abs( strobe.strobeInterval ) == 1 )
				strobe.frameState |= FRAME_RENDER;
			else
				strobe.frameState = ( ( ( strobe.nCounter - 1 ) % ( ( abs( strobe.strobeInterval ) + 1 ) / 2 ) ) == 0 ) ? strobe.frameState | FRAME_RENDER : strobe.frameState & ~FRAME_RENDER; //odd
		}
		break;

	case 0:
		if ( ( abs( strobe.strobeInterval ) % 2 ) == 0 )
			strobe.frameState = ( ( ( strobe.nCounter - 1 ) % ( abs( strobe.strobeInterval ) + 1 ) ) == ( abs( strobe.strobeInterval ) / 2 ) ) ? strobe.frameState | FRAME_RENDER : strobe.frameState & ~FRAME_RENDER; //even
		else
			strobe.frameState &= ~FRAME_RENDER;
		break;

	default:
		strobe.frameState = ( PHASE_POSITIVE | FRAME_RENDER );
	}

	if ( strobe.strobeInterval < 0 )
		strobe.frameState ^= FRAME_RENDER;

	_processFrame( );
}

void R_Strobe_Init( void )
{
	r_strobe              = Cvar_Get( "r_strobe", "0", CVAR_ARCHIVE, "black frame replacement algorithm interval" );
	r_strobe_swapinterval = Cvar_Get( "r_strobe_swapinterval", "0", CVAR_ARCHIVE, "swapping phase interval (seconds)" );
	r_strobe_debug        = Cvar_Get( "r_strobe_debug", "0", CVAR_ARCHIVE, "show strobe debug information" );
	r_strobe_cooldown     = Cvar_Get( "r_strobe_cooldown", "3", CVAR_ARCHIVE, "strobe cooldown period in seconds" );
	_reset( );
}
