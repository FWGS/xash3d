/*
cl_main.c - client main loop
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

#ifndef XASH_DEDICATED

#include "common.h"
#include "client.h"
#include "net_encode.h"
#include "cl_tent.h"
#include "gl_local.h"
#include "input.h"
#include "touch.h"
#include "kbutton.h"
#include "vgui_draw.h"
#include "library.h"

#define MAX_TOTAL_CMDS		16
#define MIN_CMD_RATE		10.0
#define MAX_CMD_BUFFER		4000
#define CONNECTION_PROBLEM_TIME	15.0	// 15 seconds

void CL_InternetServers_f( void );

convar_t	*r_oldparticles;
convar_t	*rcon_client_password;
convar_t	*rcon_address;

convar_t	*cl_timeout;
convar_t	*cl_predict;
convar_t	*cl_showfps;
convar_t	*cl_showpos;
convar_t	*cl_nodelta;
convar_t	*cl_crosshair;
convar_t	*cl_cmdbackup;
convar_t	*cl_showerror;
convar_t    *cl_nosmooth;
convar_t    *cl_smoothtime;
convar_t	*cl_draw_particles;
convar_t	*cl_lightstyle_lerping;
convar_t	*cl_idealpitchscale;
convar_t	*cl_solid_players;
convar_t	*cl_draw_beams;
convar_t	*cl_cmdrate;
convar_t	*cl_interp;
convar_t	*cl_allow_fragment;
convar_t	*cl_lw;
convar_t	*cl_trace_events;
convar_t	*cl_trace_stufftext;
convar_t    *cl_trace_messages;
convar_t	*cl_charset;
convar_t	*cl_sprite_nearest;
convar_t	*cl_updaterate;
convar_t	*cl_nat;
convar_t	*hud_scale;
convar_t	*cl_maxpacket;
convar_t	*cl_maxpayload;

convar_t    *r_bmodelinterp;

convar_t	*hud_utf8;

convar_t    *ui_renderworld;
//
// userinfo
//
convar_t	*name;
convar_t	*model;
convar_t	*topcolor;
convar_t	*bottomcolor;
convar_t	*rate;
convar_t	*hltv;

client_t		cl;
client_static_t	cls;
clgame_static_t	clgame;

//======================================================================
qboolean CL_Active( void )
{
	return ( cls.state == ca_active );
}

//======================================================================
qboolean CL_IsInGame( void )
{
	if( Host_IsDedicated() ) return true;	// always active for dedicated servers
	if( CL_GetMaxClients() > 1 ) return true;	// always active for multiplayer
	return ( cls.key_dest == key_game );		// active if not menu or console
}

qboolean CL_IsInMenu( void )
{
	return ( cls.key_dest == key_menu );
}

qboolean CL_IsInConsole( void )
{
	return ( cls.key_dest == key_console );
}

qboolean CL_IsIntermission( void )
{
	return cl.refdef.intermission;
}

qboolean CL_IsPlaybackDemo( void )
{
	return cls.demoplayback;
}

qboolean CL_DisableVisibility( void )
{
	return cls.envshot_disable_vis;
}

qboolean CL_IsBackgroundDemo( void )
{
	return ( cls.demoplayback && cls.demonum != -1 );
}

qboolean CL_IsBackgroundMap( void )
{
	return ( cl.background && !cls.demoplayback );
}

/*
===============
CL_ChangeGame

This is experiment. Use with precaution
===============
*/
qboolean CL_ChangeGame( const char *gamefolder, qboolean bReset )
{
	if( Host_IsDedicated() )
		return false;

	if( Q_stricmp( host.gamefolder, gamefolder ))
	{
		kbutton_t	*mlook, *jlook;
		qboolean	mlook_active = false, jlook_active = false;
		string	mapname, maptitle;
		int	maxEntities;

		mlook = (kbutton_t *)clgame.dllFuncs.KB_Find( "in_mlook" );
		jlook = (kbutton_t *)clgame.dllFuncs.KB_Find( "in_jlook" );

		if( mlook && ( mlook->state & 1 ))
			mlook_active = true;

		if( jlook && ( jlook->state & 1 ))
			jlook_active = true;

		// so reload all images (remote connect)
		Mod_ClearAll( true );
		R_ShutdownImages();
		FS_LoadGameInfo( (bReset) ? host.gamefolder : gamefolder );
		R_InitImages();

		// save parms
		maxEntities = clgame.maxEntities;
		Q_strncpy( mapname, clgame.mapname, MAX_STRING );
		Q_strncpy( maptitle, clgame.maptitle, MAX_STRING );

		Com_ResetLibraryError();
		if( !CL_LoadProgs( va( "%s/%s", GI->dll_path, GI->client_lib)))
			Sys_Warn( "Can't initialize client library\n%s", Com_GetLibraryError() );

		// restore parms
		clgame.maxEntities = maxEntities;
		Q_strncpy( clgame.mapname, mapname, MAX_STRING );
		Q_strncpy( clgame.maptitle, maptitle, MAX_STRING );

		// invalidate fonts so we can reloading them again
		Q_memset( &cls.creditsFont, 0, sizeof( cls.creditsFont ));
		SCR_InstallParticlePalette();
		SCR_LoadCreditsFont();
		Con_InvalidateFonts();

		SCR_RegisterTextures ();
		CL_FreeEdicts ();
		SCR_VidInit ();

		if( cls.key_dest == key_game ) // restore mouse state
			clgame.dllFuncs.IN_ActivateMouse();

		// restore mlook state
		if( mlook_active ) Cmd_ExecuteString( "+mlook\n", src_command );
		if( jlook_active ) Cmd_ExecuteString( "+jlook\n", src_command );
		return true;
	}

	return false;
}

/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
static float CL_LerpPoint( void )
{
	float	f, frac;

	f = cl.mtime[0] - cl.mtime[1];

	if( !f || SV_Active( ))
	{
		cl.time = cl.mtime[0];
		return 1.0f;
	}

	if( f > 0.1f )
	{
		// dropped packet, or start of demo
		cl.mtime[1] = cl.mtime[0] - 0.1f;
		f = 0.1f;
	}

	frac = ( cl.time - cl.mtime[1] ) / f;

	if( frac < 0 )
	{
		if( frac < -0.01f )
			cl.time = cl.mtime[1];
		frac = 0.0f;
	}
	else if( frac > 1.0f )
	{
		if( frac > 1.01f )
			cl.time = cl.mtime[0];
		frac = 1.0f;
	}

	return frac;
}

/*
=================
CL_ComputePacketLoss

=================
*/
void CL_ComputePacketLoss( void )
{
	int	i, frm;
	frame_t	*frame;
	int	count = 0;
	int	lost = 0;

	if( host.realtime < cls.packet_loss_recalc_time )
		return;

	// recalc every second
	cls.packet_loss_recalc_time = host.realtime + 1.0;

	// compuate packet loss
	for( i = cls.netchan.incoming_sequence - CL_UPDATE_BACKUP + 1; i <= cls.netchan.incoming_sequence; i++ )
	{
		frm = i;
		frame = &cl.frames[frm & CL_UPDATE_MASK];

		if( frame->receivedtime == -1.0 )
			lost++;
		count++;
	}

	if( count <= 0 ) cls.packet_loss = 0.0f;
	else cls.packet_loss = ( 100.0f * (float)lost ) / (float)count;
}

/*
=================
CL_UpdateFrameLerp

=================
*/
void CL_UpdateFrameLerp( void )
{
	// not in server yet, no entities to redraw
	if( cls.state != ca_active ) return;

	// we haven't received our first valid update from the server.
	if( !cl.frame.valid || !cl.validsequence )
		return;

	// compute last interpolation amount
	cl.commands[( cls.netchan.outgoing_sequence - 1 ) & CL_UPDATE_MASK].frame_lerp = cl.lerpFrac;
}

/*
====================
CL_CheckUpdateRate

Check is current cl_updaterate valid
====================
*/
void CL_CheckUpdateRate()
{
	const float cl_updaterate_min = 10, cl_updaterate_max = 102;

	if( cl_updaterate->value < cl_updaterate_min )
	{
		Cvar_SetFloat( "cl_updaterate", cl_updaterate_min );
		MsgDev( D_INFO, "cl_updaterate minimum is %f, resetting to default (%f)\n", cl_updaterate_min, cl_updaterate_min );
	}
	else if( cl_updaterate->value > cl_updaterate_max )
	{
		Cvar_SetFloat( "cl_updaterate", cl_updaterate_max );
		MsgDev( D_INFO, "cl_updaterate maximum is %f, resetting to maximum (%f)\n", cl_updaterate_max, cl_updaterate_max );
	}
}

/*
====================
CL_DriftInterpAmount

Smoothly drift lerp_msec
====================
*/
int CL_DriftInterpolationAmount( float interp_sec )
{
	static float interpolationAmount = 0.1f;

	if( interp_sec != interpolationAmount )
	{
		float maxmove = host.frametime * 0.05;
		float diff = interp_sec - interpolationAmount;

		if( diff > 0 )
		{
			if( diff > maxmove )
				diff = maxmove;
		}
		else
		{
			diff = -diff;

			if( diff > maxmove )
				diff = -maxmove;
		}

		interpolationAmount += diff;
	}

	return bound( 0, interpolationAmount * 1000.0, 100 );
}

/*
====================
CL_ComputeClientInterpAmount

Check update rate, ex_interp value and calculate lerp_msec
====================
*/
void CL_ComputeClientInterpAmount( usercmd_t *cmd )
{
	float interp_min, interp_max;

	CL_CheckUpdateRate();

	interp_max = 0.2f; // hltv value
	interp_min = max( 0.001, 1 / cl_updaterate->value );

	if( cl_interp->value + 0.001 < interp_min )
	{
		MsgDev( D_NOTE, "ex_interp forced up to %.f msec\n", interp_min * 1000 );
		Cvar_SetFloat( "ex_interp", interp_min );
	}
	else if( cl_interp->value - 0.001 > interp_max )
	{
		MsgDev( D_NOTE, "ex_interp forced down to %.f msec\n", interp_max * 1000 );
		Cvar_SetFloat( "ex_interp", interp_max );
	}

	cmd->lerp_msec = CL_DriftInterpolationAmount( cl_interp->value );
}


/*
=======================================================================

CLIENT MOVEMENT COMMUNICATION

=======================================================================
*/
/*
=================
CL_CreateCmd
=================
*/
void CL_CreateCmd( void )
{
	usercmd_t		cmd = { 0 };
	runcmd_t		*pcmd;
	color24		color;
	vec3_t		angles;
	qboolean		active;
	int		i, ms;

	ms = host.frametime * 1000;
	if( ms > 250 ) ms = 100;	// time was unreasonable
	else if( ms <= 0 ) ms = 1; // keep time an actual

	// build list of all solid entities per next frame (exclude clients)
	CL_SetSolidEntities ();
	CL_PushPMStates();
	CL_SetSolidPlayers ( cl.playernum );
	VectorCopy( cl.refdef.cl_viewangles, angles );

	// write cdata
	VectorCopy( cl.frame.client.origin, cl.data.origin );
	VectorCopy( cl.refdef.cl_viewangles, cl.data.viewangles );
	cl.data.iWeaponBits = cl.frame.client.weapons;
	cl.data.fov = cl.scr_fov;

	if( clgame.dllFuncs.pfnUpdateClientData( &cl.data, cl.time ) )
	{
		// grab changes if successful
		VectorCopy( cl.data.viewangles, cl.refdef.cl_viewangles );
		cl.scr_fov = cl.data.fov;
	}

	// allways dump the first ten messages,
	// because it may contain leftover inputs
	// from the last level
	if( ++cl.movemessages <= 10 )
	{
		if( !cls.demoplayback )
		{
			cl.refdef.cmd = &cl.commands[cls.netchan.outgoing_sequence & CL_UPDATE_MASK].cmd;
			*cl.refdef.cmd = cmd;
		}
		CL_PopPMStates();
		return;
	}

	// message we are constructing.
	i = cls.netchan.outgoing_sequence & CL_UPDATE_MASK;

	pcmd = &cl.commands[i];

	pcmd->senttime = cls.demoplayback ? 0.0 : host.realtime;
	memset( &pcmd->cmd, 0, sizeof( pcmd->cmd ));
	pcmd->receivedtime = -1.0;
	pcmd->processedfuncs = false;
	pcmd->heldback = false;
	pcmd->sendsize = 0;

	active = ( cls.state == ca_active && !cl.refdef.paused && !cls.demoplayback );

#ifdef XASH_SDL
	if( m_ignore->integer )
	{
		int x, y;
		SDL_GetRelativeMouseState( &x, &y );
	}
#endif

	clgame.dllFuncs.CL_CreateMove( cl.time - cl.oldtime, &pcmd->cmd, active );
	CL_PopPMStates();


	// after command generated in client,
	// add motion events from engine controls
	IN_EngineAppendMove( host.frametime, &pcmd->cmd, active);

	R_LightForPoint( cl.frame.client.origin, &color, false, false, 128.0f );
	cmd.lightlevel = (color.r + color.g + color.b) / 3;

	// never let client.dll calc frametime for player
	// because is potential backdoor for cheating
	pcmd->cmd.msec = ms;
	CL_ComputeClientInterpAmount( &pcmd->cmd );

	V_ProcessOverviewCmds( &pcmd->cmd );
	V_ProcessShowTexturesCmds( &pcmd->cmd );

	if(( cl.background && !cls.demoplayback ) || gl_overview->integer || cls.changelevel )
	{
		VectorCopy( angles, cl.refdef.cl_viewangles );
		VectorCopy( angles, pcmd->cmd.viewangles );
		pcmd->cmd.msec = 0;
	}

	// demo always have commands
	// so don't overwrite them
	if( !cls.demoplayback ) cl.refdef.cmd = &pcmd->cmd;
}

void CL_WriteUsercmd( sizebuf_t *msg, int from, int to )
{
	usercmd_t	nullcmd;
	usercmd_t	*f, *t;

	ASSERT( from == -1 || ( from >= 0 && from < MULTIPLAYER_BACKUP ));
	ASSERT( to >= 0 && to < MULTIPLAYER_BACKUP );

	if( from == -1 )
	{
		Q_memset( &nullcmd, 0, sizeof( nullcmd ));
		f = &nullcmd;
	}
	else
	{
		f = &cl.commands[from].cmd;
	}

	t = &cl.commands[to].cmd;

	// write it into the buffer
	MSG_WriteDeltaUsercmd( msg, f, t );
}

/*
===================
CL_WritePacket

Create and send the command packet to the server
Including both the reliable commands and the usercmds
===================
*/
void CL_WritePacket( void )
{
	sizebuf_t		buf;
	qboolean		send_command = false;
	byte		data[MAX_CMD_BUFFER];
	int		i, from, to, key, size;
	int		numbackup = 2;
	int		numcmds;
	int		newcmds;
	int		cmdnumber;

	// don't send anything if playing back a demo
	if( cls.demoplayback || cls.state == ca_cinematic )
		return;

	if( cls.state == ca_disconnected || cls.state == ca_connecting )
		return;

	CL_ComputePacketLoss ();

#ifndef _DEBUG
	if( cl_cmdrate->value < MIN_CMD_RATE )
	{
		Cvar_SetFloat( "cl_cmdrate", MIN_CMD_RATE );
	}
#endif
	Q_memset( data, 0, MAX_CMD_BUFFER );
	BF_Init( &buf, "ClientData", data, sizeof( data ));

	// Determine number of backup commands to send along
	numbackup = bound( 0, cl_cmdbackup->integer, MAX_BACKUP_COMMANDS );
	if( cls.state == ca_connected ) numbackup = 0;

	// Check to see if we can actually send this command

	// In single player, send commands as fast as possible
	// Otherwise, only send when ready and when not choking bandwidth
	if(( cl.maxclients == 1 ) || ( NET_IsLocalAddress( cls.netchan.remote_address ) && !host_limitlocal->integer ))
		send_command = true;
	else if(( host.realtime >= cls.nextcmdtime ) && Netchan_CanPacket( &cls.netchan ))
		send_command = true;

	if( cl.force_send_usercmd )
	{
		send_command = true;
		cl.force_send_usercmd = false;
	}

	if(( cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged ) >= CL_UPDATE_MASK )
	{
		if(( host.realtime - cls.netchan.last_received ) > CONNECTION_PROBLEM_TIME )
		{
			Con_NPrintf( 1, "^3Warning:^1 Connection Problem^7\n" );
			cl.validsequence = 0;
		}
	}

	if( cl_nodelta->integer )
	{
		cl.validsequence = 0;
	}

	// send a userinfo update if needed
	if( userinfo->modified )
	{
		BF_WriteByte( &cls.netchan.message, clc_userinfo );
		BF_WriteString( &cls.netchan.message, Cvar_Userinfo( ));
	}

	if( send_command )
	{
		int	outgoing_sequence;

		if( cl_cmdrate->integer > 0 )
			cls.nextcmdtime = host.realtime + ( 1.0f / cl_cmdrate->value );
		else cls.nextcmdtime = host.realtime; // always able to send right away

		if( cls.lastoutgoingcommand == -1 )
		{
			outgoing_sequence = cls.netchan.outgoing_sequence;
			cls.lastoutgoingcommand = cls.netchan.outgoing_sequence;
		}
		else outgoing_sequence = cls.lastoutgoingcommand + 1;

		// begin a client move command
		BF_WriteByte( &buf, clc_move );

		// save the position for a checksum byte
		key = BF_GetRealBytesWritten( &buf );
		BF_WriteByte( &buf, 0 );

		// write packet lossage percentation
		BF_WriteByte( &buf, cls.packet_loss );

		// say how many backups we'll be sending
		BF_WriteByte( &buf, numbackup );

		// how many real commands have queued up
		newcmds = ( cls.netchan.outgoing_sequence - cls.lastoutgoingcommand );

		// put an upper/lower bound on this
		newcmds = bound( 0, newcmds, MAX_TOTAL_CMDS );
		if( cls.state == ca_connected ) newcmds = 0;

		BF_WriteByte( &buf, newcmds );

		numcmds = newcmds + numbackup;
		from = -1;

		for( i = numcmds - 1; i >= 0; i-- )
		{
			cmdnumber = ( cls.netchan.outgoing_sequence - i ) & CL_UPDATE_MASK;
			//if( i == 0 ) cl.commands[cmdnumber].processedfuncs = true; // only last cmd allow to run funcs

			to = cmdnumber;
			CL_WriteUsercmd( &buf, from, to );
			from = to;

			if( BF_CheckOverflow( &buf ))
				Host_Error( "CL_Move, overflowed command buffer (%i bytes)\n", MAX_CMD_BUFFER );
		}

		// calculate a checksum over the move commands
		size = BF_GetRealBytesWritten( &buf ) - key - 1;
		buf.pData[key] = CRC32_BlockSequence( buf.pData + key + 1, size, cls.netchan.outgoing_sequence );

		// message we are constructing.
		i = cls.netchan.outgoing_sequence & CL_UPDATE_MASK;

		// determine if we need to ask for a new set of delta's.
		if( cl.validsequence && (cls.state == ca_active) && !( cls.demorecording && cls.demowaiting ))
		{
			cl.delta_sequence = cl.validsequence;

			BF_WriteByte( &buf, clc_delta );
			BF_WriteByte( &buf, cl.validsequence & 0xFF );
		}
		else
		{
			// request delta compression of entities
			cl.delta_sequence = -1;
		}

		if( BF_CheckOverflow( &buf ))
		{
			Host_Error( "CL_Move, overflowed command buffer (%i bytes)\n", MAX_CMD_BUFFER );
		}

		// remember outgoing command that we are sending
		cls.lastoutgoingcommand = cls.netchan.outgoing_sequence;

		// update size counter for netgraph
		cl.commands[cls.netchan.outgoing_sequence & CL_UPDATE_MASK].sendsize = BF_GetNumBytesWritten( &buf );

		// composite the rest of the datagram..
		if( BF_GetNumBitsWritten( &cls.datagram ) <= BF_GetNumBitsLeft( &buf ))
			BF_WriteBits( &buf, BF_GetData( &cls.datagram ), BF_GetNumBitsWritten( &cls.datagram ));
		BF_Clear( &cls.datagram );

		// deliver the message (or update reliable)
		Netchan_TransmitBits( &cls.netchan, BF_GetNumBitsWritten( &buf ), BF_GetData( &buf ));
	}
	else
	{
		// mark command as held back so we'll send it next time
		cl.commands[cls.netchan.outgoing_sequence & CL_UPDATE_MASK].heldback = true;

		// increment sequence number so we can detect that we've held back packets.
		cls.netchan.outgoing_sequence++;
	}

	if( cls.demorecording )
	{
		// Back up one because we've incremented outgoing_sequence each frame by 1 unit
		cmdnumber = ( cls.netchan.outgoing_sequence - 1 ) & CL_UPDATE_MASK;
		CL_WriteDemoUserCmd( cmdnumber );
	}

	// update download/upload slider.
	Netchan_UpdateProgress( &cls.netchan );
}

/*
=================
CL_SendCmd

Called every frame to builds and sends a command packet to the server.
=================
*/
void CL_SendCmd( void )
{
	// we create commands even if a demo is playing,
	CL_CreateCmd();
	// clc_move, userinfo etc
	CL_WritePacket();

	// make sure what menu and CL_WritePacket catch changes
	userinfo->modified = false;
}

/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f( void )
{
	CL_Disconnect();
	Sys_Quit();
}

/*
================
CL_Drop

Called after an Host_Error was thrown
================
*/
void CL_Drop( void )
{
	if( cls.state == ca_uninitialized )
		return;
	CL_Disconnect();

	// This fixes crash in menu_playersetup after disconnecting from server
	CL_ClearEdicts();

	if( cls.need_save_config && !SV_Active() )
	{
		Cbuf_AddText( "host_writeconfig\n" );
		cls.need_save_config = false;
	}
}

/*
================
CL_TrySaveConfig_f

Connfiguration changed in menu, so schedule config update
================
*/
void CL_TrySaveConfig_f( void )
{
	if( cls.state <= ca_disconnected )
		Cbuf_AddText( "host_writeconfig\n" );
	else
		cls.need_save_config = true;
}

/*
=======================
CL_SendConnectPacket

We have gotten a challenge from the server, so try and
connect.
======================
*/
void CL_SendConnectPacket( void )
{
	netadr_t	adr;
	int	port;
	unsigned int extensions = 0;
	char useragent[MAX_INFO_STRING] = "";
	unsigned int input_devices = 0;

	if( !NET_StringToAdr( cls.servername, &adr ))
	{
		MsgDev( D_INFO, "CL_SendConnectPacket: bad server address\n");
		cls.connect_time = 0;
		return;
	}

	if( adr.port == 0 ) adr.port = BF_BigShort( PORT_SERVER );
	port = Cvar_VariableValue( "net_qport" );

	userinfo->modified = false;

	if( adr.type != NA_LOOPBACK )
	{
		qboolean huff = Cvar_VariableInteger( "cl_enable_compress" );
		if( huff )
			extensions |= NET_EXT_HUFF;

		if( Cvar_VariableInteger( "cl_enable_split" ) )
		{
			extensions |= NET_EXT_SPLIT;
			if( !huff && Cvar_VariableInteger( "cl_enable_splitcompress" ) )
				extensions |= NET_EXT_SPLITHUFF;
		}

		if( !m_ignore->integer )
			input_devices |= INPUT_DEVICE_MOUSE;

		if( touch_enable->integer )
			input_devices |= INPUT_DEVICE_TOUCH;

		if(  Cvar_VariableInteger( "joy_enable" ) && Cvar_VariableInteger( "joy_found" ) )
			input_devices |= INPUT_DEVICE_JOYSTICK;

		// lock input devices change
		Cvar_FullSet( "touch_enable", va( "%s", touch_enable->string ), touch_enable->flags | CVAR_READ_ONLY );
		Cvar_FullSet( "m_ignore", va( "%s", m_ignore->string ), m_ignore->flags | CVAR_READ_ONLY );
		Cvar_FullSet( "joy_enable", va( "%s", Cvar_VariableString( "joy_enable" ) ), CVAR_ARCHIVE | CVAR_READ_ONLY );


		Info_SetValueForKey( useragent, "d", va( "%d", input_devices ), sizeof( useragent ) );
		Info_SetValueForKey( useragent, "v", XASH_VERSION, sizeof( useragent ) );
		Info_SetValueForKey( useragent, "b", va( "%d", Q_buildnum() ), sizeof( useragent ) );
		Info_SetValueForKey( useragent, "o", Q_buildos(), sizeof( useragent ) );
		Info_SetValueForKey( useragent, "a", Q_buildarch(), sizeof( useragent ) );
		Info_SetValueForKey( useragent, "i", ID_GetMD5(), sizeof( useragent ) );
	}
	else
	{
		// reset to writable state
		Cvar_FullSet( "touch_enable", va( "%s", touch_enable->string ), touch_enable->flags & ~CVAR_READ_ONLY );
		Cvar_FullSet( "m_ignore", va( "%s", m_ignore->string ), m_ignore->flags & ~CVAR_READ_ONLY );
		Cvar_FullSet( "joy_enable", va( "%s", Cvar_VariableString( "joy_enable" ) ), CVAR_ARCHIVE );
	}

	Netchan_OutOfBandPrint( NS_CLIENT, adr, "connect %i %i %i \"%s\" %d %s\n", PROTOCOL_VERSION, port, cls.challenge, Cvar_Userinfo( ), extensions, useragent );
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend( void )
{
	netadr_t	adr;
	int res;

	if( cls.internetservers_wait )
		CL_InternetServers_f();

	// if the local server is running and we aren't then connect
	if( cls.state == ca_disconnected && ( SV_Active( ) ) )
	{
		cls.state = ca_connecting;
		cls.connect_time = host.realtime;
		Q_strncpy( cls.servername, "loopback", sizeof( cls.servername ) );
		cls.serveradr.type = NA_LOOPBACK;
		// we don't need a challenge on the localhost
		CL_SendConnectPacket();
		return;
	}

	// resend if we haven't gotten a reply yet
	if( cls.demoplayback || cls.state != ca_connecting )
		return;

	if(( host.realtime - cls.connect_time ) < 10.0f )
		return;

	res = NET_StringToAdrNB( cls.servername, &adr );

	if( !res )
	{
		MsgDev( D_ERROR, "CL_CheckForResend: bad server address\n" );
		cls.state = ca_disconnected;
		Q_memset( &cls.serveradr, 0, sizeof( cls.serveradr ) );
		return;
	}

	// blocked
	if( res == 2 )
	{
		cls.connect_time = MAX_HEARTBEAT;
		return;
	}

	if( adr.port == 0 ) adr.port = BF_BigShort( PORT_SERVER );
	cls.connect_time = host.realtime; // for retransmit requests

	// do not make non-blocking requests again
	cls.serveradr = adr;
	Q_strncpy( cls.servername, NET_AdrToString( adr ), sizeof( cls.servername ));

	MsgDev( D_NOTE, "Connecting to %s...\n", cls.servername );
	Netchan_OutOfBandPrint( NS_CLIENT, adr, "getchallenge\n" );
}

/*
================
CL_Connect_f

================
*/
void CL_Connect_f( void )
{
	string server;

	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: connect <server>\n" );
		return;
	}

	// default value 40000 ignored as we don't want to grow userinfo string
	if( ( cl_maxpacket->integer < 40000 ) && ( cl_maxpacket->integer > 99 ) )
	{
		cl_maxpacket->flags |= CVAR_USERINFO;
		userinfo->modified = true;
	}
	else if( cl_maxpacket->flags & CVAR_USERINFO )
	{
		cl_maxpacket->flags &= ~CVAR_USERINFO;
		userinfo->modified = true;
	}

	// allow override payload size for some bad networks
	if( ( cl_maxpayload->integer < 40000 ) && ( cl_maxpayload->integer > 99 ) )
	{
		cl_maxpayload->flags |= CVAR_USERINFO;
		userinfo->modified = true;
	}
	else if( cl_maxpayload->flags & CVAR_USERINFO )
	{
		cl_maxpayload->flags &= ~CVAR_USERINFO;
		userinfo->modified = true;
	}

	Q_strncpy( server, Cmd_Argv( 1 ), MAX_STRING );

	if( Host_ServerState())
	{
		// if running a local server, kill it and reissue
		Q_strncpy( host.finalmsg, "Server quit", MAX_STRING );
		SV_Shutdown( false );
	}

	NET_Config( true, !cl_nat->integer ); // allow remote

	Msg( "server %s\n", server );
	CL_Disconnect();

	HTTP_Clear_f();
	cls.state = ca_connecting;
	Q_strncpy( cls.servername, server, sizeof( cls.servername ));
	cls.connect_time = MAX_HEARTBEAT; // CL_CheckForResend() will fire immediately
}


/*
=====================
CL_Rcon_f

Send the rest of the command line over as
an unconnected command.
=====================
*/
void CL_Rcon_f( void )
{
	char	message[1024];
	netadr_t	to;
	int	i = 1;

	if( !rcon_client_password->string )
	{
		Msg( "You must set 'rcon_password' before issuing an rcon command.\n" );
		return;
	}

	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = 0;

	NET_Config( true, false );	// allow remote

	Q_strncat( message, "rcon ", sizeof( message ) );

	if( !rcon_client_password->string[0] )
	{
		// HACK: allow pass password with first argument
		// do not port this to new engine, it is better to fix on server side
		Q_strncat( message, Cmd_Argv( 1 ), sizeof( message ) );
		Q_strncat( message, " ", sizeof( message ) );
		i++;
	}
	else
	{
		Q_strncat( message, rcon_client_password->string, sizeof( message ) );
		Q_strncat( message, " ", sizeof( message ) );
	}

	for( ; i < Cmd_Argc(); i++ )
	{
		string command;

		Com_EscapeCommand( command, Cmd_Argv( i ), MAX_STRING );

		Q_strncat( message, "\"", sizeof( message ) );
		Q_strncat( message, command, sizeof( message ) );
		Q_strncat( message, "\" ", sizeof( message ) );
	}

	if( cls.state >= ca_connected )
	{
		to = cls.netchan.remote_address;
	}
	else
	{
		if( !Q_strlen( rcon_address->string ))
		{
			Msg( "You must either be connected or set the 'rcon_address' cvar to issue rcon commands\n" );
			return;
		}

		NET_StringToAdr( rcon_address->string, &to );
		if( to.port == 0 ) to.port = BF_BigShort( PORT_SERVER );
	}

	NET_SendPacket( NS_CLIENT, Q_strlen( message ) + 1, message, to );
}

void CL_WarnLostSplitPacket( void )
{
	if( cls.state != ca_connected )
		return;

	if( Host_IsLocalClient() )
		return;

	if( ++cl.lostpackets == 8 )
	{
		CL_Disconnect();
		Cbuf_AddText( "menu_connectionwarning" );
		MsgDev( D_WARN, "Too many lost packets! Showing Network options menu\n" );

	}
}


/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState( void )
{
	S_StopAllSounds ();
	R_ClearVBO();
	CL_ClearEffects ();
	CL_FreeEdicts ();

	CL_ClearPhysEnts ();

	// wipe the entire cl structure
	Q_memset( &cl, 0, sizeof( cl ));
	BF_Clear( &cls.netchan.message );
	Q_memset( &clgame.fade, 0, sizeof( clgame.fade ));
	Q_memset( &clgame.shake, 0, sizeof( clgame.shake ));

	Cvar_FullSet( "cl_background", "0", CVAR_READ_ONLY );
	cl.refdef.movevars = &clgame.movevars;
	cl.maxclients = 1; // allow to drawing player in menu
	cl.scr_fov = 90.0f;

	Cvar_SetFloat( "scr_download", 0.0f );
	Cvar_SetFloat( "scr_loading", 0.0f );

	// restore real developer level
	host.developer = host.old_developer;

	if( !SV_Active() && !CL_IsPlaybackDemo() && !cls.demorecording )
	{
		Delta_Shutdown();
		Delta_InitClient();
	}
	HTTP_ClearCustomServers();
}

/*
=====================
CL_SendDisconnectMessage

Sends a disconnect message to the server
=====================
*/
void CL_SendDisconnectMessage( void )
{
	sizebuf_t	buf;
	byte	data[32];

	if( cls.state == ca_disconnected ) return;

	BF_Init( &buf, "LastMessage", data, sizeof( data ));
	BF_WriteByte( &buf, clc_stringcmd );
	BF_WriteString( &buf, "disconnect" );

	if( !cls.netchan.remote_address.type )
		cls.netchan.remote_address.type = NA_LOOPBACK;

	// make sure message will be delivered
	Netchan_Transmit( &cls.netchan, BF_GetNumBytesWritten( &buf ), BF_GetData( &buf ));
	Netchan_Transmit( &cls.netchan, BF_GetNumBytesWritten( &buf ), BF_GetData( &buf ));
	Netchan_Transmit( &cls.netchan, BF_GetNumBytesWritten( &buf ), BF_GetData( &buf ));
}

/*
=====================
CL_Disconnect

Goes from a connected state to full screen console state
Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect( void )
{
	if( cls.state == ca_disconnected )
		return;

	cls.connect_time = 0;
	cls.changedemo = false;
	CL_Stop_f();

	// send a disconnect message to the server
	CL_SendDisconnectMessage();
	CL_ClearState ();

	S_StopBackgroundTrack ();
	SCR_EndLoadingPlaque (); // get rid of loading plaque

	// clear the network channel, too.
	Netchan_Clear( &cls.netchan );

	cls.state = ca_disconnected;
	Q_memset( &cls.serveradr, 0, sizeof( cls.serveradr ) );

	// restore gamefolder here (in case client was connected to another game)
	CL_ChangeGame( GI->gamefolder, true );

	// reset to writable state
	Cvar_FullSet( "touch_enable", va( "%s", touch_enable->string ), touch_enable->flags & ~CVAR_READ_ONLY );
	Cvar_FullSet( "m_ignore", va( "%s", m_ignore->string ), m_ignore->flags & ~CVAR_READ_ONLY );
	Cvar_FullSet( "joy_enable", va( "%s", Cvar_VariableString( "joy_enable" ) ), CVAR_ARCHIVE );
	Cbuf_InsertText( "menu_connectionprogress disconnect\n" );

	// back to menu if developer mode set to "player" or "mapper"
	if( host.developer > 2 ) return;
	UI_SetActiveMenu( true );
}

void CL_Disconnect_f( void )
{
	Host_Error( "Disconnected from server\n" );
}

void CL_Crashed( void )
{
	// already freed
	if( host.state == HOST_CRASHED ) return;
	if( Host_IsDedicated() ) return;
	if( !cls.initialized ) return;

	host.state = HOST_CRASHED;

	CL_Stop_f(); // stop any demos

	// send a disconnect message to the server
	CL_SendDisconnectMessage();
}

/*
=================
CL_LocalServers_f
=================
*/
void CL_LocalServers_f( void )
{
	netadr_t	adr;

	MsgDev( D_INFO, "Scanning for servers on the local network area...\n" );
	NET_Config( true, true ); // allow remote

	// send a broadcast packet
	adr.type = NA_BROADCAST;
	adr.port = BF_BigShort( PORT_SERVER );

	Netchan_OutOfBandPrint( NS_CLIENT, adr, "info %i", PROTOCOL_VERSION );
}

#define MS_SCAN_REQUEST "1\xFF" "0.0.0.0:0\0"

/*
=================
CL_InternetServers_f
=================
*/
void CL_InternetServers_f( void )
{
	char	fullquery[512] = MS_SCAN_REQUEST;
	char *info = fullquery + sizeof( MS_SCAN_REQUEST ) - 1;
	const size_t remaining = sizeof( fullquery ) - sizeof( MS_SCAN_REQUEST );

	Info_SetValueForKey( info, "nat", cl_nat->string, remaining );
	Info_SetValueForKey( info, "gamedir", GI->gamefolder, remaining );

	// let master know about client version
	Info_SetValueForKey( info, "clver", XASH_VERSION, remaining );

	NET_Config( true, true ); // allow remote

	cls.internetservers_wait = NET_SendToMasters( NS_CLIENT, sizeof( MS_SCAN_REQUEST ) + Q_strlen( info ), fullquery );
	cls.internetservers_pending = true;
}

/*
====================
CL_QueryServer_f
====================
*/
void CL_QueryServer_f( void )
{
	netadr_t adr;

	if( Cmd_Argc() != 2 )
	{
		MsgDev( D_INFO, "Usage: queryserver <adr>\n" );
		return;
	}

	NET_Config( true, true ); // allow remote

	if( NET_StringToAdr( Cmd_Argv( 1 ), &adr ) )
	{
		Netchan_OutOfBandPrint( NS_CLIENT, adr, "info %i", PROTOCOL_VERSION );
	}
	else
	{
		Msg( "Bad address\n" );
	}
}

/*
====================
CL_Packet_f

packet <destination> <contents>
Contents allows \n escape character
====================
*/
void CL_Packet_f( void )
{
	char	send[2048];
	char	*in, *out;
	int	i, l;
	netadr_t	adr;

	if( Cmd_Argc() != 3 )
	{
		Msg( "packet <destination> <contents>\n" );
		return;
	}

	NET_Config( true, false ); // allow remote

	if( !NET_StringToAdr( Cmd_Argv( 1 ), &adr ))
	{
		Msg( "Bad address\n" );
		return;
	}

	if( !adr.port ) adr.port = BF_BigShort( PORT_SERVER );

	in = Cmd_Argv( 2 );
	out = send + 4;
	send[0] = send[1] = send[2] = send[3] = (char)0xff;

	l = Q_strlen( in );

	for( i = 0; i < l; i++ )
	{
		if( in[i] == '\\' && in[i+1] == 'n' )
		{
			*out++ = '\n';
			i++;
		}
		else *out++ = in[i];
	}
	*out = 0;

	NET_SendPacket( NS_CLIENT, out - send, send, adr );
}

/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
void CL_Reconnect_f( void )
{
	if( cls.state == ca_disconnected )
		return;

	S_StopAllSounds ();

	if( cls.state == ca_connected )
	{
		cls.demonum = cls.movienum = -1;	// not in the demo loop now
		cls.state = ca_connected;

		// clear channel and stuff
		Netchan_Clear( &cls.netchan );
		BF_Clear( &cls.netchan.message );

		BF_WriteByte( &cls.netchan.message, clc_stringcmd );
		BF_WriteString( &cls.netchan.message, "new" );

		cl.validsequence = 0;		// haven't gotten a valid frame update yet
		cl.delta_sequence = -1;		// we'll request a full delta from the baseline
		cls.lastoutgoingcommand = -1;		// we don't have a backed up cmd history yet
		cls.nextcmdtime = host.realtime;	// we can send a cmd right away
		cl.last_command_ack = -1;

		CL_StartupDemoHeader ();
		return;
	}

	if( cls.servername[0] )
	{
		if( cls.state >= ca_connected )
		{
			CL_Disconnect();
			cls.connect_time = host.realtime - 1.5;
		}
		else cls.connect_time = MAX_HEARTBEAT; // fire immediately

		cls.demonum = cls.movienum = -1;	// not in the demo loop now
		cls.state = ca_connecting;
		Msg( "reconnecting...\n" );
	}
}

/*
=================
CL_ParseStatusMessage

Handle a reply from a info
=================
*/
void CL_ParseStatusMessage( netadr_t from, sizebuf_t *msg )
{
	char	*s;

	s = BF_ReadString( msg );
	MsgDev( D_NOTE, "Got info string: %s\n", s );
	UI_AddServerToList( from, s );
}

/*
=================
CL_ParseNETInfoMessage

Handle a reply from a netinfo
=================
*/
void CL_ParseNETInfoMessage( netadr_t from, sizebuf_t *msg )
{
	char		*s;
	net_request_t	*nr;
	int		i, context, type;

	context = Q_atoi( Cmd_Argv( 1 ));
	type = Q_atoi( Cmd_Argv( 2 ));
	s = Cmd_Argv( 3 );

	// find a request with specified context
	for( i = 0; i < MAX_REQUESTS; i++ )
	{
		nr = &clgame.net_requests[i];

		if( nr->resp.context == context && nr->resp.type == type )
		{
			if( nr->timeout > host.realtime )
			{
				// setup the answer
				nr->resp.response = s;
				nr->resp.remote_address = from;
				nr->resp.error = NET_SUCCESS;
				nr->resp.ping = host.realtime - nr->timesend;
				nr->pfnFunc( &nr->resp );

				if(!( nr->flags & FNETAPI_MULTIPLE_RESPONSE ))
					Q_memset( nr, 0, sizeof( *nr )); // done
			}
			else
			{
				Q_memset( nr, 0, sizeof( *nr ));
			}
			return;
		}
	}
}

//===================================================================

/*
======================
CL_PrepSound

Call before entering a new level, or after changing dlls
======================
*/
void CL_PrepSound( void )
{
	int	i, sndcount, step;

	MsgDev( D_NOTE, "CL_PrepSound: %s\n", clgame.mapname );
	for( i = 0, sndcount = 0; i < MAX_SOUNDS && cl.sound_precache[i+1][0]; i++ )
		sndcount++; // total num sounds
	step = sndcount/10;

	S_BeginRegistration();

	for( i = 0; i < MAX_SOUNDS - 1 && cl.sound_precache[i+1][0]; i++ )
	{
		cl.sound_index[i+1] = S_RegisterSound( cl.sound_precache[i+1] );
		Cvar_SetFloat( "scr_loading", scr_loading->value + 5.0f / sndcount );
		if( step && !( i % step ) && ( cl_allow_levelshots->integer || cl.background ))
			SCR_UpdateScreen();
	}

	S_EndRegistration();

	if( host.soundList )
	{
		// need to reapply all ambient sounds after restarting
		for( i = 0; i < host.numsounds; i++)
		{
			soundlist_t *entry = &host.soundList[i];
			if( entry->looping && entry->entnum != -1 )
			{
				MsgDev( D_NOTE, "Restarting sound %s...\n", entry->name );
				S_AmbientSound( entry->origin, entry->entnum,
					S_RegisterSound( entry->name ), entry->volume, entry->attenuation,
					entry->pitch, 0 );
			}
		}
	}

	host.soundList = NULL;
	host.numsounds = 0;

	cl.audio_prepped = true;
}

/*
=================
CL_PrepVideo

Call before entering a new level, or after changing dlls
=================
*/
void CL_PrepVideo( void )
{
	string	mdlname, mapname;
	int	i, mdlcount, step;
	int	map_checksum; // dummy

	if( !cl.model_precache[1][0] )
		return; // no map loaded

	Cvar_SetFloat( "scr_loading", 0.0f ); // reset progress bar
	MsgDev( D_NOTE, "CL_PrepVideo: %s\n", clgame.mapname );

	// let the render dll load the map
	Q_strncpy( mapname, cl.model_precache[1], MAX_STRING );
	Mod_LoadWorld( mapname, (uint *)&map_checksum, cl.maxclients > 1 );
	cl.worldmodel = Mod_Handle( 1 ); // get world pointer
	Cvar_SetFloat( "scr_loading", 25.0f );

	SCR_UpdateScreen();

	// make sure what map is valid
	if( !cls.demoplayback && map_checksum != cl.checksum )
		Host_Error( "Local map version differs from server: %i != '%i'\n", map_checksum, cl.checksum );

	for( i = 0, mdlcount = 0; i < MAX_MODELS && cl.model_precache[i+1][0]; i++ )
		mdlcount++; // total num models
	step = mdlcount/10;

	for( i = 0; i < MAX_MODELS - 1 && cl.model_precache[i+1][0]; i++ )
	{
		Q_strncpy( mdlname, cl.model_precache[i+1], MAX_STRING );
		Mod_RegisterModel( mdlname, i+1 );
		Cvar_SetFloat( "scr_loading", scr_loading->value + 75.0f / mdlcount );
		if( step && !( i % step ) && ( cl_allow_levelshots->integer || cl.background ) )
			SCR_UpdateScreen();
	}

	// update right muzzleflash indexes
	CL_RegisterMuzzleFlashes ();

	// invalidate all decal indexes
	Q_memset( cl.decal_index, 0, sizeof( cl.decal_index ));

	CL_ClearWorld ();

	R_NewMap(); // tell the render about new map

	V_SetupOverviewState(); // set overview bounds

	// must be called after lightmap loading!
	clgame.dllFuncs.pfnVidInit();

	// release unused SpriteTextures
	for( i = 1; i < MAX_IMAGES; i++ )
	{
		if( !clgame.sprites[i].name[0] ) continue; // free slot
		if( clgame.sprites[i].needload != clgame.load_sequence )
			Mod_UnloadSpriteModel( &clgame.sprites[i] );
	}

	Mod_FreeUnused ();

	Q_memset( cl.playermodels, 0, sizeof( cl.playermodels ) );

	Cvar_SetFloat( "scr_loading", 100.0f );	// all done

	if( host.decalList )
	{
		// need to reapply all decals after restarting
		for( i = 0; i < host.numdecals; i++ )
		{
			decallist_t *entry = &host.decalList[i];
			cl_entity_t *pEdict = CL_GetEntityByIndex( entry->entityIndex );
			int decalIndex = CL_DecalIndex( CL_DecalIndexFromName( entry->name ));
			int modelIndex = 0;

			if( pEdict ) modelIndex = pEdict->curstate.modelindex;
			CL_DecalShoot( decalIndex, entry->entityIndex, modelIndex, entry->position, entry->flags );
		}
		Z_Free( host.decalList );
	}

	host.decalList = NULL;
	host.numdecals = 0;

	if( host.soundList )
	{
		// need to reapply all ambient sounds after restarting
		for( i = 0; i < host.numsounds; i++ )
		{
			soundlist_t *entry = &host.soundList[i];
			if( entry->looping && entry->entnum != -1 )
			{
				MsgDev( D_NOTE, "Restarting sound %s...\n", entry->name );
				S_AmbientSound( entry->origin, entry->entnum,
				S_RegisterSound( entry->name ), entry->volume, entry->attenuation,
				entry->pitch, 0 );
			}
		}
	}

	host.soundList = NULL;
	host.numsounds = 0;

	if( host.developer <= 2 )
		Con_ClearNotify(); // clear any lines of console text

	SCR_UpdateScreen ();

	cl.video_prepped = true;
	cl.force_refdef = true;
}

/*
=================
CL_IsFromConnectingServer

Used for connectionless packets, when netchan may not be ready.
=================
*/
static qboolean CL_IsFromConnectingServer( netadr_t from )
{
	return NET_IsLocalAddress( from ) ||
		NET_CompareAdr( cls.serveradr, from );
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket( netadr_t from, sizebuf_t *msg )
{
	char	*args;
	char	*c, buf[MAX_SYSPATH];
	int	len = sizeof( buf ), i = 0;
	netadr_t servadr;

	BF_Clear( msg );
	BF_ReadLong( msg ); // skip the -1

	args = BF_ReadStringLine( msg );

	Cmd_TokenizeString( args );
	c = Cmd_Argv( 0 );

	MsgDev( D_NOTE, "CL_ConnectionlessPacket: %s : %s\n", NET_AdrToString( from ), c );

	// server connection
	if( !Q_strcmp( c, "client_connect" ))
	{
		unsigned int extensions;

		if( !CL_IsFromConnectingServer( from ) )
			return;

		extensions = Q_atoi( Cmd_Argv( 1 ) );
		if( cls.state == ca_connected )
		{
			MsgDev( D_INFO, "Dup connect received. Ignored.\n");
			return;
		}

		Netchan_Setup( NS_CLIENT, &cls.netchan, from, net_qport->integer );
		cls.splitcompress = false;

		if( extensions & NET_EXT_SPLIT )
		{
			if( cl_maxpacket->integer >= 40000 || cl_maxpacket->integer < 100 )
				Cvar_SetFloat( "cl_maxpacket", 1400 );

			cls.netchan.maxpacket = Cvar_VariableInteger( "cl_maxoutpacket" );
			if( cls.netchan.maxpacket < 100 )
				cls.netchan.maxpacket = cl_maxpacket->integer;

			cls.netchan.split = true;
			MsgDev( D_INFO, "^2NET_EXT_SPLIT enabled^7 (packet sizes is %d/%d)\n", cl_maxpacket->integer, cls.netchan.maxpacket );

			if( extensions & NET_EXT_SPLITHUFF )
			{
				MsgDev( D_INFO, "^2NET_EXT_SPLITHUFF enabled^7\n");
				cls.splitcompress = true;
			}
		}

		if( extensions & NET_EXT_HUFF )
		{
			MsgDev( D_INFO, "^2NET_EXT_HUFF enabled\n" );

			cls.netchan.compress = true;
		}

		BF_WriteByte( &cls.netchan.message, clc_stringcmd );
		BF_WriteString( &cls.netchan.message, "new" );
		cls.state = ca_connected;

		cl.validsequence = 0;		// haven't gotten a valid frame update yet
		cl.delta_sequence = -1;		// we'll request a full delta from the baseline
		cls.lastoutgoingcommand = -1;		// we don't have a backed up cmd history yet
		cls.nextcmdtime = host.realtime;	// we can send a cmd right away
		cl.last_command_ack = -1;

		CL_StartupDemoHeader ();
	}
	else if( !Q_strcmp( c, "info" ))
	{
		// server responding to a status broadcast
		CL_ParseStatusMessage( from, msg );
	}
	else if( !Q_strcmp( c, "netinfo" ))
	{
		// server responding to a status broadcast
		CL_ParseNETInfoMessage( from, msg );
	}
	else if( !Q_strcmp( c, "cmd" ))
	{
		// remote command from gui front end
		if( !NET_IsLocalAddress( from ))
		{
			Msg( "Command packet from remote host. Ignored.\n" );
			return;
		}
#ifdef XASH_SDL
		SDL_RestoreWindow( host.hWnd );
#endif
		args = BF_ReadString( msg );
		Cbuf_AddText( args );
		Cbuf_AddText( "\n" );
	}
	else if( !Q_strcmp( c, "print" ))
	{
		// print command from somewhere
		char *str = BF_ReadString( msg );
		Msg("^5r:^7%s", str );

		if( str[0] == 0 || str[ Q_strlen( str ) - 1 ] != '\n' )
			Msg( "\n" );
	}
	else if( !Q_strcmp( c, "errormsg" ))
	{
		char *str;

		if( !CL_IsFromConnectingServer( from ))
			return;

		str = BF_ReadString( msg );
		if( UI_IsVisible() )
			Cmd_ExecuteString( va("menu_showmessagebox \"^3Server message^7\n%s\"", str ), src_command );
		Msg( "%s", str );
	}
	else if( !Q_strcmp( c, "updatemsg" ))
	{
		// got an update message from master server
		// show update dialog from menu
		netadr_t adr;
		qboolean preferStore = true;

		if( !Q_strcmp( Cmd_Argv( 1 ), "nostore" ) )
			preferStore = false;

		if( NET_StringToAdr( DEFAULT_PRIMARY_MASTER, &adr ) )
		{
			if( NET_CompareAdr( from, adr ))
			{
				// update from masterserver
				Cbuf_AddText( va( "menu_updatedialog %s\n", preferStore ? "store" : "nostore" ) );
			}
		}
		else
		{
			// in case we don't have master anymore
			Cbuf_AddText( va( "menu_updatedialog %s\n", preferStore ? "store" : "nostore" ) );
		}
	}
	else if( !Q_strcmp( c, "ping" ))
	{
		// ping from somewhere
		Netchan_OutOfBandPrint( NS_CLIENT, from, "ack" );
	}
	else if( !Q_strcmp( c, "challenge" ))
	{
		if( !CL_IsFromConnectingServer( from ))
			return;

		// challenge from the server we are connecting to
		cls.challenge = Q_atoi( Cmd_Argv( 1 ));
		CL_SendConnectPacket();
		return;
	}
	else if( !Q_strcmp( c, "echo" ))
	{
		// echo request from server
		Netchan_OutOfBandPrint( NS_CLIENT, from, "%s", Cmd_Argv( 1 ));
	}
	else if( !Q_strcmp( c, "disconnect" ))
	{
		if( !CL_IsFromConnectingServer( from ))
			return;

		// a disconnect message from the server, which will happen if the server
		// dropped the connection but it is still getting packets from us
		CL_Disconnect();
		CL_ClearEdicts();
	}
	else if( !Q_strcmp( c, "f") )
	{
		// serverlist got from masterserver
		while( !msg->bOverflow )
		{
			servadr.type = NA_IP;
			// 4 bytes for IP
			BF_ReadBytes( msg, servadr.ip, sizeof( servadr.ip ));
			// 2 bytes for Port
			servadr.port = BF_ReadShort( msg );

			if( !servadr.port )
				break;

			MsgDev( D_INFO, "Found server: %s\n", NET_AdrToString( servadr ));

			NET_Config( true, false ); // allow remote

			Netchan_OutOfBandPrint( NS_CLIENT, servadr, "info %i", PROTOCOL_VERSION );
		}

		// execute at next frame preventing relation on fps
		if( cls.internetservers_pending )
			Cbuf_AddText("menu_resetping\n");
		cls.internetservers_pending = false;
	}
	else if( clgame.dllFuncs.pfnConnectionlessPacket( &from, args, buf, &len ))
	{
		// user out of band message (must be handled in CL_ConnectionlessPacket)
		if( len > 0 ) Netchan_OutOfBand( NS_SERVER, from, len, (byte *)buf );
	}
	else MsgDev( D_ERROR, "Bad connectionless packet from %s:\n%s\n", NET_AdrToString( from ), args );
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
static qboolean CL_GetMessage( byte *data, size_t *length )
{
	if( cls.demoplayback )
	{
		return CL_DemoReadMessage( data, length );
	}

	return NET_GetPacket( NS_CLIENT, &net_from, data, length );
}

/*
=================
CL_ReadNetMessage
=================
*/
void CL_ReadNetMessage( void )
{
	size_t	curSize;

	while( CL_GetMessage( net_message_buffer, &curSize ))
	{
		if( *((int *)&net_message_buffer) == 0xFFFFFFFE )
			// Will rewrite existing packet by merged
			if( !NetSplit_GetLong( &cls.netchan.netsplit, &net_from, net_message_buffer, &curSize, cls.splitcompress ) )
				continue;

		BF_Init( &net_message, "ServerData", net_message_buffer, curSize );

		// check for connectionless packet (0xffffffff) first
		if( BF_GetMaxBytes( &net_message ) >= 4 && *(int *)net_message.pData == -1 )
		{
			CL_ConnectionlessPacket( net_from, &net_message );
			continue;
		}

		// can't be a valid sequenced packet
		if( cls.state < ca_connected ) continue;

		if( BF_GetMaxBytes( &net_message ) < 8 )
		{
			MsgDev( D_WARN, "%s: runt packet\n", NET_AdrToString( net_from ));
			continue;
		}

		// packet from server
		if( !cls.demoplayback && !NET_CompareAdr( net_from, cls.netchan.remote_address ))
		{
			MsgDev( D_ERROR, "CL_ReadPackets: %s:sequenced packet without connection\n", NET_AdrToString( net_from ));
			continue;
		}

		if( !cls.demoplayback && !Netchan_Process( &cls.netchan, &net_message ))
			continue;	// wasn't accepted for some reason

		CL_ParseServerMessage( &net_message );
	}

	// check for fragmentation/reassembly related packets.
	if( cls.state != ca_disconnected && Netchan_IncomingReady( &cls.netchan ))
	{
		// the header is different lengths for reliable and unreliable messages
		int headerBytes = BF_GetNumBytesRead( &net_message );

		// process the incoming buffer(s)
		if( Netchan_CopyNormalFragments( &cls.netchan, &net_message ))
		{
			CL_ParseServerMessage( &net_message );
		}

		if( Netchan_CopyFileFragments( &cls.netchan, &net_message ))
		{
			// remove from resource request stuff.
			CL_ProcessFile( true, cls.netchan.incomingfilename );
		}
	}

	Netchan_UpdateProgress( &cls.netchan );
}

void CL_ReadPackets( void )
{
	CL_ReadNetMessage();

	cl.lerpFrac = CL_LerpPoint();
	cl.lerpBack = 1.0f - cl.lerpFrac;

	if( clgame.entities )
		cl.thirdperson = clgame.dllFuncs.CL_IsThirdPerson();

#if 0
	// keep cheat cvars are unchanged
	if( cl.maxclients > 1 && cls.state == ca_active && host.developer <= 1 )
		Cvar_SetCheatState();
#endif
	CL_UpdateFrameLerp ();

	// singleplayer never has connection timeout
	if( NET_IsLocalAddress( cls.netchan.remote_address ))
		return;

	// check timeout
	if( cls.state >= ca_connected && !cls.demoplayback && cls.state != ca_cinematic )
	{
		if( host.realtime - cls.netchan.last_received > cl_timeout->value )
		{
			if( ++cl.timeoutcount > 5 ) // timeoutcount saves debugger
			{
				Msg( "\nServer connection timed out.\n" );
				CL_Disconnect();
				CL_ClearEdicts();
				return;
			}
		}
	}
	else cl.timeoutcount = 0;

}

/*
====================
CL_ProcessFile

A file has been received via the fragmentation/reassembly layer, put it in the right spot and
 see if we have finished downloading files.
====================
*/
void CL_ProcessFile( qboolean successfully_received, const char *filename )
{
	if( successfully_received)
		MsgDev( D_INFO, "Received %s\n", filename );
	else
		MsgDev( D_WARN, "Failed to download %s\n", filename );

	if( downloadfileid == downloadcount - 1 )
	{
		MsgDev( D_INFO, "Download completed, resuming connection\n" );
		FS_Rescan();

		if( cls.state < ca_connecting )
		{
				Cbuf_AddText( "menu_connectionprogress dlend\n" );
				return;
		}

		BF_WriteByte( &cls.netchan.message, clc_stringcmd );
		BF_WriteString( &cls.netchan.message, "continueloading" );
		downloadfileid = 0;
		downloadcount = 0;
		return;
	}

	downloadfileid++;
}

//=============================================================================
/*
==============
CL_Userinfo_f
==============
*/
void CL_Userinfo_f( void )
{
	Msg( "User info settings:\n" );
	Info_Print( Cvar_Userinfo( ));
	Msg( "Total %i symbols\n", Q_strlen( Cvar_Userinfo( )));
}

/*
==============
CL_Physinfo_f
==============
*/
void CL_Physinfo_f( void )
{
	Msg( "Phys info settings:\n" );
	Info_Print( cl.frame.client.physinfo );
	Msg( "Total %i symbols\n", Q_strlen( cl.frame.client.physinfo ));
}

/*
=================
CL_Precache_f

The server will send this command right
before allowing the client into the server
=================
*/
void CL_Precache_f( void )
{
	int	spawncount;

	spawncount = Q_atoi( Cmd_Argv( 1 ));

	CL_PrepSound();
	CL_PrepVideo();

	if( Cmd_Argc() > 2 )
	{
		int svCheatState = Q_atoi( Cmd_Argv( 2 ) );

		// don't let server send other values than 1 or 0
		Cvar_SetFloat("sv_cheats", svCheatState ? 1.0f : 0.0f );
	}
	else
	{
		// disable just in case for old servers.
		// TODO: move to a part of new protocol
		Cvar_SetFloat("sv_cheats", 0.0f );
	}
	Cvar_SetCheatState( false );

	BF_WriteByte( &cls.netchan.message, clc_stringcmd );
	BF_WriteString( &cls.netchan.message, va( "begin %i\n", spawncount ));
}

/*
=================
CL_Escape_f

Escape to menu from game
=================
*/
void CL_Escape_f( void )
{
	if( cls.key_dest == key_menu )
		return;

	// the final credits is running
	if( UI_CreditsActive( )) return;

	if( cls.state == ca_cinematic )
		SCR_NextMovie(); // jump to next movie
	else UI_SetActiveMenu( true );
}

/*
=================
CL_InitLocal
=================
*/
void CL_InitLocal( void )
{
	cls.state = ca_disconnected;
	Q_memset( &cls.serveradr, 0, sizeof( cls.serveradr ) );

	// register our variables
	cl_predict = Cvar_Get( "cl_predict", "0", CVAR_ARCHIVE, "enable client movement prediction" );
	cl_crosshair = Cvar_Get( "crosshair", "1", CVAR_ARCHIVE, "show weapon chrosshair" );
	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0, "disable delta-compression for usercommands" );
	cl_idealpitchscale = Cvar_Get( "cl_idealpitchscale", "0.8", 0, "how much to look up/down slopes and stairs when not using freelook" );
	cl_solid_players = Cvar_Get( "cl_solid_players", "1", 0, "make all players non-solid (can't traceline them)" );
	cl_interp = Cvar_Get( "ex_interp", "0.01", 0, "interpolate object positions starting this many seconds in past" );
	//Cvar_Get( "ex_maxerrordistance", "0", 0, "" );
	cl_allow_fragment = Cvar_Get( "cl_allow_fragment", "0", CVAR_ARCHIVE, "allow downloading files directly from game server" );
	cl_timeout = Cvar_Get( "cl_timeout", "60", 0, "connect timeout (in seconds)" );
	cl_charset = Cvar_Get( "cl_charset", "utf-8", CVAR_ARCHIVE, "1-byte charset to use (iconv style)" );

	rcon_client_password = Cvar_Get( "rcon_password", "", 0, "remote control client password" );
	rcon_address = Cvar_Get( "rcon_address", "", 0, "remote control address" );

	r_oldparticles = Cvar_Get("r_oldparticles", "0", CVAR_ARCHIVE, "make some particle textures a simple square, like with software rendering");

	cl_trace_events = Cvar_Get( "cl_trace_events", "0", CVAR_ARCHIVE|CVAR_CHEAT, "enable client event tracing (good for developers)" );
	cl_trace_messages = Cvar_Get( "cl_trace_messages", "0", CVAR_ARCHIVE|CVAR_CHEAT, "enable message names tracing (good for developers)");
	cl_trace_stufftext = Cvar_Get( "cl_trace_stufftext", "0", CVAR_ARCHIVE|CVAR_CHEAT, "enable stufftext commands tracing (good for developers)" );

	// userinfo
	Cvar_Get( "password", "", CVAR_USERINFO, "player password" );
	// cvar is not registered as userinfo as it not needed usually
	// it will be set as userinfo only if it has non-default and correct value
	cl_maxpacket = Cvar_Get( "cl_maxpacket", "40000", CVAR_ARCHIVE, "split packet size" );
	cl_maxpayload = Cvar_Get( "cl_maxpayload", "0", CVAR_ARCHIVE, "max netchan size from server durning connection" );
	name = Cvar_Get( "name", Sys_GetCurrentUser(), CVAR_USERINFO|CVAR_ARCHIVE|CVAR_PRINTABLEONLY, "player name" );
	model = Cvar_Get( "model", "player", CVAR_USERINFO|CVAR_ARCHIVE, "player model ('player' is a singleplayer model)" );
	topcolor = Cvar_Get( "topcolor", "0", CVAR_USERINFO|CVAR_ARCHIVE, "player top color" );
	bottomcolor = Cvar_Get( "bottomcolor", "0", CVAR_USERINFO|CVAR_ARCHIVE, "player bottom color" );
	rate = Cvar_Get( "rate", "25000", CVAR_USERINFO|CVAR_ARCHIVE, "player network rate" );
	hltv = Cvar_Get( "hltv", "0", CVAR_USERINFO|CVAR_LATCH, "HLTV mode" );
	cl_showfps = Cvar_Get( "cl_showfps", "1", CVAR_ARCHIVE, "show client fps" );
	cl_showpos = Cvar_Get( "cl_showpos", "0", CVAR_ARCHIVE, "show local player position and velocity" );
	cl_cmdbackup = Cvar_Get( "cl_cmdbackup", "10", CVAR_ARCHIVE, "how many additional history commands are sent" );
	cl_cmdrate = Cvar_Get( "cl_cmdrate", "30", CVAR_ARCHIVE, "max number of command packets sent to server per second" );
	cl_draw_particles = Cvar_Get( "cl_draw_particles", "1", CVAR_ARCHIVE, "disable particle effects" );
	cl_draw_beams = Cvar_Get( "cl_draw_beams", "1", CVAR_ARCHIVE, "disable view beams" );
	cl_lightstyle_lerping = Cvar_Get( "cl_lightstyle_lerping", "0", CVAR_ARCHIVE, "enable animated light lerping (perfomance option)" );
	cl_sprite_nearest = Cvar_Get( "cl_sprite_nearest", "0", CVAR_ARCHIVE, "disable texture filtering on sprites" );
	cl_showerror = Cvar_Get( "cl_showerror", "0", CVAR_ARCHIVE, "show prediction error" );
	cl_updaterate = Cvar_Get( "cl_updaterate", "60", CVAR_USERINFO|CVAR_ARCHIVE, "refresh rate of server messages" );
	cl_nosmooth = Cvar_Get( "cl_nosmooth", "0", CVAR_ARCHIVE, "smooth up stair climbing and interpolate position in multiplayer" );
	cl_smoothtime = Cvar_Get( "cl_smoothtime", "0.1", CVAR_ARCHIVE, "time to smooth up" );
	r_bmodelinterp = Cvar_Get( "r_bmodelinterp", "1", 0, "enable bmodel interpolation" );
	cl_nat = Cvar_Get( "cl_nat", "0", 0, "Show servers running under nat" );

	hud_scale = Cvar_Get( "hud_scale", "0", CVAR_ARCHIVE|CVAR_LATCH, "scale hud at current resolution" );
	hud_utf8 = Cvar_Get( "hud_utf8", "0", CVAR_ARCHIVE, "Use utf-8 encoding for hud text" );

	ui_renderworld = Cvar_Get( "ui_renderworld", "0", CVAR_ARCHIVE, "render world when UI is visible" );

	Cvar_Get( "skin", "", CVAR_USERINFO, "player skin" ); // XDM 3.3 want this cvar
	Cvar_Get( "cl_background", "0", CVAR_READ_ONLY, "indicates that background map is running" );

	Cvar_Get( "cl_enable_compress", "0", CVAR_ARCHIVE, "request huffman compression from server" );
	Cvar_Get( "cl_enable_split", "1", CVAR_ARCHIVE, "request packet split from server" );
	Cvar_Get( "cl_enable_splitcompress", "0", CVAR_ARCHIVE, "request compressing all splitpackets" );

	Cvar_Get( "cl_maxoutpacket", "0", CVAR_ARCHIVE, "max outcoming packet size (equal cl_maxpacket if 0)" );

	// these two added to shut up CS 1.5 about 'unknown' commands
	Cvar_Get( "lightgamma", "1", 0, "ambient lighting level (legacy, unused)" );
	Cvar_Get( "direct", "1", 0, "direct lighting level (legacy, unused)" );
	Cvar_Get( "voice_serverdebug", "0", 0, "debug voice (legacy, unused)" );

	// server commands
	Cmd_AddCommand ("noclip", NULL, "toggle noclipping mode" );
	Cmd_AddCommand ("notarget", NULL, "notarget mode (monsters do not see you)" );
	Cmd_AddCommand ("fullupdate", NULL, "re-init HUD on start of demo recording" );
	Cmd_AddCommand ("give", NULL, "give specified item or weapon" );
	Cmd_AddCommand ("drop", NULL, "drop current/specified item or weapon" );
	Cmd_AddCommand ("gametitle", NULL, "show game logo" );
	Cmd_AddCommand ("god", NULL, "toggle godmode" );
	Cmd_AddCommand ("fov", NULL, "set client field of view" );

	Cmd_AddCommand ("kill", NULL, "commit suicide" );
	Cmd_AddCommand ("ent_list", NULL, "list entities on server" );
	Cmd_AddCommand ("ent_fire", NULL, "fire entity command (be careful)" );
	Cmd_AddCommand ("ent_info", NULL, "dump entity information" );
	Cmd_AddCommand ("ent_create", NULL, "create entity with specified values (be careful)" );
	Cmd_AddCommand ("ent_getvars", NULL, "put parameters of specified entities to client's' ent_last_* cvars" );

	// register our commands
	Cmd_AddCommand ("pause", NULL, "pause the game (if the server allows pausing)" );
	Cmd_AddCommand ("localservers", CL_LocalServers_f, "collect info about local servers" );
	Cmd_AddCommand ("internetservers", CL_InternetServers_f, "collect info about internet servers" );
	Cmd_AddCommand ("queryserver", CL_QueryServer_f, "collect info about server by address");
	Cmd_AddCommand ("cd", CL_PlayCDTrack_f, "play cd-track (not real cd-player of course)" );
	Cmd_AddCommand ("mp3", CL_MP3Command_f, "mp3 command" );

	Cmd_AddCommand ("userinfo", CL_Userinfo_f, "print current client userinfo" );
	Cmd_AddCommand ("physinfo", CL_Physinfo_f, "print current client physinfo" );
	Cmd_AddCommand ("disconnect", CL_Disconnect_f, "disconnect from server" );
	Cmd_AddCommand ("record", CL_Record_f, "record a demo" );
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f, "play a demo" );
	Cmd_AddCommand ("killdemo", CL_DeleteDemo_f, "delete a specified demo file and demoshot" );
	Cmd_AddCommand ("startdemos", CL_StartDemos_f, "start playing back the selected demos sequentially" );
	Cmd_AddCommand ("demos", CL_Demos_f, "restart looping demos defined by the last startdemos command" );
	Cmd_AddCommand ("movie", CL_PlayVideo_f, "play a movie" );
	Cmd_AddCommand ("stop", CL_Stop_f, "stop playing or recording a demo" );
	Cmd_AddCommand ("info", NULL, "collect info about local servers with specified protocol" );
	Cmd_AddCommand ("escape", CL_Escape_f, "escape from game to menu" );
	Cmd_AddCommand ("pointfile", CL_ReadPointFile_f, "show leaks on a map (if present of course)" );
	Cmd_AddCommand ("linefile", CL_ReadLineFile_f, "show leaks on a map (if present of course)" );

	Cmd_AddCommand ("quit", CL_Quit_f, "quit from game" );
	Cmd_AddCommand ("exit", CL_Quit_f, "quit from game" );

	Cmd_AddCommand ("screenshot", CL_ScreenShot_f, "takes a screenshot of the next rendered frame" );
	Cmd_AddCommand ("snapshot", CL_SnapShot_f, "takes a snapshot of the next rendered frame" );
	Cmd_AddCommand ("envshot", CL_EnvShot_f, "takes a six-sides cubemap shot with specified name" );
	Cmd_AddCommand ("skyshot", CL_SkyShot_f, "takes a six-sides envmap (skybox) shot with specified name" );
	Cmd_AddCommand ("levelshot", CL_LevelShot_f, "same as \"screenshot\", used to create plaque images" );
	Cmd_AddCommand ("saveshot", CL_SaveShot_f, "used to create save previews with LoadGame menu" );
	Cmd_AddCommand ("demoshot", CL_DemoShot_f, "used to create demo previews with PlayDemo menu" );

	Cmd_AddCommand ("connect", CL_Connect_f, "connect to a server by hostname" );
	Cmd_AddCommand ("reconnect", CL_Reconnect_f, "reconnect to current level" );

	Cmd_AddCommand ("rcon", CL_Rcon_f, "sends a command to the server console (rcon_password and rcon_address required)" );

	// this is dangerous to leave in
// 	Cmd_AddCommand ("packet", CL_Packet_f, "send a packet with custom contents" );

	Cmd_AddCommand ("precache", CL_Precache_f, "precache specified resource (by index)" );
	Cmd_AddCommand( "trysaveconfig", CL_TrySaveConfig_f, "schedule config save on disconnected state" );
}

//============================================================================

/*
==================
CL_SendCommand

==================
*/
void CL_SendCommand( void )
{
	// send intentions now
	CL_SendCmd ();

	// resend a connection request if necessary
	CL_CheckForResend ();
}

/*
==================
Host_ClientBegin

Send command before server runs frame to prevent frame delay
==================
*/
void Host_ClientBegin( void )
{
	if( !cls.initialized )
		return;

	if( SV_Active() )
	{
		CL_SendCommand();

		CL_PredictMovement();
	}
}

/*
==================
Host_ClientFrame

==================
*/
void Host_ClientFrame( void )
{
	// if client is not active, skip render functions

	// decide the simulation time
	cl.oldtime = cl.time;
	cl.time += host.frametime;

	if( menu.hInstance )
	{
		// menu time (not paused, not clamped)
		menu.globals->time = host.realtime;
		menu.globals->frametime = host.realframetime;
		menu.globals->demoplayback = cls.demoplayback;
		menu.globals->demorecording = cls.demorecording;
	}

	VGui_RunFrame ();

	if( cls.initialized )
	{
		clgame.dllFuncs.pfnFrame( host.frametime );

		// fetch results from server
		CL_ReadPackets();

		VID_CheckChanges();

		// allow sound and video DLL change
		if( cls.state == ca_active )
		{
			if( !cl.video_prepped ) CL_PrepVideo();
			if( !cl.audio_prepped ) CL_PrepSound();
		}

		// send a new command message to the server
		if( !SV_Active() )
		{
			CL_SendCommand();

			// predict all unacknowledged movements
			CL_PredictMovement();
		}
	}

	// update the screen
	SCR_UpdateScreen ();

	if( cls.initialized )
	{
		// update audio
		S_RenderFrame( &cl.refdef );

		// decay dynamic lights
		CL_DecayLights ();

		SCR_RunCinematic();
	}

	Con_RunConsole();

	cls.framecount++;
}


//============================================================================

/*
====================
CL_Init
====================
*/
void CL_Init( void )
{
	qboolean loaded;

	Q_memset( &cls, 0, sizeof( cls ) );

	if( Host_IsDedicated() )
		return; // nothing running on the client

	Con_Init();
	CL_InitLocal();

	R_Init();	// init renderer
	S_Init();	// init sound

	// unreliable buffer. unsed for unreliable commands and voice stream
	BF_Init( &cls.datagram, "cls.datagram", cls.datagram_buf, sizeof( cls.datagram_buf ));

	Touch_Init();

	{
		char clientlib[256];
		Com_ResetLibraryError();
		if( Sys_GetParmFromCmdLine( "-clientlib", clientlib ) )
			loaded = CL_LoadProgs( clientlib );
		else
#ifdef XASH_INTERNAL_GAMELIBS
			loaded = CL_LoadProgs( "client" );
#else
			loaded = CL_LoadProgs( va( "%s/%s" , GI->dll_path, SI.clientlib ));
#endif
		if( !loaded )
		{
			loaded = CL_LoadProgs( CLIENTDLL );
		}
	}

	if( loaded )
	{
		cls.initialized = true;
		cls.keybind_changed = false;
		cl.maxclients = 1; // allow to drawing player in menu
		cls.olddemonum = -1;
		cls.demonum = -1;
	}
	else
		Sys_Warn("Could not load client library:\n%s", Com_GetLibraryError());
}

/*
===============
CL_Shutdown

===============
*/
void CL_Shutdown( void )
{
	MsgDev( D_INFO, "CL_Shutdown()\n" );

	if( cls.initialized && !host.crashed )
	{
		Host_WriteOpenGLConfig ();
		Host_WriteVideoConfig ();
	}
	Touch_Shutdown();
	CL_CloseDemoHeader();
	IN_Shutdown ();
	Mobile_Shutdown();

	SCR_Shutdown ();
	if( cls.initialized )
	{
		CL_UnloadProgs ();
		cls.initialized = false;
	}

	FS_Delete( "demoheader.tmp" ); // remove tmp file
	SCR_FreeCinematic (); // release AVI's *after* client.dll because custom renderer may use them
	S_Shutdown ();
	R_Shutdown ();
}
#endif // XASH_DEDICATED
