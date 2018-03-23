/*
cl_cmds.c - client console commnds
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

#ifndef XASH_DEDICATED
#include "common.h"
#include "client.h"
#include "gl_local.h"

/*
====================
CL_PlayVideo_f

movie <moviename>
====================
*/
void CL_PlayVideo_f( void )
{
	string	path;

	if( Cmd_Argc() != 2 && Cmd_Argc() != 3 )
	{
		Msg( "movie <moviename> [full]\n" );
		return;
	}

	if( cls.state == ca_active )
	{
		Msg( "Can't play movie while connected to a server.\nPlease disconnect first.\n" );
		return;
	}

	switch( Cmd_Argc( ))
	{
	case 2:	// simple user version
		Q_snprintf( path, sizeof( path ), "media/%s.avi", Cmd_Argv( 1 ));
		SCR_PlayCinematic( path );
		break;
	case 3:	// sequenced cinematics used this
		SCR_PlayCinematic( Cmd_Argv( 1 ));
		break;
	}
}

/*
===============
CL_PlayCDTrack_f

Emulate audio-cd system
===============
*/
void CL_PlayCDTrack_f( void )
{
	const char	*command;
	static int	track = 0;
	static qboolean	paused = false;
	static qboolean	looped = false;
	static qboolean	enabled = true;

	if( Cmd_Argc() < 2 ) return;
	command = Cmd_Argv( 1 );

	if( !enabled && Q_stricmp( command, "on" )) return; // CD-player is disabled

	if( !Q_stricmp( command, "play" ))
	{
		track = bound( 1, Q_atoi( Cmd_Argv( 2 )), MAX_CDTRACKS );
		S_StartBackgroundTrack( clgame.cdtracks[track-1], NULL, 0 );
		paused = false;
		looped = false;
	}
	else if( !Q_stricmp( command, "loop" ))
	{
		track = bound( 1, Q_atoi( Cmd_Argv( 2 )), MAX_CDTRACKS );
		S_StartBackgroundTrack( clgame.cdtracks[track-1], clgame.cdtracks[track-1], 0 );
		paused = false;
		looped = true;
	}
	else if( !Q_stricmp( command, "pause" ))
	{
		S_StreamSetPause( true );
		paused = true;
	}
	else if( !Q_stricmp( command, "resume" ))
	{
		S_StreamSetPause( false );
		paused = false;
	}
	else if( !Q_stricmp( command, "stop" ))
	{
		S_StopBackgroundTrack();
		paused = false;
		looped = false;
		track = 0;
	}
	else if( !Q_stricmp( command, "on" ))
	{
		enabled = true;
	}
	else if( !Q_stricmp( command, "off" ))
	{
		enabled = false;
	}
	else if( !Q_stricmp( command, "info" ))
	{
		int	i, maxTrack;

		for( maxTrack = i = 0; i < MAX_CDTRACKS; i++ )
			if( Q_strlen( clgame.cdtracks[i] )) maxTrack++;
			
		Msg( "%u tracks\n", maxTrack );
		if( track )
		{
			if( paused ) Msg( "Paused %s track %u\n", looped ? "looping" : "playing", track );
			else Msg( "Currently %s track %u\n", looped ? "looping" : "playing", track );
		}
		Msg( "Volume is %f\n", Cvar_VariableValue( "MP3Volume" ));
		return;
	}
	else Msg( "cd: unknown command %s\n", command );
}

/*
===============
CL_PlayCDTrack_f

Handle "mp3" console command
===============
*/
void CL_MP3Command_f ( void )
{
	char *pszCommand, *pszTrack;

	if ( Cmd_Argc() < 2 )
		return;

	pszCommand = Cmd_Argv (1);
	pszTrack = Cmd_Argv (2);

	if (Q_stricmp(pszCommand, "play") == 0)
	{
		S_StartBackgroundTrack( pszTrack, NULL, 0 );
		return;
	}
	else if (Q_stricmp(pszCommand, "playfile") == 0)
	{
		S_StartBackgroundTrack( pszTrack, NULL, 0 );
		return;
	}
	else if (Q_stricmp(pszCommand, "loop") == 0)
	{
		S_StartBackgroundTrack( pszTrack, pszTrack, 0 );
		return;
	}
	else if (Q_stricmp(pszCommand, "loopfile") == 0)
	{
		S_StartBackgroundTrack( pszTrack, pszTrack, 0 );
		return;
	}
	else if (Q_stricmp(pszCommand, "stop") == 0)
	{
		S_StopBackgroundTrack();
		return;
	}
}


/* 
================== 
CL_ScreenshotGetName
================== 
*/  
void CL_ScreenshotGetName( int lastnum, char *filename )
{
	if( lastnum < 0 || lastnum > 9999 )
	{
		// bound
		Q_sprintf( filename, "scrshots/%s/!error.bmp", clgame.mapname );
		return;
	}

	Q_sprintf( filename, "scrshots/%s_shot%04d.bmp", clgame.mapname, lastnum );
}

/* 
================== 
CL_SnapshotGetName
================== 
*/  
qboolean CL_SnapshotGetName( int lastnum, char *filename )
{
	if( lastnum < 0 || lastnum > 9999 )
	{
		MsgDev( D_ERROR, "unable to write snapshot\n" );
		FS_AllowDirectPaths( false );
		return false;
	}

	Q_sprintf( filename, "%s_%04d.bmp", clgame.mapname, lastnum );

	return true;
}

/* 
============================================================================== 
 
			SCREEN SHOTS 
 
============================================================================== 
*/
/* 
================== 
CL_ScreenShot_f

normal screenshot
================== 
*/
void CL_ScreenShot_f( void ) 
{
	int	i;
	string	checkname;

	if( gl_overview->integer == 1 )
	{
		// special case for write overview image and script file
		Q_snprintf( cls.shotname, sizeof( cls.shotname ), "overviews/%s.bmp", clgame.mapname );
		cls.scrshot_action = scrshot_mapshot; // build new frame for mapshot
	}
	else
	{
		// scan for a free filename
		for( i = 0; i < 9999; i++ )
		{
			CL_ScreenshotGetName( i, checkname );
			if( !FS_FileExists( checkname, false ))
				break;
		}

		Q_strncpy( cls.shotname, checkname, sizeof( cls.shotname ));
		cls.scrshot_action = scrshot_normal; // build new frame for screenshot
	}

	cls.envshot_vieworg = NULL; // no custom view
	cls.envshot_viewsize = 0;
}

/* 
================== 
CL_SnapShot_f

save screenshots into root dir
================== 
*/
void CL_SnapShot_f( void ) 
{
	int	i;
	string	checkname;

	if( gl_overview->integer == 1 )
	{
		// special case for write overview image and script file
		Q_snprintf( cls.shotname, sizeof( cls.shotname ), "overviews/%s.bmp", clgame.mapname );
		cls.scrshot_action = scrshot_mapshot; // build new frame for mapshot
	}
	else
	{
		FS_AllowDirectPaths( true );

		// scan for a free filename
		for( i = 0; i < 9999; i++ )
		{
			if( !CL_SnapshotGetName( i, checkname ))
				return;	// no namespace

			if( !FS_FileExists( checkname, false ))
				break;
		}

		FS_AllowDirectPaths( false );
		Q_strncpy( cls.shotname, checkname, sizeof( cls.shotname ));
		cls.scrshot_action = scrshot_snapshot; // build new frame for screenshot
	}

	cls.envshot_vieworg = NULL; // no custom view
	cls.envshot_viewsize = 0;
}

/* 
================== 
CL_EnvShot_f

cubemap view
================== 
*/
void CL_EnvShot_f( void )
{
	if( Cmd_Argc() < 2 )
	{
		Msg( "Usage: envshot <shotname>\n" );
		return;
	}

	Q_sprintf( cls.shotname, "gfx/env/%s", Cmd_Argv( 1 ));
	cls.scrshot_action = scrshot_envshot;	// build new frame for envshot
	cls.envshot_vieworg = NULL; // no custom view
	cls.envshot_viewsize = 0;
}

/* 
================== 
CL_SkyShot_f

skybox view
================== 
*/
void CL_SkyShot_f( void )
{
	if( Cmd_Argc() < 2 )
	{
		Msg( "Usage: skyshot <shotname>\n" );
		return;
	}

	Q_sprintf( cls.shotname, "gfx/env/%s", Cmd_Argv( 1 ));
	cls.scrshot_action = scrshot_skyshot;	// build new frame for skyshot
	cls.envshot_vieworg = NULL; // no custom view
	cls.envshot_viewsize = 0;
}

/* 
================== 
CL_LevelShot_f

splash logo while map is loading
================== 
*/ 
void CL_LevelShot_f( void )
{
	size_t	ft1, ft2;
	string	filename;

	if( cls.scrshot_request != scrshot_plaque ) return;
	cls.scrshot_request = scrshot_inactive;

	// check for existence
	if( cls.demoplayback && ( cls.demonum != -1 ))
	{
		Q_sprintf( cls.shotname, "levelshots/%s_%s.bmp", cls.demoname, glState.wideScreen ? "16x9" : "4x3" );
		Q_snprintf( filename, sizeof( filename ), "demos/%s.dem", cls.demoname );

		// make sure that levelshot is newer than demo
		ft1 = FS_FileTime( filename, false );
		ft2 = FS_FileTime( cls.shotname, true );
	}
	else if( cl.worldmodel->name )
	{
		Q_sprintf( cls.shotname, "levelshots/%s_%s.bmp", clgame.mapname, glState.wideScreen ? "16x9" : "4x3" );

		// make sure that levelshot is newer than bsp
		ft1 = FS_FileTime( cl.worldmodel->name, false );
		ft2 = FS_FileTime( cls.shotname, true );
	}
	else ft1 = ft2 = 0;

	// missing levelshot or level newer than levelshot
	if( ft2 == (size_t)-1 || ft1 > ft2 )
		cls.scrshot_action = scrshot_plaque;	// build new frame for levelshot
	else cls.scrshot_action = scrshot_inactive;	// disable - not needs
}

/* 
================== 
CL_SaveShot_f

mini-pic in loadgame menu
================== 
*/ 
void CL_SaveShot_f( void )
{
	if( Cmd_Argc() < 2 )
	{
		Msg( "Usage: saveshot <savename>\n" );
		return;
	}

	Q_sprintf( cls.shotname, "save/%s.bmp", Cmd_Argv( 1 ));
	cls.scrshot_action = scrshot_savegame;	// build new frame for saveshot
}

/* 
================== 
CL_DemoShot_f

mini-pic in playdemo menu
================== 
*/ 
void CL_DemoShot_f( void )
{
	if( Cmd_Argc() < 2 )
	{
		Msg( "Usage: demoshot <demoname>\n" );
		return;
	}

	Q_sprintf( cls.shotname, "demos/%s.bmp", Cmd_Argv( 1 ));
	cls.scrshot_action = scrshot_demoshot; // build new frame for demoshot
}

/*
==============
CL_DeleteDemo_f

==============
*/
void CL_DeleteDemo_f( void )
{
	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: killdemo <name>\n" );
		return;
	}

	if( cls.demorecording && !Q_stricmp( cls.demoname, Cmd_Argv( 1 )))
	{
		Msg( "Can't delete %s - recording\n", Cmd_Argv( 1 ));
		return;
	}

	// delete save and saveshot
	FS_Delete( va( "demos/%s.dem", Cmd_Argv( 1 )));
	FS_Delete( va( "demos/%s.bmp", Cmd_Argv( 1 )));
}

/*
=================
CL_SetSky_f

Set a specified skybox (only for local clients)
=================
*/
void CL_SetSky_f( void )
{
	if( Cmd_Argc() < 2 )
	{
		Msg( "Usage: skyname <shadername>\n" );
		return;
	}

	R_SetupSky( Cmd_Argv( 1 ));
}

/*
================
SCR_TimeRefresh_f

timerefresh [noflip]
================
*/
void SCR_TimeRefresh_f( void )
{
	int	i;
	double	start, stop;
	double	time;

	if( cls.state != ca_active )
		return;

	start = Sys_DoubleTime();

	if( Cmd_Argc() == 2 )
	{	
		// run without page flipping
		R_BeginFrame( false );
		for( i = 0; i < 128; i++ )
		{
			cl.refdef.viewangles[1] = i / 128.0 * 360.0f;
			R_RenderFrame( &cl.refdef, true );
		}
		R_EndFrame();
	}
	else
	{
		for( i = 0; i < 128; i++ )
		{
			cl.refdef.viewangles[1] = i / 128.0 * 360.0f;

			R_BeginFrame( true );
			R_RenderFrame( &cl.refdef, true );
			R_EndFrame();
		}
	}

	stop = Sys_DoubleTime ();
	time = (stop - start);
	Msg( "%f seconds (%f fps)\n", time, 128 / time );
}

/*
=============
SCR_Viewpos_f

viewpos (level-designer helper)
=============
*/
void SCR_Viewpos_f( void )
{
	Msg( "org ( %g %g %g )\n", cl.refdef.vieworg[0], cl.refdef.vieworg[1], cl.refdef.vieworg[2] );
	Msg( "ang ( %g %g %g )\n", cl.refdef.viewangles[0], cl.refdef.viewangles[1], cl.refdef.viewangles[2] );
}

#endif //XASH_DEDICATED
