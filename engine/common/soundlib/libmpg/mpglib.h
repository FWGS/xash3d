struct buf
{
	unsigned char *pnt;
	int size;
	int pos;
	struct buf *next;
	struct buf *prev;
};

struct framebuf
{
	struct buf *buf;
	int pos;
	struct frame *next;
	struct frame *prev;
};

struct mpstr
{
	struct buf *head,*tail;
	int bsize;
	int framesize;
	int fsizeold;
	struct frame fr;
	unsigned char bsspace[2][MAXFRAMESIZE+512]; /* MAXFRAMESIZE */
	float hybrid_block[2][2][SBLIMIT*SSLIMIT];
	int hybrid_blc[2];
	unsigned int header;
	int bsnum;
	float synth_buffs[2][2][0x110];
	int  synth_bo;
	struct StaticData psd;
	int ndatabegin;
};


#define MP3_ERR		-1
#define MP3_OK		0
#define MP3_NEED_MORE	1

#ifdef __cplusplus
extern "C" {
#endif

extern int InitMP3( struct mpstr *mp );
extern int decodeMP3( struct mpstr *mp,char *inmemory,int inmemsize, char *outmemory, int outmemsize, int *done );
extern void ExitMP3( struct mpstr *mp );
extern double compute_bpf( struct frame *fr );
extern double compute_tpf( struct frame *fr );
extern int GetVbrTag( VBRTAGDATA *pTagData,  unsigned char *buf );
extern int SeekPoint( unsigned char TOC[NUMTOCENTRIES], int file_bytes, double percent );

#ifdef __cplusplus
}
#endif
