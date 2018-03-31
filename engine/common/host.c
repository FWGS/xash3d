/*
host.c - dedicated and normal host
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

#include "common.h"

#if defined(XASH_SDL)
#include <SDL.h>
#endif

#include <stdarg.h>  // va_args
#include <errno.h> // errno
#include <string.h> // strerror
#include <time.h>

#ifndef _WIN32
#include <unistd.h> // fork
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "netchan.h"
#include "server.h"
#include "protocol.h"
#include "mod_local.h"
#include "mathlib.h"
#include "input.h"
#include "touch.h"
#include "engine_features.h"
#include "render_api.h"	// decallist_t
#include "library.h"
#ifdef XASH_SDL
#include "platform/sdl/events.h"
#endif

typedef void (*pfnChangeGame)( const char *progname );

pfnChangeGame	pChangeGame = NULL;
host_parm_t	host;	// host parms
sysinfo_t		SI;

convar_t	*host_serverstate;
convar_t	*host_gameloaded;
convar_t	*host_clientloaded;
convar_t	*host_limitlocal;
convar_t	*host_cheats;
convar_t	*host_maxfps;
convar_t	*host_framerate;
convar_t	*host_sleeptime;
convar_t	*host_xashds_hacks;
convar_t	*con_gamemaps;
convar_t	*download_types;
convar_t	*build, *ver; // original xash3d info
convar_t	*host_build, *host_ver; // fork info
convar_t	*host_mapdesign_fatal;
convar_t 	*cmd_scripting = NULL;

static convar_t *host_nojoke;
static convar_t *host_forcejoke;

static int num_decals;

void Sys_PrintUsage( void )
{
#define O(x,y) "   "x"  "y"\n"

	const char *usage_str = "Usage:\n"
	"\t    "
	#ifndef __ANDROID__
		"<xash_binary>"
		#ifdef _WIN32
			".exe"
		#endif
	#endif
	" [options] [+command1] [+command2 arg]\n"
	"Availiable options:\n"
	O("-dev <level>     ","set developer level")
	O("-log             ","write log to \"engine.log\"")
	O("-toconsole       ","start witn console open")
	O("-nowriteconfig   ","disable config save")
	O("-casesensitive   ","disable case-insensitive FS emulation")
	#ifndef XASH_MOBILE_PLATFORM
		O("-daemonize       ", "run engine in background(only for dedicated)")
	#endif

	#ifndef XASH_DEDICATED
		O("-width <n>       ","specifies width of engine window")
		O("-height <n>      ","specifies height of engine window")

	#ifndef __ANDROID__
		O("-fullscreen      ","runs engine in fullscreen mode")
		O("-windowed        ","runs engine in windowed mode")
	#endif

		O("-nojoy           ","disable joystick support")
		O("-nosound         ","disable sound")
		O("-noenginemouse   ","disable mouse completely")
	#ifndef XASH_MOBILE_PLATFORM
			O("-dedicated       ","run in dedicated server mode")
	#endif
	#endif

	#ifdef __ANDROID__
		O("-nonativeegl  ","use java egl implementation. Use if screen does not update")
	#endif

	#ifdef _WIN32
		O("-noavi           ","disable AVI support")
		O("-nointro         ","disable intro video")
		O("-nowcon          ","disable win32 console")
	#endif

	#ifdef XASH_IPX
		O("-noipx           ","disable IPX")
	#endif
	O("-noip            ","disable TCP/IP")
	O("-noch            ","disable crashhandler")
	O("-disablehelp     ","disable this message")
	O("-dll <path>      ","override server DLL path")
	#ifndef XASH_DEDICATED
		O("-clientlib <path>","override client DLL path")
	#endif
	O("-rodir <path>    ","set read-only base directory, experimental")

	#if !defined(XASH_GLES) || !defined(XASH_NANOGL) || !defined(XASH_DEDICATED)
		O("-gldebug         ","enable OpenGL debug log through GL_EXT_debug_output, depends on platform")
	#endif
	;
#undef O

	Sys_Error( "%s", usage_str );
}

// these cvars will be duplicated on each client across network
int Host_ServerState( void )
{
	return host_serverstate->integer;
}

int Host_CompareFileTime( int ft1, int ft2 )
{
	if( ft1 < ft2 )
	{
		return -1;
	}
	else if( ft1 > ft2 )
	{
		return 1;
	}
	return 0;
}

void Host_ShutdownServer( void )
{
	if( !SV_Active()) return;
	Q_strncpy( host.finalmsg, "Server was killed", MAX_STRING );

	Log_Printf( "Server shutdown\n" );
	Log_Close();

	SV_Shutdown( false );
}

/*
================
Host_PrintEngineFeatures
================
*/
void Host_PrintEngineFeatures( void )
{
	if( host.features & ENGINE_WRITE_LARGE_COORD )
		MsgDev( D_AICONSOLE, "^3EXT:^7 Big world support enabled\n" );

	if( host.features & ENGINE_BUILD_SURFMESHES )
		MsgDev( D_AICONSOLE, "^3EXT:^7 Surfmeshes enabled\n" );

	if( host.features & ENGINE_LOAD_DELUXEDATA )
		MsgDev( D_AICONSOLE, "^3EXT:^7 Deluxemap support enabled\n" );

	if( host.features & ENGINE_TRANSFORM_TRACE_AABB )
		MsgDev( D_AICONSOLE, "^3EXT:^7 Transform trace AABB enabled\n" );

	if( host.features & ENGINE_LARGE_LIGHTMAPS )
		MsgDev( D_AICONSOLE, "^3EXT:^7 Large lightmaps enabled\n" );

	if( host.features & ENGINE_COMPENSATE_QUAKE_BUG )
		MsgDev( D_AICONSOLE, "^3EXT:^7 Quake bug compensation enabled\n" );
}

/*
================
Host_NewGame
================
*/
qboolean Host_NewGame( const char *mapName, qboolean loadGame )
{
	qboolean	iRet;

	iRet = SV_NewGame( mapName, loadGame );

	return iRet;
}

/*
================
Host_EndGame
================
*/
void Host_EndGame( const char *message, ... )
{
	va_list		argptr;
	static char	string[MAX_SYSPATH];
	
	va_start( argptr, message );
	Q_vsnprintf( string, sizeof( string ), message, argptr );
	va_end( argptr );

	MsgDev( D_INFO, "Host_EndGame: %s\n", string );
	
	if( SV_Active())
	{
		Q_snprintf( host.finalmsg, sizeof( host.finalmsg ), "Host_EndGame: %s", string );
		SV_Shutdown( false );
		return;
	}
	
	if( Host_IsDedicated() )
		Sys_Break( "Host_EndGame: %s\n", string ); // dedicated servers exit

	SV_Shutdown( false );
	CL_Disconnect();

	// recreate world if needs
	CL_ClearEdicts ();

	// release all models
	Mod_ClearAll( true );

	Host_AbortCurrentFrame ();
}

/*
================
Host_AbortCurrentFrame

aborts the current host frame and goes on with the next one
================
*/
void Host_Frame( float time );
void Host_RunFrame()
{
	static double	oldtime, newtime;

	if( !oldtime )
		oldtime = Sys_DoubleTime();

#if XASH_INPUT == INPUT_SDL
	SDLash_RunEvents();
#elif XASH_INPUT == INPUT_ANDROID
	Android_RunEvents();
#endif

	newtime = Sys_DoubleTime ();

	Host_Frame( newtime - oldtime );

	oldtime = newtime;
#ifdef __EMSCRIPTEN__
#ifdef EMSCRIPTEN_ASYNC
	emscripten_sleep(1);
#else
	if( host.crashed || host.shutdown_issued )
		emscripten_cancel_main_loop();
#endif
#endif
}

void Host_FrameLoop()
{
#if defined __EMSCRIPTEN__ && !defined EMSCRIPTEN_ASYNC
	emscripten_cancel_main_loop();
	emscripten_set_main_loop( Host_RunFrame, 0, 0 );
#else
	// main window message loop
	while( !host.crashed && !host.shutdown_issued )
	{
		Host_RunFrame();
	}
#endif
}

void EXPORT Host_AbortCurrentFrame( void )
{
#ifndef NO_SJLJ
	if( host.framecount == 0 ) // abort frame was not set up
		Sys_Break("Could not abort current frame");
	else
		longjmp( host.abortframe, 1 );
#else // sj/lj not supported, so re-run main loop with shifted stack
	Host_FrameLoop();
#endif
#ifdef __EMSCRIPTEN__
	EM_ASM(throw 'SimulateInfiniteLoop');
#else
	exit(127);
#endif
}

/*
==================
Host_SetServerState
==================
*/
void Host_SetServerState( int state )
{
	Cvar_FullSet( "host_serverstate", va( "%i", state ), CVAR_INIT );
}

void Host_NewInstance( const char *name, const char *finalmsg )
{
	if( !pChangeGame ) return;

	host.change_game = true;
	Q_strncpy( host.finalmsg, finalmsg, sizeof( host.finalmsg ));
	pChangeGame( name ); // call from hl.exe
}

/*
=================
Host_ChangeGame_f

Change game modification
=================
*/
void Host_ChangeGame_f( void )
{
	int	i;

	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: game <directory>\n" );
		return;
	}

	// validate gamedir
	for( i = 0; i < SI.numgames; i++ )
	{
		if( !Q_stricmp( SI.games[i]->gamefolder, Cmd_Argv( 1 )))
			break;
	}

	if( i == SI.numgames )
	{
		Msg( "%s not exist\n", Cmd_Argv( 1 ));
	}
	else if( !Q_stricmp( GI->gamefolder, Cmd_Argv( 1 )))
	{
		Msg( "%s already active\n", Cmd_Argv( 1 ));	
	}
	else
	{
		const char *arg1 = va( "%s%s", (host.type == HOST_NORMAL) ? "" : "#", Cmd_Argv( 1 ));
		const char *arg2 = va( "change game to '%s'", SI.games[i]->title );

		Host_NewInstance( arg1, arg2 );
	}
}

/*
===============
Host_Exec_f
===============
*/
void Host_Exec_f( void )
{
	string	cfgpath;
	char	*f;

	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: exec <filename>\n" );
		return;
	}

	// HACKHACK: don't execute listenserver.cfg in singleplayer
	if( !Q_stricmp( Cvar_VariableString( "lservercfgfile" ),  Cmd_Argv( 1 )))
	{
		if( Cvar_VariableValue( "maxplayers" ) == 1.0f )
			return;
	}

	Q_strncpy( cfgpath, Cmd_Argv( 1 ), sizeof( cfgpath )); 
	FS_DefaultExtension( cfgpath, ".cfg" ); // append as default

	f = (char *)FS_LoadFile( cfgpath, NULL, false );
	if( !f )
	{
		MsgDev( D_NOTE, "couldn't exec %s\n", Cmd_Argv( 1 ));
		return;
	}

	if( !Q_stricmp( Cvar_VariableString( "lservercfgfile" ),  Cmd_Argv( 1 )))
	{
		if( Q_strstr( f, "//=======================================================================" ) &&
			Q_strstr( f, "//\t\t\tCopyright XashXT Group" ) &&
			Q_strstr( f, "//\t\t\tserver.cfg - server temp" ) )
		{
			Msg( "^1Found old generated xash3d listenserver config, skipping!\n" );
			Msg( "^1Remove Xash3D header to use it\n" );
			Mem_Free( f );
			return;
		}
	}

	MsgDev( D_INFO, "execing %s\n", Cmd_Argv( 1 ));

	// terminate the string with newline just in case it's missing
	// insertion order is backwards from execution order
	Cbuf_InsertText( "\n" );
	Cbuf_InsertText( f );
	Mem_Free( f );
}

/*
===============
Host_Clear_f

Clear all consoles
===============
*/
void Host_Clear_f( void )
{
#ifndef XASH_DEDICATED
	Con_Clear();
#endif
#ifdef XASH_W32CON
	Wcon_Clear();
#endif
}

/*
===============
Host_MemStats_f
===============
*/
void Host_MemStats_f( void )
{
	switch( Cmd_Argc( ))
	{
	case 1:
		Mem_PrintList( 1<<30 );
		Mem_PrintStats();
		break;
	case 2:
		Mem_PrintList( Q_atoi( Cmd_Argv( 1 )) * 1024 );
		Mem_PrintStats();
		break;
	default:
		Msg( "Usage: memlist <all>\n" );
		break;
	}
}

void Host_Minimize_f( void )
{
#ifdef XASH_SDL
	if( host.hWnd )
		SDL_MinimizeWindow( host.hWnd );
#endif
}

qboolean Host_IsLocalGame( void )
{
	if( Host_IsDedicated() )
		return false;
	if( CL_Active() && SV_Active() && CL_GetMaxClients() == 1 )
		return true;
	return false;
}

qboolean Host_IsLocalClient( void )
{
	// only the local client has the active server
	if( CL_Active() && SV_Active())
		return true;
	return false;
}

/*
=================
Host_RegisterDecal
=================
*/
qboolean Host_RegisterDecal( const char *name )
{
	char	shortname[CS_SIZE];
	int	i;

	if( !name || !name[0] )
		return 0;

	FS_FileBase( name, shortname );

	for( i = 1; i < MAX_DECALS && host.draw_decals[i][0]; i++ )
	{
		if( !Q_stricmp( (char *)host.draw_decals[i], shortname ))
			return true;
	}

	if( i == MAX_DECALS )
	{
		MsgDev( D_ERROR, "Host_RegisterDecal: MAX_DECALS limit exceeded\n" );
		return false;
	}

	// register new decal
	Q_strncpy( (char *)host.draw_decals[i], shortname, sizeof( host.draw_decals[i] ));
	num_decals++;

	return true;
}

/*
=================
Host_InitDecals
=================
*/
void Host_InitDecals( void )
{
	search_t	*t;
	int	i;

	Q_memset( host.draw_decals, 0, sizeof( host.draw_decals ));
	num_decals = 0;

	// lookup all decals in decals.wad
	t = FS_Search( "decals.wad/*.*", true, false );

	for( i = 0; t && i < t->numfilenames; i++ )
	{
		if( !Host_RegisterDecal( t->filenames[i] ))
			break;
	}

	if( t ) Mem_Free( t );
	MsgDev( D_NOTE, "InitDecals: %i decals\n", num_decals );
}

/*
=================
Host_RestartAmbientSounds

Write ambient sounds into demo
=================
*/
void Host_RestartAmbientSounds( void )
{
	soundlist_t	soundInfo[64];
	string		curtrack, looptrack;
	int		i, nSounds;
	fs_offset_t	position;

	if( !SV_Active( ))
	{
		return;
	}

	nSounds = S_GetCurrentStaticSounds( soundInfo, 64 );
	
	for( i = 0; i < nSounds; i++ )
	{
		if( !soundInfo[i].looping || soundInfo[i].entnum == -1 )
			continue;

		MsgDev( D_NOTE, "Restarting sound %s...\n", soundInfo[i].name );
		S_StopSound( soundInfo[i].entnum, soundInfo[i].channel, soundInfo[i].name );
		SV_StartSound( pfnPEntityOfEntIndex( soundInfo[i].entnum ), CHAN_STATIC, soundInfo[i].name,
		soundInfo[i].volume, soundInfo[i].attenuation, 0, soundInfo[i].pitch );
	}

	// restart soundtrack
	if( S_StreamGetCurrentState( curtrack, looptrack, &position ))
	{
		SV_StartMusic( curtrack, looptrack, position );
	}
}

/*
=================
Host_RestartDecals

Write all the decals into demo
=================
*/
void Host_RestartDecals( void )
{
	decallist_t	*entry;
	int		decalIndex;
	int		modelIndex;
	sizebuf_t		*msg;
	int		i;

	if( !SV_Active( ))
	{
		return;
	}

	// g-cont. add space for studiodecals if present
	host.decalList = (decallist_t *)Z_Malloc( sizeof( decallist_t ) * MAX_RENDER_DECALS * 2 );
	host.numdecals = R_CreateDecalList( host.decalList, false );

	// remove decals from map
	R_ClearAllDecals();

	// write decals into reliable datagram
	msg = SV_GetReliableDatagram();

	// restore decals and write them into network message
	for( i = 0; i < host.numdecals; i++ )
	{
		entry = &host.decalList[i];
		modelIndex = pfnPEntityOfEntIndex( entry->entityIndex )->v.modelindex;

		// game override
		if( SV_RestoreCustomDecal( entry, pfnPEntityOfEntIndex( entry->entityIndex ), false ))
			continue;

		decalIndex = pfnDecalIndex( entry->name );

		// BSP and studio decals have different messages
		if( entry->flags & FDECAL_STUDIO )
		{
			// NOTE: studio decal trace start saved into impactPlaneNormal
			SV_CreateStudioDecal( msg, entry->position, entry->impactPlaneNormal, decalIndex, entry->entityIndex,
			modelIndex, entry->flags, &entry->studio_state );
		}
		else
		{
			SV_CreateDecal( msg, entry->position, decalIndex, entry->entityIndex, modelIndex, entry->flags, entry->scale );
		}
	}

	Z_Free( host.decalList );
	host.decalList = NULL;
	host.numdecals = 0;
}

/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands( void )
{
	char	*cmd;

	while( ( cmd = Sys_Input() ) )
	{
		Cbuf_AddText( cmd );
		Cbuf_Execute();
	}
}

/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
qboolean Host_FilterTime( float time )
{
	static double	oldtime;
	float		fps;

	host.realtime += time;

	// dedicated's tic_rate regulates server frame rate.  Don't apply fps filter here.
	fps = host_maxfps->value;

	if( fps != 0 )
	{
		float	minframetime;

		// limit fps to within tolerable range
		fps = bound( MIN_FPS, fps, MAX_FPS );

		minframetime = 1.0f / fps;

		if(( host.realtime - oldtime ) < minframetime )
		{
			// framerate is too high
			//Sys_Sleep( 2000 * ( minframetime - ( host.realtime - oldtime ) ) );
			return false;
		}
	}

	host.frametime = host.realtime - oldtime;
	host.realframetime = bound( MIN_FRAMETIME, host.frametime, MAX_FRAMETIME );
	oldtime = host.realtime;

	if( host_framerate->value > 0 && ( Host_IsLocalGame()))
	{
		fps = host_framerate->value;
		if( fps > 1 ) fps = 1.0f / fps;
		host.frametime = fps;
	}
	else
	{	// don't allow really long or short frames
		host.frametime = bound( MIN_FRAMETIME, host.frametime, MAX_FRAMETIME );
	}
	
	return true;
}

/*
=================
Host_Autosleep
=================
*/
void Host_Autosleep( void )
{
	int sleeptime = host_sleeptime->integer;

	if( Host_IsDedicated() )
	{
		// let the dedicated server some sleep
		Sys_Sleep( sleeptime );

	}
	else
	{
		if( host.state == HOST_NOFOCUS )
		{
			if( Host_ServerState() && CL_IsInGame( ))
				Sys_Sleep( sleeptime ); // listenserver
			else Sys_Sleep( 20 ); // sleep 20 ms otherwise
		}
		else if( host.state == HOST_SLEEP )
		{
			// completely sleep in minimized state
			Sys_Sleep( 20 );
		}
		else
		{
			Sys_Sleep( sleeptime );
		}
	}
}

/*
=================
Host_Frame
=================
*/
void Host_Frame( float time )
{
#ifndef NO_SJLJ
	if( setjmp( host.abortframe ))
		return;
#endif

	Host_Autosleep();

	// decide the simulation time
	if( !Host_FilterTime( time ))
		return;

	rand (); // keep the random time dependent

	Sys_SendKeyEvents (); // call WndProc on WIN32

	Host_InputFrame ();	// input frame

#ifndef XASH_DEDICATED
	Host_ClientBegin(); // prepare client command
#endif

	Host_GetConsoleCommands ();

	Host_ServerFrame (); // server frame

	if ( !Host_IsDedicated() )
		Host_ClientFrame (); // client frame

	HTTP_Run();

	host.framecount++;
}

/*
================
Host_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void Host_Print( const char *txt )
{
	Rcon_Print( txt );
	Con_Print( txt ); // echo to client console
}

/*
=================
Host_Error
=================
*/
void Host_Error( const char *error, ... )
{
	static char	hosterror1[MAX_SYSPATH];
	static char	hosterror2[MAX_SYSPATH];
	static qboolean	recursive = false;
	va_list		argptr;

	if( host.mouse_visible && !CL_IsInMenu( ))
	{
		// hide VGUI mouse
#ifdef XASH_SDL
		SDL_ShowCursor( false );
#endif
		host.mouse_visible = false;
	}

	va_start( argptr, error );
	Q_vsnprintf( hosterror1, sizeof( hosterror1 ), error, argptr );
	va_end( argptr );

	CL_WriteMessageHistory (); // before Q_error call

	if( host.framecount < 3 )
	{
		Sys_Error( "Host_InitError: %s", hosterror1 );
	}
	else if( host.framecount == host.errorframe )
	{
		Sys_Error( "Host_MultiError: %s", hosterror2 );
		return;
	}
	else
	{
		if( host.developer > 0 )
		{
			UI_SetActiveMenu( false );
			Key_SetKeyDest( key_console );
			Msg( "^1Host_Error: ^7%s", hosterror1 );
		}
		else MSGBOX2( hosterror1 );
	}

	// host is shutting down. don't invoke infinite loop
	if( host.state == HOST_SHUTDOWN ) return;

	if( recursive )
	{ 
		Msg( "Host_RecursiveError: %s", hosterror2 );
		Sys_Error( "%s", hosterror1 );
		return; // don't multiple executes
	}

	recursive = true;
	Q_strncpy( hosterror2, hosterror1, MAX_SYSPATH );
	host.errorframe = host.framecount; // to avoid multple calls per frame
	Q_sprintf( host.finalmsg, "Server crashed: %s", hosterror1 );

	// clear cmd buffer to prevent execution of any commands
	Cbuf_Clear();

	SV_Shutdown( false );
	CL_Drop(); // drop clients

	// release all models
	Mod_ClearAll( false );

	recursive = false;

	Host_AbortCurrentFrame();
}

void Host_Error_f( void )
{
	const char *error = Cmd_Argv( 1 );

	if( !*error ) error = "Invoked host error";
	Host_Error( "%s\n", error );
}

void Sys_Error_f( void )
{
	const char *error = Cmd_Argv( 1 );

	if( !*error ) error = "Invoked sys error";
	Sys_Error( "%s\n", error );
}

void Net_Error_f( void )
{
	Q_strncpy( host.finalmsg, Cmd_Argv( 1 ), sizeof( host.finalmsg ));
	SV_ForceError();
}

/*
=================
Host_Crash_f
=================
*/
static void Host_Crash_f( void )
{
	*(int *)0 = 0xffffffff;
}
/*
=================
Host_MapDesignError

Stop mappers for making bad maps
but allow to ignore errors when need
=================
*/
void Host_MapDesignError( const char *format, ... )
{
	char str[256];
	va_list		argptr;
	va_start( argptr, format );
	Q_vsnprintf( str, 256, format, argptr );
	va_end( argptr );
	if( host_mapdesign_fatal->integer )
		Host_Error( "Map Design Error: %s\n", str );
	else
		Msg( "^1Map Design Error: ^3%s", str );
}
/*
=================
Host_Userconfigd_f

Exec all configs from userconfig.d directory
=================
*/
void Host_Userconfigd_f( void )
{
	search_t		*t;
	int		i;

	t = FS_Search( "userconfig.d/*.cfg", true, false );
	if( !t ) return;

	for( i = 0; i < t->numfilenames; i++ )
	{
		Cbuf_AddText( va("exec %s\n", t->filenames[i] ) );
	}

	Mem_Free( t );

}


/*
=================
Host_InitCommon
=================
*/
void Host_InitCommon( int argc, const char** argv, const char *progname, qboolean bChangeGame )
{
	char		dev_level[4];
	char		*baseDir;

	// some commands may turn engine into infinite loop,
	// e.g. xash.exe +game xash -game xash
	// so we clear all cmd_args, but leave dbg states as well
	Sys_ParseCommandLine( argc, argv );

	// to be accessed later
	if( ( host.daemonized = Sys_CheckParm( "-daemonize" ) ) )
	{
#if defined(_POSIX_VERSION) && !defined(XASH_MOBILE_PLATFORM)
		pid_t daemon;

		daemon = fork();

		if( daemon < 0 )
		{
			Host_Error( "fork() failed: %s\n", strerror( errno ) );
		}

		if( daemon > 0 )
		{
			// parent
			MsgDev( D_INFO, "Child pid: %i\n", daemon );
			exit( 0 );
		}
		else
		{
			// don't be closed by parent
			if( setsid() < 0 )
			{
				Host_Error( "setsid() failed: %s\n", strerror( errno ) );
			}

			// set permissions
			umask( 0 );

			// engine will still use stdin/stdout,
			// so just redirect them to /dev/null
			close( STDIN_FILENO );
			close( STDOUT_FILENO );
			close( STDERR_FILENO );
			open("/dev/null", O_RDONLY); // becomes stdin
			open("/dev/null", O_RDWR); // stdout
			open("/dev/null", O_RDWR); // stderr

			// fallthrough
		}
#elif defined(XASH_MOBILE_PLATFORM)
		Sys_Error( "Can't run in background on mobile platforms!" );
#else
		Sys_Error( "Daemonize not supported on this platform!" );
#endif
	}

	
	host.enabledll = !Sys_CheckParm( "-nodll" );

	host.shutdown_issued = false;
	host.crashed = false;
#ifdef DLL_LOADER
	if( host.enabledll )
		Setup_LDT_Keeper( ); // Must call before creating any thread
#endif

	if( ( baseDir = getenv( "XASH3D_BASEDIR" ) ) )
	{
		Q_strncpy( host.rootdir, baseDir, sizeof(host.rootdir) );
	}
	else
	{
#if TARGET_OS_IPHONE
		const char *IOS_GetDocsDir();
		Q_strncpy( host.rootdir, IOS_GetDocsDir(), sizeof(host.rootdir) );
#elif defined(XASH_SDL)
		if( !( baseDir = SDL_GetBasePath() ) )
			Sys_Error( "couldn't determine current directory: %s", SDL_GetError() );
		Q_strncpy( host.rootdir, baseDir, sizeof( host.rootdir ) );
		SDL_free( baseDir );
#else
		if( !getcwd( host.rootdir, sizeof(host.rootdir) ) )
		{
			Sys_Error( "couldn't determine current directory: %s", strerror( errno ) );
			host.rootdir[0] = 0;
		}
#endif
	}

	// get readonly root. The order is: check for arg, then env.
	// If still not got it, rodir is disabled.
	host.rodir[0] = 0;
	if( !Sys_GetParmFromCmdLine( "-rodir", host.rodir ) )
	{
		char *roDir;

		if( ( roDir = getenv( "XASH3D_RODIR" ) ) )
		{
			Q_strncpy( host.rodir, roDir, sizeof( host.rodir ) );
		}
	}

	if( !Sys_CheckParm( "-disablehelp" ) )
	{
	    if( Sys_CheckParm( "-help" ) || Sys_CheckParm( "-h" ) || Sys_CheckParm( "--help" ) )
	    {
			Sys_PrintUsage();
	    }
	}
	if( host.rootdir[Q_strlen( host.rootdir ) - 1] == '/' )
		host.rootdir[Q_strlen( host.rootdir ) - 1] = 0;

	if( host.rodir[0] && host.rodir[Q_strlen( host.rodir ) - 1] == '/' )
		host.rodir[Q_strlen( host.rodir ) - 1] = 0;

	if( !Sys_CheckParm( "-noch" ) )
	{
		Sys_SetupCrashHandler();
	}

	host.state = HOST_INIT; // initialization started
	host.developer = host.old_developer = DEFAULT_DEV;
	host.textmode = false;

	host.mempool = Mem_AllocPool( "Zone Engine" );

	if( Sys_CheckParm( "-console" )) host.developer = 1;
	if( Sys_CheckParm( "-dev" ))
	{
		if( Sys_GetParmFromCmdLine( "-dev", dev_level ))
		{
			if( Q_isdigit( dev_level ))
				host.developer = abs( Q_atoi( dev_level ));
			else host.developer++; // -dev == 1, -dev -console == 2
		}
		else host.developer++; // -dev == 1, -dev -console == 2
	}

#ifdef XASH_DEDICATED
	host.type = HOST_DEDICATED; // predict state
#else
	if( Sys_CheckParm("-dedicated") || progname[0] == '#' )
	{
		host.type = HOST_DEDICATED;
	}
	else
	{
		host.type = HOST_NORMAL;
	}
#endif

	host.con_showalways = true;
	host.mouse_visible = false;

#ifdef XASH_SDL
	// should work even if it failed
	SDL_Init( SDL_INIT_TIMER );

	if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) )
	{
		Sys_Warn( "SDL_Init failed: %s", SDL_GetError() );
		host.type = HOST_DEDICATED;
	}
	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
#ifdef XASH_GLES
	SDL_SetHint( SDL_HINT_OPENGL_ES_DRIVER, "1" );
#endif
#endif

	if ( !host.rootdir[0] || SetCurrentDirectory( host.rootdir ) != 0)
		MsgDev( D_INFO, "%s is working directory now\n", host.rootdir );
	else
		Sys_Error( "Changing working directory to %s failed.\n", host.rootdir );

	Sys_InitLog();

	// set default gamedir
	if( progname[0] == '#' ) progname++;
	Q_strncpy( SI.ModuleName, progname, sizeof( SI.ModuleName ));

	if( Host_IsDedicated() )
	{
		Sys_MergeCommandLine( );

		if( host.developer < 3 ) host.developer = 3; // otherwise we see empty console
	}
	else
	{
		// don't show console as default
		if( host.developer < D_WARN ) host.con_showalways = false;
	}

	host.old_developer = host.developer;

#ifdef XASH_W32CON
	Wcon_Init();
	Wcon_CreateConsole();
#endif

	// first text message into console or log 
	MsgDev( D_NOTE, "Console initialized\n" );

	BaseCmd_Init();

	// startup cmds and cvars subsystem
	Cmd_Init();
	Cvar_Init();

	Cmd_AddCommand( "clear", Host_Clear_f, "clear console history" );

	// share developer level across all dlls
	Q_snprintf( dev_level, sizeof( dev_level ), "%i", host.developer );
	Cvar_Get( "developer", dev_level, CVAR_INIT, "current developer level" );
	Cmd_AddCommand( "exec", Host_Exec_f, "execute a script file" );
	Cmd_AddCommand( "memlist", Host_MemStats_f, "prints memory pool information" );
	Cmd_AddCommand( "userconfigd", Host_Userconfigd_f, "execute all scripts from userconfig.d" );
	cmd_scripting = Cvar_Get( "cmd_scripting", "0", CVAR_ARCHIVE, "enable simple condition checking and variable operations" );
	
	FS_Init();
	Image_Init();
	Sound_Init();

	FS_LoadGameInfo( NULL );
	Q_strncpy( host.gamefolder, GI->gamefolder, sizeof( host.gamefolder ));


#ifdef DEPRECATED_SECURE
	if( GI->secure )
	{
		// clear all developer levels when game is protected
		Cvar_FullSet( "developer", "0", CVAR_INIT );
		host.developer = host.old_developer = 0;
		host.con_showalways = false;
	}
#endif

	HPAK_Init();

	IN_Init();
	Key_Init();
}

void Host_FreeCommon( void )
{
	Image_Shutdown();
	Sound_Shutdown();
	Netchan_Shutdown();
	FS_Shutdown();

	Mem_FreePool( &host.mempool );
}
/*
=================
Host_Main
=================
*/
int EXPORT Host_Main( int argc, const char **argv, const char *progname, int bChangeGame, pfnChangeGame func )
{
	pChangeGame = func;	// may be NULL

	host.change_game = bChangeGame;

	Host_InitCommon( argc, argv, progname, bChangeGame );

	// init commands and vars
	if( host.developer >= 3 )
	{
		Cmd_AddCommand ( "sys_error", Sys_Error_f, "just throw a fatal error to test shutdown procedures");
		Cmd_AddCommand ( "host_error", Host_Error_f, "just throw a host error to test shutdown procedures");
		Cmd_AddCommand ( "crash", Host_Crash_f, "a way to force a bus error for development reasons");
		Cmd_AddCommand ( "net_error", Net_Error_f, "send network bad message from random place");
	}

	host_cheats = Cvar_Get( "sv_cheats", "0", CVAR_LATCH, "allow usage of cheat commands and variables" );
	host_maxfps = Cvar_Get( "fps_max", "72", CVAR_ARCHIVE, "host fps upper limit" );
	host_sleeptime = Cvar_Get( "sleeptime", "1", CVAR_ARCHIVE, "higher value means lower accuracy" );
	host_framerate = Cvar_Get( "host_framerate", "0", 0, "locks frame timing to this value in seconds" );  
	host_serverstate = Cvar_Get( "host_serverstate", "0", CVAR_INIT, "displays current server state" );
	host_gameloaded = Cvar_Get( "host_gameloaded", "0", CVAR_INIT, "indicates a loaded game library" );
	host_clientloaded = Cvar_Get( "host_clientloaded", "0", CVAR_INIT, "indicates a loaded client library" );
	host_limitlocal = Cvar_Get( "host_limitlocal", "0", 0, "apply cl_cmdrate and rate to loopback connection" );
	con_gamemaps = Cvar_Get( "con_mapfilter", "1", CVAR_ARCHIVE, "when enabled, show only maps in game folder (no maps from base folder when running mod)" );
	download_types = Cvar_Get( "download_types", "msec", CVAR_ARCHIVE, "list of types to download: Model, Sounds, Events, Custom" );
	build = Cvar_Get( "build", va( "%i", Q_buildnum_compat()), CVAR_INIT, "returns an original xash3d build number. left for compability" );
	ver = Cvar_Get( "ver", va( "%i/%g.%i", PROTOCOL_VERSION, BASED_VERSION, Q_buildnum_compat( ) ), CVAR_INIT, "shows an engine version. left for compabiltiy" );
	host_build = Cvar_Get( "host_build", va("%i", Q_buildnum() ), CVAR_INIT, "returns current build number" );
	host_ver = Cvar_Get( "host_ver", va("%i %s %s %s %s", Q_buildnum(), XASH_VERSION, Q_buildos(), Q_buildarch(), Q_buildcommit() ), CVAR_INIT, "detailed info about this build" );
	host_mapdesign_fatal = Cvar_Get( "host_mapdesign_fatal", "1", CVAR_ARCHIVE, "make map design errors fatal" );
	host_xashds_hacks = Cvar_Get( "xashds_hacks", "0", 0, "hacks for xashds in singleplayer" );
	host_nojoke = Cvar_Get( "host_nojoke", "0", CVAR_ARCHIVE, "disable april fools joke");
	host_forcejoke = Cvar_Get( "host_forcejoke", "0", 0, "let april fools continue" );

	// content control
	Cvar_Get( "violence_hgibs", "1", CVAR_ARCHIVE, "show human gib entities" );
	Cvar_Get( "violence_agibs", "1", CVAR_ARCHIVE, "show alien gib entities" );
	Cvar_Get( "violence_hblood", "1", CVAR_ARCHIVE, "draw human blood" );
	Cvar_Get( "violence_ablood", "1", CVAR_ARCHIVE, "draw alien blood" );
	if( !Host_IsDedicated() )
	{
		// when we're in developer-mode, automatically turn cheats on
		if( host.developer > 1 ) Cvar_SetFloat( "sv_cheats", 1.0f );
		Cbuf_AddText( "exec video.cfg\n" );
	}

	Mod_Init();
	NET_Init();
	NET_InitMasters();
	Netchan_Init();

	// allow to change game from the console
	if( pChangeGame != NULL )
	{
		Cmd_AddCommand( "game", Host_ChangeGame_f, "change active game/mod" );
		Cvar_Get( "host_allow_changegame", "1", CVAR_READ_ONLY, "whether changing game/mod is allowed" );
	}
	else
	{
		Cvar_Get( "host_allow_changegame", "0", CVAR_READ_ONLY, "allows to change games" );
	}

	SV_Init();
	CL_Init();

#if defined(__ANDROID__) && !defined( XASH_SDL ) && !defined( XASH_DEDICATED )
	Android_Init();
#endif

	HTTP_Init();

	ID_Init();

	// post initializations
	switch( host.type )
	{
	case HOST_NORMAL:
#ifdef XASH_W32CON
		Wcon_ShowConsole( false ); // hide console
#endif
		// execute startup config and cmdline
		Cbuf_AddText( va( "exec %s.rc\n", GI->gamefolder ) );
		CSCR_LoadDefaultCVars( "settings.scr" );
		CSCR_LoadDefaultCVars( "user.scr" );
		// intentional fallthrough
	case HOST_DEDICATED:
		//Cbuf_Execute(); // force stuffcmds run if it is in cbuf
		// if stuffcmds wasn't run, then init.rc is probably missing, use default
		if( !FS_FileExists( va( "%s.rc\n", SI.ModuleName ), false ) ) Cbuf_AddText( "stuffcmds\n" );

		break;
	case HOST_UNKNOWN:
		break;
	}

	if( Host_IsDedicated() )
	{
		Cmd_AddCommand( "quit", Sys_Quit, "quit the game" );
		Cmd_AddCommand( "exit", Sys_Quit, "quit the game" );

		SV_InitGameProgs();

		Cbuf_AddText( "exec config.cfg\n" );

		if( !Sys_CheckParm( "+map" ) )
				Cbuf_AddText( "startdefaultmap" );

		Cvar_FullSet( "xashds_hacks", "0", CVAR_READ_ONLY );

		Cbuf_Execute(); // apply port cvar

		NET_Config( true, true );
	}
	else
	{
		Cmd_AddCommand( "minimize", Host_Minimize_f, "minimize main window to taskbar" );
		Cbuf_AddText( "exec config.cfg\n" );
		// listenserver/multiplayer config.
		// need load it to update menu options.
		Cbuf_AddText( "exec game.cfg\n" );
		Cmd_AddCommand( "host_writeconfig", Host_WriteConfig, "force save configs. use with care" );
	}

	host.errorframe = 0;


	host.change_game = false;	// done
	Cmd_RemoveCommand( "setr" );	// remove potential backdoor for changing renderer settings
	Cmd_RemoveCommand( "setgl" );

	// we need to execute it again here
	if( !Host_IsDedicated() )
		Cmd_ExecuteString( "exec config.cfg\n", src_command );

	Cbuf_Execute();

	// exec all files from userconfig.d 
	Host_Userconfigd_f();

	// in case of empty init.rc
	if( !host.stuffcmdsrun )
		Cbuf_AddText( "stuffcmds\n" );

	IN_TouchInitConfig();
	SCR_CheckStartupVids();	// must be last
#ifdef XASH_SDL
	SDL_StopTextInput(); // disable text input event. Enable this in chat/console?
#endif

	if( host.state == HOST_INIT )
		host.state = HOST_FRAME; // initialization is finished

	if( host_forcejoke->value )
	{
		host.joke = true;
	}
	else
	{
	#ifdef __ANDROID__
		time_t timeval;
		struct tm *timeinfo;

		timeval = time( NULL );
		timeinfo = localtime( &timeval );

		host.joke = !host_nojoke->value && ( timeinfo->tm_year == 118 && timeinfo->tm_mday == 1 && timeinfo->tm_mon == 3 );
	#else
		host.joke = false;
	#endif
	}
	Host_FrameLoop();

	// never reached
	return 0;
}

/*
=================
Host_Shutdown
=================
*/
void EXPORT Host_Shutdown( void )
{
	if( host.shutdown_issued ) return;
	host.shutdown_issued = true;


	switch( host.state )
	{
	case HOST_INIT:
	case HOST_CRASHED:
	case HOST_ERR_FATAL:
		if( !Host_IsDedicated() )
			MsgDev( D_WARN, "Not shutting down normally (%d), skipping config save!\n", host.state );
		if( host.state != HOST_ERR_FATAL)
			host.state = HOST_SHUTDOWN;
		break;
	default:
#ifndef XASH_DEDICATED
		if( !Host_IsDedicated() && !host.skip_configs )
		{
			// restore all latched cheat cvars
			Cvar_SetCheatState( true );
			Host_WriteConfig();
			IN_TouchWriteConfig();
			host.skip_configs = false;
		}
#endif
		host.state = HOST_SHUTDOWN; // prepare host to normal shutdown
	}

	if( !host.change_game )
		Q_strncpy( host.finalmsg, "Server shutdown", sizeof( host.finalmsg ));

	Log_Printf( "Server shutdown\n" );
	Log_Close();

	SV_Shutdown( false );
	SV_ShutdownFilter();
	SV_UnloadProgs();
	CL_Shutdown();

	Mod_Shutdown();
	NET_Shutdown();
	HTTP_Shutdown();
	Con_ClearAutoComplete();
	Cmd_Shutdown();
	Host_FreeCommon();
	Sys_DestroyConsole();
	Sys_CloseLog();
	Sys_RestoreCrashHandler();

}
