#include "RoachLib.h"

extern roach_rf_nvm_t nvm_rf;

roach_nvm_gui_desc_t cfggroup_rf[] = {
    { ((uint32_t)(&(nvm_rf.uid     )) - (uint32_t)(&nvm_rf)), "UID"     , "hex", (int32_t)0x1234ABCD, 0, 0, 1, },
    { ((uint32_t)(&(nvm_rf.salt    )) - (uint32_t)(&nvm_rf)), "salt"    , "hex", (int32_t)0xDEADBEEF, 0, 0, 1, },
    { ((uint32_t)(&(nvm_rf.chan_map)) - (uint32_t)(&nvm_rf)), "chan map", "hex", (int32_t)0x0FFFFFFF, 0, 0, 1, },
    ROACH_NVM_GUI_DESC_END,
};

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
