/*
common.h - definitions common between client and server
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

// disable some warnings
#pragma warning(disable : 4244)	// MIPS
#pragma warning(disable : 4018)	// signed/unsigned mismatch
#pragma warning(disable : 4305)	// truncation from const double to float
#pragma warning(disable : 4115)	// named type definition in parentheses
#pragma warning(disable : 4100)	// unreferenced formal parameter
#pragma warning(disable : 4127)	// conditional expression is constant
#pragma warning(disable : 4057)	// differs in indirection to slightly different base types
#pragma warning(disable : 4201)	// nonstandard extension used
#pragma warning(disable : 4706)	// assignment within conditional expression
#pragma warning(disable : 4054)	// type cast' : from function pointer
#pragma warning(disable : 4310)	// cast truncates constant value

#define MAX_STRING		256	// generic string
#define MAX_INFO_STRING	256	// infostrings are transmitted across network
#define MAX_SYSPATH		1024	// system filepath
#define MAX_MODS		512	// environment games that engine can keep visible
#define EXPORT		__declspec( dllexport )
#define BIT( n )		(1<<( n ))

#ifndef __cplusplus
#define NULL		((void *)0)
#endif

// color strings
#define IsColorString( p )	( p && *( p ) == '^' && *(( p ) + 1) && *(( p ) + 1) >= '0' && *(( p ) + 1 ) <= '9' )
#define ColorIndex( c )	((( c ) - '0' ) & 7 )

#define Mod_AllowMaterials()	( mod_allow_materials != NULL && mod_allow_materials->integer && !( host.features & ENGINE_DISABLE_HDTEXTURES ))

typedef unsigned long	dword;
typedef unsigned int	uint;
typedef char		string[MAX_STRING];
typedef long		fs_offset_t;
typedef struct file_s	file_t;		// normal file
typedef struct wfile_s	wfile_t;		// wad file
typedef struct stream_s	stream_t;		// sound stream for background music playing

typedef struct
{
	int	numfilenames;
	char	**filenames;
	char	*filenamesbuffer;
} search_t;

enum
{
	D_INFO = 1,	// "-dev 1", shows various system messages
	D_WARN,		// "-dev 2", shows not critical system warnings
	D_ERROR,		// "-dev 3", shows critical warnings 
	D_AICONSOLE,	// "-dev 4", special case for game aiconsole
	D_NOTE		// "-dev 5", show system notifications for engine developers
};

typedef enum
{	
	HOST_NORMAL,	// listen server, singleplayer
	HOST_DEDICATED,
} instance_t;

#include "system.h"
#include "ref_params.h"
#include "com_model.h"
#include "crtlib.h"

#define XASH_VERSION	0.96f		// engine current version

// PERFORMANCE INFO
#define MIN_FPS         	15.0		// host minimum fps value for maxfps.
#define MAX_FPS         	500.0		// upper limit for maxfps.

#define MAX_FRAMETIME	0.1
#define MIN_FRAMETIME	0.001

#define MAX_CMD_TOKENS	80		// cmd tokens
#define MAX_ENTNUMBER	99999		// for server and client parsing
#define MAX_HEARTBEAT	-99999		// connection time
#define QCHAR_WIDTH		16		// font width

#define CIN_MAIN		0
#define CIN_LOGO		1

#define MAX_NUM_ARGVS	128

// config strings are a general means of communication from
// the server to all connected clients.
// each config string can be at most CS_SIZE characters.
#define CS_SIZE		64	// size of one config string
#define CS_TIME		16	// size of time string

#define MAX_DECALS		512	// touching TE_DECAL messages, etc
#define MAX_STATIC_ENTITIES	512	// static entities that moved on the client when level is spawn

// filesystem flags
#define FS_STATIC_PATH	1	// FS_ClearSearchPath will be ignore this path
#define FS_NOWRITE_PATH	2	// default behavior - last added gamedir set as writedir. This flag disables it
#define FS_GAMEDIR_PATH	4	// just a marker for gamedir path

#define GI		SI.GameInfo
#define FS_Gamedir()	SI.GameInfo->gamedir
#define FS_Title()		SI.GameInfo->title

#ifdef _DEBUG
void DBG_AssertFunction( qboolean fExpr, const char* szExpr, const char* szFile, int szLine, const char* szMessage );
#define Assert( f )		DBG_AssertFunction( f, #f, __FILE__, __LINE__, NULL )
#else
#define Assert( f )
#endif

extern convar_t	*scr_width;
extern convar_t	*scr_height;
extern convar_t	*scr_loading;
extern convar_t	*scr_download;
extern convar_t	*cl_allow_levelshots;
extern convar_t	*mod_allow_materials;
extern convar_t	*host_limitlocal;
extern convar_t	*host_maxfps;

/*
==============================================================

HOST INTERFACE

==============================================================
*/
/*
========================================================================

GAMEINFO stuff

internal shared gameinfo structure (readonly for engine parts)
========================================================================
*/
typedef struct gameinfo_s
{
	// filesystem info
	char		gamefolder[64];	// used for change game '-game x'
	char		basedir[64];	// base game directory (like 'id1' for Quake or 'valve' for Half-Life)
	char		gamedir[64];	// game directory (can be match with basedir, used as game dir and as write path)
	char		falldir[64];	// used as second basedir 
	char		startmap[64];	// map to start singleplayer game
	char		trainmap[64];	// map to start hazard course (if specified)
	char		title[64];	// Game Main Title
	float		version;		// game version (optional)

	// .dll pathes
	char		dll_path[64];	// e.g. "bin" or "cl_dlls"
	char		game_dll[64];	// custom path for game.dll

	// .ico path
	char		iconpath[64];	// "game.ico" by default

	// about mod info
	string		game_url;		// link to a developer's site
	string		update_url;	// link to updates page
	char		type[64];		// single, toolkit, multiplayer etc
	char		date[64];
	size_t		size;

	int		gamemode;
	qboolean		secure;		// prevent to console acess
	qboolean		nomodels;		// don't let player to choose model (use player.mdl always)

	char		sp_entity[32];	// e.g. info_player_start
	char		mp_entity[32];	// e.g. info_player_deathmatch

	float		client_mins[MAX_MAP_HULLS][3];	// 4 hulls allowed
	float		client_maxs[MAX_MAP_HULLS][3];	// 4 hulls allowed

	char		ambientsound[NUM_AMBIENTS][64];	// quake ambient sounds
	int		soundclip_dist;			// custom distance to clip sound

	int		max_edicts;	// min edicts is 600, max edicts is 4096
	int		max_tents;	// min temp ents is 300, max is 2048
	int		max_beams;	// min beams is 64, max beams is 512
	int		max_particles;	// min particles is 4096, max particles is 32768
} gameinfo_t;

typedef struct sysinfo_s
{
	string		ModuleName;	// exe.filename
	gameinfo_t	*GameInfo;	// current GameInfo
	gameinfo_t	*games[MAX_MODS];	// environment games (founded at each engine start)
	int		numgames;
} sysinfo_t;

typedef enum
{
	HOST_INIT = 0,	// initalize operations
	HOST_FRAME,	// host running
	HOST_SHUTDOWN,	// shutdown operations	
	HOST_ERR_FATAL,	// sys error
	HOST_SLEEP,	// sleeped by different reason, e.g. minimize window
	HOST_NOFOCUS,	// same as HOST_FRAME, but disable mouse
	HOST_RESTART,	// during the changes video mode
	HOST_CRASHED	// an exception handler called
} host_state;

typedef enum
{
	key_console = 0,
	key_game,
	key_menu,
	key_message
} keydest_t;

// MD5 Hash
typedef struct
{
	uint	buf[4];
	uint	bits[2];
	byte	in[64];
} MD5Context_t;

typedef enum
{
	RD_NONE = 0,
	RD_CLIENT,
	RD_PACKET
} rdtype_t;

// game print level
typedef enum
{
	PRINT_LOW,	// pickup messages
	PRINT_MEDIUM,	// death messages
	PRINT_HIGH,	// critical messages
	PRINT_CHAT,	// chat messages
} messagelevel_t;

typedef enum
{
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

#include "netadr.h"

typedef struct host_redirect_s
{
	rdtype_t		target;
	char		*buffer;
	int		buffersize;
	netadr_t		address;
	void		(*flush)( netadr_t adr, rdtype_t target, char *buffer );
} host_redirect_t;

typedef struct
{
	char		name[64];
	short		entnum;
	vec3_t		origin;
	float		volume;
	float		attenuation;
	qboolean		looping;
	byte		channel;
	byte		pitch;
	byte		wordIndex;	// current playing word in sentence
	double		samplePos;
	double		forcedEnd;
} soundlist_t;

typedef struct host_parm_s
{
	HINSTANCE			hInst;
	HANDLE			hMutex;
	LPTOP_LEVEL_EXCEPTION_FILTER	oldFilter;

	host_state	state;		// global host state
	uint		type;		// running at
	jmp_buf		abortframe;	// abort current frame
	dword		errorframe;	// to prevent multiple host error
	byte		*mempool;		// static mempool for misc allocations
	string		finalmsg;		// server shutdown final message
	host_redirect_t	rd;		// remote console

	// command line parms
	int		argc;
	const char	*argv[MAX_NUM_ARGVS];

	double		realtime;		// host.curtime
	double		frametime;	// time between engine frames
	double		realframetime;	// for some system events, e.g. console animations

	uint		framecount;	// global framecount

	// list of unique decal indexes
	char		draw_decals[MAX_DECALS][CS_SIZE];

	HWND		hWnd;		// main window
	int		developer;	// show all developer's message
	int		old_developer;	// keep real dev state (we need enable dev-mode in multiplayer)
	qboolean		key_overstrike;	// key overstrike mode
	qboolean		stuffcmdsrun;	// execute stuff commands
	qboolean		con_showalways;	// show console always (developer and dedicated)
	qboolean		change_game;	// initialize when game is changed
	qboolean		mouse_visible;	// vgui override cursor control
	qboolean		input_enabled;	// vgui override mouse & keyboard input
	qboolean		shutdown_issued;	// engine is shutting down
	qboolean		decal_loading;	// nasty hack to tell imagelib about decal
	qboolean		overview_loading;	// another nasty hack to tell imagelib about ovierview
	qboolean		force_draw_version;	// used when fraps is loaded
	qboolean		write_to_clipboard;	// put image to clipboard instead of disk
	qboolean		crashed;		// set to true if crashed

	char		rootdir[256];	// member root directory
	char		gamefolder[64];	// it's a default gamefolder	
	byte		*imagepool;	// imagelib mempool
	byte		*soundpool;	// soundlib mempool

	uint		features;		// custom features that enables by mod-maker request

	// for IN_MouseMove() easy access
	int		window_center_x;
	int		window_center_y;

	struct decallist_s	*decalList;	// used for keep decals, when renderer is restarted or changed
	int		numdecals;

	soundlist_t	*soundList;	// used for keep ambient sounds, when renderer or sound is restarted
	int		numsounds;
} host_parm_t;

extern host_parm_t	host;
extern sysinfo_t	SI;

//
// filesystem.c
//
void FS_Init( void );
void FS_Path( void );
void FS_Shutdown( void );
void FS_ClearSearchPath( void );
void FS_AllowDirectPaths( qboolean enable );
void FS_AddGameDirectory( const char *dir, int flags );
void FS_AddGameHierarchy( const char *dir, int flags );
void FS_LoadGameInfo( const char *rootfolder );
void FS_FileBase( const char *in, char *out );
const char *FS_FileExtension( const char *in );
void FS_DefaultExtension( char *path, const char *extension );
void FS_ExtractFilePath( const char* const path, char* dest );
const char *FS_GetDiskPath( const char *name, qboolean gamedironly );
const char *FS_FileWithoutPath( const char *in );
wfile_t *W_Open( const char *filename, const char *mode );
byte *W_LoadLump( wfile_t *wad, const char *lumpname, size_t *lumpsizeptr, const char type );
void W_Close( wfile_t *wad );
file_t *FS_OpenFile( const char *path, fs_offset_t *filesizeptr, qboolean gamedironly );
byte *FS_LoadFile( const char *path, fs_offset_t *filesizeptr, qboolean gamedironly );
qboolean FS_WriteFile( const char *filename, const void *data, fs_offset_t len );
int COM_FileSize( const char *filename );
void COM_FixSlashes( char *pname );
void COM_FreeFile( void *buffer );
int COM_CompareFileTime( const char *filename1, const char *filename2, int *iCompare );
search_t *FS_Search( const char *pattern, int caseinsensitive, int gamedironly );
file_t *FS_Open( const char *filepath, const char *mode, qboolean gamedironly );
fs_offset_t FS_Write( file_t *file, const void *data, size_t datasize );
fs_offset_t FS_Read( file_t *file, void *buffer, size_t buffersize );
int FS_VPrintf( file_t *file, const char *format, va_list ap );
int FS_Seek( file_t *file, fs_offset_t offset, int whence );
int FS_Gets( file_t *file, byte *string, size_t bufsize );
int FS_Printf( file_t *file, const char *format, ... );
fs_offset_t FS_FileSize( const char *filename, qboolean gamedironly );
fs_offset_t FS_FileTime( const char *filename, qboolean gamedironly );
int FS_Print( file_t *file, const char *msg );
qboolean FS_Rename( const char *oldname, const char *newname );
qboolean FS_FileExists( const char *filename, qboolean gamedironly );
void FS_FileCopy( file_t *pOutput, file_t *pInput, int fileSize );
qboolean FS_Delete( const char *path );
int FS_UnGetc( file_t *file, byte c );
void FS_StripExtension( char *path );
fs_offset_t FS_Tell( file_t *file );
qboolean FS_Eof( file_t *file );
void FS_Purge( file_t *file );
int FS_Close( file_t *file );
int FS_Getc( file_t *file );
qboolean FS_Eof( file_t *file );
fs_offset_t FS_FileLength( file_t *f );

//
// network.c
//
void NET_Init( void );
void NET_Shutdown( void );
void NET_Sleep( int msec );
void NET_Config( qboolean net_enable );
qboolean NET_IsLocalAddress( netadr_t adr );
char *NET_AdrToString( const netadr_t a );
char *NET_BaseAdrToString( const netadr_t a );
qboolean NET_StringToAdr( const char *string, netadr_t *adr );
qboolean NET_CompareAdr( const netadr_t a, const netadr_t b );
qboolean NET_CompareBaseAdr( const netadr_t a, const netadr_t b );
qboolean NET_GetPacket( netsrc_t sock, netadr_t *from, byte *data, size_t *length );
void NET_SendPacket( netsrc_t sock, size_t length, const void *data, netadr_t to );

/*
========================================================================

internal image format

typically expanded to rgba buffer
NOTE: number at end of pixelformat name it's a total bitscount e.g. PF_RGB_24 == PF_RGB_888
========================================================================
*/
typedef enum
{
	PF_UNKNOWN = 0,
	PF_INDEXED_24,	// inflated palette (768 bytes)
	PF_INDEXED_32,	// deflated palette (1024 bytes)
	PF_RGBA_32,	// normal rgba buffer
	PF_BGRA_32,	// big endian RGBA (MacOS)
	PF_RGB_24,	// uncompressed dds or another 24-bit image 
	PF_BGR_24,	// big-endian RGB (MacOS)
	PF_TOTALCOUNT,	// must be last
} pixformat_t;

typedef struct bpc_desc_s
{
	int	format;	// pixelformat
	char	name[16];	// used for debug
	uint	glFormat;	// RGBA format
	int	bpp;	// channels (e.g. rgb = 3, rgba = 4)
} bpc_desc_t;

// imagelib global settings
typedef enum
{
	IL_USE_LERPING	= BIT(0),	// lerping images during resample
	IL_KEEP_8BIT	= BIT(1),	// don't expand paletted images
	IL_ALLOW_OVERWRITE	= BIT(2),	// allow to overwrite stored images
	IL_DONTFLIP_TGA	= BIT(3),	// Steam background completely ignore tga attribute 0x20 (stupid lammers!)
} ilFlags_t;

// rgbdata output flags
typedef enum
{
	// rgbdata->flags
	IMAGE_CUBEMAP	= BIT(0),		// it's 6-sides cubemap buffer
	IMAGE_HAS_ALPHA	= BIT(1),		// image contain alpha-channel
	IMAGE_HAS_COLOR	= BIT(2),		// image contain RGB-channel
	IMAGE_COLORINDEX	= BIT(3),		// all colors in palette is gradients of last color (decals)
	IMAGE_HAS_LUMA	= BIT(4),		// image has luma pixels (q1-style maps)
	IMAGE_SKYBOX	= BIT(5),		// only used by FS_SaveImage - for write right suffixes
	IMAGE_QUAKESKY	= BIT(6),		// it's a quake sky double layered clouds (so keep it as 8 bit)

	// Image_Process manipulation flags
	IMAGE_FLIP_X	= BIT(16),	// flip the image by width
	IMAGE_FLIP_Y	= BIT(17),	// flip the image by height
	IMAGE_ROT_90	= BIT(18),	// flip from upper left corner to down right corner
	IMAGE_ROT180	= IMAGE_FLIP_X|IMAGE_FLIP_Y,
	IMAGE_ROT270	= IMAGE_FLIP_X|IMAGE_FLIP_Y|IMAGE_ROT_90,	
	IMAGE_ROUND	= BIT(19),	// round image to nearest Pow2
	IMAGE_RESAMPLE	= BIT(20),	// resample image to specified dims
	IMAGE_PALTO24	= BIT(21),	// turn 32-bit palette into 24-bit mode (only for indexed images)
	IMAGE_ROUNDFILLER	= BIT(22),	// round image to Pow2 and fill unused entries with single color	
	IMAGE_FORCE_RGBA	= BIT(23),	// force image to RGBA buffer
	IMAGE_MAKE_LUMA	= BIT(24),	// create luma texture from indexed
	IMAGE_QUANTIZE	= BIT(25),	// make indexed image from 24 or 32- bit image
	IMAGE_LIGHTGAMMA	= BIT(26),	// apply gamma for image
	IMAGE_REMAP	= BIT(27),	// interpret width and height as top and bottom color
} imgFlags_t;

// ordering is important!
typedef enum
{
	BLUR_FILTER = 0,
	BLUR_FILTER2,
	EDGE_FILTER,
	EMBOSS_FILTER,
	NUM_FILTERS,
} pixfilter_t;

typedef struct rgbdata_s
{
	word	width;		// image width
	word	height;		// image height
	word	depth;		// image depth
	uint	type;		// compression type
	uint	flags;		// misc image flags
	byte	*palette;		// palette if present
	byte	*buffer;		// image buffer
	rgba_t	fogParams;	// some water textures in hl1 has info about fog color and alpha
	size_t	size;		// for bounds checking
} rgbdata_t;

// imgfilter processing flags
typedef enum
{
	FILTER_GRAYSCALE	= BIT(0),
} flFlags_t;

typedef struct imgfilter_s
{
	int	filter;		// pixfilter_t
	float	factor;		// filter factor value
	float	bias;		// filter bias value
	flFlags_t	flags;		// filter additional flags
	uint	blendFunc;	// blending mode
} imgfilter_t;

//
// imagelib
//
void Image_Init( void );
void Image_Shutdown( void );
rgbdata_t *FS_LoadImage( const char *filename, const byte *buffer, size_t size );
qboolean FS_SaveImage( const char *filename, rgbdata_t *pix );
rgbdata_t *FS_CopyImage( rgbdata_t *in );
void FS_FreeImage( rgbdata_t *pack );
extern const bpc_desc_t PFDesc[];	// image get pixelformat
qboolean Image_Process( rgbdata_t **pix, int width, int height, float gamma, uint flags, imgfilter_t *filter );
void Image_PaletteHueReplace( byte *palSrc, int newHue, int start, int end );
void Image_SetForceFlags( uint flags );	// set image force flags on loading

/*
========================================================================

internal sound format

typically expanded to wav buffer
========================================================================
*/
typedef enum
{
	WF_UNKNOWN = 0,
	WF_PCMDATA,
	WF_MPGDATA,
	WF_TOTALCOUNT,	// must be last
} sndformat_t;

// soundlib global settings
typedef enum
{
	SL_USE_LERPING	= BIT(0),		// lerping sounds during resample
	SL_KEEP_8BIT	= BIT(1),		// don't expand 8bit sounds automatically up to 16 bit
	SL_ALLOW_OVERWRITE	= BIT(2),		// allow to overwrite stored sounds
} slFlags_t;

// wavdata output flags
typedef enum
{
	// wavdata->flags
	SOUND_LOOPED	= BIT( 0 ),	// this is looped sound (contain cue markers)
	SOUND_STREAM	= BIT( 1 ),	// this is a streaminfo, not a real sound

	// Sound_Process manipulation flags
	SOUND_RESAMPLE	= BIT(12),	// resample sound to specified rate
	SOUND_CONVERT16BIT	= BIT(13),	// change sound resolution from 8 bit to 16
} sndFlags_t;

typedef struct
{
	word	rate;		// num samples per second (e.g. 11025 - 11 khz)
	byte	width;		// resolution - bum bits divided by 8 (8 bit is 1, 16 bit is 2)
	byte	channels;		// num channels (1 - mono, 2 - stereo)
	int	loopStart;	// offset at this point sound will be looping while playing more than only once
	int	samples;		// total samplecount in wav
	uint	type;		// compression type
	uint	flags;		// misc sound flags
	byte	*buffer;		// sound buffer
	size_t	size;		// for bounds checking
} wavdata_t;

//
// soundlib
//
void Sound_Init( void );
void Sound_Shutdown( void );
wavdata_t *FS_LoadSound( const char *filename, const byte *buffer, size_t size );
void FS_FreeSound( wavdata_t *pack );
stream_t *FS_OpenStream( const char *filename );
wavdata_t *FS_StreamInfo( stream_t *stream );
long FS_ReadStream( stream_t *stream, int bytes, void *buffer );
long FS_SetStreamPos( stream_t *stream, long newpos );
long FS_GetStreamPos( stream_t *stream );
void FS_FreeStream( stream_t *stream );
qboolean Sound_Process( wavdata_t **wav, int rate, int width, uint flags );
uint Sound_GetApproxWavePlayLen( const char *filepath );

//
// build.c
//
int Q_buildnum( void );

//
// host.c
//
void EXPORT Host_Shutdown( void );
void Host_SetServerState( int state );
int Host_ServerState( void );
int Host_CompareFileTime( long ft1, long ft2 );
void Host_NewInstance( const char *name, const char *finalmsg );
qboolean Host_NewGame( const char *mapName, qboolean loadGame );
void Host_EndGame( const char *message, ... );
void Host_AbortCurrentFrame( void );
void Host_RestartAmbientSounds( void );
void Host_RestartDecals( void );
qboolean CL_ChangeGame( const char *gamefolder, qboolean bReset );
void Host_WriteServerConfig( const char *name );
void Host_WriteOpenGLConfig( void );
void Host_WriteVideoConfig( void );
void Host_WriteConfig( void );
qboolean Host_IsLocalGame( void );
void Host_ShutdownServer( void );
void Host_Print( const char *txt );
void Host_Error( const char *error, ... );
void Host_PrintEngineFeatures( void );
void Host_InitDecals( void );
void Host_Credits( void );

/*
==============================================================

CLIENT / SERVER SYSTEMS

==============================================================
*/
void CL_Init( void );
void CL_Shutdown( void );
void Host_ClientFrame( void );
qboolean CL_Active( void );

void SV_Init( void );
void SV_Shutdown( qboolean reconnect );
void Host_ServerFrame( void );
qboolean SV_Active( void );

/*
==============================================================

	SHARED ENGFUNCS

==============================================================
*/
cvar_t *pfnCvar_RegisterVariable( const char *szName, const char *szValue, int flags );
char *COM_MemFgets( byte *pMemFile, int fileSize, int *filePos, char *pBuffer, int bufferSize );
byte* COM_LoadFileForMe( const char *filename, int *pLength );
cvar_t *pfnCVarGetPointer( const char *szVarName );
int pfnDrawConsoleString( int x, int y, char *string );
void pfnDrawSetTextColor( float r, float g, float b );
void pfnDrawConsoleStringLen( const char *pText, int *length, int *height );
int pfnAddClientCommand( const char *cmd_name, xcommand_t func );
void *Cache_Check( byte *mempool, struct cache_user_s *c );
edict_t* pfnPEntityOfEntIndex( int iEntIndex );
void pfnGetModelBounds( model_t *mod, float *mins, float *maxs );
void pfnGetGameDir( char *szGetGameDir );
int pfnDecalIndex( const char *m );
int pfnGetModelType( model_t *mod );
int pfnIsMapValid( char *filename );
void Con_DPrintf( char *fmt, ... );
void Con_Printf( char *szFmt, ... );
int pfnIsInGame( void );

// CS:CS engfuncs (stubs)
void *pfnSequenceGet( const char *fileName, const char *entryName );
void *pfnSequencePickSentence( const char *groupName, int pickMethod, int *picked );
int pfnIsCareerMatch( void );

// Decay engfuncs (stubs)
int pfnGetTimesTutorMessageShown( int mid );
void pfnRegisterTutorMessageShown( int mid );
void pfnConstructTutorMessageDecayBuffer( int *buffer, int buflen );
void pfnProcessTutorMessageDecayBuffer( int *buffer, int bufferLength );
void pfnResetTutorMessageDecayData( void );

/*
==============================================================

	MISC COMMON FUNCTIONS

==============================================================
*/
#define Z_Malloc( size )		Mem_Alloc( host.mempool, size )
#define Z_Realloc( ptr, size )	Mem_Realloc( host.mempool, ptr, size )
#define Z_Free( ptr )		if( ptr ) Mem_Free( ptr )

//
// crclib.c
//
void CRC32_Init( dword *pulCRC );
byte CRC32_BlockSequence( byte *base, int length, int sequence );
void CRC32_ProcessBuffer( dword *pulCRC, const void *pBuffer, int nBuffer );
void CRC32_ProcessByte( dword *pulCRC, byte ch );
void CRC32_Final( dword *pulCRC );
qboolean CRC32_File( dword *crcvalue, const char *filename );
qboolean CRC32_MapFile( dword *crcvalue, const char *filename );
void MD5Init( MD5Context_t *ctx );
void MD5Update( MD5Context_t *ctx, const byte *buf, uint len );
void MD5Final( byte digest[16], MD5Context_t *ctx );
qboolean MD5_HashFile( byte digest[16], const char *pszFileName, uint seed[4] );
uint Com_HashKey( const char *string, uint hashSize );

//
// hpak.c
//
void HPAK_Init( void );
qboolean HPAK_GetDataPointer( const char *filename, struct resource_s *pRes, byte **buffer, int *size );
qboolean HPAK_ResourceForHash( const char *filename, char *hash, struct resource_s *pRes );
void HPAK_AddLump( qboolean queue, const char *filename, struct resource_s *pRes, byte *data, file_t *f );
void HPAK_CheckIntegrity( const char *filename );
void HPAK_CheckSize( const char *filename );
void HPAK_FlushHostQueue( void );

//
// keys.c
//
qboolean Key_IsDown( int keynum );
const char *Key_IsBind( int keynum );
void Key_Event( int key, qboolean down );
void Key_Init( void );
void Key_WriteBindings( file_t *f );
const char *Key_GetBinding( int keynum );
void Key_SetBinding( int keynum, const char *binding );
void Key_ClearStates( void );
const char *Key_KeynumToString( int keynum );
int Key_StringToKeynum( const char *str );
int Key_GetKey( const char *binding );
void Key_EnumCmds_f( void );
void Key_SetKeyDest( int key_dest );

//
// avikit.c
//
typedef struct movie_state_s	movie_state_t;
long AVI_GetVideoFrameNumber( movie_state_t *Avi, float time );
byte *AVI_GetVideoFrame( movie_state_t *Avi, long frame );
qboolean AVI_GetVideoInfo( movie_state_t *Avi, long *xres, long *yres, float *duration );
qboolean AVI_GetAudioInfo( movie_state_t *Avi, wavdata_t *snd_info );
fs_offset_t AVI_GetAudioChunk( movie_state_t *Avi, char *audiodata, long offset, long length );
void AVI_OpenVideo( movie_state_t *Avi, const char *filename, qboolean load_audio, qboolean ignore_hwgamma, int quiet );
movie_state_t *AVI_LoadVideo( const char *filename, qboolean load_audio, qboolean ignore_hwgamma );
movie_state_t *AVI_LoadVideoNoSound( const char *filename, qboolean ignore_hwgamma );
void AVI_CloseVideo( movie_state_t *Avi );
qboolean AVI_IsActive( movie_state_t *Avi );
void AVI_FreeVideo( movie_state_t *Avi );
movie_state_t *AVI_GetState( int num );
qboolean AVI_Initailize( void );
void AVI_Shutdown( void );

// shared calls
qboolean CL_IsInGame( void );
qboolean CL_IsInMenu( void );
qboolean CL_IsInConsole( void );
qboolean CL_IsThirdPerson( void );
qboolean CL_IsIntermission( void );
float CL_GetServerTime( void );
float CL_GetLerpFrac( void );
void CL_CharEvent( int key );
qboolean CL_DisableVisibility( void );
int CL_PointContents( const vec3_t point );
char *COM_ParseFile( char *data, char *token );
byte *COM_LoadFile( const char *filename, int usehunk, int *pLength );
void CL_StudioEvent( struct mstudioevent_s *event, struct cl_entity_s *ent );
qboolean CL_GetComment( const char *demoname, char *comment );
void COM_AddAppDirectoryToSearchPath( const char *pszBaseDir, const char *appName );
int COM_ExpandFilename( const char *fileName, char *nameOutBuffer, int nameOutBufferSize );
struct pmtrace_s *PM_TraceLine( float *start, float *end, int flags, int usehull, int ignore_pe );
void SV_StartSound( edict_t *ent, int chan, const char *sample, float vol, float attn, int flags, int pitch );
void SV_StartMusic( const char *curtrack, const char *looptrack, fs_offset_t position );
void SV_CreateDecal( struct sizebuf_s *msg, const float *origin, int decalIndex, int entityIndex, int modelIndex, int flags, float scale );
void SV_CreateStudioDecal( struct sizebuf_s *msg, const float *origin, const float *start, int decalIndex, int entityIndex, int modelIndex,
int flags, struct modelstate_s *state );
struct sizebuf_s *SV_GetReliableDatagram( void );
qboolean SV_RestoreCustomDecal( struct decallist_s *entry, edict_t *pEdict, qboolean adjacent );
int R_CreateDecalList( struct decallist_s *pList, qboolean changelevel );
void R_ClearAllDecals( void );
void R_ClearStaticEntities( void );
qboolean S_StreamGetCurrentState( char *currentTrack, char *loopTrack, int *position );
struct cl_entity_s *CL_GetEntityByIndex( int index );
struct cl_entity_s *CL_GetLocalPlayer( void );
struct player_info_s *CL_GetPlayerInfo( int playerIndex );
void SV_DrawDebugTriangles( void );
void SV_DrawOrthoTriangles( void );
qboolean UI_CreditsActive( void );
void CL_ExtraUpdate( void );
int CL_GetMaxClients( void );
qboolean CL_IsPlaybackDemo( void );
qboolean CL_IsBackgroundDemo( void );
qboolean CL_IsBackgroundMap( void );
qboolean CL_LoadProgs( const char *name );
qboolean SV_GetComment( const char *savename, char *comment );
qboolean SV_NewGame( const char *mapName, qboolean loadGame );
void SV_ClipPMoveToEntity( struct physent_s *pe, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, struct pmtrace_s *tr );
void CL_ClipPMoveToEntity( struct physent_s *pe, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, struct pmtrace_s *tr );
void SV_SysError( const char *error_string );
void SV_InitGameProgs( void );
void SV_FreeGameProgs( void );
void SV_ForceError( void );
void CL_WriteMessageHistory( void );
void CL_SendCmd( void );
void CL_Disconnect( void );
void CL_ClearEdicts( void );
void CL_Crashed( void );
qboolean CL_NextDemo( void );
void CL_Drop( void );
void SCR_Init( void );
void SCR_UpdateScreen( void );
void SCR_BeginLoadingPlaque( qboolean is_background );
void SCR_CheckStartupVids( void );
long SCR_GetAudioChunk( char *rawdata, long length );
wavdata_t *SCR_GetMovieInfo( void );
void SCR_Shutdown( void );
void Con_Print( const char *txt );
void Con_NPrintf( int idx, char *fmt, ... );
void Con_NXPrintf( struct con_nprint_s *info, char *fmt, ... );
void UI_NPrintf( int idx, char *fmt, ... );
void UI_NXPrintf( struct con_nprint_s *info, char *fmt, ... );
char *Info_ValueForKey( const char *s, const char *key );
void Info_RemovePrefixedKeys( char *start, char prefix );
qboolean Info_RemoveKey( char *s, const char *key );
qboolean Info_SetValueForKey( char *s, const char *key, const char *value );
qboolean Info_SetValueForStarKey( char *s, const char *key, const char *value, int maxsize );
qboolean Info_Validate( const char *s );
void Info_Print( const char *s );
char *Cvar_Userinfo( void );
char *Cvar_Serverinfo( void );
void Cmd_WriteVariables( file_t *f );
qboolean Cmd_CheckMapsList( qboolean fRefresh );
void Cmd_AutoComplete( char *complete_string );
void COM_SetRandomSeed( long lSeed );
long Com_RandomLong( long lMin, long lMax );
float Com_RandomFloat( float fMin, float fMax );
void TrimSpace( const char *source, char *dest );
void GL_FreeImage( const char *name );
void VID_RestoreGamma( void );
void UI_SetActiveMenu( qboolean fActive );
struct cmd_s *Cmd_GetFirstFunctionHandle( void );
struct cmd_s *Cmd_GetNextFunctionHandle( struct cmd_s *cmd );
struct cmdalias_s *Cmd_AliasGetList( void );
char *Cmd_GetName( struct cmd_s *cmd );
cvar_t *Cvar_GetList( void );
void Cmd_Null_f( void );

typedef struct autocomplete_list_s
{
	const char *name;
	qboolean (*func)( const char *s, char *name, int length );
} autocomplete_list_t;

extern autocomplete_list_t cmd_list[];
extern const char *svc_strings[256];

// soundlib shared exports
qboolean S_Init( void );
void S_Shutdown( void );
void S_Activate( qboolean active, void *hInst );
void S_StopSound( int entnum, int channel, const char *soundname );
int S_GetCurrentStaticSounds( soundlist_t *pout, int size );
void S_StopAllSounds( void );

// gamma routines
void BuildGammaTable( float gamma, float texGamma );
byte TextureToTexGamma( byte b );
byte TextureToGamma( byte b );

#ifdef __cplusplus
}
#endif
#endif//COMMON_H