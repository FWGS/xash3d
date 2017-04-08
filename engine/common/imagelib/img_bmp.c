/*
img_bmp.c - bmp format load & save
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

#include "imagelib.h"

#define BI_SIZE	40 //size of bitmap info header.
#ifndef _WIN32
#define BI_RGB 0

typedef struct tagRGBQUAD {
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
} RGBQUAD;
#endif

/*
=============
Image_LoadBMP
=============
*/
qboolean Image_LoadBMP( const char *name, const byte *buffer, size_t filesize )
{
	byte	*buf_p, *pixbuf, magic[2];
	byte	palette[256][4];
	int	i, columns, column, rows, row, bpp = 1;
	int	padSize = 0, bps;
	qboolean	load_qfont = false;
	bmp_t	bhdr;

	if( filesize < sizeof( bhdr )) return false; 

	buf_p = (byte *)buffer;
	Q_memcpy( magic, buf_p, sizeof( magic ) );
	buf_p += sizeof( magic );	// move pointer
	Q_memcpy( &bhdr, buf_p, sizeof( bmp_t ));
	buf_p += sizeof( bmp_t );

	// bogus file header check
	if( bhdr.reserved0 != 0 ) return false;
	if( bhdr.planes != 1 ) return false;

	if( Q_memcmp( magic, "BM", 2 ))
	{
		MsgDev( D_ERROR, "Image_LoadBMP: only Windows-style BMP files supported (%s)\n", name );
		return false;
	} 

	if( bhdr.bitmapHeaderSize != 0x28 )
	{
		MsgDev( D_ERROR, "Image_LoadBMP: invalid header size %i\n", bhdr.bitmapHeaderSize );
		return false;
	}

	// bogus info header check
	if( bhdr.fileSize != filesize )
	{
		// Sweet Half-Life issues. splash.bmp have bogus filesize
		MsgDev( D_WARN, "Image_LoadBMP: %s have incorrect file size %i should be %i\n", name, filesize, bhdr.fileSize );
          }
          
	// bogus compression?  Only non-compressed supported.
	if( bhdr.compression != BI_RGB ) 
	{
		MsgDev( D_ERROR, "Image_LoadBMP: only uncompressed BMP files supported (%s)\n", name );
		return false;
	}

	image.width = columns = bhdr.width;
	image.height = rows = abs( bhdr.height );

	if( !Image_ValidSize( name ))
		return false;          

	// special hack for loading qfont
	if( !Q_strncmp( "#XASH_SYSTEMFONT_001", name, 20 ))
	{
		// NOTE: same as system font we can use 4-bit bmps only
		// step1: move main layer into alpha-channel (give grayscale from RED channel)
		// step2: fill main layer with 255 255 255 color (white)
		// step3: ????
		// step4: PROFIT!!! (economy up to 150 kb for menu.dll final size)
		image.flags |= IMAGE_HAS_ALPHA;
		load_qfont = true;
	}

	if( bhdr.bitsPerPixel <= 8 )
	{
		int cbPalBytes;

		// figure out how many entries are actually in the table
		if( bhdr.colors == 0 )
		{
			bhdr.colors = 256;
			cbPalBytes = (1 << bhdr.bitsPerPixel) * sizeof( RGBQUAD );
		}
		else cbPalBytes = bhdr.colors * sizeof( RGBQUAD );

		Q_memcpy( palette, buf_p, cbPalBytes );
		buf_p += cbPalBytes;
	}

	if( host.overview_loading && bhdr.bitsPerPixel == 8 )
	{
		// convert green background into alpha-layer, make opacity for all other entries
		for( i = 0; i < bhdr.colors; i++ )
		{
			if( palette[i][0] == 0 && palette[i][1] == 255 && palette[i][2] == 0 )
			{
				palette[i][0] = palette[i][1] = palette[i][2] = palette[i][3] = 0;
				image.flags |= IMAGE_HAS_ALPHA;
			}
			else palette[i][3] = 255;
		}
	}

	if( Image_CheckFlag( IL_KEEP_8BIT ) && bhdr.bitsPerPixel == 8 )
	{
		pixbuf = image.palette = Mem_Alloc( host.imagepool, 1024 );
		image.flags |= IMAGE_HAS_COLOR;
 
		// bmp have a reversed palette colors
		for( i = 0; i < bhdr.colors; i++ )
		{
			*pixbuf++ = palette[i][2];
			*pixbuf++ = palette[i][1];
			*pixbuf++ = palette[i][0];
			*pixbuf++ = palette[i][3];
		}
		image.type = PF_INDEXED_32; // 32 bit palette
	}
	else
	{
		image.palette = NULL;
		image.type = PF_RGBA_32;
		bpp = 4;
	}

	image.size = image.width * image.height * bpp;
	image.rgba = Mem_Alloc( host.imagepool, image.size );

	switch( bhdr.bitsPerPixel )
	{
	case 1:
		padSize = (( 32 - ( bhdr.width % 32 )) / 8 ) % 4;
		break;
	case 4:
		padSize = (( 8 - ( bhdr.width % 8 )) / 2 ) % 4;
		break;
	case 16:
		padSize = ( 4 - ( image.width * 2 % 4 )) % 4;
		break;
	case 8:
	case 24:
		bps = image.width * (bhdr.bitsPerPixel >> 3);
		padSize = ( 4 - ( bps % 4 )) % 4;
		break;
	}

	for( row = rows - 1; row >= 0; row-- )
	{
		pixbuf = image.rgba + (row * columns * bpp);

		for( column = 0; column < columns; column++ )
		{
			byte	red = '\0', green = '\0', blue = '\0', alpha;
			word	shortPixel;
			int	c, k, palIndex;

			switch( bhdr.bitsPerPixel )
			{
			case 1:
				alpha = *buf_p++;
				column--;	// ingnore main iterations
				for( c = 0, k = 128; c < 8; c++, k >>= 1 )
				{
					red = green = blue = (!!(alpha & k) == 1 ? 0xFF : 0x00);
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 0x00;
					if( ++column == columns )
						break;
				}
				break;
			case 4:
				alpha = *buf_p++;
				palIndex = alpha >> 4;
				if( load_qfont )
				{
					*pixbuf++ = red = 255;
					*pixbuf++ = green = 255;
					*pixbuf++ = blue = 255;
					*pixbuf++ = palette[palIndex][2];
				}
				else
				{
					*pixbuf++ = red = palette[palIndex][2];
					*pixbuf++ = green = palette[palIndex][1];
					*pixbuf++ = blue = palette[palIndex][0];
					*pixbuf++ = palette[palIndex][3];
				}
				if( ++column == columns ) break;
				palIndex = alpha & 0x0F;
				if( load_qfont )
				{
					*pixbuf++ = red = 255;
					*pixbuf++ = green = 255;
					*pixbuf++ = blue = 255;
					*pixbuf++ = palette[palIndex][2];
				}
				else
				{
					*pixbuf++ = red = palette[palIndex][2];
					*pixbuf++ = green = palette[palIndex][1];
					*pixbuf++ = blue = palette[palIndex][0];
					*pixbuf++ = palette[palIndex][3];
				}
				break;
			case 8:
				palIndex = *buf_p++;
				red = palette[palIndex][2];
				green = palette[palIndex][1];
				blue = palette[palIndex][0];
				alpha = palette[palIndex][3];

				if( Image_CheckFlag( IL_KEEP_8BIT ))
				{
					*pixbuf++ = palIndex;
				}
				else
				{
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alpha;
				}
				break;
			case 16:
				shortPixel = *(word *)buf_p, buf_p += 2;
				*pixbuf++ = blue = (shortPixel & ( 31 << 10 )) >> 7;
				*pixbuf++ = green = (shortPixel & ( 31 << 5 )) >> 2;
				*pixbuf++ = red = (shortPixel & ( 31 )) << 3;
				*pixbuf++ = 0xff;
				break;
			case 24:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = 0xFF;
				break;
			case 32:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				alpha = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = alpha;
				if( alpha != 255 ) image.flags |= IMAGE_HAS_ALPHA;
				break;
			default:
				MsgDev( D_ERROR, "Image_LoadBMP: illegal pixel_size (%s)\n", name );
				Mem_Free( image.palette );
				Mem_Free( image.rgba );
				return false;
			}
			if( !Image_CheckFlag( IL_KEEP_8BIT ) && ( red != green || green != blue ))
				image.flags |= IMAGE_HAS_COLOR;
		}
		buf_p += padSize;	// actual only for 4-bit bmps
	}

	if( image.palette ) Image_GetPaletteBMP( image.palette );

	return true;
}

qboolean Image_SaveBMP( const char *name, rgbdata_t *pix )
{
	file_t		*pfile = NULL;
	bmp_t		bhdr;
	size_t		total_size = 0, cur_size;
	RGBQUAD		rgrgbPalette[256];
	uint		cbBmpBits;
	byte		*clipbuf = NULL, magic[2];
	byte		*pb, *pbBmpBits;
	uint		cbPalBytes;
	uint		biTrueWidth;
	int		pixel_size;
	int		i, x, y;

	if( FS_FileExists( name, false ) && !Image_CheckFlag( IL_ALLOW_OVERWRITE ) && !host.write_to_clipboard )
		return false; // already existed

	// bogus parameter check
	if( !pix->buffer )
		return false;

	// get image description
	switch( pix->type )
	{
	case PF_INDEXED_24:
	case PF_INDEXED_32:
		pixel_size = 1;
		break;
	case PF_RGB_24:
		pixel_size = 3;
		break;
	case PF_RGBA_32:
		pixel_size = 4;
		break;	
	default:
		MsgDev( D_ERROR, "Image_SaveBMP: unsupported image type %s\n", PFDesc[pix->type].name );
		return false;
	}

	if( !host.write_to_clipboard )
	{
		pfile = FS_Open( name, "wb", false );
		if( !pfile ) return false;
	}

	// NOTE: align transparency column will sucessfully removed
	// after create sprite or lump image, it's just standard requiriments 
	//biTrueWidth = pix->width;
	biTrueWidth = ((pix->width + 3) & ~3); // What is this?
	cbBmpBits = biTrueWidth * pix->height * pixel_size;
	cbPalBytes = ( pixel_size == 1 ) ? 256 * sizeof( RGBQUAD ) : 0;

	// Bogus file header check
	magic[0] = 'B';
	magic[1] = 'M';
	bhdr.fileSize = sizeof( magic ) + sizeof( bmp_t ) + cbBmpBits + cbPalBytes;
	bhdr.reserved0 = 0;
	bhdr.bitmapDataOffset = sizeof( magic ) + sizeof( bmp_t ) + cbPalBytes;
	bhdr.bitmapHeaderSize = BI_SIZE;
	bhdr.width = biTrueWidth;
	bhdr.height = pix->height;
	bhdr.planes = 1;
	bhdr.bitsPerPixel = pixel_size * 8;
	bhdr.compression = BI_RGB;
	bhdr.bitmapDataSize = cbBmpBits;
	bhdr.hRes = 0;
	bhdr.vRes = 0;
	bhdr.colors = ( pixel_size == 1 ) ? 256 : 0;
	bhdr.importantColors = 0;

	if( host.write_to_clipboard )
	{
		// NOTE: the cbPalBytes may be 0
		total_size = BI_SIZE + cbPalBytes + cbBmpBits;
		clipbuf = Z_Malloc( total_size );
		Q_memcpy( clipbuf, (byte *)&bhdr + ( sizeof( bmp_t ) - BI_SIZE ), BI_SIZE );
		cur_size = BI_SIZE;
	}
	else
	{
		// Write header
		FS_Write( pfile, magic, sizeof( magic ));
		FS_Write( pfile, &bhdr, sizeof( bmp_t ));
	}

	pbBmpBits = Mem_Alloc( host.imagepool, cbBmpBits );

	if( pixel_size == 1 )
	{
		pb = pix->palette;

		// copy over used entries
		for( i = 0; i < (int)bhdr.colors; i++ )
		{
			rgrgbPalette[i].rgbRed = *pb++;
			rgrgbPalette[i].rgbGreen = *pb++;
			rgrgbPalette[i].rgbBlue = *pb++;

			// bmp feature - can store 32-bit palette if present
			// some viewers e.g. fimg.exe can show alpha-chanell for it
			if( pix->type == PF_INDEXED_32 )
				rgrgbPalette[i].rgbReserved = *pb++;
			else rgrgbPalette[i].rgbReserved = 0;
		}

		if( host.write_to_clipboard )
		{
			Q_memcpy( clipbuf + cur_size, rgrgbPalette, cbPalBytes );
			cur_size += cbPalBytes;
		}
		else
		{
			// write palette
			FS_Write( pfile, rgrgbPalette, cbPalBytes );
		}
	}

	pb = pix->buffer;

	for( y = 0; y < bhdr.height; y++ )
	{
		i = ( bhdr.height - 1 - y ) * ( bhdr.width );

		for( x = 0; x < pix->width; x++ )
		{
			if( pixel_size == 1 )
			{
				// 8-bit
				pbBmpBits[i] = pb[x];
			}
			else
			{
				// 24 bit
				pbBmpBits[i*pixel_size+0] = pb[x*pixel_size+2];
				pbBmpBits[i*pixel_size+1] = pb[x*pixel_size+1];
				pbBmpBits[i*pixel_size+2] = pb[x*pixel_size+0];
			}

			if( pixel_size == 4 ) // write alpha channel
				pbBmpBits[i*pixel_size+3] = pb[x*pixel_size+3];
			i++;
		}

		pb += pix->width * pixel_size;
	}

	if( host.write_to_clipboard )
	{
		Q_memcpy( clipbuf + cur_size, pbBmpBits, cbBmpBits );
		cur_size += cbBmpBits;
		Sys_SetClipboardData( clipbuf, total_size );
		Z_Free( clipbuf );
	}
	else
	{
		// write bitmap bits (remainder of file)
		FS_Write( pfile, pbBmpBits, cbBmpBits );
		FS_Close( pfile );
	}

	Mem_Free( pbBmpBits );

	return true;
}
