#ifndef WAVEHDR_H
#define WAVEHDR_H

#define RIFFHEADER		(('F'<<24)+('F'<<16)+('I'<<8)+'R') // little-endian "RIFF"
#define WAVEHEADER		(('E'<<24)+('V'<<16)+('A'<<8)+'W') // little-endian "WAVE"
#define FORMHEADER		((' '<<24)+('t'<<16)+('m'<<8)+'f') // little-endian "fmt "
#define DATAHEADER		(('a'<<24)+('t'<<16)+('a'<<8)+'d') // little-endian "data"

typedef struct
{
	int	riff_id;		// 'RIFF' 
	int	rLen;
	int	wave_id;		// 'WAVE' 
	int	fmt_id;		// 'fmt ' 
	int	pcm_header_len;	// varies...
	short	wFormatTag;
	short	nChannels;	// 1,2 for stereo data is (l,r) pairs 
	int	nSamplesPerSec;
	int	nAvgBytesPerSec;
	short	nBlockAlign;      
	short	nBitsPerSample;
} wavehdr_t;

typedef struct
{
	int	data_id;		// 'data' or 'fact' 
	int	dLen;
} chunkhdr_t;

#endif//WAVEHDR_H
