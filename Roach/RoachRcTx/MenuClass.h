#ifndef _MENUCLASS_H_
#define _MENUCLASS_H_

#include <Arduino.h>

#include <RoachLib.h>
#include <RoachPot.h>
#include <RoachButton.h>
#include <RoachRotaryEncoder.h>

#include "gui_symbols.h"

enum
{
    MENUID_NONE,
    MENUID_HOME,                  // display input visualizations
    //MENUID_STATS,                 // display extra live data
    MENUID_INFO,                  // display stuff like profile, UID, version
    //MENUID_RAW,                   // display raw input data
    MENUID_CONFIG_CALIBSYNC,      // options for calibration and synchronization
    MENUID_CONFIG_FILELOAD,       // choose to load a file
    MENUID_CONFIG_ROBOT,
    MENUID_CONFIG_CTRLER,         // edit parameters about the controller
};

enum
{
    EXITCODE_NONE,
    EXITCODE_LEFT,
    EXITCODE_RIGHT,
    EXITCODE_UP,
    EXITCODE_DOWN,
    EXITCODE_BACK,
    EXITCODE_HOME,
    EXITCODE_UNABLE,
};

enum
{
    BTNID_NONE,
    BTNID_LEFT,
    BTNID_RIGHT,
    BTNID_UP,
    BTNID_DOWN,
    BTNID_CENTER,
    BTNID_G5,
    BTNID_G6,
};

extern uint32_t gui_last_activity_time;

class RoachMenu
{
    public:
        RoachMenu(uint8_t id = 0);
        virtual void run(void);
        inline int getExitCode(void) { return _exit; };
        inline uint8_t getId(void) { return _id; };
        inline void interrupt(int x) { _interrupt = x; } ;
        inline bool isRunning(void) { return _running; };
        void* prev_menu = NULL;
        void* next_menu = NULL;
        void* parent_menu = NULL;
    protected:
        uint8_t  _id;
        int      _exit = 0;
        volatile int _interrupt = 0;
        bool _running = false;
        virtual void draw(void);
        virtual void clear(void);
                void display(void);
        virtual void taskHP(void);
        virtual void taskLP(void);
        virtual void draw_sidebar(void);
        virtual void draw_title(void);
        virtual void onEnter(void);
        virtual void onExit(void);
        virtual void onButton(uint8_t btn);
        virtual void onButtonCheckExit(uint8_t btn);
        virtual void checkButtons(void);
};

class RoachMenuListItem
{
    public:
        ~RoachMenuListItem(void);
        virtual void sprintName(char* s);
        virtual char* getName(void);
        void* next_node = NULL;
    protected:
        char* _txt = NULL;
};


class RoachMenuFunctionItem : public RoachMenuListItem, public RoachMenu
{
    public:
        RoachMenuFunctionItem(const char* name);
};

class RoachMenuFileItem : public RoachMenuListItem
{
    public:
        RoachMenuFileItem(const char* fname);
        ~RoachMenuFileItem(void);
        void sprintFName(char* s);
        char* getFName(void);
    protected:
        char* _fname = NULL;
};

class RoachMenuCfgItem : public RoachMenuListItem
{
    public:
        RoachMenuCfgItem(void* struct_ptr, roach_nvm_gui_desc_t* desc);
        virtual void sprintName(char* s);
        virtual char* getName(void);
    protected:
        roach_nvm_gui_desc_t* _desc = NULL;
        void* _struct = NULL;
};

class RoachMenuCfgItemEditor : public RoachMenu
{
    public:
        RoachMenuCfgItemEditor(void* struct_ptr, roach_nvm_gui_desc_t* desc);
    protected:
        roach_nvm_gui_desc_t* _desc = NULL;
        void* _struct = NULL;
        virtual void draw(void);
        virtual void draw_sidebar(void);
        virtual void draw_title(void);
        virtual void taskLP(void);
        virtual void onEnter(void);
        virtual void onExit(void);
        virtual void onButton(uint8_t btn);
        virtual void checkButtons(void);
};

class RoachMenuLister : public RoachMenu
{
    public:
        RoachMenuLister(uint8_t id);
        virtual void draw(void);
    protected:
        virtual void onExit(void);
        //virtual RoachMenuListItem* getItem(int idx);
        //inline  RoachMenuListItem* getCurItem(void) { return getItem(_list_idx); };
        virtual char* getItemText(int idx);
        inline  char* getCurItemText(void) { return getItemText(_list_idx); };
        uint8_t _list_cnt, _list_idx;
        RoachMenuListItem* _head_node = NULL;
        RoachMenuListItem* _tail_node = NULL;
        bool _delete_on_exit = false;
        virtual void onEnter(void);
        void buildFileList(const char* filter);
        RoachMenuListItem* getNodeAt(int idx);
        void addNode(RoachMenuListItem*);
        virtual void onButton(uint8_t btn);

};

class RoachMenuFileOpenList : public RoachMenuLister
{
    public:
        RoachMenuFileOpenList(void);
    protected:
        virtual void draw_sidebar(void);
        virtual void draw_title(void);
        //virtual RoachMenuListItem* getItem(int idx);
        virtual void onEnter(void);
        virtual void onButton(uint8_t btn);
};


class RoachMenuFileSaveList : public RoachMenuLister
{
    public:
        RoachMenuFileSaveList(const char* filter);
    protected:
        char _filter[16];
        char _newfilename[32];
        virtual void draw_sidebar(void);
        virtual void draw_title(void);
        //virtual RoachMenuListItem* getItem(int idx);
        virtual void onEnter(void);
        virtual void onButton(uint8_t btn);
};

class RoachMenuCfgLister : public RoachMenuLister
{
    public:
        RoachMenuCfgLister(uint8_t id, const char* name, const char* filter, void* struct_ptr, roach_nvm_gui_desc_t* desc_tbl);
    protected:
        void* _struct;
        roach_nvm_gui_desc_t* _desc_tbl;
        RoachMenuCfgItem* _list = NULL;
        char _title[32];
        char _filter[16];
        virtual char* getItemText(int idx);
        virtual void draw_sidebar(void);
        virtual void draw_title(void);
        virtual void onButton(uint8_t btn);
};

#endif
