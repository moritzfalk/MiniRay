@echo off

REM Optimisation flags: -Ox -Ot -fp:fast -fp:except-
set CommonCompilerFlags=-Ox -Ot -nologo -Gm- -GR- -EHa- -fp:fast -fp:except- -Oi -WX -W4 -wd4505 -wd4201 -wd4100 -wd4189 -wd4456 -wd4302 -wd4324 -wd4311 -FC -Z7 
set CommonLinkerFlags= -INCREMENTAL:no -opt:ref kernel32.lib user32.lib gdi32.lib winmm.lib Ole32.lib d3d11.lib d3dcompiler.lib dxgi.lib dxguid.lib

IF NOT EXIST ..\..\build_miniray mkdir ..\..\build_miniray
pushd ..\..\build_miniray

REM Build OBJ parser
cl %CommonCompilerFlags% ..\MiniRay\code\obj_parser.cpp /link %CommonLinkerFlags%

REM -DMINIRAY_DEBUG -DSSAA_4x
REM cl %CommonCompilerFlags% -DMINIRAY_WIN32=1 ..\MiniRay\code\miniray.cpp -Fmminiray.map /link -DEBUG:FULL %CommonLinkerFlags%


REM Here you can read about how to compile without the CRT: https://hero.handmade.network/forums/code-discussion/t/94-guide_-_how_to_avoid_c_c++_runtime_on_windows

REM Compiling without the C standard library
cl %CommonCompilerFlags% /Gs9999999 /GS- -DSSAA_4x=0 -DMINIRAY_WIN32=1 ..\MiniRay\code\miniray.cpp -Fmminiray.map /link -DEBUG:FULL %CommonLinkerFlags% /STACK:0x100000,0x100000 /NODEFAULTLIB /subsystem:windows 

popd