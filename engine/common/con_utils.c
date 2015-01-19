/*
con_utils.c - console helpers
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
#include "bspfile.h"
#include "../cl_dll/kbutton.h"

extern convar_t	*con_gamemaps;

#ifdef _DEBUG
void DBG_AssertFunction( qboolean fExpr, const char* szExpr, const char* szFile, int szLine, const char* szMessage )
{
	if( fExpr ) return;

	if( szMessage != NULL )
		MsgDev( at_error, "ASSERT FAILED:\n %s \n(%s@%d)\n%s", szExpr, szFile, szLine, szMessage );
	else MsgDev( at_error, "ASSERT FAILED:\n %s \n(%s@%d)\n", szExpr, szFile, szLine );
}
#endif	// DEBUG

/*
=======================================================================

			FILENAME AUTOCOMPLETION
=======================================================================
*/
/*
=====================================
Cmd_GetMapList

Prints or complete map filename
=====================================
*/
qboolean Cmd_GetMapList( const char *s, char *completedname, int length )
{
	search_t		*t;
	file_t		*f;
	string		message;
	string		matchbuf;
	byte		buf[MAX_SYSPATH]; // 1 kb
	int		i, nummaps;

	t = FS_Search( va( "maps/%s*.bsp", s ), true, con_gamemaps->integer );
	if( !t ) return false;

	FS_FileBase( t->filenames[0], matchbuf ); 
	Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for( i = 0, nummaps = 0; i < t->numfilenames; i++ )
	{
		char		entfilename[CS_SIZE];
		int		ver = -1, mapver = -1, lumpofs = 0, lumplen = 0;
		const char	*ext = FS_FileExtension( t->filenames[i] ); 
		char		*ents = NULL, *pfile;
		qboolean		gearbox = false;
			
		if( Q_stricmp( ext, "bsp" )) continue;
		Q_strncpy( message, "^1error^7", sizeof( message ));
		f = FS_Open( t->filenames[i], "rb", con_gamemaps->integer );
	
		if( f )
		{
			dheader_t	*header, tmphdr;

			Q_memset( &tmphdr, 0, sizeof( tmphdr ));
			FS_Read( f, &tmphdr, sizeof( tmphdr ));
			ver = tmphdr.version;
			header = &tmphdr;
                              
			switch( ver )
			{
			case Q1BSP_VERSION:
			case HLBSP_VERSION:
			case XTBSP_VERSION:
				if( header->lumps[LUMP_ENTITIES].fileofs <= 1024 && !(header->lumps[LUMP_ENTITIES].filelen % sizeof(dplane_t)))
				{
					lumpofs = header->lumps[LUMP_PLANES].fileofs;
					lumplen = header->lumps[LUMP_PLANES].filelen;
					gearbox = true;
				}
				else
				{
					lumpofs = header->lumps[LUMP_ENTITIES].fileofs;
					lumplen = header->lumps[LUMP_ENTITIES].filelen;
					gearbox = false;
				}
				break;
			}

			Q_strncpy( entfilename, t->filenames[i], sizeof( entfilename ));
			FS_StripExtension( entfilename );
			FS_DefaultExtension( entfilename, ".ent" );
			ents = FS_LoadFile( entfilename, NULL, true );

			if( !ents && lumplen >= 10 )
			{
				FS_Seek( f, lumpofs, SEEK_SET );
				ents = (char *)Mem_Alloc( host.mempool, lumplen + 1 );
				FS_Read( f, ents, lumplen );
			}

			if( ents )
			{
				// if there are entities to parse, a missing message key just
				// means there is no title, so clear the message string now
				char	token[2048];

				message[0] = 0;
				pfile = ents;

				while(( pfile = COM_ParseFile( pfile, token )) != NULL )
				{
					if( !Q_strcmp( token, "{" )) continue;
					else if(!Q_strcmp( token, "}" )) break;
					else if(!Q_strcmp( token, "message" ))
					{
						// get the message contents
						pfile = COM_ParseFile( pfile, message );
					}
					else if(!Q_strcmp( token, "mapversion" ))
					{
						// get the message contents
						pfile = COM_ParseFile( pfile, token );
						mapver = Q_atoi( token );
					}
				}
				Mem_Free( ents );
			}
		}

		if( f ) FS_Close(f);
		FS_FileBase( t->filenames[i], matchbuf );

		switch( ver )
		{
		case Q1BSP_VERSION:
			if( mapver == 220 ) Q_strncpy( buf, "Half-Life Alpha", sizeof( buf ));
			else Q_strncpy( buf, "Quake", sizeof( buf ));
			break;
		case HLBSP_VERSION:
			if( gearbox ) Q_strncpy( buf, "Blue-Shift", sizeof( buf ));
			else Q_strncpy( buf, "Half-Life", sizeof( buf ));
			break;
		case XTBSP_VERSION:
			Q_strncpy( buf, "Xash3D", sizeof( buf ));
			break;
		default:	Q_strncpy( buf, "??", sizeof( buf )); break;
		}

		Msg( "%16s (%s) ^3%s^7\n", matchbuf, buf, message );
		nummaps++;
	}

	Msg( "\n^3 %i maps found.\n", nummaps );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	for( i = 0; matchbuf[i]; i++ )
	{
		if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
			completedname[i] = 0;
	}
	return true;
}

/*
=====================================
Cmd_GetDemoList

Prints or complete demo filename
=====================================
*/
qboolean Cmd_GetDemoList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numdems;

	t = FS_Search( va( "demos/%s*.dem", s ), true, true );	// lookup only in gamedir
	if( !t ) return false;

	FS_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length ) Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for( i = 0, numdems = 0; i < t->numfilenames; i++ )
	{
		const char *ext = FS_FileExtension( t->filenames[i] ); 

		if( Q_stricmp( ext, "dem" )) continue;
		FS_FileBase( t->filenames[i], matchbuf );
		Msg( "%16s\n", matchbuf );
		numdems++;
	}

	Msg( "\n^3 %i demos found.\n", numdems );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}
	return true;
}

/*
=====================================
Cmd_GetMovieList

Prints or complete movie filename
=====================================
*/
qboolean Cmd_GetMovieList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, nummovies;

	t = FS_Search( va( "media/%s*.avi", s ), true, false );
	if( !t ) return false;

	FS_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length ) Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for(i = 0, nummovies = 0; i < t->numfilenames; i++)
	{
		const char *ext = FS_FileExtension( t->filenames[i] ); 

		if( Q_stricmp( ext, "avi" )) continue;
		FS_FileBase( t->filenames[i], matchbuf );
		Msg( "%16s\n", matchbuf );
		nummovies++;
	}

	Msg( "\n^3 %i movies found.\n", nummovies );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}

	return true;
}

/*
=====================================
Cmd_GetMusicList

Prints or complete background track filename
=====================================
*/
qboolean Cmd_GetMusicList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numtracks;

	t = FS_Search( va( "media/%s*.*", s ), true, false );
	if( !t ) return false;

	FS_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length ) Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for(i = 0, numtracks = 0; i < t->numfilenames; i++)
	{
		const char *ext = FS_FileExtension( t->filenames[i] ); 

		if( !Q_stricmp( ext, "wav" ) || !Q_stricmp( ext, "mp3" ));
		else continue;

		FS_FileBase( t->filenames[i], matchbuf );
		Msg( "%16s\n", matchbuf );
		numtracks++;
	}

	Msg( "\n^3 %i soundtracks found.\n", numtracks );
	Mem_Free(t);

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}
	return true;
}

/*
=====================================
Cmd_GetSavesList

Prints or complete savegame filename
=====================================
*/
qboolean Cmd_GetSavesList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numsaves;

	t = FS_Search( va( "save/%s*.sav", s ), true, true );	// lookup only in gamedir
	if( !t ) return false;

	FS_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length ) Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for( i = 0, numsaves = 0; i < t->numfilenames; i++ )
	{
		const char *ext = FS_FileExtension( t->filenames[i] ); 

		if( Q_stricmp( ext, "sav" )) continue;
		FS_FileBase( t->filenames[i], matchbuf );
		Msg( "%16s\n", matchbuf );
		numsaves++;
	}

	Msg( "\n^3 %i saves found.\n", numsaves );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}

	return true;
}

/*
=====================================
Cmd_GetConfigList

Prints or complete .cfg filename
=====================================
*/
qboolean Cmd_GetConfigList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numconfigs;

	t = FS_Search( va( "%s*.cfg", s ), true, false );
	if( !t ) return false;

	FS_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length ) Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for( i = 0, numconfigs = 0; i < t->numfilenames; i++ )
	{
		const char *ext = FS_FileExtension( t->filenames[i] );

		if( Q_stricmp( ext, "cfg" )) continue;
		FS_FileBase( t->filenames[i], matchbuf );
		Msg( "%16s\n", matchbuf );
		numconfigs++;
	}

	Msg( "\n^3 %i configs found.\n", numconfigs );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}

	return true;
}

/*
=====================================
Cmd_GetSoundList

Prints or complete sound filename
=====================================
*/
qboolean Cmd_GetSoundList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numsounds;
	const char	*snddir = "sound/"; // constant

	t = FS_Search( va( "%s%s*.*", snddir, s ), true, false );
	if( !t ) return false;

	Q_strncpy( matchbuf, t->filenames[0] + Q_strlen( snddir ), MAX_STRING ); 
	FS_StripExtension( matchbuf ); 
	if( completedname && length ) Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for(i = 0, numsounds = 0; i < t->numfilenames; i++)
	{
		const char *ext = FS_FileExtension( t->filenames[i] ); 

		if( !Q_stricmp( ext, "wav" ) || !Q_stricmp( ext, "mp3" ));
		else continue;

		Q_strncpy( matchbuf, t->filenames[i] + Q_strlen(snddir), MAX_STRING ); 
		FS_StripExtension( matchbuf );
		Msg( "%16s\n", matchbuf );
		numsounds++;
	}

	Msg( "\n^3 %i sounds found.\n", numsounds );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}

	return true;
}

/*
=====================================
Cmd_GetItemsList

Prints or complete item classname (weapons only)
=====================================
*/
qboolean Cmd_GetItemsList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numitems;

	if( !clgame.itemspath[0] ) return false; // not in game yet
	t = FS_Search( va( "%s/%s*.txt", clgame.itemspath, s ), true, false );
	if( !t ) return false;

	FS_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length ) Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for(i = 0, numitems = 0; i < t->numfilenames; i++)
	{
		const char *ext = FS_FileExtension( t->filenames[i] ); 

		if( Q_stricmp( ext, "txt" )) continue;
		FS_FileBase( t->filenames[i], matchbuf );
		Msg( "%16s\n", matchbuf );
		numitems++;
	}

	Msg( "\n^3 %i items found.\n", numitems );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}
	return true;
}

/*
=====================================
Cmd_GetCustomList

Prints or complete .HPK filenames
=====================================
*/
qboolean Cmd_GetCustomList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numitems;

	t = FS_Search( va( "%s*.hpk", s ), true, false );
	if( !t ) return false;

	FS_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length ) Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for(i = 0, numitems = 0; i < t->numfilenames; i++)
	{
		const char *ext = FS_FileExtension( t->filenames[i] ); 

		if( Q_stricmp( ext, "hpk" )) continue;
		FS_FileBase( t->filenames[i], matchbuf );
		Msg( "%16s\n", matchbuf );
		numitems++;
	}

	Msg( "\n^3 %i items found.\n", numitems );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}
	return true;
}

/*
=====================================
Cmd_GetTexturemodes

Prints or complete sound filename
=====================================
*/
qboolean Cmd_GetTexturemodes( const char *s, char *completedname, int length )
{
	int	i, numtexturemodes;
	string	texturemodes[6];	// keep an actual ( sizeof( gl_texturemode) / sizeof( gl_texturemode[0] ))
	string	matchbuf;

	const char *gl_texturemode[] =
	{
	"GL_LINEAR",
	"GL_LINEAR_MIPMAP_LINEAR",
	"GL_LINEAR_MIPMAP_NEAREST",
	"GL_NEAREST",
	"GL_NEAREST_MIPMAP_LINEAR",
	"GL_NEAREST_MIPMAP_NEAREST",
	};

	// compare gamelist with current keyword
	for( i = 0, numtexturemodes = 0; i < 6; i++ )
	{
		if(( *s == '*' ) || !Q_strnicmp( gl_texturemode[i], s, Q_strlen( s )))
			Q_strcpy( texturemodes[numtexturemodes++], gl_texturemode[i] ); 
	}

	if( !numtexturemodes ) return false;
	Q_strncpy( matchbuf, gl_texturemode[0], MAX_STRING ); 
	if( completedname && length ) Q_strncpy( completedname, matchbuf, length );
	if( numtexturemodes == 1 ) return true;

	for( i = 0; i < numtexturemodes; i++ )
	{
		Q_strncpy( matchbuf, texturemodes[i], MAX_STRING ); 
		Msg( "%16s\n", matchbuf );
	}

	Msg( "\n^3 %i filters found.\n", numtexturemodes );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}
	return true;
}

/*
=====================================
Cmd_GetGameList

Prints or complete gamedir name
=====================================
*/
qboolean Cmd_GetGamesList( const char *s, char *completedname, int length )
{
	int	i, numgamedirs;
	string	gamedirs[MAX_MODS];
	string	matchbuf;

	// stand-alone games doesn't have cmd "game"
	if( !Cmd_Exists( "game" ))
		return false;

	// compare gamelist with current keyword
	for( i = 0, numgamedirs = 0; i < SI.numgames; i++ )
	{
		if(( *s == '*' ) || !Q_strnicmp( SI.games[i]->gamefolder, s, Q_strlen( s )))
			Q_strcpy( gamedirs[numgamedirs++], SI.games[i]->gamefolder ); 
	}

	if( !numgamedirs ) return false;
	Q_strncpy( matchbuf, gamedirs[0], MAX_STRING ); 
	if( completedname && length ) Q_strncpy( completedname, matchbuf, length );
	if( numgamedirs == 1 ) return true;

	for( i = 0; i < numgamedirs; i++ )
	{
		Q_strncpy( matchbuf, gamedirs[i], MAX_STRING ); 
		Msg( "%16s\n", matchbuf );
	}

	Msg( "\n^3 %i games found.\n", numgamedirs );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}
	return true;
}

qboolean Cmd_CheckMapsList_R( qboolean fRefresh, qboolean onlyingamedir )
{
	byte	buf[MAX_SYSPATH];
	char	*buffer;
	string	result;
	int	i, size;
	search_t	*t;
	file_t	*f;

	if( FS_FileSize( "maps.lst", onlyingamedir ) > 0 && !fRefresh )
	{
		MsgDev( D_NOTE, "maps.lst is exist: %s\n", onlyingamedir ? "basedir" : "gamedir" );
		return true; // exist 
	}

	t = FS_Search( "maps/*.bsp", false, onlyingamedir );

	if( !t )
	{
		if( onlyingamedir )
		{
			// mod doesn't contain any maps (probably this is a bot)
			return Cmd_CheckMapsList_R( fRefresh, false );
		}

		return false;
	}

	buffer = Mem_Alloc( host.mempool, t->numfilenames * 2 * sizeof( result ));

	for( i = 0; i < t->numfilenames; i++ )
	{
		char		*ents = NULL, *pfile;
		int		ver = -1, lumpofs = 0, lumplen = 0;
		string		mapname, message, entfilename;
		const char	*ext = FS_FileExtension( t->filenames[i] ); 

		if( Q_stricmp( ext, "bsp" )) continue;
		f = FS_Open( t->filenames[i], "rb", onlyingamedir );
		FS_FileBase( t->filenames[i], mapname );

		if( f )
		{
			int	num_spawnpoints = 0;
			dheader_t	*header;

			Q_memset( buf, 0, MAX_SYSPATH );
			FS_Read( f, buf, MAX_SYSPATH );
			ver = *(uint *)buf;
                              
			switch( ver )
			{
			case Q1BSP_VERSION:
			case HLBSP_VERSION:
			case XTBSP_VERSION:
				header = (dheader_t *)buf;
				if( header->lumps[LUMP_ENTITIES].fileofs <= 1024 )
				{
					lumpofs = header->lumps[LUMP_PLANES].fileofs;
					lumplen = header->lumps[LUMP_PLANES].filelen;
				}
				else
				{
					lumpofs = header->lumps[LUMP_ENTITIES].fileofs;
					lumplen = header->lumps[LUMP_ENTITIES].filelen;
				}
				break;
			}

			Q_strncpy( entfilename, t->filenames[i], sizeof( entfilename ));
			FS_StripExtension( entfilename );
			FS_DefaultExtension( entfilename, ".ent" );
			ents = FS_LoadFile( entfilename, NULL, true );

			if( !ents && lumplen >= 10 )
			{
				FS_Seek( f, lumpofs, SEEK_SET );
				ents = (char *)Mem_Alloc( host.mempool, lumplen + 1 );
				FS_Read( f, ents, lumplen );
			}

			if( ents )
			{
				// if there are entities to parse, a missing message key just
				// means there is no title, so clear the message string now
				char	token[2048];
				qboolean	worldspawn = true;

				Q_strncpy( message, "No Title", MAX_STRING );
				pfile = ents;

				while(( pfile = COM_ParseFile( pfile, token )) != NULL )
				{
					if( token[0] == '}' && worldspawn )
						worldspawn = false;
					else if( !Q_strcmp( token, "message" ) && worldspawn )
					{
						// get the message contents
						pfile = COM_ParseFile( pfile, message );
					}
					else if( !Q_strcmp( token, "classname" ))
					{
						pfile = COM_ParseFile( pfile, token );
						if( !Q_strcmp( token, GI->mp_entity ))
							num_spawnpoints++;
					}
					if( num_spawnpoints ) break; // valid map
				}
				Mem_Free( ents );
			}

			if( f ) FS_Close( f );

			if( num_spawnpoints )
			{
				// format: mapname "maptitle"\n
				Q_sprintf( result, "%s \"%s\"\n", mapname, message );
				Q_strcat( buffer, result ); // add new string
			}
		}
	}

	if( t ) Mem_Free( t ); // free search result
	size = Q_strlen( buffer );

	if( !size )
	{
          	if( buffer ) Mem_Free( buffer );

		if( onlyingamedir )
			return Cmd_CheckMapsList_R( fRefresh, false );
		return false;
	}

	// write generated maps.lst
	if( FS_WriteFile( "maps.lst", buffer, Q_strlen( buffer )))
	{
          	if( buffer ) Mem_Free( buffer );
		return true;
	}
	return false;
}

qboolean Cmd_CheckMapsList( qboolean fRefresh )
{
	return Cmd_CheckMapsList_R( fRefresh, true );
}

autocomplete_list_t cmd_list[] =
{
{ "gl_texturemode", Cmd_GetTexturemodes },
{ "map_background", Cmd_GetMapList },
{ "changelevel", Cmd_GetMapList },
{ "playdemo", Cmd_GetDemoList, },
{ "playvol", Cmd_GetSoundList },
{ "hpkval", Cmd_GetCustomList },
{ "entpatch", Cmd_GetMapList },
{ "music", Cmd_GetMusicList, },
{ "movie", Cmd_GetMovieList },
{ "exec", Cmd_GetConfigList },
{ "give", Cmd_GetItemsList },
{ "drop", Cmd_GetItemsList },
{ "game", Cmd_GetGamesList },
{ "save", Cmd_GetSavesList },
{ "load", Cmd_GetSavesList },
{ "play", Cmd_GetSoundList },
{ "map", Cmd_GetMapList },
{ NULL }, // termiantor
};

/*
============
Cmd_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
static void Cmd_WriteCvar(const char *name, const char *string, const char *desc, void *f )
{
	if( !desc || !*desc ) return; // ignore cvars without description (fantom variables)
	FS_Printf( f, "%s \"%s\"\n", name, string );
}

static void Cmd_WriteServerCvar(const char *name, const char *string, const char *desc, void *f )
{
	if( !desc || !*desc ) return; // ignore cvars without description (fantom variables)
	FS_Printf( f, "set %s \"%s\"\n", name, string );
}

static void Cmd_WriteOpenGLCvar( const char *name, const char *string, const char *desc, void *f )
{
	if( !desc || !*desc ) return; // ignore cvars without description (fantom variables)
	FS_Printf( f, "setgl %s \"%s\"\n", name, string );
}

static void Cmd_WriteRenderCvar( const char *name, const char *string, const char *desc, void *f )
{
	if( !desc || !*desc ) return; // ignore cvars without description (fantom variables)
	FS_Printf( f, "setr %s \"%s\"\n", name, string );
}

static void Cmd_WriteHelp(const char *name, const char *unused, const char *desc, void *f )
{
	if( !desc ) return;				// ignore fantom cmds
	if( !Q_strcmp( desc, "" )) return;		// blank description
	if( name[0] == '+' || name[0] == '-' ) return;	// key bindings	
	FS_Printf( f, "%s\t\t\t\"%s\"\n", name, desc );
}

void Cmd_WriteVariables( file_t *f )
{
	Cvar_LookupVars( CVAR_ARCHIVE, NULL, f, Cmd_WriteCvar ); 
}

void Cmd_WriteServerVariables( file_t *f )
{
	Cvar_LookupVars( CVAR_SERVERNOTIFY, NULL, f, Cmd_WriteServerCvar ); 
}

void Cmd_WriteOpenGLVariables( file_t *f )
{
	Cvar_LookupVars( CVAR_GLCONFIG, NULL, f, Cmd_WriteOpenGLCvar ); 
}

void Cmd_WriteRenderVariables( file_t *f )
{
	Cvar_LookupVars( CVAR_RENDERINFO, NULL, f, Cmd_WriteRenderCvar ); 
}

/*
===============
Host_WriteConfig

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfig( void )
{
	kbutton_t	*mlook, *jlook;
	file_t	*f;

	if( !clgame.hInstance ) return;

	MsgDev( D_NOTE, "Host_WriteConfig()\n" );
	f = FS_Open( "config.cfg", "w", false );
	if( f )
	{
		FS_Printf( f, "//=======================================================================\n");
		FS_Printf( f, "//\t\t\tCopyright XashXT Group %s ©\n", Q_timestamp( TIME_YEAR_ONLY ));
		FS_Printf( f, "//\t\t\tconfig.cfg - archive of cvars\n" );
		FS_Printf( f, "//=======================================================================\n" );
		Key_WriteBindings( f );
		Cmd_WriteVariables( f );

		mlook = (kbutton_t *)clgame.dllFuncs.KB_Find( "in_mlook" );
		jlook = (kbutton_t *)clgame.dllFuncs.KB_Find( "in_jlook" );

		if( mlook && ( mlook->state & 1 )) 
			FS_Printf( f, "+mlook\n" );

		if( jlook && ( jlook->state & 1 ))
			FS_Printf( f, "+jlook\n" );

		FS_Printf( f, "exec userconfig.cfg" );

		FS_Close( f );
	}
	else MsgDev( D_ERROR, "Couldn't write config.cfg.\n" );
}

/*
===============
Host_WriteServerConfig

save serverinfo variables into server.cfg (using for dedicated server too)
===============
*/
void Host_WriteServerConfig( const char *name )
{
	file_t	*f;

	SV_InitGameProgs();	// collect user variables
	
	if(( f = FS_Open( name, "w", false )) != NULL )
	{
		FS_Printf( f, "//=======================================================================\n" );
		FS_Printf( f, "//\t\t\tCopyright XashXT Group %s ©\n", Q_timestamp( TIME_YEAR_ONLY ));
		FS_Printf( f, "//\t\t\tserver.cfg - server temporare config\n" );
		FS_Printf( f, "//=======================================================================\n" );
		Cmd_WriteServerVariables( f );
		FS_Close( f );
	}
	else MsgDev( D_ERROR, "Couldn't write %s.\n", name );

	SV_FreeGameProgs();	// release progs with all variables
}

/*
===============
Host_WriteOpenGLConfig

save opengl variables into opengl.cfg
===============
*/
void Host_WriteOpenGLConfig( void )
{
	file_t	*f;

	MsgDev( D_NOTE, "Host_WriteGLConfig()\n" );
	f = FS_Open( "opengl.cfg", "w", false );
	if( f )
	{
		FS_Printf( f, "//=======================================================================\n" );
		FS_Printf( f, "//\t\t\tCopyright XashXT Group %s ©\n", Q_timestamp( TIME_YEAR_ONLY ));
		FS_Printf( f, "//\t\t    opengl.cfg - archive of opengl extension cvars\n");
		FS_Printf( f, "//=======================================================================\n" );
		Cmd_WriteOpenGLVariables( f );
		FS_Close( f );	
	}                                                
	else MsgDev( D_ERROR, "can't update opengl.cfg.\n" );
}

/*
===============
Host_WriteVideoConfig

save render variables into video.cfg
===============
*/
void Host_WriteVideoConfig( void )
{
	file_t	*f;

	MsgDev( D_NOTE, "Host_WriteVideoConfig()\n" );
	f = FS_Open( "video.cfg", "w", false );
	if( f )
	{
		FS_Printf( f, "//=======================================================================\n" );
		FS_Printf( f, "//\t\t\tCopyright XashXT Group %s ©\n", Q_timestamp( TIME_YEAR_ONLY ));
		FS_Printf( f, "//\t\tvideo.cfg - archive of renderer variables\n");
		FS_Printf( f, "//=======================================================================\n" );
		Cmd_WriteRenderVariables( f );
		FS_Close( f );	
	}                                                
	else MsgDev( D_ERROR, "can't update video.cfg.\n" );
}

void Key_EnumCmds_f( void )
{
	file_t	*f;

	FS_AllowDirectPaths( true );
	if( FS_FileExists( "../help.txt", false ))
	{
		Msg( "help.txt already exist\n" );
		FS_AllowDirectPaths( false );
		return;
	}

	f = FS_Open( "../help.txt", "w", false );
	if( f )
	{
		FS_Printf( f, "//=======================================================================\n");
		FS_Printf( f, "//\t\t\tCopyright XashXT Group %s ©\n", Q_timestamp( TIME_YEAR_ONLY ));
		FS_Printf( f, "//\t\thelp.txt - xash commands and console variables\n");
		FS_Printf( f, "//=======================================================================\n");

		FS_Printf( f, "\n\n\t\t\tconsole variables\n\n");
		Cvar_LookupVars( 0, NULL, f, Cmd_WriteHelp ); 
		FS_Printf( f, "\n\n\t\t\tconsole commands\n\n");
		Cmd_LookupCmds( NULL, f, Cmd_WriteHelp ); 
  		FS_Printf( f, "\n\n");
		FS_Close( f );
		Msg( "help.txt created\n" );
	}
	else MsgDev( D_ERROR, "Couldn't write help.txt.\n");
	FS_AllowDirectPaths( false );
}