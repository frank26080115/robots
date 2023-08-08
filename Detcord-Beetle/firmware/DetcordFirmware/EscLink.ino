/*
Detcord uses a weapon ESC that runs AM32 firmware
AM32 firmware can be reconfigured through a serial port, with a configuration GUI running on a PC
This function will just loop and act as a passthrough between the CDC serial port and a physical serial port
*/

void EscLink(void)
{
    static nRF52OneWireSerial* esc_link;

    weapon.detach();

    const int baud = 19200;

    #ifdef NRFOWS_USE_HW_SERIAL
    Serial1.begin(baud);
    esc_link = new nRF52OneWireSerial(&Serial1, NRF_UARTE0, DETCORDHW_PIN_SERVO_WEAP);
    #else
    esc_link = new nRF52OneWireSerial(DETCORDHW_PIN_SERVO_WEAP, baud);
    #endif

    esc_link->begin();

    // this loop will not service anything unnecessary, and the robot needs to be rebooted
    while (true)
    {
        esc_link->echo(&Serial, true);
        heartbeat_task();
        RoachWdt_feed();
    }
}
