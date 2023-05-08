#include "RoachLib.h"

#if defined(ESP32)
#include <SPIFFS.h>
#define RoachFile File
#define roachfileprint print
#elif defined(NRF52840_XXAA)
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#define RoachFile FatFile
#define roachfileprint write
#endif

int32_t roachnvm_getval(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm)
{
    if (strcmp("hex", desc_itm->type_code) == 0)
    {
        uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset];
        return (int32_t)*wptr;
    }
    if (desc_itm->type_code[0] == 'u' || desc_itm->type_code[0] == 's')
    {
        int32_t x;

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
                return 0;
        }
        return x;
    }
    return 0;
}

void roachnvm_setval(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm, int32_t val)
{
    if (strcmp("hex", desc_itm->type_code) == 0)
    {
        uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset];
        *wptr = (uint32_t)val;
        return;
    }
    if (desc_itm->type_code[0] == 'u' || desc_itm->type_code[0] == 's')
    {
        int32_t x = val;

        int sz = atoi(&(desc_itm->type_code[1]));
        if (desc_itm->type_code[0] == 's') {
            sz *= -1;
        }

        x = (x > desc_itm->limit_max) ? desc_itm->limit_max : x;
        x = (x < desc_itm->limit_min) ? desc_itm->limit_min : x;

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
        return;
    }
}

int32_t roachnvm_incval(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm, int32_t x)
{
    int32_t y = roachnvm_getval(struct_ptr, desc_itm);
    y += x;
    roachnvm_setval(struct_ptr, desc_itm, y);
    return roachnvm_getval(struct_ptr, desc_itm);
}

bool roachnvm_parseitem(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl, char* name, char* value)
{
    roach_nvm_gui_desc_t* desc_itm = NULL;

    int i;
    int slen = strlen(name);
    for (i = slen - 1; i > 0; i--) {
        if (name[i] == ' ' || name[i] == '\t') {
            name[i] = 0;
        }
    }

    // iterate through all items to look for a name match
    for (i = 0; i < 0xFFFF; i++)
    {
        desc_itm = &desc_tbl[i];
        if (desc_itm->name == NULL || desc_itm->name[0] == 0) {
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

    while (value[0] == ' ' || value[0] == '\t') {
        value = &value[1];
    }

    if (strcmp("hex", desc_itm->type_code) == 0)
    {
        uint32_t x = strtoul(value, NULL, 16);
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
            x = strtol(value, NULL, 10);
        }
        else
        {
            double flt = strtod(value, NULL);
            flt *= m;
            x = (int32_t)lround(flt);
        }
        int sz = atoi(&(desc_itm->type_code[1]));
        if (desc_itm->type_code[0] == 's') {
            sz *= -1;
        }

        x = (x > desc_itm->limit_max) ? desc_itm->limit_max : x;
        x = (x < desc_itm->limit_min) ? desc_itm->limit_min : x;

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

void roachnvm_readfromfile(RoachFile* f, uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl)
{
    char temp_buff[64];
    while (f->available())
    {
        int i = 0;
        while (f->available())
        {
            int c = f->read();
            bool is_end = (c == 0 || c == '\n' || c == '\r' || c < 0);
            if (is_end && i > 0)
            {
                temp_buff[i] = 0;
                int j;
                for (j = 1; j < i; j++)
                {
                    char fc = temp_buff[j];
                    if (fc == '=' || fc == ':' || fc == ',') {
                        temp_buff[j] = 0;
                        j += 1;
                        roachnvm_parseitem(struct_ptr, desc_tbl, temp_buff, &(temp_buff[j]));
                        break;
                    }
                }
            }
            else
            {
                temp_buff[i] = c;
                temp_buff[i + 1] = 0;
            }
            if (c < 0) {
                break;
            }
        }
    }
}

void roachnvm_formatitem(char* str, uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm)
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
            xd /= m;
            sprintf(str, "%0.8f", x);
        }
    }
}

void roachnvm_writetofile(RoachFile* f, uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl)
{
    int i;
    roach_nvm_gui_desc_t* desc_itm;
    for (i = 0; i < 0xFFFF; i++)
    {
        desc_itm = &desc_tbl[i];
        if (desc_itm->name == NULL || desc_itm->name[0] == 0) {
            break;
        }
        f->roachfileprint(desc_itm->name);
        f->roachfileprint('=');
        char str[32];
        roachnvm_formatitem(str, struct_ptr, desc_itm);
        f->roachfileprint(str);
        f->roachfileprint("\r\n");
    }
}

void roachnvm_writedescfile(RoachFile* f, roach_nvm_gui_desc_t* desc_tbl)
{
    int i;
    roach_nvm_gui_desc_t* desc_itm;
    for (i = 0; i < 0xFFFF; i++)
    {
        desc_itm = &desc_tbl[i];
        if (desc_itm->name == NULL || desc_itm->name[0] == 0) {
            break;
        }
        f->roachfileprint(desc_itm->name);
        f->roachfileprint(',');
        f->roachfileprint(desc_itm->type_code);
        f->roachfileprint(',');
        char str[32];
        sprintf(str, "%d", desc_itm->def_val);
        f->roachfileprint(str);
        f->roachfileprint(',');
        sprintf(str, "%d", desc_itm->limit_min);
        f->roachfileprint(str);
        f->roachfileprint(',');
        sprintf(str, "%d", desc_itm->limit_max);
        f->roachfileprint(str);
        f->roachfileprint(',');
        sprintf(str, "%d", desc_itm->step);
        f->roachfileprint(str);
        f->roachfileprint(";\r\n");
    }
}

void roachnvm_setdefaults(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl)
{
    int i;
    roach_nvm_gui_desc_t* desc_itm;
    for (i = 0; i < 0xFFFF; i++)
    {
        desc_itm = &desc_tbl[i];
        if (desc_itm->name == NULL || desc_itm->name[0] == 0) {
            break;
        }
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
