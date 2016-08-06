/*
enginecallback.h - actual engine callbacks
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

#ifndef ENGINECALLBACKS_H
#define ENGINECALLBACKS_H

// built-in memory manager
#define MALLOC( x )		(*g_engfuncs.pfnMemAlloc)( x, __FILE__, __LINE__ )
#define CALLOC( x, y )	(*g_engfuncs.pfnMemAlloc)((x) * (y), __FILE__, __LINE__ )
#define FREE( x )		(*g_engfuncs.pfnMemFree)( x, __FILE__, __LINE__ )

// screen handlers
#define PIC_Width		(*g_engfuncs.pfnPIC_Width)
#define PIC_Height		(*g_engfuncs.pfnPIC_Height)
#define PIC_EnableScissor	(*g_engfuncs.pfnPIC_EnableScissor)
#define PIC_DisableScissor	(*g_engfuncs.pfnPIC_DisableScissor)
#define FillRGBA		(*g_engfuncs.pfnFillRGBA)
#define GetScreenInfo	(*g_engfuncs.pfnGetScreenInfo)
#define GetGameInfo		(*g_engfuncs.pfnGetGameInfo)
#define CheckGameDll	(*g_engfuncs.pfnCheckGameDll)

#define PIC_SetGamma( x, y )	(*g_engfuncs.pfnProcessImage)( x, y, -1, -1 )
#define PIC_Remap( x, y, z )	(*g_engfuncs.pfnProcessImage)( x, -1.0f, y, z )

#define DRAW_LOGO		(*g_engfuncs.pfnDrawLogo)
#define PRECACHE_LOGO( x )	(*g_engfuncs.pfnDrawLogo)( x, 0, 0, 0, 0 )
#define GetLogoWidth	(*g_engfuncs.pfnGetLogoWidth)
#define GetLogoHeight	(*g_engfuncs.pfnGetLogoHeight)
#define GetLogoLength	(*g_engfuncs.pfnGetLogoLength)

inline HIMAGE PIC_Load( const char *szPicName, long flags = 0 )
{
	return g_engfuncs.pfnPIC_Load( szPicName, NULL, 0, flags );
}

inline HIMAGE PIC_Load( const char *szPicName, const byte *ucRawImage, long ulRawImageSize, long flags = 0 )
{
	return g_engfuncs.pfnPIC_Load( szPicName, ucRawImage, ulRawImageSize, flags );
}

#define PIC_Free		(*g_engfuncs.pfnPIC_Free)
#define PLAY_SOUND		(*g_engfuncs.pfnPlayLocalSound)
#define CVAR_REGISTER	(*g_engfuncs.pfnRegisterVariable)
#define CVAR_SET_FLOAT	(*g_engfuncs.pfnCvarSetValue)
#define CVAR_GET_FLOAT	(*g_engfuncs.pfnGetCvarFloat)
#define CVAR_GET_STRING	(*g_engfuncs.pfnGetCvarString)
#define CVAR_SET_STRING	(*g_engfuncs.pfnCvarSetString)
#define CLIENT_COMMAND	(*g_engfuncs.pfnClientCmd)
#define CLIENT_JOIN		(*g_engfuncs.pfnClientJoin)

#define GET_MENU_EDICT	(*g_engfuncs.pfnGetPlayerModel)
#define ENGINE_SET_MODEL	(*g_engfuncs.pfnSetModel)
#define R_ClearScene	(*g_engfuncs.pfnClearScene)
#define R_AddEntity		(*g_engfuncs.CL_CreateVisibleEntity)
#define R_RenderFrame	(*g_engfuncs.pfnRenderScene)

#define LOAD_FILE		(*g_engfuncs.COM_LoadFile)
#define FILE_EXISTS( file )	(*g_engfuncs.pfnFileExists)( file, FALSE )
#define FREE_FILE		(*g_engfuncs.COM_FreeFile)
#define GET_GAME_DIR	(*g_engfuncs.pfnGetGameDir)
#define HOST_ERROR		(*g_engfuncs.pfnHostError)
#define COM_ParseFile	(*g_engfuncs.COM_ParseFile)
#define KEY_SetDest		(*g_engfuncs.pfnSetKeyDest)
#define KEY_ClearStates	(*g_engfuncs.pfnKeyClearStates)
#define KEY_KeynumToString	(*g_engfuncs.pfnKeynumToString)
#define KEY_GetBinding	(*g_engfuncs.pfnKeyGetBinding)
#define KEY_SetBinding	(*g_engfuncs.pfnKeySetBinding)
#define KEY_IsDown		(*g_engfuncs.pfnKeyIsDown)
#define KEY_GetOverstrike	(*g_engfuncs.pfnKeyGetOverstrikeMode)
#define KEY_SetOverstrike	(*g_engfuncs.pfnKeySetOverstrikeMode)
#define Key_GetState	(*g_engfuncs.pfnKeyGetState)
#define SET_CURSOR		(*g_engfuncs.pfnSetCursor)
	
#define Cmd_AddCommand	(*g_engfuncs.pfnAddCommand)
#define Cmd_RemoveCommand	(*g_engfuncs.pfnDelCommand)
#define CMD_ARGC		(*g_engfuncs.pfnCmdArgc)
#define CMD_ARGV		(*g_engfuncs.pfnCmdArgv)
#define Con_Printf		(*g_engfuncs.Con_Printf)
#define Con_NPrintf		(*g_engfuncs.Con_NPrintf)

#define GET_GAMES_LIST	(*g_engfuncs.pfnGetGamesList)
#define BACKGROUND_TRACK	(*g_engfuncs.pfnPlayBackgroundTrack)
#define SHELL_EXECUTE	(*g_engfuncs.pfnShellExecute)
#define HOST_WRITECONFIG	(*g_engfuncs.pfnWriteServerConfig)
#define HOST_CHANGEGAME	(*g_engfuncs.pfnChangeInstance)
#define CHECK_MAP_LIST	(*g_engfuncs.pfnCreateMapsList)
#define HOST_ENDGAME	(*g_engfuncs.pfnHostEndGame)
#define GET_CLIPBOARD	(*g_engfuncs.pfnGetClipboardData)
#define FS_SEARCH		(*g_engfuncs.pfnGetFilesList)
#define MAP_IS_VALID	(*g_engfuncs.pfnIsMapValid)

#define GET_SAVE_COMMENT	(*g_engfuncs.pfnGetSaveComment)
#define GET_DEMO_COMMENT	(*g_engfuncs.pfnGetDemoComment)

#define CL_IsActive()	(g_engfuncs.pfnClientInGame() && !CVAR_GET_FLOAT( "cl_background" ))

inline void PIC_Set( HIMAGE hPic, int r, int g, int b )
{
	g_engfuncs.pfnPIC_Set( hPic, r, g, b, 255 );
}

inline void PIC_Set( HIMAGE hPic, int r, int g, int b, int a )
{
	g_engfuncs.pfnPIC_Set( hPic, r, g, b, a );
}

inline void PIC_Draw( int x, int y, const wrect_t *prc )
{
	g_engfuncs.pfnPIC_Draw( x, y, -1, -1, prc );
}

inline void PIC_Draw( int x, int y, int width, int height )
{
	g_engfuncs.pfnPIC_Draw( x, y, width, height, NULL );
}

inline void PIC_Draw( int x, int y, int width, int height, const wrect_t *prc )
{
	g_engfuncs.pfnPIC_Draw( x, y, width, height, prc );
}

inline void PIC_DrawTrans( int x, int y, const wrect_t *prc )
{
	g_engfuncs.pfnPIC_DrawTrans( x, y, -1, -1, prc );
}

inline void PIC_DrawTrans( int x, int y, int width, int height )
{
	g_engfuncs.pfnPIC_DrawTrans( x, y, width, height, NULL );
}

inline void PIC_DrawTrans( int x, int y, int width, int height, const wrect_t *prc )
{
	g_engfuncs.pfnPIC_DrawTrans( x, y, width, height, prc );
}

inline void PIC_DrawHoles( int x, int y, const wrect_t *prc )
{
	g_engfuncs.pfnPIC_DrawHoles( x, y, -1, -1, prc );
}

inline void PIC_DrawHoles( int x, int y, int width, int height )
{
	g_engfuncs.pfnPIC_DrawHoles( x, y, width, height, NULL );
}

inline void PIC_DrawHoles( int x, int y, int width, int height, const wrect_t *prc )
{
	g_engfuncs.pfnPIC_DrawHoles( x, y, width, height, prc );
}

inline void PIC_DrawAdditive( int x, int y, int width, int height )
{
	g_engfuncs.pfnPIC_DrawAdditive( x, y, width, height, NULL );
}

inline void PIC_DrawAdditive( int x, int y, const wrect_t *prc )
{
	g_engfuncs.pfnPIC_DrawAdditive( x, y, -1, -1, prc );
}

inline void PIC_DrawAdditive( int x, int y, int w, int h, const wrect_t *prc )
{
	g_engfuncs.pfnPIC_DrawAdditive( x, y, w, h, prc );
}

inline void TextMessageSetColor( int r, int g, int b, int alpha = 255 )
{
	g_engfuncs.pfnDrawSetTextColor( r, g, b, alpha );
}

#define TextMessageDrawChar	(*g_engfuncs.pfnDrawCharacter)
#define DrawConsoleString	(*g_engfuncs.pfnDrawConsoleString)
#define GetConsoleStringSize	(*g_engfuncs.pfnDrawConsoleStringLen)
#define ConsoleSetColor	(*g_engfuncs.pfnSetConsoleDefaultColor)

#define RANDOM_LONG		(*g_engfuncs.pfnRandomLong)
#define RANDOM_FLOAT	(*g_engfuncs.pfnRandomFloat)

#define COMPARE_FILE_TIME	(*g_engfuncs.pfnCompareFileTime)

//#define UtfProcessChar		(*g_textfuncs.pfnUtfProcessChar)
//#define UtfMoveLeft		(*g_textfuncs.pfnUtfMoveLeft)
//#define UtfMoveRight		(*g_textfuncs.pfnUtfMoveRight)
//#define EnableTextInput		(*g_textfuncs.pfnEnableTextInput)
inline void EnableTextInput( int enable )
{
	if( g_textfuncs.pfnEnableTextInput )
		g_textfuncs.pfnEnableTextInput( enable );
}
inline int UtfMoveRight( char *str, int pos, int length )
{
	if( g_textfuncs.pfnUtfMoveRight )
		return g_textfuncs.pfnUtfMoveRight( str, pos, length );
	return pos + 1;
}

inline int UtfMoveLeft( char *str, int pos )
{
	if( g_textfuncs.pfnUtfMoveLeft )
		return g_textfuncs.pfnUtfMoveLeft( str, pos );
	return pos - 1;
}
inline int UtfProcessChar( int in )
{
	if( g_textfuncs.pfnUtfProcessChar )
		return g_textfuncs.pfnUtfProcessChar( in );
	return in;
}

#endif//ENGINECALLBACKS_H
