/*
hpak.c - custom user package to send other clients
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

#include "common.h"
#include "filesystem.h"

convar_t		*hpk_maxsize;
hpak_t		*hpak_queue = NULL;
hpak_header_t	hash_pack_header;
hpak_container_t	hash_pack_dir;

const char *HPAK_TypeFromIndex( int type )
{
	switch( type )
	{
	case t_sound: return "decal";
	case t_skin: return "skin";
	case t_model: return "model";
	case t_decal: return "decal";
	case t_generic: return "generic";
	case t_eventscript: return "event";
	case t_world: return "map";	
	}
	return "generic";
}

void HPAK_FileCopy( file_t *pOutput, file_t *pInput, int fileSize )
{
	char	buf[MAX_SYSPATH];	// a small buffer for the copy
	int	size;

	while( fileSize > 0 )
	{
		if( fileSize > MAX_SYSPATH )
			size = MAX_SYSPATH;
		else size = fileSize;

		FS_Read( pInput, buf, size );
		FS_Write( pOutput, buf, size );
		
		fileSize -= size;
	}
}

void HPAK_AddToQueue( const char *name, resource_t *DirEnt, byte *data, file_t *f )
{
	hpak_t	*ptr;

	ptr = Z_Malloc( sizeof( hpak_t ));
	ptr->name = copystring( name );
	ptr->HpakResource = *DirEnt;
	ptr->size = DirEnt->nDownloadSize;
	ptr->data = Z_Malloc( ptr->size );

	if( data ) Q_memcpy( ptr->data, data, ptr->size );
	else if( f ) FS_Read( f, ptr->data, ptr->size );
	else Host_Error( "HPAK_AddToQueue: data == NULL.\n" );

	ptr->next = hpak_queue;
	hpak_queue = ptr;
}

void HPAK_CreatePak( const char *filename, resource_t *DirEnt, byte *data, file_t *f )
{
	int		filelocation;
	string		pakname;
	char		md5[16];
	char		*temp;
	MD5Context_t	MD5_Hash;
	file_t		*fout;

	if( !filename || !filename[0] )
	{
		MsgDev( D_ERROR, "HPAK_CreatePak: NULL name\n" );
		return;
	}

	if(( f != NULL && data != NULL ) || ( f == NULL && data == NULL ))
	{
		MsgDev( D_ERROR, "HPAK_CreatePak: too many sources, please leave one.\n" );
		return;
	}

	Q_strncpy( pakname, filename, sizeof( pakname ));
	FS_StripExtension( pakname );
	FS_DefaultExtension( pakname, ".hpk" );

	MsgDev( D_INFO, "creating HPAK %s.\n", pakname );
	fout = FS_Open( pakname, "wb", false );
	if( !fout )
	{
		MsgDev( D_ERROR, "HPAK_CreatePak: can't write %s.\n", pakname );
		return;
	}

	// let's hash it.
	Q_memset( &MD5_Hash, 0, sizeof( MD5Context_t ));
	MD5Init( &MD5_Hash );

	if( data == NULL )
	{
		// there are better ways
		filelocation = FS_Tell( f );
		temp = Z_Malloc( DirEnt->nDownloadSize );
		FS_Read( f, temp, DirEnt->nDownloadSize );
		FS_Seek( f, filelocation, SEEK_SET );

		MD5Update( &MD5_Hash, (byte *)temp, DirEnt->nDownloadSize );
		Mem_Free( temp );
	}
	else
	{
		MD5Update( &MD5_Hash, data, DirEnt->nDownloadSize );
	}

	MD5Final( (byte *)md5, &MD5_Hash );

	if( Q_memcmp( md5, DirEnt->rgucMD5_hash, 16 ))
	{
		MsgDev( D_ERROR, "HPAK_CreatePak: bad checksum for %s. Ignored\n", pakname );
		return;
	}

	hash_pack_header.ident = IDCUSTOMHEADER;
	hash_pack_header.version = IDCUSTOM_VERSION;
	hash_pack_header.seek = 0;

	FS_Write( fout, &hash_pack_header, sizeof( hash_pack_header ));

	hash_pack_dir.count = 1;
	hash_pack_dir.dirs = Z_Malloc( sizeof( hpak_dir_t ));
	hash_pack_dir.dirs[0].DirectoryResource = *DirEnt;
	hash_pack_dir.dirs[0].seek = FS_Tell( fout );
	hash_pack_dir.dirs[0].size = DirEnt->nDownloadSize;

	if( data == NULL )
	{
		HPAK_FileCopy( fout, f, hash_pack_dir.dirs[0].size );
	}
	else
	{
		FS_Write( fout, data, hash_pack_dir.dirs[0].size );
	}

	filelocation = FS_Tell( fout );
	FS_Write( fout, &hash_pack_dir.count, sizeof( hash_pack_dir.count ));
	FS_Write( fout, &hash_pack_dir.dirs[0], sizeof( hpak_dir_t ));

	Mem_Free( hash_pack_dir.dirs );
	Q_memset( &hash_pack_dir, 0, sizeof( hpak_container_t ));

	hash_pack_header.seek = filelocation;
	FS_Seek( fout, 0, SEEK_SET );
	FS_Write( fout, &hash_pack_header, sizeof( hpak_header_t ));
	FS_Close( fout );
}

qboolean HPAK_FindResource( hpak_container_t *hpk, char *inHash, resource_t *pRes )
{
	int	i;

	for( i = 0; i < hpk->count; i++ )
	{
		if( !Q_memcmp( hpk->dirs[i].DirectoryResource.rgucMD5_hash, inHash, 16 ))
		{
			if( pRes ) *pRes = hpk->dirs[i].DirectoryResource; // get full copy

			return true;
		}
	}
	return false;
}

void HPAK_AddLump( qboolean add_to_queue, const char *name, resource_t *DirEnt, byte *data, file_t *f )
{
	int		i, position, length;
	string		pakname1, pakname2;
	char		md5[16];
	MD5Context_t	MD5_Hash;
	hpak_container_t	hpak1, hpak2;
	file_t		*f1, *f2;
	hpak_dir_t	*dirs;
	byte		*temp;

	if( !name || !name[0] )
	{
		MsgDev( D_ERROR, "HPAK_AddLump: NULL name\n" );
		return;
	}

	if( !DirEnt )
	{
		MsgDev( D_ERROR, "HPAK_AddLump: invalid lump\n" );
		return;
	}

	if( data == NULL && f == NULL )
	{
		MsgDev( D_ERROR, "HPAK_AddLump: missing lump data\n" );
		return;
	}

	if( DirEnt->nDownloadSize < 1024 || DirEnt->nDownloadSize > 131072 )
	{
		MsgDev( D_ERROR, "HPAK_AddLump: invalid size %s\n", Q_pretifymem( DirEnt->nDownloadSize, 2 ));
		return;
	}

	// hash it
	Q_memset( &MD5_Hash, 0, sizeof( MD5Context_t ));
	MD5Init( &MD5_Hash );

	if( data == NULL )
	{
		// there are better ways
		position = FS_Tell( f );
		temp = Z_Malloc( DirEnt->nDownloadSize );
		FS_Read( f, temp, DirEnt->nDownloadSize );
		FS_Seek( f, position, SEEK_SET );

		MD5Update( &MD5_Hash, temp, DirEnt->nDownloadSize );
		Mem_Free( temp );
	}
	else
	{
		MD5Update( &MD5_Hash, data, DirEnt->nDownloadSize );
	}

	MD5Final( (byte *)md5, &MD5_Hash );

	if( Q_memcmp( md5, DirEnt->rgucMD5_hash, 0x10 ))
	{
		MsgDev( D_ERROR, "HPAK_AddLump: bad checksum for %s. Ignored\n", DirEnt->szFileName );
		return;
	}

	if( add_to_queue )
	{
		HPAK_AddToQueue( name, DirEnt, data, f );
		return;
	}

	Q_strncpy( pakname1, name, sizeof( pakname1 ));
	FS_StripExtension( pakname1 );
	FS_DefaultExtension( pakname1, ".hpk" );

	f1 = FS_Open( pakname1, "rb", false );

	if( !f1 )
	{
		// create new pack
		HPAK_CreatePak( name, DirEnt, data, f );
		return;
	}

	Q_strncpy( pakname2, pakname1, sizeof( pakname2 ));
	FS_StripExtension( pakname2 );
	FS_DefaultExtension( pakname2, ".hp2" );

	f2 = FS_Open( pakname2, "w+b", false );

	if( !f2 )
	{
		MsgDev( D_ERROR, "HPAK_AddLump: couldn't open %s.\n", pakname2 );
		FS_Close( f1 );
		return;
	}

	// load headers
	FS_Read( f1, &hash_pack_header, sizeof( hpak_header_t ));

	if( hash_pack_header.version != IDCUSTOM_VERSION )
	{
		// we don't check the HPAK bit for some reason.
		MsgDev( D_ERROR, "HPAK_AddLump: %s does not have a valid header.\n", pakname2 );
		FS_Close( f1 );
		FS_Close( f2 );
	}

	length = FS_FileLength( f1 );
	HPAK_FileCopy( f2, f1, length );

	FS_Seek( f1, hash_pack_header.seek, SEEK_SET );
	FS_Read( f1, &hpak1.count, sizeof( hpak1.count ));

	if( hpak1.count < 1 || hpak1.count > MAX_FILES_IN_WAD )
	{
		MsgDev( D_ERROR, "HPAK_AddLump: %s contain too many lumps.\n", pakname1 );
		FS_Close( f1 );
		FS_Close( f2 );
		return;
	}

	// load the data
	hpak1.dirs = Z_Malloc( sizeof( hpak_dir_t ) * hpak1.count );
	FS_Read( f1, hpak1.dirs, sizeof( hpak_dir_t ) * hpak1.count );
	FS_Close( f1 );

	if( HPAK_FindResource( &hpak1, (char *)DirEnt->rgucMD5_hash, NULL ))
	{
		Mem_Free( hpak1.dirs );
		FS_Close( f2 );
	}

	// make a new container
	hpak2.count = hpak1.count;
	hpak2.dirs = Z_Malloc( sizeof( hpak_dir_t ) * hpak2.count );
	Q_memcpy( hpak2.dirs, hpak1.dirs, hpak1.count );

	for( i = 0, dirs = NULL; i < hpak1.count; i++ )
	{
		if( Q_memcmp( hpak1.dirs[i].DirectoryResource.rgucMD5_hash, DirEnt->rgucMD5_hash, 16 ) < 0 )
		{
			dirs = &hpak1.dirs[i];
			while( i < hpak1.count )
			{
				hpak2.dirs[i+1] = hpak1.dirs[i];
				i++;
			}
			break;
		}
	}

	if( dirs == NULL ) dirs = &hpak2.dirs[hpak2.count-1];

	Q_memset( dirs, 0, sizeof( hpak_dir_t ));
	FS_Seek( f2, hash_pack_header.seek, SEEK_SET );
	dirs->DirectoryResource = *DirEnt;
	dirs->seek = FS_Tell( f2 );
	dirs->size = DirEnt->nDownloadSize;

	if( !data ) HPAK_FileCopy( f2, f, dirs->size );
	else FS_Write( f2, data, dirs->size );

	hash_pack_header.seek = FS_Tell( f2 );
	FS_Write( f2, &hpak2.count, sizeof( hpak2.count ));

	for( i = 0; i < hpak2.count; i++ )
	{
		FS_Write( f2, &hpak2.dirs[i], sizeof( hpak_dir_t ));
	}

	// finalize
	Mem_Free( hpak1.dirs );
	Mem_Free( hpak2.dirs );

	FS_Seek( f2, 0, SEEK_SET );
	FS_Write( f2, &hash_pack_header, sizeof( hpak_header_t ));
	FS_Close( f2 );

	FS_Delete( pakname1 );
	FS_Rename( pakname2, pakname1 );
}

void HPAK_FlushHostQueue( void )
{
	hpak_t	*ptr;

	for( ptr = hpak_queue; ptr != NULL; ptr = hpak_queue )
	{
		hpak_queue = hpak_queue->next; // it's here so we get that null check in first
		HPAK_AddLump( 0, ptr->name, &ptr->HpakResource, ptr->data, 0 );
		Mem_Free( ptr->name );
		Mem_Free( ptr->data );
		Mem_Free( ptr );
	}
}

static qboolean HPAK_Validate( const char *filename, qboolean quiet )
{
	file_t		*f;
	hpak_dir_t	*dataDir;
	hpak_header_t	hdr;
	byte		*dataPak;
	int		i, num_lumps;
	MD5Context_t	MD5_Hash;
	string		pakname;
	resource_t	*pRes;
	char		md5[16];

	if( quiet ) HPAK_FlushHostQueue();

	// not an error - just flush queue
	if( !filename || !*filename )
		return true;

	Q_strncpy( pakname, filename, sizeof( pakname ));
	FS_StripExtension( pakname );
	FS_DefaultExtension( pakname, ".hpk" );

	f = FS_Open( pakname, "rb", false );
	if( !f )
	{
		MsgDev( D_INFO, "Couldn't find %s.\n", pakname );
		return true;
	}

	if( !quiet ) MsgDev( D_INFO, "Validating %s\n", pakname );

	FS_Read( f, &hdr, sizeof( hdr ));
	if( hdr.ident != IDCUSTOMHEADER || hdr.version != IDCUSTOM_VERSION )
	{
		MsgDev( D_ERROR, "HPAK_ValidatePak: %s does not have a valid HPAK header.\n", pakname );
		FS_Close( f );
		return false;
	}

	FS_Seek( f, hdr.seek, SEEK_SET );
	FS_Read( f, &num_lumps, sizeof( num_lumps ));

	if( num_lumps < 1 || num_lumps > MAX_FILES_IN_WAD )
	{
		MsgDev( D_ERROR, "HPAK_ValidatePak: %s has too many lumps %u.\n", pakname, num_lumps );
		FS_Close( f );
		return false;
	}

	if( !quiet ) MsgDev( D_INFO, "# of Entries:  %i\n", num_lumps );

	dataDir = Z_Malloc( sizeof( hpak_dir_t ) * num_lumps );
	FS_Read( f, dataDir, sizeof( hpak_dir_t ) * num_lumps );

	if( !quiet ) MsgDev( D_INFO, "# Type Size FileName : MD5 Hash\n" );

	for( i = 0; i < num_lumps; i++ )
	{
		if( dataDir[i].size < 1 || dataDir[i].size > 131071 )
		{
			// odd max size
			MsgDev( D_ERROR, "HPAK_ValidatePak: lump %i has invalid size %s\n", i, Q_pretifymem( dataDir[i].size, 2 ));
			Mem_Free( dataDir );
			FS_Close(f);
			return false;
		}

		dataPak = Z_Malloc( dataDir[i].size );
		FS_Seek( f, dataDir[i].seek, SEEK_SET );
		FS_Read( f, dataPak, dataDir[i].size );

		Q_memset( &MD5_Hash, 0, sizeof( MD5Context_t ));
		MD5Init( &MD5_Hash );
		MD5Update( &MD5_Hash, dataPak, dataDir[i].size );
		MD5Final( (byte *)md5, &MD5_Hash );

		pRes = &dataDir[i].DirectoryResource;

		MsgDev( D_INFO, "%i:      %s %s %s:   ", i, HPAK_TypeFromIndex( pRes->type ),
		Q_pretifymem( pRes->nDownloadSize, 2 ), pRes->szFileName );  

		if( Q_memcmp( md5, pRes->rgucMD5_hash, 0x10 ))
		{
			if( quiet )
			{
				MsgDev( D_ERROR, "HPAK_ValidatePak: %s has invalid checksum.\n", pakname );
				Mem_Free( dataPak );
				Mem_Free( dataDir );
				FS_Close( f );
				return false;
			}
			else MsgDev( D_INFO, "failed\n" );
		}
		else
		{
			if( !quiet ) MsgDev( D_INFO, "OK\n" );
		}

		// at this point, it's passed our checks.
		Mem_Free( dataPak );
	}

	Mem_Free( dataDir );
	FS_Close( f );
	return true;
}

void HPAK_ValidatePak( const char *filename )
{
	HPAK_Validate( filename, true );
}

void HPAK_CheckIntegrity( const char *filename )
{
	string	pakname;

	if( !filename || !filename[0] ) return;
	Q_strncpy( pakname, filename, sizeof( pakname ));
	FS_StripExtension( pakname );
	FS_DefaultExtension( pakname, ".hpk" );

	HPAK_ValidatePak( pakname );
}

void HPAK_CheckSize( const char *filename )
{
	string	pakname;
	int	maxsize;

	maxsize = hpk_maxsize->integer;
	if( maxsize <= 0 ) return;

	if( !filename || !filename[0] ) return;
	Q_strncpy( pakname, filename, sizeof( pakname ));
	FS_StripExtension( pakname );
	FS_DefaultExtension( pakname, ".hpk" );

	if( FS_FileSize( pakname, false ) > ( maxsize * 1000000 ))
		MsgDev( D_ERROR, "HPAK_CheckSize: %s is too large.\n", filename );
}

qboolean HPAK_ResourceForHash( const char *filename, char *inHash, resource_t *pRes )
{
	file_t		*f;
	hpak_t		*hpak;
	hpak_container_t	hpakcontainer;
	hpak_header_t	hdr;
	string		pakname;
	int		ret;

	if( !filename || !filename[0] )
		return false;
	
	for( hpak = hpak_queue; hpak != NULL; hpak = hpak->next )
	{
		if( !Q_stricmp( hpak->name, filename ) && !Q_memcmp( hpak->HpakResource.rgucMD5_hash, inHash, 0x10 ))
		{
			if( pRes != NULL ) *pRes = hpak->HpakResource;
			return true;
		}
	}

	Q_strncpy( pakname, filename, sizeof( pakname ));
	FS_StripExtension( pakname );
	FS_DefaultExtension( pakname, ".hpk" );

	f = FS_Open( pakname, "rb", false );
	if( !f ) return false;

	FS_Read( f, &hdr, sizeof( hdr ));

	if( hdr.ident != IDCUSTOMHEADER )
	{
		MsgDev( D_ERROR, "HPAK_ResourceForHash: %s it's not a HPK file.\n", pakname );
		FS_Close( f );
		return false;
	}

	if( hdr.version != IDCUSTOM_VERSION )
	{
		MsgDev( D_ERROR, "HPAK_ResourceForHash: %s has invalid version (%i should be %i).\n", pakname, hdr.version, IDCUSTOM_VERSION );
		FS_Close( f );
		return false;
	}

	FS_Seek( f, hdr.seek, SEEK_SET );
	FS_Read( f, &hpakcontainer.count, sizeof( hpakcontainer.count ));

	if( hpakcontainer.count < 1 || hpakcontainer.count > MAX_FILES_IN_WAD )
	{
		MsgDev( D_ERROR, "HPAK_ResourceForHash: %s has too many lumps %u.\n", pakname, hpakcontainer.count );
		FS_Close( f );
		return false;
	}

	hpakcontainer.dirs = Z_Malloc( sizeof( hpak_dir_t ) * hpakcontainer.count );
	FS_Read( f, hpakcontainer.dirs, sizeof( hpak_dir_t ) * hpakcontainer.count );
	ret = HPAK_FindResource( &hpakcontainer, inHash, pRes );

	Mem_Free( hpakcontainer.dirs );
	FS_Close( f );
	return(ret);
}

qboolean HPAK_ResourceForIndex( const char *filename, int index, resource_t *pRes )
{
	file_t		*f;
	hpak_header_t	hdr;
	hpak_container_t	hpakcontainer;
	string		pakname;

	if( !filename || !filename[0] )
		return false;

	Q_strncpy( pakname, filename, sizeof( pakname ));
	FS_StripExtension( pakname );
	FS_DefaultExtension( pakname, ".hpk" );

	f = FS_Open( pakname, "rb", false );
	FS_Read( f, &hdr, sizeof( hdr ));

	if( hdr.ident != IDCUSTOMHEADER )
	{
		MsgDev( D_ERROR, "HPAK_ResourceForIndex: %s it's not a HPK file.\n", pakname );
		FS_Close( f );
		return false;
	}

	if( hdr.version != IDCUSTOM_VERSION )
	{
		MsgDev( D_ERROR, "HPAK_ResourceForIndex: %s has invalid version (%i should be %i).\n", pakname, hdr.version, IDCUSTOM_VERSION );
		FS_Close( f );
		return false;
	}

	FS_Seek( f, hdr.seek, SEEK_SET );
	FS_Read( f, &hpakcontainer.count, sizeof( hpakcontainer.count ));

	if( hpakcontainer.count < 1 || hpakcontainer.count > MAX_FILES_IN_WAD )
	{
		MsgDev( D_ERROR, "HPAK_ResourceForIndex: %s has too many lumps %u.\n", pakname, hpakcontainer.count );
		FS_Close( f );
		return false;
	}

	if( index < 1 || index > hpakcontainer.count )
	{
		MsgDev( D_ERROR, "HPAK_ResourceForIndex: %s, lump with index %i doesn't exist.\n", pakname, index );
		FS_Close( f );
		return false;
	}

	hpakcontainer.dirs = Z_Malloc( sizeof( hpak_dir_t ) * hpakcontainer.count );

	// we could just seek the right data...
	FS_Read( f, hpakcontainer.dirs, sizeof( hpak_dir_t ) * hpakcontainer.count );
	*pRes = hpakcontainer.dirs[index-1].DirectoryResource;
	Mem_Free( hpakcontainer.dirs );
	FS_Close( f );

	return true;
}

qboolean HPAK_GetDataPointer( const char *filename, resource_t *pResource, byte **buffer, int *size )
{
	file_t		*f;
	int		i, num_lumps;
	hpak_dir_t	*direntries;
	byte		*tmpbuf;
	string		pakname;
	hpak_t		*queue;
	hpak_header_t	hdr;

	if( !filename || !filename[0] )
		return false;

	if( buffer ) *buffer = NULL;
	if( size ) *size = 0;

	for( queue = hpak_queue; queue != NULL; queue = queue->next )
	{
		if( !Q_stricmp(queue->name, filename ) && !Q_memcmp( queue->HpakResource.rgucMD5_hash, pResource->rgucMD5_hash, 16 ))
		{
			if( buffer )
			{
				tmpbuf = Z_Malloc( queue->size );
				Q_memcpy( tmpbuf, queue->data, queue->size );
				*buffer = tmpbuf;
			}

			if( size ) *size = queue->size;

			return true;
		}
	}

	Q_strncpy( pakname, filename, sizeof( pakname ));
	FS_StripExtension( pakname );
	FS_DefaultExtension( pakname, ".hpk" );

	f = FS_Open( pakname, "rb", false );
	if( !f ) return false;

	FS_Read( f, &hdr, sizeof( hdr ));

	if( hdr.ident != IDCUSTOMHEADER )
	{
		MsgDev( D_ERROR, "HPAK_GetDataPointer: %s it's not a HPK file.\n", pakname );
		FS_Close( f );
		return false;
	}

	if( hdr.version != IDCUSTOM_VERSION )
	{
		MsgDev( D_ERROR, "HPAK_GetDataPointer: %s has invalid version (%i should be %i).\n", pakname, hdr.version, IDCUSTOM_VERSION );
		FS_Close( f );
		return false;
	}

	FS_Seek( f, hdr.seek, SEEK_SET );
	FS_Read( f, &num_lumps, sizeof( num_lumps ));

	if( num_lumps < 1 || num_lumps > MAX_FILES_IN_WAD )
	{
		MsgDev( D_ERROR, "HPAK_GetDataPointer: %s has too many lumps %u.\n", filename, num_lumps );
		FS_Close( f );
		return false;
	}

	direntries = Z_Malloc( sizeof( hpak_dir_t ) * num_lumps );
	FS_Read( f, direntries, sizeof( hpak_dir_t ) * num_lumps );

	for( i = 0; i < num_lumps; i++ )
	{
		if( !Q_memcmp( direntries[i].DirectoryResource.rgucMD5_hash, pResource->rgucMD5_hash, 16 ))
		{
			FS_Seek( f, direntries[i].seek, SEEK_SET );

			if( buffer && direntries[i].size > 0 )
			{
				tmpbuf = Z_Malloc( direntries[i].size );
				FS_Read( f, tmpbuf, direntries[i].size );
				*buffer = tmpbuf;
			}

			Mem_Free( direntries );
			FS_Close( f );
			return true;
		}
	}

	Mem_Free( direntries );
	FS_Close( f );
	return false;
}

void HPAK_RemoveLump( const char *name, resource_t *resource )
{
	string		read_path;
	string		save_path;
	file_t		*f1, *f2;
	hpak_container_t	hpak_read;
	hpak_container_t	hpak_save;
	int		i, j;

	if( !name || !name[0] || !resource )
		return;

	HPAK_FlushHostQueue();

	Q_strncpy( read_path, name, sizeof( read_path ));
	FS_StripExtension( read_path );
	FS_DefaultExtension( read_path, ".hpk" );

	f1 = FS_Open( read_path, "rb", false );
	if( !f1 )
	{
		MsgDev( D_ERROR, "HPAK_RemoveLump: %s couldn't open.\n", read_path );
		return;
	}

	Q_strncpy( save_path, read_path, sizeof( save_path ));
	FS_StripExtension( save_path );
	FS_DefaultExtension( save_path, ".hp2" );
	f2 = FS_Open( save_path, "w+b", false );
	if( !f2 )
	{
		MsgDev( D_ERROR, "HPAK_RemoveLump: %s couldn't open.\n", save_path );
		FS_Close( f1 );
		return;
	}

	FS_Seek( f1, 0, SEEK_SET );
	FS_Seek( f2, 0, SEEK_SET );

	// header copy
	FS_Read( f1, &hash_pack_header, sizeof( hpak_header_t ));
	FS_Write( f2, &hash_pack_header, sizeof( hpak_header_t ));

	if( hash_pack_header.ident != IDCUSTOMHEADER || hash_pack_header.version != IDCUSTOM_VERSION )
	{
		MsgDev( D_ERROR, "HPAK_RemoveLump: %s has invalid header.\n", read_path );
		FS_Close( f1 );
		FS_Close( f2 );
		FS_Delete( save_path ); // delete temp file
		return;
	}

	FS_Seek( f1, hash_pack_header.seek, SEEK_SET );
	FS_Read( f1, &hpak_read.count, sizeof( hpak_read.count ));

	if( hpak_read.count < 1 || hpak_read.count > MAX_FILES_IN_WAD )
	{
		MsgDev( D_ERROR, "HPAK_RemoveLump: %s has invalid number of lumps.\n", read_path );
		FS_Close( f1 );
		FS_Close( f2 );
		FS_Delete( save_path ); // delete temp file
		return;
	}

	if( hpak_read.count == 1 )
	{
		MsgDev( D_ERROR, "HPAK_RemoveLump: %s only has one element, so it's not deleted.\n", read_path );
		FS_Close( f1 );
		FS_Close( f2 );
		FS_Delete( read_path );
		FS_Delete( save_path );
		return;
	}

	hpak_save.count = hpak_read.count - 1;
	hpak_read.dirs = Z_Malloc( sizeof( hpak_dir_t ) * hpak_read.count );
	hpak_save.dirs = Z_Malloc( sizeof( hpak_dir_t ) * hpak_save.count );

	FS_Read( f1, hpak_read.dirs, sizeof( hpak_dir_t ) * hpak_read.count );

	if( !HPAK_FindResource( &hpak_read, (char *)resource->rgucMD5_hash, NULL ))
	{
		MsgDev( D_ERROR, "HPAK_RemoveLump: Couldn't find the lump %s in hpak %s.n", resource->szFileName, read_path );
		Mem_Free( hpak_read.dirs );
		Mem_Free( hpak_save.dirs );
		FS_Close( f1 );
		FS_Close( f2 );
		FS_Delete( save_path );
		return;
	}

	MsgDev( D_INFO, "Removing lump %s from %s.\n", resource->szFileName, read_path );

	// If there's a collision, we've just corrupted this hpak.
	for( i = 0, j = 0; i < hpak_read.count; i++ )
	{
		if( !Q_memcmp( hpak_read.dirs[i].DirectoryResource.rgucMD5_hash, resource->rgucMD5_hash, 16 ))
			continue;

		hpak_save.dirs[j] = hpak_read.dirs[i];
		hpak_save.dirs[j].seek = FS_Tell( f2 );
		FS_Seek( f1, hpak_read.dirs[j].seek, SEEK_SET );
		HPAK_FileCopy( f2, f1, hpak_save.dirs[j].size );
		j++;
	}

	hash_pack_header.seek = FS_Tell( f2 );
	FS_Write( f2, &hpak_save.count, ( hpak_save.count ));

	for( i = 0; i < hpak_save.count; i++ )
	{
		FS_Write( f2, &hpak_save.dirs[i], sizeof( hpak_dir_t ));
	}

	FS_Seek( f2, 0, SEEK_SET );
	FS_Write( f2, &hash_pack_header, sizeof( hpak_header_t ));

	Mem_Free( hpak_read.dirs );
	Mem_Free( hpak_save.dirs );
	FS_Close( f1 );
	FS_Close( f2 );

	FS_Delete( read_path );
	FS_Rename( save_path, read_path );
}

void HPAK_List_f( void )
{
	// TODO: implement
}

void HPAK_Extract_f( void )
{
	// TODO: implement
}

void HPAK_Remove_f( void )
{
	// TODO: implement
}

void HPAK_Validate_f( void )
{
	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: hpkval <filename>\n" );
		return;
	}

	HPAK_Validate( Cmd_Argv( 1 ), false );
}

void HPAK_Init( void )
{
	Cmd_AddCommand( "hpklist", HPAK_List_f, "list all files in specified HPK-file" );
	Cmd_AddCommand( "hpkremove", HPAK_Remove_f, "remove specified file from HPK-file" );
	Cmd_AddCommand( "hpkval", HPAK_Validate_f, "validate specified HPK-file" );
	Cmd_AddCommand( "hpkextract", HPAK_Extract_f, "extract all lumps from specified HPK-file" );
	hpk_maxsize = Cvar_Get( "hpk_maxsize", "0", 0, "set limit by size for all HPK-files ( 0 - unlimited )" );

	hpak_queue = NULL;
}
