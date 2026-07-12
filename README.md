# PocketOS — LILYGO T5 E-Paper S3 Pro

Kompletní handheld e-paper OS pro **LILYGO T5 E-Paper S3 Pro** (4.7" ED047TC1,
ESP32-S3, LoRa SX1262, GPS, RTC, battery gauge).

> **v1.1** – přechod z (nefunkčního) IT8951/SPI ovladače na **FastEPD**
> (paralelní panel ED047TC1 + TPS65185 PMU), reálný pinout desky a 4 nové appky.

## Funkce

| App | Popis |
|-----|-------|
| **Books** | Čtečka knih (.txt i **.epub** ze SD), kapitoly, reflow, velikost písma |
| **Maps** | Offline dlaždicové mapy z SD, GPS tracking, zoom |
| **LoRa** | LoRa messenger s klávesnicí na displeji (SX1262) |
| **Settings** | Jas, font, **Wi-Fi setup (sken + heslo)**, LoRa frekvence, GPS, timezone, sleep |
| **Clock** | Analogové/digitální hodiny + kalendář (z RTC PCF8563) |
| **Chess** | Šachy s AI (minimax) |
| **Conquest** | Tahová hex strategie (Polytopia-like) |
| **Browser** | Text-mode HTTP/HTTPS browser, výchozí **google.com** (potřebuje Wi-Fi) |
| **Notes** | Poznámky uložené na SD |
| **Calculator** | Kalkulačka |
| **Device Info** | **ADI** – kompletní live info: SoC, paměť, baterie (BQ25896), radia, GPS, RTC |
| **Sketch** | Kreslení prstem na e-ink, uložení do BMP na SD |
| **Sudoku** | Offline hlavolam |
| **Files** | Správce SD karty (procházení, náhled textu, mazání) |

## UI — iOS-like

- **Home screen**: ikonová mřížka 4×4, dock (4 ikony vždy viditelné), stránky
- **Control Center**: přejetí prstem dolů z horního okraje → ovládání jasu, fontu, LoRa, WiFi, sleep
- **Nav bar**: každá app má tlačítko `< Home` v levém horním rohu
- **Status bar**: čas, GPS, LoRa stav, baterie

## Hardware

| Komponenta | Popis |
|-----------|-------|
| MCU | ESP32-S3-WROOM-1 (dual-core 240MHz, 16MB flash, 8MB PSRAM) |
| Display | 4.7" **ED047TC1** paralelní e-paper, 960×540, 16 gray (FastEPD + TPS65185) |
| Touch | GT911 capacitive (sdílená I2C) |
| LoRa | SX1262 (433/868/915 MHz) |
| GPS | u-blox MIA-M10Q přes UART2 |
| RTC | PCF8563 |
| Battery | BQ25896 charger/gauge (I2C) |
| Storage | MicroSD (sdílená SPI s LoRa) |

## Pinout (config.h → z LilyGO `docs/pin_define.md`)

```
I2C (touch+RTC+PMU):  SDA=39  SCL=40
Touch GT911:          INT=3   RST=9   (addr 0x14 / 0x5D)
RTC PCF8563:          IRQ=2   (addr 0x51)
Battery BQ25896:      addr 0x6B
SPI (SD + LoRa):      MISO=21 MOSI=13 SCLK=14
SD Card:              CS=12
LoRa SX1262:          CS=46   RST=1   DIO1=10  BUSY=47
GPS UART2:            RX=44   TX=43
ED047 data bus:       D0=5 D1=6 D2=7 D3=15 D4=16 D5=17 D6=18 D7=8
ED047 control:        CKV=48  STH=41  LEH=42  STV=45  CKH=4   (řídí FastEPD)
```

> Display + PMU řídí knihovna **FastEPD** přes profil `BB_PANEL_EPDIY_V7`
> (vendornutá v `lib/FastEPD/`). Displej nemá frontlight – GPIO11 je eink DC/DC power.

## SD karta — struktura

```
/pocket/
├── config.json        ← nastavení (auto-generováno)
├── books/             ← .txt soubory knih
│   ├── bible.txt
│   └── dracula.txt
├── maps/              ← offline dlaždice (viz tools/download_maps.py)
│   └── 13/
│       └── 4397/
│           └── 2808.raw
├── notes/             ← poznámky .txt
└── saves/             ← herní uložení
```

## Instalace

### 1. Závislosti (PlatformIO)

```bash
cd T5S3-PocketOS
pio lib install
```

### 2. Kompilace a nahrání

```bash
pio run -t upload
```

### 3. Offline mapy

```bash
pip install Pillow
python3 tools/download_maps.py --lat 50.075 --lng 14.437 --radius 0.2 --zoom 11 12 13 14
# Zkopírovat maps_out/ na SD kartu jako /pocket/maps/
```

### 4. Knihy

Zkopírujte `.txt` soubory na SD kartu do `/pocket/books/`.

## Control Center — gesta

| Gesto | Akce |
|-------|------|
| Přejet dolů z horní hrany | Otevřít Control Center |
| Přejet nahoru v CC | Zavřít CC |
| Tap na slider jasu | Nastavit jas |
| Tap na slider fontu | Nastavit velikost písma |
| Tap toggle LoRa/WiFi | Zapnout/vypnout |
| Tap Sleep | Deep sleep |

## Ladění pinů

Pokud váš konkrétní model T5S3 má jiné piny, upravte `src/config.h`. Pinout se liší mezi revizemi desky — ověřte si schéma na [github.com/Xinyuan-LilyGO/LilyGo-EPD47](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47).

## Přidání vlastní appky

1. Vytvořte `src/apps/myapp.h` a `.cpp` — dědičnost z `App`
2. Definujte unikátní `#define MYAPP_APP_ID  11`
3. V `main.cpp`: přidejte instanci, zaregistrujte `am.registerApp(&myApp)`
4. Přidejte do `APPS[]` s ikonou

## Licence

MIT — použijte jak chcete.
