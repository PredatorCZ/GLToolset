#ifdef FRAGMENT
#define INTERFACE in
#endif

#ifdef VERTEX
#define INTERFACE out
#endif

#define CPPATTR(...)

#define dquat_arr(name, numItems) vec4 name[numItems * 2]
#define dquat(name) vec4 name[2]
