#include <RoachLib.h>

roach_rf_nvm_t nvm_rf;
roach_rx_nvm_t nvm_rx;
roach_tx_nvm_t nvm_tx;

RoachButton btn_up    = RoachButton(ROACHHW_PIN_BTN_UP);
RoachButton btn_down  = RoachButton(ROACHHW_PIN_BTN_DOWN);
RoachButton btn_left  = RoachButton(ROACHHW_PIN_BTN_LEFT);
RoachButton btn_right = RoachButton(ROACHHW_PIN_BTN_RIGHT);
RoachButton btn_g5    = RoachButton(ROACHHW_PIN_BTN_G5);
RoachButton btn_g6    = RoachButton(ROACHHW_PIN_BTN_G6);
RoachButton btn_sw1   = RoachButton(ROACHHW_PIN_BTN_SW1);
RoachButton btn_sw2   = RoachButton(ROACHHW_PIN_BTN_SW2);
RoachButton btn_sw3   = RoachButton(ROACHHW_PIN_BTN_SW3);
RoachButton btn_sw4   = RoachButton(ROACHHW_PIN_BTN_SW4);

RoachPot pot_throttle = RoachPot(ROACHHW_PIN_POT_THROTTLE, &(nvm_tx.pot_throttle));
RoachPot pot_steering = RoachPot(ROACHHW_PIN_POT_STEERING, &(nvm_tx.pot_steering));
RoachPot pot_weapon   = RoachPot(ROACHHW_PIN_POT_WEAPON  , &(nvm_tx.pot_weapon));
RoachPot pot_aux      = RoachPot(ROACHHW_PIN_POT_AUX     , &(nvm_tx.pot_aux));
RoachPot pot_battery  = RoachPot(ROACHHW_PIN_POT_BATTERY , &(nvm_tx.pot_battery));

roach_ctrl_pkt_t tx_pkt = {0};
int headingx = 0, heading = 0;

void setup(void)
{
    nrf5rand_init(512, true, false);
    Serial.begin(115200);
    RoachButton_allBegin();
    RoachPot_allBegin();
    RoachEnc_begin(ROACHHW_PIN_ENC_A, ROACHHW_PIN_ENC_B);
    nbtwi_init();
    oled.begin();
    gui_drawSplash();
    menu_setup();
    Serial.println("\r\nRoach RC Transmitter - Hello World!");
    cmdline.begin();
    RoachUsbMsd_begin();
    menu_run(); // the menu's run loop will execute other tasks, such as ctrler_task
}

void loop(void)
{
    // this will never actually run
}

void ctrler_tasks(void)
{
    uint32_t now = millis();
    RoachPot_allTask();
    // TODO: buttons are using interrupts only, no tasks, but check reliability
    nbtwi_task();
    radio.task();

    if (radio.isBusy())
    {
        RoachEnc_task();
        nrf5rand_task();
        RoSync_task();
        RoachUsbMsd_task();

        tx_pkt.throttle = pot_throttle.get();
        if (encoder_mode == ENCODERMODE_NORMAL)
        {
            tx_pkt.steering = pot_steering.get();
            int h = encoder.get(true);
            headingx += h * nvm_tx.heading_multiplier;
            while (headingx >= 18000) {
                headingx -= 36000;
            }
            while (headingx <= -18000) {
                headingx += 36000;
            }
            heading = (headingx + 5) / 10;
            tx_pkt.heading = heading;
        }
        else if (encoder_mode == ENCODERMODE_USEPOT)
        {
            tx_pkt.steering = 0;
            int hx = headingx;
            hx += roach_value_map(pot_steering.get(), 0, ROACH_SCALE_MULTIPLIER, 0, 18000, false);
            while (hx >= 18000) {
                hx -= 36000;
            }
            while (hx <= -18000) {
                hx += 36000;
            }
            tx_pkt.heading = (hx + 5) / 10;
        }
        tx_pkt.pot_weap = pot_weapon.get();
        tx_pkt.pot_aux = pot_aux.get();
        uint32_t f = 0;
        f |= btn_sw1.isHeld() > 0 ? ROACHPKTFLAG_BTN1 : 0;
        f |= btn_sw2.isHeld() > 0 ? ROACHPKTFLAG_BTN2 : 0;
        f |= btn_sw3.isHeld() > 0 ? ROACHPKTFLAG_BTN3 : 0;
        f |= btn_sw4.isHeld() > 0 ? ROACHPKTFLAG_BTN4 : 0;
        tx_pkt.flags = f;
        radio.send(&tx_pkt);
    }
}