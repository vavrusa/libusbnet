#include "common.h"

#ifdef DEBUG
int sLogLevel = MsgError|MsgLog|MsgDebug;
#else
int sLogLevel = MsgError|MsgLog;
#endif

int log_level()
{
    return sLogLevel;
}

int log_setlevel(int flags)
{
    int ret = sLogLevel;
    sLogLevel = flags;
    return ret;
}
