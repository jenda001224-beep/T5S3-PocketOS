#include "adi.h"
#include "settings.h"
#include "../ui/canvas.h"
#include "../os/appmanager.h"
#include "../hal/storage.h"
#include "../hal/power_hw.h"
#include "../hal/gps_hw.h"
#include "../hal/lora_hw.h"
#include "../hal/epd.h"
#include <WiFi.h>
#include <esp_system.h>
#include <esp_chip_info.h>

void AdiApp::onEnter() {
    _scroll = 0;
    build();
    draw();
}

void AdiApp::tick() {
    build();      // live refresh
    draw();
}

void AdiApp::section(const char* title) {
    if (_n >= MAX_LINES) return;
    _isHeader[_n] = true;
    _lines[_n++]  = title;
}

void AdiApp::add(const char* k, const String& v) {
    if (_n >= MAX_LINES) return;
    _isHeader[_n] = false;
    _lines[_n++]  = String(k) + "\t" + v;
}

static String kib(uint32_t b)  { return String(b / 1024.0f, 1) + " KB"; }
static String mib(uint32_t b)  { return String(b / (1024.0f*1024.0f), 2) + " MB"; }

static const char* resetReasonStr(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON:  return "Power-on";
        case ESP_RST_SW:       return "Software";
        case ESP_RST_PANIC:    return "Panic";
        case ESP_RST_INT_WDT:  return "Int watchdog";
        case ESP_RST_TASK_WDT: return "Task watchdog";
        case ESP_RST_WDT:      return "Other watchdog";
        case ESP_RST_DEEPSLEEP:return "Deep sleep wake";
        case ESP_RST_BROWNOUT: return "Brownout";
        case ESP_RST_SDIO:     return "SDIO";
        default:               return "Unknown";
    }
}

void AdiApp::build() {
    _n = 0;
    SystemConfig& cfg = SettingsApp::config();

    // ── SoC ───────────────────────────────────────────────────────────────────
    section("System on Chip");
    esp_chip_info_t ci; esp_chip_info(&ci);
    add("Chip",      ESP.getChipModel());
    add("Revision",  String(ESP.getChipRevision()));
    add("Cores",     String(ci.cores));
    add("CPU freq",  String(ESP.getCpuFreqMHz()) + " MHz");
    uint64_t mac = ESP.getEfuseMac();
    char macs[24];
    snprintf(macs, sizeof(macs), "%02X:%02X:%02X:%02X:%02X:%02X",
        (uint8_t)(mac>>40), (uint8_t)(mac>>32), (uint8_t)(mac>>24),
        (uint8_t)(mac>>16), (uint8_t)(mac>>8), (uint8_t)mac);
    add("eFuse MAC", macs);
    add("SDK",       ESP.getSdkVersion());
    add("Reset",     resetReasonStr(esp_reset_reason()));
    float tc = temperatureRead();
    add("Die temp",  String(tc, 1) + " C");
    uint32_t up = millis() / 1000;
    char ub[24]; snprintf(ub, sizeof(ub), "%luh %02lum %02lus", up/3600, (up%3600)/60, up%60);
    add("Uptime",    ub);

    // ── Memory ────────────────────────────────────────────────────────────────
    section("Memory");
    add("Heap total", kib(ESP.getHeapSize()));
    add("Heap free",  kib(ESP.getFreeHeap()));
    add("Heap min",   kib(ESP.getMinFreeHeap()));
    add("Max alloc",  kib(ESP.getMaxAllocHeap()));
    add("PSRAM",      mib(ESP.getPsramSize()));
    add("PSRAM free", mib(ESP.getFreePsram()));
    add("Flash",      mib(ESP.getFlashChipSize()));
    add("Flash speed",String(ESP.getFlashChipSpeed()/1000000) + " MHz");
    add("Sketch",     kib(ESP.getSketchSize()));
    add("Free sketch",kib(ESP.getFreeSketchSpace()));

    // ── Power / battery ───────────────────────────────────────────────────────
    section("Power / Battery (BQ25896)");
    Power& pw = Power::instance();
    if (pw.bqPresent()) {
        BatteryInfo b = pw.battery();
        add("Battery",   String(b.millivolts) + " mV  (" + String(b.percent) + "%)");
        const char* cs = (b.chargeStat==0)?"Not charging":(b.chargeStat==1)?"Pre-charge":
                         (b.chargeStat==2)?"Fast charge":"Charge done";
        add("Charge",    cs);
        add("USB / VBUS",b.usb ? (String("Yes  ") + b.vbusMv + " mV") : String("No"));
        add("Charge cur",String(b.chargeMa) + " mA");
        add("System",    String(b.sysMv) + " mV");
    } else {
        add("BQ25896", "not detected");
    }

    // ── Radios ────────────────────────────────────────────────────────────────
    section("Wi-Fi");
    if (WiFi.status() == WL_CONNECTED) {
        add("Status", "Connected");
        add("SSID",   WiFi.SSID());
        add("IP",     WiFi.localIP().toString());
        add("RSSI",   String(WiFi.RSSI()) + " dBm");
        add("Channel",String(WiFi.channel()));
        add("MAC",    WiFi.macAddress());
    } else {
        add("Status", cfg.wifiEnabled ? "Enabled (not connected)" : "Disabled");
        if (cfg.wifiSSID[0]) add("Saved SSID", cfg.wifiSSID);
    }

    section("LoRa (SX1262)");
    LoRaHW& lr = LoRaHW::instance();
    add("Radio",    lr.ready() ? (lr.enabled() ? "Ready" : "Sleeping") : "Init failed");
    add("Frequency",String(cfg.loraFreq, 1) + " MHz");
    add("Last RSSI",String(lr.rssi()) + " dBm");
    add("Last SNR",  String(lr.snr(), 1) + " dB");

    section("GPS (MIA-M10Q)");
    GPSHW& gp = GPSHW::instance();
    if (gp.hasFix()) {
        GPSFix f = gp.fix();
        add("Fix",      "Yes");
        add("Satellites",String(f.sats));
        add("Latitude", String(f.lat, 6));
        add("Longitude",String(f.lng, 6));
        add("Altitude", String(f.alt, 0) + " m");
        add("Speed",    String(f.speed, 1) + " km/h");
    } else {
        add("Fix", cfg.gpsEnabled ? "Searching…" : "GPS off");
    }

    // ── Storage / clock / display ─────────────────────────────────────────────
    section("Storage");
    Storage& sd = Storage::instance();
    if (sd.mounted()) {
        add("SD total", mib(sd.totalBytes()));
        add("SD used",  mib(sd.usedBytes()));
        add("SD free",  mib(sd.totalBytes() - sd.usedBytes()));
    } else {
        add("SD card", "not mounted");
    }

    section("Clock (PCF8563)");
    struct tm rt;
    if (pw.rtcPresent() && pw.getTime(rt)) {
        char ts[40];
        snprintf(ts, sizeof(ts), "%04d-%02d-%02d %02d:%02d:%02d",
            rt.tm_year+1900, rt.tm_mon+1, rt.tm_mday, rt.tm_hour, rt.tm_min, rt.tm_sec);
        add("RTC time", ts);
    } else {
        add("RTC", pw.rtcPresent() ? "time invalid" : "not detected");
    }
    time_t now = time(nullptr);
    struct tm* lt = localtime(&now);
    char sysclk[40];
    snprintf(sysclk, sizeof(sysclk), "%04d-%02d-%02d %02d:%02d:%02d",
        lt->tm_year+1900, lt->tm_mon+1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
    add("System time", sysclk);

    section("Display");
    add("Panel",   "ED047TC1 4.7\" 16-gray");
    add("Driver",  "FastEPD (EPDIY_V7)");
    add("Resolution", String(EPD::instance().panelW()) + "x" + String(EPD::instance().panelH()));
    add("Init",    EPD::instance().ok() ? "OK" : "warn");
}

void AdiApp::draw() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Device Info");

    int y      = STATUS_BAR_H + 6;
    int lineH  = 15;
    int bottom = EPD_H - DOCK_H - 4;
    int maxVis = (bottom - y) / lineH;

    for (int i = _scroll; i < _n && (i - _scroll) < maxVis; i++) {
        int ly = y + (i - _scroll) * lineH;
        if (_isHeader[i]) {
            cv.fillRect(0, ly - 1, EPD_W, lineH, C_LIGHT);
            cv.drawText(10, ly + 2, _lines[i].c_str(), C_BLACK, C_LIGHT, 1);
        } else {
            int tab = _lines[i].indexOf('\t');
            String k = tab >= 0 ? _lines[i].substring(0, tab) : _lines[i];
            String v = tab >= 0 ? _lines[i].substring(tab + 1) : "";
            cv.drawText(20, ly + 2, k.c_str(), C_MID,   C_WHITE, 1);
            cv.drawText(300, ly + 2, v.c_str(), C_BLACK, C_WHITE, 1);
        }
    }

    // scrollbar
    if (_n > maxVis) {
        int trackH = bottom - y;
        int barH   = max(16, trackH * maxVis / _n);
        int barY   = y + (trackH - barH) * _scroll / max(1, _n - maxVis);
        cv.fillRect(EPD_W - 5, y, 3, trackH, C_LIGHT);
        cv.fillRect(EPD_W - 5, barY, 3, barH, C_MID);
    }

    cv.drawText(10, bottom + 2, "swipe up/down to scroll", C_MID, C_WHITE, 1);
    cv.flushFull();
}

void AdiApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) { AppManager::instance().goHome(); return; }

    int lineH  = 15;
    int maxVis = (EPD_H - DOCK_H - 4 - (STATUS_BAR_H + 6)) / lineH;
    int step   = maxVis - 2;
    if (ev.type == TouchEvent::SWIPE_UP)   { _scroll = min(_scroll + step, max(0, _n - maxVis)); draw(); }
    if (ev.type == TouchEvent::SWIPE_DOWN) { _scroll = max(0, _scroll - step); draw(); }
}
