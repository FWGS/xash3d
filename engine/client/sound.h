/*
sound.h - sndlib main header
Copyright (C) 2009 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef SOUND_H
#define SOUND_H

extern byte *sndpool;

#include "mathlib.h"

// local flags (never sending acorss the net)
#define SND_LOCALSOUND	(1U << 9)	// not paused, not looped, for internal use
#define SND_STOP_LOOPING	(1U << 10)	// stop all looping sounds on the entity.

// sound engine rate defines
#define SOUND_11k		11025	// 11khz sample rate
#define SOUND_16k		16000	// 16khz sample rate
#define SOUND_22k		22050	// 22khz sample rate
#define SOUND_32k		32000	// 32khz sample rate
#define SOUND_44k		44100	// 44khz sample rate
#define SOUND_DMA_SPEED	SOUND_44k	// hardware playback rate

#define SND_TRACE_UPDATE_MAX  	2	// max of N channels may be checked for obscured source per frame
#define SND_RADIUS_MAX		240.0f	// max sound source radius
#define SND_RADIUS_MIN		24.0f	// min sound source radius
#define SND_OBSCURED_LOSS_DB		-2.70f	// dB loss due to obscured sound source

// calculate gain based on atmospheric attenuation.
// as gain excedes threshold, round off (compress) towards 1.0 using spline
#define SND_GAIN_COMP_EXP_MAX		2.5f	// Increasing SND_GAIN_COMP_EXP_MAX fits compression curve
					// more closely to original gain curve as it approaches 1.0.  
#define SND_GAIN_FADE_TIME		0.25f	// xfade seconds between obscuring gain changes
#define SND_GAIN_COMP_EXP_MIN		0.8f	
#define SND_GAIN_COMP_THRESH		0.5f	// gain value above which gain curve is rounded to approach 1.0
#define SND_DB_MAX			140.0f	// max db of any sound source
#define SND_DB_MED			90.0f	// db at which compression curve changes
#define SND_DB_MIN			60.0f	// min db of any sound source
#define SND_GAIN_PLAYER_WEAPON_DB	2.0f	// increase player weapon gain by N dB

// fixed point stuff for real-time resampling
#define FIX_BITS		28
#define FIX_SCALE		(1U << FIX_BITS)
#define FIX_MASK		((1U << FIX_BITS)-1)
#define FIX_FLOAT(a)	((int)((a) * FIX_SCALE))
#define FIX(a)		(((uint)(a)) << FIX_BITS)
#define FIX_INTPART(a)	(((int)(a)) >> FIX_BITS)
#define FIX_FRACTION(a,b)	(FIX(a)/(b))
#define FIX_FRACPART(a)	((a) & FIX_MASK)

#define SNDLVL_TO_DIST_MULT( sndlvl ) \
	( sndlvl ? ((pow( 10, s_refdb->value / 20 ) / pow( 10, (float)sndlvl / 20 )) / s_refdist->value ) : 0 )

#define DIST_MULT_TO_SNDLVL( dist_mult ) \
	(int)( dist_mult ? ( 20 * log10( pow( 10, s_refdb->value / 20 ) / (dist_mult * s_refdist->value ))) : 0 )

// NOTE: clipped sound at 32760 to avoid overload
#define CLIP( x )		(( x ) > 32760 ? 32760 : (( x ) < -32760 ? -32760 : ( x )))
#define SWAP( a, b, t )	{(t) = (a); (a) = (b); (b) = (t);}
#define AVG( a, b )		(((a) + (b)) >> 1 )
#define AVG4( a, b, c, d )	(((a) + (b) + (c) + (d)) >> 2 )

#define PAINTBUFFER_SIZE	1024	// 44k: was 512
#define PAINTBUFFER		(g_curpaintbuffer)
#define CPAINTBUFFERS	3

typedef struct
{
	int		left;
	int		right;
} portable_samplepair_t;

// sound mixing buffer

#define CPAINTFILTERMEM		3
#define CPAINTFILTERS		4	// maximum number of consecutive upsample passes per paintbuffer

typedef struct
{
	qboolean			factive;	// if true, mix to this paintbuffer using flags
	portable_samplepair_t	*pbuf;	// front stereo mix buffer, for 2 or 4 channel mixing
	int			ifilter;	// current filter memory buffer to use for upsampling pass
	portable_samplepair_t	fltmem[CPAINTFILTERS][CPAINTFILTERMEM];
} paintbuffer_t;

typedef struct sfx_s
{
	string 		name;
	wavdata_t		*cache;

	int		touchFrame;
	uint		hashValue;
	struct sfx_s	*hashNext;
} sfx_t;

extern portable_samplepair_t	drybuffer[];
extern portable_samplepair_t	paintbuffer[];
extern portable_samplepair_t	roombuffer[];
extern portable_samplepair_t	temppaintbuffer[];
extern portable_samplepair_t	*g_curpaintbuffer;
extern paintbuffer_t	paintbuffers[];

// structure used for fading in and out client sound volume.
typedef struct
{
	float		initial_percent;
	float		percent;  	// how far to adjust client's volume down by.
	float		starttime;	// GetHostTime() when we started adjusting volume
	float		fadeouttime;	// # of seconds to get to faded out state
	float		holdtime;		// # of seconds to hold
	float		fadeintime;	// # of seconds to restore
} soundfade_t;

typedef struct
{
	float		percent;
} musicfade_t;

typedef struct snd_format_s
{
	unsigned int	speed;
	unsigned int	width;
	unsigned int	channels;
} snd_format_t;

typedef struct
{
	int		samples;		// mono samples in buffer
	int		samplepos;	// in mono samples
	int     sampleframes;
	byte		*buffer;
	qboolean		initialized;	// sound engine is active
	snd_format_t	format;
} dma_t;

#include "vox.h"

typedef struct
{
	double		sample;
	double 		forcedEndSample;
	wavdata_t		*pData;
	qboolean		finished;
} mixer_t;

struct channel_s
{
	char		name[16];		// keept sentence name
	sfx_t		*sfx;		// sfx number

	int		leftvol;		// 0-255 left volume
	int		rightvol;		// 0-255 right volume

	int		entnum;		// entity soundsource
	int		entchannel;	// sound channel (CHAN_STREAM, CHAN_VOICE, etc.)	
	vec3_t		origin;		// only use if fixed_origin is set
	float		dist_mult;	// distance multiplier (attenuation/clipK)
	int		master_vol;	// 0-255 master volume
	qboolean		isSentence;	// bit who indicated sentence
	int		basePitch;	// base pitch percent (100% is normal pitch playback)
	float		pitch;		// real-time pitch after any modulation or shift by dynamic data
	qboolean		use_loop;		// don't loop default and local sounds
	qboolean		staticsound;	// use origin instead of fetching entnum's origin
	qboolean		localsound;	// it's a local menu sound (not looped, not paused)
	mixer_t		pMixer;

	// sound culling
	qboolean		bfirstpass;	// true if this is first time sound is spatialized
	float		ob_gain;		// gain drop if sound source obscured from listener
	float		ob_gain_target;	// target gain while crossfading between ob_gain & ob_gain_target
	float		ob_gain_inc;	// crossfade increment
	qboolean		bTraced;		// true if channel was already checked this frame for obscuring
	float		radius;		// radius of this sound effect

	// sentence mixer
	int		wordIndex;
	mixer_t		*currentWord;	// NULL if sentence is finished
	voxword_t		words[CVOXWORDMAX];
};

typedef struct
{
	vec3_t		origin;		// simorg + view_ofs
	vec3_t		velocity;
	vec3_t		forward;
	vec3_t		right;
	vec3_t		up;

	int		entnum;
	int		waterlevel;
	float		frametime;	// used for sound fade
	qboolean		active;
	qboolean		inmenu;		// listener in-menu ?
	qboolean		paused;
	qboolean		streaming;	// playing AVI-file
	qboolean		lerping;		// lerp stream ?
	qboolean		stream_paused;	// pause only background track
} listener_t;

typedef struct
{
	string		current;		// a currently playing track
	string		loopName;		// may be empty
	stream_t		*stream;
	int		source;		// may be game, menu, etc
} bg_track_t;

/*
====================================================================

  SYSTEM SPECIFIC FUNCTIONS

====================================================================
*/
// initializes cycling through a DMA buffer and returns information on it
qboolean SNDDMA_Init( void *hInst );
int SNDDMA_GetSoundtime( void );
void SNDDMA_Shutdown( void );
void SNDDMA_BeginPainting( void );
void SNDDMA_Submit( void );

//====================================================================

#define MAX_DYNAMIC_CHANNELS	(28 + NUM_AMBIENTS)
#define MAX_CHANNELS	(128 + MAX_DYNAMIC_CHANNELS)	// Scourge Of Armagon has too many static sounds on hip2m4.bsp
#define MAX_RAW_SAMPLES	8192

extern sound_t	ambient_sfx[NUM_AMBIENTS];
extern qboolean	snd_ambient;
extern channel_t	channels[MAX_CHANNELS];
extern int	total_channels;
extern int	paintedtime;
extern int	s_rawend;
extern int	soundtime;
extern dma_t	dma;
extern listener_t	s_listener;
extern int	idsp_room;

extern convar_t	*s_volume;
extern convar_t	*s_musicvolume;
extern convar_t	*s_show;
extern convar_t	*s_mixahead;
extern convar_t	*s_lerping;
extern convar_t	*dsp_off;
extern convar_t	*s_test;
extern convar_t	*s_phs;
extern convar_t *s_reverse_channels;
extern convar_t	*dsp_room;
extern convar_t *s_samplecount;
extern portable_samplepair_t		s_rawsamples[MAX_RAW_SAMPLES];

void S_InitScaletable( void );
wavdata_t *S_LoadSound( sfx_t *sfx );
float S_GetMasterVolume( void );
float S_GetMusicVolume( void );
void S_PrintDeviceName( void );
void S_Activate( qboolean active );

//
// s_main.c
//
void S_FreeChannel( channel_t *ch );

//
// s_mix.c
//
int S_MixDataToDevice( channel_t *pChannel, int sampleCount, int outputRate, int outputOffset );
void MIX_ClearAllPaintBuffers( int SampleCount, qboolean clearFilters );
void MIX_InitAllPaintbuffers( void );
void MIX_FreeAllPaintbuffers( void );
void MIX_PaintChannels( int endtime );

// s_load.c
qboolean S_TestSoundChar( const char *pch, char c );
char *S_SkipSoundChar( const char *pch );
sfx_t *S_FindName( const char *name, int *pfInCache );
sound_t S_RegisterSound( const char *name );
void S_FreeSound( sfx_t *sfx );

// s_dsp.c
qboolean AllocDsps( void );
void FreeDsps( void );
void CheckNewDspPresets( void );
void DSP_Process( int idsp, portable_samplepair_t *pbfront, int sampleCount );
void DSP_ClearState( void );

qboolean S_Init( void );
void S_Shutdown( void );
void S_SoundList_f( void );
void S_SoundInfo_f( void );

channel_t *SND_PickDynamicChannel( int entnum, int channel, sfx_t *sfx, qboolean *ignore );
channel_t *SND_PickStaticChannel( const vec3_t pos , sfx_t *sfx );
int S_GetCurrentStaticSounds( soundlist_t *pout, int size );
int S_GetCurrentDynamicSounds( soundlist_t *pout, int size );
sfx_t *S_GetSfxByHandle( sound_t handle );
void S_StopSound( int entnum, int channel, const char *soundname );
void S_StopAllSounds( void );
void S_FreeSounds( void );

//
// s_mouth.c
//
void SND_InitMouth( int entnum, int entchannel );
void SND_MoveMouth8( channel_t *ch, wavdata_t *pSource, int count );
void SND_MoveMouth16( channel_t *ch, wavdata_t *pSource, int count );
void SND_CloseMouth( channel_t *ch );

//
// s_stream.c
//
void S_StreamSoundTrack( void );
void S_StreamBackgroundTrack( void );
qboolean S_StreamGetCurrentState( char *currentTrack, char *loopTrack, fs_offset_t *position );
void S_StopBackgroundTrack( void );
void S_PrintBackgroundTrackState( void );
void S_FadeMusicVolume( float fadePercent );

//
// s_utils.c
//
int S_ZeroCrossingAfter( wavdata_t *pWaveData, int sample );
int S_ZeroCrossingBefore( wavdata_t *pWaveData, int sample );
int S_GetOutputData( wavdata_t *pSource, void **pData, int samplePosition, int sampleCount, qboolean use_loop );
void S_SetSampleStart( channel_t *pChan, wavdata_t *pSource, int newPosition );
void S_SetSampleEnd( channel_t *pChan, wavdata_t *pSource, int newEndPosition );
float S_SimpleSpline( float value );

//
// s_vox.c
//
void VOX_Init( void );
void VOX_Shutdown( void );
void VOX_SetChanVol( channel_t *ch );
void VOX_LoadSound( channel_t *pchan, const char *psz );
float VOX_ModifyPitch( channel_t *ch, float pitch );
int VOX_MixDataToDevice( channel_t *pChannel, int sampleCount, int outputRate, int outputOffset );

#endif//SOUND_H
