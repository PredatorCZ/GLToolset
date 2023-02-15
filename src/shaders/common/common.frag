
#if defined(TS_NORMAL_ATTR) || defined(TS_TANGENT_ATTR)
#include "ts_normal.frag"
#endif

#ifdef NUM_LIGHTS
#include "light_omni.frag"
#endif
