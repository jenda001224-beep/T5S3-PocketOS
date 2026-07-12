#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>
#include "config.h"

// HAL
#include "hal/epd.h"
#include "hal/touch.h"
#include "hal/storage.h"
#include "hal/lora_hw.h"
#include "hal/gps_hw.h"
#include "hal/power_hw.h"

// UI
#include "ui/canvas.h"
#include "ui/homescreen.h"
#include "ui/controlcenter.h"
#include "ui/statusbar.h"
#include "ui/icons.h"

// OS
#include "os/appmanager.h"

// Apps
#include "apps/ebook.h"
#include "apps/chess.h"
#include "apps/maps.h"
#include "apps/lora_app.h"
#include "apps/strategy.h"
#include "apps/settings.h"
#include "apps/clock_app.h"
#include "apps/notes.h"
#include "apps/calculator.h"
#include "apps/browser.h"
#include "apps/adi.h"
#include "apps/sketch.h"
#include "apps/sudoku.h"
#include "apps/files.h"

// ─── App instances ────────────────────────────────────────────────────────────
static ClockApp      clockApp;
static EbookApp      ebookApp;
static MapsApp       mapsApp;
static ChessApp      chessApp;
static LoRaAppView   loraApp;
static StrategyApp   strategyApp;
static SettingsApp   settingsApp;
static NotesApp      notesApp;
static CalculatorApp calcApp;
static BrowserApp    browserApp;
static AdiApp        adiApp;
static SketchApp     sketchApp;
static SudokuApp     sudokuApp;
static FilesApp      filesApp;

// ─── App registry for home screen (order = appearance on screen) ──────────────
// First 4 entries appear in the dock.
// Remaining entries fill the grid pages.
static const AppEntry APPS[] = {
    // ── Dock ─────────────────────────────────────────────────────────────────
    { "Books",      ICON_BOOK,      EBOOK_APP_ID      },
    { "Maps",       ICON_MAP,       MAPS_APP_ID       },
    { "LoRa",       ICON_LORA,      LORA_APP_ID       },
    { "Settings",   ICON_SETTINGS,  SETTINGS_APP_ID   },
    // ── Page 1 ───────────────────────────────────────────────────────────────
    { "Clock",      ICON_CLOCK,     CLOCK_APP_ID      },
    { "Chess",      ICON_CHESS,     CHESS_APP_ID      },
    { "Conquest",   ICON_STRATEGY,  STRATEGY_APP_ID   },
    { "Browser",    ICON_BROWSER,   BROWSER_APP_ID    },
    { "Notes",      ICON_NOTES,     NOTES_APP_ID      },
    { "Calculator", ICON_CALC,      CALC_APP_ID       },
    { "Device Info",ICON_INFO,      ADI_APP_ID        },
    { "Sketch",     ICON_SKETCH,    SKETCH_APP_ID     },
    { "Sudoku",     ICON_SUDOKU,    SUDOKU_APP_ID     },
    { "Files",      ICON_FILES,     FILES_APP_ID      },
};
static constexpr int APP_COUNT = sizeof(APPS) / sizeof(APPS[0]);

void splashScreen() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);

    // Logo
    int cx = EPD_W / 2, cy = EPD_H / 2 - 40;
    cv.fillRoundRect(cx - 80, cy - 60, 160, 120, 24, C_BLACK);
    cv.drawTextCentered(cx, cy - 14, 200, "Pocket", C_WHITE, C_BLACK, 3);
    cv.drawTextCentered(cx, cy + 18, 200, "OS",     C_WHITE, C_BLACK, 3);

    cv.drawTextCentered(cx, cy + 80, EPD_W, "LILYGO T5 E-Paper S3 Pro", C_MID, C_WHITE, 1);
    cv.drawTextCentered(cx, cy + 96, EPD_W, "v1.1  — Booting...",        C_MID, C_WHITE, 1);

    cv.flushFull();
}

// Boot-time init results, surfaced in every heartbeat for easy diagnostics.
static bool g_sdOk = false, g_touchOk = false;

void setup() {
    Serial.begin(115200);
    // USB CDC needs ~1s to enumerate before output is visible
    delay(1500);
    Serial.println("\n\n=== PocketOS boot (v1.1 FastEPD) ===");
    Serial.printf("[BOOT] chip=%s rev=%d cpu=%dMHz psram=%uKB free=%uKB heap=%uKB\n",
        ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz(),
        ESP.getPsramSize()/1024, ESP.getFreePsram()/1024, ESP.getFreeHeap()/1024);
    Serial.flush();

    // ── Display via FastEPD ───────────────────────────────────────────────────
    Serial.println("[BOOT] EPD init (FastEPD EPDIY_V7)...");
    Serial.flush();
    EPD::instance().begin();
    Serial.printf("[BOOT] EPD done: ok=%d %dx%d\n",
        EPD::instance().ok(), EPD::instance().panelW(), EPD::instance().panelH());
    Serial.flush();

    Serial.println("[BOOT] Canvas alloc...");
    Serial.flush();
    if (!Canvas::instance().begin()) {
        Serial.println("[BOOT] Canvas PSRAM alloc FAILED");
    }
    Serial.flush();
    splashScreen();
    Serial.println("[BOOT] splash drawn");
    Serial.flush();

    // ── Storage ───────────────────────────────────────────────────────────────
    Serial.println("[BOOT] SD init...");
    Serial.flush();
    g_sdOk = Storage::instance().begin();
    Serial.println(g_sdOk ? "[BOOT] SD OK" : "[BOOT] SD mount failed — continuing without SD");
    Serial.flush();

    // ── Touch (shares I2C with FastEPD's expander) ────────────────────────────
    Serial.println("[BOOT] Touch init...");
    Serial.flush();
    g_touchOk = Touch::instance().begin();
    if (!g_touchOk) Serial.println("[BOOT] Touch init FAILED");
    Serial.flush();

    // ── Power: BQ25896 battery ADC + PCF8563 RTC (same I2C bus) ────────────────
    Serial.println("[BOOT] Power (BQ25896/PCF8563) init...");
    Serial.flush();
    Power::instance().begin();
    Serial.printf("[BOOT] Power: bq=%d rtc=%d\n",
        Power::instance().bqPresent(), Power::instance().rtcPresent());
    Serial.flush();

    // ── Load config from SD (needs it now, before any app opens) ──────────────
    SettingsApp::loadConfigStatic();
    EPD::instance().setFrontlight(SettingsApp::config().brightness);

    // ── Timezone + clock: seed system time from the RTC, refine later via NTP ──
    setenv("TZ", SettingsApp::config().timezone, 1);
    tzset();
    {
        struct tm rt;
        if (Power::instance().getTime(rt)) {
            time_t t = mktime(&rt);
            struct timeval tv = { t, 0 };
            settimeofday(&tv, nullptr);
        }
    }

    // ── GPS ───────────────────────────────────────────────────────────────────
    GPSHW::instance().begin();

    // ── LoRa ─────────────────────────────────────────────────────────────────
    if (!LoRaHW::instance().begin()) {
        Serial.println("LoRa init failed");
    } else {
        LoRaHW::instance().setFrequency(SettingsApp::config().loraFreq);
    }

    // ── WiFi: auto-connect if enabled & saved; sync NTP -> RTC when it lands ──
    WiFi.mode(WIFI_STA);
    SystemConfig& cfg = SettingsApp::config();
    if (cfg.wifiEnabled && cfg.wifiSSID[0]) {
        WiFi.begin(cfg.wifiSSID, cfg.wifiPass);
        uint32_t t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 6000) delay(150);
    }
    configTzTime(cfg.timezone, "pool.ntp.org", "time.nist.gov");
    if (WiFi.status() == WL_CONNECTED) {
        // give SNTP a moment, then persist the accurate time into the RTC
        uint32_t t0 = millis();
        while (time(nullptr) < 1700000000 && millis() - t0 < 4000) delay(150);
        if (time(nullptr) > 1700000000) {
            time_t now = time(nullptr);
            struct tm lt; localtime_r(&now, &lt);
            Power::instance().setTime(lt);
        }
    }

    // ── Register apps ─────────────────────────────────────────────────────────
    AppManager& am = AppManager::instance();
    am.registerApp(&clockApp);
    am.registerApp(&ebookApp);
    am.registerApp(&mapsApp);
    am.registerApp(&chessApp);
    am.registerApp(&loraApp);
    am.registerApp(&strategyApp);
    am.registerApp(&settingsApp);
    am.registerApp(&notesApp);
    am.registerApp(&calcApp);
    am.registerApp(&browserApp);
    am.registerApp(&adiApp);
    am.registerApp(&sketchApp);
    am.registerApp(&sudokuApp);
    am.registerApp(&filesApp);

    // ── Home screen ───────────────────────────────────────────────────────────
    HomeScreen::instance().begin(APPS, APP_COUNT);

    // ── Start ─────────────────────────────────────────────────────────────────
    Serial.println("[BOOT] setup complete — entering loop, drawing home screen");
    Serial.flush();
    delay(800);  // let splash breathe
    am.begin();  // shows home screen
}

void loop() {
    AppManager::instance().loop();

    // Heartbeat every 3 s so the serial monitor always shows the device is alive
    // (and where it is) even when the e-paper isn't visibly changing.
    static uint32_t lastHB = 0;
    if (millis() - lastHB > 3000) {
        lastHB = millis();
        Serial.printf("[HB] up=%lus epd=%d(%dx%d) sd=%d touch=%d bq=%d rtc=%d wifi=%d heap=%uKB psram=%uKB\n",
            millis()/1000, EPD::instance().ok(),
            EPD::instance().panelW(), EPD::instance().panelH(),
            g_sdOk, g_touchOk, Power::instance().bqPresent(), Power::instance().rtcPresent(),
            WiFi.status() == WL_CONNECTED,
            ESP.getFreeHeap()/1024, ESP.getFreePsram()/1024);
        Serial.flush();
    }

    // Keep touch polling fast, yield to RTOS between frames
    vTaskDelay(pdMS_TO_TICKS(16));
}
