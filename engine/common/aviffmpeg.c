/*
avikit.c - playing AVI files (based on original AVIKit code)
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "common.h"
#include "client.h"
#include "gl_local.h"
#include <libavformat/avformat.h>

typedef struct movie_state_s
{
	qboolean		active;
	qboolean		ignore_hwgamma;	// ignore hw gamma-correction
	qboolean		quiet;		// ignore error messages

	AVFormatContext		*avf_context;

	//PAVIFILE		pfile;		// avi file pointer
	//PAVISTREAM	video_stream;	// video stream pointer
	//PGETFRAME		video_getframe;	// pointer to getframe object for video stream
	long		video_frames;	// total frames
	long		video_xres;	// video stream resolution
	long		video_yres;
	float		video_fps;	// video stream fps

	//PAVISTREAM	audio_stream;	// audio stream pointer
	//WAVEFORMAT	*audio_header;	// audio stream header
	long		audio_header_size;	// WAVEFORMAT is returned for PCM data; WAVEFORMATEX for others
	long		audio_codec;	// WAVE_FORMAT_PCM is oldstyle: anything else needs conversion
	long		audio_length;	// in converted samples
	long		audio_bytes_per_sample; // guess.

	// compressed audio specific data
	dword		cpa_blockalign;	// block size to read
	//HACMSTREAM	cpa_conversion_stream;
	//ACMSTREAMHEADER	cpa_conversion_header;
	byte		*cpa_srcbuffer;	// maintained buffer for raw data
	byte		*cpa_dstbuffer;

	dword		cpa_blocknum;	// current block
	dword		cpa_blockpos;	// read position in current block
	dword		cpa_blockoffset;	// corresponding offset in bytes in the output stream

	// for additional unpack Ms-RLE codecs etc
	//HDC		hDC;		// compatible DC
	//HDRAWDIB		hDD;		// DrawDib handler
	//HBITMAP		hBitmap;		// for DIB conversions
	byte		*pframe_data;	// converted framedata
} movie_state_t;

static qboolean		avi_initialized = false;
static AVCodec		*avcodec;
static movie_state_t	avi[2];
// Converts a compressed audio stream into uncompressed PCM.
qboolean AVI_ACMConvertAudio( movie_state_t *Avi )
{
}

qboolean AVI_GetVideoInfo( movie_state_t *Avi, long *xres, long *yres, float *duration )
{
	ASSERT( Avi != NULL );

	if( !Avi->active )
		return false;

	if( xres != NULL )
		*xres = Avi->video_xres;

	if( yres != NULL )
		*yres = Avi->video_yres;

	if( duration != NULL )
		*duration = (float)Avi->video_frames / Avi->video_fps;

	return true;
}

// returns a unique frame identifier
long AVI_GetVideoFrameNumber( movie_state_t *Avi, float time )
{
	ASSERT( Avi != NULL );

	if( !Avi->active )
		return 0;

	return (time * Avi->video_fps);
}

// gets the raw frame data
byte *AVI_GetVideoFrame( movie_state_t *Avi, long frame )
{
}

qboolean AVI_GetAudioInfo( movie_state_t *Avi, wavdata_t *snd_info )
{
}

// sync the current audio read to a specific offset
qboolean AVI_SeekPosition( movie_state_t *Avi, dword offset )
{
}

// get a chunk of audio from the stream (in bytes)
fs_offset_t AVI_GetAudioChunk( movie_state_t *Avi, char *audiodata, long offset, long length )
{
}

void AVI_CloseVideo( movie_state_t *Avi )
{
	if(Avi->active)
		avformat_close_input(&Avi->avf_context);
}

void AVI_OpenVideo( movie_state_t *Avi, const char *filename, qboolean load_audio, qboolean ignore_hwgamma, int quiet )
{
	ASSERT( Avi != NULL );
	// default state: non-working.
	Avi->active = false;
	Avi->quiet = quiet;

	// can't load Video For Windows :-(
	if( !avi_initialized ) return;

	if( !filename ) return;

	int ret = avformat_open_input( &Avi->avf_context, filename, NULL, NULL );
	if( ret < 0 )
	{
		// Error handling
		char avf_error[1024];
		av_strerror( ret, avf_error, sizeof( avf_error));
		MsgDev( D_ERROR, "AVI_OpenVideo: %s\n", avf_error );
		return;
	}
}

qboolean AVI_IsActive( movie_state_t *Avi )
{
	if( Avi != NULL )
		return Avi->active;
	return false;
}

movie_state_t *AVI_GetState( int num )
{
	return &avi[num];
}

/*
=============
AVIKit user interface

=============
*/
movie_state_t *AVI_LoadVideo( const char *filename, qboolean load_audio, qboolean ignore_hwgamma )
{
}

movie_state_t *AVI_LoadVideoNoSound( const char *filename, qboolean ignore_hwgamma )
{
	return AVI_LoadVideo( filename, false, ignore_hwgamma );
}

void AVI_FreeVideo( movie_state_t *state )
{
	if( !state ) return;

	if( Mem_IsAllocatedExt( cls.mempool, state ))
	{
		AVI_CloseVideo( state );
		Mem_Free( state );
	}
}

qboolean AVI_Initailize( void )
{
	if( Sys_CheckParm( "-noavi" ))
	{
		MsgDev( D_INFO, "AVI: Disabled\n" );
		return false;
	}

	av_register_all();

	avi_initialized = true;
	MsgDev( D_NOTE, "AVI_Initailize: done\n" );
		
	return true;
}

void AVI_Shutdown( void )
{
	if( !avi_initialized ) return;

	avi_initialized = false;
}
