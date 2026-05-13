# PocketOS — LILYGO T5S3 4.7" E-Paper

Kompletní handheld e-paper OS pro LILYGO T5S3 4.7" s LoRa.

## Funkce

| App | Popis |
|-----|-------|
| **Books** | Čtečka knih (.txt ze SD karty), stránkování přejetím |
| **Maps** | Offline vektorové mapy z SD, GPS tracking, zoom |
| **LoRa** | LoRa messenger s klávesnicí na displeji |
| **Settings** | Jas, font, LoRa frekvence, sleep timer |
| **Clock** | Analogové/digitální hodiny + kalendář |
| **Chess** | Šachy s AI (minimax hloubka 2) |
| **Conquest** | Tahová hex strategie (Polytopia-like) |
| **Browser** | Text-mode HTTP browser (potřebuje WiFi) |
| **Notes** | Poznámky uložené na SD |
| **Calculator** | Kalkulačka |

## UI — iOS-like

- **Home screen**: ikonová mřížka 4×4, dock (4 ikony vždy viditelné), stránky
- **Control Center**: přejetí prstem dolů z horního okraje → ovládání jasu, fontu, LoRa, WiFi, sleep
- **Nav bar**: každá app má tlačítko `< Home` v levém horním rohu
- **Status bar**: čas, GPS, LoRa stav, baterie

## Hardware

| Komponenta | Popis |
|-----------|-------|
| MCU | ESP32-S3 (dual-core 240MHz, 8MB PSRAM) |
| Display | 4.7" IT8951 e-paper, 960×540px |
| Touch | GT911 capacitive |
| LoRa | SX1262 (868/915 MHz) |
| GPS | NEO-6M nebo kompatibilní přes UART2 |
| Storage | MicroSD (SPI) |
| Frontlight | PWM LED, 0-255 |

## Pinout (config.h)

```
EPD SPI:    MOSI=11  MISO=13  SCK=12  CS=15  RST=16  BUSY=17
Touch I2C:  SDA=21   SCL=22   INT=38
SD Card:    CS=10  (sdílí SPI s EPD)
LoRa SPI:   MOSI=41  MISO=42  SCK=40  CS=8   RST=18  DIO1=45  BUSY=46
GPS UART2:  RX=44  TX=43
Frontlight: PWM=48
Battery:    ADC=1
```

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
