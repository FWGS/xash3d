rc game.rc
set SDL_PATH=../SDL2-2.0.8
cl /MD /W3 /GX /O2 /D_USRDLL /D_WINDLL -Dvsnprintf=_vsnprintf xash.c -I %SDL_PATH%/include/ -Icommon -I../common -I. -I../pm_shared -Iclient -Iserver -Iclient/vgui -Icommon/sdl -o xash_vc.exe -DXASH_SDL -DNO_ICO /link game.RES user32.lib shell32.lib gdi32.lib msvcrt.lib winmm.lib /nodefaultlib:"libc.lib" /subsystem:windows %SDL_PATH%/lib/x86/SDL2.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib
