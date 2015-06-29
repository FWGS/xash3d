
#include "system.h"

#ifdef XASH_SDL
#include <SDL_events.h>
typedef SDL_Event Xash_Event;
#else
typedef void Xash_Event;
#endif

// VGUI generic vertex

typedef struct
{
	vec2_t	point;
	vec2_t	coord;
} vpoint_t;

typedef struct  vguiapi_s
{
	qboolean initialized;
	void	(*DrawInit)( void );
	void	(*DrawShutdown)( void );
	void	(*SetupDrawingText)( int *pColor );
	void	(*SetupDrawingRect)( int *pColor );
	void	(*SetupDrawingImage)( int *pColor );
	void	(*BindTexture)( int id );
	void	(*EnableTexture)( qboolean enable );
	void	(*CreateTexture)( int id, int width, int height );
	void	(*UploadTexture)( int id, const char *buffer, int width, int height );
	void	(*UploadTextureBlock)( int id, int drawX, int drawY, const byte *rgba, int blockWidth, int blockHeight );
	void	(*DrawQuad)( const vpoint_t *ul, const vpoint_t *lr );
	void	(*GetTextureSizes)( int *width, int *height );
	int		(*GenerateTexture)( void );
	void	*(*EngineMalloc)( size_t size );
	void	(*ShowCursor)( void );
	void	(*HideCursor)( void );
	void	(*ActivateCurrentCursor)( void );
	byte		(*GetColor)( int i, int j );
	qboolean	(*IsInGame)( void );
	void	(*Startup)( int width, int height );
	void	(*Shutdown)( void );
	void	*(*GetPanel)( void );
	void	(*RunFrame)( void );
	void	(*ViewportPaintBackground)( int extents[4] );
	void	(*Paint)( void );
	long	(*SurfaceWndProc)( Xash_Event *event );
} vguiapi_t;
