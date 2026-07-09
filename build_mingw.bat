@echo off
setlocal enabledelayedexpansion
REM ============================================================
REM  Harfbek - MinGW-w64 (g++) bilan qurish
REM  g++ ni avval PATH'dan, topilmasa WinGet WinLibs papkasidan izlaydi.
REM  Ikonkani (app.rc) windres bilan kompilyatsiya qilib bog'laydi.
REM ============================================================

REM Skript qaysi papkada bo'lsa, o'sha yerda ishlaymiz.
REM ("Run as administrator" da CWD System32 bo'lib qoladi — shuni tuzatadi.)
cd /d "%~dp0"

set "GPP="
where g++ >nul 2>nul && set "GPP=g++"

if not defined GPP (
    for /d %%d in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*") do (
        if exist "%%d\mingw64\bin\g++.exe" set "GPP=%%d\mingw64\bin\g++.exe"
    )
)

if not defined GPP (
    echo [XATO] g++ topilmadi. O'rnatish uchun:
    echo       winget install BrechtSanders.WinLibs.POSIX.UCRT
    echo     yoki build.bat / MSVC dan foydalaning.
    pause
    exit /b 1
)
echo g++: !GPP!

REM windres ni g++ yonidan (yoki PATH'dan) topamiz
set "WINDRES=windres"
if /i not "!GPP!"=="g++" (
    for %%f in ("!GPP!") do set "WINDRES=%%~dpfwindres.exe"
)

REM ikonka resursini kompilyatsiya qilamiz
set "RESOBJ="
if exist app.rc (
    "!WINDRES!" -I . -O coff app.rc app_res.o
    if exist app_res.o (
        set "RESOBJ=app_res.o"
        echo ikonka: app_res.o bog'landi
    ) else (
        echo [Ogohlantirish] ikonka kompilyatsiya bo'lmadi, ikonkasiz davom etamiz.
    )
)

REM dastur ishlab turgan bo'lsa, exe ustiga yozib bo'lmaydi — avval yopamiz
tasklist /fi "imagename eq harfbek.exe" 2>nul | find /i "harfbek.exe" >nul
if not errorlevel 1 (
    echo harfbek.exe ishlab turibdi - yopilmoqda...
    taskkill /f /im harfbek.exe >nul 2>nul
    ping -n 2 127.0.0.1 >nul
)

REM eski exe qolib ketsa "yolg'on OK" bo'lmasligi uchun o'chiramiz
if exist harfbek.exe del /q harfbek.exe

REM fayllar sonini sanaymiz (progress ko'rsatish uchun)
set /a TOTAL=0
for %%f in (src\*.cpp) do set /a TOTAL+=1

echo.
echo Kompilyatsiya boshlandi: %TOTAL% ta fayl. Biroz kuting...
echo.

REM har bir faylni alohida kompilyatsiya qilamiz — jarayon ko'rinib turadi
if not exist build_obj mkdir build_obj
set /a N=0
set "OBJS="
for %%f in (src\*.cpp) do (
    set /a N+=1
    title Harfbek build: !N!/%TOTAL% - %%~nxf
    echo   [!N!/%TOTAL%] %%f
    "!GPP!" -O2 -finput-charset=UTF-8 -c "%%f" -o "build_obj\%%~nf.o"
    if errorlevel 1 goto :xato
    set "OBJS=!OBJS! build_obj\%%~nf.o"
)

REM  -static* : libstdc++/libgcc/winpthread ni EXE ichiga statik bog'laymiz,
REM  shunda dastur MinGW o'rnatilmagan boshqa kompyuterlarda ham ishlaydi.
REM  -s : ramzlarni olib tashlab, hajmni kichraytiramiz.
title Harfbek build: bog'lanmoqda...
echo   [link] harfbek.exe yig'ilmoqda...
"!GPP!" -static -static-libgcc -static-libstdc++ -s ^
    !OBJS! !RESOBJ! -o harfbek.exe ^
    -luser32 -lshell32 -lgdi32 -ladvapi32 -mwindows
if errorlevel 1 goto :xato
if not exist harfbek.exe goto :xato

rd /s /q build_obj 2>nul
title Harfbek build: tayyor
echo.
echo [OK] harfbek.exe tayyor. Ishga tushirish: harfbek.exe
endlocal
pause
exit /b 0

:xato
title Harfbek build: XATO
echo.
echo [XATO] Qurishda xato. Yuqoridagi xabarlarni o'qing.
endlocal
pause
exit /b 1
