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
#if defined(_WIN32) && !defined(XASH_DEDICATED)
#define USE_VFW
#endif
#ifdef USE_VFW
#ifdef __MINGW32__
#include <mmreg.h>
#include <msacm.h>

#endif
#include <vfw.h> // video for windows

// msvfw32.dll exports
static HDRAWDIB (_stdcall *pDrawDibOpen)( void );
static BOOL (_stdcall *pDrawDibClose)( HDRAWDIB hdd );
static BOOL (_stdcall *pDrawDibDraw)( HDRAWDIB, HDC, int, int, int, int, LPBITMAPINFOHEADER, void*, int, int, int, int, uint );

static dllfunc_t msvfw_funcs[] =
{
{ "DrawDibOpen", (void **) &pDrawDibOpen },
{ "DrawDibDraw", (void **) &pDrawDibDraw },
{ "DrawDibClose", (void **) &pDrawDibClose },
{ NULL, NULL }
};

dll_info_t msvfw_dll = { "msvfw32.dll", msvfw_funcs, false };

// msacm32.dll exports
static MMRESULT (_stdcall *pacmStreamOpen)( LPHACMSTREAM, HACMDRIVER, LPWAVEFORMATEX, LPWAVEFORMATEX, LPWAVEFILTER, DWORD, DWORD, DWORD );
static MMRESULT (_stdcall *pacmStreamPrepareHeader)( HACMSTREAM, LPACMSTREAMHEADER, DWORD );
static MMRESULT (_stdcall *pacmStreamUnprepareHeader)( HACMSTREAM, LPACMSTREAMHEADER, DWORD );
static MMRESULT (_stdcall *pacmStreamConvert)( HACMSTREAM, LPACMSTREAMHEADER, DWORD );
static MMRESULT (_stdcall *pacmStreamSize)( HACMSTREAM, DWORD, LPDWORD, DWORD );
static MMRESULT (_stdcall *pacmStreamClose)( HACMSTREAM, DWORD );

static dllfunc_t msacm_funcs[] =
{
{ "acmStreamOpen", (void **) &pacmStreamOpen },
{ "acmStreamPrepareHeader", (void **) &pacmStreamPrepareHeader },
{ "acmStreamUnprepareHeader", (void **) &pacmStreamUnprepareHeader },
{ "acmStreamConvert", (void **) &pacmStreamConvert },
{ "acmStreamSize", (void **) &pacmStreamSize },
{ "acmStreamClose", (void **) &pacmStreamClose },
{ NULL, NULL }
};

dll_info_t msacm_dll = { "msacm32.dll", msacm_funcs, false };

// avifil32.dll exports
static int (_stdcall *pAVIStreamInfo)( PAVISTREAM pavi, AVISTREAMINFO *psi, LONG lSize );
static int (_stdcall *pAVIStreamRead)( PAVISTREAM pavi, LONG lStart, LONG lSamples, void *lpBuffer, LONG cbBuffer, LONG *plBytes, LONG *plSamples );
static PGETFRAME (_stdcall *pAVIStreamGetFrameOpen)( PAVISTREAM pavi, LPBITMAPINFOHEADER lpbiWanted );
static void* (_stdcall *pAVIStreamGetFrame)( PGETFRAME pg, LONG lPos );
static int (_stdcall *pAVIStreamGetFrameClose)( PGETFRAME pg );
static dword (_stdcall *pAVIStreamRelease)( PAVISTREAM pavi );
static int (_stdcall *pAVIFileOpen)( PAVIFILE *ppfile, LPCSTR szFile, UINT uMode, LPCLSID lpHandler );
static int (_stdcall *pAVIFileGetStream)( PAVIFILE pfile, PAVISTREAM *ppavi, DWORD fccType, LONG lParam );
static int (_stdcall *pAVIStreamReadFormat)( PAVISTREAM pavi, LONG lPos,LPVOID lpFormat, LONG *lpcbFormat );
static long (_stdcall *pAVIStreamStart)( PAVISTREAM pavi );
static dword (_stdcall *pAVIFileRelease)( PAVIFILE pfile );
static void (_stdcall *pAVIFileInit)( void );
static void (_stdcall *pAVIFileExit)( void );

static dllfunc_t avifile_funcs[] =
{
{ "AVIFileExit", (void **) &pAVIFileExit },
{ "AVIFileGetStream", (void **) &pAVIFileGetStream },
{ "AVIFileInit", (void **) &pAVIFileInit },
{ "AVIFileOpenA", (void **) &pAVIFileOpen },
{ "AVIFileRelease", (void **) &pAVIFileRelease },
{ "AVIStreamGetFrame", (void **) &pAVIStreamGetFrame },
{ "AVIStreamGetFrameClose", (void **) &pAVIStreamGetFrameClose },
{ "AVIStreamGetFrameOpen", (void **) &pAVIStreamGetFrameOpen },
{ "AVIStreamInfoA", (void **) &pAVIStreamInfo },
{ "AVIStreamRead", (void **) &pAVIStreamRead },
{ "AVIStreamReadFormat", (void **) &pAVIStreamReadFormat },
{ "AVIStreamRelease", (void **) &pAVIStreamRelease },
{ "AVIStreamStart", (void **) &pAVIStreamStart },
{ NULL, NULL }
};

dll_info_t avifile_dll = { "avifil32.dll", avifile_funcs, false };
typedef struct movie_state_s
{
	qboolean		active;
	qboolean		ignore_hwgamma;	// ignore hw gamma-correction
	qboolean		quiet;		// ignore error messages

	PAVIFILE		pfile;		// avi file pointer
	PAVISTREAM	video_stream;	// video stream pointer
	PGETFRAME		video_getframe;	// pointer to getframe object for video stream
	long		video_frames;	// total frames
	long		video_xres;	// video stream resolution
	long		video_yres;
	float		video_fps;	// video stream fps

	PAVISTREAM	audio_stream;	// audio stream pointer
	WAVEFORMAT	*audio_header;	// audio stream header
	long		audio_header_size;	// WAVEFORMAT is returned for PCM data; WAVEFORMATEX for others
	long		audio_codec;	// WAVE_FORMAT_PCM is oldstyle: anything else needs conversion
	long		audio_length;	// in converted samples
	long		audio_bytes_per_sample; // guess.

	// compressed audio specific data
	dword		cpa_blockalign;	// block size to read
	HACMSTREAM	cpa_conversion_stream;
	ACMSTREAMHEADER	cpa_conversion_header;
	byte		*cpa_srcbuffer;	// maintained buffer for raw data
	byte		*cpa_dstbuffer;

	dword		cpa_blocknum;	// current block
	dword		cpa_blockpos;	// read position in current block
	dword		cpa_blockoffset;	// corresponding offset in bytes in the output stream

	// for additional unpack Ms-RLE codecs etc
	HDC		hDC;		// compatible DC
	HDRAWDIB		hDD;		// DrawDib handler
	HBITMAP		hBitmap;		// for DIB conversions
	byte		*pframe_data;	// converted framedata
} movie_state_t;

static qboolean		avi_initialized = false;
static movie_state_t	avi[2];
#ifndef ACM_STREAMSIZEF_SOURCE
#define ACM_STREAMSIZEF_SOURCE 0

#define ACM_STREAMCONVERTF_BLOCKALIGN 0x00000004
#define ACM_STREAMCONVERTF_START      0x00000010
#define ACM_STREAMCONVERTF_END        0x00000020
#endif
#endif
// Converts a compressed audio stream into uncompressed PCM.
qboolean AVI_ACMConvertAudio( movie_state_t *Avi )
{
#ifdef USE_VFW
	WAVEFORMATEX	dest_header, *sh, *dh;
	AVISTREAMINFO	stream_info;
	dword		dest_length;
	short		bits;

	ASSERT( Avi != NULL );
    
    // WMA codecs, both versions - they simply don't work.
	if( Avi->audio_header->wFormatTag == 0x160 || Avi->audio_header->wFormatTag == 0x161 )
	{
		if( !Avi->quiet ) MsgDev( D_ERROR, "ACM does not support this audio codec.\n" );
		return false;
	}

	// get audio stream info to work with
	pAVIStreamInfo( Avi->audio_stream, &stream_info, sizeof( stream_info ));

	if( Avi->audio_header_size < sizeof( WAVEFORMATEX ))
	{
		if( !Avi->quiet ) MsgDev( D_ERROR, "ACM failed to open conversion stream.\n" );
		return false;
	}

	sh = (WAVEFORMATEX *)Avi->audio_header;
	bits = 16; // predict state

	// how much of this is actually required?
	dest_header.wFormatTag = WAVE_FORMAT_PCM; // yay
	dest_header.wBitsPerSample = bits; // 16bit
	dest_header.nChannels = sh->nChannels;
	dest_header.nSamplesPerSec = sh->nSamplesPerSec; // take straight from the source stream
	dest_header.nAvgBytesPerSec = (bits >> 3) * sh->nChannels * sh->nSamplesPerSec;
	dest_header.nBlockAlign = (bits >> 3) * sh->nChannels;
	dest_header.cbSize = 0; // no more data.

	dh = &dest_header;

	// open the stream
	if( pacmStreamOpen( &Avi->cpa_conversion_stream, NULL, sh, dh, NULL, 0, 0, 0 ) != MMSYSERR_NOERROR )
	{
		// try with 8 bit destination instead
		bits = 8;

		dest_header.wBitsPerSample = bits; // 8bit
		dest_header.nAvgBytesPerSec = ( bits >> 3 ) * sh->nChannels * sh->nSamplesPerSec;
		dest_header.nBlockAlign = ( bits >> 3 ) * sh->nChannels; // 1 sample at a time

		if( pacmStreamOpen( &Avi->cpa_conversion_stream, NULL, sh, dh, NULL, 0, 0, 0 ) != MMSYSERR_NOERROR )
		{
			if( !Avi->quiet ) MsgDev( D_ERROR, "ACM failed to open conversion stream.\n" );
			return false;
		}
	}

	Avi->cpa_blockalign = sh->nBlockAlign;
	dest_length = 0;

	// mp3 specific fix
	if( sh->wFormatTag == 0x55 )
	{
		LPMPEGLAYER3WAVEFORMAT	k;

		k = (LPMPEGLAYER3WAVEFORMAT)sh;
		Avi->cpa_blockalign = k->nBlockSize;
	}

	// get the size of the output buffer for streaming the compressed audio
	if( pacmStreamSize( Avi->cpa_conversion_stream, Avi->cpa_blockalign, &dest_length, ACM_STREAMSIZEF_SOURCE ) != MMSYSERR_NOERROR )
	{
		if( !Avi->quiet ) MsgDev( D_ERROR, "Couldn't get ACM conversion stream size.\n" );
		pacmStreamClose( Avi->cpa_conversion_stream, 0 );
		return false;
	}

	Avi->cpa_srcbuffer = (byte *)Mem_Alloc( cls.mempool, Avi->cpa_blockalign );
	Avi->cpa_dstbuffer = (byte *)Mem_Alloc( cls.mempool, dest_length ); // maintained buffer for raw data

	// prep the headers!
	Avi->cpa_conversion_header.cbStruct = sizeof( ACMSTREAMHEADER );
	Avi->cpa_conversion_header.fdwStatus = 0;
	Avi->cpa_conversion_header.dwUser = 0;				// no user data
	Avi->cpa_conversion_header.pbSrc = Avi->cpa_srcbuffer;		// source buffer
	Avi->cpa_conversion_header.cbSrcLength = Avi->cpa_blockalign;	// source buffer size
	Avi->cpa_conversion_header.cbSrcLengthUsed = 0;
	Avi->cpa_conversion_header.dwSrcUser = 0;			// no user data
	Avi->cpa_conversion_header.pbDst = Avi->cpa_dstbuffer;		// dest buffer
	Avi->cpa_conversion_header.cbDstLength = dest_length;		// dest buffer size
	Avi->cpa_conversion_header.cbDstLengthUsed = 0;
	Avi->cpa_conversion_header.dwDstUser = 0;			// no user data

	if( pacmStreamPrepareHeader( Avi->cpa_conversion_stream, &Avi->cpa_conversion_header, 0 ) != MMSYSERR_NOERROR )
	{
		if( !Avi->quiet ) MsgDev( D_ERROR, "couldn't prep headers.\n" );
		pacmStreamClose( Avi->cpa_conversion_stream, 0 );
		return false;
	}

	Avi->cpa_blocknum = 0; // start at 0.
	Avi->cpa_blockpos = 0;
	Avi->cpa_blockoffset = 0;

	pAVIStreamRead( Avi->audio_stream, Avi->cpa_blocknum * Avi->cpa_blockalign, Avi->cpa_blockalign, Avi->cpa_srcbuffer, Avi->cpa_blockalign, NULL, NULL );
	pacmStreamConvert( Avi->cpa_conversion_stream, &Avi->cpa_conversion_header, ACM_STREAMCONVERTF_BLOCKALIGN|ACM_STREAMCONVERTF_START );

	// convert first chunk twice. it often fails the first time. BLACK MAGIC.
	pAVIStreamRead( Avi->audio_stream, Avi->cpa_blocknum * Avi->cpa_blockalign, Avi->cpa_blockalign, Avi->cpa_srcbuffer, Avi->cpa_blockalign, NULL, NULL );
	pacmStreamConvert( Avi->cpa_conversion_stream, &Avi->cpa_conversion_header, ACM_STREAMCONVERTF_BLOCKALIGN );

	Avi->audio_bytes_per_sample = (bits >> 3 ) * Avi->audio_header->nChannels;

	return true;
#else
	return false;
#endif
}

qboolean AVI_GetVideoInfo( movie_state_t *Avi, long *xres, long *yres, float *duration )
{
#ifdef USE_VFW
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
#else
	return false;
#endif
}

// returns a unique frame identifier
long AVI_GetVideoFrameNumber( movie_state_t *Avi, float time )
{
#ifdef USE_VFW
	ASSERT( Avi != NULL );

	if( !Avi->active )
		return 0;

	return (time * Avi->video_fps);
#else
	return 0;
#endif
}

// gets the raw frame data
byte *AVI_GetVideoFrame( movie_state_t *Avi, long frame )
{
#ifdef USE_VFW
	LPBITMAPINFOHEADER	frame_info;
	byte		*frame_raw, *tmp;
	int		i;

	ASSERT( Avi != NULL );

	if( !Avi->active ) return NULL;

	if( frame >= Avi->video_frames )
		frame = Avi->video_frames - 1;

	frame_info = (LPBITMAPINFOHEADER)pAVIStreamGetFrame( Avi->video_getframe, frame );
	frame_raw = (byte *)frame_info + frame_info->biSize + frame_info->biClrUsed * sizeof( RGBQUAD );
	pDrawDibDraw( Avi->hDD, Avi->hDC, 0, 0, Avi->video_xres, Avi->video_yres, frame_info, frame_raw, 0, 0, Avi->video_xres, Avi->video_yres, 0 );

	// adjust gamma only if hardware gamma is enabled
	if( Avi->ignore_hwgamma && glConfig.deviceSupportsGamma )
	{
		tmp = Avi->pframe_data;

		// renormalize gamma
		for( i = 0; i < Avi->video_xres * Avi->video_yres * 4; i++, tmp++ )
			*tmp = clgame.ds.gammaTable[*tmp];
	}

	return Avi->pframe_data;
#else
	return NULL;
#endif
}

qboolean AVI_GetAudioInfo( movie_state_t *Avi, wavdata_t *snd_info )
{
#ifdef USE_VFW
	ASSERT( Avi != NULL );

	if( !Avi->active || Avi->audio_stream == NULL || snd_info == NULL )
	{
		return false;
	}

	snd_info->rate = Avi->audio_header->nSamplesPerSec;
	snd_info->channels = Avi->audio_header->nChannels;

	if( Avi->audio_codec == WAVE_FORMAT_PCM ) // uncompressed audio!
		snd_info->width = ( Avi->audio_bytes_per_sample > Avi->audio_header->nChannels ) ? 2 : 1;
	else snd_info->width = 2; // assume compressed audio is always 16 bit

	snd_info->size = snd_info->rate * snd_info->width * snd_info->channels;
	snd_info->loopStart = 0;	// HACKHACK: use loopStart as streampos

	return true;
#else
	return false;
#endif
}

// sync the current audio read to a specific offset
qboolean AVI_SeekPosition( movie_state_t *Avi, dword offset )
{
#ifdef USE_VFW
	int	breaker;

	ASSERT( Avi != NULL );

	if( offset < Avi->cpa_blockoffset ) // well, shit. we can't seek backwards... restart
	{
		if( Avi->cpa_blockoffset - offset < 500000 )
			return false; // don't bother if it's gonna catch up soon (cheap hack! works!)

		Avi->cpa_blocknum = 0; // start at 0, eh.
		Avi->cpa_blockpos = 0;
		Avi->cpa_blockoffset = 0;

		pAVIStreamRead( Avi->audio_stream, Avi->cpa_blocknum * Avi->cpa_blockalign, Avi->cpa_blockalign, Avi->cpa_srcbuffer, Avi->cpa_blockalign, NULL, NULL );
		pacmStreamConvert( Avi->cpa_conversion_stream, &Avi->cpa_conversion_header, ACM_STREAMCONVERTF_BLOCKALIGN|ACM_STREAMCONVERTF_START );

		// convert first chunk twice. it often fails the first time. BLACK MAGIC.
		pAVIStreamRead( Avi->audio_stream, Avi->cpa_blocknum * Avi->cpa_blockalign, Avi->cpa_blockalign, Avi->cpa_srcbuffer, Avi->cpa_blockalign, NULL, NULL );
		pacmStreamConvert( Avi->cpa_conversion_stream, &Avi->cpa_conversion_header, ACM_STREAMCONVERTF_BLOCKALIGN );
	}

	// now then: seek forwards to the required block
	breaker = 30; // maximum zero blocks: anti-freeze protection

	while( Avi->cpa_blockoffset + Avi->cpa_conversion_header.cbDstLengthUsed < offset )
	{
		Avi->cpa_blocknum++;
		Avi->cpa_blockoffset += Avi->cpa_conversion_header.cbDstLengthUsed;

		pAVIStreamRead( Avi->audio_stream, Avi->cpa_blocknum * Avi->cpa_blockalign, Avi->cpa_blockalign, Avi->cpa_srcbuffer, Avi->cpa_blockalign, NULL, NULL );
		pacmStreamConvert( Avi->cpa_conversion_stream, &Avi->cpa_conversion_header, ACM_STREAMCONVERTF_BLOCKALIGN );

		if( Avi->cpa_conversion_header.cbDstLengthUsed == 0 )
			breaker--;
		else breaker = 30;

		if( breaker <= 0 )
			return false;

		Avi->cpa_blockpos = 0;
	}

	// seek to the right position inside the block
	Avi->cpa_blockpos = offset - Avi->cpa_blockoffset;

	return true;
#else
	return false;
#endif
}

// get a chunk of audio from the stream (in bytes)
fs_offset_t AVI_GetAudioChunk( movie_state_t *Avi, char *audiodata, long offset, long length )
{
#ifdef USE_VFW
	int	i;
	long	result = 0;

	ASSERT( Avi != NULL );

	// zero data past the end of the file
	if( offset + length > Avi->audio_length )
	{
		if( offset <= Avi->audio_length )
		{
			long	remaining_length = Avi->audio_length - offset;

			AVI_GetAudioChunk( Avi, audiodata, offset, remaining_length );

			for( i = remaining_length; i < length; i++ )
				audiodata[i] = 0;
		}
		else
		{
			for( i = 0; i < length; i++ )
				audiodata[i] = 0;
		}
	}

	// uncompressed audio!
	if( Avi->audio_codec == WAVE_FORMAT_PCM )
	{
		// very simple - read straight out
		pAVIStreamRead( Avi->audio_stream, offset / Avi->audio_bytes_per_sample, length / Avi->audio_bytes_per_sample, audiodata, length, &result, NULL );
		return result;
	}
	else
	{
		// compressed audio!
		result = 0;

		// seek to correct chunk and all that stuff
		if( !AVI_SeekPosition( Avi, offset )) 
			return 0; // don't continue if we're waiting for the play pointer to catch up

		while( length > 0 )
		{
			long	blockread = Avi->cpa_conversion_header.cbDstLengthUsed - Avi->cpa_blockpos;

			if( blockread <= 0 ) // read next
			{
				Avi->cpa_blocknum++;
				Avi->cpa_blockoffset += Avi->cpa_conversion_header.cbDstLengthUsed;

				pAVIStreamRead( Avi->audio_stream, Avi->cpa_blocknum * Avi->cpa_blockalign, Avi->cpa_blockalign, Avi->cpa_srcbuffer, Avi->cpa_blockalign, NULL, NULL );
				pacmStreamConvert( Avi->cpa_conversion_stream, &Avi->cpa_conversion_header, ACM_STREAMCONVERTF_BLOCKALIGN );

				Avi->cpa_blockpos = 0;
				continue;
			}

			if( blockread > length )
				blockread = length;

			// copy the data
			Q_memcpy( audiodata + result, (void *)( Avi->cpa_dstbuffer + Avi->cpa_blockpos ), blockread );

			Avi->cpa_blockpos += blockread;
			result += blockread;
			length -= blockread;
		}

		return result;
	}
#else
	return 0;
#endif
}

void AVI_CloseVideo( movie_state_t *Avi )
{
#ifdef USE_VFW
	ASSERT( Avi != NULL );

	if( Avi->active )
	{
		pAVIStreamGetFrameClose( Avi->video_getframe );

		if( Avi->audio_stream != NULL )
		{
			pAVIStreamRelease( Avi->audio_stream );
			Mem_Free( Avi->audio_header );

			if( Avi->audio_codec != WAVE_FORMAT_PCM )
			{
				pacmStreamUnprepareHeader( Avi->cpa_conversion_stream, &Avi->cpa_conversion_header, 0 );
				pacmStreamClose( Avi->cpa_conversion_stream, 0 );
				Mem_Free( Avi->cpa_srcbuffer );
				Mem_Free( Avi->cpa_dstbuffer );
			}
		}

		pAVIStreamRelease( Avi->video_stream );

		DeleteObject( Avi->hBitmap );
		pDrawDibClose( Avi->hDD );
		DeleteDC( Avi->hDC );
	}

	Q_memset( Avi, 0, sizeof( movie_state_t ));
#endif
}

void AVI_OpenVideo( movie_state_t *Avi, const char *filename, qboolean load_audio, qboolean ignore_hwgamma, int quiet )
{
#ifdef USE_VFW
	BITMAPINFOHEADER	bmih;
	AVISTREAMINFO	stream_info;
	long		opened_streams = 0;
	LONG		hr;

	ASSERT( Avi != NULL );

	// default state: non-working.
	Avi->active = false;
	Avi->quiet = quiet;

	// can't load Video For Windows :-(
	if( !avi_initialized ) return;

	// load the AVI
	hr = pAVIFileOpen( &Avi->pfile, filename, OF_SHARE_DENY_WRITE, 0L );

	if( hr != 0 ) // error opening AVI:
	{
		switch( hr )
		{
		case AVIERR_BADFORMAT:
			if( !Avi->quiet ) MsgDev( D_ERROR, "corrupt file or unknown format.\n" );
			break;
		case AVIERR_MEMORY:
			if( !Avi->quiet ) MsgDev( D_ERROR, "insufficient memory to open file.\n" );
			break;
		case AVIERR_FILEREAD:
			if( !Avi->quiet ) MsgDev( D_ERROR, "disk error reading file.\n" );
			break;
		case AVIERR_FILEOPEN:
			if( !Avi->quiet ) MsgDev( D_ERROR, "disk error opening file.\n" );
			break;
		case REGDB_E_CLASSNOTREG:
		default:
			if( !Avi->quiet ) MsgDev( D_ERROR, "no handler found (or file not found).\n" );
			break;
		}
		return;
	}

	Avi->video_stream = Avi->audio_stream = NULL;

	// open the streams until a stream is not available. 
	while( 1 )
	{
		PAVISTREAM	stream = NULL;

		if( pAVIFileGetStream( Avi->pfile, &stream, 0L, opened_streams++ ) != AVIERR_OK )
			break;

		if( stream == NULL )
			break;

		pAVIStreamInfo( stream, &stream_info, sizeof( stream_info ));

		if( stream_info.fccType == streamtypeVIDEO && Avi->video_stream == NULL )
		{
			Avi->video_stream = stream;
			Avi->video_frames = stream_info.dwLength;
			Avi->video_xres = stream_info.rcFrame.right - stream_info.rcFrame.left;
			Avi->video_yres = stream_info.rcFrame.bottom - stream_info.rcFrame.top;
			Avi->video_fps = (float)stream_info.dwRate / (float)stream_info.dwScale;
		}
		else if( stream_info.fccType == streamtypeAUDIO && Avi->audio_stream == NULL && load_audio )
		{
			long	size;

			Avi->audio_stream = stream;

			// read the audio header
			pAVIStreamReadFormat( Avi->audio_stream, pAVIStreamStart( Avi->audio_stream ), 0, &size );

			Avi->audio_header = (WAVEFORMAT *)Mem_Alloc( cls.mempool, size );
			pAVIStreamReadFormat( Avi->audio_stream, pAVIStreamStart( Avi->audio_stream ), Avi->audio_header, &size );
			Avi->audio_header_size = size;
			Avi->audio_codec = Avi->audio_header->wFormatTag;

			// length of converted audio in samples
			Avi->audio_length = (long)((float)stream_info.dwLength / Avi->audio_header->nAvgBytesPerSec );
			Avi->audio_length *= Avi->audio_header->nSamplesPerSec;

			if( Avi->audio_codec != WAVE_FORMAT_PCM )
			{
				if( !AVI_ACMConvertAudio( Avi ))
				{
					Mem_Free( Avi->audio_header );
					Avi->audio_stream = NULL;
					continue;
				}
			}
			else Avi->audio_bytes_per_sample = Avi->audio_header->nBlockAlign;
			Avi->audio_length *= Avi->audio_bytes_per_sample;
		} 
		else
		{
			pAVIStreamRelease( stream );
		}
	}

	// display error message-stream not found. 
	if( Avi->video_stream == NULL )
	{
		if( Avi->pfile ) // if file is open, close it 
			pAVIFileRelease( Avi->pfile );
		if( !Avi->quiet ) MsgDev( D_ERROR, "couldn't find a valid video stream.\n" );
		return;
	}

	pAVIFileRelease( Avi->pfile ); // release the file
	Avi->video_getframe = pAVIStreamGetFrameOpen( Avi->video_stream, NULL ); // open the frame getter

	if( Avi->video_getframe == NULL )
	{
		if( !Avi->quiet ) MsgDev( D_ERROR, "error attempting to read video frames.\n" );
		return; // couldn't open frame getter.
	}

	Avi->ignore_hwgamma = ignore_hwgamma;

	bmih.biSize = sizeof( BITMAPINFOHEADER );
	bmih.biPlanes = 1;	
	bmih.biBitCount = 32;
	bmih.biCompression = BI_RGB;	
	bmih.biWidth = Avi->video_xres;
	bmih.biHeight = -Avi->video_yres; // invert height to flip image upside down

	Avi->hDC = CreateCompatibleDC( 0 );
	Avi->hDD = pDrawDibOpen();
	Avi->hBitmap = CreateDIBSection( Avi->hDC, (BITMAPINFO*)(&bmih), DIB_RGB_COLORS, (void**)(&Avi->pframe_data), NULL, 0 );
	SelectObject( Avi->hDC, Avi->hBitmap );

	Avi->active = true; // done
#endif
}

qboolean AVI_IsActive( movie_state_t *Avi )
{
#ifdef USE_VFW
	if( Avi != NULL )
		return Avi->active;
#endif
	return false;
}

movie_state_t *AVI_GetState( int num )
{
#ifdef USE_VFW
	return &avi[num];
#else
	return NULL;
#endif
}

/*
=============
AVIKit user interface

=============
*/
movie_state_t *AVI_LoadVideo( const char *filename, qboolean load_audio, qboolean ignore_hwgamma )
{
#ifdef USE_VFW
	movie_state_t	*Avi;
	string		path;
	const char	*fullpath;

	// fast reject
	if( !avi_initialized )
	{
		MsgDev( D_ERROR, "AVI_LoadVideo: movie support is disabled\n" );
		return NULL;
	}	

	// open cinematic
	Q_snprintf( path, sizeof( path ), "media/%s", filename );
	FS_DefaultExtension( path, ".avi" );
	fullpath = FS_GetDiskPath( path, false );

	if( FS_FileExists( path, false ) && !fullpath )
	{
		MsgDev( D_ERROR, "AVI_LoadVideo: Couldn't load %s from packfile. Please extract it\n", path );
		return NULL;
	}

	Avi = Mem_Alloc( cls.mempool, sizeof( movie_state_t ));
	AVI_OpenVideo( Avi, fullpath, load_audio, ignore_hwgamma, false );

	if( !AVI_IsActive( Avi ))
	{
		AVI_FreeVideo( Avi ); // something bad happens
		return NULL;
	}

	// all done
	return Avi;
#else
	return NULL;
#endif
}

movie_state_t *AVI_LoadVideoNoSound( const char *filename, qboolean ignore_hwgamma )
{
	return AVI_LoadVideo( filename, false, ignore_hwgamma );
}

void AVI_FreeVideo( movie_state_t *state )
{
#ifdef USE_VFW
	if( !state ) return;

	if( Mem_IsAllocatedExt( cls.mempool, state ))
	{
		AVI_CloseVideo( state );
		Mem_Free( state );
	}
#endif
}

qboolean AVI_Initailize( void )
{
#ifdef USE_VFW
	if( Sys_CheckParm( "-noavi" ))
	{
		MsgDev( D_INFO, "AVI: Disabled\n" );
		return false;
	}

	if( !Sys_LoadLibrary( &avifile_dll ))
	{
		MsgDev( D_ERROR, "AVI_Initailize: failed\n" );
		return false;
	}

	if( !Sys_LoadLibrary( &msvfw_dll ))
	{
		MsgDev( D_ERROR, "AVI_Initailize: failed\n" );
		Sys_FreeLibrary( &avifile_dll );
		return false;
	}

	if( !Sys_LoadLibrary( &msacm_dll ))
	{
		MsgDev( D_ERROR, "AVI_Initailize: failed\n" );
		Sys_FreeLibrary( &avifile_dll );
		Sys_FreeLibrary( &msvfw_dll );
		return false;
	}
	
	avi_initialized = true;
	MsgDev( D_NOTE, "AVI_Initailize: done\n" );
		
	pAVIFileInit();
	return true;
#endif
	MsgDev( D_INFO, "AVI: Not supported\n" );
	return false;
}

void AVI_Shutdown( void )
{
#ifdef USE_VFW
	if( !avi_initialized ) return;

	pAVIFileExit();

	Sys_FreeLibrary( &avifile_dll );
	Sys_FreeLibrary( &msvfw_dll );
	Sys_FreeLibrary( &msacm_dll );
	avi_initialized = false;
#endif
}
