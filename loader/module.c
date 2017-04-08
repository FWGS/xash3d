/*
 * Modules
 *
 * Copyright 1995 Alexandre Julliard
 *
 * Got from mplayer:
 * http://svn.mplayerhq.hu/mplayer/trunk/
 * 
 * Load win32 game libraries
 *
 */

#include "config.h"
#include "debug.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <inttypes.h>

#include "wine/windef.h"
#include "wine/winerror.h"
#include "wine/heap.h"
#include "wine/module.h"
#include "wine/pe_image.h"
#include "wine/debugtools.h"

#undef HAVE_LIBDL

#ifdef HAVE_LIBDL
#include <dlfcn.h>
#include "wine/elfdll.h"
#endif
#include "win32.h"

#ifdef EMU_QTX_API
#include "wrapper.h"
static int report_func(void *stack_base, int stack_size, reg386_t *reg, uint32_t *flags);
static int report_func_ret(void *stack_base, int stack_size, reg386_t *reg, uint32_t *flags);
#endif

//#undef TRACE
//#define TRACE printf

//WINE_MODREF *local_wm=NULL;
modref_list* local_wm=NULL;

HANDLE SegptrHeap;

WINE_MODREF* MODULE_FindModule(LPCSTR m)
{
    modref_list* list=local_wm;
    TRACE("FindModule: Module %s request\n", m);
    if(list==NULL)
	return NULL;
//    while(strcmp(m, list->wm->filename))
    while(!strstr(list->wm->filename, m))
    {
	TRACE("%s: %x\n", list->wm->filename, list->wm->module);
	list=list->prev;
	if(list==NULL)
	    return NULL;
    }
    TRACE("Resolved to %s\n", list->wm->filename);
    return list->wm;
}
WINE_MODREF* MODULE_FindNearFunctionName(FARPROC f)
{
    modref_list* list=local_wm;
    //TRACE("FindModule: Module %s request\n", m);
    if(list==NULL)
		return NULL;
	const char *s = NULL;
//    while(strcmp(m, list->wm->filename))
    while(list && !s)
    {
		s = PE_FindNearFunctionName(list->wm, f);
		list = list->prev;
	}
	return (WINE_MODREF*)s;
}


static void MODULE_RemoveFromList(WINE_MODREF *mod)
{
    modref_list* list=local_wm;
    if(list==0)
	return;
    if(mod==0)
	return;
    if((list->prev==NULL)&&(list->next==NULL))
    {
	free(list);
	local_wm=NULL;
//	uninstall_fs();
	return;
    }
    for(;list;list=list->prev)
    {
	if(list->wm==mod)
	{
	    if(list->prev)
		list->prev->next=list->next;
	    if(list->next)
		list->next->prev=list->prev;
	    if(list==local_wm)
		local_wm=list->prev;
	    free(list);
	    return;
	}
    }

}

WINE_MODREF *MODULE32_LookupHMODULE(HMODULE m)
{
    modref_list* list=local_wm;
    TRACE("LookupHMODULE: Module %X request\n", m);
    if(list==NULL)
    {
	TRACE("LookupHMODULE failed\n");
	return NULL;
    }
    while(m!=list->wm->module)
    {
//      printf("Checking list %X wm %X module %X\n",
//	list, list->wm, list->wm->module);
	list=list->prev;
	if(list==NULL)
	{
	    TRACE("LookupHMODULE failed\n");
	    return NULL;
	}
    }
    TRACE("LookupHMODULE hit %p\n", list->wm);
    return list->wm;
}

/*************************************************************************
 *		MODULE_InitDll
 */
static WIN_BOOL MODULE_InitDll( WINE_MODREF *wm, DWORD type, LPVOID lpReserved )
{
    WIN_BOOL retv = TRUE;

    #ifdef DEBUG
    static LPCSTR typeName[] = { "PROCESS_DETACH", "PROCESS_ATTACH",
                                 "THREAD_ATTACH", "THREAD_DETACH" };
    #endif

    assert( wm );


    /* Skip calls for modules loaded with special load flags */

    if (    ( wm->flags & WINE_MODREF_DONT_RESOLVE_REFS )
         || ( wm->flags & WINE_MODREF_LOAD_AS_DATAFILE ) )
        return TRUE;


    TRACE("(%s,%s,%p) - CALL\n", wm->modname, typeName[type], lpReserved );

    /* Call the initialization routine */
    switch ( wm->type )
    {
    case MODULE32_PE:
        retv = PE_InitDLL( wm, type, lpReserved );
        break;

    case MODULE32_ELF:
        /* no need to do that, dlopen() already does */
        break;

    default:
        ERR("wine_modref type %d not handled.\n", wm->type );
        retv = FALSE;
        break;
    }

    /* The state of the module list may have changed due to the call
       to PE_InitDLL. We cannot assume that this module has not been
       deleted.  */
    TRACE("(%p,%s,%p) - RETURN %d\n", wm, typeName[type], lpReserved, retv );

    return retv;
}

/*************************************************************************
 *		MODULE_DllProcessAttach
 *
 * Send the process attach notification to all DLLs the given module
 * depends on (recursively). This is somewhat complicated due to the fact that
 *
 * - we have to respect the module dependencies, i.e. modules implicitly
 *   referenced by another module have to be initialized before the module
 *   itself can be initialized
 *
 * - the initialization routine of a DLL can itself call LoadLibrary,
 *   thereby introducing a whole new set of dependencies (even involving
 *   the 'old' modules) at any time during the whole process
 *
 * (Note that this routine can be recursively entered not only directly
 *  from itself, but also via LoadLibrary from one of the called initialization
 *  routines.)
 *
 * Furthermore, we need to rearrange the main WINE_MODREF list to allow
 * the process *detach* notifications to be sent in the correct order.
 * This must not only take into account module dependencies, but also
 * 'hidden' dependencies created by modules calling LoadLibrary in their
 * attach notification routine.
 *
 * The strategy is rather simple: we move a WINE_MODREF to the head of the
 * list after the attach notification has returned.  This implies that the
 * detach notifications are called in the reverse of the sequence the attach
 * notifications *returned*.
 *
 * NOTE: Assumes that the process critical section is held!
 *
 */
static WIN_BOOL MODULE_DllProcessAttach( WINE_MODREF *wm, LPVOID lpReserved )
{
    WIN_BOOL retv = TRUE;
    //int i;
    assert( wm );

    /* prevent infinite recursion in case of cyclical dependencies */
    if (    ( wm->flags & WINE_MODREF_MARKER )
         || ( wm->flags & WINE_MODREF_PROCESS_ATTACHED ) )
        return retv;

    TRACE("(%s,%p) - START\n", wm->modname, lpReserved );

    /* Tag current MODREF to prevent recursive loop */
    wm->flags |= WINE_MODREF_MARKER;

    /* Recursively attach all DLLs this one depends on */
/*    for ( i = 0; retv && i < wm->nDeps; i++ )
        if ( wm->deps[i] )
            retv = MODULE_DllProcessAttach( wm->deps[i], lpReserved );
*/
    /* Call DLL entry point */

    //local_wm=wm;
    if(local_wm)
    {
        local_wm->next = malloc(sizeof(modref_list));
        local_wm->next->prev=local_wm;
        local_wm->next->next=NULL;
        local_wm->next->wm=wm;
        local_wm=local_wm->next;
    }
    else
    {
	local_wm = malloc(sizeof(modref_list));
	local_wm->next=local_wm->prev=NULL;
	local_wm->wm=wm;
    }
    /* Remove recursion flag */
    wm->flags &= ~WINE_MODREF_MARKER;

    if ( retv )
    {
        retv = MODULE_InitDll( wm, DLL_PROCESS_ATTACH, lpReserved );
        if ( retv )
            wm->flags |= WINE_MODREF_PROCESS_ATTACHED;
    }


    TRACE("(%s,%p) - END\n", wm->modname, lpReserved );

    return retv;
}

/*************************************************************************
 *		MODULE_DllProcessDetach
 *
 * Send DLL process detach notifications.  See the comment about calling
 * sequence at MODULE_DllProcessAttach.  Unless the bForceDetach flag
 * is set, only DLLs with zero refcount are notified.
 */
static void MODULE_DllProcessDetach( WINE_MODREF* wm, WIN_BOOL bForceDetach, LPVOID lpReserved )
{
    //    WINE_MODREF *wm=local_wm;
    //modref_list* l = local_wm;
    wm->flags &= ~WINE_MODREF_PROCESS_ATTACHED;
    MODULE_InitDll( wm, DLL_PROCESS_DETACH, lpReserved );
/*    while (l)
    {
	modref_list* f = l;
	l = l->next;
	free(f);
    }
    local_wm = 0;*/
}

/***********************************************************************
 *	MODULE_LoadLibraryExA	(internal)
 *
 * Load a PE style module according to the load order.
 *
 * The HFILE parameter is not used and marked reserved in the SDK. I can
 * only guess that it should force a file to be mapped, but I rather
 * ignore the parameter because it would be extremely difficult to
 * integrate this with different types of module represenations.
 *
 */
static WINE_MODREF *MODULE_LoadLibraryExA( LPCSTR libname, HFILE hfile, DWORD flags )
{
	DWORD err = GetLastError();
	WINE_MODREF *pwm;
//	module_loadorder_t *plo;

        SetLastError( ERROR_FILE_NOT_FOUND );
	TRACE("Trying native dll '%s'\n", libname);
	pwm = PE_LoadLibraryExA(libname, flags);
#ifdef HAVE_LIBDL
	if(!pwm)
	{
    	    TRACE("Trying ELF dll '%s'\n", libname);
	    pwm=(WINE_MODREF*)ELFDLL_LoadLibraryExA(libname, flags);
	}
#endif
//		printf("0x%08x\n", pwm);
//		break;
	if(pwm)
	{
		/* Initialize DLL just loaded */
		TRACE("Loaded module '%s' at 0x%08x, \n", libname, pwm->module);
		/* Set the refCount here so that an attach failure will */
		/* decrement the dependencies through the MODULE_FreeLibrary call. */
		pwm->refCount++;

                SetLastError( err );  /* restore last error */
		return pwm;
	}


	WARN("Failed to load module '%s'; error=0x%08lx, \n", libname, GetLastError());
	return NULL;
}

/***********************************************************************
 *           MODULE_FreeLibrary
 *
 * NOTE: Assumes that the process critical section is held!
 */
static WIN_BOOL MODULE_FreeLibrary( WINE_MODREF *wm )
{
    TRACE("(%s) - START\n", wm->modname );

    /* Call process detach notifications */
    MODULE_DllProcessDetach( wm, FALSE, NULL );

    PE_UnloadLibrary(wm);

    TRACE("END\n");

    return TRUE;
}

/***********************************************************************
 *           LoadLibraryExA   (KERNEL32)
 */
HMODULE WINAPI LoadLibraryExA(LPCSTR libname, HANDLE hfile, DWORD flags)
{
	WINE_MODREF *wm = 0;
	char* listpath[] = { "", "", 0 };
	char path[512];
	char checked[2000];
        int i = -1;

        checked[0] = 0;
	if(!libname)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	wm=MODULE_FindModule(libname);
	if(wm) return wm->module;

//	if(fs_installed==0)
//	    install_fs();

	// Do not load libraries from a path relative to the current directory
	if (*libname != '/')
	    i++;

	while (wm == 0 && listpath[++i])
	{
	    if (i < 2)
	    {
		if (i == 0)
		    /* check just original file name */
		    strncpy(path, libname, 511);
                else
		    /* check default user path */
		    strncpy(path, "./", 300);
	    }
	    else if (strcmp("./", listpath[i]))
                /* path from the list */
		strncpy(path, listpath[i], 300);
	    else
		continue;

	    if (i > 0)
	    {
		strcat(path, "/");
		strncat(path, libname, 100);
	    }
	    path[511] = 0;
	    wm = MODULE_LoadLibraryExA( path, hfile, flags );

	    if (!wm)
	    {
		if (checked[0])
		    strcat(checked, ", ");
		strcat(checked, path);
                checked[1500] = 0;

	    }
	}
	if ( wm )
	{
		if ( !MODULE_DllProcessAttach( wm, NULL ) )
		{
			WARN_(module)("Attach failed for module '%s', \n", libname);
			MODULE_FreeLibrary(wm);
			SetLastError(ERROR_DLL_INIT_FAILED);
			MODULE_RemoveFromList(wm);
			wm = NULL;
		}
	}
	#if 0
	if (!wm && !strstr(checked, "avisynth.dll"))
	    printf("Win32 LoadLibrary failed to load: %s\n", checked);

#define RVA(x) ((char *)wm->module+(unsigned int)(x))
	if (strstr(libname, "CFDecode2.ax") && wm)
	{
	    if (PE_FindExportedFunction(wm, "DllGetClassObject", TRUE) == RVA(0xd00e0))
	    {
	        // Patch some movdqa to movdqu
	        // It is currently unclear why this is necessary, it seems
	        // to be some output frame, but our frame seems correctly
	        // aligned
	        int offsets[] = {0x7318c, 0x731ba, 0x731e0, 0x731fe, 0};
	        int i;
	        for (i = 0; offsets[i]; i++)
	        {
	            int ofs = offsets[i];
	            if (RVA(ofs)[0] == 0x66 && RVA(ofs)[1] == 0x0f &&
	                RVA(ofs)[2] == 0x7f)
	                RVA(ofs)[0] = 0xf3;
	        }
	    }
	}
	if (strstr(libname,"vp31vfw.dll") && wm)
	{
	    int i;

	  // sse hack moved from patch dll into runtime patching
          if (PE_FindExportedFunction(wm, "DriverProc", TRUE)==RVA(0x1000)) {
	    fprintf(stderr, "VP3 DLL found\n");
	    for (i=0;i<18;i++) RVA(0x4bd6)[i]=0x90;
	  }
	}

        // remove a few divs in the VP codecs that make trouble
        if (strstr(libname,"vp5vfw.dll") && wm)
        {
          int i;
          if (PE_FindExportedFunction(wm, "DriverProc", TRUE)==RVA(0x3930)) {
            for (i=0;i<3;i++) RVA(0x4e86)[i]=0x90;
            for (i=0;i<3;i++) RVA(0x5a23)[i]=0x90;
            for (i=0;i<3;i++) RVA(0x5bff)[i]=0x90;
          } else {
            fprintf(stderr, "Unsupported VP5 version\n");
            return 0;
          }
        }

        if (strstr(libname,"vp6vfw.dll") && wm)
        {
          int i;
          if (PE_FindExportedFunction(wm, "DriverProc", TRUE)==RVA(0x3ef0)) {
            // looks like VP 6.1.0.2
            for (i=0;i<6;i++) RVA(0x7268)[i]=0x90;
            for (i=0;i<6;i++) RVA(0x7e83)[i]=0x90;
            for (i=0;i<6;i++) RVA(0x806a)[i]=0x90;
          } else if (PE_FindExportedFunction(wm, "DriverProc", TRUE)==RVA(0x4120)) {
            // looks like VP 6.2.0.10
            for (i=0;i<6;i++) RVA(0x7688)[i]=0x90;
            for (i=0;i<6;i++) RVA(0x82c3)[i]=0x90;
            for (i=0;i<6;i++) RVA(0x84aa)[i]=0x90;
            for (i=0;i<6;i++) RVA(0x1d2cc)[i]=0x90;
            for (i=0;i<6;i++) RVA(0x2179d)[i]=0x90;
            for (i=0;i<6;i++) RVA(0x1977f)[i]=0x90;
          } else if (PE_FindExportedFunction(wm, "DriverProc", TRUE)==RVA(0x3e70)) {
            // looks like VP 6.0.7.3
            for (i=0;i<6;i++) RVA(0x7559)[i]=0x90;
            for (i=0;i<6;i++) RVA(0x81c3)[i]=0x90;
            for (i=0;i<6;i++) RVA(0x839e)[i]=0x90;
          } else {
            fprintf(stderr, "Unsupported VP6 version\n");
            return 0;
          }
        }

        // Windows Media Video 9 Advanced
        if (strstr(libname,"wmvadvd.dll") && wm)
        {
          // The codec calls IsRectEmpty with coords 0,0,0,0 => result is 0
          // but it really wants the rectangle to be not empty
          if (PE_FindExportedFunction(wm, "CreateInstance", TRUE)==RVA(0xb812)) {
            // Dll version is 10.0.0.3645
            *RVA(0x8b0f)=0xeb; // Jump always, ignoring IsRectEmpty result
          } else {
            fprintf(stderr, "Unsupported WMVA version\n");
            return 0;
          }
        }

	if (strstr(libname,"QuickTime.qts") && wm)
	{
	    void** ptr;
	    void *dispatch_addr;
	    int i;

//	    dispatch_addr = GetProcAddress(wm->module, "theQuickTimeDispatcher", TRUE);
	    dispatch_addr = PE_FindExportedFunction(wm, "theQuickTimeDispatcher", TRUE);
	    if (dispatch_addr == RVA(0x124c30))
	    {
	        fprintf(stderr, "QuickTime5 DLLs found\n");
		ptr = (void **)RVA(0x375ca4); // dispatch_ptr
	        for (i=0;i<5;i++)  RVA(0x19e842)[i]=0x90; // make_new_region ?
	        for (i=0;i<28;i++) RVA(0x19e86d)[i]=0x90; // call__call_CreateCompatibleDC ?
		for (i=0;i<5;i++)  RVA(0x19e898)[i]=0x90; // jmp_to_call_loadbitmap ?
	        for (i=0;i<9;i++)  RVA(0x19e8ac)[i]=0x90; // call__calls_OLE_shit ?
	        for (i=0;i<106;i++) RVA(0x261b10)[i]=0x90; // disable threads
#if 0
		/* CreateThread callers */
		for (i=0;i<5;i++) RVA(0x1487c5)[i]=0x90;
		for (i=0;i<5;i++) RVA(0x14b275)[i]=0x90;
		for (i=0;i<5;i++) RVA(0x1a24b1)[i]=0x90;
		for (i=0;i<5;i++) RVA(0x1afc5a)[i]=0x90;
		for (i=0;i<5;i++) RVA(0x2f799c)[i]=0x90;
		for (i=0;i<5;i++) RVA(0x2f7efe)[i]=0x90;
		for (i=0;i<5;i++) RVA(0x2fa33e)[i]=0x90;
#endif

#if 0
		/* TerminateQTML fix */
		for (i=0;i<47;i++) RVA(0x2fa3b8)[i]=0x90; // terminate thread
		for (i=0;i<47;i++) RVA(0x2f7f78)[i]=0x90; // terminate thread
		for (i=0;i<77;i++) RVA(0x1a13d5)[i]=0x90;
		RVA(0x08e0ae)[0] = 0xc3; // font/dc remover
		for (i=0;i<24;i++) RVA(0x07a1ad)[i]=0x90; // destroy window
#endif
	    } else if (dispatch_addr == RVA(0x13b330))
	    {
    		fprintf(stderr, "QuickTime6 DLLs found\n");
		ptr = (void **)RVA(0x3b9524); // dispatcher_ptr
		for (i=0;i<5;i++)  RVA(0x2730cc)[i]=0x90; // make_new_region
		for (i=0;i<28;i++) RVA(0x2730f7)[i]=0x90; // call__call_CreateCompatibleDC
		for (i=0;i<5;i++)  RVA(0x273122)[i]=0x90; // jmp_to_call_loadbitmap
		for (i=0;i<9;i++)  RVA(0x273131)[i]=0x90; // call__calls_OLE_shit
		for (i=0;i<96;i++) RVA(0x2ac852)[i]=0x90; // disable threads
	    } else if (dispatch_addr == RVA(0x13c3e0))
	    {
    		fprintf(stderr, "QuickTime6.3 DLLs found\n");
		ptr = (void **)RVA(0x3ca01c); // dispatcher_ptr
		for (i=0;i<5;i++)  RVA(0x268f6c)[i]=0x90; // make_new_region
		for (i=0;i<28;i++) RVA(0x268f97)[i]=0x90; // call__call_CreateCompatibleDC
		for (i=0;i<5;i++)  RVA(0x268fc2)[i]=0x90; // jmp_to_call_loadbitmap
		for (i=0;i<9;i++)  RVA(0x268fd1)[i]=0x90; // call__calls_OLE_shit
		for (i=0;i<96;i++) RVA(0x2b4722)[i]=0x90; // disable threads
	    } else
	    {
	        fprintf(stderr, "Unsupported QuickTime version (%p)\n",
		    dispatch_addr);
		return 0;
	    }

	    fprintf(stderr,"QuickTime.qts patched!!! old entry=%p\n",ptr[0]);

	}
#undef RVA
	#endif
	return wm ? wm->module : 0;
}


/***********************************************************************
 *           LoadLibraryA         (KERNEL32)
 */
HMODULE WINAPI LoadLibraryA(LPCSTR libname) {
	return LoadLibraryExA(libname,0,0);
}

/***********************************************************************
 *           FreeLibrary
 */
WIN_BOOL WINAPI FreeLibrary(HINSTANCE hLibModule)
{
    WIN_BOOL retv = FALSE;
    WINE_MODREF *wm;

    wm=MODULE32_LookupHMODULE(hLibModule);

    if ( !wm || !hLibModule )
    {
        SetLastError( ERROR_INVALID_HANDLE );
	return 0;
    }
    else
        retv = MODULE_FreeLibrary( wm );

    MODULE_RemoveFromList(wm);

    /* garbage... */
    if (local_wm == NULL) my_garbagecollection();

    return retv;
}

/***********************************************************************
 *           GetProcAddress   		(KERNEL32.257)
 */
FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR function )
{
    return MODULE_GetProcAddress( hModule, function, TRUE );
}

/***********************************************************************
 *           MODULE_GetProcAddress   		(internal)
 */
FARPROC MODULE_GetProcAddress(
	HMODULE hModule, 	/* [in] current module handle */
	LPCSTR function,	/* [in] function to be looked up */
	WIN_BOOL snoop )
{
    WINE_MODREF	*wm = MODULE32_LookupHMODULE( hModule );
//    WINE_MODREF *wm=local_wm;
    FARPROC	retproc;
#ifdef DEBUG
    if (HIWORD(function))
	fprintf(stderr,"XXX GetProcAddress(%08lx,%s)\n",(DWORD)hModule,function);
    else
	fprintf(stderr,"XXX GetProcAddress(%08lx,%p)\n",(DWORD)hModule,function);
#endif
//	TRACE_(win32)("(%08lx,%s)\n",(DWORD)hModule,function);
//    else
//	TRACE_(win32)("(%08lx,%p)\n",(DWORD)hModule,function);

    if (!wm) {
    	SetLastError(ERROR_INVALID_HANDLE);
        return (FARPROC)0;
    }
    switch (wm->type)
    {
    case MODULE32_PE:
     	retproc = PE_FindExportedFunction( wm, function, snoop );
	if (!retproc) SetLastError(ERROR_PROC_NOT_FOUND);
	break;
#ifdef HAVE_LIBDL
    case MODULE32_ELF:
	retproc = (FARPROC) dlsym( (void*) wm->module, function);
	if (!retproc) SetLastError(ERROR_PROC_NOT_FOUND);
	return retproc;
#endif
    default:
    	ERR("wine_modref type %d not handled.\n",wm->type);
    	SetLastError(ERROR_INVALID_HANDLE);
    	return (FARPROC)0;
    }
    return retproc;
}
