#include <SoftwareSerial.h>

#define PIN_NOT_EXIST 0xFFFFFFFF
#define END_OF_LIST   0xFFFFFFFF

typedef struct
{
    // every phase has a HIN pin, a LIN pin, and a FB pin
    uint32_t hin;
    uint32_t lin;
    uint32_t fb;
}
phase_group_t;

// list the potential candidates for pins

const uint32_t all_hin[] = {
    PA10,
    PA9,
    PA8,
    END_OF_LIST,
};

const uint32_t all_lin[] = {
    PB1,
    PB0,
    PA7,
    END_OF_LIST,
};

const uint32_t all_fb[] = {
    PA5,
    PA0,
    PA4,
    END_OF_LIST,
};

// master list of all pins
// canditates will be removed from this list to make searching for other pins faster
const uint32_t all_pins[] =
{
    PA0,
    PA1,
    PA2,
    PA3,
    PA4,
    PA5,
    PA6,
    PA7,
    PB0,
    PB1,
    PB2,
    PA8,
    PA9,
    PA10,
    PA11,
    PA12,
    //PA15,
    PB3,
    PB4,
    PB5,
    PB6,
    PB7,
    PB8,
    PF0,
    PF1,
    END_OF_LIST,
};

//#define PINTRANSLATE(x) (x)
//#define PINTRANSLATE(x) digitalPinToAnalogInput(x)
#define PINTRANSLATE(x) analogPinTranslate(x)

#define SERIAL_BAUD_RATE 19200

// declare all potential input pins (note: setting RX = TX means half-duplex mode automatically enabled)
SoftwareSerial SerialA2 = SoftwareSerial(PA2, PA2);
SoftwareSerial SerialB4 = SoftwareSerial(PB4, PB4);

SoftwareSerial* printer = NULL; // this will point to one of the serial ports that's actually working
uint32_t rc_input = PIN_NOT_EXIST; // which pin is RC input

uint32_t remaining_pins[32]; // all pins except the ones used for the phases

phase_group_t* phase_groups = NULL; // will list all phase pin groups
uint16_t phase_groups_cnt = 0;      // number of potential groups
uint32_t phase_groups_sz = 0;       // size of list in bytes, used for memset

uint32_t led_pin; // used to store which LED pin is currently being blinked

// state machine state names
enum
{
    STATEMACH_WAITING_START,      // output text on serial port, wait for user to respond
    STATEMACH_TEST_PHASE_START,   // start checking phase pins
    STATEMACH_TEST_PHASE_A,       // checking all HIN and FB pins
    STATEMACH_TEST_PHASE_B,       // checking all LIN pins
    STATEMACH_TEST_REPORT_PHASES, // print out the result of the test
    STATEMACH_CMD_PROMPT,         // ask the user to input a command
    STATEMACH_CMD_WAIT,           // wait for user to give a command
    STATEMACH_SEARCH_VOLTAGE,        // simply print out ADC readings
    STATEMACH_SEARCH_CURRENTSENSOR,  // make the motor draw current, and see if any ADC readings change
    STATEMACH_SEARCH_LED,            // blink LED pins until user sees it blinking
};

uint16_t state_machine = STATEMACH_WAITING_START;

void setup()
{
    SerialA2.begin(SERIAL_BAUD_RATE);
    SerialB4.begin(SERIAL_BAUD_RATE);

    // remove pins from the list, to make searching easier later
    memcpy(remaining_pins, all_pins, sizeof(all_pins));
    filter_out((uint32_t*)remaining_pins, all_hin);
    filter_out((uint32_t*)remaining_pins, all_lin);
    filter_out((uint32_t*)remaining_pins, all_fb );

    // prepare the list of phase pin groups
    phase_groups_cnt = count_tbl(all_hin);
    phase_groups_sz = sizeof(phase_group_t) * phase_groups_cnt;
    phase_groups = (phase_group_t*)malloc(phase_groups_sz);
    memset(phase_groups, END_OF_LIST, phase_groups_sz);

    // according to AM32 source code, this is permanently a LED
    pinMode(PA15, OUTPUT);
    digitalWrite(PA15, LOW);
    filter_out_one((uint32_t*)remaining_pins, PA15);
}

void loop()
{
    static uint32_t tick = 0;      // state machine sub counter
    static uint32_t last_time = 0; // used to time state transitions
    static uint32_t hftick = 0;    // used to toggle pins at high frequency
    uint32_t now = millis();

    hftick++;

    if (state_machine == STATEMACH_WAITING_START)
    {
        if ((now - last_time) >= 500)
        {
            // have to swap between PA2 and PB4 because only one SoftwareSerial instance can be listening at a time

            last_time = now;
            if ((tick % 2) == 0) {
                SerialA2.print("rc input is PA2; ");
                //printer = &SerialA2;
                //print_all_adc();
                SerialA2.println();
                SerialA2.listen();
            }
            else {
                SerialB4.print("rc input is PB4; ");
                //printer = &SerialB4;
                //print_all_adc();
                SerialB4.println();
                SerialB4.listen();
            }
            tick++;
        }

        // which ever one gets the enter key (Arduino IDE, press SEND button with newline enabled)
        // will be the one that's valid

        uint8_t c = 0;
        if (SerialA2.available() > 0)
        {
            c = SerialA2.read();
            if (c == '\n')
            {
                rc_input = PA2;
                SerialB4.end();
                printer = &SerialA2;
                state_machine = STATEMACH_TEST_PHASE_START;
            }
        }
        if (SerialB4.available() > 0)
        {
            c = SerialB4.read();
            if (c == '\n')
            {
                rc_input = PB4;
                SerialA2.end();
                printer = &SerialB4;
                state_machine = STATEMACH_TEST_PHASE_START;
            }
        }
    }
    else if (state_machine == STATEMACH_TEST_PHASE_START)
    {
        // remove the RC input pin from potential searched pins
        filter_out_one((uint32_t*)remaining_pins, rc_input);
        printer->print("pin list: ");
        print_all_pins(remaining_pins);
        // prep for testing, all pins output low
        set_all_pins(all_hin, OUTPUT, LOW);
        set_all_pins(all_lin, OUTPUT, LOW);
        set_all_pins(all_fb,  INPUT,  LOW);
        state_machine = STATEMACH_TEST_PHASE_A;
        tick = 0;
        printer->println("started to test HIN pins");

    }
    else if (state_machine == STATEMACH_TEST_PHASE_A)
    {
        uint32_t p = all_hin[tick];
        if (p == END_OF_LIST)
        {
            set_all_pins(all_hin, OUTPUT, HIGH);
            set_all_pins(all_lin, OUTPUT, LOW);
            printer->println("started to test LIN pins");
            state_machine = STATEMACH_TEST_PHASE_B;
            tick = 0;
            delay(500); // cool down in case of HW mistake
            return;
        }
        uint16_t adc_res[32];

        pulse_all_phases(10, 20);
        set_all_pins(all_hin, OUTPUT, LOW);
        // pulse each HIN pin, see if ADC readings change
        pulse_and_measure(p, 100, 200, adc_res);
        //print_adc_res(adc_res);
        uint32_t fb = pick_max(adc_res);

        phase_groups[tick].hin = p;
        phase_groups[tick].fb = fb;

        tick++;
        delay(500); // cool down in case of HW mistake
    }
    else if (state_machine == STATEMACH_TEST_PHASE_B)
    {
        uint32_t p = all_lin[tick];
        if (p == END_OF_LIST)
        {
            set_all_pins(all_hin, OUTPUT, LOW);
            set_all_pins(all_lin, OUTPUT, LOW);
            state_machine = STATEMACH_TEST_REPORT_PHASES;
            tick = 0;
            return;
        }
        uint16_t adc_res[32];

        // pulse each LIN pin, see if ADC readings change
        pulse_all_phases(10, 20);
        set_all_pins(all_hin, OUTPUT, LOW);
        pulse_and_measure(p, 2, 200, adc_res);
        //print_adc_res(adc_res);
        uint32_t fb = pick_min(adc_res);

        // match the FB pin with an existing group, so we can save the LIN pin in the correct group
        for (int i = 0; i < phase_groups_cnt; i++)
        {
            phase_group_t* pg = &(phase_groups[i]);
            if (pg->fb == fb)
            {
                pg->lin = p;
            }
        }
        tick++;
        delay(500); // cool down in case of HW mistake
    }
    else if (state_machine == STATEMACH_TEST_REPORT_PHASES)
    {
        printer->println("phase pin matching results:");
        int i;
        for (i = 0; i < phase_groups_cnt; i++)
        {
            phase_group_t* pg = &(phase_groups[i]);
            printer->print("\t");
            printer->print(i + 1, DEC);
            printer->print(": HIN->");
            print_pin_name(pg->hin);
            printer->print(" , LIN->");
            print_pin_name(pg->lin);
            printer->print(" , FB->");
            print_pin_name(pg->fb);
            printer->println();
        }
        printer->println("end of list");
        state_machine = STATEMACH_CMD_PROMPT;
    }
    else if (state_machine == STATEMACH_CMD_PROMPT)
    {
        printer->println("enter command key, one of: V (voltage pin search), C (current sensor search), L (LED search)");
        printer->listen();
        state_machine = STATEMACH_CMD_WAIT;
    }
    else if (state_machine == STATEMACH_CMD_WAIT)
    {
        if (printer->available() > 0)
        {
            // handle command keys
            char c = printer->read();
            if (c == 'V' || c == 'v')
            {
                printer->println("searching voltage pin, X to quit");
                printer->listen();
                state_machine = STATEMACH_SEARCH_VOLTAGE;
                tick = 0;
                last_time = now;
            }
            else if (c == 'C' || c == 'c')
            {
                printer->println("searching current sensor pin, X to quit");
                printer->listen();
                state_machine = STATEMACH_SEARCH_CURRENTSENSOR;
                tick = 0;
                last_time = now;
            }
            else if (c == 'L' || c == 'l')
            {
                printer->println("searching LED pin, X to quit, P to pause, G to continue, < and > to scroll");
                printer->listen();
                state_machine = STATEMACH_SEARCH_LED;
                tick = 0;
                last_time = now;
            }
            else if (c == '\r' || c == '\n' || c == '\0' || c == ' ' || c == '\t')
            {
                // do nothing
            }
            else
            {
                printer->println("invalid cmd key");
                printer->listen();
                state_machine = STATEMACH_CMD_PROMPT;
            }
        }
    }
    else if (state_machine == STATEMACH_SEARCH_VOLTAGE)
    {
        // monitor all ADC readings to see fluctuations
        // works best with a variable voltage power supply

        if (check_search_exit()) {
            return;
        }
        if ((now - last_time) >= 1000)
        {
            last_time = now;
            print_all_adc();
            printer->println();
            printer->listen();
            tick++;
        }
        set_all_pins(all_hin, OUTPUT, ((hftick % 2) == 0) ? HIGH : LOW);
        set_all_pins(all_lin, OUTPUT, ((hftick % 2) == 0) ? LOW : HIGH);
    }
    else if (state_machine == STATEMACH_SEARCH_CURRENTSENSOR)
    {
        // intentionally cause current draw and monitor all ADC readings to see fluctuations

        if (check_search_exit()) {
            return;
        }
        if ((now - last_time) >= 100)
        {
            last_time = now;

            uint8_t tm = tick % 16;
            if (tm == 0)
            {
                pinMode(phase_groups[0].hin, OUTPUT);
                pinMode(phase_groups[0].lin, OUTPUT);
                pinMode(phase_groups[1].hin, OUTPUT);
                pinMode(phase_groups[1].lin, OUTPUT);
                // attempt to draw power
                digitalWrite(phase_groups[0].hin, LOW);
                analogWrite (phase_groups[0].lin, 200);
                analogWrite (phase_groups[1].hin, 200);
                digitalWrite(phase_groups[1].lin, LOW);
            }
            else if (tm == 2)
            {
                // attempt to draw power
                analogWrite (phase_groups[0].hin, 200);
                digitalWrite(phase_groups[0].lin, LOW);
                digitalWrite(phase_groups[1].hin, LOW);
                analogWrite (phase_groups[1].lin, 200);
            }
            else if (tm == 4)
            {
                // cool down, see if current drops
                digitalWrite(phase_groups[0].hin, LOW);
                digitalWrite(phase_groups[0].lin, LOW);
                digitalWrite(phase_groups[1].hin, LOW);
                digitalWrite(phase_groups[1].lin, LOW);
            }
            else if (tm == 5)
            {
                // cool down, see if current drops
                pinMode(phase_groups[0].hin, INPUT);
                pinMode(phase_groups[0].lin, INPUT);
                pinMode(phase_groups[1].hin, INPUT);
                pinMode(phase_groups[1].lin, INPUT);
            }

            if ((tm % 2) != 0)
            {
                print_all_adc();
                printer->println();
                printer->listen();
            }
            tick++;
        }
    }
    else if (state_machine == STATEMACH_SEARCH_LED)
    {
        static uint8_t led_continue = 1;
        if (printer->available() > 0)
        {
            char c = printer->read();
            if (c == 'X' || c == 'x')
            {
                printer->println("search exit");
                printer->listen();
                state_machine = STATEMACH_CMD_PROMPT;
                return;
            }
            else if (c == 'P' || c == 'p')
            {
                // press 'P' to pause the advancement
                led_continue = 0;
            }
            else if (c == 'G' || c == 'g')
            {
                // press 'P' to pause the advancement
                led_continue = 1;
            }
            else if (c == '<')
            {
                // previous
                tick = (tick == 0) ? 0 : (tick - 1);
                led_continue = 0; // pause
            }
            else if (c == '>')
            {
                // next
                tick++;
                if (remaining_pins[tick / 2] == END_OF_LIST)
                {
                    tick = 0;
                }
                led_continue = 0; // pause
            }
        }
        if ((now - last_time) >= 3000)
        {
            // every two seconds, another pin will be blinked
            // the user needs to watch the LED and also watch the serial terminal for the text

            last_time = now;
            uint32_t p = remaining_pins[tick / 2];
            if (p == END_OF_LIST)
            {
                printer->println("LED search looping");
                tick = 0;
                return;
            }
            led_pin = p;
            printer->print("LED pin ");
            print_pin_name(led_pin);
            printer->print(" blink ");
            if ((tick % 2) == 0) {
                printer->println("HIGH");
            }
            else {
                printer->println("LOW");
            }
            printer->listen();
            pinMode(led_pin, OUTPUT);
            digitalWrite(led_pin, (tick % 2) == 0 ? HIGH : LOW);
            delay(100); // short blink just in case we are driving something we shouldn't be
            pinMode(led_pin, INPUT);
            digitalWrite(led_pin, (tick % 2) == 0 ? LOW : HIGH);
            delay(100);
            pinMode(led_pin, OUTPUT);
            digitalWrite(led_pin, (tick % 2) == 0 ? HIGH : LOW);
            delay(100);
            pinMode(led_pin, INPUT);
            digitalWrite(led_pin, (tick % 2) == 0 ? LOW : HIGH);
            if (led_continue != 0) {
                tick++;
            }
            return;
        }
    }
}
