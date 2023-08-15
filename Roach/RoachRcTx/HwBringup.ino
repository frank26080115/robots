void hw_bringup(void)
{
    pinMode(ROACHHW_PIN_LED_RED, OUTPUT);
    Serial.begin(115200);

#if 0

    pinMode(ROACHHW_PIN_BTN_UP, INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_DOWN, INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_LEFT, INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_RIGHT, INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_CENTER, INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_G5, INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_G6, INPUT_PULLUP);

    uint8_t px = 0;
    while (1)
    {
        static uint32_t lt = 0;
        now = millis();
        digitalWrite(ROACHHW_PIN_LED_RED, ((now % 500) <= 100));
        //continue;
        uint8_t x = 0;
        x |= (digitalRead(ROACHHW_PIN_BTN_UP) == LOW ? 1 : 0) << 0;
        x |= (digitalRead(ROACHHW_PIN_BTN_DOWN) == LOW ? 1 : 0) << 1;
        x |= (digitalRead(ROACHHW_PIN_BTN_LEFT) == LOW ? 1 : 0) << 2;
        x |= (digitalRead(ROACHHW_PIN_BTN_RIGHT) == LOW ? 1 : 0) << 3;
        x |= (digitalRead(ROACHHW_PIN_BTN_CENTER) == LOW ? 1 : 0) << 4;
        x |= (digitalRead(ROACHHW_PIN_BTN_G5) == LOW ? 1 : 0) << 5;
        x |= (digitalRead(ROACHHW_PIN_BTN_G6) == LOW ? 1 : 0) << 6;
        if (x != px && (now - lt) >= 100)
        {
            Serial.printf("[%u]: 0x%02X\r\n", millis(), x);
            px = x;
            lt = now;
        }
        yield();
    }

#endif

    RoachButton_allBegin();
    RoachPot_allBegin();
    RoachEnc_begin(ROACHHW_PIN_ENC_A, ROACHHW_PIN_ENC_B);
    nbtwi_init(ROACHHW_PIN_I2C_SCL, ROACHHW_PIN_I2C_SDA, (SCREEN_WIDTH * SCREEN_HEIGHT / 8) * 2);
    Serial.println("\r\nRoach RC Transmitter - HW Bringup!");
    while (true)
    {
        hw_report();
    }
}

void hw_report(void)
{
    uint32_t now;
    static uint32_t lt = 0;
    static uint32_t pb = 0;
    now = millis();

    bool need_show = false;
    if ((now - lt) >= 200)
    {
        need_show = true;
    }

    RoachButton_allTask();
    RoachPot_allTask();
    RoachEnc_task();

    uint32_t b = RoachButton_isAnyHeld();
    if (b != pb)
    {
        need_show = true;
        pb = b;
    }

    if (RoachEnc_hasMoved(true))
    {
        need_show = true;
    }

    if (need_show)
    {
        lt = now;
        Serial.printf("[%u]: 0x%08X, %d, ", now, b, RoachEnc_get(false));

        int i;
        #if 1
        for (i = 0; i < POT_CNT_MAX; i++)
        {
            int x = RoachPot_getRawAtIdx(i);
            if (x >= 0) {
                Serial.printf("%d, ", x);
            }
            else {
                break;
            }
        }
        #else
        Serial.printf("%d, ", analogRead(ROACHHW_PIN_POT_THROTTLE));
        Serial.printf("%d, ", analogRead(ROACHHW_PIN_POT_STEERING));
        Serial.printf("%d, ", analogRead(ROACHHW_PIN_POT_WEAPON));
        Serial.printf("%d, ", analogRead(ROACHHW_PIN_POT_AUX));
        Serial.printf("%d, ", analogRead(ROACHHW_PIN_POT_BATTERY));
        #endif
        Serial.printf("\r\n");
    }

    digitalWrite(ROACHHW_PIN_LED_RED, ((now % 500) <= 100));
    yield();
}