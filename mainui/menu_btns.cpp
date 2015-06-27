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

#include "port.h"

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

#pragma pack(push,1)
typedef struct
{
		char magic[2];
		unsigned int filesz;
		unsigned short creator1;
		unsigned short creator2;
		unsigned int bmp_offset;
} bmphdr_t;
#pragma pack(pop)

#ifndef _WIN32
struct BITMAPFILEHEADER
{
	WORD    bfType;
	DWORD   bfSize;
	WORD    bfReserved1;
	WORD    bfReserved2;
	DWORD   bfOffBits;
};

struct BITMAPINFOHEADER
{
	DWORD    biSize;
	DWORD    biWidth;
	DWORD    biHeight;
	WORD    biPlanes;
	WORD    biBitCount;
	DWORD    biCompression;
	DWORD    biSizeImage;
	DWORD    biXPelsPerMeter;
	DWORD    biYPelsPerMeter;
	DWORD    biClrUsed;
	DWORD    biClrImportant;
};
#endif


/*
=================
UI_LoadBmpButtons
=================
*/
void UI_LoadBmpButtons( void )
{
	memset( uiStatic.buttonsPics, 0, sizeof( uiStatic.buttonsPics ));

	int bmp_len_holder;
	byte *bmp_buffer = (byte*)LOAD_FILE( ART_BUTTONS_MAIN, &bmp_len_holder );

	if( !bmp_buffer || !bmp_len_holder )
	{
		Con_Printf( "UI_LoadBmpButtons: btns_main.bmp not found\n" );
		return;
	}

	BITMAPINFOHEADER *pInfoHdr;
	bmphdr_t *pHdr;

	pInfoHdr =(BITMAPINFOHEADER *)&bmp_buffer[sizeof(bmphdr_t)];
	pHdr = (bmphdr_t*)bmp_buffer;

	BITMAPINFOHEADER CuttedDibHdr;
	bmphdr_t CuttedHdr;

	memcpy( &CuttedHdr, pHdr, sizeof( bmphdr_t ));
	memcpy( &CuttedDibHdr, pInfoHdr, pInfoHdr->biSize );

	int pallete_sz = pHdr->bmp_offset - sizeof( bmphdr_t ) - pInfoHdr->biSize;

	uiStatic.buttons_height = ( pInfoHdr->biBitCount == 4 ) ? 80 : 78; // bugstompers issues
	uiStatic.buttons_width = pInfoHdr->biWidth - 3; // make some offset

	int stride = (pInfoHdr->biWidth * pInfoHdr->biBitCount / 8);
	int cutted_img_sz = ((stride + 3 ) & ~3) * uiStatic.buttons_height;
	int CuttedBmpSize = sizeof( bmphdr_t ) + pInfoHdr->biSize + pallete_sz + cutted_img_sz;
	byte *img_data = &bmp_buffer[bmp_len_holder-cutted_img_sz];

	if ( pInfoHdr->biBitCount <= 8 )
	{
		byte* pallete=&bmp_buffer[sizeof( bmphdr_t ) + pInfoHdr->biSize];
		byte* firstpixel_col=&pallete[img_data[0]*4];
		firstpixel_col[0]=firstpixel_col[1]=firstpixel_col[2]=0;
	}

	CuttedDibHdr.biHeight = 78;     //uiStatic.buttons_height;
	CuttedHdr.filesz = CuttedBmpSize;
	CuttedDibHdr.biSizeImage = CuttedBmpSize - CuttedHdr.bmp_offset;

	char fname[256];
	byte *raw_img_buff = (byte *)MALLOC( sizeof( bmphdr_t ) + pInfoHdr->biSize + pallete_sz + cutted_img_sz );

	// determine buttons count by image height...
	//      int pic_count = ( pInfoHdr->biHeight == 5538 ) ? PC_BUTTONCOUNT : PC_BUTTONCOUNT - 2;
	int pic_count = ( pInfoHdr->biHeight / 78 );

	for( int i = 0; i < pic_count; i++ )
	{
		sprintf( fname, "#btns_%d.bmp", i );

		int offset = 0;
		memcpy( &raw_img_buff[offset], &CuttedHdr, sizeof( bmphdr_t ));
		offset += sizeof( bmphdr_t );

		memcpy( &raw_img_buff[offset], &CuttedDibHdr, CuttedDibHdr.biSize );
		offset += CuttedDibHdr.biSize;

		if( CuttedDibHdr.biBitCount <= 8 )
		{
			memcpy( &raw_img_buff[offset], &bmp_buffer[offset], pallete_sz );
			offset += pallete_sz;
		}

		memcpy( &raw_img_buff[offset], img_data, cutted_img_sz );

		// upload image into viedo memory
		uiStatic.buttonsPics[i] = PIC_Load( fname, raw_img_buff, CuttedBmpSize );

		img_data -= cutted_img_sz;
	}

	FREE( raw_img_buff );
	FREE_FILE( bmp_buffer );
}
