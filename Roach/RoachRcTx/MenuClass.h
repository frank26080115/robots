#ifndef _MENUCLASS_H_
#define _MENUCLASS_H_

#include <Arduino.h>

class RoachMenu
{
    public:
        RoachMenu(uint8_t id = 0);
        virtual void run(void);
        inline int getExitCode(void) { return _exit; };
        void* prev_menu = NULL;
        void* next_menu = NULL;
        void* parent_menu = NULL;
    protected:
        uint8_t  _id;
        int      _exit = 0;
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


class RoachMenuFunctionItem : RoachMenuListItem, RoachMenu
{
    public:
        RoachMenuFunctionItem(const char* fname);
};

class RoachMenuFileItem : RoachMenuListItem
{
    public:
        RoachMenuFileItem(const char* fname);
        ~RoachMenuFileItem(void);
        void sprintFName(char* s);
        char* getFName(void);
    protected:
        char* _fname = NULL;
};

class RoachMenuCfgItem : RoachMenuListItem
{
    public:
        RoachMenuCfgItem(void* struct_ptr, roach_nvm_gui_desc_t* desc);
        virtual void sprintName(char* s);
        virtual char* getName(void);
    protected:
        roach_nvm_gui_desc_t* _desc = NULL;
        void* _struct = NULL;
};

class RoachMenuCfgItemEditor : RoachMenu
{
    public:
        RoachMenuCfgItemEditor(void* struct_ptr, roach_nvm_gui_desc_t* desc);
    protected:
        roach_nvm_gui_desc_t* _desc = NULL;
        void* _struct = NULL;
        virtual void onEnter(void);
        virtual void onExit(void);
        virtual void onButton(uint8_t btn);
        virtual void checkButtons(void);
};

class RoachMenuLister : RoachMenu
{
    public:
        RoachMenuLister(uint8_t id);
        virtual void draw(void);
    protected:
        virtual void onExit(void);
        virtual RoachMenuListItem* getItem(int idx);
        inline  RoachMenuListItem* getCurItem(void) { return getItem(_list_idx); };
        virtual char* getItemText(int idx);
        inline  char* getCurItemText(void) { return getItemText(_list_idx); };
        uint8_t _list_cnt, _list_idx;
        RoachMenuListItem* _head_node = NULL, _tail_node = NULL;
        virtual void onEnter(void);
        void buildFileList(const char* filter);
        RoachMenuListItem* getNodeAt(int idx);
        void addNode(RoachMenuListItem*);
        virtual void onButton(uint8_t btn);

};

class RoachMenuFileOpenList : RoachMenuLister
{
    public:
        RoachMenuFileOpenList(uint8_t id);
    protected:
        virtual void draw_sidebar(void);
        virtual void draw_title(void);
        virtual RoachMenuListItem* getItem(int idx);
        virtual void onEnter(void);
        virtual void onButton(uint8_t btn);
};


class RoachMenuFileSaveList : RoachMenuLister
{
    public:
        RoachMenuFileSaveList(const char* filter);
    protected:
        char _filter[16];
        char _newfilename[32];
        virtual void draw_sidebar(void);
        virtual void draw_title(void);
        virtual RoachMenuListItem* getItem(int idx);
        virtual void onEnter(void);
        virtual void onButton(uint8_t btn);;
};

class RoachMenuCfgLister : RoachMenuLister
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
