# ============================================================
#  Harfbek - release uchun SHA256 checksum yasash skripti.
#  harfbek.exe ning SHA256 ini hisoblab, ekranga chiqaradi va
#  harfbek.exe.sha256 fayliga yozadi (GitHub Releases'ga yuklash uchun).
#  Ishlatish:  powershell -ExecutionPolicy Bypass -File checksum.ps1
# ============================================================
param([string]$File = (Join-Path $PSScriptRoot "harfbek.exe"))

if (-not (Test-Path $File)) {
    Write-Error "Fayl topilmadi: $File  (avval build_mingw.bat ni ishga tushiring)"
    exit 1
}

$hash = (Get-FileHash $File -Algorithm SHA256).Hash
$name = Split-Path $File -Leaf
$size = (Get-Item $File).Length
$out  = "$File.sha256"

# sha256sum bilan mos format:  <hash><ikki bo'sh joy><fayl nomi>
"$hash  $name" | Set-Content -Path $out -Encoding ascii

Write-Output ""
Write-Output ("Fayl:   " + $name)
Write-Output ("Hajmi:  {0:N0} bayt" -f $size)
Write-Output ("SHA256: " + $hash)
Write-Output ""
Write-Output ("Saqlandi: " + $out)
Write-Output "Buni harfbek.exe bilan birga GitHub Releases'ga yuklang."
