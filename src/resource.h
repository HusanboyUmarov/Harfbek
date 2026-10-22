#pragma once
// Tashqi havolalar (ui_about.cpp va ui_license.cpp ishlatadi)
#define URL_GITHUB      L"https://github.com/HusanboyUmarov/Harfbek"

// Dastur ikonkasi
#define IDI_APPICON     101

// Til tanlash dialogi
#define IDD_LANG        200
#define IDC_LANG_PROMPT 201
#define IDC_LANG_UZ     202
#define IDC_LANG_RU     203
#define IDC_LANG_EN     204

// Sozlamalar dialogi: asosiy oyna + 4 ta tab sahifasi (child dialog)
#define IDD_SETTINGS      210
#define IDC_SET_TAB       211   // tab control
#define IDD_SET_GENERAL   250
#define IDD_SET_MODE      251
#define IDD_SET_HOTKEYS   252
#define IDD_SET_EXCLUDE   253

// Tab "Umumiy"
#define IDC_SET_LANG_LBL  212
#define IDC_SET_LANG      213   // til combobox
#define IDC_SET_APO_LBL   214
#define IDC_SET_APO       215   // apostrof combobox
#define IDC_SET_STARTUP   216   // Windows bilan ishga tushish (checkbox)
#define IDC_SET_NOTIFY    217   // bildirishnoma (checkbox)

// Tab "Rejim" (IDC_SET_MODE_TRIGGER..IDC_SET_MODE_AUTO ketma-ket: CheckRadioButton)
#define IDC_SET_MODE_TRIGGER 220   // radio: trigger tugma
#define IDC_SET_MODE_AUTO    221   // radio: avto rejim
#define IDC_SET_TRIG_LBL  222
#define IDC_SET_TRIG      223   // trigger combobox
#define IDC_SET_RULES_LBL 224
#define IDC_SET_RULE_O    225   // avto qoidalar (checkbox): o' g' sh ch
#define IDC_SET_RULE_G    226
#define IDC_SET_RULE_SH   227
#define IDC_SET_RULE_CH   228
#define IDC_SET_EXAMPLE   229   // misol matni

// Tab "Tezkor tugmalar"
#define IDC_SET_HK_MODE_LBL     232
#define IDC_SET_HK_MODE         233   // hotkey control
#define IDC_SET_HK_ENABLE_LBL   234
#define IDC_SET_HK_ENABLE       235
#define IDC_SET_HK_CONVERT_LBL  236
#define IDC_SET_HK_CONVERT      237
#define IDC_SET_HK_HINT         238

// Tab "Istisnolar"
#define IDC_SET_EX_HINT   260
#define IDC_SET_EX_LIST   261
#define IDC_SET_EX_EDIT   262
#define IDC_SET_EX_ADD    263
#define IDC_SET_EX_BROWSE 264
#define IDC_SET_EX_REMOVE 265
// Dastur haqida dialogi
#define IDD_ABOUT         230
#define IDC_ABOUT_DESC    231
#define IDC_ABOUT_DEV     232
#define IDC_ABOUT_OPEN    233   // LinkedIn'ni ochish tugmasi
#define IDC_ABOUT_GITHUB  234   // GitHub repo tugmasi
#define IDC_ABOUT_LICENSE 235   // Litsenziya tugmasi

// Litsenziya / ogohlantirish dialogi
#define IDD_LICENSE       240
#define IDC_LIC_TEXT      241
#define IDC_LIC_GITHUB    242
