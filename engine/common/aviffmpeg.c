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
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#define STREAM( x ) Avi->avf_context->streams[x]

typedef struct movie_state_s
{
	qboolean		active;
	qboolean		ignore_hwgamma;	// ignore hw gamma-correction
	qboolean		quiet;		// ignore error messages

	AVFormatContext		*avf_context;
	AVFrame *frame;
	AVFrame *frameGL;

	struct SwsContext *sws_context;

	int	video_stream;	// video stream idx
	AVPacketList *video_packets;
	AVPacketList *video_last_packet;
	float video_fps;
	  double          video_clock; // pts of last decoded frame / predicted pts of next decoded frame

	int audio_stream;	// audio stream idx
	AVPacketList *audio_packets;
	AVPacketList *audio_last_packet;
} movie_state_t;

static qboolean		avi_initialized = false;
static movie_state_t	avi[2];

qboolean AVI_GetVideoInfo( movie_state_t *Avi, long *xres, long *yres, float *duration )
{
	ASSERT( Avi != NULL );

	if( !Avi->active && Avi->video_stream == -1 )
		return false;

	if( xres != NULL )
		*xres = STREAM(Avi->video_stream)->codec->width;

	if( yres != NULL )
		*yres = STREAM(Avi->video_stream)->codec->height;

	if( duration != NULL )
		*duration = (float)Avi->avf_context->duration / AV_TIME_BASE;
	return true;
}

// returns a unique frame identifier
long AVI_GetVideoFrameNumber( movie_state_t *Avi, float time )
{
	ASSERT( Avi != NULL );

	if( !Avi->active )
		return 0;

	return time * Avi->video_fps;
}

// gets the raw frame data
byte *AVI_GetVideoFrame( movie_state_t *Avi, long frame )
{
	int i = 0, ret, pts = 0;
	AVPacket *packet;

	if( !Avi->active )
		return NULL;

	if( !Avi->video_last_packet )
		return NULL;

	for(;;)
	{
		packet = Avi->video_last_packet;
		Avi->video_last_packet = Avi->video_last_packet->next;
		ret = avcodec_decode_video2( STREAM(Avi->video_stream)->codec, Avi->frame, &i, packet );
		if( ret < 0 )
			return NULL;

		// Frame sucessfully decompressed, so don't skip it
		if( i != 0 )
			break;
	}

	sws_scale( Avi->sws_context, Avi->frame->data, Avi->frame->linesize, 0,
			STREAM(Avi->video_stream)->codec->height,
			Avi->frameGL->data, Avi->frameGL->linesize);


	return Avi->frameGL->data[0];
}

qboolean AVI_GetAudioInfo( movie_state_t *Avi, wavdata_t *snd_info )
{
	ASSERT( Avi != NULL );

	if( !Avi->active || Avi->audio_stream == -1 || !snd_info )
	{
		return false;
	}

	snd_info->loopStart = 0;
	snd_info->channels = STREAM(Avi->audio_stream)->codec->channels;
	snd_info->rate = STREAM(Avi->audio_stream)->codec->sample_rate;
	switch(STREAM(Avi->audio_stream)->codec->sample_fmt)
	{
	case AV_SAMPLE_FMT_U8:
	case AV_SAMPLE_FMT_U8P:
		snd_info->width = 1;break;
	default:
		snd_info->width = 2;
	}

	snd_info->size = snd_info->rate * snd_info->width * snd_info->channels;

	return true;
}

// sync the current audio read to a specific offset
qboolean AVI_SeekPosition( movie_state_t *Avi, dword offset )
{
	if( av_seek_frame( Avi->avf_context, Avi->audio_stream, offset, 0 ) < 0)
		return false;
	else return true;
}

// get a chunk of audio from the stream (in bytes)
fs_offset_t AVI_GetAudioChunk( movie_state_t *Avi, char *audiodata, long offset, long length )
{
	ASSERT( Avi != NULL );

	int i, ret;
	AVPacketList *packet;

	if( !Avi->active )
		return NULL;

	if( !Avi->audio_last_packet )
		return NULL;

	for(;;)
	{
		packet = Avi->audio_last_packet;
		Avi->audio_last_packet = Avi->audio_last_packet->next;

		ret = avcodec_decode_audio4( STREAM(Avi->audio_stream)->codec, Avi->frame, &i, packet );

		if( ret < 0 )
			return NULL;

		// Frame sucessfully decompressed, so don't skip it
		if( i != 0 )
			break;
	}


	Q_memcpy( audiodata, Avi->frame->data[0],
	Avi->frame->linesize[0] > 8192 ? 8192 : Avi->frame->linesize[0]);

	return ret;
}

void AVI_Demuxe( movie_state_t *Avi )
{
	AVPacketList *last_packet;
	while(true)
	{
		last_packet = (AVPacketList *)av_malloc(sizeof(AVPacketList));
		if(!last_packet)
			return;

		if( av_read_frame( Avi->avf_context, &last_packet->pkt ) < 0 )
			return;

		if( last_packet->pkt.stream_index == Avi->video_stream )
		{
			if( !Avi->video_packets )
			{
				Avi->video_packets = last_packet;
				Avi->video_last_packet = last_packet;
			}
			else
			{
				Avi->video_last_packet->next = last_packet;
				Avi->video_last_packet = last_packet;
				last_packet->next = NULL;
			}
		}
		else if( last_packet->pkt.stream_index == Avi->audio_stream )
		{
			if( !Avi->audio_packets )
			{
				Avi->audio_packets = last_packet;
				Avi->audio_last_packet = last_packet;
			}
			else
			{
				Avi->audio_last_packet->next = last_packet;
				Avi->audio_last_packet = last_packet;
				last_packet->next = NULL;
			}
		}
	}
}

void AVI_CloseVideo( movie_state_t *Avi )
{
	if( Avi->active )
	{
		//avcodec_free_frame( Avi->frame );
		while( Avi->video_packets )
		{
			AVPacketList *pktl = Avi->video_packets;
			Avi->video_packets = Avi->video_packets->next;
			av_free_packet(&pktl->pkt);
			av_freep(&pktl);
		}
		while( Avi->audio_packets )
		{
			AVPacketList *pktl = Avi->audio_packets;
			Avi->audio_packets = Avi->audio_packets->next;
			av_free_packet(&pktl->pkt);
			av_freep(&pktl);
		}
		sws_freeContext(Avi->sws_context);
		av_free(Avi->frame);
		av_free(Avi->frameGL);

		Avi->frame = NULL;
		Avi->frameGL = NULL;
		Avi->sws_context = NULL;
		avformat_close_input( &Avi->avf_context );
		Avi->active = false;
	}
}

void AVI_OpenVideo( movie_state_t *Avi, const char *filename, qboolean load_audio, qboolean ignore_hwgamma, int quiet )
{
	int ret, i;
	char avf_error[1024];

	ASSERT( Avi != NULL );

	if( !avi_initialized || !filename )
		return;

	// default state: non-working
	Avi->active = false;
	Avi->quiet = quiet;
	Avi->avf_context = Avi->audio_stream = Avi->video_stream = 0;
	Avi->frame = avcodec_alloc_frame();
	Avi->video_stream = Avi->audio_stream = -1;

	if( (ret = avformat_open_input( &Avi->avf_context, filename, NULL, NULL )) < 0 )
	{
		// Error handling
		av_strerror( ret, avf_error, sizeof( avf_error));
		MsgDev( D_ERROR, "AVI_OpenVideo: %s\n", avf_error );

		avformat_close_input( &Avi->avf_context );
		return;
	}

	if( (ret = avformat_find_stream_info( Avi->avf_context, NULL )) < 0)
	{
		av_strerror( ret, avf_error, sizeof( avf_error));
		MsgDev( D_ERROR, "AVI_OpenVideo: %s\n", avf_error );

		avformat_close_input( &Avi->avf_context );
		return;
	}

	// Find first video and audio stream
	for( i = 0; i < Avi->avf_context->nb_streams; i++ )
	{
		if( Avi->avf_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && Avi->video_stream == -1 )
		{
			Avi->video_stream = i;
			Avi->video_fps = (float)STREAM(i)->time_base.den / STREAM(i)->time_base.num;
			AVCodec *pCodec = avcodec_find_decoder(STREAM(i)->codec->codec_id);
			avcodec_open2(STREAM(i)->codec, pCodec, NULL);

			Avi->sws_context = sws_getContext(
				STREAM(i)->codec->width,
				STREAM(i)->codec->height,
				STREAM(i)->codec->pix_fmt,
				STREAM(i)->codec->width,
				STREAM(i)->codec->height, PIX_FMT_RGB32,
				SWS_FAST_BILINEAR, NULL, NULL, NULL);

			int size = avpicture_get_size(PIX_FMT_RGB32, STREAM(i)->codec->width,
				STREAM(i)->codec->height);
			Avi->frameGL = avcodec_alloc_frame();
			avpicture_fill((AVPicture*)Avi->frameGL, (uint8_t*) av_malloc(size * sizeof(uint8_t)),
				PIX_FMT_RGB32, STREAM(i)->codec->width,
				STREAM(i)->codec->height);

			continue;
		}
		if( Avi->avf_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && Avi->audio_stream == -1 && load_audio )
		{
			Avi->audio_stream = i;
			AVCodec *pCodec = avcodec_find_decoder(STREAM(i)->codec->codec_id);
			avcodec_open2(STREAM(i)->codec, pCodec, NULL);
			continue;
		}
	}

	Avi->video_packets = Avi->audio_packets = NULL;

	// Get all packets and put them to queue
	AVI_Demuxe( Avi );
	// Avi-> last video and audio packet now will be used for
	// fast access to latest decoded packet
	Avi->video_last_packet = Avi->video_packets;
	Avi->audio_last_packet = Avi->audio_packets;

	Avi->ignore_hwgamma = ignore_hwgamma;
	Avi->active = true;
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
	movie_state_t *Avi = Mem_Alloc( cls.mempool, sizeof( movie_state_t ));

	AVI_OpenVideo( Avi, filename, load_audio, ignore_hwgamma, false );

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
	avcodec_register_all();

	avi_initialized = true;
	MsgDev( D_NOTE, "AVI_Initailize: done\n" );
		
	return true;
}

void AVI_Shutdown( void )
{
	if( !avi_initialized ) return;

	avi_initialized = false;
}
