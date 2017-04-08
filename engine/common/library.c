/*
library.c - custom dlls loader
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

#define _GNU_SOURCE

#include "common.h"
#include "library.h"
#include "filesystem.h"
#include "server.h"

char lasterror[1024] = "";
const char *Com_GetLibraryError()
{
	return lasterror;
}

void Com_ResetLibraryError()
{
	lasterror[0] = 0;
}

void Com_PushLibraryError( const char *error )
{
	Q_strncat( lasterror, error, sizeof( lasterror ) );
	Q_strncat( lasterror, "\n", sizeof( lasterror ) );
}

void *Com_FunctionFromName_SR( void *hInstance, const char *pName )
{
	#ifdef XASH_ALLOW_SAVERESTORE_OFFSETS
	if( !Q_memcmp( pName, "ofs:",4 ) )
		return svgame.dllFuncs.pfnGameInit + Q_atoi(pName + 4);
	#endif
	return Com_FunctionFromName( hInstance, pName );
}

#ifdef XASH_ALLOW_SAVERESTORE_OFFSETS
char *Com_OffsetNameForFunction( void *function )
{
	static string sname;
	Q_snprintf( sname, MAX_STRING, "ofs:%d", (int)(void*)(function - (void*)svgame.dllFuncs.pfnGameInit) );
	MsgDev( D_NOTE, "Com_OffsetNameForFunction %s\n", sname );
	return sname;
}
#endif

#ifndef _WIN32

#ifdef __ANDROID__
#include "platform/android/dlsym-weak.h"
#endif


#ifdef NO_LIBDL

#ifndef DLL_LOADER
#error Enable at least one dll backend!!!
#endif

void *dlsym(void *handle, const char *symbol )
{
	MsgDev( D_NOTE, "dlsym( %p, \"%s\" ): stub\n", handle, symbol );
	return NULL;
}
void *dlopen(const char *name, int flag )
{
	MsgDev( D_NOTE, "dlopen( \"%s\", %d ): stub\n", name, flag );
	return NULL;
}
int dlclose(void *handle)
{
	MsgDev( D_NOTE, "dlsym( %p ): stub\n", handle );
	return 0;
}
char *dlerror( void )
{
	return "Loading ELF libraries not supported in this build!\n";
}
int dladdr( const void *addr, Dl_info *info )
{
	return 0;
}



#endif
#ifdef XASH_SDL
#include <SDL_filesystem.h>
#endif

#ifdef TARGET_OS_IPHONE

static void *IOS_LoadLibraryInternal( const char *dllname )
{
	void *pHandle;
	string errorstring = "";
	char path[MAX_SYSPATH];
	
	// load frameworks from Documents directory
	// frameworks should be signed with same key with application
	// Useful for debug to prevent rebuilding app on every library update
	// NOTE: Apple polices forbids loading code from shared places
#ifdef ENABLE_FRAMEWORK_SIDELOAD
	Q_snprintf( path, MAX_SYSPATH, "%s.framework/lib", dllname );
	if( pHandle = dlopen( path, RTLD_LAZY ) )
		return pHandle;
	Q_snprintf( errorstring, MAX_STRING, dlerror() );
#endif
	
#ifdef DLOPEN_FRAMEWORKS
	// load frameworks as it should be located in Xcode builds
	Q_snprintf( path, MAX_SYSPATH, "%s%s.framework/lib", SDL_GetBasePath(), dllname );
#else
	// load libraries from app root to allow re-signing ipa with custom utilities
	Q_snprintf( path, MAX_SYSPATH, "%s%s", SDL_GetBasePath(), dllname );
#endif
	pHandle = dlopen( path, RTLD_LAZY );
	if( !pHandle )
	{
		Com_PushLibraryError(errorstring);
		Com_PushLibraryError(dlerror());
	}
	return pHandle;
}
extern char *g_szLibrarySuffix;
static void *IOS_LoadLibrary( const char *dllname )
{
	string name;
	char *postfix = g_szLibrarySuffix;
	char *pHandle;

	if( !postfix ) postfix = GI->gamedir;

	Q_snprintf( name, MAX_STRING, "%s_%s", dllname, postfix );
	pHandle = IOS_LoadLibraryInternal( name );
	if( pHandle )
		return pHandle;
	return IOS_LoadLibraryInternal( dllname );
}

#endif


void *Com_LoadLibrary( const char *dllname, int build_ordinals_table )
{
	searchpath_t	*search = NULL;
	int		pack_ind;
	char	path [MAX_SYSPATH];
	void *pHandle;

#if TARGET_OS_IPHONE
	return IOS_LoadLibrary(dllname);
#endif

#ifdef __EMSCRIPTEN__
	{
		string prefix;
		Q_strcpy(prefix, getenv( "LIBRARY_PREFIX" ) );
		Q_snprintf( path, MAX_SYSPATH, "%s%s%s",  prefix, dllname, getenv( "LIBRARY_SUFFIX" ) );
		pHandle = dlopen( path, RTLD_LAZY );
		if( !pHandle )
		{
			Com_PushLibraryError( va("Loading %s:\n", path ) );
			Com_PushLibraryError( dlerror() );
		}
		return pHandle;
	}
#endif

	qboolean dll = host.enabledll && ( Q_stristr( dllname, ".dll" ) != 0 );
#ifdef DLL_LOADER
	if(dll)
	{
		pHandle = Loader_LoadLibrary( dllname );
		if(!pHandle)
		{
			string errorstring;
			Q_snprintf( errorstring, MAX_STRING, "Failed to load dll with dll loader: %s", dllname );
			Com_PushLibraryError( errorstring );
		}
	}
	else
#endif
	{
		pHandle = dlopen( dllname, RTLD_LAZY );
		if( !pHandle )
			Com_PushLibraryError(dlerror());
	}
	if(!pHandle)
	{
		search = FS_FindFile( dllname, &pack_ind, true );

		if( !search )
		{
			return NULL;
		}
		sprintf( path, "%s%s", search->filename, dllname );


#ifdef DLL_LOADER
		if(dll)
		{
			pHandle = Loader_LoadLibrary( path );
			if(!pHandle)
			{
				string errorstring;
				Q_snprintf( errorstring, MAX_STRING, "Failed to load dll with dll loader: %s", dllname );
				Com_PushLibraryError( errorstring );
			}
		}
		else
#endif
		{
			pHandle = dlopen( path, RTLD_LAZY );
			if( !pHandle )
				Com_PushLibraryError( dlerror() );
		}
		if(!pHandle)
		{
			return NULL;
		}
	}

	return pHandle;
}

void Com_FreeLibrary( void *hInstance )
{
#ifdef DLL_LOADER
	void *wm;
	if( host.enabledll && (wm = Loader_GetDllHandle( hInstance )) )
		return Loader_FreeLibrary(hInstance);
	else
#endif
	dlclose( hInstance );
}

void *Com_GetProcAddress( void *hInstance, const char *name )
{
#ifdef DLL_LOADER
	void *wm;
	if( host.enabledll && (wm = Loader_GetDllHandle( hInstance )) )
		return Loader_GetProcAddress(hInstance, name);
	else
#endif
	return dlsym( hInstance, name );
}

void *Com_FunctionFromName( void *hInstance, const char *pName )
{
	void *function;
#ifdef DLL_LOADER
	void *wm;
	if( host.enabledll && (wm = Loader_GetDllHandle( hInstance )) )
		return Loader_GetProcAddress(hInstance, pName);
	else
#endif
	function = dlsym( hInstance, pName );
	if(!function)
	{
#ifdef __ANDROID__
		// Shitty Android's dlsym don't resolve weak symbols
		function = dlsym_weak( hInstance, pName );
		if(!function)
#endif
			MsgDev(D_ERROR, "FunctionFromName: Can't get symbol %s: %s\n", pName, dlerror());
	}
	return function;
}

#ifdef XASH_DYNAMIC_DLADDR
int d_dladdr( void *sym, Dl_info *info )
{
	static int (*dladdr_real) ( void *sym, Dl_info *info );

	if( !dladdr_real )
		dladdr_real = dlsym( (void*)(size_t)(-1), "dladdr" );

	Q_memset( info, 0, sizeof( *info ) );

	if( !dladdr_real )
		return -1;

	return dladdr_real(  sym, info );
}
#endif

const char *Com_NameForFunction( void *hInstance, void *function )
{
#ifdef DLL_LOADER
	void *wm;
	if( host.enabledll && (wm = Loader_GetDllHandle( hInstance )) )
		return Loader_GetFuncName_int(wm, function);
	else
#endif
	// Note: dladdr() is a glibc extension
	{
		Dl_info info = {0};
		dladdr((void*)function, &info);
		if(info.dli_sname)
			return info.dli_sname;
	}
#ifdef XASH_ALLOW_SAVERESTORE_OFFSETS
	return Com_OffsetNameForFunction( function );
#endif
}
#elif defined __amd64__
#include <dbghelp.h>
void *Com_LoadLibrary( const char *dllname, int build_ordinals_table )
{
	return LoadLibraryA( dllname );
}
void Com_FreeLibrary( void *hInstance )
{
	FreeLibrary( hInstance );
}


void *Com_GetProcAddress( void *hInstance, const char *name )
{
	return GetProcAddress( hInstance, name );
}

void *Com_FunctionFromName( void *hInstance, const char *name )
{
	return GetProcAddress( hInstance, name );
}

const char *Com_NameForFunction( void *hInstance, void *function )
{
#if 0
	static qboolean initialized = false;
	if( initialized )
	{
		char message[1024];
		int len = 0;
		size_t i;
		HANDLE process = GetCurrentProcess();
		HANDLE thread = GetCurrentThread();
		IMAGEHLP_LINE64 line;
		DWORD dline = 0;
		DWORD options;
		CONTEXT context;
		STACKFRAME64 stackframe;
		DWORD image;
		char buffer[sizeof( IMAGEHLP_SYMBOL64) + MAX_SYM_NAME * sizeof(TCHAR)];
		PIMAGEHLP_SYMBOL64 symbol = ( PIMAGEHLP_SYMBOL64)buffer;
		memset( symbol, 0, sizeof(IMAGEHLP_SYMBOL64) + MAX_SYM_NAME );
		symbol->SizeOfStruct = sizeof( IMAGEHLP_SYMBOL64);
		symbol->MaxNameLength = MAX_SYM_NAME;
		DWORD displacement = 0;

		options = SymGetOptions();
		SymSetOptions( options );

		SymInitialize( process, NULL, TRUE );

		if( SymGetSymFromAddr64( process, function, &displacement, symbol ) )
		{
			Msg( "%s\n", symbol->Name );
			return copystring( symbol->Name );
		}

	}
#endif

#ifdef XASH_ALLOW_SAVERESTORE_OFFSETS
	return Com_OffsetNameForFunction( function );
#endif

	return NULL;
}

#else
/*
---------------------------------------------------------------

		Custom dlls loader

---------------------------------------------------------------
*/

#ifndef IMAGE_SIZEOF_BASE_RELOCATION
// Vista SDKs no longer define IMAGE_SIZEOF_BASE_RELOCATION!?
#define IMAGE_SIZEOF_BASE_RELOCATION (sizeof(IMAGE_BASE_RELOCATION))
#endif

#if defined(_M_X64)
#error "Xash's nonstandart loader will not work on Win64. Set target to Win32 or disable nonstandart loader"
#endif


typedef struct
{
	PIMAGE_NT_HEADERS	headers;
	byte		*codeBase;
	void		**modules;
	int		numModules;
	int		initialized;
} MEMORYMODULE, *PMEMORYMODULE;

// Protection flags for memory pages (Executable, Readable, Writeable)
static int ProtectionFlags[2][2][2] =
{
{
{ PAGE_NOACCESS, PAGE_WRITECOPY },		// not executable
{ PAGE_READONLY, PAGE_READWRITE },
},
{
{ PAGE_EXECUTE, PAGE_EXECUTE_WRITECOPY },	// executable
{ PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE },
},
};

typedef BOOL (WINAPI *DllEntryProc)( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved );

#define GET_HEADER_DICTIONARY( module, idx )	&(module)->headers->OptionalHeader.DataDirectory[idx]
#define CALCULATE_ADDRESS( base, offset )	(((DWORD)(base)) + (offset))

static void CopySections( const byte *data, PIMAGE_NT_HEADERS old_headers, PMEMORYMODULE module )
{
	int i, size;
	byte *dest;
	byte *codeBase = module->codeBase;
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION( module->headers );

	for( i = 0; i < module->headers->FileHeader.NumberOfSections; i++, section++ )
	{
		if( section->SizeOfRawData == 0 )
		{
			// section doesn't contain data in the dll itself, but may define
			// uninitialized data
			size = old_headers->OptionalHeader.SectionAlignment;

			if( size > 0 )
			{
				dest = (byte *)VirtualAlloc((byte *)CALCULATE_ADDRESS(codeBase, section->VirtualAddress), size, MEM_COMMIT, PAGE_READWRITE );
				section->Misc.PhysicalAddress = (DWORD)dest;
				Q_memset( dest, 0, size );
			}
			// section is empty
			continue;
		}

		// commit memory block and copy data from dll
		dest = (byte *)VirtualAlloc((byte *)CALCULATE_ADDRESS(codeBase, section->VirtualAddress), section->SizeOfRawData, MEM_COMMIT, PAGE_READWRITE );
		Q_memcpy( dest, (byte *)CALCULATE_ADDRESS(data, section->PointerToRawData), section->SizeOfRawData );
		section->Misc.PhysicalAddress = (DWORD)dest;
	}
}

static void FreeSections( PIMAGE_NT_HEADERS old_headers, PMEMORYMODULE module )
{
	int	i, size;
	byte	*codeBase = module->codeBase;
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);

	for( i = 0; i < module->headers->FileHeader.NumberOfSections; i++, section++ )
	{
		if( section->SizeOfRawData == 0 )
		{
			size = old_headers->OptionalHeader.SectionAlignment;
			if( size > 0 )
			{
				VirtualFree( codeBase + section->VirtualAddress, size, MEM_DECOMMIT );
				section->Misc.PhysicalAddress = 0;
			}
			continue;
		}

		VirtualFree( codeBase + section->VirtualAddress, section->SizeOfRawData, MEM_DECOMMIT );
		section->Misc.PhysicalAddress = 0;
	}
}

static void FinalizeSections( MEMORYMODULE *module )
{
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION( module->headers );
	int	i;
	
	// loop through all sections and change access flags
	for( i = 0; i < module->headers->FileHeader.NumberOfSections; i++, section++ )
	{
		DWORD	protect, oldProtect, size;
		int	executable = (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
		int	readable = (section->Characteristics & IMAGE_SCN_MEM_READ) != 0;
		int	writeable = (section->Characteristics & IMAGE_SCN_MEM_WRITE) != 0;

		if( section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE )
		{
			// section is not needed any more and can safely be freed
			VirtualFree((LPVOID)section->Misc.PhysicalAddress, section->SizeOfRawData, MEM_DECOMMIT);
			continue;
		}

		// determine protection flags based on characteristics
		protect = ProtectionFlags[executable][readable][writeable];
		if( section->Characteristics & IMAGE_SCN_MEM_NOT_CACHED )
			protect |= PAGE_NOCACHE;

		// determine size of region
		size = section->SizeOfRawData;

		if( size == 0 )
		{
			if( section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA )
				size = module->headers->OptionalHeader.SizeOfInitializedData;
			else if( section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA )
				size = module->headers->OptionalHeader.SizeOfUninitializedData;
		}

		if( size > 0 )
		{         
			// change memory access flags
			if( !VirtualProtect((LPVOID)section->Misc.PhysicalAddress, size, protect, &oldProtect ))
				Sys_Error( "Com_FinalizeSections: error protecting memory page\n" );
		}
	}
}

static void PerformBaseRelocation( MEMORYMODULE *module, DWORD delta )
{
	DWORD	i;
	byte	*codeBase = module->codeBase;
	PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY( module, IMAGE_DIRECTORY_ENTRY_BASERELOC );

	if( directory->Size > 0 )
	{
		PIMAGE_BASE_RELOCATION relocation = (PIMAGE_BASE_RELOCATION)CALCULATE_ADDRESS( codeBase, directory->VirtualAddress );
		for( ; relocation->VirtualAddress > 0; )
		{
			byte	*dest = (byte *)CALCULATE_ADDRESS( codeBase, relocation->VirtualAddress );
			word	*relInfo = (word *)((byte *)relocation + IMAGE_SIZEOF_BASE_RELOCATION );

			for( i = 0; i<((relocation->SizeOfBlock-IMAGE_SIZEOF_BASE_RELOCATION) / 2); i++, relInfo++ )
			{
				DWORD	*patchAddrHL;
				int	type, offset;

				// the upper 4 bits define the type of relocation
				type = *relInfo >> 12;
				// the lower 12 bits define the offset
				offset = *relInfo & 0xfff;
				
				switch( type )
				{
				case IMAGE_REL_BASED_ABSOLUTE:
					// skip relocation
					break;
				case IMAGE_REL_BASED_HIGHLOW:
					// change complete 32 bit address
					patchAddrHL = (DWORD *)CALCULATE_ADDRESS( dest, offset );
					*patchAddrHL += delta;
					break;
				default:
					MsgDev( D_ERROR, "PerformBaseRelocation: unknown relocation: %d\n", type );
					break;
				}
			}

			// advance to next relocation block
			relocation = (PIMAGE_BASE_RELOCATION)CALCULATE_ADDRESS( relocation, relocation->SizeOfBlock );
		}
	}
}

static FARPROC MemoryGetProcAddress( void *module, const char *name )
{
	int	idx = -1;
	DWORD	i, *nameRef;
	WORD	*ordinal;
	PIMAGE_EXPORT_DIRECTORY exports;
	byte	*codeBase = ((PMEMORYMODULE)module)->codeBase;
	PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY((MEMORYMODULE *)module, IMAGE_DIRECTORY_ENTRY_EXPORT );

	if( directory->Size == 0 )
	{
		// no export table found
		return NULL;
	}

	exports = (PIMAGE_EXPORT_DIRECTORY)CALCULATE_ADDRESS( codeBase, directory->VirtualAddress );

	if( exports->NumberOfNames == 0 || exports->NumberOfFunctions == 0 )
	{
		// DLL doesn't export anything
		return NULL;
	}

	// search function name in list of exported names
	nameRef = (DWORD *)CALCULATE_ADDRESS( codeBase, exports->AddressOfNames );
	ordinal = (WORD *)CALCULATE_ADDRESS( codeBase, exports->AddressOfNameOrdinals );

	for( i = 0; i < exports->NumberOfNames; i++, nameRef++, ordinal++ )
	{
		// GetProcAddress case insensative ?????
		if( !Q_stricmp( name, (const char *)CALCULATE_ADDRESS( codeBase, *nameRef )))
		{
			idx = *ordinal;
			break;
		}
	}

	if( idx == -1 )
	{
		// exported symbol not found
		return NULL;
	}

	if((DWORD)idx > exports->NumberOfFunctions )
	{
		// name <-> ordinal number don't match
		return NULL;
	}

	// addressOfFunctions contains the RVAs to the "real" functions
	return (FARPROC)CALCULATE_ADDRESS( codeBase, *(DWORD *)CALCULATE_ADDRESS( codeBase, exports->AddressOfFunctions + (idx * 4)));
}

static int BuildImportTable( MEMORYMODULE *module )
{
	int	result=1;
	byte	*codeBase = module->codeBase;
	PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY( module, IMAGE_DIRECTORY_ENTRY_IMPORT );

	if( directory->Size > 0 )
	{
		PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)CALCULATE_ADDRESS( codeBase, directory->VirtualAddress );

		for( ; !IsBadReadPtr( importDesc, sizeof( IMAGE_IMPORT_DESCRIPTOR )) && importDesc->Name; importDesc++ )
		{
			DWORD	*thunkRef, *funcRef;
			LPCSTR	libname;
			void	*handle;

			libname = (LPCSTR)CALCULATE_ADDRESS( codeBase, importDesc->Name );
			handle = Com_LoadLibraryExt( libname, false, true );

			if( handle == NULL )
			{
				MsgDev( D_ERROR, "couldn't load library %s\n", libname );
				result = 0;
				break;
			}

			module->modules = (void *)Mem_Realloc( host.mempool, module->modules, (module->numModules + 1) * (sizeof( void* )));
			module->modules[module->numModules++] = handle;

			if( importDesc->OriginalFirstThunk )
			{
				thunkRef = (DWORD *)CALCULATE_ADDRESS( codeBase, importDesc->OriginalFirstThunk );
				funcRef = (DWORD *)CALCULATE_ADDRESS( codeBase, importDesc->FirstThunk );
			}
			else
			{
				// no hint table
				thunkRef = (DWORD *)CALCULATE_ADDRESS( codeBase, importDesc->FirstThunk );
				funcRef = (DWORD *)CALCULATE_ADDRESS( codeBase, importDesc->FirstThunk );
			}

			for( ; *thunkRef; thunkRef++, funcRef++ )
			{
				if( IMAGE_SNAP_BY_ORDINAL( *thunkRef ))
				{
					LPCSTR	funcName = (LPCSTR)IMAGE_ORDINAL( *thunkRef );
					*funcRef = (DWORD)Com_GetProcAddress( handle, funcName );
				}
				else
				{
					PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME)CALCULATE_ADDRESS( codeBase, *thunkRef );
					LPCSTR	funcName = (LPCSTR)&thunkData->Name;
					*funcRef = (DWORD)Com_GetProcAddress( handle, funcName );
				}

				if( *funcRef == 0 )
				{
					result = 0;
					break;
				}
			}
			if( !result ) break;
		}
	}
	return result;
}

static void MemoryFreeLibrary( void *hInstance )
{
	MEMORYMODULE	*module = (MEMORYMODULE *)hInstance;

	if( module != NULL )
	{
		int	i;
	
		if( module->initialized != 0 )
		{
			// notify library about detaching from process
			DllEntryProc DllEntry = (DllEntryProc)CALCULATE_ADDRESS( module->codeBase, module->headers->OptionalHeader.AddressOfEntryPoint );
			(*DllEntry)((HINSTANCE)module->codeBase, DLL_PROCESS_DETACH, 0 );
			module->initialized = 0;
		}

		if( module->modules != NULL )
		{
			// free previously opened libraries
			for( i = 0; i < module->numModules; i++ )
			{
				if( module->modules[i] != NULL )
				{
					Com_FreeLibrary( module->modules[i] );
				}
			}
			Mem_Free( module->modules ); // Mem_Realloc end
		}

		FreeSections( module->headers, module );

		if( module->codeBase != NULL )
		{
			// release memory of library
			VirtualFree( module->codeBase, 0, MEM_RELEASE );
		}

		HeapFree( GetProcessHeap(), 0, module );
	}
}

void *MemoryLoadLibrary( const char *name )
{
	MEMORYMODULE	*result = NULL;
	PIMAGE_DOS_HEADER	dos_header;
	PIMAGE_NT_HEADERS	old_header;
	byte		*code, *headers;
	DWORD		locationDelta;
	DllEntryProc	DllEntry;
	string		errorstring;
	qboolean		successfull;
	void		*data = NULL;

	data = FS_LoadFile( name, NULL, false );

	if( !data )
	{
		Q_sprintf( errorstring, "couldn't load %s", name );
		goto library_error;
	}

	dos_header = (PIMAGE_DOS_HEADER)data;
	if( dos_header->e_magic != IMAGE_DOS_SIGNATURE )
	{
		Q_sprintf( errorstring, "%s it's not a valid executable file", name );
		goto library_error;
	}

	old_header = (PIMAGE_NT_HEADERS)&((const byte *)(data))[dos_header->e_lfanew];
	if( old_header->Signature != IMAGE_NT_SIGNATURE )
	{
		Q_sprintf( errorstring, "%s missing PE header", name );
		goto library_error;
	}

	// reserve memory for image of library
	code = (byte *)VirtualAlloc((LPVOID)(old_header->OptionalHeader.ImageBase), old_header->OptionalHeader.SizeOfImage, MEM_RESERVE, PAGE_READWRITE );

	if( code == NULL )
	{
		// try to allocate memory at arbitrary position
		code = (byte *)VirtualAlloc( NULL, old_header->OptionalHeader.SizeOfImage, MEM_RESERVE, PAGE_READWRITE );
	}    
	if( code == NULL )
	{
		Q_sprintf( errorstring, "%s can't reserve memory", name );
		goto library_error;
	}

	result = (MEMORYMODULE *)HeapAlloc( GetProcessHeap(), 0, sizeof( MEMORYMODULE ));
	result->codeBase = code;
	result->numModules = 0;
	result->modules = NULL;
	result->initialized = 0;

	// XXX: is it correct to commit the complete memory region at once?
	// calling DllEntry raises an exception if we don't...
	VirtualAlloc( code, old_header->OptionalHeader.SizeOfImage, MEM_COMMIT, PAGE_READWRITE );

	// commit memory for headers
	headers = (byte *)VirtualAlloc( code, old_header->OptionalHeader.SizeOfHeaders, MEM_COMMIT, PAGE_READWRITE );
	
	// copy PE header to code
	Q_memcpy( headers, dos_header, dos_header->e_lfanew + old_header->OptionalHeader.SizeOfHeaders );
	result->headers = (PIMAGE_NT_HEADERS)&((const byte *)(headers))[dos_header->e_lfanew];

	// update position
	result->headers->OptionalHeader.ImageBase = (DWORD)code;

	// copy sections from DLL file block to new memory location
	CopySections( data, old_header, result );

	// adjust base address of imported data
	locationDelta = (DWORD)(code - old_header->OptionalHeader.ImageBase);
	if( locationDelta != 0 ) PerformBaseRelocation( result, locationDelta );

	// load required dlls and adjust function table of imports
	if( !BuildImportTable( result ))
	{
		Q_sprintf( errorstring, "%s failed to build import table", name );
		goto library_error;
	}

	// mark memory pages depending on section headers and release
	// sections that are marked as "discardable"
	FinalizeSections( result );

	// get entry point of loaded library
	if( result->headers->OptionalHeader.AddressOfEntryPoint != 0 )
	{
		DllEntry = (DllEntryProc)CALCULATE_ADDRESS( code, result->headers->OptionalHeader.AddressOfEntryPoint );
		if( DllEntry == 0 )
		{
			Q_sprintf( errorstring, "%s has no entry point", name );
			goto library_error;
		}

		// notify library about attaching to process
		successfull = (*DllEntry)((HINSTANCE)code, DLL_PROCESS_ATTACH, 0 );
		if( !successfull )
		{
			Q_sprintf( errorstring, "can't attach library %s", name );
			goto library_error;
		}
		result->initialized = 1;
	}

	Mem_Free( data ); // release memory
	return (void *)result;
library_error:
	// cleanup
	if( data ) Mem_Free( data );
	MemoryFreeLibrary( result );
	Com_PushLibraryError( errorstring );
	MsgDev( D_ERROR, "LoadLibrary: %s\n", errorstring );

	return NULL;
}

/*
---------------------------------------------------------------

		Name for function stuff

---------------------------------------------------------------
*/
static void FsGetString( file_t *f, char *str )
{
	char	ch;

	while(( ch = FS_Getc( f )) != EOF )
	{
		*str++ = ch;
		if( !ch ) break;
	}
}

static void FreeNameFuncGlobals( dll_user_t *hInst )
{
	int	i;

	if( !hInst ) return;

	if( hInst->ordinals ) Mem_Free( hInst->ordinals );
	if( hInst->funcs ) Mem_Free( hInst->funcs );

	for( i = 0; i < hInst->num_ordinals; i++ )
	{
		if( hInst->names[i] )
			Mem_Free( hInst->names[i] );
	}

	hInst->num_ordinals = 0;
	hInst->ordinals = NULL;
	hInst->funcs = NULL;
}

char *GetMSVCName( const char *in_name )
{
	char	*pos, *out_name;

	if( in_name[0] == '?' )  // is this a MSVC C++ mangled name?
	{
		if(( pos = Q_strstr( in_name, "@@" )) != NULL )
		{
			int	len = pos - in_name;

			// strip off the leading '?'
			out_name = copystring( in_name + 1 );
			out_name[len-1] = 0; // terminate string at the "@@"
			return out_name;
		}
	}
	return copystring( in_name );
}

qboolean LibraryLoadSymbols( dll_user_t *hInst )
{
	file_t		*f;
	string		errorstring;
	DOS_HEADER	dos_header;
	LONG		nt_signature;
	PE_HEADER		pe_header;
	SECTION_HEADER	section_header;
	qboolean		rdata_found;
	OPTIONAL_HEADER	optional_header;
	long		rdata_delta = 0;
	EXPORT_DIRECTORY	export_directory;
	long		name_offset;
	long		exports_offset;
	long		ordinal_offset;
	long		function_offset;
	string		function_name;
	dword		*p_Names = NULL;
	int		i, index;

	// can only be done for loaded libraries
	if( !hInst ) return false;

	for( i = 0; i < hInst->num_ordinals; i++ )
		hInst->names[i] = NULL;

	f = FS_Open( hInst->shortPath, "rb", false );
	if( !f )
	{
		Q_sprintf( errorstring, "couldn't load %s", hInst->shortPath );
		goto table_error;
	}

	if( FS_Read( f, &dos_header, sizeof( dos_header )) != sizeof( dos_header ))
	{
		Q_sprintf( errorstring, "%s has corrupted EXE header", hInst->shortPath );
		goto table_error;
	}

	if( dos_header.e_magic != DOS_SIGNATURE )
	{
		Q_sprintf( errorstring, "%s does not have a valid dll signature", hInst->shortPath );
		goto table_error;
	}

	if( FS_Seek( f, dos_header.e_lfanew, SEEK_SET ) == -1 )
	{
		Q_sprintf( errorstring, "%s error seeking for new exe header", hInst->shortPath );
		goto table_error;
	}

	if( FS_Read( f, &nt_signature, sizeof( nt_signature )) != sizeof( nt_signature ))
	{
		Q_sprintf( errorstring, "%s has corrupted NT header", hInst->shortPath );
		goto table_error;
	}

	if( nt_signature != NT_SIGNATURE )
	{
		Q_sprintf( errorstring, "%s does not have a valid NT signature", hInst->shortPath );
		goto table_error;
	}

	if( FS_Read( f, &pe_header, sizeof( pe_header )) != sizeof( pe_header ))
	{
		Q_sprintf( errorstring, "%s does not have a valid PE header", hInst->shortPath );
		goto table_error;
	}

	if( !pe_header.SizeOfOptionalHeader )
	{
		Q_sprintf( errorstring, "%s does not have an optional header", hInst->shortPath );
		goto table_error;
	}

	if( FS_Read( f, &optional_header, sizeof( optional_header )) != sizeof( optional_header ))
	{
		Q_sprintf( errorstring, "%s optional header probably corrupted", hInst->shortPath );
		goto table_error;
	}

	rdata_found = false;

	for( i = 0; i < pe_header.NumberOfSections; i++ )
	{
		if( FS_Read( f, &section_header, sizeof( section_header )) != sizeof( section_header ))
		{
			Q_sprintf( errorstring, "%s error during reading section header", hInst->shortPath );
			goto table_error;
		}

		if((( optional_header.DataDirectory[0].VirtualAddress >= section_header.VirtualAddress ) && 
			(optional_header.DataDirectory[0].VirtualAddress < (section_header.VirtualAddress + section_header.Misc.VirtualSize))))
		{
			rdata_found = true;
			break;
		}
	}

	if( rdata_found )
	{
		rdata_delta = section_header.VirtualAddress - section_header.PointerToRawData; 
	}

	exports_offset = optional_header.DataDirectory[0].VirtualAddress - rdata_delta;

	if( FS_Seek( f, exports_offset, SEEK_SET ) == -1 )
	{
		Q_sprintf( errorstring, "%s does not have a valid exports section", hInst->shortPath );
		goto table_error;
	}

	if( FS_Read( f, &export_directory, sizeof( export_directory )) != sizeof( export_directory ))
	{
		Q_sprintf( errorstring, "%s does not have a valid optional header", hInst->shortPath );
		goto table_error;
	}

	hInst->num_ordinals = export_directory.NumberOfNames;	// also number of ordinals

	if( hInst->num_ordinals > MAX_LIBRARY_EXPORTS )
	{
		Q_sprintf( errorstring, "%s too many exports %i", hInst->shortPath, hInst->num_ordinals );
		hInst->num_ordinals = 0;
		goto table_error;
	}

	ordinal_offset = export_directory.AddressOfNameOrdinals - rdata_delta;

	if( FS_Seek( f, ordinal_offset, SEEK_SET ) == -1 )
	{
		Q_sprintf( errorstring, "%s does not have a valid ordinals section", hInst->shortPath );
		goto table_error;
	}

	hInst->ordinals = Mem_Alloc( host.mempool, hInst->num_ordinals * sizeof( word ));

	if( FS_Read( f, hInst->ordinals, hInst->num_ordinals * sizeof( word )) != (hInst->num_ordinals * sizeof( word )))
	{
		Q_sprintf( errorstring, "%s error during reading ordinals table", hInst->shortPath );
		goto table_error;
	}

	function_offset = export_directory.AddressOfFunctions - rdata_delta;

	if( FS_Seek( f, function_offset, SEEK_SET ) == -1 )
	{
		Q_sprintf( errorstring, "%s does not have a valid export address section", hInst->shortPath );
		goto table_error;
	}

	hInst->funcs = Mem_Alloc( host.mempool, hInst->num_ordinals * sizeof( dword ));

	if( FS_Read( f, hInst->funcs, hInst->num_ordinals * sizeof( dword )) != (hInst->num_ordinals * sizeof( dword )))
	{
		Q_sprintf( errorstring, "%s error during reading export address section", hInst->shortPath );
		goto table_error;
	}

	name_offset = export_directory.AddressOfNames - rdata_delta;

	if( FS_Seek( f, name_offset, SEEK_SET ) == -1 )
	{
		Q_sprintf( errorstring, "%s file does not have a valid names section", hInst->shortPath );
		goto table_error;
	}

	p_Names = Mem_Alloc( host.mempool, hInst->num_ordinals * sizeof( dword ));

	if( FS_Read( f, p_Names, hInst->num_ordinals * sizeof( dword )) != (hInst->num_ordinals * sizeof( dword )))
	{
		Q_sprintf( errorstring, "%s error during reading names table", hInst->shortPath );
		goto table_error;
	}

	for( i = 0; i < hInst->num_ordinals; i++ )
	{
		name_offset = p_Names[i] - rdata_delta;

		if( name_offset != 0 )
		{
			if( FS_Seek( f, name_offset, SEEK_SET ) != -1 )
			{
				FsGetString( f, function_name );
				hInst->names[i] = GetMSVCName( function_name );
			}
			else break;
		}
	}

	if( i != hInst->num_ordinals )
	{
		Q_sprintf( errorstring, "%s error during loading names section", hInst->shortPath );
		goto table_error;
	}
	FS_Close( f );

	for( i = 0; i < hInst->num_ordinals; i++ )
	{
		if( !Q_strcmp( "GiveFnptrsToDll", hInst->names[i] ))	// main entry point for user dlls
		{
			void	*fn_offset;

			index = hInst->ordinals[i];
			fn_offset = (void *)Com_GetProcAddress( hInst, "GiveFnptrsToDll" );
			hInst->funcBase = (dword)(fn_offset) - hInst->funcs[index];
			break;
		}
	}

	if( p_Names ) Mem_Free( p_Names );
	return true;
table_error:
	// cleanup
	if( f ) FS_Close( f );
	if( p_Names ) Mem_Free( p_Names );
	FreeNameFuncGlobals( hInst );
	Com_PushLibraryError( errorstring );
	MsgDev( D_ERROR, "LoadLibrary: %s\n", errorstring );

	return false;
}

/*
================
Com_LoadLibrary

smart dll loader - can loading dlls from pack or wad files
================
*/
void *Com_LoadLibraryExt( const char *dllname, int build_ordinals_table, qboolean directpath )
{
	dll_user_t *hInst;

	hInst = FS_FindLibrary( dllname, directpath );
	if( !hInst )
	{
		char errorstring[256];
		Q_snprintf( errorstring, 256, "LoadLibraryExt: could not find %s!", dllname );
		Com_PushLibraryError(errorstring);
		return NULL; // nothing to load
	}
		
	if( hInst->custom_loader )
	{
		if( hInst->encrypted )
		{
			char errorstring[256];
			Q_snprintf( errorstring, 256, "couldn't load encrypted library %s", dllname );
			Com_PushLibraryError(errorstring);
			MsgDev( D_ERROR, "Sys_LoadLibrary: couldn't load encrypted library %s\n", dllname );
			return NULL;
		}

		hInst->hInstance = MemoryLoadLibrary( hInst->fullPath );
	}
	else hInst->hInstance = LoadLibrary( hInst->fullPath );

	if( !hInst->hInstance )
	{
		string errorstring;
		Q_snprintf( errorstring, MAX_STRING, "LoadLibrary failed for %s:%d", dllname, GetLastError() );
		Com_PushLibraryError( errorstring );
		Com_FreeLibrary( hInst );
		return NULL;
	}

	// if not set - FunctionFromName and NameForFunction will not working
	if( build_ordinals_table )
	{
		if( !LibraryLoadSymbols( hInst ))
		{
			string errorstring;
			Q_snprintf( errorstring, MAX_STRING, "Failed to build ordinals table for %s", dllname );
			Com_PushLibraryError( errorstring );
			//MsgDev( D_NOTE, "Sys_LoadLibrary: Loading %s - failed\n", dllname );

			Com_FreeLibrary( hInst );
			return NULL;
		}
	}

	MsgDev( D_NOTE, "Sys_LoadLibrary: Loading %s - ok\n", dllname );

	return hInst;
}

void *Com_LoadLibrary( const char *dllname, int build_ordinals_table )
{
	return Com_LoadLibraryExt( dllname, build_ordinals_table, false );
}

void *Com_GetProcAddress( void *hInstance, const char *name )
{
	dll_user_t *hInst = (dll_user_t *)hInstance;

	if( !hInst || !hInst->hInstance )
		return NULL;

	if( hInst->custom_loader )
		return (void *)MemoryGetProcAddress( hInst->hInstance, name );
	return (void *)GetProcAddress( hInst->hInstance, name );
}

void Com_FreeLibrary( void *hInstance )
{
	dll_user_t *hInst = (dll_user_t *)hInstance;

	if( !hInst || !hInst->hInstance )
		return; // already freed

	if( host.state == HOST_CRASHED )
	{
		// we need to hold down all modules, while MSVC can find error
		MsgDev( D_NOTE, "Sys_FreeLibrary: hold %s for debugging\n", hInst->dllName );
		return;
	}
	else MsgDev( D_NOTE, "Sys_FreeLibrary: Unloading %s\n", hInst->dllName );
	
	if( hInst->custom_loader )
		MemoryFreeLibrary( hInst->hInstance );
	else FreeLibrary( hInst->hInstance );

	hInst->hInstance = NULL;

	if( hInst->num_ordinals )
		FreeNameFuncGlobals( hInst );
	Mem_Free( hInst );	// done
}

void *Com_FunctionFromName(void *hInstance, const char *pName)
{
	dll_user_t	*hInst = (dll_user_t *)hInstance;
	int		i, index;

	if( !hInst || !hInst->hInstance )
		return 0;

	for( i = 0; i < hInst->num_ordinals; i++ )
	{
		if( !Q_strcmp( pName, hInst->names[i] ))
		{
			index = hInst->ordinals[i];
			return (void*)(hInst->funcs[index] + hInst->funcBase);
		}
	}
	// couldn't find the function name to return address
	return 0;
}

const char *Com_NameForFunction( void *hInstance, void * function )
{
	dll_user_t	*hInst = (dll_user_t *)hInstance;
	int		i, index;

	if( !hInst || !hInst->hInstance )
		return NULL;

	for( i = 0; i < hInst->num_ordinals; i++ )
	{
		index = hInst->ordinals[i];

		// 32 bit only cast :(
		if( ( (dword)function - hInst->funcBase ) == hInst->funcs[index] )
			return hInst->names[i];
	}
	// couldn't find the function address to return name
	return NULL;
}
#endif
