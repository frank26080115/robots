// check if user wants to enter bootloader mode
// the Roach robot remote control is enclosed with no access to the reset button
// if during development, bad code is written and the microcontroller becomes not responsive over virtual serial port
// a combination of buttons held, during power-up, will force the microcontroller into bootloader mode
// thus providing a way to recover from the bad code without disassembling the enclosure
void safeboot_check(void)
{
    pinMode(ROACHHW_PIN_BTN_UP    , INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_DOWN  , INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_LEFT  , INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_CENTER, INPUT_PULLUP);

    if ((digitalRead(ROACHHW_PIN_BTN_UP) == LOW || digitalRead(ROACHHW_PIN_BTN_DOWN) == LOW) || digitalRead(ROACHHW_PIN_BTN_LEFT) == LOW || digitalRead(ROACHHW_PIN_BTN_CENTER) == LOW)
    {
        pinMode(ROACHHW_PIN_LED_RED, OUTPUT);
        pinMode(ROACHHW_PIN_LED_BLU, OUTPUT);

        uint32_t now = millis();
        uint32_t tstart = now;
        while (digitalRead(ROACHHW_PIN_BTN_UP) == LOW || digitalRead(ROACHHW_PIN_BTN_DOWN) == LOW || digitalRead(ROACHHW_PIN_BTN_LEFT) == LOW || digitalRead(ROACHHW_PIN_BTN_CENTER) == LOW)
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
                else if (digitalRead(ROACHHW_PIN_BTN_LEFT) == LOW)
                {
                    // use this to nuke a file system that's causing a no-boot
                    safeboot_usbmsd();
                }
                else if (digitalRead(ROACHHW_PIN_BTN_CENTER) == LOW)
                {
                    // just get stuck with serial port
                    // it might make sense to put the command line interface here
                    // but that interface might be the one crashing
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

    #ifdef DEVMODE_START_USBMSD_ONLY // use this to nuke a file system that's causing a no-boot
    safeboot_usbmsd();
    #endif
}

void safeboot_usbmsd(void)
{
    // use this to nuke a file system that's causing a no-boot
    Serial.begin(115200);
    RoachUsbMsd_begin();
    RoachUsbMsd_presentUsbMsd();
    uint32_t now = millis();
    uint32_t tstart = now;
    while (true)
    {
        now = millis();
        RoachUsbMsd_task();
        //RoachWdt_feed();
        yield();
        bool blynk = ((now % 200) <= 100);
        digitalWrite(ROACHHW_PIN_LED_RED, blynk ? HIGH : LOW );
        digitalWrite(ROACHHW_PIN_LED_BLU, blynk ? LOW  : HIGH);
        if ((now - tstart) >= 1000)
        {
            tstart = now;
            Serial.printf("safe boot USB MSD %u\r\n", now);
        }
    }
}
