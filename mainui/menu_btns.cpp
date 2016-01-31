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

typedef struct bmp_s
{
	//char magic[2];	//Useless.
	unsigned int	filesz;
	unsigned short	creator1;
	unsigned short	creator2;
	unsigned int	bmp_offset;
	unsigned int	biSize;
	unsigned int	biWidth;
	unsigned int	biHeight;
	unsigned short	biPlanes;
	unsigned short	biBitCount;
	unsigned int	biCompression;
	unsigned int	biSizeImage;
	unsigned int	biXPelsPerMeter;
	unsigned int	biYPelsPerMeter;
	unsigned int	biClrUsed;
	unsigned int	biClrImportant;
}bmp_t;

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

	bmp_t bhdr;
	memcpy( &bhdr, &bmp_buffer[sizeof( short )], sizeof( bmp_t ));
	
	int pallete_sz = bhdr.bmp_offset - sizeof( bmp_t ) - sizeof( short );

	uiStatic.buttons_height = ( bhdr.biBitCount == 4 ) ? 80 : 78; // bugstompers issues
	uiStatic.buttons_width = bhdr.biWidth - 3; // make some offset

	int stride = bhdr.biWidth * bhdr.biBitCount / 8;
	int cutted_img_sz = ((stride + 3 ) & ~3) * uiStatic.buttons_height;
	int CuttedBmpSize = sizeof( bmp_t ) + sizeof( short ) + pallete_sz + cutted_img_sz;
	byte *img_data = &bmp_buffer[bmp_len_holder-cutted_img_sz];

	if ( bhdr.biBitCount <= 8 )
	{
		byte* pallete=&bmp_buffer[sizeof( bmp_t ) + sizeof( short )];
		byte* firstpixel_col=&pallete[img_data[0]*4];
		firstpixel_col[0]=firstpixel_col[1]=firstpixel_col[2]=0;
	}

	// determine buttons count by image height...
	// int pic_count = ( pInfoHdr->biHeight == 5538 ) ? PC_BUTTONCOUNT
	int pic_count = ( bhdr.biHeight / 78 );

	bhdr.biHeight = 78;     //uiStatic.buttons_height;
	bhdr.filesz = CuttedBmpSize;
	bhdr.biSizeImage = CuttedBmpSize - bhdr.bmp_offset;

	char fname[256];
	byte *raw_img_buff = (byte *)MALLOC( sizeof( bmp_t ) + sizeof( short ) + pallete_sz + cutted_img_sz );

	for( int i = 0; i < pic_count; i++ )
	{
		int offset = sizeof( short );
		sprintf( fname, "#btns_%d.bmp", i );

		memcpy( raw_img_buff, bmp_buffer, offset);
		
		memcpy( &raw_img_buff[offset], &bhdr, sizeof( bmp_t ));
		offset += sizeof( bmp_t );

		if( bhdr.biBitCount <= 8 )
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
