#pragma once
#include <Arduino.h>

// ─── Display IT8951 (SPI0) ────────────────────────────────────────────────────
#define EPD_MOSI    11
#define EPD_MISO    13
#define EPD_SCK     12
#define EPD_CS      15
#define EPD_RST     16
#define EPD_BUSY    17
#define EPD_W       960
#define EPD_H       540

// ─── Frontlight LED ───────────────────────────────────────────────────────────
#define FRONTLIGHT_PIN     48
#define FRONTLIGHT_CH      0
#define FRONTLIGHT_FREQ    5000
#define FRONTLIGHT_RES     8

// ─── Touch GT911 (I2C) ────────────────────────────────────────────────────────
#define TOUCH_SDA   21
#define TOUCH_SCL   22
#define TOUCH_INT   38
#define TOUCH_RST   -1
#define TOUCH_ADDR  0x14   // alt: 0x5D

// ─── SD Card (shared SPI0 bus) ────────────────────────────────────────────────
#define SD_CS       10

// ─── LoRa SX1262 (SPI1 separate bus) ─────────────────────────────────────────
#define LORA_SCK    40
#define LORA_MOSI   41
#define LORA_MISO   42
#define LORA_CS      8
#define LORA_RST    18
#define LORA_DIO1   45
#define LORA_BUSY   46
#define LORA_FREQ   868.0   // MHz — change to 915.0 for US

// ─── GPS UART2 ───────────────────────────────────────────────────────────────
#define GPS_RX      44
#define GPS_TX      43
#define GPS_BAUD    9600

// ─── Battery ADC ─────────────────────────────────────────────────────────────
#define BATTERY_ADC_PIN   1
#define BATTERY_MIN_MV    3300
#define BATTERY_MAX_MV    4200
#define BATTERY_R1        100  // kOhm voltage divider
#define BATTERY_R2        100

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

// ─── Config file on SD ────────────────────────────────────────────────────────
#define CONFIG_PATH   "/pocket/config.json"
#define BOOKS_PATH    "/pocket/books"
#define MAPS_PATH     "/pocket/maps"
#define NOTES_PATH    "/pocket/notes"
