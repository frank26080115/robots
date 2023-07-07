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
    remaining_pins[0] = END_OF_LIST; // empty list

    // remove pins from the list, to make searching easier later
    filter_out(all_pins                       , all_hin, (uint32_t*)remaining_pins);
    filter_out((const uint32_t*)remaining_pins, all_lin, (uint32_t*)remaining_pins);
    filter_out((const uint32_t*)remaining_pins, all_fb , (uint32_t*)remaining_pins);

    // prepare the list of phase pin groups
    phase_groups_cnt = count_tbl(all_hin);
    phase_groups_sz = sizeof(phase_group_t) * phase_groups_cnt;
    phase_groups = (phase_group_t*)malloc(phase_groups_sz);
    memset(phase_groups, END_OF_LIST, phase_groups_sz);

    pinMode(PA15, OUTPUT);
    digitalWrite(PA15, LOW);
    filter_out_one((const uint32_t*)remaining_pins, PA15, (uint32_t*)remaining_pins);
}

void loop()
{
    static uint32_t tick = 0;      // state machine sub counter
    static uint32_t last_time = 0; // used to time state transitions
    uint32_t now = millis();

    if (state_machine == STATEMACH_WAITING_START)
    {
        if ((now - last_time) >= 500)
        {
            // have to swap between PA2 and PB4 because only one SoftwareSerial instance can be listening at a time

            last_time = now;
            if ((tick % 2) == 0) {
                SerialA2.println("rc input is PA2");
                SerialA2.listen();
            }
            else {
                SerialB4.println("rc input is PB4");
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
        filter_out_one((const uint32_t*)remaining_pins, rc_input, (uint32_t*)remaining_pins);
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

        // pulse each HIN pin, see if ADC readings change
        pulse_and_measure(p, HIGH, adc_res);
        //print_adc_res(adc_res);
        uint32_t fb = pick_different(adc_res);

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

        // pulse each COM pin, see if ADC readings change
        pulse_and_measure(p, HIGH, adc_res);
        //print_adc_res(adc_res);
        uint32_t fb = pick_different(adc_res);

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
            char c = printer->read();
            if (c == 'V' || c == 'v')
            {
                printer->println("searching voltage pin, X to stop");
                printer->listen();
                state_machine = STATEMACH_SEARCH_VOLTAGE;
                tick = 0;
                last_time = now;
            }
            else if (c == 'C' || c == 'c')
            {
                printer->println("searching current sensor pin, X to stop");
                printer->listen();
                state_machine = STATEMACH_SEARCH_CURRENTSENSOR;
                tick = 0;
                last_time = now;
            }
            else if (c == 'L' || c == 'l')
            {
                printer->println("searching LED pin, X to stop");
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
            for (uint8_t i = 0; ; i++)
            {
                uint32_t p = remaining_pins[i];
                if (p == END_OF_LIST)
                {
                    break;
                }
                if (digitalpinIsAnalogInput(p))
                {
                    int16_t x = analogRead(digitalPinToAnalogInput(p));
                    print_pin_name(p);
                    printer->printf(" = %5d ; ", x);
                }
            }
            printer->println();
            printer->listen();
            tick++;
        }
    }
    else if (state_machine == STATEMACH_SEARCH_CURRENTSENSOR)
    {
        // intentionally cause current draw and monitor all ADC readings to see fluctuations

        if (check_search_exit()) {
            return;
        }
        if ((now - last_time) >= 200)
        {
            last_time = now;

            uint8_t tm = tick % 8;
            if (tm == 0)
            {
                pinMode(phase_groups[0].hin, OUTPUT);
                pinMode(phase_groups[0].lin, OUTPUT);
                pinMode(phase_groups[1].hin, OUTPUT);
                pinMode(phase_groups[1].lin, OUTPUT);
                // attempt to draw power
                digitalWrite(phase_groups[0].hin, LOW);
                digitalWrite(phase_groups[0].lin, HIGH);
                digitalWrite(phase_groups[1].hin, HIGH);
                digitalWrite(phase_groups[1].lin, LOW);
            }
            else if (tm == 2)
            {
                // attempt to draw power
                digitalWrite(phase_groups[0].hin, HIGH);
                digitalWrite(phase_groups[0].lin, LOW);
                digitalWrite(phase_groups[1].hin, LOW);
                digitalWrite(phase_groups[1].lin, HIGH);
            }
            else if (tm == 4)
            {
                // cool down, see if current drops
                digitalWrite(phase_groups[0].hin, LOW);
                digitalWrite(phase_groups[0].lin, LOW);
                digitalWrite(phase_groups[1].hin, LOW);
                digitalWrite(phase_groups[1].lin, LOW);
            }

            if ((tm % 2) != 0)
            {
                for (uint8_t i = 0; ; i++)
                {
                    uint32_t p = remaining_pins[i];
                    if (p == END_OF_LIST)
                    {
                        break;
                    }
                    if (digitalpinIsAnalogInput(p))
                    {
                        int16_t x = analogRead(digitalPinToAnalogInput(p));
                        print_pin_name(p);
                        printer->printf(" = %5d ; ", x);
                    }
                }
                printer->println();
                printer->listen();
            }
            tick++;
        }
    }
    else if (state_machine == STATEMACH_SEARCH_LED)
    {
        if (check_search_exit()) {
            return;
        }
        if ((now - last_time) >= 2000)
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
            printer->println(" blink ");
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
            digitalWrite(led_pin, (tick % 2) == 0 ? LOW : HIGH);
            pinMode(led_pin, INPUT);
            tick++;
            return;
        }
    }
}

// check if the user wants to end a search mode
// if returned true, then do a state transition
bool check_search_exit()
{
    if (printer->available() > 0) {
        if (printer->read() == 'X') {
            printer->println("search exit");
            printer->listen();
            state_machine = STATEMACH_CMD_PROMPT;
            return true;
        }
    }
    return false;
}

// set all pins a certain way
void set_all_pins(const uint32_t* lst, uint8_t mode, uint8_t pol)
{
    for (uint8_t i = 0; ; i++)
    {
        uint32_t p = lst[i];
        if (p == END_OF_LIST)
        {
            return;
        }
        pinMode(p, mode);
        if (mode == OUTPUT) {
            digitalWrite(p, pol);
        }
    }
}

// set all pins a certain way, except for one
void set_all_pins_except(const uint32_t* lst, uint32_t not_me, uint8_t mode, uint8_t pol)
{
    for (uint8_t i = 0; ; i++)
    {
        uint32_t p = lst[i];
        if (p == END_OF_LIST)
        {
            return;
        }
        if (p == not_me) {
            continue;
        }
        pinMode(p, mode);
        if (mode == OUTPUT) {
            digitalWrite(p, pol);
        }
    }
}

// pulse a pin, measure all FB pins ADC values after the pulse
void pulse_and_measure(uint32_t outpin, uint8_t pol, uint16_t* res)
{
    digitalWrite(outpin, pol == LOW ? LOW : HIGH);
    delay(100);
    for (uint8_t i = 0; ; i++)
    {
        uint32_t p = all_fb[i];
        if (p == END_OF_LIST)
        {
            break;
        }
        int16_t x = analogRead(digitalPinToAnalogInput(p));
        res[i] = x;
    }
    digitalWrite(outpin, pol == LOW ? HIGH : LOW);
}

// pick the FB pin that reported a value that's very different from the average
uint32_t pick_different(const uint16_t* adc_buf)
{
    uint32_t x = PIN_NOT_EXIST;
    uint16_t max_val = 0;
    uint32_t avg_sum = 0;
    uint32_t cnt = 0;

    for (uint8_t i = 0; ; i++)
    {
        uint32_t p = all_fb[i];
        if (p == END_OF_LIST)
        {
            break;
        }
        uint16_t v = adc_buf[i];
        avg_sum += v;
        cnt += 1;
    }
    avg_sum /= cnt;

    for (uint8_t i = 0; ; i++)
    {
        uint32_t p = all_fb[i];
        if (p == END_OF_LIST)
        {
            break;
        }
        uint16_t v = adc_buf[i];
        uint16_t diff = (v > avg_sum) ? (v - avg_sum) : (avg_sum - v);
        if (diff > max_val) {
            max_val = diff;
            x = p;
        }
    }
    return x;
}

// pick the FB pin that reported a value that's higher than the rest
uint32_t pick_max(const uint16_t* adc_buf)
{
    uint32_t x = PIN_NOT_EXIST;
    uint16_t max_val = 0;
    for (uint8_t i = 0; ; i++)
    {
        uint32_t p = all_fb[i];
        if (p == END_OF_LIST)
        {
            return x;
        }
        uint16_t v = adc_buf[i];
        if (v > max_val)
        {
            x = p;
            max_val = v;
        }
    }
    return x;
}

// pick the FB pin that reported a value that's lower than the rest
uint32_t pick_min(const uint16_t* adc_buf)
{
    uint32_t x = PIN_NOT_EXIST;
    int16_t min_val = -1;
    for (uint8_t i = 0; ; i++)
    {
        uint32_t p = all_fb[i];
        if (p == END_OF_LIST)
        {
            return x;
        }
        uint16_t v = adc_buf[i];
        if (v < min_val || min_val < 0)
        {
            x = p;
            min_val = v;
        }
    }
    return x;
}

// count number of entries in a pin table
uint16_t count_tbl(const uint32_t* input_list)
{
    for (int i = 0; ; i++)
    {
        uint32_t p = input_list[i];
        if (p == END_OF_LIST)
        {
            return i;
        }
    }
    return 0;
}

// remove entries from a list, based on a blacklist
void filter_out(const uint32_t* input_list, const uint32_t* dont_want, uint32_t* output_list)
{
    int i, j, k;
    k = 0;
    for (i = 0; i < 999; i++)
    {
        uint32_t p = input_list[i];
        if (p == END_OF_LIST)
        {
            break;
        }
        bool found = false;
        for (j = 0; j < 999; j++)
        {
            uint32_t d = dont_want[j];
            if (d == END_OF_LIST)
            {
                break;
            }
            if (d == p)
            {
                found = true;
                break;
            }
        }
        if (found == false)
        {
            output_list[k] = p;
            k += 1;
        }
    }
    output_list[k] = END_OF_LIST;
}

void filter_out_one(const uint32_t* input_list, uint32_t dont_want, uint32_t* output_list)
{
    uint32_t tmp[2] = {dont_want, END_OF_LIST};
    filter_out(input_list, (const uint32_t*)tmp, output_list);
}

void print_pin_name(uint32_t p)
{
    switch (p)
    {
        case PA0:  printer->print("PA0" ); break;
        case PA1:  printer->print("PA1" ); break;
        case PA2:  printer->print("PA2" ); break;
        case PA3:  printer->print("PA3" ); break;
        case PA4:  printer->print("PA4" ); break;
        case PA5:  printer->print("PA5" ); break;
        case PA6:  printer->print("PA6" ); break;
        case PA7:  printer->print("PA7" ); break;
        case PA8:  printer->print("PA8" ); break;
        case PA9:  printer->print("PA9" ); break;
        case PA10: printer->print("PA10"); break;
        case PA11: printer->print("PA11"); break;
        case PA12: printer->print("PA12"); break;
        case PA13: printer->print("PA13"); break;
        case PA14: printer->print("PA14"); break;
        case PA15: printer->print("PA15"); break;
        case PB0:  printer->print("PB0" ); break;
        case PB1:  printer->print("PB1" ); break;
        case PB2:  printer->print("PB2" ); break;
        case PB3:  printer->print("PB3" ); break;
        case PB4:  printer->print("PB4" ); break;
        case PB5:  printer->print("PB5" ); break;
        case PB6:  printer->print("PB6" ); break;
        case PB7:  printer->print("PB7" ); break;
        case PB8:  printer->print("PB8" ); break;
        //case PB9:  printer->print("PB9" ); break;
        //case PB10: printer->print("PB10"); break;
        //case PB11: printer->print("PB11"); break;
        //case PB12: printer->print("PB12"); break;
        //case PB13: printer->print("PB13"); break;
        //case PB14: printer->print("PB14"); break;
        //case PB15: printer->print("PB15"); break;
        case PF0:  printer->print("PF0" ); break;
        case PF1:  printer->print("PF1" ); break;
        //case PF2:  printer->print("PF2" ); break;
        //case PF3:  printer->print("PF3" ); break;
        //case PF4:  printer->print("PF4" ); break;
        //case PF5:  printer->print("PF5" ); break;
        //case PF6:  printer->print("PF6" ); break;
        //case PF7:  printer->print("PF7" ); break;
        //case PF8:  printer->print("PF8" ); break;
        //case PF9:  printer->print("PF9" ); break;
        //case PF10: printer->print("PF10"); break;
        //case PF11: printer->print("PF11"); break;
        //case PF12: printer->print("PF12"); break;
        //case PF13: printer->print("PF13"); break;
        //case PF14: printer->print("PF14"); break;
        //case PF15: printer->print("PF15"); break;
        default: printer->print("P??"); break;
    }
}

void print_adc_res(const uint16_t* adc_buf)
{
    printer->print("ADC res: ");
    for (uint8_t i = 0; ; i++)
    {
        uint32_t p = all_fb[i];
        if (p == END_OF_LIST)
        {
            break;
        }
        uint16_t v = adc_buf[i];
        print_pin_name(p);
        printer->printf(" = %u ; ", v);
    }
    printer->println();
}

void print_all_pins(const uint32_t* lst)
{
    for (uint8_t i = 0; ; i++)
    {
        uint32_t p = lst[i];
        if (p == END_OF_LIST)
        {
            break;
        }
        print_pin_name(p);
        printer->print(" ");
    }
    printer->println("");
}
