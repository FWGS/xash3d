/*
s_backend.c - sound hardware output
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
#ifndef XASH_OPENSL
#include "port.h"

#include "common.h"
#include "sound.h"
#ifdef XASH_SDL
#include <SDL.h>
#endif
#define SAMPLE_16BIT_SHIFT		1
#define SECONDARY_BUFFER_SIZE		0x10000

/*
=======================================================================
Global variables. Must be visible to window-procedure function
so it can unlock and free the data block after it has been played.
=======================================================================
*/
convar_t		*s_primary;
convar_t		*s_khz;
dma_t			dma;

//static qboolean	snd_firsttime = true;
//static qboolean	primary_format_set;

#ifdef XASH_SDL
void SDL_SoundCallback( void* userdata, Uint8* stream, int len)
{
	int size = dma.samples << 1;
	int pos = dma.samplepos << 1;
	int wrapped = pos + len - size;

	if (wrapped < 0) {
		memcpy(stream, dma.buffer + pos, len);
		dma.samplepos += len >> 1;
	} else {
		int remaining = size - pos;
		memcpy(stream, dma.buffer + pos, remaining);
		memcpy(stream + remaining, dma.buffer, wrapped);
		dma.samplepos = wrapped >> 1;
	}
}

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/
qboolean SNDDMA_Init( void *hInst )
{
	SDL_AudioSpec desired, obtained;
	int ret = 0;

	if (SDL_WasInit(SDL_INIT_AUDIO) == 0)
		ret = SDL_InitSubSystem(SDL_INIT_AUDIO);
	if (ret == -1) {
		Con_Printf("Couldn't initialize SDL audio: %s\n", SDL_GetError());
		return false;
	}

	Q_memset(&desired, 0, sizeof(desired));
	switch (s_khz->integer) {
	case 48:
		desired.freq = 48000;
		break;
	case 44:
		desired.freq = 44100;
		break;
	case 22:
		desired.freq = 22050;
		break;
	default:
		desired.freq = 11025;
		break;
	}

	desired.format = AUDIO_S16LSB;
	desired.samples = 512;
	desired.channels = 2;
	desired.callback = SDL_SoundCallback;
	ret = SDL_OpenAudio(&desired, &obtained);
	if (ret == -1) {
		Con_Printf("Couldn't open SDL audio: %s\n", SDL_GetError());
		return false;
	}

	if (obtained.format != AUDIO_S16LSB) {
		Con_Printf("SDL audio format %d unsupported.\n", obtained.format);
		goto fail;
	}

	if (obtained.channels != 1 && obtained.channels != 2) {
		Con_Printf("SDL audio channels %d unsupported.\n", obtained.channels);
		goto fail;
	}

	dma.format.speed = obtained.freq;
	dma.format.channels = obtained.channels;
	dma.format.width = 2;
	dma.samples = 0x8000 * obtained.channels;
	dma.buffer = Z_Malloc(dma.samples * 2);
	dma.samplepos = 0;
	dma.sampleframes = dma.samples / dma.format.channels;

	Con_Printf("Using SDL audio driver: %s @ %d Hz\n", SDL_GetCurrentAudioDriver(), obtained.freq);

	SDL_PauseAudio(0);

	dma.initialized = true;
	return true;

fail:
	SNDDMA_Shutdown();
	return false;
}
#else
qboolean SNDDMA_Init( void *hInst )
{
	return false;
}
#endif
/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos( void )
{
	return dma.samplepos;
}

/*
==============
SNDDMA_GetSoundtime

update global soundtime
===============
*/
int SNDDMA_GetSoundtime( void )
{
	static int	buffers, oldsamplepos;
	int		samplepos, fullsamples;
	
	fullsamples = dma.samples / 2;

	// it is possible to miscount buffers
	// if it has wrapped twice between
	// calls to S_Update.  Oh well.
	samplepos = SNDDMA_GetDMAPos();

	if( samplepos < oldsamplepos )
	{
		buffers++; // buffer wrapped

		if( paintedtime > 0x40000000 )
		{	
			// time to chop things off to avoid 32 bit limits
			buffers = 0;
			paintedtime = fullsamples;
			S_StopAllSounds();
		}
	}

	oldsamplepos = samplepos;

	return (buffers * fullsamples + samplepos / 2);
}

/*
==============
SNDDMA_BeginPainting

Makes sure dma.buffer is valid
===============
*/
void SNDDMA_BeginPainting( void )
{
#ifdef XASH_SDL
	SDL_LockAudio();
#endif
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
Also unlocks the dsound buffer
===============
*/
void SNDDMA_Submit( void )
{
#ifdef XASH_SDL
	SDL_UnlockAudio();
#endif
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown( void )
{
	Con_Printf("Shutting down audio.\n");
	dma.initialized = false;
#ifdef XASH_SDL
	SDL_CloseAudio();
	if (SDL_WasInit(SDL_INIT_AUDIO != 0))
		 SDL_QuitSubSystem(SDL_INIT_AUDIO);
#endif
	if (dma.buffer) {
		 Z_Free(dma.buffer);
		 dma.buffer = NULL;
	}
}

/*
===========
S_PrintDeviceName
===========
*/
void S_PrintDeviceName( void )
{
#ifdef XASH_SDL
	Msg( "Audio: SDL (driver: %s)\n", SDL_GetCurrentAudioDriver() );
#endif
}
#else
/*
Copyright (C) 2015 SiPlus, Chasseur de bots

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "common.h"
#include <SLES/OpenSLES.h>
#include "pthread.h"
#include "sound.h"

static convar_t *s_bits;
static convar_t *s_channels;
convar_t		*s_primary;
convar_t		*s_khz;
dma_t			dma;

static SLObjectItf snddma_android_engine = NULL;
static SLObjectItf snddma_android_outputMix = NULL;
static SLObjectItf snddma_android_player = NULL;
static SLBufferQueueItf snddma_android_bufferQueue;
static SLPlayItf snddma_android_play;

static pthread_mutex_t snddma_android_mutex = PTHREAD_MUTEX_INITIALIZER;

static int snddma_android_pos;
static int snddma_android_size;

static const SLInterfaceID *pSL_IID_ENGINE;
static const SLInterfaceID *pSL_IID_BUFFERQUEUE;
static const SLInterfaceID *pSL_IID_PLAY;
static SLresult SLAPIENTRY (*pslCreateEngine)(
		SLObjectItf             *pEngine,
		SLuint32                numOptions,
		const SLEngineOption    *pEngineOptions,
		SLuint32                numInterfaces,
		const SLInterfaceID     *pInterfaceIds,
		const SLboolean         * pInterfaceRequired
);

void S_Activate( qboolean active )
{
	if( active )
	{
		memset( dma.buffer, 0, snddma_android_size * 2 );
		(*snddma_android_bufferQueue)->Enqueue( snddma_android_bufferQueue, dma.buffer, snddma_android_size );
		(*snddma_android_play)->SetPlayState( snddma_android_play, SL_PLAYSTATE_PLAYING );
	}
	else
	{
		//if( s_globalfocus->integer )
			//return;
		(*snddma_android_play)->SetPlayState( snddma_android_play, SL_PLAYSTATE_STOPPED );
		(*snddma_android_bufferQueue)->Clear( snddma_android_bufferQueue );
	}
}

static void SNDDMA_Android_Callback( SLBufferQueueItf bq, void *context )
{
	uint8_t *buffer2;

	pthread_mutex_lock( &snddma_android_mutex );

	buffer2 = ( uint8_t * )dma.buffer + snddma_android_size;
	(*bq)->Enqueue( bq, buffer2, snddma_android_size );
	memcpy( buffer2, dma.buffer, snddma_android_size );
	memset( dma.buffer, 0, snddma_android_size );
	snddma_android_pos += dma.samples;

	pthread_mutex_unlock( &snddma_android_mutex );
}

static const char *SNDDMA_Android_Init( void )
{
	SLresult result;

	SLEngineItf engine;

	int freq;

	SLDataLocator_BufferQueue sourceLocator;
	SLDataFormat_PCM sourceFormat;
	SLDataSource source;

	SLDataLocator_OutputMix sinkLocator;
	SLDataSink sink;

	SLInterfaceID interfaceID;
	SLboolean interfaceRequired;

	int samples;
	void *handle = dlopen( "libOpenSLES.so", RTLD_LAZY );

	if( !handle )
		return "Failed to load libOpenSLES.so";

	pslCreateEngine = dlsym( handle,"slCreateEngine" );

	if( !pslCreateEngine )
		return "Failed to resolve slCreateEngine";

	pSL_IID_ENGINE = dlsym( handle,"SL_IID_ENGINE" );

	if( !pSL_IID_ENGINE )
		return "Failed to resolve SL_IID_ENGINE";

	pSL_IID_PLAY = dlsym( handle,"SL_IID_PLAY" );

	if( !pSL_IID_PLAY )
		return "Failed to resolve SL_IID_PLAY";

	pSL_IID_BUFFERQUEUE = dlsym( handle,"SL_IID_BUFFERQUEUE" );

	if( !pSL_IID_BUFFERQUEUE )
		return "Failed to resolve SL_IID_BUFFERQUEUE";


	result = pslCreateEngine( &snddma_android_engine, 0, NULL, 0, NULL, NULL );
	if( result != SL_RESULT_SUCCESS ) return "slCreateEngine";
	result = (*snddma_android_engine)->Realize( snddma_android_engine, SL_BOOLEAN_FALSE );
	if( result != SL_RESULT_SUCCESS ) return "engine->Realize";
	result = (*snddma_android_engine)->GetInterface( snddma_android_engine, *pSL_IID_ENGINE, &engine );
	if( result != SL_RESULT_SUCCESS ) return "engine->GetInterface(ENGINE)";

	result = (*engine)->CreateOutputMix( engine, &snddma_android_outputMix, 0, NULL, NULL );
	if( result != SL_RESULT_SUCCESS ) return "engine->CreateOutputMix";
	result = (*snddma_android_outputMix)->Realize( snddma_android_outputMix, SL_BOOLEAN_FALSE );
	if( result != SL_RESULT_SUCCESS ) return "outputMix->Realize";

	if( s_khz->integer >= 44 )
		freq = 44100;
	else if( s_khz->integer >= 22 )
		freq = 22050;
	else
		freq = 11025;

	sourceLocator.locatorType = SL_DATALOCATOR_BUFFERQUEUE;
	sourceLocator.numBuffers = 2;
	sourceFormat.formatType = SL_DATAFORMAT_PCM;
	sourceFormat.numChannels = bound( 1, s_channels->integer, 2 );
	sourceFormat.samplesPerSec = freq * 1000;
	sourceFormat.bitsPerSample = 16;
	sourceFormat.containerSize = sourceFormat.bitsPerSample;
	sourceFormat.channelMask = ( ( sourceFormat.numChannels == 2 ) ? SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT : SL_SPEAKER_FRONT_CENTER );
	sourceFormat.endianness = SL_BYTEORDER_LITTLEENDIAN;
	source.pLocator = &sourceLocator;
	source.pFormat = &sourceFormat;

	sinkLocator.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	sinkLocator.outputMix = snddma_android_outputMix;
	sink.pLocator = &sinkLocator;
	sink.pFormat = NULL;

	interfaceID = *pSL_IID_BUFFERQUEUE;
	interfaceRequired = SL_BOOLEAN_TRUE;

	result = (*engine)->CreateAudioPlayer( engine, &snddma_android_player, &source, &sink, 1, &interfaceID, &interfaceRequired );
	if( result != SL_RESULT_SUCCESS ) return "engine->CreateAudioPlayer";
	result = (*snddma_android_player)->Realize( snddma_android_player, SL_BOOLEAN_FALSE );
	if( result != SL_RESULT_SUCCESS ) return "player->Realize";
	result = (*snddma_android_player)->GetInterface( snddma_android_player, *pSL_IID_BUFFERQUEUE, &snddma_android_bufferQueue );
	if( result != SL_RESULT_SUCCESS ) return "player->GetInterface(BUFFERQUEUE)";
	result = (*snddma_android_player)->GetInterface( snddma_android_player, *pSL_IID_PLAY, &snddma_android_play );
	if( result != SL_RESULT_SUCCESS ) return "player->GetInterface(PLAY)";
	result = (*snddma_android_bufferQueue)->RegisterCallback( snddma_android_bufferQueue, SNDDMA_Android_Callback, NULL );
	if( result != SL_RESULT_SUCCESS ) return "bufferQueue->RegisterCallback";

	if( freq <= 11025 )
		samples = 1024;
	else if( freq <= 22050 )
		samples = 2048;
	else
		samples = 4096;

	dma.format.channels = sourceFormat.numChannels;
	dma.samples = samples * sourceFormat.numChannels;
	//dma.submission_chunk = 1;
	//dma.format.samplebits = sourceFormat.bitsPerSample;
	dma.format.speed = freq;
	//dma.msec_per_sample = 1000.0 / freq;
	snddma_android_size = dma.samples * ( sourceFormat.bitsPerSample >> 3 );
	dma.buffer = malloc( snddma_android_size * 2 );
	dma.samplepos = 0;
	dma.sampleframes = dma.samples / dma.format.channels;
	dma.format.width = 2;
	if( !dma.buffer ) return "malloc";

	//snddma_android_mutex = trap_Mutex_Create();

	snddma_android_pos = 0;
	dma.initialized = true;

	S_Activate( true );

	return NULL;
}

qboolean SNDDMA_Init( void *hwnd)
{
	const char *initError;
	qboolean verbose = true;

	if( verbose )
		Msg( "OpenSL ES audio device initializing...\n" );


	s_bits = Cvar_Get( "s_bits", "16", CVAR_ARCHIVE|CVAR_LATCH, "snd bits" );
	s_channels = Cvar_Get( "s_channels", "2", CVAR_ARCHIVE|CVAR_LATCH, "snd channels" );


	initError = SNDDMA_Android_Init();
	if( initError )
	{
		Msg( "SNDDMA_Init: %s failed.\n", initError );
		SNDDMA_Shutdown();
		return false;
	}

	if( verbose )
		Msg( "OpenSL ES audio initialized.\n" );

	return true;
}

int SNDDMA_GetDMAPos( void )
{
	return snddma_android_pos;
}

void SNDDMA_Shutdown( void )
{
	qboolean verbose = true;
	if( verbose )
		Msg( "Closing OpenSL ES audio device...\n" );

	if( snddma_android_player )
	{
		(*snddma_android_player)->Destroy( snddma_android_player );
		snddma_android_player = NULL;
	}
	if( snddma_android_outputMix )
	{
		(*snddma_android_outputMix)->Destroy( snddma_android_outputMix );
		snddma_android_outputMix = NULL;
	}
	if( snddma_android_engine )
	{
		(*snddma_android_engine)->Destroy( snddma_android_engine );
		snddma_android_engine = NULL;
	}

	if( dma.buffer )
	{
		free( dma.buffer );
		dma.buffer = NULL;
	}

	//if( snddma_android_mutex )
		//trap_Mutex_Destroy( &snddma_android_mutex );

	if( verbose )
		Msg( "OpenSL ES audio device shut down.\n" );
}

void SNDDMA_Submit( void )
{
	pthread_mutex_unlock( &snddma_android_mutex );
}

void SNDDMA_BeginPainting( void )
{
	pthread_mutex_lock( &snddma_android_mutex );
}


/*
==============
SNDDMA_GetSoundtime

update global soundtime
===============
*/
int SNDDMA_GetSoundtime( void )
{
	static int	buffers, oldsamplepos;
	int		samplepos, fullsamples;

	fullsamples = dma.samples / 2;

	// it is possible to miscount buffers
	// if it has wrapped twice between
	// calls to S_Update.  Oh well.
	samplepos = SNDDMA_GetDMAPos();

	if( samplepos < oldsamplepos )
	{
		buffers++; // buffer wrapped

		if( paintedtime > 0x40000000 )
		{
			// time to chop things off to avoid 32 bit limits
			buffers = 0;
			paintedtime = fullsamples;
			S_StopAllSounds();
		}
	}

	oldsamplepos = samplepos;

	return (buffers * fullsamples + samplepos / 2);
}
void S_PrintDeviceName( void )
{
	Msg( "Audio: OpenSL\n" );
}
#endif
