#include <Esp32RcRadio.h>

Esp32RcRadio radio(false); // is receiver

char msg_buf[E32RCRAD_PAYLOAD_SIZE];

void setup()
{
    Serial.begin(115200);
    radio.begin(0x1001, 0x1234ABCD, 0xDEADBEEF); // initialize with default channel map, a unique ID, and a salt
}

void loop()
{
    static uint32_t last_time_stat = 0;
    static uint32_t last_time_mesg = 0;
    uint32_t now = millis();

    // housekeeping
    radio.task();

    // report the message that was received once every 200 ms
    if ((now - last_time_mesg) >= 200)
    {
        if (radio.available() > 0) // new message available
        {
            radio.read((uint8_t*)msg_buf); // read the new message into our application buffer
            Serial.printf("MESG: %s\r\n", msg_buf);
            last_time_mesg = now;
        }
    }

    // do a statistics report every one second
    if ((now - last_time_stat) >= 1000)
    {
        uint32_t rx_total, rx_good;
        signed rssi;
        radio.get_rx_stats(&rx_total, &rx_good, &rssi);
        Serial.printf("STAT: %u , %u / %u , %d\r\n", radio.get_data_rate(), rx_good, rx_total, rssi);
        last_time_stat = now;
    }
}
