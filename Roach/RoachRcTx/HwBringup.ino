void hw_bringup(void)
{
    Serial.begin(115200);
    RoachButton_allBegin();
    RoachPot_allBegin();
    RoachEnc_begin(ROACHHW_PIN_ENC_A, ROACHHW_PIN_ENC_B);
    nbtwi_init(ROACHHW_PIN_I2C_SCL, ROACHHW_PIN_I2C_SDA, (SCREEN_WIDTH * SCREEN_HEIGHT / 8) * 2);
    Serial.println("\r\nRoach RC Transmitter - HW Bringup!");
    while (true)
    {
        static uint32_t lt = 0;
        static uint32_t pb = 0;
        uint32_t now = millis();

        bool need_show = false;
        if ((now - lt) >= 200)
        {
            need_show = true;
        }

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
    }
}
