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

#ifndef debug_printf
//#define debug_printf(...)            // do nothing
#define debug_printf(format, ...)    do { Serial.printf((format), ##__VA_ARGS__); } while (0)
#endif

int roachnvm_cntgroup(roach_nvm_gui_desc_t* g)
{
    int cnt = 0;
    while (true)
    {
        roach_nvm_gui_desc_t* x = &(g[cnt]);
        if (x->name == NULL || x->name[0] == 0)
        {
            return cnt;
        }
        cnt += 1;
    }
}

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

void roachnvm_setval_inner(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm, int32_t val, bool def_if_wrong)
{
    if (strcmp("hex", desc_itm->type_code) == 0)
    {
        uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset];
        *wptr = (uint32_t)val;
        return;
    }

    char typecode[32];
    int typecode_len;
    strcpy(typecode, desc_itm->type_code);
    typecode_len = strlen(typecode);
    for (int i = 2; i < typecode_len; i++) {
        if (typecode[i] == 'x') {
            typecode[i] = 0;
            break;
        }
    }

    if (typecode[0] == 'u' || typecode[0] == 's')
    {
        int32_t x = val;

        int sz = atoi(&(typecode[1]));
        if (typecode[0] == 's') {
            sz *= -1;
        }

        if (def_if_wrong)
        {
            if (x > desc_itm->limit_max || x < desc_itm->limit_min) {
                x = desc_itm->def_val;
            }
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

void roachnvm_setval(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm, int32_t val)
{
    roachnvm_setval_inner(struct_ptr, desc_itm, val, false);
}

int32_t roachnvm_incval(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm, int32_t x)
{
    int32_t y = roachnvm_getval(struct_ptr, desc_itm);
    y += x;
    roachnvm_setval(struct_ptr, desc_itm, y);
    return roachnvm_getval(struct_ptr, desc_itm);
}

void roachnvm_valValidate(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_itm)
{
    roachnvm_setval_inner(struct_ptr, desc_itm, roachnvm_getval(struct_ptr, desc_itm), true);
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

    char typecode[32];
    int typecode_len;
    strcpy(typecode, desc_itm->type_code);
    typecode_len = strlen(typecode);
    for (int i = 2; i < typecode_len; i++) {
        if (typecode[i] == 'x') {
            typecode[i] = 0;
            break;
        }
    }

    if (strcmp("hex", typecode) == 0)
    {
        uint32_t x = strtoul(value, NULL, 16);
        uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset];
        *wptr = x;
        ret = true;
    }
    else if (typecode[0] == 'u' || typecode[0] == 's')
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
        int sz = atoi(&(typecode[1]));
        if (typecode[0] == 's') {
            sz *= -1;
        }

        x = (x > desc_itm->limit_max) ? desc_itm->limit_max : x;
        x = (x < desc_itm->limit_min) ? desc_itm->limit_min : x;

        ret = true;
        switch (sz)
        {
            case  8 : {  uint8_t* wptr = ( uint8_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
            case  16: { uint16_t* wptr = (uint16_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
            case  32: { uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
            case -8 : {   int8_t* wptr = (  int8_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
            case -16: {  int16_t* wptr = ( int16_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
            case -32: {  int32_t* wptr = ( int32_t*)&struct_ptr[desc_itm->byte_offset]; *wptr = x; } break;
            default:
                ret = false;
                break;
        }
    }
    return ret;
}

bool roachnvm_parsecmd(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl, char* str)
{
    int slen = strlen(str);

    if (slen >= 256) {
        return false;
    }

    int i, ii, start1 = -1, k = 0, start2 = -1, kk;
    for (i = 0; i < slen; i++)
    {
        char c = str[i];

        // trim head
        if (start1 < 0 && c != ' ') {
            start1 = i;
        }

        // find delimitor
        if (c == '=' || c == ':')
        {
            k = i + 1;
            // trim tail from delimiter, to the left
            for (ii = i; ii >= 0; ii--)
            {
                char cc = str[ii];
                if (cc == ' ' || cc == '=' || cc == ':') {
                    str[ii] = 0; // null terminate early
                }
                else {
                    break;
                }
            }
        }

        // trim head from delimiter to the right
        if (start2 < 0 && k > 0 && c != ' ') {
            start2 = i;
        }

        // find end of string
        if ((c == '\r' || c == '\n' || c == '\0') && start2 > 0)
        {
            // trim tail of string
            for (ii = i; ii >= start1; ii--)
            {
                char cc = str[ii];
                if (cc == ' ' || cc == '\r' || cc == '\n' || cc == '\0') {
                    str[ii] = 0; // null terminate early
                }
                else {
                    break;
                }
            }

            // all done
            char* s1 = (char*)(str[start1]);
            char* s2 = (char*)(str[start2]);
            if (ii > start2) {
                return roachnvm_parseitem(struct_ptr, desc_tbl, s1, s2);
            }
            break;
        }
    }
    return false;
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
    char typecode[32];
    int typecode_len;
    strcpy(typecode, desc_itm->type_code);
    typecode_len = strlen(typecode);
    for (int i = 2; i < typecode_len; i++) {
        if (typecode[i] == 'x') {
            typecode[i] = 0;
            break;
        }
    }

    if (strcmp("hex", typecode) == 0)
    {
        uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset];
        sprintf(str, "0x%08X", *wptr);
    }
    else if (typecode[0] == 'u' || typecode[0] == 's')
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

        int sz = atoi(&(typecode[1]));
        if (typecode[0] == 's') {
            sz *= -1;
        }

        switch (sz)
        {
            case  8 : {  uint8_t* wptr = ( uint8_t*)&struct_ptr[desc_itm->byte_offset]; x = *wptr; } break;
            case  16: { uint16_t* wptr = (uint16_t*)&struct_ptr[desc_itm->byte_offset]; x = *wptr; } break;
            case  32: { uint32_t* wptr = (uint32_t*)&struct_ptr[desc_itm->byte_offset]; x = *wptr; } break;
            case -8 : {   int8_t* wptr = (  int8_t*)&struct_ptr[desc_itm->byte_offset]; x = *wptr; } break;
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
    char str[32];
    roach_nvm_gui_desc_t* desc_itm;
    for (i = 0; i < 0xFFFF; i++)
    {
        desc_itm = &desc_tbl[i];
        if (desc_itm->name == NULL || desc_itm->name[0] == 0) {
            break;
        }

        debug_printf("\t{%d} %s = ", i, desc_itm->name);
        f->roachfileprint(desc_itm->name);
        f->roachfileprint('=');
        roachnvm_formatitem(str, struct_ptr, desc_itm);
        debug_printf("%s\r\n", str);
        f->roachfileprint(str);
        f->roachfileprint("\r\n");
    }
}

void roachnvm_debugNvm(Stream* stream, uint8_t* struct_ptr, uint32_t struct_sz, roach_nvm_gui_desc_t* desc_tbl)
{
    int i;
    for (i = 0; i < struct_sz; i++)
    {
        if ((i % 16) == 0 && i != 0) {
            stream->printf("\r\n");
        }
        stream->printf("%02X ", struct_ptr[i]);
    }
    stream->printf("\r\n");

    roach_nvm_gui_desc_t* desc_itm;
    for (i = 0; i < 0xFFFF; i++)
    {
        desc_itm = &desc_tbl[i];
        if (desc_itm->name == NULL || desc_itm->name[0] == 0) {
            break;
        }
        stream->printf("%s = ", desc_itm->name);
        char str[32];
        roachnvm_formatitem(str, struct_ptr, desc_itm);
        stream->printf("%s [%d ~ %d ; %d]\r\n", str, desc_itm->limit_min, desc_itm->limit_max, desc_itm->def_val);
    }
}

void roachnvm_debugDesc(Stream* stream, roach_nvm_gui_desc_t* desc_itm)
{
    stream->printf("dbg desc itm - \"%s\" %u %d %d %d\r\n", desc_itm->name, desc_itm->byte_offset, desc_itm->def_val, desc_itm->limit_min, desc_itm->limit_max);
}

void roachnvm_validateAll(uint8_t* struct_ptr, roach_nvm_gui_desc_t* desc_tbl)
{
    int i;
    roach_nvm_gui_desc_t* desc_itm;
    for (i = 0; i < 0xFFFF; i++)
    {
        desc_itm = &desc_tbl[i];
        if (desc_itm->name == NULL || desc_itm->name[0] == 0) {
            break;
        }
        roachnvm_valValidate(struct_ptr, desc_itm);
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

uint32_t roachnvm_getConfCrc(uint8_t* data, roach_nvm_gui_desc_t* desc_tbl)
{
    int i;
    roach_nvm_gui_desc_t* desc_itm;
    uint32_t crc = 0xFFFFFFFF;
    for (i = 0; i < 0xFFFF; i++)
    {
        desc_itm = &desc_tbl[i];
        if (desc_itm->name == NULL || desc_itm->name[0] == 0) {
            break;
        }
        uint32_t v = (uint32_t)roachnvm_getval(data, desc_itm);
        crc = crc ^ v;
        for (uint32_t j = 8; j > 0; j--)
        {
            crc = (crc >> 1) ^ (0xEDB88320U & ((crc & 1) ? 0xFFFFFFFF : 0));
        }
    }
    return crc;
}

uint16_t roachnvm_getDescCrc(roach_nvm_gui_desc_t* desc_tbl)
{
    uint8_t* data_ptr8 = (uint8_t*)desc_tbl;
    uint32_t data_sz = roachnvm_getDescCnt(desc_tbl) * sizeof(roach_nvm_gui_desc_t);

    // from Wikipedia about Fletcher's Checksum

    uint16_t sum1 = 0;
    uint16_t sum2 = 0;
    int index;

    for (index = 0; index < data_sz; index++)
    {
        sum1 = (sum1 + data_ptr8[index]) % 255;
        sum2 = (sum2 + sum1) % 255;
    }

    return (sum2 << 8) | sum1;
}

uint32_t roachnvm_getDescCnt(roach_nvm_gui_desc_t* desc_tbl)
{
    int i;
    for (i = 0; i < 0xFFFF; i++)
    {
        roach_nvm_gui_desc_t* p = &(desc_tbl[i]);
        if (p->name == NULL || p->name[0] == 0)
        {
            break;
        }
    }
    return i;
}
