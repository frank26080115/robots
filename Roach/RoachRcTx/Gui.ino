#include "MenuClass.h"

uint32_t gui_last_draw_time = 0;

void gui_init(void)
{
    oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    oled.setRotation(0);
    oled.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    oled.setCursor(0, 0);
    oled.setTextWrap(false);
    gui_drawSplash();
    menu_setup();
}

void gui_reboot(void)
{
    oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
}

extern const uint8_t splash[];

void gui_drawSplash(void)
{
    oled.clearDisplay();
    oled.drawBitmap(0, 0, splash, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    gui_drawNow();
}

bool gui_canDisplay(void)
{
    if (nbtwi_isBusy() != false) {
        return false;
    }
    if (radio.is_busy())
    {
        uint32_t now = millis();
        if ((now - gui_last_draw_time) >= 80) {
            gui_last_draw_time = now;
            return true;
        }
    }
    return false;
}

void gui_drawWait(void)
{
    while (nbtwi_isBusy()) {
        yield();
        ctrler_tasks();
    }
}

void gui_drawNow(void)
{
    oled.display();
    while (nbtwi_isBusy()) {
        yield();
        ctrler_tasks();
    }
}

void showError(const char* s)
{
    btns_disableAll();
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.print("ERROR:");
    oled.setCursor(0, ROACHGUI_LINE_HEIGHT);
    oled.print(s);
    oled.display();
    uint32_t t = millis();
    while ((millis() - t) < ROACHGUI_ERROR_SHOW_TIME) {
        yield();
        ctrler_tasks();
        if (btns_hasAnyPressed()) {
            break;
        }
    }
}

void showMessage(const char* s1, const char* s2)
{
    btns_disableAll();
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.print(s1);
    oled.setCursor(0, ROACHGUI_LINE_HEIGHT);
    oled.print(s2);
    oled.display();
    uint32_t t = millis();
    while ((millis() - t) < ROACHGUI_ERROR_SHOW_TIME) {
        yield();
        ctrler_tasks();
        if (btns_hasAnyPressed()) {
            break;
        }
    }
}

void printEllipses(void)
{
    uint8_t ticks;
    uint32_t now = millis();
    uint32_t now_mod = now % 1500;
    ticks = (now_mod / 500) + 1;
    uint8_t i;
    for (i = 0; i < 3; i++)
    {
        oled.write(i < ticks ? '.' : ' ');
    }
}

void drawSideBar(const char* upper, const char* lower, bool line)
{
    int i, x, y;
    int slen;
    if (upper != NULL)
    {
        x = SCREEN_WIDTH - 5;
        slen = strlen(upper);
        for (i = 0, y = 0; i < slen; i++, y += ROACHGUI_LINE_HEIGHT)
        {
            oled.setCursor(x, y);
            oled.write(upper[i]);
        }
    }
    if (lower != NULL)
    {
        slen = strlen(lower);
        x = SCREEN_WIDTH - 10;
        y = SCREEN_HEIGHT - (slen * ROACHGUI_LINE_HEIGHT);
        for (i = 0; i < slen; i++, y += ROACHGUI_LINE_HEIGHT)
        {
            oled.setCursor(x, y);
            oled.write(lower[i]);
        }
    }
    if (line)
    {
        x = SCREEN_WIDTH - 12;
        oled.drawFastVLine(x, 0, SCREEN_HEIGHT, 1);
    }
}

void drawTitleBar(const char* s, bool center, bool line, bool arrows)
{
    int slen = strlen(s);
    int x, y;
    x = center ? (((SCREEN_WIDTH - 12) / 2) - ((slen * 6) / 2)) : 0;
    oled.setCursor(x, 0);
    oled.print(s);
    if (arrows)
    {
        oled.setCursor(0, 0);
        oled.write((char)0x1B);
        oled.setCursor(SCREEN_WIDTH - 12 - 5, 0);
        oled.write((char)0x1A);
    }
    if (line)
    {
        y = 9;
        oled.drawFastHLine(0, y, SCREEN_WIDTH - 12, 1);
    }
}

void drawProgressBar(int x, int y, int w, int h, int prog, int total)
{
    oled.drawRect(x, y, w, h, SSD1306_WHITE);
    oled.fillRect(x, y, map(prog, 0, total, 0, w), h, SSD1306_WHITE);
}
