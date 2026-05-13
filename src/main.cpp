#include <Arduino.h>
#include "config.h"

// HAL
#include "hal/epd.h"
#include "hal/touch.h"
#include "hal/storage.h"
#include "hal/lora_hw.h"
#include "hal/gps_hw.h"

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
    { "Weather",    ICON_WEATHER,   CLOCK_APP_ID      },  // placeholder
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

    cv.drawTextCentered(cx, cy + 80, EPD_W, "LILYGO T5S3 4.7\" E-Paper", C_MID, C_WHITE, 1);
    cv.drawTextCentered(cx, cy + 96, EPD_W, "v1.0  — Booting...",        C_MID, C_WHITE, 1);

    cv.flushFull();
}

void setup() {
    Serial.begin(115200);

    // ── Display first (so we can show status) ─────────────────────────────────
    if (!EPD::instance().begin()) {
        Serial.println("EPD INIT FAILED");
        while (true) delay(1000);
    }
    Canvas::instance().begin();
    splashScreen();

    // ── Storage ───────────────────────────────────────────────────────────────
    if (!Storage::instance().begin()) {
        Serial.println("SD mount failed — continuing without SD");
    }

    // ── Touch ─────────────────────────────────────────────────────────────────
    if (!Touch::instance().begin()) {
        Serial.println("Touch init failed");
    }

    // ── Load config (needs SD) ────────────────────────────────────────────────
    SettingsApp::config();  // triggers loadConfig via onEnter first call

    // ── Apply saved brightness ────────────────────────────────────────────────
    EPD::instance().setFrontlight(SettingsApp::config().brightness);

    // ── GPS ───────────────────────────────────────────────────────────────────
    GPSHW::instance().begin();

    // ── LoRa ─────────────────────────────────────────────────────────────────
    if (!LoRaHW::instance().begin()) {
        Serial.println("LoRa init failed");
    }

    // ── WiFi (lazy — browser app connects on demand) ──────────────────────────
    WiFi.mode(WIFI_STA);

    // ── NTP time sync ─────────────────────────────────────────────────────────
    configTzTime(SettingsApp::config().timezone, "pool.ntp.org", "time.nist.gov");

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

    // ── Home screen ───────────────────────────────────────────────────────────
    HomeScreen::instance().begin(APPS, APP_COUNT);

    // ── Start ─────────────────────────────────────────────────────────────────
    delay(800);  // let splash breathe
    am.begin();  // shows home screen
}

void loop() {
    AppManager::instance().loop();

    // Keep touch polling fast, yield to RTOS between frames
    vTaskDelay(pdMS_TO_TICKS(16));
}
