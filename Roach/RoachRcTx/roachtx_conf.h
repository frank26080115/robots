#ifndef _ROACHTX_CONF_H_
#define _ROACHTX_CONF_H_

#define ROACH_WDT_TIMEOUT_MS    200

#define SCREEN_WIDTH  SSD1306_LCDWIDTH
#define SCREEN_HEIGHT SSD1306_LCDHEIGHT

#define ROACH_STARTUP_CONF_NAME     "startup.txt"
#define ROACH_STARTUP_DESC_NAME     "startup_desc.bin"
#define ROACHGUI_ERROR_SHOW_TIME    3000
#define ROACHGUI_LINE_HEIGHT        8
#define ROACHMENU_LIST_MAX          6
#define QUICKACTION_HOLD_TIME       2000
#define NRF5RAND_BUFF_SIZE          512
#define ROACHTX_AUTOSAVE
#define ROACHTX_AUTOEXIT

#include "roachtx_conf_hw.h"

#endif
