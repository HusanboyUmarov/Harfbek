@echo off
setlocal enabledelayedexpansion
REM ============================================================
REM  Harfbek - MinGW-w64 (g++) bilan qurish
REM  g++ ni avval PATH'dan, topilmasa WinGet WinLibs papkasidan izlaydi.
REM  Ikonkani (app.rc) windres bilan kompilyatsiya qilib bog'laydi.
REM ============================================================

set "GPP="
where g++ >nul 2>nul && set "GPP=g++"

if not defined GPP (
    for /d %%d in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*") do (
        if exist "%%d\mingw64\bin\g++.exe" set "GPP=%%d\mingw64\bin\g++.exe"
    )
)

if not defined GPP (
    echo [!] g++ topilmadi. MinGW-w64 o'rnating yoki build.bat / MSVC dan foydalaning.
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
        echo [!] ogohlantirish: ikonka kompilyatsiya bo'lmadi, ikonkasiz davom etamiz.
    )
)

REM barcha .cpp fayllarni yig'amiz
set "SRCS="
for %%f in (src\*.cpp) do set "SRCS=!SRCS! %%f"

REM  -static* : libstdc++/libgcc/winpthread ni EXE ichiga statik bog'laymiz,
REM  shunda dastur MinGW o'rnatilmagan boshqa kompyuterlarda ham ishlaydi.
REM  -s : ramzlarni olib tashlab, hajmni kichraytiramiz.
"!GPP!" -O2 -finput-charset=UTF-8 ^
    -static -static-libgcc -static-libstdc++ -s ^
    !SRCS! !RESOBJ! -o harfbek.exe ^
    -luser32 -lshell32 -lgdi32 -ladvapi32 -mwindows

if exist harfbek.exe (
    echo.
    echo [OK] harfbek.exe tayyor. Ishga tushirish: harfbek.exe
) else (
    echo [!] Qurishda xato.
)
endlocal
