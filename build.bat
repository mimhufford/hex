@echo off
if exist hex.obj del hex.obj
if exist hex.exe del hex.exe
cl /nologo /MD /O2 /GR- /EHa- hex.cpp ^
   /I raylib/include ^
   /link /SUBSYSTEM:WINDOWS /entry:mainCRTStartup /LTCG ^
   raylib/lib/raylib.lib Winmm.lib User32.lib Gdi32.lib Shell32.lib
if exist hex.obj del hex.obj