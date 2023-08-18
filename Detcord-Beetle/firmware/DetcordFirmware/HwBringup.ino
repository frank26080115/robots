void hwbringup_imuScan(void)
{
    Serial.begin(19200);

    XiaoBleSenseLsm_powerOn(ROACHIMU_DEF_PIN_PWR);

    while (true)
    {
        if (Serial.available()) {
            Serial.read();
            break;
        }
        Serial.println("waiting to start i2c scan");
        delay(100);
    }

    pinMode(PIN_WIRE1_SDA, INPUT);
    pinMode(PIN_WIRE1_SCL, INPUT);
    delay(1);
    while (digitalRead(PIN_WIRE1_SDA) == LOW || digitalRead(PIN_WIRE1_SCL) == LOW)
    {
        Serial.println("ERROR: I2C PIN(S) LOW");
        pinMode(ROACHIMU_DEF_PIN_PWR, OUTPUT);
        digitalWrite(ROACHIMU_DEF_PIN_PWR, HIGH);
        delay(100);
    }

#if 0
    nbtwi_init(DETCORDHW_PIN_I2C_SCL, DETCORDHW_PIN_I2C_SDA, ROACHIMU_BUFF_RX_SIZE, false);
    uint16_t adr = 1;
    while (adr <= 0xFF)
    {
        uint8_t res = nbtwi_scan(adr);
        if (res == 0)
        {
            Serial.println("end of scan");
            break;
        }
        else
        {
            Serial.printf("found 0x%02X\r\n", res);
            adr = res + 1;
        }
    }
#else
    Serial.printf("Starting Wire1\r\n");
    delay(100);
    Wire1.begin();
    Wire1.setClock(100000);
    int address, error;
    Serial.printf("Starting Scan\r\n");
    delay(100);
    for(address = 1; address < 127; address++ )
    {
        Serial.printf("checking 0x%02X ... ", address);
        Wire1.beginTransmission(address);
        error = Wire1.endTransmission();
        if (error == 0)
        {
            Serial.printf("found 0x%02X\r\n", address);
        }
        else if (error==4)
        {
            Serial.printf("unknown error at address 0x%02X\r\n", address);
        }
    }
    Serial.println("finished scan");
#endif

    while (true)
    {
        yield();
    }
}
