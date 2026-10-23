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
bosib Sozlamalar, Tanişuv yoki Çiqiş'ni tanlaysiz. Birinchi ishga tushishda
ish stolida **Harfbek yorlig'i** ham avtomatik yaratiladi — keyingi safar dasturni
shu yorliqdan ochasiz.

## ✨ Imkoniyatlar

- **Maxsus harflar** — trigger + harf → ö ğ ş ç (Shift yoki CapsLock bilan katta harf).
- **Matnni o'girish** — matnni belgilab **Ctrl + sichqoncha o'ng tugmasi** bossangiz
  menyu chiqadi: `ö ğ ş ç → o' g' sh ch` yoki teskarisi. Konsol oynalarida (cmd,
  PowerShell, Windows Terminal) Ctrl + o'ng tugma ushlanmaydi — terminalning o'z
  xatti-harakati qoladi. Belgilangan asl matnning nusxasi (Ctrl+C) Win+V tarixiga
  baribir tushadi; faqat o'girilgan vaqtinchalik matn tarixga tushmaydi va asl
  clipboard dublikatsiz tiklanadi.
- **Avto rejim** — o'zbek alifbosiga yangi harflar (ö ğ ş ç) kiritilgan: ular
  `o'`, `g'`, `sh`, `ch` o'rniga ishlatiladi. Avto rejimda eski yozuvda yozasiz
  (`o'` `g'` `sh` `ch`), dastur uni avtomatik ö ğ ş ç ga o'zgartiradi; katta harf
  saqlanadi (`Sh` → **Ş**, `sh` → **ş**). O'zgarishdan keyin darhol Backspace
  bossangiz, asl yozuv qaytadi (`ş` → `sh`) — inglizcha so'zlar uchun qulay.
  Terminal oynalarida ishlamaydi. Tray menyudan yoki Sozlamalardan yoqiladi.
  Trigger rejimi bilan bir vaqtda ishlamaydi — biri tanlanadi.
  **Parol yozganda ehtiyot bo'ling:** avto rejim parol maydonini ajrata olmaydi —
  parolda `sh`, `ch` yoki `o'` bo'lsa, u ham almashadi. Parol yozishdan oldin
  `Ctrl+Shift+F10` bilan vaqtincha o'chirib qo'ying yoki o'sha dasturni
  Istisnolar ro'yxatiga qo'shing.
- **Zamonaviy interfeys** — yorug' va qorong'i mavzu Windows sozlamasiga qarab
  avtomatik tanlanadi; tray ikonkaga chap tugma bilan tezkor panel ochiladi
  (o'ng tugma — oddiy menyu).
- **Sinab köriş maydoni** — Sozlamalardagi alohida sahifa: shu yerda yozib, Harfbek
  joriy rejimda qanday ishlashini jonli ko'rasiz; maxsus harflar sanagichi va kichik
  sinovlar bor. Yozganingiz hech qayerga saqlanmaydi.
- **Tanişuv oynasi** — birinchi ishga tushganda ochiladi: yangi alifbo, ikki rejim
  va tezkor tugmalar haqida qisqa tushuntirish, rejimni shu yerning o'zida tanlaysiz.
  Keyin tray menyudagi **Tanişuv...** orqali qayta ochish mumkin.
- **Sozlamalar tablari** — Umumiy | Rejim | Tezkor tugmalar | Istisnolar.
- **Tezkor tugmalar** (Sozlamalarda o'zgartiriladi): rejimni almashtirish
  `Ctrl+Shift+F9`, yoqish/o'chirish `Ctrl+Shift+F10`, belgilangan matnni o'girish
  `Ctrl+Shift+F11`.
- **Istisno dasturlar** — avto rejim ishlamaydigan dasturlar ro'yxati (terminallar
  doim istisno). Ro'yxatga dastur nomini (masalan `code.exe`) yozib yoki
  **Tanlaş...** tugmasi bilan `.exe` faylini tanlab qo'shasiz.
- **Ekrandagi bildirishnoma** — rejim yoki yoqish/o'chirish o'zgarganda kichik xabar
  qisqa vaqtga chiqadi (Sozlamalarda o'chirib qo'yish mumkin).
- **Avto rejim harflari** — `o'`, `g'`, `sh`, `ch` ning har birini alohida yoqish
  yoki o'chirish mumkin (Sozlamalar → Rejim tabi, avto rejim tanlanganda).
- **Trigger tugmani tanlash** — 10 ta sinalgan variant: o'ng Alt (AltGr), chap Alt,
  ikkala Alt, o'ng Ctrl, o'ng Shift, CapsLock, Menyu tugmasi, Scroll Lock, Pause, F12.
- **3 til interfeysi** — o'zbekcha, ruscha, inglizcha.
- **Apostrof tanlash** — `o'` va `g'` uchun `'`, `ʻ`, `` ` `` yoki `’`.
- **Windows bilan ishga tushish** — bitta belgi bilan yoqiladi.
- **Xavfsiz** — hech narsa yozib olinmaydi va saqlanmaydi, internetga chiqmaydi.
  Dastur administrator huquqi so'ramaydi (`asInvoker`), registrda faqat
  `HKCU\...\Run` yozuviga tegadi (avtozapusk yoqilganda). Tizim kutubxonalari
  (gdiplus, dwmapi, msimg32, comdlg32) faqat `System32` papkasidan yuklanadi —
  exe yoniga qo'yilgan begona DLL ishga tushmaydi. Kod to'liq ochiq (`src/`).

> **Nega aynan o'ng Alt?** Chap Alt + Shift Windows'da klaviatura tilini
> almashtiradi. Harfbek o'ng Alt (AltGr) bilan ishlaganda til almashishi va
> "yolg'iz Alt" menyu rejimini maxsus himoya bilan bostiradi — katta harf
> yozganda til almashib ketmaydi, oynaning menyu qatori faollashmaydi.

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
> exe ustiga yoza olmaydi. Tray'dagi **H** ikonkaga o'ng tugma → **Çiqiş**
> qilib, keyin build'ni qayta ishga tushiring. (Oddiy huquqda ishlayotgan
> bo'lsa, skript o'zi yopadi — hech narsa qilish shart emas.)

Visual Studio bo'lsa, MSVC bilan ham qursa bo'ladi — "Developer Command Prompt"
ochib: `build.bat`.

### Moslashtirish

- **Harflarni o'zgartirish:** `src/config.h` dagi jadvalni tahrirlang (Unicode
  kodlari izohlar bilan berilgan), keyin qayta quring.
- **Ikonkani o'zgartirish:** `make_icon.ps1` ni tahrirlab ishga tushiring
  (`powershell -ExecutionPolicy Bypass -File make_icon.ps1`), keyin qayta quring.
- **Manifest:** `app.manifest` (Common Controls v6, DPI) `app.rc` orqali exe ichiga
  o'rnatiladi; alohida fayl sifatida tarqatish shart emas.

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

## 📝 Versiyalar

Har bir release'ning to'liq tavsifi va fayllari:
<https://github.com/HusanboyUmarov/Harfbek/releases>

### v1.1.0 — 2026-10

Katta yangilanish: ikkinchi yozish rejimi, tezkor tugmalar va to'liq yangi
interfeys. Eski versiyadan o'tganda sozlamalar saqlanib qoladi, yangi kalitlar
standart qiymat bilan qo'shiladi.

**Yangi imkoniyatlar**

- **Avto rejim** — `o' g' sh ch` yozilishi bilan `ö ğ ş ç` ga almashadi, katta
  harf saqlanadi, darhol Backspace bosilsa asl yozuv qaytadi. Har bir juftlikni
  alohida yoqish/o'chirish mumkin. Terminal va istisno dasturlarda ishlamaydi.
- **Tezkor tugmalar** — rejim almashtirish `Ctrl+Shift+F9`, yoqish/o'chirish
  `Ctrl+Shift+F10`, belgilangan matnni o'girish `Ctrl+Shift+F11`. Band tugma
  haqida Sozlamalar ogohlantiradi.
- **Istisno dasturlar** — avto rejim ishlamaydigan dasturlar ro'yxati, nom bilan
  yoki `.exe` faylni tanlab qo'shiladi.
- **Trigger tugma tanlovi** — 10 ta sinalgan variant (o'ng/chap/ikkala Alt, o'ng
  Ctrl, o'ng Shift, CapsLock, Menyu, Scroll Lock, Pause, F12).
- **Yangi interfeys** — yorug'/qorong'i mavzu, tray ikonkaga chap tugma bilan
  tezkor panel, Sozlamalar tablari (Umumiy | Rejim | Tezkor tugmalar |
  Istisnolar), Tanişuv oynasi, sinab köriş maydoni, ekrandagi bildirishnoma.
- Exe'da versiya ma'lumoti (Xossalar → Tafsilotlar) va manifest (Common
  Controls v6, DPI, administrator huquqi so'ralmaydi).

**Tuzatishlar**

- Klaviatura hook'i alohida thread'da ishlaydi. Oldin Sozlamalar yoki menyu
  ochiq paytda butun tizim klaviaturasi kechikishi va Windows hook'ni jimgina
  olib tashlashi mumkin edi.
- Matnni o'girishda clipboard to'liq (barcha formatlar) saqlanib tiklanadi,
  vaqtinchalik matn Win+V tarixiga tushmaydi, bir vaqtda ikki o'girish bloklanadi.
- Ctrl+o'ng tugma: bosish va qo'yib yuborish juft holda ushlanadi, terminal
  oynalarida ushlanmaydi.
- Sozlamalar fayli: uzun yo'llar va buzilgan qiymatlar (noma'lum trigger, rejim,
  tezkor tugma) standartga qaytadi; rejim yoki trigger o'zgarganda eski
  "bosib turilgan" holat tozalanadi.
- Avtozapusk yozuvi exe ko'chirilganda yangi yo'lga moslanadi; Explorer qayta
  ishga tushganda tray ikonka tiklanadi.
- Tizim kutubxonalari (gdiplus, dwmapi, msimg32, comdlg32) faqat `System32`
  dan yuklanadi — exe yoniga qo'yilgan begona DLL ishga tushmaydi.

**Bilib qo'ying**

- Avto rejim parol maydonini ajrata olmaydi: parolda `sh`, `ch`, `o'` bo'lsa u
  ham almashadi. Parol yozishdan oldin `Ctrl+Shift+F10` bilan o'chirib qo'ying.

### v1.0 — 2026-07

Birinchi chiqarilish.

- Trigger rejimi: AltGr + `o g s c` → `ö ğ ş ç`, Shift yoki CapsLock bilan katta harf.
- Belgilangan matnni o'girish: Ctrl + sichqoncha o'ng tugmasi → `ö ğ ş ç ↔ o' g' sh ch`.
- Trigger tugmani tanlash, apostrof belgisini tanlash (`'`, `ʻ`, `` ` ``, `’`).
- Interfeys 3 tilda (o'zbek, rus, ingliz), birinchi ishga tushganda til tanlash va
  qo'llanma.
- Windows bilan ishga tushish, ish stolida yorliq, bitta buyruq bilan o'chirish
  (sozlamalar, avtozapusk, yorliq tozalanadi).
- Til almashish (Alt+Shift) va "yolg'iz Alt" menyu rejimiga qarshi himoya,
  osilib qolgan modifikator tekshiruvi.

## 📄 Litsenziya

[MIT](LICENSE) — erkin va ochiq. Dastur "borligicha" (AS IS), kafolatsiz beriladi;
muallif hech qanday zarar uchun javobgar emas.

---

Ishlab chiquvchi: **Husanboy Umarov (ascoder)**
