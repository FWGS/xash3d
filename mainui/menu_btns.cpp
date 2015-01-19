/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "extdll.h"
#include "basemenu.h"
#include "utils.h"
#include "menu_btnsbmp_table.h"

#define ART_BUTTONS_MAIN		"gfx/shell/btns_main.bmp"	// we support bmp only

const char *MenuButtons[PC_BUTTONCOUNT] =
{
	"New game",
	"Resume Game",
	"Hazard Course",
	"Configuration",
	"Load game",
	"Save/load game",
	"View readme",
	"Quit",
	"Multiplayer",
	"Easy",
	"Medium",
	"Difficult",
	"Save game",
	"Load game",
	"Cancel",
	"Game options",
	"Video",
	"Audio",
	"Controls",
	"Done",
	"Quickstart",
	"Use defaults",
	"Ok",
	"Video options",
	"Video modes",
	"Adv controls",
	"Order Half-life",
	"Delete",
	"Internet games",
	"Chat rooms",
	"Lan games",
	"Customize",
	"Skip",
	"Exit",
	"Connect",
	"Refresh",
	"Filter",
	"Filter",
	"Create",
	"Create game",
	"Chat rooms",
	"List rooms",
	"Search",
	"Servers",
	"Join",
	"Find",
	"Create room",
	"Join game",
	"Search games",
	"Find game",
	"Start game",
	"View game info",
	"Update",
	"Add server",
	"Disconnect",
	"Console",
	"Content control",
	"Update",
	"Visit won",
	"Previews",
	"Adv options",
	"3D info site",
	"Custom Game",
	"Activate",
	"Install",
	"Visit web site",
	"Refresh list",
	"Deactivate",
	"Adv options",
	"Spectate game",
	"Spectate games"
};

/*
=================
UI_LoadBmpButtons
=================
*/
void UI_LoadBmpButtons( void )
{
	memset( uiStatic.buttonsPics, 0, sizeof( uiStatic.buttonsPics ));

	int bmp_filesize;
	byte *bmp_buffer = LOAD_FILE( ART_BUTTONS_MAIN, &bmp_filesize );

	if( !bmp_buffer || !bmp_filesize )
	{
		Con_Printf( "UI_LoadBmpButtons: btns_main.bmp not found\n" );
		return;
	}

	BITMAPFILEHEADER *pFileHdr = (BITMAPFILEHEADER *)bmp_buffer;
	BITMAPINFOHEADER *pInfoHdr = (BITMAPINFOHEADER *)&bmp_buffer[sizeof( BITMAPFILEHEADER )];

	BITMAPINFOHEADER NewInfoHdr;
	BITMAPFILEHEADER NewFileHdr;

	if( pInfoHdr->biBitCount == 8 && pInfoHdr->biClrUsed == 0 )
		pInfoHdr->biClrUsed = 256; // all colors used

	memcpy( &NewFileHdr, pFileHdr, sizeof( BITMAPFILEHEADER ));
	memcpy( &NewInfoHdr, pInfoHdr, sizeof( BITMAPINFOHEADER ));

	byte *palette = bmp_buffer + sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER );
	int palette_sz = pInfoHdr->biClrUsed * sizeof( RGBQUAD );
	uiStatic.buttons_width = pInfoHdr->biWidth;
	uiStatic.buttons_height = 78;	// fixed height

	// determine buttons count by image height...
	int pic_count = ( pInfoHdr->biHeight / uiStatic.buttons_height );

	int cutted_img_sz = ( pInfoHdr->biWidth * uiStatic.buttons_height * ( pInfoHdr->biBitCount >> 3 ));
	int CuttedBmpSize = sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER ) + palette_sz + cutted_img_sz;
	byte *img_data = &bmp_buffer[pFileHdr->bfOffBits + cutted_img_sz * ( pic_count - 1 )];

	NewFileHdr.bfSize = CuttedBmpSize;
	NewFileHdr.bfOffBits = sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER ) + palette_sz;
	NewInfoHdr.biHeight = uiStatic.buttons_height;
	NewInfoHdr.biSizeImage = cutted_img_sz;

	char fname[256];
	byte *raw_img_buff = (byte *)MALLOC( CuttedBmpSize );

	for( int i = 0; i < pic_count; i++ )
	{
		sprintf( fname, "#btns_%d.bmp", i );

		int offset = 0;
		memcpy( &raw_img_buff[offset], &NewFileHdr, sizeof( BITMAPFILEHEADER ));
		offset += sizeof( BITMAPFILEHEADER );

		memcpy( &raw_img_buff[offset], &NewInfoHdr, NewInfoHdr.biSize );
		offset += NewInfoHdr.biSize;

		if( NewInfoHdr.biBitCount <= 8 )
		{
 			memcpy( &raw_img_buff[offset], palette, palette_sz );
 			offset += palette_sz;
		}
		
		memcpy( &raw_img_buff[offset], img_data, cutted_img_sz );

		// upload image into video memory
		uiStatic.buttonsPics[i] = PIC_Load( fname, raw_img_buff, CuttedBmpSize );

		img_data -= cutted_img_sz;
	}

	FREE( raw_img_buff );
	FREE_FILE( bmp_buffer );
}