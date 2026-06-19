#include "core/powerSave.h"
#include <interface.h>

unsigned long lastActivityTime = 0;

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(R_BTN, INPUT_PULLUP);
    pinMode(L_BTN, INPUT_PULLUP);
    pinMode(BK_BTN, INPUT_PULLUP);

    bruceConfig.colorInverted = 0;
    bruceConfigPins.rotation = 3;
    
    lastActivityTime = millis();
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    pinMode(TFT_BL, OUTPUT);
    ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcWrite(TFT_BL, 255);
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() { return 70; }

/*********************************************************************
** Function: _setBrightness(uint8_t brightval)
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    int dutyCycle;
    if (brightval == 100) dutyCycle = 255;
    else if (brightval == 75) dutyCycle = 130;
    else if (brightval == 50) dutyCycle = 70;
    else if (brightval == 25) dutyCycle = 20;
    else if (brightval == 0) dutyCycle = 0;
    else dutyCycle = ((brightval * 255) / 100);

    // log_i("dutyCycle for bright 0-255: %d", dutyCycle);
    ledcWrite(TFT_BL, dutyCycle);
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = millis();
    if (!(millis() - tm > 200 || LongPress)) return;
    
    bool s = digitalRead(SEL_BTN);
    bool r = digitalRead(R_BTN);
    bool l = digitalRead(L_BTN);
    bool b = digitalRead(BK_BTN);
    if (!s || !r || !l || !b) {
        tm = millis();
        lastActivityTime = millis();
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    }
    if (!s) SelPress = true;
    if (!r) NextPress = true;
    if (!l) PrevPress = true;
    if (!b) {
        EscPress = true;
        DownPress = true;
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    tft.fillScreen(bruceConfig.bgColor);
    digitalWrite(TFT_BL, LOW);
    tft.writecommand(0x10);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {
    if (millis() - lastActivityTime >= 900000) {
        tft.fillScreen(bruceConfig.bgColor);
        tft.setTextColor(bruceConfig.priColor);
        tft.setTextSize(2);
        tft.drawCentreString("SLEEPING...", tftWidth / 2, tftHeight / 2, 1);
        delay(2000); 
        powerOff();
    }
    if (!digitalRead(SEL_BTN) && !digitalRead(BK_BTN)) {
        uint32_t startTime = millis();
        int lastProgress = -1;
        while (!digitalRead(SEL_BTN) && !digitalRead(BK_BTN)) {
            uint32_t held = millis() - startTime;
            if (held > 500) {
                tft.fillScreen(bruceConfig.bgColor);
                tft.setTextColor(
                    bruceConfig.priColor,
                    bruceConfig.bgColor
                );
                tft.setTextSize(2);
                tft.drawCentreString(
                    "SHUT DOWN",
                    tftWidth / 2,
                    tftHeight / 2 - 30,
                    1
                );
                tft.setTextSize(FM);
                tft.drawCentreString(
                    "Release to cancel",
                    tftWidth / 2,
                    tftHeight / 2 + 12,
                    1
                );
                tft.drawRect(
                    20,
                    tftHeight / 2 - 2,
                    tftWidth - 40,
                    8,
                    bruceConfig.priColor
                );
                int progress = min(
                    100,
                    (int)((held * 100UL) / 3000UL)
                );
                if (progress != lastProgress) {
                    lastProgress = progress;
                    int totalWidth = tftWidth - 42;
                    int progressWidth = (totalWidth * progress) / 100;
                    int remainderWidth = totalWidth - progressWidth;
                    int barY = tftHeight / 2 - 1;
                    if (progressWidth > 0) {
                        tft.fillRect(21, barY, progressWidth, 6, bruceConfig.priColor);
                    }
                    if (remainderWidth > 0) {
                        tft.fillRect(21 + progressWidth, barY, remainderWidth, 6, bruceConfig.bgColor);
                    }
                }
                int secLeft = max(
                    0,
                    3 - (int)(held / 1000)
                );
                tft.drawCentreString(
                    String(secLeft + 1) + "s",
                    tftWidth / 2,
                    tftHeight / 2 - 10,
                    1
                );
            }

            if (held >= 3000) {
                tft.drawCentreString(
                    "Release to Shutdown",
                    tftWidth / 2,
                    tftHeight / 2,
                    1
                );
                while (!digitalRead(SEL_BTN) || !digitalRead(BK_BTN))
                    delay(10);
                
                tft.fillScreen(bruceConfig.bgColor);
                tft.setTextColor(bruceConfig.priColor);

                for (int i = 3; i >= 1; i--) {
                    tft.fillScreen(bruceConfig.bgColor);
                    tft.setTextSize(2);
                    tft.drawCentreString(
                        "TURNING OFF",
                        tftWidth / 2,
                        20,
                        1
                    );
                    tft.setTextSize(3);
                    tft.drawCentreString(
                        String(i),
                        tftWidth / 2,
                        tftHeight / 2 - 15,
                        1
                    );
                    delay(1000);
                }
                tft.fillScreen(bruceConfig.bgColor);
                tft.setTextSize(2);
                tft.drawCentreString(
                    "SHUTTING DOWN",
                    tftWidth / 2,
                    tftHeight / 2 - 5,
                    1
                );
                delay(1000);
                powerOff();
                return;
            }
            delay(10);
        }
        if (millis() - startTime > 500) {
            tft.fillRect(
                0,
                tftHeight / 2 - 30,
                tftWidth,
                60,
                bruceConfig.bgColor
            );
            drawStatusBar();
        }
    }
}
