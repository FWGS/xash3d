/*
s_dsp.c - digital signal processing algorithms for audio FX
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

#ifndef XASH_DEDICATED

#include "common.h"
#include "client.h"
#include "sound.h"

// a1batross: trying to defend disgusting sound on 44k
#define XASH_DSP_FIXES

typedef struct sx_preset_s {
	float room_lp; // lowpass
	float room_mod; // modulation

	// reverb
	float room_size;
	float room_refl;
	float room_rvblp;

	// delay
	float room_delay;
	float room_feedback;
	float room_dlylp;
	float room_left;
} sx_preset_t;

typedef struct dly_s {
	int cdelaysamplesmax; // delay line array size

	// delay line pointers
	size_t idelayinput;
	size_t idelayoutput;

	// crossfade
	int idelayoutputxf;   // output pointer
	int xfade;            // value

	int delaysamples;     // delay setting
	int delayfeedback;    // feedback setting

	// lowpass
	int lp;               // is lowpass enabled
	int lp0, lp1;         // lowpass buffer

	// modulation
	int mod;
	int modcur;

	// delay line
	int *lpdelayline;
} dly_t;


const sx_preset_t rgsxpre[] =
{
//          -------reverb------  -------delay--------
// lp  mod  size   refl   rvblp  delay  feedback  dlylp  left
{ 0.0, 0.0, 0.0,   0.0,   1.0,   0.0,   0.0,      2.0,   0.0    }, // 0 off
{ 0.0, 0.0, 0.0,   0.0,   1.0,   0.065, 0.1,      0.0,   0.01   }, // 1 generic
{ 0.0, 0.0, 0.0,   0.0,   1.0,   0.02,  0.75,     0.0,   0.01   }, // 2 metalic
{ 0.0, 0.0, 0.0,   0.0,   1.0,   0.03,  0.78,     0.0,   0.02   }, // 3
{ 0.0, 0.0, 0.0,   0.0,   1.0,   0.06,  0.77,     0.0,   0.03   }, // 4
{ 0.0, 0.0, 0.05,  0.85,  1.0,   0.008, 0.96,     2.0,   0.01   }, // 5 tunnel
{ 0.0, 0.0, 0.05,  0.88,  1.0,   0.01,  0.98,     2.0,   0.02   }, // 6
{ 0.0, 0.0, 0.05,  0.92,  1.0,   0.015, 0.995,    2.0,   0.04   }, // 7
{ 0.0, 0.0, 0.05,  0.84,  1.0,   0.0,   0.0,      2.0,   0.012  }, // 8 chamber
{ 0.0, 0.0, 0.05,  0.9,   1.0,   0.0,   0.0,      2.0,   0.008  }, // 9
{ 0.0, 0.0, 0.05,  0.95,  1.0,   0.0,   0.0,      2.0,   0.004  }, // 10
{ 0.0, 0.0, 0.05,  0.7,   0.0,   0.0,   0.0,      2.0,   0.012  }, // 11 brite
{ 0.0, 0.0, 0.055, 0.78,  0.0,   0.0,   0.0,      2.0,   0.008  }, // 12
{ 0.0, 0.0, 0.05,  0.86,  0.0,   0.0,   0.0,      2.0,   0.002  }, // 13
{ 1.0, 0.0, 0.0,   0.0,   1.0,   0.0,   0.0,      2.0,   0.01   }, // 14 water
{ 1.0, 0.0, 0.0,   0.0,   1.0,   0.06,  0.85,     2.0,   0.02   }, // 15
{ 1.0, 0.0, 0.0,   0.0,   1.0,   0.2,   0.6,      2.0,   0.05   }, // 16
{ 0.0, 0.0, 0.05,  0.8,   1.0,   0.0,   0.48,     2.0,   0.016  }, // 17 concrete
{ 0.0, 0.0, 0.06,  0.9,   1.0,   0.0,   0.52,     2.0,   0.01   }, // 18
{ 0.0, 0.0, 0.07,  0.94,  1.0,   0.3,   0.6,      2.0,   0.008  }, // 19
{ 0.0, 0.0, 0.0,   0.0,   1.0,   0.3,   0.42,     2.0,   0.0    }, // 20 outside
{ 0.0, 0.0, 0.0,   0.0,   1.0,   0.35,  0.48,     2.0,   0.0    }, // 21
{ 0.0, 0.0, 0.0,   0.0,   1.0,   0.38,  0.6,      2.0,   0.0    }, // 22
{ 0.0, 0.0, 0.05,  0.9,   1.0,   0.2,   0.28,     0.0,   0.0    }, // 23 cavern
{ 0.0, 0.0, 0.07,  0.9,   1.0,   0.3,   0.4,      0.0,   0.0    }, // 24
{ 0.0, 0.0, 0.09,  0.9,   1.0,   0.35,  0.5,      0.0,   0.0    }, // 25
{ 0.0, 1.0, 0.01,  0.9,   0.0,   0.0,   0.0,      2.0,   0.05   }, // 26 weirdo
{ 0.0, 0.0, 0.0,   0.0,   1.0,   0.009, 0.999,    2.0,   0.04   }, // 27
{ 0.0, 0.0, 0.001, 0.999, 0.0,   0.2,   0.8,      2.0,   0.05   }  // 28
};

enum
{
	MONODLY    = 0,
	REVERBPOS,
	REVERBPOS2,
	STEREODLY,
	MAXDLY
};

#define MAX_DELAY        0.4f
#define MAX_MONO_DELAY   0.4f
#define MAX_REVERB_DELAY 0.1f
#define MAX_STEREO_DELAY 0.1f

#define MAX_ROOM_TYPES ARRAYSIZE(rgsxpre)


// cvars
convar_t *dsp_off;        // disable dsp
convar_t *roomwater_type;  // water room_type
convar_t *room_type;       // current room type

// underwater/special fx modulations
convar_t *sxmod_mod;
convar_t *sxmod_lowpass;

// stereo delay(no feedback)
convar_t *sxste_delay;     // straight left delay

// mono reverb
convar_t *sxrvb_lp;        // lowpass
convar_t *sxrvb_feedback;  // reverb decay. Higher -- longer
convar_t *sxrvb_size;      // room size. Higher -- larger

// mono delay
convar_t *sxdly_lp;        // lowpass
convar_t *sxdly_feedback;  // cycles
convar_t *sxdly_delay;     // current delay in seconds

convar_t *dsp_room;        // for compatibility
convar_t *hisound;
int idsp_room;

static int room_typeprev;
// routines
static int sxamodl, sxamodr,      // amplitude modulation values
	sxamodlt, sxamodrt,    // modulation targets
	sxmod1, sxmod2,
	sxmod1cur, sxmod2cur,
	sxhires;
static int speed;  // dma sample speed (private to dsp)
static portable_samplepair_t *paintto = NULL;
static dly_t rgsxdly[MAXDLY]; // stereo is last
static int   rgsxlp[10];

void SX_Profiling_f( void );

/*
============
SX_ReloadRoomFX

============
*/
void SX_ReloadRoomFX( void )
{
	if( !dsp_room )
		return; // not initialized

	sxste_delay->modified    = true;
	sxrvb_feedback->modified = true;
	sxdly_delay->modified    = true;
	room_type->modified      = true;
}

/*
============
SX_Init()

Starts sound crackling system
============
*/
void SX_Init( void )
{
	Q_memset( rgsxdly, 0, sizeof( rgsxdly ) );
	Q_memset( rgsxlp,  0, sizeof( rgsxlp  ) );

#ifdef XASH_DSP_FIXES
	speed = SOUND_11k; // dsp works best with 11k. Xash will upsample DSP buffer during mix
#else
	speed = SOUND_DMA_SPEED;
#endif

	sxamodr = sxamodl = sxamodrt = sxamodlt = 255;

	sxhires = 2;
	hisound = Cvar_Get( "room_hires", "2", 0, "dsp quality. 1 for 22k, 2 for 44k(recommended) and 3 for 96k" );

	sxmod1cur = sxmod1 = 350 * ( speed / SOUND_11k );
	sxmod2cur = sxmod2 = 450 * ( speed / SOUND_11k );

	dsp_off          = Cvar_Get( "dsp_off",        "0",  CVAR_ARCHIVE, "disable DSP processing" );
	roomwater_type   = Cvar_Get( "waterroom_type", "14", 0, "water room type" );
	room_type        = Cvar_Get( "room_type",      "0",  0, "current room type preset" );

	sxmod_lowpass    = Cvar_Get( "room_lp",  "0", 0, "for water fx, lowpass for entire room" );
	sxmod_mod        = Cvar_Get( "room_mod", "0", 0, "stereo amptitude modulation for room" );

	sxrvb_size       = Cvar_Get( "room_size",  "0", 0, "reverb: initial reflection size" );
	sxrvb_feedback   = Cvar_Get( "room_refl",  "0", 0, "reverb: decay time" );
	sxrvb_lp         = Cvar_Get( "room_rvblp", "1", 0, "reverb: low pass filtering level" );

	sxdly_delay      = Cvar_Get( "room_delay",    "0.8", 0, "mono delay: delay time" );
	sxdly_feedback   = Cvar_Get( "room_feedback", "0.2", 0, "mono delay: decay time" );
	sxdly_lp         = Cvar_Get( "room_dlylp",    "1",   0, "mono delay: low pass filtering level" );

	sxste_delay      = Cvar_Get( "room_left", "0", 0, "left channel delay time" );


	Cmd_AddCommand( "dsp_profile", SX_Profiling_f, "dsp stress-test, first argument is room_type" );

	// for compatibility
	dsp_room         = room_type;
	SX_ReloadRoomFX();
}


/*
===========
DLY_Free

Free memory allocated for DSP
===========
*/
void DLY_Free( int idelay )
{
	Assert( idelay >= 0 && idelay < MAXDLY );

	Z_Free( rgsxdly[idelay].lpdelayline );
	rgsxdly[idelay].lpdelayline = NULL;
}

/*
==========
SX_Shutdown

Stop DSP processor
==========
*/
void SX_Free( )
{
	int i;
	for( i = 0; i <= 3; i++ )
		DLY_Free( i );

	Cmd_RemoveCommand( "dsp_profile" );
}


/*
===========
DLY_Init

Initialize dly
===========
*/
int DLY_Init( int idelay, float delay )
{
	dly_t *cur;

	// DLY_Init called anytime with constants. So valid it in debug builds only.
	Assert( idelay >= 0 && idelay < MAXDLY );
	Assert( delay > 0.0f && delay <= MAX_DELAY );

	DLY_Free( idelay ); // free dly if it's allocated

	cur = &rgsxdly[idelay];
	cur->cdelaysamplesmax = ((int)(delay * speed) << sxhires) + 1;
	cur->lpdelayline      = (int *) Z_Malloc( cur->cdelaysamplesmax * sizeof( int ) );
	if( !cur->lpdelayline )
	{
		MsgDev( D_ERROR, "Failed to allocate memory for SoundFX!\n");
		return 0;
	}

	cur->xfade = 0;

	// init modulation
	cur->mod = cur->modcur = 0;

	// init lowpass
	cur->lp = 1;
	cur->lp0 = cur->lp1 = 0;

	cur->idelayinput = 0;
	cur->idelayoutput = cur->cdelaysamplesmax - cur->delaysamples; // NOTE: delaysamples must be set!!!


	return 1;
}

/*
============
DLY_MovePointer

Checks overflow and moves pointer
============
*/
_inline void DLY_MovePointer(dly_t *dly)
{
	if( ++dly->idelayinput >= dly->cdelaysamplesmax )
		dly->idelayinput = 0;

	if( ++dly->idelayoutput >= dly->cdelaysamplesmax )
		dly->idelayoutput = 0;
}



/*
=============
DLY_CheckNewStereoDelayVal

Update stereo processor settings if we are in new room
=============
*/
void DLY_CheckNewStereoDelayVal( )
{
	dly_t * const dly = &rgsxdly[STEREODLY];
	float delay = sxste_delay->value;

	if( !sxste_delay->modified )
		return;
	sxste_delay->modified = false;

	if( delay == 0 )
	{
		DLY_Free( STEREODLY );
	}
	else
	{
		int samples;

		delay = min( delay, MAX_STEREO_DELAY );
		samples = (int)(delay * speed) << sxhires;

		// re-init dly
		if( !dly->lpdelayline )
		{
			dly->delaysamples = samples;
			DLY_Init( STEREODLY, MAX_STEREO_DELAY );
		}

		if( dly->delaysamples != samples )
		{
			dly->xfade = 128;
			dly->idelayoutputxf = dly->idelayinput - samples;
			if( dly->idelayoutputxf < 0 )
				dly->idelayoutputxf += dly->cdelaysamplesmax;
		}
		dly->modcur = dly->mod = 0;

		if( dly->delaysamples == 0 )
			DLY_Free( STEREODLY );
	}
}

/*
=============
DLY_DoStereoDelay

Do stereo processing
=============
*/
void DLY_DoStereoDelay( int count )
{
	int delay;
	int samplexf;
	dly_t *const dly = &rgsxdly[STEREODLY];
	portable_samplepair_t *paint = paintto;

	if( !dly->lpdelayline )
		return; // inactive

	for( ; count; count--, paint++ )
	{
		if( dly->mod && --dly->modcur < 0 )
			dly->modcur = dly->mod;

		delay = dly->lpdelayline[dly->idelayoutput];

		// process only if crossfading, active left value or delayline
		if( delay || paint->left || dly->xfade )
		{
			// set up new crossfade, if not crossfading, not modulating, but going to
			if( !dly->xfade && !dly->modcur && dly->mod )
			{
				dly->idelayoutputxf = dly->idelayoutput +
						((Com_RandomLong( 0, 255 ) * dly->delaysamples ) >> 9 );

				dly->xfade = 128;
			}

			dly->idelayoutputxf %= dly->cdelaysamplesmax;

			// modify delay, if crossfading
			if( dly->xfade )
			{
				samplexf = dly->lpdelayline[dly->idelayoutputxf] * (128 - dly->xfade) >> 7;
				delay = samplexf + ((delay * dly->xfade) >> 7);
				if( ++dly->idelayoutputxf >= dly->cdelaysamplesmax )
					dly->idelayoutputxf = 0;

				if( --dly->xfade == 0 )
					dly->idelayoutput = dly->idelayoutputxf;
			}

			// save left value to delay line
			dly->lpdelayline[dly->idelayinput] = CLIP(paint->left);

			// paint new delay value
			paint->left = delay;
		}
		else // clear delay line
			dly->lpdelayline[dly->idelayinput] = 0;

		DLY_MovePointer( dly );
	}
}


/*
=============
DLY_CheckNewDelayVal

Update delay processor settings if we are in new room
=============
*/
void DLY_CheckNewDelayVal( )
{
	float delay = sxdly_delay->value;
	dly_t * const dly = &rgsxdly[MONODLY];

	if( !sxdly_delay->modified )
		return;
	sxdly_delay->modified = false;


	if( delay == 0 )
		DLY_Free( MONODLY );
	else
	{
		delay = min( delay, MAX_MONO_DELAY );
		dly->delaysamples = (int)(delay * speed) << sxhires;

		// init dly
		if( !dly->lpdelayline )
			DLY_Init( MONODLY, MAX_MONO_DELAY );

		if( dly->lpdelayline )
		{
			Q_memset( dly->lpdelayline, 0, dly->cdelaysamplesmax * sizeof( int ) );
			dly->lp0 = dly->lp1 = 0;
		}

		dly->idelayinput = 0;
		dly->idelayoutput = dly->cdelaysamplesmax - dly->delaysamples;

		if( !dly->delaysamples )
			DLY_Free( MONODLY );
	}

	dly->lp = sxdly_lp->integer;
	dly->delayfeedback = 255 * sxdly_feedback->value;
}

/*
=============
DLY_DoDelay

Do delay processing
=============
*/
void DLY_DoDelay( int count )
{
	dly_t * const dly = &rgsxdly[MONODLY];
	portable_samplepair_t *paint = paintto;
	int delay;

	if( !dly->lpdelayline )
		return; // inactive

	for( ; count; count--, paint++ )
	{
		delay = dly->lpdelayline[dly->idelayoutput];

		// don't process if delay line and left/right samples are zero
		if( delay || paint->left || paint->right )
		{
			// calculate delayed value from average
			int val = (( paint->left + paint->right ) >> 1 ) +
					 (( dly->delayfeedback * delay ) >> 8);
			val = CLIP( val );

			if( dly->lp ) // lowpass
			{
				int newval = ( dly->lp0 + dly->lp1 + (val << 1) ) >> 2;

				dly->lp0 = dly->lp1;
				dly->lp1 = val;

				val = newval;
			}

			dly->lpdelayline[dly->idelayinput] = val;

			val >>= 2;

			paint->left = CLIP( paint->left + val );
			paint->right = CLIP( paint->right + val );
		}
		else
		{
			dly->lpdelayline[dly->idelayinput] = 0;
			dly->lp0 = dly->lp1 = 0;
		}

		DLY_MovePointer( dly );
	}
}

/*
===========
RVB_SetUpDly

Set up dly for reverb
===========
*/
void RVB_SetUpDly( int pos, float delay, int kmod )
{
	int samples;
	delay = min( delay, MAX_REVERB_DELAY );
	samples = (int)(delay * speed) << sxhires;

	if( !rgsxdly[pos].lpdelayline )
	{
		rgsxdly[pos].delaysamples = samples;
		DLY_Init( pos, MAX_REVERB_DELAY );
	}

	rgsxdly[pos].modcur = rgsxdly[pos].mod = (int)(kmod * speed / SOUND_11k) << sxhires;

	// set up crossfade, if delay has changed
	if( rgsxdly[pos].delaysamples != samples )
	{
		rgsxdly[pos].idelayoutputxf = rgsxdly[pos].idelayinput - samples;
		if( rgsxdly[pos].idelayoutputxf < 0 )
			rgsxdly[pos].idelayoutputxf += rgsxdly[pos].cdelaysamplesmax;
		rgsxdly[pos].xfade = 32;
	}

	if( !rgsxdly[pos].delaysamples )
		DLY_Free( pos );

}


/*
===========
RVB_CheckNewReverbVal

Update reverb settings if we are in new room
===========
*/
void RVB_CheckNewReverbVal( )
{
	float delay = sxrvb_size->value;
	dly_t *const dly1 = &rgsxdly[REVERBPOS],
			  *const dly2 = &rgsxdly[REVERBPOS + 1];
//	int samples;

	if( !sxrvb_size->modified )
		return;
	sxrvb_size->modified = false;

	if( delay == 0.0f )
	{
		DLY_Free( REVERBPOS );
		DLY_Free( REVERBPOS + 1 );
	}
	else
	{
		RVB_SetUpDly( REVERBPOS, sxrvb_size->value, 500 );
		RVB_SetUpDly( REVERBPOS+1, sxrvb_size->value*0.71, 700 );
	}

	dly1->lp = dly2->lp = sxrvb_lp->integer;
	dly1->delayfeedback = dly2->delayfeedback = (int)(255 * sxrvb_feedback->value);
}

/*
===========
RVB_DoReverbForOneDly

Do reverberation for one dly
===========
*/
#define REVERB_XFADE 32
int RVB_DoReverbForOneDly( dly_t *dly, const int vlr, const portable_samplepair_t *samplepair )
{
	int delay;
	int samplexf;
	int val;
	int valt;
	int voutm = 0;

	if( --dly->modcur < 0 )
		dly->modcur = dly->mod;

	delay = dly->lpdelayline[dly->idelayoutput];

	if( dly->xfade || delay || samplepair->left || samplepair->right )
	{
		// modulate delay rate
		if( !dly->mod )
		{
			dly->idelayoutputxf = dly->idelayoutput + ((Com_RandomLong( 0, 255 ) * delay) >> 9 );

			if( dly->idelayoutputxf >= dly->cdelaysamplesmax )
				dly->idelayoutputxf -= dly->cdelaysamplesmax;

			dly->xfade = REVERB_XFADE;
		}

		if( dly->xfade )
		{
			samplexf = (dly->lpdelayline[dly->idelayoutputxf] * (REVERB_XFADE - dly->xfade)) / REVERB_XFADE;
			delay = ((delay * dly->xfade) / REVERB_XFADE) + samplexf;

			if( ++dly->idelayoutputxf >= dly->cdelaysamplesmax )
				dly->idelayoutputxf = 0;

			if( --dly->xfade == 0 )
				dly->idelayoutput = dly->idelayoutputxf;
		}


		if( delay )
		{
			val = vlr + ( ( dly->delayfeedback * delay ) >> 8 );
			val = CLIP( val );
		}
		else
			val = vlr;

		if( dly->lp )
		{
			valt = (dly->lp0 + val) >> 1;
			dly->lp0 = val;
		}
		else
			valt = val;

		voutm = dly->lpdelayline[dly->idelayinput] = valt;
	}
	else
	{
		voutm = dly->lpdelayline[dly->idelayinput] = 0;
		dly->lp0 = 0;
	}

	DLY_MovePointer( dly );

	return voutm;

}

/*
===========
RVB_DoReverb

Do reverberation processing
===========
*/
void RVB_DoReverb( int count )
{
	int vlr;
	dly_t *const dly1 = &rgsxdly[REVERBPOS],
			  *const dly2 = &rgsxdly[REVERBPOS+1];
	portable_samplepair_t *paint = paintto;

	if( !dly1->lpdelayline )
		return;

	for( ; count; count--, paint++ )
	{
		int voutm = 0;
		vlr = ( paint->left + paint->right ) >> 1;

		voutm = RVB_DoReverbForOneDly( dly1, vlr, paint );
		voutm += RVB_DoReverbForOneDly( dly2, vlr, paint );

		voutm = (11 * voutm) >> 6;

		paint->left = CLIP( paint->left + voutm);
		paint->right = CLIP( paint->right + voutm);
	}
}

/*
===========
RVB_DoAMod

Do amplification modulation processing
===========
*/
void RVB_DoAMod( int count )
{
	portable_samplepair_t *paint = paintto;

	if( !sxmod_lowpass->integer && !sxmod_mod->integer )
		return;

	for( ; count; count--, paint++ )
	{
		portable_samplepair_t res = *paint;
		if( sxmod_lowpass->integer )
		{
			res.left  = ( rgsxlp[0] + rgsxlp[1] + rgsxlp[2] + rgsxlp[3] + rgsxlp[4] + res.left ) >> 2;
			res.right = ( rgsxlp[5] + rgsxlp[6] + rgsxlp[7] + rgsxlp[8] + rgsxlp[9] + res.right ) >> 2;

			rgsxlp[4] = paint->left;
			rgsxlp[9] = paint->right;

			rgsxlp[0] = rgsxlp[1];
			rgsxlp[1] = rgsxlp[2];
			rgsxlp[2] = rgsxlp[3];
			rgsxlp[3] = rgsxlp[4];
			rgsxlp[4] = rgsxlp[5];
			rgsxlp[5] = rgsxlp[6];
			rgsxlp[6] = rgsxlp[7];
			rgsxlp[7] = rgsxlp[8];
			rgsxlp[8] = rgsxlp[9];
		}

		if( sxmod_mod->integer )
		{
#ifndef XASH_DSP_FIXES // code below is useless on current DSP configuring possibilities, so disable it
			if( --sxmod1cur < 0 )
				sxmod1cur = sxmod1;

			if( !sxmod1 )
				sxamodlt = Com_RandomLong( 32, 255 );

			if( --sxmod2cur < 0 )
				sxmod2cur = sxmod2;

			if( !sxmod2 )
				sxamodrt = Com_RandomLong( 32, 255 );

			res.left = (sxamodl * res.left) >> 8;
			res.right = (sxamodr * res.right) >> 8;

			if( sxamodl < sxamodlt )
				sxamodl++;
			else if( sxamodl > sxamodlt )
				sxamodl--;

			if( sxamodr < sxamodrt )
				sxamodr++;
			else if( sxamodr > sxamodrt )
				sxamodr--;
#else
			// NOTE: sxamodr & sxamodl is always set to 255 and this value does not changes
			res.left = (255 * res.left) >> 8;
			res.right = (255 * res.right) >> 8;
#endif
		}

		paint->left = CLIP(res.left);
		paint->right = CLIP(res.right);
	}
}


/*
===========
DSP_Process

(xash dsp interface)
===========
*/
void DSP_Process(int idsp, portable_samplepair_t *pbfront, int sampleCount)
{
	if( dsp_off->integer )
		return;

	// HACKHACK: don't process while in menu
	if( cls.key_dest == key_menu )
		return;

	if( !sampleCount )
		return;

	// preset is already installed by CheckNewDspPresets
	paintto = pbfront;

	RVB_DoAMod( sampleCount );
	RVB_DoReverb( sampleCount );
	DLY_DoDelay( sampleCount );
	DLY_DoStereoDelay( sampleCount );
}


/*
===========
DSP_ClearState

(xash dsp interface)
===========
*/
void DSP_ClearState( void )
{
	Cvar_SetFloat( "room_type", 0.0f );
	SX_ReloadRoomFX();
}

/*
===========
AllocDsps

(xash dsp interface)
===========
*/
qboolean AllocDsps( void )
{
	SX_Init();

	return 1;
}

/*
===========
FreeDsps

(xash dsp interface)
===========
*/
void FreeDsps( void )
{
	SX_Free();
}

/*
===========
CheckNewDspPresets

(xash dsp interface)
===========
*/
void CheckNewDspPresets( void )
{
	if( dsp_off->integer )
		return;

	if( s_listener.waterlevel > 2 )
		idsp_room = roomwater_type->integer;
	else
		idsp_room = room_type->integer;

	if( hisound->modified )
	{
		sxhires = hisound->integer;

		hisound->modified = false;
	}

	if( idsp_room == room_typeprev && idsp_room == 0 )
		return;

	if( idsp_room > MAX_ROOM_TYPES )
		return;

	if( idsp_room != room_typeprev )
	{
		const sx_preset_t *cur = rgsxpre + idsp_room;

		Cvar_SetFloat( "room_lp", cur->room_lp );
		Cvar_SetFloat( "room_mod", cur->room_mod );
		Cvar_SetFloat( "room_size", cur->room_size );
		Cvar_SetFloat( "room_refl", cur->room_refl );
		Cvar_SetFloat( "room_rvblp", cur->room_rvblp );
		Cvar_SetFloat( "room_delay", cur->room_delay );
		Cvar_SetFloat( "room_feedback", cur->room_feedback );
		Cvar_SetFloat( "room_dlylp", cur->room_dlylp );
		Cvar_SetFloat( "room_left", cur->room_left );
	}

	room_typeprev = idsp_room;

	RVB_CheckNewReverbVal( );
	DLY_CheckNewDelayVal( );
	DLY_CheckNewStereoDelayVal();
}

void SX_Profiling_f( void )
{
	portable_samplepair_t testbuffer[512];
	int i;
	int calls = 10000;
	double start, end;
	float oldroom = room_type->value;

	for( i = 0; i < 512; i++ )
	{
		testbuffer[i].left = Com_RandomLong( 0, 3000 );
		testbuffer[i].right = Com_RandomLong( 0, 3000 );
	}

	if( Cmd_Argc() > 1 )
	{
		Cvar_SetFloat("room_type", atof(Cmd_Argv( 1 )));
		SX_ReloadRoomFX();
		CheckNewDspPresets(); // we just need idsp_room immediately, for message below
	}

	MsgDev( D_INFO, "Profiling 10000 calls to DSP. Sample count is 512, room_type is %i\n", idsp_room );


	start = Sys_DoubleTime();
	for( ; calls; calls-- )
	{
		DSP_Process( idsp_room, testbuffer, 512 );
	}
	end = Sys_DoubleTime();

	MsgDev( D_INFO, "----------\nTook %.3f seconds.\n", end - start );

	if( Cmd_Argc() > 1 )
	{
		Cvar_SetFloat( "room_type", oldroom );
		SX_ReloadRoomFX();
		CheckNewDspPresets();
	}
}
#endif // XASH_DEDICATED
