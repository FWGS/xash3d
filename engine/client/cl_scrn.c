/*
cl_scrn.c - refresh screen
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
#include "vgui_draw.h"
#include "qfont.h"
#include "library.h"

convar_t *scr_centertime;
convar_t *scr_loading;
convar_t *scr_download;
convar_t *scr_width;
convar_t *scr_height;
convar_t *scr_viewsize;
convar_t *cl_testlights;
convar_t *cl_allow_levelshots;
convar_t *cl_levelshot_name;
convar_t *cl_envshot_size;
convar_t *scr_dark;
convar_t *net_graph;
convar_t *net_graphpos;
convar_t *net_graphwidth;
convar_t *net_graphheight;
convar_t *net_scale;
convar_t *net_graphfillsegments;

typedef struct
{
	int	x1, y1, x2, y2;
} dirty_t;

static dirty_t	scr_dirty, scr_old_dirty[2];
static qboolean	scr_init = false;

#define TIMINGS 1024
#define TIMINGS_MASK (TIMINGS - 1)
#define LATENCY_AVG_FRAC 0.5
#define PACKETLOSS_AVG_FRAC 0.5
#define PACKETCHOKE_AVG_FRAC 0.5
#define LERP_HEIGHT 24
#define NUM_LATENCY_SAMPLES 8
static struct packet_latency_t
{
	int latency, choked;
} packet_latency[ TIMINGS ];
static struct cmdinfo_t
{
	float	cmd_lerp;
	int		size;
	qboolean sent;
} cmdinfo[ TIMINGS ];
static netbandwidthgraph_t	graph[ TIMINGS ];
static float				packet_loss;
static float				packet_choke;
static byte					netcolors[ LERP_HEIGHT + 5 ][4] =
{
	{ 255, 0,   0,   255 },
	{ 0,   0,   255, 255 },
	{ 240, 127, 63,  255 },
	{ 255, 255, 0,   255 },
	{ 63,  255, 63,  150 }
	// other will be generated through NetGraph_InitColors()
};


/*
==============
SCR_DrawFPS
==============
*/
void SCR_DrawFPS( void )
{
	float		calc;
	rgba_t		color;
	static double	nexttime = 0, lasttime = 0;
	static double	framerate = 0, avgrate = 0;
	static int	framecount = 0;
	static int	minfps = 9999;
	static int	maxfps = 0;
	double		newtime;
	char		fpsstring[64];
	int		offset;

	if( cls.state != ca_active ) return; 
	if( !cl_showfps->integer || cl.background ) return;

	switch( cls.scrshot_action )
	{
	case scrshot_normal:
	case scrshot_snapshot:
	case scrshot_inactive:
		break;
	default: return;
	}

	newtime = Sys_DoubleTime();
	if( newtime >= nexttime )
	{
		framerate = framecount / (newtime - lasttime);
		lasttime = newtime;
		nexttime = max( nexttime + 1, lasttime - 1 );
		framecount = 0;
	}

	framecount++;
	calc = framerate;

	if( calc == 0 ) return;

	if( calc < 1.0f )
	{
		Q_snprintf( fpsstring, sizeof( fpsstring ), "%4i spf", (int)(1.0f / calc + 0.5f));
		MakeRGBA( color, 255, 0, 0, 255 );
	}
	else
	{
		int	curfps = (int)(calc + 0.5f);

		if( curfps < minfps ) minfps = curfps;
		if( curfps > maxfps ) maxfps = curfps;

		/*if( !avgrate ) avgrate = ( maxfps - minfps ) / 2.0f;
		else */avgrate += ( calc - avgrate ) / host.framecount;

		switch( cl_showfps->integer )
		{
		case 3:
			Q_snprintf( fpsstring, sizeof( fpsstring ), "fps: ^1%4i min, ^3%4i cur, ^2%4i max | ^3%.2f avg", minfps, curfps, maxfps, avgrate );
			break;
		case 2:
			Q_snprintf( fpsstring, sizeof( fpsstring ), "fps: ^1%4i min, ^3%4i cur, ^2%4i max", minfps, curfps, maxfps );
			break;
		case 1:
		default:
			Q_snprintf( fpsstring, sizeof( fpsstring ), "%4i fps", curfps );
		}

		MakeRGBA( color, 255, 255, 255, 255 );
	}

	Con_DrawStringLen( fpsstring, &offset, NULL );
	Con_DrawString( scr_width->integer - offset - 2, 4, fpsstring, color );
}

/*
==============
SCR_DrawPos

Draw local player position, angles and velocity
==============
*/
void SCR_DrawPos( void )
{
	static char	msg[MAX_SYSPATH];
	float speed;
	cl_entity_t *pPlayer;
	rgba_t color;

	if( cls.state != ca_active ) return;
	if( !cl_showpos->integer || cl.background ) return;

	pPlayer = CL_GetLocalPlayer();
	speed = VectorLength( cl.frame.client.velocity );

	Q_snprintf( msg, MAX_SYSPATH,
	"pos: %.2f %.2f %.2f\n"
	"ang: %.2f %.2f %.2f\n"
	"velocity: %.2f", pPlayer->origin[0], pPlayer->origin[1], pPlayer->origin[2],
					pPlayer->angles[0], pPlayer->angles[1], pPlayer->angles[2],
					speed );

	MakeRGBA( color, 255, 255, 255, 255 );

	Con_DrawString( scr_width->integer / 2, 4, msg, color );

}

/*
==============
SCR_NetSpeeds

same as r_speeds but for network channel
==============
*/
void SCR_NetSpeeds( void )
{
	static char	msg[MAX_SYSPATH];
	int		x, y, height;
	char		*p, *start, *end;
	float		time = cl.mtime[0];
	rgba_t		color;

	if( !net_speeds->integer ) return;
	if( cls.state != ca_active ) return; 

	switch( net_speeds->integer )
	{
	case 1:
		if( cls.netchan.compress )
		{
			Q_snprintf( msg, sizeof( msg ), "Game Time: %02d:%02d\nTotal received from server:\n Huffman %s\nUncompressed %s\nSplit %s\n",
			(int)(time / 60.0f ), (int)fmod( time, 60.0f ), Q_memprint( cls.netchan.total_received ), Q_memprint( cls.netchan.total_received_uncompressed ),
						Q_memprint( cls.netchan.netsplit.total_received ) );
		}
		else
		{
			Q_snprintf( msg, sizeof( msg ), "Game Time: %02d:%02d\nTotal received from server:\nUncompressed %s\nSplit %s/%s(%f)\n",
			(int)(time / 60.0f ), (int)fmod( time, 60.0f ), Q_memprint( cls.netchan.total_received_uncompressed ),
						Q_memprint( cls.netchan.netsplit.total_received ), Q_memprint( cls.netchan.netsplit.total_received_uncompressed ),
						((float)cls.netchan.netsplit.total_received) / cls.netchan.netsplit.total_received_uncompressed );
		}
		break;
	case 2:
		if( cls.netchan.compress )
		{
			Q_snprintf( msg, sizeof( msg ), "Game Time: %02d:%02d\nTotal sended to server:\nHuffman %s\nUncompressed %s\n",
			(int)(time / 60.0f ), (int)fmod( time, 60.0f ), Q_memprint( cls.netchan.total_sended ), Q_memprint( cls.netchan.total_sended_uncompressed ));
		}
		else
		{
			Q_snprintf( msg, sizeof( msg ), "Game Time: %02d:%02d\nTotal sended to server:\nUncompressed %s\n",
			(int)(time / 60.0f ), (int)fmod( time, 60.0f ), Q_memprint( cls.netchan.total_sended_uncompressed ));
		}
		break;
	default: return;
	}

	x = scr_width->integer - 320;
	y = 256;

	Con_DrawStringLen( NULL, NULL, &height );
	MakeRGBA( color, 255, 255, 255, 255 );

	p = start = msg;

	do
	{
		end = Q_strchr( p, '\n' );
		if( end ) msg[end-start] = '\0';

		Con_DrawString( x, y, p, color );
		y += height;

		if( end ) p = end + 1;
		else break;
	} while( 1 );
}

/*
================
SCR_RSpeeds
================
*/
void SCR_RSpeeds( void )
{
	char	msg[MAX_SYSPATH];

	if( R_SpeedsMessage( msg, sizeof( msg )))
	{
		int	x, y, height;
		char	*p, *start, *end;
		rgba_t	color;

		x = scr_width->integer - 340;
		y = 64;

		Con_DrawStringLen( NULL, NULL, &height );
		MakeRGBA( color, 255, 255, 255, 255 );

		p = start = msg;
		do
		{
			end = Q_strchr( p, '\n' );
			if( end ) msg[end-start] = '\0';

			Con_DrawString( x, y, p, color );
			y += height;

			if( end ) p = end + 1;
			else break;
		} while( 1 );
	}
}


/*
==========
NetGraph_DrawLine

CL_FillRGBA shortcut
==========
*/
_inline void NetGraph_DrawRect( wrect_t *rect, byte colors[4] )
{
	CL_FillRGBA( rect->left, rect->top, rect->right, rect->bottom, colors[0], colors[1], colors[2], colors[3] );
}

/*
==========
NetGraph_InitColors

init netgraph colors
==========
*/
void NetGraph_InitColors( void )
{
	int i;

	for( i = 0; i < LERP_HEIGHT; i++ )
	{
		if( i <= LERP_HEIGHT / 3 )
		{
			netcolors[i + 5][0] = i * -7.875 + 63;
			netcolors[i + 5][1] = i *  7.875;
			netcolors[i + 5][2] = i * 19.375 + 100;
		}
		else
		{
			netcolors[i + 5][0] = ( i - 8 ) * -0.3125 + 255;
			netcolors[i + 5][1] = ( i - 8 ) * -7.9375 + 127;
			netcolors[i + 5][2] = 0;
		}
	}
}

/*
==========
NetGraph_GetFrameData

get frame data info, like chokes, packet losses, also update graph, packet and cmdinfo
==========
*/
void NetGraph_GetFrameData( int *biggest_message, float *latency, int *latency_count )
{
	int i, choke_count = 0, loss_count = 0;
	float loss, choke;

	*biggest_message = *latency_count = 0;
	*latency = 0.0f;

	for( i = cls.netchan.incoming_sequence - CL_UPDATE_BACKUP + 1; i <= cls.netchan.incoming_sequence; i++ )
	{
		frame_t *f = cl.frames + ( i & CL_UPDATE_MASK );
		struct packet_latency_t *p = packet_latency + ( i & TIMINGS_MASK );
		netbandwidthgraph_t *g = graph + ( i & TIMINGS_MASK );

		p->choked = f->receivedtime == -2.0f ? true : false;
		if( p->choked )
			choke_count++;

		if( !f->valid || p->choked )
		{
			p->latency = 9998; // broken delta
		}
		else if( f->receivedtime == -1 )
		{
			p->latency = 9999; // dropped
			loss_count++;
		}
		else
		{
			int frame_latency = min( 1.0f, f->latency );
			p->latency = (( frame_latency + 0.1 ) / 1.1) * (net_graphheight->integer - LERP_HEIGHT - 2);

			if( i > cls.netchan.incoming_sequence - NUM_LATENCY_SAMPLES )
			{
				*latency += 1000.0f * f->latency;
				latency_count++;
			}
		}

		Q_memcpy( g, &f->graphdata, sizeof( netbandwidthgraph_t ));

		if( *biggest_message < g->msgbytes )
			*biggest_message = g->msgbytes;
	}

	for( i = cls.netchan.outgoing_sequence - CL_UPDATE_BACKUP + 1; i <= cls.netchan.outgoing_sequence; i++ )
	{
		cmdinfo[ i & TIMINGS_MASK ].cmd_lerp = cl.commands[ i & CL_UPDATE_MASK ].frame_lerp;
		cmdinfo[ i & TIMINGS_MASK ].sent = !cl.commands[ i & CL_UPDATE_MASK ].heldback;
		cmdinfo[ i & TIMINGS_MASK ].size = cl.commands[ i & CL_UPDATE_MASK ].sendsize;
	}

	// Packet loss
	loss = 100.0 * (float)loss_count / CL_UPDATE_BACKUP;
	packet_loss = PACKETLOSS_AVG_FRAC * packet_loss + ( 1.0 - PACKETLOSS_AVG_FRAC ) * loss;

	// Packet choke
	choke = 100.0 * (float)choke_count / CL_UPDATE_BACKUP;
	packet_choke = PACKETCHOKE_AVG_FRAC * packet_choke + ( 1.0 - PACKETCHOKE_AVG_FRAC ) * choke;
}

/*
===========
NetGraph_DrawTimes

===========
*/
void NetGraph_DrawTimes( wrect_t rect, int x, int w )
{
	int i, j, extrap_point = LERP_HEIGHT / 3, a, h;
	POINT pt = { max( x + w - 1 - 25, 1 ),
				 max( rect.top + rect.bottom - 4 - LERP_HEIGHT + 1, 1 ) };
	wrect_t fill;
	rgba_t colors = { 0.9 * 255, 0.9 * 255, 0.7 * 255, 255 };

	Con_DrawString( pt.x, pt.y, va("%i/s", cl_cmdrate->integer), colors );

	for( a = 0; a < w; a++ )
	{
		i = ( cls.netchan.outgoing_sequence - a ) & TIMINGS_MASK;
		h = ( cmdinfo[i].cmd_lerp / 3.0f ) * LERP_HEIGHT;

		fill.left = x + w - a - 1;
		fill.right = fill.bottom = 1;
		fill.top = rect.top + rect.bottom - 4;

		if( h >= extrap_point )
		{
			int start = 0;
			h -= extrap_point;
			fill.top = extrap_point;

			for( j = start; j < h; j++ )
			{
				Q_memcpy( colors, netcolors[j + extrap_point], sizeof( byte ) * 3 );
				NetGraph_DrawRect( &fill, colors );
				fill.top--;
			}
		}
		else
		{
			int oldh = h;
			fill.top -= h;
			h = extrap_point - h;

			for( j = 0; j < h; j++ )
			{
				Q_memcpy( colors, netcolors[j + oldh], sizeof( byte ) * 3 );
				NetGraph_DrawRect( &fill, colors );
				fill.top--;
			}
		}

		fill.top = rect.top + rect.bottom - 4 - extrap_point;

		CL_FillRGBA( fill.left, fill.top, fill.right, fill.bottom, 255, 255, 255, 255 );

		fill.top = rect.top + rect.bottom - 3;

		if( !cmdinfo[i].sent )
			CL_FillRGBA( fill.left, fill.top, fill.right, fill.bottom, 255, 0, 0, 255 );
	}
}

/*
===========
NetGraph_DrawHatches

===========
*/
void NetGraph_DrawHatches( int x, int y, int maxmsgbytes )
{
	int starty;
	int ystep = max( (int)( 10.0 / net_scale->value ), 1 );

	wrect_t hatch = { x, 4, y, 1 };


	for( starty = hatch.top; hatch.top > 0 && ((starty - hatch.top) * net_scale->value < (maxmsgbytes + 50)); hatch.top -= ystep )
	{
		CL_FillRGBA( hatch.left, hatch.top, hatch.right, hatch.bottom, 63, 63, 0, 200 );
	}
}

/*
===========
NetGraph_DrawTextFields

===========
*/
void NetGraph_DrawTextFields( int x, int y, int count, float avg, int packet_loss, int packet_choke )
{
	static int lastout;

	rgba_t colors = { 0.9 * 255, 0.9 * 255, 0.7 * 255, 255 };

	float latency = count > 0 ? max( 0,  avg / count - 0.5 * host.frametime - 1000.0 / cl_updaterate->value ) : 0;

	float framerate = 1 / host.realframetime;
	int i = ( cls.netchan.outgoing_sequence - 1 ) & TIMINGS_MASK;

	Con_DrawString( x, y - net_graphheight->integer, va( "%.1ffps" , framerate ), colors );
	Con_DrawString( x + 75, y - net_graphheight->integer, va( "%i ms" , (int)latency ), colors );
	Con_DrawString( x + 150, y - net_graphheight->integer, va( "%i/s" , cl_updaterate->integer ), colors );

	if( cmdinfo[i].size )
		lastout =  cmdinfo[i].size;

	Con_DrawString( x, y - net_graphheight->integer + 15,
		va( "in :  %i %.2f k/s",
			graph[i].msgbytes,
			cls.netchan.flow[FLOW_INCOMING].avgkbytespersec ),
		colors );
	Con_DrawString( x, y - net_graphheight->integer + 30,
		va( "out:  %i %.2f k/s",
			lastout,
			cls.netchan.flow[FLOW_OUTGOING].avgkbytespersec ),
		colors );
	if( net_graph->integer > 2 )
	{
		Con_DrawString( x, y - net_graphheight->integer + 45,
			va( "loss: %i choke: %i",
				packet_loss,
				packet_choke ),
			colors );
	}

}
/*
===========
NetGraph_DrawDataSegment

===========
*/
int NetGraph_DrawDataSegment( wrect_t *fill, int bytes, byte r, byte g, byte b, byte a )
{
	int h = bytes / net_scale->value;
	byte colors[4] = { r, g, b, a };

	fill->top -= h;

	if( net_graphfillsegments->integer )
		fill->bottom = h;
	else
		fill->bottom = 1;

	if( fill->top > 1 )
	{
		NetGraph_DrawRect( fill, colors );
		return 1;
	}
	return 0;
}

void NetGraph_ColorForHeight( struct packet_latency_t *packet, byte color[4], int *ping )
{
	switch( packet->latency )
	{
	case 9999:
		Q_memcpy( color, netcolors[0], sizeof( byte ) * 4 ); // dropped
		*ping = 0;
		break;
	case 9998:
		Q_memcpy( color, netcolors[1], sizeof( byte ) * 4 ); // invalid
		*ping = 0;
		break;
	case 9997:
		Q_memcpy( color, netcolors[2], sizeof( byte ) * 4 ); // skipped
		*ping = 0;
		break;
	default:
		*ping = 1;
		if( packet->choked )
		{
			Q_memcpy( color, netcolors[3], sizeof( byte ) * 4 );
		}
		else
		{
			Q_memcpy( color, netcolors[4], sizeof( byte ) * 4 );
		}
	}
}

/*
===========
NetGraph_DrawDataUsage

===========
*/
void NetGraph_DrawDataUsage( int x, int y, int w, int maxmsgbytes )
{
	int a, i, h, lastvalidh = 0, ping;
	wrect_t fill = { 0 };
	byte color[4];

	for( a = 0; a < w; a++ )
	{
		i = (cls.netchan.incoming_sequence - a) & TIMINGS_MASK;
		h = packet_latency[i].latency;

		NetGraph_ColorForHeight( &packet_latency[i], color, &ping );

		if( !ping )
			h = lastvalidh;
		else
			lastvalidh = h;

		if( h > net_graphheight->value - LERP_HEIGHT - 2 )
			h = net_graphheight->value - LERP_HEIGHT - 2;

		fill.left = x + w - a - 1;
		fill.top = y - h;
		fill.right = 1;
		fill.bottom = ping ? 1: h;

		NetGraph_DrawRect( &fill, color );

		fill.top = y;
		fill.bottom = 1;

		color[0] = 0; color[1] = 255; color[2] = 0; color[3] = 160;

		NetGraph_DrawRect( &fill, color );

		if( net_graph->integer < 2 )
			continue;

		fill.top = y - net_graphheight->value - 1;
		fill.bottom = 1;
		color[0] = color[1] = color[2] = color[3] = 255;

		NetGraph_DrawRect( &fill, color );

		fill.top -= 1;

		if( packet_latency[i].latency > 9995 )
			continue; // skip invalid

		if( !NetGraph_DrawDataSegment( &fill, graph[i].client, 255, 0, 0, 128 ) )
			continue;

		if( !NetGraph_DrawDataSegment( &fill, graph[i].players, 255, 255, 0, 128 ) )
			continue;

		if( !NetGraph_DrawDataSegment( &fill, graph[i].entities, 255, 0, 255, 128 ) )
			continue;

		if( !NetGraph_DrawDataSegment( &fill, graph[i].tentities, 0, 0, 255, 128 ) )
			continue;

		if( !NetGraph_DrawDataSegment( &fill, graph[i].sound, 0, 255, 0, 128 ) )
			continue;

		if( !NetGraph_DrawDataSegment( &fill, graph[i].event, 0, 255, 255, 128 ) )
			continue;

		if( !NetGraph_DrawDataSegment( &fill, graph[i].usr, 200, 200, 200, 128 ) )
			continue;

		// special case for absolute usage

		h = graph[i].msgbytes / net_scale->value;

		color[0] = color[1] = color[2] = 240; color[3] = 255;

		fill.bottom = 1;

		fill.top = y - net_graphheight->value - 1 - h;

		if( fill.top < 2 )
			continue;

		NetGraph_DrawRect( &fill, color );
	}

	if( net_graph->integer >= 2 )
		NetGraph_DrawHatches( x, y - net_graphheight->value - 1, maxmsgbytes );
}

/*
===========
NetGraph_GetScreenPos

===========
*/
void NetGraph_GetScreenPos( wrect_t *rect, int *w, int *x, int *y )
{
	rect->left = rect->top = 0;
	rect->right = clgame.scrInfo.iWidth;
	rect->bottom = clgame.scrInfo.iHeight;

	*w = min( TIMINGS, net_graphwidth->integer );
	if( rect->right < *w + 10 )
	{
		*w = rect->right - 10;
	}

	// detect x and y position
	switch( net_graphpos->integer )
	{
	case 1: // right sided
		*x = rect->left + rect->right - 5 - *w;
		break;
	case 2: // center
		*x = rect->left + ( rect->right - 10 - *w ) / 2;
		break;
	default: // left sided
		*x = rect->left + 5;
		break;
	}
	*y = rect->bottom + rect->top - LERP_HEIGHT - 5;
}

/*
===========
SCR_DrawNetGraph

===========
*/
void SCR_DrawNetGraph( void )
{
	wrect_t rect;
	float avg_ping;
	int ping_count, maxmsgbytes;
	int w, x, y;

	if( !net_graph->integer )
		return;

	if( net_scale->value <= 0 )
		Cvar_SetFloat( "net_scale", 0.1f );

	NetGraph_GetScreenPos( &rect, &w, &x, &y );

	// this will update packet_loss and packet_choke
	NetGraph_GetFrameData( &maxmsgbytes, &avg_ping, &ping_count );

	if( net_graph->integer < 3 )
	{
		NetGraph_DrawTimes( rect, x, w );
		NetGraph_DrawDataUsage( x, y, w, maxmsgbytes );
	}

	NetGraph_DrawTextFields( x, y, ping_count, avg_ping, packet_loss, packet_choke );
}

void SCR_MakeLevelShot( void )
{
	if( cls.scrshot_request != scrshot_plaque )
		return;

	// make levelshot at nextframe()
	Cbuf_AddText( "levelshot\n" );
}

void SCR_MakeScreenShot( void )
{
	qboolean	iRet = false;
	int	viewsize;

	if( cls.envshot_viewsize > 0 )
		viewsize = cls.envshot_viewsize;
	else viewsize = cl_envshot_size->integer;

	switch( cls.scrshot_action )
	{
	case scrshot_normal:
		iRet = VID_ScreenShot( cls.shotname, VID_SCREENSHOT );
		break;
	case scrshot_snapshot:
		iRet = VID_ScreenShot( cls.shotname, VID_SNAPSHOT );
		break;
	case scrshot_plaque:
		iRet = VID_ScreenShot( cls.shotname, VID_LEVELSHOT );
		break;
	case scrshot_savegame:
	case scrshot_demoshot:
		iRet = VID_ScreenShot( cls.shotname, VID_MINISHOT );
		break;
	case scrshot_envshot:
		iRet = VID_CubemapShot( cls.shotname, viewsize, cls.envshot_vieworg, false );
		break;
	case scrshot_skyshot:
		iRet = VID_CubemapShot( cls.shotname, viewsize, cls.envshot_vieworg, true );
		break;
	case scrshot_mapshot:
		iRet = VID_ScreenShot( cls.shotname, VID_MAPSHOT );
		break;
	case scrshot_inactive:
		return;
	}

	// report
	if( iRet )
	{
		// snapshots don't writes message about image		
		if( cls.scrshot_action != scrshot_snapshot )
			MsgDev( D_AICONSOLE, "Write %s\n", cls.shotname );
	}
	else MsgDev( D_ERROR, "Unable to write %s\n", cls.shotname );

	cls.envshot_vieworg = NULL;
	cls.scrshot_action = scrshot_inactive;
	cls.envshot_disable_vis = false;
	cls.envshot_viewsize = 0;
	cls.shotname[0] = '\0';
}

void SCR_DrawPlaque( void )
{
	int	levelshot;

	if(( cl_allow_levelshots->integer && !cls.changelevel ) || cl.background )
	{
		levelshot = GL_LoadTexture( cl_levelshot_name->string, NULL, 0, TF_IMAGE, NULL );
		GL_SetRenderMode( kRenderNormal );
		R_DrawStretchPic( 0, 0, scr_width->value, scr_height->value, 0, 0, 1, 1, levelshot );
		if( !cl.background ) CL_DrawHUD( CL_LOADING );
	}
}

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque( qboolean is_background )
{
	S_StopAllSounds();
	cl.audio_prepped = false;			// don't play ambients

	if( cls.disable_screen ) return;		// already set
	if( cls.state == ca_disconnected ) return;	// if at console, don't bring up the plaque
	if( cls.key_dest == key_console ) return;

	cls.draw_changelevel = is_background ? false : true;
	SCR_UpdateScreen();
	cls.disable_screen = host.realtime;
	cls.disable_servercount = cl.servercount;
	cl.background = is_background;		// set right state before svc_serverdata is came
}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque( void )
{
	cls.disable_screen = 0.0f;
	Con_ClearNotify();
}

/*
=================
SCR_AddDirtyPoint
=================
*/
void SCR_AddDirtyPoint( int x, int y )
{
	if( x < scr_dirty.x1 ) scr_dirty.x1 = x;
	if( x > scr_dirty.x2 ) scr_dirty.x2 = x;
	if( y < scr_dirty.y1 ) scr_dirty.y1 = y;
	if( y > scr_dirty.y2 ) scr_dirty.y2 = y;
}

/*
================
SCR_DirtyScreen
================
*/
void SCR_DirtyScreen( void )
{
	SCR_AddDirtyPoint( 0, 0 );
	SCR_AddDirtyPoint( scr_width->integer - 1, scr_height->integer - 1 );
}

/*
================
SCR_TileClear
================
*/
void SCR_TileClear( void )
{
	int	i, top, bottom, left, right;
	dirty_t	clear;

	if( scr_viewsize->integer >= 120 )
		return; // full screen rendering

	// erase rect will be the union of the past three frames
	// so tripple buffering works properly
	clear = scr_dirty;

	for( i = 0; i < 2; i++ )
	{
		if( scr_old_dirty[i].x1 < clear.x1 )
			clear.x1 = scr_old_dirty[i].x1;
		if( scr_old_dirty[i].x2 > clear.x2 )
			clear.x2 = scr_old_dirty[i].x2;
		if( scr_old_dirty[i].y1 < clear.y1 )
			clear.y1 = scr_old_dirty[i].y1;
		if( scr_old_dirty[i].y2 > clear.y2 )
			clear.y2 = scr_old_dirty[i].y2;
	}

	scr_old_dirty[1] = scr_old_dirty[0];
	scr_old_dirty[0] = scr_dirty;

	scr_dirty.x1 = 9999;
	scr_dirty.x2 = -9999;
	scr_dirty.y1 = 9999;
	scr_dirty.y2 = -9999;

	if( clear.y2 <= clear.y1 )
		return; // nothing disturbed

	top = cl.refdef.viewport[1];
	bottom = top + cl.refdef.viewport[3] - 1;
	left = cl.refdef.viewport[0];
	right = left + cl.refdef.viewport[2] - 1;

	if( clear.y1 < top )
	{	
		// clear above view screen
		i = clear.y2 < top-1 ? clear.y2 : top - 1;
		R_DrawTileClear( clear.x1, clear.y1, clear.x2 - clear.x1 + 1, i - clear.y1 + 1 );
		clear.y1 = top;
	}

	if( clear.y2 > bottom )
	{	
		// clear below view screen
		i = clear.y1 > bottom + 1 ? clear.y1 : bottom + 1;
		R_DrawTileClear( clear.x1, i, clear.x2 - clear.x1 + 1, clear.y2 - i + 1 );
		clear.y2 = bottom;
	}

	if( clear.x1 < left )
	{
		// clear left of view screen
		i = clear.x2 < left - 1 ? clear.x2 : left - 1;
		R_DrawTileClear( clear.x1, clear.y1, i - clear.x1 + 1, clear.y2 - clear.y1 + 1 );
		clear.x1 = left;
	}

	if( clear.x2 > right )
	{	
		// clear left of view screen
		i = clear.x1 > right + 1 ? clear.x1 : right + 1;
		R_DrawTileClear( i, clear.y1, clear.x2 - i + 1, clear.y2 - clear.y1 + 1 );
		clear.x2 = right;
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen( void )
{
	if( !V_PreRender( )) return;

	switch( cls.state )
	{
	case ca_disconnected:
		break;
	case ca_connecting:
	case ca_connected:
		SCR_DrawPlaque();
		break;
	case ca_active:
		V_RenderView();
		break;
	case ca_cinematic:
		SCR_DrawCinematic();
		break;
	default:
		Host_Error( "SCR_UpdateScreen: bad cls.state\n" );
		break;
	}

	V_PostRender();
}

void SCR_LoadCreditsFont( void )
{
	int	fontWidth;
	dword crc = 0;
	const char *path = "gfx/creditsfont.fnt";
	byte	*buffer;
	fs_offset_t	length;
	qfont_t	*src;

	if( cls.creditsFont.valid ) return; // already loaded

	// replace default gfx.wad textures by current charset's font
	if( !CRC32_File( &crc, "gfx.wad" ) || crc == 0x49eb9f16 )
	{
		const char *path2 = va("creditsfont_%s.fnt", Cvar_VariableString( "con_charset" ) );
		if( FS_FileExists( path2, false ) )
			path = path2;
	}

	cls.creditsFont.hFontTexture = GL_LoadTexture( path, NULL, 0, TF_IMAGE, NULL );
	R_GetTextureParms( &fontWidth, NULL, cls.creditsFont.hFontTexture );

	if( fontWidth == 0 ) return;
	
	// setup creditsfont
	// half-life font with variable chars width
	buffer = FS_LoadFile( path, &length, false );

	if( buffer && length >= ( fs_offset_t )sizeof( qfont_t ))
	{
		int	i;

		src = (qfont_t *)buffer;
		cls.creditsFont.charHeight = clgame.scrInfo.iCharHeight = src->rowheight;

		// build rectangles
		for( i = 0; i < 256; i++ )
		{
			cls.creditsFont.fontRc[i].left = (word)src->fontinfo[i].startoffset % fontWidth;
			cls.creditsFont.fontRc[i].right = cls.creditsFont.fontRc[i].left + src->fontinfo[i].charwidth;
			cls.creditsFont.fontRc[i].top = (word)src->fontinfo[i].startoffset / fontWidth;
			cls.creditsFont.fontRc[i].bottom = cls.creditsFont.fontRc[i].top + src->rowheight;
			cls.creditsFont.charWidths[i] = clgame.scrInfo.charWidths[i] = src->fontinfo[i].charwidth;
		}
		cls.creditsFont.valid = true;
	}
	if( buffer ) Mem_Free( buffer );
}

void SCR_InstallParticlePalette( void )
{
	rgbdata_t	*pic;
	int	i;

	// first check 'palette.lmp' then 'palette.pal'
	pic = FS_LoadImage( "gfx/palette.lmp", NULL, 0 );
	if( !pic ) pic = FS_LoadImage( "gfx/palette.pal", NULL, 0 );

	// NOTE: imagelib required this fakebuffer for loading internal palette
	if( !pic ) pic = FS_LoadImage( "#valve.pal", ((byte *)&i), 768 );

	if( pic )
	{
		for( i = 0; i < 256; i++ )
		{
			clgame.palette[i][0] = pic->palette[i*4+0];
			clgame.palette[i][1] = pic->palette[i*4+1];
			clgame.palette[i][2] = pic->palette[i*4+2];
		}
		FS_FreeImage( pic );
	}
	else
	{
		for( i = 0; i < 256; i++ )
		{
			clgame.palette[i][0] = i;
			clgame.palette[i][1] = i;
			clgame.palette[i][2] = i;
		}
		MsgDev( D_WARN, "CL_InstallParticlePalette: failed. Force to grayscale\n" );
	}
}

void SCR_RegisterTextures( void )
{
	cls.fillImage = GL_LoadTexture( "*white", NULL, 0, TF_IMAGE, NULL ); // used for FillRGBA
	cls.particleImage = GL_LoadTexture( "*particle", NULL, 0, TF_IMAGE, NULL );

	// register gfx.wad images
	cls.oldParticleImage = GL_LoadTexture("*oldparticle", NULL, 0, TF_IMAGE, NULL);
	cls.pauseIcon = GL_LoadTexture( "gfx.wad/paused.lmp", NULL, 0, TF_IMAGE, NULL );
	if( cl_allow_levelshots->integer )
		cls.loadingBar = GL_LoadTexture( "gfx.wad/lambda.lmp", NULL, 0, TF_IMAGE|TF_LUMINANCE, NULL );
	else cls.loadingBar = GL_LoadTexture( "gfx.wad/lambda.lmp", NULL, 0, TF_IMAGE, NULL ); 
	cls.tileImage = GL_LoadTexture( "gfx.wad/backtile.lmp", NULL, 0, TF_UNCOMPRESSED|TF_NOPICMIP|TF_NOMIPMAP, NULL );
	cls.hChromeSprite = pfnSPR_Load( "sprites/shellchrome.spr" );
}

/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f( void )
{
	Cvar_SetFloat( "viewsize", min( scr_viewsize->value + 10, 120 ));
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f( void )
{
	Cvar_SetFloat( "viewsize", max( scr_viewsize->value - 10, 30 ));
}

/*
==================
SCR_VidInit
==================
*/
void SCR_VidInit( void )
{
	Q_memset( &clgame.ds, 0, sizeof( clgame.ds )); // reset a draw state
	Q_memset( &menu.ds, 0, sizeof( menu.ds )); // reset a draw state
	Q_memset( &clgame.centerPrint, 0, sizeof( clgame.centerPrint ));

	if( menu.globals )
	{
		// update screen sizes for menu
		menu.globals->scrWidth = scr_width->integer;
		menu.globals->scrHeight = scr_height->integer;
	}

	SCR_RebuildGammaTable();
#ifdef XASH_VGUI
	VGui_Startup (scr_width->integer, scr_height->integer);
#endif

	clgame.load_sequence++; // now all hud sprites are invalid
	
	// vid_state has changed
	if( menu.hInstance ) menu.dllFuncs.pfnVidInit();
	if( clgame.hInstance ) clgame.dllFuncs.pfnVidInit();

	// restart console size
	Con_VidInit ();
}

/*
==================
SCR_Init
==================
*/
void SCR_Init( void )
{
	if( scr_init ) return;

	MsgDev( D_NOTE, "SCR_Init()\n" );
	scr_centertime = Cvar_Get( "scr_centertime", "2.5", 0, "centerprint hold time" );
	cl_levelshot_name = Cvar_Get( "cl_levelshot_name", "*black", 0, "contains path to current levelshot" );
	cl_allow_levelshots = Cvar_Get( "allow_levelshots", "0", CVAR_ARCHIVE, "allow engine to use indivdual levelshots instead of 'loading' image" );
	scr_loading = Cvar_Get( "scr_loading", "0", 0, "loading bar progress" );
	scr_download = Cvar_Get( "scr_download", "0", 0, "downloading bar progress" );
	cl_testlights = Cvar_Get( "cl_testlights", "0", 0, "test dynamic lights" );
	cl_envshot_size = Cvar_Get( "cl_envshot_size", "256", CVAR_ARCHIVE, "envshot size of cube side" );
	scr_dark = Cvar_Get( "v_dark", "0", 0, "starts level from dark screen" );
	scr_viewsize = Cvar_Get( "viewsize", "120", CVAR_ARCHIVE, "screen size" );
	net_graph = Cvar_Get( "net_graph", "0", CVAR_ARCHIVE, "draw network usage graph" );
	net_graphpos = Cvar_Get( "net_graphpos", "1", CVAR_ARCHIVE, "network usage graph position" );
	net_scale = Cvar_Get( "net_scale", "5", CVAR_ARCHIVE, "network usage graph scale level" );
	net_graphwidth = Cvar_Get( "net_graphwidth", "192", CVAR_ARCHIVE, "network usage graph width" );
	net_graphheight = Cvar_Get( "net_graphheight", "64", CVAR_ARCHIVE, "network usage graph height" );
	net_graphfillsegments = Cvar_Get( "net_graphfillsegments", "1", CVAR_ARCHIVE, "fill segments in network usage graph" );
	// register our commands
	Cmd_AddCommand( "timerefresh", SCR_TimeRefresh_f, "turn quickly and print rendering statistcs" );
	Cmd_AddCommand( "skyname", CL_SetSky_f, "set new skybox by basename" );
	Cmd_AddCommand( "viewpos", SCR_Viewpos_f, "prints current player origin" );
	Cmd_AddCommand( "sizeup", SCR_SizeUp_f, "screen size up to 10 points" );
	Cmd_AddCommand( "sizedown", SCR_SizeDown_f, "screen size down to 10 points" );

	Com_ResetLibraryError();

	if( host.state != HOST_RESTART && !UI_LoadProgs( ))
	{
		Sys_Warn( "can't initialize menu library:\n%s", Com_GetLibraryError() ); // this is not fatal for us
		// console still can't be toggled in-game without extra cmd-line switch
		if( !host.developer ) host.developer = 1; // we need console, because menu is missing
	}

	SCR_LoadCreditsFont ();
	SCR_InstallParticlePalette ();
	SCR_RegisterTextures ();
	SCR_InitCinematic();
	NetGraph_InitColors();
	SCR_VidInit();

	if( host.state != HOST_RESTART )
          {
		if( host.developer && Sys_CheckParm( "-toconsole" ))
			Cbuf_AddText( "toggleconsole\n" );
		else UI_SetActiveMenu( true );
	}

	scr_init = true;
}

void SCR_Shutdown( void )
{
	if( !scr_init ) return;

	MsgDev( D_NOTE, "SCR_Shutdown()\n" );
	Cmd_RemoveCommand( "timerefresh" );
	Cmd_RemoveCommand( "skyname" );
	Cmd_RemoveCommand( "viewpos" );
	Cmd_RemoveCommand( "sizeup" );
	Cmd_RemoveCommand( "sizedown" );
	UI_SetActiveMenu( false );

	if( host.state != HOST_RESTART )
		UI_UnloadProgs();
	cls.creditsFont.valid = false;
	scr_init = false;
}
#endif // XASH_DEDICATED
