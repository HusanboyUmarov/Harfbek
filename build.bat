@echo off
REM ============================================================
REM  Harfbek - qurish (build) skripti (MSVC)
REM  Visual Studio "Developer Command Prompt" dan ishga tushiring.
REM  Agar cl topilmasa, MinGW uchun build_mingw.bat ni ishlating.
REM ============================================================

REM Skript qaysi papkada bo'lsa, o'sha yerda ishlaymiz.
cd /d "%~dp0"

where cl >nul 2>nul
if errorlevel 1 (
    echo [!] "cl" topilmadi. Visual Studio "Developer Command Prompt" dan ishga tushiring,
    echo     yoki MinGW uchun build_mingw.bat ni ishlating.
    pause
    exit /b 1
)

REM ikonka resursi (rc.exe Windows SDK bilan keladi)
set "RESFILE="
if exist app.rc (
    rc /nologo /fo app.res app.rc
    if exist app.res set "RESFILE=app.res"
)

REM eski exe qolib ketsa "yolg'on OK" bo'lmasligi uchun o'chiramiz
if exist harfbek.exe del /q harfbek.exe

REM  /MT : CRT ni statik bog'laymiz (boshqa kompyuterlarda VC redist kerak bo'lmasin).
REM  src\*.cpp : barcha manba fayllar kompilyatsiya qilinadi.
cl /nologo /EHsc /O2 /MT /utf-8 /DUNICODE /D_UNICODE ^
   src\*.cpp %RESFILE% ^
   /Fe:harfbek.exe ^
   /link user32.lib shell32.lib gdi32.lib advapi32.lib ole32.lib uuid.lib /SUBSYSTEM:WINDOWS

if exist harfbek.exe (
    del /q *.obj 2>nul
    echo.
    echo [OK] harfbek.exe tayyor. Ishga tushirish: harfbek.exe
) else (
    echo [!] Qurishda xato. Yuqoridagi xabarlarni o'qing.
)
pause
