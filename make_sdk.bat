@echo off
color 5A
echo 			 XashXT Group 2010 (C)
echo 			   Create Xash3D SDK
echo.

if not exist D:\Xash3D\src_main\xash_sdk/ mkdir D:\Xash3D\src_main\xash_sdk\
if not exist D:\Xash3D\src_main\xash_sdk\engine/ mkdir D:\Xash3D\src_main\xash_sdk\engine\
if not exist D:\Xash3D\src_main\xash_sdk\common/ mkdir D:\Xash3D\src_main\xash_sdk\common\
if not exist D:\Xash3D\src_main\xash_sdk\mainui/ mkdir D:\Xash3D\src_main\xash_sdk\mainui\
if not exist D:\Xash3D\src_main\xash_sdk\mainui\legacy/ mkdir D:\Xash3D\src_main\xash_sdk\mainui\legacy
if not exist D:\Xash3D\src_main\xash_sdk\utils/ mkdir D:\Xash3D\src_main\xash_sdk\utils\
if not exist D:\Xash3D\src_main\xash_sdk\utils\makefont/ mkdir D:\Xash3D\src_main\xash_sdk\utils\makefont
if not exist D:\Xash3D\src_main\xash_sdk\utils\vgui/ mkdir D:\Xash3D\src_main\xash_sdk\utils\vgui
if not exist D:\Xash3D\src_main\xash_sdk\utils\vgui\include/ mkdir D:\Xash3D\src_main\xash_sdk\utils\vgui\include
if not exist D:\Xash3D\src_main\xash_sdk\utils\vgui\lib/ mkdir D:\Xash3D\src_main\xash_sdk\utils\vgui\lib
if not exist D:\Xash3D\src_main\xash_sdk\utils\vgui\lib\win32_vc6/ mkdir D:\Xash3D\src_main\xash_sdk\utils\vgui\lib\win32_vc6
if not exist D:\Xash3D\src_main\xash_sdk\game_launch/ mkdir D:\Xash3D\src_main\xash_sdk\game_launch\
if not exist D:\Xash3D\src_main\xash_sdk\cl_dll/ mkdir D:\Xash3D\src_main\xash_sdk\cl_dll\
if not exist D:\Xash3D\src_main\xash_sdkcl_dll\hl/ mkdir D:\Xash3D\src_main\xash_sdk\cl_dll\hl\
if not exist D:\Xash3D\src_main\xash_sdk\dlls/ mkdir D:\Xash3D\src_main\xash_sdk\dlls\
if not exist D:\Xash3D\src_main\xash_sdk\dlls\wpn_shared/ mkdir D:\Xash3D\src_main\xash_sdk\dlls\wpn_shared\
if not exist D:\Xash3D\src_main\xash_sdk\game_shared/ mkdir D:\Xash3D\src_main\xash_sdk\game_shared\
if not exist D:\Xash3D\src_main\xash_sdk\pm_shared/ mkdir D:\Xash3D\src_main\xash_sdk\pm_shared\
@copy /Y engine\*.h xash_sdk\engine\*.h
@copy /Y game_launch\*.* xash_sdk\game_launch\*.*
@copy /Y mainui\*.* xash_sdk\mainui\*.*
@copy /Y mainui\legacy\*.* xash_sdk\mainui\legacy\*.*
@copy /Y common\*.* xash_sdk\common\*.*
@copy /Y cl_dll\*.* xash_sdk\cl_dll\*.*
@copy /Y cl_dll\hl\*.* xash_sdk\cl_dll\hl\*.*
@copy /Y dlls\*.* xash_sdk\dlls\*.*
@copy /Y dlls\wpn_shared\*.* xash_sdk\dlls\wpn_shared\*.*
@copy /Y utils\makefont\*.* xash_sdk\utils\makefont\*.*
@copy /Y utils\vgui\include\*.* xash_sdk\utils\vgui\include\*.*
@copy /Y utils\vgui\lib\win32_vc6\*.* xash_sdk\utils\vgui\lib\win32_vc6\*.*
@copy /Y game_shared\*.* xash_sdk\game_shared\*.*
@copy /Y pm_shared\*.* xash_sdk\pm_shared\*.*
@copy /Y xash_sdk.dsw xash_sdk\xash_sdk.dsw
echo 			     Prepare OK!
echo 		     Please wait: creating SDK in progress
C:\Progra~1\WinRar\rar a xash_sdk -dh -k -r -s -df -m5 @xash_sdk.lst >>makesdk.log
if errorlevel 1 goto error
if errorlevel 0 goto ok
:ok
cls
echo 		     SDK was sucessfully created
echo 		     and stored in RAR-chive "xash_sdk"
echo 		      Press any key for exit. :-)
if exist makesdk.log del /f /q makesdk.log
exit
:error
echo 		    ******************************
echo 		    ***********Error!*************
echo 		    ******************************
echo 		    *See makesdk.log for details**
echo 		    ******************************
echo 		    ******************************
echo.
echo 		      press any key for exit :-(
pause>nul
exit