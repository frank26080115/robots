static const uint8_t hbani_triple_fast[] = { 0x11, 0x11, 0x1A, 0x00, };
static const uint8_t hbani_double_fast[] = { 0x11, 0x1A, 0x00, };
static const uint8_t hbani_double_slow[] = { 0x53, 0x5B, 0x00, };
static const uint8_t hbani_single_fast[] = { 0x1B, 0x00, };
static const uint8_t hbani_single_slow[] = { 0x5B, 0x00, };

void heartbeat_task()
{
    static bool prev_radio_connected = false;
    static bool prev_usb_connected   = false;
    bool _radio_connected            = radio.isConnected();
    bool _usb_connected              = RoachUsbMsd_isUsbPresented();

    hb_red.task();

    if (_radio_connected && prev_radio_connected == false)
    {
        hb_rgb.set(0, 255, 0);
        hb_red.play(_usb_connected ? hbani_triple_fast : hbani_double_fast);
    }
    else if (_radio_connected == false && prev_radio_connected != false)
    {
        hb_rgb.set(255, 0, 0);
        hb_red.play(_usb_connected ? hbani_single_slow : hbani_single_fast);
    }

    if (_usb_connected && prev_usb_connected == false)
    {
        hb_red.play(_radio_connected ? hbani_triple_fast : hbani_single_slow);
    }
    else if (_usb_connected == false && prev_usb_connected != false)
    {
        hb_red.play(_radio_connected ? hbani_double_fast : hbani_single_fast);
    }

    prev_radio_connected = _radio_connected;
    prev_usb_connected = _usb_connected;
}
