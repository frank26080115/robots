#include <nRF52RcRadio.h>
#include <Adafruit_TinyUSB.h>

nRF52RcRadio radio(false); // is receiver

char msg_buf[NRFRR_PAYLOAD_SIZE];

void setup()
{
    Serial.begin(500000);
    radio.begin();
    radio.config(0x00FF, 12, 34); // initialize with default channel map, a unique ID, and a salt
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

    // report any text messages that have been received
    if (radio.textAvail() > 0)
    {
        radio.textRead((char*)msg_buf); // read the new message into our application buffer
        Serial.printf("TEXT[%u]: %s\r\n", now, msg_buf);
    }

    #ifdef NRFRR_BIDIRECTIONAL
    // send this message, with the current timestamp
    sprintf(msg_buf, "Holy Cow %u", now);
    radio.send((uint8_t*)msg_buf);
    #endif

    // do a statistics report every one second
    if ((now - last_time_stat) >= 1000)
    {
        Serial.printf("STAT: %u , %u / %u , %u, %d"
            , radio.stats_rate.drate
            , radio.stats_rate.good
            , radio.stats_rate.calls
            , radio.stats_rate.loss
            , radio.getRssi()
            );
        #ifdef NRFRR_COUNT_RX_SPENT_TIME
        Serial.printf(" , %0.1f / %u", (float)(((float)radio.get_rx_spent_time()) / (((float)1000.0))), now);
        #endif

        #ifdef NRFRR_DEBUG_RX_ERRSTATS
        Serial.printf(" , RX-ERRs: ");
        // print out the occurance of each type of error
        uint32_t* rx_errs = radio.getRxErrStat();
        int erridx;
        for (erridx = 0; erridx < 12; erridx++)
        {
            Serial.printf(" %u ", rx_errs[erridx]);
        }
        memset(rx_errs, 0, 4*12); // clear errors for this second
        #endif

        Serial.printf("\r\n");
        last_time_stat = now;
    }
}
