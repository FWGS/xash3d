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

#define ART_BANNER	  	"gfx/shell/head_touchoptions"
#define ART_GAMMA		"gfx/shell/gamma"

#define ID_BACKGROUND 	0
#define ID_BANNER	  	1
#define ID_DONE	  	2
#define ID_CANCEL 3
#define ID_PREVIEW 4
#define ID_FILELIST 5

uiFileDialogGlobal_t	uiFileDialogGlobal;

typedef struct
{
	menuFramework_s	menu;
	char		filePath[UI_MAXGAMES][95];
	char		*filePathPtr[UI_MAXGAMES];
	menuBitmap_s	background;
	//menuBitmap_s	banner;
	menuAction_s preview;
	HIMAGE image;

	menuPicButton_s	done;
	menuPicButton_s	cancel;

	menuScrollList_s fileList;
} uiFileDialog_t;

static uiFileDialog_t	uiFileDialog;

static void UI_Preview_Ownerdraw( void *self )
{
	menuCommon_s	*item = (menuCommon_s *)self;
	UI_FillRect( item->x - 2, item->y - 2, item->width + 4, item->height + 4, 0xFFC0C0C0 );
	UI_FillRect( item->x, item->y, item->width, item->height, 0xFF808080 );
	PIC_Set( uiFileDialog.image, 255, 255, 255, 255 );
	PIC_DrawTrans( item->x, item->y, item->width, item->height );
}

static void UI_FileDialog_GetFileList( void )
{
	char	**filenames;
	int	i = 0, numFiles, j, k;


	for( k = 0; k < uiFileDialogGlobal.npatterns; k++)
	{
		filenames = FS_SEARCH( uiFileDialogGlobal.patterns[k], &numFiles, TRUE );
		for ( j = 0; j < numFiles; i++, j++ )
		{
			if( i >= UI_MAXGAMES ) break;
			strcpy( uiFileDialog.filePath[i],filenames[j] );
			uiFileDialog.filePathPtr[i] = uiFileDialog.filePath[i];
		}
	}
	uiFileDialog.fileList.numItems = i;

	if( uiFileDialog.fileList.generic.charHeight )
	{
		uiFileDialog.fileList.numRows = (uiFileDialog.fileList.generic.height2 / uiFileDialog.fileList.generic.charHeight) - 2;
		if( uiFileDialog.fileList.numRows > uiFileDialog.fileList.numItems )
			uiFileDialog.fileList.numRows = i;
	}

	for ( ; i < UI_MAXGAMES; i++ )
		uiFileDialog.filePathPtr[i] = NULL;


	uiFileDialog.fileList.itemNames = (const char **)uiFileDialog.filePathPtr;
	uiFileDialog.image = PIC_Load( uiFileDialog.filePath[ uiFileDialog.fileList.curItem ] );
}

/*
=================
UI_FileDialog_Callback
=================
*/
static void UI_FileDialog_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	switch( item->id )
	{/*
	   // checkboxes
	case ID_XXX
		if( event == QM_PRESSED )
			((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_PRESSED;
		else ((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_FOCUS;
		break;*/
	}

	if( event == QM_CHANGED )
	{
		switch( item->id )
		{
		case ID_FILELIST:
			if( uiFileDialogGlobal.preview )
				uiFileDialog.image = PIC_Load( uiFileDialog.filePath[ uiFileDialog.fileList.curItem ] );
			break;
		}

		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		strcpy( uiFileDialogGlobal.result, uiFileDialog.filePath[uiFileDialog.fileList.curItem] );
		uiFileDialogGlobal.valid = false;
		UI_PopMenu();
		uiFileDialogGlobal.callback( true );
		break;
	case ID_CANCEL:
		strcpy( uiFileDialogGlobal.result, "" );
		uiFileDialogGlobal.valid = false;
		UI_PopMenu();
		uiFileDialogGlobal.callback( false );
		break;
	}
}

/*
=================
UI_FileDialog_Init
=================
*/
static void UI_FileDialog_Init( void )
{
	memset( &uiFileDialog, 0, sizeof( uiFileDialog_t ));

	//uiTouchOptions.hTestImage = PIC_Load( ART_GAMMA, PIC_KEEP_RGBDATA );

	uiFileDialog.menu.vidInitFunc = UI_FileDialog_Init;

	uiFileDialog.background.generic.id = ID_BACKGROUND;
	uiFileDialog.background.generic.type = QMTYPE_BITMAP;
	uiFileDialog.background.generic.flags = QMF_INACTIVE;
	uiFileDialog.background.generic.x = 0;
	uiFileDialog.background.generic.y = 0;
	uiFileDialog.background.generic.width = uiStatic.width;
	uiFileDialog.background.generic.height = 768;
	uiFileDialog.background.pic = ART_BACKGROUND;

	/*uiTouchOptions.banner.generic.id = ID_BANNER;
	uiTouchOptions.banner.generic.type = QMTYPE_BITMAP;
	uiTouchOptions.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiTouchOptions.banner.generic.x = UI_BANNER_POSX;
	uiTouchOptions.banner.generic.y = UI_BANNER_POSY;
	uiTouchOptions.banner.generic.width = UI_BANNER_WIDTH;
	uiTouchOptions.banner.generic.height = UI_BANNER_HEIGHT;
	uiTouchOptions.banner.pic = ART_BANNER;*/

	uiFileDialog.done.generic.id = ID_DONE;
	uiFileDialog.done.generic.type = QMTYPE_BM_BUTTON;
	uiFileDialog.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiFileDialog.done.generic.x = 72;
	uiFileDialog.done.generic.y = 150;
	uiFileDialog.done.generic.name = "Done";
	uiFileDialog.done.generic.statusText = "Use selected file";
	uiFileDialog.done.generic.callback = UI_FileDialog_Callback;

	UI_UtilSetupPicButton( &uiFileDialog.done, PC_DONE );

	uiFileDialog.cancel.generic.id = ID_CANCEL;
	uiFileDialog.cancel.generic.type = QMTYPE_BM_BUTTON;
	uiFileDialog.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiFileDialog.cancel.generic.x = 72;
	uiFileDialog.cancel.generic.y = 210;
	uiFileDialog.cancel.generic.name = "Cancel";
	uiFileDialog.cancel.generic.statusText = "Cancel file selection";
	uiFileDialog.cancel.generic.callback = UI_FileDialog_Callback;

	UI_UtilSetupPicButton( &uiFileDialog.cancel, PC_CANCEL );

	uiFileDialog.fileList.generic.id = ID_FILELIST;
	uiFileDialog.fileList.generic.type = QMTYPE_SCROLLLIST;
	uiFileDialog.fileList.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_SMALLFONT;
	uiFileDialog.fileList.generic.x = 340;
	uiFileDialog.fileList.generic.y = 150;
	uiFileDialog.fileList.generic.width = 600;
	uiFileDialog.fileList.generic.height = 500;
	uiFileDialog.fileList.generic.callback = UI_FileDialog_Callback;

	uiFileDialog.preview.generic.id = ID_PREVIEW;
	uiFileDialog.preview.generic.type = QMTYPE_ACTION;
	uiFileDialog.preview.generic.flags =  QMF_INACTIVE;
	uiFileDialog.preview.generic.x = 72;
	uiFileDialog.preview.generic.y = 300;
	uiFileDialog.preview.generic.width = 196;
	uiFileDialog.preview.generic.height = 196;
	uiFileDialog.preview.generic.ownerdraw = UI_Preview_Ownerdraw;

	UI_FileDialog_GetFileList();

	UI_AddItem( &uiFileDialog.menu, (void *)&uiFileDialog.background );
	//UI_AddItem( &uiTouchOptions.menu, (void *)&uiTouchOptions.banner );
	UI_AddItem( &uiFileDialog.menu, (void *)&uiFileDialog.done );
	UI_AddItem( &uiFileDialog.menu, (void *)&uiFileDialog.cancel );
	if( uiFileDialogGlobal.preview )
	UI_AddItem( &uiFileDialog.menu, (void *)&uiFileDialog.preview );
	UI_AddItem( &uiFileDialog.menu, (void *)&uiFileDialog.fileList );

}

/*
=================
UI_FileDialog_Precache
=================
*/
void UI_FileDialog_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	//PIC_Load( ART_BANNER );
}

/*
=================
UI_FileDialog_Menu
=================
*/
void UI_FileDialog_Menu( void )
{
	UI_FileDialog_Precache();
	UI_FileDialog_Init();


	UI_PushMenu( &uiFileDialog.menu );
	//if( !uiFileDialogGlobal.valid )
		//UI_PopMenu();
}
