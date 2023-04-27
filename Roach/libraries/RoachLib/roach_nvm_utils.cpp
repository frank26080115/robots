#include "RoachLib.h"

#ifdef ESP32 // TODO: proper detection
#include <SPIFFS.h>
#else
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#endif

bool roach_nvm_write_item(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl, char* name, char* value)
{
    roach_nvm_gui_desc_t* desc_itm = NULL;
    int i;
    // iterate through all items to look for a name match
    for (i = 0; i < 0xFFFF; i++)
    {
        desc_itm = &desc_tbl[i];
        if (desc_itm->name[0] == 0) {
            // end of table, item not found;
            return false;
        }
        if (strncmp(desc_itm->name, name, 32) == 0) {
            break; // name match
        }
    }
    if (strncmp(desc_itm->name, name, 32) != 0) {
        return false; // no name match
    }

    bool ret = false;

    if (strcmp("hex", desc_itm->type_code) == 0)
    {
        uint32_t x = strtoul(value, 16);
        uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset];
        *wptr = x;
        ret = true;
    }
    else if (desc_itm->type_code[0] == 'u' || desc_itm->type_code[0] == 's')
    {
        uint32_t m = 1;
        if (desc_itm->type_code[3] == 'x')
        {
            m = atoi(&(desc_itm->type_code[4]));
        }
        else if (desc_itm->type_code[2] == 'x')
        {
            m = atoi(&(desc_itm->type_code[3]));
        }
        int32_t x;
        if (m <= 1)
        {
            x = strtol(value, 10);
        }
        else
        {
            double flt = strtod(value);
            flt *= m;
            x = (int32_t)lround(flt);
        }
        int sz = atoi(&(desc_itm->type_code[1]));
        if (desc_itm->type_code[0] == 's') {
            sz *= -1;
        }

        ret = true;
        switch (sz)
        {
            case  8 : { uint8_t*  wptr = (uint8_t* )&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
            case  16: { uint16_t* wptr = (uint16_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
            case  32: { uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
            case -8 : {  int8_t*  wptr = ( int8_t* )&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
            case -16: {  int16_t* wptr = ( int16_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
            case -32: {  int32_t* wptr = ( int32_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
            default:
                ret = false;
                break;
        }
    }
    return ret;
}

void roach_nvm_read_from_file(File* f, uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl)
{
    while (f->available())
    {
        String s = f->readStringUntil('\n');
        int spci = s.indexOf(' ');
        String left = s.substring(0, spci);
        String right = s.substring(spci + 1, s.length());
        roach_nvm_write_item(struct_ptr, desc_tbl, left.c_str(), right.c_str());
    }
}

void roach_nvm_format_item(char* str, uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm)
{
    if (strcmp("hex", desc_itm->type_code) == 0)
    {
        uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset];
        sprintf(str, "0x%08X", *wptr);
    }
    else if (desc_itm->type_code[0] == 'u' || desc_itm->type_code[0] == 's')
    {
        uint32_t m = 1;
        int32_t x;

        if (desc_itm->type_code[3] == 'x')
        {
            m = atoi(&(desc_itm->type_code[4]));
        }
        else if (desc_itm->type_code[2] == 'x')
        {
            m = atoi(&(desc_itm->type_code[3]));
        }

        int sz = atoi(&(desc_itm->type_code[1]));
        if (desc_itm->type_code[0] == 's') {
            sz *= -1;
        }

        switch (sz)
        {
            case  8 : { uint8_t*  wptr = (uint8_t* )&struct_ptr[desc_itm->byte_offset]; x = *wptr; } break;
            case  16: { uint16_t* wptr = (uint16_t*)&struct_ptr[desc_itm->byte_offset]; x = *wptr; } break;
            case  32: { uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset]; x = *wptr; } break;
            case -8 : {  int8_t*  wptr = ( int8_t* )&struct_ptr[desc_itm->byte_offset]; x = *wptr; } break;
            case -16: {  int16_t* wptr = ( int16_t*)&struct_ptr[desc_itm->byte_offset]; x = *wptr; } break;
            case -32: {  int32_t* wptr = ( int32_t*)&struct_ptr[desc_itm->byte_offset]; x = *wptr; } break;
            default:
                return;
        }

        if (m <= 1)
        {
            sprintf(str, "%d", x);
        }
        else
        {
            volatile double xd = x;
            dx /= m;
            sprintf(str, "%0.8f", x);
        }
    }
}

void roach_nvm_write_to_file(File* f, uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl)
{
    int i;
    roach_nvm_write_item* desc_itm;
    for (i = 0; i < 0xFFFF; i++)
    {
        desc_itm = &desc_tbl[i];
        f->print(desc_itm->name);
        f->print(' ');
        char str[32];
        roach_nvm_format_item(str, struct_ptr, desc_itm);
        f->print(str);
        f->print("\r\n");
    }
}

void roach_nvm_write_desc_file(File* f, roach_nvm_gui_desc_t* desc_tbl)
{
    int i;
    roach_nvm_write_item* desc_itm;
    for (i = 0; i < 0xFFFF; i++)
    {
        desc_itm = &desc_tbl[i];
        f->print(desc_itm->name);
        f->print(',');
        f->print(desc_itm->type_code);
        f->print(',');
        f->printf("%d", desc_itm->def_val);
        f->print(',');
        f->printf("%d", desc_itm->limit_min);
        f->print(',');
        f->printf("%d", desc_itm->limit_max);
        f->print(',');
        f->printf("%d", desc_itm->step);
        f->print(";\r\n");
    }
}

void roach_nvm_set_defaults(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl)
{
    int i;
    roach_nvm_write_item* desc_itm;
    for (i = 0; i < 0xFFFF; i++)
    {
        desc_itm = &desc_tbl[i];
        int32_t x = desc_itm->def_val;
        if (strcmp("hex", desc_itm->type_code) == 0)
        {
            uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset];
            *wptr = x;
        }
        else if (desc_itm->type_code[0] == 'u' || desc_itm->type_code[0] == 's')
        {
            int sz = atoi(&(desc_itm->type_code[1]));
            if (desc_itm->type_code[0] == 's') {
                sz *= -1;
            }

            switch (sz)
            {
                case  8 : { uint8_t*  wptr = (uint8_t* )&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
                case  16: { uint16_t* wptr = (uint16_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
                case  32: { uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
                case -8 : {  int8_t*  wptr = ( int8_t* )&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
                case -16: {  int16_t* wptr = ( int16_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
                case -32: {  int32_t* wptr = ( int32_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
                default:
                    break;
            }
        }
    }
}
