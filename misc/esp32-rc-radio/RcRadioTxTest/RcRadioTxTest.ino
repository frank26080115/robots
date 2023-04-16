#include <Esp32RcRadio.h>

Esp32RcRadio radio(true); // is transmitter

char msg_buf[E32RCRAD_PAYLOAD_SIZE];

void setup()
{
    Serial.begin(115200);
    radio.begin(0x1001, 0x1234ABCD, 0xDEADBEEF); // initialize with default channel map, a unique ID, and a salt
}

void loop()
{
    static uint32_t last_time = 0;
    uint32_t now = millis();

    // send this message, with the current timestamp
    sprintf(msg_buf, "Hello World %u", now);
    radio.send((uint8_t*)msg_buf, false);

    // do housekeeping things (actually sends the queued packet at the correct interval)
    radio.task();

    // do a statistics report every one second
    if ((now - last_time) >= 1000)
    {
        Serial.printf("%u\r\n", radio.get_data_rate());
        last_time = now;
    }
}
