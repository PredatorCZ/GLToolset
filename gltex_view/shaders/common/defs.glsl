#ifdef FRAGMENT
#define INTERFACE in
#endif

#ifdef VERTEX
#define INTERFACE out
#endif

#define CPPATTR(a1)
#define CPPATTR2(a1, a2)
#define CPPATTR3(a1, a2, a3)

#define dquat_arr(name, numItems) vec4 name[numItems * 2]
#define dquat(name) vec4 name[2]
