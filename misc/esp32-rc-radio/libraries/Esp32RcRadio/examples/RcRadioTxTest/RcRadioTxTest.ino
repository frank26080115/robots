#include <Esp32RcRadio.h>

Esp32RcRadio radio(true); // is transmitter

char msg_buf[E32RCRAD_PAYLOAD_SIZE];

void setup()
{
    Serial.begin(115200);
    radio.begin(0x2000, 0x1234ABCD, 0xDEADBEEF); // initialize with default channel map, a unique ID, and a salt
}

void loop()
{
    static uint32_t last_time_stat = 0;
    static uint32_t last_time_text = 0;
    uint32_t now = millis();

    // send this message, with the current timestamp
    sprintf(msg_buf, "Hello World %u", now);
    radio.send((uint8_t*)msg_buf);

    #ifdef E32RCRAD_BIDIRECTIONAL
    if (radio.available() > 0) // new message available
    {
        radio.read((uint8_t*)msg_buf); // read the new message into our application buffer
        Serial.printf("RX: %s\r\n", msg_buf);
    }
    if (radio.textAvail() > 0) // new text message available
    {
        radio.textRead((char*)msg_buf); // read the new message into our application buffer
        Serial.printf("RX TXT: %s\r\n", msg_buf);
    }
    #endif

    // do a statistics report every 5 seconds
    if ((now - last_time_text) >= 5000)
    {
        sprintf(msg_buf, "BLARG %u\r\n", now);
        radio.textSend((char*)msg_buf);
        last_time_text = now;
    }

    // do housekeeping things (actually sends the queued packet at the correct interval)
    radio.task();

    // do a statistics report every one second
    if ((now - last_time_stat) >= 1000)
    {
        Serial.printf("%u\r\n", radio.get_data_rate());
        last_time_stat = now;
    }
}
