void safeboot_check(void)
{
    pinMode(ROACHHW_PIN_BTN_UP    , INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_DOWN  , INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_CENTER, INPUT_PULLUP);

    if ((digitalRead(ROACHHW_PIN_BTN_UP) == LOW || digitalRead(ROACHHW_PIN_BTN_DOWN) == LOW) || digitalRead(ROACHHW_PIN_BTN_CENTER) == LOW)
    {
        pinMode(ROACHHW_PIN_LED_RED, OUTPUT);
        pinMode(ROACHHW_PIN_LED_BLU, OUTPUT);

        uint32_t now = millis();
        uint32_t tstart = now;
        while (digitalRead(ROACHHW_PIN_BTN_UP) == LOW || digitalRead(ROACHHW_PIN_BTN_DOWN) == LOW || digitalRead(ROACHHW_PIN_BTN_CENTER) == LOW)
        {
            now = millis();

            bool blynk = ((now % 200) <= 100);
            digitalWrite(ROACHHW_PIN_LED_RED, blynk ? HIGH : LOW );
            digitalWrite(ROACHHW_PIN_LED_BLU, blynk ? LOW  : HIGH);

            if ((now - tstart) >= 500)
            {
                if (RoachUsbMsd_hasVbus() == false)
                {
                    return;
                }
            }
            if ((now - tstart) >= 3000)
            {
                if (digitalRead(ROACHHW_PIN_BTN_UP) == LOW)
                {
                    enterUf2Dfu();
                }
                else if (digitalRead(ROACHHW_PIN_BTN_DOWN) == LOW)
                {
                    enterSerialDfu();
                }
                else if (digitalRead(ROACHHW_PIN_BTN_CENTER) == LOW)
                {
                    // just get stuck with serial port
                    Serial.begin(115200);
                    while (true)
                    {
                        now = millis();
                        blynk = ((now % 200) <= 100);
                        digitalWrite(ROACHHW_PIN_LED_RED, blynk ? HIGH : LOW );
                        digitalWrite(ROACHHW_PIN_LED_BLU, blynk ? LOW  : HIGH);
                        if ((now - tstart) >= 1000)
                        {
                            tstart = now;
                            Serial.printf("safe boot %u\r\n", now);
                        }
                        yield();
                    }
                }
            }
        }
    }
}
