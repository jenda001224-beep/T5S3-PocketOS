#pragma once
#include <Arduino.h>

// ══════════════════════════════════════════════════════════════════════════════
//  LILYGO T5 E-Paper S3 Pro  (board rev H752 / v1.0-241224)
//  Pinout is authoritative — taken from LilyGO docs/pin_define.md.
//  Display: ED047TC1 parallel panel driven via FastEPD (BB_PANEL_EPDIY_V7 profile)
//           + TPS65185 PMU controlled through a PCA9535 I/O expander.
//  DO NOT reintroduce IT8951/SPI display pins — this board has no IT8951.
// ══════════════════════════════════════════════════════════════════════════════

// ─── Display (managed by FastEPD, pins baked into BB_PANEL_EPDIY_V7) ──────────
// ED047 data bus: D0=5 D1=6 D2=7 D3=15 D4=16 D5=17 D6=18 D7=8
// Control: CKV=48 STH=41 LEH=42 STV=45 CKH=4 ;  PMU power enable = GPIO11
#define EPD_W       960
#define EPD_H       540

// ─── Shared I2C bus  (GT911 touch · PCF8563 RTC · BQ25896 PMU · PCA9535) ───────
#define BOARD_SDA   39
#define BOARD_SCL   40

// ─── Touch GT911 (on shared I2C) ──────────────────────────────────────────────
#define TOUCH_SDA   BOARD_SDA
#define TOUCH_SCL   BOARD_SCL
#define TOUCH_INT   3
#define TOUCH_RST   9
#define TOUCH_ADDR  0x14   // primary; 0x5D probed as fallback

// ─── RTC PCF8563 (on shared I2C) ──────────────────────────────────────────────
#define RTC_IRQ     2
#define RTC_ADDR    0x51

// ─── Battery / charger BQ25896 (on shared I2C) ────────────────────────────────
#define BQ25896_ADDR   0x6B
#define BATTERY_MIN_MV 3300
#define BATTERY_MAX_MV 4200

// ─── PCA9535 IO expander interrupt ────────────────────────────────────────────
#define PCA9535_INT 38

// ─── Shared SPI bus  (SD card + LoRa SX1262) ──────────────────────────────────
#define BOARD_SPI_MISO  21
#define BOARD_SPI_MOSI  13
#define BOARD_SPI_SCLK  14

// ─── SD Card ──────────────────────────────────────────────────────────────────
#define SD_CS       12

// ─── LoRa SX1262 (shared SPI bus) ─────────────────────────────────────────────
#define LORA_SCK    BOARD_SPI_SCLK
#define LORA_MOSI   BOARD_SPI_MOSI
#define LORA_MISO   BOARD_SPI_MISO
#define LORA_CS     46
#define LORA_RST     1
#define LORA_DIO1   10
#define LORA_BUSY   47
#define LORA_FREQ   868.0   // MHz — EU. 915.0 for US, 433.0 for Asia.

// ─── GPS MIA-M10Q (UART2) ─────────────────────────────────────────────────────
#define GPS_RX      44
#define GPS_TX      43
#define GPS_BAUD    9600

// ─── UI constants ─────────────────────────────────────────────────────────────
#define STATUS_BAR_H    44
#define DOCK_H          96
#define ICON_SIZE       80
#define ICON_PAD        20
#define APP_COLS        4
#define APP_ROWS        4
#define CORNER_R        12
#define PAGE_DOT_Y      (EPD_H - DOCK_H - 16)

// ─── Colors (4bpp grayscale, 0=black 15=white) ───────────────────────────────
#define C_BLACK     0
#define C_DARK      3
#define C_MID       7
#define C_LIGHT     11
#define C_WHITE     15

// ─── Config / SD paths ────────────────────────────────────────────────────────
#define CONFIG_PATH   "/pocket/config.json"
#define BOOKS_PATH    "/pocket/books"
#define MAPS_PATH     "/pocket/maps"
#define NOTES_PATH    "/pocket/notes"
#define SKETCH_PATH   "/pocket/sketches"
