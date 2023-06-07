#include "RoachRobotNvm.h"

static roach_nvm_gui_desc_t* robot_nvm_desc;
static void* robot_nvm_ptr;
static uint32_t desc_size = 0;

bool robotnvm_init(roach_nvm_gui_desc_t* desc, void* nvm_ptr)
{
    robot_nvm_ptr = ptr;
    robot_nvm_desc = desc;
    int i;
    for (i = 0; i < 0xFFFF; i++)
    {
        roach_nvm_gui_desc_t* p = &(robot_nvm_desc[i]);
        if (p->name[0] == 0)
        {
            break;
        }
    }
    desc_size = i;

    robotnvm_genDescFile(RONVM_DEFAULT_DESC_FILENAME, robot_nvm_desc, false);
    if (robotnvm_loadFromFile(RONVM_DEFAULT_CFG_FILENAME))
    {
        robotnvm_setDefaults();
        robotnvm_saveToFile(RONVM_DEFAULT_CFG_FILENAME);
    }
}

bool robotnvm_genDescFile(const char* fname, roach_nvm_gui_desc_t* desc_tbl, bool force)
{
    RoachFile f;
    bool suc;
    suc = f.open(fname, O_RDONLY);
    if (suc && force == false)
    {
        return true;
    }
    suc = f.open(fname, O_RDWR | O_CREAT | O_TRUNC);
    if (suc == false)
    {
        return false;
    }
    f.seek(0);
    uint8_t* data_ptr = (uint8_t*)desc_tbl;
    int i;
    for (i = 0; i < 0xFFFF; i++)
    {
        roach_nvm_gui_desc_t* p = &(desc_tbl[i]);
        if (p->name[0] == 0)
        {
            break;
        }
    }
    uint32_t data_sz = i * sizeof(roach_nvm_gui_desc_t);
    f.write(data_ptr, data_sz);
    f.close();
    return true;
}

void robotnvm_setDefaults(void)
{
    roachnvm_setdefaults((uint8_t*)robot_nvm_ptr, robot_nvm_desc);
}

bool robotnvm_saveToFile(const char* fname)
{
    RoachFile f;
    bool suc;
    suc = f.open(fname, O_RDWR | O_CREAT | O_TRUNC);
    if (suc == false)
    {
        return false;
    }
    f.seek(0);
    roachnvm_writetofile(&f, (uint8_t*)robot_nvm_ptr, robot_nvm_desc);
    f.close();
    return true;
}

bool robotnvm_loadFromFile(const char* fname)
{
    RoachFile f;
    bool suc;
    suc = f.open(fname, O_RDONLY);
    if (suc == false)
    {
        return false;
    }
    f.seek(0);
    roachnvm_readfromfile(&f, (uint8_t*)robot_nvm_ptr, robot_nvm_desc);
    f.close();
    return true;
}
