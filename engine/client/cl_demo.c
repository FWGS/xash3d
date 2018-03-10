/*
cl_demo.c - demo record & playback
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
#include "net_encode.h"

#define dem_unknown		0	// unknown command
#define dem_norewind	1	// startup message
#define dem_read		2	// it's a normal network packet
#define dem_jumptime	3	// move the demostart time value forward by this amount
#define dem_userdata	4	// userdata from the client.dll
#define dem_usercmd		5	// read usercmd_t
#define dem_stop		6	// end of time
#define dem_lastcmd		dem_stop

#define DEMO_STARTUP	0	// this lump contains startup info needed to spawn into the server
#define DEMO_NORMAL		1	// this lump contains playback info of messages, etc., needed during playback.

#define IDEMOHEADER		(('M'<<24)+('E'<<16)+('D'<<8)+'I') // little-endian "IDEM"
#define DEMO_PROTOCOL	1

const char *demo_cmd[dem_lastcmd+1] =
{
	"dem_unknown",
	"dem_norewind",
	"dem_read",
	"dem_jumptime",
	"dem_userdata",
	"dem_usercmd",
	"dem_stop",
};

typedef struct
{
	int		id;		// should be IDEM
	int		dem_protocol;	// should be DEMO_PROTOCOL
	int		net_protocol;	// should be PROTOCOL_VERSION
	char		mapname[64];	// name of map
	char		gamedir[64];	// name of game directory (FS_Gamedir())
	int		directory_offset;	// offset of Entry Directory.
} demoheader_t;

typedef struct
{
	int		entrytype;	// DEMO_STARTUP or DEMO_NORMAL
	float		playback_time;	// time of track
	int		playback_frames;	// # of frames in track
	int		offset;		// file offset of track data
	int		length;		// length of track
} demoentry_t;

typedef struct
{
	demoentry_t	*entries;		// track entry info
	int		numentries;	// number of tracks
} demodirectory_t;

// private demo states
struct
{
	demoheader_t	header;
	demoentry_t	*entry;
	demodirectory_t	directory;
	int		framecount;
	float		starttime;
	float		realstarttime;
	int		entryIndex;
} demo;

/*
====================
CL_StartupDemoHeader

spooling demo header in case
we record a demo on this level
====================
*/
void CL_StartupDemoHeader( void )
{
	if( cls.demoheader )
	{
		FS_Close( cls.demoheader );
	}

	// Note: this is replacing tmpfile()
	cls.demoheader = FS_Open( "demoheader.tmp", "w+b", true );

	if( !cls.demoheader )
	{
		MsgDev( D_ERROR, "couldn't open temporary header file.\n" );
		return;
	}

	MsgDev( D_INFO, "Spooling demo header.\n" );
}

/*
====================
CL_CloseDemoHeader

close demoheader file on engine shutdown
====================
*/
void CL_CloseDemoHeader( void )
{
	if( !cls.demoheader )
		return;

	FS_Close( cls.demoheader );
}

/*
====================
CL_GetDemoRecordClock

write time while demo is recording
====================
*/
float CL_GetDemoRecordClock( void ) 
{
	return cl.mtime[0];
}

/*
====================
CL_GetDemoPlaybackClock

overwrite host.realtime
====================
*/
float CL_GetDemoPlaybackClock( void ) 
{
	return host.realtime + host.frametime;
}

/*
====================
CL_WriteDemoCmdHeader

Writes the demo command header and time-delta
====================
*/
void CL_WriteDemoCmdHeader( byte cmd, file_t *file )
{
	float	dt;

	ASSERT( cmd >= 1 && cmd <= dem_lastcmd );
	if( !file ) return;

	// command
	FS_Write( file, &cmd, sizeof( byte ));

	// time offset
	dt = (float)(CL_GetDemoRecordClock() - demo.starttime);
	FS_Write( file, &dt, sizeof( float ));
}

/*
====================
CL_WriteDemoJumpTime

Update level time on a next level
====================
*/
void CL_WriteDemoJumpTime( void )
{
	if( cls.demowaiting || !cls.demofile )
		return;

	demo.starttime = CL_GetDemoRecordClock(); // setup the demo starttime

	// demo playback should read this as an incoming message.
	// write the client's realtime value out so we can synchronize the reads.
	CL_WriteDemoCmdHeader( dem_jumptime, cls.demofile );
}

/*
====================
CL_WriteDemoUserCmd

Writes the current user cmd
====================
*/
void CL_WriteDemoUserCmd( int cmdnumber )
{
	sizebuf_t	buf;
	word	bytes;
	byte	data[1024];

	if( !cls.demorecording || !cls.demofile )
		return;

	CL_WriteDemoCmdHeader( dem_usercmd, cls.demofile );

	FS_Write( cls.demofile, &cls.netchan.outgoing_sequence, sizeof( int ));
	FS_Write( cls.demofile, &cmdnumber, sizeof( int ));

	// write usercmd_t
	BF_Init( &buf, "UserCmd", data, sizeof( data ));
	CL_WriteUsercmd( &buf, -1, cmdnumber );	// always no delta

	bytes = BF_GetNumBytesWritten( &buf );

	FS_Write( cls.demofile, &bytes, sizeof( word ));
	FS_Write( cls.demofile, data, bytes );
}

/*
====================
CL_WriteDemoSequence

Save state of cls.netchan sequences
so that we can play the demo correctly.
====================
*/
void CL_WriteDemoSequence( file_t *file )
{
	ASSERT( file );

	FS_Write( file, &cls.netchan.incoming_sequence, sizeof( int ));
	FS_Write( file, &cls.netchan.incoming_acknowledged, sizeof( int ));
	FS_Write( file, &cls.netchan.incoming_reliable_acknowledged, sizeof( int ));
	FS_Write( file, &cls.netchan.incoming_reliable_sequence, sizeof( int ));
	FS_Write( file, &cls.netchan.outgoing_sequence, sizeof( int ));
	FS_Write( file, &cls.netchan.reliable_sequence, sizeof( int ));
	FS_Write( file, &cls.netchan.last_reliable_sequence, sizeof( int ));
}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage( qboolean startup, int start, sizebuf_t *msg )
{
	file_t	*file = startup ? cls.demoheader : cls.demofile;
	int	swlen;
	byte	c;

	if( !file ) return;

	// past the start but not recording a demo.
	if( !startup && !cls.demorecording )
		return;

	swlen = BF_GetNumBytesWritten( msg ) - start;
	if( swlen <= 0 ) return;

	if( !startup )
	{
		cls.demotime += host.frametime;
		demo.framecount++;
	}

	// demo playback should read this as an incoming message.
	c = (cls.state != ca_active) ? dem_norewind : dem_read;

	CL_WriteDemoCmdHeader( c, file );
	CL_WriteDemoSequence( file );

	// write the length out.
	FS_Write( file, &swlen, sizeof( int ));

	// output the buffer. Skip the network packet stuff.
	FS_Write( file, BF_GetData( msg ) + start, swlen );
}

/*
====================
CL_WriteDemoUserMessage

Dumps the user message (demoaction)
====================
*/
void CL_WriteDemoUserMessage( const byte *buffer, size_t size )
{
	if( !cls.demorecording || cls.demowaiting )
		return;

	if( !cls.demofile || !buffer || size <= 0 )
		return;

	CL_WriteDemoCmdHeader( dem_userdata, cls.demofile );

	// write the length out.
	FS_Write( cls.demofile, &size, sizeof( int ));

	// output the buffer.
	FS_Write( cls.demofile, buffer, size );
}

/*
====================
CL_WriteDemoHeader

Write demo header
====================
*/
void CL_WriteDemoHeader( const char *name )
{
	fs_offset_t	copysize;
	fs_offset_t	savepos;
	fs_offset_t	curpos;
	
	MsgDev( D_INFO, "recording to %s.\n", name );
	cls.demofile = FS_Open( name, "wb", false );
	cls.demotime = 0.0;

	if( !cls.demofile )
	{
		MsgDev( D_ERROR, "couldn't open %s.\n", name );
		return;
	}

	cls.demorecording = true;
	cls.demowaiting = true;	// don't start saving messages until a non-delta compressed message is received

	Q_memset( &demo.header, 0, sizeof( demo.header ));

	demo.header.id = IDEMOHEADER;
	demo.header.dem_protocol = DEMO_PROTOCOL;
	demo.header.net_protocol = PROTOCOL_VERSION;
	Q_strncpy( demo.header.mapname, clgame.mapname, sizeof( demo.header.mapname ));
	Q_strncpy( demo.header.gamedir, FS_Gamedir(), sizeof( demo.header.gamedir ));

	// write header
	FS_Write( cls.demofile, &demo.header, sizeof( demo.header ));

	demo.directory.numentries = 2;
	demo.directory.entries = Mem_Alloc( cls.mempool, sizeof( demoentry_t ) * demo.directory.numentries );

	// DIRECTORY ENTRY # 0
	demo.entry = &demo.directory.entries[0];	// only one here.
	demo.entry->entrytype = DEMO_STARTUP;
	demo.entry->playback_time = 0.0f;		// startup takes 0 time.
	demo.entry->offset = FS_Tell( cls.demofile );	// position for this chunk.

	// finish off the startup info.
	CL_WriteDemoCmdHeader( dem_stop, cls.demoheader );

	// now copy the stuff we cached from the server.
	copysize = savepos = FS_Tell( cls.demoheader );

	FS_Seek( cls.demoheader, 0, SEEK_SET );

	FS_FileCopy( cls.demofile, cls.demoheader, copysize );

	// jump back to end, in case we record another demo for this session.
	FS_Seek( cls.demoheader, savepos, SEEK_SET );

	demo.starttime = CL_GetDemoRecordClock();	// setup the demo starttime
	demo.realstarttime = demo.starttime;
	demo.framecount = 0;

	// now move on to entry # 1, the first data chunk.
	curpos = FS_Tell( cls.demofile );
	demo.entry->length = curpos - demo.entry->offset;

	// now we are writing the first real lump.
	demo.entry = &demo.directory.entries[1]; // first real data lump
	demo.entry->entrytype = DEMO_NORMAL;
	demo.entry->playback_time = 0.0f; // startup takes 0 time.

	demo.entry->offset = FS_Tell( cls.demofile );

	// demo playback should read this as an incoming message.
	// write the client's realtime value out so we can synchronize the reads.
	CL_WriteDemoCmdHeader( dem_jumptime, cls.demofile );

	if( clgame.hInstance ) clgame.dllFuncs.pfnReset();

	Cbuf_InsertText( "fullupdate\n" );
	Cbuf_Execute();
}

/*
=================
CL_StopRecord

finish recording demo
=================
*/
void CL_StopRecord( void )
{
	int	i, curpos;
	float	stoptime;

	if( !cls.demorecording ) return;

	// demo playback should read this as an incoming message.
	CL_WriteDemoCmdHeader( dem_stop, cls.demofile );

	stoptime = CL_GetDemoRecordClock();

	// close down the hud for now.
	// g-cont. is this need???
	if( clgame.hInstance ) clgame.dllFuncs.pfnReset();

	curpos = FS_Tell( cls.demofile );
	demo.entry->length = curpos - demo.entry->offset;
	demo.entry->playback_time = stoptime - demo.realstarttime;
	demo.entry->playback_frames = demo.framecount;

	//  Now write out the directory and free it and touch up the demo header.
	FS_Write( cls.demofile, &demo.directory.numentries, sizeof( int ));

	for( i = 0; i < demo.directory.numentries; i++ )
	{
		FS_Write( cls.demofile, &demo.directory.entries[i], sizeof( demoentry_t ));
	}

	Mem_Free( demo.directory.entries );
	demo.directory.numentries = 0;

	demo.header.directory_offset = curpos;
	FS_Seek( cls.demofile, 0, SEEK_SET );
	FS_Write( cls.demofile, &demo.header, sizeof( demo.header ));
	
	FS_Close( cls.demofile );
	cls.demofile = NULL;
	cls.demorecording = false;
	cls.demoname[0] = '\0';
	menu.globals->demoname[0] = '\0';

	Msg( "Completed demo\n" );
	MsgDev( D_INFO, "Recording time %.2f\n", cls.demotime );
	cls.demotime = 0.0;
}

/*
=================
CL_DrawDemoRecording
=================
*/
void CL_DrawDemoRecording( void )
{
	char		string[64];
	rgba_t		color = { 255, 255, 255, 255 };
	fs_offset_t	pos;
	int		len;

	if(!( host.developer && cls.demorecording ))
		return;

	pos = FS_Tell( cls.demofile );
	Q_snprintf( string, sizeof( string ), "RECORDING %s: %ik", cls.demoname, (int)(pos / 1024) );

	Con_DrawStringLen( string, &len, NULL );
	Con_DrawString(( scr_width->integer - len) >> 1, scr_height->integer >> 2, string, color );
}

/*
=======================================================================

CLIENT SIDE DEMO PLAYBACK

=======================================================================
*/
/*
=================
CL_ReadDemoCmdHeader

read the demo command
=================
*/
void CL_ReadDemoCmdHeader( byte *cmd, float *dt )
{
	// read the command
	FS_Read( cls.demofile, cmd, sizeof( byte ));
	ASSERT( *cmd >= 1 && *cmd <= dem_lastcmd );

	// read the timestamp
	FS_Read( cls.demofile, dt, sizeof( float ));
}

/*
=================
CL_ReadDemoUserCmd

read the demo usercmd for predicting
and smooth movement during playback the demo
=================
*/
void CL_ReadDemoUserCmd( qboolean discard )
{
	byte	data[1024];
	int	cmdnumber;
	int	outgoing_sequence;
	runcmd_t	*pcmd;
	word	bytes;

	FS_Read( cls.demofile, &outgoing_sequence, sizeof( int ));
	FS_Read( cls.demofile, &cmdnumber, sizeof( int ));
	FS_Read( cls.demofile, &bytes, sizeof( short ));
	FS_Read( cls.demofile, data, bytes );

	if( !discard )
	{
		usercmd_t	nullcmd;
		sizebuf_t	buf;

		Q_memset( &nullcmd, 0, sizeof( nullcmd ));
		BF_Init( &buf, "UserCmd", data, sizeof( data ));

		pcmd = &cl.commands[cmdnumber & CL_UPDATE_MASK];
		pcmd->processedfuncs = false;
		pcmd->senttime = 0.0f;
		pcmd->receivedtime = 0.1f;
		pcmd->frame_lerp = 0.1f;
		pcmd->heldback = false;
		pcmd->sendsize = 1;

		// always delta'ing from null
		cl.refdef.cmd = &pcmd->cmd;

		MSG_ReadDeltaUsercmd( &buf, &nullcmd, cl.refdef.cmd );

		// NOTE: we need to have the current outgoing sequence correct
		// so we can do prediction correctly during playback
		cls.netchan.outgoing_sequence = outgoing_sequence;
	}
}

/*
=================
CL_ReadDemoSequence

read netchan sequences
=================
*/
void CL_ReadDemoSequence( qboolean discard )
{
	int	incoming_sequence;
	int	incoming_acknowledged;
	int	incoming_reliable_acknowledged;
	int	incoming_reliable_sequence;
	int	outgoing_sequence;
	int	reliable_sequence;
	int	last_reliable_sequence;

	FS_Read( cls.demofile, &incoming_sequence, sizeof( int ));
	FS_Read( cls.demofile, &incoming_acknowledged, sizeof( int ));
	FS_Read( cls.demofile, &incoming_reliable_acknowledged, sizeof( int ));
	FS_Read( cls.demofile, &incoming_reliable_sequence, sizeof( int ));
	FS_Read( cls.demofile, &outgoing_sequence, sizeof( int ));
	FS_Read( cls.demofile, &reliable_sequence, sizeof( int ));
	FS_Read( cls.demofile, &last_reliable_sequence, sizeof( int ));

	if( discard ) return;

	cls.netchan.incoming_sequence	= incoming_sequence;
	cls.netchan.incoming_acknowledged = incoming_acknowledged;
	cls.netchan.incoming_reliable_acknowledged = incoming_reliable_acknowledged;
	cls.netchan.incoming_reliable_sequence = incoming_reliable_sequence;
	cls.netchan.outgoing_sequence	= outgoing_sequence;
	cls.netchan.reliable_sequence	= reliable_sequence;
	cls.netchan.last_reliable_sequence = last_reliable_sequence;
}

/*
=================
CL_DemoCompleted
=================
*/
void CL_DemoCompleted( void )
{
	if( cls.demonum != -1 )
		cls.changedemo = true;

	CL_StopPlayback();

	if( !CL_NextDemo() && host.developer <= 2 )
		UI_SetActiveMenu( true );
}

/*
=================
CL_DemoMoveToNextSection

returns true on success, false on failure
g-cont. probably captain obvious mode is ON
=================
*/
qboolean CL_DemoMoveToNextSection( void )
{
	if( ++demo.entryIndex >= demo.directory.numentries )
	{
		// done
		CL_DemoCompleted();
		return false;
	}

	// switch to next section, we got a dem_stop
	demo.entry = &demo.directory.entries[demo.entryIndex];
	
	// ready to continue reading, reset clock.
	FS_Seek( cls.demofile, demo.entry->offset, SEEK_SET ); 

	// time is now relative to this chunk's clock.
	demo.starttime = CL_GetDemoPlaybackClock();
	demo.framecount = 0;

	return true;
}

qboolean CL_ReadRawNetworkData( byte *buffer, size_t *length )
{
	int	msglen = 0;	

	ASSERT( buffer != NULL );
	ASSERT( length != NULL );

	*length = 0; // assume we fail
	FS_Read( cls.demofile, &msglen, sizeof( int ));

	if( msglen < 0 )
	{
		MsgDev( D_ERROR, "Demo message length < 0\n" );
		CL_DemoCompleted();
		return false;
	}
	
	if( msglen < 8 )
	{
		MsgDev( D_NOTE, "read runt demo message\n" );
	}

	if( msglen > NET_MAX_PAYLOAD )
	{
		MsgDev( D_ERROR, "Demo message %i > %i\n", msglen, NET_MAX_PAYLOAD );
		CL_DemoCompleted();
		return false;
	}

	if( msglen > 0 )
	{
		if( FS_Read( cls.demofile, buffer, msglen ) != msglen )
		{
			MsgDev( D_ERROR, "Error reading demo message data\n" );
			CL_DemoCompleted();
			return false;
		}
	}

	*length = msglen;

	if( cls.state != ca_active )
		Cbuf_Execute();

	return true;
}

/*
=================
CL_DemoReadMessage

reads demo data and write it to client
=================
*/
qboolean CL_DemoReadMessage( byte *buffer, size_t *length )
{
	float	f = 0.0f;
	int	curpos = 0;
	float	fElapsedTime = 0.0f;
	qboolean	swallowmessages = true;
	byte	*userbuf = NULL;
	size_t	size;
	byte	cmd;

	if( !cls.demofile )
	{
		MsgDev( D_ERROR, "tried to read a demo message with no demo file\n" );
		CL_DemoCompleted();
		return false;
	}

	// HACKHACK: changedemo issues
	if( !cls.netchan.remote_address.type )
		cls.netchan.remote_address.type = NA_LOOPBACK;

	if(( !cl.background && ( cl.refdef.paused || cls.key_dest != key_game )) || cls.key_dest == key_console )
	{
		demo.starttime += host.frametime;
		return false; // paused
	}

	do
	{
		qboolean	bSkipMessage = false;

		if( !cls.demofile ) break;
		curpos = FS_Tell( cls.demofile );

		CL_ReadDemoCmdHeader( &cmd, &f );

		fElapsedTime = CL_GetDemoPlaybackClock() - demo.starttime;
		bSkipMessage = (f >= fElapsedTime) ? true : false;

		if( cls.changelevel )
			demo.framecount = 1;

		// HACKHACK: changelevel issues
		if( demo.framecount <= 10 && ( fElapsedTime - f ) > host.frametime )
			demo.starttime = CL_GetDemoPlaybackClock();

		// not ready for a message yet, put it back on the file.
		if( cmd != dem_norewind && cmd != dem_stop && bSkipMessage )
		{
			// never skip first message
			if( demo.framecount != 0 )
			{
				FS_Seek( cls.demofile, curpos, SEEK_SET );
				return false; // not time yet.
			}
		}

		// COMMAND HANDLERS
		switch( cmd )
		{
		case dem_jumptime:
			demo.starttime = CL_GetDemoPlaybackClock();
			break;
		case dem_stop:
			CL_DemoMoveToNextSection();
			break;
		case dem_userdata:
			FS_Read( cls.demofile, &size, sizeof( int ));
			userbuf = Mem_Alloc( cls.mempool, size );
			FS_Read( cls.demofile, userbuf, size );

			if( clgame.hInstance )
				clgame.dllFuncs.pfnDemo_ReadBuffer( size, userbuf );
			Mem_Free( userbuf );
			userbuf = NULL;
			break;
		case dem_usercmd:
			CL_ReadDemoUserCmd( false );
			break;
		default:
			swallowmessages = false;
			break;
		}
	} while( swallowmessages );

	if( !cls.demofile )
		return false;

	// if not on "LOADING" section, check a few things
	if( demo.entryIndex )
	{
		// We are now on the second frame of a new section,
		// if so, reset start time (unless in a timedemo)
		if( demo.framecount == 1 )
		{
			// cheat by moving the relative start time forward.
			demo.starttime = CL_GetDemoPlaybackClock();
		}	
	}

	demo.framecount++;
	CL_ReadDemoSequence( false );

	return CL_ReadRawNetworkData( buffer, length );
}

/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/
void CL_StopPlayback( void )
{
	if( !cls.demoplayback ) return;

	// release demofile
	FS_Close( cls.demofile );
	cls.demoplayback = false;
	demo.framecount = 0;
	cls.demofile = NULL;

	cls.olddemonum = max( -1, cls.demonum - 1 );
	Mem_Free( demo.directory.entries );
	demo.directory.numentries = 0;
	demo.directory.entries = NULL;
	demo.entry = NULL;

	cls.demoname[0] = '\0';	// clear demoname too
	menu.globals->demoname[0] = '\0';

	S_StopAllSounds();
	S_StopBackgroundTrack();

	if( !cls.changedemo )
	{
		// let game known about demo state	
		Cvar_FullSet( "cl_background", "0", CVAR_READ_ONLY );
		cls.state = ca_disconnected;
		Q_memset( &cls.serveradr, 0, sizeof( cls.serveradr ) );
		cl.background = 0;
		cls.demonum = -1;
	}
}

/* 
================== 
CL_GetComment
================== 
*/  
qboolean CL_GetComment( const char *demoname, char *comment )
{
	file_t		*demfile;
	demoheader_t	demohdr;
	demodirectory_t	directory;
	demoentry_t	entry;
	float		playtime = 0.0f;
	int		i;
	
	if( !comment ) return false;

	demfile = FS_Open( demoname, "rb", false );
	if( !demfile )
	{
		Q_strncpy( comment, "", MAX_STRING );
		return false;
	}

	// read in the m_DemoHeader
	FS_Read( demfile, &demohdr, sizeof( demoheader_t ));

	if( demohdr.id != IDEMOHEADER )
	{
		FS_Close( demfile );
		Q_strncpy( comment, "<corrupted>", MAX_STRING );
		return false;
	}

	if( demohdr.net_protocol != PROTOCOL_VERSION || demohdr.dem_protocol != DEMO_PROTOCOL )
	{
		FS_Close( demfile );
		Q_strncpy( comment, "<invalid protocol>", MAX_STRING );
		return false;
	}

	// now read in the directory structure.
	FS_Seek( demfile, demohdr.directory_offset, SEEK_SET );
	FS_Read( demfile, &directory.numentries, sizeof( int ));

	if( directory.numentries < 1 || directory.numentries > 1024 )
	{
		FS_Close( demfile );
		Q_strncpy( comment, "<corrupted>", MAX_STRING );
		return false;
	}

	for( i = 0; i < directory.numentries; i++ )
	{
		FS_Read( demfile, &entry, sizeof( demoentry_t ));
		playtime += entry.playback_time;
	}

	// split comment to sections
	Q_strncpy( comment, demohdr.mapname, CS_SIZE );
	Q_strncpy( comment + CS_SIZE, "<No Title>", CS_SIZE );	// TODO: write titles or somewhat
	Q_strncpy( comment + CS_SIZE * 2, va( "%g sec", playtime ), CS_TIME );

	// all done
	FS_Close( demfile );
		
	return true;
}

/*
==================
CL_NextDemo

Called when a demo or cinematic finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
qboolean CL_NextDemo( void )
{
	string	str;

	if( cls.demonum == -1 )
		return false; // don't play demos
	S_StopAllSounds();

	if( !cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS )
	{
		cls.demonum = 0;
		if( !cls.demos[cls.demonum][0] )
		{
			MsgDev( D_INFO, "no demos listed with startdemos\n" );
			cls.demonum = -1;
			return false;
		}
	}

	Q_snprintf( str, MAX_STRING, "playdemo %s\n", cls.demos[cls.demonum] );

	Cbuf_InsertText( str );
	cls.demonum++;

	return true;
}

/* 
================== 
CL_DemoGetName
================== 
*/  
void CL_DemoGetName( int lastnum, char *filename )
{
	if( !filename )
		return;

	if( lastnum < 0 || lastnum > 9999 )
	{
		// bound
		Q_strcpy( filename, "demo9999" );
		return;
	}


	Q_sprintf( filename, "demo%04d", lastnum );
}

/*
====================
CL_Record_f

record <demoname>
Begins recording a demo from the current position
====================
*/
void CL_Record_f( void )
{
	const char	*name;
	string		demoname, demopath, demoshot;
	int		n;

	if( Cmd_Argc() == 1 )
	{
		name = "new";
	}
	else if( Cmd_Argc() == 2 )
	{
		name = Cmd_Argv( 1 );
	}
	else
	{
		Msg( "Usage: record <demoname>\n" );
		return;
	}

	if( cls.demorecording )
	{
		Msg( "Already recording.\n");
		return;
	}

	if( cls.demoplayback )
	{
		Msg( "Can't record during demo playback.\n");
		return;
	}

	if( !cls.demoheader || cls.state != ca_active )
	{
		Msg( "You must be in a level to record.\n");
		return;
	}

	if( !Q_stricmp( name, "new" ))
	{
		// scan for a free filename
		for( n = 0; n < 10000; n++ )
		{
			CL_DemoGetName( n, demoname );
			if( !FS_FileExists( va( "demos/%s.dem", demoname ), true ))
				break;
		}

		if( n == 10000 )
		{
			Msg( "^3ERROR: no free slots for demo recording\n" );
			return;
		}
	}
	else Q_strncpy( demoname, name, sizeof( demoname ));

	// open the demo file
	Q_sprintf( demopath, "demos/%s.dem", demoname );
	Q_sprintf( demoshot, "demos/%s.bmp", demoname );

	// make sure what old demo is removed
	if( FS_FileExists( demopath, false )) FS_Delete( demopath );
	if( FS_FileExists( demoshot, false )) FS_Delete( demoshot );

	// write demoshot for preview
	Cbuf_AddText( va( "demoshot \"%s\"\n", demoname ));
	Q_strncpy( cls.demoname, demoname, sizeof( cls.demoname ));
	Q_strncpy( menu.globals->demoname, demoname, sizeof( menu.globals->demoname ));
	
	CL_WriteDemoHeader( demopath );
}

/*
====================
CL_PlayDemo_f

playdemo <demoname>
====================
*/
void CL_PlayDemo_f( void )
{
	string	filename;
	string	demoname;
	int	i;

	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: playdemo <demoname>\n" );
		return;
	}

	if( cls.demoplayback )
	{
		CL_StopPlayback();
	}

	if( cls.demorecording )
	{
		Msg( "Can't playback during demo record.\n");
		return;
	}

	Q_strncpy( demoname, Cmd_Argv( 1 ), sizeof( demoname ) - 1 );
	Q_snprintf( filename, sizeof( filename ), "demos/%s.dem", demoname );

	if( !FS_FileExists( filename, true ))
	{
		MsgDev( D_ERROR, "couldn't open %s\n", filename );
		cls.demonum = -1; // stop demo loop
		return;
	}

	cls.demofile = FS_Open( filename, "rb", true );
	Q_strncpy( cls.demoname, demoname, sizeof( cls.demoname ));
	Q_strncpy( menu.globals->demoname, demoname, sizeof( menu.globals->demoname ));

	// read in the m_DemoHeader
	FS_Read( cls.demofile, &demo.header, sizeof( demoheader_t ));

	if( demo.header.id != IDEMOHEADER )
	{
		MsgDev( D_ERROR, "%s is not a demo file\n", filename );
		FS_Close( cls.demofile );
		cls.demofile = NULL;
		cls.demonum = -1; // stop demo loop
		return;
	}

	if( demo.header.net_protocol != PROTOCOL_VERSION || demo.header.dem_protocol != DEMO_PROTOCOL )
	{
		MsgDev( D_ERROR, "demo protocol outdated\n"
			"Demo file protocols Network(%i), Demo(%i)\n"
			"Server protocol is at Network(%i), Demo(%i)\n",
			demo.header.net_protocol, 
			demo.header.dem_protocol,
			PROTOCOL_VERSION,
			DEMO_PROTOCOL
		);

		FS_Close( cls.demofile );
		cls.demofile = NULL;
		cls.demonum = -1; // stop demo loop
		return;
	}

	// now read in the directory structure.
	FS_Seek( cls.demofile, demo.header.directory_offset, SEEK_SET );
	FS_Read( cls.demofile, &demo.directory.numentries, sizeof( int ));

	if( demo.directory.numentries < 1 || demo.directory.numentries > 1024 )
	{
		MsgDev( D_ERROR, "demo had bogus # of directory entries: %i\n", demo.directory.numentries );
		FS_Close( cls.demofile );
		cls.demofile = NULL;
		cls.demonum = -1; // stop demo loop
		cls.changedemo = false;
		return;
	}

	if( cls.changedemo )
	{
		S_StopAllSounds();
		SCR_BeginLoadingPlaque( false );

		CL_ClearState ();
		CL_InitEdicts (); // re-arrange edicts
	}
	else
	{
		// NOTE: at this point demo is still valid
		CL_Disconnect();
		Host_ShutdownServer();

		Con_Close();
		UI_SetActiveMenu( false );
	}

	// allocate demo entries
	demo.directory.entries = Mem_Alloc( cls.mempool, sizeof( demoentry_t ) * demo.directory.numentries );

	for( i = 0; i < demo.directory.numentries; i++ )
	{
		FS_Read( cls.demofile, &demo.directory.entries[i], sizeof( demoentry_t ));
	}

	demo.entryIndex = 0;
	demo.entry = &demo.directory.entries[demo.entryIndex];

	FS_Seek( cls.demofile, demo.entry->offset, SEEK_SET );

	cls.demoplayback = true;
	cls.state = ca_connected;
	cl.background = (cls.demonum != -1) ? true : false;

	demo.starttime = CL_GetDemoPlaybackClock(); // for determining whether to read another message

	Netchan_Setup( NS_CLIENT, &cls.netchan, net_from, net_qport->integer );

	demo.framecount = 0;
	cls.lastoutgoingcommand = -1;
 	cls.nextcmdtime = host.realtime;

	// g-cont. is this need?
	Q_strncpy( cls.servername, demoname, sizeof( cls.servername ));

	// begin a playback demo
}

/*
==================
CL_StartDemos_f
==================
*/
void CL_StartDemos_f( void )
{
	int	i, c;

	if( cls.key_dest != key_menu )
	{
		MsgDev( D_INFO, "startdemos is not valid from the console\n" );
		return;
	}

	c = Cmd_Argc() - 1;
	if( c > MAX_DEMOS )
	{
		MsgDev( D_WARN, "Host_StartDemos: max %i demos in demoloop\n", MAX_DEMOS );
		c = MAX_DEMOS;
	}

	MsgDev( D_INFO, "%i demo%s in loop\n", c, (c > 1) ? "s" : "" );

	for( i = 1; i < c + 1; i++ )
		Q_strncpy( cls.demos[i-1], Cmd_Argv( i ), sizeof( cls.demos[0] ));

	if( !SV_Active() && !cls.demoplayback )
	{
		// run demos loop in background mode
		Cvar_SetFloat( "v_dark", 1.0f );
		cls.demonum = 0;
		CL_NextDemo ();
	}
	else cls.demonum = -1;
}

/*
==================
CL_Demos_f

Return to looping demos
==================
*/
void CL_Demos_f( void )
{
	if( cls.key_dest != key_menu )
	{
		MsgDev( D_INFO, "demos is not valid from the console\n" );
		return;
	}

	cls.demonum = cls.olddemonum;

	if( cls.demonum == -1 )
		cls.demonum = 0;

	if( !SV_Active() && !cls.demoplayback )
	{
		// run demos loop in background mode
		cls.changedemo = true;
		CL_NextDemo ();
	}
}


/*
====================
CL_Stop_f

stop any client activity
====================
*/
void CL_Stop_f( void )
{
	// stop all
	CL_StopRecord();
	CL_StopPlayback();
	SCR_StopCinematic();

	// stop background track that was runned from the console
	if( !SV_Active( ))
	{
		S_StopBackgroundTrack();
	}
}
#endif // XASH_DEDICATED
