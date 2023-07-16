void EscLink(void)
{
    static nRF52OneWireSerial* esc_link;
    esc_link = new nRF52OneWireSerial(&Serial1, NRF_UARTE0, DETCORDHW_PIN_SERVO_WEAP);
    
    while (true)
    {
        
    }
}