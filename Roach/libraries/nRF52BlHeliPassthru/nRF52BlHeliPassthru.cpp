#include "nRF52BlHeliPassthru.h"
#include "nRF52BlHeliPassthru_consts.h"
#include <Arduino.h>

#define NRFBHPT_SER_BUFF_SIZE 512

static uint32_t last_avail = 0;
static uint32_t last_ser_time = 0;
static bool     serial_command = false;
static uint8_t  serial_rx[NRFBHPT_SER_BUFF_SIZE];
static uint16_t serial_rx_counter = 0;
static uint16_t serial_tx_counter = 0;
static uint16_t serial_buffer_len = 0;
static uint16_t esc_crc = 0;
static bool     enable4way = true;

uint16_t nrfbhpt_4way(uint8_t buf[]);
uint16_t nrfbhpt_msp(uint8_t buf[], uint8_t buf_size);
uint16_t nrfbhpt_getFromEsc(uint8_t rx_buf[], uint16_t wait_ms);
uint16_t nrfbhpt_sendToEsc(uint8_t tx_buf[], uint16_t buf_size, bool CRC);
uint16_t nrfbhpt_sendToEsc(uint8_t tx_buf[], uint16_t buf_size); // defaults to CRC=true

static uint16_t _byteCrc(uint8_t data, uint16_t crc);
static uint16_t _crc_xmodem_update(uint16_t crc, uint8_t data);

Stream* softSer = NULL;

void nrfbhpt_begin(Stream* ser)
{
    softSer = ser;
}

void nrfbhpt_task(void)
{
    uint32_t avail = Serial.available();
    uint32_t now = millis();
    if (avail != last_avail)
    {
        last_ser_time = now;
        last_avail = avail;
    }

    if ((now - last_ser_time) > 2 && avail > 0)
    {
        serial_command = true;
        while (Serial.available())
        {
            uint8_t c = Serial.read();
            serial_rx[serial_rx_counter] = c;

            if ((serial_rx_counter == 4) && (serial_rx[0] == 0x2F)) {
                // 4 Way Command: Size (0 = 256) + 7 Byte Overhead
                if (serial_rx[4] == 0) {
                    serial_buffer_len = 256 + 7;
                }
                else {
                    serial_buffer_len = serial_rx[4] + 7;
                }
            }
            if ((serial_rx_counter == 3) && (serial_rx[0] == 0x24)) {
                // MSP Command: Size (0 = 0)+ 6 Byte Overhead
                serial_buffer_len = serial_rx[3] + 6;
            }

            serial_rx_counter++;
            if (serial_rx_counter >= serial_buffer_len || serial_rx_counter >= NRFBHPT_SER_BUFF_SIZE) {
                break;
            }
        }
    }

    if (serial_command)
    {
        if ((serial_rx[0] == 0x2F) && (serial_rx_counter >= serial_buffer_len))
        {
            // 4 Way Command
            // 4 Way proceed
            serial_tx_counter = nrfbhpt_4way(serial_rx);
            for (uint16_t b = 0; b < serial_tx_counter; b++) {
                Serial.write(serial_rx[b]);
            }
            serial_command = false;
            serial_rx_counter = 0;
            serial_tx_counter = 0;
        }
        else if (serial_rx[0] == 0x24 && serial_rx[1] == 0x4D && serial_rx[2] == 0x3C)
        {
            // MSP Command
            // MSP Proceed
            serial_tx_counter = MSP_Check(serial_rx, serial_rx_counter);
            //Serial.print("length ");
            //Serial.println(i);
            for (uint8_t b = 0; b < serial_tx_counter; b++) {
                Serial.write(serial_rx[b]);
            }
            serial_command = false;
            serial_rx_counter = 0;
            serial_tx_counter = 0;
        }
    }
}

uint16_t nrfbhpt_4way(uint8_t buf[])
{
    uint8_t cmd = buf[1];
    uint8_t addr_high = buf[2];
    uint8_t addr_low = buf[3];
    uint8_t I_Param_Len = buf[4];
    uint8_t param = buf[5];               // param = ParamBuf[0]
    uint8_t ParamBuf[256] = {0};          // Parameter Buffer
    uint16_t crc = 0;
    uint16_t buf_size;                    // return Output Buffer Size -> O_Param_Len + Header + CRC
    uint8_t ack_out = ACK_OK;
    uint16_t O_Param_Len = 0;

    for (uint8_t i = 0; i < 5 ; i++) {            // CRC Check of Header (CMD, Adress, Size
        crc = _crc_xmodem_update (crc, buf[i]);
    }
    uint8_t InBuff = I_Param_Len;                 // I_Param_Len = 0 -> 256Bytes
    uint16_t i = 0;                               // work counter
    do {                                          // CRC Check of Input Parameter Buffer
        crc = _crc_xmodem_update (crc, buf[i + 5]);
        ParamBuf[i] = buf[i + 5];
        i++;
        InBuff--;
    } while (InBuff != 0);
    uint16_t crc_in = ((buf[i + 5] << 8) | buf[i + 6]);

#ifdef _DEBUG_
    Serial.print("4Way CMD: ");
    Serial.print(cmd, HEX);
    Serial.print(" Adress: ");
    Serial.print(addr_high, HEX);
    Serial.print(addr_low, HEX);
    Serial.print(" ParamBuf Size: ");
    Serial.print(I_Param_Len, HEX);
    // buffer
    Serial.print(" ParamBuf[0]: ");
    Serial.print(ParamBuf[0], HEX);
    // buffer
    Serial.print(" CRC_in: ");
    Serial.print(crc_in, HEX);
    Serial.print(" CRC calculated: ");
    Serial.println(crc, HEX);
#endif

    if (crc_in != crc)
    {
#ifdef _DEBUG_
        Serial.print("Wrong CRC ");
#endif
        buf[0] = cmd_Remote_Escape;
        buf[1] = cmd;
        buf[2] = addr_high;
        buf[3] = addr_low;
        O_Param_Len = 0x01;
        buf[4] = 0x01;        // Output Param Lenght
        buf[5] = 0x00;        // Dummy
        ack_out = ACK_I_INVALID_CRC;
        buf[6] = ACK_I_INVALID_CRC;    // ACK
        for (uint8_t i = 0; i < 7; i++) {
            crc = _crc_xmodem_update (crc, buf[i]);
        }
        buf[7] = (crc >> 8) & 0xff;
        buf[8] = crc & 0xff;
#ifdef _DEBUG_
        Serial.print("with CRC: 0x");
        Serial.print(buf[7], HEX);
        Serial.print(" ");
        Serial.print(buf[8], HEX);
        Serial.print(" ");
#endif
        buf_size = 9;
        if (cmd < 0x50) {
            return buf_size;
        }
    }

    crc = 0;
    ack_out = ACK_OK;
    buf[5] = 0;

    if (cmd == cmd_DeviceInitFlash)
    {
#ifdef _DEBUG_
        Serial.print("DeviceInitFlash ");
#endif
        O_Param_Len = 0x04;
        if (param == 0x00)
        {  // ESC Count
            uint8_t BootInit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0x0D, 'B', 'L', 'H', 'e', 'l', 'i', 0xF4, 0x7D};
            uint8_t Init_Size = 17;
            uint8_t RX_Size = 0;
            uint8_t RX_Buf[250] = {0};
            // DeviceInitFlash hat die CRC bereits im Array enthalten, daher darf keine CRC gesendet werden
            RX_Size = nrfbhpt_sendToEsc(BootInit, Init_Size, false);        // send without CRC
            //RX_Size = nrfbhpt_sendToEsc(BootInit, Init_Size);             // send with CRC
            delay(50);
            // read Answer Format = BootMsg("471c") SIGNATURE_001, SIGNATURE_002, BootVersion, BootPages (,ACK = brSUCCESS = 0x30)
            RX_Size = nrfbhpt_getFromEsc(RX_Buf, 200);
#ifdef _DEBUG_
            Serial.print("ESC Received: ");
            Serial.print(RX_Size);
            Serial.print(" ");
            Serial.print("Bytes: ");
            Serial.print(RX_Buf[0], HEX);
            Serial.print(" ");
            Serial.print(RX_Buf[1], HEX);
            Serial.print(" ");
            Serial.print(RX_Buf[2], HEX);
            Serial.print(" ");
            Serial.print(RX_Buf[3], HEX);
            Serial.print(" ");
            Serial.print(RX_Buf[4], HEX);
            Serial.print(" ");
            Serial.print(RX_Buf[5], HEX);
            Serial.print(" ");
            Serial.print(RX_Buf[6], HEX);
            Serial.print(" ");
            Serial.print(RX_Buf[7], HEX);
            Serial.print(" ");
            Serial.print(RX_Buf[8], HEX);
#endif
            if (RX_Buf[RX_Size - 1] == brSUCCESS) {
                buf[5] = RX_Buf[5];       // Device Signature2?
                buf[6] = RX_Buf[4];       // Device Signature1?
                buf[7] = RX_Buf[3];       // "c"?
                buf[8] = imARM_BLB;       // 4-Way Mode: ARMBLB = 0x04
                buf[9] = ACK_OK;          // ACK
#ifdef _DEBUG_
                Serial.print("OK");
#endif
            }
            else {
                buf[5] = 0x06;        // Device Signature2?
                buf[6] = 0x33;        // Device Signature1?
                buf[7] = 0x67;        // "c"?
                buf[8] = imARM_BLB;   // Boot Pages?
                ack_out = ACK_D_GENERAL_ERROR;
                buf[9] = ACK_D_GENERAL_ERROR;    // ACK
#ifdef _DEBUG_
                Serial.print("General Error");
#endif
            }
        }
        else {
            ack_out = ACK_I_INVALID_CHANNEL;
            buf[9] = ACK_I_INVALID_CHANNEL;    // ACK
#ifdef _DEBUG_
            Serial.print("Invalid channel");
#endif
        }
    }
    else if (cmd == cmd_DeviceReset)
    {
#ifdef _DEBUG_
        Serial.print("DeviceReset ");
#endif
        O_Param_Len = 0x01;
        if ((param == 0x00)/*&& (addr_high == 0x00)*/)
        {
            // ESC Count
            if (enable4way)
            {
                buf[6] = ACK_OK;    // ACK
                uint8_t ESC_data[2] = {RestartBootloader, 0};
                uint16_t Data_Size = 2;
                uint16_t RX_Size = 0;
                RX_Size = nrfbhpt_sendToEsc(ESC_data, Data_Size);
                // Betaflight setzt ausgang Low -> wartet 300ms -> setzt Ausgang High
                digitalWrite(SERVO_OUT, LOW);
                delay(300);
                digitalWrite(SERVO_OUT, HIGH);
                // Keine Antwort vom ESC -> trotzdem serial leeren
                uint8_t RX_Buf[5] = {0};
                RX_Size = nrfbhpt_getFromEsc(RX_Buf, 50);
            }
            else {
                ack_out = ACK_D_GENERAL_ERROR;
                buf[6] = ACK_D_GENERAL_ERROR;
#ifdef _DEBUG_
                Serial.print("4Way not aktive");
#endif
            }
        }
        else
        {
            buf[5] = 0x00;
            ack_out = ACK_I_INVALID_CHANNEL;
            buf[6] = ACK_I_INVALID_CHANNEL;    // ACK
#ifdef _DEBUG_
            Serial.print("Invalid channel");
#endif
        }
    }
    else if (cmd == cmd_InterfaceTestAlive)
    {
#ifdef _DEBUG_
        Serial.print("InterfaceTestAlive ");
#endif
        O_Param_Len = 0x01;
        uint8_t ESC_data[2] = {CMD_KEEP_ALIVE, 0};
        uint16_t Data_Size = 2;
        uint16_t RX_Size = 0;
        uint8_t RX_Buf[250] = {0};
        RX_Size = nrfbhpt_sendToEsc(ESC_data, Data_Size);
        delay(5);
        RX_Size = nrfbhpt_getFromEsc(RX_Buf, 200);
#ifdef _DEBUG_
        Serial.print("ESC Received: ");
        Serial.print(RX_Size);
        Serial.print(" Bytes: ");
        Serial.print(RX_Buf[0], HEX);
#endif
        if (RX_Buf[0] == brSUCCESS) {
            //buf[6] = ACK_OK;    // ACK
        }
        else {
            //buf[6] = ACK_D_GENERAL_ERROR;    // ACK
        }
    }
    else if (cmd == cmd_DeviceRead)
    {
#ifdef _DEBUG_
        Serial.print("DeviceRead @ Adress ");
        Serial.print(addr_high, HEX);
        Serial.println(addr_low, HEX);
#endif
        uint8_t ESC_data[4] = {CMD_SET_ADDRESS, 0x00, addr_high, addr_low};
        uint16_t Data_Size = 4;
        uint16_t RX_Size = 0;
        uint8_t RX_Buf[300] = {0};
        uint16_t esc_rx_crc = 0;
        RX_Size = nrfbhpt_sendToEsc(ESC_data, Data_Size);
        delay(5);
        RX_Size = nrfbhpt_getFromEsc(RX_Buf, 200);
#ifdef _DEBUG_
        Serial.print("Set Adress -> ESC Received: ");
        Serial.print(RX_Size);
        Serial.print(" ACK: ");
        Serial.print(RX_Buf[0], HEX);
#endif

        if (RX_Buf[0] == brSUCCESS)
        {
#ifdef _DEBUG_
            Serial.println(" -> Success");
#endif
            // alles ok
            ESC_data[0] = CMD_READ_FLASH_SIL;
            ESC_data[1] = param;
            Data_Size = 2;
            RX_Size = nrfbhpt_sendToEsc(ESC_data, Data_Size);
            if (param == 0) {
                O_Param_Len = 256;
                delay(256);
            }
            else {
                delay(param);
                O_Param_Len = param;
            }
            RX_Size = nrfbhpt_getFromEsc(RX_Buf, 500);
#ifdef _DEBUG_
            Serial.print("Read Flash -> ESC Received: ");
            Serial.print(RX_Size);
            Serial.print(" ACK: ");
            Serial.print(RX_Buf[(RX_Size - 1)], HEX);
#endif
            if (RX_Size != 0)
            {
                if (RX_Buf[(RX_Size - 1)] == brSUCCESS) {
#ifdef _DEBUG_
                    Serial.println(" -> Success");
#endif
                }
                else {
#ifdef _DEBUG_
                    Serial.println(" -> Fail");
#endif
                    ack_out = ACK_D_GENERAL_ERROR;
                }
                RX_Size = RX_Size - 3;                              // CRC High, CRC_Low and ACK from ESC
                O_Param_Len = RX_Size;
                for (uint16_t i = 5; i < (RX_Size + 5); i++) {
                    buf[i] = RX_Buf[i - 5]; // buf[5] = RX_Buf[0]
                    esc_rx_crc = _byteCrc(buf[i], esc_rx_crc);
                    //Data_Size = i;
                }
                esc_rx_crc = _byteCrc(RX_Buf[(RX_Size)], esc_rx_crc);
                esc_rx_crc = _byteCrc(RX_Buf[(RX_Size + 1)], esc_rx_crc);
#ifdef _DEBUG_
                Serial.print("ESC CRC: : ");
                Serial.print(esc_rx_crc, HEX);
#endif
                if (esc_rx_crc == 0) {
#ifdef _DEBUG_
                    Serial.println("-> CRC OK");
#endif
                }
                else {
                    ack_out = ACK_D_GENERAL_ERROR;
                    O_Param_Len = 0x01;
#ifdef _DEBUG_
                    Serial.println("-> CRC Fail");
#endif
                }
            }
            else
            {
#ifdef _DEBUG_
                Serial.println(" -> Fail");
#endif
                ack_out = ACK_D_GENERAL_ERROR;
                O_Param_Len = 0x01;
            }
        }
        else
        {
#ifdef _DEBUG_
            Serial.println(" -> Fail");
#endif
            // nix ok
            O_Param_Len = 0x01;
            ack_out = ACK_D_GENERAL_ERROR;
        }
    }
    else if (cmd == cmd_InterfaceExit)
    {
#ifdef _DEBUG_
        Serial.print("Interface Exit ");
#endif
        DeinitSerialOutput();           // initialisiert PPM IN/OUT
        O_Param_Len = 0x01;
    }
    else if (cmd == cmd_ProtocolGetVersion)
    {
#ifdef _DEBUG_
        Serial.print("ProtocolGetVersion ");
#endif
        O_Param_Len = 0x01;
        buf[5] = SERIAL_4WAY_PROTOCOL_VER;//0x6C;
    }
    else if (cmd == cmd_InterfaceGetName)
    {
#ifdef _DEBUG_
        Serial.print("InterfaceGetName ");
#endif
        // SERIAL_4WAY_INTERFACE_NAME_STR "m4wFCIntf"
        O_Param_Len = 0x09;
        //buf[4] = 0x09;        // Output Param Lenght
        buf[5] = 'm';//0x6D;
        buf[6] = '4';//0x34;
        buf[7] = 'w';//0x77;
        buf[8] = 'F';//0x46;
        buf[9] = 'C';//0x43;
        buf[10] = 'I';//0x49;
        buf[11] = 'n';//0x6E;
        buf[12] = 't';//0x74;
        buf[13] = 'f';//0x66;
    }
    else if (cmd == cmd_InterfaceGetVersion)
    {
#ifdef _DEBUG_
        Serial.print("InterfaceGetVersion ");
#endif
        O_Param_Len = 0x02;
        buf[5] = SERIAL_4WAY_VERSION_HI;//0xC8;
        buf[6] = SERIAL_4WAY_VERSION_LO;//0x04;
    }
    else if (cmd == cmd_InterfaceSetMode)
    {
#ifdef _DEBUG_
        Serial.print("InterfaceSetMode ");
#endif
        O_Param_Len = 0x01;
        if (param == imARM_BLB) {
            //buf[6] = ACK_OK;      // ACK
        }
        else {
            buf[6] = ACK_I_INVALID_PARAM;      // ACK
            ack_out = ACK_I_INVALID_PARAM;
        }
    }
    else if (cmd == cmd_DeviceVerify)
    {
#ifdef _DEBUG_
        Serial.print("DeviceVerify ");
#endif
        O_Param_Len = 0x01;
    }
    else if (cmd == cmd_DevicePageErase)
    {
#ifdef _DEBUG_
        Serial.print("DevicePageErase ");
#endif
        O_Param_Len = 0x01;
        addr_high = (param << 2);
        addr_low = 0;
        // Send CMD Adress
        uint8_t ESC_data[4] = {CMD_SET_ADDRESS, 0, addr_high, addr_low};
        uint16_t Data_Size = 4;
        uint16_t RX_Size = 0;
        uint8_t RX_Buf[250] = {0};
        RX_Size = nrfbhpt_sendToEsc(ESC_data, Data_Size);
        delay(5);
        RX_Size = nrfbhpt_getFromEsc(RX_Buf, 200);
#ifdef _DEBUG_
        Serial.print("ESC Received: ");
        Serial.print(RX_Size);
        Serial.print(" Bytes: ");
        Serial.print(RX_Buf[0], HEX);
#endif
        if (RX_Buf[0] == brSUCCESS) {
            //buf[6] = ACK_OK;    // ACK
        }
        else {
            ack_out = ACK_D_GENERAL_ERROR;
            buf[6] = ACK_D_GENERAL_ERROR;    // ACK
        }
        // Send Data
        ESC_data[0] = CMD_ERASE_FLASH;
        ESC_data[1] = 0x01;
        Data_Size = 2;
        RX_Size = 0;
        RX_Size = nrfbhpt_sendToEsc(ESC_data, Data_Size);
        delay(50);
        RX_Size = nrfbhpt_getFromEsc(RX_Buf, 100);
#ifdef _DEBUG_
        Serial.print("ESC Received: ");
        Serial.print(RX_Size);
        Serial.print(" Bytes: ");
        Serial.print(RX_Buf[0], HEX);
#endif
        if (RX_Buf[0] == brSUCCESS) {
            //buf[6] = ACK_OK;    // ACK
        }
        else {
            ack_out = ACK_D_GENERAL_ERROR;
            buf[6] = ACK_D_GENERAL_ERROR;    // ACK
        }
    }
    else if (cmd == cmd_DeviceWrite)
    {
#ifdef _DEBUG_
        Serial.print("DeviceWrite ");
#endif
        O_Param_Len = 0x01;
        // Send CMD Adress
        uint8_t ESC_data[4] = {CMD_SET_ADDRESS, 0, addr_high, addr_low};
        uint16_t Data_Size = 4;
        uint16_t RX_Size = 0;
        uint8_t RX_Buf[250] = {0};
        RX_Size = nrfbhpt_sendToEsc(ESC_data, Data_Size);
        delay(50);
        RX_Size = nrfbhpt_getFromEsc(RX_Buf, 100);
#ifdef _DEBUG_
        Serial.print("ESC Received: ");
        Serial.print(RX_Size);
        Serial.print(" Bytes: ");
        Serial.print(RX_Buf[0], HEX);
#endif
        if (RX_Buf[0] == brSUCCESS) {

        }
        else {
            ack_out = ACK_D_GENERAL_ERROR;
        }
        // sende Buffer init
        ESC_data[0] = CMD_SET_BUFFER;
        ESC_data[1] = 0x00;
        ESC_data[2] = 0x00;
        ESC_data[3] = I_Param_Len;
        Data_Size = 4;
        RX_Size = 0;
        if (I_Param_Len == 0) {
            ESC_data[2] = 0x01;
        }
        RX_Size = nrfbhpt_sendToEsc(ESC_data, Data_Size);
        delay(5);
        // Keine Anwort vom ESC

        // sende Buffer data
        RX_Size = nrfbhpt_sendToEsc(ParamBuf, I_Param_Len);
        delay(5);
        RX_Size = nrfbhpt_getFromEsc(RX_Buf, 200);
#ifdef _DEBUG_
        Serial.print("ESC Received: ");
        Serial.print(RX_Size);
        Serial.print(" Bytes: ");
        Serial.print(RX_Buf[0], HEX);
#endif
        if (RX_Buf[0] == brSUCCESS) {

        }
        else {
            ack_out = ACK_D_GENERAL_ERROR;
        }

        // sende write CMD
        ESC_data[0] = CMD_PROG_FLASH;
        ESC_data[1] = 0x01;
        Data_Size = 2;
        RX_Size = 0;
        RX_Size = nrfbhpt_sendToEsc(ESC_data, Data_Size);
        delay(30);
        // brSUCCESS wird nicht sofort gesendet
        RX_Size = nrfbhpt_getFromEsc(RX_Buf, 100);
#ifdef _DEBUG_
        Serial.print("ESC Received: ");
        Serial.print(RX_Size);
        Serial.print(" Bytes: ");
        Serial.print(RX_Buf[0], HEX);
#endif
        if (RX_Buf[0] == brSUCCESS) {
            //buf[6] = ACK_OK;    // ACK
        }
        else {
            ack_out = ACK_D_GENERAL_ERROR;
            //buf[6] = ACK_D_GENERAL_ERROR;    // ACK
        }
    }

    else {
        buf_size = 0;
#ifdef _DEBUG2_
        Serial.print("else command: ");
        Serial.print(cmd, HEX);
#endif
    }

    crc = 0;
    buf[0] = cmd_Remote_Escape;
    buf[1] = cmd;
    buf[2] = addr_high;
    buf[3] = addr_low;
    buf[4] = O_Param_Len & 0xff;        // Output Param Lenght
    buf[O_Param_Len + 5] = ack_out;
    // CRC
    for (uint16_t i = 0; i < (O_Param_Len + 6); i++) {
        crc = _crc_xmodem_update (crc, buf[i]);
    }
    buf[O_Param_Len + 6] = (crc >> 8) & 0xff;
    buf[O_Param_Len + 7] = crc & 0xff;
#ifdef _DEBUG_
    Serial.print(" with CRC: 0x");
    Serial.print(buf[O_Param_Len + 6], HEX);
    Serial.print(" ");
    Serial.print(buf[O_Param_Len + 7], HEX);
    Serial.print(" ");
#endif
    buf_size = (O_Param_Len + 8);

    return buf_size;
}

uint8_t MSP_Check(uint8_t MSP_buf[], uint8_t buf_size)
{
    // For BlHeli App Communication, there are just MSP Request's
    // http://www.stefanocottafavi.com/msp-the-multiwii-serial-protocol/
    // Checksum is 8 bit XOR of Size,type, and Payload

    uint8_t MSP_OSize = 0;
    uint8_t MSP_type = MSP_buf[4];
    uint8_t MSP_crc = MSP_buf[buf_size-1];    // For Request (buf_size-1) is always 5
    uint8_t crc;

    if(MSP_type == MSP_API_VERSION && MSP_crc == 0x01) // MSP_API_VERSION
    {
#ifdef _DEBUG_
         Serial.print("MSP_API_VERSION ");
#endif
         MSP_buf[2]= 0x3E;    // Response Header
         MSP_buf[3]= 0x03;    // Size
         MSP_buf[4]= MSP_API_VERSION;
         MSP_buf[5]= MSP_PROTOCOL_VERSION;
         MSP_buf[6]= API_VERSION_MAJOR;
         MSP_buf[7]= API_VERSION_MINOR;
         MSP_OSize = 8;
    }
    else if(MSP_type == MSP_FC_VARIANT && MSP_crc == 0x02) // MSP_FC_VARIANT
    {
#ifdef _DEBUG_
         Serial.print("MSP_FC_VARIANT ");
#endif
         MSP_buf[2]= 0x3E;    // Response Header
         MSP_buf[3]= 0x04;    // Size
         MSP_buf[4]= MSP_FC_VARIANT;
         MSP_buf[5]= 0x42;     //BETAFLIGHT_IDENTIFIER "B"
         MSP_buf[6]= 0x54;     //BETAFLIGHT_IDENTIFIER "T"
         MSP_buf[7]= 0x46;     //BETAFLIGHT_IDENTIFIER "F"
         MSP_buf[8]= 0x4C;     //BETAFLIGHT_IDENTIFIER "L"
         MSP_OSize = 9;
    }

    else if(MSP_type == MSP_FC_VERSION && MSP_crc == 0x03){   // MSP_FC_VERSION
#ifdef _DEBUG_
         Serial.print("MSP_FC_VERSION ");
#endif
         MSP_buf[2]= 0x3E;    // Response Header
         MSP_buf[3]= 0x03;    // Size
         MSP_buf[4]= MSP_FC_VERSION;
         MSP_buf[5]= FC_VERSION_MAJOR;
         MSP_buf[6]= FC_VERSION_MINOR;
         MSP_buf[7]= FC_VERSION_PATCH_LEVEL;
         MSP_OSize = 8;
    }
    else if(MSP_type == MSP_BOARD_INFO && MSP_crc == 0x04) // MSP_BOARD_INFO
    {
#ifdef _DEBUG_
         Serial.print("MSP_BOARD_INFO ");
#endif
         MSP_buf[2]= 0x3E;    // Response Header
         MSP_buf[3]= 0x4F;    // Size
         MSP_buf[4]= MSP_BOARD_INFO;
         MSP_buf[5]= 'B';
         MSP_buf[6]= 'P';
         MSP_buf[7]= 'M';
         MSP_buf[8]= 'C';
         MSP_buf[9]= 0x00;    // HW Revision
         MSP_buf[10]= 0x00;   // HW Revision
         MSP_buf[11]= 0x00;   // FC w/wo MAX7456
         MSP_buf[12]= 0x00;   // targetCapabilities
         MSP_buf[13]= 0x14;   // Name String length
         MSP_buf[14]= 'B';    // Name.....
         MSP_buf[15]= 'r';
         MSP_buf[16]= 'u';
         MSP_buf[17]= 's';
         MSP_buf[18]= 'h';
         MSP_buf[19]= 'l';
         MSP_buf[20]= 'e';
         MSP_buf[21]= 's';
         MSP_buf[22]= 's';
         MSP_buf[23]= 'P';
         MSP_buf[24]= 'o';
         MSP_buf[25]= 'w';
         MSP_buf[26]= 'e';
         MSP_buf[27]= 'r';
         MSP_buf[28]= ' ';
         MSP_buf[29]= 'M';
         MSP_buf[30]= 'C';
         MSP_buf[31]= 'T';
         MSP_buf[32]= 'R';
         MSP_buf[33]= 'L';
         MSP_buf[34]= 0x00;
         MSP_buf[35]= 0x00;
         MSP_buf[36]= 0x00;
         MSP_buf[37]= 0x00;
         MSP_buf[38]= 0x00;
         MSP_buf[39]= 0x00;
         MSP_buf[40]= 0x00;
         MSP_buf[41]= 0x00;
         MSP_buf[42]= 0x00;
         MSP_buf[43]= 0x00;
         MSP_buf[44]= 0x00;
         MSP_buf[45]= 0x00;
         MSP_buf[46]= 0x00;
         MSP_buf[47]= 0x00;
         MSP_buf[48]= 0x00;
         MSP_buf[49]= 0x00;
         MSP_buf[50]= 0x00;
         MSP_buf[51]= 0x00;
         MSP_buf[52]= 0x00;
         MSP_buf[53]= 0x00;
         MSP_buf[54]= 0x00;
         MSP_buf[55]= 0x00;
         MSP_buf[56]= 0x00;
         MSP_buf[57]= 0x00;
         MSP_buf[58]= 0x00;
         MSP_buf[59]= 0x00;
         MSP_buf[60]= 0x00;
         MSP_buf[61]= 0x00;
         MSP_buf[62]= 0x00;
         MSP_buf[63]= 0x00;
         MSP_buf[64]= 0x00;
         MSP_buf[65]= 0x00;
         MSP_buf[66]= 0x00;
         MSP_buf[67]= 0x00;
         MSP_buf[68]= 0x00;
         MSP_buf[69]= 0x00;
         MSP_buf[70]= 0x00;
         MSP_buf[71]= 0x00;
         MSP_buf[72]= 0x00;
         MSP_buf[73]= 0x00;
         MSP_buf[74]= 0x00;   // getMcuTypeId
         MSP_buf[75]= 0x00;   // configurationState
         MSP_buf[76]= 0x00;   // Gyro
         MSP_buf[77]= 0x00;   // Gyro
         MSP_buf[78]= 0x00;   // configurationProblems
         MSP_buf[79]= 0x00;   // configurationProblems
         MSP_buf[80]= 0x00;   // configurationProblems
         MSP_buf[81]= 0x00;   // configurationProblems
         MSP_buf[82]= 0x00;   // SPI
         MSP_buf[83]= 0x00;   // I2C
         MSP_OSize = 84;
    }
    else if(MSP_type == MSP_BUILD_INFO && MSP_crc == 0x05) // MSP_BUILD_INFO
    {
#ifdef _DEBUG_
         Serial.print("MSP_BUILD_INFO ");
#endif
         MSP_buf[2]= 0x3E;    // Response Header
         MSP_buf[3]= 0x1A;    // Size
         MSP_buf[4]= MSP_BUILD_INFO;
         MSP_buf[5]= 0x00;
         MSP_buf[6]= 0x00;
         MSP_buf[7]= 0x00;
         MSP_buf[8]= 0x00;
         MSP_buf[9]= 0x00;
         MSP_buf[10]= 0x00;
         MSP_buf[11]= 0x00;
         MSP_buf[12]= 0x00;
         MSP_buf[13]= 0x00;
         MSP_buf[14]= 0x00;
         MSP_buf[15]= 0x00;
         MSP_buf[16]= 0x00;
         MSP_buf[17]= 0x00;
         MSP_buf[18]= 0x00;
         MSP_buf[19]= 0x00;
         MSP_buf[20]= 0x00;
         MSP_buf[21]= 0x00;
         MSP_buf[22]= 0x00;
         MSP_buf[23]= 0x00;
         MSP_buf[24]= 0x00;
         MSP_buf[25]= 0x00;
         MSP_buf[26]= 0x00;
         MSP_buf[27]= 0x00;
         MSP_buf[28]= 0x00;
         MSP_buf[29]= 0x00;
         MSP_buf[30]= 0x00;
         MSP_OSize = 31;
    }
    else if(MSP_type == MSP_STATUS && MSP_crc == 0x65) // MSP_STATUS
    {
#ifdef _DEBUG_
         Serial.print("MSP_STATUS ");
#endif
         MSP_buf[2]= 0x3E;    // Response Header
         MSP_buf[3]= 0x16;    // Size
         MSP_buf[4]= MSP_STATUS;
         MSP_buf[5]= 0x00;//0xFA;   //getTaskDeltaTime(TASK_GYROPID)
         MSP_buf[6]= 0x00;    // getTaskDeltaTime(TASK_GYROPID)
         MSP_buf[7]= 0x00;    // i2cGetErrorCounter()
         MSP_buf[8]= 0x00;    // i2cGetErrorCounter()
         MSP_buf[9]= 0x00;//0x23;    // sensors(SENSOR_ACC) | sensors(SENSOR_BARO) << 1 | sensors(SENSOR_MAG) << 2 | sensors(SENSOR_GPS) << 3 | sensors(SENSOR_RANGEFINDER) << 4 | sensors(SENSOR_GYRO) << 5
         MSP_buf[10]= 0x00;   // sensors(SENSOR_ACC) | sensors(SENSOR_BARO) << 1 | sensors(SENSOR_MAG) << 2 | sensors(SENSOR_GPS) << 3 | sensors(SENSOR_RANGEFINDER) << 4 | sensors(SENSOR_GYRO) << 5
         MSP_buf[11]= 0x00;//0x02;   // flightModeFlags
         MSP_buf[12]= 0x00;   // flightModeFlags
         MSP_buf[13]= 0x00;   // flightModeFlags
         MSP_buf[14]= 0x00;   // flightModeFlags
         MSP_buf[15]= 0x00;   // getCurrentPidProfileIndex()
         MSP_buf[16]= 0x00;//0x02;   // constrain(averageSystemLoadPercent, 0, 100)
         MSP_buf[17]= 0x00;   // constrain(averageSystemLoadPercent, 0, 100)
         MSP_buf[18]= 0x00;   // PID_PROFILE_COUNT
         MSP_buf[19]= 0x00;   // getCurrentControlRateProfileIndex()
         MSP_buf[20]= 0x00;   // byteCount -> flightModeFlags
         MSP_buf[21]= 0x00;//0x18;   // ARMING_DISABLE_FLAGS_COUNT
         MSP_buf[22]= 0x00;//0x04;   // armingDisableFlags
         MSP_buf[23]= 0x00;//0x01;   // armingDisableFlags
         MSP_buf[24]= 0x00,//0x10;   // armingDisableFlags
         MSP_buf[25]= 0x00;   // armingDisableFlags
         MSP_buf[26]= 0x00;   // getRebootRequired()
         MSP_OSize = 27;
    }
    else if(MSP_type == MSP_MOTOR_3D_CONFIG && MSP_crc == 0x7C) // MSP_3D
    {
#ifdef _DEBUG_
         Serial.print("MSP_3D ");
#endif
         MSP_buf[2]= 0x3E;
         MSP_buf[3]= 0x06;
         MSP_buf[4]= MSP_MOTOR_3D_CONFIG;
         MSP_buf[5]= 0x00;//0x7E;   // flight3DConfig()->deadband3d_low
         MSP_buf[6]= 0x00;//0x05;   // flight3DConfig()->deadband3d_low
         MSP_buf[7]= 0x00;//0xEA;   // flight3DConfig()->deadband3d_high
         MSP_buf[8]= 0x00;//0x05;   // flight3DConfig()->deadband3d_high
         MSP_buf[9]= 0x00;//0xB4;   // flight3DConfig()->neutral3d
         MSP_buf[10]= 0x00;//0x05;    // flight3DConfig()->neutral3d
         MSP_OSize = 11;
    }
    else if(MSP_type == MSP_MOTOR_CONFIG && MSP_crc == 0x83) // MSP_MOTOR_CONFIG
    {
#ifdef _DEBUG_
         Serial.print("MSP_MOTOR_CONFIG ");
#endif
         MSP_buf[2]= 0x3E;
         MSP_buf[3]= 0x0A;
         MSP_buf[4]= MSP_MOTOR_CONFIG;
         MSP_buf[5]= 0x2E;    // motorConfig()->minthrottle -> 1070
         MSP_buf[6]= 0x04;    // motorConfig()->minthrottle -> 1070
         MSP_buf[7]= 0xD0;    // motorConfig()->maxthrottle -> 2000
         MSP_buf[8]= 0x07;    // motorConfig()->maxthrottle -> 2000
         MSP_buf[9]= 0xE8;    // motorConfig()->mincommand -> 1000
         MSP_buf[10]= 0x03;   // motorConfig()->mincommand -> 1000
         MSP_buf[11]= 0x01;//0x04;   // getMotorCount()
         MSP_buf[12]= 0x00;//0x10;   // motorConfig()->motorPoleCount
         MSP_buf[13]= 0x00;//0x01;   // motorConfig()->dev.useDshotTelemetry
         MSP_buf[14]= 0x00;   // featureIsEnabled(FEATURE_ESC_SENSOR)
         MSP_OSize = 15;
    }
    else if(MSP_type == MSP_MOTOR && MSP_crc == 0x68)// MSP_MOTOR
    {
#ifdef _DEBUG_
         Serial.print("MSP_MOTOR ");
#endif
         MSP_buf[2]= 0x3E;
         MSP_buf[3]= 0x10;
         MSP_buf[4]= MSP_MOTOR;
         MSP_buf[5]= 0xE8;    // motorConvertToExternal(motor[i]) -> 1000
         MSP_buf[6]= 0x03;    // motorConvertToExternal(motor[i]) -> 1000
         MSP_buf[7]= 0x00;//0xE8;
         MSP_buf[8]= 0x00;//0x03;
         MSP_buf[9]= 0x00;//0xE8;
         MSP_buf[10]= 0x00;//0x03;
         MSP_buf[11]= 0x00;//0xE8;
         MSP_buf[12]= 0x00;//0x03;
         MSP_buf[13]= 0x00;
         MSP_buf[14]= 0x00;
         MSP_buf[15]= 0x00;
         MSP_buf[16]= 0x00;
         MSP_buf[17]= 0x00;
         MSP_buf[18]= 0x00;
         MSP_buf[19]= 0x00;
         MSP_buf[20]= 0x00;
         MSP_OSize = 21;
    }
    else if(MSP_type == MSP_FEATURE_CONFIG && MSP_crc == 0x24)// MSP_FEATURE_CONFIG
    {
#ifdef _DEBUG_
         Serial.print("MSP_FEATURE_CONFIG ");
#endif
         MSP_buf[2]= 0x3E;
         MSP_buf[3]= 0x04;
         MSP_buf[4]= MSP_FEATURE_CONFIG;
         MSP_buf[5]= 0x00;//0x08;    // getFeatureMask()
         MSP_buf[6]= 0x00;//0x04;    // getFeatureMask()
         MSP_buf[7]= 0x00;//0x00;    // getFeatureMask()
         MSP_buf[8]= 0x00;//0x30;    // getFeatureMask()
         MSP_OSize = 9;
    }
    else if(MSP_type == MSP_BOXIDS && MSP_crc == 0x77) // MSP_BOXIDS
    {
#ifdef _DEBUG_
         Serial.print("MSP_BOXIDS ");
#endif
         MSP_buf[2]= 0x3E;
         MSP_buf[3]= 0x18;
         MSP_buf[4]= MSP_BOXIDS;
         MSP_buf[5]= 0x00;
         MSP_buf[6]= 0x00;
         MSP_buf[7]= 0x00;
         MSP_buf[8]= 0x00;
         MSP_buf[9]= 0x00;
         MSP_buf[10]= 0x00;
         MSP_buf[11]= 0x00;
         MSP_buf[12]= 0x00;
         MSP_buf[13]= 0x00;
         MSP_buf[14]= 0x00;
         MSP_buf[15]= 0x00;
         MSP_buf[16]= 0x00;
         MSP_buf[17]= 0x00;
         MSP_buf[18]= 0x00;
         MSP_buf[19]= 0x00;
         MSP_buf[20]= 0x00;
         MSP_buf[21]= 0x00;
         MSP_buf[22]= 0x00;
         MSP_buf[23]= 0x00;
         MSP_buf[24]= 0x00;
         MSP_buf[25]= 0x00;
         MSP_buf[26]= 0x00;
         MSP_buf[27]= 0x00;
         MSP_buf[28]= 0x00;
         MSP_OSize = 29;
    }
    else if(MSP_type == MSP_SET_4WAY_IF && MSP_crc == 0xF5) // MSP_4wayInit
    {
#ifdef _DEBUG_
         Serial.print("MSP_SET_4WAY_IF ");
#endif
         MSP_buf[2]= 0x3E;
         MSP_buf[3]= 0x01;
         MSP_buf[4]= MSP_SET_4WAY_IF;
         MSP_buf[5]= 0x01;//0x04;    // get channel number, switch all motor lines HI, reply with the count of ESC found
         InitSerialOutput();
         MSP_OSize = 6;
    }
    else if(MSP_type == MSP_ADVANCED_CONFIG && MSP_crc == 0x5A) // MSP_ADVACED_CONFIG
    {
#ifdef _DEBUG_
         Serial.print("MSP_ADVACED_CONFIG ");
#endif
         MSP_buf[2]= 0x3E;
         MSP_buf[3]= 0x14;
         MSP_buf[4]= MSP_ADVANCED_CONFIG;
         MSP_buf[5]= 0x00;//0x02;    // gyroConfig()->gyro_sync_denom
         MSP_buf[6]= 0x00;//0x01;    // pidConfig()->pid_process_denom
         MSP_buf[7]= 0x00;    // motorConfig()->dev.useUnsyncedPwm
         MSP_buf[8]= 0x06;    // motorConfig()->dev.motorPwmProtocol -> 0x06 = Dshot300 / 0x00 = Servo / 0x07 = Dshot600
         MSP_buf[9]= 0xE0;    // motorConfig()->dev.motorPwmRate -> 480 -> DShot alle 2.08ms
         MSP_buf[10]= 0x01;    // motorConfig()->dev.motorPwmRate -> 480 -> DShot alle 2.08ms
         MSP_buf[11]= 0x00;//0x90;    // motorConfig()->digitalIdleOffsetValue
         MSP_buf[12]= 0x00;//0x01;    // motorConfig()->digitalIdleOffsetValue
         MSP_buf[13]= 0x00;    // DEPRECATED: gyro_use_32kHz
         MSP_buf[14]= 0x00;    // motorConfig()->dev.motorPwmInversion
         MSP_buf[15]= 0x00;//0x02;    // gyroConfig()->gyro_to_use
         MSP_buf[16]= 0x00;//0x00;    // gyroConfig()->gyro_high_fsr
         MSP_buf[17]= 0x00;//0x30;    // gyroConfig()->gyroMovementCalibrationThreshold
         MSP_buf[18]= 0x00;//0x7D;    // gyroConfig()->gyroCalibrationDuration
         MSP_buf[19]= 0x00;//0x00;    // gyroConfig()->gyroCalibrationDuration
         MSP_buf[20]= 0x00;    // gyroConfig()->gyro_offset_yaw
         MSP_buf[21]= 0x00;    // gyroConfig()->gyro_offset_yaw
         MSP_buf[22]= 0x00;//0x02;    // gyroConfig()->checkOverflow
         MSP_buf[23]= 0x00;//0x06;    // systemConfig()->debug_mode
         MSP_buf[24]= 0x00;//0x3C;    // DEBUG_COUNT
         MSP_OSize = 25;
    }
    else if(MSP_type == MSP_UID && MSP_crc == 0xA0) // MSP_UID
    {
#ifdef _DEBUG_
         Serial.print("MSP_UID ");
#endif
         MSP_buf[2]= 0x3E;
         MSP_buf[3]= 0x0C;
         MSP_buf[4]= MSP_UID;
         MSP_buf[5]= 0x00;
         MSP_buf[6]= 0x00;
         MSP_buf[7]= 0x00;
         MSP_buf[8]= 0x00;
         MSP_buf[9]= 0x00;
         MSP_buf[10]= 0x00;
         MSP_buf[11]= 0x00;
         MSP_buf[12]= 0x00;
         MSP_buf[13]= 0x00;
         MSP_buf[14]= 0x00;
         MSP_buf[15]= 0x00;
         MSP_buf[16]= 0x00;
         MSP_OSize = 17;
    }

    crc = MSP_buf[3] ^ MSP_buf[4];
    for(uint8_t i = 5; i < MSP_OSize;i++) {
       crc = crc ^ MSP_buf[i];
    }
    MSP_buf[MSP_OSize]= crc;
#ifdef _DEBUG_
    Serial.print("with CRC: 0x");
    Serial.print(crc,HEX);
    Serial.print(" ");
#endif
    buf_size = MSP_OSize + 1;

    return buf_size;
}

uint16_t nrfbhpt_sendToEsc(uint8_t tx_buf[], uint16_t buf_size)
{
    return nrfbhpt_sendToEsc(tx_buf, buf_size, true); // defaults to CRC=true
}

uint16_t nrfbhpt_sendToEsc(uint8_t tx_buf[], uint16_t buf_size, bool CRC)
{
    uint16_t i = 0;
    uint16_t send_size = buf_size + (CRC ? 2 : 0);

    uint8_t* tmp_buf = (uint8_t*)malloc(send_size);
    if (tmp_buf == NULL) {
        return 0;
    }

    esc_crc = 0;
    if (buf_size == 0) {
        buf_size = 256;
    }
#ifdef _DEBUG_
    Serial.print("write ESC: ");
#endif
    //softSer->enableTx(true);
    for (i = 0; i < buf_size; i++)
    {
        uint8_t x = tx_buf[i];
        //softSer->write(x);
        tmp_buf[i] = x;
        esc_crc = _byteCrc(x, esc_crc);
    }
    if (CRC)
    {
        //softSer->write(esc_crc & 0xff);
        //softSer->write((esc_crc >> 8) & 0xff);
        tmp_buf[i] = esc_crc & 0xff;
        i++;
        tmp_buf[i] = (esc_crc >> 8) & 0xff;
        i++;
    }
    //softSer->enableTx(false);
    softSer->write((const uint8_t*)tmp_buf, i);
#ifdef _DEBUG_
    Serial.println("done");
#endif

    free(tmp_buf);

    return 0;
}

uint16_t nrfbhpt_getFromEsc(uint8_t rx_buf[], uint16_t wait_ms)
{
    uint32_t now = millis();
    uint32_t t_start = now;
    uint16_t i = 0;
    esc_crc = 0;
    bool timeout = false;
#ifdef _DEBUG_
    Serial.print("ESC Read: ");
#endif
    while (softSer->available() <= 0 && ((now = millis()) - t_start) < wait_ms)
    {
        yield();
    }
    if (softSer->available() <= 0) {
#ifdef _DEBUG_
        Serial.println("timeout");
#endif
        return 0;
    }
    i = 0;
    t_start = millis();
    while (true)
    {
        while (softSer->available() > 0)
        {
            rx_buf[i] = softSer->read();
#ifdef _DEBUG_
            Serial.print(rx_buf[i], HEX);
            Serial.print(" ");
#endif
            i++;
            t_start = millis();
        }
        yield();
        if ((millis() - t_start) > 2) {
            break;
        }
    }

#ifdef _DEBUG_
    Serial.println("done");
#endif
    return i;
}

static uint16_t _byteCrc(uint8_t data, uint16_t crc)
{
    uint8_t xb = data;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (((xb & 0x01) ^ (crc & 0x0001)) != 0 ) {
            crc = crc >> 1;
            crc = crc ^ 0xA001;
        } else {
            crc = crc >> 1;
        }
        xb = xb >> 1;
    }
    return crc;
}

static uint16_t _crc_xmodem_update(uint16_t crc, uint8_t data)
{
    int i;

    crc = crc ^ ((uint16_t)data << 8);
    for (i = 0; i < 8; i++)
    {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ 0x1021;
        }
        else {
            crc <<= 1;
        }
    }
    return crc;
}
