#include <Adafruit_TinyUSB.h>
#include <RoachPot.h>
#include <RoachButton.h>
#include <RoachServo.h>
#include <nRF52Dshot.h>
#include <RoachCmdLine.h>
#include <RoachHeartbeat.h>
#include <nRF52OneWireSerial.h>
#include <nRF52UicrWriter.h>

#define ROACHHW_PIN_LED_RED  3
#define ROACHHW_PIN_LED_BLU  4
#define ROACHHW_PIN_LED_NEO  8

#define PIN_SERVO_DRIVE  1
#define PIN_SERVO_WEAPON 0

RoachButton btn1   = RoachButton(13);
RoachButton btn2   = RoachButton(12);
RoachPot    pot    = RoachPot(A0, NULL);
RoachServo  drive  = RoachServo(PIN_SERVO_DRIVE);
nRF52Dshot  weapon = nRF52Dshot(PIN_SERVO_WEAPON, DSHOT_SPEED_600);

const uint8_t hb_short[] = { 0x18, 0x00, };
const uint8_t hb_long[]  = { 0x42, 0x00, };
const uint8_t hb_longshort[] = { 0x42, 0x18, 0x00, };
RoachHeartbeat hb = RoachHeartbeat(ROACHHW_PIN_LED_RED);

void esclink_func(void* cmd, char* argstr, Stream* stream);
void nfc_func(void* cmd, char* argstr, Stream* stream);
const cmd_def_t cmds[] = {
    { "esclink", esclink_func },
    { "nfc",     nfc_func },
    { "", NULL }, // end of table
};

RoachCmdLine cmdline(&Serial, (cmd_def_t*)cmds, false, (char*)">>>", (char*)"???", true, 512);

void setup()
{
    Serial.begin(115200);
    pinMode(ROACHHW_PIN_LED_RED, OUTPUT);
    pinMode(ROACHHW_PIN_LED_BLU, OUTPUT);
    digitalWrite(ROACHHW_PIN_LED_RED, HIGH);
    digitalWrite(ROACHHW_PIN_LED_BLU, LOW);
    RoachButton_allBegin();
    RoachPot_allBegin();
    hb.begin();
    hb.play(hb_short);
    drive.begin();
    drive.writeMicroseconds(1500);
    weapon.begin();
    weapon.setThrottle(0);
    RoachWdt_init(100);
    Serial.println("Servo Tester Hello World");
    while (btn1.isHeld() || btn2.isHeld())
    {
        all_tasks();
    }
}

void all_tasks()
{
    cmdline.task();
    RoachPot_allTask();
    RoachButton_allTask();
    weapon.task();
    hb.task();
    RoachWdt_feed();
}

void loop()
{
    uint32_t now = millis();
    static uint32_t last_t = 0;
    uint8_t did = 0;
    static uint8_t prev_did = 0;

    all_tasks();

    static int32_t fx = 0;
    int32_t x = pot.get();

    fx = (((63 * fx) + (x)) >> 6);

    static const int32_t deadzone = 32;
    static const int32_t roof = 1023 - (32 * 5);

    int32_t us = constrain(map(fx, deadzone, roof, 0,  500), 0, 500);
    int32_t th = constrain(map(fx, deadzone, roof, 0, 2047), 0, 2047 - 48);
    

    if (btn1.isHeld() && btn2.isHeld())
    {
        hb.queue(hb_long);
        weapon.setThrottle(th);
        drive.writeMicroseconds(1500);
        did = 2;
    }
    else
    {
        weapon.setThrottle(0);

        if (btn1.isHeld())
        {
            hb.queue(hb_long);
            drive.writeMicroseconds(1500 + us);
            did = 1;
        }
        else if (btn2.isHeld())
        {
            hb.queue(hb_long);
            drive.writeMicroseconds(1500 - us);
            did = 1;
        }
        else
        {
            hb.queue(hb_short);
            drive.writeMicroseconds(1500);
            did = 0;
        }
    }

    if (did != prev_did || (now - last_t) >= 500)
    {
        last_t = now;
        prev_did = did;
        Serial.printf("[%u] %u, %u, %u, %u\r\n", now, did, fx, us, th);
    }
}

void esclink_func(void* cmd, char* argstr, Stream* stream)
{
    static nRF52OneWireSerial* esc_link;
    Serial.println("Esc Link Starting");
    weapon.detach();

    static const uint32_t baud = 19200;

    #ifdef NRFOWS_USE_HW_SERIAL
    Serial1.begin(baud);
    esc_link = new nRF52OneWireSerial(&Serial1, NRF_UARTE0, PIN_SERVO_WEAPON);
    #else
    esc_link = new nRF52OneWireSerial(PIN_SERVO_WEAPON, baud);
    #endif

    esc_link->begin();

    hb.play(hb_longshort);

    uint32_t now = millis();
    uint32_t tdown = 0;

    // this loop will not service anything unnecessary, and the robot needs to be rebooted
    while (true)
    {
        esc_link->echo(&Serial, true);
        hb.task();
        RoachWdt_feed();

        now = millis();
        if (btn1.isHeld() && btn2.isHeld())
        {
            if (tdown <= 0) {
                tdown = now;
            }
            if ((now - tdown) >= 5000)
            {
                break;
            }
        }
        else
        {
            tdown = 0;
        }
    }

    Serial.println("Esc Link Exiting");
    while (btn1.isHeld() || btn2.isHeld())
    {
        hb.task();
    }

    delay(500);
    NVIC_SystemReset();
}

void nfc_func(void* cmd, char* argstr, Stream* stream)
{
    if (nrfuw_uicrIsNfcEnabled())
    {
        Serial.println("NFC is enabled");
        nrfuw_uicrDisableNfc();
        if (nrfuw_uicrIsNfcEnabled() == false)
        {
            Serial.println("NFC has been disabled");
        }
        else
        {
            Serial.println("Failed to disable NFC");
        }
    }
    else
    {
        Serial.println("NFC is disabled");
    }
}
