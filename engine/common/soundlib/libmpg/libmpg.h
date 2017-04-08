#ifndef LIBMPG_H
#define LIBMPG_H

#ifdef __cplusplus
extern "C" {
#endif

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

extern int create_decoder( mpeg_t *mpg );
extern int read_mpeg_header( mpeg_t *mpg, const char *data, int bufsize, int streamsize );
extern int read_mpeg_stream( mpeg_t *mpg, const char *data, int bufsize );
extern int set_current_pos( mpeg_t *mpg, int newpos, int (*pfnSeek)( void*, int, int ), void *file );
extern int get_current_pos( mpeg_t *mpg, int curpos );
extern void close_decoder( mpeg_t *mpg );

#ifdef __cplusplus
}
#endif

#endif//LIBMPG_H
