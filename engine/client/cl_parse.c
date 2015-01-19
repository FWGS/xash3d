/*
cl_parse.c - parse a message received from the server
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
#include "net_encode.h"
#include "particledef.h"
#include "gl_local.h"
#include "cl_tent.h"
#include "shake.h"

#define MSG_COUNT		32		// last 32 messages parsed
#define MSG_MASK		(MSG_COUNT - 1)

int CL_UPDATE_BACKUP = SINGLEPLAYER_BACKUP;

const char *svc_strings[256] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_changing",
	"svc_version",
	"svc_setview",
	"svc_sound",
	"svc_time",
	"svc_print",
	"svc_stufftext",
	"svc_setangle",
	"svc_serverdata",
	"svc_lightstyle",
	"svc_updateuserinfo",
	"svc_deltatable",
	"svc_clientdata",
	"svc_stopsound",
	"svc_updatepings",
	"svc_particle",
	"svc_restoresound",
	"svc_spawnstatic",
	"svc_event_reliable",
	"svc_spawnbaseline",
	"svc_temp_entity",
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_event",
	"svc_soundindex",
	"svc_ambientsound",
	"svc_intermission",
	"svc_modelindex",
	"svc_cdtrack",
	"svc_serverinfo",
	"svc_eventindex",
	"svc_weaponanim",
	"svc_bspdecal",
	"svc_roomtype",
	"svc_addangle",
	"svc_usermessage",
	"svc_packetentities",
	"svc_deltapacketentities",
	"svc_chokecount",
	"svc_resourcelist",
	"svc_deltamovevars",
	"svc_customization",
	"svc_unused46",
	"svc_crosshairangle",
	"svc_soundfade",
	"svc_unused49",
	"svc_unused50",
	"svc_director",
	"svc_studiodecal",
	"svc_unused53",
	"svc_unused54",
	"svc_unused55",
	"svc_unused56",
	"svc_querycvarvalue",
	"svc_querycvarvalue2",
	"svc_unused59",
	"svc_unused60",
	"svc_unused61",
	"svc_unused62",
	"svc_unused63",
};

typedef struct
{
	int	command;
	int	starting_offset;
	int	frame_number;
} oldcmd_t;

typedef struct
{
	oldcmd_t	oldcmd[MSG_COUNT];   
	int	currentcmd;
	qboolean	parsing;
} msg_debug_t;

static msg_debug_t		cls_message_debug;
static int		starting_count;
static resourcelist_t	reslist;

const char *CL_MsgInfo( int cmd )
{
	static string	sz;

	Q_strcpy( sz, "???" );

	if( cmd >= 0 && cmd < svc_lastmsg )
	{
		// get engine message name
		Q_strncpy( sz, svc_strings[cmd], sizeof( sz ));
	}
	else if( cmd >= svc_lastmsg && cmd < ( svc_lastmsg + MAX_USER_MESSAGES ))
	{
		int	i;

		for( i = 0; i < MAX_USER_MESSAGES; i++ )
		{
			if( clgame.msg[i].number == cmd )
			{
				Q_strncpy( sz, clgame.msg[i].name, sizeof( sz ));
				break;
			}
		}
	}
	return sz;
}

/*
=====================
CL_Parse_RecordCommand

record new message params into debug buffer
=====================
*/
void CL_Parse_RecordCommand( int cmd, int startoffset )
{
	int	slot;

	if( cmd == svc_nop ) return;
	
	slot = ( cls_message_debug.currentcmd++ & MSG_MASK );
	cls_message_debug.oldcmd[slot].command = cmd;
	cls_message_debug.oldcmd[slot].starting_offset = startoffset;
	cls_message_debug.oldcmd[slot].frame_number = host.framecount;
}

/*
=====================
CL_WriteErrorMessage

write net_message into buffer.dat for debugging
=====================
*/
void CL_WriteErrorMessage( int current_count, sizebuf_t *msg )
{
	file_t		*fp;
	const char	*buffer_file = "buffer.dat";
	
	fp = FS_Open( buffer_file, "wb", false );
	if( !fp ) return;

	FS_Write( fp, &starting_count, sizeof( int ));
	FS_Write( fp, &current_count, sizeof( int ));
	FS_Write( fp, BF_GetData( msg ), BF_GetMaxBytes( msg ));
	FS_Close( fp );

	MsgDev( D_INFO, "Wrote erroneous message to %s\n", buffer_file );
}

/*
=====================
CL_WriteMessageHistory

list last 32 messages for debugging net troubleshooting
=====================
*/
void CL_WriteMessageHistory( void )
{
	int	i, thecmd;
	oldcmd_t	*old, *failcommand;
	sizebuf_t	*msg = &net_message;

	if( !cls.initialized || cls.state == ca_disconnected )
		return;

	if( !cls_message_debug.parsing )
		return;

	MsgDev( D_INFO, "Last %i messages parsed.\n", MSG_COUNT );

	// finish here
	thecmd = cls_message_debug.currentcmd - 1;
	thecmd -= ( MSG_COUNT - 1 );	// back up to here

	for( i = 0; i < MSG_COUNT - 1; i++ )
	{
		thecmd &= MSG_MASK;
		old = &cls_message_debug.oldcmd[thecmd];

		MsgDev( D_INFO,"%i %04i %s\n", old->frame_number, old->starting_offset, CL_MsgInfo( old->command ));

		thecmd++;
	}

	failcommand = &cls_message_debug.oldcmd[thecmd];
	MsgDev( D_INFO, "BAD:  %3i:%s\n", BF_GetNumBytesRead( msg ) - 1, CL_MsgInfo( failcommand->command ));

	if( host.developer >= 3 )
	{
		CL_WriteErrorMessage( BF_GetNumBytesRead( msg ) - 1, msg );
	}
	cls_message_debug.parsing = false;
}

/*
===============
CL_UserMsgStub

Default stub for missed callbacks
===============
*/
int CL_UserMsgStub( const char *pszName, int iSize, void *pbuf )
{
	return 1;
}

/*
==================
CL_ParseSoundPacket

==================
*/
void CL_ParseSoundPacket( sizebuf_t *msg, qboolean is_ambient )
{
	vec3_t	pos;
	int 	chan, sound;
	float 	volume, attn;  
	int	flags, pitch, entnum;
	sound_t	handle = 0;

	flags = BF_ReadWord( msg );
	if( flags & SND_LARGE_INDEX )
		sound = BF_ReadWord( msg );
	else sound = BF_ReadByte( msg );
	chan = BF_ReadByte( msg );

	if( flags & SND_VOLUME )
		volume = (float)BF_ReadByte( msg ) / 255.0f;
	else volume = VOL_NORM;

	if( flags & SND_ATTENUATION )
		attn = (float)BF_ReadByte( msg ) / 64.0f;
	else attn = ATTN_NONE;	

	if( flags & SND_PITCH )
		pitch = BF_ReadByte( msg );
	else pitch = PITCH_NORM;

	// entity reletive
	entnum = BF_ReadWord( msg ); 

	// positioned in space
	BF_ReadVec3Coord( msg, pos );

	if( flags & SND_SENTENCE )
	{
		char	sentenceName[32];

		Q_snprintf( sentenceName, sizeof( sentenceName ), "!%i", sound );
		handle = S_RegisterSound( sentenceName );
	}
	else handle = cl.sound_index[sound];	// see precached sound

	if( is_ambient )
	{
		S_AmbientSound( pos, entnum, handle, volume, attn, pitch, flags );
	}
	else
	{
		S_StartSound( pos, entnum, chan, handle, volume, attn, pitch, flags );
	}
}

/*
==================
CL_ParseRestoreSoundPacket

==================
*/
void CL_ParseRestoreSoundPacket( sizebuf_t *msg )
{
	vec3_t	pos;
	int 	chan, sound;
	float 	volume, attn;  
	int	flags, pitch, entnum;
	double	samplePos, forcedEnd;
	int	wordIndex;
	sound_t	handle = 0;

	flags = BF_ReadWord( msg );
	if( flags & SND_LARGE_INDEX )
		sound = BF_ReadWord( msg );
	else sound = BF_ReadByte( msg );
	chan = BF_ReadByte( msg );

	if( flags & SND_VOLUME )
		volume = (float)BF_ReadByte( msg ) / 255.0f;
	else volume = VOL_NORM;

	if( flags & SND_ATTENUATION )
		attn = (float)BF_ReadByte( msg ) / 64.0f;
	else attn = ATTN_NONE;	

	if( flags & SND_PITCH )
		pitch = BF_ReadByte( msg );
	else pitch = PITCH_NORM;

	if( flags & SND_SENTENCE )
	{
		char	sentenceName[32];

		Q_snprintf( sentenceName, sizeof( sentenceName ), "!%i", sound );
		handle = S_RegisterSound( sentenceName );
	}
	else handle = cl.sound_index[sound]; // see precached sound

	// entity reletive
	entnum = BF_ReadWord( msg ); 

	// positioned in space
	BF_ReadVec3Coord( msg, pos );
	wordIndex = BF_ReadByte( msg );

	// 16 bytes here
	BF_ReadBytes( msg, &samplePos, sizeof( samplePos ));
	BF_ReadBytes( msg, &forcedEnd, sizeof( forcedEnd ));

	S_RestoreSound( pos, entnum, chan, handle, volume, attn, pitch, flags, samplePos, forcedEnd, wordIndex );
}

/*
==================
CL_ParseMovevars

==================
*/
void CL_ParseMovevars( sizebuf_t *msg )
{
	Delta_InitClient ();	// finalize client delta's

	MSG_ReadDeltaMovevars( msg, &clgame.oldmovevars, &clgame.movevars );

	// update sky if changed
	if( Q_strcmp( clgame.oldmovevars.skyName, clgame.movevars.skyName ) && cl.video_prepped )
		R_SetupSky( clgame.movevars.skyName );

	Q_memcpy( &clgame.oldmovevars, &clgame.movevars, sizeof( movevars_t ));
	// keep features an actual!
	clgame.oldmovevars.features = clgame.movevars.features = host.features;
}

/*
==================
CL_ParseParticles

==================
*/
void CL_ParseParticles( sizebuf_t *msg )
{
	vec3_t		org, dir;
	int		i, count, color;
	float		life;
	
	BF_ReadVec3Coord( msg, org );	

	for( i = 0; i < 3; i++ )
		dir[i] = BF_ReadChar( msg ) * (1.0f / 16);

	count = BF_ReadByte( msg );
	color = BF_ReadByte( msg );
	if( count == 255 ) count = 1024;
	life = BF_ReadByte( msg ) * 0.125f;

	if( life != 0.0f && count == 1 )
	{
		particle_t	*p;

		p = CL_AllocParticle( NULL );
		if( !p ) return;

		p->die += life;
		p->color = color;
		p->type = pt_static;

		VectorCopy( org, p->org );
		VectorCopy( dir, p->vel );
	}
	else CL_RunParticleEffect( org, dir, color, count );
}

/*
==================
CL_ParseStaticEntity

static client entity
==================
*/
void CL_ParseStaticEntity( sizebuf_t *msg )
{
	entity_state_t	state;
	cl_entity_t	*ent;
	int		i;

	Q_memset( &state, 0, sizeof( state ));

	state.modelindex = BF_ReadShort( msg );
	state.sequence = BF_ReadByte( msg );
	state.frame = BF_ReadByte( msg );
	state.colormap = BF_ReadWord( msg );
	state.skin = BF_ReadByte( msg );

	for( i = 0; i < 3; i++ )
	{
		state.origin[i] = BF_ReadCoord( msg );
		state.angles[i] = BF_ReadBitAngle( msg, 16 );
	}

	state.rendermode = BF_ReadByte( msg );

	if( state.rendermode != kRenderNormal )
	{
		state.renderamt = BF_ReadByte( msg );
		state.rendercolor.r = BF_ReadByte( msg );
		state.rendercolor.g = BF_ReadByte( msg );
		state.rendercolor.b = BF_ReadByte( msg );
		state.renderfx = BF_ReadByte( msg );
	}

	i = clgame.numStatics;
	if( i >= MAX_STATIC_ENTITIES )
	{
		MsgDev( D_ERROR, "CL_ParseStaticEntity: static entities limit exceeded!\n" );
		return;
	}

	ent = &clgame.static_entities[i];
	clgame.numStatics++;

	ent->index = 0; // ???
	ent->baseline = state;
	ent->curstate = state;
	ent->prevstate = state;

	// statics may be respawned in game e.g. for demo recording
	if( cls.state == ca_connected )
		ent->trivial_accept = INVALID_HANDLE;

	// setup the new static entity
	CL_UpdateEntityFields( ent );

	if( Mod_GetType( state.modelindex ) == mod_studio )
	{
		CL_UpdateStudioVars( ent, &state, true );

		// animate studio model
		ent->curstate.animtime = cl.time;
		ent->curstate.framerate = 1.0f;
		ent->latched.prevframe = 0.0f;
	}

	R_AddEfrags( ent );	// add link
}

/*
==================
CL_ParseStaticDecal

==================
*/
void CL_ParseStaticDecal( sizebuf_t *msg )
{
	vec3_t		origin;
	int		decalIndex, entityIndex, modelIndex;
	cl_entity_t	*ent = NULL;
	float		scale;
	int		flags;

	BF_ReadVec3Coord( msg, origin );
	decalIndex = BF_ReadWord( msg );
	entityIndex = BF_ReadShort( msg );

	if( entityIndex > 0 )
		modelIndex = BF_ReadWord( msg );
	else modelIndex = 0;
	flags = BF_ReadByte( msg );
	scale = (float)BF_ReadWord( msg ) / 4096.0f;

	CL_FireCustomDecal( CL_DecalIndex( decalIndex ), entityIndex, modelIndex, origin, flags, scale );
}

/*
==================
CL_ParseSoundFade

==================
*/
void CL_ParseSoundFade( sizebuf_t *msg )
{
	float	fadePercent, fadeOutSeconds;
	float	holdTime, fadeInSeconds;

	fadePercent = (float)BF_ReadByte( msg );
	holdTime = (float)BF_ReadByte( msg );
	fadeOutSeconds = (float)BF_ReadByte( msg );
	fadeInSeconds = (float)BF_ReadByte( msg );

	S_FadeClientVolume( fadePercent, fadeOutSeconds, holdTime, fadeInSeconds );
}

/*
==================
CL_ParseCustomization

==================
*/
void CL_ParseCustomization( sizebuf_t *msg )
{
	// TODO: ???
}

/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/
/*
==================
CL_ParseServerData
==================
*/
void CL_ParseServerData( sizebuf_t *msg )
{
	string	gamefolder;
	qboolean	background;
	int	i;

	MsgDev( D_NOTE, "Serverdata packet received.\n" );

	cls.demowaiting = false;	// server is changed
	clgame.load_sequence++;	// now all hud sprites are invalid

	// wipe the client_t struct
	if( !cls.changelevel && !cls.changedemo )
		CL_ClearState ();
	cls.state = ca_connected;

	// parse protocol version number
	i = BF_ReadLong( msg );
	cls.serverProtocol = i;

	if( i != PROTOCOL_VERSION )
		Host_Error( "Server use invalid protocol (%i should be %i)\n", i, PROTOCOL_VERSION );

	cl.servercount = BF_ReadLong( msg );
	cl.checksum = BF_ReadLong( msg );
	cl.playernum = BF_ReadByte( msg );
	cl.maxclients = BF_ReadByte( msg );
	clgame.maxEntities = BF_ReadWord( msg );
	clgame.maxEntities = bound( 600, clgame.maxEntities, 4096 );
	Q_strncpy( clgame.mapname, BF_ReadString( msg ), MAX_STRING );
	Q_strncpy( clgame.maptitle, BF_ReadString( msg ), MAX_STRING );
	background = BF_ReadOneBit( msg );
	Q_strncpy( gamefolder, BF_ReadString( msg ), MAX_STRING );
	host.features = (uint)BF_ReadLong( msg );

	if( cl.maxclients > 1 && host.developer < 1 )
		host.developer++;

	// set the background state
	if( cls.demoplayback && ( cls.demonum != -1 ))
	{
		// re-init mouse
		host.mouse_visible = false;
		cl.background = true;
	}
	else cl.background = background;

	if( cl.background )	// tell the game parts about background state
		Cvar_FullSet( "cl_background", "1", CVAR_READ_ONLY );
	else Cvar_FullSet( "cl_background", "0", CVAR_READ_ONLY );

	if( !cls.changelevel ) 
	{
		// continue playing if we are changing level
		S_StopBackgroundTrack ();
	}
#if 0
	// NOTE: this is not tested as well. Use with precaution
	CL_ChangeGame( gamefolder, false );
#endif
	if( !cls.changedemo )
		UI_SetActiveMenu( cl.background );
	else if( !cls.demoplayback )
		Key_SetKeyDest( key_menu );

	cl.refdef.viewentity = cl.playernum + 1; // always keep viewent an actual

	menu.globals->maxClients = cl.maxclients;
	Q_strncpy( menu.globals->maptitle, clgame.maptitle, sizeof( menu.globals->maptitle ));

	if( !cls.changelevel && !cls.changedemo )
		CL_InitEdicts (); // re-arrange edicts

	// get splash name
	if( cls.demoplayback && ( cls.demonum != -1 ))
		Cvar_Set( "cl_levelshot_name", va( "levelshots/%s_%s", cls.demoname, glState.wideScreen ? "16x9" : "4x3" ));
	else Cvar_Set( "cl_levelshot_name", va( "levelshots/%s_%s", clgame.mapname, glState.wideScreen ? "16x9" : "4x3" ));
	Cvar_SetFloat( "scr_loading", 0.0f ); // reset progress bar

	if(( cl_allow_levelshots->integer && !cls.changelevel ) || cl.background )
	{
		if( !FS_FileExists( va( "%s.bmp", cl_levelshot_name->string ), true )) 
			Cvar_Set( "cl_levelshot_name", "*black" ); // render a black screen
		cls.scrshot_request = scrshot_plaque; // request levelshot even if exist (check filetime)
	}

	if( scr_dark->integer )
	{
		screenfade_t		*sf = &clgame.fade;
		client_textmessage_t	*title;

		title = CL_TextMessageGet( "GAMETITLE" );

		if( title )
		{
			// get settings from titles.txt
			sf->fadeEnd = title->holdtime + title->fadeout;
			sf->fadeReset = title->fadeout;
		}
		else sf->fadeEnd = sf->fadeReset = 4.0f;
	
		sf->fadeFlags = FFADE_IN;
		sf->fader = sf->fadeg = sf->fadeb = 0;
		sf->fadealpha = 255;
		sf->fadeSpeed = (float)sf->fadealpha / sf->fadeReset;
		sf->fadeReset += cl.time;
		sf->fadeEnd += sf->fadeReset;
		
		Cvar_SetFloat( "v_dark", 0.0f );
	}

	// need to prep refresh at next oportunity
	cl.video_prepped = false;
	cl.audio_prepped = false;

	Q_memset( &clgame.movevars, 0, sizeof( clgame.movevars ));
	Q_memset( &clgame.oldmovevars, 0, sizeof( clgame.oldmovevars ));
}

/*
===================
CL_ParseClientData
===================
*/
void CL_ParseClientData( sizebuf_t *msg )
{
	int		i, j;
	clientdata_t	*from_cd, *to_cd;
	weapon_data_t	*from_wd, *to_wd;
	weapon_data_t	nullwd[32];
	clientdata_t	nullcd;
	frame_t		*frame;
	int		idx;

	// this is the frame update that this message corresponds to
	i = cls.netchan.incoming_sequence;

	// did we drop some frames?
	if( i > cl.last_incoming_sequence + 1 )
	{
		// mark as dropped
		for( j = cl.last_incoming_sequence + 1; j < i; j++ )
		{
			if( cl.frames[j & CL_UPDATE_MASK].receivedtime >= 0.0 )
			{
				cl.frames[j & CL_UPDATE_MASK].receivedtime = -1;
				cl.frames[j & CL_UPDATE_MASK].latency = 0;
			}
		}
	}

	cl.parsecount = i;					// ack'd incoming messages.  
	cl.parsecountmod = cl.parsecount & CL_UPDATE_MASK;	// index into window.     
	frame = &cl.frames[cl.parsecountmod];			// frame at index.

	frame->time = cl.mtime[0];				// mark network received time
	frame->receivedtime = host.realtime;			// time now that we are parsing.  

	// do this after all packets read for this frame?
	cl.last_incoming_sequence = cls.netchan.incoming_sequence;

	if( hltv->integer ) return;	// clientdata for spectators ends here
	
	to_cd = &frame->local.client;
	to_wd = frame->local.weapondata;

	// clear to old value before delta parsing
	if( !BF_ReadOneBit( msg ))
	{
		Q_memset( &nullcd, 0, sizeof( nullcd ));
		Q_memset( nullwd, 0, sizeof( nullwd ));
		from_cd = &nullcd;
		from_wd = nullwd;
	}
	else
	{
		int	delta_sequence = BF_ReadByte( msg );

		from_cd = &cl.frames[delta_sequence & CL_UPDATE_MASK].local.client;
		from_wd = cl.frames[delta_sequence & CL_UPDATE_MASK].local.weapondata;
	}

	MSG_ReadClientData( msg, from_cd, to_cd, cl.mtime[0] );

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		// check for end of weapondata (and clientdata_t message)
		if( !BF_ReadOneBit( msg )) break;

		// read the weapon idx
		idx = BF_ReadUBitLong( msg, MAX_WEAPON_BITS );

		MSG_ReadWeaponData( msg, &from_wd[idx], &to_wd[idx], cl.mtime[0] );
	}
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline( sizebuf_t *msg )
{
	int		newnum;
	float		timebase;
	cl_entity_t	*ent;

	Delta_InitClient ();	// finalize client delta's

	newnum = BF_ReadWord( msg );

	if( newnum < 0 ) Host_Error( "CL_SpawnEdict: invalid number %i\n", newnum );
	if( newnum >= clgame.maxEntities ) Host_Error( "CL_AllocEdict: no free edicts\n" );

	ent = CL_EDICT_NUM( newnum );
	Q_memset( &ent->prevstate, 0, sizeof( ent->prevstate ));
	ent->index = newnum;

	if( cls.state == ca_active )
		timebase = cl.mtime[0];
	else timebase = 1.0f; // sv.state == ss_loading

	MSG_ReadDeltaEntity( msg, &ent->prevstate, &ent->baseline, newnum, CL_IsPlayerIndex( newnum ), timebase );
}

/*
================
CL_ParseLightStyle
================
*/
void CL_ParseLightStyle( sizebuf_t *msg )
{
	int		style;
	const char	*s;
	float		f;

	style = BF_ReadByte( msg );
	s = BF_ReadString( msg );
	f = BF_ReadFloat( msg );

	CL_SetLightstyle( style, s, f );
}

/*
================
CL_ParseSetAngle

set the view angle to this absolute value
================
*/
void CL_ParseSetAngle( sizebuf_t *msg )
{
	cl.refdef.cl_viewangles[0] = BF_ReadBitAngle( msg, 16 );
	cl.refdef.cl_viewangles[1] = BF_ReadBitAngle( msg, 16 );
	cl.refdef.cl_viewangles[2] = BF_ReadBitAngle( msg, 16 );
}

/*
================
CL_ParseAddAngle

add the view angle yaw
================
*/
void CL_ParseAddAngle( sizebuf_t *msg )
{
	float	add_angle;
	
	add_angle = BF_ReadBitAngle( msg, 16 );
	cl.refdef.cl_viewangles[1] += add_angle;
}

/*
================
CL_ParseCrosshairAngle

offset crosshair angles
================
*/
void CL_ParseCrosshairAngle( sizebuf_t *msg )
{
	cl.refdef.crosshairangle[0] = BF_ReadChar( msg ) * 0.2f;
	cl.refdef.crosshairangle[1] = BF_ReadChar( msg ) * 0.2f;
	cl.refdef.crosshairangle[2] = 0.0f; // not used for screen space
}

/*
================
CL_RegisterUserMessage

register new user message or update existing
================
*/
void CL_RegisterUserMessage( sizebuf_t *msg )
{
	char	*pszName;
	int	svc_num, size;
	
	svc_num = BF_ReadByte( msg );
	size = BF_ReadByte( msg );
	pszName = BF_ReadString( msg );

	// important stuff
	if( size == 0xFF ) size = -1;
	svc_num = bound( 0, svc_num, 255 );

	CL_LinkUserMessage( pszName, svc_num, size );
}

/*
================
CL_UpdateUserinfo

collect userinfo from all players
================
*/
void CL_UpdateUserinfo( sizebuf_t *msg )
{
	int		slot;
	qboolean		active;
	player_info_t	*player;

	slot = BF_ReadUBitLong( msg, MAX_CLIENT_BITS );

	if( slot >= MAX_CLIENTS )
		Host_Error( "CL_ParseServerMessage: svc_updateuserinfo > MAX_CLIENTS\n" );

	player = &cl.players[slot];
	active = BF_ReadOneBit( msg ) ? true : false;

	if( active )
	{
		Q_strncpy( player->userinfo, BF_ReadString( msg ), sizeof( player->userinfo ));
		Q_strncpy( player->name, Info_ValueForKey( player->userinfo, "name" ), sizeof( player->name ));
		Q_strncpy( player->model, Info_ValueForKey( player->userinfo, "model" ), sizeof( player->model ));
		player->topcolor = Q_atoi( Info_ValueForKey( player->userinfo, "topcolor" ));
		player->bottomcolor = Q_atoi( Info_ValueForKey( player->userinfo, "bottomcolor" ));

		if( slot == cl.playernum ) Q_memcpy( &menu.playerinfo, player, sizeof( player_info_t ));
	}
	else Q_memset( player, 0, sizeof( *player ));
}

/*
================
CL_PrecacheModel

prceache model from server
================
*/
void CL_PrecacheModel( sizebuf_t *msg )
{
	int	modelIndex;

	modelIndex = BF_ReadUBitLong( msg, MAX_MODEL_BITS );

	if( modelIndex < 0 || modelIndex >= MAX_MODELS )
		Host_Error( "CL_PrecacheModel: bad modelindex %i\n", modelIndex );

	Q_strncpy( cl.model_precache[modelIndex], BF_ReadString( msg ), sizeof( cl.model_precache[0] ));

	// when we loading map all resources is precached sequentially
	if( !cl.video_prepped ) return;

	Mod_RegisterModel( cl.model_precache[modelIndex], modelIndex );
}

/*
================
CL_PrecacheSound

prceache sound from server
================
*/
void CL_PrecacheSound( sizebuf_t *msg )
{
	int	soundIndex;

	soundIndex = BF_ReadUBitLong( msg, MAX_SOUND_BITS );

	if( soundIndex < 0 || soundIndex >= MAX_SOUNDS )
		Host_Error( "CL_PrecacheSound: bad soundindex %i\n", soundIndex );

	Q_strncpy( cl.sound_precache[soundIndex], BF_ReadString( msg ), sizeof( cl.sound_precache[0] ));

	// when we loading map all resources is precached sequentially
	if( !cl.audio_prepped ) return;

	cl.sound_index[soundIndex] = S_RegisterSound( cl.sound_precache[soundIndex] );
}

/*
================
CL_PrecacheEvent

prceache event from server
================
*/
void CL_PrecacheEvent( sizebuf_t *msg )
{
	int	eventIndex;

	eventIndex = BF_ReadUBitLong( msg, MAX_EVENT_BITS );

	if( eventIndex < 0 || eventIndex >= MAX_EVENTS )
		Host_Error( "CL_PrecacheEvent: bad eventindex %i\n", eventIndex );

	Q_strncpy( cl.event_precache[eventIndex], BF_ReadString( msg ), sizeof( cl.event_precache[0] ));

	// can be set now
	CL_SetEventIndex( cl.event_precache[eventIndex], eventIndex );
}

/*
================
CL_UpdateUserPings

collect pings and packet lossage from clients
================
*/
void CL_UpdateUserPings( sizebuf_t *msg )
{
	int		i, slot;
	player_info_t	*player;
	
	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		if( !BF_ReadOneBit( msg )) break; // end of message

		slot = BF_ReadUBitLong( msg, MAX_CLIENT_BITS );

		if( slot >= MAX_CLIENTS )
			Host_Error( "CL_ParseServerMessage: svc_updatepings > MAX_CLIENTS\n" );

		player = &cl.players[slot];
		player->ping = BF_ReadUBitLong( msg, 12 );
		player->packet_loss = BF_ReadUBitLong( msg, 7 );
	}
}

/*
==============
CL_ServerInfo

change serverinfo
==============
*/
void CL_ServerInfo( sizebuf_t *msg )
{
	string	key, value;

	Q_strncpy( key, BF_ReadString( msg ), sizeof( key ));
	Q_strncpy( value, BF_ReadString( msg ), sizeof( value ));
	Info_SetValueForKey( cl.serverinfo, key, value );
}

/*
==============
CL_CheckingResFile

==============
*/
void CL_CheckingResFile( char *pResFileName )
{
	sizebuf_t	buf;
	byte	data[32];

	if( FS_FileExists( pResFileName, false ))
		return;	// already existing

	cls.downloadcount++;

	Msg( "Starting downloads file: %s\n", pResFileName );

	if( cls.state == ca_disconnected ) return;

	BF_Init( &buf, "ClientPacket", data, sizeof( data ));
	BF_WriteByte( &buf, clc_resourcelist );
	BF_WriteString( &buf, pResFileName );

	if( !cls.netchan.remote_address.type )	// download in singleplayer ???
		cls.netchan.remote_address.type = NA_LOOPBACK;

	// make sure message will be delivered
	Netchan_Transmit( &cls.netchan, BF_GetNumBytesWritten( &buf ), BF_GetData( &buf ));

}

/*
==============
CL_CheckingSoundResFile

==============
*/
void CL_CheckingSoundResFile( char *pResFileName )
{
	string	filepath;

	Q_snprintf( filepath, sizeof( filepath ), "sound/%s", pResFileName );
	CL_CheckingResFile( filepath );
}

/*
==============
CL_ParseResourceList

==============
*/
void CL_ParseResourceList( sizebuf_t *msg )
{
	int	i = 0;

	Q_memset( &reslist, 0, sizeof( resourcelist_t ));

	reslist.rescount = BF_ReadWord( msg ) - 1;

	for( i = 0; i < reslist.rescount; i++ )
	{
		reslist.restype[i] = BF_ReadWord( msg );
		Q_strncpy( reslist.resnames[i], BF_ReadString( msg ), CS_SIZE );
	}

	cls.downloadcount = 0;

	for( i = 0; i < reslist.rescount; i++ )
	{
		if( reslist.restype[i] == t_sound )
			CL_CheckingSoundResFile( reslist.resnames[i] );
		else CL_CheckingResFile( reslist.resnames[i] );
	}

	if( !cls.downloadcount )
	{
		BF_WriteByte( &cls.netchan.message, clc_stringcmd );
		BF_WriteString( &cls.netchan.message, "continueloading" );
	}
}

/*
==============
CL_ParseDirector

spectator message (hltv)
==============
*/
void CL_ParseDirector( sizebuf_t *msg )
{
	byte	pbuf[256];
	int	iSize = BF_ReadByte( msg );

	// parse user message into buffer
	BF_ReadBytes( msg, pbuf, iSize );
	clgame.dllFuncs.pfnDirectorMessage( iSize, pbuf );
}

/*
==============
CL_ParseStudioDecal

Studio Decal message. Used by engine in case
we need save\restore decals
==============
*/
void CL_ParseStudioDecal( sizebuf_t *msg )
{
	modelstate_t	state;
	vec3_t		start, pos;
	int		decalIndex, entityIndex;
	int		modelIndex = 0;
	int		flags;

	BF_ReadVec3Coord( msg, pos );
	BF_ReadVec3Coord( msg, start );
	decalIndex = BF_ReadWord( msg );
	entityIndex = BF_ReadWord( msg );
	flags = BF_ReadByte( msg );

	state.sequence = BF_ReadShort( msg );
	state.frame = BF_ReadShort( msg );
	state.blending[0] = BF_ReadByte( msg );
	state.blending[1] = BF_ReadByte( msg );
	state.controller[0] = BF_ReadByte( msg );
	state.controller[1] = BF_ReadByte( msg );
	state.controller[2] = BF_ReadByte( msg );
	state.controller[3] = BF_ReadByte( msg );
	modelIndex = BF_ReadWord( msg );
	state.body = BF_ReadByte( msg );
	state.skin = BF_ReadByte( msg );

	if( clgame.drawFuncs.R_StudioDecalShoot != NULL )
	{
		int decalTexture = CL_DecalIndex( decalIndex );
		cl_entity_t *ent = CL_GetEntityByIndex( entityIndex );

		if( ent && !ent->model && modelIndex != 0 )
			ent->model = Mod_Handle( modelIndex );

		clgame.drawFuncs.R_StudioDecalShoot( decalTexture, ent, start, pos, flags, &state );
	}
}

/*
==============
CL_ParseScreenShake

Set screen shake
==============
*/
void CL_ParseScreenShake( sizebuf_t *msg )
{
	clgame.shake.amplitude = (float)(word)BF_ReadShort( msg ) * (1.0f / (float)(1<<12));
	clgame.shake.duration = (float)(word)BF_ReadShort( msg ) * (1.0f / (float)(1<<12));
	clgame.shake.frequency = (float)(word)BF_ReadShort( msg ) * (1.0f / (float)(1<<8));
	clgame.shake.time = cl.time + max( clgame.shake.duration, 0.01f );
	clgame.shake.next_shake = 0.0f; // apply immediately
}

/*
==============
CL_ParseScreenFade

Set screen fade
==============
*/
void CL_ParseScreenFade( sizebuf_t *msg )
{
	float		duration, holdTime;
	screenfade_t	*sf = &clgame.fade;

	duration = (float)(word)BF_ReadShort( msg ) * (1.0f / (float)(1<<12));
	holdTime = (float)(word)BF_ReadShort( msg ) * (1.0f / (float)(1<<12));
	sf->fadeFlags = BF_ReadShort( msg );

	sf->fader = BF_ReadByte( msg );
	sf->fadeg = BF_ReadByte( msg );
	sf->fadeb = BF_ReadByte( msg );
	sf->fadealpha = BF_ReadByte( msg );
	sf->fadeSpeed = 0.0f;
	sf->fadeEnd = duration;
	sf->fadeReset = holdTime;

	// calc fade speed
	if( duration > 0 )
	{
		if( sf->fadeFlags & FFADE_OUT )
		{
			if( sf->fadeEnd )
			{
				sf->fadeSpeed = -(float)sf->fadealpha / sf->fadeEnd;
			}

			sf->fadeEnd += cl.time;
			sf->fadeReset += sf->fadeEnd;
		}
		else
		{
			if( sf->fadeEnd )
			{
				sf->fadeSpeed = (float)sf->fadealpha / sf->fadeEnd;
			}

			sf->fadeReset += cl.time;
			sf->fadeEnd += sf->fadeReset;
		}
	}
}

/*
==============
CL_ParseCvarValue

Find the client cvar value
and sent it back to the server
==============
*/
void CL_ParseCvarValue( sizebuf_t *msg )
{
	const char *cvarName = BF_ReadString( msg );
	convar_t *cvar = Cvar_FindVar( cvarName );

	// build the answer
	BF_WriteByte( &cls.netchan.message, clc_requestcvarvalue );
	BF_WriteString( &cls.netchan.message, cvar ? cvar->string : "Not Found" );
}

/*
==============
CL_ParseCvarValue2

Find the client cvar value
and sent it back to the server
==============
*/
void CL_ParseCvarValue2( sizebuf_t *msg )
{
	int requestID = BF_ReadLong( msg );
	const char *cvarName = BF_ReadString( msg );
	convar_t *cvar = Cvar_FindVar( cvarName );

	// build the answer
	BF_WriteByte( &cls.netchan.message, clc_requestcvarvalue2 );
	BF_WriteLong( &cls.netchan.message, requestID );
	BF_WriteString( &cls.netchan.message, cvarName );
	BF_WriteString( &cls.netchan.message, cvar ? cvar->string : "Not Found" );
}

/*
==============
CL_DispatchUserMessage

Dispatch user message by engine request
==============
*/
qboolean CL_DispatchUserMessage( const char *pszName, int iSize, void *pbuf )
{
	int	i;

	if( !pszName || !*pszName )
		return false;

	for( i = 0; i < MAX_USER_MESSAGES; i++ )
	{
		// search for user message
		if( !Q_strcmp( clgame.msg[i].name, pszName ))
			break;
	}

	if( i == MAX_USER_MESSAGES )
	{
		MsgDev( D_ERROR, "CL_DispatchUserMessage: bad message %s\n", pszName );
		return false;
	}

	if( clgame.msg[i].func )
	{
		clgame.msg[i].func( pszName, iSize, pbuf );
	}
	else
	{
		MsgDev( D_ERROR, "CL_DispatchUserMessage: %s not hooked\n", pszName );
		clgame.msg[i].func = CL_UserMsgStub; // throw warning only once
	}
	return true;
}

/*
==============
CL_ParseUserMessage

handles all user messages
==============
*/
void CL_ParseUserMessage( sizebuf_t *msg, int svc_num )
{
	int	i, iSize;
	byte	pbuf[256]; // message can't be larger than 255 bytes

	// NOTE: any user message parse on engine, not in client.dll
	if( svc_num < svc_lastmsg || svc_num >= ( MAX_USER_MESSAGES + svc_lastmsg ))
	{
		// out or range
		Host_Error( "CL_ParseUserMessage: illegible server message %d\n", svc_num );
		return;
	}

	for( i = 0; i < MAX_USER_MESSAGES; i++ )
	{
		// search for user message
		if( clgame.msg[i].number == svc_num )
			break;
	}

	if( i == MAX_USER_MESSAGES ) // probably unregistered
		Host_Error( "CL_ParseUserMessage: illegible server message %d\n", svc_num );

	// NOTE: some user messages handled into engine
	if( !Q_strcmp( clgame.msg[i].name, "ScreenShake" ))
	{
		CL_ParseScreenShake( msg );
		return;
	}
	else if( !Q_strcmp( clgame.msg[i].name, "ScreenFade" ))
	{
		CL_ParseScreenFade( msg );
		return;
	}

	iSize = clgame.msg[i].size;

	// message with variable sizes receive an actual size as first byte
	if( iSize == -1 ) iSize = BF_ReadByte( msg );

	// parse user message into buffer
	BF_ReadBytes( msg, pbuf, iSize );

	if( clgame.msg[i].func )
	{
		clgame.msg[i].func( clgame.msg[i].name, iSize, pbuf );

		// HACKHACK: run final credits for Half-Life
		// because hl1 doesn't have call END_SECTION
		if( !Q_stricmp( clgame.msg[i].name, "HudText" ) && !Q_stricmp( GI->gamefolder, "valve" ))
		{
			// it's a end, so we should run credits
			if( !Q_strcmp( (char *)pbuf, "END3" ))
				Host_Credits();
		}
	}
	else
	{
		MsgDev( D_ERROR, "CL_ParseUserMessage: %s not hooked\n", clgame.msg[i].name );
		clgame.msg[i].func = CL_UserMsgStub; // throw warning only once
	}
}

/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/
/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage( sizebuf_t *msg )
{
	char	*s;
	int	i, j, cmd;
	int	param1, param2;
	int	bufStart;

	cls_message_debug.parsing = true;		// begin parsing
	starting_count = BF_GetNumBytesRead( msg );	// updates each frame
	
	// parse the message
	while( 1 )
	{
		if( BF_CheckOverflow( msg ))
		{
			Host_Error( "CL_ParseServerMessage: overflow!\n" );
			return;
		}

		// mark start position
		bufStart = BF_GetNumBytesRead( msg );

		// end of message
		if( BF_GetNumBitsLeft( msg ) < 8 )
			break;		

		cmd = BF_ReadByte( msg );

		// record command for debugging spew on parse problem
		CL_Parse_RecordCommand( cmd, bufStart );

		// other commands
		switch( cmd )
		{
		case svc_bad:
			Host_Error( "svc_bad\n" );
			break;
		case svc_nop:
			// this does nothing
			break;
		case svc_disconnect:
			CL_Drop ();
			Host_AbortCurrentFrame ();
			break;
		case svc_changing:
			if( BF_ReadOneBit( msg ))
			{
				cls.changelevel = true;
				S_StopAllSounds();

				if( cls.demoplayback )
				{
					SCR_BeginLoadingPlaque( cl.background );
					cls.changedemo = true;
				}
			}
			else MsgDev( D_INFO, "Server disconnected, reconnecting\n" );

			CL_ClearState ();
			CL_InitEdicts (); // re-arrange edicts

			if( cls.demoplayback )
			{
				cl.background = (cls.demonum != -1) ? true : false;
				cls.state = ca_connected;
			}
			else cls.state = ca_connecting;
			cls.connect_time = MAX_HEARTBEAT; // CL_CheckForResend() will fire immediately
			break;
		case svc_setview:
			cl.refdef.viewentity = BF_ReadWord( msg );
			break;
		case svc_sound:
			CL_ParseSoundPacket( msg, false );
			break;
		case svc_time:
			// shuffle timestamps
			cl.mtime[1] = cl.mtime[0];
			cl.mtime[0] = BF_ReadFloat( msg );			
			break;
		case svc_print:
			i = BF_ReadByte( msg );
			MsgDev( D_INFO, "^6%s", BF_ReadString( msg ));
			if( i == PRINT_CHAT ) S_StartLocalSound( "common/menu2.wav", VOL_NORM, false );
			break;
		case svc_stufftext:
			s = BF_ReadString( msg );
			Cbuf_AddText( s );
			break;
		case svc_lightstyle:
			CL_ParseLightStyle( msg );
			break;
		case svc_setangle:
			CL_ParseSetAngle( msg );
			break;
		case svc_serverdata:
			Cbuf_Execute(); // make sure any stuffed commands are done
			CL_ParseServerData( msg );
			break;
		case svc_addangle:
			CL_ParseAddAngle( msg );
			break;
		case svc_clientdata:
			CL_ParseClientData( msg );
			break;
		case svc_packetentities:
			CL_ParsePacketEntities( msg, false );
			break;
		case svc_deltapacketentities:
			CL_ParsePacketEntities( msg, true );
			break;
		case svc_updatepings:
			CL_UpdateUserPings( msg );
			break;
		case svc_usermessage:
			CL_RegisterUserMessage( msg );
			break;
		case svc_particle:
			CL_ParseParticles( msg );
			break;
		case svc_restoresound:
			CL_ParseRestoreSoundPacket( msg );
			break;
		case svc_spawnstatic:
			CL_ParseStaticEntity( msg );
			break;
		case svc_ambientsound:
			CL_ParseSoundPacket( msg, true );
			break;
		case svc_crosshairangle:
			CL_ParseCrosshairAngle( msg );
			break;
		case svc_spawnbaseline:
			CL_ParseBaseline( msg );
			break;
		case svc_temp_entity:
			CL_ParseTempEntity( msg );
			break;
		case svc_setpause:
			cl.refdef.paused = ( BF_ReadOneBit( msg ) != 0 );
			break;
		case svc_deltamovevars:
			CL_ParseMovevars( msg );
			break;
		case svc_customization:
			CL_ParseCustomization( msg );
			break;
		case svc_centerprint:
			CL_CenterPrint( BF_ReadString( msg ), 0.25f );
			break;
		case svc_event:
			CL_ParseEvent( msg );
			break;
		case svc_event_reliable:
			CL_ParseReliableEvent( msg );
			break;
		case svc_updateuserinfo:
			CL_UpdateUserinfo( msg );
			break;
		case svc_intermission:
			cl.refdef.intermission = true;
			break;
		case svc_modelindex:
			CL_PrecacheModel( msg );
			break;
		case svc_soundindex:
			CL_PrecacheSound( msg );
			break;
		case svc_soundfade:
			CL_ParseSoundFade( msg );
			break;
		case svc_cdtrack:
			param1 = BF_ReadByte( msg );
			param1 = bound( 1, param1, MAX_CDTRACKS ); // tracknum
			param2 = BF_ReadByte( msg );
			param2 = bound( 1, param2, MAX_CDTRACKS ); // loopnum
			S_StartBackgroundTrack( clgame.cdtracks[param1-1], clgame.cdtracks[param2-1], 0 );
			break;
		case svc_serverinfo:
			CL_ServerInfo( msg );
			break;
		case svc_eventindex:
			CL_PrecacheEvent( msg );
			break;
		case svc_deltatable:
			Delta_ParseTableField( msg );
			break;
		case svc_weaponanim:
			param1 = BF_ReadByte( msg );	// iAnim
			param2 = BF_ReadByte( msg );	// body
			CL_WeaponAnim( param1, param2 );
			break;
		case svc_bspdecal:
			CL_ParseStaticDecal( msg );
			break;
		case svc_roomtype:
			param1 = BF_ReadShort( msg );
			Cvar_SetFloat( "room_type", param1 );
			break;
		case svc_chokecount:
			i = BF_ReadByte( msg );
			j = cls.netchan.incoming_acknowledged - 1;
			for( ; i > 0 && j > cls.netchan.outgoing_sequence - CL_UPDATE_BACKUP; j-- )
			{
				if( cl.frames[j & CL_UPDATE_MASK].receivedtime != -3.0 )
				{
					cl.frames[j & CL_UPDATE_MASK].receivedtime = -2.0;
					i--;
				}
			}
			break;
		case svc_resourcelist:
			CL_ParseResourceList( msg );
			break;
		case svc_director:
			CL_ParseDirector( msg );
			break;
		case svc_studiodecal:
			CL_ParseStudioDecal( msg );
			break;
		case svc_querycvarvalue:
			CL_ParseCvarValue( msg );
			break;
		case svc_querycvarvalue2:
			CL_ParseCvarValue2( msg );
			break;
		default:
			CL_ParseUserMessage( msg, cmd );
			break;
		}
	}

	cls_message_debug.parsing = false;	// done

	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	if( !cls.demoplayback )
	{
		if( cls.demorecording && !cls.demowaiting )
		{
			CL_WriteDemoMessage( false, starting_count, msg );
		}
		else if( cls.state != ca_active )
		{
			CL_WriteDemoMessage( true, starting_count, msg );
		}
	}
}