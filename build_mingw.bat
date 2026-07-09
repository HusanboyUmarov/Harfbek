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
if not defined GPP call :find_winget_gpp

REM g++ yo'q bo'lsa - foydalanuvchi roziligi bilan winget orqali o'zimiz o'rnatamiz
if not defined GPP (
    echo [XATO] g++ topilmadi. g++ - C++ kompilyatori, .exe yasash uchun kerak.
    echo.
    echo   Eslatma: agar sizga faqat TAYYOR dastur kerak bo'lsa, build shart emas -
    echo   boshqa kompyuterda qurilgan harfbek.exe ni oddiy ko'chirib ishlating.
    echo   Kompilyator faqat kodni o'zgartirib qayta quradiganlarga kerak.
    echo.
    echo   O'rnatish bir martalik, lekin sekin kompyuterda 20-40 daqiqa olishi
    echo   mumkin. Keyingi build'lar esa bir necha soniyada o'tadi.
    echo.
    set "JAVOB="
    set /p "JAVOB=MinGW-w64 hozir avtomatik o'rnatilsinmi? Internet kerak. [h=ha / y=yo'q]: "
    if /i not "!JAVOB!"=="h" (
        echo.
        echo Bekor qilindi. Qo'lda o'rnatish uchun:
        echo   winget install BrechtSanders.WinLibs.POSIX.UCRT
        echo yoki build.bat / MSVC dan foydalaning.
        pause
        exit /b 1
    )
    where winget >nul 2>nul
    if errorlevel 1 (
        echo [XATO] winget topilmadi. Windows'ni yangilang yoki qo'lda o'rnating: https://winlibs.com
        pause
        exit /b 1
    )
    echo.
    echo MinGW-w64 o'rnatilmoqda...
    echo "Extracting archive..." qadami eng uzuni - qotib qolgan EMAS, kuting.
    winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e --accept-source-agreements --accept-package-agreements
    call :find_winget_gpp
    if not defined GPP (
        echo.
        echo [XATO] O'rnatishdan keyin ham g++ topilmadi.
        echo Terminalni yopib qayta oching va build_mingw.bat ni yana ishga tushiring.
        pause
        exit /b 1
    )
    echo.
    echo [OK] MinGW-w64 o'rnatildi, davom etamiz.
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

REM WinGet o'rnatgan WinLibs papkasidan g++ ni izlaydi (PATH yangilanishi shart emas)
:find_winget_gpp
for /d %%d in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*") do (
    if exist "%%d\mingw64\bin\g++.exe" set "GPP=%%d\mingw64\bin\g++.exe"
)
exit /b 0
