#pragma once
#include "app.h"

#define LORA_APP_ID  4

struct ChatMessage {
    char    text[128];
    bool    outgoing;
    uint32_t ts;
    int8_t  rssi;
};

class LoRaAppView : public App {
public:
    LoRaAppView() : App("LoRa", LORA_APP_ID) {}

    void onEnter() override;
    void draw()    override;
    void handleEvent(const TouchEvent& ev) override;
    void tick()    override;

private:
    void drawMessages();
    void drawInput();
    void sendMessage();
    void addMessage(const char* text, bool outgoing, int8_t rssi = 0);

    static constexpr int MAX_MSG = 20;
    ChatMessage _msgs[MAX_MSG];
    int         _msgCount = 0;
    char        _input[64] = "";
    int         _inputLen  = 0;
    bool        _keyboardVisible = false;
    int         _scroll = 0;

    // Simple on-screen keyboard state
    int  _kbdKey = -1;
    static const char* KBD_ROWS[];
};
