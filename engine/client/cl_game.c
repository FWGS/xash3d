/*
cl_game.c - client dll interaction
Copyright (C) 2008 Uncle Mike

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
#include "const.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "demo_api.h"
#include "ivoicetweak.h"
#include "pm_local.h"
#include "cl_tent.h"
#include "input.h"
#include "shake.h"
#include "sprite.h"
#include "gl_local.h"
#include "library.h"
#include "vgui_draw.h"
#include "sound.h"		// SND_STOP_LOOPING

#define MAX_TEXTCHANNELS	8		// must be power of two (GoldSrc uses 4 channels)
#define TEXT_MSGNAME	"TextMessage%i"

char			cl_textbuffer[MAX_TEXTCHANNELS][512];
client_textmessage_t	cl_textmessage[MAX_TEXTCHANNELS];

static dllfunc_t cdll_exports[] =
{
{ "Initialize", (void **)&clgame.dllFuncs.pfnInitialize },
{ "HUD_VidInit", (void **)&clgame.dllFuncs.pfnVidInit },
{ "HUD_Init", (void **)&clgame.dllFuncs.pfnInit },
{ "HUD_Shutdown", (void **)&clgame.dllFuncs.pfnShutdown },
{ "HUD_Redraw", (void **)&clgame.dllFuncs.pfnRedraw },
{ "HUD_UpdateClientData", (void **)&clgame.dllFuncs.pfnUpdateClientData },
{ "HUD_Reset", (void **)&clgame.dllFuncs.pfnReset },
{ "HUD_PlayerMove", (void **)&clgame.dllFuncs.pfnPlayerMove },
{ "HUD_PlayerMoveInit", (void **)&clgame.dllFuncs.pfnPlayerMoveInit },
{ "HUD_PlayerMoveTexture", (void **)&clgame.dllFuncs.pfnPlayerMoveTexture },
{ "HUD_ConnectionlessPacket", (void **)&clgame.dllFuncs.pfnConnectionlessPacket },
{ "HUD_GetHullBounds", (void **)&clgame.dllFuncs.pfnGetHullBounds },
{ "HUD_Frame", (void **)&clgame.dllFuncs.pfnFrame },
{ "HUD_PostRunCmd", (void **)&clgame.dllFuncs.pfnPostRunCmd },
{ "HUD_Key_Event", (void **)&clgame.dllFuncs.pfnKey_Event },
{ "HUD_AddEntity", (void **)&clgame.dllFuncs.pfnAddEntity },
{ "HUD_CreateEntities", (void **)&clgame.dllFuncs.pfnCreateEntities },
{ "HUD_StudioEvent", (void **)&clgame.dllFuncs.pfnStudioEvent },
{ "HUD_TxferLocalOverrides", (void **)&clgame.dllFuncs.pfnTxferLocalOverrides },
{ "HUD_ProcessPlayerState", (void **)&clgame.dllFuncs.pfnProcessPlayerState },
{ "HUD_TxferPredictionData", (void **)&clgame.dllFuncs.pfnTxferPredictionData },
{ "HUD_TempEntUpdate", (void **)&clgame.dllFuncs.pfnTempEntUpdate },
{ "HUD_DrawNormalTriangles", (void **)&clgame.dllFuncs.pfnDrawNormalTriangles },
{ "HUD_DrawTransparentTriangles", (void **)&clgame.dllFuncs.pfnDrawTransparentTriangles },
{ "HUD_GetUserEntity", (void **)&clgame.dllFuncs.pfnGetUserEntity },
{ "Demo_ReadBuffer", (void **)&clgame.dllFuncs.pfnDemo_ReadBuffer },
{ "CAM_Think", (void **)&clgame.dllFuncs.CAM_Think },
{ "CL_IsThirdPerson", (void **)&clgame.dllFuncs.CL_IsThirdPerson },
{ "CL_CameraOffset", (void **)&clgame.dllFuncs.CL_CameraOffset },
{ "CL_CreateMove", (void **)&clgame.dllFuncs.CL_CreateMove },
{ "IN_ActivateMouse", (void **)&clgame.dllFuncs.IN_ActivateMouse },
{ "IN_DeactivateMouse", (void **)&clgame.dllFuncs.IN_DeactivateMouse },
{ "IN_MouseEvent", (void **)&clgame.dllFuncs.IN_MouseEvent },
{ "IN_Accumulate", (void **)&clgame.dllFuncs.IN_Accumulate },
{ "IN_ClearStates", (void **)&clgame.dllFuncs.IN_ClearStates },
{ "V_CalcRefdef", (void **)&clgame.dllFuncs.pfnCalcRefdef },
{ "KB_Find", (void **)&clgame.dllFuncs.KB_Find },
{ NULL, NULL }
};

// optional exports
static dllfunc_t cdll_new_exports[] = 	// allowed only in SDK 2.3 and higher
{
{ "HUD_GetStudioModelInterface", (void **)&clgame.dllFuncs.pfnGetStudioModelInterface },
{ "HUD_DirectorMessage", (void **)&clgame.dllFuncs.pfnDirectorMessage },
{ "HUD_VoiceStatus", (void **)&clgame.dllFuncs.pfnVoiceStatus },
{ "HUD_ChatInputPosition", (void **)&clgame.dllFuncs.pfnChatInputPosition },
{ "HUD_GetRenderInterface", (void **)&clgame.dllFuncs.pfnGetRenderInterface },	// Xash3D ext
{ "HUD_GetPlayerTeam", (void **)&clgame.dllFuncs.pfnGetPlayerTeam },
{ "HUD_ClipMoveToEntity", (void **)&clgame.dllFuncs.pfnClipMoveToEntity },	// Xash3D ext
{ NULL, NULL }
};

/*
====================
CL_GetEntityByIndex

Render callback for studio models
====================
*/
cl_entity_t *CL_GetEntityByIndex( int index )
{
	if( !clgame.entities ) // not in game yet
		return NULL;

	if( index == 0 )
		return cl.world;

	if( index < 0 )
		return clgame.dllFuncs.pfnGetUserEntity( abs( index ));

	if( index >= clgame.maxEntities )
		return NULL;

	return CL_EDICT_NUM( index );
}

/*
====================
CL_GetServerTime

don't clamped time that come from server
====================
*/
float CL_GetServerTime( void )
{
	return cl.mtime[0];
}

/*
====================
CL_GetLerpFrac

returns current lerp fraction
====================
*/
float CL_GetLerpFrac( void )
{
	return cl.lerpFrac;
}

/*
====================
CL_IsThirdPerson

returns true if thirdperson is enabled
====================
*/
qboolean CL_IsThirdPerson( void )
{
	return cl.thirdperson;
}

/*
====================
CL_GetPlayerInfo

get player info by render request
====================
*/
player_info_t *CL_GetPlayerInfo( int playerIndex )
{
	if( playerIndex < 0 || playerIndex >= cl.maxclients )
		return NULL;

	return &cl.players[playerIndex];
}

/*
====================
CL_CreatePlaylist

Create a default valve playlist
====================
*/
void CL_CreatePlaylist( const char *filename )
{
	file_t	*f;

	f = FS_Open( filename, "w", false );
	if( !f ) return;

	// make standard cdaudio playlist
	FS_Print( f, "blank\n" );		// #1
	FS_Print( f, "Half-Life01.mp3\n" );	// #2
	FS_Print( f, "Prospero01.mp3\n" );	// #3
	FS_Print( f, "Half-Life12.mp3\n" );	// #4
	FS_Print( f, "Half-Life07.mp3\n" );	// #5
	FS_Print( f, "Half-Life10.mp3\n" );	// #6
	FS_Print( f, "Suspense01.mp3\n" );	// #7
	FS_Print( f, "Suspense03.mp3\n" );	// #8
	FS_Print( f, "Half-Life09.mp3\n" );	// #9
	FS_Print( f, "Half-Life02.mp3\n" );	// #10
	FS_Print( f, "Half-Life13.mp3\n" );	// #11
	FS_Print( f, "Half-Life04.mp3\n" );	// #12
	FS_Print( f, "Half-Life15.mp3\n" );	// #13
	FS_Print( f, "Half-Life14.mp3\n" );	// #14
	FS_Print( f, "Half-Life16.mp3\n" );	// #15
	FS_Print( f, "Suspense02.mp3\n" );	// #16
	FS_Print( f, "Half-Life03.mp3\n" );	// #17
	FS_Print( f, "Half-Life08.mp3\n" );	// #18
	FS_Print( f, "Prospero02.mp3\n" );	// #19
	FS_Print( f, "Half-Life05.mp3\n" );	// #20
	FS_Print( f, "Prospero04.mp3\n" );	// #21
	FS_Print( f, "Half-Life11.mp3\n" );	// #22
	FS_Print( f, "Half-Life06.mp3\n" );	// #23
	FS_Print( f, "Prospero03.mp3\n" );	// #24
	FS_Print( f, "Half-Life17.mp3\n" );	// #25
	FS_Print( f, "Prospero05.mp3\n" );	// #26
	FS_Print( f, "Suspense05.mp3\n" );	// #27
	FS_Print( f, "Suspense07.mp3\n" );	// #28
	FS_Close( f );
}

/*
====================
CL_InitCDAudio

Initialize CD playlist
====================
*/
void CL_InitCDAudio( const char *filename )
{
	char	*afile, *pfile;
	string	token;
	int	c = 0;

	if( !FS_FileExists( filename, false ))
	{
		// create a default playlist
		CL_CreatePlaylist( filename );
	}

	afile = FS_LoadFile( filename, NULL, false );
	if( !afile ) return;

	pfile = afile;

	// format: trackname\n [num]
	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		if( !Q_stricmp( token, "blank" )) token[0] = '\0';
		Q_strncpy( clgame.cdtracks[c], token, sizeof( clgame.cdtracks[0] ));

		if( ++c > MAX_CDTRACKS - 1 )
		{
			MsgDev( D_WARN, "CD_Init: too many tracks %i in %s\n", filename, MAX_CDTRACKS );
			break;
		}
	}

	Mem_Free( afile );
}

/*
====================
CL_PointContents

Return contents for point
====================
*/
int CL_PointContents( const vec3_t p )
{
	int cont = CL_TruePointContents( p );

	if( cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN )
		cont = CONTENTS_WATER;
	return cont;
}

/*
====================
StudioEvent

Event callback for studio models
====================
*/
void CL_StudioEvent( struct mstudioevent_s *event, cl_entity_t *pEdict )
{
	clgame.dllFuncs.pfnStudioEvent( event, pEdict );
}

/*
=============
CL_AdjustXPos

adjust text by x pos
=============
*/
static int CL_AdjustXPos( float x, int width, int totalWidth )
{
	int	xPos;

	if( x == -1 )
	{
		xPos = ( clgame.scrInfo.iWidth - width ) * 0.5f;
	}
	else
	{
		if ( x < 0 )
			xPos = (1.0f + x) * clgame.scrInfo.iWidth - totalWidth;	// Alight right
		else // align left
			xPos = x * clgame.scrInfo.iWidth;
	}

	if( xPos + width > clgame.scrInfo.iWidth )
		xPos = clgame.scrInfo.iWidth - width;
	else if( xPos < 0 )
		xPos = 0;

	return xPos;
}

/*
=============
CL_AdjustYPos

adjust text by y pos
=============
*/
static int CL_AdjustYPos( float y, int height )
{
	int	yPos;

	if( y == -1 ) // centered?
	{
		yPos = ( clgame.scrInfo.iHeight - height ) * 0.5f;
	}
	else
	{
		// Alight bottom?
		if( y < 0 )
			yPos = (1.0f + y) * clgame.scrInfo.iHeight - height; // Alight bottom
		else // align top
			yPos = y * clgame.scrInfo.iHeight;
	}

	if( yPos + height > clgame.scrInfo.iHeight )
		yPos = clgame.scrInfo.iHeight - height;
	else if( yPos < 0 )
		yPos = 0;

	return yPos;
}

/*
=============
CL_CenterPrint

print centerscreen message
=============
*/
void CL_CenterPrint( const char *text, float y )
{
	char	*s;
	int	width = 0;
	int	length = 0;

	clgame.centerPrint.lines = 1;
	clgame.centerPrint.totalWidth = 0;
	clgame.centerPrint.time = cl.mtime[0]; // allow pause for centerprint
	Q_strncpy( clgame.centerPrint.message, text, sizeof( clgame.centerPrint.message ));
	s = clgame.centerPrint.message;

	// count the number of lines for centering
	while( *s )
	{
		if( *s == '\n' )
		{
			clgame.centerPrint.lines++;
			if( width > clgame.centerPrint.totalWidth )
				clgame.centerPrint.totalWidth = width;
			width = 0;
		}
		else width += clgame.scrInfo.charWidths[*s];
		s++;
		length++;
	}

	clgame.centerPrint.totalHeight = ( clgame.centerPrint.lines * clgame.scrInfo.iCharHeight ); 
	clgame.centerPrint.y = CL_AdjustYPos( y, clgame.centerPrint.totalHeight );
}

/*
====================
SPR_AdjustSize

draw hudsprite routine
====================
*/
static void SPR_AdjustSize( float *x, float *y, float *w, float *h )
{
	float	xscale, yscale;

	if( !x && !y && !w && !h ) return;

	// scale for screen sizes
	xscale = scr_width->integer / (float)clgame.scrInfo.iWidth;
	yscale = scr_height->integer / (float)clgame.scrInfo.iHeight;

	if( x ) *x *= xscale;
	if( y ) *y *= yscale;
	if( w ) *w *= xscale;
	if( h ) *h *= yscale;
}

/*
====================
TextAdjustSize

draw hudsprite routine
====================
*/
void TextAdjustSize( int *x, int *y, int *w, int *h )
{
	float	xscale, yscale;

	if( !clgame.ds.adjust_size ) return;
	if( !x && !y && !w && !h ) return;

	// scale for screen sizes
	xscale = scr_width->integer / (float)clgame.scrInfo.iWidth;
	yscale = scr_height->integer / (float)clgame.scrInfo.iHeight;

	if( x ) *x *= xscale;
	if( y ) *y *= yscale;
	if( w ) *w *= xscale;
	if( h ) *h *= yscale;
}

/*
====================
PictAdjustSize

draw hudsprite routine
====================
*/
void PicAdjustSize( float *x, float *y, float *w, float *h )
{
	float	xscale, yscale;

	if( !clgame.ds.adjust_size ) return;
	if( !x && !y && !w && !h ) return;

	// scale for screen sizes
	xscale = scr_width->integer / (float)clgame.scrInfo.iWidth;
	yscale = scr_height->integer / (float)clgame.scrInfo.iHeight;

	if( x ) *x *= xscale;
	if( y ) *y *= yscale;
	if( w ) *w *= xscale;
	if( h ) *h *= yscale;
}

static qboolean SPR_Scissor( float *x, float *y, float *width, float *height, float *u0, float *v0, float *u1, float *v1 )
{
	float	dudx, dvdy;

	// clip sub rect to sprite
	if(( width == 0 ) || ( height == 0 ))
		return false;

	if( *x + *width <= clgame.ds.scissor_x )
		return false;
	if( *x >= clgame.ds.scissor_x + clgame.ds.scissor_width )
		return false;
	if( *y + *height <= clgame.ds.scissor_y )
		return false;
	if( *y >= clgame.ds.scissor_y + clgame.ds.scissor_height )
		return false;

	dudx = (*u1 - *u0) / *width;
	dvdy = (*v1 - *v0) / *height;

	if( *x < clgame.ds.scissor_x )
	{
		*u0 += (clgame.ds.scissor_x - *x) * dudx;
		*width -= clgame.ds.scissor_x - *x;
		*x = clgame.ds.scissor_x;
	}

	if( *x + *width > clgame.ds.scissor_x + clgame.ds.scissor_width )
	{
		*u1 -= (*x + *width - (clgame.ds.scissor_x + clgame.ds.scissor_width)) * dudx;
		*width = clgame.ds.scissor_x + clgame.ds.scissor_width - *x;
	}

	if( *y < clgame.ds.scissor_y )
	{
		*v0 += (clgame.ds.scissor_y - *y) * dvdy;
		*height -= clgame.ds.scissor_y - *y;
		*y = clgame.ds.scissor_y;
	}

	if( *y + *height > clgame.ds.scissor_y + clgame.ds.scissor_height )
	{
		*v1 -= (*y + *height - (clgame.ds.scissor_y + clgame.ds.scissor_height)) * dvdy;
		*height = clgame.ds.scissor_y + clgame.ds.scissor_height - *y;
	}

	return true;
}

/*
====================
SPR_DrawGeneric

draw hudsprite routine
====================
*/
static void SPR_DrawGeneric( int frame, float x, float y, float width, float height, const wrect_t *prc )
{
	float	s1, s2, t1, t2;
	int	texnum;

	if( width == -1 && height == -1 )
	{
		int	w, h;

		// assume we get sizes from image
		R_GetSpriteParms( &w, &h, NULL, frame, clgame.ds.pSprite );

		width = w;
		height = h;
	}

	if( prc )
	{
		wrect_t	rc;

		rc = *prc;

		// Sigh! some stupid modmakers set wrong rectangles in hud.txt 
		if( rc.left <= 0 || rc.left >= width ) rc.left = 0;
		if( rc.top <= 0 || rc.top >= height ) rc.top = 0;
		if( rc.right <= 0 || rc.right > width ) rc.right = width;
		if( rc.bottom <= 0 || rc.bottom > height ) rc.bottom = height;

		// calc user-defined rectangle
		s1 = (float)rc.left / width;
		t1 = (float)rc.top / height;
		s2 = (float)rc.right / width;
		t2 = (float)rc.bottom / height;
		width = rc.right - rc.left;
		height = rc.bottom - rc.top;
	}
	else
	{
		s1 = t1 = 0.0f;
		s2 = t2 = 1.0f;
	}

	// pass scissor test if supposed
	if( clgame.ds.scissor_test && !SPR_Scissor( &x, &y, &width, &height, &s1, &t1, &s2, &t2 ))
		return;

	// scale for screen sizes
	SPR_AdjustSize( &x, &y, &width, &height );
	texnum = R_GetSpriteTexture( clgame.ds.pSprite, frame );
	pglColor4ubv( clgame.ds.spriteColor );
	R_DrawStretchPic( x, y, width, height, s1, t1, s2, t2, texnum );
}

/*
=============
CL_DrawCenterPrint

called each frame
=============
*/
void CL_DrawCenterPrint( void )
{
	char	*pText;
	int	i, j, x, y;
	int	width, lineLength;
	byte	*colorDefault, line[80];
	int	charWidth, charHeight;

	if( !clgame.centerPrint.time )
		return;

	if(( cl.time - clgame.centerPrint.time ) >= scr_centertime->value )
	{
		// time expired
		clgame.centerPrint.time = 0.0f;
		return;
	}

	y = clgame.centerPrint.y; // start y
	colorDefault = g_color_table[7];
	pText = clgame.centerPrint.message;
	Con_DrawCharacterLen( 0, NULL, &charHeight );
	
	for( i = 0; i < clgame.centerPrint.lines; i++ )
	{
		lineLength = 0;
		width = 0;

		while( *pText && *pText != '\n' )
		{
			byte c = *pText;
			line[lineLength] = c;
			Con_DrawCharacterLen( c, &charWidth, NULL );
			width += charWidth;
			lineLength++;
			pText++;
		}

		pText++; // Skip LineFeed
		line[lineLength] = 0;

		x = CL_AdjustXPos( -1, width, clgame.centerPrint.totalWidth );

		for( j = 0; j < lineLength; j++ )
		{
			if( x >= 0 && y >= 0 && x <= clgame.scrInfo.iWidth )
				x += Con_DrawCharacter( x, y, line[j], colorDefault );
		}
		y += charHeight;
	}
}

/*
=============
CL_DrawScreenFade

fill screen with specfied color
can be modulated
=============
*/
void CL_DrawScreenFade( void )
{
	screenfade_t	*sf = &clgame.fade;
	int		iFadeAlpha, testFlags;

	// keep pushing reset time out indefinitely
	if( sf->fadeFlags & FFADE_STAYOUT )
		sf->fadeReset = cl.time + 0.1f;
		
	if( sf->fadeReset == 0.0f && sf->fadeEnd == 0.0f )
		return;	// inactive

	// all done?
	if(( cl.time > sf->fadeReset ) && ( cl.time > sf->fadeEnd ))
	{
		Q_memset( &clgame.fade, 0, sizeof( clgame.fade ));
		return;
	}

	testFlags = (sf->fadeFlags & ~FFADE_MODULATE);

	// fading...
	if( testFlags == FFADE_STAYOUT )
	{
		iFadeAlpha = sf->fadealpha;
	}
	else
	{
		iFadeAlpha = sf->fadeSpeed * ( sf->fadeEnd - cl.time );
		if( sf->fadeFlags & FFADE_OUT ) iFadeAlpha += sf->fadealpha;
		iFadeAlpha = bound( 0, iFadeAlpha, sf->fadealpha );
	}

	pglColor4ub( sf->fader, sf->fadeg, sf->fadeb, iFadeAlpha );

	if( sf->fadeFlags & FFADE_MODULATE )
		GL_SetRenderMode( kRenderTransAdd );
	else GL_SetRenderMode( kRenderTransTexture );
	R_DrawStretchPic( 0, 0, scr_width->integer, scr_height->integer, 0, 0, 1, 1, cls.fillImage );
	pglColor4ub( 255, 255, 255, 255 );
}

/*
====================
CL_InitTitles

parse all messages that declared in titles.txt
and hold them into permament memory pool 
====================
*/
static void CL_InitTitles( const char *filename )
{
	size_t	fileSize;
	byte	*pMemFile;
	int	i;

	// initialize text messages (game_text)
	for( i = 0; i < MAX_TEXTCHANNELS; i++ )
	{
		cl_textmessage[i].pName = _copystring( clgame.mempool, va( TEXT_MSGNAME, i ), __FILE__, __LINE__ );
		cl_textmessage[i].pMessage = cl_textbuffer[i];
	}

	// clear out any old data that's sitting around.
	if( clgame.titles ) Mem_Free( clgame.titles );

	clgame.titles = NULL;
	clgame.numTitles = 0;

	pMemFile = FS_LoadFile( filename, &fileSize, false );
	if( !pMemFile ) return;

	CL_TextMessageParse( pMemFile, fileSize );
	Mem_Free( pMemFile );
}

/*
====================
CL_ParseTextMessage

Parse TE_TEXTMESSAGE
====================
*/
void CL_ParseTextMessage( sizebuf_t *msg )
{
	static int		msgindex = 0;
	client_textmessage_t	*text;
	int			channel;

	// read channel ( 0 - auto)
	channel = BF_ReadByte( msg );

	if( channel <= 0 || channel > ( MAX_TEXTCHANNELS - 1 ))
	{
		// invalid channel specified, use internal counter		
		if( channel != 0 ) MsgDev( D_ERROR, "HudText: invalid channel %i\n", channel );
		channel = msgindex;
		msgindex = (msgindex + 1) & (MAX_TEXTCHANNELS - 1);
	}	

	// grab message channel
	text = &cl_textmessage[channel];

	text->x = (float)(BF_ReadShort( msg ) / 8192.0f);
	text->y = (float)(BF_ReadShort( msg ) / 8192.0f);
	text->effect = BF_ReadByte( msg );
	text->r1 = BF_ReadByte( msg );
	text->g1 = BF_ReadByte( msg );
	text->b1 = BF_ReadByte( msg );
	text->a1 = BF_ReadByte( msg );
	text->r2 = BF_ReadByte( msg );
	text->g2 = BF_ReadByte( msg );
	text->b2 = BF_ReadByte( msg );
	text->a2 = BF_ReadByte( msg );
	text->fadein = (float)(BF_ReadShort( msg ) / 256.0f );
	text->fadeout = (float)(BF_ReadShort( msg ) / 256.0f );
	text->holdtime = (float)(BF_ReadShort( msg ) / 256.0f );

	if( text->effect == 2 )
		text->fxtime = (float)(BF_ReadShort( msg ) / 256.0f );
	else text->fxtime = 0.0f;

	// to prevent grab too long messages
	Q_strncpy( (char *)text->pMessage, BF_ReadString( msg ), 512 ); 		

	// NOTE: a "HudText" message contain only 'string' with message name, so we
	// don't needs to use MSG_ routines here, just directly write msgname into netbuffer
	CL_DispatchUserMessage( "HudText", Q_strlen( text->pName ) + 1, (void *)text->pName );
}

/*
====================
CL_GetLocalPlayer

Render callback for studio models
====================
*/
cl_entity_t *CL_GetLocalPlayer( void )
{
	cl_entity_t	*player;

	player = CL_EDICT_NUM( cl.playernum + 1 );
	ASSERT( player != NULL );

	return player;
}

/*
====================
CL_GetMaxlients

Render callback for studio models
====================
*/
int CL_GetMaxClients( void )
{
	return cl.maxclients;
}

/*
====================
CL_SoundFromIndex

return soundname from index
====================
*/
const char *CL_SoundFromIndex( int index )
{
	sfx_t	*sfx = NULL;
	int	hSound;

	// make sure what we in-bounds
	index = bound( 0, index, MAX_SOUNDS );
	hSound = cl.sound_index[index];

	if( !hSound )
	{
		MsgDev( D_ERROR, "CL_SoundFromIndex: invalid sound index %i\n", index );
		return NULL;
	}

	sfx = S_GetSfxByHandle( hSound );
	if( !sfx )
	{
		MsgDev( D_ERROR, "CL_SoundFromIndex: bad sfx for index %i\n", index );
		return NULL;
	}

	return sfx->name;
}

/*
=========
SPR_EnableScissor

=========
*/
static void SPR_EnableScissor( int x, int y, int width, int height )
{
	// check bounds
	x = bound( 0, x, clgame.scrInfo.iWidth );
	y = bound( 0, y, clgame.scrInfo.iHeight );
	width = bound( 0, width, clgame.scrInfo.iWidth - x );
	height = bound( 0, height, clgame.scrInfo.iHeight - y );

	clgame.ds.scissor_x = x;
	clgame.ds.scissor_width = width;
	clgame.ds.scissor_y = y;
	clgame.ds.scissor_height = height;
	clgame.ds.scissor_test = true;
}

/*
=========
SPR_DisableScissor

=========
*/
static void SPR_DisableScissor( void )
{
	clgame.ds.scissor_x = 0;
	clgame.ds.scissor_width = 0;
	clgame.ds.scissor_y = 0;
	clgame.ds.scissor_height = 0;
	clgame.ds.scissor_test = false;
}

/*
====================
CL_DrawCrosshair

Render crosshair
====================
*/
void CL_DrawCrosshair( void )
{
	int		x, y, width, height;
	cl_entity_t	*pPlayer;

	if( !clgame.ds.pCrosshair || cl.refdef.crosshairangle[2] || !cl_crosshair->integer )
		return;

	pPlayer = CL_GetLocalPlayer();

	if( cl.frame.local.client.deadflag != DEAD_NO || cl.frame.local.client.flags & FL_FROZEN )
		return;

	// any camera on
	if( cl.refdef.viewentity != pPlayer->index )
		return;

	// get crosshair dimension
	width = clgame.ds.rcCrosshair.right - clgame.ds.rcCrosshair.left;
	height = clgame.ds.rcCrosshair.bottom - clgame.ds.rcCrosshair.top;

	x = clgame.scrInfo.iWidth / 2; 
	y = clgame.scrInfo.iHeight / 2;

	// g-cont - cl.refdef.crosshairangle is the autoaim angle.
	// if we're not using autoaim, just draw in the middle of the screen
	if( !VectorIsNull( cl.refdef.crosshairangle ))
	{
		vec3_t	angles;
		vec3_t	forward;
		vec3_t	point, screen;

		VectorAdd( cl.refdef.viewangles, cl.refdef.crosshairangle, angles );
		AngleVectors( angles, forward, NULL, NULL );
		VectorAdd( cl.refdef.vieworg, forward, point );
		R_WorldToScreen( point, screen );

		x += 0.5f * screen[0] * scr_width->integer + 0.5f;
		y += 0.5f * screen[1] * scr_height->integer + 0.5f;
	}

	clgame.ds.pSprite = clgame.ds.pCrosshair;

	GL_SetRenderMode( kRenderTransTexture );
	*(int *)clgame.ds.spriteColor = *(int *)clgame.ds.rgbaCrosshair;

	SPR_EnableScissor( x - 0.5f * width, y - 0.5f * height, width, height );
	SPR_DrawGeneric( 0, x - 0.5f * width, y - 0.5f * height, -1, -1, &clgame.ds.rcCrosshair );
	SPR_DisableScissor();
}

/*
=============
CL_DrawLoading

draw loading progress bar
=============
*/
static void CL_DrawLoading( float percent )
{
	int	x, y, width, height, right;
	float	xscale, yscale, step, s2;

	R_GetTextureParms( &width, &height, cls.loadingBar );
	x = ( clgame.scrInfo.iWidth - width ) >> 1;
	y = ( clgame.scrInfo.iHeight - height) >> 1;

	xscale = scr_width->integer / (float)clgame.scrInfo.iWidth;
	yscale = scr_height->integer / (float)clgame.scrInfo.iHeight;

	x *= xscale;
	y *= yscale;
	width *= xscale;
	height *= yscale;

	if( cl_allow_levelshots->integer )
          {
		pglColor4ub( 128, 128, 128, 255 );
		GL_SetRenderMode( kRenderTransTexture );
		R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, cls.loadingBar );

		step = (float)width / 100.0f;
		right = (int)ceil( percent * step );
		s2 = (float)right / width;
		width = right;
	
		pglColor4ub( 208, 152, 0, 255 );
		GL_SetRenderMode( kRenderTransTexture );
		R_DrawStretchPic( x, y, width, height, 0, 0, s2, 1, cls.loadingBar );
		pglColor4ub( 255, 255, 255, 255 );
	}
	else
	{
		pglColor4ub( 255, 255, 255, 255 );
		GL_SetRenderMode( kRenderTransTexture );
		R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, cls.loadingBar );
	}
}

/*
=============
CL_DrawPause

draw pause sign
=============
*/
static void CL_DrawPause( void )
{
	int	x, y, width, height;
	float	xscale, yscale;

	R_GetTextureParms( &width, &height, cls.pauseIcon );
	x = ( clgame.scrInfo.iWidth - width ) >> 1;
	y = ( clgame.scrInfo.iHeight - height) >> 1;

	xscale = scr_width->integer / (float)clgame.scrInfo.iWidth;
	yscale = scr_height->integer / (float)clgame.scrInfo.iHeight;

	x *= xscale;
	y *= yscale;
	width *= xscale;
	height *= yscale;

	pglColor4ub( 255, 255, 255, 255 );
	GL_SetRenderMode( kRenderTransTexture );
	R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, cls.pauseIcon );
}

void CL_DrawHUD( int state )
{
	if( state == CL_ACTIVE && !cl.video_prepped )
		state = CL_LOADING;

	if( state == CL_ACTIVE && cl.refdef.paused )
		state = CL_PAUSED;

	switch( state )
	{
	case CL_ACTIVE:
		CL_DrawScreenFade ();
		CL_DrawCrosshair ();
		CL_DrawCenterPrint ();
		clgame.dllFuncs.pfnRedraw( cl.time, cl.refdef.intermission );
		break;
	case CL_PAUSED:
		CL_DrawScreenFade ();
		CL_DrawCrosshair ();
		CL_DrawCenterPrint ();
		clgame.dllFuncs.pfnRedraw( cl.time, cl.refdef.intermission );
		CL_DrawPause();
		break;
	case CL_LOADING:
		CL_DrawLoading( scr_loading->value );
		break;
	case CL_CHANGELEVEL:
		if( cls.draw_changelevel )
		{
			CL_DrawLoading( 100.0f );
			cls.draw_changelevel = false;
		}
		break;
	}
}

void CL_LinkUserMessage( char *pszName, const int svc_num, int iSize )
{
	int	i;

	if( !pszName || !*pszName )
		Host_Error( "CL_LinkUserMessage: bad message name\n" );

	if( svc_num < svc_lastmsg )
		Host_Error( "CL_LinkUserMessage: tried to hook a system message \"%s\"\n", svc_strings[svc_num] );	

	// see if already hooked
	for( i = 0; i < MAX_USER_MESSAGES && clgame.msg[i].name[0]; i++ )
	{
		// NOTE: no check for DispatchFunc, check only name
		if( !Q_strcmp( clgame.msg[i].name, pszName ))
		{
			clgame.msg[i].number = svc_num;
			clgame.msg[i].size = iSize;
			return;
		}
	}

	if( i == MAX_USER_MESSAGES ) 
	{
		Host_Error( "CL_LinkUserMessage: MAX_USER_MESSAGES hit!\n" );
		return;
	}

	// register new message without DispatchFunc, so we should parse it properly
	Q_strncpy( clgame.msg[i].name, pszName, sizeof( clgame.msg[i].name ));
	clgame.msg[i].number = svc_num;
	clgame.msg[i].size = iSize;
}

void CL_FreeEntity( cl_entity_t *pEdict )
{
	ASSERT( pEdict );
	R_RemoveEfrags( pEdict );
	CL_KillDeadBeams( pEdict );
}

void CL_ClearWorld( void )
{
	cl.world = clgame.entities;
	cl.world->curstate.modelindex = 1;	// world model
	cl.world->curstate.solid = SOLID_BSP;
	cl.world->curstate.movetype = MOVETYPE_PUSH;
	cl.world->model = cl.worldmodel;
	cl.world->index = 0;

	clgame.ds.cullMode = GL_FRONT;
	clgame.numStatics = 0;
}

void CL_InitEdicts( void )
{
	ASSERT( clgame.entities == NULL );

	CL_UPDATE_BACKUP = ( cl.maxclients == 1 ) ? SINGLEPLAYER_BACKUP : MULTIPLAYER_BACKUP;
	cls.num_client_entities = CL_UPDATE_BACKUP * 64;
	cls.packet_entities = Z_Realloc( cls.packet_entities, sizeof( entity_state_t ) * cls.num_client_entities );
	clgame.entities = Mem_Alloc( clgame.mempool, sizeof( cl_entity_t ) * clgame.maxEntities );
	clgame.static_entities = Mem_Alloc( clgame.mempool, sizeof( cl_entity_t ) * MAX_STATIC_ENTITIES );
	clgame.numStatics = 0;

	if(( clgame.maxRemapInfos - 1 ) != clgame.maxEntities )
	{
		CL_ClearAllRemaps (); // purge old remap info
		clgame.maxRemapInfos = clgame.maxEntities + 1; 
		clgame.remap_info = (remap_info_t **)Mem_Alloc( clgame.mempool, sizeof( remap_info_t* ) * clgame.maxRemapInfos );
	}
}

void CL_FreeEdicts( void )
{
	if( clgame.entities )
		Mem_Free( clgame.entities );
	clgame.entities = NULL;

	if( clgame.static_entities )
		Mem_Free( clgame.static_entities );
	clgame.static_entities = NULL;

	if( cls.packet_entities )
		Z_Free( cls.packet_entities );

	cls.packet_entities = NULL;
	cls.num_client_entities = 0;
	cls.next_client_entities = 0;
	clgame.numStatics = 0;
}

void CL_ClearEdicts( void )
{
	if( clgame.entities != NULL )
		return;

	// in case we stopped with error
	clgame.maxEntities = 2;
	CL_InitEdicts();
}

/*
===============================================================================
	CGame Builtin Functions

===============================================================================
*/
static qboolean CL_LoadHudSprite( const char *szSpriteName, model_t *m_pSprite, qboolean mapSprite, uint texFlags )
{
	byte	*buf;
	size_t	size;
	qboolean	loaded;

	ASSERT( m_pSprite != NULL );

	buf = FS_LoadFile( szSpriteName, &size, false );
	if( !buf ) return false;

	Q_strncpy( m_pSprite->name, szSpriteName, sizeof( m_pSprite->name ));
	m_pSprite->flags = 256; // it's hud sprite, make difference names to prevent free shared textures

	if( mapSprite ) Mod_LoadMapSprite( m_pSprite, buf, size, &loaded );
	else Mod_LoadSpriteModel( m_pSprite, buf, &loaded, texFlags );		

	Mem_Free( buf );

	if( !loaded )
	{
		Mod_UnloadSpriteModel( m_pSprite );
		return false;
	}
	return true;
}

/*
=========
pfnSPR_LoadExt

=========
*/
HSPRITE pfnSPR_LoadExt( const char *szPicName, uint texFlags )
{
	char	name[64];
	int	i;

	if( !szPicName || !*szPicName )
	{
		MsgDev( D_ERROR, "CL_LoadSprite: bad name!\n" );
		return 0;
	}

	Q_strncpy( name, szPicName, sizeof( name ));
	COM_FixSlashes( name );

	// slot 0 isn't used
	for( i = 1; i < MAX_IMAGES; i++ )
	{
		if( !Q_stricmp( clgame.sprites[i].name, name ))
		{
			// prolonge registration
			clgame.sprites[i].needload = clgame.load_sequence;
			return i;
		}
	}

	// find a free model slot spot
	for( i = 1; i < MAX_IMAGES; i++ )
	{
		if( !clgame.sprites[i].name[0] )
			break; // this is a valid spot
	}

	if( i == MAX_IMAGES ) 
	{
		MsgDev( D_ERROR, "SPR_Load: can't load %s, MAX_HSPRITES limit exceeded\n", szPicName );
		return 0;
	}

	// load new model
	if( CL_LoadHudSprite( name, &clgame.sprites[i], false, texFlags ))
	{
		clgame.sprites[i].needload = clgame.load_sequence;
		return i;
	}
	return 0;
}

/*
=========
pfnSPR_Load

=========
*/
HSPRITE pfnSPR_Load( const char *szPicName )
{
	return pfnSPR_LoadExt( szPicName, 0 );
}

/*
=============
CL_GetSpritePointer

=============
*/
const model_t *CL_GetSpritePointer( HSPRITE hSprite )
{
	if( hSprite <= 0 || hSprite > ( MAX_IMAGES - 1 ))
		return NULL; // bad image
	return &clgame.sprites[hSprite];
}

/*
=========
pfnSPR_Frames

=========
*/
static int pfnSPR_Frames( HSPRITE hPic )
{
	int	numFrames;

	R_GetSpriteParms( NULL, NULL, &numFrames, 0, CL_GetSpritePointer( hPic ));

	return numFrames;
}

/*
=========
pfnSPR_Height

=========
*/
static int pfnSPR_Height( HSPRITE hPic, int frame )
{
	int	sprHeight;

	R_GetSpriteParms( NULL, &sprHeight, NULL, frame, CL_GetSpritePointer( hPic ));

	return sprHeight;
}

/*
=========
pfnSPR_Width

=========
*/
static int pfnSPR_Width( HSPRITE hPic, int frame )
{
	int	sprWidth;

	R_GetSpriteParms( &sprWidth, NULL, NULL, frame, CL_GetSpritePointer( hPic ));

	return sprWidth;
}

/*
=========
pfnSPR_Set

=========
*/
static void pfnSPR_Set( HSPRITE hPic, int r, int g, int b )
{
	clgame.ds.pSprite = CL_GetSpritePointer( hPic );
	clgame.ds.spriteColor[0] = bound( 0, r, 255 );
	clgame.ds.spriteColor[1] = bound( 0, g, 255 );
	clgame.ds.spriteColor[2] = bound( 0, b, 255 );
	clgame.ds.spriteColor[3] = 255;

	// set default state
	pglDisable( GL_BLEND );
	pglDisable( GL_ALPHA_TEST );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
}

/*
=========
pfnSPR_Draw

=========
*/
static void pfnSPR_Draw( int frame, int x, int y, const wrect_t *prc )
{
	pglEnable( GL_ALPHA_TEST );
	SPR_DrawGeneric( frame, x, y, -1, -1, prc );
}

/*
=========
pfnSPR_DrawHoles

=========
*/
static void pfnSPR_DrawHoles( int frame, int x, int y, const wrect_t *prc )
{
	GL_SetRenderMode( kRenderTransAlpha );
	SPR_DrawGeneric( frame, x, y, -1, -1, prc );
}

/*
=========
pfnSPR_DrawAdditive

=========
*/
static void pfnSPR_DrawAdditive( int frame, int x, int y, const wrect_t *prc )
{
	GL_SetRenderMode( kRenderTransAdd );
	SPR_DrawGeneric( frame, x, y, -1, -1, prc );
}

/*
=========
pfnSPR_GetList

for parsing half-life scripts - hud.txt etc
=========
*/
static client_sprite_t *pfnSPR_GetList( char *psz, int *piCount )
{
	client_sprite_t	*pList;
	int		index, numSprites = 0;
	char		*afile, *pfile;
	string		token;
	byte		*pool;

	if( piCount ) *piCount = 0;

	if( !clgame.itemspath[0] )	// typically it's sprites\*.txt
		FS_ExtractFilePath( psz, clgame.itemspath );

	afile = FS_LoadFile( psz, NULL, false );
	if( !afile ) return NULL;

	pfile = afile;
	pfile = COM_ParseFile( pfile, token );          
	numSprites = Q_atoi( token );

	if( !cl.video_prepped ) pool = cls.mempool;	// static memory
	else pool = com_studiocache;			// temporary

	// name, res, pic, x, y, w, h
	// NOTE: we must use com_studiocache because it will be purge on next restart or change map
	pList = Mem_Alloc( pool, sizeof( client_sprite_t ) * numSprites );

	for( index = 0; index < numSprites; index++ )
	{
		if(( pfile = COM_ParseFile( pfile, token )) == NULL )
			break;

		Q_strncpy( pList[index].szName, token, sizeof( pList[index].szName ));

		// read resolution
		pfile = COM_ParseFile( pfile, token );
		pList[index].iRes = Q_atoi( token );

		// read spritename
		pfile = COM_ParseFile( pfile, token );
		Q_strncpy( pList[index].szSprite, token, sizeof( pList[index].szSprite ));

		// parse rectangle
		pfile = COM_ParseFile( pfile, token );
		pList[index].rc.left = Q_atoi( token );

		pfile = COM_ParseFile( pfile, token );
		pList[index].rc.top = Q_atoi( token );

		pfile = COM_ParseFile( pfile, token );
		pList[index].rc.right = pList[index].rc.left + Q_atoi( token );

		pfile = COM_ParseFile( pfile, token );
		pList[index].rc.bottom = pList[index].rc.top + Q_atoi( token );

		if( piCount ) (*piCount)++;
	}

	if( index < numSprites )
		MsgDev( D_WARN, "SPR_GetList: unexpected end of %s (%i should be %i)\n", psz, numSprites, index );

	Mem_Free( afile );

	return pList;
}

/*
=============
pfnFillRGBA

=============
*/
static void pfnFillRGBA( int x, int y, int width, int height, int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );
	pglColor4ub( r, g, b, a );

	SPR_AdjustSize( (float *)&x, (float *)&y, (float *)&width, (float *)&height );

	GL_SetRenderMode( kRenderTransAdd );
	R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, cls.fillImage );
	pglColor4ub( 255, 255, 255, 255 );
}

/*
=============
pfnGetScreenInfo

get actual screen info
=============
*/
static int pfnGetScreenInfo( SCREENINFO *pscrinfo )
{
	// setup screen info
	clgame.scrInfo.iSize = sizeof( clgame.scrInfo );

	if( Cvar_VariableInteger( "hud_scale" ))
	{
		if( scr_width->integer < 640 )
		{
			// virtual screen space 320x200
			clgame.scrInfo.iWidth = 320;
			clgame.scrInfo.iHeight = 200;
		}
		else
		{
			// virtual screen space 640x480
			clgame.scrInfo.iWidth = 640;
			clgame.scrInfo.iHeight = 480;
		}
		clgame.scrInfo.iFlags |= SCRINFO_STRETCHED;
	}
	else
	{
		clgame.scrInfo.iWidth = scr_width->integer;
		clgame.scrInfo.iHeight = scr_height->integer;
		clgame.scrInfo.iFlags &= ~SCRINFO_STRETCHED;
	}

	if( !pscrinfo ) return 0;

	if( pscrinfo->iSize != clgame.scrInfo.iSize )
		clgame.scrInfo.iSize = pscrinfo->iSize;

	// copy screeninfo out
	Q_memcpy( pscrinfo, &clgame.scrInfo, clgame.scrInfo.iSize );

	return 1;
}

/*
=============
pfnSetCrosshair

setup crosshair
=============
*/
static void pfnSetCrosshair( HSPRITE hspr, wrect_t rc, int r, int g, int b )
{
	clgame.ds.rgbaCrosshair[0] = (byte)r;
	clgame.ds.rgbaCrosshair[1] = (byte)g;
	clgame.ds.rgbaCrosshair[2] = (byte)b;
	clgame.ds.rgbaCrosshair[3] = (byte)0xFF;
	clgame.ds.pCrosshair = CL_GetSpritePointer( hspr );
	clgame.ds.rcCrosshair = rc;
}

/*
=============
pfnHookUserMsg

=============
*/
static int pfnHookUserMsg( const char *pszName, pfnUserMsgHook pfn )
{
	int	i;

	// ignore blank names or invalid callbacks
	if( !pszName || !*pszName || !pfn )
		return 0;	

	for( i = 0; i < MAX_USER_MESSAGES && clgame.msg[i].name[0]; i++ )
	{
		// see if already hooked
		if( !Q_strcmp( clgame.msg[i].name, pszName ))
			return 1;
	}

	if( i == MAX_USER_MESSAGES ) 
	{
		Host_Error( "HookUserMsg: MAX_USER_MESSAGES hit!\n" );
		return 0;
	}

	// hook new message
	Q_strncpy( clgame.msg[i].name, pszName, sizeof( clgame.msg[i].name ));
	clgame.msg[i].func = pfn;

	return 1;
}

/*
=============
pfnServerCmd

=============
*/
static int pfnServerCmd( const char *szCmdString )
{
	string buf;

	if( !szCmdString || !szCmdString[0] )
		return 0;

	// just like the client typed "cmd xxxxx" at the console
	Q_snprintf( buf, sizeof( buf ) - 1, "cmd %s\n", szCmdString );
	Cbuf_AddText( buf );

	return 1;
}

/*
=============
pfnClientCmd

=============
*/
static int pfnClientCmd( const char *szCmdString )
{
	if( !szCmdString || !szCmdString[0] )
		return 0;

	Cbuf_AddText( szCmdString );
	Cbuf_AddText( "\n" );
	return 1;
}

/*
=============
pfnGetPlayerInfo

=============
*/
static void pfnGetPlayerInfo( int ent_num, hud_player_info_t *pinfo )
{
	player_info_t	*player;
	cl_entity_t	*ent;
	qboolean		spec = false;

	ent = CL_GetEntityByIndex( ent_num );
	ent_num -= 1; // player list if offset by 1 from ents

	if( ent_num >= cl.maxclients || ent_num < 0 || !cl.players[ent_num].name[0] )
	{
		Q_memset( pinfo, 0, sizeof( *pinfo ));
		return;
	}

	player = &cl.players[ent_num];
	pinfo->thisplayer = ( ent_num == cl.playernum ) ? true : false;
	if( ent ) spec = ent->curstate.spectator;

	pinfo->name = player->name;
	pinfo->model = player->model;

	pinfo->spectator = spec;		
	pinfo->ping = player->ping;
	pinfo->packetloss = player->packet_loss;
	pinfo->topcolor = Q_atoi( Info_ValueForKey( player->userinfo, "topcolor" ));
	pinfo->bottomcolor = Q_atoi( Info_ValueForKey( player->userinfo, "bottomcolor" ));
}

/*
=============
pfnPlaySoundByName

=============
*/
static void pfnPlaySoundByName( const char *szSound, float volume )
{
	int hSound = S_RegisterSound( szSound );
	S_StartSound( NULL, cl.refdef.viewentity, CHAN_ITEM, hSound, volume, ATTN_NORM, PITCH_NORM, SND_STOP_LOOPING );
}

/*
=============
pfnPlaySoundByIndex

=============
*/
static void pfnPlaySoundByIndex( int iSound, float volume )
{
	int hSound;

	// make sure what we in-bounds
	iSound = bound( 0, iSound, MAX_SOUNDS );
	hSound = cl.sound_index[iSound];

	if( !hSound )
	{
		MsgDev( D_ERROR, "CL_PlaySoundByIndex: invalid sound handle %i\n", iSound );
		return;
	}
	S_StartSound( NULL, cl.refdef.viewentity, CHAN_ITEM, hSound, volume, ATTN_NORM, PITCH_NORM, SND_STOP_LOOPING );
}

/*
=============
pfnTextMessageGet

returns specified message from titles.txt
=============
*/
client_textmessage_t *CL_TextMessageGet( const char *pName )
{
	int	i;

	// first check internal messages
	for( i = 0; i < MAX_TEXTCHANNELS; i++ )
	{
		if( !Q_strcmp( pName, va( TEXT_MSGNAME, i )))
			return cl_textmessage + i;
	}

	// find desired message
	for( i = 0; i < clgame.numTitles; i++ )
	{
		if( !Q_stricmp( pName, clgame.titles[i].pName ))
			return clgame.titles + i;
	}
	return NULL; // found nothing
}

/*
=============
pfnDrawCharacter

returns drawed chachter width (in real screen pixels)
=============
*/
static int pfnDrawCharacter( int x, int y, int number, int r, int g, int b )
{
	if( !cls.creditsFont.valid )
		return 0;

	number &= 255;

	if( number < 32 ) return 0;
	if( y < -clgame.scrInfo.iCharHeight )
		return 0;

	clgame.ds.adjust_size = true;
	pfnPIC_Set( cls.creditsFont.hFontTexture, r, g, b, 255 );
	pfnPIC_DrawAdditive( x, y, -1, -1, &cls.creditsFont.fontRc[number] );
	clgame.ds.adjust_size = false;

	return clgame.scrInfo.charWidths[number];
}

/*
=============
pfnDrawConsoleString

drawing string like a console string 
=============
*/
int pfnDrawConsoleString( int x, int y, char *string )
{
	int	drawLen;

	if( !string || !*string ) return 0; // silent ignore
	clgame.ds.adjust_size = true;
	Con_SetFont( con_fontsize->integer );
	drawLen = Con_DrawString( x, y, string, clgame.ds.textColor );
	MakeRGBA( clgame.ds.textColor, 255, 255, 255, 255 );
	clgame.ds.adjust_size = false;
	Con_RestoreFont();

	return (x + drawLen); // exclude color prexfixes
}

/*
=============
pfnDrawSetTextColor

set color for anything
=============
*/
void pfnDrawSetTextColor( float r, float g, float b )
{
	// bound color and convert to byte
	clgame.ds.textColor[0] = (byte)bound( 0, r * 255, 255 );
	clgame.ds.textColor[1] = (byte)bound( 0, g * 255, 255 );
	clgame.ds.textColor[2] = (byte)bound( 0, b * 255, 255 );
	clgame.ds.textColor[3] = (byte)0xFF;
}

/*
=============
pfnDrawConsoleStringLen

compute string length in screen pixels
=============
*/
void pfnDrawConsoleStringLen( const char *pText, int *length, int *height )
{
	Con_SetFont( con_fontsize->integer );
	Con_DrawStringLen( pText, length, height );
	Con_RestoreFont();
}

/*
=============
pfnConsolePrint

prints directly into console (can skip notify)
=============
*/
static void pfnConsolePrint( const char *string )
{
	if( !string || !*string ) return;
	if( *string != 1 ) Con_Print( (char *)string ); // show notify
	else Con_NPrintf( 0, (char *)string + 1 ); // skip notify
}

/*
=============
pfnCenterPrint

holds and fade message at center of screen
like trigger_multiple message in q1
=============
*/
static void pfnCenterPrint( const char *string )
{
	if( !string || !*string ) return; // someone stupid joke
	CL_CenterPrint( string, 0.25f );
}

/*
=========
GetWindowCenterX

=========
*/
static int pfnGetWindowCenterX( void )
{
	return host.window_center_x;
}

/*
=========
GetWindowCenterY

=========
*/
static int pfnGetWindowCenterY( void )
{
	return host.window_center_y;
}

/*
=============
pfnGetViewAngles

return interpolated angles from previous frame
=============
*/
static void pfnGetViewAngles( float *angles )
{
	if( angles ) VectorCopy( cl.refdef.cl_viewangles, angles );
}

/*
=============
pfnSetViewAngles

return interpolated angles from previous frame
=============
*/
static void pfnSetViewAngles( float *angles )
{
	if( angles ) VectorCopy( angles, cl.refdef.cl_viewangles );
}

/*
=============
pfnPhysInfo_ValueForKey

=============
*/
static const char* pfnPhysInfo_ValueForKey( const char *key )
{
	return Info_ValueForKey( cl.frame.local.client.physinfo, key );
}

/*
=============
pfnServerInfo_ValueForKey

=============
*/
static const char* pfnServerInfo_ValueForKey( const char *key )
{
	return Info_ValueForKey( cl.serverinfo, key );
}

/*
=============
pfnGetClientMaxspeed

value that come from server
=============
*/
static float pfnGetClientMaxspeed( void )
{
	return cl.frame.local.client.maxspeed;
}

/*
=============
pfnCheckParm

=============
*/
static int pfnCheckParm( char *parm, char **ppnext )
{
	static char	str[64];

	if( Sys_GetParmFromCmdLine( parm, str ))
	{
		// get the pointer on cmdline param
		if( ppnext ) *ppnext = str;
		return 1;
	}
	return 0;
}

/*
=============
pfnGetMousePosition

=============
*/
static void pfnGetMousePosition( int *mx, int *my )
{
	POINT	curpos;

	GetCursorPos( &curpos );
	ScreenToClient( host.hWnd, &curpos );

	if( mx ) *mx = curpos.x;
	if( my ) *my = curpos.y;
}

/*
=============
pfnIsNoClipping

=============
*/
int pfnIsNoClipping( void )
{
	cl_entity_t *pl = CL_GetLocalPlayer();

	if( !pl ) return false;

	return pl->curstate.movetype == MOVETYPE_NOCLIP;
}

/*
=============
pfnGetViewModel

=============
*/
static cl_entity_t* pfnGetViewModel( void )
{
	return &clgame.viewent;
}

/*
=============
pfnGetClientTime

=============
*/
static float pfnGetClientTime( void )
{
	return cl.time;
}

/*
=============
pfnCalcShake

=============
*/
void pfnCalcShake( void )
{
	int	i;
	float	fraction, freq;
	float	localAmp;

	if( clgame.shake.time == 0 )
		return;

	if(( cl.time > clgame.shake.time ) || clgame.shake.amplitude <= 0 || clgame.shake.frequency <= 0 )
	{
		Q_memset( &clgame.shake, 0, sizeof( clgame.shake ));
		return;
	}

	if( cl.time > clgame.shake.next_shake )
	{
		// higher frequency means we recalc the extents more often and perturb the display again
		clgame.shake.next_shake = cl.time + ( 1.0f / clgame.shake.frequency );

		// compute random shake extents (the shake will settle down from this)
		for( i = 0; i < 3; i++ )
			clgame.shake.offset[i] = Com_RandomFloat( -clgame.shake.amplitude, clgame.shake.amplitude );
		clgame.shake.angle = Com_RandomFloat( -clgame.shake.amplitude * 0.25f, clgame.shake.amplitude * 0.25f );
	}

	// ramp down amplitude over duration (fraction goes from 1 to 0 linearly with slope 1/duration)
	fraction = ( clgame.shake.time - cl.time ) / clgame.shake.duration;

	// ramp up frequency over duration
	if( fraction )
	{
		freq = ( clgame.shake.frequency / fraction );
	}
	else
	{
		freq = 0;
	}

	// square fraction to approach zero more quickly
	fraction *= fraction;

	// Sine wave that slowly settles to zero
	fraction = fraction * sin( cl.time * freq );
	
	// add to view origin
	VectorScale( clgame.shake.offset, fraction, clgame.shake.applied_offset );

	// add to roll
	clgame.shake.applied_angle = clgame.shake.angle * fraction;

	// drop amplitude a bit, less for higher frequency shakes
	localAmp = clgame.shake.amplitude * ( host.frametime / ( clgame.shake.duration * clgame.shake.frequency ));
	clgame.shake.amplitude -= localAmp;
}

/*
=============
pfnApplyShake

=============
*/
void pfnApplyShake( float *origin, float *angles, float factor )
{
	if( origin ) VectorMA( origin, factor, clgame.shake.applied_offset, origin );
	if( angles ) angles[ROLL] += clgame.shake.applied_angle * factor;
}
	
/*
=============
pfnIsSpectateOnly

=============
*/
static int pfnIsSpectateOnly( void )
{
	cl_entity_t *pPlayer = CL_GetLocalPlayer();
	return pPlayer ? (pPlayer->curstate.spectator != 0) : 0;
}

/*
=============
pfnPointContents

=============
*/
static int pfnPointContents( const float *p, int *truecontents )
{
	int	cont, truecont;

	truecont = cont = CL_TruePointContents( p );
	if( truecontents ) *truecontents = truecont;

	if( cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN )
		cont = CONTENTS_WATER;
	return cont;
}

/*
=============
pfnTraceLine

=============
*/
static pmtrace_t *pfnTraceLine( float *start, float *end, int flags, int usehull, int ignore_pe )
{
	static pmtrace_t	tr;
	int		old_usehull;

	old_usehull = clgame.pmove->usehull;
	clgame.pmove->usehull = usehull;	

	switch( flags )
	{
	case PM_TRACELINE_PHYSENTSONLY:
		tr = PM_PlayerTraceExt( clgame.pmove, start, end, 0, clgame.pmove->numphysent, clgame.pmove->physents, ignore_pe, NULL );
		break;
	case PM_TRACELINE_ANYVISIBLE:
		tr = PM_PlayerTraceExt( clgame.pmove, start, end, 0, clgame.pmove->numvisent, clgame.pmove->visents, ignore_pe, NULL );
		break;
	}

	clgame.pmove->usehull = old_usehull;

	return &tr;
}

static void pfnPlaySoundByNameAtLocation( char *szSound, float volume, float *origin )
{
	int hSound = S_RegisterSound( szSound );
	S_StartSound( origin, 0, CHAN_AUTO, hSound, volume, ATTN_NORM, PITCH_NORM, 0 );
}

/*
=============
pfnPrecacheEvent

=============
*/
static word pfnPrecacheEvent( int type, const char* psz )
{
	return CL_EventIndex( psz );
}

/*
=============
pfnHookEvent

=============
*/
static void pfnHookEvent( const char *filename, pfnEventHook pfn )
{
	char		name[64];
	cl_user_event_t	*ev;
	int		i;

	// ignore blank names
	if( !filename || !*filename )
		return;	

	Q_strncpy( name, filename, sizeof( name ));
	COM_FixSlashes( name );

	// find an empty slot
	for( i = 0; i < MAX_EVENTS; i++ )
	{
		ev = clgame.events[i];		
		if( !ev ) break;

		if( !Q_stricmp( name, ev->name ) && ev->func != NULL )
		{
			MsgDev( D_WARN, "CL_HookEvent: %s already hooked!\n" );
			return;
		}
	}

	CL_RegisterEvent( i, name, pfn );
}

/*
=============
pfnKillEvent

=============
*/
static void pfnKillEvents( int entnum, const char *eventname )
{
	int		i;
	event_state_t	*es;
	event_info_t	*ei;
	int		eventIndex = CL_EventIndex( eventname );

	if( eventIndex < 0 || eventIndex >= MAX_EVENTS )
		return;

	if( entnum < 0 || entnum > clgame.maxEntities )
		return;

	es = &cl.events;

	// find all events with specified index and kill it
	for( i = 0; i < MAX_EVENT_QUEUE; i++ )
	{
		ei = &es->ei[i];

		if( ei->index == eventIndex && ei->entity_index == entnum )
		{
			CL_ResetEvent( ei );
			break;
		}
	}
}

/*
=============
pfnPlaySound

=============
*/
void pfnPlaySound( int ent, float *org, int chan, const char *samp, float vol, float attn, int flags, int pitch )
{
	S_StartSound( org, ent, chan, S_RegisterSound( samp ), vol, attn, pitch, flags );
}

/*
=============
CL_FindModelIndex

=============
*/
int CL_FindModelIndex( const char *m )
{
	int	i;

	if( !m || !m[0] )
		return 0;

	for( i = 1; i < MAX_MODELS && cl.model_precache[i][0]; i++ )
	{
		if( !Q_stricmp( cl.model_precache[i], m ))
			return i;
	}

	if( cls.state == ca_active && Q_strnicmp( m, "models/player/", 14 ))
	{
		// tell user about problem (but don't spam console about playermodel)
		MsgDev( D_NOTE, "CL_ModelIndex: %s not precached\n", m );
	}
	return 0;
}

/*
=============
pfnIsLocal

=============
*/
int pfnIsLocal( int playernum )
{
	if( playernum == cl.playernum )
		return true;
	return false;
}

/*
=============
pfnLocalPlayerDucking

=============
*/
int pfnLocalPlayerDucking( void )
{
	return cl.frame.local.client.bInDuck;
}

/*
=============
pfnLocalPlayerViewheight

=============
*/
void pfnLocalPlayerViewheight( float *view_ofs )
{
	// predicted or smoothed
	if( view_ofs ) VectorCopy( cl.frame.local.client.view_ofs, view_ofs );
}

/*
=============
pfnLocalPlayerBounds

=============
*/
void pfnLocalPlayerBounds( int hull, float *mins, float *maxs )
{
	if( hull >= 0 && hull < 4 )
	{
		if( mins ) VectorCopy( clgame.pmove->player_mins[hull], mins );
		if( maxs ) VectorCopy( clgame.pmove->player_maxs[hull], maxs );
	}
}

/*
=============
pfnIndexFromTrace

=============
*/
int pfnIndexFromTrace( struct pmtrace_s *pTrace )
{
	if( pTrace->ent >= 0 && pTrace->ent < clgame.pmove->numphysent )
	{
		// return cl.entities number
		return clgame.pmove->physents[pTrace->ent].info;
	}
	return -1;
}

/*
=============
pfnGetPhysent

=============
*/
physent_t *pfnGetPhysent( int idx )
{
	if( idx >= 0 && idx < clgame.pmove->numphysent )
	{
		// return physent
		return &clgame.pmove->physents[idx];
	}
	return NULL;
}

/*
=============
pfnSetUpPlayerPrediction

=============
*/
void pfnSetUpPlayerPrediction( int dopred, int bIncludeLocalClient )
{
	// TODO: implement
}

/*
=============
pfnPushPMStates

=============
*/
void pfnPushPMStates( void )
{
	clgame.oldcount = clgame.pmove->numphysent;
}

/*
=============
pfnPopPMStates

=============
*/
void pfnPopPMStates( void )
{
	clgame.pmove->numphysent = clgame.oldcount;
}

/*
=============
pfnSetTraceHull

=============
*/
void CL_SetTraceHull( int hull )
{
	clgame.old_trace_hull = clgame.pmove->usehull;
	clgame.pmove->usehull = bound( 0, hull, 3 );

}

/*
=============
pfnPlayerTrace

=============
*/
void CL_PlayerTrace( float *start, float *end, int traceFlags, int ignore_pe, pmtrace_t *tr )
{
	if( !tr ) return;
	*tr = PM_PlayerTraceExt( clgame.pmove, start, end, traceFlags, clgame.pmove->numphysent, clgame.pmove->physents, ignore_pe, NULL );
	clgame.pmove->usehull = clgame.old_trace_hull;	// restore old trace hull 
}

/*
=============
pfnPlayerTraceExt

=============
*/
void CL_PlayerTraceExt( float *start, float *end, int traceFlags, int (*pfnIgnore)( physent_t *pe ), pmtrace_t *tr )
{
	if( !tr ) return;
	*tr = PM_PlayerTraceExt( clgame.pmove, start, end, traceFlags, clgame.pmove->numphysent, clgame.pmove->physents, -1, pfnIgnore );
	clgame.pmove->usehull = clgame.old_trace_hull;	// restore old trace hull 
}

/*
=============
pfnTraceTexture

=============
*/
static const char *pfnTraceTexture( int ground, float *vstart, float *vend )
{
	physent_t *pe;

	if( ground < 0 || ground >= clgame.pmove->numphysent )
		return NULL; // bad ground

	pe = &clgame.pmove->physents[ground];
	return PM_TraceTexture( pe, vstart, vend );
}
	
/*
=============
pfnStopAllSounds

=============
*/
void pfnStopAllSounds( int ent, int entchannel )
{
	S_StopSound( ent, entchannel, NULL );
}

/*
=============
CL_LoadModel

=============
*/
model_t *CL_LoadModel( const char *modelname, int *index )
{
	int	idx;

	idx = CL_FindModelIndex( modelname );
	if( !idx ) return NULL;
	if( index ) *index = idx;
	
	return Mod_Handle( idx );
}

int CL_AddEntity( int entityType, cl_entity_t *pEnt )
{
	if( !pEnt ) return false;

	// clear effects for all temp entities
	if( !pEnt->index ) pEnt->curstate.effects = 0;

	// let the render reject entity without model
	return CL_AddVisibleEntity( pEnt, entityType );
}

/*
=============
pfnGetGameDirectory

=============
*/
const char *pfnGetGameDirectory( void )
{
	static char	szGetGameDir[MAX_SYSPATH];

	Q_sprintf( szGetGameDir, "%s/%s", host.rootdir, GI->gamedir );
	return szGetGameDir;
}

/*
=============
Key_LookupBinding

=============
*/
const char *Key_LookupBinding( const char *pBinding )
{
	return Key_KeynumToString( Key_GetKey( pBinding ));
}

/*
=============
pfnGetLevelName

=============
*/
static const char *pfnGetLevelName( void )
{
	static char	mapname[64];

	if( cls.state >= ca_connected )
		Q_snprintf( mapname, sizeof( mapname ), "maps/%s.bsp", clgame.mapname );
	else mapname[0] = '\0'; // not in game

	return mapname;
}

/*
=============
pfnGetScreenFade

=============
*/
static void pfnGetScreenFade( struct screenfade_s *fade )
{
	if( fade ) *fade = clgame.fade;
}

/*
=============
pfnSetScreenFade

=============
*/
static void pfnSetScreenFade( struct screenfade_s *fade )
{
	if( fade ) clgame.fade = *fade;
}

/*
=============
pfnLoadMapSprite

=============
*/
model_t *pfnLoadMapSprite( const char *filename )
{
	char	name[64];
	int	i;

	if( !filename || !*filename )
	{
		MsgDev( D_ERROR, "CL_LoadMapSprite: bad name!\n" );
		return NULL;
	}

	Q_strncpy( name, filename, sizeof( name ));
	COM_FixSlashes( name );

	// slot 0 isn't used
	for( i = 1; i < MAX_IMAGES; i++ )
	{
		if( !Q_stricmp( clgame.sprites[i].name, name ))
		{
			// prolonge registration
			clgame.sprites[i].needload = clgame.load_sequence;
			return &clgame.sprites[i];
		}
	}

	// find a free model slot spot
	for( i = 1; i < MAX_IMAGES; i++ )
	{
		if( !clgame.sprites[i].name[0] )
			break; // this is a valid spot
	}

	if( i == MAX_IMAGES ) 
	{
		MsgDev( D_ERROR, "LoadMapSprite: can't load %s, MAX_HSPRITES limit exceeded\n", filename );
		return NULL;
	}

	// load new map sprite
	if( CL_LoadHudSprite( name, &clgame.sprites[i], true, 0 ))
	{
		clgame.sprites[i].needload = clgame.load_sequence;
		return &clgame.sprites[i];
	}
	return NULL;
}

/*
=============
PlayerInfo_ValueForKey

=============
*/
const char *PlayerInfo_ValueForKey( int playerNum, const char *key )
{
	// find the player
	if(( playerNum > cl.maxclients ) || ( playerNum < 1 ))
		return NULL;

	if(( cl.players[playerNum-1].name == NULL ) || (*(cl.players[playerNum-1].name) == 0 ))
		return NULL;

	return Info_ValueForKey( cl.players[playerNum-1].userinfo, key );
}

/*
=============
PlayerInfo_SetValueForKey

=============
*/
void PlayerInfo_SetValueForKey( const char *key, const char *value )
{
	cvar_t	*var;

	var = (cvar_t *)Cvar_FindVar( key );
	if( !var || !(var->flags & CVAR_USERINFO ))
		return;

	Cvar_DirectSet( var, value );
}

/*
=============
pfnGetPlayerUniqueID

=============
*/
qboolean pfnGetPlayerUniqueID( int iPlayer, char playerID[16] )
{
	// TODO: implement

	playerID[0] = '\0';
	return false;
}

/*
=============
pfnGetTrackerIDForPlayer

=============
*/
int pfnGetTrackerIDForPlayer( int playerSlot )
{
	playerSlot -= 1;	// make into a client index

	if( !cl.players[playerSlot].userinfo[0] || !cl.players[playerSlot].name[0] )
			return 0;
	return Q_atoi( Info_ValueForKey( cl.players[playerSlot].userinfo, "*tracker" ));
}

/*
=============
pfnGetPlayerForTrackerID

=============
*/
int pfnGetPlayerForTrackerID( int trackerID )
{
	int	i;

	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		if( !cl.players[i].userinfo[0] || !cl.players[i].name[0] )
			continue;

		if( Q_atoi( Info_ValueForKey( cl.players[i].userinfo, "*tracker" )) == trackerID )
		{
			// make into a player slot
			return (i+1);
		}
	}
	return 0;
}

/*
=============
pfnServerCmdUnreliable

=============
*/
int pfnServerCmdUnreliable( char *szCmdString )
{
	if( !szCmdString || !szCmdString[0] )
		return 0;

	BF_WriteByte( &cls.datagram, clc_stringcmd );
	BF_WriteString( &cls.datagram, szCmdString );

	return 1;
}

/*
=============
pfnGetMousePos

=============
*/
void pfnGetMousePos( struct tagPOINT *ppt )
{
	ASSERT( ppt != NULL );

	// find mouse movement
	GetCursorPos( ppt );
	ScreenToClient( host.hWnd, ppt );
}

/*
=============
pfnSetMousePos

=============
*/
void pfnSetMousePos( int mx, int my )
{
	POINT pt;

	pt.x = mx;
	pt.y = my;

	ClientToScreen( host.hWnd, &pt );
	SetCursorPos( pt.x, pt.y );
}

/*
=============
pfnSetMouseEnable

=============
*/
void pfnSetMouseEnable( qboolean fEnable )
{
	if( fEnable ) IN_ActivateMouse( false );
	else IN_DeactivateMouse();
}

/*
=============
pfnGetServerTime

=============
*/
float pfnGetClientOldTime( void )
{
	return cl.oldtime;
}

/*
=============
pfnGetGravity

=============
*/
float pfnGetGravity( void )
{
	return clgame.movevars.gravity;
}

/*
=============
pfnEnableTexSort

TODO: implement
=============
*/
void pfnEnableTexSort( int enable )
{
}

/*
=============
pfnSetLightmapColor

TODO: implement
=============
*/
void pfnSetLightmapColor( float red, float green, float blue )
{
}

/*
=============
pfnSetLightmapScale

TODO: implement
=============
*/
void pfnSetLightmapScale( float scale )
{
}

/*
=============
pfnSPR_DrawGeneric

=============
*/
void pfnSPR_DrawGeneric( int frame, int x, int y, const wrect_t *prc, int blendsrc, int blenddst, int width, int height )
{
	pglEnable( GL_BLEND );
	pglBlendFunc( blendsrc, blenddst ); // g-cont. are params is valid?
	SPR_DrawGeneric( frame, x, y, width, height, prc );
}

/*
=============
pfnDrawString

TODO: implement
=============
*/
int pfnDrawString( int x, int y, const char *str, int r, int g, int b )
{
	return 0;
}

/*
=============
pfnDrawStringReverse

TODO: implement
=============
*/
int pfnDrawStringReverse( int x, int y, const char *str, int r, int g, int b )
{
	return 0;
}

/*
=============
LocalPlayerInfo_ValueForKey

=============
*/
const char *LocalPlayerInfo_ValueForKey( const char* key )
{
	return Info_ValueForKey( Cvar_Userinfo(), key );
}

/*
=============
pfnVGUI2DrawCharacter

TODO: implement
=============
*/
int pfnVGUI2DrawCharacter( int x, int y, int ch, unsigned int font )
{
	return 0;
}

/*
=============
pfnVGUI2DrawCharacterAdditive

TODO: implement
=============
*/
int pfnVGUI2DrawCharacterAdditive( int x, int y, int ch, int r, int g, int b, unsigned int font )
{
	return 0;
}

/*
=============
GetCareerGameInterface

=============
*/
void *GetCareerGameInterface( void )
{
	Msg( "^1Career GameInterface called!\n" );
	return NULL;
}

/*
=============
pfnPlaySoundVoiceByName

=============
*/
void pfnPlaySoundVoiceByName( char *filename, float volume, int pitch )
{
	int hSound = S_RegisterSound( filename );
	S_StartSound( NULL, cl.refdef.viewentity, CHAN_AUTO, hSound, volume, ATTN_NORM, pitch, SND_STOP_LOOPING );
}

/*
=============
pfnMP3_InitStream

=============
*/
void pfnMP3_InitStream( char *filename, int looping )
{
	if( !filename )
	{
		S_StopBackgroundTrack();
		return;
	}

	if( looping )
	{
		S_StartBackgroundTrack( filename, filename, 0 );
	}
	else
	{
		S_StartBackgroundTrack( filename, NULL, 0 );
	}
}

/*
=============
pfnPlaySoundByNameAtPitch

=============
*/
void pfnPlaySoundByNameAtPitch( char *filename, float volume, int pitch )
{
	int hSound = S_RegisterSound( filename );
	S_StartSound( NULL, cl.refdef.viewentity, CHAN_STATIC, hSound, volume, ATTN_NORM, pitch, SND_STOP_LOOPING );
}

/*
=============
pfnFillRGBABlend

=============
*/
void pfnFillRGBABlend( int x, int y, int width, int height, int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );
	pglColor4ub( r, g, b, a );

	SPR_AdjustSize( (float *)&x, (float *)&y, (float *)&width, (float *)&height );

	pglEnable( GL_BLEND );
	pglDisable( GL_ALPHA_TEST );
	pglBlendFunc( GL_ONE_MINUS_SRC_ALPHA, GL_ONE );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, cls.fillImage );
	pglColor4ub( 255, 255, 255, 255 );
}

/*
=============
pfnGetAppID

=============
*/
int pfnGetAppID( void )
{
	return 220; // standard Valve value
}

/*
=============
pfnVguiWrap2_GetMouseDelta

TODO: implement
=============
*/
void pfnVguiWrap2_GetMouseDelta( int *x, int *y )
{
}

/*
=================
TriApi implementation

=================
*/
/*
=============
TriRenderMode

set rendermode
=============
*/
void TriRenderMode( int mode )
{
	switch( mode )
	{
	case kRenderNormal:
	default:	pglDisable( GL_BLEND );
		pglDisable( GL_ALPHA_TEST );
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		break;
	case kRenderTransColor:
	case kRenderTransAlpha:
	case kRenderTransTexture:
		// NOTE: TriAPI doesn't have 'solid' mode
		pglEnable( GL_BLEND );
		pglDisable( GL_ALPHA_TEST );
		pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		break;
	case kRenderGlow:
	case kRenderTransAdd:
		pglEnable( GL_BLEND );
		pglDisable( GL_ALPHA_TEST );
		pglBlendFunc( GL_SRC_ALPHA, GL_ONE );
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		break;
	}
}

/*
=============
TriBegin

begin triangle sequence
=============
*/
void TriBegin( int mode )
{
	switch( mode )
	{
	case TRI_POINTS:
		mode = GL_POINTS;
		break;
	case TRI_TRIANGLES:
		mode = GL_TRIANGLES;
		break;
	case TRI_TRIANGLE_FAN:
		mode = GL_TRIANGLE_FAN;
		break;
	case TRI_QUADS:
		mode = GL_QUADS;
		break;
	case TRI_LINES:
		mode = GL_LINES;
		break;
	case TRI_TRIANGLE_STRIP:
		mode = GL_TRIANGLE_STRIP;
		break;
	case TRI_QUAD_STRIP:
		mode = GL_QUAD_STRIP;
		break;
	case TRI_POLYGON:
	default:	mode = GL_POLYGON;
		break;
	}

	pglBegin( mode );
}

/*
=============
TriEnd

draw triangle sequence
=============
*/
void TriEnd( void )
{
	pglEnd();
	pglDisable( GL_ALPHA_TEST );
}

/*
=============
TriColor4f

=============
*/
void TriColor4f( float r, float g, float b, float a )
{
	clgame.ds.triColor[0] = (byte)bound( 0, (r * 255.0f), 255 );
	clgame.ds.triColor[1] = (byte)bound( 0, (g * 255.0f), 255 );
	clgame.ds.triColor[2] = (byte)bound( 0, (b * 255.0f), 255 );
	clgame.ds.triColor[3] = (byte)bound( 0, (a * 255.0f), 255 );
	pglColor4ub( clgame.ds.triColor[0], clgame.ds.triColor[1], clgame.ds.triColor[2], clgame.ds.triColor[3] );
}

/*
=============
TriColor4ub

=============
*/
void TriColor4ub( byte r, byte g, byte b, byte a )
{
	clgame.ds.triColor[0] = r;
	clgame.ds.triColor[1] = g;
	clgame.ds.triColor[2] = b;
	clgame.ds.triColor[3] = a;
	pglColor4ub( r, g, b, a );
}

/*
=============
TriTexCoord2f

=============
*/
void TriTexCoord2f( float u, float v )
{
	pglTexCoord2f( u, v );
}

/*
=============
TriVertex3fv

=============
*/
void TriVertex3fv( const float *v )
{
	pglVertex3fv( v );
}

/*
=============
TriVertex3f

=============
*/
void TriVertex3f( float x, float y, float z )
{
	pglVertex3f( x, y, z );
}

/*
=============
TriBrightness

=============
*/
void TriBrightness( float brightness )
{
	rgba_t	rgba;

	brightness = max( 0.0f, brightness );
	rgba[0] = clgame.ds.triColor[0] * brightness;
	rgba[1] = clgame.ds.triColor[1] * brightness;
	rgba[2] = clgame.ds.triColor[2] * brightness;

	pglColor3ub( rgba[0], rgba[1], rgba[2] );
}

/*
=============
TriCullFace

=============
*/
void TriCullFace( TRICULLSTYLE mode )
{
	switch( mode )
	{
	case TRI_FRONT:
		clgame.ds.cullMode = GL_FRONT;
		break;
	default:
		clgame.ds.cullMode = GL_NONE;
		break;
	}
	GL_Cull( clgame.ds.cullMode );
}

/*
=============
TriSpriteTexture

bind current texture
=============
*/
int TriSpriteTexture( model_t *pSpriteModel, int frame )
{
	int	gl_texturenum;
	msprite_t	*psprite;

	if(( gl_texturenum = R_GetSpriteTexture( pSpriteModel, frame )) == 0 )
		return 0;

	if( gl_texturenum <= 0 || gl_texturenum > MAX_TEXTURES )
	{
		MsgDev( D_ERROR, "TriSpriteTexture: bad index %i\n", gl_texturenum );
		gl_texturenum = tr.defaultTexture;
	}

	psprite = pSpriteModel->cache.data;
	if( psprite->texFormat == SPR_ALPHTEST )
	{
		pglEnable( GL_ALPHA_TEST );
		pglAlphaFunc( GL_GREATER, 0.0f );
	}

	GL_Bind( GL_TEXTURE0, gl_texturenum );

	return 1;
}

/*
=============
TriWorldToScreen

convert world coordinates (x,y,z) into screen (x, y)
=============
*/
int TriWorldToScreen( float *world, float *screen )
{
	int	retval;

	retval = R_WorldToScreen( world, screen );

	screen[0] =  0.5f * screen[0] * (float)cl.refdef.viewport[2];
	screen[1] = -0.5f * screen[1] * (float)cl.refdef.viewport[3];
	screen[0] += 0.5f * (float)cl.refdef.viewport[2];
	screen[1] += 0.5f * (float)cl.refdef.viewport[3];

	return retval;
}

/*
=============
TriFog

enables global fog on the level
=============
*/
void TriFog( float flFogColor[3], float flStart, float flEnd, int bOn )
{
	if( RI.fogEnabled ) return;
	RI.fogCustom = true;

	if( !bOn )
	{
		pglDisable( GL_FOG );
		RI.fogCustom = false;
		return;
	}

	// copy fog params
	RI.fogColor[0] = flFogColor[0] / 255.0f;
	RI.fogColor[1] = flFogColor[1] / 255.0f;
	RI.fogColor[2] = flFogColor[2] / 255.0f;
	RI.fogStart = flStart;
	RI.fogDensity = 0.0f;
	RI.fogEnd = flEnd;

	if( VectorIsNull( RI.fogColor ))
	{
		pglDisable( GL_FOG );
		return;	
	}

	pglEnable( GL_FOG );
	pglFogi( GL_FOG_MODE, GL_LINEAR );
	pglFogf( GL_FOG_START, RI.fogStart );
	pglFogf( GL_FOG_END, RI.fogEnd );
	pglFogfv( GL_FOG_COLOR, RI.fogColor );
	pglHint( GL_FOG_HINT, GL_NICEST );
}

/*
=============
TriGetMatrix

very strange export
=============
*/
void TriGetMatrix( const int pname, float *matrix )
{
	pglGetFloatv( pname, matrix );
}

/*
=============
TriBoxInPVS

check box in pvs (absmin, absmax)
=============
*/
int TriBoxInPVS( float *mins, float *maxs )
{
	return Mod_BoxVisible( mins, maxs, Mod_GetCurrentVis( ));
}

/*
=============
TriLightAtPoint

NOTE: dlights are ignored
=============
*/
void TriLightAtPoint( float *pos, float *value )
{
	color24	ambient;

	if( !pos || !value )
		return;

	R_LightForPoint( pos, &ambient, false, false, 0.0f );

	value[0] = (float)ambient.r * 255.0f;
	value[1] = (float)ambient.g * 255.0f;
	value[2] = (float)ambient.b * 255.0f;
}

/*
=============
TriColor4fRendermode

Heavy legacy of Quake...
=============
*/
void TriColor4fRendermode( float r, float g, float b, float a, int rendermode )
{
	if( rendermode == kRenderTransAlpha )
		pglColor4f( r, g, b, a );
	else pglColor4f( r * a, g * a, b * a, 1.0f );
}

/*
=============
TriForParams

=============
*/
void TriFogParams( float flDensity, int iFogSkybox )
{
	// TODO: implement
}

/*
=================
DemoApi implementation

=================
*/
/*
=================
Demo_IsRecording

=================
*/
static int Demo_IsRecording( void )
{
	return cls.demorecording;
}

/*
=================
Demo_IsPlayingback

=================
*/
static int Demo_IsPlayingback( void )
{
	return cls.demoplayback;
}

/*
=================
Demo_IsTimeDemo

=================
*/
static int Demo_IsTimeDemo( void )
{
	return cls.timedemo;
}

/*
=================
Demo_WriteBuffer

=================
*/
static void Demo_WriteBuffer( int size, byte *buffer )
{
	CL_WriteDemoUserMessage( buffer, size );
}

/*
=================
NetworkApi implementation

=================
*/
/*
=================
NetAPI_InitNetworking

=================
*/
void NetAPI_InitNetworking( void )
{
	NET_Config( true ); // allow remote
}

/*
=================
NetAPI_InitNetworking

=================
*/
void NetAPI_Status( net_status_t *status )
{
	ASSERT( status != NULL );

	status->connected = NET_IsLocalAddress( cls.netchan.remote_address ) ? false : true;
	status->connection_time = host.realtime - cls.netchan.connect_time;
	status->remote_address = cls.netchan.remote_address;
	status->packet_loss = cls.packet_loss / 100; // percent
	status->latency = cl.frame.latency;
	status->local_address = net_local;
	status->rate = cls.netchan.rate;
}

/*
=================
NetAPI_SendRequest

=================
*/
void NetAPI_SendRequest( int context, int request, int flags, double timeout, netadr_t *remote_address, net_api_response_func_t response )
{
	net_request_t	*nr = NULL;
	string		req;
	int		i;

	if( !response )
	{
		MsgDev( D_ERROR, "Net_SendRequest: no callbcak specified for request with context %i!\n", context );
		return;
	}

	// find a free request
	for( i = 0; i < MAX_REQUESTS; i++ )
	{
		nr = &clgame.net_requests[i];
		if( !nr->pfnFunc || nr->timeout < host.realtime )
			break;
	}

	if( i == MAX_REQUESTS )
	{
		double	max_timeout = 0;

		// no free requests? use older
		for( i = 0, nr = NULL; i < MAX_REQUESTS; i++ )
		{
			if(( host.realtime - clgame.net_requests[i].timesend ) > max_timeout )
			{
				max_timeout = host.realtime - clgame.net_requests[i].timesend;
				nr = &clgame.net_requests[i];
			}
		}
	}

	ASSERT( nr != NULL );

	// clear slot
	Q_memset( nr, 0, sizeof( *nr ));

	// create a new request
	nr->timesend = host.realtime;
	nr->timeout = nr->timesend + timeout;
	nr->pfnFunc = response;
	nr->resp.context = context;
	nr->resp.type = request;	
	nr->resp.remote_address = *remote_address; 
	nr->flags = flags;

	if( request == NETAPI_REQUEST_SERVERLIST )
	{
		// UNDONE: build request for master-server
	}
	else
	{
		// send request over the net
		Q_snprintf( req, sizeof( req ), "netinfo %i %i %i", PROTOCOL_VERSION, context, request );
		Netchan_OutOfBandPrint( NS_CLIENT, nr->resp.remote_address, req );
	}
}

/*
=================
NetAPI_CancelRequest

=================
*/
void NetAPI_CancelRequest( int context )
{
	int	i;

	// find a specified request
	for( i = 0; i < MAX_REQUESTS; i++ )
	{
		if( clgame.net_requests[i].resp.context == context )
		{
			MsgDev( D_NOTE, "Request with context %i cancelled\n", context );
			Q_memset( &clgame.net_requests[i], 0, sizeof( net_request_t ));
			break;
		}
	}
}

/*
=================
NetAPI_CancelAllRequests

=================
*/
void NetAPI_CancelAllRequests( void )
{
	Q_memset( clgame.net_requests, 0, sizeof( clgame.net_requests ));
}

/*
=================
NetAPI_AdrToString

=================
*/
char *NetAPI_AdrToString( netadr_t *a )
{
	return NET_AdrToString( *a );
}

/*
=================
NetAPI_CompareAdr

=================
*/
int NetAPI_CompareAdr( netadr_t *a, netadr_t *b )
{
	return NET_CompareAdr( *a, *b );
}

/*
=================
NetAPI_StringToAdr

=================
*/
int NetAPI_StringToAdr( char *s, netadr_t *a )
{
	return NET_StringToAdr( s, a );
}

/*
=================
NetAPI_ValueForKey

=================
*/
const char *NetAPI_ValueForKey( const char *s, const char *key )
{
	return Info_ValueForKey( s, key );
}

/*
=================
NetAPI_RemoveKey

=================
*/
void NetAPI_RemoveKey( char *s, const char *key )
{
	Info_RemoveKey( s, key );
}

/*
=================
NetAPI_SetValueForKey

=================
*/
void NetAPI_SetValueForKey( char *s, const char *key, const char *value, int maxsize )
{
	if( key[0] == '*' ) return;
	Info_SetValueForStarKey( s, key, value, maxsize );
}


/*
=================
IVoiceTweak implementation

=================
*/
/*
=================
Voice_StartVoiceTweakMode

=================
*/
int Voice_StartVoiceTweakMode( void )
{
	// TODO: implement
	return 0;
}

/*
=================
Voice_EndVoiceTweakMode

=================
*/
void Voice_EndVoiceTweakMode( void )
{
	// TODO: implement
}

/*
=================
Voice_SetControlFloat

=================
*/	
void Voice_SetControlFloat( VoiceTweakControl iControl, float value )
{
	// TODO: implement
}

/*
=================
Voice_GetControlFloat

=================
*/
float Voice_GetControlFloat( VoiceTweakControl iControl )
{
	// TODO: implement
	return 1.0f;
}

/*
=================
Voice_GetSpeakingVolume

=================
*/
int Voice_GetSpeakingVolume( void )
{
	// TODO: implement
	return 255;
}

// shared between client and server			
triangleapi_t gTriApi =
{
	TRI_API_VERSION,	
	TriRenderMode,
	TriBegin,
	TriEnd,
	TriColor4f,
	TriColor4ub,
	TriTexCoord2f,
	TriVertex3fv,
	TriVertex3f,
	TriBrightness,
	TriCullFace,
	TriSpriteTexture,
	R_WorldToScreen,	// NOTE: XPROJECT, YPROJECT should be done in client.dll
	TriFog,
	R_ScreenToWorld,
	TriGetMatrix,
	TriBoxInPVS,
	TriLightAtPoint,
	TriColor4fRendermode,
	TriFogParams,
};

static efx_api_t gEfxApi =
{
	CL_AllocParticle,
	CL_BlobExplosion,
	CL_Blood,
	CL_BloodSprite,
	CL_BloodStream,
	CL_BreakModel,
	CL_Bubbles,
	CL_BubbleTrail,
	CL_BulletImpactParticles,
	CL_EntityParticles,
	CL_Explosion,
	CL_FizzEffect,
	CL_FireField,
	CL_FlickerParticles,
	CL_FunnelSprite,
	CL_Implosion,
	CL_Large_Funnel,
	CL_LavaSplash,
	CL_MultiGunshot,
	CL_MuzzleFlash,
	CL_ParticleBox,
	CL_ParticleBurst,
	CL_ParticleExplosion,
	CL_ParticleExplosion2,
	CL_ParticleLine,
	CL_PlayerSprites,
	CL_Projectile,
	CL_RicochetSound,
	CL_RicochetSprite,
	CL_RocketFlare,
	CL_RocketTrail,
	CL_RunParticleEffect,
	CL_ShowLine,
	CL_SparkEffect,
	CL_SparkShower,
	CL_SparkStreaks,
	CL_Spray,
	CL_Sprite_Explode,
	CL_Sprite_Smoke,
	CL_Sprite_Spray,
	CL_Sprite_Trail,
	CL_Sprite_WallPuff,
	CL_StreakSplash,
	CL_TracerEffect,
	CL_UserTracerParticle,
	CL_TracerParticles,
	CL_TeleportSplash,
	CL_TempSphereModel,
	CL_TempModel,
	CL_DefaultSprite,
	CL_TempSprite,
	CL_DecalIndex,
	CL_DecalIndexFromName,
	CL_DecalShoot,
	CL_AttachTentToPlayer,
	CL_KillAttachedTents,
	CL_BeamCirclePoints,
	CL_BeamEntPoint,
	CL_BeamEnts,
	CL_BeamFollow,
	CL_BeamKill,
	CL_BeamLightning,
	CL_BeamPoints,
	CL_BeamRing,
	CL_AllocDlight,
	CL_AllocElight,
	CL_TempEntAlloc,
	CL_TempEntAllocNoModel,
	CL_TempEntAllocHigh,
	CL_TempEntAllocCustom,
	CL_GetPackedColor,
	CL_LookupColor,
	CL_DecalRemoveAll,
	CL_FireCustomDecal,
};

static event_api_t gEventApi =
{
	EVENT_API_VERSION,
	pfnPlaySound,
	S_StopSound,
	CL_FindModelIndex,
	pfnIsLocal,
	pfnLocalPlayerDucking,
	pfnLocalPlayerViewheight,
	pfnLocalPlayerBounds,
	pfnIndexFromTrace,
	pfnGetPhysent,
	pfnSetUpPlayerPrediction,
	pfnPushPMStates,
	pfnPopPMStates,
	CL_SetSolidPlayers,
	CL_SetTraceHull,
	CL_PlayerTrace,
	CL_WeaponAnim,
	pfnPrecacheEvent,
	CL_PlaybackEvent,
	pfnTraceTexture,
	pfnStopAllSounds,
	pfnKillEvents,
	CL_EventIndex,
	CL_IndexEvent,
	CL_PlayerTraceExt,
	CL_SoundFromIndex,
};

static demo_api_t gDemoApi =
{
	Demo_IsRecording,
	Demo_IsPlayingback,
	Demo_IsTimeDemo,
	Demo_WriteBuffer,
};

static net_api_t gNetApi =
{
	NetAPI_InitNetworking,
	NetAPI_Status,
	NetAPI_SendRequest,
	NetAPI_CancelRequest,
	NetAPI_CancelAllRequests,
	NetAPI_AdrToString,
	NetAPI_CompareAdr,
	NetAPI_StringToAdr,
	NetAPI_ValueForKey,
	NetAPI_RemoveKey,
	NetAPI_SetValueForKey,
};

static IVoiceTweak gVoiceApi =
{
	Voice_StartVoiceTweakMode,
	Voice_EndVoiceTweakMode,
	Voice_SetControlFloat,
	Voice_GetControlFloat,
	Voice_GetSpeakingVolume,
};

// engine callbacks
static cl_enginefunc_t gEngfuncs = 
{
	pfnSPR_Load,
	pfnSPR_Frames,
	pfnSPR_Height,
	pfnSPR_Width,
	pfnSPR_Set,
	pfnSPR_Draw,
	pfnSPR_DrawHoles,
	pfnSPR_DrawAdditive,
	SPR_EnableScissor,
	SPR_DisableScissor,
	pfnSPR_GetList,
	pfnFillRGBA,
	pfnGetScreenInfo,
	pfnSetCrosshair,
	pfnCvar_RegisterVariable,
	Cvar_VariableValue,
	Cvar_VariableString,
	pfnAddClientCommand,
	pfnHookUserMsg,
	pfnServerCmd,
	pfnClientCmd,
	pfnGetPlayerInfo,
	pfnPlaySoundByName,
	pfnPlaySoundByIndex,
	AngleVectors,
	CL_TextMessageGet,
	pfnDrawCharacter,
	pfnDrawConsoleString,
	pfnDrawSetTextColor,
	pfnDrawConsoleStringLen,
	pfnConsolePrint,
	pfnCenterPrint,
	pfnGetWindowCenterX,
	pfnGetWindowCenterY,
	pfnGetViewAngles,
	pfnSetViewAngles,
	CL_GetMaxClients,
	Cvar_SetFloat,
	Cmd_Argc,
	Cmd_Argv,
	Con_Printf,
	Con_DPrintf,
	Con_NPrintf,
	Con_NXPrintf,
	pfnPhysInfo_ValueForKey,
	pfnServerInfo_ValueForKey,
	pfnGetClientMaxspeed,
	pfnCheckParm,
	Key_Event,
	pfnGetMousePosition,
	pfnIsNoClipping,
	CL_GetLocalPlayer,
	pfnGetViewModel,
	CL_GetEntityByIndex,
	pfnGetClientTime,
	pfnCalcShake,
	pfnApplyShake,
	pfnPointContents,
	CL_WaterEntity,
	pfnTraceLine,
	CL_LoadModel,
	CL_AddEntity,
	CL_GetSpritePointer,
	pfnPlaySoundByNameAtLocation,
	pfnPrecacheEvent,
	CL_PlaybackEvent,
	CL_WeaponAnim,
	Com_RandomFloat,
	Com_RandomLong,
	pfnHookEvent,
	Con_Visible,
	pfnGetGameDirectory,
	pfnCVarGetPointer,
	Key_LookupBinding,
	pfnGetLevelName,
	pfnGetScreenFade,
	pfnSetScreenFade,
	VGui_GetPanel,
	VGui_ViewportPaintBackground,
	COM_LoadFile,
	COM_ParseFile,
	COM_FreeFile,
	&gTriApi,
	&gEfxApi,
	&gEventApi,
	&gDemoApi,
	&gNetApi,
	&gVoiceApi,
	pfnIsSpectateOnly,
	pfnLoadMapSprite,
	COM_AddAppDirectoryToSearchPath,
	COM_ExpandFilename,
	PlayerInfo_ValueForKey,
	PlayerInfo_SetValueForKey,
	pfnGetPlayerUniqueID,
	pfnGetTrackerIDForPlayer,
	pfnGetPlayerForTrackerID,
	pfnServerCmdUnreliable,
	pfnGetMousePos,
	pfnSetMousePos,
	pfnSetMouseEnable,
	Cvar_GetList,
	Cmd_GetFirstFunctionHandle,
	Cmd_GetNextFunctionHandle,
	Cmd_GetName,
	pfnGetClientOldTime,
	pfnGetGravity,
	Mod_Handle,
	pfnEnableTexSort,
	pfnSetLightmapColor,
	pfnSetLightmapScale,
	pfnSequenceGet,
	pfnSPR_DrawGeneric,
	pfnSequencePickSentence,
	pfnDrawString,
	pfnDrawStringReverse,
	LocalPlayerInfo_ValueForKey,
	pfnVGUI2DrawCharacter,
	pfnVGUI2DrawCharacterAdditive,
	Sound_GetApproxWavePlayLen,
	GetCareerGameInterface,
	Cvar_Set,
	pfnIsCareerMatch,
	pfnPlaySoundVoiceByName,
	pfnMP3_InitStream,
	Sys_DoubleTime,
	pfnProcessTutorMessageDecayBuffer,
	pfnConstructTutorMessageDecayBuffer,
	pfnResetTutorMessageDecayData,
	pfnPlaySoundByNameAtPitch,
	pfnFillRGBABlend,
	pfnGetAppID,
	Cmd_AliasGetList,
	pfnVguiWrap2_GetMouseDelta,
};

void CL_UnloadProgs( void )
{
	if( !clgame.hInstance ) return;

	CL_FreeEdicts();
	CL_FreeTempEnts();
	CL_FreeViewBeams();
	CL_FreeParticles();
	CL_ClearAllRemaps();
	Mod_ClearUserData();
	VGui_Shutdown();

	// NOTE: HLFX 0.5 has strange bug: hanging on exit if no map was loaded
	if( !( !Q_stricmp( GI->gamedir, "hlfx" ) && GI->version == 0.5f ))
		clgame.dllFuncs.pfnShutdown();

	Cvar_FullSet( "cl_background", "0", CVAR_READ_ONLY );
	Cvar_FullSet( "host_clientloaded", "0", CVAR_INIT );

	Com_FreeLibrary( clgame.hInstance );
	Mem_FreePool( &cls.mempool );
	Mem_FreePool( &clgame.mempool );
	Q_memset( &clgame, 0, sizeof( clgame ));

	Cvar_Unlink();
	Cmd_Unlink( CMD_CLIENTDLL );
}

qboolean CL_LoadProgs( const char *name )
{
	static playermove_t		gpMove;
	const dllfunc_t		*func;
	CL_EXPORT_FUNCS		F; // export 'F'
	qboolean			critical_exports = true;

	if( clgame.hInstance ) CL_UnloadProgs();

	// setup globals
	cl.refdef.movevars = &clgame.movevars;

	// initialize PlayerMove
	clgame.pmove = &gpMove;

	cls.mempool = Mem_AllocPool( "Client Static Pool" );
	clgame.mempool = Mem_AllocPool( "Client Edicts Zone" );
	clgame.entities = NULL;

	// NOTE: important stuff!
	// vgui must startup BEFORE loading client.dll to avoid get error ERROR_NOACESS
	// during LoadLibrary
	VGui_Startup ();
	
	clgame.hInstance = Com_LoadLibrary( name, false );
	if( !clgame.hInstance ) return false;

	// clear exports
	for( func = cdll_exports; func && func->name; func++ )
		*func->func = NULL;

	// trying to get single export named 'F'
	if(( F = (void *)Com_GetProcAddress( clgame.hInstance, "F" )) != NULL )
	{
		MsgDev( D_NOTE, "CL_LoadProgs: found single callback export\n" );		

		// trying to fill interface now
		F( &clgame.dllFuncs );

		// check critical functions again
		for( func = cdll_exports; func && func->name; func++ )
		{
			if( func->func == NULL )
				break; // BAH critical function was missed
		}

		// because all the exports are loaded through function 'F"
		if( !func || !func->name )
			critical_exports = false;
	}

	for( func = cdll_exports; func && func->name != NULL; func++ )
	{
		if( *func->func != NULL )
			continue;	// already get through 'F'

		// functions are cleared before all the extensions are evaluated
		if(!( *func->func = (void *)Com_GetProcAddress( clgame.hInstance, func->name )))
		{
          		MsgDev( D_NOTE, "CL_LoadProgs: failed to get address of %s proc\n", func->name );

			if( critical_exports )
			{
				Com_FreeLibrary( clgame.hInstance );
				clgame.hInstance = NULL;
				return false;
			}
		}
	}

	// it may be loaded through 'F' so we don't need to clear them
	if( critical_exports )
	{
		// clear new exports
		for( func = cdll_new_exports; func && func->name; func++ )
			*func->func = NULL;
	}

	for( func = cdll_new_exports; func && func->name != NULL; func++ )
	{
		if( *func->func != NULL )
			continue;	// already get through 'F'

		// functions are cleared before all the extensions are evaluated
		// NOTE: new exports can be missed without stop the engine
		if(!( *func->func = (void *)Com_GetProcAddress( clgame.hInstance, func->name )))
          		MsgDev( D_NOTE, "CL_LoadProgs: failed to get address of %s proc\n", func->name );
	}

	if( !clgame.dllFuncs.pfnInitialize( &gEngfuncs, CLDLL_INTERFACE_VERSION ))
	{
		Com_FreeLibrary( clgame.hInstance );
		MsgDev( D_NOTE, "CL_LoadProgs: can't init client API\n" );
		clgame.hInstance = NULL;
		return false;
	}

	Cvar_Get( "cl_nopred", "1", CVAR_ARCHIVE|CVAR_USERINFO, "disable client movement predicting" );
	Cvar_Get( "cl_lw", "0", CVAR_ARCHIVE|CVAR_USERINFO, "enable client weapon predicting" );
	Cvar_Get( "cl_lc", "0", CVAR_ARCHIVE|CVAR_USERINFO, "enable lag compensation" );
	Cvar_FullSet( "host_clientloaded", "1", CVAR_INIT );

	clgame.maxRemapInfos = 0; // will be alloc on first call CL_InitEdicts();
	clgame.maxEntities = 2; // world + localclient (have valid entities not in game)

	CL_InitCDAudio( "media/cdaudio.txt" );
	CL_InitTitles( "titles.txt" );
	CL_InitParticles ();
	CL_InitViewBeams ();
	CL_InitTempEnts ();
	CL_InitEdicts ();	// initailize local player and world
	CL_InitClientMove(); // initialize pm_shared

	if( !R_InitRenderAPI())	// Xash3D extension
	{
		MsgDev( D_WARN, "CL_LoadProgs: couldn't get render API\n" );
	}

	// initialize game
	clgame.dllFuncs.pfnInit();

	CL_InitStudioAPI( );

	return true;
}