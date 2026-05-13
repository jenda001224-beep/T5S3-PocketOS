#include "lora_app.h"
#include "../ui/canvas.h"
#include "../hal/lora_hw.h"
#include "../os/appmanager.h"

const char* LoRaAppView::KBD_ROWS[] = {
    "QWERTYUIOP",
    "ASDFGHJKL",
    "ZXCVBNM.,!",
    " <SEND",
};

void LoRaAppView::onEnter() {
    _keyboardVisible = true;
    draw();
}

void LoRaAppView::addMessage(const char* text, bool outgoing, int8_t rssi) {
    if (_msgCount >= MAX_MSG) {
        memmove(_msgs, _msgs + 1, (MAX_MSG - 1) * sizeof(ChatMessage));
        _msgCount = MAX_MSG - 1;
    }
    ChatMessage& m = _msgs[_msgCount++];
    strncpy(m.text, text, 127);
    m.outgoing = outgoing;
    m.ts       = millis();
    m.rssi     = rssi;
}

void LoRaAppView::tick() {
    LoRaHW& lora = LoRaHW::instance();
    if (lora.available()) {
        LoRaPacket pkt = lora.receive();
        addMessage(pkt.text.c_str(), false, pkt.rssi);
        draw();
    }
}

void LoRaAppView::sendMessage() {
    if (_inputLen == 0) return;
    _input[_inputLen] = '\0';
    LoRaHW::instance().send(String(_input));
    addMessage(_input, true);
    _input[0] = '\0';
    _inputLen  = 0;
    draw();
}

void LoRaAppView::drawMessages() {
    Canvas& cv = Canvas::instance();
    int msgH   = 36;
    int chatY  = STATUS_BAR_H;
    int chatH  = _keyboardVisible ? (EPD_H / 2 - chatY) : (EPD_H - DOCK_H - chatY);

    cv.fillRect(0, chatY, EPD_W, chatH, C_WHITE);

    // Status bar for LoRa signal
    char sig[32];
    snprintf(sig, sizeof(sig), "LoRa %s  RSSI:%d", LoRaHW::instance().ready() ? "OK" : "ERR", LoRaHW::instance().rssi());
    cv.fillRect(0, chatY, EPD_W, 20, C_LIGHT);
    cv.drawText(10, chatY + 6, sig, C_BLACK, C_LIGHT, 1);

    int startY = chatY + 24;
    int visCount = (chatH - 24) / msgH;
    int startIdx = max(0, _msgCount - visCount - _scroll);

    for (int i = startIdx; i < _msgCount; i++) {
        const ChatMessage& m = _msgs[i];
        int y = startY + (i - startIdx) * msgH;

        if (m.outgoing) {
            cv.fillRoundRect(EPD_W/2, y+2, EPD_W/2 - 10, msgH-4, 6, C_MID);
            cv.drawText(EPD_W/2 + 8, y + 12, m.text, C_BLACK, C_MID, 1);
        } else {
            cv.fillRoundRect(10, y+2, EPD_W/2 - 20, msgH-4, 6, C_LIGHT);
            cv.drawText(18, y + 12, m.text, C_BLACK, C_LIGHT, 1);
            char rss[8]; snprintf(rss, sizeof(rss), "%d", m.rssi);
            cv.drawText(10, y+2, rss, C_DARK, C_WHITE, 1);
        }
    }
}

void LoRaAppView::drawInput() {
    Canvas& cv = Canvas::instance();
    if (!_keyboardVisible) return;

    int kbdY  = EPD_H / 2;
    int kbdH  = EPD_H - kbdY;
    cv.fillRect(0, kbdY, EPD_W, kbdH, C_LIGHT);

    // Input field
    cv.fillRect(0, kbdY, EPD_W, 28, C_WHITE);
    cv.drawRect(0, kbdY, EPD_W, 28, C_MID);
    char display[66] = "> ";
    strncat(display, _input, 63);
    strncat(display, "_", 1);
    cv.drawText(8, kbdY + 8, display, C_BLACK, C_WHITE, 1);

    // Keyboard rows
    int rowH = (kbdH - 28) / 4;
    for (int row = 0; row < 4; row++) {
        const char* keys = KBD_ROWS[row];
        int nkeys = strlen(keys);
        int keyW  = EPD_W / nkeys;
        for (int k = 0; k < nkeys; k++) {
            int kx = k * keyW;
            int ky = kbdY + 28 + row * rowH;
            cv.fillRoundRect(kx+2, ky+2, keyW-4, rowH-4, 4, C_WHITE);
            cv.drawRoundRect(kx+2, ky+2, keyW-4, rowH-4, 4, C_MID);
            char ch[2] = { keys[k], 0 };
            cv.drawTextCentered(kx + keyW/2, ky + rowH/2 - 4, keyW, ch, C_BLACK, C_WHITE, 1);
        }
    }
}

void LoRaAppView::draw() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("LoRa Messenger");
    drawMessages();
    drawInput();
    cv.flushFull();
}

void LoRaAppView::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) { AppManager::instance().goHome(); return; }

    if (ev.type == TouchEvent::TAP) {
        int kbdY = EPD_H / 2;
        if (ev.y >= kbdY + 28 && _keyboardVisible) {
            // Keyboard hit
            int rowH  = (EPD_H - kbdY - 28) / 4;
            int row   = (ev.y - kbdY - 28) / rowH;
            if (row >= 0 && row < 4) {
                const char* keys = KBD_ROWS[row];
                int nkeys = strlen(keys);
                int keyW  = EPD_W / nkeys;
                int col   = ev.x / keyW;
                if (col >= 0 && col < nkeys) {
                    char k = keys[col];
                    if (k == '<') {
                        if (_inputLen > 0) _input[--_inputLen] = '\0';
                    } else if (k == 'S' && row == 3) {
                        sendMessage();
                        return;
                    } else if (_inputLen < 63) {
                        _input[_inputLen++] = (k == ' ') ? ' ' : k;
                        _input[_inputLen]   = '\0';
                    }
                    draw();
                }
            }
        }
    }
    if (ev.type == TouchEvent::SWIPE_UP)   { _scroll = max(0, _scroll - 1); draw(); }
    if (ev.type == TouchEvent::SWIPE_DOWN) { _scroll++;                      draw(); }
}
