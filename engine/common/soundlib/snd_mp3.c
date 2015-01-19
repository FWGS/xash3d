/*
snd_mp3.c - mp3 format loading and streaming
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

#include "soundlib.h"

/*
=======================================================================
		MPG123 DEFINITION
=======================================================================
*/
#define MP3_ERR		-1
#define MP3_OK		0
#define MP3_NEED_MORE	1

#define FRAME_SIZE		16384	// must match with mp3 frame size

typedef struct mpeg_s
{
	void	*state;		// hidden decoder state
	void	*vbrtag;		// valid for VBR-encoded mpegs

	int	channels;		// num channels
	int	samples;		// per one second
	int	play_time;	// stream size in milliseconds
	int	rate;		// frequency
	int	outsize;		// current data size
	char	out[8192];	// temporary buffer
	size_t	streamsize;	// size in bytes
} mpeg_t;

// mpg123 exports
int create_decoder( mpeg_t *mpg );
int read_mpeg_header( mpeg_t *mpg, const char *data, long bufsize, long streamsize );
int read_mpeg_stream( mpeg_t *mpg, const char *data, long bufsize );
extern int set_current_pos( mpeg_t *mpg, int newpos, int (*pfnSeek)( void*, long, int ), void *file );
int get_current_pos( mpeg_t *mpg, int curpos );
void close_decoder( mpeg_t *mpg );

/*
=================================================================

	MPEG decompression

=================================================================
*/
qboolean Sound_LoadMPG( const char *name, const byte *buffer, size_t filesize )
{
	mpeg_t	mpeg;
	size_t	pos = 0;
	size_t	bytesWrite = 0;

	// load the file
	if( !buffer || filesize < FRAME_SIZE )
		return false;

	// couldn't create decoder
	if( !create_decoder( &mpeg ))
		return false;

	// trying to read header
	if( !read_mpeg_header( &mpeg, buffer, FRAME_SIZE, filesize ))
	{
		MsgDev( D_ERROR, "Sound_LoadMPG: (%s) is probably corrupted\n", name );
		close_decoder( &mpeg );
		return false;
	}

	sound.channels = mpeg.channels;
	sound.rate = mpeg.rate;
	sound.width = 2; // always 16-bit PCM
	sound.loopstart = -1;
	sound.size = ( sound.channels * sound.rate * sound.width ) * ( mpeg.play_time / 1000 ); // in bytes
	pos += FRAME_SIZE;	// evaluate pos

	if( !sound.size )
	{
		// bad mpeg file ?
		MsgDev( D_ERROR, "Sound_LoadMPG: (%s) is probably corrupted\n", name );
		close_decoder( &mpeg );
		return false;
	}

	sound.type = WF_PCMDATA;
	sound.wav = (byte *)Mem_Alloc( host.soundpool, sound.size );

	// decompress mpg into pcm wav format
	while( bytesWrite < sound.size )
	{
		int	outsize;

		if( read_mpeg_stream( &mpeg, NULL, 0 ) != MP3_OK )
		{
			char	*data = (char *)buffer + pos;
			int	bufsize;

			// if there are no bytes remainig so we can decompress the new frame
			if( pos + FRAME_SIZE > filesize )
				bufsize = ( filesize - pos );
			else bufsize = FRAME_SIZE;
			pos += bufsize;

			if( read_mpeg_stream( &mpeg, data, bufsize ) != MP3_OK )
				break; // there was end of the stream
		}

		if( bytesWrite + mpeg.outsize > sound.size )
			outsize = ( sound.size - bytesWrite );
		else outsize = mpeg.outsize;

		Q_memcpy( &sound.wav[bytesWrite], mpeg.out, outsize );
		bytesWrite += outsize;
	}

	sound.samples = bytesWrite / ( sound.width * sound.channels );
	close_decoder( &mpeg );

	return true;
}

/*
=================
Stream_OpenMPG
=================
*/
stream_t *Stream_OpenMPG( const char *filename )
{
	mpeg_t	*mpegFile;
	stream_t	*stream;
	file_t	*file;
	long	filesize, read_len;
	char	tempbuff[FRAME_SIZE];

	file = FS_Open( filename, "rb", false );
	if( !file ) return NULL;

	filesize = FS_FileLength( file );
	if( filesize < FRAME_SIZE )
	{
		MsgDev( D_ERROR, "Stream_OpenMPG: %s is probably corrupted\n", filename );
		FS_Close( file );
		return NULL;
	}

	// at this point we have valid stream
	stream = Mem_Alloc( host.soundpool, sizeof( stream_t ));
	stream->file = file;
	stream->pos = 0;

	mpegFile = Mem_Alloc( host.soundpool, sizeof( mpeg_t ));

	// couldn't create decoder
	if( !create_decoder( mpegFile ))
	{
		MsgDev( D_ERROR, "Stream_OpenMPG: couldn't create decoder\n" );
		Mem_Free( mpegFile );
		Mem_Free( stream );
		FS_Close( file );
		return NULL;
	}

	read_len = FS_Read( file, tempbuff, sizeof( tempbuff ));
	if( read_len < sizeof( tempbuff ))
	{
		MsgDev( D_ERROR, "Stream_OpenMPG: %s is probably corrupted\n", filename );
		close_decoder( mpegFile );
		Mem_Free( mpegFile );
		Mem_Free( stream );
		FS_Close( file );
		return NULL;
	}

	// trying to read header
	if( !read_mpeg_header( mpegFile, tempbuff, sizeof( tempbuff ), filesize ))
	{
		MsgDev( D_ERROR, "Sound_LoadMPG: (%s) is probably corrupted\n", filename );
		close_decoder( mpegFile );
		Mem_Free( mpegFile );
		Mem_Free( stream );
		FS_Close( file );
		return NULL;
	}

	stream->buffsize = 0; // how many samples left from previous frame
	stream->channels = mpegFile->channels;
	stream->pos += mpegFile->outsize;
	stream->rate = mpegFile->rate;
	stream->width = 2;	// always 16 bit
	stream->ptr = mpegFile;
	stream->type = WF_MPGDATA;

	return stream;
}

/*
=================
Stream_ReadMPG

assume stream is valid
=================
*/
long Stream_ReadMPG( stream_t *stream, long needBytes, void *buffer )
{
	// buffer handling
	int	bytesWritten = 0;
	int	result;
	mpeg_t	*mpg;

	mpg = (mpeg_t *)stream->ptr;
	ASSERT( mpg != NULL );

	while( 1 )
	{
		char	tempbuff[FRAME_SIZE];
		long	read_len, outsize;
		byte	*data;

		if( !stream->buffsize )
		{
			if( stream->timejump )
			{
				int numReads = 0;

				// flush all the previous data				
				while( read_mpeg_stream( mpg, NULL, 0 ) == MP3_OK && numReads++ < 255 );
				read_len = FS_Read( stream->file, tempbuff, sizeof( tempbuff ));
				result = read_mpeg_stream( mpg, tempbuff, read_len );
				stream->timejump = false;
				bytesWritten = 0;
			}
			else result = read_mpeg_stream( mpg, NULL, 0 );

			stream->pos += mpg->outsize;

			if( result != MP3_OK )
			{
				// if there are no bytes remainig so we can decompress the new frame
				read_len = FS_Read( stream->file, tempbuff, sizeof( tempbuff ));
				result = read_mpeg_stream( mpg, tempbuff, read_len );
				stream->pos += mpg->outsize;

				if( result != MP3_OK )
					break; // there was end of the stream
			}
		}

		// check remaining size
		if( bytesWritten + mpg->outsize > needBytes )
			outsize = ( needBytes - bytesWritten ); 
		else outsize = mpg->outsize;

		// copy raw sample to output buffer
		data = (byte *)buffer + bytesWritten;
		Q_memcpy( data, &mpg->out[stream->buffsize], outsize );
		bytesWritten += outsize;
		mpg->outsize -= outsize;
		stream->buffsize += outsize;

		// continue from this sample on a next call
		if( bytesWritten >= needBytes )
			return bytesWritten;

		stream->buffsize = 0; // no bytes remaining
	}

	return 0;
}

/*
=================
Stream_SetPosMPG

assume stream is valid
=================
*/
long Stream_SetPosMPG( stream_t *stream, long newpos )
{
	// update stream pos for right work GetPos function
	int newPos = set_current_pos( stream->ptr, newpos, FS_Seek, stream->file );

	if( newPos != -1 )
	{
		stream->pos = newPos;
		stream->timejump = true;
		stream->buffsize = 0;
		return true;
	}

	// failed to seek for some reasons
	return false;
}

/*
=================
Stream_GetPosMPG

assume stream is valid
=================
*/
long Stream_GetPosMPG( stream_t *stream )
{
	return get_current_pos( stream->ptr, stream->pos );
}

/*
=================
Stream_FreeMPG

assume stream is valid
=================
*/
void Stream_FreeMPG( stream_t *stream )
{
	if( stream->ptr )
	{
		mpeg_t	*mpg;

		mpg = (mpeg_t *)stream->ptr;
		close_decoder( mpg );
		Mem_Free( stream->ptr );
	}

	if( stream->file )
	{
		FS_Close( stream->file );
	}

	Mem_Free( stream );
}