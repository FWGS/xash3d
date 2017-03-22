rem msvc writes all objects to CWD, so we need do some renames to resolve conflicts
move common\common.c common\host_common.c
move common\soundlib\libmpg\common.c common\soundlib\libmpg\mpg_common.c
cl -Dvsnprintf=_vsnprintf -o xash_sdl.dll /DEBUG /Zi /DLL /W3 /Gm /GD /G6 /LD /O2 /MD /O2 /D_USRDLL /DXASH_FORCEINLINE /DXASH_FASTSTR /D_WINDLL common\*.c client\*.c server\*.c client\vgui\*.c common\soundlib\*.c common\soundlib\libmpg\*.c common\imagelib\*.c platform\sdl\*.c platform\win32\*.c -I ../msvc6/ -I ../SDL2-2.0.5/include/ -Icommon -I../common -I. -I../pm_shared -Iclient -Iserver -Iclient/vgui -Icommon/sdl -DXASH_VGUI -DXASH_SDL -Dsnprintf=_snprintf -DDBGHELP /link /DLL /LIBPATH:..\msvc6 user32.lib shell32.lib gdi32.lib msvcrt.lib winmm.lib /nodefaultlib:"libc.lib" /subsystem:windows ../SDL2-2.0.5/lib/x86/SDL2.lib kernel32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /DEBUG
move common\host_common.c common\common.c
move common\soundlib\libmpg\mpg_common.c common\soundlib\libmpg\common.c
