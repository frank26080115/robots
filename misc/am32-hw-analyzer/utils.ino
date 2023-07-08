// check if the user wants to end a search mode
// if returned true, then do a state transition
bool check_search_exit()
{
    if (printer->available() > 0) {
        char c = printer->read();
        if (c == 'X' || c == 'x') {
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
            if (pol == 123) {
                analogWrite(p, pol);
            }
            else {
                digitalWrite(p, pol);
            }
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

void pulse_all_phases(uint32_t dly, uint32_t cnt)
{
    for (uint8_t i = 0; i < cnt; i++)
    {
        set_all_pins(all_lin, OUTPUT, 123);
        set_all_pins(all_hin, OUTPUT, LOW);
        delay(dly);
        set_all_pins(all_lin, OUTPUT, LOW);
        set_all_pins(all_hin, OUTPUT, 123);
        delay(dly);
    }
}

// pulse a pin, measure all FB pins ADC values after the pulse
void pulse_and_measure(uint32_t outpin, uint32_t dly, uint8_t pol, uint16_t* res)
{
    if (pol != HIGH && pol != LOW)
    {
        analogWrite(outpin, pol);
    }
    else
    {
        digitalWrite(outpin, pol == LOW ? LOW : HIGH);
    }
    delay(dly);
    for (uint8_t i = 0; ; i++)
    {
        uint32_t p = all_fb[i];
        if (p == END_OF_LIST)
        {
            break;
        }
        int16_t x = analogRead(PINTRANSLATE(p));
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
void filter_out(uint32_t* list, const uint32_t* dont_want)
{
    int i, j, k;
    k = 0;
    for (i = 0; i < 999; i++)
    {
        uint32_t p = list[i];
        if (p == END_OF_LIST)
        {
            break;
        }
        bool found = false;
        for (j = 0; j < 999; j++)
        {
            uint32_t d = dont_want[j];
            if (d == END_OF_LIST) {
                break;
            }
            if (d == p) {
                found = true;
                break;
            }
        }
        if (found)
        {
            for (k = i; ; k++)
            {
                uint32_t pnext = list[k + 1];
                list[k] = pnext;
                if (pnext == END_OF_LIST) {
                    break;
                }
            }
            i--;
        }
    }
}

void filter_out_one(uint32_t* list, uint32_t dont_want)
{
    uint32_t tmp[2] = {dont_want, END_OF_LIST};
    filter_out(list, (const uint32_t*)tmp);
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

void print_all_adc()
{
    for (uint8_t i = 0; ; i++)
    {
        uint32_t p = remaining_pins[i];
        if (p == END_OF_LIST)
        {
            break;
        }
        if (p == PA2 && printer == &SerialA2) {
            continue;
        }
        if (p == PB4 && printer == &SerialB4) {
            continue;
        }
        if (digitalpinIsAnalogInput(p))
        {
            int16_t x = analogRead(PINTRANSLATE(p));
            print_pin_name(p);
            printer->printf(" = %5d ; ", x);
        }
    }
}

uint32_t analogPinTranslate(uint32_t p)
{
    switch (p)
    {
        case PA0:  return A0;
        case PA1:  return A1;
        case PA2:  return A2;
        case PA3:  return A3;
        case PA4:  return A4;
        case PA5:  return A5;
        case PA6:  return A6;
        case PA7:  return A7;
        case PB0:  return A8;
        case PB1:  return A9;
    }
    return PIN_NOT_EXIST;
}
