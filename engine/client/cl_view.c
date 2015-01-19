/*
cl_view.c - player rendering positioning
Copyright (C) 2009 Uncle Mike

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
#include "entity_types.h"
#include "gl_local.h"
#include "vgui_draw.h"

/*
===============
V_SetupRefDef

update refdef values each frame
===============
*/
void V_SetupRefDef( void )
{
	cl_entity_t	*clent;
	int		size;
	int		sb_lines;

	clent = CL_GetLocalPlayer ();

	clgame.entities->curstate.scale = clgame.movevars.waveHeight;
	VectorCopy( cl.frame.local.client.punchangle, cl.refdef.punchangle );
	clgame.viewent.curstate.modelindex = cl.frame.local.client.viewmodel;
	clgame.viewent.model = Mod_Handle( clgame.viewent.curstate.modelindex );
	clgame.viewent.curstate.number = cl.playernum + 1;
	clgame.viewent.curstate.entityType = ET_NORMAL;
	clgame.viewent.index = cl.playernum + 1;

	cl.refdef.movevars = &clgame.movevars;
	cl.refdef.onground = ( cl.frame.local.client.flags & FL_ONGROUND ) ? 1 : 0;
	cl.refdef.health = cl.frame.local.client.health;
	cl.refdef.playernum = cl.playernum;
	cl.refdef.max_entities = clgame.maxEntities;
	cl.refdef.maxclients = cl.maxclients;
	cl.refdef.time = cl.time;
	cl.refdef.frametime = cl.time - cl.oldtime;
	cl.refdef.demoplayback = cls.demoplayback;
	cl.refdef.smoothing = cl_smooth->integer;
	cl.refdef.viewsize = scr_viewsize->integer;
	cl.refdef.waterlevel = cl.frame.local.client.waterlevel;		
	cl.refdef.onlyClientDraw = 0;	// reset clientdraw
	cl.refdef.hardware = true;	// always true
	cl.refdef.spectator = (clent->curstate.spectator != 0);
	cl.refdef.nextView = 0;

	SCR_AddDirtyPoint( 0, 0 );
	SCR_AddDirtyPoint( scr_width->integer - 1, scr_height->integer - 1 );

	if( cl.refdef.viewsize >= 120 )
		sb_lines = 0;		// no status bar at all
	else if( cl.refdef.viewsize >= 110 )
		sb_lines = 24;		// no inventory
	else sb_lines = 48;

	size = min( scr_viewsize->integer, 100 );

	cl.refdef.viewport[2] = scr_width->integer * size / 100;
	cl.refdef.viewport[3] = scr_height->integer * size / 100;

	if( cl.refdef.viewport[3] > scr_height->integer - sb_lines )
		cl.refdef.viewport[3] = scr_height->integer - sb_lines;
	if( cl.refdef.viewport[3] > scr_height->integer )
		cl.refdef.viewport[3] = scr_height->integer;

	cl.refdef.viewport[0] = (scr_width->integer - cl.refdef.viewport[2]) / 2;
	cl.refdef.viewport[1] = (scr_height->integer - sb_lines - cl.refdef.viewport[3]) / 2;

	// calc FOV
	cl.refdef.fov_x = cl.data.fov; // this is a final fov value
	cl.refdef.fov_y = V_CalcFov( &cl.refdef.fov_x, cl.refdef.viewport[2], cl.refdef.viewport[3] );

	// adjust FOV for widescreen
	if( glState.wideScreen && r_adjust_fov->integer )
		V_AdjustFov( &cl.refdef.fov_x, &cl.refdef.fov_y, cl.refdef.viewport[2], cl.refdef.viewport[3], false );

	if( CL_IsPredicted( ) && !cl.refdef.demoplayback )
	{	
		VectorCopy( cl.predicted_origin, cl.refdef.simorg );
		VectorCopy( cl.predicted_velocity, cl.refdef.simvel );
		VectorCopy( cl.predicted_viewofs, cl.refdef.viewheight );
	}
	else
	{
		VectorCopy( cl.frame.local.client.origin, cl.refdef.simorg );
		VectorCopy( cl.frame.local.client.view_ofs, cl.refdef.viewheight );
		VectorCopy( cl.frame.local.client.velocity, cl.refdef.simvel );
	}
}

/*
===============
V_SetupOverviewState

Get initial overview values
===============
*/
void V_SetupOverviewState( void )
{
	ref_overview_t	*ov = &clgame.overView;
	float		mapAspect, screenAspect, aspect;

	ov->rotated = ( world.size[1] <= world.size[0] ) ? true : false;

	// calculate nearest aspect
	mapAspect = world.size[!ov->rotated] / world.size[ov->rotated];
	screenAspect = (float)glState.width / (float)glState.height;
	aspect = max( mapAspect, screenAspect );

	ov->zNear = world.maxs[2];
	ov->zFar = world.mins[2];
	ov->flZoom = ( 8192.0f / world.size[ov->rotated] ) / aspect;

	VectorAverage( world.mins, world.maxs, ov->origin );
}

/*
===============
V_WriteOverviewScript

Create overview scrip file
===============
*/
void V_WriteOverviewScript( void )
{
	ref_overview_t	*ov = &clgame.overView;
	string		filename;
	file_t		*f;

	Q_snprintf( filename, sizeof( filename ), "overviews/%s.txt", clgame.mapname );

	f = FS_Open( filename, "w", false );
	if( !f ) return;

	FS_Printf( f, "// overview description file for %s.bsp\n\n", clgame.mapname );
	FS_Print( f, "global\n{\n" );
	FS_Printf( f, "\tZOOM\t%.2f\n", ov->flZoom );
	FS_Printf( f, "\tORIGIN\t%.f\t%.f\t%.f\n", ov->origin[0], ov->origin[1], ov->zFar + 1 );
	FS_Printf( f, "\tROTATED\t%i\n", ov->rotated ? 1 : 0 );
	FS_Print( f, "}\n\nlayer\n{\n" );
	FS_Printf( f, "\tIMAGE\t\"overviews/%s.bmp\"\n", clgame.mapname );
	FS_Printf( f, "\tHEIGHT\t%.f\n", ov->zFar + 1 );	// ???
	FS_Print( f, "}\n" );

	FS_Close( f );
}

/*
===============
V_ProcessOverviewCmds

Transform user movement into overview adjust
===============
*/
void V_ProcessOverviewCmds( usercmd_t *cmd )
{
	ref_overview_t	*ov = &clgame.overView;
	int		sign = 1;

	if( !gl_overview->integer ) return;

	if( ov->flZoom < 0.0f ) sign = -1;

	if( cmd->upmove > 0.0f ) ov->zNear += 1.0f;
	else if( cmd->upmove < 0.0f ) ov->zNear -= 1.0f;

	if( cmd->buttons & IN_JUMP ) ov->zFar += 1.0f;
	else if( cmd->buttons & IN_DUCK ) ov->zFar -= 1.0f;

	if( cmd->buttons & IN_FORWARD ) ov->origin[ov->rotated] -= sign * 1.0f;
	else if( cmd->buttons & IN_BACK ) ov->origin[ov->rotated] += sign * 1.0f;

	if( ov->rotated )
	{
		if( cmd->buttons & ( IN_RIGHT|IN_MOVERIGHT ))
			ov->origin[0] -= sign * 1.0f;
		else if( cmd->buttons & ( IN_LEFT|IN_MOVELEFT ))
			ov->origin[0] += sign * 1.0f;
	}
	else
	{
		if( cmd->buttons & ( IN_RIGHT|IN_MOVERIGHT ))
			ov->origin[1] += sign * 1.0f;
		else if( cmd->buttons & ( IN_LEFT|IN_MOVELEFT ))
			ov->origin[1] -= sign * 1.0f;
	}

	if( cmd->buttons & IN_ATTACK ) ov->flZoom += 0.01f;
	else if( cmd->buttons & IN_ATTACK2 ) ov->flZoom -= 0.01f;

	if( ov->flZoom == 0.0f ) ov->flZoom = 0.0001f; // to prevent disivion by zero
}

/*
===============
V_MergeOverviewRefdef

merge refdef with overview settings
===============
*/
void V_MergeOverviewRefdef( ref_params_t *fd )
{
	ref_overview_t	*ov = &clgame.overView;
	float		aspect;
	float		size_x, size_y;
	vec2_t		mins, maxs;

	if( !gl_overview->integer ) return;

	// NOTE: Xash3D may use 16:9 or 16:10 aspects
	aspect = (float)glState.width / (float)glState.height;

	size_x = fabs( 8192.0f / ov->flZoom );
	size_y = fabs( 8192.0f / (ov->flZoom * aspect ));

	// compute rectangle
	ov->xLeft = -(size_x / 2);
	ov->xRight = (size_x / 2);
	ov->xTop = -(size_y / 2);
	ov->xBottom = (size_y / 2);

	if( gl_overview->integer == 1 )
	{
		Con_NPrintf( 0, " Overview: Zoom %.2f, Map Origin (%.2f, %.2f, %.2f), Z Min %.2f, Z Max %.2f, Rotated %i\n",
		ov->flZoom, ov->origin[0], ov->origin[1], ov->origin[2], ov->zNear, ov->zFar, ov->rotated );
	}

	VectorCopy( ov->origin, fd->vieworg );
	fd->vieworg[2] = ov->zFar + ov->zNear;
	Vector2Copy( fd->vieworg, mins );
	Vector2Copy( fd->vieworg, maxs );

	mins[!ov->rotated] += ov->xLeft;
	maxs[!ov->rotated] += ov->xRight;
	mins[ov->rotated] += ov->xTop;
	maxs[ov->rotated] += ov->xBottom;

	fd->viewangles[0] = 90.0f;
	fd->viewangles[1] = 90.0f;
	fd->viewangles[2] = (ov->rotated) ? (ov->flZoom < 0.0f) ? 180.0f : 0.0f : (ov->flZoom < 0.0f) ? -90.0f : 90.0f;

	Mod_SetOrthoBounds( mins, maxs );
}

/*
===============
V_ProcessShowTexturesCmds

navigate around texture atlas
===============
*/
void V_ProcessShowTexturesCmds( usercmd_t *cmd )
{
	static int	oldbuttons;
	int		changed;
	int		pressed, released;

	if( !gl_showtextures->integer ) return;

	changed = (oldbuttons ^ cmd->buttons);
	pressed =  changed & cmd->buttons;
	released = changed & (~cmd->buttons);

	if( released & ( IN_RIGHT|IN_MOVERIGHT ))
		Cvar_SetFloat( "r_showtextures", gl_showtextures->integer + 1 );
	if( released & ( IN_LEFT|IN_MOVELEFT ))
		Cvar_SetFloat( "r_showtextures", max( 1, gl_showtextures->integer - 1 ));
	oldbuttons = cmd->buttons;
}

/*
===============
V_CalcRefDef

sets cl.refdef view values
===============
*/
void V_CalcRefDef( void )
{
	R_Set2DMode( false );
	tr.framecount++;	// g-cont. keep actual frame for all viewpasses

	do
	{
		clgame.dllFuncs.pfnCalcRefdef( &cl.refdef );
		V_MergeOverviewRefdef( &cl.refdef );
		R_RenderFrame( &cl.refdef, true );
		cl.refdef.onlyClientDraw = false;
	} while( cl.refdef.nextView );

	// Xash3D extension. draw debug triangles on a server
	SV_DrawDebugTriangles ();

	SCR_AddDirtyPoint( cl.refdef.viewport[0], cl.refdef.viewport[1] );
	SCR_AddDirtyPoint( cl.refdef.viewport[0] + cl.refdef.viewport[2] - 1, cl.refdef.viewport[1] + cl.refdef.viewport[3] - 1 );
}

//============================================================================

/*
==================
V_RenderView

==================
*/
void V_RenderView( void )
{
	if( !cl.video_prepped || ( UI_IsVisible() && !cl.background ))
		return; // still loading

	if( cl.frame.valid && ( cl.force_refdef || !cl.refdef.paused ))
	{
		cl.force_refdef = false;

		R_ClearScene ();
		CL_AddEntities ();
		V_SetupRefDef ();
	}

	V_CalcRefDef ();
}

/*
==================
V_PreRender

==================
*/
qboolean V_PreRender( void )
{
	// too early
	if( !glw_state.initialized )
		return false;

	if( host.state == HOST_NOFOCUS )
		return false;

	if( host.state == HOST_SLEEP )
		return false;

	// if the screen is disabled (loading plaque is up)
	if( cls.disable_screen )
	{
		if(( host.realtime - cls.disable_screen ) > cl_timeout->value )
		{
			MsgDev( D_NOTE, "V_PreRender: loading plaque timed out.\n" );
			cls.disable_screen = 0.0f;
		}
		return false;
	}
	
	R_BeginFrame( !cl.refdef.paused );

	return true;
}

/*
==================
V_PostRender

==================
*/
void V_PostRender( void )
{
	qboolean	draw_2d = false;

	R_Set2DMode( true );

	if( cls.state == ca_active )
	{
		SCR_TileClear();
		CL_DrawHUD( CL_ACTIVE );
		VGui_Paint();
	}

	switch( cls.scrshot_action )
	{
	case scrshot_inactive:
	case scrshot_normal:
	case scrshot_snapshot:
		draw_2d = true;
		break;
	}

	if( draw_2d )
	{
		SCR_RSpeeds();
		SCR_NetSpeeds();
		SCR_DrawFPS();
		SV_DrawOrthoTriangles();
		CL_DrawDemoRecording();
		R_ShowTextures();
		CL_DrawHUD( CL_CHANGELEVEL );
		Con_DrawConsole();
		UI_UpdateMenu( host.realtime );
		Con_DrawVersion();
		Con_DrawDebug(); // must be last
		S_ExtraUpdate();
	}

	SCR_MakeScreenShot();
	R_EndFrame();
}