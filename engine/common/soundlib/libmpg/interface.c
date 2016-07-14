/*
 * External functions declared as __declspec(dllexport)
 * to work in a Win32 DLL (use mpglibdll.h to access)
 */

#include <stdlib.h>
#include <stdio.h>

#include "mpg123.h"
#include "mpglib.h"
#include "libmpg.h"

void initStaticData(struct StaticData * psd)
{
	psd->pnts[0] = psd->cos64;
	psd->pnts[1] = psd->cos32;
	psd->pnts[2] = psd->cos16;
	psd->pnts[3] = psd->cos8;
	psd->pnts[4] = psd->cos4;

	psd->bo = 1;
}


int InitMP3(struct mpstr *mp)
{
	memset(mp,0,sizeof(struct mpstr));

	initStaticData(&mp->psd);

	mp->framesize = 0;
	mp->fsizeold = -1;
	mp->bsize = 0;
	mp->head = mp->tail = NULL;
	mp->fr.single = -1;
	mp->bsnum = 0;
	mp->synth_bo = 1;

	make_decode_tables(&mp->psd, 32767);
	init_layer3(&mp->psd, SBLIMIT);

	mp->fr.II_sblimit=SBLIMIT;
	init_layer2(&mp->psd);

	return !0;
}

void ExitMP3( struct mpstr *mp )
{
	struct buf *b, *bn;

	b = mp->tail;

	while( b )
	{
		free( b->pnt );
		bn = b->next;
		free( b );
		b = bn;
	}
}

static struct buf *addbuf(struct mpstr *mp,char *buf,int size)
{
	struct buf *nbuf;

	nbuf = (struct buf*) malloc( sizeof(struct buf) );
	if(!nbuf) {
		return NULL;
	}
	nbuf->pnt = (unsigned char*) malloc(size);
	if(!nbuf->pnt) {
		free(nbuf);
		return NULL;
	}
	nbuf->size = size;
	memcpy(nbuf->pnt,buf,size);
	nbuf->next = NULL;
	nbuf->prev = mp->head;
	nbuf->pos = 0;

	if(!mp->tail) {
		mp->tail = nbuf;
	}
	else {
	  mp->head->next = nbuf;
	}

	mp->head = nbuf;
	mp->bsize += size;

	return nbuf;
}

static void remove_buf( struct mpstr *mp )
{
	struct buf *buf = mp->tail;

	mp->tail = buf->next;

	if( mp->tail )
		mp->tail->prev = NULL;
	else
		mp->tail = mp->head = NULL;

	free( buf->pnt );
	free( buf );
}

static int read_buf_byte( struct mpstr *mp )
{
	unsigned int b;

	int pos = mp->tail->pos;

	while( pos >= mp->tail->size )
	{
		remove_buf( mp );

		if( !mp->tail )
		{
			return 0;
		}
		pos = mp->tail->pos;
	}

	b = mp->tail->pnt[pos];
	mp->bsize--;
	mp->tail->pos++;

	return b;
}

static void read_head( struct mpstr *mp )
{
	unsigned int head = 0;

	while( mp->tail )
	{
		head <<= 8;
		head |= read_buf_byte(mp);
		head &= 0xffffffff;
		if( head_check( head ))
			break;
	}

	mp->header = head;
}

int decodeMP3( struct mpstr *mp, char *in, int isize, char *out, int osize, int *done )
{
	int len;

	if( osize < 4608 )
	{
		return MP3_ERR;
	}

	if( in )
	{
		if( addbuf( mp, in, isize ) == NULL )
		{
			return MP3_ERR;
		}
	}


	// first decode header
	if( mp->framesize == 0 )
	{
		if( mp->bsize < 4 )
		{
			return MP3_NEED_MORE;
		}

		read_head( mp );

		if( mp->tail )
			mp->ndatabegin = mp->tail->pos - 4;

		if( decode_header( mp, &mp->fr,mp->header ) <= 0 )
			return MP3_ERR;

		mp->framesize = mp->fr.framesize;
	}

	if( mp->fr.framesize > mp->bsize )
	{
		return MP3_NEED_MORE;
	}

	mp->psd.wordpointer = mp->bsspace[mp->bsnum] + 512;
	mp->bsnum = (mp->bsnum + 1) & 0x1;
	mp->psd.bitindex = 0;

	len = 0;
	while( len < mp->framesize )
	{
		int nlen;
		int blen = mp->tail->size - mp->tail->pos;
		if(( mp->framesize - len ) <= blen )
		{
			nlen = mp->framesize-len;
		}
		else
		{
			nlen = blen;
		}

		memcpy( mp->psd.wordpointer + len, mp->tail->pnt + mp->tail->pos, nlen );
		len += nlen;
		mp->tail->pos += nlen;
		mp->bsize -= nlen;

		if( mp->tail->pos == mp->tail->size )
		{
			remove_buf(mp);
		}
	}

	*done = 0;

	if( mp->fr.error_protection )
		getbits(&mp->psd, 16);

	// FOR mpglib.dll: see if error and return it
	if(( &mp->fr )->do_layer( &mp->psd, mp, &mp->fr, (unsigned char *)out, done) < 0 )
		return MP3_ERR;

	mp->fsizeold = mp->framesize;
	mp->framesize = 0;
	return MP3_OK;
}

int set_pointer( struct StaticData *psd, struct mpstr *gmp, int backstep )
{
	unsigned char	*bsbufold;

	if( gmp->fsizeold < 0 && backstep > 0 )
		return MP3_ERR;

	bsbufold = gmp->bsspace[gmp->bsnum] + 512;
	psd->wordpointer -= backstep;

	if( backstep )
		memcpy( psd->wordpointer, bsbufold+gmp->fsizeold-backstep, backstep );

	psd->bitindex = 0;
	return MP3_OK;
}

int create_decoder( mpeg_t *mpg )
{
	if( !mpg ) return 0;

	mpg->state = malloc( sizeof( struct mpstr ));
	if( !mpg->state ) return 0;

	// initailize decoder
	InitMP3( mpg->state );

	// clearing info
	mpg->channels = mpg->samples = mpg->rate = 0;
	mpg->play_time = mpg->outsize = 0;
	mpg->vbrtag = NULL;

	return 1;	// done
}

int read_mpeg_header( mpeg_t *mpg, const char *data, int bufsize, int streamsize )
{
	struct mpstr	*mp;
	unsigned char	vbrbuf[VBRHEADERSIZE+36];
	VBRTAGDATA	vbrtag;
	
	if( !mpg->state )
		return 0;	// decoder was not initialized

	if( bufsize < AUDIOBUFSIZE )
		return 0;	// too small buffer for analyze it

	if(( decodeMP3( mpg->state, (char *)data, bufsize, mpg->out, 8192, &mpg->outsize )) != MP3_OK )
		return 0;

	mp = (struct mpstr *)mpg->state;

	// fill info about stream
	mpg->rate = freqs[mp->fr.sampling_frequency];
	mpg->channels = mp->fr.stereo;
	mpg->streamsize = streamsize;

	memcpy( vbrbuf, mp->tail->pnt + mp->ndatabegin, VBRHEADERSIZE + 36 );

	if( GetVbrTag( &vbrtag, (unsigned char *)vbrbuf ))
	{
		int	cur_bitrate;

		if( vbrtag.frames < 1 || vbrtag.bytes < 1 )
			return 0;

		mpg->vbrtag = malloc( sizeof( VBRTAGDATA ));
		if( !mpg->vbrtag )
			return 0;	// failed to allocate memory

		cur_bitrate = (int)(vbrtag.bytes * 8 / (vbrtag.frames * 576.0 * (mp->fr.lsf ? 1 : 2) / mpg->rate ));
		mpg->play_time = vbrtag.frames * 576.0 * (mp->fr.lsf ? 1 : 2) / mpg->rate * 1000;
		memcpy( mpg->vbrtag, &vbrtag, sizeof( VBRTAGDATA ));
	}
	else
	{
		double	bpf = 0.0;
		double	tpf = 0.0;

		bpf = compute_bpf( &mp->fr );
		tpf = compute_tpf( &mp->fr );

		mpg->play_time = (unsigned int)((double)(streamsize) / bpf * tpf ) * 1000;
	}

	return 1;
}

int read_mpeg_stream( mpeg_t *mpg, const char *data, int bufsize )
{
	return decodeMP3( mpg->state, (char *)data, bufsize, mpg->out, 8192, &mpg->outsize );
}

int get_current_pos( mpeg_t *mpg, int curpos )
{
	struct mpstr *mp = (struct mpstr *)mpg->state;
	return (double)curpos / (double)freqs[mp->fr.sampling_frequency] / (double)mp->fr.stereo / 2.0 * 1000.0;
}

int set_current_pos( mpeg_t *mpg, int newpos, int (*pfnSeek)( void*, int, int ), void *file )
{
	struct mpstr *mp = (struct mpstr *)mpg->state;
	VBRTAGDATA *vbrtag = (VBRTAGDATA *)mpg->vbrtag;

	if( !pfnSeek ) return -1; // failed to seek

	if( vbrtag != NULL )
	{
		int fileofs = SeekPoint( vbrtag->toc, mpg->streamsize, (double)newpos * 100 / (double)mpg->play_time );

		if( pfnSeek( file, fileofs, SEEK_SET ) == -1 )
			return -1; // failed to seek

		return (double)newpos / 1000.0 * (double)freqs[mp->fr.sampling_frequency] * (double)mp->fr.stereo * 2.0;
	}

	if( pfnSeek( file, (double)mpg->streamsize * ((double)newpos / (double)mpg->play_time ), SEEK_SET ) == -1 )
		return -1; // failed to seek

	return (double)newpos / 1000.0 * (double)freqs[mp->fr.sampling_frequency] * (double)mp->fr.stereo * 2.0;
}

void close_decoder( mpeg_t *mpg )
{
	if( !mpg ) return;

	if( mpg->state )
	{
		ExitMP3( mpg->state );
		free( mpg->state );
	}

	if( mpg->vbrtag )
	{
		free( mpg->vbrtag );
	}

	memset( mpg, 0, sizeof( *mpg ));
}
