#ifndef _ROACHCMDLINE_H_
#define _ROACHCMDLINE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <Arduino.h>
#include <Stream.h>

typedef void (*cmd_handler_t)(void*, char*, Stream*);

typedef struct
{
    const char cmd_header[64];
    cmd_handler_t handler_callback;
}
cmd_def_t;

class RoachCmdLine
{
    public:
        RoachCmdLine(Stream* stream_obj, cmd_def_t* user_cmd_list, bool local_echo, const char* prompt, const char* unknown_reply, bool higher_priori, uint32_t buffer_size);
        void print_prompt(void);
        int task(void);
        inline void set_echo(bool x) { this->_echo = x; };
        void sideinput_writec(char c);
        void sideinput_writes(const char* s);
        void sideinput_writeb(const char* s, uint32_t len);
        inline bool has_interaction(void) { return _has_interaction; };
    protected:
        Stream* _stream;
        cmd_def_t* _cmd_list;
        bool _echo;
        char* _prompt;
        char* _unknown_reply;
        uint8_t* _buffer;
        uint32_t _buffer_size;
        uint32_t _buff_idx;
        char _prev_char;
        bool _higher_priori;
        uint8_t* _side_fifo;
        uint32_t _sidefifo_w, _sidefifo_r, _sidefifo_s;
        bool _has_interaction;
};

#endif