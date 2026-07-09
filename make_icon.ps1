# ============================================================
#  Harfbek - ikonka yasash skripti
#  assets/icon.ico faylini yaratadi (binafsha gradient + oq "H" harfi).
#  Harf yoki rangni o'zgartirmoqchi bo'lsangiz pastdan tahrirlang.
# ============================================================
Add-Type -AssemblyName System.Drawing

$OutFile = Join-Path $PSScriptRoot "assets\icon.ico"
$AssetsDir = Split-Path $OutFile
if (-not (Test-Path $AssetsDir)) { New-Item -ItemType Directory -Path $AssetsDir | Out-Null }

# --- sozlash ---
$Letter = 'H'                                            # ko'rsatiladigan harf: Harfbek "H"
$Col1   = [System.Drawing.Color]::FromArgb(255, 129, 140, 248)  # gradient yuqori (binafsha)
$Col2   = [System.Drawing.Color]::FromArgb(255,  67,  56, 202)  # gradient past  (to'q binafsha)
$Sizes  = 16,24,32,48,64,128,256

function New-RoundedPath([int]$w, [int]$h, [int]$r) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = 2 * $r
    $p.AddArc(0,        0,        $d, $d, 180, 90)
    $p.AddArc($w-$d-1,  0,        $d, $d, 270, 90)
    $p.AddArc($w-$d-1,  $h-$d-1,  $d, $d, 0,   90)
    $p.AddArc(0,        $h-$d-1,  $d, $d, 90,  90)
    $p.CloseFigure()
    return $p
}

function Get-DibBytes([System.Drawing.Bitmap]$bmp) {
    $w = $bmp.Width; $h = $bmp.Height
    $rect = New-Object System.Drawing.Rectangle(0, 0, $w, $h)
    $bd = $bmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
                        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $stride = $bd.Stride
    $buf = New-Object byte[] ($stride * $h)
    [System.Runtime.InteropServices.Marshal]::Copy($bd.Scan0, $buf, 0, $buf.Length)
    $bmp.UnlockBits($bd)

    $ms = New-Object System.IO.MemoryStream
    $bw = New-Object System.IO.BinaryWriter($ms)
    # BITMAPINFOHEADER
    $bw.Write([int]40)          # biSize
    $bw.Write([int]$w)          # biWidth
    $bw.Write([int](2*$h))      # biHeight (XOR + AND)
    $bw.Write([int16]1)         # biPlanes
    $bw.Write([int16]32)        # biBitCount
    $bw.Write([int]0)           # biCompression BI_RGB
    $andRow = [int](([math]::Floor(($w + 31) / 32)) * 4)
    $bw.Write([int]($w*$h*4 + $andRow*$h))  # biSizeImage
    $bw.Write([int]0); $bw.Write([int]0)    # X/Y PelsPerMeter
    $bw.Write([int]0); $bw.Write([int]0)    # ClrUsed / ClrImportant
    # XOR pixel data (pastdan yuqoriga)
    for ($y = $h - 1; $y -ge 0; $y--) {
        $bw.Write($buf, $y * $stride, $w * 4)
    }
    # AND mask (hammasi 0 - shaffoflik alpha orqali)
    $zero = New-Object byte[] $andRow
    for ($y = 0; $y -lt $h; $y++) { $bw.Write($zero, 0, $andRow) }
    $bw.Flush()
    return $ms.ToArray()
}

# Har o'lcham uchun rasm chizamiz
$blobs = @()
foreach ($s in $Sizes) {
    $bmp = New-Object System.Drawing.Bitmap($s, $s, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode     = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $g.Clear([System.Drawing.Color]::Transparent)

    $radius = [int]($s * 0.22)
    $path = New-RoundedPath $s $s $radius
    $rectF = New-Object System.Drawing.Rectangle(0, 0, $s, $s)
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rectF, $Col1, $Col2, 90)
    $g.FillPath($brush, $path)

    $font = New-Object System.Drawing.Font("Segoe UI", ($s * 0.60), [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $sf = New-Object System.Drawing.StringFormat
    $sf.Alignment = [System.Drawing.StringAlignment]::Center
    $sf.LineAlignment = [System.Drawing.StringAlignment]::Center
    $layout = New-Object System.Drawing.RectangleF(0, -($s*0.02), $s, $s)
    $g.DrawString([string]$Letter, $font, [System.Drawing.Brushes]::White, $layout, $sf)

    $g.Dispose()
    $blobs += ,(Get-DibBytes $bmp)
    $bmp.Dispose()
}

# ICO faylini yig'amiz
$fs = New-Object System.IO.FileStream($OutFile, [System.IO.FileMode]::Create)
$bw = New-Object System.IO.BinaryWriter($fs)
$n = $Sizes.Count
$bw.Write([int16]0)   # reserved
$bw.Write([int16]1)   # type = icon
$bw.Write([int16]$n)  # count

$offset = 6 + 16 * $n
for ($i = 0; $i -lt $n; $i++) {
    $s = $Sizes[$i]
    $blob = $blobs[$i]
    $bw.Write([byte]($(if ($s -ge 256) { 0 } else { $s })))  # width
    $bw.Write([byte]($(if ($s -ge 256) { 0 } else { $s })))  # height
    $bw.Write([byte]0)     # colors
    $bw.Write([byte]0)     # reserved
    $bw.Write([int16]1)    # planes
    $bw.Write([int16]32)   # bitcount
    $bw.Write([int]$blob.Length)  # bytesInRes
    $bw.Write([int]$offset)       # imageOffset
    $offset += $blob.Length
}
foreach ($blob in $blobs) { $bw.Write($blob, 0, $blob.Length) }
$bw.Flush(); $fs.Close()

Write-Output ("[OK] Ikonka yaratildi: " + $OutFile + "  (" + (Get-Item $OutFile).Length + " bayt, " + $n + " o'lcham)")
