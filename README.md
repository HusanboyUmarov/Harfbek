# Harfbek

![Windows](https://img.shields.io/badge/Windows-10%20%7C%2011-0078D6?logo=windows&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-Win32-blue)
![License](https://img.shields.io/badge/litsenziya-MIT-green)

**O'zbek maxsus harflarini (ö ğ ş ç) istalgan dasturda bitta tugma bilan yozing.**

Harfbek — tray'da turadigan kichik dastur. **Trigger tugmani** (standart: **AltGr —
o'ng Alt**) bosib turib harfni bossangiz, maxsus harf chiqadi. Word, brauzer,
Telegram — hamma joyda ishlaydi.

| Bosasiz | Chiqadi | | Bosasiz | Chiqadi |
|---|---|---|---|---|
| `AltGr` + `o` | **ö** | | `AltGr` + `Shift` + `o` | **Ö** |
| `AltGr` + `g` | **ğ** | | `AltGr` + `Shift` + `g` | **Ğ** |
| `AltGr` + `s` | **ş** | | `AltGr` + `Shift` + `s` | **Ş** |
| `AltGr` + `c` | **ç** | | `AltGr` + `Shift` + `c` | **Ç** |

> CapsLock yoqiq bo'lsa ham katta harf chiqadi.

---

## 📥 Yuklab olish

Tayyor dastur **[Releases](https://github.com/HusanboyUmarov/Harfbek/releases)**
bo'limida — `harfbek.exe` ni yuklab, ikki marta bosing. Bo'ldi!

- O'rnatish shart emas — **bitta mustaqil fayl** (hech qanday qo'shimcha dastur
  yoki DLL talab qilmaydi).
- Windows 10 va 11 da ishlaydi.

Ishga tushgach, soat yonida binafsha **H** ikonka paydo bo'ladi. Unga o'ng tugma
bosib Sozlamalar, Qo'llanma yoki Chiqish'ni tanlaysiz.

## ✨ Imkoniyatlar

- **Maxsus harflar** — trigger + harf → ö ğ ş ç (Shift yoki CapsLock bilan katta harf).
- **Matnni o'girish** — matnni belgilab **Ctrl + sichqoncha o'ng tugmasi** bossangiz
  menyu chiqadi: `ö ğ ş ç → o' g' sh ch` yoki teskarisi.
- **Trigger tugmani tanlash** — 10 ta sinalgan variant: o'ng Alt (AltGr), chap Alt,
  ikkala Alt, o'ng Ctrl, o'ng Shift, CapsLock, Menyu tugmasi, Scroll Lock, Pause, F12.
- **3 til interfeysi** — o'zbekcha, ruscha, inglizcha.
- **Apostrof tanlash** — `o'` va `g'` uchun `'`, `ʻ`, `` ` `` yoki `’`.
- **Windows bilan ishga tushish** — bitta belgi bilan yoqiladi.
- **Xavfsiz** — hech narsa yozib olinmaydi va saqlanmaydi, internetga chiqmaydi.
  Kod to'liq ochiq (`src/`).

> **Nega aynan o'ng Alt?** Chap Alt + Shift Windows'da klaviatura tilini
> almashtiradi. O'ng Alt (AltGr) unga tegmaydi — katta harf yozganda til almashib
> ketmaydi.

> **Eslatma:** Alt/Ctrl/Shift bo'lmagan trigger (masalan, CapsLock yoki Menyu
> tugmasi) tanlansa, u tugma faqat trigger bo'lib ishlaydi — o'zining odatdagi
> vazifasini bajarmaydi. Bu ataylab shunday qilingan.

> Administrator huquqida ishlayotgan dasturlarda yozish uchun `harfbek.exe` ni ham
> administrator sifatida ishga tushiring.

Sozlamalar `%APPDATA%\Harfbek\settings.ini` faylida saqlanadi.

## 🛠 Manbadan qurish (build)

Oddiy foydalanuvchiga kerak emas — tayyor `.exe` ni
[Releases](https://github.com/HusanboyUmarov/Harfbek/releases) dan oling.
Kodni o'zgartirib qurmoqchi bo'lsangiz:

### 1. Repozitoriyni yuklab oling

```bat
git clone https://github.com/HusanboyUmarov/Harfbek.git
cd Harfbek
```

### 2. Build skriptini ishga tushiring

```bat
build_mingw.bat
```

Skript hammasini o'zi qiladi:

- **g++ (C++ kompilyatori) topilmasa — o'rnatishni taklif qiladi**: `h` (ha) deb
  javob bersangiz, winget orqali MinGW-w64 ni avtomatik o'rnatib, build'ni davom
  ettiradi. Qo'lda o'rnatish uchun:
  ```powershell
  winget install BrechtSanders.WinLibs.POSIX.UCRT
  ```
- Har bir fayl kompilyatsiyasini `[3/10]` ko'rinishida ko'rsatib boradi.
- Natija: `harfbek.exe` — statik bog'langan, boshqa kompyuterlarga ko'chirsa
  bo'ladigan bitta fayl.

> **O'rnatish sekinmi?** MinGW-w64 o'rnatilayotganda `Extracting archive...`
> qadami sekin kompyuterda (ayniqsa HDD'da) **20–40 daqiqa** olishi mumkin —
> paket minglab fayldan iborat va antivirus har birini tekshiradi. Bu qotib
> qolgani EMAS — bekor qilmasdan kuting. O'rnatish **bir martalik**: keyingi
> barcha build'lar bir necha soniyada o'tadi.

> **Build `Permission denied` bilan yiqildimi?** Harfbek **administrator
> huquqida** ishlab turgan bo'lsa, build skripti uni avtomatik yopolmaydi va
> exe ustiga yoza olmaydi. Tray'dagi **H** ikonkaga o'ng tugma → **Chiqish**
> qilib, keyin build'ni qayta ishga tushiring. (Oddiy huquqda ishlayotgan
> bo'lsa, skript o'zi yopadi — hech narsa qilish shart emas.)

Visual Studio bo'lsa, MSVC bilan ham qursa bo'ladi — "Developer Command Prompt"
ochib: `build.bat`.

### Moslashtirish

- **Harflarni o'zgartirish:** `src/config.h` dagi jadvalni tahrirlang (Unicode
  kodlari izohlar bilan berilgan), keyin qayta quring.
- **Ikonkani o'zgartirish:** `make_icon.ps1` ni tahrirlab ishga tushiring
  (`powershell -ExecutionPolicy Bypass -File make_icon.ps1`), keyin qayta quring.

## 🔒 Rasmiy manba va fayl tekshiruvi

Bu — Harfbek'ning **rasmiy repozitoriysi**:
<https://github.com/HusanboyUmarov/Harfbek>

Rasmiy `.exe` **faqat** shu repozitoriyning Releases bo'limida tarqatiladi.
Boshqa manbalardan olingan yoki o'zgartirilgan versiyalar muallifga aloqador emas.

Har bir release'da `harfbek.exe` yonida uning SHA256 checksum'i
(`harfbek.exe.sha256`) beriladi. Yuklab olgach tekshiring:

```powershell
Get-FileHash harfbek.exe -Algorithm SHA256
```

Chiqqan kod release'dagi qiymat bilan bir xil bo'lsa — fayl asl. Mos kelmasa,
ishlatmang.

> **Muallif uchun:** release'dan oldin `checksum.ps1` ni ishga tushiring — u
> `harfbek.exe.sha256` faylini yasaydi (uni `.exe` bilan birga yuklang).

## 📄 Litsenziya

[MIT](LICENSE) — erkin va ochiq. Dastur "borligicha" (AS IS), kafolatsiz beriladi;
muallif hech qanday zarar uchun javobgar emas.

---

Ishlab chiquvchi: **Husanboy Umarov (ascoder)**
