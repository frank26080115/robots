#include "RoachInversionMgr.h"

static float _hyster     = RINVMGR_HYSTER_DEFAULT;
static bool  _isinverted = false;
static bool  _haschange  = false;

bool rInvMgr_isInverted(euler* x)
{
    if (x == NULL) {
        return _isinverted;
    }

    if (_isinverted == false)
    {
        if (x->roll >= (90 + _hyster) || x->roll <= (-90 - _hyster) || x->pitch >= (90 + _hyster) || x->pitch <= (-90 - _hyster)) {
            _isinverted = true;
            _haschange = true;
        }
    }
    else // _isinverted == true
    {
        if (x->roll <= (90 - _hyster) || x->roll >= (-90 + _hyster) || x->pitch <= (90 - _hyster) || x->pitch >= (-90 + _hyster)) {
            _isinverted = false;
            _haschange = true;
        }
    }
    return _isinverted;
}

bool rInvMgr_hasChangedInversion(bool clr)
{
    bool ret = _haschange;
    if (clr) {
        _haschange = false;
    }
    return ret;
}

void rInvMgr_setHyster(float x)
{
    _hyster = x;
}
